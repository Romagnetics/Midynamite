/*
 * menu_arp.c
 *
 *  Created on: Jan 10, 2026
 *      Author: Romain Dereu
 */
#include "_menu_ui.h"
#include "menus.h"

void ui_update_arpeggiator(void)
{
    #define CENTER_SPLIT 62

    const ui_element elems[] = {
        // type          save_item            text                         x             y       ctrl_group_id
        { ELEM_TEXT,     0,                   TEXT_(arpeggiator_1),       TXT_LEFT,     LINE_0,  CTRL_ARPEGGIATOR_PAGE_1 },

        // Tempo
        { ELEM_TEXT,     0,                   TEXT_(tempo),              TXT_LEFT,     LINE_1,  CTRL_ARPEGGIATOR_PAGE_1 },
        { ELEM_ITEM,     TEMPO_CURRENT_TEMPO, TEXT_(zer_to_300),         CENTER_SPLIT, LINE_1,  CTRL_SHARED_TEMPO },

        // Division
        { ELEM_TEXT,     0,                   TEXT_(division_),          TXT_LEFT,     LINE_2,  CTRL_ARPEGGIATOR_PAGE_1 },
        { ELEM_ITEM,     ARPEGGIATOR_DIVISION,TEXT_(division_list),      CENTER_SPLIT, LINE_2,  CTRL_ARPEGGIATOR_PAGE_1 },

        // Gate / Octaves
        { ELEM_TEXT,     0,                   TEXT_(gate_),              TXT_LEFT,     LINE_3,  CTRL_ARPEGGIATOR_PAGE_1 },
        { ELEM_ITEM,     ARPEGGIATOR_GATE,    TEXT_(ten_hundred_ten_percent), 35,      LINE_3,  CTRL_ARPEGGIATOR_PAGE_1 },
        { ELEM_ITEM,     ARPEGGIATOR_OCTAVES, TEXT_(octave_count),       CENTER_SPLIT, LINE_3,  CTRL_ARPEGGIATOR_PAGE_1 },

        // Pattern
        { ELEM_TEXT,     0,                   TEXT_(pattern_),           TXT_LEFT,     LINE_4,  CTRL_ARPEGGIATOR_PAGE_1 },
        { ELEM_ITEM,     ARPEGGIATOR_PATTERN, TEXT_(arp_patterns),       CENTER_SPLIT, LINE_4,  CTRL_ARPEGGIATOR_PAGE_1 },

        // Page 2
        { ELEM_TEXT,     0,                   TEXT_(arpeggiator_2),       TXT_LEFT,     LINE_0,  CTRL_ARPEGGIATOR_PAGE_2 },

        { ELEM_TEXT,     0,                   TEXT_(swing_),             TXT_LEFT,     LINE_1,  CTRL_ARPEGGIATOR_PAGE_2 },
        { ELEM_SWINGPCT, ARPEGGIATOR_SWING,   NULL,                      CENTER_SPLIT, LINE_1,  CTRL_ARPEGGIATOR_PAGE_2 },

        { ELEM_TEXT,     0,                   TEXT_(length_),            TXT_LEFT,     LINE_2,  CTRL_ARPEGGIATOR_PAGE_2 },
        { ELEM_ITEM,     ARPEGGIATOR_LENGTH,  TEXT_(zer_to_300),         CENTER_SPLIT, LINE_2,  CTRL_ARPEGGIATOR_PAGE_2 },

        { ELEM_TEXT,     0,                   TEXT_(steps_),             TXT_LEFT,     LINE_3,  CTRL_ARPEGGIATOR_PAGE_2 },
        { ELEM_8STEPS,   ARPEGGIATOR_NOTES,   "X",                       45,           LINE_3,  CTRL_ARPEGGIATOR_PAGE_2 },

        { ELEM_TEXT,     0,                   TEXT_(hold_),              TXT_LEFT,     LINE_4,  CTRL_ARPEGGIATOR_PAGE_2 },
        { ELEM_ITEM,     ARPEGGIATOR_HOLD,    TEXT_(off_on),             35,           LINE_4,  CTRL_ARPEGGIATOR_PAGE_2 },
        { ELEM_TEXT,     0,                   TEXT_(key_sync_),          60,           LINE_4,  CTRL_ARPEGGIATOR_PAGE_2 },
        { ELEM_ITEM,     ARPEGGIATOR_KEY_SYNC,TEXT_(off_on),             110,          LINE_4,  CTRL_ARPEGGIATOR_PAGE_2 },
    };

    menu_ui_render(
        MENU_ARPEGGIATOR,
        elems,
        (uint8_t)(sizeof(elems) / sizeof(elems[0]))
    );
}

void ui_code_arpeggiator(void)
{
    midi_display_on_off(save_get(ARPEGGIATOR_CURRENTLY_SENDING), LINE_3 - 3);
    draw_line(ON_OFF_VERT_LINE, LINE_3 - 3, 127, LINE_3 - 3);
}
