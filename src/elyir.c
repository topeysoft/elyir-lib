
#include "mgos.h"
#include "elyir.h"
#include "elyir_mqtt.h"
#include "elyir_net_visual.h"

void elyir_begin() {
  elyir_handle_mqtt();
  setup_elyir_net_visual();
}


bool mgos_elyir_lib_init(void) {
  elyir_begin();
  return true;
}