/*
 * _menu_controller.c
 *
 *  Created on: Sep 8, 2025
 *      Author: Romain Dereu
 *
 */
#include "_menu_controller.h"
#include "_menu_ui.h" //get_current_menu
#include "screen_driver.h" //update_contrast

extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;

static uint8_t s_menu_selects[AMOUNT_OF_MENUS] = {0};
static uint8_t s_prev_selects[AMOUNT_OF_MENUS] = {0};

uint32_t s_field_change_bits[CHANGE_BITS_WORDS] = {0};

static volatile uint8_t s_ui_reload = 0;

// -------------------------
// Encoder & value helpers (logic-agnostic)
// -------------------------
static int8_t encoder_read_step(TIM_HandleTypeDef *timer) {
    int32_t delta = __HAL_TIM_GET_COUNTER(timer) - ENCODER_CENTER;
    if (delta <= -ENCODER_THRESHOLD) { __HAL_TIM_SET_COUNTER(timer, ENCODER_CENTER); return -1; }
    if (delta >=  ENCODER_THRESHOLD) { __HAL_TIM_SET_COUNTER(timer, ENCODER_CENTER); return +1; }
    return 0; // no step
}

STATIC_PRODUCTION void no_update(save_field_t field, uint8_t arg) { (void)field; (void)arg; }
STATIC_PRODUCTION void shadow_select(save_field_t field, uint8_t arg) { (void)field; (void)arg; }

STATIC_PRODUCTION void update_value(save_field_t field, uint8_t multiplier)
{
    TIM_HandleTypeDef *timer = &htim4;
    uint8_t active_mult = 1;

    if (multiplier != 1) {
        uint8_t Btn2State = HAL_GPIO_ReadPin(GPIOB, Btn2_Pin);
        active_mult = (Btn2State == 0) ? multiplier : 1;
    }

    int8_t step = encoder_read_step(timer);
    if (step == 0) return;

    const int32_t delta = (int32_t)step * (int32_t)active_mult;
    int32_t cur = (int32_t)save_get(field);
    int32_t next = cur + delta;

    if (u32_fields[field]) { (void)save_modify_u32(field, SAVE_MODIFY_SET, (uint32_t)next); }
    else                  { (void)save_modify_u8 (field, SAVE_MODIFY_SET, (uint8_t) next); }
    s_ui_reload = 1;
}

STATIC_PRODUCTION void update_contrast(save_field_t f, uint8_t step) {
    update_value(f, step);
    screen_driver_UpdateContrast();
}

STATIC_PRODUCTION void update_channel_filter(save_field_t field, uint8_t bit_index)
{
    if (bit_index > 15) return;

    int8_t step = encoder_read_step(&htim4);
    if (step == 0) return;

    uint32_t mask = (uint32_t)save_get(field);
    const uint32_t bit = (1UL << bit_index);

    if (step > 0)  mask |= bit;   // clockwise => enable channel
    else           mask &= ~bit;  // counterclockwise => disable channel

    (void)save_modify_u32(field, SAVE_MODIFY_SET, mask);
    s_ui_reload = 1;
}

// -------------------------
// Controller table (DATA)
// -------------------------

// Put these macros just above the table (or in a shared header if you prefer)
#define MC_BASE (__COUNTER__)
#define MC(field, wrap, handler, arg, group) \
    [field] = { (wrap), (handler), (arg), (group), (uint16_t)(__COUNTER__ - MC_BASE) }

