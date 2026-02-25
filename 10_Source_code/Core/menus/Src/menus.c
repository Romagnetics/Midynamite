/*
 * menus.c
 *
 *  Created on: Sep 13, 2025
 *      Author: Astaa
 */
#include <stddef.h>
#include "cmsis_os2.h" //For osDelay
#include "main.h" // Timer
#include "menus.h"
#include "_menu_controller.h" // For menu_field_row_span
#include "_menu_ui.h" // menu change functions
#include "midi_tempo.h" //mt_start_stop
#include "screen_driver.h" //Font
#include "stm32f4xx_hal.h"   // HAL types (TIM, GPIO)
#include "utils.h" // Debounce

extern TIM_HandleTypeDef htim2;

// ==============================
// UI update fan-out
// ==============================

typedef void (*ui_fn0_t)(void);
typedef void (*cont_fn1_t)(menu_list_t);

typedef struct {
    ui_fn0_t   ui_update;
    ui_fn0_t   ui_code;
    cont_fn1_t cont_update;
} MenuVTable;

static inline menu_list_t current_menu(void) {
    uint8_t m = get_current_menu(CURRENT_MENU);
    return (m < AMOUNT_OF_MENUS) ? (menu_list_t)m : MENU_TEMPO;
}

static const MenuVTable kMenuVT[AMOUNT_OF_MENUS] = {
    [MENU_TEMPO]       = { ui_update_tempo,       ui_code_tempo,       NULL },
    [MENU_MODIFY]      = { ui_update_modify,      ui_code_modify,      cont_update_modify },
    [MENU_TRANSPOSE]   = { ui_update_transpose,   ui_code_transpose,   cont_update_transpose },
    [MENU_ARPEGGIATOR] = { ui_update_arpeggiator, ui_code_arpeggiator, cont_update_arpeggiator },
    [MENU_SETTINGS]    = { ui_update_settings,    ui_code_settings,    (cont_fn1_t)cont_update_settings },
};

void screen_update_menu(uint32_t flag){
    const menu_list_t m = current_menu();
    if (flag & flag_for_menu(m)) kMenuVT[m].ui_update();
}

void ui_code_menu(void){
    const menu_list_t m = current_menu();
    kMenuVT[m].ui_code();
}

void cont_update_menu(menu_list_t field){
    const menu_list_t m = current_menu();
    if (kMenuVT[m].cont_update) {
        kMenuVT[m].cont_update(field);
    }
}

// ==============================
// Selector DATA & computes
// ==============================

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

// save-based computes
static uint8_t sel_mod_change_split() { return (save_get(MODIFY_CHANGE_OR_SPLIT)  == MIDI_MODIFY_SPLIT)    ? 1 : 0; }
static uint8_t sel_mod_vel_type()     { return (save_get(MODIFY_VELOCITY_TYPE)    == MIDI_MODIFY_FIXED_VEL) ? 1 : 0; }
static uint8_t sel_transpose_type()   { return (save_get(TRANSPOSE_TRANSPOSE_TYPE)== MIDI_TRANSPOSE_SCALED) ? 1 : 0; }
static uint8_t sel_fixed0()           { return 0u; }

// -------------------------
// Group-id lists (avoid compound literals in static tables)
// -------------------------
static const ctrl_group_id_t GR_TEMPO_ALL[]        = { CTRL_TEMPO_ALL, CTRL_SHARED_TEMPO, 0};

static const ctrl_group_id_t GR_MODIFY_ALL[]       = { CTRL_MODIFY_ALL };
static const ctrl_group_id_t GR_MODIFY_TYPE[]      = { CTRL_MODIFY_CHANGE, CTRL_MODIFY_SPLIT };
static const ctrl_group_id_t GR_MODIFY_VEL_TYPE[]  = { CTRL_MODIFY_VEL_CHANGED, CTRL_MODIFY_VEL_FIXED };

static const ctrl_group_id_t GR_TRANSPOSE_ALL[]    = { CTRL_TRANSPOSE_ALL };
static const ctrl_group_id_t GR_TRANSPOSE_TYPE[]   = { CTRL_TRANSPOSE_SHIFT, CTRL_TRANSPOSE_SCALED };

static const ctrl_group_id_t GR_ARP_SECTIONS[] = {CTRL_ARPEGGIATOR_PAGE_1, CTRL_ARPEGGIATOR_PAGE_2,};

static const ctrl_group_id_t GR_ARP_TEMPO_GATE[] = {CTRL_SHARED_TEMPO, 0};

static const ctrl_group_id_t GR_SETTINGS_ALWAYS[]  = { CTRL_SETTINGS_ALWAYS };
static const ctrl_group_id_t GR_SETTINGS_SECTIONS[] = {
    CTRL_SETTINGS_GLOBAL1,
    CTRL_SETTINGS_GLOBAL2,
    CTRL_SETTINGS_FILTER,
    CTRL_SETTINGS_ABOUT
};



