#ifndef THREADS_H_
#define THREADS_H_

#include "cmsis_os2.h"

#define MIDI_CORE_FLAG_ARP_TICK  (1 << 0)
#define MIDI_CORE_FLAG_TEMPO_OUT (1 << 1)
#define MIDI_CORE_FLAG_MASK      (MIDI_CORE_FLAG_ARP_TICK | MIDI_CORE_FLAG_TEMPO_OUT)

void threads_start(void);
void threads_display_notify(uint32_t flags);
void threads_midi_core_set_flags(uint32_t flags);
void threads_midi_core_notify_tempo_tick_from_isr(void);

osThreadId_t threads_display_handle(void);

#endif /* THREADS_H_ */
