
#include "mgos.h"
#include "mgos_sys_config.h"
#include "elyir_net_visual.h"
#include "mgos_gpio.h"
#include "fw/src/mgos_timers.h"
#include "mgos_system.h"
#include "elyir_common.h"
#include "elyir_mqtt.h"
#include "elyir_net_visual.h"
#include "elyir.h"

elyir_button_cb_t _button_cb;
int ev_monitor_time = 500;
int bt_hold_time = 0;
int wifi_conn_time = 0;
bool net_conn = false;

void save_config()
{
  char *msg = NULL;
  if (!save_cfg(&mgos_sys_config, &msg))
  {
    LOG(LL_ERROR, ("error saving device state: %s", (msg ? msg : "")));
    free(msg);
    return;
  }
}
void save_state(char *state)
{
  mgos_sys_config_set_device_default_state(state);
  save_config();
}
const char *get_state()
{
  return mgos_sys_config_get_device_default_state();
}

void elyir_set_button_handler(elyir_button_cb_t cb)
{
  _button_cb = cb;
}
void reset_to_factory()
{
  mgos_config_reset(MGOS_CONFIG_LEVEL_USER);
  mgos_system_restart();
}
void reset_wifi()
{
  mgos_sys_config_set_wifi_ap_enable(true);
  // mgos_sys_config_set_wifi_sta_ssid(" ");
  // mgos_sys_config_set_wifi_sta_pass(" ");
  save_config();
  mgos_system_restart();
}
void check_button_press(){
  bool pressed = !mgos_gpio_read(mgos_sys_config_get_pins_button());
  if (pressed)
  {
    bt_hold_time += ev_monitor_time;
  }
  else
  {
    bt_hold_time = 0;
  }
}
void check_wifi_conn(){
  if (!net_conn)
  {
    wifi_conn_time += ev_monitor_time;
  }
  else
  {
    wifi_conn_time = 0;
  }
  if(wifi_conn_time >= mgos_sys_config_get_device_wifi_fail_time() 
  && !mgos_sys_config_get_wifi_ap_enable() ){
    reset_wifi();
  }
}

void ev_monitor_cb(void *arg)
{
  check_button_press();
  check_wifi_conn();
  (void)arg;
}
void button_cb(int pin, void *arg)
{
  if (bt_hold_time >= mgos_sys_config_get_device_bt_factory_reset_time())
  {
    LOG(LL_INFO, ("Held button for: %d ms: ", bt_hold_time));
    LOG(LL_INFO, ("Restoring factory default setting"));
    reset_to_factory();
    return;
  }
  // if (bt_hold_time >= mgos_sys_config_get_device_bt_wifi_reset_time())
  // {
  //   LOG(LL_INFO, ("Held button for: %d ms: ", bt_hold_time));
  //   LOG(LL_INFO, ("Resetting WiFi config"));
  //   reset_wifi();
  //   return;
  // }
  LOG(LL_INFO, ("Click , PIN: %d, Held for : %d ms ", pin, bt_hold_time));
  if (_button_cb)
    _button_cb(pin, bt_hold_time);
  // bool state = !mgos_gpio_read_out(mgos_sys_config_get_pins_relay());
  // process_state(state);
  // mgos_gpio_toggle(mgos_sys_config_get_pins_relay());
  // match_led_light();
  // (void)pin;
  (void)arg;
}



void wifi_ev_handler(enum mgos_net_event ev,
                    const struct mgos_net_event_data *ev_data, void *arg)
{
  switch (ev)
  {
  case MGOS_NET_EV_DISCONNECTED:
    net_conn = false;
    break;
  case MGOS_NET_EV_IP_ACQUIRED:
    net_conn = true;
    break;
  default:
    break;
  }
  (void)arg;
  (void)ev_data;
}
void elyir_begin()
{
  // mgos_sys_config_set_wifi_ap_keep_enabled(false);
  mgos_set_timer(ev_monitor_time /* ms */, true /* repeat */, ev_monitor_cb, NULL);
  mgos_gpio_set_button_handler(mgos_sys_config_get_pins_button(), BUTTON_PULL, BUTTON_EDGE,
                               100 /* debounce_ms */, button_cb, NULL);
  mgos_net_add_event_handler(wifi_ev_handler, NULL);

  elyir_handle_mqtt();
  setup_elyir_net_visual();
}

bool mgos_elyir_lib_init(void)
{
  elyir_begin();
  return true;
}