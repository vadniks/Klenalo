
#include <SDL2/SDL.h>
#include "xlvgl.h"
#include "defs.h"
#include "scenes.h"
#include "consts.h"
#include "input.h"
#include "resources.h"
#include "net.h"
#include "lifecycle.h"
#include "loginScene.h"

static atomic bool gInitialized = false;

static lv_obj_t* gScreen = nullptr;
static lv_group_t* gGroup = nullptr;
static lv_obj_t* gWelcomeLabel = nullptr;
static lv_obj_t* gNetsLayout = nullptr;
static lv_obj_t* gNetsLabel = nullptr;
static lv_obj_t* gNetsDropdown = nullptr;
static lv_obj_t* gAddressLabel = nullptr;
static lv_obj_t* gPasswordTextArea = nullptr;
static lv_obj_t* gRememberCredentialsCheckbox = nullptr;
static lv_obj_t* gSignInButton = nullptr;
static lv_obj_t* gSignInLabel = nullptr;

static SDL_TimerID gTimer = 0;
static List* nullable gNetsList = nullptr; // <NetNet*>

static unsigned update(const unsigned interval, void* const);

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
    lv_label_set_text_static(gWelcomeLabel, constsString(WELCOME));

    assert(gNetsLayout = lv_obj_create(gScreen));
    lv_obj_set_layout(gNetsLayout, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(gNetsLayout, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(gNetsLayout, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    assert(gNetsLabel = lv_label_create(gNetsLayout));
    lv_label_set_text_static(gNetsLabel, "Network: ");

    assert(gNetsDropdown = lv_dropdown_create(gNetsLayout));
    lv_dropdown_clear_options(gNetsDropdown);

    lv_obj_set_width(gNetsLayout, lv_obj_get_width(gNetsLabel) + lv_obj_get_width(gNetsDropdown) + 10);
    lv_obj_set_height(gNetsLayout, max(lv_obj_get_height(gNetsLabel), lv_obj_get_height(gNetsDropdown)) + 10);
    lv_obj_remove_flag(gNetsLayout, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(gNetsLayout, 0, 0);
    lv_obj_set_style_border_opa(gNetsLayout, 0, 0);
    lv_obj_scroll_to(gNetsLayout, 0, 0, LV_ANIM_OFF);

    assert(gAddressLabel = lv_label_create(gScreen));
    lv_obj_set_style_text_font(gAddressLabel, resourcesFont(RESOURCES_FONT_SIZE_NORMAL, RESOURCES_FONT_TYPE_BOLD), 0);
    lv_obj_set_style_text_opa(gAddressLabel, 0xff / 3 * 2, 0);
    lv_label_set_text_static(gAddressLabel, "IP address: 0.0.0.0"); // TODO: stub

    assert(gPasswordTextArea = lv_textarea_create(gScreen));
    lv_textarea_set_one_line(gPasswordTextArea, true);
    lv_textarea_set_max_length(gPasswordTextArea, MAX_PASSWORD_SIZE);
    lv_textarea_set_placeholder_text(gPasswordTextArea, constsString(PASSWORD));
    lv_textarea_set_password_mode(gPasswordTextArea, true);

    assert(gRememberCredentialsCheckbox = lv_checkbox_create(gScreen));
    lv_checkbox_set_text_static(gRememberCredentialsCheckbox, constsString(REMEMBER_CREDENTIALS));

    assert(gSignInButton = lv_button_create(gScreen));

    assert(gSignInLabel = lv_label_create(gSignInButton));
    lv_label_set_text_static(gSignInLabel, constsString(SIGN_IN));
    lv_obj_center(gSignInLabel);

    assert(gTimer = SDL_AddTimer(500, update, nullptr));
}

static unsigned update(const unsigned interval, void* const) {
    if (!gInitialized) return 0;
    assert(scenesInitialized());

    if (gNetsList) listDestroy(gNetsList);
    gNetsList = netNets();

    lifecycleUIMutexCommand(RW_MUTEX_COMMAND_WRITE_LOCK);

    lv_dropdown_clear_options(gNetsDropdown);
    for (int i = 0; i < listSize(gNetsList); i++)
        lv_dropdown_add_option(gNetsDropdown, ((NetNet*) listGet(gNetsList, i))->name, i);

    lifecycleUIMutexCommand(RW_MUTEX_COMMAND_WRITE_UNLOCK);

    return interval;
}

void loginSceneQuit(void) {
    assert(scenesInitialized() && gInitialized);
    gInitialized = false;

    if (gNetsList) listDestroy(gNetsList);
    assert(SDL_RemoveTimer(gTimer));

    lv_obj_delete(gSignInLabel);
    lv_obj_delete(gSignInButton);
    lv_obj_delete(gRememberCredentialsCheckbox);
    lv_obj_delete(gPasswordTextArea);
    lv_obj_delete(gAddressLabel);
    lv_obj_delete(gNetsDropdown);
    lv_obj_delete(gNetsLabel);
    lv_obj_delete(gNetsLayout);
    lv_obj_delete(gWelcomeLabel);
    lv_group_delete(gGroup);
    lv_obj_delete(gScreen);
}
