#ifndef THREADS_H_
#define THREADS_H_

#include "cmsis_os2.h"

void threads_start(void);
void threads_display_notify(uint32_t flags);
osThreadId_t threads_display_handle(void);

#endif /* THREADS_H_ */
