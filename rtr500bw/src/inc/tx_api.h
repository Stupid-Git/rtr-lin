/*
 * tx_api.h
 *
 *  Created on: Oct 17, 2024
 *      Author: karel
 */

#ifndef INC_TX_API_H_
#define INC_TX_API_H_

#include "stdint.h"
#include "stdbool.h"

#define TX_SUCCESS 0

#define TX_NO_WAIT 0
#define TX_WAIT_FOREVER 0xFFFFFFFF



typedef uint32_t ssp_err_t;

typedef char CHAR;
typedef long int LONG;

typedef unsigned char UCHAR;
typedef unsigned long int ULONG;
typedef unsigned short int USHORT;
typedef unsigned int UINT;


typedef struct TX_THREAD_STRUCT
{
	int xxx;
} TX_THREAD;

typedef struct TX_MUTEX_STRUCT
{
	int xxx;
} TX_MUTEX;



#endif /* INC_TX_API_H_ */
