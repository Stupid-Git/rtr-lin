/* generated thread source file - do not edit */
#include "system_thread.h"

TX_THREAD system_thread;
void system_thread_create(void);
static void system_thread_func(ULONG thread_input);
static uint8_t system_thread_stack[1024] BSP_PLACE_IN_SECTION_V2(".stack.system_thread") BSP_ALIGN_VARIABLE_V2(BSP_STACK_ALIGNMENT);
void tx_startup_err_callback(void *p_instance, void *p_data);
void tx_startup_common_init(void);
#if (2) != BSP_IRQ_DISABLED
#if !defined(SSP_SUPPRESS_ISR_g_timer3) && !defined(SSP_SUPPRESS_ISR_GPT3)
SSP_VECTOR_DEFINE_CHAN(gpt_counter_overflow_isr, GPT, COUNTER_OVERFLOW, 3);
#endif
#endif
static gpt_instance_ctrl_t g_timer3_ctrl;
static const timer_on_gpt_cfg_t g_timer3_extend =
{ .gtioca =
{ .output_enabled = false, .stop_level = GPT_PIN_LEVEL_LOW },
  .gtiocb =
  { .output_enabled = false, .stop_level = GPT_PIN_LEVEL_LOW },
  .shortest_pwm_signal = GPT_SHORTEST_LEVEL_OFF, };
static const timer_cfg_t g_timer3_cfg =
{ .mode = TIMER_MODE_PERIODIC,
  .period = 50,
  .unit = TIMER_UNIT_PERIOD_MSEC,
  .duty_cycle = 50,
  .duty_cycle_unit = TIMER_PWM_UNIT_RAW_COUNTS,
  .channel = 3,
  .autostart = false,
  .p_callback = timer3_callback,
  .p_context = &g_timer3,
  .p_extend = &g_timer3_extend,
  .irq_ipl = (2), };
/* Instance structure to use this module. */
const timer_instance_t g_timer3 =
{ .p_ctrl = &g_timer3_ctrl, .p_cfg = &g_timer3_cfg, .p_api = &g_timer_on_gpt };
#if (BSP_IRQ_DISABLED) != BSP_IRQ_DISABLED
#if !defined(SSP_SUPPRESS_ISR_g_transfer_s1r) && !defined(SSP_SUPPRESS_ISR_DTCELC_EVENT_SPI1_RXI)
#define DTC_ACTIVATION_SRC_ELC_EVENT_SPI1_RXI
#if defined(DTC_ACTIVATION_SRC_ELC_EVENT_ELC_SOFTWARE_EVENT_0) && !defined(DTC_VECTOR_DEFINED_SOFTWARE_EVENT_0)
SSP_VECTOR_DEFINE(elc_software_event_isr, ELC, SOFTWARE_EVENT_0);
#define DTC_VECTOR_DEFINED_SOFTWARE_EVENT_0
#endif
#if defined(DTC_ACTIVATION_SRC_ELC_EVENT_ELC_SOFTWARE_EVENT_1) && !defined(DTC_VECTOR_DEFINED_SOFTWARE_EVENT_1)
SSP_VECTOR_DEFINE(elc_software_event_isr, ELC, SOFTWARE_EVENT_1);
#define DTC_VECTOR_DEFINED_SOFTWARE_EVENT_1
#endif
#endif
#endif

dtc_instance_ctrl_t g_transfer_s1r_ctrl;
transfer_info_t g_transfer_s1r_info =
{ .dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED,
  .repeat_area = TRANSFER_REPEAT_AREA_DESTINATION,
  .irq = TRANSFER_IRQ_END,
  .chain_mode = TRANSFER_CHAIN_MODE_DISABLED,
  .src_addr_mode = TRANSFER_ADDR_MODE_FIXED,
  .size = TRANSFER_SIZE_1_BYTE,
  .mode = TRANSFER_MODE_NORMAL,
  .p_dest = (void *) NULL,
  .p_src = (void const *) NULL,
  .num_blocks = 0,
  .length = 0, };
