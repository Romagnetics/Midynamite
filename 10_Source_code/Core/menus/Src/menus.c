/*
 * menus.c
 *
 *  Created on: Sep 13, 2025
 *      Author: Astaa
 */
#include <stddef.h>
#include "cmsis_os.h" //For osDelay
#include "main.h" // Timer
#include "menus.h"
#include "_menu_ui.h" // menu change functions
#include "midi_tempo.h" //mt_start_stop
#include "screen_driver.h" //Font
#include "stm32f4xx_hal.h"   // HAL types (TIM, GPIO)
#include "utils.h" // Debounce

extern TIM_HandleTypeDef htim2;

// ==============================
// UI update fan-out
// ==============================

void screen_update_menu(uint32_t flag){
    uint8_t current = get_current_menu(CURRENT_MENU);
    if (flag & flag_for_menu((menu_list_t)current)) {
        switch (current) {
            case MENU_TEMPO:     ui_update_tempo();          break;
            case MENU_MODIFY:    ui_update_modify();         break;
            case MENU_TRANSPOSE: ui_update_transpose();      break;
            case MENU_ARPEGGIATOR:  ui_update_arpeggiator();       break;
            case MENU_SETTINGS:  ui_update_settings();       break;
        }
    }
}

void ui_code_menu(){
    uint8_t current = get_current_menu(CURRENT_MENU);
    switch (current) {
        case MENU_TEMPO:     ui_code_tempo();     break;
        case MENU_MODIFY:    ui_code_modify();    break;
        case MENU_TRANSPOSE: ui_code_transpose(); break;
        case MENU_ARPEGGIATOR:  ui_code_arpeggiator();       break;
        case MENU_SETTINGS:  ui_code_settings();  break;

    }
}

void cont_update_menu(menu_list_t field){
    uint8_t current = get_current_menu(CURRENT_MENU);
    switch (current) {
        case MENU_TEMPO:     cont_update_tempo();          break;
        case MENU_MODIFY:    cont_update_modify(field);    break;
        case MENU_TRANSPOSE: cont_update_transpose(field); break;
        case MENU_ARPEGGIATOR:  cont_update_arpeggiator(field);       break;
        case MENU_SETTINGS:  cont_update_settings();       break;
    }
}

// ==============================
// Selector DATA & computes
// ==============================

typedef enum {
    SEL_SAVE_BASED = 0,
    SEL_POSITION_BASED
} selector_kind_t;

typedef struct selector_def_s {
    selector_kind_t   kind;
    uint8_t           cases;
    const ctrl_group_id_t *groups;
    save_field_t      field;
    selector_compute_fn_t compute;
    uint8_t           cycle_on_press;
    menu_list_t       page;
} selector_def_t;

// save-based computes
static uint8_t sel_mod_change_split() { return (save_get(MODIFY_CHANGE_OR_SPLIT)  == MIDI_MODIFY_SPLIT)    ? 1 : 0; }
static uint8_t sel_mod_vel_type()     { return (save_get(MODIFY_VELOCITY_TYPE)    == MIDI_MODIFY_FIXED_VEL) ? 1 : 0; }
static uint8_t sel_transpose_type()   { return (save_get(TRANSPOSE_TRANSPOSE_TYPE)== MIDI_TRANSPOSE_SCALED) ? 1 : 0; }
static uint8_t sel_fixed0()           { return 0; }

