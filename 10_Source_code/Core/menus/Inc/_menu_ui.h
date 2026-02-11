/*
 * _menu_ui.h
 *
 *  Created on: Aug 23, 2025
 *      Author: Astaa
 */

#ifndef MENU_INC_MENU_UI_H_
#define MENU_INC_MENU_UI_H_

#include "_menu_controller.h" // Access to menu items

#define LINE_0 0
#define LINE_1 15
#define LINE_2 25
#define LINE_3 35
#define LINE_4 45
#define B_LINE LINE_4 + 3
#define SCREEN_BOTTOM 64

#define ON_OFF_VERT_LINE 92


#define TXT_LEFT 5

// Fonts you already use via write_* functions
typedef enum {
    UI_6x8,
    UI_6x8_2, //For midi 16 Channels
    UI_11x18,
    UI_16x24
} ui_font_t;

// What to render
typedef enum {
    ELEM_TEXT,
	ELEM_ITEM,
	ELEM_16CH,
} ui_elem_type_t;

#define UI_CHOICE(tbl) ((const char*)(tbl))
#define UI_TEXT_NUM    ((const char*)-1)

typedef struct {
    ui_elem_type_t type;
    uint8_t save_item;
    const char *text;
    ui_font_t font;
    int16_t x;
    int16_t y;
    uint8_t ctrl_group_id;
} ui_element;

// -------------------------
// Menu switch logic
// -------------------------
typedef enum {
    UI_MODIFY_INCREMENT = 0,
    UI_MODIFY_SET,
} ui_modify_op_t;

uint8_t  set_current_menu(menu_list_t field, ui_modify_op_t op, uint8_t value_if_set);
uint8_t  get_current_menu(menu_list_t field);


// ---------------------
// API
// ---------------------
void initialize_screen(void);

void draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);

void menu_ui_render(menu_list_t menu, const ui_element *elems, size_t count);

#endif /* MENU_INC_MENU_UI_H_ */
