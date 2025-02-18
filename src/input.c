
#include <SDL3/SDL.h>
#include "xlvgl.h"
#include "defs.h"
#include "video.h"
#include "input.h"

static atomic bool gInitialized = false;

static lv_indev_t* gMouse = nullptr;
static lv_indev_t* gMouseWheel = nullptr;
static lv_indev_t* gKeyboard = nullptr;

static int gMouseX = 0, gMouseY = 0;
static bool gMousePressed = false, gMouseWheelPressed = false;
static short gMouseWheelDiff = 0;
static unsigned gKeyboardInput = 0;

static void processMouse(lv_indev_t*, lv_indev_data_t* data);
static void processMouseWheel(lv_indev_t*, lv_indev_data_t* data);
static void processKeyboard(lv_indev_t*, lv_indev_data_t* data);

void inputInit(void) {
    assert(videoInitialized() && !gInitialized);
    gInitialized = true;

    assert(gMouse = lv_indev_create());
    lv_indev_set_type(gMouse, LV_INDEV_TYPE_POINTER);
    lv_indev_set_mode(gMouse, LV_INDEV_MODE_EVENT);
    lv_indev_set_read_cb(gMouse, processMouse);

    assert(gMouseWheel = lv_indev_create());
    lv_indev_set_type(gMouseWheel, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_mode(gMouseWheel, LV_INDEV_MODE_EVENT);
    lv_indev_set_read_cb(gMouseWheel, processMouseWheel);

    assert(gKeyboard = lv_indev_create());
    lv_indev_set_type(gKeyboard, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_mode(gKeyboard, LV_INDEV_MODE_EVENT);
    lv_indev_set_read_cb(gKeyboard, processKeyboard);
}

static void processMouse(lv_indev_t* const, lv_indev_data_t* const data) {
    assert(videoInitialized() && gInitialized);

    data->point.x = gMouseX;
    data->point.y = gMouseY;
    data->state = gMousePressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static void processMouseWheel(lv_indev_t* const, lv_indev_data_t* const data) {
    assert(videoInitialized() && gInitialized);

    data->enc_diff = gMouseWheelDiff;
    gMouseWheelDiff = 0;
    data->state = gMouseWheelPressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static void processKeyboard(lv_indev_t* const, lv_indev_data_t* const data) {
    assert(videoInitialized() && gInitialized);

    if (!gKeyboardInput) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    data->state = LV_INDEV_STATE_PRESSED;
    data->key = gKeyboardInput;
}

bool inputInitialized(void) {
    return gInitialized;
}

void inputAssignToGroup(lv_group_t* const group) {
    assert(videoInitialized() && gInitialized);

    lv_indev_set_group(gMouse, group);
    lv_indev_set_group(gMouseWheel, group);
    lv_indev_set_group(gKeyboard, group);
}

static void processMouseButton(const SDL_Event* const event, bool const down) {
    switch (event->button.button) {
        case SDL_BUTTON_LEFT:
            gMousePressed = down;
            lv_indev_read(gMouse);
            break;
        case SDL_BUTTON_MIDDLE:
            gMouseWheelPressed = down;
            lv_indev_read(gMouseWheel);
            break;
    }
}

static void processKeyDown(const SDL_Event* const event) {
    unsigned key = 0;

    switch (event->key.key) {
        case SDLK_RIGHT: fallthrough
        case SDLK_KP_PLUS:
            key = LV_KEY_RIGHT;
            break;
        case SDLK_LEFT: fallthrough
        case SDLK_KP_MINUS:
            key = LV_KEY_LEFT;
            break;
        case SDLK_UP:
            key = LV_KEY_UP;
            break;
        case SDLK_DOWN:
            key = LV_KEY_DOWN;
            break;
        case SDLK_ESCAPE:
            key = LV_KEY_ESC;
            break;
        case SDLK_BACKSPACE:
            key = LV_KEY_BACKSPACE;
            break;
        case SDLK_DELETE:
            key = LV_KEY_DEL;
            break;
        case SDLK_KP_ENTER: fallthrough
        case SDLK_RETURN:
            key = LV_KEY_ENTER;
            break;
        case SDLK_TAB: fallthrough
        case SDLK_PAGEDOWN:
            key = LV_KEY_NEXT;
            break;
        case SDLK_PAGEUP:
            key = LV_KEY_PREV;
            break;
        case SDLK_HOME:
            key = LV_KEY_HOME;
            break;
        case SDLK_END:
            key = LV_KEY_END;
            break;
    }

    if (!key) return;
    gKeyboardInput = key;
    lv_indev_read(gKeyboard);
}

void inputProcessEvent(const SDL_Event* const event) {
    assert(videoInitialized() && gInitialized);
    switch (event->type) {
        case SDL_EVENT_MOUSE_MOTION:
            gMouseX = (int) event->motion.x;
            gMouseY = (int) event->motion.y;
            lv_indev_read(gMouse);
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            processMouseButton(event, true);
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            processMouseButton(event, false);
            break;
        case SDL_EVENT_WINDOW_MOUSE_ENTER: fallthrough
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
            gMousePressed = false;
            gMouseWheelPressed = false;
            gKeyboardInput = 0;
            lv_indev_read(gMouse);
            lv_indev_read(gMouseWheel);
            lv_indev_read(gKeyboard);
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            gMouseWheelDiff = (short) -(event->wheel.y);
            lv_indev_read(gMouseWheel);
            break;
        case SDL_EVENT_KEY_DOWN:
            processKeyDown(event);
            break;
        case SDL_EVENT_KEY_UP:
            gKeyboardInput = 0;
            lv_indev_read(gKeyboard);
            break;
        case SDL_EVENT_TEXT_INPUT:
            strncpy((char*) &gKeyboardInput, event->text.text, sizeof(int));
            lv_indev_read(gKeyboard);
            break;
    }
}

void inputQuit(void) {
    assert(gInitialized);
    gInitialized = false;

    lv_indev_delete(gMouse);
    lv_indev_delete(gMouseWheel);
    lv_indev_delete(gKeyboard);
}
