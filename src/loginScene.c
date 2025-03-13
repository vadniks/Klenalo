
#include "xlvgl.h"
#include "defs.h"
#include "scenes.h"
#include "consts.h"
#include "input.h"
#include "resources.h"
#include "net.h"
#include "lifecycle.h"
#include "loginScene.h"

static const int FETCH_SUBNETS_HOSTS_ADDRESSES_TICKER_PERIOD = 100;

static atomic bool gInitialized = false;

static lv_obj_t* gScreen = nullptr;
static lv_group_t* gGroup = nullptr;
static lv_obj_t* gWelcomeLabel = nullptr;
static lv_obj_t* gSubnetsHostsAddressesDropdown = nullptr;
static lv_obj_t* gPasswordTextArea = nullptr;
static lv_obj_t* gRememberCredentialsCheckbox = nullptr;
static lv_obj_t* gSignInButton = nullptr;
static lv_obj_t* gSignInLabel = nullptr;

static List* nullable gSubnetsHostsAddressesList = nullptr; // <int>
static int gSelectedSubnetHostAddress = 0;
static int gFetchSubnetsHostsAddressesTicker = 0;

static void subnetsHostsAddressesDropdownValueChangeCallback(lv_event_t* nullable const);
static void fetchSubnetsHostsAddresses(void* nullable const);

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

    assert(gSubnetsHostsAddressesDropdown = lv_dropdown_create(gScreen));
    lv_dropdown_set_text(gSubnetsHostsAddressesDropdown, constsString(CONSTS_STRING_SUBNET_HOST_ADDRESS));
    lv_obj_set_width(gSubnetsHostsAddressesDropdown, lv_obj_get_width(gSubnetsHostsAddressesDropdown) * 4 / 3);
    lv_dropdown_clear_options(gSubnetsHostsAddressesDropdown);
    lv_obj_add_event_cb(gSubnetsHostsAddressesDropdown, subnetsHostsAddressesDropdownValueChangeCallback, LV_EVENT_VALUE_CHANGED, nullptr);

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

    lifecycleRunInMainThread(fetchSubnetsHostsAddresses, nullptr);
}

static void fetchSubnetsHostsAddresses(void* nullable const) {
    const bool wasOpened = lv_dropdown_is_open(gSubnetsHostsAddressesDropdown);

    if (++gFetchSubnetsHostsAddressesTicker < FETCH_SUBNETS_HOSTS_ADDRESSES_TICKER_PERIOD) // TODO: refactor
        goto end;
    else
        gFetchSubnetsHostsAddressesTicker = 0;

    lv_dropdown_clear_options(gSubnetsHostsAddressesDropdown);

    if (gSubnetsHostsAddressesList) listDestroy(gSubnetsHostsAddressesList);
    if (!(gSubnetsHostsAddressesList = netSubnetsHostsAddresses())) goto end;

    for (int i = 0; i < listSize(gSubnetsHostsAddressesList); i++) {
        const int subnetHostAddress = (int) (long) listGet(gSubnetsHostsAddressesList, i);

        char option[NET_ADDRESS_STRING_SIZE];
        netAddressToString(option, subnetHostAddress);

        lv_dropdown_add_option(gSubnetsHostsAddressesDropdown, option, i);
    }

    if (wasOpened)
        lv_dropdown_open(gSubnetsHostsAddressesDropdown); // to update the visual representation of dropdown's options while it's being opened

    end:
    lifecycleRunInMainThread(fetchSubnetsHostsAddresses, nullptr);
}

static void subnetsHostsAddressesDropdownValueChangeCallback(lv_event_t* nullable const) {
    if (!gSubnetsHostsAddressesList) return;
    gSelectedSubnetHostAddress = (int) (long) listGet(
        gSubnetsHostsAddressesList,
        (int) lv_dropdown_get_selected(gSubnetsHostsAddressesDropdown)
    );
    if (!gSelectedSubnetHostAddress) return;

    netStartBroadcastingAndListeningSubnet(gSelectedSubnetHostAddress); // TODO: test only
}

void loginSceneQuit(void) {
    assert(gInitialized);

    if (gSubnetsHostsAddressesList) listDestroy(gSubnetsHostsAddressesList);

    lv_obj_delete(gSignInLabel);
    lv_obj_delete(gSignInButton);
    lv_obj_delete(gRememberCredentialsCheckbox);
    lv_obj_delete(gPasswordTextArea);
    lv_obj_delete(gSubnetsHostsAddressesDropdown);
    lv_obj_delete(gWelcomeLabel);
    lv_group_delete(gGroup);
    lv_obj_delete(gScreen);

    gInitialized = false;
}
