/*
 * midi_transform.c
 *
 *  Created on: Sep 5, 2025
 *      Author: Astaa
 */


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "menus.h"

#include "midi_transform.h"
#include "midi_arp.h"

#include "main.h"
#include "midi_usb.h"
#include "memory_main.h"

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

// Circular buffer instance declared externally
extern midi_modify_circular_buffer midi_modify_buff;


// ---------------------
// Helpers
// ---------------------
static inline uint8_t midi_msg_len(uint8_t status)
{
    // Realtime (F8–FF) are 1 byte
    if (status >= 0xF8) return 1;

    switch (status) {
        case 0xF1: return 2; // MTC Quarter Frame
        case 0xF2: return 3; // Song Position
        case 0xF3: return 2; // Song Select
        case 0xF6: return 1; // Tune Request
        case 0xF7: return 1; // EOX
        default: break;
    }

    // Channel voice:
    const uint8_t hi = (uint8_t)(status & 0xF0);
    return (hi == 0xC0 || hi == 0xD0) ? 2 : 3;
}

static inline uint8_t midi_is_channel_voice(uint8_t status)
{
    const uint8_t hi = (uint8_t)(status & 0xF0);
    return (hi >= 0x80 && hi <= 0xE0) ? 1 : 0;
}


uint8_t midi_is_note_message(const midi_note *msg, uint8_t *is_note_on)
{
    if ((msg == NULL) || (is_note_on == NULL)) {
        return 0;
    }

    const uint8_t status_nibble = (uint8_t)(msg->status & 0xF0);

    if (status_nibble == 0x90) {
        *is_note_on = (msg->velocity > 0) ? 1 : 0;
        return 1;
    }

    if (status_nibble == 0x80) {
        *is_note_on = 0;
        return 1;
    }

    return 0;
}



static void change_midi_channel(midi_note *midi_msg, uint8_t send_to_midi_channel) {
    uint8_t status = midi_msg->status;

    if (status >= 0x80 && status <= 0xEF) {
        uint8_t status_nibble = status & 0xF0;
        midi_msg->status = status_nibble | ((send_to_midi_channel - 1) & 0x0F);
    }
}




static void change_velocity(midi_note *midi_msg)
{
    const uint8_t status_nibble = (uint8_t)(midi_msg->status & 0xF0);
    uint8_t is_note_on = 0;
    const uint8_t is_note = midi_is_note_message(midi_msg, &is_note_on);

    const uint8_t is_cc = (status_nibble == 0xB0) ? 1 : 0;
    const uint8_t is_cc64 = (is_cc && midi_msg->note == 64) ? 1 : 0;

    // Velocity shaping applies to note messages and to CC values,
    // except sustain pedal (CC64) which must remain untouched.
    if (!is_note && !is_cc) {
        return;
    }
    if (is_cc64) {
        return;
    }

    // Keep explicit Note Off with velocity 0 unchanged,
    // while allowing non-zero Note Off release velocity to be shaped.
    if (is_note && is_note_on == 0 && midi_msg->velocity == 0) {
        return;
    }


    // int32 to avoid overflow
    int32_t velocity = (int32_t)midi_msg->velocity;

    if (save_get(MODIFY_VELOCITY_TYPE) == MIDI_MODIFY_CHANGED_VEL) {
        velocity += (int32_t)save_get(MODIFY_VEL_PLUS_MINUS);
    } else if (save_get(MODIFY_VELOCITY_TYPE) == MIDI_MODIFY_FIXED_VEL) {
        velocity = (int32_t)save_get(MODIFY_VEL_ABSOLUTE);
    }

    if (velocity < 0)   velocity = 0;
    if (velocity > 127) velocity = 127;

    midi_msg->velocity = (uint8_t)velocity;
}


// ---------------------
// Circular buffer logic
// ---------------------
void midi_buffer_push(uint8_t byte) {
    uint16_t next = (midi_modify_buff.head + 1) % MIDI_MODIFY_BUFFER_SIZE;
    if (next != midi_modify_buff.tail) { // Not full
        midi_modify_buff.data[midi_modify_buff.head] = byte;
        midi_modify_buff.head = next;
    }
}

