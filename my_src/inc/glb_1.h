/*
 * glb_1.h
 *
 *  Created on: Dec 2, 2024
 *      Author: karel
 */

#ifndef GLB_1_H_
#define GLB_1_H_

#include "_r500_config.h"

#include <stdint.h>


#ifdef EDF
#undef EDF
#endif
#ifdef GLB_1_C_
#define EDF
#else
#define EDF extern
#endif


#include "file_structs.h"

extern  my_config_t my_config;

extern fact_config_t fact_config;


#endif /* GLB_1_H_ */