const transfer_cfg_t g_transfer_s1r_cfg =
{ .p_info = &g_transfer_s1r_info,
  .activation_source = ELC_EVENT_SPI1_RXI,
  .auto_enable = false,
  .p_callback = NULL,
  .p_context = &g_transfer_s1r,
  .irq_ipl = (BSP_IRQ_DISABLED) };
/* Instance structure to use this module. */
const transfer_instance_t g_transfer_s1r =
{ .p_ctrl = &g_transfer_s1r_ctrl, .p_cfg = &g_transfer_s1r_cfg, .p_api = &g_transfer_on_dtc };
#if (BSP_IRQ_DISABLED) != BSP_IRQ_DISABLED
#if !defined(SSP_SUPPRESS_ISR_g_transfer_s1t) && !defined(SSP_SUPPRESS_ISR_DTCELC_EVENT_SPI1_TXI)
#define DTC_ACTIVATION_SRC_ELC_EVENT_SPI1_TXI
#if defined(DTC_ACTIVATION_SRC_ELC_EVENT_ELC_SOFTWARE_EVENT_0) && !defined(DTC_VECTOR_DEFINED_SOFTWARE_EVENT_0)
SSP_VECTOR_DEFINE(elc_software_event_isr, ELC, SOFTWARE_EVENT_0);
#define DTC_VECTOR_DEFINED_SOFTWARE_EVENT_0
#endif
#if defined(DTC_ACTIVATION_SRC_ELC_EVENT_ELC_SOFTWARE_EVENT_1) && !defined(DTC_VECTOR_DEFINED_SOFTWARE_EVENT_1)
SSP_VECTOR_DEFINE(elc_software_event_isr, ELC, SOFTWARE_EVENT_1);
#define DTC_VECTOR_DEFINED_SOFTWARE_EVENT_1
#endif
#endif
#endif

dtc_instance_ctrl_t g_transfer_s1t_ctrl;
transfer_info_t g_transfer_s1t_info =
{ .dest_addr_mode = TRANSFER_ADDR_MODE_FIXED,
  .repeat_area = TRANSFER_REPEAT_AREA_SOURCE,
  .irq = TRANSFER_IRQ_END,
  .chain_mode = TRANSFER_CHAIN_MODE_DISABLED,
  .src_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED,
  .size = TRANSFER_SIZE_1_BYTE,
  .mode = TRANSFER_MODE_NORMAL,
  .p_dest = (void *) NULL,
  .p_src = (void const *) NULL,
  .num_blocks = 0,
  .length = 0, };
const transfer_cfg_t g_transfer_s1t_cfg =
{ .p_info = &g_transfer_s1t_info,
  .activation_source = ELC_EVENT_SPI1_TXI,
  .auto_enable = false,
  .p_callback = NULL,
  .p_context = &g_transfer_s1t,
  .irq_ipl = (BSP_IRQ_DISABLED) };
