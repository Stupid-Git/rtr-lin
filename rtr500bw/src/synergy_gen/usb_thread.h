/* generated thread header file - do not edit */
#ifndef USB_THREAD_H_
#define USB_THREAD_H_
#include "bsp_api.h"
#include "tx_api.h"
#include "hal_data.h"
#ifdef __cplusplus
extern "C" void usb_thread_entry(void);
#else
extern void usb_thread_entry(void);
#endif
#ifdef __cplusplus
extern "C"
{
#endif
extern TX_SEMAPHORE g_cdc_activate_semaphore0;
extern TX_EVENT_FLAGS_GROUP event_flags_usb;
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* USB_THREAD_H_ */
