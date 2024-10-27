/*
 * RfCmd_func.h
 *
 *  Created on: 2020/02/07
 *      Author: tooru.hayashi
 */

#ifndef _RFCMD_FUNC_H_
#define _RFCMD_FUNC_H_

#ifdef EDF
#undef EDF
#endif

#ifdef _RFCMD_FUNC_C_
#define EDF
#else
#define EDF extern
#endif

EDF uint32_t start_rftask(uint8_t cmd);


//EDF uint8_t Rec_StartTime_set(uint32_t Secs);					// [EWSTW]にUTCが含まれていた場合、指定時刻で記録開始する

EDF void RegistTableWrite(void);

//EDF uint8_t RegistTableEWSTR(void);
EDF void RegistTableBleWrite(void);
EDF void RegistTableSlWrite(void);

#endif /* _RFCMD_FUNC_H_ */
