/*
 * midi_usb.h
 *
 *  Created on: Jul 18, 2025
 *      Author: Romain Dereu
 */

#ifndef INC_MIDI_USB_H_
#define INC_MIDI_USB_H_

enum {
	MIDI_USB_OFF = 0,
	MIDI_USB_OUT,
	MIDI_USB_THRU,
	MIDI_USB_THRU_OUT
};

void send_usb_midi_message(uint8_t *midi_message, uint8_t length);



#endif /* INC_MIDI_USB_H_ */
