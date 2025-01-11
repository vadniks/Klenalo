
#include "xlvgl.h"
#include "scenes.h"
#include "defs.h"
#include "lifecycle.h"
#include "splashScene.h"

static atomic bool gInitialized = false;

static lv_obj_t* gPreviousScreen = nullptr;
static lv_obj_t* gScreen = nullptr;
static lv_obj_t* gLabel = nullptr;

static void end(void) {
    scenesSetCurrentScene(SCENES_SCENE_LOGIN);
}

void splashSceneInit(void) {
    assert(scenesInitialized() && !gInitialized);
    gInitialized = true;

    assert(gPreviousScreen = lv_screen_active());

    assert(gScreen = lv_obj_create(nullptr));
    lv_screen_load(gScreen);
    lv_obj_align(gScreen, LV_ALIGN_DEFAULT, 0, 0);

    assert(gLabel = lv_label_create(gScreen));
    lv_label_set_text_static(gLabel, "Splash"); // TODO
    lv_obj_align(gLabel, LV_ALIGN_CENTER, 0, 0);

    lifecycleAsync((LifecycleAsyncActionFunction) end, nullptr, 500);
}

void splashSceneQuit(void) {
    assert(gInitialized);
    gInitialized = false;

    lv_screen_load(gPreviousScreen);

    lv_obj_delete(gLabel);
    lv_obj_delete(gScreen);
}
