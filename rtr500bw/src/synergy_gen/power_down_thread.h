/* generated thread header file - do not edit */
#ifndef POWER_DOWN_THREAD_H_
#define POWER_DOWN_THREAD_H_
#include "bsp_api.h"
#include "tx_api.h"
#include "hal_data.h"
#ifdef __cplusplus
extern "C" void power_down_thread_entry(void);
#else
extern void power_down_thread_entry(void);
#endif
#include "r_lvd.h"
#include "r_lvd_api.h"
#include "sf_power_profiles_v2.h"
#ifdef __cplusplus
extern "C"
{
#endif
/**
 * LVD Instance
 */

extern const lvd_instance_t g_lvd1;
#ifndef lvd1_callback
void lvd1_callback(lvd_callback_args_t *p_args);
#endif

/**
 * End LVD Instance
 */
#ifdef g_bsp_pd_pin_cfg
#define POWER_PROFILES_V2_ENTER_PIN_CFG_TBL_USED_g_sf_power_profiles_v2_low_power (0)
#else
#define POWER_PROFILES_V2_ENTER_PIN_CFG_TBL_USED_g_sf_power_profiles_v2_low_power (1)
#endif
#if POWER_PROFILES_V2_ENTER_PIN_CFG_TBL_USED_g_sf_power_profiles_v2_low_power
extern const ioport_cfg_t g_bsp_pd_pin_cfg;
#endif
#ifdef NULL
#define POWER_PROFILES_V2_EXIT_PIN_CFG_TBL_USED_g_sf_power_profiles_v2_low_power (0)
#else
#define POWER_PROFILES_V2_EXIT_PIN_CFG_TBL_USED_g_sf_power_profiles_v2_low_power (1)
#endif
#if POWER_PROFILES_V2_EXIT_PIN_CFG_TBL_USED_g_sf_power_profiles_v2_low_power
extern const ioport_cfg_t NULL;
#endif
#ifdef NULL
#define POWER_PROFILES_V2_CALLBACK_USED_g_sf_power_profiles_v2_low_power (0)
#else
#define POWER_PROFILES_V2_CALLBACK_USED_g_sf_power_profiles_v2_low_power (1)
#endif
#if POWER_PROFILES_V2_CALLBACK_USED_g_sf_power_profiles_v2_low_power
void NULL(sf_power_profiles_v2_callback_args_t *p_args);
#endif
/** Power Profiles run structure */
extern sf_power_profiles_v2_low_power_cfg_t g_sf_power_profiles_v2_low_power;
extern TX_EVENT_FLAGS_GROUP g_power_down_event_flags;
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* POWER_DOWN_THREAD_H_ */
