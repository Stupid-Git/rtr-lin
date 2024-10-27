/**
 * @brief   network thread
 * @file    network_thread_entry.c
 * @note    2020.Jul.01 GitHub 630ソース反映済み
 * @note	2020.Aug.07	0807ソース反映済み
 */
#if 1
#include "MyDefine.h"
#include "flag_def.h"
#include "Globals.h"
#include "General.h"
#include "Config.h"
#include "Log.h"
#include "network_thread.h"
#include "event_thread.h"
#include "http_thread.h"
#include "ftp_thread.h"
#include "http_thread.h"
#include "smtp_thread.h"
#include "auto_thread.h"
#include "system_thread.h"
#include "led_thread_entry.h"
#include "wifi_thread.h"
#include "usb_thread_entry.h"

extern TX_THREAD udp_thread;
extern TX_THREAD tcp_thread;
//extern TX_THREAD ftp_thread;
extern TX_THREAD http_thread;           //sakaguchi
//extern TX_THREAD smtp_thread;
extern TX_THREAD event_thread;
//extern TX_THREAD auto_thread;     //未使用
extern TX_THREAD wifi_thread;
extern TX_THREAD network_thread;

extern void wifi_reboot(void);

//プロトタイプ
void link_status_change_notify ( NX_IP *ip_ptr, UINT interface_index, UINT link_up);

void mac_get(void);
void interface_select(void);

void net_restart(void);

