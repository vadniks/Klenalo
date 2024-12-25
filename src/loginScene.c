
#include "xlvgl.h"
#include "defs.h"
#include "consts.h"
#include "lifecycle.h"
#include "input.h"
#include "loginScene.h"

static atomic bool gInitialized = false;

static lv_obj_t* gPreviousScreen = nullptr;
static lv_obj_t* gScreen = nullptr;
static lv_group_t* gGroup = nullptr;
static lv_obj_t* gTitleLabel = nullptr;
static lv_obj_t* gUsernameTextField = nullptr;
static lv_obj_t* gPasswordTextField = nullptr;
static lv_obj_t* gRememberCredentialsCheckbox = nullptr;
static lv_obj_t* gSignInButton = nullptr;
static lv_obj_t* gSignInLabel = nullptr;

void loginSceneInit(void) {
    assert(lifecycleInitialized() && !gInitialized);
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

    assert(gTitleLabel = lv_label_create(gScreen));
//    lv_obj_get_style_text_font(); // TODO
    lv_label_set_text(gTitleLabel, TITLE);

    assert(gUsernameTextField = lv_textarea_create(gScreen));
    lv_textarea_set_one_line(gUsernameTextField, true);
    lv_textarea_set_max_length(gUsernameTextField, MAX_USERNAME_SIZE);
    lv_textarea_set_placeholder_text(gUsernameTextField, USERNAME);

    assert(gPasswordTextField = lv_textarea_create(gScreen));
    lv_textarea_set_one_line(gPasswordTextField, true);
    lv_textarea_set_max_length(gPasswordTextField, MAX_PASSWORD_SIZE);
    lv_textarea_set_placeholder_text(gPasswordTextField, PASSWORD);
    lv_textarea_set_password_mode(gPasswordTextField, true);

    assert(gRememberCredentialsCheckbox = lv_checkbox_create(gScreen));
    lv_checkbox_set_text(gRememberCredentialsCheckbox, REMEMBER_CREDENTIALS);

    assert(gSignInButton = lv_button_create(gScreen));

    assert(gSignInLabel = lv_label_create(gSignInButton));
    lv_label_set_text(gSignInLabel, SIGN_IN);
    lv_obj_center(gSignInLabel);
}

void loginSceneQuit(void) {
    assert(gInitialized);
    gInitialized = false;

    lv_screen_load(gPreviousScreen);

    lv_obj_delete(gSignInLabel);
    lv_obj_delete(gSignInButton);
    lv_obj_delete(gRememberCredentialsCheckbox);
    lv_obj_delete(gPasswordTextField);
    lv_obj_delete(gUsernameTextField);
    lv_obj_delete(gTitleLabel);
    lv_group_delete(gGroup);
    lv_obj_delete(gScreen);
}
