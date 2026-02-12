/*
 * menu_artpeggiator.c
 *
 *  Created on: Jan 10, 2026
 *      Author: Romain Dereu
 */
#include "memory_main.h"
#include "_menu_ui.h"
#include "menus.h"
#include "text.h"

#define ARP_CENTER_SPLIT_X 62
#define ARP_GATE_X 35
#define ARP_SWING_X 35
#define ARP_HOLD_X 35
#define ARP_KEY_SYNC_LABEL_X 60
#define ARP_KEY_SYNC_VALUE_X 110

void cont_update_arpeggiator(menu_list_t field) {
	toggle_subpage(field);
}


void ui_update_arpeggiator(void)
{
    const ui_element elems[] = {
        // type       save_item               text                          font    x             y        ctrl_group_id
        { ELEM_TEXT , 0,                      TEXT_(arpeggiator_1),         UI_6x8, TXT_LEFT,     LINE_0,  CTRL_ARPEGGIATOR_PAGE_1 },

        // Tempo
        { ELEM_TEXT , 0,                      TEXT_(tempo),                 UI_6x8, TXT_LEFT,     LINE_1,  CTRL_ARPEGGIATOR_PAGE_1 },
        { ELEM_ITEM , TEMPO_CURRENT_TEMPO,    TEXT_(zer_to_300),            UI_6x8, ARP_CENTER_SPLIT_X, LINE_1,  CTRL_SHARED_TEMPO      },

        // Division
        { ELEM_TEXT , 0,                      TEXT_(division_),              UI_6x8, TXT_LEFT,     LINE_2,  CTRL_ARPEGGIATOR_PAGE_1 },
        { ELEM_ITEM , ARPEGGIATOR_DIVISION,   TEXT_(division_list),         UI_6x8, ARP_CENTER_SPLIT_X, LINE_2,  CTRL_ARPEGGIATOR_PAGE_1 },

        // Gate
        { ELEM_TEXT , 0,                      TEXT_(gate_),                  UI_6x8, TXT_LEFT,     LINE_3,  CTRL_ARPEGGIATOR_PAGE_1 },
        { ELEM_ITEM , ARPEGGIATOR_GATE,       TEXT_(ten_hundred_ten_percent), UI_6x8, ARP_GATE_X, LINE_3,  CTRL_ARPEGGIATOR_PAGE_1 },
        { ELEM_ITEM , ARPEGGIATOR_OCTAVES,    TEXT_(octave_count),          UI_6x8, ARP_CENTER_SPLIT_X, LINE_3,  CTRL_ARPEGGIATOR_PAGE_1 },

        // Pattern
        { ELEM_TEXT , 0,                      TEXT_(pattern_),               UI_6x8, TXT_LEFT,     LINE_4,  CTRL_ARPEGGIATOR_PAGE_1 },
        { ELEM_ITEM , ARPEGGIATOR_PATTERN,    TEXT_(arp_patterns),          UI_6x8, ARP_CENTER_SPLIT_X, LINE_4,  CTRL_ARPEGGIATOR_PAGE_1 },

        // Page 2
        { ELEM_TEXT , 0,                      TEXT_(arpeggiator_2),         UI_6x8, TXT_LEFT,     LINE_0,  CTRL_ARPEGGIATOR_PAGE_2 },

        { ELEM_TEXT , 0,                      TEXT_(swing_),                 UI_6x8, TXT_LEFT,     LINE_1,  CTRL_ARPEGGIATOR_PAGE_2 },
        { ELEM_ITEM , ARPEGGIATOR_SWING,      TEXT_(zer_to_300),             UI_6x8, ARP_SWING_X, LINE_1,  CTRL_ARPEGGIATOR_PAGE_2 },

        { ELEM_TEXT , 0,                      TEXT_(length_),       UI_6x8, TXT_LEFT,     LINE_2,  CTRL_ARPEGGIATOR_PAGE_2 },
        { ELEM_ITEM , ARPEGGIATOR_LENGTH,     TEXT_(zer_to_300),    UI_6x8, ARP_CENTER_SPLIT_X, LINE_2,  CTRL_ARPEGGIATOR_PAGE_2 },



        { ELEM_TEXT , 0,                      TEXT_(steps_),               UI_6x8, TXT_LEFT,     LINE_3,  CTRL_ARPEGGIATOR_PAGE_2 },
        { ELEM_8STEPS, ARPEGGIATOR_NOTES,    "X",                          UI_6x8_2, 45,         LINE_3, CTRL_ARPEGGIATOR_PAGE_2 },



        { ELEM_TEXT , 0,                      TEXT_(hold_),                 UI_6x8, TXT_LEFT,     LINE_4,  CTRL_ARPEGGIATOR_PAGE_2 },
        { ELEM_ITEM , ARPEGGIATOR_HOLD,       TEXT_(off_on),                UI_6x8, ARP_HOLD_X,  LINE_4,  CTRL_ARPEGGIATOR_PAGE_2 },
        { ELEM_TEXT , 0,                      TEXT_(key_sync_),             UI_6x8, ARP_KEY_SYNC_LABEL_X, LINE_4,  CTRL_ARPEGGIATOR_PAGE_2 },
        { ELEM_ITEM , ARPEGGIATOR_KEY_SYNC,   TEXT_(off_on),                UI_6x8, ARP_KEY_SYNC_VALUE_X, LINE_4,  CTRL_ARPEGGIATOR_PAGE_2 },


    };

    menu_ui_render(
        MENU_ARPEGGIATOR,
        elems,
        (uint8_t)(sizeof(elems) / sizeof(elems[0]))
    );
}



void ui_code_arpeggiator()    {

	midi_display_on_off(save_get(ARPEGGIATOR_CURRENTLY_SENDING), LINE_3 - 3);
	draw_line(ON_OFF_VERT_LINE, LINE_3-3, 127, LINE_3-3);


}
