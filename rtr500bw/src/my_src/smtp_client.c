/**
 * @file smtp_client.c
 *
 * @date     Created on: 2019/08/30
 * @author  tooru.hayashi
 */

#define _SMTP_CLIENT_C_

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "tx_api.h"
#include "nx_api.h"
#include "MyDefine.h"
#include "Globals.h"
#include "smtp_client.h"
#include "wifi_thread.h"
#include "General.h"

//#define td_printf               printf

static NX_TCP_SOCKET tcp_socket;

static char mail_buffer[1024];      //  email header and footer
static char base[1024]; // static -> BSS
    /* buf is used as the destination buffer for printing base with the format:
     * "%s\r\n". Hence, the size of buf should be at least the size of base
     * plus 2 bytes for the \r and \n characters.
     */
static unsigned char dst_buf[sizeof( base ) + 2]; // static -> BSS

UINT find_host_ip_address_smtp( smtp_client_t* p_client );
static int write_and_get_response(NX_TCP_SOCKET *socket_ptr, NX_PACKET_POOL *packet_pool_ptr, unsigned char *buf, /*size_t*/int len );
static int write_data(NX_TCP_SOCKET *socket_ptr, NX_PACKET_POOL *packet_pool_ptr, unsigned char *buf, /*size_t*/int len );


int MailPacketGen(int mode, char *fname);
int TestMailData2(void);


/**
 *
 * @param p_client
 * @param p_msg
 * @return
 */
