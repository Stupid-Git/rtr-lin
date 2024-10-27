/* generated thread header file - do not edit */
#ifndef FTP_THREAD_H_
#define FTP_THREAD_H_
#include "bsp_api.h"
#include "tx_api.h"
#include "hal_data.h"
#ifdef __cplusplus
extern "C" void ftp_thread_entry(void);
#else
extern void ftp_thread_entry(void);
#endif
#include "nxd_ftp_client.h"
#ifdef __cplusplus
extern "C"
{
#endif
extern NX_FTP_CLIENT g_ftp_client0;
void g_ftp_client0_err_callback(void *p_instance, void *p_data);
void ftp_client_init0(void);
extern TX_EVENT_FLAGS_GROUP event_flags_ftp;
extern TX_MUTEX mutex_ftp;
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* FTP_THREAD_H_ */
