#include "handles.h"

#include <commandbuffer.h>
#include <components/camera.h>
#include <components/actor.h>

#include <resources/material.h>

#include "handletools.h"

#define SIDES 32
#define ALPHA 0.3

#define OVERRIDE "texture0"

Vector4 Handles::s_Normal   = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
Vector4 Handles::s_Selected = Vector4(1.0f, 1.0f, 0.0f, 1.0f);
Vector4 Handles::s_xColor   = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
Vector4 Handles::s_yColor   = Vector4(0.0f, 1.0f, 0.0f, 1.0f);
Vector4 Handles::s_zColor   = Vector4(0.0f, 0.0f, 1.0f, 1.0f);

Vector4 Handles::s_Color    = Handles::s_Normal;
Vector4 Handles::s_Second   = Handles::s_Normal;

Vector3 Handles::m_sMouse   = Vector3();

const Vector4 grey(0.3, 0.3, 0.3, 0.6);

Camera *Handles::s_ActiveCamera = nullptr;

uint8_t Handles::s_Axes     = 0;

static ICommandBuffer *s_Buffer = nullptr;

float length    = 1.0f;
float sense     = 0.02f;
float conesize  = length / 5.0f;

Mesh *Handles::s_Cone   = nullptr;
Mesh *Handles::s_Quad   = nullptr;
Mesh *Handles::s_Move   = nullptr;

MaterialInstance *Handles::s_Gizmo  = nullptr;
MaterialInstance *Handles::s_Sprite = nullptr;

enum {
    AXIS,
    SCALE,
    SCALE_XY,
    SCALE_XYZ,
    MOVE,
    MOVE_XY,
    CIRCLE
};

void Handles::init() {
    if(s_Quad == nullptr) {
        s_Quad      = Engine::loadResource<Mesh>(".embedded/plane.fbx");
    }
    if(s_Sprite == nullptr) {
        Material *m = Engine::loadResource<Material>(".embedded/DefaultSprite.mtl");
        if(m) {
            s_Sprite = m->createInstance();
        }
    }

    if(s_Cone == nullptr) {
        s_Cone      = Engine::loadResource<Mesh>(".embedded/cone.fbx");
    }
    if(s_Gizmo == nullptr) {
        Material *m = Engine::loadResource<Material>(".embedded/gizmo.mtl");
        if(m) {
            s_Gizmo = m->createInstance();
        }
    }

    if(s_Move == nullptr) {
        s_Move  = Engine::objectCreate<Mesh>("Move");

        Mesh::Lod lod;
        lod.vertices    = {Vector3(0.0f), Vector3(0, 5, 0)};
        lod.indices     = {0, 1};
        {
            Mesh::Surface surface;
            surface.mode    = Mesh::MODE_LINES;
            surface.lods.push_back(lod);
            s_Move->addSurface(surface);
        }
        lod.vertices    = {Vector3(0, 2, 0), Vector3(1, 1, 0), Vector3(0, 3, 0), Vector3(1.5, 1.5, 0)};
        lod.indices     = {0, 1, 2, 3};
        {
            Mesh::Surface surface;
            surface.mode    = Mesh::MODE_LINES;
            surface.lods.push_back(lod);
            s_Move->addSurface(surface);
        }
        lod.indices     = {0, 1, 2, 1, 3, 2};
        {
            Mesh::Surface surface;
            surface.mode    = Mesh::MODE_TRIANGLES;
            surface.lods.push_back(lod);
            s_Move->addSurface(surface);
        }
        lod.vertices    = {Vector3(0, 2, 0), Vector3(0, 0, 0), Vector3(1, 1, 0)};
        lod.indices     = {0, 1, 2};
        {
            Mesh::Surface surface;
            surface.mode    = Mesh::MODE_TRIANGLES;
            surface.lods.push_back(lod);
            s_Move->addSurface(surface);
        }
        lod.vertices    = {Vector3(0, 1, 0), Vector3(2, 1, 0)};
        lod.indices     = {0, 1};
        {
            Mesh::Surface surface;
            surface.mode    = Mesh::MODE_LINES;
            surface.lods.push_back(lod);
            s_Move->addSurface(surface);
        }
        lod.vertices    = {Vector3(0,-1, 0), Vector3(2,-1, 0), Vector3(0, 1, 0), Vector3(2, 1, 0)};
        lod.indices     = {0, 1, 2, 1, 3, 2};
        {
            Mesh::Surface surface;
            surface.mode    = Mesh::MODE_TRIANGLES;
            surface.lods.push_back(lod);
            s_Move->addSurface(surface);
        }
        lod.vertices    = HandleTools::pointsArc(Quaternion(), 5.0, 0, 180);
        lod.indices.clear();
        {
            Mesh::Surface surface;
            surface.mode    = Mesh::MODE_LINE_STRIP;
            surface.lods.push_back(lod);
            s_Move->addSurface(surface);
        }
        s_Move->apply();
    }
}

