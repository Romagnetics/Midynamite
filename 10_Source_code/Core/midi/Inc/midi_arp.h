/*
 * midi_arp.h
 *
 *  Created on: Feb 21, 2026
 *      Author: Romain Dereu
 */


#ifndef MIDI_INC_MIDI_ARP_H_
#define MIDI_INC_MIDI_ARP_H_

#include <stdint.h>
#include "midi_transform.h"

void arp_state_reset(void);
void arp_sync_hold_mode(void);
void arp_handle_midi_note(const midi_note *msg);
uint8_t arp_handle_midi_cc64(const midi_note *msg);


void arp_process_pending_tempo_ticks(void);

void arp_on_tempo_tick(void);
uint8_t arp_get_pressed_keys(uint8_t *out_notes, uint8_t max_notes);
uint8_t arp_get_pressed_key_count(void);



#endif /* MIDI_INC_MIDI_ARP_H_ */
