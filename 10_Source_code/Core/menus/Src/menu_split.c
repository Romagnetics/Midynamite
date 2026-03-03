/*
 * menu_split.c
 *
 *  Created on: Mar 3, 2026
 *      Author: Astaa
 */
#include "memory_main.h"
#include "_menu_ui.h"
#include "menus.h"
#include "text.h"

void cont_update_split(menu_list_t field)
{
    (void)field;
}

void ui_update_split(void)
{
    const ui_element elems[] = {
        { ELEM_TEXT , 0,                 TEXT_(midi_split),      UI_6x8, TXT_LEFT, LINE_0, CTRL_SPLIT_MAIN },

        { ELEM_TEXT , 0,                 TEXT_(split_point),     UI_6x8, TXT_LEFT, LINE_1, CTRL_SPLIT_MAIN },
        { ELEM_ITEM , SPLIT_NOTE,        TEXT_(midi_note_names), UI_6x8, 65,       LINE_1, CTRL_SPLIT_MAIN },


        { ELEM_TEXT , 0,                 TEXT_(output_sem),      UI_6x8, TXT_LEFT, LINE_2, CTRL_SPLIT_MAIN },
        { ELEM_ITEM , SPLIT_SEND_TO_MIDI_OUT, TEXT_(midi_outs_split), UI_6x8, 50,  LINE_2, CTRL_SPLIT_MAIN },

        { ELEM_TEXT , 0,                 TEXT_(low_sem),         UI_6x8, TXT_LEFT, LINE_3, CTRL_SPLIT_MAIN },
        { ELEM_ITEM , SPLIT_MIDI_CH1,    TEXT_(zero_to_sixteen), UI_6x8, 35,       LINE_3, CTRL_SPLIT_MAIN },
        { ELEM_TEXT , 0,                 TEXT_(send_to),         UI_6x8, 60,       LINE_3, CTRL_SPLIT_MAIN },
        { ELEM_ITEM , SPLIT_SEND_CH1,    TEXT_(dry_fx),          UI_6x8, 105,      LINE_3, CTRL_SPLIT_MAIN },


        { ELEM_TEXT , 0,                 TEXT_(high_sem),        UI_6x8, TXT_LEFT, LINE_4, CTRL_SPLIT_MAIN },
        { ELEM_ITEM , SPLIT_MIDI_CH2,    TEXT_(zero_to_sixteen), UI_6x8, 35,       LINE_4, CTRL_SPLIT_MAIN },
        { ELEM_TEXT , 0,                 TEXT_(send_to),         UI_6x8, 60,       LINE_4, CTRL_SPLIT_MAIN },
        { ELEM_ITEM , SPLIT_SEND_CH2,    TEXT_(dry_fx),          UI_6x8, 105,      LINE_4, CTRL_SPLIT_MAIN },

    };
    menu_ui_render(MENU_SPLIT, elems, (uint8_t)(sizeof(elems) / sizeof(elems[0])));
}

void ui_code_split()
{
    midi_display_on_off(save_get(SPLIT_CURRENTLY_SENDING), LINE_3 - 3);
    draw_line(ON_OFF_VERT_LINE, LINE_3 - 3, 127, LINE_3 - 3);
}


