/*
 * midi_dispatch.c
 *
 *  Created on: Mar 4, 2026
 *      Author: Romain Dereu
 */


#include <stdlib.h>
#include <string.h>

#include "memory_main.h"
#include "midi_dispatch.h"

static inline uint8_t dispatch_msg_len(uint8_t status)
{
    if (status >= 0xF8) return 1;

    switch (status) {
        case 0xF1: return 2;
        case 0xF2: return 3;
        case 0xF3: return 2;
        case 0xF6: return 1;
        case 0xF7: return 1;
        default: break;
    }

    const uint8_t hi = (uint8_t)(status & 0xF0);
    return (hi == 0xC0 || hi == 0xD0) ? 2 : 3;
}

static inline uint8_t dispatch_is_channel_voice(uint8_t status)
{
    const uint8_t hi = (uint8_t)(status & 0xF0);
    return (hi >= 0x80 && hi <= 0xE0) ? 1 : 0;
}

static void dispatch_change_midi_channel(midi_note *midi_msg, uint8_t send_to_midi_channel)
{
    const uint8_t status = midi_msg->status;

    if (status >= 0x80 && status <= 0xEF) {
        const uint8_t status_nibble = (uint8_t)(status & 0xF0);
        midi_msg->status = (uint8_t)(status_nibble | ((send_to_midi_channel - 1) & 0x0F));
    }
}

typedef enum {
    DISPATCH_VOICE_FIRST = 0,
    DISPATCH_VOICE_LAST,
    DISPATCH_VOICE_VELOC,
    DISPATCH_VOICE_RANDOM
} dispatch_voice_manage_t;

#define DISPATCH_MAX_SYNTHS 16
#define DISPATCH_MAX_NOTES_PER_SYNTH 6
#define DISPATCH_MAX_VOICES (DISPATCH_MAX_SYNTHS * DISPATCH_MAX_NOTES_PER_SYNTH)

typedef struct {
    uint8_t active;
    uint8_t note;
    uint8_t velocity;
    uint8_t synth_idx;
    uint32_t age;
} dispatch_voice_t;

typedef struct {
    dispatch_voice_t voices[DISPATCH_MAX_VOICES];
    uint8_t active_per_synth[DISPATCH_MAX_SYNTHS];
    uint8_t amount_of_synths;
    uint8_t from_channel;
    uint8_t notes_per_synth;
    uint8_t voice_manage;
    uint8_t rr_next_synth;
    uint8_t initialized;
    uint32_t age_counter;
} dispatch_state_t;

static dispatch_state_t g_dispatch_state;

extern void send_midi_out(midi_note *midi_message_raw, uint8_t length);
extern void send_usb_midi_out(midi_note *midi_message_raw, uint8_t length);
extern uint8_t midi_is_note_message(const midi_note *msg, uint8_t *is_note_on);

static uint8_t dispatch_synth_channel(const dispatch_state_t *state, uint8_t synth_idx)
{
    const uint8_t first_channel = (state->from_channel > 0) ? state->from_channel : 1;
    return (uint8_t)(first_channel + synth_idx);
}

static void dispatch_send_on_synth(const midi_note *src_msg, uint8_t synth_channel)
{
    midi_note out = *src_msg;
    dispatch_change_midi_channel(&out, synth_channel);
    send_midi_out(&out, dispatch_msg_len(out.status));
    send_usb_midi_out(&out, dispatch_msg_len(out.status));
}

static void dispatch_send_note_off_on_synth(uint8_t note, uint8_t synth_channel)
{
    midi_note off_msg = {
        .status = 0x80,
        .note = note,
        .velocity = 0
    };
    dispatch_change_midi_channel(&off_msg, synth_channel);
    send_midi_out(&off_msg, dispatch_msg_len(off_msg.status));
    send_usb_midi_out(&off_msg, dispatch_msg_len(off_msg.status));
}

static void dispatch_release_all_tracked_notes(dispatch_state_t *state)
{
    for (uint8_t i = 0; i < DISPATCH_MAX_VOICES; ++i) {
        dispatch_voice_t *voice = &state->voices[i];
        if (voice->active == 0) {
            continue;
        }

        dispatch_send_note_off_on_synth(voice->note, dispatch_synth_channel(state, voice->synth_idx));
        voice->active = 0;
    }

    memset(state->active_per_synth, 0, sizeof(state->active_per_synth));
    state->rr_next_synth = 0;
}

