#ifndef ELYIR_h
#define ELYIR_h

#include "elyir_common.h"
#include "mongoose.h"

#ifdef __cplusplus
extern "C" {
#endif


#if CS_PLATFORM == CS_P_ESP8266
/* On ESP-12E there is a blue LED connected to GPIO2 (aka U1TX). */
#define BUTTON_PULL MGOS_GPIO_PULL_UP
#define BUTTON_EDGE MGOS_GPIO_INT_EDGE_POS
#elif CS_PLATFORM == CS_P_ESP32
/* Unfortunately, there is no LED on DevKitC, so this is random GPIO. */
#define BUTTON_PULL MGOS_GPIO_PULL_UP
#define BUTTON_EDGE MGOS_GPIO_INT_EDGE_POS
#elif CS_PLATFORM == CS_P_CC3200 || CS_PLATFORM == CS_P_CC3220
/* On CC3200 LAUNCHXL pin 64 is the red LED. */
#define BUTTON_PULL MGOS_GPIO_PULL_NONE /* External pull-downs */
#define BUTTON_EDGE MGOS_GPIO_INT_EDGE_NEG
#elif (CS_PLATFORM == CS_P_STM32) && defined(BSP_NUCLEO_F746ZG)
/* Nucleo-144 F746 */
#define BUTTON_PULL MGOS_GPIO_PULL_NONE
#define BUTTON_EDGE MGOS_GPIO_INT_EDGE_POS
#elif (CS_PLATFORM == CS_P_STM32) && defined(BSP_DISCO_F746G)
/* Discovery-0 F746 */
#define BUTTON_PULL MGOS_GPIO_PULL_NONE
#define BUTTON_EDGE MGOS_GPIO_INT_EDGE_POS
#else
#error Unknown platform
#endif

typedef void(* elyir_button_cb_t)(int pin, int hold_time);


void elyir_begin(void);

void save_state(char *state);
const char *get_state();

void elyir_set_button_handler(elyir_button_cb_t cb);

// void analog_write(int pin, float value);



bool mgos_elyir_lib_init(void);
// static void process_state(bool on);
// static void restore_def_state();
// void set_state_handler(struct state_handler);


#ifdef __cplusplus
}  // extern "C"
#endif

#endif