UINT smtp_mail_client_send(smtp_client_t *p_client, smtp_msg_t *p_msg/*, smtp_tls_t *p_tls*/)
{
    uint32_t status = 0;

    NX_PACKET_POOL *packet_pool_ptr = p_client->p_packet_pool;
    //ULONG tls_server_port = p_client->server_port;        //未使用のためコメントアウト2020.01.17

    int ret = 1;
    int len;

 

//    char hostname[32];       //未使用のためコメントアウト2020.01.17

        
    Printf ("    server name = %s\r\n", p_client->server_name);
    status = find_host_ip_address_smtp (p_client);
    //status = find_host_ip_address_smtp (smtp_client.server_name);
    if (status != NX_SUCCESS)
    {
        return (status);
    }

    Printf ("    server name = %s\r\n", p_client->server_name);
    uint32_t server_address_ipv4 = p_client->server_ip.nxd_ip_address.v4;
    int server_port = (int)p_client->server_port;
    Printf ("    server_address = %d.%d.%d.%d server_port = %d\r\n", (int) (server_address_ipv4 >> 24),
               (int) (server_address_ipv4 >> 16) & 0xFF, (int) (server_address_ipv4 >> 8) & 0xFF,
               (int) (server_address_ipv4) & 0xFF, server_port);


    Printf("  . nx_tcp_socket_create() ... ");
    status =  nx_tcp_socket_create(&g_ip0, &tcp_socket, "Client Socket",
                                   NX_IP_NORMAL, NX_FRAGMENT_OKAY /*NX_DONT_FRAGMENT*/, NX_IP_TIME_TO_LIVE, 8192,
                                   NX_NULL, NX_NULL);
    if (status != NX_SUCCESS)
    {
        Printf("FAILED.\r\n");
        goto exit1;
    }
    Printf("ok.\r\n");

    ULONG client_port = 0;

    Printf ("  . nx_tcp_client_socket_bind() ... ");
    status = nx_tcp_client_socket_bind (&tcp_socket, client_port, 500);
    if (status != NX_SUCCESS)
    {
        Printf ("nx_tcp_client_socket_bind FAILED status = %d, 0x%x\n", status, status);
        goto exit1;
    }
    Printf ("ok.\r\n");

    Printf("  . Connecting (tcp) to Mail Server ... ");
    uint32_t server_adr_ipv4 = p_client->server_ip.nxd_ip_address.v4;

    status =  nx_tcp_client_socket_connect(&tcp_socket, server_adr_ipv4, (uint16_t)server_port, 5000);
    if (status != NX_SUCCESS)
    {
        // 56, 0x38 #define NX_NOT_CONNECTED 0x38
        Printf("nx_tcp_client_socket_connect FAILED status = %d, 0x%x\n", status, status );
        goto exit1;
    }
    Printf("ok.\r\n");


    Printf( "  > Get header from server:" );
    ret = write_and_get_response( &tcp_socket, packet_pool_ptr, dst_buf, 0 );
    if( ret < 200 || ret > 299 )
    {
        Printf( " failed 3\n  ! server responded with %d\n\n", ret );
        goto exit1;
    }
    Printf(" ok\n" );
    //  > Get header from server:
    //  220 SMTP at 42tev
    //  ok


    size_t m;
   

    //---------------------------------------------------------------------
    Printf( "  > Write AUTH LOGIN to server:" );
    len = sprintf( (char *) dst_buf, "AUTH LOGIN\r\n" );
    ret = (int)write_and_get_response( &tcp_socket , packet_pool_ptr , dst_buf, len );
    if( ret < 200 || ret > 399 )
    {
        Printf( " failed 4\n  ! server responded with %d\n\n", ret );
        goto exit;
    }
    Printf(" ok\n" );

    //---------------------------------------------------------------------
    Printf( "  > Write username to server: %s", p_client->user_name );

    base64Encode(  (char *) p_client->user_name, strlen( p_client->user_name ), (int8_t *)base, /*sizeof( base ),*/ &m );

    len = sprintf( (char *) dst_buf, "%s\r\n", base );
    ret = write_and_get_response( &tcp_socket , packet_pool_ptr , dst_buf, len );
    if( ret < 300 || ret > 399 )
    {
        Printf( " failed 5\n  ! server responded with %d\n\n", ret );
        goto exit;
    }
    Printf(" ok\n" );

//---------------------------------------------------------------------
    Printf( "  > Write password to server: %s", p_client->user_pwd );
    base64Encode(  (const unsigned char *) p_client->user_pwd, strlen( p_client->user_pwd ), (int8_t *)base, /*sizeof( base ),*/ &m );
    len = sprintf( (char *) dst_buf, "%s\r\n", base );
    ret = write_and_get_response( &tcp_socket, packet_pool_ptr , dst_buf, len );
    if( ret < 200 || ret > 399 )
    {
        Printf( " failed 6\n  ! server responded with %d\n\n", ret );
        goto exit;
    }
    Printf(" ok\n" );

    len = sprintf( (char *) dst_buf, "MAIL FROM:<%s>\r\n", p_msg->mail_from );
    ret = write_and_get_response( &tcp_socket , packet_pool_ptr , dst_buf, len );
    if( ret < 200 || ret > 299 )
    {
        Printf( " failed 7\n  ! server responded with %d\n\n", ret );
        goto exit;
    }
    //Printf(" ok\n" );


    Printf( "  > Write RCPT TO to server:" );
    len = sprintf( (char *) dst_buf, "RCPT TO:<%s>\r\n", p_msg->mail_to );
    ret = write_and_get_response( &tcp_socket , packet_pool_ptr , dst_buf, len );
    if( ret < 200 || ret > 299 )
    {
        Printf( " failed 8\n  ! server responded with %d\n\n", ret );
        goto exit;
    }

    len = sprintf( (char *) dst_buf, "DATA\r\n" );
    ret = write_and_get_response( &tcp_socket , packet_pool_ptr , dst_buf, len );
    if( ret < 300 || ret > 399 )
    {
        Printf( " failed 9\n  ! server responded with %d\n\n", ret );
        goto exit;
    }

    len = sprintf( (char *) dst_buf,
                    "From: %s\r\n"
                    "Subject: %s\r\n"
                    , p_msg->mail_from , p_msg->subject  );

    ret = write_data( &tcp_socket , packet_pool_ptr , dst_buf, len );


    //len = TestMailData2();
    //ret = write_data( &tcp_socket , packet_pool_ptr , mail_buffer, len );

    len = MailPacketGen(0, "");
    ret = write_data( &tcp_socket , packet_pool_ptr , (uint8_t *)mail_buffer, len );

    // body
    len = sprintf( (char *) dst_buf, "%s\r\n\r\n", p_msg->body);
    Printf("Body Size %d\r\n", len);
    ret = write_data( &tcp_socket , packet_pool_ptr , dst_buf, len );

    if(p_msg->attach != 0){
        Printf("Send Attach File \r\n");
        // add file
        len = MailPacketGen(1, p_msg->file);
        ret = write_data( &tcp_socket , packet_pool_ptr , (uint8_t *)mail_buffer, len );


        //#### file body send    
        len = (int)strlen(xml_buffer);
        Printf("    ####  smtp file size %ld\r\n", len);
        ret = write_data( &tcp_socket , packet_pool_ptr , (uint8_t *)xml_buffer, len );



        // end boundary
        len = MailPacketGen(2, "");
        ret = write_data( &tcp_socket , packet_pool_ptr , (uint8_t *)mail_buffer, len );
    }

    len = sprintf( (char *) dst_buf, "\r\n.\r\n");
    ret = write_and_get_response( &tcp_socket , packet_pool_ptr /*&ssl*/, dst_buf, len );
    if( ret < 200 || ret > 299 )
    {
        Printf( " failed 10\n  ! server responded with %d\n\n", ret );
        goto exit;
    }
    //Printf(" ok\n" );



    //Printf( "  > Write QUIT to server:" );
    len = sprintf( (char *) dst_buf, "QUIT\r\n" );
    ret = write_and_get_response( &tcp_socket , packet_pool_ptr /*&ssl*/, dst_buf, len );
    if( ret < 200 || ret > 299 )
    {
        Printf( " failed 11\n  ! server responded with %d\n\n", ret );
        goto exit;
    }
    Printf("smtp  ok!!\n" );




exit:
exit1:    

    
    status =  nx_tcp_socket_disconnect(&tcp_socket, 500);
    if (status)
    {
        //NOTE: if we send a "QUIT", the server will close the socket and so we get an error here
        //         nx_tcp_socket_disconnect FAILED  status = 65, 0x41
        Printf("nx_tcp_socket_disconnect FAILED  status = %d, 0x%x\r\n", status, status);
    }
    //NX 15. Release the local port by calling the nx_tcp_client_socket_unbind API.
    status = nx_tcp_client_socket_unbind(&tcp_socket);
    if (status)
    {
        Printf("nx_tcp_client_socket_unbind FAILED  status = %d, 0x%x\r\n", status, status);
    }

    status += nx_tcp_socket_delete(&tcp_socket);
    if (status)
    {
        Printf("nx_tcp_socket_delete FAILED  status = %d, 0x%x\r\n", status, status);
    }


    return (status);

   
}