/** Network Thread entry function */
void network_thread_entry(void)
{

    UINT status;
    ULONG actual_events;
//    ULONG actual_status;      2020.01.16コメントアウト

    CHAR *name;
    UINT state;
    ULONG run_count;
    UINT error_count;
    UINT priority;
    UINT preemption_threshold;
    ULONG time_slice;
    TX_THREAD *next_thread;
    TX_THREAD *suspended_thread;
//    int i;
//    ssp_err_t err;     //未使用
    UINT retry_cnt;                     // 2022.04.11 add

///2023.11.16 ↓
    void call_to_check_and_update_CC3135_Firmware(void);
    call_to_check_and_update_CC3135_Firmware();
///2023.11.16 ↑

    NetReboot = 0;
    Link_Status = 0;                     // Ethrnet Link Down 
    Sntp_Status = SNTP_ERROR;            // SNTP error              // sakaguchi UT-0035
    nx_common_init0();
    nx_secure_common_init();

         
    packet_pool_init1();
    packet_pool_init0();

    ip_init0();

    dhcp_client_init0();
    nx_dhcp_interface_disable(&g_dhcp_client0, 0);  // nx_dhcp_createを実行すると、DHCPに対しインタフェースが自動で有効となるためここで停止させる sakaguchi 2020.08.31
    nx_dhcp_interface_disable(&g_dhcp_client0, 1);  // 有効になるのはプライマリインタフェース(wifi)だけだが念のためLANも停止させる sakaguchi 2020.08.31
    sntp_client_init0();
    dns_client_init0();
    
    
    for(;;){
        NetStatus = NETWORK_DOWN;     // 0: 未接続 	1: 接続完了 // sakaguchi 2020.09.17

        error_count = 0;

        tx_event_flags_get (&event_flags_link, 0xffffffff,TX_OR_CLEAR, &actual_events,TX_NO_WAIT);          // flag clear
        Printf("    Net Network Mutex get wait\r\n");
        tx_mutex_get(&mutex_network_init, TX_WAIT_FOREVER);    
        Printf("    Net Network Mutex get\r\n");
  

ReStart:;
        NetStatus = NETWORK_DOWN;     // 0: 未接続      // sakaguchi 2020.09.17

        Printf("inteface select \r\n");
        interface_select();
        if(net_cfg.interface_index == 0){  //WIFI
            //WIFI_RESET_ACTIVE;    //ここでOK？？？
            //tx_thread_sleep (2); 
            //WIFI_RESET_INACTIVE;
            ;
        }


        LED_Set( LED_DIAG, LED_ON );
        
        tx_thread_sleep (100); 
        Printf("event_flags_network clear \r\n");
        tx_event_flags_get (&event_flags_network, 0xffffffff,TX_OR_CLEAR, &actual_events,TX_NO_WAIT);  
        //Printf("              wifi thread start(resume) \r\n");

        Printf("WIFI Thread info-0 %02X(%02X)\r\n", state, status);
        status =tx_thread_info_get(&wifi_thread, &name, &state, &run_count,
        		&priority,&preemption_threshold,&time_slice,&next_thread,&suspended_thread);
        Printf("WIFI Thread info-1 %02X(%02X)\r\n", state, status);

            

        status =tx_thread_resume(&wifi_thread);
        Printf("    wifi thread start 1 (resume) %d \r\n" ,status);
        /*
        for(;;){
            status =tx_thread_info_get(&wifi_thread, &name, &state, &run_count,
        		    &priority,&preemption_threshold,&time_slice,&next_thread,&suspended_thread);
            Printf("WIFI Thread info-3 %02X (%02X)\r\n", state, status);
            if(state == TX_READY)
                break; 
            tx_thread_sleep (1);
        }
        */
        //status =tx_thread_resume(&wifi_thread);
        //Printf("              wifi thread start(resume) %d \r\n" ,status);

        Printf("    wifi thread flag wait \r\n");
        status = tx_event_flags_get (&event_flags_network, FLG_DHCP_SUCCESS | FLG_DHCP_ERROR , TX_OR, &actual_events,TX_WAIT_FOREVER);
        if(actual_events & FLG_DHCP_ERROR){
            Printf("Network Thread Error %ld\r\n", error_count);
            if(isUSBState() == USB_DISCONNECT){
                //LED_Set(LED_LANON, LED_OFF);
                LED_Set( LED_NETER, LED_ON );
                error_count ++;
            }
            tx_thread_sleep (1000);         // 10sec 
            goto ReStart;
        }

        error_count = 0;
        
        Printf("Network Setup Success \r\n");

        mac_get();
        //###################################################

        NetStatus = NETWORK_UP;			    // 0: 未接続 	1: 接続完了     // sakaguchi 2020.09.17
        Printf("Network Setup Success 2\r\n");
        //LED_Set(LED_DIAGOFF, LED_OFF);     // 
        LED_Set( LED_NETER, LED_OFF );
        LED_Set(LED_DIAGOFF, LED_OFF);     // 

        tx_thread_sleep (100);
        LED_Set( LED_LANON, LED_ON );
        //Printf("    #######  Network Mutex release  ############\r\n");
        //tx_mutex_put(&mutex_network_init);    

        if(TestMode != TEST_MODE){

            status = tx_thread_resume(&udp_thread);
            Printf("udp thread start(resume) %d \r\n" ,status);

            status = tx_thread_resume(&tcp_thread);
            Printf("tcp thread start(resume) %d \r\n" ,status);
        }
        tx_event_flags_get (&event_flags_cycle, 0xffffffff,TX_OR_CLEAR, &actual_events,TX_NO_WAIT);          // flag clear
        tx_event_flags_get (&g_wmd_event_flags, 0xffffffff,TX_OR_CLEAR, &actual_events,TX_NO_WAIT);          // flag clear

        if(TestMode != TEST_MODE){
            status = tx_thread_resume(&event_thread);               // 順番はこれでいいの？
            Printf("event thread start(resume) %d \r\n" ,status);

            tx_thread_sleep (10);
            // 2020 07 31 sakaguchi del
            //tx_event_flags_get (&g_wmd_event_flags, 0xffffffff,TX_OR_CLEAR, &actual_events,TX_NO_WAIT);          // flag clear

            //tx_event_flags_set (&g_wmd_event_flags, FLG_EVENT_INITIAL, TX_OR);                  // AT Start起動

            //status = tx_thread_resume(&auto_thread);
            //Printf("auto thread start(resume) %d \r\n" ,status);

            status = tx_thread_resume(&http_thread);
            Printf("http thread start(resume) %d \r\n" ,status);

        }
        Printf("    #######  Network Mutex release  ############\r\n");
        tx_mutex_put(&mutex_network_init);    
        
// Exit:;       //2020.01.17 コメントアウト

        Printf("Network thread end\n");

        UnitState = STATE_OK;		// OK  取りあえず

        if(TestMode != TEST_MODE){
           if(my_config.network.Phy[0] == 0){ // Ethrnet
            
                tx_event_flags_get (&event_flags_link, FLG_LINK_DOWN | FLG_LINK_REBOOT ,TX_OR_CLEAR, &actual_events,TX_WAIT_FOREVER);
                Printf("===> Ethrnet Link down %02X\r\n", actual_events);
                //if(actual_events & FLG_LINK_REBOOT){
                Printf("===> Ethrnet Link down(WIFI) Reboot %02X\r\n", actual_events);
                if(NetReboot == 0){
                    PutLog( LOG_LAN, "Ethrnet Link Down");
                }
                //}
                //else{
                //    tx_thread_sleep (100);
                //    tx_event_flags_get (&event_flags_link, FLG_LINK_UP  ,TX_OR_CLEAR, &actual_events,TX_WAIT_FOREVER);
                //    Printf("===> Ethrnet Link up %02X\r\n", actual_events);
                //}

// 2022.04.11 HTTP thread の処理が終わるまで待つ ↓
                retry_cnt = 0;
                while(1){
                    status = tx_thread_info_get(&http_thread, &name, &state, &run_count, &priority,&preemption_threshold,&time_slice,&next_thread,&suspended_thread);
                    if(state == TX_SUSPENDED){
                        Printf("HTTP Thread SUSPEND retry[%d]\r\n", retry_cnt);
                        break;
                    }
                    else if(state == TX_EVENT_FLAG){
                        Printf("HTTP Thread EVENT_FLAG retry[%d]\r\n", retry_cnt);
                        status = tx_thread_suspend(&http_thread);
                        Printf("http thread suspend %02X\r\n", status);
                        break;
                    }
                    else{
                        Printf("HTTP Thread info %02X %02X\r\n", status, state);    // httpスレッドを待つ
                    }
                    retry_cnt++;
                    if(90 <= retry_cnt){        // 90秒経過してもHTTPスレッドの処理が終わらない場合はリブート
                        Printf("reboot network_thread %02X\r\n", status);
                        tx_event_flags_set(&event_flags_reboot, FLG_REBOOT_EXEC, TX_OR);
                        status = tx_thread_suspend(&network_thread);
                    }
                    tx_thread_sleep(100);
                };

                if(NetReboot == 0){     // リンクダウン検出の場合はリブート
                    tx_thread_sleep (500);      // 5sec遅延
                    tx_event_flags_set(&event_flags_reboot, FLG_REBOOT_EXEC, TX_OR);
                    status = tx_thread_suspend(&network_thread);
                }
// 2022.04.11 HTTP thread の処理が終わるまで待つ ↑


                status = nx_ip_interface_detach(&g_ip0, 1);
                Printf("Ethernet detach %02X\r\n", status);

// sakaguchi 2020.09.16 ↓
                // DHCPクライアント停止
                if(my_config.network.DhcpEnable[0] != 0x00){
                    status = nx_dhcp_stop(&g_dhcp_client0);
                    Printf("dhcp stop status %02X\r\n", status);
                    status = nx_dhcp_reinitialize(&g_dhcp_client0);
                    Printf("dhcp reinitialize status %02X\r\n", status);
                    status = nx_dhcp_interface_disable(&g_dhcp_client0, 1);
                    Printf("dhcp disable status %02X\r\n", status);
                }
// sakaguchi 2020.09.16 ↑
            }
            else{
                // WIFI接続の場合も、Link down flagで、再起動のトリガーとする
                tx_event_flags_get (&event_flags_link, FLG_LINK_DOWN | FLG_LINK_REBOOT ,TX_OR_CLEAR, &actual_events,TX_WAIT_FOREVER); 
                Printf("===> WIFI Link down(WIFI) Reboot %02X\r\n", actual_events);
                if(NetReboot == 0){
                    PutLog( LOG_LAN, "WIFI Link Down");
                }

// 2022.04.11 HTTP thread の処理が終わるまで待つ ↓
                retry_cnt = 0;
                while(1){
                    status = tx_thread_info_get(&http_thread, &name, &state, &run_count, &priority,&preemption_threshold,&time_slice,&next_thread,&suspended_thread);
                    if(state == TX_SUSPENDED){
                        Printf("HTTP Thread SUSPEND[%d]\r\n", retry_cnt);
                        break;
                    }
                    else if(state == TX_EVENT_FLAG){
                        Printf("HTTP Thread EVENT_FLAG[%d]\r\n", retry_cnt);
                        status = tx_thread_suspend(&http_thread);
                        Printf("http thread suspend %02X\r\n", status);
                        break;
                    }
                    else{
                        Printf("HTTP Thread info %02X %02X\r\n", status, state);    // httpスレッドを待つ
                    }
                    retry_cnt++;
                    if(90 <= retry_cnt){    // 90秒経過してもHTTPスレッドの処理が終わらない場合はリブート
                        Printf("reboot network_thread %02X\r\n", status);
                        tx_event_flags_set(&event_flags_reboot, FLG_REBOOT_EXEC, TX_OR);
                        status = tx_thread_suspend(&network_thread);
                    }
                    tx_thread_sleep(100);
                };

                if(NetReboot == 0){     // リンクダウン検出の場合はリブート
                    tx_thread_sleep (500);      // 5sec遅延
                    tx_event_flags_set(&event_flags_reboot, FLG_REBOOT_EXEC, TX_OR);
                    status = tx_thread_suspend(&network_thread);
                }
// 2022.04.11 HTTP thread の処理が終わるまで待つ ↑

// sakaguchi 2020.09.16 ↓
                // DHCPクライアント停止
                if(my_config.network.DhcpEnable[0] != 0x00){
                    status = nx_dhcp_stop(&g_dhcp_client0);
                    Printf("dhcp stop status %02X\r\n", status);
                    status = nx_dhcp_reinitialize(&g_dhcp_client0);
                    Printf("dhcp reinitialize status %02X\r\n", status);
                    status = nx_dhcp_interface_disable(&g_dhcp_client0, 0);
                    Printf("dhcp disable status %02X\r\n", status);
                }
// sakaguchi 2020.09.16 ↑
//wifi_reboot();
                //WIFI_RESET_ACTIVE;    //ここでOK？？？
                //tx_thread_sleep (2); 
                //WIFI_RESET_INACTIVE;
                //tx_thread_sleep (1000);      // 10sec 
            

                //net_restart();      //  hayashi 2020.10.07
                

            }

// 2022.04.11 ↓ リブート追加のため削除  // 2023.03.01 馬鹿除けで復活
            // 2021.02.10  packet pool 枯渇対策　delete & create
            status = nx_packet_pool_delete(&g_packet_pool1);
            Printf("### delete packet_pool %02X\r\n", status);
            packet_pool_init1();
// 2022.04.11 ↑
        }
        else{
            tx_thread_suspend(&network_thread);
        }

    }  // end of while

}


