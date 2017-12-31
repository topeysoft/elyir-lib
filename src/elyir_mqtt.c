#include "mgos.h"
#include "mgos.h"
#include "mgos_rpc.h"
#include "mgos_mqtt.h"
#include "mgos_utils.h"
#include "fw/src/mgos_timers.h"
#include "elyir_mqtt.h"

bool mqtt_conn = false;
bool elyir_mqtt_initialized = false;
mgos_timer_id status_timer_id = false;

elyir_handler_t _dev_state_cb;
elyir_handler_t _on_mqtt_connect;

struct sub_handler
{
  const char *topic_path;
  void *user_data;
  sub_handler_t cb;
  SLIST_ENTRY(sub_handler)
  sub_handlers;
};

struct elyir_mqtt
{
  SLIST_HEAD(sub_handlers, sub_handler)
  sub_handlers;
};

struct elyir_mqtt *ah;

void make_lowercase(char *str)
{
  for (int i = 0; str[i]; i++)
  {
    str[i] = tolower((unsigned char)str[i]);
  }
}

void make_topic(char *topic, const char *path)
{
  // char topic[200];
  sprintf(topic, mgos_sys_config_get_mqtt_base(),
          mgos_sys_config_get_owner_uid(), mgos_sys_config_get_device_id());
  strcat(topic, path);
  make_lowercase(topic);
  // return topic;
}
void elyir_add_mqtt_sub(const char *topic_path, sub_handler_t cb, void *ud)
{
  if (topic_path == NULL)
    return;
  struct sub_handler *sh =
      (struct sub_handler *)calloc(1, sizeof(*sh));
  sh->topic_path = topic_path;
  sh->cb = cb;
  sh->user_data = ud;
  SLIST_INSERT_HEAD(&ah->sub_handlers, sh, sub_handlers);
}
void elyir_set_device_state_handler(elyir_handler_t cb)
{
  _dev_state_cb = cb;
}
void elyir_set_on_mqtt_connect_handler(elyir_handler_t cb)
{
  _on_mqtt_connect = cb;
}

static void pub_info()
{
  // char topic[200];
  char info_tp[200];
  char info_get_tp[200];
  char state_tp[200];
  char state_set_tp[200];
  char state_get_tp[200];


  // make_topic(topic, mgos_sys_config_get_mqtt_topic_info());
  make_topic(info_tp, mgos_sys_config_get_mqtt_topic_info());
  make_topic(info_get_tp, mgos_sys_config_get_mqtt_topic_info_get());
  make_topic(state_tp, mgos_sys_config_get_mqtt_topic_state());
  make_topic(state_get_tp, mgos_sys_config_get_mqtt_topic_state_get());
  make_topic(state_set_tp, mgos_sys_config_get_mqtt_topic_state_set());

  mgos_sys_config_set_mqtt_will_topic(info_tp);
  
  char msg[400];
  struct json_out jmo = JSON_OUT_BUF(msg, sizeof(msg));
  json_printf(&jmo, "%M", mgos_print_sys_info);
  pub(info_tp, "%Q", msg);
  pub(info_tp, "{topics:{info:{dump:%Q, get: %Q}, state:{dump:%Q, get: %Q, set:%Q} }, type: %Q  }", info_tp, info_get_tp, state_tp
  , state_get_tp
  , state_set_tp
  , mgos_sys_config_get_device_type());
}

static void info_handler(struct mg_connection *c, const char *topic, int topic_len,
                         const char *msg, int msg_len, void *userdata)
{
  LOG(LL_DEBUG, ("Got info message on topic %.*s", topic_len, topic));
  LOG(LL_DEBUG, ("The message %s", msg));
  pub_info();
  (void)msg;
  (void)msg_len;
  (void)userdata;
  (void)c;
}
static void pub_status_cb(void *user_data)
{
  if (mqtt_conn && _dev_state_cb)
  {
    _dev_state_cb();
  }
  (void)user_data;
}
static void subscribe_all()
{
  struct sub_handler *sh;
  SLIST_FOREACH(sh, &ah->sub_handlers, sub_handlers)
  {
    char topic[200];
    make_topic(topic, sh->topic_path);
    mgos_mqtt_sub(topic, sh->cb, sh->user_data); /* Subscribe */
  }
}
void begin_status_interval()
{
  if (mqtt_conn)
  {
    status_timer_id = mgos_set_timer(mgos_sys_config_get_mqtt_status_interval() /* ms */, true /* repeat */, pub_status_cb, NULL);
  }
}

void end_status_interval()
{
  mgos_clear_timer(status_timer_id);
}

void sub(const char *topic_path, sub_handler_t handler)
{
  char topic[200];
  make_topic(topic, topic_path);
  LOG(LL_INFO, ("Subscribing to %s", topic));
  mgos_mqtt_sub(topic, handler, NULL);
}

void pub(const char *topic_path, const char *fmt, ...)
{
  if (mqtt_conn)
  {
    char topic[200];
    make_topic(topic, topic_path);
    char msg[1000];
    struct json_out jmo = JSON_OUT_BUF(msg, sizeof(msg));
    va_list ap;
    int len;
    va_start(ap, fmt);
    len = json_vprintf(&jmo, fmt, ap);
    va_end(ap);
    LOG(LL_DEBUG, ("%s -> %s", topic, msg));
    mgos_mqtt_pub(topic, msg, len, MG_MQTT_QOS(1), 0);
  }
}

void on_mqtt_connected(void *p, void *user_data)
{
  mqtt_conn = true;
  subscribe_all();
  pub_info();
  if (_on_mqtt_connect)
    _on_mqtt_connect();
  begin_status_interval();
  (void)user_data;
  (void)p;
}
void on_mqtt_disconnected(void *p, void *user_data)
{
  mqtt_conn = false;
  end_status_interval();
  (void)user_data;
  (void)p;
}
void on_mqtt_recieved(void *p, void *user_data)
{
  (void)user_data;
  (void)p;
}
void on_mqtt_subscribed(void *p, void *user_data)
{
  // cb(p, user_data);
  (void)user_data;
  (void)p;
}

static void mqtt_event_handler(struct mg_connection *c, int ev, void *p,
                               void *user_data)
{
  struct mg_mqtt_message *msg = (struct mg_mqtt_message *)p;

  if (ev == MG_EV_MQTT_CONNACK)
  {
    on_mqtt_connected(p, user_data);
  }
  else if (ev == MG_EV_MQTT_DISCONNECT)
  {
    on_mqtt_disconnected(p, user_data);
  }
  else if (ev == MG_EV_MQTT_SUBACK)
  {
    on_mqtt_subscribed(p, user_data);
    LOG(LL_INFO, ("Subscription %u acknowledged", msg->message_id));
  }
  else if (ev == MG_EV_MQTT_PUBLISH)
  {

    /* Our subscription is at QoS 1, we must acknowledge messages sent ot us. */
    mg_mqtt_puback(c, msg->message_id);
    on_mqtt_recieved(p, user_data);
  }
  (void)user_data;
}

void initialize()
{
  if (!elyir_mqtt_initialized)
  {
    ah = (struct elyir_mqtt *)calloc(1, sizeof(*ah));
    SLIST_INIT(&ah->sub_handlers);
    elyir_add_mqtt_sub(mgos_sys_config_get_mqtt_topic_info_get(), info_handler, NULL);
    elyir_mqtt_initialized = true;
  }
}

void elyir_handle_mqtt()
{
  initialize();
  mgos_mqtt_add_global_handler(mqtt_event_handler, NULL);
}
