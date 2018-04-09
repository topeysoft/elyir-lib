
#ifndef ELYIR_COMMON_h
#define ELYIR_COMMON_h

// #include <stdlib.h>
// #include <stdint.h>
// #include <stdbool.h>
// #include <stddef.h>
// #include <stdarg.h>
// #include <stdio.h>
// #include <string.h>
// #include <math.h>

// #ifdef __cplusplus
// extern "C" {
// #endif

typedef void(* elyir_handler_t)(void);
typedef void(* elyir_button_cb_t)(int pin, int hold_time);

void analog_write(int pin, float value);
bool is_sta_ip();

// #ifdef __cplusplus
// }  // extern "C"
// #endif

#endif