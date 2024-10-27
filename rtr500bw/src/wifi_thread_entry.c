/**
 * @file    wifi_thread_entry.c
 * @brief
 * @note    2020.01.30  v6 0130 ソース反映
 * @note	2020.Jul.27 GitHub 0722ソース反映済み
 * @note	2020.Jul.27 GitHub 0727ソース反映済み 
 * @note	2020.Aug.07	0807ソース反映済み
 */

#define _WIFI_THREAD_ENTRY_C_
//#include "nx_api.h"
//#include "wifi_thread.h"
#include "network_thread.h"
#include "flag_def.h"
#include "Globals.h"
#include "General.h"
#include "config.h"
#include "MyDefine.h"
#include "sntp_client.h"
#include "sntp_thread.h"
#include "DateTime.h"
#include "Log.h"
#include "wifi_thread_entry.h"
#include "ble_thread_entry.h"       // 2024 02 21 D.00.03.185
#include "usb_thread_entry.h"       // 2024 02 21 D.00.03.185

extern TX_THREAD sntp_thread;

NX_IP   *g_active_ip;   
NX_DHCP *g_active_dhcp;    

typedef struct
{
    UCHAR *ssid;
    UCHAR *pwd;
} wifi_ap_info_t;

typedef struct
{
    UCHAR *ap_ssid;
    UCHAR *ap_pwd;
} wifi_queue_payload_t;

#define APP_ERR_TRAP(a)     if(a) {__asm("BKPT #0\n");} /* trap the error location */
#define UX_DEMO_BUFFER_SIZE (65)    /* assume SSID and password are 64 character or less */
#define MAX_RETRY_CNT       (10)

//2023.11.16 ------------------------------------------------------------
#include "wifi_sdk6.h"
//2023.11.16 ------------------------------------------------------------

extern TX_THREAD wifi_thread;
//extern TX_THREAD eth_thread;      //2019.Dec.26 コメントアウト

void print_ipv4_addr(ULONG address, char *str, size_t len);
static int wait_for_lnk_wifi(void);             // Check on NetX IP Link enable
//static int wifi_module_scan(void);
static int wifi_module_provision(void);
//static int32_t start_dhcp_client (uint32_t);
static uint32_t start_up_dns(void);
//static uint32_t start_up_sntp(void);
//uint32_t start_up_sntp2(void);
void provConDisconCB(sf_wifi_callback_args_t * p_args);

/**
 *
 * @param address
 * @param str
 * @param len
 */
void print_ipv4_addr(ULONG address, char *str, size_t len)
{
    snprintf(str, len, "%lu.%lu.%lu.%lu",
             (address >> 24) & 0xff,
             (address >> 16) & 0xff,
             (address >> 8) & 0xff,
             (address >> 0) & 0xff);
}

#define NET_SUCCESS         (uint32_t)0
#define NET_LINK_ERROR      (uint32_t)1
#define NET_DNS_ERROR       (uint32_t)2
#define NET_SNTP_ERROR      (uint32_t)3
#define NET_DHCP_ERROR      (uint32_t)4
#define NET_STATIC_IP_ERROR (uint32_t)5
#define NET_ATTACH_ERROR    (uint32_t)6
#define NET_WIFI_PROV_ERROR (uint32_t)7
#define NET_WIFI_SCAN_ERROR (uint32_t)8

#define NET_WIFI_PARAM_ERROR (uint32_t)100

#define ETHERNET    0
#define WIFI        1


static uint32_t connect_eth_dhcp(void);
static uint32_t connect_eth_static(void);
static uint32_t connect_wifi_dhcp(void);
static uint32_t connect_wifi_static(void);
uint32_t check_wifi_para(void);
// WIFI Thread entry function

//net_input_cfg_t my_net_cfg;

static UCHAR init_sntp;
//static uint8_t net_error;       //未使用
static uint16_t wifi_faile;  
static uint8_t  max_rssi_channel;   // 2021.03.12

static uint8_t wifi_sec_mode = 0;         // セキュリティモード     // sakaguchi 2020.09.07
//static sf_wifi_provisioning_t g_provision_info;                   // sakaguchi 2020.09.07
static sf_wifi_provisioning_ext_t g_provision_info;                 // BSSIDとcountry codeを拡張    // sakaguchi 2021.11.25
//static sf_wifi_cfg_t p_cfg; // 2020.09.15                         // 未使用のため削除

static char BSSID[6+1];                   // BSSID                  // sakaguchi 2021.11.25

/**
 * WIFI Thread entry function
 */
void wifi_thread_entry(void)
{
    // TODO: add your own code here
    ULONG   actual_events;
    ULONG   actual_status;        //2019.Dec.26 コメントアウト

    int link_status;
    uint32_t status;
    int sts;
    uint32_t ret = 0;
//    uint32_t err;
    int i;
    uint8_t p_mac[6];
    uint16_t err_count;
    uint32_t physical_msw; 
    uint32_t physical_lsw;
    uint16_t mtu_size = 0;        // MTU sakaguchi 2021.04.07


    //nx_common_init0();
    //nx_secure_common_init();
    
    Printf("#######  wifi thread start!!\r\n");

    while(1){
        init_sntp = 0;
        err_count = 0;
        wifi_faile = 0;

        tx_thread_sleep (1);

        while(1){
        
            Printf("   network  select %02X (%d)\r\n",  my_config.network.Phy[0], NetReboot);

            if(my_config.network.Phy[0] == ETHERNET)       // Ethernet
            {
                net_cfg.interface_index = 1;
                status = tx_event_flags_get (&event_flags_network, 0xffffffff,TX_OR_CLEAR, &actual_events,TX_NO_WAIT);

                status = nx_ip_interface_status_check(&g_ip0, 1 ,NX_IP_INITIALIZE_DONE, &actual_status, TX_NO_WAIT);
                Printf("Ethernet interface status %02X  %04X\r\n", status, actual_status);
                //if()
                Printf("   Ethernet attach start\r\n");

                status = nx_ip_interface_attach(&g_ip0, "ETH1", 0, 0, g_sf_el_nx_eth);

                // MTU
                //nx_ip_interface_mtu_set(&g_ip0, 1 , 1400);          // sakaguchi 2020.09.02
                mtu_size = *(uint16_t *)&my_config.network.Mtu[0];      // MTU追加 sakaguchi 2021.04.07
                if((mtu_size < 576) || (mtu_size > 1400))   mtu_size = 1400;
                nx_ip_interface_mtu_set(&g_ip0, 1 , mtu_size);

                Printf("Ethernet attach %02X\r\n", status);
                if(status == NX_SUCCESS || status == NX_NO_MORE_ENTRIES){
                    //ret = (my_config.network.DhcpEnable[0] == 0x00) ? connect_eth_static():connect_eth_dhcp();
                    if(my_config.network.DhcpEnable[0] == 0x00){
                        ret  = connect_eth_static();
                        DHCP_Status = 0;                            // 静的IPアドレス
                    }
                    else
                    {
                        Printf("Start Ethernet DHCP\r\n");
                        DHCP_Status = 3;                            // DHCP取得中
                        ret = connect_eth_dhcp();
                        if(ret != NX_SUCCESS){
                            DHCP_Status = 2;                            // DHCP取得失敗
                            NetTestCount++;
                            if(NetTestCount > 3)
                                NetTest = 1;
                        }else{
                            DHCP_Status = 1;                            // DHCP取得成功
                        }
                    }

                    Printf("    ### WIFI(ETH) Thread Status %d ###\r\n", ret);
                }
                else{
                    PutLog( LOG_LAN, "debug Eth Link %d", ret);
                    ret = NET_LINK_ERROR;                    // Link error
                }
            }
            else{                                   // WIFI
                 
                status = tx_event_flags_get (&event_flags_network, 0xffffffff,TX_OR_CLEAR, &actual_events,TX_NO_WAIT);
             

                if(check_wifi_para() != 0){
                    // 2021.03.02 hayashi WIFIのSSIDが設定していない場合に、macが出ない対策
                    net_address.eth.mac1 = net_address.eth.mac2 = 0; 
                    status = nx_ip_interface_physical_address_get(&g_ip0, 0, &physical_msw, &physical_lsw);
                    if(status == NX_SUCCESS){
                        net_address.eth.mac1 = physical_msw;
                        net_address.eth.mac2 = physical_lsw;
                    }
                    ret = NET_WIFI_PARAM_ERROR;
                }
                else{
                    //WIFI_RESET_ACTIVE;    //ここでOK？？？
                    //tx_thread_sleep (2); 
                    //WIFI_RESET_INACTIVE;
                    net_address.eth.mac1 = net_address.eth.mac2 = 0; 
                    status = nx_ip_interface_physical_address_get(&g_ip0, 0, &physical_msw, &physical_lsw);
                    if(status == NX_SUCCESS){
                        net_address.eth.mac1 = physical_msw;
                        net_address.eth.mac2 = physical_lsw;
                    }
                    Printf("wifi_faile %ld\r\n",wifi_faile );
                    Printf("MAC-0 %04X-%04X   %04X-%04X (%ld)\r\n", physical_msw,physical_lsw , net_address.eth.mac1,net_address.eth.mac2, status);                    
                    tx_thread_sleep (5); 
                    tx_thread_sleep (100);

                    // MTU
                    //nx_ip_interface_mtu_set(&g_ip0, 0, 1400);       // sakaguchi 2020.09.02
                    mtu_size = *(uint16_t *)&my_config.network.Mtu[0];      // MTU追加 sakaguchi 2021.04.07
                    if((mtu_size < 576) || (1400 < mtu_size))   mtu_size = 1400;
                    nx_ip_interface_mtu_set(&g_ip0, 0, mtu_size);
                    Printf("wifi for link\r\n");

                    link_status = wait_for_lnk_wifi();
                    if(link_status==0)
                    {

                        tx_thread_sleep (10);
                        for(i=0;i<10;i++){
                            sts = wifi_module_scan();
                            Printf("wifi scan status %02X (%d)\r\n", status,i);
                            tx_thread_sleep (1);
                            if(sts==0){
                                break;
                            }
                            if(sts != -1){
                                ret = NET_WIFI_SCAN_ERROR;                // WIFI Scan error
                                goto Exit;
                            }
                            tx_thread_sleep (50);    
                        }
                        tx_thread_sleep (10);

                        status = 0;         // ステルスの場合、SSIDが読めない
                        if(status == 0){
                            status = (uint32_t)wifi_module_provision(); // provision the Wi-Fi module
                            Printf("provison %02X\r\n", status);
                            if(status == 0){
                                if(my_config.network.DhcpEnable[0] == 0x00){
                                    ret = connect_wifi_static();
                                    DHCP_Status = 0;                            // 静的IPアドレス
                                }
                                else
                                {
                                    DHCP_Status = 3;                            // DHCP取得中
                                    ret = connect_wifi_dhcp();
                                    if(ret != NX_SUCCESS){
                                        DHCP_Status = 2;                            // DHCP取得失敗
                                        NetTestCount++;
                                        if(NetTestCount > 3)
                                            NetTest = 1;
                                    }else{
                                        DHCP_Status = 1;                            // DHCP取得成功
                                    }
                                }

                                //ret = (my_config.network.DhcpEnable[0] == 0x00) ? connect_wifi_static() : connect_wifi_dhcp();
                            }
                            else{
                                ret = NET_WIFI_PROV_ERROR;            // WIFI module provison set error
                                if(err_count < 5)
                                    //PutLog( LOG_LAN, "WLAN setup error %ld", err_count);
                                    PutLog( LOG_LAN, "WLAN setup error [%d][%ld]", status, err_count);        // sakaguchi 2021.08.02
                                tx_thread_sleep (500);      // 5sec 
                            } 
                        }
                        else{
                            ret = NET_WIFI_SCAN_ERROR;                // WIFI Scan error
                            PutLog( LOG_LAN, "WLAN AP not found");
                        }  

                    }
                    else{
                        ret = NET_LINK_ERROR;                    // Link error
                        //PutLog( LOG_LAN, "WLAN Link error %d");
                        PutLog( LOG_LAN, "WLAN Link error %d", link_status );           // sakaguchi 2021.08.02
                    }
                }

                Printf("    ### WIFI Thread Status %d ###\r\n", ret);
                
            }  // end if

Exit: 
            NetReboot = 0;

            switch(ret){
                case NET_SUCCESS:
                    g_sf_wifi0.p_api->macAddressGet(g_sf_wifi0.p_ctrl, (uint8_t *)&p_mac);
                    PutLog( LOG_LAN, "Network Connect Success" );
                    err_count = 0;
                    wifi_faile = 0;
                    Net_LOG_Write( 0, "%04X", 0 );              // クリア
                    status = tx_event_flags_set (&event_flags_network, FLG_DHCP_SUCCESS,TX_OR);
                    break;
                case NET_DNS_ERROR:
                    PutLog( LOG_LAN, "DNS error" );
                    Net_LOG_Write( 102, "%04X", ret );          // DNSエラー
                    status = tx_event_flags_set (&event_flags_network, FLG_DNS_ERROR | FLG_DHCP_ERROR,TX_OR);
                    break;
                case NET_SNTP_ERROR:
                    PutLog( LOG_LAN, "SNTP error" );
                    Net_LOG_Write( 103, "%04X", ret );          // SNTPエラー
                    status = tx_event_flags_set (&event_flags_network, FLG_SNTP_ERROR | FLG_DHCP_ERROR,TX_OR);
                    break;
                case NET_DHCP_ERROR:
                    err_count++;
                    if(err_count < 5)
                        PutLog( LOG_LAN, "DHCP error" );
                    Net_LOG_Write( 101, "%04X", ret );          // DHCPエラー
                    status = tx_event_flags_set (&event_flags_network, FLG_DHCP_ERROR,TX_OR);
                    break;
                case NET_LINK_ERROR:
                    Net_LOG_Write( 402, "%04X", ret );          // Linkエラー
                    status = tx_event_flags_set (&event_flags_network, FLG_DHCP_ERROR,TX_OR);
                    break;
                case NET_WIFI_PROV_ERROR:
                    Net_LOG_Write( 401, "%04X", ret );          // 無線LAN認証エラー
                    status = tx_event_flags_set (&event_flags_network, FLG_DHCP_ERROR,TX_OR);
                    break;
                case NET_WIFI_SCAN_ERROR:
// 2024 02 21 D.00.03.185 ↓
                    if((USB_CONNECT == isUSBState()) || (isBleConnect() == BLE_CONNECT)){
                        wifi_faile = 0;                             // カウントクリア
                    }else{
                        wifi_faile++;                               // 2023.12.26 D.00.03.183
                    }
// 2024 02 21 D.00.03.185 ↑
                    Net_LOG_Write( 400, "%04X", ret );          // 無線LANAP検出エラー
                    status = tx_event_flags_set (&event_flags_network, FLG_DHCP_ERROR,TX_OR);
                    break;
                case NET_WIFI_PARAM_ERROR:
                    PutLog( LOG_LAN, "WLAN Parameter error" );
                    status = tx_event_flags_set (&event_flags_network, FLG_DHCP_ERROR,TX_OR);
                    break;
                default:
                    err_count++;
                    if(err_count < 5)
                        PutLog( LOG_LAN, "Network error %d", ret );
                    Net_LOG_Write( 100, "%04X", ret );          // 設定エラー
                    status = tx_event_flags_set (&event_flags_network, FLG_DHCP_ERROR,TX_OR);
                    break;
            }
  
            // Test Reboot
            //if(wifi_faile > 25 || tls_error > 5){
            //if(wifi_faile > 10 || tls_error > 3 || err_count > 10){
            //if(wifi_faile > 5 || tls_error > 3 || err_count > 10){          // ★ 2022.04.11 Wifiエラー時のリブートまでのリトライ回数を変更
            if(wifi_faile > 4 || tls_error > 3 || err_count > 10){                                                  // 2023.12.26 D.00.03.183
                PutLog( LOG_LAN, "Network error wifi[%d] tls[%d] err[%d]", wifi_faile, tls_error, err_count);       // 2023.12.26 D.00.03.183
#ifdef DBG_TERM
                PutLog( LOG_LAN, "Debug Network error Reboot System");
#endif
                DebugLog( LOG_DBG, "Debug Network error Reboot System w:%ld t:%ld)", wifi_faile, tls_error);
                //tx_event_flags_get (&event_flags_reboot, FLG_REBOOT_REQUEST ,TX_OR_CLEAR, &actual_events,TX_NO_WAIT);
                //Printf("\r\n   Reboot Request (%04X) !!!!!\r\n\r\n", actual_events);
                tx_event_flags_set(&event_flags_reboot, FLG_REBOOT_EXEC, TX_OR);        //リブート実行イベントシグナル セット
                //tx_thread_sleep (50);
            }
            
            Printf("==> WIFI thread suspend \r\n");
            
            tx_thread_suspend(&wifi_thread);

        }  // while end


    }   // while end
 
}

