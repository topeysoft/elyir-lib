
#include "elyir_common.h"
#ifndef ELYIR_CRON_h
#define ELYIR_CRON_h

void elyirCronInit();

void elyir_cron_set_device_state_handler(elyir_generic_state_callback_t handler);
#endif