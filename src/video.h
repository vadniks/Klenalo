
#pragma once

union SDL_Event;

void videoInit(void);
bool videoInitialized(void);
void videoProcessEvent(union SDL_Event* const event);
void videoQuit(void);