/* Instance structure to use this module. */
const transfer_instance_t g_transfer_s1t =
{ .p_ctrl = &g_transfer_s1t_ctrl, .p_cfg = &g_transfer_s1t_cfg, .p_api = &g_transfer_on_dtc };
#define RSPI_TRANSFER_SIZE_1_BYTE (0x52535049)
#define RSPI_SYNERGY_NOT_DEFINED 1
#if (RSPI_SYNERGY_NOT_DEFINED != RSPI_TRANSFER_SIZE_1_BYTE)
dtc_instance_ctrl_t g_spi_transfer_tx_ctrl;
uint32_t g_spi_tx_inter = 0;
transfer_info_t g_spi_transfer_tx_info[2] =
{
#if (RSPI_TRANSFER_SIZE_1_BYTE == RSPI_TRANSFER_SIZE_1_BYTE)
  { .dest_addr_mode = TRANSFER_ADDR_MODE_FIXED,
    .repeat_area = TRANSFER_REPEAT_AREA_SOURCE,
    .irq = TRANSFER_IRQ_END,
    .chain_mode = TRANSFER_CHAIN_MODE_EACH,
    .src_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED,
    .size = TRANSFER_SIZE_1_BYTE,
    .mode = TRANSFER_MODE_NORMAL,
    .p_dest = (void *) &g_spi_tx_inter,
    .p_src = (void const *) NULL,
    .num_blocks = 0,
    .length = 0, },
  { .dest_addr_mode = TRANSFER_ADDR_MODE_FIXED,
    .repeat_area = TRANSFER_REPEAT_AREA_SOURCE,
    .irq = TRANSFER_IRQ_END,
    .chain_mode = TRANSFER_CHAIN_MODE_DISABLED,
    .src_addr_mode = TRANSFER_ADDR_MODE_FIXED,
    .size = TRANSFER_SIZE_4_BYTE,
    .mode = TRANSFER_MODE_NORMAL,
    .p_dest = (void *) NULL,
    .p_src = (void const *) &g_spi_tx_inter,
    .num_blocks = 0,
    .length = 0, },
#else
        {
            .dest_addr_mode = TRANSFER_ADDR_MODE_FIXED,
            .repeat_area = TRANSFER_REPEAT_AREA_SOURCE,
            .irq = TRANSFER_IRQ_END,
            .chain_mode = TRANSFER_CHAIN_MODE_DISABLED,
            .src_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED,
            .size =TRANSFER_SIZE_1_BYTE,
            .mode = TRANSFER_MODE_NORMAL,
            .p_dest = (void *) NULL,
            .p_src = (void const *) NULL,
            .num_blocks = 0,
            .length = 0,
        },
        {
            .dest_addr_mode = TRANSFER_ADDR_MODE_FIXED,
            .repeat_area = TRANSFER_REPEAT_AREA_SOURCE,
            .irq = TRANSFER_IRQ_END,
            .chain_mode = TRANSFER_CHAIN_MODE_DISABLED,
            .src_addr_mode = TRANSFER_ADDR_MODE_FIXED,
            .size = TRANSFER_SIZE_4_BYTE,
            .mode = TRANSFER_MODE_NORMAL,
            .p_dest =(void *) NULL,
            .p_src = (void const *) &g_spi_tx_inter,
            .num_blocks = 0,
            .length = 0,
        },
#endif
    };

const transfer_cfg_t g_spi_transfer_tx_cfg =
{ .p_info = g_spi_transfer_tx_info,
  .activation_source = ELC_EVENT_SPI1_TXI,
  .auto_enable = false,
  .p_callback = NULL,
  .p_context = &g_spi_transfer_tx,
  .irq_ipl = (BSP_IRQ_DISABLED) };
/* Instance structure to use this module. */
const transfer_instance_t g_spi_transfer_tx =
{ .p_ctrl = &g_spi_transfer_tx_ctrl, .p_cfg = &g_spi_transfer_tx_cfg, .p_api = &g_transfer_on_dtc };

dtc_instance_ctrl_t g_spi_transfer_rx_ctrl;
uint32_t g_spi_rx_inter = 0;
transfer_info_t g_spi_transfer_rx_info[2] =
{
#if (RSPI_TRANSFER_SIZE_1_BYTE == RSPI_TRANSFER_SIZE_1_BYTE)
  { .dest_addr_mode = TRANSFER_ADDR_MODE_FIXED,
    .repeat_area = TRANSFER_REPEAT_AREA_SOURCE,
    .irq = TRANSFER_IRQ_END,
    .chain_mode = TRANSFER_CHAIN_MODE_EACH,
    .src_addr_mode = TRANSFER_ADDR_MODE_FIXED,
    .size = TRANSFER_SIZE_4_BYTE,
    .mode = TRANSFER_MODE_NORMAL,
    .p_dest = (void *) &g_spi_rx_inter,
    .p_src = (void const *) NULL,
    .num_blocks = 0,
    .length = 0, },
  { .dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED,
    .repeat_area = TRANSFER_REPEAT_AREA_SOURCE,
    .irq = TRANSFER_IRQ_END,
    .chain_mode = TRANSFER_CHAIN_MODE_DISABLED,
    .src_addr_mode = TRANSFER_ADDR_MODE_FIXED,
    .size = TRANSFER_SIZE_1_BYTE,
    .mode = TRANSFER_MODE_NORMAL,
    .p_dest = (void *) NULL,
    .p_src = (void const *) &g_spi_rx_inter,
    .num_blocks = 0,
    .length = 0, },
#else
        {
            .dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED,
            .repeat_area = TRANSFER_REPEAT_AREA_DESTINATION,
            .irq = TRANSFER_IRQ_END,
            .chain_mode = TRANSFER_CHAIN_MODE_DISABLED,
            .src_addr_mode = TRANSFER_ADDR_MODE_FIXED,
            .size = TRANSFER_SIZE_1_BYTE,
            .mode = TRANSFER_MODE_NORMAL,
            .p_dest = (void *) NULL,
            .p_src = (void const *) NULL,
            .num_blocks = 0,
        },
        {
            .dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED,
            .repeat_area = TRANSFER_REPEAT_AREA_SOURCE,
            .irq = TRANSFER_IRQ_END,
            .chain_mode = TRANSFER_CHAIN_MODE_DISABLED,
            .src_addr_mode = TRANSFER_ADDR_MODE_FIXED,
            .size = TRANSFER_SIZE_1_BYTE,
            .mode = TRANSFER_MODE_NORMAL,
            .p_dest = (void *) NULL,
            .p_src = (void const *) &g_spi_rx_inter,
            .num_blocks = 0,
            .length = 0,
        },
#endif
    };