/**
 * Ethrnet connect DHCP
 * @return      NET_LINK_EEROR
 *              NET_DNS_ERROR
 *              NET_DHCP_ERROR
 */
static uint32_t connect_eth_dhcp(void)
{
    ULONG   ip0_ip_address = 0;
    ULONG   ip0_mask = 0;
    ULONG   gateway = 0;
    UINT ip_status;
    uint32_t physical_msw; 
    uint32_t physical_lsw;
    uint32_t status,ret = 0;
    ULONG   actual_status;
    int      try = 100 ;
    UCHAR retry_cnt = 0;
    UINT opt_len;
//    UCHAR   dns_ip[4*2];            // セカンダリDNS対応
    UCHAR   dns_ip[4*10];            // セカンダリDNS対応    // DNSサーバアドレスを10個まで受信可能とする   // 2023.02.20

    memset(dns_ip,0x00,sizeof(dns_ip));                 // sakaguchi 2020.08.31


    
    Printf("    connect eth dhcp & Wait Link up\r\n");
    do{
        try--;
        if(Link_Status==1){
            Printf("    ===>  Link UP !! (%d)\r\n",try);
            break;
        }

        tx_thread_sleep(10);
       
    }while(try>0);
    

    if(Link_Status==0){
        //PutLog( LOG_LAN, "debug Eth DHCP Link 3 %d/%08X", status, actual_status);
        return(NET_LINK_ERROR);   
    }

    tx_thread_sleep(50);
    try = 100;
    // この後時間が掛かる場合が有る
    do{
        try--;
        if(try==0){
            Printf("   LINK Error !! \r\n");
            //PutLog( LOG_LAN, "debug Eth DHCP Link 2 %d/%08X", status, actual_status);
            return(NET_LINK_ERROR);    
        }
        // Make sure we have a valid network link
        Printf("   LINK Check(%d)\r\n",try);
        status =  nx_ip_interface_status_check(&g_ip0, 1, NX_IP_LINK_ENABLED | NX_IP_INITIALIZE_DONE, &actual_status, NX_NO_WAIT);
        
        Printf("   LINK Status %02X %02X \r\n", status ,actual_status);
        if((actual_status & NX_IP_LINK_ENABLED) == NX_IP_LINK_ENABLED)
            break;
        tx_thread_sleep (50);               // 10 -->　50  2020 8/28        
    } while (1);    //status != NX_SUCCESS);


        

    if((actual_status & NX_IP_LINK_ENABLED) != NX_IP_LINK_ENABLED){
        //PutLog( LOG_LAN, "debug Eth DHCP Link 3 %d/%08X", status, actual_status);
        return(NET_LINK_ERROR);    
    }


    physical_msw = fact_config.mac_h;
    physical_lsw = fact_config.mac_l;
 //   status = nx_ip_interface_physical_address_set(&g_ip0, 1, &physical_msw, &physical_lsw, NX_TRUE);        //2019.Dec.26 bug?
    status = nx_ip_interface_physical_address_set(&g_ip0, 1, physical_msw, physical_lsw, NX_TRUE);        //2019.Dec.26 bug?

    Printf("MAC Set Status %04X \r\n",status); 


    status = nx_dhcp_interface_enable(&g_dhcp_client0, 1);
    Printf("## Ethernet DHCP interface enable 1 %02X\r\n", status);
    if (NX_SUCCESS == status){

        // DHCPブロードキャストフラグOFF    // sakaguchi 2021.07.16
        nx_dhcp_interface_clear_broadcast_flag(&g_dhcp_client0, 1, NX_TRUE);

        status = nx_dhcp_interface_start (&g_dhcp_client0, 1);
        Printf("    nx_dhcp_interface_start. ETH %02X\r\n",status);
        if(!((status == NX_SUCCESS) || (status == NX_DHCP_ALREADY_STARTED)))
        {
            Printf ("\r\nStart DHCP client failed!");
            goto EXIT_ERR;
        }
        
        if(status == NX_DHCP_ALREADY_STARTED){
            Printf("<<<< DHCP Already Satrt !!!>>>\r\n");
        }

        /*
        do{    
            // status       0x43    NX_NOT_SUCCESSFUL (Time out)
            // ip_status    0x02    NX_IP_ADDRESS_RESOLVED    
            status = nx_ip_interface_status_check(&g_ip0, 1, NX_IP_ADDRESS_RESOLVED, (ULONG *) &ip_status, 2000);    // 1000->2000  2020 8/28
                 
            retry_cnt++;
            if((status == NX_SUCCESS) && (ip_status == NX_IP_ADDRESS_RESOLVED )){
                Printf("   IP Address Resolved !!");
                break;
            }
            ///@todo 要チェック @bug 要チェック
            Printf("DHCP ip check %04X  %04X (%d)\r\n", status, ip_status, retry_cnt);
            tx_thread_sleep(5);     //仮
        }while((retry_cnt < MAX_RETRY_CNT));
        

        Printf(" DHCP Try !! %d %02X %02X\r\n", retry_cnt, status, ip_status);

        if(retry_cnt >= MAX_RETRY_CNT)
        {
            PutLog(LOG_LAN, "DHCP Error %ld", status);
            Printf ("\r\nIP Address is not resolved!  1 %d %02X \r\n",retry_cnt, status);
             
            goto EXIT_ERR;
        } 
        */

        // status       0x43    NX_NOT_SUCCESSFUL (Time out)
        // ip_status    0x02    NX_IP_ADDRESS_RESOLVED    
        status = nx_ip_interface_status_check(&g_ip0, 1, NX_IP_ADDRESS_RESOLVED, (ULONG *) &ip_status, 4000);    // 1000->2000  2020 8/28
                 
        retry_cnt++;
        if((status == NX_SUCCESS) && (ip_status == NX_IP_ADDRESS_RESOLVED )){
            ;
        }
        else{
            PutLog(LOG_LAN, "DHCP Error %ld", status);
            Printf ("\r\nIP Address is not resolved!  1 %d %02X \r\n",retry_cnt, status);
            goto EXIT_ERR;
        }


        // retrieve the IP address
        status =  nx_ip_interface_address_get(&g_ip0, 1, &ip0_ip_address,&ip0_mask);
        if (status != NX_SUCCESS)
        {
            Printf ("\r\nIP Address is not received. 2 \r\n");
            goto EXIT_ERR;    
        }     

        net_address.eth.address = net_address.active.address = ip0_ip_address;
        net_address.eth.mask = net_address.active.mask = ip0_mask;

        if(TestMode == TEST_MODE){
            PutLog(LOG_LAN,"DHCP E-Test $S (%d.%d.%d.%d)",
                (int)(ip0_ip_address>>24), (int)(ip0_ip_address>>16)&0xFF, (int)(ip0_ip_address>>8)&0xFF, (int)(ip0_ip_address)&0xFF
            );    
            NetTest = 0;
            return (NX_SUCCESS);
        }

        //Gateway address get
        status = nx_ip_gateway_address_get(&g_ip0, &gateway);
        Printf("   Gateway address %d\r\n", status);
        if(NX_SUCCESS != status){
            Printf ("\r\nGateway Address is not received. 2 \r\n");
            goto EXIT_ERR;    
        }

        net_address.eth.gateway = net_address.active.gateway = gateway;

        // get dns-server address form dns client
        opt_len = sizeof(dns_ip);
        status = nx_dhcp_interface_user_option_retrieve(&g_dhcp_client0, 1 ,NX_DHCP_OPTION_DNS_SVR,dns_ip, &opt_len);
		Printf("   dns server address %d\r\n", status);
        if(NX_SUCCESS != status){
            Printf ("\r\nDNS Address is not received. 2 \r\n");
            goto EXIT_ERR;  
        }

        net_address.eth.dns1 = net_address.active.dns1 = (uint32_t)dns_ip[0] + ((uint32_t)(dns_ip[1]) << 8) + ((uint32_t)(dns_ip[2]) << 16) + ((uint32_t)(dns_ip[3]) << 24);
        net_address.eth.dns2 = net_address.active.dns2 = (uint32_t)dns_ip[4] + ((uint32_t)(dns_ip[5]) << 8) + ((uint32_t)(dns_ip[6]) << 16) + ((uint32_t)(dns_ip[7]) << 24);
// 2023.02.20 ↓
        net_address.eth.dns3 = net_address.active.dns3 = (uint32_t)dns_ip[8] + ((uint32_t)(dns_ip[9]) << 8) + ((uint32_t)(dns_ip[10]) << 16) + ((uint32_t)(dns_ip[11]) << 24);
        net_address.eth.dns4 = net_address.active.dns4 = (uint32_t)dns_ip[12] + ((uint32_t)(dns_ip[13]) << 8) + ((uint32_t)(dns_ip[14]) << 16) + ((uint32_t)(dns_ip[15]) << 24);
        net_address.eth.dns5 = net_address.active.dns5 = (uint32_t)dns_ip[16] + ((uint32_t)(dns_ip[17]) << 8) + ((uint32_t)(dns_ip[18]) << 16) + ((uint32_t)(dns_ip[19]) << 24);
// 2023.02.20 ↑

        Printf("IP  ADDRESS is: %d.%d.%d.%d \n\r", (int)(ip0_ip_address>>24), (int)(ip0_ip_address>>16)&0xFF, (int)(ip0_ip_address>>8)&0xFF, (int)(ip0_ip_address)&0xFF );
        Printf("NET  MASK   is: %d.%d.%d.%d \n\r", (int)(ip0_mask>>24), (int)(ip0_mask>>16)&0xFF, (int)(ip0_mask>>8)&0xFF, (int)(ip0_mask)&0xFF );
        Printf("Gate  Way   is: %d.%d.%d.%d \n\r", (int)(gateway>>24), (int)(gateway>>16)&0xFF, (int)(gateway>>8)&0xFF, (int)(gateway)&0xFF );
        Printf("IP  ADDRESS is: %d.%d.%d.%d \n\r", (int)(net_address.active.address>>24), (int)(net_address.active.address>>16)&0xFF, (int)(net_address.active.address>>8)&0xFF, (int)(net_address.active.address)&0xFF );
        Printf("DNS ADDRESS is: %d.%d.%d.%d \n\r", (int)(net_address.active.dns1>>24), (int)(net_address.active.dns1>>16)&0xFF, (int)(net_address.active.dns1>>8)&0xFF, (int)(net_address.active.dns1)&0xFF );
        Printf("DNS2 ADDRESS is: %d.%d.%d.%d \n\r", (int)(net_address.active.dns2>>24), (int)(net_address.active.dns2>>16)&0xFF, (int)(net_address.active.dns2>>8)&0xFF, (int)(net_address.active.dns2)&0xFF ); // sakaguchi 2020.08.31

        if(NetReboot == 0){
            PutLog(LOG_LAN,"DHCP $S (%d.%d.%d.%d/%d.%d.%d.%d/%d.%d.%d.%d)",
                (int)(ip0_ip_address>>24), (int)(ip0_ip_address>>16)&0xFF, (int)(ip0_ip_address>>8)&0xFF, (int)(ip0_ip_address)&0xFF,
                (int)(ip0_mask>>24), (int)(ip0_mask>>16)&0xFF, (int)(ip0_mask>>8)&0xFF, (int)(ip0_mask)&0xFF ,
                (int)(gateway>>24), (int)(gateway>>16)&0xFF, (int)(gateway>>8)&0xFF, (int)(gateway)&0xFF
            );
        }       
        DebugLog(LOG_DBG,"E-DHCP $S (%d.%d.%d.%d/%d.%d.%d.%d/%d.%d.%d.%d)",
                (int)(ip0_ip_address>>24), (int)(ip0_ip_address>>16)&0xFF, (int)(ip0_ip_address>>8)&0xFF, (int)(ip0_ip_address)&0xFF,
                (int)(ip0_mask>>24), (int)(ip0_mask>>16)&0xFF, (int)(ip0_mask>>8)&0xFF, (int)(ip0_mask)&0xFF ,
                (int)(gateway>>24), (int)(gateway>>16)&0xFF, (int)(gateway>>8)&0xFF, (int)(gateway)&0xFF
        );       

     
        if(TestMode==0){
            status = start_up_dns();
            Printf("     dns status %02X\r\n", status);
            //ret = (status != NX_SUCCESS) ? NET_DNS_ERROR : start_up_sntp();             // DNS error
            if(status != NX_SUCCESS){       // 2020/09/29
                ret = NET_DNS_ERROR;
            }
            else{
                if(my_config.device.TimeSync[0] != 0 &&  NetReboot == 0){
                    ret = start_up_sntp();
                }
                else{
                    ret = NET_SUCCESS;
                }
            }
        }
    }
    else{
        ret = NET_DHCP_ERROR;        // DHCP error
        goto EXIT_ERR;      // debug 06 19
    }
//EXIT:
    return (ret);
EXIT_ERR:

    status = nx_dhcp_stop(&g_dhcp_client0);
    Printf("dhcp stop status %02X\r\n", status);
    status = nx_dhcp_reinitialize(&g_dhcp_client0);             // sakaguchi 2020.09.14 
    Printf("dhcp reinitialize status %02X\r\n", status);
    status = nx_dhcp_interface_disable(&g_dhcp_client0, 1);
    Printf("dhcp disable status %02X\r\n", status);
    ret = NET_DHCP_ERROR;    

    return (ret);



    /*
    status = nx_dhcp_interface_enable(&g_dhcp_client0, 1);
    Printf("## Ethernet DHCP interface enable 1 %02X\r\n", status);
    if ((NX_SUCCESS == status) || (NX_DHCP_INTERFACE_ALREADY_ENABLED == status)){
        status = (uint32_t)start_dhcp_client(1);
        Printf("    start dhcp client status %02X\r\n", status);
        if(NX_SUCCESS == status){

            if(TestMode==0){    
            status = start_up_dns();
            Printf("     dns status %02X\r\n", status);
            ret = (status != NX_SUCCESS) ? NET_DNS_ERROR : start_up_sntp();        // DNS error
        }
        }
        else{
            Printf("\r\n  dhcp Failed.\r\n");
            ret = NET_DHCP_ERROR;    // DHCP Error
        }
    }

    return (ret);
    */
}


