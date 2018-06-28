/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "mgos_ota_http_client.h"

#include "common/cs_dbg.h"
#include "mgos_hal.h"
#include "mgos_mongoose.h"
#include "mgos_sys_config.h"
#include "mgos_timers.h"
#include "mgos_utils.h"
#include "WString.h"

#include "elyir_mqtt.h"
#include "elyir_ota_client.h"

#define ARDUINOJSON_ENABLE_ARDUINO_STREAM 0
#include "ArduinoJson.h"

// #if MGOS_ENABLE_UPDATER
String elyir_ota_url_data = "";

static void fw_download_handler(struct mg_connection *c, int ev, void *p,
                                void *user_data)
{
    struct mbuf *io = &c->recv_mbuf;
    struct update_context *ctx = (struct update_context *)user_data;
    int res = 0;
    struct mg_str *loc;
    (void)p;

    switch (ev)
    {
    case MG_EV_CONNECT:
    {
        int result = *((int *)p);
        if (result != 0)
            LOG(LL_ERROR, ("connect error: %d", result));
        break;
    }
    case MG_EV_RECV:
    {
        if (ctx->file_size == 0)
        {
            LOG(LL_DEBUG, ("Looking for HTTP header"));
            struct http_message hm;
            int parsed = mg_parse_http(io->buf, io->len, &hm, 0);
            if (parsed <= 0)
            {
                return;
            }
            if (hm.resp_code != 200)
            {
                if (hm.resp_code == 304)
                {
                    ctx->result = 1;
                    ctx->need_reboot = false;
                    ctx->status_msg = "Not Modified";
                    updater_finish(ctx);
                }
                else if ((hm.resp_code == 301 || hm.resp_code == 302) &&
                         (loc = mg_get_http_header(&hm, "Location")) != NULL)
                {
                    /* NUL-terminate the URL. Every header must be followed by \r\n,
             * so there is deifnitely space there. */
                    ((char *)loc->p)[loc->len] = '\0';
                    /* We were told to look elsewhere. Detach update context from this
             * connection so that it doesn't get finalized when it's closed. */
                    elyir_mgos_ota_http_start(ctx, loc->p);
                    c->user_data = NULL;
                }
                else
                {
                    ctx->result = -hm.resp_code;
                    ctx->need_reboot = false;
                    ctx->status_msg = "Invalid HTTP response code";
                    updater_finish(ctx);
                }
                c->flags |= MG_F_CLOSE_IMMEDIATELY;
                return;
            }
            if (hm.body.len != 0)
            {
                LOG(LL_DEBUG, ("HTTP header: file size: %d", (int)hm.body.len));
                if (hm.body.len == (size_t)~0)
                {
                    LOG(LL_ERROR, ("Invalid content-length, perhaps chunked-encoding"));
                    ctx->status_msg =
                        "Invalid content-length, perhaps chunked-encoding";
                    c->flags |= MG_F_CLOSE_IMMEDIATELY;
                    break;
                }
                else
                {
                    ctx->file_size = hm.body.len;
                }

                mbuf_remove(io, parsed);
            }
        }

        if (io->len != 0)
        {
            res = updater_process(ctx, io->buf, io->len);
            mbuf_remove(io, io->len);

            if (res == 0)
            {
                if (is_write_finished(ctx))
                    res = updater_finalize(ctx);
                if (res == 0)
                {
                    /* Need more data, everything is OK */
                    break;
                }
            }

            if (res < 0)
            {
                /* Error */
                LOG(LL_ERROR, ("Update error: %d %s", ctx->result, ctx->status_msg));
            }
            c->flags |= MG_F_CLOSE_IMMEDIATELY;
        }
        break;
    }
    case MG_EV_CLOSE:
    {
        if (ctx == NULL)
            break;

        if (is_write_finished(ctx))
            updater_finalize(ctx);

        if (!is_update_finished(ctx))
        {
            /* Update failed or connection was terminated by server */
            if (ctx->status_msg == NULL)
                ctx->status_msg = "Update failed";
            ctx->result = -1;
        }
        else if (is_reboot_required(ctx))
        {
            LOG(LL_INFO, ("Rebooting device"));
            mgos_system_restart_after(100);
        }
        updater_finish(ctx);
        updater_context_free(ctx);
        c->user_data = NULL;
        break;
    }
    }
}

void elyir_publish_ota_result(struct update_context *ctx)
{
    pub(mgos_sys_config_get_mqtt_topic_ota_api_get(), "{ota_status: {result:%d, status:%.*s}",
        ctx->result,
        ctx->status_msg);
}


