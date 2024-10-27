/**
 * @file    system_thread_entry.c
 * @brief
 * @note    2020.01.30  v6 0130 ソース反映
 */
/*

#define SEMI_HOSTING

#if    defined(SEMI_HOSTING)
#include <stdio.h>
#if    defined(__GNUC__)
extern void initialise_monitor_handles(void);
#endif
#endif
*/


#include "bsp_api.h"
#include "common_data.h"
#include "MyDefine.h"
#include "General.h"
#include "Version.h"
#include "flag_def.h"
#include "Log.h"

#include "system_thread.h"
//#include "ble_thread.h"
//#include "event_thread.h"
#include "rf_thread.h"
#include "network_thread.h"
//#include "usb_thread.h"
#include "led_thread.h"
//#include "led_cyc_thread.h"

#include "system_time.h"

//#include "TestData.h"
#include "Sfmem.h"
#include "Base.h"
#include "opt_cmd_thread_entry.h"

#include "Suction.h"
#include "Log.h"
#include "PDown.h"
#include "led_thread_entry.h"
//#include "smtp_thread_entry.h"
#include "RTT/SEGGER_RTT.h"
#include "Globals.h"
#include "rf_thread_entry.h"
#include "tx_api.h"
#include "ble_thread_entry.h"
#include "usb_thread_entry.h"
#include "event_thread_entry.h"
#include "cmd_thread_entry.h"
#include "Rfunc.h"
#include "power_down_thread.h"
#include "tls_client.h"
#include "auto_thread_entry.h"
#include "Net_cache.h"
#include "new_g.h"                      // 2023.05.31

extern TX_THREAD network_thread;
//extern TX_THREAD ble_thread;
//extern TX_THREAD system_thread;
extern TX_THREAD usb_thread;
extern TX_THREAD led_thread;
//extern TX_THREAD led_cyc_thread;
extern TX_THREAD rf_thread;
//extern TX_THREAD event_thread;
extern TX_THREAD power_down_thread;

extern UINT g_ftp_server_control_port_user;
extern UINT g_ftp_server_data_port_user;

#define NX_FTP_SERVER_CONTROL_PORT          g_ftp_server_control_port_user          /* Control Port for FTP server                         */

#define NX_FTP_SERVER_DATA_PORT             g_ftp_server_data_port_user          /* Data Port for FTP server in active transfer mode    */

cgc_clock_t             clock_type;
cgc_clock_t             clock_out_type;
cgc_system_clock_cfg_t  clock_config;

char ver[32];

//プロトタイプ
static int read_system_settings(void);

// リリース時はここをコメントアウト
#ifndef DBG_TERM
//extern void initialise_monitor_handles(void);         //関数本体未定義なのでコメントアウト 2020.01.27
//#define initialise_monitor_handles SEGGER_RTT_Init

void tx_application_define_user( void * first_unused_memory );
/**
 *
 * @param first_unused_memory
 */
void tx_application_define_user( void * first_unused_memory )
{
    (void)(first_unused_memory);
//    initialise_monitor_handles();
    SEGGER_RTT_Init();          //2020.01.27    追加
}
#endif

















// このThreadでは、無線・USB・Network等を実行しないこと
// 関数内部を確認のこと

