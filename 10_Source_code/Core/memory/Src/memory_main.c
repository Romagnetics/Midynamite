/*
 * memory_main.c
 *
 *  Created on: Aug 23, 2025
 *      Author: Romain Dereu
 */
#include "memory_main.h"
#include "_menu_controller.h" //For STATIC_PRODUCTION

// Expose for tests
const save_limits_t save_limits[SAVE_FIELD_COUNT] = {
    //                                   min         max         default
    [TEMPO_CURRENT_TEMPO]        = {     20,        300,        120 },
    [TEMPO_TEMPO_CLICK_RATE]     = {      1,      50000,         24 },
    [TEMPO_CURRENTLY_SENDING]    = {      0,          1,          0 },
    [TEMPO_SEND_TO_MIDI_OUT]     = {      0,          2,          0 },

    [MODIFY_CHANGE_OR_SPLIT]     = {      0,          1,          1 },
    [MODIFY_VELOCITY_TYPE]       = {      0,          1,          0 },

    [MODIFY_SEND_TO_MIDI_CH1]    = {      1,         16,          1 },
    [MODIFY_SEND_TO_MIDI_CH2]    = {      0,         16,          0 },

    [MODIFY_SPLIT_MIDI_CH1]      = {      1,         16,          1 },
    [MODIFY_SPLIT_MIDI_CH2]      = {      1,         16,          2 },
    [MODIFY_SPLIT_NOTE]          = {      0,        127,         60 },

    [MODIFY_SEND_TO_MIDI_OUT]    = {      0,          2,          0 },

    [MODIFY_VEL_PLUS_MINUS]      = {    -80,         80,          0 },
    [MODIFY_VEL_ABSOLUTE]        = {      0,        127,         64 },

    [MODIFY_SENDING]             = {      0,          1,          0 },

    [TRANSPOSE_TRANSPOSE_TYPE]   = {      0,          1,          0 },
    [TRANSPOSE_MIDI_SHIFT_VALUE] = {    -36,         36,          0 },
    [TRANSPOSE_BASE_NOTE]        = {      0,         11,          0 },
    [TRANSPOSE_INTERVAL]         = {      0,          9,          0 },
    [TRANSPOSE_TRANSPOSE_SCALE]  = {      0,          6,          0 },
    [TRANSPOSE_SEND_ORIGINAL]    = {      0,          1,          0 },
    [TRANSPOSE_SENDING]          = {      0,          1,          0 },

	[ARPEGGIATOR_SENDING]        = {      0,       1,          0 },
	[ARPEGGIATOR_INTERVAL]       = {      4,       32,          4 },
	[ARPEGGIATOR_PATTERN]        = {      0,       10,          0 },
	[ARPEGGIATOR_GATE]        = {      10,       100,          100 },
	[ARPEGGIATOR_OCTAVES]        = {      1,       5,          1 },
	[ARPEGGIATOR_LATCH]        = {      0,       1,          0 },
	[ARPEGGIATOR_SWING]        = {      0,       100,          50 },
	[ARPEGGIATOR_RETRIGGER]        = {      0,       1,          0 },
	[ARPEGGIATOR_LENGTH]   = {      1,       8,          8 },
	[ARPEGGIATOR_NOTES]  = {      1,       0b11111111,    0b11111111 },

    [SETTINGS_START_MENU]        = {      0,          AMOUNT_OF_MENUS-1,          0 },
    [SETTINGS_SEND_USB]          = {      0,          1,          0 },
    [SETTINGS_BRIGHTNESS]        = {      0,          9,          6 },
    [SETTINGS_MIDI_THRU]         = {      0,          1,          0 },
    [SETTINGS_USB_THRU]          = {      0,          1,          0 },
    [SETTINGS_CHANNEL_FILTER]    = {      0,          1,          0 },
    [SETTINGS_FILTERED_CH] = {      0,  0x0000FFFF,          0 },
    [SETTINGS_ABOUT]             = {      0,          0,          0 },

    [SAVE_DATA_VALIDITY]         = {      0,  0xFFFFFFFF, DATA_VALIDITY_CHECKSUM },
};

static volatile uint8_t save_busy = 0;

// ---------------------
// Lock helpers
// ---------------------
static int save_try_lock(void) {
    if (save_busy) return 0;
    save_busy = 1;
    return 1;
}

void save_unlock(void) { save_busy = 0; }

// (Optional) bounded retry for writers; cheap and unit-test friendly
uint8_t save_lock_with_retries(void) {
    for (int i = 0; i < 5; i++) {
        if (save_try_lock()) return 1;
        // short, bounded spin
        volatile int spin = 200;
        while (spin--) { /* no-op */ }
    }
    return 0;
}