// Selector table (DATA only, page-driven)
static const page_group_rule_t kPageGroupRules[] = {
    { GROUP_STATE_BASED, 1, GR_TEMPO_ALL,         SAVE_FIELD_INVALID,          sel_fixed0,           0, MENU_TEMPO },

    { GROUP_STATE_BASED, 1, GR_MODIFY_ALL,        SAVE_FIELD_INVALID,          sel_fixed0,           0, MENU_MODIFY },
    { GROUP_STATE_BASED, 2, GR_MODIFY_TYPE,       MODIFY_CHANGE_OR_SPLIT,      sel_mod_change_split, 1, MENU_MODIFY },
    { GROUP_STATE_BASED, 2, GR_MODIFY_VEL_TYPE,   MODIFY_VELOCITY_TYPE,        sel_mod_vel_type,     1, MENU_MODIFY },

    { GROUP_STATE_BASED, 1, GR_TRANSPOSE_ALL,     SAVE_FIELD_INVALID,          sel_fixed0,           0, MENU_TRANSPOSE },
    { GROUP_STATE_BASED, 2, GR_TRANSPOSE_TYPE,    TRANSPOSE_TRANSPOSE_TYPE,    sel_transpose_type,   1, MENU_TRANSPOSE },

    { GROUP_STATE_BASED, 1, GR_SETTINGS_ALWAYS,   SAVE_FIELD_INVALID,          sel_fixed0,           0, MENU_SETTINGS },
    { CURRENT_POSITION_BASED, 4, GR_SETTINGS_SECTIONS, SAVE_FIELD_INVALID,     NULL,                0, MENU_SETTINGS },

	{ CURRENT_POSITION_BASED, 2, GR_ARP_SECTIONS,    SAVE_FIELD_INVALID, NULL, 0, MENU_ARPEGGIATOR },
	{ CURRENT_POSITION_BASED, 2, GR_ARP_TEMPO_GATE,  SAVE_FIELD_INVALID, NULL, 0, MENU_ARPEGGIATOR },
};

#define KPAGEGROUPRULES_COUNT (sizeof(kPageGroupRules) / sizeof(kPageGroupRules[0]))


// -------------------------
// Active lists cache per page
// -------------------------
typedef struct {
    CtrlActiveList list[AMOUNT_OF_MENUS];
} MenuActiveLists;

static MenuActiveLists s_menu_lists;

CtrlActiveList* list_for_page(menu_list_t page)
{
    if (page >= AMOUNT_OF_MENUS) page = MENU_TEMPO;
    return &s_menu_lists.list[page];
}


// -------------------------
// Local helpers (private to menus.c)
// -------------------------

static void build_union_for_groups_local(const ctrl_group_id_t *groups, uint8_t n_groups, CtrlActiveList *out)
{
    uint8_t count = 0;

    for (uint16_t f = 0; f < SAVE_FIELD_COUNT && count < MENU_ACTIVE_LIST_CAP; ++f) {
        const menu_controls_t mt = menu_controls[f];

        // must be one of requested groups
        uint8_t ok = 0;
        for (uint8_t g = 0; g < n_groups; ++g) {
        	 if (groups[g] == 0) break;
        	 if (mt.groups == groups[g]) { ok = 1; break; }
        }
        if (!ok) continue;

        // only include selectable fields
        if (mt.handler == NULL) continue;

        // insert sorted by ui_order (same ordering rule as controller uses)
        uint8_t pos = count;
        while (pos > 0) {
            const save_field_t prev_f = (save_field_t)out->fields_idx[pos - 1];
            if (menu_controls[prev_f].ui_order <= mt.ui_order) break;
            out->fields_idx[pos] = out->fields_idx[pos - 1];
            --pos;
        }
        out->fields_idx[pos] = f;
        ++count;
    }

    out->count = count;
}


static uint8_t idx_from_save(save_field_t f, uint8_t case_count) {
    int32_t v = (f != SAVE_FIELD_INVALID) ? save_get(f) : 0;
    if (v < 0) v = 0;
    uint8_t idx = (uint8_t)v;
    return (idx < case_count) ? idx : 0;
}


static uint8_t group_list_len(const ctrl_group_id_t *groups)
{
    uint8_t n = 0;
    while (n < 31 && groups[n] != 0) n++;   // 0 terminator
    return n;
}

