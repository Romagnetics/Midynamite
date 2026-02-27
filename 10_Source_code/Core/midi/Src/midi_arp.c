/*
 * midi_arp.c
 *
 *  Created on: Feb 21, 2026
 *      Author: Romain Dereu
 */

#include <string.h>

#include "memory_main.h"
#include "midi_arp.h"

static uint8_t s_physical_notes[128];
static uint8_t s_active_notes[128];

static midi_note last_note_sent = {0x90u, 60u, 10u};

static uint8_t s_note_on_playing = 0u;
static uint8_t s_step_index = 0u;
static uint16_t s_tick_counter = 0u;

static uint8_t should_process_arp_step(uint16_t clocks_per_step_value, uint8_t has_pressed_notes)
{
    if ((save_get(TEMPO_CURRENTLY_SENDING) == 0) &&
        (s_note_on_playing == 0u) &&
        (has_pressed_notes != 0u)) {
        s_tick_counter = 0u;
        return 1u;
    }

    s_tick_counter++;
    if (s_tick_counter < clocks_per_step_value) {
        return 0u;
    }

    s_tick_counter = 0u;
    return 1u;
}




static uint16_t clocks_per_step(uint8_t div)
{
    // 48 PPQ grid. Divisions: 1/4, 1/6, 1/8, 1/12, 1/16, 1/24, 1/32
    static const uint16_t map[7] = {48u, 32u, 24u, 16u, 12u, 8u, 6u};
    return map[div % 7u];
}

static uint8_t physical_note_count(void)
{
    uint8_t count = 0;

    for (uint8_t note = 0; note < 128u; ++note) {
        count += s_physical_notes[note] ? 1u : 0u;
    }

    return count;
}

void arp_state_reset(void)
{
    memset(s_physical_notes, 0, sizeof(s_physical_notes));
    memset(s_active_notes, 0, sizeof(s_active_notes));
    last_note_sent.status = 0x90u;
    last_note_sent.note = 60u;
    last_note_sent.velocity = 100u;
    s_note_on_playing = 0u;
    s_step_index = 0u;
    s_tick_counter = 0u;
}

void arp_sync_hold_mode(void)
{
    if (save_get(ARPEGGIATOR_HOLD) == 0) {
        memcpy(s_active_notes, s_physical_notes, sizeof(s_active_notes));
    }
}

void arp_handle_midi_note(const midi_note *msg)
{
    uint8_t is_note_on = 0;

    if (!midi_is_note_message(msg, &is_note_on)) {
        return;
    }

    arp_sync_hold_mode();

    const uint8_t note = (uint8_t)(msg->note & 0x7Fu);
    const uint8_t hold_enabled = save_get(ARPEGGIATOR_HOLD);

    // Keep only channel from last played input note; arp will set 0x90/0x80 itself.
    last_note_sent.status = (uint8_t)(0x90u | (msg->status & 0x0Fu));
    last_note_sent.velocity = msg->velocity;

    if (is_note_on) {
        if ((hold_enabled != 0u) && (physical_note_count() == 0u)) {
            memset(s_active_notes, 0, sizeof(s_active_notes));
            s_step_index = 0u;
        }

        s_physical_notes[note] = 1u;
        s_active_notes[note] = 1u;
        return;
    }

    s_physical_notes[note] = 0u;

    if (hold_enabled == 0u) {
        s_active_notes[note] = 0u;
    }
}

void arp_on_tempo_tick(void)
{
    if (save_get(ARPEGGIATOR_CURRENTLY_SENDING) == 0) {
        return;
    }

    uint8_t notes[128];
    uint8_t count = arp_get_pressed_keys(notes, 128u);


    const uint16_t cps = clocks_per_step((uint8_t)save_get(ARPEGGIATOR_DIVISION));
    if (!should_process_arp_step(cps, count)) {
        return;
    }

    if (s_note_on_playing) {
    midi_note off = last_note_sent;
    off.status   = (uint8_t)(0x80u | (last_note_sent.status & 0x0Fu));
    off.velocity = 0u;
    pipeline_final(&off, 3);
    s_note_on_playing = 0u;
}

    if (count == 0u) {
        s_step_index = 0u;
        return;
    }

    if (s_step_index >= count) {
        s_step_index = 0u;
    }

    last_note_sent.note = notes[s_step_index];
    s_step_index = (uint8_t)((s_step_index + 1u) % count);

    midi_note on = last_note_sent;
    on.status = (uint8_t)(0x90u | (last_note_sent.status & 0x0Fu));
    // on.note already set
    // on.velocity already set (default 100 or captured from input if you enabled that)
    pipeline_final(&on, 3);

    last_note_sent = on;
    s_note_on_playing = 1u;
}

uint8_t arp_get_pressed_keys(uint8_t *out_notes, uint8_t max_notes)
{
    uint8_t count = 0;

    for (uint8_t note = 0; note < 128u; ++note) {
        if (s_active_notes[note] == 0u) {
            continue;
        }

        if ((out_notes != NULL) && (count < max_notes)) {
            out_notes[count] = note;
        }

        ++count;
    }

    return count;
}

uint8_t arp_get_pressed_key_count(void)
{
    return arp_get_pressed_keys(NULL, 0);
}
