/* generated thread header file - do not edit */
#ifndef BLE_THREAD_H_
#define BLE_THREAD_H_
#include "bsp_api.h"
#include "tx_api.h"
#include "hal_data.h"
#ifdef __cplusplus
extern "C" void ble_thread_entry(void);
#else
extern void ble_thread_entry(void);
#endif
#include "r_icu.h"
#include "r_external_irq_api.h"
#include "r_dtc.h"
#include "r_transfer_api.h"
#include "r_sci_uart.h"
#include "r_uart_api.h"
#include "sf_uart_comms.h"
#ifdef __cplusplus
extern "C"
{
#endif
/* External IRQ on ICU Instance. */
extern const external_irq_instance_t g_external_irq2;
#ifndef BLE_break_callback
void BLE_break_callback(external_irq_callback_args_t *p_args);
#endif
/* Transfer on DTC Instance. */
extern const transfer_instance_t g_transfer_u4r;
#ifndef NULL
void NULL(transfer_callback_args_t *p_args);
#endif
/* Transfer on DTC Instance. */
extern const transfer_instance_t g_transfer_u4t;
#ifndef NULL
void NULL(transfer_callback_args_t *p_args);
#endif
/** UART on SCI Instance. */
extern const uart_instance_t g_uart4;
#ifdef NULL
#else
extern void NULL(uint32_t channel, uint32_t level);
#endif
#ifndef NULL
void NULL(uart_callback_args_t *p_args);
#endif
/* UART Communications Framework Instance. */
extern const sf_comms_instance_t g_sf_comms4;
void g_sf_comms4_err_callback(void *p_instance, void *p_data);
void sf_comms_init4(void);
extern TX_EVENT_FLAGS_GROUP g_ble_event_flags;
extern TX_QUEUE g_ble_usb_queue;
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* BLE_THREAD_H_ */
