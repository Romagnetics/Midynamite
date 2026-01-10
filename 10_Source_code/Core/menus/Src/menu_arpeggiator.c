/*
 * menu_modify.c
 *
 *  Created on: Feb 27, 2025
 *      Author: Romain Dereu
 */
#include "memory_main.h"
#include "_menu_ui.h"
#include "menus.h"
#include "text.h"

void cont_update_arpeggiator() {
}


void ui_update_arpeggiator(void)
{
    const ui_element elems[] = {
        // type      save_item                text                        font    x       y           ctrl_group_id
        { ELEM_TEXT , 0,                       TEXT_(arpeggiator),        UI_6x8, TXT_LEFT, LINE_0,      CTRL_ARPEGGIATOR_ALL },

    };
    menu_ui_render(MENU_ARPEGGIATOR, elems, (uint8_t)(sizeof(elems)/sizeof(elems[0])));
}


void ui_code_arpeggiator()    {

}