//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
/**
 *
 * @param socket_ptr
 * @param packet_pool_ptr
 * @param buf
 * @param len
 * @return
 */
static int write_and_get_response(NX_TCP_SOCKET *socket_ptr, NX_PACKET_POOL *packet_pool_ptr, unsigned char *buf, /*size_t*/int len )
{
    UINT                  status;
    NX_PACKET            *send_packet;
    NX_PACKET            *receive_packet;
    static UCHAR          receive_buffer[512];
    static unsigned char  data[512];
    ULONG                 receive_bytes;

    char code[4];
    int i, idx = 0;

    Printf("\n SMTP_CLIENT3:   [%s]", buf);

    //---------- Send the TCP Packet to the Server ----------
    if( len > 0 )
    {
        //UINT nx_packet_allocate(NX_PACKET_POOL *pool_ptr, NX_PACKET **packet_ptr, ULONG packet_type, ULONG wait_option);
        status = nx_packet_allocate(packet_pool_ptr, &send_packet, NX_IPv4_TCP_PACKET, 500);
        if (status != NX_SUCCESS)
        {
            Printf("nx_packet_allocate FAILED wg 1 status = %d 0x%x \r\n", status, status );
            nx_packet_release(send_packet);
            return( -1 );
        } else {
            /* Add data to the packet */
            status = nx_packet_data_append(send_packet, buf, (ULONG)len, packet_pool_ptr, 500);
            if (status != NX_SUCCESS)
            {
                Printf("nx_packet_data_append FAILED wg 2 status = %d 0x%x \r\n", status, status );
                nx_packet_release(send_packet);
                return( -1 );
            } else {
                /* Send to the server */
                //UINT nx_tcp_socket_send(NX_TCP_SOCKET *socket_ptr, NX_PACKET *packet_ptr, ULONG wait_option);
                status = nx_tcp_socket_send(socket_ptr, send_packet, 100);
                if (status != NX_SUCCESS)
                {
                    Printf("nx_tcp_socket_send FAILED wg 3 status = %d 0x%x \r\n", status, status );
                    nx_packet_release(send_packet);
                    return( -1 );
                }
            }
            nx_packet_release(send_packet);
        }
    }

    //---------- Receive the TCP Packet to the Server ----------
    do
    {
        len = sizeof( data ) - 1;
        memset( data, 0, sizeof( data ) );

        /* Receive data from the server. */
        //UINT nx_tcp_socket_receive(NX_TCP_SOCKET *socket_ptr, NX_PACKET **packet_ptr, ULONG wait_option);
        status = nx_tcp_socket_receive( socket_ptr, &receive_packet, 500); // 500->5000
        if (status != NX_SUCCESS)
        {
            // 0x1 ?NO PACKET @ TO=500
            Printf("nx_tcp_socket_receive FAILED wg 4 status = %d 0x%x \r\n", status, status );
            //nx_packet_release(send_packet);
            return ( -1 );
        }
        else
        {
            /* Do things with the data.  Then release the packet */
            receive_bytes = 0;
            status = nx_packet_data_extract_offset(receive_packet, 0, receive_buffer, sizeof(receive_buffer), &receive_bytes);
            if (status != NX_SUCCESS)
            {
                Printf("nx_tcp_socket_receive FAILED wg 5 status = %d 0x%x \r\n", status, status );
            }
            //Printf("nx_packet_data_extract_offset status = %d 0x%x, receive_bytes = %d \r\n", (int)status, (int)status, (int)receive_bytes );
            memcpy(data, receive_buffer, receive_bytes);
            len = (int)receive_bytes;
            //nx_packet_release(receive_packet);
        }

        data[len] = '\0';
        Printf("\n SMTP_SERVER: [%s]",data);
        //len = ret;
        for( i = 0; i < len; i++ )
        {
            if( data[i] != '\n' )
            {
                if( idx < 4 )
                    code[ idx++ ] = (char)data[i];
                continue;
            }

            if( idx == 4 && code[0] >= '0' && code[0] <= '9' && code[3] == ' ' )
            {
                code[3] = '\0';
                return (atoi( code ));
            }

            idx = 0;
        }
    }
    while( 1 );
}