/**
 * @brief   Networkのリソースのクリア
 * @param 
 * @return
 * @note    2020.10.07
 */
void net_restart(void)
{
    UINT status;

    status = nx_dhcp_delete(&g_dhcp_client0);
    Printf("dhcp delete %02X\r\n", status);
    memset(&g_dhcp_client0, 0, sizeof(g_dhcp_client0));

    status = nx_dns_delete(&g_dns0);
    Printf("dns delete %02X\r\n", status);
    memset(&g_dns0, 0, sizeof(g_dns0));

    status = nx_sntp_client_delete(&g_sntp0);
    Printf("sntp delete %02X\r\n", status);
    memset(&g_sntp0, 0, sizeof(g_sntp0));

    status = nx_ip_delete(&g_ip0);
    Printf("ip delete %02X\r\n", status);
    memset(&g_ip0, 0, sizeof(g_ip0));

    status = nx_packet_pool_delete(&g_packet_pool0);
    Printf("packet pool delete %02X\r\n", status);
    memset(&g_packet_pool0, 0, sizeof(g_packet_pool0));

    Printf("nx_common_init0\r\n");
    nx_common_init0();
    Printf("nx_secure_common_init\r\n");
    nx_secure_common_init();                // ここで暴走
    Printf("packet_pool_init0\r\n");
    packet_pool_init0();
    Printf("ip_init0\r\n");
    ip_init0();
    Printf("dhcp_client_init0\r\n");
    dhcp_client_init0();
    Printf("sntp_client_init0\r\n");
    sntp_client_init0();
    Printf("dns_client_init0\r\n");
    dns_client_init0();

}