uint8_t midi_buffer_pop(uint8_t *byte) {
    if (midi_modify_buff.head == midi_modify_buff.tail)
        return 0; // Empty
    *byte = midi_modify_buff.data[midi_modify_buff.tail];
    midi_modify_buff.tail = (midi_modify_buff.tail + 1) % MIDI_MODIFY_BUFFER_SIZE;
    return 1;
}



// ---------------------
// MIDI parse & dispatch
// ---------------------
void calculate_incoming_midi() {
    static uint8_t running_status = 0;
    static midi_note msg;
    static uint8_t byte_count = 0;
    static uint8_t expected_length = 0;

    uint8_t byte;
    while (midi_buffer_pop(&byte)) {
        // Real-time messages (do not affect parsing)
        if (byte >= 0xF8) {
            if (save_get(SETTINGS_MIDI_THRU) == 1) {
                midi_note rt = { .status = byte, .note = 0, .velocity = 0 };
                pipeline_final(&rt, 1);
            }
            continue;
        }

        // SysEx (skip until 0xF7)
        if (byte == 0xF0) {
            running_status = 0;
            while (midi_buffer_pop(&byte) && byte != 0xF7);
            continue;
        }

        if (byte & 0x80) {
            // Status byte
            if (byte >= 0xF0) {
                // System common — unsupported
                running_status = 0;
                continue;
            }

            running_status = byte;
            msg.status = byte;

            uint8_t status_nibble = byte & 0xF0;
            expected_length = (status_nibble == 0xC0 || status_nibble == 0xD0) ? 2 : 3;
            byte_count = 1; // waiting for data
            continue;
        }

        // Data byte
        if (running_status == 0) continue; // no valid running status

        if (byte_count == 0) {
            // We assume running status is in effect, start a new message
            msg.status = running_status;

            uint8_t status_nibble = running_status & 0xF0;
            expected_length = (status_nibble == 0xC0 || status_nibble == 0xD0) ? 2 : 3;

            msg.note = byte;
            byte_count = 2;
            if (expected_length == 2) {
                pipeline_start(&msg);
                byte_count = 0;
            }
        } else if (byte_count == 1) {
            msg.note = byte;
            byte_count++;
            if (expected_length == 2) {
                pipeline_start(&msg);
                byte_count = 0;
            }
        } else if (byte_count == 2) {
            msg.velocity = byte;
            pipeline_start(&msg);
            byte_count = 0;
        }
    }
}

// ---------------------
// Filters / blockers
// ---------------------
static uint8_t is_channel_blocked(uint8_t status_byte) {
    if (!save_get(SETTINGS_CHANNEL_FILTER)) return 0;

    uint8_t status_nibble = status_byte & 0xF0;
    uint8_t channel = status_byte & 0x0F;

    if (status_nibble >= 0x80 && status_nibble <= 0xE0) {
        return (save_get(SETTINGS_FILTERED_CH) >> channel) & 0x01;
    }

    return 0;
}





// ---------------------
// Pipeline entry
// ---------------------
void pipeline_start(midi_note *midi_msg)
{
    const uint8_t status = midi_msg->status;
    const uint8_t length = midi_msg_len(status);
    const uint8_t status_nibble = (uint8_t)(status & 0xF0);
    const uint8_t is_cc64 = ((status_nibble == 0xB0) && ((midi_msg->note & 0x7F) == 64)) ? 1 : 0;


    arp_handle_midi_cc64(midi_msg);

    if ((save_get(ARPEGGIATOR_CURRENTLY_SENDING) == 1) && (is_cc64 != 0)) {
        // While arp is active, sustain pedal is an internal hold input only.
        // Do not forward it, otherwise destination synth sustain overrides arp gate timing.
        return;
    }


    if (is_channel_blocked(status)) return;

    if (save_get(SPLIT_CURRENTLY_SENDING) == 1) {
        pipeline_midi_split(midi_msg);
        return;
    }


    if (save_get(MODIFY_CURRENTLY_SENDING) == 1) {
        pipeline_midi_modify(midi_msg);
        return;
    }

    if (save_get(TRANSPOSE_CURRENTLY_SENDING) == 1) {
        pipeline_midi_transpose(midi_msg);
        return;
    }

    if (save_get(ARPEGGIATOR_CURRENTLY_SENDING) == 1) {
        pipeline_arp(midi_msg, length);
        return;
    }

    if (save_get(SETTINGS_MIDI_THRU) == 1) {
        send_midi_out(midi_msg, length);
    }
    if (save_get(SETTINGS_USB_THRU) == 1) {
        send_usb_midi_out(midi_msg, length);
    }
}

