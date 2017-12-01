
#include "mgos.h"
#include "elyir.h"
#include "elyir_mqtt.h"
#include "mgos_sys_config.h"
#include "elyir_net_visual.h"

void save_state(char *state)
{
  mgos_sys_config_set_device_default_state(state);

  char *msg = NULL;
  if (!save_cfg(&mgos_sys_config, &msg))
  {
    LOG(LL_ERROR, ("error saving device state: %s", (msg ? msg : "")));
    free(msg);
    return;
  }
}
const char *get_state()
{
  return mgos_sys_config_get_device_default_state();
}

void elyir_begin()
{
  elyir_handle_mqtt();
  setup_elyir_net_visual();
}

bool mgos_elyir_lib_init(void)
{
  elyir_begin();
  return true;
}
