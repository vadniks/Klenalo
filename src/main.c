
#include <SDL2/SDL.h>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <lvgl/lvgl.h>
#pragma clang diagnostic pop
#include "defs.h"

static int gWidth = 1600, gHeight = 900;

static SDL_Window* gWindow = nullptr;
static SDL_Renderer* gRenderer = nullptr;
static SDL_Texture* gTexture = nullptr;

static int gMouseX = 0, gMouseY = 0;
static bool gMousePressed = false, gMouseWheelPressed = false;
static short gMouseWheelDiff = 0;
static unsigned gKeyboardInput = 0;

static const int UPDATE_PERIOD = 1000 / 60;

static inline void resizeBuffer(lv_display_t* display) {
    if (gTexture) SDL_DestroyTexture(gTexture);
    gTexture = SDL_CreateTexture(
        gRenderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        gWidth,
        gHeight
    );
    assert(gTexture);

    void* buffer = nullptr;
    assert(!SDL_LockTexture(gTexture, nullptr, &buffer, unusedVariableBuffer(int)));
    assert(buffer);
    SDL_UnlockTexture(gTexture);

    lv_display_set_resolution(display, gWidth, gHeight);
    lv_display_set_buffers(
        display,
        buffer, // this is possibly unsafe that the texture's internal buffer is used directly as the lvgl's display buffer (optimization) but it works just fine
        nullptr,
        gWidth * gHeight * LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_ARGB8888),
        LV_DISPLAY_RENDER_MODE_DIRECT
    );
}

static void renderCallback(lv_display_t* display, const lv_area_t*, byte*) {
    SDL_UnlockTexture(gTexture); // upload changes to video memory

    assert(!SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 0));
    assert(!SDL_RenderClear(gRenderer));
    assert(!SDL_RenderCopy(gRenderer, gTexture, nullptr, nullptr));
    SDL_RenderPresent(gRenderer);

    lv_display_flush_ready(display);
}

static void mouseCallback(lv_indev_t*, lv_indev_data_t* data) {
    data->point.x = gMouseX;
    data->point.y = gMouseY;
    data->state = gMousePressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static void mouseWheelCallback(lv_indev_t*, lv_indev_data_t* data) {
    data->enc_diff = gMouseWheelDiff;
    gMouseWheelDiff = 0;
    data->state = gMouseWheelPressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static void keyboardCallback(lv_indev_t*, lv_indev_data_t* data) {
    if (!gKeyboardInput) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    data->state = LV_INDEV_STATE_PRESSED;
    data->key = gKeyboardInput;
}

static void buttonCallback(lv_event_t*) {
    SDL_Log("a");
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
    SDL_SetWindowMinimumSize(gWindow, gWidth, gHeight);

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
    lv_display_set_antialiasing(display, true);
    resizeBuffer(display);
    lv_display_set_flush_cb(display, renderCallback);

    float hdpi, vdpi;
    assert(!SDL_GetDisplayDPI(0, nullptr, &hdpi, &vdpi));
    lv_display_set_dpi(display, (int) min(hdpi, vdpi));

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

    lv_font_t* font = lv_freetype_font_create(
        "res/Roboto-Regular.ttf",
        LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
        24,
        LV_FREETYPE_FONT_STYLE_NORMAL
    );

    lv_group_t* group = lv_group_create();
    lv_group_set_default(group);
    lv_indev_set_group(mouse, group);
    lv_indev_set_group(mouseWheel, group);
    lv_indev_set_group(keyboard, group);

    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x003a57), LV_PART_MAIN);

    lv_obj_t* label = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(label, font, 0);
    lv_label_set_text(label, u8" Hello world!\nПривет мир!");
    lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t* button = lv_button_create(lv_screen_active());
    lv_obj_set_pos(button, 10, 10);
    lv_obj_set_size(button, 120, 50);
    lv_obj_add_event_cb(button, buttonCallback, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* label2 = lv_label_create(button);
    lv_label_set_text(label2, "Button");
    lv_obj_center(label2);

    lv_obj_t* textArea = lv_textarea_create(lv_screen_active());
    lv_obj_set_style_text_font(textArea, font, 0);
    lv_obj_align(textArea, LV_ALIGN_TOP_MID, 100, 10);

    unsigned startMillis, differenceMillis;
    while (true) {
        startMillis = SDL_GetTicks();
        lv_timer_periodic_handler();

        SDL_Event event;
        while (SDL_PollEvent(&event) == 1) {
            switch (event.type) {
                case SDL_QUIT:
                    goto endLoop;
                case SDL_MOUSEMOTION:
                    gMouseX = event.motion.x;
                    gMouseY = event.motion.y;
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

                    if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        SDL_GetRendererOutputSize(gRenderer, &gWidth, &gHeight);
                        resizeBuffer(display);
                    }
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
                            break;
                        case SDLK_LEFT:
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
                        case SDLK_KP_ENTER:
                        case SDLK_RETURN:
                            key = LV_KEY_ENTER;
                            break;
                        case SDLK_TAB:
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

                    if (key) {
                        gKeyboardInput = key;
                        lv_indev_read(keyboard);
                    }
                    break;
                case SDL_KEYUP:
                    gKeyboardInput = 0;
                    lv_indev_read(keyboard);
                    break;
                case SDL_TEXTINPUT:
                    gKeyboardInput = *((int*) event.text.text);
                    lv_indev_read(keyboard);
                    break;
            }
        }

        differenceMillis = SDL_GetTicks() - startMillis;
        if (UPDATE_PERIOD > differenceMillis)
            SDL_Delay(UPDATE_PERIOD - differenceMillis);
//        SDL_Log("fps %d", (int) (1000.0f / (float) (SDL_GetTicks() - startMillis)));
    }
    endLoop:

    lv_obj_delete(textArea);
    lv_obj_delete(label2);
    lv_obj_delete(button);
    lv_obj_delete(label);
    lv_group_delete(group);
    lv_freetype_font_delete(font);
    lv_indev_delete(mouse);
    lv_indev_delete(mouseWheel);
    lv_indev_delete(keyboard);
    lv_display_delete(display);
    lv_deinit();

    SDL_DestroyTexture(gTexture);
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);

    SDL_Quit();
    return 0;
}