/**
 * Ethrnet connect Static Address
 * @return
 */
static uint32_t connect_eth_static(void)
{


    uint32_t status,ret = NET_SUCCESS;
    uint32_t physical_msw; 
    uint32_t physical_lsw;

    uint32_t try = 10;
//    uint32_t i;       //2019.Dec.26 コメントアウト
    uint32_t   actual_status;
    uint32_t   ip_address;
    uint32_t   mask;
    uint32_t   gateway;
    uint32_t   dns;
    uint32_t   dns2;                    // セカンダリDNS追加 sakaguchi 2021.04.07
    uint32_t   ip0_ip_address = 0;
    uint32_t   ip0_mask = 0;
    NX_PACKET *response_ptr;            // sakaguchi 2020.09.17
   

    Printf("       Ethrnet Static Address \r\n");
    status = nx_dhcp_interface_disable(&g_dhcp_client0, 1);
    Printf("       dhcp interface disable  %02X\r\n", status);

    tx_thread_sleep (10);    
    do{
        try--;
        if(try==0){
            Printf("   LINK Error !! \r\n");
            return(NET_LINK_ERROR);    
        }
        // Make sure we have a valid network link
        status =  nx_ip_interface_status_check(&g_ip0, 1, NX_IP_LINK_ENABLED, &actual_status, 100);
        
        tx_thread_sleep (10);        
    } while (status != NX_SUCCESS);

    if(actual_status != NX_IP_LINK_ENABLED){
        return(NET_LINK_ERROR);    
    }


    physical_msw = fact_config.mac_h;
    physical_lsw = fact_config.mac_l;
    status = nx_ip_interface_physical_address_set(&g_ip0, 1, physical_msw, physical_lsw, NX_TRUE);        //2019.Dec.26 bug?
    Printf("MAC Set Status %04X \r\n",status); 

    ip_address  =  AdrStr2Long(&my_config.network.IpAddrss[0]);
    gateway     =  AdrStr2Long(&my_config.network.GateWay[0]);
    mask        =  AdrStr2Long(&my_config.network.SnMask[0]);
    dns         =  AdrStr2Long(&my_config.network.Dns1[0]);
    dns2        =  AdrStr2Long(&my_config.network.Dns2[0]);                     // セカンダリDNS追加 sakaguchi 2021.04.07

    Printf("           IP ADDRESS is: %d.%d.%d.%d \n\r", (int)(ip_address>>24), (int)(ip_address>>16)&0xFF, (int)(ip_address>>8)&0xFF, (int)(ip_address)&0xFF );
    Printf("           MASK       is: %d.%d.%d.%d \n\r", (int)(mask>>24), (int)(mask>>16)&0xFF, (int)(mask>>8)&0xFF, (int)(mask)&0xFF );
    Printf("           GateWay    is: %d.%d.%d.%d \n\r", (int)(gateway>>24), (int)(gateway>>16)&0xFF, (int)(gateway>>8)&0xFF, (int)(gateway)&0xFF );
    Printf("           DNS        is: %d.%d.%d.%d \n\r", (int)(dns>>24), (int)(dns>>16)&0xFF, (int)(dns>>8)&0xFF, (int)(dns)&0xFF );
    Printf("           DNS2       is: %d.%d.%d.%d \n\r", (int)(dns2>>24), (int)(dns2>>16)&0xFF, (int)(dns2>>8)&0xFF, (int)(dns2)&0xFF );

    status = nx_ip_interface_address_set(&g_ip0, 1, ip_address, mask);
    if(NX_SUCCESS != status)
    {
         Printf("IP Address set error %d \n", status);
         ret = NET_STATIC_IP_ERROR;
    }
    else{

        status = nx_ip_gateway_address_set(&g_ip0, gateway);
        if(NX_SUCCESS != status)
        {
            Printf("Gateway Address set error %d \n", status);
            ret = NET_STATIC_IP_ERROR;
        }
        else
        {
// sakaguchi 2020.09.17 ↓   // sakaguchi 2021.07.16 ↓
            // GARP送信
            //status = nx_arp_gratuitous_send(&g_ip0, NX_NULL); 
            // gatewayにping送信
            status = nx_icmp_ping(&g_ip0, gateway, "1", 1, &response_ptr, 100);
            if(NX_SUCCESS != status){
                // 処理無し
            }else{
                nx_packet_release(response_ptr);    // リリースが必要
            }
            Printf("ping to Gateway %02X \n", status);
// sakaguchi 2020.09.17 ↑   // sakaguchi 2021.07.16 ↑

            net_address.eth.address = net_address.active.address = ip_address;
            net_address.eth.mask = net_address.active.mask = mask;
            net_address.eth.gateway = net_address.active.gateway = gateway;
            net_address.eth.dns1 = net_address.active.dns1 = dns;
            net_address.eth.dns2 = net_address.active.dns2 = dns2;                   // セカンダリDNS追加 sakaguchi 2021.04.07

            nx_ip_interface_address_get(&g_ip0, 1, &ip0_ip_address,&ip0_mask);
            Printf("Get IP ADDRESS is: %d.%d.%d.%d \n\r", (int)(ip0_ip_address>>24), (int)(ip0_ip_address>>16)&0xFF, (int)(ip0_ip_address>>8)&0xFF, (int)(ip0_ip_address)&0xFF );
            if(NetReboot == 0){
                PutLog(LOG_LAN,"Static IP $S (%d.%d.%d.%d/%d.%d.%d.%d/%d.%d.%d.%d)",
                    (int)(ip0_ip_address>>24), (int)(ip0_ip_address>>16)&0xFF, (int)(ip0_ip_address>>8)&0xFF, (int)(ip0_ip_address)&0xFF,
                    (int)(ip0_mask>>24), (int)(ip0_mask>>16)&0xFF, (int)(ip0_mask>>8)&0xFF, (int)(ip0_mask)&0xFF ,
                    (int)(gateway>>24), (int)(gateway>>16)&0xFF, (int)(gateway>>8)&0xFF, (int)(gateway)&0xFF
                );          
            } 
            tx_thread_sleep(50);

            status = start_up_dns();
            Printf("     dns status %02X\r\n", status);
            //ret = (status != NX_SUCCESS) ? NET_DNS_ERROR : start_up_sntp();             // DNS error
            if(status != NX_SUCCESS){       // 2020/09/29
                ret = NET_DNS_ERROR;
            }
            else{
                if(my_config.device.TimeSync[0] != 0 &&  NetReboot == 0){
                    ret = start_up_sntp();
                }
                else{
                    ret = NET_SUCCESS;
                }
            }
        }

    }

    DebugLog(LOG_DBG,"E-S $S (%d.%d.%d.%d/%d.%d.%d.%d/%d.%d.%d.%d)",
                (int)(ip_address>>24), (int)(ip_address>>16)&0xFF, (int)(ip_address>>8)&0xFF, (int)(ip_address)&0xFF,
                (int)(mask>>24), (int)(mask>>16)&0xFF, (int)(mask>>8)&0xFF, (int)(mask)&0xFF ,
                (int)(gateway>>24), (int)(gateway>>16)&0xFF, (int)(gateway>>8)&0xFF, (int)(gateway)&0xFF
        );

    return (ret);

}


