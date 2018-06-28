#include "elyir_common.h"
#include "elyir_mqtt.h"

#include "mgos_crontab.h"
#include "mgos.h"

elyir_generic_state_callback_t _cron_dev_state_cb;

static void stateAction(struct mg_str action, struct mg_str payload,
                                void *userdata) {
  LOG(LL_INFO, ("Crontab stateAction job fired! %d Payload: %.s", payload.len, payload.p));
  if (_cron_dev_state_cb)
  {
    _cron_dev_state_cb(payload.p, "Schedule");
  }
  (void) action;
  (void) userdata;
}

void elyir_cron_set_device_state_handler(elyir_generic_state_callback_t cb)
{
  _cron_dev_state_cb = cb;
}









void elyirCronInit(){
    mgos_crontab_register_handler(mg_mk_str("setState"), stateAction, NULL);
}
