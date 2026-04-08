/*
 * midi_transform.c
 *
 *  Created on: Sep 5, 2025
 *      Author: Romain Dereu
 */
#include "midi_tempo.h"
#include "utils.h" //For enums
#include "midi_usb.h"
#include "threads.h"

#define MT_UART_TX_TIMEOUT_MS 1

static uint8_t *mt_send_to_midi_out_cache(void)
{
    static uint8_t sending_to_midi_out = 0;
    return &sending_to_midi_out;
}

static uint8_t *mt_clock_divider_cache(void)
{
    static uint8_t divider_phase = 0;
    return &divider_phase;
}

static inline void mt_reset_clock_divider(void)
{
    *mt_clock_divider_cache() = 0;
}

static inline uint8_t mt_usb_tempo_out_enabled(void)
{
    const uint8_t usb_mode = (uint8_t)save_get(SETTINGS_SEND_USB);
    return (uint8_t)((usb_mode == MIDI_USB_OUT) || (usb_mode == MIDI_USB_THRU_OUT));
}

void set_tempo_bpm(uint32_t bpm)
{
    const uint32_t tempo_click_rate = (bpm > 0) ? (6000000 / (bpm * 48)) : 0;
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
    mt_reset_clock_divider();
}


void mt_process_pending_tempo_out(void)
{
    if (save_get(TEMPO_CURRENTLY_SENDING) == 0) {
        mt_reset_clock_divider();
        return;
    }

    uint8_t *divider_phase = mt_clock_divider_cache();
    *divider_phase ^= 1;
    if (*divider_phase != 0) {
        return;
    }

    static uint8_t clock_tick = 0xF8;

    UART_HandleTypeDef *UART_list_tempo[2];
    list_of_UART_to_send_to(*mt_send_to_midi_out_cache(), UART_list_tempo);

    if (mt_usb_tempo_out_enabled() != 0) {
        send_usb_midi_message(&clock_tick, 1);
    }
    for (int i = 0; i < 2; i++) {
        if (UART_list_tempo[i] != NULL) {
            HAL_UART_Transmit(UART_list_tempo[i], &clock_tick, 1, MT_UART_TX_TIMEOUT_MS);
        }
    }
}

void mt_start_stop(void) {
	static uint8_t clock_start = 0xFA;
	static uint8_t clock_stop  = 0xFC;

	static UART_HandleTypeDef *UART_list_tempo[2];
	list_of_UART_to_send_to(*mt_send_to_midi_out_cache(), UART_list_tempo);

	uint8_t clock_sending = save_get(TEMPO_CURRENTLY_SENDING);

    // Stop clock
    if (clock_sending == 0) {
        mt_reset_clock_divider();
        for (int i = 0; i < 2; i++) {
            if (UART_list_tempo[i] != NULL) {
                HAL_UART_Transmit(UART_list_tempo[i], &clock_stop, 1, MT_UART_TX_TIMEOUT_MS);
            }
        }
        if (mt_usb_tempo_out_enabled() != 0) {
            send_usb_midi_message(&clock_stop, 1);
        }
    }
    // Start clock
    else if (clock_sending == 1) {
        mt_reset_clock_divider();
        for (int i = 0; i < 2; i++) {
            if (UART_list_tempo[i] != NULL) {
                HAL_UART_Transmit(UART_list_tempo[i], &clock_start, 1, MT_UART_TX_TIMEOUT_MS);
            }
        }
        if (mt_usb_tempo_out_enabled() != 0) {
            send_usb_midi_message(&clock_start, 1);
        }

    }
}
