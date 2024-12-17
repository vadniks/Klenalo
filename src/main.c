
#include <SDL2/SDL.h>
#include <lvgl/lvgl.h>
#include "defs.h"

static const int KEYBOARD_INPUT_BUFFER_SIZE = KEYBOARD_BUFFER_SIZE;

static int gWidth = 1600, gHeight = 900;

static SDL_Window* gWindow = nullptr;
static SDL_Renderer* gRenderer = nullptr;
static SDL_Texture* gTexture = nullptr;

static byte* gBuffer = nullptr;

static int gMouseX = 0, gMouseY = 0;
static bool gMousePressed = false;
static short gMouseWheelDiff = 0;
static bool gMouseWheelPressed = false;
static char gKeyboardInputBuffer[KEYBOARD_INPUT_BUFFER_SIZE] = {0};
static bool gKeyboardDummyRead = false;

static inline int bufferSize() { return gWidth * gHeight * LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_ARGB8888); }

static inline void resizeBuffer(lv_display_t* display) {
    gBuffer = SDL_realloc(gBuffer, bufferSize());
    assert(gBuffer);

    lv_display_set_resolution(display, gWidth, gHeight);
    lv_display_set_buffers(display, gBuffer, nullptr, bufferSize(), LV_DISPLAY_RENDER_MODE_DIRECT);

    if (gTexture) SDL_DestroyTexture(gTexture);
    gTexture = SDL_CreateTexture(
        gRenderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        gWidth,
        gHeight
    );
    assert(gTexture);
}

static void renderCallback(lv_display_t* display, const lv_area_t*, byte*) {
    void* texturePixels = nullptr;
    int texturePitch = 0;
    assert(!SDL_LockTexture(gTexture, nullptr, &texturePixels, &texturePitch));
    assert(texturePitch == gWidth * 4);

    SDL_memcpy(texturePixels, gBuffer, bufferSize()); // TODO: resize sdl texture

    SDL_UnlockTexture(gTexture);

    lv_display_flush_ready(display);

    assert(!SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 0));
    assert(!SDL_RenderClear(gRenderer));
    assert(!SDL_RenderCopy(gRenderer, gTexture, nullptr, nullptr));
    SDL_RenderPresent(gRenderer);
}

