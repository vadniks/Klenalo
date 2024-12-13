
#pragma once

union SDL_Event;

void renderInit(void);

void renderInputBegan(void);
void renderProcessEvent(union SDL_Event* event);
void renderInputEnded(void);

void renderDraw(void);

void renderClean(void);
