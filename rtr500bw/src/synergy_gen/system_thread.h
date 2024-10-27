/* generated thread header file - do not edit */
#ifndef SYSTEM_THREAD_H_
#define SYSTEM_THREAD_H_
#include "bsp_api.h"
#include "tx_api.h"
#include "hal_data.h"
#ifdef __cplusplus
extern "C" void system_thread_entry(void);
#else
extern void system_thread_entry(void);
#endif
#include "r_gpt.h"
#include "r_timer_api.h"
#include "r_dtc.h"
#include "r_transfer_api.h"
#include "r_rspi.h"
#include "r_spi_api.h"
#include "r_rtc.h"
#include "r_rtc_api.h"
#ifdef __cplusplus
extern "C"
{
#endif
/** Timer on GPT Instance. */
extern const timer_instance_t g_timer3;
#ifndef timer3_callback
void timer3_callback(timer_callback_args_t *p_args);
#endif
/* Transfer on DTC Instance. */
extern const transfer_instance_t g_transfer_s1r;
#ifndef NULL
void NULL(transfer_callback_args_t *p_args);
#endif
/* Transfer on DTC Instance. */
extern const transfer_instance_t g_transfer_s1t;
#ifndef NULL
void NULL(transfer_callback_args_t *p_args);
#endif
/** SPI config */
extern const spi_cfg_t g_spi_cfg;
/** RSPI extended config */
extern const spi_on_rspi_cfg_t g_spi_ext_cfg;
/** SPI on RSPI Instance. */
extern const spi_instance_t g_spi;
/** SPI instance control */
extern rspi_instance_ctrl_t g_spi_ctrl;
#ifndef spi_callback
void spi_callback(spi_callback_args_t *p_args);
#endif

extern const transfer_instance_t g_spi_transfer_rx;
extern const transfer_instance_t g_spi_transfer_tx;

#define SYNERGY_NOT_DEFINED (1)
#define RSPI_TRANSFER_SIZE_1_BYTE (0x52535049)

#if (SYNERGY_NOT_DEFINED == g_transfer_s1t)
#define g_spi_P_TRANSFER_TX (NULL)
#else
#define g_spi_P_TRANSFER_TX (&g_spi_transfer_tx)
#endif
#if (SYNERGY_NOT_DEFINED == g_transfer_s1r)
#define g_spi_P_TRANSFER_RX (NULL)
#else
#define g_spi_P_TRANSFER_RX (&g_spi_transfer_rx)
#endif

#undef RSPI_TRANSFER_SIZE_1_BYTE
#undef SYNERGY_NOT_DEFINED
#define g_spi_P_EXTEND (&g_spi_ext_cfg)
/** Timer on GPT Instance. */
extern const timer_instance_t g_timer2;
#ifndef timer2_callback
void timer2_callback(timer_callback_args_t *p_args);
#endif
/** Timer on GPT Instance. */
extern const timer_instance_t g_timer1;
#ifndef timer1_callback
void timer1_callback(timer_callback_args_t *p_args);
#endif
/** Timer on GPT Instance. */
extern const timer_instance_t g_timer0;
#ifndef timer0_callback
void timer0_callback(timer_callback_args_t *p_args);
#endif
/** RTC on RTC Instance. */
extern const rtc_instance_t g_rtc;
#ifndef time_update_callback
void time_update_callback(rtc_callback_args_t *p_args);
#endif
extern TX_EVENT_FLAGS_GROUP g_rtc_event_flags;
extern TX_MUTEX mutex_crc;
extern TX_MUTEX mutex_log;
extern TX_MUTEX mutex_crc32;
extern TX_MUTEX mutex_sfmem;
extern TX_EVENT_FLAGS_GROUP g_wmd_event_flags;
extern TX_EVENT_FLAGS_GROUP event_flags_reboot;
extern TX_MUTEX mutex_network_init;
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* SYSTEM_THREAD_H_ */
