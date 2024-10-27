/*
 * smtp_client.h
 *
 *  Created on: 2019/08/30
 *      Author: tooru.hayashi
 */

#ifndef _SMTP_CLIENT_H_
#define _SMTP_CLIENT_H_

#ifdef EDF
#undef EDF
#endif
#ifdef _SMTP_CLIENT_C_
#define EDF
#else
#define EDF extern
#endif



#include "nx_api.h"

typedef struct S_smtp_msg
{
    const char  *mail_from;         ///< E-Mail address to use as sender
    const char  *mail_to;           ///< E-Mail address to use as recipient
    char        *subject;
    char        *body;
    char        *file;
    int         attach;             ///< 0: no file 1: attach file

} smtp_msg_t;


typedef struct S_smtp_client
{
    NX_IP          *p_ip;
    NX_PACKET_POOL *p_packet_pool;  ///<   &g_packet_pool0,

    //int debug_level;              ///< level of debugging
    int             authentication; ///< if authentication is required
    int             mode;           ///< SSL/TLS (0) or STARTTLS (1)

    NXD_ADDRESS     server_ip;      ///< ip address of host

    const char      *server_name;   ///< hostname of the server (client only)
    UINT            server_port;    ///< port on which the ssl service runs
    const char      *user_name;     ///< username to use for authentication
    const char      *user_pwd;      ///< password to use for authentication

  //const char *ca_file;            // the file with the CA certificate(s)
  //const char *crt_file;           // the file with the client certificate
  //const char *key_file;           // the file with the client key      
  //int force_ciphersuite[2];       // protocol/ciphersuite to use, or all


} smtp_client_t;



EDF UINT smtp_mail_client_send(smtp_client_t *p_client, smtp_msg_t *p_msg/*, smtp_tls_t *p_tls*/);

#endif /* _SMTP_CLIENT_H_ */
