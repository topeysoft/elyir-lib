typedef void(* elyir_handler_t)(void);
typedef void(* elyir_button_cb_t)(int pin, int hold_time);

void analog_write(int pin, float value);
bool is_sta_ip();