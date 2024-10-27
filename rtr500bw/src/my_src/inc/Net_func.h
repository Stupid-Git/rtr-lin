/*
 * Net_def.h
 *
 *  Created on: 2019/10/17
 *      Author: tooru.hayashi
 */

#ifndef _INC_NET_FUNC_H_
#define _INC_NET_FUNC_H_



#ifdef EDF
#undef EDF
#endif

#ifdef _NET_FUNC_C_
#define EDF
#else
#define EDF extern
#endif

#include "tx_api.h"
#include "nx_api.h"
#include "nx_web_http_client.h"
#include "nxd_bsd.h"    //inet_aton() 2020.01.21

typedef struct st_srv_info
{
    char                    *host;              // e.g. "api.webstorage.jp"  "os.mbed.com"
    char                    *resource;          // e.g. "https://api.webstorage.jp/xyz.php"

    NXD_ADDRESS             server_ip;          // ip address of host
    UINT                    server_port;

    char                    *username;          // ""
    char                    *password;          // ""

    NX_WEB_HTTP_CLIENT      *p_secure_client;   //< HTTP client structure


} srv_info_t;


EDF UINT find_ip_address( srv_info_t* p_info );
EDF UINT find_ip_address2( char *target );
EDF UINT find_ip_address_1_2( srv_info_t* p_info );

EDF bool JudgeIPAdrs( char *src );

#endif /* _INC_NET_FUNC_H_ */
