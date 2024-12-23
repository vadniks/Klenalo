
#pragma once

union SDL_Event;
struct _lv_group_t;

void inputInit(void);
bool inputInitialized(void);
void inputAssignToGroup(struct _lv_group_t* const group);
void inputProcessEvent(union SDL_Event* const event);
void inputQuit(void);