static int write_data(NX_TCP_SOCKET *socket_ptr, NX_PACKET_POOL *packet_pool_ptr, unsigned char *buf, /*size_t*/int len )
{
    UINT                  status;
    NX_PACKET            *send_packet;
 //   NX_PACKET            *receive_packet;
//    static UCHAR          receive_buffer[512];
//    static unsigned char  data[512];
//    ULONG                 receive_bytes;

 //   char code[4];
    int i = 0;
    //int idx = 0;
    int block = 0, amari;
//    int end;

    int zan = len;
    amari = len % 1360;

    Printf("\n SMTP_CLIENT4:   [%d] [%d] \r\n", zan, amari);
    int bk = 0;
    while(zan>0)
    {
        status = nx_packet_allocate(packet_pool_ptr, &send_packet, NX_IPv4_TCP_PACKET, 500);
        if (status != NX_SUCCESS)
        {
            Printf("nx_packet_allocate FAILED wd 1 status = %d 0x%x \r\n", status, status );
            nx_packet_release(send_packet);
            return( -1 );
        } 

         Printf("\n SMTP_CLIENT4 1:   [%d] [%d] [%d] \r\n", amari, zan, bk);      

        if(zan >= 1360){
            status = nx_packet_data_append(send_packet, buf+bk*1360, 1360, packet_pool_ptr, 500);
            zan -= 1360;
            Printf("\n HTTP BODY4 2:   [%d] [%d] [%d] zan[%d]\r\n", block, amari, i, zan);     
        }
        else{
            status = nx_packet_data_append(send_packet, buf+bk*1360, (ULONG)amari, packet_pool_ptr, 500);
            zan -= amari;
            Printf("\n HTTP BODY4 1:   [%d] [%d] [%d] zan[%d]\r\n", block, amari, i, zan);     
        }

        bk++;

                if (status != NX_SUCCESS)
                {
                    Printf("nx_packet_data_append FAILED wd  status = %d 0x%x \r\n", status, status );
                    nx_packet_release(send_packet);
                    return( -1 );
                } else {
                    /* Send to the server */
                    //UINT nx_tcp_socket_send(NX_TCP_SOCKET *socket_ptr, NX_PACKET *packet_ptr, ULONG wait_option);
                    status = nx_tcp_socket_send(socket_ptr, send_packet, 500);
                    if (status != NX_SUCCESS)
                    {
                        Printf("nx_tcp_socket_send FAILED wd 3 status = %d 0x%x (%d) \r\n", status, status, block );
                        nx_packet_release(send_packet);
                        return( -1 );
                        // NX_TX_QUEUE_DEPTH (0x49)   最大送信キュー深度に達しました。
                    }
                }
                tx_thread_sleep (1);
                
                tx_thread_sleep (2);
                //nx_packet_release(send_packet);
    }

     nx_packet_release(send_packet);

#ifndef UN_USE


    //Printf("\n SMTP_CLIENT3:   [%s]", buf);

    //---------- Send the TCP Packet to the Server ----------
    if( len > 0 )
    {
        //UINT nx_packet_allocate(NX_PACKET_POOL *pool_ptr, NX_PACKET **packet_ptr, ULONG packet_type, ULONG wait_option);

        amari = len % 1360;
        block = len / 1360;

        if( amari!=0 && block !=0){
            block++;
        }
        end = block;

        Printf("\n SMTP_CLIENT4:   [%d] [%d] [%d] \r\n", block, amari, end);

        if(block==0){
           status = nx_packet_allocate(packet_pool_ptr, &send_packet, NX_IPv4_TCP_PACKET, 500);
            if (status != NX_SUCCESS)
            {
                Printf("nx_packet_allocate FAILED wd 1 status = %d 0x%x \r\n", status, status );
                nx_packet_release(send_packet);
                return( -1 );
            } 
            else {
                Printf("\n SMTP_CLIENT4 10:   [%d] [%d] [%d] \r\n", block, amari, i);                
                /* Add data to the packet */
                status = nx_packet_data_append(send_packet, buf, (ULONG)len, packet_pool_ptr, 500);

                if (status != NX_SUCCESS)
                {
                    Printf("nx_packet_data_append FAILED wd  status = %d 0x%x \r\n", status, status );
                    nx_packet_release(send_packet);
                    return( -1 );
                } else {
                    /* Send to the server */
                    //UINT nx_tcp_socket_send(NX_TCP_SOCKET *socket_ptr, NX_PACKET *packet_ptr, ULONG wait_option);
                    status = nx_tcp_socket_send(socket_ptr, send_packet, 500);
                    if (status != NX_SUCCESS)
                    {
                        Printf("nx_tcp_socket_send FAILED wd 3 status = %d 0x%x (%d) \r\n", status, status, block );
                        nx_packet_release(send_packet);
                        return( -1 );
                        // NX_TX_QUEUE_DEPTH (0x49)   最大送信キュー深度に達しました。
                    }
                }
                nx_packet_release(send_packet);

            }

            goto Exit;
        }

        for(i=0;i<block;i++){
            
            status = nx_packet_allocate(packet_pool_ptr, &send_packet, NX_IPv4_TCP_PACKET, 500);
            if (status != NX_SUCCESS)
            {
                Printf("nx_packet_allocate FAILED wd 1 status = %d 0x%x \r\n", status, status );
                nx_packet_release(send_packet);
                return( -1 );
            } else {


                Printf("\n SMTP_CLIENT4 1:   [%d] [%d] [%d] \r\n", block, amari, i);                
                /* Add data to the packet */
                //if(end==0)
                //    status = nx_packet_data_append(send_packet, buf, (ULONG)len, packet_pool_ptr, 500);
                if(i==end-1){
                    status = nx_packet_data_append(send_packet, buf+i*1360, (ULONG)amari, packet_pool_ptr, 500);
                    Printf("          end status = %d \r\n", status);
                }
                else
                    status = nx_packet_data_append(send_packet, buf+i*1360, 1360, packet_pool_ptr, 500);


                if (status != NX_SUCCESS)
                {
                    Printf("nx_packet_data_append FAILED wd  status = %d 0x%x \r\n", status, status );
                    nx_packet_release(send_packet);
                    return( -1 );
                } else {
                    /* Send to the server */
                    //UINT nx_tcp_socket_send(NX_TCP_SOCKET *socket_ptr, NX_PACKET *packet_ptr, ULONG wait_option);
                    status = nx_tcp_socket_send(socket_ptr, send_packet, 500);
                    if (status != NX_SUCCESS)
                    {
                        Printf("nx_tcp_socket_send FAILED wd 3 status = %d 0x%x (%d) \r\n", status, status, block );
                        nx_packet_release(send_packet);
                        return( -1 );
                        // NX_TX_QUEUE_DEPTH (0x49)   最大送信キュー深度に達しました。
                    }
                }
                tx_thread_sleep (1);
                nx_packet_release(send_packet);
                tx_thread_sleep (2);
            }

        }
    }
#endif // !UN_USE

//Exit:

    return(0);
}