// Selector table (DATA only, page-driven)
static const selector_def_t kSelectors[] = {
    // TEMPO: ALWAYS include CTRL_TEMPO_ALL
    { SEL_SAVE_BASED,     1,
      (const ctrl_group_id_t[]){ CTRL_TEMPO_ALL },
      SAVE_FIELD_INVALID, sel_fixed0, 0, MENU_TEMPO },

    // MODIFY: ALWAYS + type splits
    { SEL_SAVE_BASED,     1,
      (const ctrl_group_id_t[]){ CTRL_MODIFY_ALL },
      SAVE_FIELD_INVALID, sel_fixed0, 0, MENU_MODIFY },

    { SEL_SAVE_BASED,     2,
      (const ctrl_group_id_t[]){ CTRL_MODIFY_CHANGE, CTRL_MODIFY_SPLIT },
      MODIFY_CHANGE_OR_SPLIT,  sel_mod_change_split, 1, MENU_MODIFY },

    { SEL_SAVE_BASED,     2,
      (const ctrl_group_id_t[]){ CTRL_MODIFY_VEL_CHANGED, CTRL_MODIFY_VEL_FIXED },
      MODIFY_VELOCITY_TYPE,    sel_mod_vel_type,     1, MENU_MODIFY },

    // TRANSPOSE: ALWAYS + type split
    { SEL_SAVE_BASED,     1,
      (const ctrl_group_id_t[]){ CTRL_TRANSPOSE_ALL },
      SAVE_FIELD_INVALID, sel_fixed0, 0, MENU_TRANSPOSE },

    { SEL_SAVE_BASED,     2,
      (const ctrl_group_id_t[]){ CTRL_TRANSPOSE_SHIFT, CTRL_TRANSPOSE_SCALED },
      TRANSPOSE_TRANSPOSE_TYPE, sel_transpose_type,   1, MENU_TRANSPOSE },

	// Arpeggiator
	{ SEL_SAVE_BASED,     1,
	(const ctrl_group_id_t[]){ CTRL_ARPEGGIATOR_ALL },
	 SAVE_FIELD_INVALID, sel_fixed0, 0, MENU_ARPEGGIATOR },

    // SETTINGS: ALWAYS + page-position selector
    { SEL_SAVE_BASED,     1,
      (const ctrl_group_id_t[]){ CTRL_SETTINGS_ALWAYS },
      SAVE_FIELD_INVALID,       sel_fixed0,           0, MENU_SETTINGS },

    { SEL_POSITION_BASED, 4,
      (const ctrl_group_id_t[]){ CTRL_SETTINGS_GLOBAL1, CTRL_SETTINGS_GLOBAL2, CTRL_SETTINGS_FILTER, CTRL_SETTINGS_ABOUT },
      SAVE_FIELD_INVALID,       NULL,                  0, MENU_SETTINGS },
};

#define KSELECTORS_COUNT (sizeof(kSelectors)/sizeof(kSelectors[0]))

// -------------------------
// Active lists cache per page
// -------------------------
typedef struct {
    CtrlActiveList tempo_item_list;
    CtrlActiveList modify_item_list;
    CtrlActiveList transpose_item_list;
    CtrlActiveList arpeggiator_item_list;
    CtrlActiveList settings_item_list;
} MenuActiveLists;

static MenuActiveLists s_menu_lists;

CtrlActiveList* list_for_page(menu_list_t page) {
    switch (page) {
        case MENU_TEMPO:     return &s_menu_lists.tempo_item_list;
        case MENU_MODIFY:    return &s_menu_lists.modify_item_list;
        case MENU_TRANSPOSE: return &s_menu_lists.transpose_item_list;
        case MENU_ARPEGGIATOR:  return &s_menu_lists.arpeggiator_item_list;
        case MENU_SETTINGS:  return &s_menu_lists.settings_item_list;
        default:             return &s_menu_lists.tempo_item_list;
    }
}

// -------------------------
// Local helpers (private to menus.c)
// -------------------------
static inline uint8_t is_bits_item_local(save_field_t f) {
    return (f == SETTINGS_FILTERED_CH) ? 1u : 0u;
}

static void build_union_for_groups_local(const ctrl_group_id_t *groups, uint8_t n_groups, CtrlActiveList *out) {
    uint8_t count = 0;
    for (uint16_t f = 0; f < SAVE_FIELD_COUNT && count < MENU_ACTIVE_LIST_CAP; ++f) {
        const menu_controls_t mt = menu_controls[f];
        for (uint8_t g = 0; g < n_groups; ++g) {
            if (mt.groups == groups[g]) { out->fields_idx[count++] = f; break; }
        }
    }
    out->count = count;
}

