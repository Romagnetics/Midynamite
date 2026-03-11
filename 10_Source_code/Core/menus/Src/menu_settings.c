/*
 * menu_settings.c
 *
 *  Created on: Jun 25, 2025
 *      Author: Romain Dereu
 */
#include "main.h" // Timer
#include "_menu_ui.h"
#include "menus.h"
#include "screen_driver.h"
#include "text.h"
#include "cmsis_os2.h" //osDelay
#include "utils.h" // Debounce


void cont_update_settings()  {
    // Needs persistent state for debounce across frames
    static uint8_t save_btn_state = 1;

    if (debounce_button(GPIOB, Btn1_Pin, &save_btn_state, 50)) {
        // Immediate feedback
        write_68(message->saving, TXT_LEFT, B_LINE);
        screen_driver_UpdateScreen();

        store_settings();

        write_68(message->saved, TXT_LEFT, B_LINE);
        screen_driver_UpdateScreen();
        osDelay(1000);

        write_68(message->save_instruction, TXT_LEFT, B_LINE);
        screen_driver_UpdateScreen();

        // Make sure the normal UI redraws ASAP after the save banner
        threads_display_notify(flag_for_menu(MENU_SETTINGS));
    }
}


void ui_update_settings(void)
{
    const ui_element elems[] = {
        // type      save_item                 text                         font    x        y     ctrl_group_id
        // -------- GLOBAL 1 --------
        { ELEM_TEXT , 0,                       TEXT_(global_settings_1),   UI_6x8, TXT_LEFT, LINE_0, CTRL_SETTINGS_GLOBAL1 },
        { ELEM_TEXT , 0,                       TEXT_(start_menu_),          UI_6x8, TXT_LEFT, LINE_1, CTRL_SETTINGS_GLOBAL1 },
        { ELEM_ITEM , SETTINGS_START_MENU,     TEXT_(menu_list),           UI_6x8, 70,       LINE_1, CTRL_SETTINGS_GLOBAL1 },

        { ELEM_TEXT , 0,                       TEXT_(usb_midi_),            UI_6x8, TXT_LEFT, LINE_2, CTRL_SETTINGS_GLOBAL1 },
        { ELEM_ITEM , SETTINGS_SEND_USB,       TEXT_(off_out_thru_thru_both),UI_6x8, 70,       LINE_2, CTRL_SETTINGS_GLOBAL1 },

        { ELEM_TEXT , 0,                       TEXT_(contrast_),            UI_6x8, TXT_LEFT, LINE_3, CTRL_SETTINGS_GLOBAL1 },
        { ELEM_ITEM , SETTINGS_BRIGHTNESS,     TEXT_(ten_hundred_ten_percent),     UI_6x8, 70,       LINE_3, CTRL_SETTINGS_GLOBAL1 },
        // -------- GLOBAL 2 --------
        { ELEM_TEXT , 0,                       TEXT_(global_settings_2),   UI_6x8, TXT_LEFT, LINE_0, CTRL_SETTINGS_GLOBAL2 },
        { ELEM_TEXT , 0,                       TEXT_(MIDI_Thru_),           UI_6x8, TXT_LEFT, LINE_1, CTRL_SETTINGS_GLOBAL2 },
        { ELEM_ITEM , SETTINGS_MIDI_THRU,      TEXT_(off_on),              UI_6x8, 80,       LINE_1, CTRL_SETTINGS_GLOBAL2 },

		{ ELEM_TEXT , 0,                       TEXT_(MIDI_Out_),           UI_6x8, TXT_LEFT, LINE_2, CTRL_SETTINGS_GLOBAL2 },
        { ELEM_ITEM , SETTINGS_SEND_TO_OUT,    TEXT_(midi_outs_split),     UI_6x8, 80,       LINE_2, CTRL_SETTINGS_GLOBAL2 },


        { ELEM_TEXT , 0,                       TEXT_(MIDI_Filter_),         UI_6x8, TXT_LEFT, LINE_3, CTRL_SETTINGS_GLOBAL2 },
        { ELEM_ITEM , SETTINGS_CHANNEL_FILTER, TEXT_(off_on),              UI_6x8, 80,       LINE_3, CTRL_SETTINGS_GLOBAL2 },
        // -------- Filters --------
        { ELEM_TEXT , 0,                       TEXT_(MIDI_Filter_),             UI_6x8, TXT_LEFT, LINE_0, CTRL_SETTINGS_FILTER },
        { ELEM_TEXT , 0,                       TEXT_(equals_ignore_channel), UI_6x8, TXT_LEFT, LINE_1, CTRL_SETTINGS_FILTER },
        { ELEM_16CH , SETTINGS_FILTERED_CH,    "0",                        UI_6x8_2,TXT_LEFT, LINE_2, CTRL_SETTINGS_FILTER },
        // -------- ABOUT--------
        { ELEM_TEXT , 0,                       TEXT_(about),               UI_6x8,  TXT_LEFT, LINE_0, CTRL_SETTINGS_ABOUT },
        { ELEM_TEXT , 0,                       TEXT_(about_brand),         UI_6x8,  TXT_LEFT, LINE_1, CTRL_SETTINGS_ABOUT },
        { ELEM_TEXT , 0,                       TEXT_(about_product),       UI_6x8,  TXT_LEFT, LINE_2, CTRL_SETTINGS_ABOUT },
        { ELEM_TEXT , 0,                       TEXT_(about_version),       UI_6x8, TXT_LEFT, LINE_3, CTRL_SETTINGS_ABOUT },
        // -------- Bottom part (always on) --------
        { ELEM_TEXT , 0,                       TEXT_(save_instruction),    UI_6x8, TXT_LEFT, B_LINE, CTRL_SETTINGS_ALWAYS },
    };
    menu_ui_render(MENU_SETTINGS, elems, sizeof(elems)/sizeof(elems[0]));
}



void ui_code_settings()  {
	draw_line(0, LINE_4, 127, LINE_4);
}

