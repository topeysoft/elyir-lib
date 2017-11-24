#include <stdio.h>
#include <time.h>

#include "mgos.h"

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

#include "elyir_net_visual.h"


//  int ON_BOARD_LED = 13; /* sonoff basic LED pin */
// struct ElyirSetup {

bool mqtt_conn_flag = false;
bool net_conn_flag = false;
 mgos_timer_id net_conn_led_timer_id;
 mgos_timer_id mqtt_conn_led_timer_id;

//  uint8_t led_timer_ticks = 0;  /* for led blinker use */

 int get_led_gpio_pin(void) {
  return mgos_sys_config_get_pins_led();
  // return get_cfg()->pins.led;
}

 int mqtt_connected(void) {
	return (int) mqtt_conn_flag;
}

 void end_mqtt_conn_visual(){
    mgos_clear_timer(mqtt_conn_led_timer_id);
}
 void end_net_conn_visual(){
    mgos_clear_timer(net_conn_led_timer_id);
}


 void blink_on_board_led_cb(void *arg) {
    //  uint8_t remainder;
    // if (mqtt_conn_flag) {
    //     remainder = (++led_timer_ticks % 40);
    //     if (remainder == 0) {
    //         led_timer_ticks = 0;
    //         mgos_gpio_write(ON_BOARD_LED, 0);  // on
    //     } else if (remainder == 1) {
    //         mgos_gpio_write(ON_BOARD_LED, 1);  // off
    //     }
    // } else {
    //     mgos_gpio_toggle(ON_BOARD_LED);
    // }

    mgos_gpio_toggle(get_led_gpio_pin());
    if(net_conn_flag){
      end_net_conn_visual();
    }
    if(mqtt_conn_flag){
      end_mqtt_conn_visual();
    }
        
    (void) arg;
}
 void begin_net_conn_visual(){
      end_net_conn_visual();
    net_conn_led_timer_id = mgos_set_timer(1000 /* ms */, true /* repeat */, blink_on_board_led_cb, NULL);
}
 void begin_mqtt_conn_visual(){
   if(net_conn_flag){
      end_mqtt_conn_visual();
     mqtt_conn_led_timer_id = mgos_set_timer(200 /* ms */, true /* repeat */, blink_on_board_led_cb, NULL);
   } 

}


// void pub(struct mg_connection *c, const char *fmt, ...) {
  //char msg[200];
  //struct json_out jmo = JSON_OUT_BUF(msg, sizeof(msg));
  //va_list ap;
  //int n;
  //va_start(ap, fmt);
  //n = json_vprintf(&jmo, fmt, ap);
  //va_end(ap);
  //mg_mqtt_publish(c, get_cfg()->mqtt.pub, 0, MG_MQTT_QOS(0), msg, n);
  //LOG(LL_INFO, ("%s -> %s", get_cfg()->mqtt.pub, msg));
//}

 void net_ev_handler(enum mgos_net_event ev,
                         const struct mgos_net_event_data *ev_data, void *arg) {
    switch (ev) {
      case MGOS_NET_EV_DISCONNECTED:
        net_conn_flag = false;
       LOG(LL_INFO, ("NET Disconnected"));
              begin_net_conn_visual();
        break;
      case MGOS_NET_EV_IP_ACQUIRED:
        net_conn_flag = true;
       LOG(LL_INFO, ("NET Connected"));
        begin_mqtt_conn_visual();  
        break;
      default:
        break;
    }
    (void) arg;
    (void) ev_data;
}

void mqtt_ev_handler(struct mg_connection *c, int ev, void *p, void *user_data) {
  struct mg_mqtt_message *msg = (struct mg_mqtt_message *) p;
  if (ev == MG_EV_MQTT_CONNACK) {
   LOG(LL_INFO, ("CONNACK: %d", msg->connack_ret_code));
    mqtt_conn_flag = true;
    //if (get_cfg()->mqtt.pub == NULL) {
      //LOG(LL_ERROR, ("Run 'mos config-set mqtt.pub=... '"));
    //} else {
      //pub(c, "{timestamp:%.3lf, mem_free:%d}", mg_time(), mgos_get_free_heap_size() );  /* post uptime */
    //}
  } else if (ev == MG_EV_CLOSE) {
      mqtt_conn_flag = false;
        begin_mqtt_conn_visual();  
  }
  (void) user_data;
  (void) msg;
  (void) c;
}

void setup_elyir_net_visual() {

    mgos_net_add_event_handler(net_ev_handler, NULL);
    mgos_gpio_set_mode(get_led_gpio_pin(), MGOS_GPIO_MODE_OUTPUT);
    
    
    
    // mgos_set_timer(100 /* ms */, true /* repeat */, blink_on_board_led_cb, NULL);


    mgos_mqtt_add_global_handler(mqtt_ev_handler, NULL);
    
    begin_net_conn_visual();
    begin_mqtt_conn_visual();  
}
    // setup_elyir_net_visual();

// }