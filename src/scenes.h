
#pragma once

typedef enum {
    SCENES_SCENE_NONE,
    SCENES_SCENE_SPLASH,
    SCENES_SCENE_LOGIN
} ScenesScene;

struct _lv_obj_t;

void scenesInit(void);
bool scenesInitialized(void);
ScenesScene scenesCurrentScene(void);
void scenesSetCurrentScene(const ScenesScene scene);
void scenesLoadScreen(struct _lv_obj_t* const screen);
void scenesQuit(void);