const transfer_cfg_t g_spi_transfer_rx_cfg =
{ .p_info = g_spi_transfer_rx_info,
  .activation_source = ELC_EVENT_SPI1_RXI,
  .auto_enable = false,
  .p_callback = NULL,
  .p_context = &g_spi_transfer_rx,
  .irq_ipl = (BSP_IRQ_DISABLED) };
/* Instance structure to use this module. */
const transfer_instance_t g_spi_transfer_rx =
{ .p_ctrl = &g_spi_transfer_rx_ctrl, .p_cfg = &g_spi_transfer_rx_cfg, .p_api = &g_transfer_on_dtc };
#endif
#undef RSPI_TRANSFER_SIZE_1_BYTE	
#undef RSPI_SYNERGY_NOT_DEFINED

#if !defined(SSP_SUPPRESS_ISR_g_spi) && !defined(SSP_SUPPRESS_ISR_SPI1)
SSP_VECTOR_DEFINE_CHAN(spi_rxi_isr, SPI, RXI, 1);
#endif
#if !defined(SSP_SUPPRESS_ISR_g_spi) && !defined(SSP_SUPPRESS_ISR_SPI1)
SSP_VECTOR_DEFINE_CHAN(spi_txi_isr, SPI, TXI, 1);
#endif
#if !defined(SSP_SUPPRESS_ISR_g_spi) && !defined(SSP_SUPPRESS_ISR_SPI1)
SSP_VECTOR_DEFINE_CHAN(spi_eri_isr, SPI, ERI, 1);
#endif
#if !defined(SSP_SUPPRESS_ISR_g_spi) && !defined(SSP_SUPPRESS_ISR_SPI1)
SSP_VECTOR_DEFINE_CHAN(spi_tei_isr, SPI, TEI, 1);
#endif
rspi_instance_ctrl_t g_spi_ctrl;

/** RSPI extended configuration for RSPI HAL driver */
const spi_on_rspi_cfg_t g_spi_ext_cfg =
{ .rspi_clksyn = RSPI_OPERATION_CLK_SYN,
  /* Communication mode is configured by the driver. write calls use TX_ONLY. read and writeRead use FULL_DUPLEX. */
  .rspi_comm = RSPI_COMMUNICATION_FULL_DUPLEX,
  .ssl_polarity.rspi_ssl0 = RSPI_SSLP_LOW,
  .loopback.rspi_loopback1 = RSPI_LOOPBACK1_NORMAL_DATA,
  .loopback.rspi_loopback2 = RSPI_LOOPBACK2_NORMAL_DATA,
  .mosi_idle.rspi_mosi_idle_fixed_val = RSPI_MOSI_IDLE_FIXED_VAL_LOW,
  .mosi_idle.rspi_mosi_idle_val_fixing = RSPI_MOSI_IDLE_VAL_FIXING_DISABLE,
  .parity.rspi_parity = RSPI_PARITY_STATE_DISABLE,
  .parity.rspi_parity_mode = RSPI_PARITY_MODE_ODD,
  .ssl_select = RSPI_SSL_SELECT_SSL0,
  .ssl_level_keep = RSPI_SSL_LEVEL_KEEP_NOT,
  .clock_delay.rspi_clock_delay_count = RSPI_CLOCK_DELAY_COUNT_1,
  .clock_delay.rspi_clock_delay_state = RSPI_CLOCK_DELAY_STATE_DISABLE,
  .ssl_neg_delay.rspi_ssl_neg_delay_count = RSPI_SSL_NEGATION_DELAY_1,
  .ssl_neg_delay.rspi_ssl_neg_delay_state = RSPI_SSL_NEGATION_DELAY_DISABLE,
  .access_delay.rspi_next_access_delay_count = RSPI_NEXT_ACCESS_DELAY_COUNT_1,
  .access_delay.rspi_next_access_delay_state = RSPI_NEXT_ACCESS_DELAY_STATE_DISABLE,
  .byte_swap = RSPI_BYTE_SWAP_DISABLE, };

