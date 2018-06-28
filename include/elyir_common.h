
#ifndef ELYIR_COMMON_h
#define ELYIR_COMMON_h


typedef void(* elyir_handler_t)(void);
typedef void(* elyir_generic_state_callback_t)(const char *state_str, const char *caller);
typedef void(* elyir_button_cb_t)(int pin, int hold_time);

void analog_write(int pin, float value);
bool is_sta_ip();

void elyir_set_device_state_handler(elyir_generic_state_callback_t hd);


#endif