void Handles::beginDraw(ICommandBuffer *buffer) {
    Matrix4 v, p;
    s_ActiveCamera->matrices(v, p);

    HandleTools::setViewProjection(v, p);

    s_Buffer    = buffer;
    s_Buffer->setViewProjection(v, p);
    s_Buffer->clearRenderTarget(false, Vector4(), true, 1.0f);
}

void Handles::endDraw() {

}

void Handles::drawArrow(const Matrix4 &transform) {
    s_Buffer->setColor(s_Color);
    s_Buffer->drawMesh(transform, s_Move, AXIS, ICommandBuffer::TRANSLUCENT, s_Gizmo);

    Matrix4 m;
    m.translate(Vector3(0, 4.0, 0));
    s_Buffer->setColor(s_Second);
    s_Buffer->drawMesh(transform * m, s_Cone, 0, ICommandBuffer::TRANSLUCENT, s_Gizmo);
}

void Handles::drawLines(const Matrix4 &transform, const Vector3Vector &points, const Mesh::IndexVector &indices) {
    Mesh *lines = Engine::objectCreate<Mesh>("Lines");

    Mesh::Lod lod;
    lod.vertices    = points;
    lod.indices     = indices;
    {
        Mesh::Surface surface;
        surface.mode    = Mesh::MODE_LINES;
        surface.lods.push_back(lod);
        lines->addSurface(surface);
        lines->apply();
    }
    s_Buffer->setColor(s_Color);
    s_Buffer->drawMesh(transform, lines, 0, ICommandBuffer::TRANSLUCENT, s_Gizmo);

    delete lines;
}

bool Handles::drawBillboard(const Vector3 &position, const Vector2 &size, Texture *texture) {
    bool result = false;
    Matrix4 model(position, Quaternion(), Vector3(size, 1.0));
    Matrix4 q   = model * Matrix4(Vector3(), s_ActiveCamera->actor().rotation(), Vector3(1.0));

    if(HandleTools::distanceToPoint(q, Vector3()) <= sense) {
        result = true;
    }

    s_Sprite->setTexture(OVERRIDE,  texture);
    s_Buffer->setColor(s_Color);
    s_Buffer->drawMesh(q, s_Quad, 0, ICommandBuffer::TRANSLUCENT, s_Sprite);
    return result;
}

