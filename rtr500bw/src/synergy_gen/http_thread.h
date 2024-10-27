/* generated thread header file - do not edit */
#ifndef HTTP_THREAD_H_
#define HTTP_THREAD_H_
#include "bsp_api.h"
#include "tx_api.h"
#include "hal_data.h"
#ifdef __cplusplus
extern "C" void http_thread_entry(void);
#else
extern void http_thread_entry(void);
#endif
#include "nx_web_http_client.h"
#ifdef __cplusplus
extern "C"
{
#endif
extern NX_WEB_HTTP_CLIENT g_web_http_client0;
void g_web_http_client0_err_callback(void *p_instance, void *p_data);
void web_http_client_init0(void);
extern TX_EVENT_FLAGS_GROUP event_flags_http;
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* HTTP_THREAD_H_ */
