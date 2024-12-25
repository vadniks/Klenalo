
#pragma once

struct _lv_font_t;

void resourcesInit(void);
const struct _lv_font_t* resourcesFontLarge(void);
const struct _lv_font_t* resourcesFontNormal(void);
const struct _lv_font_t* resourcesFontSmall(void);
void resourcesQuit(void);
