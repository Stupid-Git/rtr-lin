/*
 * comp_gos.h
 *
 *  Created on: Dec 2, 2024
 *      Author: karel
 */

#ifndef COMP_GOS_H_
#define COMP_GOS_H_

#include "_r500_config.h"


#ifdef EDF
#undef EDF
#endif
#ifdef _COMP_GOS_C_
#define EDF
#else
#define EDF extern
#endif

#include "tdx.h"

EDF tdx_flags_t optc_event_flags;
EDF tdx_queue_t optc_queue;

EDF tdx_flags_t event_flags_reboot;

EDF tdx_flags_t event_flags_http;

EDF pthread_mutex_t mutex_network_init;
EDF pthread_mutex_t mutex_crc;



int comp_gos_init(void);


#endif /* COMP_GOS_H_ */
