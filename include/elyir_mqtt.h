#include "mgos_mqtt.h"
#include "elyir_common.h"


void sub(const char *topic, sub_handler_t handler);
void pub(const char *topic, const char *fmt, ...);

void elyir_set_device_state_handler(elyir_handler_t cb);
void elyir_set_on_mqtt_connect_handler(elyir_handler_t cb);


void elyir_add_mqtt_sub(const char *topic_path, sub_handler_t cb, void *user_data);

void elyir_handle_mqtt();

void on_mqtt_connected(void *p, void *user_data);
// void on_mqtt_disconnected(struct mg_connection *c, void *p, void *user_data);
// void on_mqtt_recieved(struct mg_connection *c, void *p, void *user_data);
// void on_mqtt_subscribed(struct mg_connection *c, void *p, void *user_data);