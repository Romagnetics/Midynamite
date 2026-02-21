/*
 * midi_tempo.h
 *
 *  Created on: Sep 5, 2025
 *      Author: Romain Dereu
 */

#ifndef INC_MIDI_TEMPO_H_
#define INC_MIDI_TEMO_H_

#include "stm32f4xx_hal.h"

void send_midi_tempo_out(int32_t tempo_click_rate, uint8_t send_to_midi_out);

void mt_start_stop(TIM_HandleTypeDef * timer);

#endif /* INC_MIDI_TEMPO_H_ */
