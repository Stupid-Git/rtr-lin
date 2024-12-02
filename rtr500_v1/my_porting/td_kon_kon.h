/*
 * td_kon_kon.h
 *
 *  Created on: Nov 25, 2024
 *      Author: karel
 */

#ifndef TD_KON_KON_H_
#define TD_KON_KON_H_


#include "_r500_config.h"

#if CONFIG_USE_TD_KON_KON

#include "stdio.h" // printf
#include "stdint.h"
#include "stdbool.h"

#undef EDF

#ifdef _TD_KON_KON_C_
#define EDF
#else
#define EDF extern
#endif


inline void Printf_StartUp(void) __attribute__((always_inline));
inline void Printf_StartUp(void)
{
}

inline void Printf_Shutdown (void) __attribute__((always_inline));
inline void Printf_Shutdown (void)
{
}

//void Printf(const char *fmt, ...);
#define Printf printf

/*EDITING*/
#define TD_NORM  0
#define TD_EVENT 1
#define TD_INFO  2
#define TD_DEBUG 3
#define TD_WARN  4
#define TD_ERROR 5
#define TD_PINK  6

int PrintDbg(int TYPE, const char *format, ...);

void PrintHex( char* banner, uint8_t* pData, int len);


#endif // CONFIG_USE_TD_KON_KON


#endif /* TD_KON_KON_H_ */