/**
 * connect_wifi_dhcp
 * @return
 */
static uint32_t connect_wifi_dhcp(void)
{
    uint32_t status;
    uint32_t ret = NET_SUCCESS;
    UINT ip_status;
    ULONG   ip0_ip_address = 0;
    ULONG   ip0_mask = 0;
    ULONG   gateway = 0;
    UINT opt_len;
//    UCHAR   dns_ip[4*2];            // セカンダリDNS対応
    UCHAR   dns_ip[4*10];            // セカンダリDNS対応    // DNSサーバアドレスを10個まで受信可能とする   // 2023.02.20
    UCHAR retry_cnt = 0;

    memset(dns_ip,0x00,sizeof(dns_ip));                 // sakaguchi 2020.08.31

    status = nx_dhcp_interface_enable(&g_dhcp_client0, 0);
    Printf("## WIFI DHCP interface enable 1 %02X\r\n", status);
   
    //if ((NX_SUCCESS == status) || (NX_DHCP_INTERFACE_ALREADY_ENABLED == status)){
    if (NX_SUCCESS == status){

        // DHCPブロードキャストフラグOFF    // sakaguchi 2021.07.16
        nx_dhcp_interface_clear_broadcast_flag(&g_dhcp_client0, 0, NX_TRUE);

        Printf("    start_dhcp_client. WIFI \r\n");
        // start DHCP client 
//       status = nx_dhcp_start (&g_dhcp_client0);
        status = nx_dhcp_interface_start (&g_dhcp_client0, 0);          // sakaguchi 2020.08.31
        Printf("   nx_dhcp_start  %04X \r\n", status);
        if(!((status == NX_SUCCESS) || (status == NX_DHCP_ALREADY_STARTED)))
        {
            //status = nx_dhcp_interface_disable(&g_dhcp_client0, 0);
            //Printf("dhcp disable status %02X\r\n", status);
            Printf ("\r\nStart DHCP client failed!");
            //ret = NET_DHCP_ERROR;        // DHCP error
            DebugLog( LOG_DBG, "Start DHCP client failed");
            goto EXIT_ERR;
        }
        
        if(status == NX_DHCP_ALREADY_STARTED){
            Printf("<<<< DHCP Already Satrt !!!>>>\r\n");
        }

        do{    
            // status       0x43    NX_NOT_SUCCESSFUL (Time out)
            // ip_status    0x02    NX_IP_ADDRESS_RESOLVED    
            //status = nx_ip_status_check(&g_ip0, NX_IP_ADDRESS_RESOLVED, (ULONG *) &ip_status, 1000);
//            status = nx_ip_status_check(&g_ip0, NX_IP_ADDRESS_RESOLVED, (ULONG *) &ip_status, 1000);
//            status = nx_ip_interface_status_check(&g_ip0, 0, NX_IP_ADDRESS_RESOLVED, (ULONG *) &ip_status, 5000);   // sakaguchi 2020.08.31
            status = nx_ip_interface_status_check(&g_ip0, 0, NX_IP_ADDRESS_RESOLVED, (ULONG *) &ip_status, 1000);   // sakaguchi 2020.08.31
            retry_cnt++;
            if((status == NX_SUCCESS) && (ip_status == NX_IP_ADDRESS_RESOLVED )){
                Printf("   IP Address Resolved !!");
                break;
            }
            ///@todo 要チェック @bug 要チェック
            Printf("DHCP ip check %04X  %04X (%d)\r\n", status, ip_status, retry_cnt);

            // リンクダウンを検出したらエラー終了 2022.04.11
            if(Link_Status == LINK_DOWN){
                Printf("DHCP LinkDown %04X  %04X (%d)\r\n", status, ip_status, retry_cnt);
                status = nx_dhcp_interface_stop (&g_dhcp_client0, 0);
                Printf("nx_dhcp_interface_stop %04X\r\n", status);
                goto EXIT_ERR;
            }
            tx_thread_sleep(5);     //仮
        }while(/*(status != NX_SUCCESS) && (ip_status != NX_IP_ADDRESS_RESOLVED )&& */(retry_cnt < MAX_RETRY_CNT));
        
         // NX_NOT_SUCCESSFUL 0x43  time out 

        Printf(" DHCP Try !! %d %02X %02X\r\n", retry_cnt, status, ip_status);

        if(retry_cnt >= MAX_RETRY_CNT)
        {
            PutLog(LOG_LAN, "DHCP Error %ld", status);
            Printf ("\r\nIP Address is not resolved!  1 %d %02X \r\n",retry_cnt, status);
             
            goto EXIT_ERR;
        }
        
        // retrieve the IP address
        status =  nx_ip_interface_address_get(&g_ip0, 0, &ip0_ip_address,&ip0_mask);
        if (status != NX_SUCCESS)
        {
            Printf ("\r\nIP Address is not received. 2 \r\n");
            goto EXIT_ERR;    
        }        
        
        net_address.wifi.address = net_address.active.address = ip0_ip_address;
        net_address.wifi.mask = net_address.active.mask = ip0_mask;
        
        Printf("IP  ADDRESS is: %d.%d.%d.%d \n\r", (int)(net_address.active.address>>24), (int)(net_address.active.address>>16)&0xFF, (int)(net_address.active.address>>8)&0xFF, (int)(net_address.active.address)&0xFF );

        if(TestMode == TEST_MODE){
            PutLog(LOG_LAN,"DHCP W-Test $S (%d.%d.%d.%d)",
                (int)(ip0_ip_address>>24), (int)(ip0_ip_address>>16)&0xFF, (int)(ip0_ip_address>>8)&0xFF, (int)(ip0_ip_address)&0xFF
            ); 
            NetTest = 0;
            return (NX_SUCCESS);
        }
        //Gateway address get
        status = nx_ip_gateway_address_get(&g_ip0, &gateway);
        Printf("   Gateway address %d\r\n", status);
        if(NX_SUCCESS != status){
            Printf ("\r\nGateWay Address is not received. 2 \r\n");
            goto EXIT_ERR;    
        }

        net_address.wifi.gateway = net_address.active.gateway = gateway;
        
        // get dns-server address form dns client
        opt_len = sizeof(dns_ip);
        status = nx_dhcp_interface_user_option_retrieve(&g_dhcp_client0, 0 ,NX_DHCP_OPTION_DNS_SVR,dns_ip, &opt_len);
		Printf("   dns server address %d\r\n", status);
        if(NX_SUCCESS != status){
            Printf ("\r\nDNS Address is not received. 2 \r\n");
            goto EXIT_ERR;  
        }

        net_address.wifi.dns1 = net_address.active.dns1 = (uint32_t)dns_ip[0] + ((uint32_t)(dns_ip[1]) << 8) + ((uint32_t)(dns_ip[2]) << 16) + ((uint32_t)(dns_ip[3]) << 24);
        net_address.wifi.dns2 = net_address.active.dns2 = (uint32_t)dns_ip[4] + ((uint32_t)(dns_ip[5]) << 8) + ((uint32_t)(dns_ip[6]) << 16) + ((uint32_t)(dns_ip[7]) << 24);           // sakaguchi 2020.08.31
// 2023.02.20 ↓
        net_address.wifi.dns3 = net_address.active.dns3 = (uint32_t)dns_ip[8] + ((uint32_t)(dns_ip[9]) << 8) + ((uint32_t)(dns_ip[10]) << 16) + ((uint32_t)(dns_ip[11]) << 24);
        net_address.wifi.dns4 = net_address.active.dns4 = (uint32_t)dns_ip[12] + ((uint32_t)(dns_ip[13]) << 8) + ((uint32_t)(dns_ip[14]) << 16) + ((uint32_t)(dns_ip[15]) << 24);
        net_address.wifi.dns5 = net_address.active.dns5 = (uint32_t)dns_ip[16] + ((uint32_t)(dns_ip[17]) << 8) + ((uint32_t)(dns_ip[18]) << 16) + ((uint32_t)(dns_ip[19]) << 24);
// 2023.02.20 ↑

        Printf("IP  ADDRESS is: %d.%d.%d.%d \n\r", (int)(ip0_ip_address>>24), (int)(ip0_ip_address>>16)&0xFF, (int)(ip0_ip_address>>8)&0xFF, (int)(ip0_ip_address)&0xFF );
        Printf("NET  MASK   is: %d.%d.%d.%d \n\r", (int)(ip0_mask>>24), (int)(ip0_mask>>16)&0xFF, (int)(ip0_mask>>8)&0xFF, (int)(ip0_mask)&0xFF );
        Printf("Gate  Way   is: %d.%d.%d.%d \n\r", (int)(gateway>>24), (int)(gateway>>16)&0xFF, (int)(gateway>>8)&0xFF, (int)(gateway)&0xFF );
        Printf("IP  ADDRESS is: %d.%d.%d.%d \n\r", (int)(net_address.active.address>>24), (int)(net_address.active.address>>16)&0xFF, (int)(net_address.active.address>>8)&0xFF, (int)(net_address.active.address)&0xFF );
        Printf("DNS ADDRESS is: %d.%d.%d.%d \n\r", (int)(net_address.active.dns1>>24), (int)(net_address.active.dns1>>16)&0xFF, (int)(net_address.active.dns1>>8)&0xFF, (int)(net_address.active.dns1)&0xFF );
        Printf("DNS2 ADDRESS is: %d.%d.%d.%d \n\r", (int)(net_address.active.dns2>>24), (int)(net_address.active.dns2>>16)&0xFF, (int)(net_address.active.dns2>>8)&0xFF, (int)(net_address.active.dns2)&0xFF );     // sakaguchi 2020.08.31

        if(NetReboot == 0){
            PutLog(LOG_LAN,"DHCP $S (%d.%d.%d.%d/%d.%d.%d.%d/%d.%d.%d.%d)",
                (int)(ip0_ip_address>>24), (int)(ip0_ip_address>>16)&0xFF, (int)(ip0_ip_address>>8)&0xFF, (int)(ip0_ip_address)&0xFF,
                (int)(ip0_mask>>24), (int)(ip0_mask>>16)&0xFF, (int)(ip0_mask>>8)&0xFF, (int)(ip0_mask)&0xFF ,
                (int)(gateway>>24), (int)(gateway>>16)&0xFF, (int)(gateway>>8)&0xFF, (int)(gateway)&0xFF
            );       
        }
        DebugLog(LOG_DBG,"W-DHCP $S (%d.%d.%d.%d/%d.%d.%d.%d/%d.%d.%d.%d)",
                (int)(ip0_ip_address>>24), (int)(ip0_ip_address>>16)&0xFF, (int)(ip0_ip_address>>8)&0xFF, (int)(ip0_ip_address)&0xFF,
                (int)(ip0_mask>>24), (int)(ip0_mask>>16)&0xFF, (int)(ip0_mask>>8)&0xFF, (int)(ip0_mask)&0xFF ,
                (int)(gateway>>24), (int)(gateway>>16)&0xFF, (int)(gateway>>8)&0xFF, (int)(gateway)&0xFF
        );      

        //status = nx_dhcp_stop(&g_dhcp_client0);
        //Printf("dhcp stop status %02X\r\n", status);
        //status = nx_dhcp_interface_disable(&g_dhcp_client0, 0);
        //("dhcp disable status %02X\r\n", status);

     
        if(TestMode==0){
            status = start_up_dns();
            Printf("     dns status %02X\r\n", status);
            //ret = (status != NX_SUCCESS) ? NET_DNS_ERROR : start_up_sntp();             // DNS error
            if(status != NX_SUCCESS){       // 2020/09/29
                ret = NET_DNS_ERROR;
            }
            else{
                if(my_config.device.TimeSync[0] != 0 &&  NetReboot == 0){
                    ret = start_up_sntp();
                }
                else{
                    ret = NET_SUCCESS;
                }
            }
        }
    }
    else{
        ret = NET_DHCP_ERROR;       // DHCP error
        goto EXIT_ERR;              // debug 06 19
    }
//EXIT:
    return (ret);
EXIT_ERR:

    status = nx_dhcp_stop(&g_dhcp_client0);
    Printf("dhcp stop status %02X\r\n", status);
    status = nx_dhcp_reinitialize(&g_dhcp_client0);             // sakaguchi 2020.09.14 
    Printf("dhcp reinitialize status %02X\r\n", status);
    status = nx_dhcp_interface_disable(&g_dhcp_client0, 0);
    Printf("dhcp disable status %02X\r\n", status);
    ret = NET_DHCP_ERROR;    

    return (ret);
}