void mac_get(void)
{
    UINT status;
    uint32_t physical_msw; 
    uint32_t physical_lsw;

// 取り合えずここで、実際はSFMのSystemエリアに書かれている
    net_address.eth.mac1 = net_address.eth.mac2 = 0;
    status = nx_ip_interface_physical_address_get(&g_ip0, 0, &physical_msw, &physical_lsw);
    if(status == NX_SUCCESS){
        net_address.eth.mac1 = physical_msw;
        net_address.eth.mac2 = physical_lsw;

    }

    Printf("MAC %04X-%04X   %04X-%04X \r\n", physical_msw,physical_lsw , net_address.eth.mac1,net_address.eth.mac2);
    status = nx_ip_interface_physical_address_get(&g_ip0, 1, &physical_msw, &physical_lsw);
    Printf("MAC %04X-%04X   %04X-%04X \r\n", physical_msw,physical_lsw , net_address.eth.mac1,net_address.eth.mac2);
    DebugLog(LOG_DBG, "MAC %04X-%04X   %04X-%04X \r\n", physical_msw,physical_lsw , net_address.eth.mac1,net_address.eth.mac2);

}


void interface_select(void)
{
        if(my_config.network.Phy[0] == 0){
            net_cfg.interface_index = 1;        // Ethrenet
            LED_Set( LED_WIFION, LED_OFF );
        }
        else{
            net_cfg.interface_index = 0;        // WIFI
            LED_Set( LED_WIFION, LED_ON );
        }

}




#define PRIMARY_INTERFACE (1)
/**
 *
 * @param ip_ptr
 * @param interface_index
 * @param link_up
 */
