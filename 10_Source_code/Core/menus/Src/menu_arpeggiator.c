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

void cont_update_arpeggiator(menu_list_t field) {
	toggle_subpage(field);
}


void ui_update_arpeggiator(void)
{
    const ui_element elems[] = {
        // type      save_item                text                        font    x       y           ctrl_group_id
        { ELEM_TEXT , 0,                       TEXT_(arpeggiator),        UI_6x8, TXT_LEFT, LINE_0,      CTRL_ARPEGGIATOR_ALL },

        //Tempo
		{ ELEM_TEXT , 0,                       TEXT_(tempo),        UI_6x8, TXT_LEFT, LINE_1,      CTRL_ARPEGGIATOR_ALL},
        { ELEM_ITEM ,  TEMPO_CURRENT_TEMPO,    TEXT_(zer_to_300),   UI_6x8, 50,      LINE_1     , CTRL_TEMPO_SHARED  },



    };
    menu_ui_render(MENU_ARPEGGIATOR, elems, (uint8_t)(sizeof(elems)/sizeof(elems[0])));
}


void ui_code_arpeggiator()    {

	midi_display_on_off(save_get(ARPEGGIATOR_CURRENTLY_SENDING), SCREEN_BOTTOM);


}