UINT find_host_ip_address_smtp( smtp_client_t* p_client )
{
    UINT status;

    ULONG ip_host;
    // Lookup server name using DNS
    // UINT  nx_dns_host_by_name_get(NX_DNS *dns_ptr, UCHAR *host_name, ULONG *host_address_ptr, ULONG wait_option);

    Printf ("  . nx_dns_host_by_name_get\r\n");
    //status = nx_dns_host_by_name_get (&g_dns0, (UCHAR*)p_client->server_name, &ip_host, 1000);
    status = nx_dns_host_by_name_get (&g_dns0, (UCHAR*)p_client->server_name, &ip_host, 2000);
    if (status != NX_SUCCESS)
    {
        Printf ("  Unable to resolve ");
        Printf (p_client->server_name);
        Printf ("\r\n");
        Printf ("  nx_dns_host_by_name_get Failed. status = 0x%x\r\n", status);
        // 0xA1
        // #define NX_DNS_NO_SERVER  0xA1  /* No DNS server was specified                          */
        return (status);
    }
    else
    {
        p_client->server_ip.nxd_ip_address.v4 = ip_host;
        p_client->server_ip.nxd_ip_version = 4;
    }

    return (status);
}

/**
 *
 * @param mode
 * @param fname
 * @return
 */