/** System Thread entry function */
void system_thread_entry(void)
{
    /* TODO: add your own code here */
    ULONG actual_events;
    rtc_time_t ct;
    int size = 0;
    ssp_err_t err;
 //   char ver[32];
    UCHAR ucChg = 0;                    // sakaguchi 2020.11.11
    uint16_t WebInterval;               // sakaguchi 2021.02.17

    //deep_standby_check();
    for(;;){
        bsp_exbus_init();               // 外部SRAM設定

        //tx_event_flags_get (&event_flags_reboot, 0xffffffff,  TX_OR_CLEAR, &actual_events, TX_NO_WAIT);
//20200626        USBState = USB_DISCONNECT;                   // USB disconnect
        // これを入れないと、EventThreadでフラグ待ちが発生する
//20200626        tx_event_flags_get (&event_flags_usb, 0xffffffff,  TX_OR_CLEAR, &actual_events, TX_NO_WAIT);
//20200626        tx_event_flags_set (&event_flags_usb, FLG_USB_DOWN, TX_OR);       //本来のFLG_USB_DOWNイベントではない event_threadを少し動作させるため？

        tx_thread_sleep (120);      //1200ms

        system_time_init();
        
        TIMER.Create();

        UnitState = STATE_INIT;				// 初期化中
//        BleStatus = BLE_DISCONNECT;       //BLEスレッドで初期化
        mate_time_flag = 0;

        mate_at_start = 0;
        EWAIT_res = 0;

        Net_LOG_Write(0,"%04X",0);                // ネットワーク通信結果の初期化

//        g_ioport.p_api->pinWrite(PORT_BLE_FREEZE, IOPORT_LEVEL_LOW);     // BLE Freez //BLEスレッドで初期化
       

        Printf("system thread start !! %d \r\n", size);
        
        err = g_cgc.p_api->clockStart( CGC_CLOCK_SUBCLOCK,NULL );
        if (SSP_SUCCESS != err && SSP_ERR_CLOCK_ACTIVE!= err)
        {
            ;//while (1);
        }

        err = g_cgc.p_api->clockOutCfg(CGC_CLOCK_SUBCLOCK, CGC_CLOCKOUT_DIV_1);
        err = g_cgc.p_api->clockOutEnable();



        // 125ms Timer start
        err = g_timer0.p_api->open(g_timer0.p_ctrl, g_timer0.p_cfg);
        err = g_timer0.p_api->start(g_timer0.p_ctrl);

        // 10ms Timer start
        err = g_timer1.p_api->open(g_timer1.p_ctrl, g_timer1.p_cfg);
        err = g_timer1.p_api->start(g_timer1.p_ctrl);

        // 1ms Timer start
        err = g_timer2.p_api->open(g_timer2.p_ctrl, g_timer2.p_cfg);
        err = g_timer2.p_api->start(g_timer2.p_ctrl);

        // SPI SFM 
        err = g_spi.p_api->open(g_spi.p_ctrl, g_spi.p_cfg);

        // Power Down IRQ
        //err = g_external_irq6.p_api->open(g_external_irq6.p_ctrl, g_external_irq6.p_cfg);

        body_get_count = 0; // debug
        led_timer_cbsy = 0;
        led_timer_alarm = 0;
        led_timer_net_err = 0;
        tx_thread_resume(&led_thread);

        tx_thread_sleep (5);


        // XRAM FileX start
        err = fx_media_init0_format();
        Printf("filex format %02X\r\n", err);
        err = fx_media_init0_open();
        Printf("filex open %02X\r\n", err);

        LED_Set( LED_START, LED_ON );
	    //LED_Set( LED_INIT,  LED_ON);
	    //LED_Set( LED_READY, LED_ON);
        tx_thread_sleep (2);

        //LED_Set( LED_WARNING, LED_ON);
        
        tx_thread_resume(&power_down_thread);       // 2020.09.17

        GetLogInfo(0);      // user area
        GetLogInfo(1);      // debug area

// 2023.05.26 ↓
        kk_delta_clear_all();
#if CONFIG_NEW_STATE_CTRL
        nsc_debug_dump_auto();
#endif
// 2023.05.26 ↑

        //g_rtc.p_api->calendarTimeGet(g_rtc.p_ctrl, &ct);
        with_retry_calendarTimeGet("system_thread_entry", &ct);     // 2022.09.09
        Printf("%d/%d/%d %d:%d:%d \r\n", ct.tm_year+1900,ct.tm_mon+1,ct.tm_mday,ct.tm_hour,ct.tm_min,ct.tm_sec);

        if(read_system_settings() != 0){     //生産時設定エリアをＳＲＡＭへ読み込み
            PutLog( LOG_SYS, "System File Error");
            DebugLog( LOG_DBG, "System File Error");
        }
        read_my_settings();         //本体設定をＳＲＡＭへ読み込み
        ceat_file_read(0, 0);   // WSS1
        ceat_file_read(1, 0);   // USER1

// 2023.05.31 ↓ 上から移動
        if(fact_config.Vender == VENDER_ESPEC){
            PutLog( LOG_SYS, "Welcome to RT24BN %s",  VERSION_FW);
        }else{
            PutLog( LOG_SYS, "Welcome to RTR500BW - T&D Corporation %s",  VERSION_FW);
        }
// 2023.05.31 ↑
        //PutLog( LOG_SYS, "F/W Build Date - %s %s", __DATE__, __TIME__ );
        //PutLog( LOG_SYS, "F/W Build Date - %s / %s", __DATE__, VERSION_FW);
#ifdef DBG_TERM
        PutLog( LOG_SYS, "F/W Build Date - %s %s / %s", __DATE__, __TIME__,VERSION_FW);
#endif        
        DebugLog( LOG_DBG, "F/W Build Date - %s %s / %s", __DATE__, __TIME__, VERSION_FW);



        g_ftp_server_control_port_user = *(uint16_t *)&my_config.ftp.Port[0];
        g_ftp_server_data_port_user = 20;
// Test HTTP
//*(uint16_t *)&my_config.websrv.Port[0] = 80;
//my_config.websrv.Mode[0] = 0;
        RTC_Create();

/*  この方法はやめる
        // Hitach VerでUpdateした場合は、強制的にHitachiに設定する
        sprintf(ver, "%s", VERSION_FW);
        Printf("Ver %s\r\n", ver);
        if(ver[0] == 'H'){
            fact_config.Vender = VENDER_HIT;
        }
 */   
        //if(fact_config.Vender != VENDER_HIT){
        //    PutLog( LOG_SYS, "F/W Build Date(for Debug) - %s %s / %s", __DATE__, __TIME__,VERSION_FW);
        //}

        EX_WRAN_OFF;
        // Power Down IRQ
        err = g_external_irq6.p_api->open(g_external_irq6.p_ctrl, g_external_irq6.p_cfg);  

        LED_Set( LED_READY, LED_ON );
        tx_thread_sleep (100);

        dns_local_cache_init();                                 // sakaguchi 2021.07.20

        FirstAlarmMoni = 1;     // 警報監視１回目ＯＮ             // sakaguchi 2021.03.01
        WR_clr_rfcnt();         // 無線エラー回数クリア           // sakaguchi 2021.04.07

//      RF_power_on(0);			// 自律無線通信で使う警報カウンタ、データ番号などをクリアする。	segi	AT_start()でやるので削除	segi 2020/08/22
        RF_full_moni_init();    // モニタリング用変数の初期化   // 2020/08/24 sakaguchi add
        memset(G_HttpFile, 0x00, sizeof(G_HttpFile));           // sakaguchi 2020.09.04
        memset(G_HttpCmd, 0x00, sizeof(G_HttpCmd));             // sakaguchi 2020.09.04

        // MTU追加 sakaguchi 2021.04.07
        if((*(uint16_t *)&my_config.network.Mtu[0] < 576) || (*(uint16_t *)&my_config.network.Mtu[0] > 1400)){
            *(uint16_t *)&my_config.network.Mtu[0] = 1400;
        }

        // 通信間隔 sakaguchi 2021.05.28
        if((my_config.network.Interval[0] < 0) || (my_config.network.Interval[0] > 200)){
            my_config.network.Interval[0] = 0;
        }
        //my_config.network.Interval[0] = 70;       // サラヤ向け(Ver16.1,Ver17.1)は70ms固定にする      // sakaguchi 2021.07.16 正式版は設定で可変とする

        //tx_thread_resume(&rf_thread);
        //err = g_external_irq2.p_api->open(g_external_irq2.p_ctrl, g_external_irq2.p_cfg);    // BLE Rx irq2

        tx_event_flags_get (&event_flags_reboot, 0xffffffff,  TX_OR_CLEAR, &actual_events, TX_NO_WAIT);
        
//        tx_thread_resume(&cmd_thread);              // sakaguchi add コマンドスレッド起動（サスペンドさせない）
        /*
        if(fact_config.Vender != VENDER_HIT){
            if(my_config.ble.active == 0){
                tx_event_flags_set( &g_ble_event_flags, FLG_PSOC_FREEZ_REQUEST, TX_OR); //PSoC FREEZ要求
            }
            tx_thread_resume(&ble_thread);
        }
        else{
            LED_Set( LED_BLEOFF, LED_OFF );
        }
        */
        if(fact_config.Vender == VENDER_HIT){
            LED_Set( LED_BLEOFF, LED_OFF );
        }

        //tx_thread_resume(&usb_thread);
        //tx_thread_resume(&rf_thread);
        
        LED_Set( LED_DIAG, LED_ON );

        if(fact_config.SerialNumber != 0xFFFFFFFF){     // 未検査の場合は、Networkを起動しない

            tx_thread_sleep (100);
            tx_thread_resume(&rf_thread);

            tx_thread_sleep (100);
            tx_thread_sleep (100);
            tx_thread_sleep (100);
            tx_thread_resume(&usb_thread);
            tx_thread_resume(&network_thread);
            tx_thread_sleep (200);

            if(fact_config.Vender != VENDER_HIT){
               if(my_config.ble.active == 0){
                    tx_event_flags_set( &g_ble_event_flags, FLG_PSOC_FREEZ_REQUEST, TX_OR); //PSoC FREEZ要求
                }
                tx_thread_resume(&ble_thread);
            }

// sakaguchi 2020.11.11 ↓
            tx_thread_sleep (800);                  // 8sec sleep
            // 親機ファームウェアバージョンチェック
            if(0 != memcmp(&my_config.version.OyaFirm[0], VERSION_FW, sizeof(VERSION_FW))){
                sprintf(&my_config.version.OyaFirm[0],"%s",VERSION_FW);
                my_config.device.SysCnt++;
                ucChg++;
            }
            // 無線モジュールバージョンチェック
            if(my_config.version.RfFirm != (regf_rfm_version_number & 0x0000ffff)){
                my_config.version.RfFirm = (regf_rfm_version_number & 0x0000ffff);
                my_config.device.SysCnt++;
                ucChg++;
            }
            // BLEファームウェアバージョンチェック
            if(0 != memcmp(&my_config.version.BleFirm[0], &psoc.revision[0], sizeof(psoc.revision))){
                memcpy(&my_config.version.BleFirm[0], &psoc.revision[0], sizeof(psoc.revision));
                my_config.device.SysCnt++;
                ucChg++;
            }
            // バージョン更新有り
            if(0 < ucChg){
                rewrite_settings();     // 本体設定書き換え
                read_my_settings();     // 本体設定をＳＲＡＭへ読み込み
                //vhttp_sysset_sndon();   // 親機設定送信
                //if(HTTP_SEND == Http_Use) vhttp_sysset_sndon();   // 親機設定送信   // sakaguchi 2020.12.24
                WebInterval = *(uint16_t *)my_config.websrv.Interval;                // sakaguchi 2021.02.17
                if( ( WebInterval )                                                             // HTTP通信間隔設定有り // HTTP通信条件修正 sakaguchi 2021.04.07
                 || (( my_config.warning.Enable[0] ) && ( my_config.warning.Route[0] == 3 ))    // 警報HTTP通信有り
                 || (( my_config.monitor.Enable[0] ) && ( my_config.monitor.Route[0] == 3 ))    // モニタリングHTTP通信有り
                 || (( my_config.suction.Enable[0] ) && ( my_config.suction.Route[0] == 3 )) ){ // 吸い上げHTTP通信有り
                    vhttp_sysset_sndon();   // 親機設定送信
                }
            }
// sakaguchi 2020.11.11 ↑
        }
        else{
            //tx_thread_suspend(&led_cyc_thread);
            tx_thread_resume(&usb_thread);
            tx_thread_suspend(&led_thread);

            tx_thread_resume(&rf_thread);
            
            tx_thread_resume(&ble_thread);      // 2020.10.16 検査機対応　BLEが動かない

        }   

        #ifdef DBG_TERM
        //WriteText(my_config.suction.Event0, "10#010", strlen("000000"), sizeof(my_config.suction.Event0)); //push debug
        #endif 
        //リブート実行イベントシグナル
        tx_event_flags_get (&event_flags_reboot, FLG_REBOOT_EXEC, TX_OR_CLEAR, &actual_events, TX_WAIT_FOREVER);
        tx_event_flags_get (&event_flags_reboot, 0xffffffff,  TX_OR_CLEAR, &actual_events, TX_NO_WAIT);

//        PutLog( LOG_SYS, "Reboot Event");       // debug 2020.09.17


        tx_thread_suspend(&power_down_thread);
        tx_thread_suspend(&usb_thread);
        
        tx_thread_suspend(&event_thread);
	    tx_thread_suspend(&ble_thread);
	    tx_thread_suspend(&led_thread);
	    //tx_thread_suspend(&led_cyc_thread);
	    tx_thread_suspend(&usb_thread);
	    //status = tx_thread_suspend(&udp_thread);
	    //status = tx_thread_suspend(&tcp_thread);

        __disable_irq();
        
        for(int i=0;i<10;i++){
            __NOP();
        }

        NVIC_SystemReset();          // reboot !!

        //ここから先には行かないはず！！
        while (1){
            tx_thread_sleep (1);
        }
    

        //tx_thread_suspend(&system_thread);
    }
    
 }

