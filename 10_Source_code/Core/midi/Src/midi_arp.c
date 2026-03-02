/*
 * midi_arp.c
 *
 *  Created on: Feb 21, 2026
 *      Author: Romain Dereu
 */

#include <string.h>

#include "memory_main.h"
#include "midi_arp.h"

static midi_note s_physical_notes[128];
static midi_note s_active_notes[128];
static uint16_t s_note_order[128];
static uint16_t s_note_order_counter = 0;

static midi_note last_note_sent = {0x90, 60, 10};

static uint8_t s_note_on_playing = 0;
static uint8_t s_has_played_note = 0;

static uint16_t s_tick_counter = 0;
static uint16_t s_gate_tick_counter = 0;

static uint8_t s_sustain_hold_active = 0;
static volatile uint16_t s_pending_tempo_ticks = 0;

static uint8_t s_was_arp_enabled = 0;
static uint8_t s_swing_phase = 0;
static uint8_t s_pattern_step = 0;

static uint8_t s_source_notes[128];
static uint8_t s_expanded_notes[128];
static uint8_t s_source_for_expanded_note[128];


typedef enum {
    ARP_PATTERN_UP = 0,
    ARP_PATTERN_ORDER = 7
} arp_pattern_t;

static uint8_t arp_next_step_index(const uint8_t *notes, uint8_t count, uint8_t keep_pitch_direction)
{
    if ((count == 0) || (notes == NULL)) {
        return 0;
    }

    if (s_has_played_note == 0) {
        return 0;
    }

    for (uint8_t i = 0; i < count; ++i) {
        if (notes[i] == last_note_sent.note) {
            return (uint8_t)((i + 1) % count);
        }
    }

    if (keep_pitch_direction != 0) {
        for (uint8_t i = 0; i < count; ++i) {
            if (notes[i] > last_note_sent.note) {
                return i;
            }
        }
    }

    return 0;
}


static uint8_t should_process_arp_step(uint16_t step_ticks_value, uint8_t has_pressed_notes)
{
    if ((save_get(TEMPO_CURRENTLY_SENDING) == 0) &&
        (s_has_played_note == 0) &&
        (has_pressed_notes != 0)) {
        s_tick_counter = 0;
        return 1;
    }

    s_tick_counter++;
    if (s_tick_counter < step_ticks_value) {
        return 0;
    }

    s_tick_counter = 0;
    return 1;
}


static void arp_send_note_off(void)
{
    midi_note off = last_note_sent;
    off.status = (uint8_t)(0x80 | (last_note_sent.status & 0x0F));
    off.velocity = 0;
    pipeline_final(&off, 3);
    s_note_on_playing = 0;
    s_gate_tick_counter = 0;
}

static void arp_process_gate(uint16_t gate_ticks)
{
    if (s_note_on_playing == 0) {
        return;
    }

    s_gate_tick_counter++;
    if (s_gate_tick_counter < gate_ticks) {
        return;
    }

    arp_send_note_off();
}



static uint16_t clocks_per_step(uint8_t div)
{
    // 48 PPQ grid. Divisions: 1/4, 1/6, 1/8, 1/12, 1/16, 1/24, 1/32
    static const uint16_t map[7] = {48, 32, 24, 16, 12, 8, 6};
    return map[div % 7];
}


static uint16_t arp_current_step_ticks(uint16_t clocks_per_step_value)
{
    uint16_t swing = (uint16_t)(uint8_t)save_get(ARPEGGIATOR_SWING);

    if (swing >= clocks_per_step_value) {
        swing = (uint16_t)((clocks_per_step_value * swing + 50) / 100);
    }

    if (swing < 1) {
        swing = 1;
    }
    if (swing >= clocks_per_step_value) {
        swing = (uint16_t)(clocks_per_step_value - 1);
    }

    if (s_swing_phase == 0) {
        return (uint16_t)(2 * swing);
    }

    return (uint16_t)(2 * (clocks_per_step_value - swing));
}


static uint8_t arp_step_is_enabled(void)
{
    uint8_t length = (uint8_t)save_get(ARPEGGIATOR_LENGTH);
    if (length < 1) {
        length = 1;
    }
    if (length > 8) {
        length = 8;
    }

    if (s_pattern_step >= length) {
        s_pattern_step = 0;
    }

    const uint8_t step = s_pattern_step;
    const uint32_t notes_mask = (uint32_t)save_get(ARPEGGIATOR_NOTES);
    const uint8_t enabled = (uint8_t)((notes_mask >> step) & 1);

    s_pattern_step = (uint8_t)((s_pattern_step + 1) % length);
    return enabled;
}





static uint8_t physical_note_count(void)
{
    uint8_t count = 0;

    for (uint8_t note = 0; note < 128; ++note) {
        count += (s_physical_notes[note].status != 0) ? 1 : 0;
    }

    return count;
}

