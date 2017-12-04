#include <stdio.h>
#include <time.h>
// #include "mgos_bitbang.h"
#include "mgos_pwm.h"
// #include "arduino.h"
#include "mgos.h"
#include "mgos_system.h"

#include "common/platform.h"
#include "common/cs_file.h"
#include "fw/src/mgos_app.h"
#include "fw/src/mgos_gpio.h"
// #include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_timers.h"
#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_dlsym.h"
#include "mgos_mqtt.h"
#include "mgos_net.h"

#include "elyir_mqtt.h"
#include "elyir_net_visual.h"

int brightest = 128; // how bright the LED is
int dullest = 64;   
int brightness = 0;   // how bright the LED is
int fade_amount = 2;  // how many points to fade the LED by

bool mqtt_conn_flag = false;
bool net_conn_flag = false;
mgos_timer_id net_conn_led_timer_id;
mgos_timer_id mqtt_conn_led_timer_id;
mgos_timer_id fade_led_timer;

elyir_handler_t _on_mqtt_visual_ended;
elyir_handler_t _on_net_visual_ended;

int get_led_gpio_pin(void)
{
  return mgos_sys_config_get_pins_led();
}

void elyir_set_on_mqtt_visual_ended_handler(elyir_handler_t cb)
{
  _on_mqtt_visual_ended = cb;
}
void elyir_set_on_net_visual_ended_handler(elyir_handler_t cb)
{
  _on_net_visual_ended = cb;
}

int mqtt_connected(void)
{
  return (int)mqtt_conn_flag;
}

void end_mqtt_conn_visual()
{
  mgos_clear_timer(mqtt_conn_led_timer_id);
  if (_on_mqtt_visual_ended)
    _on_mqtt_visual_ended();
}
void end_net_conn_visual()
{
  // if (!mgos_sys_config_get_wifi_ap_enable())
  // {
  //   mgos_clear_timer(fade_led_timer);
  // }
  if (mgos_sys_config_get_wifi_ap_enable() && mgos_sys_config_get_wifi_sta_enable() && net_conn_flag)
  {
    LOG(LL_INFO, ("WiFi connected, disabling Access Point mode."));
    mgos_sys_config_set_wifi_ap_enable(false);
    char *msg = NULL;
    if (!save_cfg(&mgos_sys_config, &msg))
    {
      LOG(LL_ERROR, ("error saving wifi config: %s", (msg ? msg : "")));
      free(msg);
      return;
    }
    LOG(LL_INFO, ("Access Point mode, disabled. Restarting device"));
    mgos_system_restart();
  }
  mgos_clear_timer(net_conn_led_timer_id);
  if (_on_net_visual_ended)
    _on_net_visual_ended();
}

void fade_led(void *arg)
{
  // analogWrite(get_led_gpio_pin(), brightness);
  mgos_pwm_set(get_led_gpio_pin(), 2000, brightness);

  // change the brightness for next time through the loop:
  brightness = brightness + fade_amount;

  // reverse the direction of the fading at the ends of the fade:
  if (brightness <= dullest || brightness >= brightest)
  {
    fade_amount *= -1;
  }
  LOG(LL_INFO, ("Fade Factor: %d, Brightness: %d", fade_amount, brightness));
  (void)arg;
}

void blink_on_board_led_cb(void *arg)
{
  if (mgos_sys_config_get_wifi_ap_enable())
  {
    return;
  }

  mgos_gpio_toggle(get_led_gpio_pin());
  if (net_conn_flag)
  {
    end_net_conn_visual();
  }
  if (mqtt_conn_flag)
  {
    end_mqtt_conn_visual();
  }

  (void)arg;
}
void begin_net_conn_visual()
{
  end_net_conn_visual();
  net_conn_led_timer_id = mgos_set_timer(1000 /* ms */, true /* repeat */, blink_on_board_led_cb, NULL);
}
void begin_mqtt_conn_visual()
{
  if (net_conn_flag)
  {
    end_mqtt_conn_visual();
    mqtt_conn_led_timer_id = mgos_set_timer(200 /* ms */, true /* repeat */, blink_on_board_led_cb, NULL);
  }
}

bool is_sta_ip()
{
  struct mgos_net_ip_info ip_info;
  memset(&ip_info, 0, sizeof(ip_info));
        return mgos_net_get_ip_info(MGOS_NET_IF_TYPE_WIFI, MGOS_NET_IF_WIFI_STA,
                                &ip_info);
}

void net_ev_handler(enum mgos_net_event ev,
                    const struct mgos_net_event_data *ev_data, void *arg)
{
  switch (ev)
  {
  case MGOS_NET_EV_DISCONNECTED:
    net_conn_flag = false;
    LOG(LL_INFO, ("NET Disconnected"));
    begin_net_conn_visual();
    break;
  case MGOS_NET_EV_IP_ACQUIRED:
    if (is_sta_ip())
    {
      net_conn_flag = true;
      LOG(LL_INFO, ("NET Connected"));
      end_net_conn_visual();
      begin_mqtt_conn_visual();
    }
    break;
  default:
    break;
  }
  (void)arg;
  (void)ev_data;
}

void mqtt_ev_handler(struct mg_connection *c, int ev, void *p, void *user_data)
{
  struct mg_mqtt_message *msg = (struct mg_mqtt_message *)p;
  if (ev == MG_EV_MQTT_CONNACK)
  {
    LOG(LL_INFO, ("MQTT Connected: %d", msg->connack_ret_code));
    LOG(LL_INFO, ("CONNACK: %d", msg->connack_ret_code));
    mqtt_conn_flag = true;
    //if (get_cfg()->mqtt.pub == NULL) {
    //LOG(LL_ERROR, ("Run 'mos config-set mqtt.pub=... '"));
    //} else {
    //pub(c, "{timestamp:%.3lf, mem_free:%d}", mg_time(), mgos_get_free_heap_size() );  /* post uptime */
    //}
  }
  else if (ev == MG_EV_MQTT_DISCONNECT)
  {
    mqtt_conn_flag = false;
    LOG(LL_INFO, ("MQTT Disconnected"));
    begin_mqtt_conn_visual();
  }
  (void)user_data;
  (void)msg;
  (void)c;
}

void setup_elyir_net_visual()
{

  brightness = dullest;
  mgos_net_add_event_handler(net_ev_handler, NULL);
  mgos_gpio_set_mode(get_led_gpio_pin(), MGOS_GPIO_MODE_OUTPUT);
  if(mgos_sys_config_get_wifi_ap_enable()){
    fade_led_timer = mgos_set_timer(30 /* ms */, true /* repeat */, fade_led, NULL);
  }

  // mgos_set_timer(100 /* ms */, true /* repeat */, blink_on_board_led_cb, NULL);

  mgos_mqtt_add_global_handler(mqtt_ev_handler, NULL);

  begin_net_conn_visual();
  begin_mqtt_conn_visual();
}
// setup_elyir_net_visual();

// }