Vector3 Handles::moveTool(const Vector3 &position, bool locked) {
    Vector3 result    = position;
    if(s_ActiveCamera) {
        float scale   = (position - s_ActiveCamera->actor().position()).length() * cos(s_ActiveCamera->fov() * DEG2RAD) * 0.2f;
        Matrix4 model(position, Quaternion(), scale);

        Matrix4 x   = model * Matrix4(Vector3(conesize, 0, 0), Quaternion(Vector3(0, 0, 1),-90) * Quaternion(Vector3(0, 1, 0),-90), conesize);
        Matrix4 y   = model * Matrix4(Vector3(0, conesize, 0), Quaternion(), conesize);
        Matrix4 z   = model * Matrix4(Vector3(0, 0, conesize), Quaternion(Vector3(0, 0, 1), 90) * Quaternion(Vector3(1, 0, 0), 90), conesize);

        Matrix4 r(Vector3(), Quaternion(Vector3(0, 1, 0),-90), Vector3(1));

        if(!locked) {
            if(HandleTools::distanceToPoint(model, Vector3()) <= sense) {
                s_Axes  = AXIS_X | AXIS_Y | AXIS_Z;
            } else if((HandleTools::distanceToMesh(x, s_Move, MOVE) <= sense) ||
                      (HandleTools::distanceToMesh(z * r, s_Move, MOVE) <= sense)) {
                s_Axes  = AXIS_X | AXIS_Z;
            } else if((HandleTools::distanceToMesh(y, s_Move, MOVE) <= sense) ||
                      (HandleTools::distanceToMesh(x * r, s_Move, MOVE) <= sense)) {
                s_Axes  = AXIS_Y | AXIS_X;
            } else if((HandleTools::distanceToMesh(z, s_Move, MOVE) <= sense) ||
                      (HandleTools::distanceToMesh(y * r, s_Move, MOVE) <= sense)) {
                s_Axes  = AXIS_Z | AXIS_Y;
            } else if(HandleTools::distanceToMesh(x, s_Move, AXIS) <= sense) {
                s_Axes  = AXIS_X;
            } else if(HandleTools::distanceToMesh(y, s_Move, AXIS) <= sense) {
                s_Axes  = AXIS_Y;
            } else if(HandleTools::distanceToMesh(z, s_Move, AXIS) <= sense) {
                s_Axes  = AXIS_Z;
            }
        }

        s_Second    = s_xColor;
        s_Color     = (s_Axes & AXIS_X) ? s_Selected : s_xColor;
        drawArrow(x);
        s_Buffer->setColor(s_Axes == (AXIS_X | AXIS_Z) ? s_Selected : s_xColor);
        s_Buffer->drawMesh(x, s_Move, MOVE, ICommandBuffer::TRANSLUCENT, s_Gizmo);
        s_Buffer->setColor(s_Axes == (AXIS_X | AXIS_Y) ? s_Selected : s_xColor);
        s_Buffer->drawMesh(x * r, s_Move, MOVE, ICommandBuffer::TRANSLUCENT, s_Gizmo);

        s_Second    = s_yColor;
        s_Color     = (s_Axes & AXIS_Y) ? s_Selected : s_yColor;
        drawArrow(y);
        s_Buffer->setColor(s_Axes == (AXIS_X | AXIS_Y) ? s_Selected : s_yColor);
        s_Buffer->drawMesh(y, s_Move, MOVE, ICommandBuffer::TRANSLUCENT, s_Gizmo);
        s_Buffer->setColor(s_Axes == (AXIS_Y | AXIS_Z) ? s_Selected : s_yColor);
        s_Buffer->drawMesh(y * r, s_Move, MOVE, ICommandBuffer::TRANSLUCENT, s_Gizmo);

        s_Second    = s_zColor;
        s_Color     = (s_Axes & AXIS_Z) ? s_Selected : s_zColor;
        drawArrow(z);
        s_Buffer->setColor(s_Axes == (AXIS_Y | AXIS_Z) ? s_Selected : s_zColor);
        s_Buffer->drawMesh(z, s_Move, MOVE, ICommandBuffer::TRANSLUCENT, s_Gizmo);
        s_Buffer->setColor(s_Axes == (AXIS_X | AXIS_Z) ? s_Selected : s_zColor);
        s_Buffer->drawMesh(z * r, s_Move, MOVE, ICommandBuffer::TRANSLUCENT, s_Gizmo);

        s_Color = s_Selected;
        s_Color.w = ALPHA;
        if(s_Axes == (AXIS_X | AXIS_Z)) {
            s_Buffer->setColor(s_Color);
            s_Buffer->drawMesh(x, s_Move, MOVE_XY, ICommandBuffer::TRANSLUCENT, s_Gizmo);
        }
        if(s_Axes == (AXIS_X | AXIS_Y)) {
            s_Buffer->setColor(s_Color);
            s_Buffer->drawMesh(y, s_Move, MOVE_XY, ICommandBuffer::TRANSLUCENT, s_Gizmo);
        }
        if(s_Axes == (AXIS_Y | AXIS_Z)) {
            s_Buffer->setColor(s_Color);
            s_Buffer->drawMesh(z, s_Move, MOVE_XY, ICommandBuffer::TRANSLUCENT, s_Gizmo);
        }
        s_Color     = s_Normal;
        s_Second    = s_Normal;

        Plane plane;
        plane.point     = position;
        plane.normal    = s_ActiveCamera->actor().rotation() * Vector3(0.0, 0.0, 1.0);
        if(s_Axes == AXIS_X || s_Axes == AXIS_Z) {
            plane.normal    = Vector3(0.0f, plane.normal.y, plane.normal.z);
        } else if(s_Axes == (AXIS_X | AXIS_Y)) {
            plane.normal    = Vector3(0.0f, 0.0f, 1.0f);
        } else if(s_Axes == (AXIS_Z | AXIS_Y)) {
            plane.normal    = Vector3(1.0f, 0.0f, 0.0f);
        } else if(s_Axes == AXIS_Y || s_Axes == (AXIS_X | AXIS_Y | AXIS_Z)) {
            plane.normal    = s_ActiveCamera->actor().rotation() * Vector3(0.0, 0.0, 1.0);
            plane.normal    = Vector3(plane.normal.x, 0.0f, plane.normal.z);
        }
        plane.d = plane.normal.dot(plane.point);

        Ray ray = s_ActiveCamera->castRay(m_sMouse.x, m_sMouse.y);
        Vector3 point;
        ray.intersect(plane, &point, true);
        if(s_Axes & AXIS_X) {
            result.x    = point.x;
        }
        if(s_Axes & AXIS_Y) {
            result.y    = point.y;
        }
        if(s_Axes & AXIS_Z) {
            result.z    = point.z;
        }
    }

    return result;
}

