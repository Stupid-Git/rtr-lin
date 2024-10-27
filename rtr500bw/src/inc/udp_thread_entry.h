/**
 * @file    udp_thread_entry.h
 *
 * @date    2019/07/02
 * @author  tooru.hayashi
 */

#ifndef _UDP_THREAD_ENTRY_H_
#define _UDP_THREAD_ENTRY_H_



#include <stdio.h>

#include "MyDefine.h"
#include "Config.h"
#include "Globals.h"


#ifdef EDF
#undef EDF
#endif

#ifndef _UDP_THREAD_ENTRY_C_
    #define EDF extern
#else
    #define EDF
#endif

EDF void Udp_Resp_Set(void);


#endif /* _UDP_THREAD_ENTRY_H_ */
