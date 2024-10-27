/* generated thread header file - do not edit */
#ifndef CMD_THREAD_H_
#define CMD_THREAD_H_
#include "bsp_api.h"
#include "tx_api.h"
#include "hal_data.h"
#ifdef __cplusplus
extern "C" void cmd_thread_entry(void);
#else
extern void cmd_thread_entry(void);
#endif
#ifdef __cplusplus
extern "C"
{
#endif
extern TX_EVENT_FLAGS_GROUP g_command_event_flags;
extern TX_MUTEX mutex_cmd;
extern TX_MUTEX mutex_tcp_cmd;
extern TX_QUEUE cmd_queue;
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* CMD_THREAD_H_ */
