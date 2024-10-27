/**
 * @file    tcp_thread_entry.h
 *
 * @date    2019/07/03
 * @author  tooru.hayashi
 */

//#include "tcp_thread.h"

#ifndef _TCP_THREAD_ENTRY_H_
#define _TCP_THREAD_ENTRY_H_




#include <stdio.h>

#include "flag_def.h"
#include "MyDefine.h"
#include "Config.h"
#include "Globals.h"


#ifdef EDF
#undef EDF
#endif

#ifndef _TCP_THREAD_ENTRY_C_
    #define EDF extern
#else
    #define EDF
#endif



#endif /* _TCP_THREAD_ENTRY_H_ */
