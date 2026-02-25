/*
 * midi_transform.c
 *
 *  Created on: Sep 5, 2025
 *      Author: Astaa
 */
#include "midi_tempo.h"
#include "utils.h" //For enums
#include "midi_usb.h"

void send_midi_tempo_out(int32_t tempo_click_rate, uint8_t send_to_midi_out){
	TIM2->ARR = tempo_click_rate;

	if (save_get(TEMPO_CURRENTLY_SENDING) == 0) {
			return;
		}

    static uint8_t div2 = 0;
    div2 ^= 1u;
    if (div2) {
        return;
    }


	uint8_t clock_tick = 0xF8;

	UART_HandleTypeDef *UART_list_tempo[2];
	list_of_UART_to_send_to(send_to_midi_out, UART_list_tempo);


    send_usb_midi_message(&clock_tick, 1);
    for (int i = 0; i < 2; i++) {
        if (UART_list_tempo[i] != NULL) {
            HAL_UART_Transmit(UART_list_tempo[i], &clock_tick, 1, 1000);
        	}
    	}
    }

void mt_start_stop(TIM_HandleTypeDef *timer) {
	uint8_t clock_start = 0xFA;
	uint8_t clock_stop  = 0xfC;

	static UART_HandleTypeDef *UART_list_tempo[2];
	uint8_t send_out_to = save_get(TEMPO_SEND_TO_MIDI_OUT);
	list_of_UART_to_send_to(send_out_to, UART_list_tempo);

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
