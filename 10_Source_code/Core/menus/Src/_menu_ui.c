/*
 * _menu_ui.c
 *
 *  Created on: Aug 23, 2025
 *      Author: Astaa
 */

#include "_menu_ui.h"
#include "screen_driver.h"

typedef struct {
    uint8_t current_menu;
    uint8_t old_menu;
} ui_state_t;

// --- function-scoped static state (internal) ---
static inline ui_state_t* get_menus_state(void) {
    static ui_state_t menus_state = { .current_menu = 0, .old_menu = 0 };
    return &menus_state;
}

// Small helpers (internal)
static inline uint8_t menu_wrap(uint8_t v) { return (uint8_t)(v % AMOUNT_OF_MENUS); }
static inline uint8_t ui_menu_current(void) { return get_menus_state()->current_menu; }
static inline uint8_t ui_menu_old(void)     { return get_menus_state()->old_menu; }

static inline void ui_menu_set(uint8_t v) {
    ui_state_t *st = get_menus_state();
    v = menu_wrap(v);
    if (v != st->current_menu) {
        st->old_menu     = st->current_menu;
        st->current_menu = v;
    }
}
static inline void ui_menu_next(void) { ui_menu_set((uint8_t)(get_menus_state()->current_menu + 1u)); }

// Public API kept stable
uint8_t get_current_menu(menu_list_t field) {
    switch (field) {
        case CURRENT_MENU: return ui_menu_current();
        case OLD_MENU:     return ui_menu_old();
        default:           return 0;
    }
}

uint8_t set_current_menu(menu_list_t field, ui_modify_op_t op, uint8_t value_if_set) {
    if (field != CURRENT_MENU) return 1;      // Only CURRENT_MENU is mutable here
    if (op == UI_MODIFY_INCREMENT) { ui_menu_next();            return 1; }
    if (op == UI_MODIFY_SET)       { ui_menu_set(value_if_set); return 1; }
    return 0;
}

// ------------------------------------------------------

void initialize_screen(void){
  screen_driver_Init();
  screen_driver_UpdateContrast();
}

void draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2){
    screen_driver_Line(x1, y1, x2, y2, White );
}

static void draw_text(const char *s, int16_t x, int16_t y, ui_font_t font) {
    if (!s) return;
    switch (font) {
        case UI_6x8:   write_68(s, x, y); break;
        case UI_6x8_2: break;
        case UI_11x18: write_1118(s, x, y); break;
        case UI_16x24: break;
    }
}

static void draw_text_ul(const char *s, int16_t x, int16_t y, ui_font_t font, uint8_t ul) {
    if (!s) return;
    switch (font) {
        case UI_6x8:   write_underline_68(s, x, y, ul);   break;
        case UI_6x8_2: write_underline_68_2(s, x, y, ul); break;
        case UI_11x18: write_underline_1118(s, x, y, ul); break;
        case UI_16x24: write_underline_1624(s, x, y, ul); break;
    }
}

static inline void draw_item_row(const ui_element *e)
{
    const save_field_t  f   = (save_field_t)e->save_item;
    const save_limits_t lim = save_limits[f];

    const int32_t v = save_get(f);

    // Compute table index
    uint32_t idx = (lim.min < 0) ? (uint32_t)(v - lim.min) : (uint32_t)v;

    // Clamp to table range implied by limits
    const uint32_t last = (lim.min < 0)
                        ? (uint32_t)(lim.max - lim.min)
                        : (uint32_t) lim.max;
    if (idx > last) idx = last;

    const char *const *table = (const char *const *)e->text;
    draw_text_ul(table[idx], e->x, e->y, e->font, ui_is_field_selected(f) ? 1u : 0u);
}

static inline uint8_t elem_is_visible(const ui_element *e, uint32_t active_groups_mask)
{
    if (e->ctrl_group_id == 0) return 1;
    uint8_t id = e->ctrl_group_id;
    if (id < 1 || id > 31) return 0;
    uint32_t bit = (1u << (id - 1));
    return (active_groups_mask & bit) ? 1u : 0u;
}

// -------------------------
// 8-step (one-line) drawing
// -------------------------
static void menu_ui_draw_8_steps(const ui_element *e)
{
    const save_field_t f    = (save_field_t)e->save_item;
    const uint32_t     mask = (uint32_t)save_get(f);
    const int8_t       selb = ui_selected_bit(f);

    uint8_t len = (uint8_t)save_get(ARPEGGIATOR_LENGTH);
    if (len < 1u) len = 1u;
    if (len > 8u) len = 8u;

    const int16_t base_x = (int16_t)e->x;
    const int16_t y      = (int16_t)e->y;

    // draw ONLY visible steps
    for (uint8_t i = 0; i < len; ++i) {
        const char *label = (mask & (1u << i)) ? "X" : "0";
        const uint8_t ul  = (selb == (int8_t)i) ? 1u : 0u;

        draw_text_ul(label, (int16_t)(base_x + 10 * i), y, e->font, ul);
    }
}

