/* generated HAL header file - do not edit */
#ifndef HAL_DATA_H_
#define HAL_DATA_H_
#include <stdint.h>
#include "bsp_api.h"
#include "common_data.h"
#include "r_flash_hp.h"
#include "r_flash_api.h"
#include "r_icu.h"
#include "r_external_irq_api.h"
#include "r_crc.h"
#include "r_crc_api.h"
#ifdef __cplusplus
extern "C"
{
#endif
/* Flash on Flash HP Instance */
extern const flash_instance_t g_flash0;
#ifndef BGO_Callback
void BGO_Callback(flash_callback_args_t *p_args);
#endif
/* External IRQ on ICU Instance. */
extern const external_irq_instance_t g_external_irq6;
#ifndef irq6_callback
void irq6_callback(external_irq_callback_args_t *p_args);
#endif
extern const crc_instance_t g_crc32;
extern const crc_instance_t g_crc;
void hal_entry(void);
void g_hal_init(void);
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* HAL_DATA_H_ */