static uint8_t idx_from_save(save_field_t f, uint8_t cases) {
    int32_t v = (f != SAVE_FIELD_INVALID) ? save_get(f) : 0;
    if (v < 0) v = 0;
    uint8_t idx = (uint8_t)v;
    return (idx < cases) ? idx : 0;
}

static const selector_def_t* first_pos_selector_for_page(menu_list_t page) {
    for (size_t i = 0; i < KSELECTORS_COUNT; ++i)
        if (kSelectors[i].page == page && kSelectors[i].kind == SEL_POSITION_BASED) return &kSelectors[i];
    return 0;
}

static uint8_t idx_from_position_selector(const selector_def_t *sel) {
    CtrlActiveList u = {0};
    build_union_for_groups_local(sel->groups, sel->cases, &u);

    const uint8_t sel_row = menu_nav_get_select(sel->page);
    uint8_t cursor = 0;

    for (uint8_t i = 0; i < u.count; ++i) {
        const save_field_t f = (save_field_t)u.fields_idx[i];
        const uint8_t span  = is_bits_item_local(f) ? 16u : 1u;

        if (sel_row < (uint8_t)(cursor + span)) {
            const ctrl_group_id_t gid = (ctrl_group_id_t)menu_controls[f].groups;
            for (uint8_t k = 0; k < sel->cases; ++k)
                if (sel->groups[k] == gid) return k;
            return 0;
        }
        cursor = (uint8_t)(cursor + span);
    }
    return 0;
}

// ==============================
// Public selector-facing surface
// ==============================
uint32_t ctrl_active_mask_for_page(menu_list_t page)
{
    uint32_t mask = 0;

    for (size_t i = 0; i < KSELECTORS_COUNT; ++i) {
        const selector_def_t *sel = &kSelectors[i];
        if (sel->page != page) continue;

        uint8_t idx = 0;
        if (sel->kind == SEL_SAVE_BASED) {
            idx = sel->compute ? sel->compute() : idx_from_save(sel->field, sel->cases);
        } else { // SEL_POSITION_BASED
            idx = idx_from_position_selector(sel);
        }
        if (idx >= sel->cases) idx = 0;
        mask |= (1u << (sel->groups[idx] - 1));
    }
    return mask;
}

uint8_t build_union_for_position_page(menu_list_t page, CtrlActiveList *out)
{
    const selector_def_t *pos_sel = first_pos_selector_for_page(page);
    if (!pos_sel) return 0;
    build_union_for_groups_local(pos_sel->groups, pos_sel->cases, out);
    return 1;
}

menu_list_t page_for_ctrl_id(uint32_t id)
{
    for (size_t i = 0; i < KSELECTORS_COUNT; ++i) {
        const selector_def_t *sel = &kSelectors[i];
        for (uint8_t k = 0; k < sel->cases; ++k) {
            if (sel->groups[k] == id) return sel->page;
        }
    }
    return 0; // fallback to first menu
}