static void mouseCallback(lv_indev_t*, lv_indev_data_t* data) {
    data->point.x = gMouseX;
    data->point.y = gMouseY;
    data->state = gMousePressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static void mouseWheelCallback(lv_indev_t*, lv_indev_data_t* data) {
    data->enc_diff = gMouseWheelDiff;
    data->state = gMouseWheelPressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static void keyboardCallback(lv_indev_t*, lv_indev_data_t* data) {
    const unsigned long length = SDL_strlen(gKeyboardInputBuffer);

    if (gKeyboardDummyRead) {
        gKeyboardDummyRead = false;
        data->state = LV_INDEV_STATE_RELEASED;
    } else if (length > 0) {
        gKeyboardDummyRead = true;
        data->state = LV_INDEV_STATE_PRESSED;
        data->key = (unsigned) gKeyboardInputBuffer[0];
        memmove(gKeyboardInputBuffer, gKeyboardInputBuffer + 1, length); // not SDL_memmove (src overflow handled)
    }
}

int main(void) {
    assert(SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0"));
    assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) == 0);

    assert(SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl"));
    assert(SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1"));
    assert(SDL_SetHint(SDL_HINT_RENDER_LINE_METHOD, "3"));
    assert(SDL_SetHint(SDL_HINT_RENDER_OPENGL_SHADERS, "1"));
    assert(SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2"));
    assert(SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1"));

    gWindow = SDL_CreateWindow(
        "LVGL",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        gWidth,
        gHeight,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );
    assert(gWindow);

    gRenderer = SDL_CreateRenderer(
        gWindow,
        0,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    assert(gRenderer);
    assert(!SDL_RenderSetScale(gRenderer, 1.0f, 1.0f));

    lv_init();
    lv_tick_set_cb(SDL_GetTicks);
    lv_delay_set_cb(SDL_Delay);

    lv_display_t* display = lv_display_create(gWidth, gHeight);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_ARGB8888);
    resizeBuffer(display);
    lv_display_set_flush_cb(display, renderCallback);

    lv_indev_t* mouse = lv_indev_create();
    lv_indev_set_type(mouse, LV_INDEV_TYPE_POINTER);
    lv_indev_set_mode(mouse, LV_INDEV_MODE_EVENT);
    lv_indev_set_read_cb(mouse, mouseCallback);

    lv_indev_t* mouseWheel = lv_indev_create();
    lv_indev_set_type(mouseWheel, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_mode(mouseWheel, LV_INDEV_MODE_EVENT);
    lv_indev_set_read_cb(mouseWheel, mouseWheelCallback);

    lv_indev_t* keyboard = lv_indev_create();
    lv_indev_set_type(keyboard, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_mode(keyboard, LV_INDEV_MODE_EVENT);
    lv_indev_set_read_cb(keyboard, keyboardCallback);

    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x003a57), LV_PART_MAIN);
    lv_obj_t* label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello world");
    lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    while (true) {
        lv_timer_periodic_handler();

        SDL_Event event;
        bool keyboardEvent = false;

        while (SDL_PollEvent(&event) == 1) {
            switch (event.type) {
                case SDL_QUIT:
                    goto endLoop;
                case SDL_MOUSEMOTION:
                    gMouseX = event.motion.xrel;
                    gMouseY = event.motion.yrel;
                    lv_indev_read(mouse);
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    switch (event.button.button) {
                        case SDL_BUTTON_LEFT:
                            gMousePressed = true;
                            lv_indev_read(mouse);
                            break;
                        case SDL_BUTTON_MIDDLE:
                            gMouseWheelPressed = true;
                            lv_indev_read(mouseWheel);
                            break;
                    }
                    break;
                case SDL_MOUSEBUTTONUP:
                    switch (event.button.button) {
                        case SDL_BUTTON_LEFT:
                            gMousePressed = false;
                            lv_indev_read(mouse);
                            break;
                        case SDL_BUTTON_MIDDLE:
                            gMouseWheelPressed = false;
                            lv_indev_read(mouseWheel);
                            break;
                    }
                    break;
                case SDL_WINDOWEVENT:
                    gMousePressed = false;
                    lv_indev_read(mouse);
                    break;
                case SDL_MOUSEWHEEL:
                    gMouseWheelDiff = (short) -(event.wheel.y);
                    lv_indev_read(mouseWheel);
                    break;
                case SDL_KEYDOWN:
                    unsigned key = 0;
                    switch (event.key.keysym.sym) {
                        case SDLK_RIGHT:
                        case SDLK_KP_PLUS:
                            key = LV_KEY_RIGHT;
                        case SDLK_LEFT:
                        case SDLK_KP_MINUS:
                            key = LV_KEY_LEFT;
                        case SDLK_UP:
                            key = LV_KEY_UP;
                        case SDLK_DOWN:
                            key = LV_KEY_DOWN;
                        case SDLK_ESCAPE:
                            key = LV_KEY_ESC;
                        case SDLK_BACKSPACE:
                            key = LV_KEY_BACKSPACE;
                        case SDLK_DELETE:
                            key = LV_KEY_DEL;
                        case SDLK_KP_ENTER:
                        case SDLK_RETURN:
                            key = LV_KEY_ENTER;
                        case SDLK_TAB:
                        case SDLK_PAGEDOWN:
                            key = LV_KEY_NEXT;
                        case SDLK_PAGEUP:
                            key = LV_KEY_PREV;
                        case SDLK_HOME:
                            key = LV_KEY_HOME;
                        case SDLK_END:
                            key = LV_KEY_END;
                    }

                    if (!key)
                        break;

                    const int length = (int) SDL_strlen(gKeyboardInputBuffer);
                    if (length < KEYBOARD_INPUT_BUFFER_SIZE - 1) {
                        gKeyboardInputBuffer[length] = (char) key;
                        gKeyboardInputBuffer[length + 1] = 0;
                    }

                    keyboardEvent = true;
                    break;
                case SDL_TEXTINPUT:
                    if (SDL_strlen(gKeyboardInputBuffer) + SDL_strlen(event.text.text) < KEYBOARD_INPUT_BUFFER_SIZE - 1)
                        strcat(gKeyboardInputBuffer, event.text.text);
                    keyboardEvent = true;
                    break;
            }

            if (keyboardEvent) {
                int length = (int) SDL_strlen(gKeyboardInputBuffer);
                while (length) {
                    lv_indev_read(keyboard);
                    lv_indev_read(keyboard);
                    length--;
                }
            }
        }

        SDL_GetRendererOutputSize(gRenderer, &gWidth, &gHeight);
        resizeBuffer(display);
    }
    endLoop:

    lv_indev_delete(mouse);
    lv_indev_delete(mouseWheel);
    lv_indev_delete(keyboard);
    lv_obj_delete(label);
    SDL_free(gBuffer);
    lv_display_delete(display);
    lv_deinit();

    SDL_DestroyTexture(gTexture);
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);

    SDL_Quit();
    return 0;
}
