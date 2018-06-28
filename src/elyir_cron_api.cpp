#include <stdbool.h>
#include <stdlib.h>

#include "common/cs_dbg.h"
#include "common/cs_file.h"
#include "common/json_utils.h"
#include "common/mbuf.h"
#include "common/mg_str.h"
#include "common/queue.h"
#include "frozen.h"

#include "mgos_sys_config.h"
#include "mgos.h"

#include "mgos_app.h"
#include "mgos_crontab.h"
#include "WString.h"

#include "elyir_mqtt.h"


#define ITEM_FMT "{id: %d, at: %T, enable: %B, action: %T, payload: %T}"

struct list_ctx {
  struct json_out out;
  int idx;
};

String elyir_cron_data = "";

/*
 * Returns newly allocated unescaped string
 */
static struct mg_str create_unescaped(struct mg_str s) {
  int len = json_unescape(s.p, s.len, NULL, 0);
  char *data = (char *) calloc(1, len);
  json_unescape(s.p, s.len, data, len);
  return mg_mk_str_n(data, len);
}

/*
 * NOTE: `at` and `action` are reallocated (because they need unescaping),
 * so the caller should free them.
 */
static char *parse_job_json(struct mg_str json, struct mg_str *at, bool *enable,
                            struct mg_str *action, struct mg_str *payload) {
  char *err = NULL;

  *enable = true;
  struct json_token t_at = JSON_INVALID_TOKEN;
  struct json_token t_action = JSON_INVALID_TOKEN;
  struct json_token t_payload = JSON_INVALID_TOKEN;
  json_scanf(json.p, json.len, "{at: %T, enable: %B, action: %T, payload: %T}",
             &t_at, enable, &t_action, &t_payload);

  if (t_at.ptr == NULL || t_action.ptr == NULL) {
    mg_asprintf(&err, 0, "'at' and 'action' are required");
    goto clean;
  }

  *at = create_unescaped(mg_mk_str_n(t_at.ptr, t_at.len));
  *action = create_unescaped(mg_mk_str_n(t_action.ptr, t_action.len));
  *payload = mg_mk_str_n(t_payload.ptr, t_payload.len);

clean:
  return err;
}

static mgos_crontab_job_id_t parse_id_json(struct mg_str json) {
  mgos_crontab_job_id_t id = MGOS_CRONTAB_INVALID_JOB_ID;
  json_scanf(json.p, json.len, "{id: %d}", &id);
  return id;
}

static int marshal_job_json(struct json_out *out, mgos_crontab_job_id_t id,
                            struct mg_str at, bool enable, struct mg_str action,
                            struct mg_str payload) {
  int ret = 0;

  ret += json_printf(out, "{");
  ret += json_printf(out, "id: %d, at: %.*Q, enable: %B, action: %.*Q", id,
                     at.len, at.p, enable, action.len, action.p);
  if (payload.p != NULL && payload.len > 0) {
    ret += json_printf(out, ", payload: %.*s", payload.len, payload.p);
  }
  ret += json_printf(out, "}");

  return ret;
}

static void list_cb(mgos_crontab_job_id_t id, struct mg_str at, bool enable,
                    struct mg_str action, struct mg_str payload,
                    void *userdata) {
  struct list_ctx *ctx = (struct list_ctx *) userdata;

  if (ctx->idx > 0) {
    json_printf(&ctx->out, ",");
  }

  marshal_job_json(&ctx->out, id, at, enable, action, payload);

  ctx->idx++;
}

static void elyir_cron_list_items() {
  char *err = NULL;

  struct mbuf databuf;
  mbuf_init(&databuf, 50);
  struct list_ctx ctx = {
      .out = JSON_OUT_MBUF(&databuf), .idx = 0,
  };

  json_printf(&ctx.out, "[");
  mgos_crontab_iterate(list_cb, &ctx, &err);
  if (err != NULL) {
    goto clean;
  }
  json_printf(&ctx.out, "]");

// mg_rpc_send_responsef(ri, "%.*s", databuf.len, databuf.buf);
  pub(mgos_sys_config_get_mqtt_topic_cron_api_get(), "{schedules: %.*s}", databuf.len, databuf.buf);
clean:
  free(err);
  mbuf_free(&databuf);
}


