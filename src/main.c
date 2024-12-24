
#include <SDL2/SDL.h>
#include "xlvgl.h"
#include "lifecycle.h"
#include "input.h"

static void buttonCallback(lv_event_t* const) {
    SDL_Log("a");
}

int main(void) {
    lifecycleInit();

    lv_font_t* font = lv_freetype_font_create(
        "res/Roboto-Regular.ttf",
        LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
        24,
        LV_FREETYPE_FONT_STYLE_NORMAL
    );

    lv_group_t* group = lv_group_create();
    lv_group_set_default(group);
    inputAssignToGroup(group);

    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x003a57), LV_PART_MAIN);

    lv_obj_t* label = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(label, font, 0);
    lv_label_set_text(label, u8" Hello world!\nПривет мир!");
    lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t* button = lv_button_create(lv_screen_active());
    lv_obj_set_pos(button, 10, 10);
    lv_obj_set_size(button, 120, 50);
    lv_obj_add_event_cb(button, buttonCallback, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* label2 = lv_label_create(button);
    lv_label_set_text(label2, "Button");
    lv_obj_center(label2);

    lv_obj_t* textArea = lv_textarea_create(lv_screen_active());
    lv_obj_set_style_text_font(textArea, font, 0);
    lv_obj_align(textArea, LV_ALIGN_TOP_MID, 100, 10);

    lifecycleLoop();

    lv_obj_delete(textArea);
    lv_obj_delete(label2);
    lv_obj_delete(button);
    lv_obj_delete(label);
    lv_group_delete(group);
    lv_freetype_font_delete(font);

    lifecycleQuit();
    return 0;
}