Vector3 Handles::rotationTool(const Vector3 &position, bool locked) {
    if(s_ActiveCamera) {
        float scale     = ((position - s_ActiveCamera->actor().position()).length() * cos(s_ActiveCamera->fov() / 2 * DEG2RAD) * 0.2f);
        Vector3 normal  = position - s_ActiveCamera->actor().position();
        normal.normalize();

        Matrix4 model(position, Quaternion(), scale);

        Matrix4 q1  = model * Matrix4(Vector3(), s_ActiveCamera->actor().rotation() * Quaternion(Vector3( 90, 0, 0)), Vector3(conesize));
        Matrix4 q2  = q1 * Matrix4(Vector3(), Quaternion(Vector3( 0, 1, 0), 180), Vector3(1));

        Matrix4 x   = model * Matrix4(Vector3(), Quaternion(Vector3(0, 0, 90)) *
                                                 Quaternion(Vector3(0, 1, 0), RAD2DEG * atan2(normal.y, normal.z) + 180), Vector3(conesize));
        Matrix4 y   = model * Matrix4(Vector3(), Quaternion(Vector3(0, 1, 0), RAD2DEG * atan2(normal.x, normal.z) + 180), Vector3(conesize));
        Matrix4 z   = model * Matrix4(Vector3(), Quaternion(Vector3(0, 0, 1),-RAD2DEG * atan2(normal.x, normal.y)) *
                                                 Quaternion(Vector3(90, 0, 0)), Vector3(conesize));

        Matrix4 m;
        m.scale(1.2);

        if(!locked) {
            if((HandleTools::distanceToMesh(q1 * m, s_Move, CIRCLE) <= sense) ||
               (HandleTools::distanceToMesh(q2 * m, s_Move, CIRCLE) <= sense)) {
                s_Axes  = AXIS_X | AXIS_Y | AXIS_Z;
            } else if(HandleTools::distanceToMesh(x, s_Move, CIRCLE) <= sense) {
                s_Axes  = AXIS_X;
            } else if(HandleTools::distanceToMesh(y, s_Move, CIRCLE) <= sense) {
                s_Axes  = AXIS_Y;
            } else if(HandleTools::distanceToMesh(z, s_Move, CIRCLE) <= sense) {
                s_Axes  = AXIS_Z;
            }
        }

        s_Buffer->setColor((s_Axes == (AXIS_X | AXIS_Y | AXIS_Z)) ? s_Selected : grey * 2.0f);
        s_Buffer->drawMesh(q1 * m, s_Move, CIRCLE, ICommandBuffer::TRANSLUCENT, s_Gizmo);
        s_Buffer->drawMesh(q2 * m, s_Move, CIRCLE, ICommandBuffer::TRANSLUCENT, s_Gizmo);
        s_Buffer->setColor(grey);
        s_Buffer->drawMesh(q1, s_Move, CIRCLE, ICommandBuffer::TRANSLUCENT, s_Gizmo);
        s_Buffer->drawMesh(q2, s_Move, CIRCLE, ICommandBuffer::TRANSLUCENT, s_Gizmo);

        if(!locked || s_Axes == AXIS_X) {
            s_Buffer->setColor((s_Axes == AXIS_X) ? s_Selected : s_xColor);
            s_Buffer->drawMesh(x, s_Move, CIRCLE, ICommandBuffer::TRANSLUCENT, s_Gizmo);
        }

        if(!locked || s_Axes == AXIS_Y) {
            s_Buffer->setColor((s_Axes == AXIS_Y) ? s_Selected : s_yColor);
            s_Buffer->drawMesh(y, s_Move, CIRCLE, ICommandBuffer::TRANSLUCENT, s_Gizmo);
        }

        if(!locked || s_Axes == AXIS_Z) {
            s_Buffer->setColor((s_Axes == AXIS_Z) ? s_Selected : s_zColor);
            s_Buffer->drawMesh(z, s_Move, CIRCLE, ICommandBuffer::TRANSLUCENT, s_Gizmo);
        }
        s_Buffer->setColor(s_Normal);
    }
    return m_sMouse * 100;
}

