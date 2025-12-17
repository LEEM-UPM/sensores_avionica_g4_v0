#ifndef MIDD_PADS_H
#define MIDD_PADS_H

#include <stdbool.h>
#include <stddef.h>

#include "WSEN_PADS_2511020213301.h"

extern WE_sensorInterface_t pads1;

bool pads_init(WE_sensorInterface_t *pads);
bool pads_start(WE_sensorInterface_t *pads, int fifo_threshold);
bool pads_read_fifo(WE_sensorInterface_t *pads, int32_t *buffer, size_t len);

void pads_int_handler(WE_sensorInterface_t *pads);
bool pads_event_pending(WE_sensorInterface_t *pads);
void pads_clear_event(WE_sensorInterface_t *pads);

#endif /* MIDD_PADS_H */