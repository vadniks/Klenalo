
#pragma once

typedef enum {
    SCENES_SCENE_NONE,
    SCENES_SCENE_SPLASH,
    SCENES_SCENE_LOGIN
} ScenesScene;

void scenesInit(void);
bool scenesInitialized(void);
ScenesScene scenesCurrentScene(void);
void scenesSetCurrentScene(const ScenesScene scene);
void scenesQuit(void);