void elyir_mgos_ota_http_start(struct update_context *ctx, const char *url)
{
    LOG(LL_INFO, ("Update URL: %s, ct: %d, isv? %d", url,
                  ctx->fctx.commit_timeout, ctx->ignore_same_version));

    struct mg_connect_opts opts;
    memset(&opts, 0, sizeof(opts));

#if MG_ENABLE_SSL
    if (strlen(url) > 8 && strncmp(url, "https://", 8) == 0)
    {
        opts.ssl_server_name = mgos_sys_config_get_update_ssl_server_name();
        opts.ssl_ca_cert = mgos_sys_config_get_update_ssl_ca_file();
        opts.ssl_cert = mgos_sys_config_get_update_ssl_client_cert_file();
    }
#endif

    char ehb[150];
    char *extra_headers = ehb;
    // mg_asprintf(
    //     &extra_headers, sizeof(ehb),
    //     "X-MGOS-Device-ID: %s %s\r\n"
    //     "X-MGOS-FW-Version: %s %s %s\r\n"
    //     , (mgos_sys_config_get_device_id() ? mgos_sys_config_get_device_id() : "-")
    //     , mgos_sys_ro_vars_get_mac_address(), mgos_sys_ro_vars_get_arch()
    //     , mgos_sys_ro_vars_get_fw_version(), mgos_sys_ro_vars_get_fw_id()
    //     );

    struct mg_connection *c = mg_connect_http_opt(
        mgos_get_mgr(), fw_download_handler, ctx, opts, url, extra_headers, NULL);

    if (extra_headers != ehb)
        free(extra_headers);

    if (c == NULL)
    {
        LOG(LL_ERROR, ("Failed to connect to %s", url));
        ctx->result = -10;
        ctx->need_reboot = false;
        ctx->status_msg = "Failed to connect";
        updater_finish(ctx);
        return;
    }

    ctx->nc = c;
}
static void elyir_beginOta(const char *url)
{
    // if (mgos_sys_config_get_device_ota_url() == NULL)
    if (url == NULL)
        return;
    LOG(LL_INFO, ("Begining update from %s", url));
    struct update_context *ctx = updater_context_create();
    if (ctx == NULL)
        return;
    ctx->ignore_same_version = true;
    ctx->fctx.commit_timeout = mgos_sys_config_get_update_commit_timeout();
    elyir_mgos_ota_http_start(ctx, url);
    elyir_publish_ota_result(ctx);
}


// static void mgos_ota_timer_cb(void *arg)
// {
//     elyir_beginOta();
//     (void)arg;
// }

// bool mgos_ota_http_client_init(void) {
//   const char *url = mgos_sys_config_get_update_url();
//   int interval = mgos_sys_config_get_update_interval();
//   if (url != NULL && interval > 0) {
//     LOG(LL_INFO, ("Updates from %s, every %d seconds", url, interval));
//     mgos_set_timer(interval * 1000, true /* repeat */, mgos_ota_timer_cb,
//                    (void *) url);
//   }
//   return true;
// }
// static void elyir_scan_ota_url_data(const char *str, int len, void *user_data) {
//     elyir_ota_url_data = str;
    
//     (void) user_data;
//     (void) len;
// }
// #endif /* MGOS_ENABLE_UPDATER */
static void elyir_ota_api_handler(struct mg_connection *c, const char *topic, int topic_len,
                                  const char *msg, int msg_len, void *userdata)
{
    // char method[5];
    // char data[200];
    // char json[] = "{\"sensor\":\"gps\",\"time\":1351824120,\"data\":[48.756080,2.302038]}";

    StaticJsonBuffer<300> jsonBuffer;

    JsonObject& root = jsonBuffer.parseObject(msg);

    const char* method = root["method"];
    const char* data          = root["data"];

    // json_scanf((const char *)msg, msg_len, "{method:%s, data:%Q}", method, data);
    LOG(LL_INFO, ("Recieved cron api call via MQTT: topic: method== %s, data== %s", method, data));
    if (String(method) == "url")
    {
        LOG(LL_INFO, ("Setting OTA URL via MQTT to %s", data));
        // mgos_sys_config_set_device_ota_url(data);
        elyir_beginOta(data);
    }
    //   else if(String(method)=="info"){
    //     LOG(LL_INFO, ("Calling edit method"));
    //     add_or_edit_job(true, mg_mk_str(elyir_cron_data.c_str()));
    //   }else if(String(method)=="remove"){
    //     LOG(LL_INFO, ("Calling remove method"));
    //     elyir_cron_remove_item(mg_mk_str(elyir_cron_data.c_str()));
    //   // }else if(String(method)=="get"){
    //   //   elyir_cron_get_item(mg_mk_str(data));
    //   }else if(String(method)=="list"){
    //     LOG(LL_INFO, ("Calling list method"));
    //     elyir_cron_list_items();
    //   }

    (void)c;
    (void)topic;
    (void)topic_len;
    (void)msg_len;
    (void)userdata;
}


void elyirOtaClientInit(void)
{
    elyir_add_mqtt_sub(mgos_sys_config_get_mqtt_topic_ota_api_set(), elyir_ota_api_handler, NULL);
}