/* generated thread header file - do not edit */
#ifndef NETWORK_THREAD_H_
#define NETWORK_THREAD_H_
#include "bsp_api.h"
#include "tx_api.h"
#include "hal_data.h"
#ifdef __cplusplus
extern "C" void network_thread_entry(void);
#else
extern void network_thread_entry(void);
#endif
#ifdef __cplusplus
extern "C"
{
#endif
extern TX_EVENT_FLAGS_GROUP event_flags_network;
extern TX_EVENT_FLAGS_GROUP event_flags_net_status;
extern TX_EVENT_FLAGS_GROUP event_flags_link;
extern TX_EVENT_FLAGS_GROUP event_flags_wifi;
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* NETWORK_THREAD_H_ */
