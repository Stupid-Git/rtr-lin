/* generated thread header file - do not edit */
#ifndef EVENT_THREAD_H_
#define EVENT_THREAD_H_
#include "bsp_api.h"
#include "tx_api.h"
#include "hal_data.h"
#ifdef __cplusplus
extern "C" void event_thread_entry(void);
#else
extern void event_thread_entry(void);
#endif
#ifdef __cplusplus
extern "C"
{
#endif
extern TX_EVENT_FLAGS_GROUP event_flags_cycle;
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* EVENT_THREAD_H_ */