void link_status_change_notify ( NX_IP *ip_ptr, UINT interface_index, UINT link_up)
{
    SSP_PARAMETER_NOT_USED(ip_ptr);
    UINT status;
    
    if(PRIMARY_INTERFACE == interface_index)
    {
        if (NX_TRUE == link_up)
        {
            Link_Status = 1;
            tx_event_flags_set (&event_flags_link, 0x00000000, TX_AND);
            status = tx_event_flags_set (&event_flags_link, FLG_LINK_UP, TX_OR);
            Printf("####   Link Up  (%d)\r\n", interface_index);
            Net_LOG_Write(0,"%04X",0);                      // ネットワーク通信結果の初期化
        }
        else if (NX_FALSE == link_up){
            Link_Status = 0;
            tx_event_flags_set (&event_flags_link, 0x00000000, TX_AND);
            status = tx_event_flags_set (&event_flags_link, FLG_LINK_DOWN, TX_OR);
            Printf("####   Link Down (%d) \r\n",interface_index);
            if(my_config.network.Phy[0] == ETHERNET){   // Ethernet
                Net_LOG_Write( 402, "%04X", 0 );            // Linkエラー
            }else{
                Net_LOG_Write( 400, "%04X", 0 );            // 無線LANAP検出エラー
            }
        }

        Printf("  link_status_change_notify  Link status %02X\r\n", status);        //2020.01.21 仮
    }
}
#endif

#if 0
#include "MyDefine.h"
#include "flag_def.h"
#include "Globals.h"
#include "General.h"
#include "Config.h"
#include "Log.h"
#include "network_thread.h"
#include "event_thread.h"
#include "http_thread.h"
#include "ftp_thread.h"
#include "http_thread.h"
#include "smtp_thread.h"
#include "auto_thread.h"
#include "system_thread.h"
#include "led_thread_entry.h"
#include "wifi_thread.h"
#include "usb_thread_entry.h"

extern TX_THREAD udp_thread;
extern TX_THREAD tcp_thread;
//extern TX_THREAD ftp_thread;
extern TX_THREAD http_thread;           //sakaguchi
//extern TX_THREAD smtp_thread;
extern TX_THREAD event_thread;
//extern TX_THREAD auto_thread;     //未使用
extern TX_THREAD wifi_thread;
extern TX_THREAD network_thread;

//プロトタイプ
void link_status_change_notify ( NX_IP *ip_ptr, UINT interface_index, UINT link_up);

void mac_get(void);
void interface_select(void);

extern uint32_t check_wifi_para(void);