const menu_controls_t menu_controls[SAVE_FIELD_COUNT] = {
    //                                  wrap     handler               handler_arg   group                   ui_order(auto)
    MC(TEMPO_CURRENT_TEMPO,          NO_WRAP, update_value,              10,        CTRL_SHARED_TEMPO),
    MC(TEMPO_CURRENTLY_SENDING,        WRAP,  no_update,                  0,        CTRL_TEMPO_ALL),
    MC(TEMPO_SEND_TO_MIDI_OUT,         WRAP,  update_value,               1,        CTRL_TEMPO_ALL),

    MC(MODIFY_CHANGE_OR_SPLIT,         WRAP,  no_update,                  0,        0),
    MC(MODIFY_VELOCITY_TYPE,           WRAP,  no_update,                  0,        0),

    MC(MODIFY_SEND_TO_MIDI_CH1,      NO_WRAP, update_value,               1,        CTRL_MODIFY_CHANGE),
    MC(MODIFY_SEND_TO_MIDI_CH2,      NO_WRAP, update_value,               1,        CTRL_MODIFY_CHANGE),

    MC(MODIFY_SPLIT_MIDI_CH1,        NO_WRAP, update_value,               1,        CTRL_MODIFY_SPLIT),
    MC(MODIFY_SPLIT_MIDI_CH2,        NO_WRAP, update_value,               1,        CTRL_MODIFY_SPLIT),
    MC(MODIFY_SPLIT_NOTE,            NO_WRAP, update_value,              12,        CTRL_MODIFY_SPLIT),

    MC(MODIFY_SEND_TO_MIDI_OUT,        WRAP,  update_value,               1,        CTRL_MODIFY_ALL),

    MC(MODIFY_VEL_PLUS_MINUS,       NO_WRAP, update_value,              10,        CTRL_MODIFY_VEL_CHANGED),
    MC(MODIFY_VEL_ABSOLUTE,         NO_WRAP, update_value,              10,        CTRL_MODIFY_VEL_FIXED),

    MC(MODIFY_CURRENTLY_SENDING,       WRAP,  no_update,                  0,        CTRL_MODIFY_ALL),

    MC(TRANSPOSE_TRANSPOSE_TYPE,       WRAP,  no_update,                  0,        0),
    MC(TRANSPOSE_MIDI_SHIFT_VALUE,   NO_WRAP, update_value,              12,        CTRL_TRANSPOSE_SHIFT),

    MC(TRANSPOSE_BASE_NOTE,         NO_WRAP, update_value,               1,        CTRL_TRANSPOSE_SCALED),
    MC(TRANSPOSE_INTERVAL,          NO_WRAP, update_value,               1,        CTRL_TRANSPOSE_SCALED),
    MC(TRANSPOSE_TRANSPOSE_SCALE,     WRAP,  update_value,               1,        CTRL_TRANSPOSE_SCALED),

    MC(TRANSPOSE_SEND_ORIGINAL,       WRAP,  update_value,               1,        CTRL_TRANSPOSE_ALL),
    MC(TRANSPOSE_CURRENTLY_SENDING,   WRAP,  no_update,                  0,        0),

    MC(ARPEGGIATOR_CURRENTLY_SENDING,  WRAP,  no_update,                 0,        CTRL_ARPEGGIATOR_PAGE_1),
    MC(ARPEGGIATOR_DIVISION,           WRAP,  update_value,              1,        CTRL_ARPEGGIATOR_PAGE_1),
    MC(ARPEGGIATOR_GATE,               WRAP,  update_value,              1,        CTRL_ARPEGGIATOR_PAGE_1),
    MC(ARPEGGIATOR_OCTAVES,            WRAP,  update_value,              1,        CTRL_ARPEGGIATOR_PAGE_1),
    MC(ARPEGGIATOR_PATTERN,            WRAP,  update_value,              1,        CTRL_ARPEGGIATOR_PAGE_1),

    MC(SETTINGS_START_MENU,            WRAP,  update_value,              1,        CTRL_SETTINGS_GLOBAL1),
    MC(SETTINGS_SEND_USB,              WRAP,  update_value,              1,        CTRL_SETTINGS_GLOBAL1),
    MC(SETTINGS_BRIGHTNESS,          NO_WRAP, update_contrast,           1,        CTRL_SETTINGS_GLOBAL1),

    MC(SETTINGS_MIDI_THRU,             WRAP,  update_value,              1,        CTRL_SETTINGS_GLOBAL2),
    MC(SETTINGS_USB_THRU,              WRAP,  update_value,              1,        CTRL_SETTINGS_GLOBAL2),
    MC(SETTINGS_CHANNEL_FILTER,        WRAP,  update_value,              1,        CTRL_SETTINGS_GLOBAL2),

    MC(SETTINGS_FILTERED_CH,           WRAP,  update_channel_filter,     1,        CTRL_SETTINGS_FILTER),

    MC(SETTINGS_ABOUT,               NO_WRAP, shadow_select,             0,        CTRL_SETTINGS_ABOUT),
};

#undef MC
#undef MC_BASE


// -------------------------
// Utility (pure logic, data-agnostic)
// -------------------------
static inline uint32_t flag_from_id(uint32_t id) { return (id >= 1 && id <= 31) ? (1u << (id - 1)) : 0u; }

static uint8_t rows_for_list(const CtrlActiveList *list) {
    uint8_t rows = 0;
    for (uint8_t i = 0; i < list->count; ++i) {
        save_field_t f = (save_field_t)list->fields_idx[i];
        rows += menu_field_row_span(f);
    }
    return rows;
}