static uint8_t arp_hold_is_active(void)
{
    return (save_get(ARPEGGIATOR_HOLD) != 0 || s_sustain_hold_active != 0) ? 1 : 0;
}

static uint8_t arp_get_ordered_keys(uint8_t *out_notes, uint8_t max_notes)
{
    uint8_t count = 0;
    for (uint16_t order = 1; order <= s_note_order_counter; ++order) {
        for (uint8_t note = 0; note < 128; ++note) {
            if ((s_active_notes[note].status == 0) || (s_note_order[note] != order)) {
                continue;
            }

            if ((out_notes != NULL) && (count < max_notes)) {
                out_notes[count] = note;
            }
            ++count;
        }
    }

    return count;
}

static uint8_t arp_expand_notes_across_octaves(uint8_t source_count)
{
    if (source_count == 0) {
        return 0;
    }

    uint8_t octaves = (uint8_t)save_get(ARPEGGIATOR_OCTAVES);
    if (octaves < 1) {
        octaves = 1;
    }
    if (octaves > 4) {
        octaves = 4;
    }

    uint8_t expanded_count = 0;
    for (uint8_t octave = 0; octave < octaves; ++octave) {
        const uint8_t transpose = (uint8_t)(12 * octave);
        for (uint8_t i = 0; i < source_count; ++i) {
            const uint16_t candidate = (uint16_t)s_source_notes[i] + transpose;
            if ((candidate > 127) || (expanded_count >= 128)) {
                continue;
            }

            s_expanded_notes[expanded_count] = (uint8_t)candidate;
            s_source_for_expanded_note[expanded_count] = s_source_notes[i];
            ++expanded_count;
        }
    }

    return expanded_count;
}



static void arp_apply_key_sync_on_note_on(uint8_t note)
{
    if (save_get(ARPEGGIATOR_KEY_SYNC) == 0) {
        return;
    }

    if (s_physical_notes[note].status != 0) {
        return;
    }

    s_has_played_note = 0;
    s_tick_counter = 0;
    s_swing_phase = 0;
}




static void arp_sync_note_order_with_active(void)
{
    for (uint8_t note = 0; note < 128; ++note) {
        if (s_active_notes[note].status != 0) {
            continue;
        }
        s_note_order[note] = 0;
    }
}

//Used for tests
void arp_state_reset(void)
{
    memset(s_physical_notes, 0, sizeof(s_physical_notes));
    memset(s_active_notes, 0, sizeof(s_active_notes));
    memset(s_note_order, 0, sizeof(s_note_order));
    s_note_order_counter = 0;
    last_note_sent.status = 0x90;
    last_note_sent.note = 60;
    last_note_sent.velocity = 100;
    s_note_on_playing = 0;
    s_has_played_note = 0;
    s_tick_counter = 0;
    s_gate_tick_counter = 0;
    s_sustain_hold_active = 0;
    s_pending_tempo_ticks = 0;
    s_was_arp_enabled = 0;
    s_swing_phase = 0;
    s_pattern_step = 0;
}

static void arp_clear_tracked_notes(void)
{
    memset(s_physical_notes, 0, sizeof(s_physical_notes));
    memset(s_active_notes, 0, sizeof(s_active_notes));
    memset(s_note_order, 0, sizeof(s_note_order));
    s_note_order_counter = 0;
    s_has_played_note = 0;
    s_tick_counter = 0;
    s_pattern_step = 0;
}

static void arp_stop_playing_note(void)
{
    if (s_note_on_playing == 0) {
        return;
    }

    midi_note off = last_note_sent;
    off.status = (uint8_t)(0x80 | (last_note_sent.status & 0x0F));
    off.velocity = 0;
    pipeline_final(&off, 3);
    s_note_on_playing = 0;
}


void arp_sync_hold_mode(void)
{
    if ((arp_hold_is_active() == 0) ||
        (save_get(ARPEGGIATOR_CURRENTLY_SENDING) == 0)) {
        memcpy(s_active_notes, s_physical_notes, sizeof(s_active_notes));
        arp_sync_note_order_with_active();
    }
}

