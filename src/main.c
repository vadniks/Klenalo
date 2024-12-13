
#include <SDL2/SDL.h>
#include <lvgl/lvgl.h>
#include <unistd.h>
#include "defs.h"

int main(void) {
    assert(SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0"));
    assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) == 0);

    lv_init();

    lv_tick_set_cb(SDL_GetTicks);

    lv_display_t* display = lv_sdl_window_create(160 * 10, 90 * 10);

    sleep(5);

    lv_display_delete(display);

    lv_sdl_quit();

    lv_deinit();

    SDL_Quit();
    return 0;
}
