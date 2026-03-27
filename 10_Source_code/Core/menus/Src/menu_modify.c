/*
 * menu_modify.c
 *
 *  Created on: Feb 27, 2025
 *      Author: Romain Dereu
 */
#include "_menu_ui.h"
#include "menus.h"

void ui_update_modify(void)
{
    const ui_element elems[] = {
        // type       save_item              text                     x         y         ctrl_group_id
        { ELEM_TEXT,  0,                     TEXT_(midi_modify),      TXT_LEFT, LINE_0,   CTRL_MODIFY_ALL },

        { ELEM_TEXT,  0,                     TEXT_(send_1_sem),       TXT_LEFT, LINE_1+5, CTRL_MODIFY_CHANGE },
        { ELEM_ITEM,  MODIFY_SEND_TO_MIDI_CH1, TEXT_(midi_channels),  50,       LINE_1+5, CTRL_MODIFY_CHANGE },

        { ELEM_TEXT,  0,                     TEXT_(send_2_sem),       TXT_LEFT, LINE_2+5, CTRL_MODIFY_CHANGE },
        { ELEM_ITEM,  MODIFY_SEND_TO_MIDI_CH2, TEXT_(midi_channels),  50,       LINE_2+5, CTRL_MODIFY_CHANGE },

		// ---------- VELOCITY (CHANGED) ---------
        { ELEM_TEXT,  0,                     TEXT_(change_velocity), TXT_LEFT, B_LINE,   CTRL_MODIFY_VEL_CHANGED },
        { ELEM_ITEM,  MODIFY_VEL_PLUS_MINUS, TEXT_(neg_pos_80),      100,      B_LINE,   CTRL_MODIFY_VEL_CHANGED },

        // ---------- VELOCITY (FIXED) ----------
        { ELEM_TEXT,  0,                     TEXT_(fixed_velocity),  TXT_LEFT, B_LINE,   CTRL_MODIFY_VEL_FIXED },
        { ELEM_ITEM,  MODIFY_VEL_ABSOLUTE,   TEXT_(zer_to_300),      100,      B_LINE,   CTRL_MODIFY_VEL_FIXED },
    };
    menu_ui_render(MENU_MODIFY, elems, (uint8_t)(sizeof(elems)/sizeof(elems[0])));
}


void ui_code_modify()    {

	midi_display_on_off(save_get(MODIFY_CURRENTLY_SENDING), LINE_4);
	//Bottom line above velocity
	draw_line(0, LINE_4, 127, LINE_4);

}
