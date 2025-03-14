
#pragma once

struct _lv_font_t;

typedef enum { // xyy: x - index, yy = size
    RESOURCES_FONT_SIZE_SMALL = 210,
    RESOURCES_FONT_SIZE_NORMAL = 114,
    RESOURCES_FONT_SIZE_LARGE = 20
} ResourcesFontSize;

typedef enum {
    RESOURCES_FONT_TYPE_REGULAR,
    RESOURCES_FONT_TYPE_ITALIC,
    RESOURCES_FONT_TYPE_BOLD,
    RESOURCES_FONT_TYPE_BOLD_ITALIC = RESOURCES_FONT_TYPE_ITALIC | RESOURCES_FONT_TYPE_BOLD,
    RESOURCES_FONT_TYPE_MONOSPACE
} ResourcesFontType;

void resourcesInit(void);
const struct _lv_font_t* resourcesFont(const ResourcesFontSize size, const ResourcesFontType type);
void resourcesQuit(void);
