/* generated thread source file - do not edit */
#include "power_down_thread.h"

TX_THREAD power_down_thread;
void power_down_thread_create(void);
static void power_down_thread_func(ULONG thread_input);
static uint8_t power_down_thread_stack[1024] BSP_PLACE_IN_SECTION_V2(".stack.power_down_thread") BSP_ALIGN_VARIABLE_V2(BSP_STACK_ALIGNMENT);
void tx_startup_err_callback(void *p_instance, void *p_data);
void tx_startup_common_init(void);
/**
 * LVD Instance
 */

#if (2) != BSP_IRQ_DISABLED
#if !defined(SSP_SUPPRESS_ISR_g_lvd1) && !defined(SSP_SUPPRESS_ISR_LVD1)
SSP_VECTOR_DEFINE( lvd_lvd_isr, LVD, LVD1);
#endif
#endif

/* Extension structure for mcu specific configuration for g_lvd1 */
static const lvd_extend_t g_lvd1_extend =
{ .negation_delay = LVD_NEGATION_DELAY_FROM_VOLTAGE, .sample_clock_divisor = LVD_SAMPLE_CLOCK_DISABLED, };

/* Configuration structure for g_lvd1 */
const lvd_cfg_t g_lvd1_cfg =
{
/** LVD settings */
.monitor_number = 1,
  .voltage_threshold = LVD_THRESHOLD_MONITOR_1_LEVEL_13, .detection_response = LVD_RESPONSE_INTERRUPT, .voltage_slope =
          LVD_VOLTAGE_SLOPE_FALLING,
  .p_callback = lvd1_callback, .p_context = &g_lvd1, .p_extend = &g_lvd1_extend, .monitor_ipl = (2), };

/* Control structure for g_lvd1 */
lvd_instance_ctrl_t g_lvd1_ctrl;

const lvd_instance_t g_lvd1 =
{ .p_api = &g_lvd_on_lvd, .p_cfg = &g_lvd1_cfg, .p_ctrl = &g_lvd1_ctrl, };

/**
 * End LVD Instance
 */
/************************************************************/
/** Begin PPM V2 Low Power Profile **************************/
/************************************************************/
#if POWER_PROFILES_V2_CALLBACK_USED_g_sf_power_profiles_v2_low_power
void NULL(sf_power_profiles_v2_callback_args_t * p_args);
#endif
sf_power_profiles_v2_low_power_cfg_t g_sf_power_profiles_v2_low_power =
{
#if POWER_PROFILES_V2_EXIT_PIN_CFG_TBL_USED_g_sf_power_profiles_v2_low_power
  .p_ioport_pin_tbl_exit = &NULL,
#else
  .p_ioport_pin_tbl_exit = NULL,
#endif
#if POWER_PROFILES_V2_ENTER_PIN_CFG_TBL_USED_g_sf_power_profiles_v2_low_power
  .p_ioport_pin_tbl_enter = &g_bsp_pd_pin_cfg,
#else
  .p_ioport_pin_tbl_enter = NULL,
#endif
  .p_lower_lvl_lpm = &g_lpmv2_deep_standby,
  .p_callback = NULL, .p_context = &g_sf_power_profiles_v2_low_power, .p_extend = NULL, };
/************************************************************/
/** End PPM V2 Low Power Profile ****************************/
/************************************************************/
TX_EVENT_FLAGS_GROUP g_power_down_event_flags;
extern bool g_ssp_common_initialized;
extern uint32_t g_ssp_common_thread_count;
extern TX_SEMAPHORE g_ssp_common_initialized_semaphore;

void power_down_thread_create(void)
{
    /* Increment count so we will know the number of ISDE created threads. */
    g_ssp_common_thread_count++;

    /* Initialize each kernel object. */
    UINT err_g_power_down_event_flags;
    err_g_power_down_event_flags = tx_event_flags_create (&g_power_down_event_flags, (CHAR *) "Power Down Event Flags");
    if (TX_SUCCESS != err_g_power_down_event_flags)
    {
        tx_startup_err_callback (&g_power_down_event_flags, 0);
    }

    UINT err;
    err = tx_thread_create (&power_down_thread, (CHAR *) "Power Down Thread", power_down_thread_func, (ULONG) NULL,
                            &power_down_thread_stack, 1024, 1, 1, 1, TX_AUTO_START);
    if (TX_SUCCESS != err)
    {
        tx_startup_err_callback (&power_down_thread, 0);
    }
}

static void power_down_thread_func(ULONG thread_input)
{
    /* Not currently using thread_input. */
    SSP_PARAMETER_NOT_USED (thread_input);

    /* Initialize common components */
    tx_startup_common_init ();

    /* Initialize each module instance. */

    /* Enter user code for this thread. */
    power_down_thread_entry ();
}