/**
 *  WIFI connect Static Address
 * @return
 */
static uint32_t connect_wifi_static(void)
{
    uint32_t status,ret = NET_SUCCESS;
 //   uint32_t try = 10 ,i;     //2019.Dec.26 コメントアウト
//    ULONG   actual_status;        //2019.Dec.26 コメントアウト
    ULONG   ip_address;
    ULONG   mask;
    ULONG   gateway;
    ULONG   dns;
    ULONG   dns2;                       // セカンダリDNS追加 sakaguchi 2021.04.07
    ULONG   ip0_ip_address = 0;
    ULONG   ip0_mask = 0;
    NX_PACKET *response_ptr;            // sakaguchi 2020.09.17

    Printf("       WIFI Static Address \r\n");
    status = nx_dhcp_interface_disable(&g_dhcp_client0, 0);
    Printf("       WIFI dhcp interface disable  %02X\r\n", status);

    tx_thread_sleep (10);    
/*
    do{
        try--;
        if(try==0){
            Printf("   LINK Error !! \r\n");
            return(NET_LINK_ERROR);    
        }
        // Make sure we have a valid network link
        status =  nx_ip_status_check(&g_ip0, NX_IP_LINK_ENABLED, &actual_status, 100);

        tx_thread_sleep (10);        
    } while (status != NX_SUCCESS);
*/
    ip_address  =  AdrStr2Long(&my_config.network.IpAddrss[0]);
    gateway     =  AdrStr2Long(&my_config.network.GateWay[0]);
    mask        =  AdrStr2Long(&my_config.network.SnMask[0]);
    dns         =  AdrStr2Long(&my_config.network.Dns1[0]);
    dns2        =  AdrStr2Long(&my_config.network.Dns2[0]);             // セカンダリDNS追加 sakaguchi 2021.04.07

    Printf("           IP ADDRESS is: %d.%d.%d.%d \n\r", (int)(ip_address>>24), (int)(ip_address>>16)&0xFF, (int)(ip_address>>8)&0xFF, (int)(ip_address)&0xFF );
    Printf("           MASK       is: %d.%d.%d.%d \n\r", (int)(mask>>24), (int)(mask>>16)&0xFF, (int)(mask>>8)&0xFF, (int)(mask)&0xFF );
    Printf("           GateWay    is: %d.%d.%d.%d \n\r", (int)(gateway>>24), (int)(gateway>>16)&0xFF, (int)(gateway>>8)&0xFF, (int)(gateway)&0xFF );
    Printf("           DNS        is: %d.%d.%d.%d \n\r", (int)(dns>>24), (int)(dns>>16)&0xFF, (int)(dns>>8)&0xFF, (int)(dns)&0xFF );
    Printf("           DNS2       is: %d.%d.%d.%d \n\r", (int)(dns2>>24), (int)(dns2>>16)&0xFF, (int)(dns2>>8)&0xFF, (int)(dns2)&0xFF );    // セカンダリDNS追加 sakaguchi 2021.04.07

    //status = nx_ip_interface_address_set(&g_ip0, 0, ip_address, mask);
    status = nx_ip_address_set(&g_ip0, ip_address, mask);
     Printf("IP Address set Status  %02X \n", status);
    if(NX_SUCCESS != status)
    {
         Printf("IP Address set error %d \n", status);
         ret = NET_STATIC_IP_ERROR;
    }
    else
    {
        status = nx_ip_gateway_address_set(&g_ip0, gateway);
        if(NX_SUCCESS != status)
        {
            Printf("Gateway Address set error %02X \n", status);
            ret = NET_STATIC_IP_ERROR;
        }
        else
        {
// sakaguchi 2020.09.17 ↓ // sakaguchi 2021.07.16 ↓
            // GARP送信
            //status = nx_arp_gratuitous_send(&g_ip0, NX_NULL); 
            // gatewayにping送信
            status = nx_icmp_ping(&g_ip0, gateway, "1", 1, &response_ptr, 100);
            if(NX_SUCCESS != status){
                // 処理無し
            }else{
                nx_packet_release(response_ptr);    // リリースが必要
            }
            Printf("ping to Gateway %02X \n", status);
// sakaguchi 2020.09.17 ↑ // sakaguchi 2021.07.16 ↑

            net_address.wifi.address = net_address.active.address = ip_address;
            net_address.wifi.mask = net_address.active.mask = mask;
            net_address.wifi.gateway = net_address.active.gateway = gateway;
            net_address.wifi.dns1 = net_address.active.dns1 = dns;
            net_address.wifi.dns2 = net_address.active.dns2 = dns2;                 // セカンダリDNS追加 sakaguchi 2021.04.07

            nx_ip_address_get(&g_ip0, &ip0_ip_address,&ip0_mask);
            Printf("Get IP ADDRESS is: %d.%d.%d.%d \n\r", (int)(ip0_ip_address>>24), (int)(ip0_ip_address>>16)&0xFF, (int)(ip0_ip_address>>8)&0xFF, (int)(ip0_ip_address)&0xFF );
            if(NetReboot == 0){
                PutLog(LOG_LAN,"Static IP $S (%d.%d.%d.%d/%d.%d.%d.%d/%d.%d.%d.%d)",
                    (int)(ip0_ip_address>>24), (int)(ip0_ip_address>>16)&0xFF, (int)(ip0_ip_address>>8)&0xFF, (int)(ip0_ip_address)&0xFF,
                    (int)(ip0_mask>>24), (int)(ip0_mask>>16)&0xFF, (int)(ip0_mask>>8)&0xFF, (int)(ip0_mask)&0xFF ,
                    (int)(gateway>>24), (int)(gateway>>16)&0xFF, (int)(gateway>>8)&0xFF, (int)(gateway)&0xFF
                );       
            }
            tx_thread_sleep(50);

            status = start_up_dns();
            Printf("     dns status %02X\r\n", status);
            //ret = (status != NX_SUCCESS) ? NET_DNS_ERROR : start_up_sntp();             // DNS error
            if(status != NX_SUCCESS){       // 2020/09/29
                ret = NET_DNS_ERROR;
            }
            else{
                if(my_config.device.TimeSync[0] != 0 &&  NetReboot == 0){
                    ret = start_up_sntp();
                }
                else{
                    ret = NET_SUCCESS;
                }
            }
        }
    }

    return (ret);
 
}




