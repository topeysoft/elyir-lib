#include "elyir.h"
// #include "elyir_mqtt.h"
#include "elyir_net_visual.h"


// void sub(struct mg_connection *c, const char *fmt, ...) {
//     subscribe(c, fmt, ...);
// }
// void pub(struct mg_connection *c, const char *fmt, ...) {
//     publish(c, fmt, ...);
// }
void elyir_begin() {
    setup_elyir_net_visual();
}