/*
 * nx_api.h
 *
 *  Created on: Oct 17, 2024
 *      Author: karel
 */

#ifndef INC_NX_API_H_
#define INC_NX_API_H_

#include "tx_api.h"

/* Define IPv4/v6 Address structure */
typedef struct NXD_ADDRESS_STRUCT
{
    /* Flag indicating IP address format.  Valid values are:
       NX_IP_VERSION_V4 and NX_IP_VERSION_V6.
     */
    ULONG nxd_ip_version;

    /* Union that holds either IPv4 or IPv6 address. */
    union
    {

#ifndef NX_DISABLE_IPV4
        ULONG v4;
#endif /* NX_DISABLE_IPV4 */
#ifdef FEATURE_NX_IPV6
        ULONG v6[4];
#endif /* FEATURE_NX_IPV6 */
    } nxd_ip_address;
} NXD_ADDRESS;


#endif /* INC_NX_API_H_ */