const spi_cfg_t g_spi_cfg =
{ .channel = 1, .operating_mode = SPI_MODE_MASTER, .clk_phase = SPI_CLK_PHASE_EDGE_ODD, .clk_polarity =
          SPI_CLK_POLARITY_LOW,
  .mode_fault = SPI_MODE_FAULT_ERROR_DISABLE, .bit_order = SPI_BIT_ORDER_MSB_FIRST, .bitrate = 4000000, .p_transfer_tx =
          g_spi_P_TRANSFER_TX,
  .p_transfer_rx = g_spi_P_TRANSFER_RX, .p_callback = spi_callback, .p_context = (void *) &g_spi, .p_extend =
          (void *) &g_spi_ext_cfg,
  .rxi_ipl = (12), .txi_ipl = (12), .eri_ipl = (12), .tei_ipl = (12), };
/* Instance structure to use this module. */
const spi_instance_t g_spi =
{ .p_ctrl = &g_spi_ctrl, .p_cfg = &g_spi_cfg, .p_api = &g_spi_on_rspi };
#if (2) != BSP_IRQ_DISABLED
#if !defined(SSP_SUPPRESS_ISR_g_timer2) && !defined(SSP_SUPPRESS_ISR_GPT2)
SSP_VECTOR_DEFINE_CHAN(gpt_counter_overflow_isr, GPT, COUNTER_OVERFLOW, 2);
#endif
#endif
static gpt_instance_ctrl_t g_timer2_ctrl;
static const timer_on_gpt_cfg_t g_timer2_extend =
{ .gtioca =
{ .output_enabled = false, .stop_level = GPT_PIN_LEVEL_LOW },
  .gtiocb =
  { .output_enabled = false, .stop_level = GPT_PIN_LEVEL_LOW },
  .shortest_pwm_signal = GPT_SHORTEST_LEVEL_OFF, };
static const timer_cfg_t g_timer2_cfg =
{ .mode = TIMER_MODE_PERIODIC,
  .period = 1,
  .unit = TIMER_UNIT_PERIOD_MSEC,
  .duty_cycle = 50,
  .duty_cycle_unit = TIMER_PWM_UNIT_RAW_COUNTS,
  .channel = 2,
  .autostart = true,
  .p_callback = timer2_callback,
  .p_context = &g_timer2,
  .p_extend = &g_timer2_extend,
  .irq_ipl = (2), };
/* Instance structure to use this module. */
const timer_instance_t g_timer2 =
{ .p_ctrl = &g_timer2_ctrl, .p_cfg = &g_timer2_cfg, .p_api = &g_timer_on_gpt };
#if (3) != BSP_IRQ_DISABLED
#if !defined(SSP_SUPPRESS_ISR_g_timer1) && !defined(SSP_SUPPRESS_ISR_GPT1)
SSP_VECTOR_DEFINE_CHAN(gpt_counter_overflow_isr, GPT, COUNTER_OVERFLOW, 1);
#endif
#endif
static gpt_instance_ctrl_t g_timer1_ctrl;
static const timer_on_gpt_cfg_t g_timer1_extend =
{ .gtioca =
{ .output_enabled = false, .stop_level = GPT_PIN_LEVEL_LOW },
  .gtiocb =
  { .output_enabled = false, .stop_level = GPT_PIN_LEVEL_LOW },
  .shortest_pwm_signal = GPT_SHORTEST_LEVEL_OFF, };
