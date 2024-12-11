/*
 * random_stuff.h
 *
 *  Created on: Nov 25, 2024
 *      Author: karel
 */

#ifndef RANDOM_STUFF_H_
#define RANDOM_STUFF_H_

#include "_r500_config.h"

#include <stdint.h>

#include "comp_datetime.h"

#include "r500_defs.h"
#undef EDF
#ifndef RANDOM_STUFF_H_
#define EDF extern
#else
#define EDF
#endif

EDF void system_time_init(void);
EDF void adjust_time(rtc_time_t * p_time);
EDF int  sync_time;


EDF void vhttp_sysset_sndon(void);


#endif /* RANDOM_STUFF_H_ */
