/*
 * midi_tranform.h
 *
 *  Created on: Sep 5, 2025
 *      Author: Romain Dereu
 */

#ifndef MIDI_INC_MIDI_TRANSFORM_H_
#define MIDI_INC_MIDI_TRANSFORM_H_



typedef struct {
    uint8_t status;   ///< 0x8n = Note Off, 0x9n = Note On, etc.
    uint8_t note;     ///< 0–127 pitch value
    uint8_t velocity; ///< 0–127 velocity
} midi_note;

typedef struct {
    uint8_t uart1;
    uint8_t uart2;
    uint8_t usb;
} output_route_t;


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

uint8_t midi_message_length(uint8_t status);
void midi_change_channel(midi_note *midi_msg, uint8_t send_to_midi_channel);


void midi_buffer_push(uint8_t byte);
uint8_t midi_buffer_pop(uint8_t *byte);

void calculate_incoming_midi();


void emit_midi(const midi_note *msg, const output_route_t *route);
void emit_midi_with_policy(const midi_note *msg);
void send_midi_out(const midi_note *midi_message_raw, uint8_t length);
void send_usb_midi_out(const midi_note *midi_message_raw, uint8_t length);


#endif /* MIDI_INC_MIDI_TRANSFORM_H_ */