/** Network Thread entry function */
void network_thread_entry(void)
{

    UINT status;
    ULONG actual_events;
//    ULONG actual_status;      2020.01.16コメントアウト

    CHAR *name;
    UINT state;
    ULONG run_count;
    UINT error_count;
    UINT priority;
    UINT preemption_threshold;
    ULONG time_slice;
    TX_THREAD *next_thread;
    TX_THREAD *suspended_thread;
//    int i;
//    ssp_err_t err;     //未使用

    NetReboot = 0;
    Link_Status = 0;                     // Ethrnet Link Down 
    Sntp_Status = SNTP_ERROR;            // SNTP error              // sakaguchi UT-0035
    nx_common_init0();
    nx_secure_common_init();

         
    packet_pool_init1();
    packet_pool_init0();

    ip_init0();

    dhcp_client_init0();
    nx_dhcp_interface_disable(&g_dhcp_client0, 0);  // nx_dhcp_createを実行すると、DHCPに対しインタフェースが自動で有効となるためここで停止させる sakaguchi 2020.08.31
    nx_dhcp_interface_disable(&g_dhcp_client0, 1);  // 有効になるのはプライマリインタフェース(wifi)だけだが念のためLANも停止させる sakaguchi 2020.08.31
    sntp_client_init0();
    dns_client_init0();
    
    tx_mutex_get(&mutex_network_init, TX_WAIT_FOREVER);     // sakaguchi 2020.09.14
    Printf("    Net Network Mutex get\r\n");
    
    for(;;){
        NetStatus = NETWORK_DOWN;			    // 0: 未接続 	1: 接続完了

        error_count = 0;

        tx_event_flags_get (&event_flags_link, 0xffffffff,TX_OR_CLEAR, &actual_events,TX_NO_WAIT);          // flag clear
        Printf("    Net Network Mutex get wait\r\n");
        //tx_mutex_get(&mutex_network_init, TX_WAIT_FOREVER);    // sakaguchi 2020.09.14 上に移動
        //Printf("    Net Network Mutex get\r\n");
  
        interface_select();
ReStart:;
        //Printf("inteface select \r\n");
        
        if(net_cfg.interface_index == 0){  //WIFI
             if(check_wifi_para() != 0){
                LED_Set( LED_DIAG, LED_ON );
                tx_thread_sleep (500);      // 5sec 
                goto ReStart;
            }
        }

        interface_select();

        LED_Set( LED_DIAG, LED_ON );
        
        tx_thread_sleep (100); 
        Printf("event_flags_network clear \r\n");
        tx_event_flags_get (&event_flags_network, 0xffffffff,TX_OR_CLEAR, &actual_events,TX_NO_WAIT);  
        //Printf("              wifi thread start(resume) \r\n");

        Printf("WIFI Thread info-0 %02X(%02X)\r\n", state, status);
        status =tx_thread_info_get(&wifi_thread, &name, &state, &run_count,
        		&priority,&preemption_threshold,&time_slice,&next_thread,&suspended_thread);
        Printf("WIFI Thread info-1 %02X(%02X)\r\n", state, status);

            

        status =tx_thread_resume(&wifi_thread);
        Printf("    wifi thread start 1 (resume) %d \r\n" ,status);
        /*
        for(;;){
            status =tx_thread_info_get(&wifi_thread, &name, &state, &run_count,
        		    &priority,&preemption_threshold,&time_slice,&next_thread,&suspended_thread);
            Printf("WIFI Thread info-3 %02X (%02X)\r\n", state, status);
            if(state == TX_READY)
                break; 
            tx_thread_sleep (1);
        }
        */
        //status =tx_thread_resume(&wifi_thread);
        //Printf("              wifi thread start(resume) %d \r\n" ,status);

        Printf("    wifi thread flag wait \r\n");
        status = tx_event_flags_get (&event_flags_network, FLG_DHCP_SUCCESS | FLG_DHCP_ERROR , TX_OR, &actual_events,TX_WAIT_FOREVER);
        if(actual_events & FLG_DHCP_ERROR){
            Printf("Network Thread Error %ld\r\n", error_count);
            if(isUSBState() == USB_DISCONNECT){
                //LED_Set(LED_LANON, LED_OFF);
                LED_Set( LED_NETER, LED_ON );
                error_count ++;
            }
            tx_thread_sleep (1000);         // 10sec 
            goto ReStart;
        }

        error_count = 0;
        
        Printf("Network Setup Success \r\n");

        mac_get();
        //###################################################

        NetStatus = NETWORK_UP;			    // 0: 未接続 	1: 接続完了
        Printf("Network Setup Success 2\r\n");
        //LED_Set(LED_DIAGOFF, LED_OFF);     // 
        LED_Set( LED_NETER, LED_OFF );
        LED_Set(LED_DIAGOFF, LED_OFF);     // 

        tx_thread_sleep (100);
        LED_Set( LED_LANON, LED_ON );
        //Printf("    #######  Network Mutex release  ############\r\n");
        //tx_mutex_put(&mutex_network_init);    

        if(TestMode != TEST_MODE){

            status = tx_thread_resume(&udp_thread);
            Printf("udp thread start(resume) %d \r\n" ,status);

            status = tx_thread_resume(&tcp_thread);
            Printf("tcp thread start(resume) %d \r\n" ,status);
        }
        tx_event_flags_get (&event_flags_cycle, 0xffffffff,TX_OR_CLEAR, &actual_events,TX_NO_WAIT);          // flag clear
        tx_event_flags_get (&g_wmd_event_flags, 0xffffffff,TX_OR_CLEAR, &actual_events,TX_NO_WAIT);          // flag clear

        if(TestMode != TEST_MODE){
            status = tx_thread_resume(&event_thread);               // 順番はこれでいいの？
            Printf("event thread start(resume) %d \r\n" ,status);

            tx_thread_sleep (10);
            // 2020 07 31 sakaguchi del
            //tx_event_flags_get (&g_wmd_event_flags, 0xffffffff,TX_OR_CLEAR, &actual_events,TX_NO_WAIT);          // flag clear

            //tx_event_flags_set (&g_wmd_event_flags, FLG_EVENT_INITIAL, TX_OR);                  // AT Start起動

            //status = tx_thread_resume(&auto_thread);
            //Printf("auto thread start(resume) %d \r\n" ,status);

            status = tx_thread_resume(&http_thread);
            Printf("http thread start(resume) %d \r\n" ,status);

        }
        Printf("    #######  Network Mutex release  ############\r\n");
        tx_mutex_put(&mutex_network_init);    
        
// Exit:;       //2020.01.17 コメントアウト

        Printf("Network thread end\n");

        UnitState = STATE_OK;		// OK  取りあえず

        if(TestMode != TEST_MODE){
           if(my_config.network.Phy[0] == 0){ // Ethrnet

                tx_event_flags_get (&event_flags_link, FLG_LINK_DOWN | FLG_LINK_REBOOT ,TX_OR_CLEAR, &actual_events,TX_WAIT_FOREVER);   // sakaguchi 2020.09.14
                tx_mutex_get(&mutex_network_init, TX_WAIT_FOREVER);     // sakaguchi 2020.09.14
                Printf("    Net Network Mutex get\r\n");
            
                //tx_event_flags_get (&event_flags_link, FLG_LINK_DOWN | FLG_LINK_REBOOT ,TX_OR_CLEAR, &actual_events,TX_WAIT_FOREVER);
                Printf("===> Ethrnet Link down %02X\r\n", actual_events);
                //if(actual_events & FLG_LINK_REBOOT){
                Printf("===> Ethrnet Link down(WIFI) Reboot %02X\r\n", actual_events);;
                PutLog( LOG_LAN, "Ethrnet Link Down");
                //}
                //else{
                //    tx_thread_sleep (100);
                //    tx_event_flags_get (&event_flags_link, FLG_LINK_UP  ,TX_OR_CLEAR, &actual_events,TX_WAIT_FOREVER);
                //    Printf("===> Ethrnet Link up %02X\r\n", actual_events);
                //}
                
// sakaguchi 2020.09.14-2 ↓
                // udp tcp httpスレッドをサスペンド
                status =tx_thread_suspend(&udp_thread);
                status =tx_thread_suspend(&tcp_thread);
                status =tx_thread_suspend(&event_thread);
                status =tx_thread_suspend(&http_thread);

                // DNSクライアント削除
                nx_dns_server_remove_all(&g_dns0);
                status = nx_dns_delete(&g_dns0);
                Printf("dns stop status %02X\r\n", status);

                // SNTPクライアント削除
                status = nx_sntp_client_delete(&g_sntp0);
                Printf("sntp stop status %02X\r\n", status);

                // DHCPクライアント停止
                if(my_config.network.DhcpEnable[0] != 0x00){
                    status = nx_dhcp_stop(&g_dhcp_client0);
                    Printf("dhcp stop status %02X\r\n", status);
                    status = nx_dhcp_reinitialize(&g_dhcp_client0);
                    Printf("dhcp reinitialize status %02X\r\n", status);
                    status = nx_dhcp_interface_disable(&g_dhcp_client0, 1);
                    Printf("dhcp disable status %02X\r\n", status);
                }
                // DHCPクライアント削除
                status = nx_dhcp_delete(&g_dhcp_client0);
                Printf("dhcp delete status %02X\r\n", status);
// sakaguchi 2020.09.14-2 ↑

                status = nx_ip_interface_detach(&g_ip0, 1);
                Printf("Ethernet detach %02X\r\n", status);

                nx_ip_delete(&g_ip0);       // IPインスタンス削除
                ip_init0();                 // IPインスタンス生成

// sakaguchi 2020.09.14-2 ↓
                dhcp_client_init0();
                nx_dhcp_interface_disable(&g_dhcp_client0, 0);  // nx_dhcp_createを実行すると、DHCPに対しインタフェースが自動で有効となるためここで停止させる
                nx_dhcp_interface_disable(&g_dhcp_client0, 1);  // 有効になるのはプライマリインタフェース(wifi)だけだが念のためLANも停止させる
                sntp_client_init0();
                dns_client_init0();
// sakaguchi 2020.09.14-2 ↑

            }
            else{
                // WIFI接続の場合も、Link down flagで、再起動のトリガーとする
                tx_event_flags_get (&event_flags_link, FLG_LINK_DOWN | FLG_LINK_REBOOT ,TX_OR_CLEAR, &actual_events,TX_WAIT_FOREVER); 
                tx_mutex_get(&mutex_network_init, TX_WAIT_FOREVER);     // sakaguchi 2020.09.14
                Printf("    Net Network Mutex get\r\n");

                Printf("===> WIFI Link down(WIFI) Reboot %02X\r\n", actual_events);
                PutLog( LOG_LAN, "WLAN Link Down");

// sakaguchi 2020.09.14 ↓
                // udp tcp httpスレッドをサスペンド
                status =tx_thread_suspend(&udp_thread);
                status =tx_thread_suspend(&tcp_thread);
                status =tx_thread_suspend(&event_thread);
                status =tx_thread_suspend(&http_thread);

                // DNSクライアント削除
                nx_dns_server_remove_all(&g_dns0);
                status = nx_dns_delete(&g_dns0);
                Printf("dns stop status %02X\r\n", status);

                // SNTPクライアント削除
                status = nx_sntp_client_delete(&g_sntp0);
                Printf("sntp stop status %02X\r\n", status);

                // DHCPクライアント停止
                if(my_config.network.DhcpEnable[0] != 0x00){
                    status = nx_dhcp_stop(&g_dhcp_client0);
                    Printf("dhcp stop status %02X\r\n", status);
                    status = nx_dhcp_reinitialize(&g_dhcp_client0);
                    Printf("dhcp reinitialize status %02X\r\n", status);
                    status = nx_dhcp_interface_disable(&g_dhcp_client0, 1);
                    Printf("dhcp disable status %02X\r\n", status);
                }
                // DHCPクライアント削除
                status = nx_dhcp_delete(&g_dhcp_client0);
                Printf("dhcp delete status %02X\r\n", status);

                WIFI_RESET_ACTIVE;    //ここでOK？？？
                tx_thread_sleep (2); 
                WIFI_RESET_INACTIVE;
                tx_thread_sleep (1000);      // 10sec 
            
                nx_ip_delete(&g_ip0);       // IPインスタンス削除
                ip_init0();                 // IPインスタンス生成

                dhcp_client_init0();
                nx_dhcp_interface_disable(&g_dhcp_client0, 0);  // nx_dhcp_createを実行すると、DHCPに対しインタフェースが自動で有効となるためここで停止させる
                nx_dhcp_interface_disable(&g_dhcp_client0, 1);  // 有効になるのはプライマリインタフェース(wifi)だけだが念のためLANも停止させる
                sntp_client_init0();
                dns_client_init0();
// sakaguchi 2020.09.14 ↑
            }
        }
        else{
            tx_thread_suspend(&network_thread);
        }

    }  // end of while

}


