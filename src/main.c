
#include "lifecycle.h"
#include "loginScene.h"

int main(void) {
    lifecycleInit();

    loginSceneInit();

    lifecycleLoop();

    loginSceneQuit();

    lifecycleQuit();
    return 0;
}
