
#include <SDL2/SDL.h>
#include <lvgl/lvgl.h>
#include <unistd.h>
#include "defs.h"

static const int WIDTH = 1600, HEIGHT = 900;

static SDL_Window* gWindow = nullptr;
static SDL_Renderer* gRenderer = nullptr;
static SDL_Texture* gTexture = nullptr;

static byte gBuffer[WIDTH * HEIGHT * LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_ARGB8888)];

static void flushCallback(lv_display_t* display, const lv_area_t*, byte*) {
    void* texturePixels = nullptr;
    int texturePitch = 0;
    assert(!SDL_LockTexture(gTexture, nullptr, &texturePixels, &texturePitch));
    assert(texturePitch == WIDTH * 4);

    SDL_memcpy(texturePixels, gBuffer, sizeof(gBuffer));

    SDL_UnlockTexture(gTexture);

    lv_display_flush_ready(display);

    assert(!SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 0));
    assert(!SDL_RenderClear(gRenderer));
    assert(!SDL_RenderCopy(gRenderer, gTexture, nullptr, nullptr));
    SDL_RenderPresent(gRenderer);
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
        WIDTH,
        HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI
    );
    assert(gWindow);

    gRenderer = SDL_CreateRenderer(
        gWindow,
        0,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    assert(gRenderer);
    assert(!SDL_RenderSetLogicalSize(gRenderer, WIDTH, HEIGHT));
    assert(!SDL_RenderSetScale(gRenderer, 1.0f, 1.0f));

    gTexture = SDL_CreateTexture(
        gRenderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        WIDTH,
        HEIGHT
    );
    assert(gTexture);

    lv_init();

    lv_tick_set_cb(SDL_GetTicks);

    lv_display_t* display = lv_display_create(WIDTH, HEIGHT);

    lv_display_set_color_format(display, LV_COLOR_FORMAT_ARGB8888);

    lv_display_set_buffers(display, gBuffer, nullptr, sizeof(gBuffer), LV_DISPLAY_RENDER_MODE_DIRECT);

    lv_display_set_flush_cb(display, flushCallback);

//    lv_display_set_resolution() // TODO

    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x003a57), LV_PART_MAIN);
    lv_obj_t* label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello world");
    lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    // TODO: delete label

    while (true) {
        /*SDL_Delay();*/ printf("%u\n", lv_timer_handler());

        SDL_Event event;
        while (SDL_PollEvent(&event) == 1) {
            if (event.type == SDL_QUIT)
                goto endLoop;
        }


    }
    endLoop:

    lv_display_delete(display);

    lv_deinit();

    SDL_DestroyTexture(gTexture);

    SDL_DestroyRenderer(gRenderer);

    SDL_DestroyWindow(gWindow);

    SDL_Quit();
    return 0;
}
