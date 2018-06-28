#include <stdio.h>
#include <time.h>
// #include "mgos_bitbang.h"
#include "mgos_pwm.h"
#include "mgos.h"
#include "mgos_system.h"
#include "common/platform.h"
#include "common/cs_file.h"
#include "mgos_app.h"
#include "mgos_gpio.h"
#include "mgos_timers.h"
#include "mgos_hal.h"
#include "mgos_dlsym.h"
#include "mgos_mqtt.h"
#include "mgos_net.h"

#include "elyir_common.h"
#include "elyir_mqtt.h"
#include "elyir_net_visual.h"

int led_wait_time;
// float brightest = 1; // how bright the LED is
// float dullest = .0;
// float brightness = .0;   // how bright the LED is
// float fade_amount = .05;  // how many points to fade the LED by

bool mqtt_conn_flag = false;
bool net_conn_flag = false;
mgos_timer_id net_conn_led_timer_id;
mgos_timer_id mqtt_conn_led_timer_id;
mgos_timer_id fade_led_timer;

elyir_handler_t _mqtt_visual_feedback;
elyir_handler_t _wifi_visual_feedback;
elyir_handler_t _on_mqtt_visual_ended;
elyir_handler_t _on_net_visual_ended;

int get_led_gpio_pin(void)
{
  return mgos_sys_config_get_pins_led();
}

void elyir_set_on_mqtt_visual_ended_handler(elyir_handler_t cb)
{
  _on_mqtt_visual_ended = cb;
  return;
}
void elyir_set_on_net_visual_ended_handler(elyir_handler_t cb)
{
  _on_net_visual_ended = cb;
  return;
}

void save_sys_config(){
    char *msg = NULL;
  if (!save_cfg(&mgos_sys_config, &msg))
    {
      LOG(LL_ERROR, ("error saving wifi config: %s", (msg ? msg : "")));
      free(msg);
      return;
    }
}

void _net_on_connected(){
  mgos_sys_config_set_device_initial_config_completed(true); 
  save_sys_config(); 
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
    save_sys_config();
    LOG(LL_INFO, ("Access Point mode, disabled. Restarting device"));
    mgos_system_restart();
  }
  mgos_clear_timer(net_conn_led_timer_id);
  if (_on_net_visual_ended)
    _on_net_visual_ended();
}

void fade_led(void *arg)
{
  led_wait_time += 1;
  if (led_wait_time > 6)
  {
    if (led_wait_time >= 12)
      led_wait_time = 0;
    return;
  }
  mgos_gpio_toggle(get_led_gpio_pin());

  // analog_write(get_led_gpio_pin(), brightness);
  // mgos_pwm_set(get_led_gpio_pin(), 2000, brightness);

  // change the brightness for next time through the loop:
  // brightness = brightness + fade_amount;

  // // reverse the direction of the fading at the ends of the fade:
  // if (brightness <= dullest || brightness >= brightest)
  // {
  //    fade_amount = -fade_amount;
  //   //  fade_amount = 5;
  // }
  // LOG(LL_INFO, ("Fade Factor: %f, Brightness: %f", fade_amount, brightness));
  (void)arg;
}

void blink_on_board_led_cb(void *arg)
{
  // if (mgos_sys_config_get_wifi_ap_enable())
  // {
  //   return;
  // }

  LOG(LL_INFO, ("Should show MQTT visual feedback"));

 
    if (_wifi_visual_feedback)
    {
      _wifi_visual_feedback();
    }
    if (_mqtt_visual_feedback)
    {
      _mqtt_visual_feedback();
    }
   if (!_wifi_visual_feedback || !_mqtt_visual_feedback)
  {
    mgos_gpio_toggle(get_led_gpio_pin());
  }
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

void elyir_set_mqtt_visual_fb(elyir_handler_t cb)
{
  _mqtt_visual_feedback = cb;
}
void elyir_set_wifi_visual_fb(elyir_handler_t cb)
{
  _wifi_visual_feedback = cb;
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
    LOG(LL_INFO, ("Starting MQTT visual feedback"));
    end_mqtt_conn_visual();
    mqtt_conn_led_timer_id = mgos_set_timer(200 /* ms */, true /* repeat */, blink_on_board_led_cb, NULL);
  }
}

void net_ev_handler(int ev,
                    void *ev_data, void *arg)
{
  // struct mg_mqtt_message *msg = (struct mg_mqtt_message *)p;
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
      _net_on_connected();  
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

  if (ev == MG_EV_CONNECT)
  {
    int status = *((int *)p);
    // LOG(LL_INFO,
    //     ("MQTT TCP connect %s (%d)", (status == 0 ? "ok" : "error"), status));
    if (status != 0)
    {
      mqtt_conn_flag = false;
      LOG(LL_INFO, ("MQTT Disconnected"));
      begin_mqtt_conn_visual();
    }
    else
    {
      mqtt_conn_flag = true;
      LOG(LL_INFO, ("MQTT now connected"));
      end_mqtt_conn_visual();
    }
  }
  else if (ev == MG_EV_MQTT_CONNACK)
  {
    LOG(LL_INFO, ("MQTT Connected: %d", msg->connack_ret_code));
    LOG(LL_INFO, ("CONNACK: %d", msg->connack_ret_code));
    // mqtt_conn_flag = true;
    // end_mqtt_conn_visual();
  }
  else if (ev == MG_EV_MQTT_DISCONNECT)
  {
    mqtt_conn_flag = false;
    LOG(LL_INFO, ("MQTT Disconnected"));
    begin_mqtt_conn_visual();
  }
  else if (ev == MG_EV_CLOSE)
  {
    LOG(LL_INFO, ("MQTT closed"));
    mqtt_conn_flag = false;
    begin_mqtt_conn_visual();
  }
  (void)user_data;
  (void)msg;
  (void)c;
}

void setup_elyir_net_visual()
{

  // mgos_net_add_event_handler(net_ev_handler, NULL);
  mgos_event_add_group_handler(MGOS_EVENT_GRP_NET, net_ev_handler, NULL);
  mgos_gpio_set_mode(get_led_gpio_pin(), MGOS_GPIO_MODE_OUTPUT);

  // if (mgos_sys_config_get_wifi_ap_enable())
  // {
  //   fade_led_timer = mgos_set_timer(100 /* ms */, true /* repeat */, fade_led, NULL);
  // }

  // pinMode(get_led_gpio_pin(), OUTPUT);
  // mgos_set_timer(100 /* ms */, true /* repeat */, blink_on_board_led_cb, NULL);

  mgos_mqtt_add_global_handler(mqtt_ev_handler, NULL);

  begin_net_conn_visual();
  begin_mqtt_conn_visual();
}
// setup_elyir_net_visual();

// }