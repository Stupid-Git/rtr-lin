#ifndef _NEW_G_H_
#define _NEW_G_H_

//#include "_r500_config.h"     // 2023.05.26
#if CONFIG_NEW_STATE_CTRL

#ifdef  EDF
#undef  EDF
#endif

#ifdef  _NEW_G_C_
#define EDF
#else
#define EDF extern
#endif

typedef enum {
    NSC_STAGE_0,
} NSC_STAGE_t;

EDF NSC_STAGE_t nsc_stage;

void nsc_all(void);
void nsc_dump_auto(void);
// 2023.05.31
void kk_delta_clear_all(void);
void kk_delta_clear(uint32_t serial_number);
void kk_delta_set(uint32_t serial_number, uint8_t flagX);
bool kk_delta_get(uint32_t serial_number, uint8_t* p_flagX);
void kk_delta_set_all(uint8_t flagX);

#endif
#endif /* _NEW_G_H_ */
