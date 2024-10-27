/* generated thread header file - do not edit */
#ifndef USB_TX_THREAD_H_
#define USB_TX_THREAD_H_
#include "bsp_api.h"
#include "tx_api.h"
#include "hal_data.h"
#ifdef __cplusplus
extern "C" void usb_tx_thread_entry(void);
#else
extern void usb_tx_thread_entry(void);
#endif
#ifdef __cplusplus
extern "C"
{
#endif
extern TX_QUEUE g_usb_tx_queue;
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* USB_TX_THREAD_H_ */