static const timer_cfg_t g_timer1_cfg =
{ .mode = TIMER_MODE_PERIODIC,
  .period = 10,
  .unit = TIMER_UNIT_PERIOD_MSEC,
  .duty_cycle = 50,
  .duty_cycle_unit = TIMER_PWM_UNIT_RAW_COUNTS,
  .channel = 1,
  .autostart = true,
  .p_callback = timer1_callback,
  .p_context = &g_timer1,
  .p_extend = &g_timer1_extend,
  .irq_ipl = (3), };
/* Instance structure to use this module. */
const timer_instance_t g_timer1 =
{ .p_ctrl = &g_timer1_ctrl, .p_cfg = &g_timer1_cfg, .p_api = &g_timer_on_gpt };
#if (3) != BSP_IRQ_DISABLED
#if !defined(SSP_SUPPRESS_ISR_g_timer0) && !defined(SSP_SUPPRESS_ISR_GPT0)
SSP_VECTOR_DEFINE_CHAN(gpt_counter_overflow_isr, GPT, COUNTER_OVERFLOW, 0);
#endif
#endif
static gpt_instance_ctrl_t g_timer0_ctrl;
static const timer_on_gpt_cfg_t g_timer0_extend =
{ .gtioca =
{ .output_enabled = false, .stop_level = GPT_PIN_LEVEL_LOW },
  .gtiocb =
  { .output_enabled = false, .stop_level = GPT_PIN_LEVEL_LOW },
  .shortest_pwm_signal = GPT_SHORTEST_LEVEL_OFF, };
static const timer_cfg_t g_timer0_cfg =
{ .mode = TIMER_MODE_PERIODIC,
  .period = 125,
  .unit = TIMER_UNIT_PERIOD_MSEC,
  .duty_cycle = 50,
  .duty_cycle_unit = TIMER_PWM_UNIT_RAW_COUNTS,
  .channel = 0,
  .autostart = true,
  .p_callback = timer0_callback,
  .p_context = &g_timer0,
  .p_extend = &g_timer0_extend,
  .irq_ipl = (3), };
/* Instance structure to use this module. */
const timer_instance_t g_timer0 =
{ .p_ctrl = &g_timer0_ctrl, .p_cfg = &g_timer0_cfg, .p_api = &g_timer_on_gpt };
#if (BSP_IRQ_DISABLED) != BSP_IRQ_DISABLED
#if !defined(SSP_SUPPRESS_ISR_g_rtc) && !defined(SSP_SUPPRESS_ISR_RTC)
SSP_VECTOR_DEFINE(rtc_alarm_isr, RTC, ALARM);
#endif
#endif
#if (3) != BSP_IRQ_DISABLED
#if !defined(SSP_SUPPRESS_ISR_g_rtc) && !defined(SSP_SUPPRESS_ISR_RTC)
SSP_VECTOR_DEFINE( rtc_period_isr, RTC, PERIOD);
#endif
#endif
#if (3) != BSP_IRQ_DISABLED
#if !defined(SSP_SUPPRESS_ISR_g_rtc) && !defined(SSP_SUPPRESS_ISR_RTC)
SSP_VECTOR_DEFINE( rtc_carry_isr, RTC, CARRY);
#endif
#endif
rtc_instance_ctrl_t g_rtc_ctrl;
const rtc_cfg_t g_rtc_cfg =
{ .clock_source = RTC_CLOCK_SOURCE_SUBCLK,
  .hw_cfg = true,
  .error_adjustment_value = 0,
  .error_adjustment_type = RTC_ERROR_ADJUSTMENT_NONE,
  .p_callback = time_update_callback,
  .p_context = &g_rtc,
  .alarm_ipl = (BSP_IRQ_DISABLED),
  .periodic_ipl = (3),
  .carry_ipl = (3), };
