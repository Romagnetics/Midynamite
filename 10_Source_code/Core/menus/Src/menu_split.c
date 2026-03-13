/*
 * menu_split.c
 *
 *  Created on: Mar 3, 2026
 *      Author: Romain Dereu
 */
#include "memory_main.h"
#include "_menu_ui.h"
#include "menus.h"
#include "text.h"

void cont_update_split(menu_list_t field)
{
    toggle_subpage(field);
}

#define MASK_COL 80

void ui_update_split(void)
{
    const ui_element elems[] = {
        { ELEM_TEXT , 0,                  TEXT_(midi_split),         UI_6x8, TXT_LEFT, LINE_0, CTRL_SPLIT_PAGE_1 },
        { ELEM_TEXT , 0,                  TEXT_(select_bus_effects), UI_6x8, TXT_LEFT, LINE_0, CTRL_SPLIT_PAGE_2 },

        { ELEM_TEXT , 0,                  TEXT_(type),               UI_6x8, TXT_LEFT, LINE_1, CTRL_SPLIT_PAGE_1 },
        { ELEM_ITEM , SPLIT_TYPE,         TEXT_(split_types),        UI_6x8, 40,       LINE_1, CTRL_SPLIT_PAGE_1 },

        { ELEM_TEXT , 0,                  TEXT_(split_note),         UI_6x8, TXT_LEFT, LINE_2, CTRL_SPLIT_TYPE_NOTE },
        { ELEM_ITEM , SPLIT_NOTE,         TEXT_(midi_note_names),    UI_6x8, 70,       LINE_2, CTRL_SPLIT_TYPE_NOTE },

        { ELEM_TEXT , 0,                  TEXT_(split_ch),           UI_6x8, TXT_LEFT, LINE_2, CTRL_SPLIT_TYPE_CH },
        { ELEM_ITEM , SPLIT_MIDI_CHANNEL, TEXT_(zero_to_sixteen),    UI_6x8, 70,       LINE_2, CTRL_SPLIT_TYPE_CH },

        { ELEM_TEXT , 0,                  TEXT_(split_velocity),     UI_6x8, TXT_LEFT, LINE_2, CTRL_SPLIT_TYPE_VELOCITY },
        { ELEM_ITEM , SPLIT_VELOCITY,     TEXT_(zer_to_300),         UI_6x8, 70,       LINE_2, CTRL_SPLIT_TYPE_VELOCITY },


        { ELEM_TEXT , 0,                  TEXT_(low_to),             UI_6x8, TXT_LEFT, LINE_3, CTRL_SPLIT_PAGE_1 },
        { ELEM_ITEM , SPLIT_SEND_CH1,     TEXT_(dry_buses_all),      UI_6x8, 55,       LINE_3, CTRL_SPLIT_PAGE_1 },
        { ELEM_TEXT , 0,                  TEXT_(ch_sem),             UI_6x8, 90,       LINE_3, CTRL_SPLIT_PAGE_1 },
        { ELEM_ITEM , SPLIT_MIDI_CH1,     TEXT_(zero_to_sixteen),    UI_6x8, 110,      LINE_3, CTRL_SPLIT_PAGE_1 },


        { ELEM_TEXT , 0,                  TEXT_(high_to),            UI_6x8, TXT_LEFT, LINE_4, CTRL_SPLIT_PAGE_1 },
        { ELEM_ITEM , SPLIT_SEND_CH2,     TEXT_(dry_buses_all),      UI_6x8, 55,       LINE_4, CTRL_SPLIT_PAGE_1 },
        { ELEM_TEXT , 0,                  TEXT_(ch_sem),             UI_6x8, 90,       LINE_4, CTRL_SPLIT_PAGE_1 },
        { ELEM_ITEM , SPLIT_MIDI_CH2,     TEXT_(zero_to_sixteen),    UI_6x8, 110,      LINE_4, CTRL_SPLIT_PAGE_1 },


        { ELEM_TEXT , 0,                  TEXT_(modify_short),       UI_6x8, TXT_LEFT, LINE_1, CTRL_SPLIT_PAGE_2 },
        { ELEM_ITEM , SPLIT_MASK_MODIFY,  TEXT_(bus_effect_mask),    UI_6x8, MASK_COL, LINE_1, CTRL_SPLIT_PAGE_2 },

        { ELEM_TEXT , 0,                  TEXT_(transpose_short),    UI_6x8, TXT_LEFT, LINE_2, CTRL_SPLIT_PAGE_2 },
        { ELEM_ITEM , SPLIT_MASK_TRANSPOSE, TEXT_(bus_effect_mask),  UI_6x8, MASK_COL, LINE_2, CTRL_SPLIT_PAGE_2 },

        { ELEM_TEXT , 0,                  TEXT_(arpeggiator_short),  UI_6x8, TXT_LEFT, LINE_3, CTRL_SPLIT_PAGE_2 },
        { ELEM_ITEM , SPLIT_MASK_ARPEGGIATOR, TEXT_(bus_effect_mask), UI_6x8, MASK_COL,LINE_3, CTRL_SPLIT_PAGE_2 },

        { ELEM_TEXT , 0,                  TEXT_(dispatch_short),     UI_6x8, TXT_LEFT, LINE_4, CTRL_SPLIT_PAGE_2 },
        { ELEM_ITEM , SPLIT_MASK_DISPATCH, TEXT_(bus_effect_mask),   UI_6x8, MASK_COL, LINE_4, CTRL_SPLIT_PAGE_2 },

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