/**
 * wait_for_lnk_wifi function
 * This function check the network link status
 * @return
 */
static int wait_for_lnk_wifi(void)
{
    UINT status = 0;
    ULONG     ip_status;
//    UCHAR retry_cnt = 0;
    int try = 100;

    
    Printf("[echo_thread] wait_for_lnk_wifi.\r\n");
    /// Wait for initialization to finish.
    do {
        status = nx_ip_status_check(&g_ip0, NX_IP_LINK_ENABLED, (ULONG *) &ip_status, TX_NO_WAIT);
        
        Printf("    nx_ip_status_check %ld (%d)\r\n", status, try);

        if(status == NX_SUCCESS)
            break;
        try--;
        tx_thread_sleep(10);
    }while(try>0);

    if(status == NX_SUCCESS){
        Printf("WIFI IP LINK \r\n");
            return (0);
    }
    else
    {
        Printf ("\r\nNetX IP interface NX_IP_LINK_ENABLE check failed.\r\n");
        return(1);
    }

    /*
    /// Wait for initialization to finish.
    do {
        //status = nx_ip_interface_status_check(&g_ip0, 0, NX_IP_LINK_ENABLED, &ip_status, 100);
        //status = nx_ip_status_check(&g_ip0, NX_IP_ADDRESS_RESOLVED, (ULONG *) &ip_status, 500);
        status = nx_ip_status_check(&g_ip0, NX_IP_LINK_ENABLED, (ULONG *) &ip_status, 500);
        
        Printf("    nx_ip_status_check %ld\r\n", status);

    } while((status != NX_SUCCESS) && (retry_cnt < MAX_RETRY_CNT));


    if(retry_cnt >= MAX_RETRY_CNT)
    {
        Printf ("\r\nNetX IP interface NX_IP_LINK_ENABLE check failed.\r\n");
        //APP_ERR_TRAP(status);
        return (1);
    }
    else{
        Printf("WIFI IP LINK \r\n");
        return (0);
    }
    */
}

/**
 * wifi apn scan
 * 
 *  Start of SF_WIFI Specific 
    SSP_ERR_WIFI_CONFIG_FAILED              = 70000,    // WiFi module Configuration failed.
    SSP_ERR_WIFI_INIT_FAILED                = 70001,    // WiFi module initialization failed.
    SSP_ERR_WIFI_TRANSMIT_FAILED            = 70002,    // Transmission failed
    SSP_ERR_WIFI_INVALID_MODE               = 70003,    // API called when provisioned in client mode
    SSP_ERR_WIFI_FAILED                     = 70004,    // WiFi Failed.
    SSP_ERR_WIFI_WPS_MULTIPLE_PB_SESSIONS   = 70005,    // Another Push button session is already in progress
    SSP_ERR_WIFI_WPS_M2D_RECEIVED           = 70006,    // M2D Error code received which means Registrar is unable to authenticate with the Enrollee
    SSP_ERR_WIFI_WPS_AUTHENTICATION_FAILED  = 70007,    // WPS authentication failed
    SSP_ERR_WIFI_WPS_CANCELLED              = 70008,    // WPS Request was not accepted by underlying driver
    SSP_ERR_WIFI_WPS_INVALID_PIN            = 70009,    // Invalid WPS Pin
 * @return
 * @attention   WiFiスレッド外からも呼ばれる。注意
 */
int wifi_module_scan(void)
{
    int ret = -1;
    ssp_err_t ssp_err = SSP_SUCCESS;

    Printf("[echo_thread] wifi_module_scan.\r\n");
    //----- karel -----
    /*call the Wi-Fi module scan API*/
    //ssp_err_t (*scan)(sf_wifi_ctrl_t * const p_ctrl, sf_wifi_scan_t * const p_scan, UCHAR * const p_cnt);
    sf_wifi_scan_t l_scan[16];
    UCHAR l_cnt = 16;
    uint32_t len;
    char SSID[32+1];                                                // sakaguchi 2020.09.07
    int8_t max_rssi = -99;                                          // 2021.03.12
    uint8_t channel = 0;
    //uint8_t bss;                                                  // 未使用のため削除

    memset(AP_List, 0x00, sizeof(AP_List));
    memset(SSID, 0x00, sizeof(SSID));                               // sakaguchi 2020.09.07
    memset(BSSID, 0x00, sizeof(BSSID));                             // sakaguchi 2021.11.25
    memset(l_scan, 0x00, sizeof(l_scan));                           // sakaguchi 2021.11.25

    AP_Count = 0;                                                   // sakaguchi 2020.09.07-2
    ssp_err = g_sf_wifi0.p_api->scan(g_sf_wifi0.p_ctrl, &l_scan[0], &l_cnt);
    if(ssp_err)
    {
        Printf ("\r\n\r\nYour Wi-Fi module scan failed. ssp_err = %d\r\n\r\n", ssp_err);
        Printf ("\r\n\r\nYour Wi-Fi module scan failed.");
        Printf ("\r\n");
        //PutLog( LOG_LAN, "WIFI Start Error (%ld)", ssp_err);  // for debug
        PutLog( LOG_LAN, "WIFI network scan failed (%ld)", ssp_err);
        //DebugLog( LOG_DBG, "WIFI Start Error (%ld)", ssp_err);
        DebugLog( LOG_DBG, "WIFI network scan failed (%ld)", ssp_err);
        ret = ssp_err;
    }
    else
    {
        Printf ("\r\n\r\nYour Wi-Fi module scan passed.\r\n");
        Printf("l_cnt = %d\r\n", l_cnt );
//        Printf("SSID  %s %d \r\n\r\n", my_config.wlan.SSID, strlen(my_config.wlan.SSID));
        memcpy( SSID, my_config.wlan.SSID, (size_t)sizeof(my_config.wlan.SSID));
        Printf("SSID  %s %d \r\n\r\n", SSID, strlen(SSID));                                     // sakaguchi 2020.09.07

//        UCHAR i;
        UCHAR i,j=0;
        AP_Count = l_cnt;

        for(i=0; i< l_cnt; i++)
        {
            Printf("%d. ",i+1);
            Printf("SSID: %.32s    ",l_scan[i].ssid);
            Printf("RSSI: %d ", (int8_t)l_scan[i].rssi);
            Printf("SEC : %d ", (int8_t)l_scan[i].security);
            Printf("Channel : %d ", (uint8_t)l_scan[i].channel);
            Printf("BSSID : %08X", l_scan[i].bssid);
            len = strlen((char *)l_scan[i].ssid);
            if(len > 32){
                len = 32;
            }

// 2023.11.16 ↓
            // l_scan[i].security = 5(WP3 only)はAP_Listに格納しない
            if(CC3135_New_Security) {
                if((int8_t)l_scan[i].security == 5){
                    if(AP_Count) AP_Count--;
                    continue;
                }
            }

            memcpy(AP_List[j].ssid, (char *)&l_scan[i].ssid, len  );
            AP_List[j].rssi = l_scan[i].rssi;

            // l_scan[i].security = 4(WPA2)は、security = 3(WPA2)に置換
            // l_scan[i].security = 6(MIX)は、security = 3(WPA2)に置換
            if(CC3135_New_Security) {
                if(((int8_t)l_scan[i].security == 4) || ((int8_t)l_scan[i].security == 6)){
                    AP_List[j].sec = 3;     // WPA2
                }else{
                    AP_List[j].sec = (char)l_scan[i].security;
                }
            }else{
                AP_List[j].sec = (char)l_scan[i].security;
            }
            j++;
// 2023.11.16 ↑

            //bss = (uint8_t)
//            if(memcmp(my_config.wlan.SSID, (char *)l_scan[i].ssid, strlen(my_config.wlan.SSID)) ==  0){
//            if(memcmp(SSID, (char *)l_scan[i].ssid, strlen(SSID)) ==  0){                       // sakaguchi 2020.09.07
            if(strcmp(SSID, (char *)l_scan[i].ssid) ==  0){                     // sakaguchi 2021.07.16 完全一致
                Printf("\r\n      ####   SSID  Match (%d)!!\r\n",i);
                if((int8_t)l_scan[i].rssi < (int8_t)(-80)){
                    PutLog( LOG_LAN, "RSSI:%d SEC:%d CH:%d %02X WLAN Bad Condition", (int8_t)l_scan[i].rssi, (int8_t)l_scan[i].security,(int8_t)l_scan[i].channel, (uint8_t)(l_scan[i].bssid));                
                }
                else{
                    PutLog( LOG_LAN, "RSSI:%d SEC:%d CH:%d %02X", (int8_t)l_scan[i].rssi, (int8_t)l_scan[i].security,(int8_t)l_scan[i].channel , (uint8_t)l_scan[i].bssid);
                }
                DebugLog( LOG_DBG, "SSID:%.32s(%.32s) RSSI:%d SEC:%d CH:%d BSSID:%08X", l_scan[i].ssid, my_config.wlan.SSID ,(int8_t)l_scan[i].rssi, (int8_t)l_scan[i].security,(int8_t)l_scan[i].channel, l_scan[i].bssid);
                ret = 0;
//2023.11.16
                if(CC3135_New_Security) {
                    //wifi_sec_mode = l_scan[i].security;                                      // sakaguchi 2020.09.07
                    uint8_t scan_secValue;
                    scan_secValue = l_scan[i].security;
                    uint8_t connect_secValue;
                    sdk6_connect_secValue_from_scan_secValue(scan_secValue, &connect_secValue);
                    wifi_sec_mode = connect_secValue;
                } else {
//2023.11.16
                wifi_sec_mode = l_scan[i].security;                                      // sakaguchi 2020.09.07
//2023.11.16
                }
//2023.11.16
                if((int8_t)l_scan[i].rssi > (int8_t)max_rssi){
                    max_rssi = (int8_t)l_scan[i].rssi;
                    channel =  (uint8_t)l_scan[i].channel;
                    Printf("App Info(%d)  ch:%d  rssi:%d \r\n", i,channel,max_rssi);
                    PutLog( LOG_LAN,"App Info(%d)  ch:%d  rssi:%d", i,channel,max_rssi);    // 2023.08.01 改行を削除
                    memcpy(BSSID, (char *)&l_scan[i].bssid, 6);                         // sakaguchi 2021.11.25
                }
            }
                
            Printf("\r\n");
        }

        Printf("App Info    ch:%d  rssi:%d \r\n", channel,max_rssi);
        
        max_rssi_channel = channel;
    }
    return (ret);
    //----- karel -----
}

