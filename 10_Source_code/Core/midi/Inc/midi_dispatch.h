/*
 * midi_dispatch.h
 *
 *  Created on: Mar 4, 2026
 *      Author: Romain Dereu
 */

#ifndef MIDI_INC_MIDI_DISPATCH_H_
#define MIDI_INC_MIDI_DISPATCH_H_

#include <stdint.h>
#include "midi_transform.h"

uint8_t midi_dispatch_process(midi_note *midi_msg, uint8_t length);

#ifdef UNIT_TEST
void midi_dispatch_reset_state(void);
#endif

#endif /* MIDI_INC_MIDI_DISPATCH_H_ */