// -------------------------
// Swing percent drawing (3 chars: " 5%".."99%")
// -------------------------
static inline uint8_t arp_step_ticks(uint8_t div_idx)
{
    static const uint8_t t[7] = { 48u, 32u, 24u, 16u, 12u, 8u, 6u };
    if (div_idx > 6u) div_idx = 6u;
    return t[div_idx];
}

// Convert swing ticks (0..ticks-1) into percent (rounded).
// Note: your controller clamps swing to >= 1, so UI never sees 0 unless old data exists.
static inline uint8_t arp_swing_percent(uint8_t div_idx, uint8_t swing_ticks)
{
    const uint8_t ticks = arp_step_ticks(div_idx);
    if (ticks == 0u) return 0u;

    const uint8_t maxv = (uint8_t)(ticks - 1u);
    if (swing_ticks > maxv) swing_ticks = maxv;

    // rounded: (swing_ticks * 100) / ticks
    const uint16_t p = (uint16_t)swing_ticks * 100u + (ticks / 2u);
    return (uint8_t)(p / ticks);
}

static void menu_ui_draw_swing_percent(const ui_element *e)
{
    const save_field_t swing_f = (save_field_t)e->save_item;

    const uint8_t div_idx = (uint8_t)save_get(ARPEGGIATOR_DIVISION);
    const uint8_t ticks   = arp_step_ticks(div_idx);
    const uint8_t maxv    = (ticks > 0u) ? (uint8_t)(ticks - 1u) : 0u;

    uint8_t swing = (uint8_t)save_get(swing_f);
    if (swing < 1u) swing = 1u;         // enforce “no 0” even if old save exists
    if (swing > maxv) swing = maxv;

    uint8_t pct = arp_swing_percent(div_idx, swing);
    if (pct > 99u) pct = 99u; // keep it 2 digits max for fixed 3-char UI

    char buf[4];
    buf[0] = (pct >= 10u) ? (char)('0' + (pct / 10u)) : ' ';   // leading space for 0..9
    buf[1] = (char)('0' + (pct % 10u));
    buf[2] = '%';
    buf[3] = 0;

    draw_text_ul(buf, e->x, e->y, e->font, ui_is_field_selected(swing_f) ? 1u : 0u);
}

// -------------------------
// 16ch drawing
// -------------------------
static void menu_ui_draw_16ch(const ui_element *e) {
    const save_field_t f    = (save_field_t)e->save_item;
    const uint32_t     mask = (uint32_t)save_get(f);
    const int8_t       selb = ui_selected_bit(f);   // -1 if not selected

    // Use the element's text as the "on" glyph, defaulting to "O"
    const char *on_label = e->text ? e->text : "O";

    const int16_t base_x = (int16_t)e->x;
    const int16_t y1     = (int16_t)e->y;          // first row at LINE_*
    const int16_t y2     = (int16_t)(y1 + 10);     // second row exactly +10 px

    for (uint8_t i = 0; i < 16; ++i) {
        const char  *label = (mask & (1u << i)) ? on_label
                                                : message->one_to_sixteen_one_char[i];
        const int16_t x    = (int16_t)(base_x + 10 * (i % 8));
        const int16_t y    = (i < 8) ? y1 : y2;
        const uint8_t ul   = (selb == (int8_t)i) ? 1u : 0u;

        draw_text_ul(label, x, y, e->font, ul);
    }
}

// One small tick per page
void menu_ui_render(menu_list_t menu, const ui_element *elems, size_t count) {
    (void)menu;

    const uint32_t active = ui_active_groups();

    screen_driver_Fill(Black);

    for (size_t i = 0; i < count; ++i) {
        const ui_element *e = &elems[i];

        if (!elem_is_visible(e, active)) continue;

        switch (e->type) {
            case ELEM_TEXT:     draw_text(e->text, e->x, e->y, e->font); break;
            case ELEM_ITEM:     draw_item_row(e);                        break;
            case ELEM_16CH:     menu_ui_draw_16ch(e);                    break;
            case ELEM_8STEPS:   menu_ui_draw_8_steps(e);                 break;
            case ELEM_SWINGPCT: menu_ui_draw_swing_percent(e);           break;
            default: break;
        }
    }

    ui_code_menu();

    draw_line(0, 10, 127, 10);
    screen_driver_UpdateScreen();
}
