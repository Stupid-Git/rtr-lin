/**
 * FTP thread
 *
 * @file	ftp_thread_entry.c
 * @note	2019.Dec.26 ビルドワーニング潰し完了
 * @note	文字コード SJIS-> UTF8
 */

#include "Net_func.h"
#include "Globals.h"
#include "Config.h"
#include "MyDefine.h"
#include "General.h"
#include "Log.h"
#include "wifi_thread.h"
#include "ftp_thread.h"
#include "Globals.h"
extern TX_THREAD ftp_thread;

UINT g_ftp_server_control_port_user;
UINT g_ftp_server_data_port_user;

#define NX_FTP_SERVER_CONTROL_PORT          g_ftp_server_control_port_user          /* Control Port for FTP server                         */

#define NX_FTP_SERVER_DATA_PORT             g_ftp_server_data_port_user          /* Data Port for FTP server in active transfer mode    */

#define FTP_PASV_MODE                       1

#define NX_FTP_ERROR                        0xD0        /* Generic FTP internal error - deprecated             */ 
#define NX_FTP_TIMEOUT                      0xD1        /* FTP timeout occurred                                */ 
#define NX_FTP_FAILED                       0xD2        /* FTP error                                           */ 
#define NX_FTP_NOT_CONNECTED                0xD3        /* FTP not connected error                             */ 
#define NX_FTP_NOT_DISCONNECTED             0xD4        /* FTP not disconnected error                          */ 
#define NX_FTP_NOT_OPEN                     0xD5        /* FTP not opened error                                */ 
#define NX_FTP_NOT_CLOSED                   0xD6        /* FTP not closed error                                */ 
#define NX_FTP_END_OF_FILE                  0xD7        /* FTP end of file status                              */ 
#define NX_FTP_END_OF_LISTING               0xD8        /* FTP end of directory listing status                 */ 
#define NX_FTP_EXPECTED_1XX_CODE            0xD9        /* Expected a 1xx response from server                 */
#define NX_FTP_EXPECTED_2XX_CODE            0xDA        /* Expected a 2xx response from server                 */
#define NX_FTP_EXPECTED_22X_CODE            0xDB        /* Expected a 22x response from server                 */
#define NX_FTP_EXPECTED_23X_CODE            0xDC        /* Expected a 23x response from server                 */
#define NX_FTP_EXPECTED_3XX_CODE            0xDD        /* Expected a 3xx response from server                 */
#define NX_FTP_EXPECTED_33X_CODE            0xDE        /* Expected a 33x response from server                 */
#define NX_FTP_INVALID_NUMBER               0xDF        /* Extraced an invalid number from server response     */
#define NX_FTP_INVALID_ADDRESS              0x1D0       /* Invalid IP address parsed from FTP command          */
#define NX_FTP_INVALID_COMMAND              0x1D1       /* Invalid FTP command (bad syntax, unknown command)   */

static UINT ftp_send_file(NX_PACKET *send_packet, unsigned char *buf, int len );

//"nxd_bsd.h" で定義済み
#if 0
struct in_addr
{
    ULONG           ip_host;             /* Internet address (32 bits).                         */
};
#endif

/*
typedef struct st_ftp_info
{
    char                    *host;      // e.g. "api.webstorage.jp"  "os.mbed.com"
    char                    *resource;  // e.g. "https://api.webstorage.jp/xyz.php"

    NXD_ADDRESS             server_ip;  // ip address of host
    UINT                    server_port;
    //ULONG                   s_addr;             // Internet address (32 bits).

    char                    *username; // ""
    char                    *password; // ""


} ftp_info_t;
*/

srv_info_t  g_ftp_info;

/**
 *  @brief	FTP Thread entry function
 *  @note	LO_HIマクロ排除
 */
