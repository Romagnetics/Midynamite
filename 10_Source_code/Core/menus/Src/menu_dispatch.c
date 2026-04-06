/*
 * menu_dispatch.c
 *
 *  Created on: Mar 3, 2026
 *      Author: Romain Dereu
 */

#include "_menu_ui.h"
#include "menus.h"

void ui_update_dispatch(void)
{
    const ui_element elems[] = {
        // type       save_item                  text                        x         y       ctrl_group_id
        { ELEM_TEXT,  0,                         TEXT_(midi_dispatch),       TXT_LEFT,  LINE_0, CTRL_DISPATCH_ALL },

        { ELEM_TEXT,  0,                         TEXT_(synths_),             TXT_LEFT,  LINE_1, CTRL_DISPATCH_ALL },
        { ELEM_ITEM,  DISPATCH_AMOUNT_OF_SYNTHS, TEXT_(zero_to_sixteen),    60,        LINE_1, CTRL_DISPATCH_ALL },

        { ELEM_TEXT,  0,                         TEXT_(from_ch_),            TXT_LEFT,  LINE_2, CTRL_DISPATCH_ALL },
        { ELEM_ITEM,  DISPATCH_FROM_CHANNEL,     TEXT_(zero_to_sixteen),    60,        LINE_2, CTRL_DISPATCH_ALL },

        { ELEM_TEXT,  0,                         TEXT_(notes_per_synth_),    TXT_LEFT,  LINE_3, CTRL_DISPATCH_ALL },
        { ELEM_ITEM,  DISPATCH_NOTES_PER_SYNTH,  TEXT_(zero_to_sixteen),    85,        LINE_3, CTRL_DISPATCH_ALL },

        { ELEM_TEXT,  0,                         TEXT_(voice_delete_),       TXT_LEFT,  LINE_4, CTRL_DISPATCH_ALL },
        { ELEM_ITEM,  DISPATCH_VOICE_MANAGE,     TEXT_(voice_manage_options),85,        LINE_4, CTRL_DISPATCH_ALL },
    };

    menu_ui_render(MENU_DISPATCH, elems, sizeof(elems) / sizeof(elems[0]));
}

void ui_code_dispatch() {
    midi_display_on_off(save_get(DISPATCH_CURRENTLY_SENDING), LINE_3 - 3);
    draw_line(ON_OFF_VERT_LINE, LINE_3 - 3, 127, LINE_3 - 3);

}
