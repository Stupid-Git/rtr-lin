/**
 * @file    wifi_thread_entry.h
 *
 * @date    2019/11/06
 * @author  tooru.hayashi
 */

//#include "wifi_thread.h"

#ifndef _WIFI_THREAD_ENTRY_H_
#define _WIFI_THREAD_ENTRY_H_





#ifdef EDF
#undef EDF
#endif

#ifndef _WIFI_THREAD_ENTRY_C_
    #define EDF extern
#else
    #define EDF
#endif
/*
typedef struct netif_static_mode
{
    ULONG address;
    ULONG mask;
    ULONG gw;
    ULONG dns;
}netif_static_mode_t;


typedef struct net_input_cfg
{
    uint8_t netif_valid;
    uint8_t netif_select[16];
    uint8_t netif_addr_mode;
    uint8_t interface_index;
    netif_static_mode_t netif_static;
    //sf_wifi_provisioning_t wifi_prov;
}net_input_cfg_t;
*/

EDF int wifi_module_scan(void);
EDF uint32_t start_up_sntp(void);

EDF void wifi_current_status(void);

EDF UCHAR   lan_connect_check(UCHAR mode);          //sakaguchi UT-0035
EDF bool    set_state_check(void);                  //sakaguchi UT-0035

#endif /* _WIFI_THREAD_ENTRY_H_ */