// ---------------------
// Split pipeline
// ---------------------
void pipeline_midi_split(midi_note *midi_msg)
{
    midi_note split_msg = *midi_msg;
    uint8_t is_note_on = 0;
    uint8_t send_dry = 0;

    if (midi_is_note_message(&split_msg, &is_note_on) != 0) {
        const uint8_t split_is_high = (split_msg.note >= save_get(SPLIT_NOTE));
        const uint8_t split_channel = (split_is_high != 0)
                                      ? (uint8_t)save_get(SPLIT_MIDI_CH2)
                                      : (uint8_t)save_get(SPLIT_MIDI_CH1);
        const save_field_t send_field = (split_is_high != 0) ? SPLIT_SEND_CH2 : SPLIT_SEND_CH1;
        send_dry = (uint8_t)(save_get(send_field) == 0);
        change_midi_channel(&split_msg, split_channel);

        if (send_dry != 0) {
            pipeline_final(&split_msg, midi_msg_len(split_msg.status));
            return;
        }
    }

    if (save_get(MODIFY_CURRENTLY_SENDING) == 1) {
        pipeline_midi_modify(&split_msg);
        return;
    }

    pipeline_midi_transpose(&split_msg);
}


// ---------------------
// Modify pipeline
// ---------------------
void pipeline_midi_modify(midi_note *midi_msg) {
    change_velocity(midi_msg);

    if (save_get(MODIFY_SEND_TO_MIDI_CH2) != 0) {
        midi_note midi_note_1 = *midi_msg;
        uint8_t send_ch_1 = (uint8_t)save_get(MODIFY_SEND_TO_MIDI_CH1);
        change_midi_channel(&midi_note_1, send_ch_1);
        pipeline_midi_transpose(&midi_note_1);

        midi_note midi_note_2 = *midi_msg;
        uint8_t send_ch_2 = (uint8_t)save_get(MODIFY_SEND_TO_MIDI_CH2);
        change_midi_channel(&midi_note_2, send_ch_2);
        pipeline_midi_transpose(&midi_note_2);

    } else {
        midi_note midi_note_1 = *midi_msg;
        uint8_t send_ch_1 = (uint8_t)save_get(MODIFY_SEND_TO_MIDI_CH1);
        change_midi_channel(&midi_note_1, send_ch_1);
        pipeline_midi_transpose(&midi_note_1);
    }
}


// ---------------------
// Scale helpers
// ---------------------
static void get_mode_scale(uint8_t mode, uint8_t *scale) {
    const uint8_t mode_intervals[7][7] = {
        {0, 2, 4, 5, 7, 9, 11},  // Ionian
        {0, 2, 3, 5, 7, 9, 10},  // Dorian
        {0, 1, 3, 5, 7, 8, 10},  // Phrygian
        {0, 2, 4, 6, 7, 9, 11},  // Lydian
        {0, 2, 4, 5, 7, 9, 10},  // Mixolydian
        {0, 2, 3, 5, 7, 8, 10},  // Aeolian
        {0, 1, 3, 5, 6, 8, 10}   // Locrian
    };
    memcpy(scale, mode_intervals[mode % AMOUNT_OF_MODES], 7);
}

