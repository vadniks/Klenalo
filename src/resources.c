
#include <SDL3/SDL.h>
#include <freetype/freetype.h>
#include "xlvgl.h"
#include "defs.h"
#include "video.h"
#include "resources.h"

#define FONT_REGULAR "../res/Roboto-Regular.ttf"
#define FONT_ITALIC "../res/Roboto-Italic.ttf"
#define FONT_BOLD "../res/Roboto-Bold.ttf"
#define FONT_BOLD_ITALIC "../res/Roboto-BoldItalic.ttf"
#define FONT_MONOSPACE "../res/SourceCodePro-Regular.ttf"

#if !defined(__CLION_IDE__) && !defined(__JETBRAINS_IDE__)
// CLion doesnt know about #embed yet, ignore the 'Invalid preprocessing directive' errors
[[gnu::section(".resources")]]
static const byte
    gRegularFont[] = {
        #embed FONT_REGULAR
    },
    gItalicFont[] = {
        #embed FONT_ITALIC
    },
    gBoldFont[] = {
        #embed FONT_BOLD
    },
    gBoldItalicFont[] = {
        #embed FONT_BOLD_ITALIC
    },
    gMonospaceFont[] = {
        #embed FONT_MONOSPACE
    };
#else
// stub
static const byte
    gRegularFont[] = {},
    gItalicFont[] = {},
    gBoldFont[] = {},
    gBoldItalicFont[] = {},
    gMonospaceFont[] = {};
#endif

static const struct {
    const char* const fileName;
    const byte* const data;
    const int size;
} gFontsData[] = {
    {FONT_REGULAR, gRegularFont, sizeof(gRegularFont)},
    {FONT_ITALIC, gItalicFont, sizeof(gItalicFont)},
    {FONT_BOLD, gBoldFont, sizeof(gBoldFont)},
    {FONT_BOLD_ITALIC, gBoldItalicFont, sizeof(gBoldItalicFont)},
    {FONT_MONOSPACE, gMonospaceFont, sizeof(gMonospaceFont)}
};

static const int FONT_SIZES = 3, FONT_TYPES = arraySize(gFontsData), FONTS = FONT_SIZES * FONT_TYPES;

static atomic bool gInitialized = false;
static bool gFreetypeInitializedInternally = false;

static lv_font_t* gFonts[FONTS] = {nullptr};

static inline int fontIndex(const ResourcesFontSize size, const ResourcesFontType type);
static void createFont(const ResourcesFontSize size, const ResourcesFontType type);

void resourcesInit(void) {
    assert(videoInitialized() && !gInitialized);
    gInitialized = true;

    gFreetypeInitializedInternally = LV_GLOBAL_DEFAULT()->ft_context;
    if (!gFreetypeInitializedInternally) lv_freetype_init(LV_FREETYPE_CACHE_FT_GLYPH_CNT);

    static const int fontSizes[FONT_SIZES] = {
        RESOURCES_FONT_SIZE_SMALL,
        RESOURCES_FONT_SIZE_NORMAL,
        RESOURCES_FONT_SIZE_LARGE
    };

    for (int sizeIndex = 0; sizeIndex < FONT_SIZES; sizeIndex++)
        for (int type = RESOURCES_FONT_TYPE_REGULAR; type < RESOURCES_FONT_TYPE_REGULAR + FONT_TYPES; type++)
            createFont(fontSizes[sizeIndex], type);

    assert(lv_theme_default_init( // there's no way to manually destroy the returned object unless using the ..._deinit(void) which doesn't require that object
        videoDisplay(),
        (lv_color_t) {0xff, 0x79, 0x29},
        (lv_color_t) {0xff, 0xe5, 0x00},
        SDL_GetSystemTheme() != SDL_SYSTEM_THEME_LIGHT,
        gFonts[fontIndex(RESOURCES_FONT_SIZE_NORMAL, RESOURCES_FONT_TYPE_REGULAR)]
    ));
}

static inline int fontIndex(const ResourcesFontSize size, const ResourcesFontType type) {
    return ((int) size / 100) * max(FONT_SIZES, FONT_TYPES) + (int) type;
}

static void createFont(const ResourcesFontSize size, const ResourcesFontType type) {
    lv_font_t* const font = lv_freetype_font_create(
        gFontsData[type].fileName,
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
    assert(gInitialized);
    gInitialized = false;

    lv_theme_default_deinit();

    for (byte i = 0; i < FONTS; lv_freetype_font_delete(gFonts[i++]));

    if (!gFreetypeInitializedInternally)
        lv_freetype_uninit();
}

export used FT_Error __wrap_FT_New_Face(
    struct FT_LibraryRec_* const library,
    const char* const filePathName,
    const FT_Long faceIndex,
    FT_Face* const aFace
) {
    for (byte i = 0; i < (byte) FONT_TYPES; i++) {
        const auto fontData = gFontsData[i];
        if (!strncmp(filePathName, fontData.fileName, 32))
            return FT_New_Memory_Face(library, fontData.data, fontData.size, faceIndex, aFace);
    }
    return 1;
}