void ftp_thread_entry(void)
{
    /* TODO: add your own code here */

    NX_PACKET * ftp_packet = 0;
    ULONG FTP_SERVER;
    UINT status ,st;
    int xsize;



    srv_info_t* p_info;
    p_info = &g_ftp_info;
    memset (&g_ftp_info, 0, sizeof(g_ftp_info));

    for(;;){
        xsize = (int)strlen((char *)xml_buffer);
        Printf("file size %d\r\n", xsize);

        if(xsize==0){
            NetCmdStatus = ETLAN_ERROR;
            tx_event_flags_set (&event_flags_ftp, FLG_FTP_ERROR, TX_OR);
            Net_LOG_Write(600,"%04X",0);
            goto EXIT;
        }
         
        if(my_config.ftp.Pasv[0] == FTP_PASV_MODE){
            status = nx_ftp_client_passive_mode_set(&g_ftp_client0, NX_TRUE);       // passive mode
        }
        else{
            status = nx_ftp_client_passive_mode_set(&g_ftp_client0, NX_FALSE);       // active mode
        }
        Printf("ftp mode %ld\r\n", status);
        

        p_info->host = (char *)my_config.ftp.Server;
        p_info->server_port = *(uint16_t *)&my_config.ftp.Port[0];

        p_info->username = (char *)my_config.ftp.UserID;
        p_info->password = (char *)my_config.ftp.UserPW;

        Printf("  FTP Srv %s  %s  %s \r\n", p_info->host, p_info->username, p_info->password );

        status = find_ip_address( p_info );

        FTP_SERVER = p_info->server_ip.nxd_ip_address.v4;

        status =  nx_ftp_client_connect(&g_ftp_client0, FTP_SERVER, p_info->username, p_info->password, 500);       // 200
        Printf("ftp connect %08X  %ld\r\n", status, status);
        //if (status != NX_SUCCESS)
        if (!((status == NX_SUCCESS) || (status == NX_FTP_NOT_DISCONNECTED)))     // NX_FTP_NOT_DISCONNECTED (0xD4) Client is already connected.
        {
            NetCmdStatus = ETLAN_ERROR;
            PutLog(LOG_LAN, "FTP Login Error %ld", status);
            DebugLog( LOG_DBG, "FTP Login Error (%04X)", status);
            tx_event_flags_set (&event_flags_ftp, FLG_FTP_ERROR, TX_OR);
            Net_LOG_Write(601,"%04X",status);
            goto EXIT;
        }
 
        //status = nx_ftp_client_file_open(&g_ftp_client0, XMLName, NX_FTP_OPEN_FOR_WRITE,100);
        //status = nx_ftp_client_file_open(&g_ftp_client0, (char *)FtpHeader.FNAME, NX_FTP_OPEN_FOR_WRITE,100);
        status = nx_ftp_client_file_open(&g_ftp_client0, (char *)FtpHeader.PATH, NX_FTP_OPEN_FOR_WRITE,100);        // 2020.12.08

        Printf("ftp file open %02X  %ld \r\n", status, status);
        if(status == NX_SUCCESS){
            //status_s = ftp_send_file(ftp_packet, (uint8_t *)xml_buffer, xsize);
            status = ftp_send_file(ftp_packet, (uint8_t *)xml_buffer, xsize);
            Printf("ftp file send %02X  %ld \r\n", status, status);

            st = nx_ftp_client_file_close(&g_ftp_client0, 2000);        // 100 --> 2000 // sakaguchi 2021.05.28
            Printf("client close write %02X\r\n", st);
            if(st != NX_SUCCESS){
                status = st;
            }
        }

        tx_thread_sleep (10);
        st =  nx_ftp_client_disconnect(&g_ftp_client0, 200);
        Printf("ftp disconnect %02X\r\n", st);
        if (status != NX_SUCCESS/* || status_s != NX_SUCCESS*/)
        {
            NetCmdStatus = ETLAN_ERROR;
            //PutLog(LOG_LAN, "FTP File Send Error %ld", status);
            switch (status)
            {
                case NX_NOT_CONNECTED:
                    //PutLog(LOG_LAN, "FTP Not Create");;
                    PutLog(LOG_LAN, "FTP Not Connected");;
                    break;
                case NX_FTP_NOT_OPEN:
                    PutLog(LOG_LAN, "FTP File Not Open");;
                    break;
                case NX_FTP_EXPECTED_2XX_CODE:
                    PutLog(LOG_LAN, "FTP Send Error");;
                    break;
                default:
                    PutLog(LOG_LAN, "FTP File Send Error %ld", status);
                    break;
            }

            DebugLog( LOG_DBG, "FTP Error (%04X)", status);
            tx_event_flags_set (&event_flags_ftp, FLG_FTP_ERROR, TX_OR);
            //Net_LOG_Write(600,"%04X",status_s);
            Net_LOG_Write(600,"%04X",status);

        }
        else{
            NetCmdStatus = ETLAN_SUCCESS;      
            tx_event_flags_set (&event_flags_ftp, FLG_FTP_SUCCESS, TX_OR);
            Net_LOG_Write(1,"%04X",0);
        }


EXIT:

        tx_thread_suspend(&ftp_thread);
    }
    
}