static uint8_t note_in_scale(uint8_t note, const uint8_t *scale, uint8_t base_note) {
    int semitone_from_root = ((note - base_note) + 120) % 12;
    for (int i = 0; i < 7; i++) {
        if (scale[i] == semitone_from_root) return 1;
    }
    return 0;
}

// Snap note up/down by semitones until in scale (max 12 semitones search)
static uint8_t snap_note_to_scale(uint8_t note, const uint8_t *scale, uint8_t base_note) {
    uint8_t up_note = note;
    uint8_t down_note = note;

    for (int i = 0; i < 12; i++) {
        if (note_in_scale(down_note, scale, base_note)) return down_note;
        if (note_in_scale(up_note,  scale, base_note)) return up_note;

        if (down_note > 0)   down_note--;
        if (up_note   < 127) up_note++;
    }
    return note;  // fallback (should not happen)
}

// ---------------------
// Transpose math
// ---------------------
static uint8_t midi_transpose_notes(uint8_t note) {
    uint8_t mode = save_get(TRANSPOSE_TRANSPOSE_SCALE) % AMOUNT_OF_MODES;

    uint8_t scale_intervals[7];
    get_mode_scale(mode, scale_intervals);

    uint8_t base = save_get(TRANSPOSE_BASE_NOTE);

    // Snap note to scale if not in scale
    if (!note_in_scale(note, scale_intervals, base)) {
        note = snap_note_to_scale(note, scale_intervals, base);
    }

    int16_t semitone_from_root = ((note - base) + 120) % 12;
    int16_t base_octave_offset = (note - base) / 12;

    int degree = -1;
    for (int i = 0; i < 7; i++) {
        if (scale_intervals[i] == semitone_from_root) {
            degree = i;
            break;
        }
    }

    // Safety fallback
    if (degree == -1) {
        return note;
    }

    static const int degree_shifts[10] = {
        -7, -5, -4, -3, -2, 2, 3, 4, 5, 7
    };
    int shift = degree_shifts[ save_get(TRANSPOSE_INTERVAL) ];

    int new_degree = degree + shift;
    int octave_shift = 0;

    while (new_degree < 0) {
        new_degree += 7;
        octave_shift -= 1;
    }
    while (new_degree >= 7) {
        new_degree -= 7;
        octave_shift += 1;
    }

    int new_note = base
                 + scale_intervals[new_degree]
                 + (base_octave_offset + octave_shift) * 12;

    if (new_note < 0)   new_note = 0;
    if (new_note > 127) new_note = 127;

    return (uint8_t)new_note;
}

static void midi_pitch_shift(midi_note *midi_msg) {
    uint8_t is_note_on = 0;

    if (midi_is_note_message(midi_msg, &is_note_on)) {
        int16_t note = midi_msg->note;

        if (save_get(TRANSPOSE_TRANSPOSE_TYPE) == MIDI_TRANSPOSE_SCALED) {
            note = midi_transpose_notes((uint8_t)note);
        }
        else if (save_get(TRANSPOSE_TRANSPOSE_TYPE) == MIDI_TRANSPOSE_SHIFT) {
            note += (int16_t)(int32_t)save_get(TRANSPOSE_MIDI_SHIFT_VALUE);
        }

        if (note < 0)   note = 0;
        if (note > 127) note = 127;

        midi_msg->note = (uint8_t)note;
    }
}