int MailPacketGen(int mode, char *fname)
{
    char *dst;
    size_t size;

    memset((uint8_t *)&mail_buffer, 0 , sizeof(mail_buffer));

    dst = (char *)&mail_buffer;

    switch (mode)
    {
    case 0:
        dst += sprintf(dst, "MIME-Version: 1.0\r\n");
        dst += sprintf(dst, "Content-Type: multipart/mixed;\r\n");
        dst += sprintf(dst, "   boundary=\"rtr500\"\r\n");
        dst += sprintf(dst, "Content-Language: en-US\r\n\r\n");

        dst += sprintf(dst,"This is multi-part message in MIME format.\r\n");
        dst += sprintf(dst, "--rtr500\r\n");

        dst += sprintf(dst, "Content-Type: text/plain; ");
        dst += sprintf(dst, "charset=utf-8; format=flowed\r\n");
        //dst += sprintf(dst, "\r\n");
        dst += sprintf(dst, "Content-Transfer-Encoding: 7bit\r\n\r\n");
        break;
    
    case 1:
        dst += sprintf(dst, "--rtr500\r\n");
        dst += sprintf(dst, "Content-Type: text/plain; charset=UTF-8;\r\n");
        dst += sprintf(dst, "  name=\"%s\"\r\n", fname);
        //dst += sprintf(dst, "Content-Transfer-Encoding: base64\r\n");
        dst += sprintf(dst, "Content-Transfer-Encoding: 8bit\r\n");
        dst += sprintf(dst, "Content-Disposition: attachment;");
        dst += sprintf(dst, " filename=\"%s\"\r\n", fname);
        dst += sprintf(dst, "\r\n");
        break;
    case 2:
        dst += sprintf(dst, "--rtr500--\r\n");
        break;

    default:
        dst += sprintf(dst, "\r\n");
        break;
    }




    size = strlen(mail_buffer);
    Printf("mail size %d\r\n", strlen(mail_buffer));

    return ((int)size);
}


