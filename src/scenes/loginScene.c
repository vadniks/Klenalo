
#include "../integration/xlvgl.h"
#include "../defs.h"
#include "scenes.h"
#include "../consts.h"
#include "../integration/input.h"
#include "../integration/resources.h"
#include "loginScene.h"

static atomic bool gInitialized = false;

static lv_obj_t* gScreen = nullptr;
static lv_group_t* gGroup = nullptr;
static lv_obj_t* gWelcomeLabel = nullptr;
static lv_obj_t* gPasswordTextArea = nullptr;
static lv_obj_t* gRememberCredentialsCheckbox = nullptr;
static lv_obj_t* gSignInButton = nullptr;
static lv_obj_t* gSignInLabel = nullptr;

void loginSceneInit(void) {
    assert(scenesInitialized() && !gInitialized);
    gInitialized = true;

    assert(gScreen = lv_obj_create(nullptr));
    scenesLoadScreen(gScreen);
    lv_obj_set_layout(gScreen, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(gScreen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(gScreen, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    assert(gGroup = lv_group_create());
    lv_group_set_default(gGroup);
    inputAssignToGroup(gGroup);

    assert(gWelcomeLabel = lv_label_create(gScreen));
    lv_obj_set_style_text_font(gWelcomeLabel, resourcesFont(RESOURCES_FONT_SIZE_LARGE, RESOURCES_FONT_TYPE_REGULAR), 0);
    lv_label_set_text_static(gWelcomeLabel, constsString(CONSTS_STRING_WELCOME));

    assert(gPasswordTextArea = lv_textarea_create(gScreen));
    scenesAddInputFocusEventsHandlerToTextarea(gPasswordTextArea);
    lv_textarea_set_one_line(gPasswordTextArea, true);
    lv_textarea_set_max_length(gPasswordTextArea, MAX_PASSWORD_SIZE);
    lv_textarea_set_placeholder_text(gPasswordTextArea, constsString(CONSTS_STRING_PASSWORD));
    lv_textarea_set_password_mode(gPasswordTextArea, true);

    assert(gRememberCredentialsCheckbox = lv_checkbox_create(gScreen)); // TODO: change font to one that supports unicode symbols
    lv_checkbox_set_text_static(gRememberCredentialsCheckbox, constsString(CONSTS_STRING_REMEMBER_CREDENTIALS));

    assert(gSignInButton = lv_button_create(gScreen));

    assert(gSignInLabel = lv_label_create(gSignInButton));
    lv_label_set_text_static(gSignInLabel, constsString(CONSTS_STRING_SIGN_IN));
    lv_obj_center(gSignInLabel);
}

void loginSceneQuit(void) {
    assert(gInitialized);

    lv_obj_delete(gSignInLabel);
    lv_obj_delete(gSignInButton);
    lv_obj_delete(gRememberCredentialsCheckbox);
    lv_obj_delete(gPasswordTextArea);
    lv_obj_delete(gWelcomeLabel);
    lv_group_delete(gGroup);
    lv_obj_delete(gScreen);

    gInitialized = false;
}
