/*
 * system_time.h
 *
 *  Created on: 2018/10/19
 *      Author: haya
 */

#ifndef _SYSTEM_TIME_H_
#define _SYSTEM_TIME_H_




/***********************************************************************************************************************
Includes
***********************************************************************************************************************/
#include "bsp_api.h"
#include "tx_api.h"

#include "hal_data.h"
#include "r_rtc_api.h"
#include <stdio.h>





#ifdef EDF
#undef EDF
#endif

#ifdef _SYSTEM_TIME_C_
#define EDF
#else
#define EDF extern
#endif




/***********************************************************************************************************************
Function prototypes
**********************************************************************************************************************/
EDF void system_time_init(void);
EDF void adjust_time(rtc_time_t * p_time);


/***********************************************************************************************************************
Global variables
***********************************************************************************************************************/
//extern volatile bool g_time_updated;
//extern volatile bool test_port;

EDF int  sync_time;

#endif /* _SYSTEM_TIME_H_ */