void mac_get(void)
{
    UINT status;
    uint32_t physical_msw; 
    uint32_t physical_lsw;

// 取り合えずここで、実際はSFMのSystemエリアに書かれている
    net_address.eth.mac1 = net_address.eth.mac2 = 0;
    status = nx_ip_interface_physical_address_get(&g_ip0, 0, &physical_msw, &physical_lsw);
    if(status == NX_SUCCESS){
        net_address.eth.mac1 = physical_msw;
        net_address.eth.mac2 = physical_lsw;

    }

    Printf("MAC %04X-%04X   %04X-%04X \r\n", physical_msw,physical_lsw , net_address.eth.mac1,net_address.eth.mac2);
    status = nx_ip_interface_physical_address_get(&g_ip0, 1, &physical_msw, &physical_lsw);
    Printf("MAC %04X-%04X   %04X-%04X \r\n", physical_msw,physical_lsw , net_address.eth.mac1,net_address.eth.mac2);

}


void interface_select(void)
{
        if(my_config.network.Phy[0] == 0){
            net_cfg.interface_index = 1;        // Ethrenet
            LED_Set( LED_WIFION, LED_OFF );
        }
        else{
            net_cfg.interface_index = 0;        // WIFI
            LED_Set( LED_WIFION, LED_ON );
        }

}




