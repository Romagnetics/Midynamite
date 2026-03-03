/*
 * midi_tranform.h
 *
 *  Created on: Sep 5, 2025
 *      Author: Astaa
 */

#ifndef MIDI_INC_MIDI_TRANSFORM_H_
#define MIDI_INC_MIDI_TRANSFORM_H_



typedef struct {
    uint8_t status;   ///< 0x8n = Note Off, 0x9n = Note On, etc.
    uint8_t note;     ///< 0–127 pitch value
    uint8_t velocity; ///< 0–127 velocity
} midi_note;


// ---------------------
// Definitions of pipelines
// ---------------------
void pipeline_start(midi_note *midi_msg);

void pipeline_midi_split(midi_note *midi_msg);
void pipeline_midi_modify(midi_note *midi_msg);
void pipeline_midi_transpose(midi_note *midi_msg);
void pipeline_arp(midi_note *midi_msg, uint8_t length);

void pipeline_final(midi_note *midi_msg, uint8_t length) ;


//midi_modify_transform
uint8_t midi_is_note_message(const midi_note *msg, uint8_t *is_note_on);
void midi_buffer_push(uint8_t byte);
uint8_t midi_buffer_pop(uint8_t *byte);

void calculate_incoming_midi();


void send_midi_out(midi_note *midi_message_raw, uint8_t length);
void send_usb_midi_out(midi_note *midi_message_raw, uint8_t length);


#endif /* MIDI_INC_MIDI_TRANSFORM_H_ */
