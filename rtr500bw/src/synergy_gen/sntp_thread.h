/* generated thread header file - do not edit */
#ifndef SNTP_THREAD_H_
#define SNTP_THREAD_H_
#include "bsp_api.h"
#include "tx_api.h"
#include "hal_data.h"
#ifdef __cplusplus
extern "C" void sntp_thread_entry(void);
#else
extern void sntp_thread_entry(void);
#endif
#ifdef __cplusplus
extern "C"
{
#endif
extern TX_EVENT_FLAGS_GROUP event_flags_sntp;
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* SNTP_THREAD_H_ */
