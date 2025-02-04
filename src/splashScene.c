
#include "xlvgl.h"
#include "scenes.h"
#include "defs.h"
#include "lifecycle.h"
#include "consts.h"
#include "resources.h"
#include "splashScene.h"

static const int PROGRESS_BAR_MAX = 100, PROGRESS_BAR_INCREMENT = 5;

static atomic bool gInitialized = false;

static lv_obj_t* gScreen = nullptr;
static lv_obj_t* gLabel = nullptr;
static lv_obj_t* gProgressBar = nullptr;

static void progress(void* const) {
    if (!gInitialized) return;

    const int value = lv_bar_get_value(gProgressBar) + PROGRESS_BAR_INCREMENT;

    if (value <= PROGRESS_BAR_MAX) {
        lv_bar_set_value(gProgressBar, value, LV_ANIM_ON);
        lifecycleRunInMainThread(progress, nullptr);
    } else
        scenesSetCurrentScene(SCENES_SCENE_LOGIN);
}

void splashSceneInit(void) {
    assert(scenesInitialized() && !gInitialized);
    gInitialized = true;

    assert(gScreen = lv_obj_create(nullptr));
    lv_screen_load(gScreen);
    lv_obj_set_layout(gScreen, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(gScreen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(gScreen, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    assert(gLabel = lv_label_create(gScreen));
    lv_label_set_text_static(gLabel, constsString(CONSTS_STRING_SPLASH));
    lv_obj_set_style_text_font(gLabel, resourcesFont(RESOURCES_FONT_SIZE_NORMAL, RESOURCES_FONT_TYPE_MONOSPACE), 0);

    assert(gProgressBar = lv_bar_create(gScreen));
    lv_bar_set_mode(gProgressBar, LV_BAR_MODE_NORMAL);
    lv_bar_set_orientation(gProgressBar, LV_BAR_ORIENTATION_HORIZONTAL);
    lv_bar_set_range(gProgressBar, 0, PROGRESS_BAR_MAX);
    lv_bar_set_start_value(gProgressBar, 0, LV_ANIM_OFF);

    lifecycleRunInMainThread(progress, nullptr);
}

void splashSceneQuit(void) {
    assert(gInitialized);
    gInitialized = false;

    lv_obj_delete(gProgressBar);
    lv_obj_delete(gLabel);
    lv_obj_delete(gScreen);
}