static uint8_t idx_from_position_selector(const page_group_rule_t *sel)
{
    CtrlActiveList u = {0};
    if (!build_union_for_position_page(sel->page, &u)) return 0;

    const uint8_t sel_row = menu_nav_get_select(sel->page);
    uint8_t cursor = 0;

    for (uint8_t i = 0; i < u.count; ++i) {
        const save_field_t f = (save_field_t)u.fields_idx[i];
        const uint8_t span = menu_field_row_span(f);

        if (sel_row < (uint8_t)(cursor + span)) {
            const ctrl_group_id_t gid = (ctrl_group_id_t)menu_controls[f].groups;

            // Map gid -> index within THIS selector's groups[]
            const uint8_t n_groups =
                (sel->case_count == 1) ? group_list_len(sel->groups) : sel->case_count;

            for (uint8_t k = 0; k < n_groups; ++k) {
                if (sel->groups[k] == 0) break;
                if (sel->groups[k] == gid) return k;
            }
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

    uint8_t  have_pos_idx = 0;
    uint8_t  pos_idx = 0;

    for (size_t i = 0; i < KPAGEGROUPRULES_COUNT; ++i) {
        const page_group_rule_t *sel = &kPageGroupRules[i];
        if (sel->page != page) continue;

        uint8_t idx = 0;

        if (sel->kind == GROUP_STATE_BASED) {
            idx = sel->compute ? sel->compute() : idx_from_save(sel->field, sel->case_count);
        } else {
            if (!have_pos_idx) {
                pos_idx = idx_from_position_selector(sel); // compute once
                have_pos_idx = 1;
            }
            idx = pos_idx; // reuse
        }

        if (idx >= sel->case_count) idx = 0;

        if (sel->case_count == 1) {
            const uint8_t n = group_list_len(sel->groups);
            for (uint8_t k = 0; k < n; ++k) {
                const uint32_t gid = (uint32_t)sel->groups[k];
                if (gid >= 1u && gid <= 31u) mask |= (1u << (gid - 1u));
            }
        } else {
            const uint32_t gid = (uint32_t)sel->groups[idx];
            if (gid >= 1u && gid <= 31u) mask |= (1u << (gid - 1u));
        }
    }
    return mask;
}



uint8_t build_union_for_position_page(menu_list_t page, CtrlActiveList *out)
{
    // Collect unique group IDs from ALL CURRENT_POSITION_BASED rules for this page
    ctrl_group_id_t tmp[32] = {0};
    uint8_t tmp_count = 0;

    for (size_t i = 0; i < KPAGEGROUPRULES_COUNT; ++i) {
        const page_group_rule_t *sel = &kPageGroupRules[i];
        if (sel->page != page) continue;
        if (sel->kind != CURRENT_POSITION_BASED) continue;

        const uint8_t n =
            (sel->case_count == 1) ? group_list_len(sel->groups) : sel->case_count;

        for (uint8_t k = 0; k < n; ++k) {
            const ctrl_group_id_t gid = sel->groups[k];
            if (gid == 0) continue; // allow gating arrays like {CTRL_SHARED_TEMPO, 0}

            // insert if not already present
            uint8_t exists = 0;
            for (uint8_t j = 0; j < tmp_count; ++j) {
                if (tmp[j] == gid) { exists = 1; break; }
            }
            if (!exists && tmp_count < (uint8_t)(sizeof(tmp)/sizeof(tmp[0]) - 1u)) {
                tmp[tmp_count++] = gid;
                tmp[tmp_count] = 0; // keep 0-terminated
            }
        }
    }

    if (tmp_count == 0) return 0;

    build_union_for_groups_local(tmp, tmp_count, out);
    return 1;
}


uint8_t menus_cycle_on_press(menu_list_t page)
{
    const uint8_t row = menu_nav_get_select(page);

    uint32_t gid = 0;
    CtrlActiveList u = {0};

    if (build_union_for_position_page(page, &u)) {
        (void)menu_row_hit(&u, row, NULL, NULL, &gid);
    } else {
        // Build active list from mask, in correct ui_order (shared implementation)
        const uint32_t mask = ctrl_active_mask_for_page(page);
        CtrlActiveList *dst = list_for_page(page);
        ctrl_build_active_fields(mask, dst);

        (void)menu_row_hit(dst, row, NULL, NULL, &gid);
    }


    if (!gid) return 0;

    for (size_t i = 0; i < KPAGEGROUPRULES_COUNT; ++i) {
        const page_group_rule_t *sel = &kPageGroupRules[i];
        if (sel->page != page) continue;
        if (sel->kind != GROUP_STATE_BASED) continue;
        if (!sel->cycle_on_press) continue;

        const uint8_t n = (sel->case_count == 1) ? group_list_len(sel->groups) : sel->case_count;

        for (uint8_t k = 0; k < n; ++k) {
            if (sel->case_count == 1 && sel->groups[k] == 0) break;
            if ((uint32_t)sel->groups[k] != gid) continue;

            if (sel->field != SAVE_FIELD_INVALID) {
                save_modify_u8(sel->field, SAVE_MODIFY_INCREMENT, 0);
                return 1;
            }
            return 0;
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
    threads_display_notify(flag_for_menu(current_menu()));
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
        if (menu == MENU_TEMPO) { mt_start_stop(); }
        refresh_screen();
    }
}

void midi_display_on_off(uint8_t on_or_off, uint8_t bottom_line){
    draw_line(ON_OFF_VERT_LINE, 10, ON_OFF_VERT_LINE, bottom_line);
    uint8_t text_position = bottom_line/2-2;
    const char *text_print = message->off_on[on_or_off];
    write_1118(text_print, 95, text_position);
}
