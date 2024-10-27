/**
 * @file    Suction.h
 *
 * @date     Created on: 2019/09/03
 * @author  tooru.hayashi
 */

#ifndef _SUCTION_H_
#define _SUCTION_H_






#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


#ifdef EDF
#undef EDF
#endif

#ifndef _SUCTION_C_
    #define EDF extern
#else
    #define EDF
#endif

EDF int SendSuctionFile( int32_t Test );

//EDF void MakeSuctionFTP(void);
//EDF void MakeSuctionEmail( int32_t );
EDF void SendSuctionTest(void);
EDF void MakeSuctionXML( int32_t Test );
EDF int SendSuctionData( int32_t Test );

#endif /* _SUCTION_H_ */
