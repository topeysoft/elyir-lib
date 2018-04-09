#include "mgos_timers.h"
#include "mgos_wifi.h"
#include "mgos_net.h"
#include "mgos.h"
#include "mgos_system.h"
#include "elyir_common.h"
#include "elyir_wifi.h"

bool ElyirWifi_net_conn_flag = false;
mgos_timer_id ElyirWifi_config_mode_timer_id;

void ElyirWifi_begin(void){
  mgos_event_add_group_handler(MGOS_EVENT_GRP_NET, ElyirWifi_wifi_event_handler, NULL);
}
void ElyirWifi_wifi_timer_callback(void *arg){
    mgos_clear_timer(ElyirWifi_config_mode_timer_id);
    mgos_sys_config_set_wifi_ap_enable(true);
    mgos_sys_config_set_wifi_ap_hidden(false);
    mgos_system_restart();
  (void)arg;
}
void ElyirWifi_onConnected(void){
    mgos_clear_timer(ElyirWifi_config_mode_timer_id);
}
void ElyirWifi_onDisconnected(void){
    if(!mgos_sys_config_get_wifi_ap_enable()){
        ElyirWifi_config_mode_timer_id = mgos_set_timer(
      mgos_sys_config_get_device_wifi_fail_time(), false, ElyirWifi_wifi_timer_callback,
      NULL);
    }
}

void ElyirWifi_wifi_event_handler (int ev,
                    void *ev_data, void *arg)
{
  switch (ev)
  {

  case MGOS_NET_EV_DISCONNECTED:
    ElyirWifi_net_conn_flag = false;
    LOG(LL_INFO, ("NET Disconnected"));
    ElyirWifi_onConnected();
    break;
  case MGOS_NET_EV_IP_ACQUIRED:
    if (is_sta_ip())
    {
      ElyirWifi_net_conn_flag = true;
      LOG(LL_INFO, ("NET Connected"));
      ElyirWifi_onDisconnected();
    }
    break;
  default:
    break;
  }
  (void)arg;
  (void)ev_data;
}
