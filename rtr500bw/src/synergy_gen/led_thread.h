/* generated thread header file - do not edit */
#ifndef LED_THREAD_H_
#define LED_THREAD_H_
#include "bsp_api.h"
#include "tx_api.h"
#include "hal_data.h"
#ifdef __cplusplus
extern "C" void led_thread_entry(void);
#else
extern void led_thread_entry(void);
#endif
#ifdef __cplusplus
extern "C"
{
#endif
extern TX_QUEUE led_queue;
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* LED_THREAD_H_ */