/**
 *
 */
void wifi_current_status(void)
{
    ssp_err_t ssp_err;
    sf_wifi_info_t info;
    uint16_t rssi;

    ssp_err = g_sf_wifi0.p_api->infoGet(g_sf_wifi0.p_ctrl, &info);

    rssi = info.rssi;

    Printf("WIFI RSSI %02X (%04X)\r\n", rssi, ssp_err);
}


/**
 *
 * @param p_args
 * @note  WIFI Eventの割り込み
 */
void provConDisconCB(sf_wifi_callback_args_t * p_args)
{
    //Printf("provConDisconCB:  p_args = 0x%08x\r\n", p_args);
    UINT status;
    UINT state;
    ULONG run_count;
    UINT priority;
    UINT preemption_threshold;
    ULONG time_slice;
    TX_THREAD *next_thread;
    TX_THREAD *suspended_thread;
    CHAR *name;

    if( p_args == 0 ){
        return;
    }

    Printf("provConDisconCB:  p_args->event = %d\r\n", p_args->event);

    if( p_args->event == SF_WIFI_EVENT_RX ) {
        Printf("\r\nPacket received event\r\n"); 
    }
    if( p_args->event == SF_WIFI_EVENT_AP_CONNECT ) {
        Printf("\r\nDevice Associated Successfully with AP\r\n"); 
        Printf("####   WIFI Link Up \r\n");
        //PutLog(LOG_LAN, "WIFI Link Up");     // AP接続
        Link_Status = 1;
        tx_event_flags_set (&event_flags_wifi, 0x00000000, TX_AND);
    }
    if( p_args->event == SF_WIFI_EVENT_AP_DISCONNECT ) { 
        Printf("\r\nDevice Disconnected with AP\r\n"); 

        // リンクダウン中はイベントフラグの再セットは実行しない 2022.04.11
        if(Link_Status != 0){
            Link_Status = 0;
            status =tx_thread_info_get(&wifi_thread, &name, &state, &run_count, &priority,&preemption_threshold,&time_slice,&next_thread,&suspended_thread);
            Printf("WIFI Thread info %02X %02X\r\n", status, state);
            if(state == TX_SUSPENDED){
                //Printf("WIFI Thread Suspend\r\n");
                Printf("####   Link Down WIFI Thread Suspend\r\n");
                tx_event_flags_set (&event_flags_link, 0x00000000, TX_AND);
                tx_event_flags_set (&event_flags_link, FLG_LINK_DOWN, TX_OR);;
            }
            else{
                Printf("####   Link Down WIFI Thread No Suspend\r\n");
                tx_event_flags_set (&event_flags_wifi, 0x00000000, TX_AND);
                tx_event_flags_set (&event_flags_wifi, FLG_WIFI_DOWN, TX_OR);
            }
            Printf("####   WIFI Link Down \r\n");
        }else{
            Printf("####   WIFI Link Down Consecutive\r\n");
        }

    }
    if( p_args->event == SF_WIFI_EVENT_CLIENT_CONNECT ) {
        Printf("\r\nClient Associated Successfully with device AP\r\n"); 
    }
    if( p_args->event == SF_WIFI_EVENT_CLIENT_DISCONNECT ) {
        Printf("\r\nClient Disconnected from device AP\r\n"); 
    }

}

/**
 *
 * @return
 */
static int wifi_module_provision(void)
{
    //sf_wifi_provisioning_t g_provision_info;
    //UCHAR wifimsg_from_queue[UX_DEMO_BUFFER_SIZE];
    //wifi_queue_payload_t *msg;
    ssp_err_t ssp_err = SSP_SUCCESS;
    UCHAR sec;
    char SSID[32+1];
    char PSK[64+1];
    char WEP[26+1];
    int len;
    int i;
    int ret;

    sec = my_config.wlan.SEC[0];
    memset(SSID, 0x00, sizeof(SSID));
    memset(PSK, 0x00, sizeof(PSK));
    memset(WEP, 0x00, sizeof(WEP));

    Printf("[echo_thread] wifi_module_provision.\r\n");
    /** Set other provisioning configuration */
    g_provision_info.mode = SF_WIFI_INTERFACE_MODE_CLIENT;
    //g_provision_info.security = SF_WIFI_SECURITY_TYPE_WPA2; /* change this setting to work with other Security types*/
    g_provision_info.encryption = SF_WIFI_ENCRYPTION_TYPE_AUTO;
    

    //2023.11.16
    if(CC3135_New_Security)
    {
        switch (sec)
        {
            case 0:     // Non
                g_provision_info.security = wifi_sec_mode;                  // スキャンで取得したセキュリティモードで動作させる
                break;
            case 2:     // WEP128
                g_provision_info.security = SL_WLAN_SEC_TYPE_WEP;       //(1) //SF_WIFI_SECURITY_TYPE_WEP; //(1)
                break;
            case 3:     // WPA-TKIP
//                g_provision_info.security = SL_WLAN_SEC_TYPE_WPA_WPA2;  //(2) //SF_WIFI_SECURITY_TYPE_WPA; //(2)
                g_provision_info.security = SL_WLAN_SEC_TYPE_WPA;       // 2023.11.16
                break;
            case 4:     // WPA2-AES
                g_provision_info.security = SL_WLAN_SEC_TYPE_WPA_WPA2;  //(2) //SF_WIFI_SECURITY_TYPE_WPA2;  //(3)
                break;

            case 5:     //
                g_provision_info.security = SL_WLAN_SEC_TYPE_WPA2_PLUS; //(11)
                break;
            case 6:     //
                g_provision_info.security = SL_WLAN_SEC_TYPE_WPA3;      //(12)
                break;

            default:
                g_provision_info.security = SL_WLAN_SEC_TYPE_WPA_WPA2;  //(2) //SF_WIFI_SECURITY_TYPE_WPA2; //(3)
                break;
        }

    } else {
    //2023.11.16

// sakaguchi 2020.09.07 ↓
    switch (sec)
    {
        case 0:     // Non
            g_provision_info.security = wifi_sec_mode;                  // スキャンで取得したセキュリティモードで動作させる
            break;
        case 2:     // WEP128
            g_provision_info.security = SF_WIFI_SECURITY_TYPE_WEP;
            break;
        case 3:     // WPA-TKIP
            g_provision_info.security = SF_WIFI_SECURITY_TYPE_WPA;
            break;
        case 4:     // WPA2-AES
            g_provision_info.security = SF_WIFI_SECURITY_TYPE_WPA2;
            break;
        default:
            g_provision_info.security = SF_WIFI_SECURITY_TYPE_WPA2;
            break;
    }
    //2023.11.16
    }
    //2023.11.16

    //g_provision_info.channel = max_rssi_channel;     現状ではこれは効かない、wifiのdriverの変更の必要あり


    // SSID
    memcpy(SSID, my_config.wlan.SSID, (size_t)sizeof(my_config.wlan.SSID));
    len = (int)strlen(SSID);
    Printf("  WIFI SSID len = %d / %s \r\n", len, SSID);
    Printf("  WIFI Channel  = %d  \r\n", g_provision_info.channel);
    //strcpy((char*)msg->ap_ssid, &SSID[0]);
    //strcpy((char *)&g_provision_info.ssid, SSID);
    memset(g_provision_info.ssid, 0x00, (size_t)sizeof(g_provision_info.ssid));
    memcpy(g_provision_info.ssid, SSID, (size_t)len);

// sakaguchi 2021.11.25 ↓
    // BSSID
    memset(g_provision_info.mac, 0x00, (size_t)sizeof(g_provision_info.mac));
    memcpy(g_provision_info.mac, BSSID, (size_t)sizeof(BSSID));

    // country code
    memset(g_provision_info.country, 0x00, (size_t)sizeof(g_provision_info.country));
    uint8_t country;                                    // 仕向け先
    uint32_t sn = fact_config.SerialNumber;
    sn = sn >> 28;                                      // 5:Jp 3:Us 4:Eu E:Espec C:Hitachi    
    country = (uint8_t)sn;

    switch (country)
    {
        case 0x03:      // US
            memcpy(g_provision_info.country, "US", (size_t)sizeof("US"));
            break;
        case 0x04:      // EU
            memcpy(g_provision_info.country, "EU", (size_t)sizeof("EU"));
            break;
        default:        // JP  or ESPEC or Hitachi
            memcpy(g_provision_info.country, "JP", (size_t)sizeof("JP"));
            break;
    }
// sakaguchi 2021.11.25 ↑

    memset(g_provision_info.key, 0x00, (size_t)sizeof(g_provision_info.key));
    switch (sec)
    {
        case 0:     // Non
            // KEY無し
            break;
        case 2:     // WEP128
            memcpy(WEP, my_config.wlan.WEP, (size_t)sizeof(my_config.wlan.WEP));
            len = (int)strlen(WEP);
            memcpy(g_provision_info.key, WEP, (size_t)len);
            //strcpy((char *)&g_provision_info.key, WEP);
            Printf("  WIFI WEP HEX len = %d / %s \r\n", len, WEP);
            //}else{
                // HEX文字列の変換が必要
                //for(i=0; i<len/2; i++){
                    //cWep_tmp[0] = WEP[i*2];
                    //cWep_tmp[1] = WEP[i*2+1];
                    //Wep_Key[i] = (char)strtol(cWep_tmp, 0, 16);
                //}
                //strcpy((char *)&g_provision_info.key, Wep_Key);
                //Printf("  WIFI WEP TEXT len = %d / %s \r\n", len/2, Wep_Key);
            //}
            //memcpy(g_provision_info.key, Wep_Key, (size_t)len);
            break;

//2023.11.16
        case 5: // PSKを使う
        case 6: // PSKを使う
//2023.11.16
        case 3:     // WPA-TKIP
        case 4:     // WPA2-AES
            memcpy(PSK, my_config.wlan.PSK, (size_t)sizeof(my_config.wlan.PSK));
            len = (int)strlen(PSK);
            Printf("  WIFI PSK  len = %d / %s \r\n", len, PSK);
            //strcpy((char*)msg->ap_pwd, &PSK[0]);
            //strcpy((char *)&g_provision_info.key, PSK);
            memcpy(g_provision_info.key, PSK, (size_t)len);
            break;
        default:
            break;
    }

    Link_Status = 0;    // 2020.09.15
    //g_provision_info.channel = 0; // Not used for STA mode
    //g_provision_info.p_callback = 0;
    g_provision_info.p_callback = provConDisconCB;

    Printf ("\r\n\r\nYour Wi-Fi module provision set execute.\r\n");
    //PutLog (LOG_LAN, "provision start");        // 開始
    //call the Wi-Fi module provision API  
    ssp_err = g_sf_wifi0.p_api->provisioningSet(g_sf_wifi0.p_ctrl, &g_provision_info);
    Printf ("\r\n   provisioned.  %ld\r\n", ssp_err);

    if(ssp_err)
    {
       //PutLog (LOG_LAN, "provision failed");     // 失敗
       Printf ("\r\n\r\nYour Wi-Fi module provision failed.");
       Printf ("\r\nPlease check your network connection, SSID and Password.\r\n");
       Printf ("\r\nYou will need to restart the MCU to provision the Wi-Fi module.\r\n");

        switch (ssp_err)
        {
            case 70000:
                DebugLog(LOG_DBG,"WiFi module Configuration failed.");
                break;
            case 70001:
                DebugLog(LOG_DBG,"WiFi module initialization failed.");
                break;
            case 70002:
                DebugLog(LOG_DBG,"Transmission failed");
                break;
            case 70003:
                DebugLog(LOG_DBG,"API called when provisioned in client mode");
                break;
            case 70004:
                wifi_faile ++;
                if(wifi_faile < 5){
                    DebugLog(LOG_DBG,"WiFi Failed. %ld",wifi_faile);
                }
                break;
            case 70005:
                DebugLog(LOG_DBG,"Another Push button session is already in progress");
                break;
            case 70006:
                DebugLog(LOG_DBG,"M2D Error code received which means Registrar is unable to authenticate with the Enrollee");
                break;
            case 70007:
                DebugLog(LOG_DBG,"WPS authentication failed");
                break;
            case 70008:
                DebugLog(LOG_DBG,"WPS Request was not accepted by underlying driver");
                break;
            case 70009:
                DebugLog(LOG_DBG,"Invalid WPS Pin");
                break;
            default:
                DebugLog(LOG_DBG,"Wifi Other Error %ld", ssp_err);
                break;
        }


        //return (1);
        return (ssp_err);         // sakaguchi 2021.08.02
    }
    //else if(Link_Status==0){
    //    DebugLog(LOG_DBG,"Link Down at Provision");
    //    return (1);
    //}
    else
    {
        ret = 1;
        for(i=0;i<100;i++){
            if(Link_Status==1){          // 1:Link Up 0:Link Down
                ret = 0;
                break;
            }
            tx_thread_sleep(10);        //  100msec
        }

        if(ret == 1){
            DebugLog(LOG_DBG,"Link Down at Provision");
        }

        Printf ("\r\n\r\nYour Wi-Fi module is provisioned.\r\n");
        return (ret);
    }

}