void arp_handle_midi_note(const midi_note *msg)
{
    uint8_t is_note_on = 0;

    if (!midi_is_note_message(msg, &is_note_on)) {
        return;
    }

    arp_sync_hold_mode();

    const uint8_t note = (uint8_t)(msg->note & 0x7F);
    const uint8_t hold_enabled = arp_hold_is_active();

    // Keep only channel from last played input note; arp will set 0x90/0x80 itself.
    last_note_sent.status = (uint8_t)(0x90 | (msg->status & 0x0F));
    last_note_sent.velocity = msg->velocity;

    if (is_note_on) {
        arp_apply_key_sync_on_note_on(note);

        if ((hold_enabled != 0) && (physical_note_count() == 0)) {
            memset(s_active_notes, 0, sizeof(s_active_notes));
            memset(s_note_order, 0, sizeof(s_note_order));
            s_note_order_counter = 0;
            s_has_played_note = 0;
        }

        s_physical_notes[note] = *msg;
        s_physical_notes[note].note = note;

        s_active_notes[note] = *msg;
        s_active_notes[note].note = note;
        s_note_order[note] = ++s_note_order_counter;

        return;
    }

    memset(&s_physical_notes[note], 0, sizeof(s_physical_notes[note]));

    if (hold_enabled == 0) {
        memset(&s_active_notes[note], 0, sizeof(s_active_notes[note]));
        s_note_order[note] = 0;
    }
}

void arp_handle_midi_cc64(const midi_note *msg)
{
    if (msg == NULL) {
        return;
    }

    const uint8_t status_nibble = (uint8_t)(msg->status & 0xF0);
    if ((status_nibble != 0xB0) || ((msg->note & 0x7F) != 64)) {
        return;
    }

    s_sustain_hold_active = (msg->velocity >= 64) ? 1 : 0;
    arp_sync_hold_mode();
}

void arp_request_tempo_tick_from_isr(void)
{
    uint16_t pending = __atomic_load_n(&s_pending_tempo_ticks, __ATOMIC_RELAXED);

    while (pending < 65535) {
        const uint16_t next = (uint16_t)(pending + 1);
        if (__atomic_compare_exchange_n(&s_pending_tempo_ticks,
                                        &pending,
                                        next,
                                        0,
                                        __ATOMIC_RELAXED,
                                        __ATOMIC_RELAXED)) {
            return;
        }
    }
}

void arp_process_pending_tempo_ticks(void)
{
    for (;;) {
        uint16_t pending = __atomic_exchange_n(&s_pending_tempo_ticks, 0, __ATOMIC_RELAXED);
        if (pending == 0) {
            return;
        }

        while (pending != 0) {
            pending--;
            arp_on_tempo_tick();
        }
    }
}


void arp_on_tempo_tick(void)
{
    arp_sync_hold_mode();

    const uint8_t arp_enabled = (save_get(ARPEGGIATOR_CURRENTLY_SENDING) != 0) ? 1 : 0;
    if ((arp_enabled == 0) && (s_was_arp_enabled != 0)) {
        arp_stop_playing_note();
        arp_clear_tracked_notes();
    }
    s_was_arp_enabled = arp_enabled;


    if (save_get(ARPEGGIATOR_CURRENTLY_SENDING) == 0) {
        return;
    }

    uint8_t source_count = 0;
    uint8_t count = 0;
    const uint8_t pattern = (uint8_t)save_get(ARPEGGIATOR_PATTERN);

    if (pattern == ARP_PATTERN_ORDER) {
        source_count = arp_get_ordered_keys(s_source_notes, 128);
    } else {
        source_count = arp_get_pressed_keys(s_source_notes, 128);
    }

    count = arp_expand_notes_across_octaves(source_count);


    const uint16_t cps = clocks_per_step((uint8_t)save_get(ARPEGGIATOR_DIVISION));
    const uint16_t gate_ticks = (uint16_t)(((cps * save_get(ARPEGGIATOR_GATE)) + 5) / 10);
    const uint16_t step_ticks = arp_current_step_ticks(cps);

    arp_process_gate(gate_ticks);



    if (!should_process_arp_step(step_ticks, count)) {
        return;
    }

    if (s_note_on_playing) {
    	arp_stop_playing_note();
    }

    if (count == 0) {
        s_has_played_note = 0;
        s_pattern_step = 0;
        s_swing_phase ^= 1;
        return;
    }

    if (arp_step_is_enabled() == 0) {
        s_swing_phase ^= 1;
        return;
    }

    const uint8_t index = arp_next_step_index(s_expanded_notes, count, (uint8_t)(pattern == ARP_PATTERN_UP));
        last_note_sent.note = s_expanded_notes[index];
        last_note_sent.velocity = s_active_notes[s_source_for_expanded_note[index]].velocity;



    midi_note on = last_note_sent;
    on.status = (uint8_t)(0x90 | (last_note_sent.status & 0x0F));
    // on.note already set
    // on.velocity already set (default 100 or captured from input if you enabled that)
    pipeline_final(&on, 3);

    last_note_sent = on;
    s_has_played_note = 1;
    s_note_on_playing = 1;
    s_gate_tick_counter = 0;
    s_swing_phase ^= 1;
}

uint8_t arp_get_pressed_keys(uint8_t *out_notes, uint8_t max_notes)
{
    uint8_t count = 0;

    for (uint8_t note = 0; note < 128; ++note) {
        if (s_active_notes[note].status == 0) {
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
