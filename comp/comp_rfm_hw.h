/*
 * comp_rfm_hw.h
 *
 *  Created on: Dec 3, 2024
 *      Author: karel
 */

#ifndef COMP_RFM_HW_H_
#define COMP_RFM_HW_H_

#include "_r500_config.h"

#include <stdint.h>
#include "r500_defs.h"

#ifdef EDF
#undef EDF
#endif
#ifndef _COMP_RFM_HW_C_
#define EDF extern
#else
#define EDF
#endif

EDF void hw_RF_CS_ACTIVE(void);
EDF void hw_RF_CS_INACTIVE(void);
EDF void hw_RF_RESET_ACTIVE(void);
EDF void hw_RF_RESET_INACTIVE(void);

EDF void hw_rfm_reset(void);                                           // 無線モジュール リセット
EDF void hw_rfm_initialize(void);                                      // 無線モジュール 初期化
EDF void hw_rfm_send(char *pData);
EDF uint8_t hw_rfm_recv(uint32_t);                                 // 無線モジュール 受信

EDF int hw_Chreck_RFM_Busy(void);      //RFMのビジーチェック
EDF int hw_Chreck_RFM_Status(void);    //無線モジュールのステータスをチェックする


#endif /* COMP_RFM_HW_H_ */
