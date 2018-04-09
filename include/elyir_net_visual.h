#ifndef ELYIR_NET_h
#define ELYIR_NET_h

#include "elyir_common.h"

#ifdef __cplusplus
extern "C" {
#endif


int get_led_gpio_pin();
void setup_elyir_net_visual();

void elyir_set_wifi_visual_fb(elyir_handler_t cb);
void elyir_set_mqtt_visual_fb(elyir_handler_t cb);
void elyir_set_on_mqtt_visual_ended_handler(elyir_handler_t cb);


#ifdef __cplusplus
}  // extern "C"
#endif

#endif