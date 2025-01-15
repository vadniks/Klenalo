
#include "xlvgl.h"
#include "defs.h"
#include "video.h"
#include "resources.h"

static const int FONT_SIZES = 3, FONT_TYPES = 4, FONTS = FONT_SIZES * FONT_TYPES;

static atomic bool gInitialized = false;
static bool gFreetypeInitializedInternally = false;

static lv_font_t* gFonts[FONTS] = {nullptr};

static inline int fontIndex(const ResourcesFontSize size, const ResourcesFontType type);
static void createFont(const ResourcesFontSize size, const ResourcesFontType type);

void resourcesInit(void) {
    assert(videoInitialized() && !gInitialized);
    gInitialized = true;

    gFreetypeInitializedInternally = !lv_freetype_init(LV_FREETYPE_CACHE_FT_GLYPH_CNT);

    static const int fontSizes[FONT_SIZES] = {
        RESOURCES_FONT_SIZE_SMALL,
        RESOURCES_FONT_SIZE_NORMAL,
        RESOURCES_FONT_SIZE_LARGE,
    };

    for (int sizeIndex = 0; sizeIndex < FONT_SIZES; sizeIndex++)
        for (int type = RESOURCES_FONT_TYPE_REGULAR; type < RESOURCES_FONT_TYPE_REGULAR + FONT_TYPES; type++)
            createFont(fontSizes[sizeIndex], type);

    assert(lv_theme_default_init( // there's no way to manually destroy the returned object unless using the ..._deinit(void) which doesn't require that object
        videoDisplay(),
        (lv_color_t) {0xff, 0x79, 0x29},
        (lv_color_t) {0xff, 0xe5, 0x00},
        true,
        gFonts[fontIndex(RESOURCES_FONT_SIZE_NORMAL, RESOURCES_FONT_TYPE_REGULAR)]
    ));
}

static inline int fontIndex(const ResourcesFontSize size, const ResourcesFontType type) {
    return (size / 100) * max(FONT_SIZES, FONT_TYPES) + type;
}

static void createFont(const ResourcesFontSize size, const ResourcesFontType type) {
    static const char* const paths[FONT_TYPES] = {
        "res/Roboto-Regular.ttf",
        "res/Roboto-Italic.ttf",
        "res/Roboto-Bold.ttf",
        "res/Roboto-BoldItalic.ttf"
    };

    lv_font_t* font = lv_freetype_font_create(
        paths[type],
        LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
        size % 100,
        (lv_freetype_font_style_t) type
    );
    assert(font);

    gFonts[fontIndex(size, type)] = font;
}

const lv_font_t* resourcesFont(const ResourcesFontSize size, const ResourcesFontType type) {
    assert(videoInitialized() && gInitialized);
    return gFonts[fontIndex(size, type)];
}

void resourcesQuit(void) {
    assert(videoInitialized() && gInitialized);
    gInitialized = false;

    lv_theme_default_deinit();

    for (byte i = 0; i < FONTS; lv_freetype_font_delete(gFonts[i++]));

    if (!gFreetypeInitializedInternally)
        lv_freetype_uninit();
}
