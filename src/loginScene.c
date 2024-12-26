
#include "xlvgl.h"
#include "defs.h"
#include "consts.h"
#include "video.h"
#include "input.h"
#include "resources.h"
#include "loginScene.h"

static atomic bool gInitialized = false;

static lv_obj_t* gPreviousScreen = nullptr;
static lv_obj_t* gScreen = nullptr;
static lv_group_t* gGroup = nullptr;
static lv_obj_t* gWelcomeLabel = nullptr;
static lv_obj_t* gAddressLabel = nullptr;
static lv_obj_t* gPasswordTextArea = nullptr;
static lv_obj_t* gRememberCredentialsCheckbox = nullptr;
static lv_obj_t* gSignInButton = nullptr;
static lv_obj_t* gSignInLabel = nullptr;

void loginSceneInit(void) {
    assert(videoInitialized() && !gInitialized);
    gInitialized = true;

    assert(gPreviousScreen = lv_screen_active());

    assert(gScreen = lv_obj_create(nullptr));
    lv_screen_load(gScreen);
    lv_obj_align(gScreen, LV_ALIGN_DEFAULT, 0, 0);
    lv_obj_set_flex_flow(gScreen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(gScreen, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    assert(gGroup = lv_group_create());
    lv_group_set_default(gGroup);
    inputAssignToGroup(gGroup);

    assert(gWelcomeLabel = lv_label_create(gScreen));
    lv_obj_set_style_text_font(gWelcomeLabel, resourcesFontLarge(), 0);
    lv_label_set_text_static(gWelcomeLabel, constsString(WELCOME));

    assert(gAddressLabel = lv_label_create(gScreen));
    lv_obj_set_style_text_font(gAddressLabel, resourcesFontNormal(), 0);
    lv_obj_set_style_text_opa(gAddressLabel, 171, 0);
    lv_label_set_text_static(gAddressLabel, "IP address: 0.0.0.0"); // TODO: stub

    assert(gPasswordTextArea = lv_textarea_create(gScreen));
    lv_textarea_set_one_line(gPasswordTextArea, true);
    lv_textarea_set_max_length(gPasswordTextArea, MAX_PASSWORD_SIZE);
    lv_textarea_set_placeholder_text(gPasswordTextArea, constsString(PASSWORD));
    lv_obj_set_style_text_font(gPasswordTextArea, resourcesFontNormal(), 0);
    lv_textarea_set_password_mode(gPasswordTextArea, true);

    assert(gRememberCredentialsCheckbox = lv_checkbox_create(gScreen));
    lv_obj_set_style_text_font(gRememberCredentialsCheckbox, resourcesFontNormal(), 0);
    lv_checkbox_set_text_static(gRememberCredentialsCheckbox, constsString(REMEMBER_CREDENTIALS));

    assert(gSignInButton = lv_button_create(gScreen));

    assert(gSignInLabel = lv_label_create(gSignInButton));
    lv_obj_set_style_text_font(gSignInLabel, resourcesFontNormal(), 0);
    lv_label_set_text_static(gSignInLabel, constsString(SIGN_IN));
    lv_obj_center(gSignInLabel);
}

void loginSceneQuit(void) {
    assert(gInitialized);
    gInitialized = false;

    lv_screen_load(gPreviousScreen);

    lv_obj_delete(gSignInLabel);
    lv_obj_delete(gSignInButton);
    lv_obj_delete(gRememberCredentialsCheckbox);
    lv_obj_delete(gPasswordTextArea);
    lv_obj_delete(gAddressLabel);
    lv_obj_delete(gWelcomeLabel);
    lv_group_delete(gGroup);
    lv_obj_delete(gScreen);
}