//#define PK_SIZE 1360          // sakaguchi 2021.07.16 del

/**
 * ftp_send_file
 * @param send_packet
 * @param buf
 * @param len
 * @return
 */
static UINT ftp_send_file(NX_PACKET *send_packet, unsigned char *buf, int len )
{
    UINT       status;
    UINT       status2;
    ULONG total_packets;
    ULONG free_packets;
    ULONG empty_pool_requests;
    ULONG empty_pool_suspensions;
    ULONG invalid_packet_releases;    
    ULONG timer_ticks;                // sakaguchi 2021.05.28

    int block, amari, end;
    int zan = len;
// sakaguchi 2021.07.16 ↓
    uint16_t PK_SIZE;

    PK_SIZE = *(uint16_t *)&my_config.network.Mtu[0];   // MTU
    PK_SIZE = (uint16_t)(PK_SIZE - 40);                 // -40byte
// sakaguchi 2021.07.16 ↑

    // packet_pool0  --> packet_pool0に変更
    amari = len % PK_SIZE;
    block = len / PK_SIZE;
    end = 0;

    Printf("\n FTP File Send:   [%d] [%d] [%d] \r\n", block, amari, end);

    int bk = 0;
    while(zan>0)
    {

        status2 = nx_packet_pool_info_get(&g_packet_pool1,&total_packets, &free_packets, &empty_pool_requests, &empty_pool_suspensions, &invalid_packet_releases);
        Printf("nx_packet_pool_info_get status = 0x%x %ld %ld %ld %ld %ld (%d)\r\n", status2, total_packets, free_packets, empty_pool_requests, empty_pool_suspensions, invalid_packet_releases,zan);

        //nx_packet_release(send_packet);
        status =  nx_packet_allocate(&g_packet_pool1, &send_packet, NX_TCP_PACKET, 100 /*NX_WAIT_FOREVER*/);
        if(status != NX_SUCCESS)
        {
            //PutLog(LOG_LAN,"FTP error %04X", status);
            Printf("nx_packet_allocate 2 Failed. status = 0x%x\r\n", status);
            return( status);
        }

        if(zan >= PK_SIZE){
            status = nx_packet_data_append(send_packet, buf+bk*PK_SIZE, PK_SIZE, &g_packet_pool1, 500);
            zan -= PK_SIZE;
            //Printf("\n FTP BODY4 2:   [%d] [%d] [%d] zan[%d]\r\n", block, amari, bk, zan);     
        }
        else{
            status = nx_packet_data_append(send_packet, buf+bk*PK_SIZE, (ULONG)amari, &g_packet_pool1, 500);
            zan -= amari;
            //Printf("\n FTP BODY4 1:   [%d] [%d] [%d] zan[%d]\r\n", block, amari, bk, zan);     
        }

        bk++;

        if (status != NX_SUCCESS)
        {
            //PutLog(LOG_LAN,"FTP error %04X", status);
            Printf("nx_packet_data_append FAILED ftp  status = %d 0x%x \r\n", status, status );
            nx_packet_release(send_packet);
            return( status );
        } 
        else 
        {
            //status = nx_ftp_client_file_write(&g_ftp_client0, send_packet, 250);    //  100->250
            status = nx_ftp_client_file_write(&g_ftp_client0, send_packet, 2500);    //  250->2500  // sakaguchi 2021.05.28
            if (status != NX_SUCCESS)
            {
                //PutLog(LOG_LAN,"FTP error %04X", status);
                Printf("nx_tcp_socket_send FAILED ftp 3 status = %d 0x%x (%d) \r\n", status, status, block );
                nx_packet_release(send_packet);
                return( status );
            }
        }

// sakaguchi 2021.07.16 ↓
        if(zan > 0){
            if(my_config.network.Interval[0]){      // sakaguchi 2021.05.28 モバイルルータ接続時の再送防止
                timer_ticks = (my_config.network.Interval[0] / 10);
                tx_thread_sleep(timer_ticks);
            }
        }
// sakaguchi 2021.07.16 ↑
    }

    tx_thread_sleep (2);
    //nx_packet_release(send_packet);

    // NX_TX_QUEUE_DEPTH (0x49)   最大送信キュー深度に達しました。
    // NX_WINDOW_OVERFLOW (0x39)  リクエストが受信側者にアドバタイズされたバイト単位のウィンドウサイズより大きくなっています。
    // NX_NOT_CONNECTED  (0x38)

    return (status);
}
