
#pragma once

union SDL_Event;
struct _lv_display_t;

void videoInit(void);
bool videoInitialized(void);
struct _lv_display_t* videoDisplay(void);
void videoProcessEvent(const union SDL_Event* const event);
void videoQuit(void);