/**
 *
 * @return
 */
int TestMailData2(void)
{

    char *dst;
    size_t size;

    memset((uint8_t *)&mail_buffer, 0 , sizeof(mail_buffer));

    dst = (char *)&mail_buffer;

    //dst += sprintf(dst, "Content-Type: text/plain;\r\n charset=\"utf-8\"\r\n");
    //dst += sprintf(dst, "Content-Transfer-Encoding: 8bit\r\n\r\n");

    dst += sprintf(dst, "MIME-Version: 1.0\r\n");
    dst += sprintf(dst, "Content-Type: multipart/mixed;\r\n");
    dst += sprintf(dst, "   boundary=\"rtr500\"\r\n");
    dst += sprintf(dst, "Content-Language: en-US\r\n\r\n");

    dst += sprintf(dst,"This is multi-part message in MIME format.\r\n");
    dst += sprintf(dst, "--rtr500\r\n");

    dst += sprintf(dst, "Content-Type: text/plain; ");
    dst += sprintf(dst, "charset=utf-8; format=flowed\r\n");
    //dst += sprintf(dst, "\r\n");
    dst += sprintf(dst, "Content-Transfer-Encoding: 7bit\r\n\r\n");

    Printf("mail headr1 %d\r\n", strlen(mail_buffer));
    dst += sprintf(dst, "Test Mail\r\n\r\n");

    dst += sprintf(dst, "--rtr500\r\n");
    dst += sprintf(dst, "Content-Type: text/plain; charset=UTF-8;\r\n");
    //dst += sprintf(dst, "charset=utf-8;\r\n");
    dst += sprintf(dst, "  name=\"Text.txt\"\r\n");
    dst += sprintf(dst, "Content-Transfer-Encoding: base64\r\n");
    //dst += sprintf(dst, "Content-Description: Attached txt file encrypted mime64\r\n");
    dst += sprintf(dst, "Content-Disposition: attachment;");
    dst += sprintf(dst, " filename=\"Test.txt\"\r\n");
    dst += sprintf(dst, "\r\n");
    dst += sprintf(dst, "QUJD\r\n");
    dst += sprintf(dst, "--rtr500--\r\n");

    size = strlen(mail_buffer);
    Printf("mail headr2 %d\r\n", strlen(mail_buffer));

    return ((int)size);
}
