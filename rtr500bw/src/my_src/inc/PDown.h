/*
 * PDown.h
 *
 *  Created on: 2019/11/07
 *      Author: tooru.hayashi
 */

#ifndef _PDOWN_H_
#define _PDOWN_H_

#ifdef EDF
#undef EDF
#endif

#ifdef _PDOWN_C_
#define EDF
#else
#define EDF extern
#endif

EDF void deep_standby_check(void);


#endif /* _PDOWN_H_ */
