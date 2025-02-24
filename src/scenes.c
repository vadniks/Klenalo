
#include <SDL3/SDL.h>
#include "xlvgl.h"
#include "defs.h"
#include "video.h"
#include "splashScene.h"
#include "loginScene.h"
#include "scenes.h"

static atomic bool gInitialized = false;

static lv_obj_t* gPreviousScreen = nullptr; // allocated elsewhere
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
    assert(videoInitialized() && gInitialized);

    assert(gPreviousScreen);
    lv_screen_load(gPreviousScreen);

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

    if (gCurrentScene != SCENES_SCENE_NONE)
        quitCurrentScene();

    assert(gPreviousScreen = lv_screen_active());

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

void scenesLoadScreen(lv_obj_t* const screen) {
    assert(videoInitialized() && gInitialized);
    lv_screen_load_anim(screen, LV_SCR_LOAD_ANIM_OVER_LEFT, 250, 0, false);
}

static void textareaFocusEventsHandler(lv_event_t* const event) {
//    SDL_Window* window = videoWindow();
//    lv_event_get_code(event) == LV_EVENT_FOCUSED ? SDL_StartTextInput(window) : SDL_StopTextInput(window);
}

void scenesAddInputFocusEventsHandlerToTextarea(struct _lv_obj_t* const textarea) {
    assert(videoInitialized() && gInitialized);
    lv_obj_add_event_cb(textarea, textareaFocusEventsHandler, LV_EVENT_FOCUSED, nullptr);
    lv_obj_add_event_cb(textarea, textareaFocusEventsHandler, LV_EVENT_DEFOCUSED, nullptr);
}

void scenesQuit(void) {
    assert(gInitialized);

    quitCurrentScene();

    gInitialized = false;
}
