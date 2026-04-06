/*
 * menu_transpose.c
 *
 *  Created on: Jul 10, 2025
 *      Author: Romain Dereu
 */
#include "_menu_ui.h"
#include "menus.h"

void ui_update_transpose(void)
{
    const ui_element elems[] = {
        // type       save_item                 text                          x         y       ctrl_group_id
        { ELEM_TEXT,  0,                        TEXT_(midi_transpose),      TXT_LEFT,  LINE_0, CTRL_TRANSPOSE_ALL },

        // ---------- SHIFT page ----------
        { ELEM_TEXT,  0,                        TEXT_(shift_by),           TXT_LEFT,  LINE_1, CTRL_TRANSPOSE_SHIFT },
        { ELEM_ITEM,  TRANSPOSE_MIDI_SHIFT_VALUE,TEXT_(neg_pos_80 + (80 - 36)), 65, LINE_1, CTRL_TRANSPOSE_SHIFT },
        { ELEM_TEXT,  0,                        TEXT_(semitones),          TXT_LEFT,  LINE_2, CTRL_TRANSPOSE_SHIFT },

        // ---------- SCALED page ----------
        { ELEM_TEXT,  0,                        TEXT_(root_note),          TXT_LEFT,  LINE_1, CTRL_TRANSPOSE_SCALED },
        { ELEM_ITEM,  TRANSPOSE_BASE_NOTE,      TEXT_(twelve_notes_names), 62,        LINE_1, CTRL_TRANSPOSE_SCALED },
        { ELEM_TEXT,  0,                        TEXT_(interval),           TXT_LEFT,  LINE_2, CTRL_TRANSPOSE_SCALED },
        { ELEM_ITEM,  TRANSPOSE_INTERVAL,       TEXT_(intervals),          55,        LINE_2, CTRL_TRANSPOSE_SCALED },
        { ELEM_TEXT,  0,                        TEXT_(scale),              TXT_LEFT,  LINE_3, CTRL_TRANSPOSE_SCALED },
        { ELEM_ITEM,  TRANSPOSE_TRANSPOSE_SCALE,TEXT_(scales),             40,        LINE_3, CTRL_TRANSPOSE_SCALED },

        // ---------- COMMON (both pages) ----------
        { ELEM_TEXT,  0,                        TEXT_(send_base),          TXT_LEFT,  LINE_4, CTRL_TRANSPOSE_ALL },
        { ELEM_ITEM,  TRANSPOSE_SEND_ORIGINAL,  TEXT_(no_yes),             65,        LINE_4, CTRL_TRANSPOSE_ALL },
    };
    menu_ui_render(MENU_TRANSPOSE, elems, sizeof(elems)/sizeof(elems[0]));
}

void ui_code_transpose() {
	midi_display_on_off(save_get(TRANSPOSE_CURRENTLY_SENDING), 63);
}
