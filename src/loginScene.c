
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
static lv_obj_t* gNetsDropdown = nullptr;
static lv_obj_t* gAddressLabel = nullptr;
static lv_obj_t* gPasswordTextArea = nullptr;
static lv_obj_t* gRememberCredentialsCheckbox = nullptr;
static lv_obj_t* gSignInButton = nullptr;
static lv_obj_t* gSignInLabel = nullptr;

static List* nullable gNetsList = nullptr; // <NetNet*>
static const NetNet* gSelectedNet = nullptr;
static int gUpdateNetsTicker = 0;

static void netsDropdownValueChangeCallback(lv_event_t* nullable const);
static void updateNets(void* nullable const);

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

    assert(gNetsDropdown = lv_dropdown_create(gScreen));
    lv_dropdown_set_text(gNetsDropdown, constsString(CONSTS_STRING_NETWORK));
    lv_dropdown_clear_options(gNetsDropdown);
    lv_obj_add_event_cb(gNetsDropdown, netsDropdownValueChangeCallback, LV_EVENT_VALUE_CHANGED, nullptr);

    assert(gAddressLabel = lv_label_create(gScreen));
    lv_obj_set_style_text_font(gAddressLabel, resourcesFont(RESOURCES_FONT_SIZE_NORMAL, RESOURCES_FONT_TYPE_BOLD), 0);
    lv_obj_set_style_text_opa(gAddressLabel, 0xff / 3 * 2, 0);
    lv_label_set_text_static(gAddressLabel, constsString(CONSTS_STRING_IP_ADDRESS));

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

    lifecycleRunInMainThread(updateNets, nullptr);
}

static void updateNets(void* nullable const) {
    const bool wasOpened = lv_dropdown_is_open(gNetsDropdown);

    if (++gUpdateNetsTicker < 100) // TODO: refactor
        goto end;
    else
        gUpdateNetsTicker = 0;

    lv_dropdown_clear_options(gNetsDropdown);

    if (gNetsList) listDestroy(gNetsList);
    if (!(gNetsList = netNets())) goto end;

    for (int i = 0; i < listSize(gNetsList); i++) {
        NetNet* const net = listGet(gNetsList, i);
        if (!net->running) continue;
        lv_dropdown_add_option(gNetsDropdown, net->name, i);
    }

    if (wasOpened)
        lv_dropdown_open(gNetsDropdown); // to update the visual representation of dropdown's options while it's being opened

    end:
    lifecycleRunInMainThread(updateNets, nullptr);
}

static void netsDropdownValueChangeCallback(lv_event_t* nullable const) {
    if (!gNetsList || !(gSelectedNet = listGet(gNetsList, (int) lv_dropdown_get_selected(gNetsDropdown)))) {
        lv_label_set_text_static(gAddressLabel, constsString(CONSTS_STRING_IP_ADDRESS));
        return;
    }

    char address[NET_ADDRESS_STRING_SIZE];
    netAddressToString(address, gSelectedNet->host);
    lv_label_set_text_fmt(gAddressLabel, "%s: %s", constsString(CONSTS_STRING_IP_ADDRESS), address);

    netStartListeningNet(gSelectedNet); // TODO: test only
}

void loginSceneQuit(void) {
    assert(gInitialized);

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

    gInitialized = false;
}