/* Instance structure to use this module. */
const rtc_instance_t g_rtc =
{ .p_ctrl = &g_rtc_ctrl, .p_cfg = &g_rtc_cfg, .p_api = &g_rtc_on_rtc };
TX_EVENT_FLAGS_GROUP g_rtc_event_flags;
TX_MUTEX mutex_crc;
TX_MUTEX mutex_log;
TX_MUTEX mutex_crc32;
TX_MUTEX mutex_sfmem;
TX_EVENT_FLAGS_GROUP g_wmd_event_flags;
TX_EVENT_FLAGS_GROUP event_flags_reboot;
TX_MUTEX mutex_network_init;
extern bool g_ssp_common_initialized;
extern uint32_t g_ssp_common_thread_count;
extern TX_SEMAPHORE g_ssp_common_initialized_semaphore;

void system_thread_create(void)
{
    /* Increment count so we will know the number of ISDE created threads. */
    g_ssp_common_thread_count++;

    /* Initialize each kernel object. */
    UINT err_g_rtc_event_flags;
    err_g_rtc_event_flags = tx_event_flags_create (&g_rtc_event_flags, (CHAR *) "Rtc Event Flags");
    if (TX_SUCCESS != err_g_rtc_event_flags)
    {
        tx_startup_err_callback (&g_rtc_event_flags, 0);
    }
    UINT err_mutex_crc;
    err_mutex_crc = tx_mutex_create (&mutex_crc, (CHAR *) "Crc Mutex", TX_NO_INHERIT);
    if (TX_SUCCESS != err_mutex_crc)
    {
        tx_startup_err_callback (&mutex_crc, 0);
    }
    UINT err_mutex_log;
    err_mutex_log = tx_mutex_create (&mutex_log, (CHAR *) "Log Mutex", TX_NO_INHERIT);
    if (TX_SUCCESS != err_mutex_log)
    {
        tx_startup_err_callback (&mutex_log, 0);
    }
    UINT err_mutex_crc32;
    err_mutex_crc32 = tx_mutex_create (&mutex_crc32, (CHAR *) "Crc32 Mutex", TX_NO_INHERIT);
    if (TX_SUCCESS != err_mutex_crc32)
    {
        tx_startup_err_callback (&mutex_crc32, 0);
    }
    UINT err_mutex_sfmem;
    err_mutex_sfmem = tx_mutex_create (&mutex_sfmem, (CHAR *) "SFM Mutex", TX_NO_INHERIT);
    if (TX_SUCCESS != err_mutex_sfmem)
    {
        tx_startup_err_callback (&mutex_sfmem, 0);
    }
    UINT err_g_wmd_event_flags;
    err_g_wmd_event_flags = tx_event_flags_create (&g_wmd_event_flags, (CHAR *) "Waring Monitor Event Flags");
    if (TX_SUCCESS != err_g_wmd_event_flags)
    {
        tx_startup_err_callback (&g_wmd_event_flags, 0);
    }
    UINT err_event_flags_reboot;
    err_event_flags_reboot = tx_event_flags_create (&event_flags_reboot, (CHAR *) "Reboot Event Flags");
    if (TX_SUCCESS != err_event_flags_reboot)
    {
        tx_startup_err_callback (&event_flags_reboot, 0);
    }
    UINT err_mutex_network_init;
    err_mutex_network_init = tx_mutex_create (&mutex_network_init, (CHAR *) "Network Init Mutex", TX_NO_INHERIT);
    if (TX_SUCCESS != err_mutex_network_init)
    {
        tx_startup_err_callback (&mutex_network_init, 0);
    }

    UINT err;
    err = tx_thread_create (&system_thread, (CHAR *) "System Thread", system_thread_func, (ULONG) NULL,
                            &system_thread_stack, 1024, 1, 1, 1, TX_AUTO_START);
    if (TX_SUCCESS != err)
    {
        tx_startup_err_callback (&system_thread, 0);
    }
}

static void system_thread_func(ULONG thread_input)
{
    /* Not currently using thread_input. */
    SSP_PARAMETER_NOT_USED (thread_input);

    /* Initialize common components */
    tx_startup_common_init ();

    /* Initialize each module instance. */

    /* Enter user code for this thread. */
    system_thread_entry ();
}