static void dispatch_init_or_reconfigure(dispatch_state_t *state)
{
    const uint8_t amount_of_synths = (uint8_t)save_get(DISPATCH_AMOUNT_OF_SYNTHS);
    const uint8_t from_channel = (uint8_t)save_get(DISPATCH_FROM_CHANNEL);
    const uint8_t notes_per_synth = (uint8_t)save_get(DISPATCH_NOTES_PER_SYNTH);
    const uint8_t voice_manage = (uint8_t)save_get(DISPATCH_VOICE_MANAGE);

    const uint8_t capped_synths = (amount_of_synths == 0) ? 1 : ((amount_of_synths > DISPATCH_MAX_SYNTHS) ? DISPATCH_MAX_SYNTHS : amount_of_synths);
    const uint8_t capped_notes = (notes_per_synth == 0) ? 1 : ((notes_per_synth > DISPATCH_MAX_NOTES_PER_SYNTH) ? DISPATCH_MAX_NOTES_PER_SYNTH : notes_per_synth);

    const uint8_t needs_reconfig = (state->initialized == 0)
        || (state->amount_of_synths != capped_synths)
        || (state->from_channel != from_channel)
        || (state->notes_per_synth != capped_notes)
        || (state->voice_manage != voice_manage);

    if (needs_reconfig != 0) {
        if (state->initialized != 0) {
            dispatch_release_all_tracked_notes(state);
        }
        memset(state, 0, sizeof(*state));
        state->amount_of_synths = capped_synths;
        state->from_channel = from_channel;
        state->notes_per_synth = capped_notes;
        state->voice_manage = voice_manage;
        state->initialized = 1;
    }
}

static int16_t dispatch_pick_voice_to_steal(dispatch_state_t *state)
{
    uint8_t active_count = 0;
    uint8_t active_indexes[DISPATCH_MAX_VOICES] = {0};

    for (uint8_t i = 0; i < DISPATCH_MAX_VOICES; ++i) {
        if (state->voices[i].active != 0) {
            active_indexes[active_count++] = i;
        }
    }

    if (active_count == 0) {
        return -1;
    }

    if (state->voice_manage == DISPATCH_VOICE_RANDOM) {
        return active_indexes[rand() % active_count];
    }

    uint8_t best_idx = active_indexes[0];

    for (uint8_t i = 1; i < active_count; ++i) {
        const uint8_t cand_idx = active_indexes[i];
        const dispatch_voice_t *best = &state->voices[best_idx];
        const dispatch_voice_t *cand = &state->voices[cand_idx];

        if (state->voice_manage == DISPATCH_VOICE_LAST) {
            if (cand->age > best->age) {
                best_idx = cand_idx;
            }
            continue;
        }

        if (state->voice_manage == DISPATCH_VOICE_VELOC) {
            if (cand->velocity < best->velocity) {
                best_idx = cand_idx;
                continue;
            }
            if ((cand->velocity == best->velocity) && (cand->age < best->age)) {
                best_idx = cand_idx;
            }
            continue;
        }

        if (cand->age < best->age) {
            best_idx = cand_idx;
        }
    }

    return best_idx;
}

static void dispatch_drop_voice(dispatch_state_t *state, uint8_t voice_idx)
{
    dispatch_voice_t *voice = &state->voices[voice_idx];
    if (voice->active == 0) {
        return;
    }

    dispatch_send_note_off_on_synth(voice->note, dispatch_synth_channel(state, voice->synth_idx));
    if (state->active_per_synth[voice->synth_idx] > 0) {
        state->active_per_synth[voice->synth_idx]--;
    }
    voice->active = 0;
}

static int16_t dispatch_find_free_voice_slot(dispatch_state_t *state)
{
    for (uint8_t i = 0; i < DISPATCH_MAX_VOICES; ++i) {
        if (state->voices[i].active == 0) {
            return i;
        }
    }

    return -1;
}

