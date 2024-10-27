/*
 * tls_client.h
 *
 *  Created on: 2019/09/18
 *      Author: tooru.hayashi
 */

#ifndef _TLS_CLIENT_H_
#define _TLS_CLIENT_H_


#ifdef EDF
#undef EDF
#endif

#ifndef _TLS_CLIENT_C_
    #define EDF extern
#else
    #define EDF
#endif

/*
// Info for driving nx_web_http_client
EDF typedef struct st_whc_info
{
    char                    *host;              // e.g. "api.webstorage.jp"  "os.mbed.com"
    char                    *resource;          // e.g. "https://api.webstorage.jp/xyz.php"

    NXD_ADDRESS             server_ip;          // ip address of host
    UINT                    server_port;

    char                    *username;          // ""
    char                    *password;          // ""

//    NX_IP                   *p_ip;            ///< NetX IP struct
//    NX_DNS                  *p_dns;           ///< NetX DNS struct
//    UINT                    thread_priority;  ///< HTTP thread priority
    NX_WEB_HTTP_CLIENT      *p_secure_client;   ///< HTTP client structure

//    void                    (*http_init)(void); ///< init function
//    uint8_t                 *p_netif;

} whc_info_t;
*/


EDF UINT my_https_setup(int which,int cert_index);
EDF UINT my_tls_setup(void);
EDF UINT my_tls_session_delete(void);               // sakaguchi 2020.09.03
EDF UINT my_client_error_exit(void);                // sakaguchi 2020.09.09
EDF UINT my_https_PUT_3(int which);
EDF UINT my_https_POST(int which);
EDF UINT my_tls_DOWN(void);
EDF int  ichttp_filesnd_check(void);
EDF void vhttp_sysset_sndon(void);
EDF void SendLogData(void);
EDF bool bhttp_request_docheck(void);               // sakaguchi 2020.09.02
EDF void vhttp_cmdflg_reset(int ifile, int imode);  // sakaguchi 2021.02.16
EDF void vhttp_cmdflg_allreset(int imode);          // sakaguchi 2021.02.16


//EDF UINT my_tls_close(NX_WEB_HTTP_CLIENT* client_ptr,int which);

//EDF UINT get_web_http_client_response_body(NX_WEB_HTTP_CLIENT* client_ptr );
//EDF UINT find_host_ip_address( whc_info_t* p_info );


#endif /* _TLS_CLIENT_H_ */