// -------------------------
// Build active list (pure logic)
// -------------------------
static void ctrl_build_active_fields(uint32_t active_groups, CtrlActiveList *out)
{
    uint8_t count = 0;

    for (uint16_t f = 0; f < SAVE_FIELD_COUNT; ++f) {
        const menu_controls_t mt = menu_controls[f];
        const uint32_t gm = flag_from_id(mt.groups);

        if ((gm & active_groups) == 0) continue;
        if (mt.handler == no_update) continue;

        // Insert in ascending ui_order
        uint8_t pos = count;
        while (pos > 0) {
            save_field_t prev_f = (save_field_t)out->fields_idx[pos - 1];
            if (menu_controls[prev_f].ui_order <= mt.ui_order) break;
            out->fields_idx[pos] = out->fields_idx[pos - 1];
            --pos;
        }
        out->fields_idx[pos] = f;

        if (++count >= MENU_ACTIVE_LIST_CAP) break;
    }

    out->count = count;
}


static const CtrlActiveList* get_list_for_page(menu_list_t page)
{
    const uint32_t mask = ctrl_active_mask_for_page(page); // from menus.c
    CtrlActiveList *dst = list_for_page(page);             // from menus.c
    ctrl_build_active_fields(mask, dst);
    return dst;
}

// -------------------------
// Row counting (pure logic)
// -------------------------
static uint8_t ctrl_row_count(const CtrlActiveList *list)
{
    return rows_for_list(list);
}

// -------------------------
// Navigation/select (pure logic)
// -------------------------
static void menu_nav_update_select(menu_list_t page)
{
    uint8_t sel = (page < AMOUNT_OF_MENUS) ? s_menu_selects[page] : 0;

    // Compute rows BEFORE saving prev / reading step
    uint8_t rows = 0;
    CtrlActiveList u = {0};
    if (build_union_for_position_page(page, &u)) { // from menus.c
        rows = rows_for_list(&u);
    } else {
        const CtrlActiveList* list = get_list_for_page(page);
        rows = ctrl_row_count(list);
    }

    if (rows == 0) { if (page < AMOUNT_OF_MENUS) s_menu_selects[page] = 0; return; }
    if (sel >= rows) sel = (uint8_t)(rows - 1);

    // Now that sel is valid for this page, record prev
    if (page < AMOUNT_OF_MENUS) s_prev_selects[page] = sel;

    // Finally read the encoder and apply
    const int8_t step = encoder_read_step(&htim3);
    if (step == 0) return;

    int16_t v = (int16_t)sel + (int16_t)step;
    while (v < 0)     v += rows;
    while (v >= rows) v -= rows;

    if (page < AMOUNT_OF_MENUS) s_menu_selects[page] = (uint8_t)v;
}

uint8_t menu_nav_get_select(menu_list_t page) {
    return (page < AMOUNT_OF_MENUS) ? s_menu_selects[page] : 0;
}

uint32_t ui_active_groups(void) {
    uint8_t m = get_current_menu(CURRENT_MENU);
    if (m >= AMOUNT_OF_MENUS) m = 0;
    return ctrl_active_mask_for_page((menu_list_t)m); // from menus.c
}

void menu_nav_begin_and_update(menu_list_t page) {
    if (page >= AMOUNT_OF_MENUS) return;
    menu_nav_update_select(page);
}

// -------------------------
// Selection resolution (pure logic)
// -------------------------
typedef struct {
    menu_list_t   page;
    uint8_t       row;
    save_field_t  field;
    uint8_t       is_bits;
    uint8_t       bit;
    uint32_t      gid;
} NavSel;

static inline NavSel nav_selection(menu_list_t page)
{
    NavSel s = (NavSel){0};
    if (page >= AMOUNT_OF_MENUS) return s;

    s.page  = page;
    s.row   = menu_nav_get_select(page);
    s.bit   = 0xFF;
    s.field = SAVE_FIELD_INVALID;

    CtrlActiveList u = {0};
    if (build_union_for_position_page(page, &u)) { // position-based page (from menus.c)
        uint8_t cursor = 0;
        for (uint8_t i = 0; i < u.count; ++i) {
            const save_field_t f = (save_field_t)u.fields_idx[i];
            const uint8_t span = menu_field_row_span(f);

            if (s.row < (uint8_t)(cursor + span)) {
                s.field   = f;
                s.is_bits = (span == 16u);
                s.bit     = s.is_bits ? (uint8_t)(s.row - cursor) : 0xFF;
                s.gid     = menu_controls[f].groups;
                return s;
            }
            cursor = (uint8_t)(cursor + span);
        }
        return s;
    }

    // save-based only page: use active list
    const CtrlActiveList *list = get_list_for_page(page);
    uint8_t row_cursor = 0;

    for (uint8_t i = 0; i < list->count; ++i) {
        const save_field_t f = (save_field_t)list->fields_idx[i];
        const uint8_t span = menu_field_row_span(f);
        if (s.row < (uint8_t)(row_cursor + span)) {
            s.field   = f;
            s.is_bits = (span == 16u);
            s.bit     = s.is_bits ? (uint8_t)(s.row - row_cursor) : 0xFF;
            s.gid     = menu_controls[f].groups;
            return s;
        }
        row_cursor = (uint8_t)(row_cursor + span);
    }
    return s;
}

