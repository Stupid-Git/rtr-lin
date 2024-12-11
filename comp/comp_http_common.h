/*
 * comp_http_common.h
 *
 *  Created on: Dec 9, 2024
 *      Author: karel
 */

#ifndef COMP_HTTP_COMMON_H_
#define COMP_HTTP_COMMON_H_


#ifdef EDF
#undef EDF
#endif

#ifdef _COMP_HTTP_COMMON_C_
#define EDF
#else
#define EDF extern
#endif


EDF int Header_Mon(void);
EDF int Body_Mon(void);


#endif /* COMP_HTTP_COMMON_H_ */
