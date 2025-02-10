/*
 * comp_acm0_thread.h
 *
 *  Created on: Dec 2, 2024
 *      Author: karel
 */

#ifndef COMP_ACM0_THREAD_H_
#define COMP_ACM0_THREAD_H_

#include "_r500_config.h"

#include <stdint.h>

#ifdef EDF
#undef EDF
#endif

#ifdef _COMP_ACM0_THREAD_C_
#define EDF
#else
#define EDF extern
#endif



#endif /* COMP_ACM0_THREAD_H_ */