#define PRIMARY_INTERFACE (1)
/**
 *
 * @param ip_ptr
 * @param interface_index
 * @param link_up
 */
void link_status_change_notify ( NX_IP *ip_ptr, UINT interface_index, UINT link_up)
{
    SSP_PARAMETER_NOT_USED(ip_ptr);
    UINT status;
    
    if(PRIMARY_INTERFACE == interface_index)
    {
        if (NX_TRUE == link_up)
        {
            Link_Status = 1;
            tx_event_flags_set (&event_flags_link, 0x00000000, TX_AND);
            status = tx_event_flags_set (&event_flags_link, FLG_LINK_UP, TX_OR);
            Printf("####   Link Up  (%d)\r\n", interface_index);
            Net_LOG_Write(0,"%04X",0);                      // ネットワーク通信結果の初期化
        }
        else if (NX_FALSE == link_up){
            Link_Status = 0;
            tx_event_flags_set (&event_flags_link, 0x00000000, TX_AND);
            status = tx_event_flags_set (&event_flags_link, FLG_LINK_DOWN, TX_OR);
            Printf("####   Link Down (%d) \r\n",interface_index);
            if(my_config.network.Phy[0] == ETHERNET){   // Ethernet
                Net_LOG_Write( 402, "%04X", 0 );            // Linkエラー
            }else{
                Net_LOG_Write( 400, "%04X", 0 );            // 無線LANAP検出エラー
            }
        }

        Printf("  link_status_change_notify  Link status %02X\r\n", status);        //2020.01.21 仮
    }
}
#endif