// ---------------------
// Generic getters (BUSY-safe)
// ---------------------
// Small bounded wait helper used by getters (non-blocking feel)
static inline void save_busy_wait_short(void) {
    // ~ a few hundred no-ops; tweak if needed
    volatile int spin = 200;
    while (spin--) { /* no-op */ }
}

static inline int32_t clamp_i32(int32_t v, int32_t lo, int32_t hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}




static int32_t save_get_u32(save_field_t field) {
    if (field < 0 || field >= SAVE_FIELD_COUNT) return 0;
    int32_t *p = u32_fields[field];
    if (!p) return 0;

    for (int i = 0; i < 5; ++i) {
        if (!save_busy) break;
        save_busy_wait_short();
    }

    int32_t v = *p;
    const save_limits_t lim = save_limits[field];
    return clamp_i32(v, lim.min, lim.max);
}

static uint8_t save_get_8(save_field_t field) {
    if (field < 0 || field >= SAVE_FIELD_COUNT) return 0;
    uint8_t *p = u8_fields[field];
    if (!p) return 0;

    for (int i = 0; i < 5; ++i) {
        if (!save_busy) break;
        save_busy_wait_short();
    }

    int32_t v = (int32_t)(*p);
    const save_limits_t lim = save_limits[field];
    return (uint8_t)clamp_i32(v, lim.min, lim.max);
}


int32_t save_get(save_field_t field) {
    if (field < 0 || field >= SAVE_FIELD_COUNT) return 0;

    // Prefer exact width based on backing storage
    if (u8_fields[field])  return (int32_t)save_get_8(field);
    if (u32_fields[field]) return        save_get_u32(field);

    return 0;
}


// ---------------------
// Increment / set (u32 / u8)
// ---------------------
static void mark_field_changed(save_field_t f) {
    if ((unsigned)f >= SAVE_FIELD_COUNT) return;
    s_field_change_bits[f >> 5] |= (1u << (f & 31));
}

// Utils: wrap/clamp a value into [min, max] with optional wrap
STATIC_PRODUCTION int32_t wrap_or_clamp_i32(int32_t v, int32_t min, int32_t max, uint8_t wrap)
{
    if (min > max) { int32_t t = min; min = max; max = t; }

    if (!wrap) {
        if (v < min) return min;
        if (v > max) return max;
        return v;
    }

    // Inclusive span so [1..16] reaches 16
    const int32_t span = (max - min) + 1;
    if (span <= 0) return min;

    int32_t off = (v - min) % span;
    if (off < 0) off += span;
    return min + off;
}


uint8_t save_modify_u32(save_field_t field, save_modify_op_t op, uint32_t value_if_set) {
    if (field < 0 || field >= SAVE_FIELD_COUNT) return 0;
    if (!u32_fields[field]) return 0;
    if (!save_lock_with_retries()) return 0;

    const save_limits_t   lim = save_limits[field];
    const menu_controls_t mt  = menu_controls[field];
    int32_t old_v = *u32_fields[field];
    int32_t v = old_v;

    switch (op) {
        case SAVE_MODIFY_INCREMENT: v += 1; break;
        case SAVE_MODIFY_SET:       v  = (int32_t)value_if_set; break;
        default: save_unlock(); return 0;
    }

    v = wrap_or_clamp_i32(v, lim.min, lim.max, mt.wrap);

    if (v != old_v) {
        *u32_fields[field] = v;
        mark_field_changed(field);
    }

    save_unlock();
    return 1;
}

uint8_t save_modify_u8(save_field_t field, save_modify_op_t op, uint8_t value_if_set) {
    if (field < 0 || field >= SAVE_FIELD_COUNT) return 0;
    if (!u8_fields[field]) return 0;
    if (!save_lock_with_retries()) return 0;

    const save_limits_t   lim = save_limits[field];
    const menu_controls_t mt  = menu_controls[field];

    uint8_t old_v = *u8_fields[field];
    int32_t v = (int32_t)old_v;

    switch (op) {
        case SAVE_MODIFY_SET: {
            int32_t desired = (int32_t)value_if_set;
            if (desired > 230) {//For warps, 255 + 25 buffer for button presses
                v = mt.wrap ? lim.max : lim.min;
            } else {
                v = desired;
            }
        } break;

        case SAVE_MODIFY_INCREMENT:
            v += 1;
            break;

        default:
            save_unlock();
            return 0;
    }

    v = wrap_or_clamp_i32(v, lim.min, lim.max, mt.wrap);
    uint8_t new_v = (uint8_t)v;

    if (new_v != old_v) {
        *u8_fields[field] = new_v;
        mark_field_changed(field);
    }

    save_unlock();
    return 1;
}
