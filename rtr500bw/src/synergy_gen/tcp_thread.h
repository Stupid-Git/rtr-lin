/* generated thread header file - do not edit */
#ifndef TCP_THREAD_H_
#define TCP_THREAD_H_
#include "bsp_api.h"
#include "tx_api.h"
#include "hal_data.h"
#ifdef __cplusplus
extern "C" void tcp_thread_entry(void);
#else
extern void tcp_thread_entry(void);
#endif
#ifdef __cplusplus
extern "C"
{
#endif
extern TX_EVENT_FLAGS_GROUP g_tcp_event_flags;
extern TX_MUTEX mutex_tcp_recv;
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* TCP_THREAD_H_ */