// -------------------------
// Press-to-cycle (menus decides if/what cycles)
// -------------------------
void select_press_menu_change(menu_list_t page) {
    if (menus_cycle_on_press(page)) { // from menus.c
        if (page < AMOUNT_OF_MENUS) s_menu_selects[page] = 0;
    }
}

// -------------------------
// Apply/toggle selected row (pure logic)
// -------------------------
static inline void nav_apply_selection(const NavSel *s)
{
    if (s->field == SAVE_FIELD_INVALID) return;
    if (s->is_bits) { update_channel_filter(s->field, s->bit); return; }
    const menu_controls_t mt = menu_controls[s->field];
    if (mt.handler) mt.handler(s->field, mt.handler_arg);
}

static inline void toggle_selected_row(menu_list_t page)
{
    const NavSel s = nav_selection(page);
    nav_apply_selection(&s);
}

// -------------------------
// Selection queries (pure logic)
// -------------------------
int8_t ui_selected_bit(save_field_t f) {
    if ((unsigned)f >= SAVE_FIELD_COUNT) return -1;

    // Selection is a UI concept → always use the CURRENT menu page.
    menu_list_t page = (menu_list_t)get_current_menu(CURRENT_MENU);
    const NavSel s = nav_selection(page);

    return (s.field == f && s.is_bits) ? (int8_t)s.bit : -1;
}

uint8_t ui_is_field_selected(save_field_t f)
{
    if ((unsigned)f >= SAVE_FIELD_COUNT) return 0;

    // Selection is a UI concept → always use the CURRENT menu page.
    menu_list_t page = (menu_list_t)get_current_menu(CURRENT_MENU);
    const NavSel s = nav_selection(page);

    return (s.field == f) ? 1u : 0u;
}
// -------------------------
// Change-bit tracking (unchanged)
// -------------------------
static inline uint8_t any_field_changed(void)
{
    for (int i = 0; i < CHANGE_BITS_WORDS; ++i)
        if (s_field_change_bits[i]) return 1;
    return 0;
}
static inline void clear_all_field_changed(void)
{
    for (int i = 0; i < CHANGE_BITS_WORDS; ++i)
        s_field_change_bits[i] = 0;
}

void save_mark_all_changed(void) {
    for (int i = 0; i < CHANGE_BITS_WORDS; ++i) s_field_change_bits[i] = 0xFFFFFFFFu;
}

static uint8_t has_menu_changed(menu_list_t page, uint8_t current_select)
{
    const uint8_t old_select  = (page < AMOUNT_OF_MENUS) ? s_prev_selects[page] : 0;
    const uint8_t sel_changed = (page < AMOUNT_OF_MENUS) && (old_select != current_select);
    const uint8_t data_changed = (uint8_t)(any_field_changed() | s_ui_reload);

    if (page < AMOUNT_OF_MENUS) s_menu_selects[page] = current_select;
    if (data_changed) {
        clear_all_field_changed();
        s_ui_reload = 0;
    }

    return (uint8_t)(sel_changed | data_changed);
}

// -------------------------
// End-of-frame + update flow (pure logic)
// -------------------------
static uint8_t menu_nav_end_auto(menu_list_t page)
{
    toggle_selected_row(page);
    const uint8_t sel = menu_nav_get_select(page);
    const uint8_t changed = has_menu_changed(page, sel);
    if (changed) threads_display_notify(flag_for_menu(page));
    return changed;
}

void update_menu()
{
    menu_list_t page = (menu_list_t)get_current_menu(CURRENT_MENU);
    if (page >= AMOUNT_OF_MENUS) page = 0;

    menu_nav_begin_and_update(page);
    cont_update_menu(page);
    (void)menu_nav_end_auto(page);
}
