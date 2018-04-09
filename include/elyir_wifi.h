#ifndef ElyirWifi_H
#define ElyirWifi_H

#include "mgos_timers.h"

void ElyirWifi_begin(void);
        void ElyirWifi_onConnected(void);
        void ElyirWifi_onDisconnected(void);
        void ElyirWifi_wifi_event_handler(int ev,
                    void *ev_data, void *arg);
        void ElyirWifi_wifi_timer_callback(void *arg);
// class ElyirWifi{
//     public:
//         void begin(void);
//         // ElyirWifi(void);
//     protected:
//         void _onConnected(void);
//         void _onDisconnected(void);
//         void _wifi_event_handler(int ev,
//                     void *ev_data, void *arg);
//     private:             
//         mgos_timer_id _config_mode_timer_id;
//         timer_callback _wifi_timer_callback(void *arg);
// };

#endif