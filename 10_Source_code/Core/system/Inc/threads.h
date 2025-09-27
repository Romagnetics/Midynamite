#ifndef THREADS_H_
#define THREADS_H_

#include "cmsis_os.h"


typedef struct {
  TIM_HandleTypeDef *htim2; // tempo timer (start/stop)
  TIM_HandleTypeDef *htim3; // encoder A
  TIM_HandleTypeDef *htim4; // encoder B
} threads_ctx_t;

typedef enum {
    FLAG_TEMPO      = (1u << 0),
    FLAG_MODIFY     = (1u << 1),
    FLAG_TRANSPOSE  = (1u << 2),
    FLAG_SETTINGS   = (1u << 3)
} DisplayFlags_t;

// Start all application threads (display, midi_core, medium_tasks)
void threads_start();
void threads_display_notify(uint32_t flags);
osThreadId_t threads_display_handle(void);


#endif /* THREADS_H_ */
