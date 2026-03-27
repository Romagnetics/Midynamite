/*
 * menu_split.c
 *
 *  Created on: Mar 3, 2026
 *      Author: Romain Dereu
 */
#include "_menu_ui.h"
#include "menus.h"

#define MASK_COL 80

void ui_update_split(void)
{
    const ui_element elems[] = {
        // type       save_item              text                        x         y       ctrl_group_id
        { ELEM_TEXT,  0,                     TEXT_(midi_split),          TXT_LEFT,  LINE_0, CTRL_SPLIT_PAGE_1 },
        { ELEM_TEXT,  0,                     TEXT_(select_bus_effects),  TXT_LEFT,  LINE_0, CTRL_SPLIT_PAGE_2 },

        { ELEM_TEXT,  0,                     TEXT_(type),                TXT_LEFT,  LINE_1, CTRL_SPLIT_PAGE_1 },
        { ELEM_ITEM,  SPLIT_TYPE,            TEXT_(split_types),         40,        LINE_1, CTRL_SPLIT_PAGE_1 },

        { ELEM_TEXT,  0,                     TEXT_(split_note),          TXT_LEFT,  LINE_2, CTRL_SPLIT_TYPE_NOTE },
        { ELEM_ITEM,  SPLIT_NOTE,            TEXT_(midi_note_names),     70,        LINE_2, CTRL_SPLIT_TYPE_NOTE },

        { ELEM_TEXT,  0,                     TEXT_(split_ch),            TXT_LEFT,  LINE_2, CTRL_SPLIT_TYPE_CH },
        { ELEM_ITEM,  SPLIT_MIDI_CHANNEL,    TEXT_(zero_to_sixteen),     70,        LINE_2, CTRL_SPLIT_TYPE_CH },

        { ELEM_TEXT,  0,                     TEXT_(split_velocity),      TXT_LEFT,  LINE_2, CTRL_SPLIT_TYPE_VELOCITY },
        { ELEM_ITEM,  SPLIT_VELOCITY,        TEXT_(zer_to_300),          70,        LINE_2, CTRL_SPLIT_TYPE_VELOCITY },

        { ELEM_TEXT,  0,                     TEXT_(low_to),              TXT_LEFT,  LINE_3, CTRL_SPLIT_PAGE_1 },
        { ELEM_ITEM,  SPLIT_SEND_CH1,        TEXT_(dry_buses_all),       55,        LINE_3, CTRL_SPLIT_PAGE_1 },
        { ELEM_TEXT,  0,                     TEXT_(ch_sem),              90,        LINE_3, CTRL_SPLIT_PAGE_1 },
        { ELEM_ITEM,  SPLIT_MIDI_CH1,        TEXT_(zero_to_sixteen),     110,       LINE_3, CTRL_SPLIT_PAGE_1 },

        { ELEM_TEXT,  0,                     TEXT_(high_to),             TXT_LEFT,  LINE_4, CTRL_SPLIT_PAGE_1 },
        { ELEM_ITEM,  SPLIT_SEND_CH2,        TEXT_(dry_buses_all),       55,        LINE_4, CTRL_SPLIT_PAGE_1 },
        { ELEM_TEXT,  0,                     TEXT_(ch_sem),              90,        LINE_4, CTRL_SPLIT_PAGE_1 },
        { ELEM_ITEM,  SPLIT_MIDI_CH2,        TEXT_(zero_to_sixteen),     110,       LINE_4, CTRL_SPLIT_PAGE_1 },

        { ELEM_TEXT,  0,                     TEXT_(modify_short),        TXT_LEFT,  LINE_1, CTRL_SPLIT_PAGE_2 },
        { ELEM_ITEM,  SPLIT_MASK_MODIFY,     TEXT_(bus_effect_mask),     MASK_COL,  LINE_1, CTRL_SPLIT_PAGE_2 },

        { ELEM_TEXT,  0,                     TEXT_(transpose_short),     TXT_LEFT,  LINE_2, CTRL_SPLIT_PAGE_2 },
        { ELEM_ITEM,  SPLIT_MASK_TRANSPOSE,  TEXT_(bus_effect_mask),     MASK_COL,  LINE_2, CTRL_SPLIT_PAGE_2 },

        { ELEM_TEXT,  0,                     TEXT_(arpeggiator_short),   TXT_LEFT,  LINE_3, CTRL_SPLIT_PAGE_2 },
        { ELEM_ITEM,  SPLIT_MASK_ARPEGGIATOR,TEXT_(bus_effect_mask),     MASK_COL,  LINE_3, CTRL_SPLIT_PAGE_2 },

        { ELEM_TEXT,  0,                     TEXT_(dispatch_short),      TXT_LEFT,  LINE_4, CTRL_SPLIT_PAGE_2 },
        { ELEM_ITEM,  SPLIT_MASK_DISPATCH,   TEXT_(bus_effect_mask),     MASK_COL,  LINE_4, CTRL_SPLIT_PAGE_2 },
    };
    menu_ui_render(MENU_SPLIT, elems, (uint8_t)(sizeof(elems) / sizeof(elems[0])));
}

void ui_code_split()
{
    const menu_group_mask_t active_groups = ui_active_groups();
    const menu_group_mask_t page_1_mask = (((menu_group_mask_t)1) << (CTRL_SPLIT_PAGE_1 - 1));
    if ((active_groups & page_1_mask) == 0) {
        return;
    }

    midi_display_on_off(save_get(SPLIT_CURRENTLY_SENDING), LINE_3 - 3);
    draw_line(ON_OFF_VERT_LINE, LINE_3 - 3, 127, LINE_3 - 3);
}
