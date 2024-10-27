/* generated thread header file - do not edit */
#ifndef WIFI_THREAD_H_
#define WIFI_THREAD_H_
#include "bsp_api.h"
#include "tx_api.h"
#include "hal_data.h"
#ifdef __cplusplus
extern "C" void wifi_thread_entry(void);
#else
extern void wifi_thread_entry(void);
#endif
#include "nxd_sntp_client.h"
#include "nxd_dns.h"
#include "nxd_dhcp_client.h"
#ifdef __cplusplus
extern "C"
{
#endif
extern NX_SNTP_CLIENT g_sntp0;
#if !defined(leap_second_handler)
UINT leap_second_handler(NX_SNTP_CLIENT *client_ptr, UINT indicator);
#endif
#if !defined(kiss_of_death_handler)
UINT kiss_of_death_handler(NX_SNTP_CLIENT *client_ptr, UINT code);
#endif
#if !defined(NULL)
VOID NULL(struct NX_SNTP_CLIENT_STRUCT *client_ptr, ULONG *rand);
#endif
void g_sntp0_err_callback(void *p_instance, void *p_data);
void sntp_client_init0(void);
extern NX_DNS g_dns0;
void g_dns0_err_callback(void *p_instance, void *p_data);
void dns_client_init0(void);
extern NX_DHCP g_dhcp_client0;
void g_dhcp_client0_err_callback(void *p_instance, void *p_data);
void dhcp_client_init0(void);

#define DHCP_USR_OPT_ADD_ENABLE_g_dhcp_client0 (0)
#define DHCP_USR_OPT_ADD_FUNCTION_ENABLE_g_dhcp_client0 (1)

#if DHCP_USR_OPT_ADD_ENABLE_g_dhcp_client0
UINT dhcp_user_option_add_client1(NX_DHCP *dhcp_ptr, UINT iface_index, UINT message_type, UCHAR *user_option_ptr, UINT *user_option_length);
#if DHCP_USR_OPT_ADD_FUNCTION_ENABLE_g_dhcp_client0
extern UCHAR g_dhcp_client0_opt_num;
extern CHAR *g_dhcp_client0_opt_val;
#endif
#endif
extern TX_MUTEX mutex_sntp;
extern TX_MUTEX mutex_dns;
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* WIFI_THREAD_H_ */
