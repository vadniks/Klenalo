
#include "defs.h"
#include "video.h"
#include "splashScene.h"
#include "loginScene.h"
#include "scenes.h"

static atomic bool gInitialized = false;

static atomic ScenesScene gCurrentScene = SCENES_SCENE_NONE;

void scenesInit(void) {
    assert(videoInitialized() && !gInitialized);
    gInitialized = true;

    scenesSetCurrentScene(SCENES_SCENE_SPLASH);
}

bool scenesInitialized(void) {
    return gInitialized;
}

ScenesScene scenesCurrentScene(void) {
    assert(videoInitialized() && gInitialized);
    return gCurrentScene;
}

static void quitCurrentScene(void) {
    switch (gCurrentScene) {
        case SCENES_SCENE_NONE:
            break;
        case SCENES_SCENE_SPLASH:
            splashSceneQuit();
            break;
        case SCENES_SCENE_LOGIN:
            loginSceneQuit();
            break;
    }
}

void scenesSetCurrentScene(const ScenesScene scene) {
    assert(videoInitialized() && gInitialized);

    quitCurrentScene();

    switch (scene) {
        case SCENES_SCENE_NONE:
            break;
        case SCENES_SCENE_SPLASH:
            splashSceneInit();
            break;
        case SCENES_SCENE_LOGIN:
            loginSceneInit();
            break;
    }

    gCurrentScene = scene;
}

void scenesQuit(void) {
    assert(videoInitialized() && gInitialized);

    quitCurrentScene();

    gInitialized = false;
}
