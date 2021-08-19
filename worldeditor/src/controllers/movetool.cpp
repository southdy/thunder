#include "movetool.h"

#include <components/actor.h>
#include <components/transform.h>

#include <editor/handles.h>

#include "objectctrl.h"
#include "undomanager.h"

MoveTool::MoveTool(ObjectCtrl *controller, SelectMap &selection) :
    SelectTool(controller, selection) {

}

void MoveTool::update() {
    bool isDrag = m_pController->isDrag();
    m_World = Handles::moveTool(objectPosition(), Quaternion(), isDrag);
    if(isDrag) {
        Vector3 delta(m_World - m_SavedWorld);
        if(m_MoveGrid > 0.0f) {
            for(int32_t i = 0; i < 3; i++) {
                delta[i] = m_MoveGrid[i] * int(delta[i] / m_MoveGrid[i]);
            }
        }
        for(const auto &it : m_Selected) {
            Vector3 dt(delta);
            Actor *a = dynamic_cast<Actor *>(it.object->parent());
            if(a && a->transform()) {
                dt = a->transform()->worldTransform().rotation().inverse() * delta;
            }
            if(it.object->transform()) {
                it.object->transform()->setPosition(it.position + dt);
            }
        }
        m_pController->objectsUpdated();
        m_pController->objectsChanged(m_pController->selected(), "Position");
    }
}

QString MoveTool::icon() const {
    return ":/Images/editor/Move.png";
}

QString MoveTool::name() const {
    return "Move";
}