// ---------------------
// Transpose pipeline
// ---------------------
void pipeline_midi_transpose(midi_note *midi_msg)
{
    const uint8_t length = midi_msg_len(midi_msg->status);

    if (save_get(TRANSPOSE_CURRENTLY_SENDING) == 0) {
        pipeline_arp(midi_msg, length);
        return;
    }

    if (save_get(TRANSPOSE_SEND_ORIGINAL) == 1) {
        midi_note pre_shift_msg = *midi_msg;

        if (save_get(TRANSPOSE_TRANSPOSE_TYPE) == MIDI_TRANSPOSE_SCALED) {
            uint8_t mode = save_get(TRANSPOSE_TRANSPOSE_SCALE) % AMOUNT_OF_MODES;
            uint8_t scale_intervals[7];
            get_mode_scale(mode, scale_intervals);
            pre_shift_msg.note = snap_note_to_scale(pre_shift_msg.note,
                                                   scale_intervals,
                                                   save_get(TRANSPOSE_BASE_NOTE));
        }

        pipeline_arp(&pre_shift_msg, length);

        midi_note shifted_msg = pre_shift_msg;
        midi_pitch_shift(&shifted_msg);
        pipeline_arp(&shifted_msg, length);
        return;
    }

    midi_pitch_shift(midi_msg);
    pipeline_arp(midi_msg, length);
}

// ---------------------
// Pipeline Arp
// --------------------
void pipeline_arp(midi_note *midi_msg, uint8_t length)
{
    if (save_get(ARPEGGIATOR_CURRENTLY_SENDING) == 1) {
        uint8_t is_note_on = 0;
        if (midi_is_note_message(midi_msg, &is_note_on)) {
            arp_handle_midi_note(midi_msg);
            return;
        }
    }

    pipeline_final(midi_msg, length);
}



// ---------------------
// Final dispatch
// ---------------------
void pipeline_final(midi_note *midi_msg, uint8_t length)
{
    send_midi_out(midi_msg, length);
    send_usb_midi_out(midi_msg, length);
}

void send_midi_out(midi_note *midi_message_raw, uint8_t length)
{
    if (midi_message_raw->status < 0x80) return;

    length = midi_msg_len(midi_message_raw->status); // enforce correctness

    uint8_t midi_bytes[3] = {0};
    midi_bytes[0] = midi_message_raw->status;
    if (length > 1) midi_bytes[1] = midi_message_raw->note;
    if (length > 2) midi_bytes[2] = midi_message_raw->velocity;

    // Determine if message is a note message for split logic
    uint8_t is_note_on = 0;
    const uint8_t is_note = midi_is_note_message(midi_message_raw, &is_note_on);
    const uint8_t note = (uint8_t)midi_bytes[1];

    const save_field_t send_to_out_field =
        (save_get(SPLIT_CURRENTLY_SENDING) == 1) ? SPLIT_SEND_TO_MIDI_OUT
                                                  : MODIFY_SEND_TO_MIDI_OUT;
    switch (save_get(send_to_out_field)) {
        case MIDI_OUT_1:
            HAL_UART_Transmit(&huart1, midi_bytes, length, 1000);
            break;

        case MIDI_OUT_2:
            HAL_UART_Transmit(&huart2, midi_bytes, length, 1000);
            break;

        case MIDI_OUT_1_2:
            HAL_UART_Transmit(&huart1, midi_bytes, length, 1000);
            HAL_UART_Transmit(&huart2, midi_bytes, length, 1000);
            break;

        case MIDI_OUT_SPLIT:
                   if (save_get(SPLIT_CURRENTLY_SENDING) == 1 && is_note) {
                       if (note < save_get(SPLIT_NOTE)) {
                           HAL_UART_Transmit(&huart1, midi_bytes, length, 1000);
                       } else {
                           HAL_UART_Transmit(&huart2, midi_bytes, length, 1000);
                       }
                   } else {
                       // Non-note or not in split mode: mirror to both like your fallback did
                       HAL_UART_Transmit(&huart1, midi_bytes, length, 1000);
                       HAL_UART_Transmit(&huart2, midi_bytes, length, 1000);
                   }
                   break;

               default:
                   break;
           }
       }


void send_usb_midi_out(midi_note *msg, uint8_t length)
{
    if (save_get(SETTINGS_SEND_USB) == USB_MIDI_OFF) {
        return;
    }

    length = midi_msg_len(msg->status);

    uint8_t bytes[3] = {0};
    bytes[0] = msg->status;
    if (length > 1) bytes[1] = msg->note;
    if (length > 2) bytes[2] = msg->velocity;

    send_usb_midi_message(bytes, length);
}