static int16_t dispatch_select_synth_round_robin(dispatch_state_t *state)
{
    for (uint8_t i = 0; i < state->amount_of_synths; ++i) {
        const uint8_t synth_idx = (uint8_t)((state->rr_next_synth + i) % state->amount_of_synths);
        if (state->active_per_synth[synth_idx] < state->notes_per_synth) {
            state->rr_next_synth = (uint8_t)((synth_idx + 1) % state->amount_of_synths);
            return synth_idx;
        }
    }

    return -1;
}

static int16_t dispatch_find_note_on_voice(const dispatch_state_t *state, uint8_t note)
{
    int16_t found_idx = -1;
    uint32_t latest_age = 0;

    for (uint8_t i = 0; i < DISPATCH_MAX_VOICES; ++i) {
        const dispatch_voice_t *voice = &state->voices[i];
        if ((voice->active == 0) || (voice->note != note)) {
            continue;
        }

        if ((found_idx < 0) || (voice->age > latest_age)) {
            found_idx = i;
            latest_age = voice->age;
        }
    }

    return found_idx;
}

uint8_t midi_dispatch_process(midi_note *midi_msg, uint8_t length)
{
    const uint8_t dispatch_enabled = (uint8_t)(save_get(DISPATCH_CURRENTLY_SENDING) == 1);
    if (dispatch_enabled == 0) {
        return 0;
    }

    (void)length;
    dispatch_init_or_reconfigure(&g_dispatch_state);

    const uint8_t status_nibble = (uint8_t)(midi_msg->status & 0xF0);
    const uint8_t is_cc64 = ((status_nibble == 0xB0) && ((midi_msg->note & 0x7F) == 64)) ? 1 : 0;
    uint8_t is_note_on = 0;
    const uint8_t is_note = midi_is_note_message(midi_msg, &is_note_on);

    if (is_note != 0) {
        if (is_note_on != 0) {
            int16_t target_synth = dispatch_select_synth_round_robin(&g_dispatch_state);

            if (target_synth < 0) {
                const int16_t voice_to_drop = dispatch_pick_voice_to_steal(&g_dispatch_state);
                if (voice_to_drop >= 0) {
                    dispatch_drop_voice(&g_dispatch_state, (uint8_t)voice_to_drop);
                }
                target_synth = dispatch_select_synth_round_robin(&g_dispatch_state);
            }

            if (target_synth >= 0) {
                const int16_t voice_slot = dispatch_find_free_voice_slot(&g_dispatch_state);
                if (voice_slot >= 0) {
                    dispatch_voice_t *voice = &g_dispatch_state.voices[voice_slot];
                    voice->active = 1;
                    voice->note = midi_msg->note;
                    voice->velocity = midi_msg->velocity;
                    voice->synth_idx = (uint8_t)target_synth;
                    voice->age = ++g_dispatch_state.age_counter;
                    g_dispatch_state.active_per_synth[target_synth]++;

                    dispatch_send_on_synth(midi_msg, dispatch_synth_channel(&g_dispatch_state, (uint8_t)target_synth));
                }
            }
        } else {
            const int16_t found_voice = dispatch_find_note_on_voice(&g_dispatch_state, midi_msg->note);
            if (found_voice >= 0) {
                const dispatch_voice_t *voice = &g_dispatch_state.voices[found_voice];
                dispatch_send_on_synth(midi_msg, dispatch_synth_channel(&g_dispatch_state, voice->synth_idx));

                if (g_dispatch_state.active_per_synth[voice->synth_idx] > 0) {
                    g_dispatch_state.active_per_synth[voice->synth_idx]--;
                }
                g_dispatch_state.voices[found_voice].active = 0;
            }
        }

        return 1;
    }

    if (is_cc64 != 0) {
        for (uint8_t i = 0; i < g_dispatch_state.amount_of_synths; ++i) {
            dispatch_send_on_synth(midi_msg, dispatch_synth_channel(&g_dispatch_state, i));
        }
        return 1;
    }

    if (dispatch_is_channel_voice(midi_msg->status) != 0) {
        for (uint8_t i = 0; i < g_dispatch_state.amount_of_synths; ++i) {
            dispatch_send_on_synth(midi_msg, dispatch_synth_channel(&g_dispatch_state, i));
        }
        return 1;
    }

    return 0;
}

#ifdef UNIT_TEST
void midi_dispatch_reset_state(void)
{
    memset(&g_dispatch_state, 0, sizeof(g_dispatch_state));
}
#endif

