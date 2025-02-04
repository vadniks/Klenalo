
#include "xlvgl.h"
#include "defs.h"
#include "scenes.h"
#include "consts.h"
#include "input.h"
#include "resources.h"
#include "net.h"
#include "loginScene.h"

static atomic bool gInitialized = false;

static lv_obj_t* gScreen = nullptr;
static lv_group_t* gGroup = nullptr;
static lv_obj_t* gWelcomeLabel = nullptr;
static lv_obj_t* gNetsDropdown = nullptr;
static lv_obj_t* gAddressLabel = nullptr;
static lv_obj_t* gPasswordTextArea = nullptr;
static lv_obj_t* gRememberCredentialsCheckbox = nullptr;
static lv_obj_t* gSignInButton = nullptr;
static lv_obj_t* gSignInLabel = nullptr;

static List* nullable gNetsList = nullptr; // <NetNet*>
static NetNet* gSelectedNet = nullptr; // allocated elsewhere

static void netsDropdownValueChangeCallback(lv_event_t* nullable const);

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

    assert(gNetsDropdown = lv_dropdown_create(gScreen));
    lv_dropdown_set_text(gNetsDropdown, constsString(NETWORK));
//    lv_dropdown_clear_options(gNetsDropdown);
    lv_dropdown_set_options_static(gNetsDropdown, "One\nTwo\nThree");
    lv_obj_add_event_cb(gNetsDropdown, netsDropdownValueChangeCallback, LV_EVENT_VALUE_CHANGED, nullptr);

    assert(gAddressLabel = lv_label_create(gScreen));
    lv_obj_set_style_text_font(gAddressLabel, resourcesFont(RESOURCES_FONT_SIZE_NORMAL, RESOURCES_FONT_TYPE_BOLD), 0);
    lv_obj_set_style_text_opa(gAddressLabel, 0xff / 3 * 2, 0);
    lv_label_set_text_static(gAddressLabel, constsString(IP_ADDRESS));

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
}

static void netsDropdownValueChangeCallback(lv_event_t* nullable const) {
    const int selected = (int) lv_dropdown_get_selected(gNetsDropdown);
    lv_dropdown_set_text(gNetsDropdown, selected == 0 ? "One" : selected == 1 ? "Two" : "Three");

//    if (!gNetsList || !(gSelectedNet = listGet(gNetsList, (int) lv_dropdown_get_selected(gNetsDropdown)))) {
//        lv_label_set_text_static(gAddressLabel, constsString(IP_ADDRESS));
//        return;
//    }
//
//    char address[NET_ADDRESS_STRING_SIZE];
//    netAddressToString(address, gSelectedNet->host);
//    lv_label_set_text_fmt(gAddressLabel, "%s: %s", constsString(IP_ADDRESS), address);
}

void loginSceneQuit(void) {
    assert(gInitialized);
    gInitialized = false;

    if (gNetsList) listDestroy(gNetsList);

    lv_obj_delete(gSignInLabel);
    lv_obj_delete(gSignInButton);
    lv_obj_delete(gRememberCredentialsCheckbox);
    lv_obj_delete(gPasswordTextArea);
    lv_obj_delete(gAddressLabel);
    lv_obj_delete(gNetsDropdown);
    lv_obj_delete(gWelcomeLabel);
    lv_group_delete(gGroup);
    lv_obj_delete(gScreen);
}