static void add_or_edit_job(bool edit, 
                            struct mg_str args) {
  char *err = NULL;

  struct mg_str at = MG_NULL_STR;
  bool enable = false;
  struct mg_str action = MG_NULL_STR;
  struct mg_str payload = MG_NULL_STR;
  mgos_crontab_job_id_t id = MGOS_CRONTAB_INVALID_JOB_ID;

  /* Get crontab job params from JSON args */
  err = parse_job_json(args, &at, &enable, &action, &payload);
  if (err != NULL) {
    LOG(LL_INFO, ("Error parsing cron job for add_or_edit: %s,", err));
    goto clean;
  }

  /* Add or edit the job */
  if (!edit) {
    mgos_crontab_job_add(at, enable, action, payload, &id, &err);
  } else {
    id = parse_id_json(args);
    mgos_crontab_job_edit(id, at, enable, action, payload, &err);
  }
  if (err != NULL) {
          LOG(LL_INFO, ("Error executing add_or_edit: %s,", err));
    goto clean;
  }

  elyir_cron_list_items();  

clean:
  free(err);

  /*
   * `at` and `action` are reallocated by `parse_job_json`, so we need to
   * free them
   */
  free((char *) at.p);
  free((char *) action.p);
}


static void elyir_cron_remove_item(struct mg_str args) {
  char *err = NULL;

  mgos_crontab_job_id_t id = parse_id_json(args);
  mgos_crontab_job_remove(id, &err);
  if (err != NULL) {
    goto clean;
  }

  elyir_cron_list_items();  

clean:
  free(err);
}

// static void elyir_cron_get_item(struct mg_str args) {
//   char *err = NULL;

//   struct mg_str at = MG_NULL_STR;
//   bool enable = false;
//   struct mg_str action = MG_NULL_STR;
//   struct mg_str payload = MG_NULL_STR;

//   struct mbuf databuf;
//   mbuf_init(&databuf, 0);

//   mgos_crontab_job_id_t id = parse_id_json(args);
//   mgos_crontab_job_get(id, &at, &enable, &action, &payload, &err);
//   if (err != NULL) {
//     goto clean;
//   }

//   mbuf_init(&databuf, 50);
//   {
//     struct json_out out = JSON_OUT_MBUF(&databuf);

//     marshal_job_json(&out, id, at, enable, action, payload);
//   }

//   // mg_rpc_send_responsef(ri, "%.*s", databuf.len, databuf.buf);
//   pub(mgos_sys_config_get_mqtt_topic_cron_api_get(), "%.*s", databuf.len, databuf.buf);

// clean:
//   free(err);

//   /*
//    * All string data is reallocated by mgos_crontab_job_get, so we need
//    * to free it
//    */
//   free((char *) at.p);
//   free((char *) action.p);
//   free((char *) payload.p);
//   mbuf_free(&databuf);
// }



static void elyir_scan_cron_data(const char *str, int len, void *user_data) {
    elyir_cron_data = String(str);
    
    (void) user_data;
    (void) len;
}

static void elyir_cron_api_handler(struct mg_connection *c, const char *topic, int topic_len,
                         const char *msg, int msg_len, void *userdata) {
    char method[15];
    char data[200];
    json_scanf((const char *)msg, msg_len, "{method:%s, data:%M}", method, elyir_scan_cron_data, data);
          LOG(LL_INFO, ("Recieved cron api call via MQTT: method== %s, data== %s", method,  elyir_cron_data.c_str()));
          if(String(method)== "add") {
            LOG(LL_INFO, ("Calling add method"));
            add_or_edit_job(false, mg_mk_str(elyir_cron_data.c_str()));
          }
          else if(String(method)=="edit"){
            LOG(LL_INFO, ("Calling edit method"));
            add_or_edit_job(true, mg_mk_str(elyir_cron_data.c_str()));
          }else if(String(method)=="remove"){
            LOG(LL_INFO, ("Calling remove method"));
            elyir_cron_remove_item(mg_mk_str(elyir_cron_data.c_str()));
          // }else if(String(method)=="get"){
          //   elyir_cron_get_item(mg_mk_str(data));
          }else if(String(method)=="list"){
            LOG(LL_INFO, ("Calling list method"));
            elyir_cron_list_items();
          }


    (void) c;
    (void) topic;
    (void) topic_len;
    (void) userdata;
}

void elyir_publish_cron_list(){
  elyir_cron_list_items();
}

void elyirCronApiInit(void) {
      elyir_add_mqtt_sub(mgos_sys_config_get_mqtt_topic_cron_api_set(), elyir_cron_api_handler, NULL);
}
