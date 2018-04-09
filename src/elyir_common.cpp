#include "mgos_pwm.h"
#include "mgos_wifi.h"
#include "mgos_net.h"

void analog_write(int pin, float value)
{
  mgos_pwm_set(pin, 10000 , value);
}

bool is_sta_ip()
{
  struct mgos_net_ip_info ip_info;
  memset(&ip_info, 0, sizeof(ip_info));
        return mgos_net_get_ip_info(MGOS_NET_IF_TYPE_WIFI, MGOS_NET_IF_WIFI_STA,
                                &ip_info);
}