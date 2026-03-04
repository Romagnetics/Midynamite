/*
 * menus.h
 *
 *  Created on: Feb 27, 2025
 *      Author: Romain Dereu
 */

#ifndef INC_MENUS_H_
#define INC_MENUS_H_

#include <stdint.h>
#include "memory_main.h"   // save_field_t, SAVE_FIELD_COUNT
#include "text.h" //definition of message

//The following is defined here to avoid importing text.h into every menu
#define TEXT_(m) ((const char*)(message->m))

#ifndef MENU_ACTIVE_LIST_CAP
#define MENU_ACTIVE_LIST_CAP 64
#endif

// ---------------------
// Menu list
// ---------------------
typedef enum {
    MENU_TEMPO = 0,
    MENU_SPLIT,
    MENU_MODIFY,
    MENU_TRANSPOSE,
	MENU_ARPEGGIATOR,
	MENU_DISPATCH,
    MENU_SETTINGS,
    AMOUNT_OF_MENUS,   // number of menus
    CURRENT_MENU,      // UI state index
    OLD_MENU,          // UI state index
    STATE_FIELD_COUNT  // total UI state slots
} menu_list_t;

// Active list (shared type)
typedef struct {
    uint16_t fields_idx[MENU_ACTIVE_LIST_CAP];
    uint8_t  count;
} CtrlActiveList;

typedef uint64_t menu_group_mask_t;

// Selector callback type used by the selector table (menus.c)
typedef uint8_t (*selector_compute_fn_t)();

// Active-list storage access for controller
CtrlActiveList* list_for_page(menu_list_t page);

// -------- Individual menu updates --------
void ui_code_tempo();
void ui_update_tempo();

void ui_code_split();
void ui_update_split();

void cont_update_modify(menu_list_t field);
void ui_code_modify();
void ui_update_modify();

void cont_update_transpose(menu_list_t field);
void ui_code_transpose();
void ui_update_transpose();

void cont_update_arpeggiator(menu_list_t field);
void ui_code_arpeggiator();
void ui_update_arpeggiator();

void ui_code_dispatch();
void ui_update_dispatch();



void cont_update_settings();
void ui_code_settings();
void ui_update_settings();

// -------- Menu helpers --------
void screen_update_menu(uint32_t flag);
void ui_code_menu();
void cont_update_menu(menu_list_t field);


void menu_change_check();
void refresh_screen();
void toggle_subpage(menu_list_t field);

// Thread related
void start_stop_pressed();
void midi_display_on_off(uint8_t on_or_off, uint8_t bottom_line);

// -------- Selector helpers exposed to controller (implemented in menus.c) --------
uint8_t            build_union_for_position_page(menu_list_t page, CtrlActiveList *out);
menu_group_mask_t  ctrl_active_mask_for_page(menu_list_t page);
menu_list_t        page_for_ctrl_id(uint32_t id);
uint8_t            menus_cycle_on_press(menu_list_t page);

#endif /* INC_MENUS_H_ */
