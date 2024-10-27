/* generated thread header file - do not edit */
#ifndef SMTP_THREAD_H_
#define SMTP_THREAD_H_
#include "bsp_api.h"
#include "tx_api.h"
#include "hal_data.h"
#ifdef __cplusplus
extern "C" void smtp_thread_entry(void);
#else
extern void smtp_thread_entry(void);
#endif
#ifdef __cplusplus
extern "C"
{
#endif
extern TX_EVENT_FLAGS_GROUP event_flags_smtp;
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* SMTP_THREAD_H_ */
