#include "components/nativebehaviour.h"

/*!
    \class NativeBehaviour
    \brief Base class for all programmable game logic.
    \inmodule Engine

    All programmable game logic must be derived from NativeBehaviour class.

    Example:
    \code
        class ExampleBehaviour : public NativeBehaviour {
            A_REGISTER(ExampleBehaviour, NativeBehaviour, General)

            A_NOMETHODS()
            A_NOPROPERTIES()

        public:
            void start() {
                Log(Log::DBG) << "Start";
            }

            void update() {
                Log(Log::DBG) << "Update";
            }
        };
    \endcode
*/

NativeBehaviour::NativeBehaviour() {

}

/*!
    Start is called on the same frame when a script is enabled just before the update method will be called the first time.
*/
void NativeBehaviour::start() {

}
/*!
    Update is called every frame, if the NativeBehaviour is enabled.
*/
void NativeBehaviour::update() {

}
