/* generated HAL source file - do not edit */
#include "hal_data.h"
#if (12) != BSP_IRQ_DISABLED
#if !defined(SSP_SUPPRESS_ISR_g_flash0) && !defined(SSP_SUPPRESS_ISR_FLASH)
SSP_VECTOR_DEFINE( fcu_frdyi_isr, FCU, FRDYI);
#endif
#endif
#if (12) != BSP_IRQ_DISABLED
#if !defined(SSP_SUPPRESS_ISR_g_flash0) && !defined(SSP_SUPPRESS_ISR_FLASH)
SSP_VECTOR_DEFINE( fcu_fiferr_isr, FCU, FIFERR);
#endif
#endif
flash_hp_instance_ctrl_t g_flash0_ctrl;
const flash_cfg_t g_flash0_cfg =
{ .data_flash_bgo = true, .p_callback = BGO_Callback, .p_context = &g_flash0, .irq_ipl = (12), .err_irq_ipl = (12), };
/* Instance structure to use this module. */
const flash_instance_t g_flash0 =
{ .p_ctrl = &g_flash0_ctrl, .p_cfg = &g_flash0_cfg, .p_api = &g_flash_on_flash_hp };
#if (1) != BSP_IRQ_DISABLED
#if !defined(SSP_SUPPRESS_ISR_g_external_irq6) && !defined(SSP_SUPPRESS_ISR_ICU6)
SSP_VECTOR_DEFINE( icu_irq_isr, ICU, IRQ6);
#endif
#endif
static icu_instance_ctrl_t g_external_irq6_ctrl;
static const external_irq_cfg_t g_external_irq6_cfg =
{ .channel = 6,
  .trigger = EXTERNAL_IRQ_TRIG_FALLING,
  .filter_enable = true,
  .pclk_div = EXTERNAL_IRQ_PCLK_DIV_BY_64,
  .autostart = true,
  .p_callback = irq6_callback,
  .p_context = &g_external_irq6,
  .p_extend = NULL,
  .irq_ipl = (1), };
/* Instance structure to use this module. */
const external_irq_instance_t g_external_irq6 =
{ .p_ctrl = &g_external_irq6_ctrl, .p_cfg = &g_external_irq6_cfg, .p_api = &g_external_irq_on_icu };
static crc_instance_ctrl_t g_crc32_ctrl;
const crc_cfg_t g_crc32_cfg =
{ .polynomial = CRC_POLYNOMIAL_CRC_32C, .bit_order = CRC_BIT_ORDER_LMS_MSB, .fifo_mode = false, };
/* Instance structure to use this module. */
const crc_instance_t g_crc32 =
{ .p_ctrl = &g_crc32_ctrl, .p_cfg = &g_crc32_cfg, .p_api = &g_crc_on_crc };
static crc_instance_ctrl_t g_crc_ctrl;
const crc_cfg_t g_crc_cfg =
{ .polynomial = CRC_POLYNOMIAL_CRC_CCITT, .bit_order = CRC_BIT_ORDER_LMS_MSB, .fifo_mode = false, };
/* Instance structure to use this module. */
const crc_instance_t g_crc =
{ .p_ctrl = &g_crc_ctrl, .p_cfg = &g_crc_cfg, .p_api = &g_crc_on_crc };
void g_hal_init(void)
{
    g_common_init ();
}
