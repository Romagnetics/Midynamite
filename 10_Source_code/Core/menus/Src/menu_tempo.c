/*
 * menu_tempo.c
 *
 *  Created on: Feb 1, 2025
 *      Author: Romain Dereu
 */
#include "_menu_ui.h"
#include "menus.h"

void ui_update_tempo(void)
{
    const ui_element elems[] = {
        // type            save_item               text                   x         y       ctrl_group_id
        { ELEM_TEXT,       0,                      TEXT_(send_midi_tempo), TXT_LEFT,  LINE_0, CTRL_TEMPO_ALL },
        { ELEM_TEXT,       0,                      TEXT_(target),          TXT_LEFT,  LINE_1, CTRL_TEMPO_ALL },
        { ELEM_ITEM,       TEMPO_SEND_TO_MIDI_OUT, TEXT_(midi_outs),       TXT_LEFT,  LINE_2, CTRL_TEMPO_ALL },
        { ELEM_ITEM_1118,  TEMPO_CURRENTLY_SENDING,TEXT_(off_on),          15,        42,     CTRL_TEMPO_ALL },
        { ELEM_ITEM_1624,  TEMPO_CURRENT_TEMPO,    TEXT_(zer_to_300),      80,        20,     CTRL_SHARED_TEMPO },
        { ELEM_TEXT,       0,                      TEXT_(bpm),             80,        48,     CTRL_TEMPO_ALL },
    };

    menu_ui_render(MENU_TEMPO, elems, sizeof(elems) / sizeof(elems[0]));
}

void ui_code_tempo() {

  //Vertical line  right of BPM
	draw_line(64, 10, 64, 64);
  //Horizontal line above On / Off
	draw_line(0, 40, 64, 40);

}
