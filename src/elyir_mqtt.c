#include "mgos.h"
#include "mgos.h"
#include "mgos_rpc.h"
#include "mgos_mqtt.h"
#include "mgos_utils.h"
#include "elyir_mqtt.h"
#include "frozen/frozen.h"


void sub(const char *topic, sub_handler_t handler) {
  LOG(LL_INFO, ("Subscribing to %s", topic));
  mgos_mqtt_sub(topic, handler, NULL);
}

void pub(const char *topic, const char *fmt, ...) {
  char msg[200];
  struct json_out jmo = JSON_OUT_BUF(msg, sizeof(msg));
  va_list ap;
  int len;
  va_start(ap, fmt);
  len = json_vprintf(&jmo, fmt, ap);
  va_end(ap);
  LOG(LL_DEBUG, ("%s -> %s", topic, msg));
  mgos_mqtt_pub(topic, msg, len, MG_MQTT_QOS(1), 0);
}
 
 void make_lowercase(char *str){
  for (int i=0; str[i]; i++){
    str[i] = tolower((unsigned char)str[i]);
  }
 }

 static void pub_info(void){
   char topic[200];
    sprintf(topic, mgos_sys_config_get_mqtt_topic_info(), mgos_sys_config_get_owner_uid(), mgos_sys_config_get_device_id());
    make_lowercase(topic);
    char msg[200];
    struct json_out *jmo = JSON_OUT_BUF(msg, sizeof(msg));
    mgos_print_sys_info(jmo);
    pub(topic, "%Q", msg);
 }
static void info_handler(struct mg_connection *c, const char *topic, int topic_len,
                    const char *msg, int msg_len, void *userdata) {
  LOG(LL_DEBUG, ("Got info message on topic %.*s", topic_len, topic));
  LOG(LL_DEBUG, ("The message %s", msg));
  pub_info();
  (void)msg;
  (void)msg_len;
  (void)userdata;
  (void)c;
}
void on_mqtt_connected(void *p, void *user_data){
    char info_get[200];
    sprintf(info_get, mgos_sys_config_get_mqtt_topic_info_get(), mgos_sys_config_get_owner_uid(), mgos_sys_config_get_device_id());
    make_lowercase(info_get);
    mgos_mqtt_sub(info_get, info_handler, NULL);       /* Subscribe */
    pub_info();
  (void)user_data;
  (void)p;
}
void on_mqtt_disconnected( void *p, void *user_data){
  (void)user_data;
  (void)p;
}
void on_mqtt_recieved( void *p, void *user_data){
  // struct mg_mqtt_message *msg = (struct mg_mqtt_message *) p;
  // struct mg_str *s = &msg->payload;
  //   struct json_token t = JSON_INVALID_TOKEN;
  //   char buf[100], asciibuf[sizeof(buf) * 2 + 1];
  //   if (json_scanf(s->p, s->len, "{gpio: {pin: %d, state: %d}}", &pin,
  //                   &state) == 2) {
  //         // pub(c, "{type: %Q, status: %d, data: %Q}", "i2c_read", ret, asciibuf);
  //   }
  (void)user_data;
  (void)p;
}
void on_mqtt_subscribed( void *p, void *user_data){
  // cb(p, user_data);
  (void)user_data;
  (void)p;
}



static void mqtt_event_handler(struct mg_connection *c, int ev, void *p,
                       void *user_data) {
  struct mg_mqtt_message *msg = (struct mg_mqtt_message *) p;

  if (ev == MG_EV_MQTT_CONNACK) {
    on_mqtt_connected(p, user_data);
    // LOG(LL_INFO, ("CONNACK: %d", msg->connack_ret_code));
    // if (mgos_sys_config_get_mqtt_sub() == NULL ||
    //     mgos_sys_config_get_mqtt_pub() == NULL) {
    //   LOG(LL_ERROR, ("Run 'mgos config-set mqtt.sub=... mqtt.pub=...'"));
    // } else {
    //   sub(c, "%s", mgos_sys_config_get_mqtt_sub());
    // }
  } else if (ev == MG_EV_MQTT_SUBACK) {
    on_mqtt_subscribed(p, user_data);
    LOG(LL_INFO, ("Subscription %u acknowledged", msg->message_id));
  } else if (ev == MG_EV_MQTT_PUBLISH) {
    

    /* Our subscription is at QoS 1, we must acknowledge messages sent ot us. */
    mg_mqtt_puback(c, msg->message_id);
    on_mqtt_recieved(p, user_data);
  }
  (void) user_data;
}

void elyir_handle_mqtt(){
  mgos_mqtt_add_global_handler(mqtt_event_handler, NULL);
}