/**
 * Start SNTP & Adjust time
 * @return
 */
uint32_t start_up_sntp(void)
{
    ULONG   actual_events;
    uint32_t ret = NET_SNTP_ERROR;

    PutLog( LOG_LAN, "Start SNTP");
    tx_thread_sleep(150);

    tx_event_flags_get (&event_flags_sntp, 0xffffffff,TX_OR_CLEAR, &actual_events,TX_NO_WAIT);          // flag clear
    tx_thread_resume(&sntp_thread);

    tx_event_flags_get (&event_flags_sntp, FLG_SNTP_SUCCESS | FLG_SNTP_ERROR, TX_OR, &actual_events, TX_WAIT_FOREVER);
    if(actual_events & FLG_SNTP_ERROR){
        Printf(" ====> SNTP Error \r\n");
    }
    else{
        Printf(" ====> SNTP OK \r\n");
        ret = NET_SUCCESS;
    }


    return (ret);
}



/**
 * Start Up DNS
 * @return
 */
static uint32_t start_up_dns(void)
{
    uint32_t status;
    uint8_t add_ok = 0;                 // 2023.02.20
//    uint32_t server_address;            // 2023.02.20
//    uint8_t i;                          // 2023.02.20

    // Remove all old DNS servers
    nx_dns_server_remove_all(&g_dns0);
    Printf("\r\n### DNS Server Start\r\n\r\n");

    // add 2020 09 03  test
    //status = nxd_dns_server_remove(&g_dns0, net_address.active.dns1);
    //Printf("DNS remove %02X\r\n", status);

// 2023.02.20 ↓
//    status = nx_dns_server_add(&g_dns0, net_address.active.dns1);
//    //status = nxd_dns_server_add(&g_dns0, &net_address.active.dns1);     // IPv4アドレスのみを受け入れるためと書かれているが、上手くいかない！！
//    Printf("DNS Status %02X\r\n", status);
//    if(status != NX_SUCCESS){
//        Printf("DNS set up error\r\n");
//    }
//    if(net_address.active.dns2 != 0){
//        nx_dns_server_add(&g_dns0, net_address.active.dns2);           // セカンダリはステータスを更新しない。プライマリが通ればOKとする。// sakaguchi 2020.08.31
//        //nxd_dns_server_add(&g_dns0, &net_address.active.dns2);           // セカンダリはステータスを更新しない。プライマリが通ればOKとする。// sakaguchi 2020.08.31
//        Printf("DNS2 Status %02X\r\n", status);
//    }
    if(net_address.active.dns1 != 0){
        status = nx_dns_server_add(&g_dns0, net_address.active.dns1);
        Printf("DNS Status %02X\r\n", status);
        if(status == NX_SUCCESS){
            add_ok = 1;
        }
    }
    if(net_address.active.dns2 != 0){
        status = nx_dns_server_add(&g_dns0, net_address.active.dns2);
        Printf("DNS2 Status %02X\r\n", status);
        if(status == NX_SUCCESS){
            add_ok = 1;
        }
    }
    if(net_address.active.dns3 != 0){
        status = nx_dns_server_add(&g_dns0, net_address.active.dns3);
        Printf("DNS3 Status %02X\r\n", status);
        if(status == NX_SUCCESS){
            add_ok = 1;
        }
    }
    if(net_address.active.dns4 != 0){
        status = nx_dns_server_add(&g_dns0, net_address.active.dns4);
        Printf("DNS4 Status %02X\r\n", status);
        if(status == NX_SUCCESS){
            add_ok = 1;
        }
    }
    if(net_address.active.dns5 != 0){
        status = nx_dns_server_add(&g_dns0, net_address.active.dns5);
        Printf("DNS5 Status %02X\r\n", status);
        if(status == NX_SUCCESS){
            add_ok = 1;
        }
    }
    //for(i=0; i<5; i++){       // デバッグ用
    //    nx_dns_server_get(&g_dns0,i,&server_address);
    //    Printf("server_address[%d] %d.%d.%d.%d \r\n", i,(int)(server_address>>24), (int)(server_address>>16)&0xFF, (int)(server_address>>8)&0xFF, (int)(server_address)&0xFF );
    //}
// 2023.02.20 ↑
    DebugLog(LOG_DBG,"DNS Start %04X", status);
    if(add_ok){
        status = NX_SUCCESS;                // 1つでもサーバ登録OKなら成功とする  2023.02.20
    }
    return (status);
}


/**
 * WIFI Parameter Check
 * @return
 */
uint32_t check_wifi_para(void)
{
    uint32_t  err = 0;
    int size;
    int len = 0;
    UCHAR sec;
    char cWork[64+2];
    memset(cWork, 0x00, sizeof(cWork));                 // sakaguchi 2020.09.07

    sec = my_config.wlan.SEC[0];

    size = sizeof(my_config.wlan.SSID);
    //len = (int)strlen(my_config.wlan.SSID);           // sakaguchi 2020.09.07
    memcpy(cWork, my_config.wlan.SSID, (size_t)size);
    len = (int)strlen(cWork);
    //Printf("SSID %d %d \r\n", size,len);
    if(!((0 < len) && (len <= size))){
        err ++;
    }

    // sakaguchi 2020.09.07 del
    //size = sizeof(my_config.wlan.SSID);
    //len = (int)strlen(my_config.wlan.SSID);
    //Printf("SSID %d %d \r\n", size,len);
    //if(!((0 < len) && (len <= size))){
    //    err ++;
    //}

    switch (sec)
    {
        case 0:     // Non
            break;                                      // sakaguchi 2021.11.09 セキュリティモード：なしの場合、パスワードはチェックしない
        case 2:     // WEP128
            size = sizeof(my_config.wlan.WEP);
            //len = (int)strlen(my_config.wlan.WEP);    // sakaguchi 2020.09.07
            memset(cWork, 0x00, sizeof(cWork));
            memcpy(cWork, my_config.wlan.WEP, (size_t)size);
            len = (int)strlen(cWork);
            //Printf("WEP %d %d \r\n", size,len);
            if(!((0 < len) && (len <= size)))
                err ++;
            break;
        case 3:     // WPA-TKIP
        case 4:     // WPA2-AES
            size = sizeof(my_config.wlan.PSK);
            //len = (int)strlen(my_config.wlan.PSK);    // sakaguchi 2020.09.07
            memset(cWork, 0x00, sizeof(cWork));
            memcpy(cWork, my_config.wlan.PSK, (size_t)size);
            len = (int)strlen(cWork);
            //Printf("PSK %d %d \r\n", size,len);
            if(!((0 < len) && (len <= size)))
                err ++;
            break;
        default:
            err++;
            break;
    }


    return (err);
}



/**
 ********************************************************************************************************************
 * set_mac_address function
 *
 * Sets the unique Mac Address of the device using the FMI unique ID.
 * 00-0D- 8B-08-0000～00-0D- 8B-08-00FF
 *******************************************************************************************************************
 */
void set_mac_callback (nx_mac_address_t*_pMacAddress)
{
    //fmi_unique_id_t id;
    ULONG lowerHalfMac;

    // Read FMI unique ID 
    //g_fmi.p_api->uniqueIdGet(&id);

    // REA's Vendor MAC range: 00:30:55:xx:xx:xx 
    // REA's Vendor MAC range: 00:0D:8B:xx:xx:xx 

    lowerHalfMac = ((0x8b000000) | (fact_config.mac_l & (0x00FFFFFF)));

    /* Return the MAC address */
    _pMacAddress->nx_mac_address_h=0x000d;
    _pMacAddress->nx_mac_address_l=lowerHalfMac;
}


/**
 * @brief   LAN接続チェック
 * @param[in]   mode        PHY(0:ETHERNET/1:WIFI)
 * @return          接続状態(0:LAN_DISCONNECT/1:LAN_CONNECT)
 */
UCHAR   lan_connect_check(UCHAR mode){

    UCHAR   ucRet = LAN_CONNECT;            // 戻り値：接続正常

    if(mode == my_config.network.Phy[0])    // Phy
    {
        if( (LINK_DOWN == Link_Status)          // Link状態
         || (NETWORK_DOWN == NetStatus)         // ネットワーク状態
         || (ETLAN_ERROR == NetCmdStatus) )     // 通信状態
        {
            ucRet = LAN_DISCONNECT;             // 接続異常
        }
    }
    return(ucRet);
}


/**
 * @brief   設定状態チェック
 * 時刻同期エラーまたは自律動作設定エラーの場合、異常(false）を返す
 * @return  設定状態（true:正常/false:異常）
 */
bool set_state_check(void){

    bool   bRet = true;                  // 戻り値：設定正常

    if( (STATE_ERROR == UnitState)          // 自律設定
     || (SNTP_ERROR == Sntp_Status) )       // SNTP状態
    {
        bRet = false;                       // 設定異常
    }
    return(bRet);
}

void wifi_reboot(void)
{
    uint32_t status1,status2;

    status1 = status2 = 0;
    status1 = g_sf_wifi0.p_api->close(g_sf_wifi0.p_ctrl);

    WIFI_RESET_ACTIVE;
    tx_thread_sleep (2); 
    WIFI_RESET_INACTIVE;
    tx_thread_sleep (2); 
    status2 = g_sf_wifi0.p_api->open(g_sf_wifi0.p_ctrl, g_sf_wifi0.p_cfg); 

    Printf("WIFI Reboot %04X %04X\r\n", status1,status2);
}
