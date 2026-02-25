/*
 * midi_transform.c
 *
 *  Created on: Sep 5, 2025
 *      Author: Astaa
 */
#include "midi_tempo.h"
#include "utils.h" //For enums
#include "midi_usb.h"

static uint8_t *mt_send_to_midi_out_cache(void)
{
    static uint8_t sending_to_midi_out = 0;
    return &sending_to_midi_out;
}

void set_tempo_bpm(uint32_t bpm)
{
    const uint32_t tempo_click_rate = (bpm > 0u) ? (6000000u / (bpm * 48u)) : 0u;
    TIM2->ARR = tempo_click_rate;
}

void mt_set_send_to_midi_out(uint8_t send_to_midi_out)
{
    *mt_send_to_midi_out_cache() = send_to_midi_out;
}

void tempo_sync_from_save(void)
{
    set_tempo_bpm(save_get(TEMPO_CURRENT_TEMPO));
    mt_set_send_to_midi_out((uint8_t)save_get(TEMPO_SEND_TO_MIDI_OUT));
}

void send_midi_tempo_out(void){

	if (save_get(TEMPO_CURRENTLY_SENDING) == 0) {
			return;
		}

    static uint8_t div2 = 0;
    div2 ^= 1u;
    if (div2) {
        return;
    }

	static uint8_t clock_tick = 0xF8;

	UART_HandleTypeDef *UART_list_tempo[2];
	list_of_UART_to_send_to(*mt_send_to_midi_out_cache(), UART_list_tempo);

    send_usb_midi_message(&clock_tick, 1);
    for (int i = 0; i < 2; i++) {
        if (UART_list_tempo[i] != NULL) {
            HAL_UART_Transmit(UART_list_tempo[i], &clock_tick, 1, 1000);
        	}
    	}
    }

void mt_start_stop(TIM_HandleTypeDef *timer) {
	static uint8_t clock_start = 0xFA;
	static uint8_t clock_stop  = 0xfC;

	static UART_HandleTypeDef *UART_list_tempo[2];
	list_of_UART_to_send_to(*mt_send_to_midi_out_cache(), UART_list_tempo);

	uint8_t clock_sending = save_get(TEMPO_CURRENTLY_SENDING);

    // Stop clock
    if (clock_sending == 0) {
        for (int i = 0; i < 2; i++) {
            if (UART_list_tempo[i] != NULL) {
                HAL_UART_Transmit(UART_list_tempo[i], &clock_stop, 1, 1000);
            }
        }
        send_usb_midi_message(&clock_stop, 1);
    }
    // Start clock
    else if (clock_sending == 1) {
        for (int i = 0; i < 2; i++) {
            if (UART_list_tempo[i] != NULL) {
                HAL_UART_Transmit(UART_list_tempo[i], &clock_start, 1, 1000);
            }
        }
        send_usb_midi_message(&clock_start, 1);

    }
}
