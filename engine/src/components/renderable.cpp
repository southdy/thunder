#include "components/renderable.h"

/*!
    \class Renderable
    \brief Base class for every object which can be drawn on the screen.
    \inmodule Engine

    \note This class must be a superclass only and shouldn't be created manually.
*/

Renderable::Renderable() {

}
/*!
    \internal
*/
void Renderable::draw(CommandBuffer &buffer, uint32_t layer) {
    A_UNUSED(buffer);
    A_UNUSED(layer);
}

/*!
    Returns a bound box of the renderable object.
*/
AABBox Renderable::bound() const {
    return AABBox();
}
/*!
    \internal
*/
bool Renderable::isLight() const {
    return false;
}
/*!
    \internal
*/
bool Renderable::isRenderable() const {
    return true;
}
/*!
    \internal
*/
void Renderable::composeComponent() {

}
