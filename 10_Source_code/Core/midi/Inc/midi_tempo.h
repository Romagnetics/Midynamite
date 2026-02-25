/*
 * midi_tempo.h
 *
 *  Created on: Sep 5, 2025
 *      Author: Romain Dereu
 */

#ifndef INC_MIDI_TEMPO_H_
#define INC_MIDI_TEMPO_H_

#include "stm32f4xx_hal.h"

void set_tempo_bpm(uint32_t bpm);
void mt_set_send_to_midi_out(uint8_t send_to_midi_out);
void tempo_sync_from_save(void);

void send_midi_tempo_out(void);


void mt_start_stop(void);

#endif /* INC_MIDI_TEMPO_H_ */
