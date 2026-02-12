/* _menu_controller.h
 *
 *  Created on: Sep 8, 2025
 *      Author: Astaa
 */

#ifndef MIDI_INC_MENU_CONTROLLER_H_
#define MIDI_INC_MENU_CONTROLLER_H_

#include <stdint.h>
#include "memory_main.h"   // for save_field_t, SAVE_FIELD_COUNT, etc.
#include "menus.h" // for menu_list_t, CtrlActiveList, list_for_page

// Will be defined somewhere else once functions to be exposed aren't related to memory
#ifndef STATIC_PRODUCTION
#  ifdef UNIT_TEST
#    define STATIC_PRODUCTION   /* empty: export symbol to linker */
#  else
#    define STATIC_PRODUCTION static
#  endif
#endif

// ---------------------
// UI submenu id
// ---------------------
typedef enum {
    CTRL_TEMPO_ALL = 1,
	CTRL_SHARED_TEMPO,

    CTRL_MODIFY_CHANGE,
    CTRL_MODIFY_SPLIT,
    CTRL_MODIFY_ALL,
    CTRL_MODIFY_VEL_CHANGED,
    CTRL_MODIFY_VEL_FIXED,

    CTRL_TRANSPOSE_SHIFT,
    CTRL_TRANSPOSE_SCALED,
    CTRL_TRANSPOSE_ALL,

	CTRL_ARPEGGIATOR_PAGE_1,
	CTRL_ARPEGGIATOR_PAGE_2,

    CTRL_SETTINGS_GLOBAL1,
    CTRL_SETTINGS_GLOBAL2,
    CTRL_SETTINGS_FILTER,
    CTRL_SETTINGS_ALL,
    CTRL_SETTINGS_ABOUT,
    CTRL_SETTINGS_ALWAYS,



} ctrl_group_id_t;



// ---------------------
// Field change bits
// ---------------------
#define CHANGE_BITS_WORDS (((SAVE_FIELD_COUNT) + 31) / 32)
extern uint32_t s_field_change_bits[CHANGE_BITS_WORDS];

// ---------------------
// Wrapping options
// ---------------------
#define WRAP        0
#define NO_WRAP     1

// ---------------------
// Menu controls
// ---------------------
typedef void (*save_handler_t)(save_field_t field, uint8_t arg);

typedef struct {
    uint8_t        wrap;
    void         (*handler)(save_field_t field, uint8_t arg);
    uint8_t        handler_arg;
    uint32_t       groups;
    uint16_t       ui_order;
} menu_controls_t;

extern const menu_controls_t menu_controls[SAVE_FIELD_COUNT];

// =====================
// Display flag helpers
// =====================

uint8_t menu_row_hit(const CtrlActiveList *list, uint8_t row, save_field_t *out_field, uint8_t *out_bit, uint32_t *out_gid);

// Forward declaration used elsewhere
void threads_display_notify(uint32_t flags);

// Return a single-bit mask for a menu.
static inline uint32_t flag_for_menu(menu_list_t m) {
    return (m < AMOUNT_OF_MENUS) ? (1u << (uint32_t)m) : (1u << (uint32_t)MENU_TEMPO);
}

// Menu → "sending" save_field_t lookup
static inline save_field_t sending_field_for_menu(menu_list_t m) {
    switch (m) {
        case MENU_TEMPO:     return TEMPO_CURRENTLY_SENDING;
        case MENU_MODIFY:    return MODIFY_CURRENTLY_SENDING;
        case MENU_TRANSPOSE: return TRANSPOSE_CURRENTLY_SENDING;
        case MENU_SETTINGS:  return SAVE_FIELD_INVALID;
        case MENU_ARPEGGIATOR: return ARPEGGIATOR_CURRENTLY_SENDING;
        default:             return SAVE_FIELD_INVALID;
    }
}

// ---------------------
// Row_span helper
// ---------------------
static inline uint8_t menu_field_row_span(save_field_t f)
{
    switch (f) {
        case SETTINGS_FILTERED_CH:
            return 16u;
        default:
            return 1u;
    }
}

// ---------------------
// UI API
// ---------------------
void     select_press_menu_change(menu_list_t sel_field);

int8_t   ui_selected_bit(save_field_t f);
uint8_t  ui_is_field_selected(save_field_t f);
uint32_t ui_active_groups(void);

void     menu_nav_begin_and_update(menu_list_t field);
void     save_mark_all_changed(void);

uint8_t  menu_nav_get_select(menu_list_t field);

int8_t   filter_selected_bits(save_field_t f); // (if implemented elsewhere)
void     update_menu();

#ifdef UNIT_TEST
void no_update(save_field_t field, uint8_t arg);
void shadow_select(save_field_t field, uint8_t arg);
void update_value(save_field_t field, uint8_t multiplier);
void update_contrast(save_field_t f, uint8_t step);
void update_channel_filter(save_field_t field, uint8_t bit_index);
#endif

#endif /* MIDI_INC_MENU_CONTROLLER_H_ */
