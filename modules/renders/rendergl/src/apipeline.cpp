#include "apipeline.h"

#include <object.h>

#include "controller.h"

#include "filters/ablurgl.h"

#include "postprocess/aambientocclusiongl.h"
#include "postprocess/aantialiasinggl.h"
#include "postprocess/abloomgl.h"

#include <components/actor.h>
#include <components/scene.h>
#include <components/component.h>
#include <components/camera.h>

#include "components/adirectlightgl.h"

#include "analytics/profiler.h"

#include "resources/ameshgl.h"

#include <log.h>

APipeline::APipeline(Engine *engine) :
        m_pEngine(engine),
        m_pScene(nullptr),
        m_pController(nullptr),
        m_Buffer(new CommandBufferGL()),
        m_Screen(Vector2(64, 64)),
        m_World(Vector3()) {

    //m_pBlur     = new ABlurGL();
    //m_pAO       = new AAmbientOcclusionGL();

    //m_PostEffects.push_back(new AAntiAliasingGL());
    //m_PostEffects.push_back(new ABloomGL());

    m_Select.create     (GL_TEXTURE_2D, GL_RGBA8, GL_RGBA, GL_UNSIGNED_INT);
    m_Depth.create      (GL_TEXTURE_2D, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT);

    m_ShadowMap.create  (GL_TEXTURE_2D, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT);
    m_ShadowMap.resize  (2048, 2048);

    m_Targets["depthMap"]   = &m_Depth;
    m_Targets["shadowMap"]  = &m_ShadowMap;

    glGenFramebuffers(1, &m_SelectBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_SelectBuffer);
    m_Buffer->setRenderTarget(1, &m_Select, &m_Depth);
}

APipeline::~APipeline() {
    glDeleteFramebuffers(1, &m_SelectBuffer);
}

void APipeline::draw(Scene &scene, uint32_t) {
    PROFILER_RESET(POLYGONS);
    PROFILER_RESET(DRAWCALLS);

    m_pScene    = &scene;
    // Light prepass
    m_Buffer->setGlobalValue("light.ambient", m_pScene->ambient());

    glDepthFunc(GL_LEQUAL);
    glBindFramebuffer(GL_FRAMEBUFFER, m_ShadowMap.buffer());
    m_Buffer->setRenderTarget(0, nullptr, &m_ShadowMap);
    m_Buffer->clearRenderTarget();
    updateShadows(scene);

    m_Buffer->setViewport(0, 0, m_Screen.x, m_Screen.y);
    analizeScene(scene);
}

ATextureGL *APipeline::postProcess(ATextureGL &source) {
    ATextureGL *result  = &source;
    for(auto it : m_PostEffects) {
        result  = it->draw(*result, *m_Buffer);
    }
    return result;
}

void APipeline::cameraReset() {
    Camera *camera  = activeCamera();
    if(camera) {
        Matrix4 mv, p;
        camera->matrices(mv, p);
        camera->setRatio(m_Screen.x / m_Screen.y);
        m_Buffer->setGlobalValue("camera.position", Vector4(camera->actor().position(), camera->nearPlane()));
        m_Buffer->setGlobalValue("camera.target", Vector4(Vector3(), camera->farPlane()));
        m_Buffer->setGlobalValue("camera.screen", Vector4(1.0 / m_Screen.x, 1.0 / m_Screen.y, m_Screen.x, m_Screen.y));
        m_Buffer->setGlobalValue("camera.mvpi", (p * mv).inverse());
        m_Buffer->setViewProjection(mv, p);
    }
}

Camera *APipeline::activeCamera() {
    if(m_pController) {
        return m_pController->activeCamera();
    }
    return m_pEngine->controller()->activeCamera();
}

void APipeline::resize(uint32_t width, uint32_t height) {
    m_Screen    = Vector2(width, height);
    //m_pAO->resize(width, height);

    m_Select.resize(width, height);
    m_Depth.resize(width, height);

    for(auto it : m_PostEffects) {
        it->resize(width, height);
    }
}

void APipeline::drawComponents(Object &object, uint8_t layer) {
    for(auto &it : object.getChildren()) {
        Object *child   = it;
        Component *draw = dynamic_cast<Component *>(child);
        if(draw) {
            draw->draw(*m_Buffer, layer);
        } else {
            Actor *actor    = dynamic_cast<Actor *>(child);
            if(actor) {
                if(!actor->isEnable()) {
                    continue;
                }
            }
            drawComponents(*child, layer);
            if(actor) {
                //m_Buffer->setColor(Vector4(1.0));
            }
        }
    }
}

void APipeline::updateShadows(Object &object) {
    for(auto &it : object.getChildren()) {
        ADirectLightGL *light = dynamic_cast<ADirectLightGL *>(it);
        if(light) {
            light->shadowsUpdate(*this);
        } else {
            updateShadows(*it);
        }
    }
}

const Texture *APipeline::pipelineTexture(const string &name) const {
    auto it = m_Targets.find(name);
    if(it != m_Targets.end()) {
        return (*it).second;
    }
    return nullptr;
}

void APipeline::updateLights(Object &object, uint8_t layer) {
    for(auto &it : object.getChildren()) {
        ADirectLightGL *light = dynamic_cast<ADirectLightGL *>(it);
        if(light) {
            Actor &a    = light->actor();
            m_Buffer->setGlobalValue("transform.position", a.position());
            m_Buffer->setGlobalValue("transform.orientation", (a.rotation() * Vector3(0.0f, 0.0f, 1.0f)));

            light->draw(*this, layer);
        } else {
            updateLights(*it, layer);
        }
    }
}

void APipeline::analizeScene(Object &object) {
    // Retrive object id
    glBindFramebuffer( GL_FRAMEBUFFER, m_SelectBuffer );
    m_Buffer->clearRenderTarget(true, Vector4(0.0));

    glDepthFunc(GL_LEQUAL);

    cameraReset();
    drawComponents(object, ICommandBuffer::RAYCAST);

    IController *controller = m_pEngine->controller();
    if(m_pController) {
        controller = m_pController;
    }

    Vector2 position;
    if(controller) {
        Vector2 v;
        controller->selectGeometry(position, v);
    }
    Vector3 screen  = Vector3(position.x / m_Screen.x, position.y / m_Screen.y, 0.0f);
    screen.y        = (1.0 - screen.y);
    glReadPixels((int)screen.x, (int)screen.y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &screen.z);
    Camera::unproject(screen, m_Buffer->modelView(), m_Buffer->projection(), m_World);
    // Get id
    uint32_t result = 0;
    if(position.x >= 0.0f && position.y >= 0.0f &&
        position.x < m_Screen.x && position.y < m_Screen.y) {

        uint8_t value[4];
        glReadPixels(position.x, (m_Screen.y - position.y), 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, value);

        result  = value[0] | (value[1] << 8) | (value[2] << 16) | (value[3] << 24);
    }
    list<uint32_t> l;
    if(result) {
        l.push_back(result);
    }
    controller->setSelectedObjects(l);
}