Vector3 Handles::scaleTool(const Vector3 &position, bool locked) {
    if(s_ActiveCamera) {
        Vector3 normal  = position - s_ActiveCamera->actor().position();
        float size      = normal.length() * cos(s_ActiveCamera->fov() / 2 * DEG2RAD) * 0.2;

        Vector3 scale(((normal.x < 0) ? 1 : -1) * size,
                      ((normal.y < 0) ? 1 : -1) * size,
                      ((normal.z < 0) ? 1 : -1) * size);

        Matrix4 model(position, Quaternion(), scale);
        Matrix4 x   = model * Matrix4(Vector3(), Quaternion(Vector3(0, 0, 1),-90) * Quaternion(Vector3(0, 1, 0),-90), Vector3(conesize));
        Matrix4 y   = model * Matrix4(Vector3(), Quaternion(), Vector3(conesize));
        Matrix4 z   = model * Matrix4(Vector3(), Quaternion(Vector3(0, 0, 1), 90) * Quaternion(Vector3(1, 0, 0), 90), Vector3(conesize));

        Matrix4 r(Vector3(), Quaternion(Vector3(0, 1, 0),-90), Vector3(1));

        if(!locked) {
            if(HandleTools::distanceToPoint(model, Vector3()) <= sense) {
                s_Axes  = AXIS_X | AXIS_Y | AXIS_Z;
            } else if((HandleTools::distanceToMesh(x, s_Move, SCALE) <= sense) ||
                      (HandleTools::distanceToMesh(z * r, s_Move, SCALE) <= sense)) {
                s_Axes  = AXIS_X | AXIS_Z;
            } else if((HandleTools::distanceToMesh(y, s_Move, SCALE) <= sense) ||
                      (HandleTools::distanceToMesh(x * r, s_Move, SCALE) <= sense)) {
                s_Axes  = AXIS_Y | AXIS_X;
            } else if((HandleTools::distanceToMesh(z, s_Move, SCALE) <= sense) ||
                      (HandleTools::distanceToMesh(y * r, s_Move, SCALE) <= sense)) {
                s_Axes  = AXIS_Z | AXIS_Y;
            } else if(HandleTools::distanceToMesh(x, s_Move, AXIS) <= sense) {
                s_Axes  = AXIS_X;
            } else if(HandleTools::distanceToMesh(y, s_Move, AXIS) <= sense) {
                s_Axes  = AXIS_Y;
            } else if(HandleTools::distanceToMesh(z, s_Move, AXIS) <= sense) {
                s_Axes  = AXIS_Z;
            }
        }

        s_Color     = s_Selected;
        s_Color.w   = ALPHA;
        {
            s_Buffer->setColor(s_xColor);
            if(s_Axes == (AXIS_X | AXIS_Z)) {
                s_Buffer->setColor(s_Color);
                s_Buffer->drawMesh(x, s_Move, SCALE_XY, ICommandBuffer::TRANSLUCENT, s_Gizmo);
                s_Buffer->setColor(s_Selected);
            }
            s_Buffer->drawMesh(x, s_Move, SCALE, ICommandBuffer::TRANSLUCENT, s_Gizmo);

            s_Buffer->setColor(s_xColor);
            if(s_Axes == (AXIS_X | AXIS_Y)) {
                s_Buffer->setColor(s_Color);
                s_Buffer->drawMesh(x * r, s_Move, SCALE_XY, ICommandBuffer::TRANSLUCENT, s_Gizmo);
                s_Buffer->setColor(s_Selected);
            }
            s_Buffer->drawMesh(x * r, s_Move, SCALE, ICommandBuffer::TRANSLUCENT, s_Gizmo);

            s_Buffer->setColor((s_Axes & AXIS_X) ? s_Selected : s_xColor);
            s_Buffer->drawMesh(x, s_Move, AXIS, ICommandBuffer::TRANSLUCENT, s_Gizmo);
        }
        {
            s_Buffer->setColor(s_yColor);
            if(s_Axes == (AXIS_Y | AXIS_X)) {
                s_Buffer->setColor(s_Color);
                s_Buffer->drawMesh(y, s_Move, SCALE_XY, ICommandBuffer::TRANSLUCENT, s_Gizmo);
                s_Buffer->setColor(s_Selected);
            }
            s_Buffer->drawMesh(y, s_Move, SCALE, ICommandBuffer::TRANSLUCENT, s_Gizmo);

            s_Buffer->setColor(s_yColor);
            if(s_Axes == (AXIS_Y | AXIS_Z)) {
                s_Buffer->setColor(s_Color);
                s_Buffer->drawMesh(y * r, s_Move, SCALE_XY, ICommandBuffer::TRANSLUCENT, s_Gizmo);
                s_Buffer->setColor(s_Selected);
            }
            s_Buffer->drawMesh(y * r, s_Move, SCALE, ICommandBuffer::TRANSLUCENT, s_Gizmo);

            s_Buffer->setColor((s_Axes & AXIS_Y) ? s_Selected : s_yColor);
            s_Buffer->drawMesh(y, s_Move, AXIS, ICommandBuffer::TRANSLUCENT, s_Gizmo);
        }
        {
            s_Buffer->setColor(s_zColor);
            if(s_Axes == (AXIS_Z | AXIS_Y)) {
                s_Buffer->setColor(s_Color);
                s_Buffer->drawMesh(z, s_Move, SCALE_XY, ICommandBuffer::TRANSLUCENT, s_Gizmo);
                s_Buffer->setColor(s_Selected);
            }
            s_Buffer->drawMesh(z, s_Move, SCALE, ICommandBuffer::TRANSLUCENT, s_Gizmo);

            s_Buffer->setColor(s_zColor);
            if(s_Axes == (AXIS_Z | AXIS_X)) {
                s_Buffer->setColor(s_Color);
                s_Buffer->drawMesh(z * r, s_Move, SCALE_XY, ICommandBuffer::TRANSLUCENT, s_Gizmo);
                s_Buffer->setColor(s_Selected);
            }
            s_Buffer->drawMesh(z * r, s_Move, SCALE, ICommandBuffer::TRANSLUCENT, s_Gizmo);

            s_Buffer->setColor((s_Axes & AXIS_Z) ? s_Selected : s_zColor);
            s_Buffer->drawMesh(z, s_Move, AXIS, ICommandBuffer::TRANSLUCENT, s_Gizmo);
        }

        if(s_Axes == (AXIS_X | AXIS_Y | AXIS_Z)) {
            s_Buffer->setColor(s_Color);
            s_Buffer->drawMesh(x,       s_Move, SCALE_XYZ, ICommandBuffer::TRANSLUCENT, s_Gizmo);
            s_Buffer->drawMesh(x * r,   s_Move, SCALE_XYZ, ICommandBuffer::TRANSLUCENT, s_Gizmo);
            s_Buffer->drawMesh(y,       s_Move, SCALE_XYZ, ICommandBuffer::TRANSLUCENT, s_Gizmo);
            s_Buffer->drawMesh(y * r,   s_Move, SCALE_XYZ, ICommandBuffer::TRANSLUCENT, s_Gizmo);
            s_Buffer->drawMesh(z,       s_Move, SCALE_XYZ, ICommandBuffer::TRANSLUCENT, s_Gizmo);
            s_Buffer->drawMesh(z * r,   s_Move, SCALE_XYZ, ICommandBuffer::TRANSLUCENT, s_Gizmo);
        }

        s_Color = s_Normal;
    }

    return m_sMouse * 100;
}