/**
 * 生産時設定エリアをＳＲＡＭへ読み込み
 * @note   ＣＲＣエラーなし、実質使用サイズ２５６バイトのとき処理時間約６ｍｓ
 * @note   ４ｋｂｙｔｅはシリアルフラッシュ消去の最小単位
 * @note  2020.Jul.22   関数内での直接crc32計算および spi.rxbufを使用しない様に変更
 */
int read_system_settings(void)
{
//    volatile bool blank = false;        //2019.Dec.26 コメントアウト
    int result = -1;
//    uint32_t j, crc32;
    uint8_t *dst;
    uint32_t crc1;


   Printf("\r\n");
   if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS)              // シリアルフラッシュ ミューテックス確保
   {
        // シリアルフラッシュメモリ 0x0000～0x00ff


        dst = (uint8_t *)&fact_config;

 //       serial_flash_multbyte_read(SFM_FACTORY_START, SFM_FACTORY_SIZE, NULL);
        serial_flash_multbyte_read(SFM_FACTORY_START, SFM_FACTORY_SIZE, (char *)dst);

        crc1 = crc32_bfr(dst, SFM_FACTORY_SIZE - 4);

 //       memcpy(dst, &spi.rxbuf[4], SFM_FACTORY_SIZE);

        if(crc1 != fact_config.crc){
            result = -1;                           // オリジナル域エラー
            Printf("CRC Error, read_fact_settings\r\n");
        }else{
            result = 0;
        }

        tx_mutex_put(&mutex_sfmem);
    }

      Printf("config read %02X\r\n", result);
      return(result);
}



