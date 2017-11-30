#include "mgos_mqtt.h"

void sub(const char *topic, sub_handler_t handler);
void pub(const char *topic, const char *fmt, ...);
void elyir_handle_mqtt();

void on_mqtt_connected(void *p, void *user_data);
// void on_mqtt_disconnected(struct mg_connection *c, void *p, void *user_data);
// void on_mqtt_recieved(struct mg_connection *c, void *p, void *user_data);
// void on_mqtt_subscribed(struct mg_connection *c, void *p, void *user_data);