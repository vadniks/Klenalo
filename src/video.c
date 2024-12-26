
#include <SDL2/SDL.h>
#include "xlvgl.h"
#include "defs.h"
#include "lifecycle.h"
#include "consts.h"
#include "video.h"

static atomic bool gInitialized = false;

static int gWidth = 1600, gHeight = 900;

static SDL_Window* gWindow = nullptr;
static SDL_Renderer* gRenderer = nullptr;
static SDL_Texture* gTexture = nullptr;

static lv_display_t* gDisplay = nullptr;

static void resizeBuffer(lv_display_t* const display);
static void render(lv_display_t* const display, const lv_area_t* const, byte* const);

void videoInit(void) {
    assert(lifecycleInitialized() && !gInitialized);
    gInitialized = true;

    assert(SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl"));
    assert(SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1"));
    assert(SDL_SetHint(SDL_HINT_RENDER_LINE_METHOD, "3"));
    assert(SDL_SetHint(SDL_HINT_RENDER_OPENGL_SHADERS, "1"));
    assert(SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2"));
    assert(SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1"));

    assert(gWindow = SDL_CreateWindow(
        constsString(TITLE),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        gWidth,
        gHeight,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    ));
    SDL_SetWindowMinimumSize(gWindow, gWidth, gHeight);

    assert(gRenderer = SDL_CreateRenderer(
        gWindow,
        0,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    ));
    assert(!SDL_RenderSetScale(gRenderer, 1.0f, 1.0f));

    assert(gDisplay = lv_display_create(gWidth, gHeight));
    lv_display_set_color_format(gDisplay, LV_COLOR_FORMAT_ARGB8888);
    lv_display_set_antialiasing(gDisplay, true);
    resizeBuffer(gDisplay);
    lv_display_set_flush_cb(gDisplay, render);

    float hdpi, vdpi;
    assert(!SDL_GetDisplayDPI(0, nullptr, &hdpi, &vdpi));
    lv_display_set_dpi(gDisplay, (int) min(hdpi, vdpi));
}

static void resizeBuffer(lv_display_t* const display) {
    if (gTexture) SDL_DestroyTexture(gTexture);
    assert(gTexture = SDL_CreateTexture(
        gRenderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        gWidth,
        gHeight
    ));

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

static void render(lv_display_t* const display, const lv_area_t* const, byte* const) {
    SDL_UnlockTexture(gTexture); // upload changes to video memory

    assert(!SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 0));
    assert(!SDL_RenderClear(gRenderer));
    assert(!SDL_RenderCopy(gRenderer, gTexture, nullptr, nullptr));
    SDL_RenderPresent(gRenderer);

    lv_display_flush_ready(display);
}

bool videoInitialized(void) {
    return gInitialized;
}

void videoProcessEvent(const SDL_Event* const event) {
    assert(lifecycleInitialized() && gInitialized);
    if (event->type != SDL_WINDOWEVENT || event->window.event != SDL_WINDOWEVENT_SIZE_CHANGED) return;
    SDL_GetRendererOutputSize(gRenderer, &gWidth, &gHeight);
    resizeBuffer(gDisplay);
}

void videoQuit(void) {
    assert(gInitialized);
    gInitialized = false;

    lv_display_delete(gDisplay);

    SDL_DestroyTexture(gTexture);
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
}
