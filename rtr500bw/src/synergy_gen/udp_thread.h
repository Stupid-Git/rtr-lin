/* generated thread header file - do not edit */
#ifndef UDP_THREAD_H_
#define UDP_THREAD_H_
#include "bsp_api.h"
#include "tx_api.h"
#include "hal_data.h"
#ifdef __cplusplus
extern "C" void udp_thread_entry(void);
#else
extern void udp_thread_entry(void);
#endif
#ifdef __cplusplus
extern "C"
{
#endif
extern TX_QUEUE g_msg_udp_queue;
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* UDP_THREAD_H_ */
