
#ifndef ELYIR_OTA_API_h
#define ELYIR_OTA_API_h

#include "elyir_common.h"
// #ifdef __cplusplus
// extern "C" {
// #endif

void elyir_mgos_ota_http_start(struct update_context *ctx, const char *url);
// void elyir_publish_ota_result();
void elyirOtaClientInit(void);

// #ifdef __cplusplus
// }  // extern "C"
// #endif
#endif