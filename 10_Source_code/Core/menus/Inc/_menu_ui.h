/*
 * _menu_ui.h
 *
 *  Created on: Aug 23, 2025
 *      Author: Romain Dereu
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
    ELEM_TEXT_1118,
    ELEM_TEXT_1624,
    ELEM_ITEM_1118,
    ELEM_ITEM_1624,
	ELEM_16CH,
    ELEM_8STEPS,
	ELEM_SWINGPCT,
} ui_elem_type_t;

#define UI_CHOICE(tbl) ((const char*)(tbl))
#define UI_TEXT_NUM    ((const char*)-1)

typedef struct {
    ui_elem_type_t type;
    uint8_t save_item;
    const char *text;
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

typedef enum {
    GROUP_STATE_BASED = 0,
    CURRENT_POSITION_BASED
} selector_kind_t;

typedef struct page_group_rule_s {
    selector_kind_t   kind;
    uint8_t           case_count;
    const ctrl_group_id_t *groups;
    save_field_t      field;
    selector_compute_fn_t compute;
    uint8_t           cycle_on_press;
    menu_list_t       page;
} page_group_rule_t;

extern const page_group_rule_t kPageGroupRules[];
extern const size_t kPageGroupRulesCount;

uint8_t idx_from_save(save_field_t f, uint8_t case_count);
uint8_t group_list_len(const ctrl_group_id_t *groups);
uint8_t idx_from_position_selector(const page_group_rule_t *sel);
void build_union_for_groups_local(const ctrl_group_id_t *groups, uint8_t n_groups, CtrlActiveList *out);
uint8_t build_union_for_position_page(menu_list_t page, CtrlActiveList *out);



// -------- Menu helpers --------
void screen_update_menu(uint32_t flag);

uint8_t settings_recently_saved(void);

// Controller-facing polling entrypoint
void refresh_menu(void);
void toggle_subpage(menu_list_t field);

void midi_display_on_off(uint8_t on_or_off, uint8_t bottom_line);

#endif /* MENU_INC_MENU_UI_H_ */
