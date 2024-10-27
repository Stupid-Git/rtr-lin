/* generated thread header file - do not edit */
#ifndef OPT_CMD_THREAD_H_
#define OPT_CMD_THREAD_H_
#include "bsp_api.h"
#include "tx_api.h"
#include "hal_data.h"
#ifdef __cplusplus
extern "C" void opt_cmd_thread_entry(void);
#else
extern void opt_cmd_thread_entry(void);
#endif
#include "r_dtc.h"
#include "r_transfer_api.h"
#include "r_sci_uart.h"
#include "r_uart_api.h"
#include "sf_uart_comms.h"
#ifdef __cplusplus
extern "C"
{
#endif
/* Transfer on DTC Instance. */
extern const transfer_instance_t g_transfer_u7r;
#ifndef NULL
void NULL(transfer_callback_args_t *p_args);
#endif
/* Transfer on DTC Instance. */
extern const transfer_instance_t g_transfer_u7t;
#ifndef NULL
void NULL(transfer_callback_args_t *p_args);
#endif
/** UART on SCI Instance. */
extern const uart_instance_t g_uart7;
#ifdef NULL
#else
extern void NULL(uint32_t channel, uint32_t level);
#endif
#ifndef NULL
void NULL(uart_callback_args_t *p_args);
#endif
/* UART Communications Framework Instance. */
extern const sf_comms_instance_t g_sf_comms7;
void g_sf_comms7_err_callback(void *p_instance, void *p_data);
void sf_comms_init7(void);
extern TX_EVENT_FLAGS_GROUP optc_event_flags;
extern TX_MUTEX optc_mutex;
extern TX_QUEUE optc_queue;
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* OPT_CMD_THREAD_H_ */
