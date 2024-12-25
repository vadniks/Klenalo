
#include "xlvgl.h"
#include "defs.h"
#include "lifecycle.h"
#include "resources.h"

static atomic bool gInitialized = false;

static struct _lv_font_t* gFontLarge = nullptr;
static struct _lv_font_t* gFontNormal = nullptr;
static struct _lv_font_t* gFontSmall = nullptr;

static void createFont(lv_font_t** const variable, const unsigned size);

void resourcesInit(void) {
    assert(lifecycleInitialized() && !gInitialized);
    gInitialized = true;

    createFont(&gFontLarge, 20);
    createFont(&gFontNormal, 14);
    createFont(&gFontSmall, 10);
}

static void createFont(lv_font_t** const variable, const unsigned size) {
    *variable = lv_freetype_font_create(
        "res/Roboto-Regular.ttf",
        LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
        size,
        LV_FREETYPE_FONT_STYLE_NORMAL
    );
}

const lv_font_t* resourcesFontLarge(void) {
    return gFontLarge;
}

const lv_font_t* resourcesFontNormal(void) {
    return gFontNormal;
}

const lv_font_t* resourcesFontSmall(void) {
    return gFontSmall;
}

void resourcesQuit(void) {
    assert(gInitialized);
    gInitialized = false;

    lv_freetype_font_delete(gFontLarge);
    lv_freetype_font_delete(gFontNormal);
    lv_freetype_font_delete(gFontSmall);
}
