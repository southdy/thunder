#ifndef COMPONENT_H
#define COMPONENT_H

#include <engine.h>

class Actor;
class ICommandBuffer;
class ComponentPrivate;

class NEXT_LIBRARY_EXPORT Component : public Object {
    A_REGISTER(Component, Object, General)

    A_PROPERTIES (
        A_PROPERTY(bool, Enable, Component::isEnable, Component::setEnable)
    )
    A_METHODS(
        A_METHOD(Actor *, Component::actor)
    )

public:
    Component();
    ~Component();

    Actor *actor () const;

    bool isEnable () const;
    void setEnable (bool enable);

    bool isStarted () const;
    void setStarted (bool started);

#ifdef NEXT_SHARED
    virtual bool drawHandles();
#endif
protected:
    bool isSerializable () const;

private:
    ComponentPrivate *p_ptr;

};

#endif // COMPONENT_H