// Press-to-cycle owner detection & action
uint8_t menus_cycle_on_press(menu_list_t page)
{
    // Determine which group owns the current row
    uint32_t gid = 0;

    CtrlActiveList u = {0};
    if (build_union_for_position_page(page, &u)) {
        // position-based page
        const uint8_t row = menu_nav_get_select(page);
        uint8_t cursor = 0;
        for (uint8_t i = 0; i < u.count; ++i) {
            const save_field_t f = (save_field_t)u.fields_idx[i];
            const uint8_t span  = is_bits_item_local(f) ? 16u : 1u;
            if (row < (uint8_t)(cursor + span)) {
                gid = menu_controls[f].groups;
                break;
            }
            cursor = (uint8_t)(cursor + span);
        }
    } else {
        // save-based only page: build union from active mask
        const uint32_t mask = ctrl_active_mask_for_page(page);
        CtrlActiveList list = {0};
        // local copy of ctrl_build_active_fields
        uint8_t count = 0;
        for (uint16_t f = 0; f < SAVE_FIELD_COUNT && count < MENU_ACTIVE_LIST_CAP; ++f) {
            const menu_controls_t mt = menu_controls[f];
            const uint32_t gm = (mt.groups >= 1 && mt.groups <= 31) ? (1u << (mt.groups - 1)) : 0u;
            if ((gm & mask) == 0) continue;
            list.fields_idx[count++] = f;
        }
        list.count = count;

        const uint8_t row = menu_nav_get_select(page);
        uint8_t cursor = 0;
        for (uint8_t i = 0; i < list.count; ++i) {
            const save_field_t f = (save_field_t)list.fields_idx[i];
            const uint8_t span  = is_bits_item_local(f) ? 16u : 1u;
            if (row < (uint8_t)(cursor + span)) {
                gid = menu_controls[f].groups;
                break;
            }
            cursor = (uint8_t)(cursor + span);
        }
    }

    if (!gid) return 0;

    // Find save-based selector that owns this gid and cycles on press
    for (size_t i = 0; i < KSELECTORS_COUNT; ++i) {
        const selector_def_t *sel = &kSelectors[i];
        if (sel->page != page) continue;
        if (sel->kind != SEL_SAVE_BASED) continue;
        if (!sel->cycle_on_press) continue;

        for (uint8_t k = 0; k < sel->cases; ++k) {
            if (sel->groups[k] == gid) {
                if (sel->field != SAVE_FIELD_INVALID) {
                    save_modify_u8(sel->field, SAVE_MODIFY_INCREMENT, 0);
                    return 1; // tell controller to reset cursor
                }
                return 0;
            }
        }
    }
    return 0;
}

// ==============================
// Save helper functions / small UI IO
// ==============================

void menu_change_check(){
    static uint8_t button_pressed = 0;
    if (debounce_button(GPIOB, Btn4_Pin, &button_pressed, 50)) {
        set_current_menu(CURRENT_MENU, UI_MODIFY_INCREMENT, 0);
    }
}

void refresh_screen(){
    menu_list_t menu = (menu_list_t)get_current_menu(CURRENT_MENU);
    threads_display_notify(flag_for_menu(menu));
}

static uint8_t handle_menu_toggle(GPIO_TypeDef *port, uint16_t pin1, uint16_t pin2)
{
    static uint8_t prev_s1 = 1;

    const uint8_t s1 = HAL_GPIO_ReadPin(port, pin1);
    const uint8_t s2 = HAL_GPIO_ReadPin(port, pin2);

    // Rising → falling on s1 while s2 is high
    if (s1 == 0 && prev_s1 == 1 && s2 == 1) {
        osDelay(100);
        // Re-read after debounce
        if (HAL_GPIO_ReadPin(port, pin1) == 0 && HAL_GPIO_ReadPin(port, pin2) == 1) {
            prev_s1 = 0;
            refresh_screen();
            return 1;
        }
    }

    prev_s1 = s1;
    return 0;
}

// Unified “subpage toggle” used by MODIFY and TRANSPOSE
void toggle_subpage(menu_list_t field) {
    if (handle_menu_toggle(GPIOB, Btn1_Pin, Btn2_Pin)) {
        select_press_menu_change(field);  // resets select + rebuilds list
    }
}

void start_stop_pressed() {
    menu_list_t menu = (menu_list_t)get_current_menu(CURRENT_MENU);
    save_field_t field = sending_field_for_menu(menu);
    if (field != SAVE_FIELD_INVALID) {
        save_modify_u8(field, SAVE_MODIFY_INCREMENT, 0);
        if (menu == MENU_TEMPO) { mt_start_stop(&htim2); }
        refresh_screen();
    }
}

void midi_display_on_off(uint8_t on_or_off, uint8_t bottom_line){
    draw_line(92, 10, 92, bottom_line);
    uint8_t text_position = bottom_line/2;
    const char *text_print = message->off_on[on_or_off];
    write_1118(text_print, 95, text_position);
}
