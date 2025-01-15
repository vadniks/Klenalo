
#include <SDL2/SDL_stdinc.h>
#include "xlvgl.h"
#include "scenes.h"
#include "defs.h"
#include "lifecycle.h"
#include "splashScene.h"

static const int PROGRESS_BAR_MAX = 100, PROGRESS_BAR_INCREMENT = 5;

static atomic bool gInitialized = false;
static void* gQuitCheck = nullptr; // ensure that the scene has been quited indeed

static lv_obj_t* gScreen = nullptr;
static lv_obj_t* gLabel = nullptr;
static lv_obj_t* gProgressBar = nullptr;

static void progress(void* const) {
    if (!gInitialized) return;

    const int value = lv_bar_get_value(gProgressBar) + PROGRESS_BAR_INCREMENT;

    if (value <= PROGRESS_BAR_MAX) {
        lv_bar_set_value(gProgressBar, value, LV_ANIM_OFF);
        lifecycleRunInMainThread(progress, nullptr);
    } else
        scenesSetCurrentScene(SCENES_SCENE_LOGIN);
}

void splashSceneInit(void) {
    assert(scenesInitialized() && !gInitialized);
    gInitialized = true;
    assert(gQuitCheck = SDL_malloc(1));

    assert(gScreen = lv_obj_create(nullptr));
    lv_screen_load(gScreen);
    lv_obj_align(gScreen, LV_ALIGN_DEFAULT, 0, 0);

    assert(gLabel = lv_label_create(gScreen));
    lv_label_set_text_static(gLabel, "Splash"); // TODO
    lv_obj_align(gLabel, LV_ALIGN_CENTER, 0, -10);

    assert(gProgressBar = lv_bar_create(gScreen));
    lv_obj_align(gProgressBar, LV_ALIGN_CENTER, 0, 10);
    lv_bar_set_mode(gProgressBar, LV_BAR_MODE_NORMAL);
    lv_bar_set_orientation(gProgressBar, LV_BAR_ORIENTATION_HORIZONTAL);
    lv_bar_set_range(gProgressBar, 0, PROGRESS_BAR_MAX);
    lv_bar_set_start_value(gProgressBar, 0, LV_ANIM_OFF);

    lifecycleRunInMainThread(progress, nullptr);
}

void splashSceneQuit(void) {
    assert(scenesInitialized() && gInitialized);
    gInitialized = false;
    SDL_free(gQuitCheck);

    lv_obj_delete(gProgressBar);
    lv_obj_delete(gLabel);
    lv_obj_delete(gScreen);
}
