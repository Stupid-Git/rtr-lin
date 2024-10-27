/**
 * @brief   BLE受信スレッド
 * @file    ble_thread_entry.c
 * @note    2020.01.30  v6 0128 ソース反映済み
 * @note    2020.04.17  受信データはBCOMのバッファに直接読み込みに変更
 * @note    2020.06.04  0604ソースマージ中
 * @note    2020.06.12  スレッド名変更（ソースファイル名も変更）
 * @note    2020.Jun.16 新基板用外部割込み（ブレーク割り込み）使用（フレームワークは未使用）
 * @note    2020.Jun.22 PSoC FW更新処理をble_threadに組み込み
 * @note    2020.Jun.22 ブレーク信号割り込み導入に合わせて、各イベントフラグの処理を整理
 * @note    2020.Jul.03 CRC/SUM両対応  BLE通信（T2コマンド）ＯＫ
 * @note	2020.Aug.07	0807ソース反映済み
 * @note   2020.Sep.15 データフラッシュのブランクチェックを厳密にした
 *
 * @note    ble_thread スタック    1024Byte
 * @note    comms4 受信キュー（バッファ） 4 × 2048 Byte → 4 × 512Byteに変更
 *
 * @note BLEスレッド配下の変数、関数はこのファイル（ble_thread_entry.c）とヘッダ（ble_thread_entry.h）に集める
 * 他スレッドから参照されていてもGeneral.hやGlobals.hに置かない
 *
 *
 * @attention    ble_connect_check()を呼んでいる個所がないのでとりあえずBLEスレッドに入れてある
 */

#define _BLE_THREAD_ENTRY_C_
#include "flag_def.h"
#include "MyDefine.h"
#include "Version.h"      //COMPANY_ID
#include "Globals.h"
#include "General.h"

//#include "ble_thread.h"
#include "system_thread.h"
#include "cmd_thread.h"
#include "led_thread.h"
#include "usb_thread.h"
#include "usb_tx_thread.h"

#include "led_thread_entry.h"
#include "ble_thread_entry.h"
#include "wifi_thread_entry.h"              // lan_connect_check(), set_state_check()
#include "rf_thread_entry.h"                // Warning_Check()
#include "cmd_thread_entry.h"
#include "usb_thread_entry.h"



#include "dataflash.h"
#include "Sfmem.h"
#include "Log.h"

#include "programPSoC.h"        //PSoC FW更新用関数



extern TX_THREAD usb_tx_thread;


//extern TX_THREAD usb_thread;
//extern TX_THREAD cmd_thread;
//extern int fPSoCFwUpExec;             // PSoC FW更新実行

static def_com BCOM;                   ///<  BLE 送受信バッファ


//プロトタイプ
static int soh_command_exe(char sohcmd);
static void ble_advertise_packet(void);
static uint32_t send_PSoC(uint8_t *pData, uint32_t length, uint32_t timeout);





// 最初はペリフェラル   TR4等と通信する場合はセントラル（クライアント）


/**
 * @brief   BLE Thread(Uart4) entry function
 *
 *
 *  */
void ble_thread_entry(void)
{
    uint32_t actual_events = 0;
    uint32_t status = 0;
    ssp_err_t err;
    uint32_t len, pre_len;
//    int32_t num;
//    static uint32_t cmd_msg[4];     ///<  キューでcmd_threadに送るメッセージ
    static CmdQueue_t cmd_msg;      ///< キューでcmd_threadに送るメッセージ
    static usb_psoc_t tensou;       ///< USB PSoC間転送用 キューメッセージ
    static usb_tx_t usbTxQueue;            ///< キューでusb_tx_threadに送るメッセージ
    uint32_t rx_wait;               ///< BLEクライアント時の受信待ち時間
    static int32_t txLen;      //CmdStatusSizeの代わりに  cmd_threadから受け取るコマンド応答のサイズ
    uint8_t ble_connect = 0;        // sakaguchi 2021.01.28 BLE接続切断のログ追加
//スレッド開始、イニシャライズ

    Printf("BLE Thread(Uart4) Start\n");

    LED_Set( LED_BLEOFF, LED_OFF );

    //変数初期化
    memset((char *)&BLE_ADV_Packet, 0x00, sizeof(BLE_ADV_Packet));      //アドバタイズパケットクリア
    memset(&BCOM,0,sizeof(BCOM));                                       //送受信バッファクリア

    pPSoCUartTx = (PSoC_tx_uart_t *)&BCOM.txbuf.header;           //PSoC SOH構造体ポインタ 送信用
    pPSoCUartRx = (PSoC_rx_uart_t *)&BCOM.rxbuf.header;           //PSoC SOH構造体ポインタ 受信用

    //ペリフェラル初期化
    sf_comms_init4();

    g_ioport.p_api->pinWrite(PORT_BLE_FREEZE, IOPORT_LEVEL_LOW);     // FREEZE する
//    g_ioport.p_api->pinWrite(PORT_BLE_ROLE_CHANGE, ROLE_SERVER);     // SERVER(ペリフェラル)

#if 0
        //ここに入れてはダメ  PSoCのファーム更新等できなくなる

        for(;;){
            if(my_config.ble.active != 0){
                break;
            }
            tx_thread_sleep (10);
        }
#endif

    ble_reset();        //BLE FREEZE解除、リセット

    g_external_irq2.p_api->open(g_external_irq2.p_ctrl, g_external_irq2.p_cfg);     //IRQ2オープン
    g_external_irq2.p_api->enable(g_external_irq2.p_ctrl);                          //IRQ2イネーブル


    tx_thread_sleep (10);


    //スレッド初期化完了
    if(my_config.ble.active == 1){
        LED_Set( LED_BLEON, LED_ON );
    }
    //スレッド ループ B
    for(;;){

        //イベントフラグ発生まで待ち
        status = tx_event_flags_get( &g_ble_event_flags, FLG_BLE_ALL_EVENT, TX_OR_CLEAR, &actual_events, TX_WAIT_FOREVER );

        //イベント発生 （TX_WAIT_FOREVERなので起きたら必ずイベントは発生しているはず）

        if(actual_events & FLG_BLE_RX_REQUEST)  //受信要求イベント
        {
            //サーバー（ペリフェラル）とクライアント（セントラル）で処理が違う

            if(isBleRole() == ROLE_SERVER) //ブレーク信号による受信リクエスト
            {
SERVER_RX_RETRY:
                //受信待ち(1Byte読み出し）
                BCOM.rxbuf.header = 0xFF;
                err = g_sf_comms4.p_api->read(g_sf_comms4.p_ctrl, (uint8_t *)&BCOM.rxbuf.header, 1, 20);       //ヘッダ1Byte読み出し(受信は完了している)

                 if(err == SSP_SUCCESS){
                     if((BCOM.rxbuf.header == CODE_STX) || (BCOM.rxbuf.header == CODE_SOH)){       //STX or SOH
                         pre_len = 4; //Lengthまで5Byte
                     }
                     else if(BCOM.rxbuf.header == 'T'){
                        pre_len = 3;//Lengthまで4Byte
                    }
                    else{       //Other  NULL等
                        goto SERVER_RX_RETRY;
                    }
                 }
                 else if(err == SSP_ERR_BREAK_DETECT){
                     goto SERVER_RX_RETRY;
                 }
                 else{      //SSP_ERR_TIMEOUT
                     goto RX_COMPLETE;  //今回の受信は修了
                 }

                 //Lengthまで読み出し
                 if(BCOM.rxbuf.header == 'T'){

                     BCOM.rxbuf.command = BCOM.rxbuf.header; //'T'をコマンド位置にコピー
                     err = g_sf_comms4.p_api->read(g_sf_comms4.p_ctrl, (uint8_t*)&BCOM.rxbuf.subcommand, pre_len, 5);     //lengthまでpre_len Byte読み出し
                     Printf("  BLE T2 Recive: %02X", BCOM.rxbuf.header, (uint32_t)(BCOM.rxbuf.length + 6));
                 }
                 else{
                      err = g_sf_comms4.p_api->read(g_sf_comms4.p_ctrl, (uint8_t*)&BCOM.rxbuf.command, pre_len, 5);     //lengthまでpre_len Byte読み出し
                      Printf("  BLE SOH/STX Recive: %02X,%02X, (%d Byte)\r\n", BCOM.rxbuf.header, BCOM.rxbuf.command, (uint32_t)(BCOM.rxbuf.length + 7));
                 }

                 if(err == SSP_SUCCESS){

                     if(BCOM.rxbuf.header == CODE_SOH){    //SOHのみ STXは通常発生しない
                         //通常BLE SOHコマンド
                         //コマンドフレーム長制限
                         len = (BCOM.rxbuf.length > 1028) ? 1028 : BCOM.rxbuf.length;

                         //残りデータ読み出し
                         err = g_sf_comms4.p_api->read(g_sf_comms4.p_ctrl, (uint8_t *)BCOM.rxbuf.data, (uint32_t)(len + 2), 200); //データ部+CRC読み込み

                         soh_command_exe(BCOM.rxbuf.command);      //SOHコマンド処理(0x9Eコマンド処理)
                         goto RX_COMPLETE;

                     }
                     else if(BCOM.rxbuf.header == CODE_STX) //STXは通常発生しない
                     {
                         //スキャンデータの残りを受信することがある
                         len = (BCOM.rxbuf.length > 1028) ? 1028 : BCOM.rxbuf.length;
                          //残りデータ読み出し
                         err = g_sf_comms4.p_api->read(g_sf_comms4.p_ctrl, (uint8_t *)BCOM.rxbuf.data, (uint32_t)(len + 2), 200); //データ部+CRC読み込み

                         goto SERVER_RX_RETRY;  //受信からやり直し
                      }
                     else if(BCOM.rxbuf.header == 'T')     //T2コマンド 'T','2',Length
                     {
                         if(BCOM.rxbuf.subcommand != '2'){
                             Printf("BLE T2 error \r\n");
                             goto  RX_COMPLETE;
                         }

                         //コマンドフレーム長制限
                         len = (BCOM.rxbuf.length > 1028) ? 1028 : BCOM.rxbuf.length;//Length位置はSOHと合わせてある
                         //残りデータ読み出し
                         err = g_sf_comms4.p_api->read(g_sf_comms4.p_ctrl, (uint8_t *)BCOM.rxbuf.data, len + 2, 50);

                         Printf("\nBLE T2 %d len = %ld\n",  len);
                         //コマンドスレッド実行中は待つ 2020.Jul.15
                         while((TX_SUSPENDED != cmd_thread.tx_thread_state) /*&& (TX_EVENT_FLAG != cmd_thread.tx_thread_state)*/){
                             tx_thread_sleep (1);
                         }

                         cmd_msg.CmdRoute = CMD_BLE;       //コマンド キュー  コマンド実行要求元
                         cmd_msg.pT2Command = &BCOM.rxbuf.command;    //コマンド処理する受信データフレームの先頭ポインタ
                         cmd_msg.pT2Status = &BCOM.txbuf.header;    //コマンド処理された応答データフレームの先頭ポインタ（2020.Jun.15現在未対応）
                         cmd_msg.pStatusSize = (int32_t *)&txLen;              //コマンド処理された応答データフレームサイズ

                         tx_queue_send(&cmd_queue, &cmd_msg, TX_WAIT_FOREVER);
                         tx_event_flags_set (&g_command_event_flags, FLG_CMD_EXEC,TX_OR);
                         status = tx_thread_resume(&cmd_thread);     //コマンドスレッド起動（コマンドスレッドはサスペンドしている）

                         //コマンド処理完了待ち
                         status = tx_event_flags_get (&g_command_event_flags, FLG_CMD_END ,TX_OR_CLEAR, &actual_events,TX_WAIT_FOREVER);

                         //コマンド処理後の応答データはStsAreaに入っている
//                         err = send_PSoC((uint8_t*)&StsArea, (uint32_t)CmdStatusSize, TX_WAIT_FOREVER);
                         err = send_PSoC((uint8_t*)&BCOM.txbuf.header, (uint32_t)txLen, TX_WAIT_FOREVER);

                         Reboot_check();//REBOOT要求のチェック 2020.Jun.10 各タスクに入っていたチェック部を関数にして分離(とりあえずcmd_thread_entry.c)
                     }
                     else{
                         Printf("BLE SOH Error %d  %02X\n", err, BCOM.rxbuf.header);
                     }
                 }
                 else{
                     Printf("BLE SOH Error %d  %02X\n", err, BCOM.rxbuf.header);
                 }
            }
            else    //クライアント（セントラル）
            {
                //スキャンデータ受信中
CLIENT_RX_SCAN:
                err = g_sf_comms4.p_api->read(g_sf_comms4.p_ctrl, (uint8_t *)tensou.pPSoC2USB , 1, 50);       //ヘッダ1Byte読み出し(受信は完了している)

                 if(err == SSP_SUCCESS){
                     if((tensou.pPSoC2USB->header == CODE_STX) || (tensou.pPSoC2USB->header == CODE_SOH))     //STX or SOH
                     {
 //                       Printf("\r\n BLE SOH/STX RX Start !!  %02X, %02X\r\n", tensou.pPSoC2USB->header, tensou.pPSoC2USB->command);
                     }else if(tensou.pPSoC2USB->header == 0x00){        //NULLブレーク
                         goto CLIENT_RX_SCAN;
                     }
                     else{ //フレーム途中？
                         //受信失敗
                         tensou.pUSB2PSoC->length = 0;
                         Printf("\r\n  PSoC Illegal Header Error !! \r\n");
    //                        goto RX_COMPLETE;
                         goto CLIENT_RX_SCAN;
                     }
                }
                else if(err == SSP_ERR_BREAK_DETECT)
                {
                    goto CLIENT_RX_SCAN;
                }
                else     //SSP_ERR_TIMEOUT,
                {
                    //受信失敗(スキャンでタイムアウト指定していると時間経過後応答が止まる)
                    if(during_scan == false){
 //                       tensou.pUSB2PSoC->length = 0;
                        Printf("\r\n  PSoC Rx Time out !! \r\n");
                    }else{
                        Printf("\r\n  Scan Time out !! \r\n");
                        during_scan = false;
                    }
                    goto RX_COMPLETE;  //今回の受信は修了
                }

                 //Lengthまで読み出し
                err = g_sf_comms4.p_api->read(g_sf_comms4.p_ctrl, (uint8_t *)&tensou.pPSoC2USB->command , 4, 50);
                if(err != SSP_SUCCESS){
                    Printf("\r\n  PSoC communication Error1 !! \r\n");
                    goto RX_COMPLETE;
                }
                //残りバイト読み込み
                err = g_sf_comms4.p_api->read(g_sf_comms4.p_ctrl, (uint8_t *)tensou.pPSoC2USB->data, (uint32_t)(tensou.pPSoC2USB->length + 2), 200); //データ部+CRC読み込み
                if(err != SSP_SUCCESS){
                    Printf("\r\n  PSoC communication Error2 !! \r\n");
                    goto RX_COMPLETE;
                }

                Printf(" BLE Receive: %02X,%02X, (%d Byte)\r\n", tensou.pPSoC2USB->header, tensou.pPSoC2USB->command, (uint32_t)(tensou.pPSoC2USB->length + 7));

                //スキャンデータは直接USB送信スレッドに送る
                if((during_scan == true) && (tensou.pPSoC2USB->command == STX_SCAN_DATA)){//スキャン中
                    usbTxQueue.pTxBuf = &tensou.pPSoC2USB->header;        //送信データへのポインタ
                    usbTxQueue.length = (uint32_t)(tensou.pPSoC2USB->length + 7);   //送信データ長

                    tx_queue_send(&g_usb_tx_queue, &usbTxQueue, TX_WAIT_FOREVER);                 //メッセージキューセット
                    tx_event_flags_set( &event_flags_usb, FLG_USB_TX_REQUEST, TX_OR);     //USB送信リクエスト イベントシグナル
                    status = tx_thread_resume(&usb_tx_thread);                            //USB TX スレッド起動
                }
                else{//スキャン中以外はUSB 受信スレッドに渡す
                    tx_event_flags_set(&event_flags_usb, FLG_USB_PSOC_COMPLETE, TX_OR);
                    //スキャンデータ以外を受信しうたので
                    tx_event_flags_set( &g_ble_event_flags, FLG_BLE_RX_REQUEST, TX_OR);     //引き続きPSoC受信リクエスト
                }
//                goto RX_COMPLETE;
            }
        }
        else if(actual_events & FLG_BLE_TX_REQUEST)   //USBからPSoCへの送信要求(送信 → 受信)
        {
            //クライアントのみ

            status = tx_queue_receive(&g_ble_usb_queue, &tensou, 10);//イベントフラグ発生時メッセージキューもセットされているはず（セットしないとダメ）

            if(isBleRole() == ROLE_CLIENT){ // BLEがクライアント（セントラル）の場合

                if(TX_SUCCESS == status){
//CLIENT_TX_RETRY:
                    err = send_PSoC((uint8_t*)tensou.pUSB2PSoC, (uint32_t)(tensou.pUSB2PSoC->length + 7), TX_WAIT_FOREVER);
                }else{
                    err = status;       //メッセージキュー取得失敗（通常あり得ない）
                }

                if(SSP_SUCCESS == err)
                {
CLIENT_RX_WAIT:
                    //クライアント時の子機への接続および子機へのSOHコマンドはタイムアウトを長くする
                    if((isBleConnect() == BLE_CONNECT)
                            || (tensou.pUSB2PSoC->command == STX_CONNECTION)
                            || (tensou.pUSB2PSoC->header == CODE_SOH) )
                    {
                        rx_wait = 1000; //10秒
                    }else{
                        rx_wait = 200;   //2秒
                    }
                    err = g_sf_comms4.p_api->read(g_sf_comms4.p_ctrl, (uint8_t *)&tensou.pPSoC2USB->header , 1, rx_wait);            //1Byte読み出し
                    if(err == SSP_SUCCESS){
                         if((tensou.pPSoC2USB->header == CODE_STX) || (tensou.pPSoC2USB->header == CODE_SOH))     //STX or SOH
                         {
//                            Printf(" BLE SOH/STX RX Start !!  %02X,%02X\r\n", tensou.pPSoC2USB->header, tensou.pPSoC2USB->command);
                         }else if(tensou.pPSoC2USB->header == 0x00){        //NULLブレーク
                             goto CLIENT_RX_WAIT;
                         }
                         else{ //フレーム途中？
                             //受信失敗
                             Printf("\r\n  PSoC Illegal Header Error  !! \r\n");
                             goto CLIENT_RX_WAIT;
                         }
                    }
                    else if(err == SSP_ERR_BREAK_DETECT){
                        goto CLIENT_RX_WAIT;
                    }
                    else     //SSP_ERR_TIMEOUT,
                    {
                        Printf("\r\n  PSoC Rx Time out !! \r\n");
                        tx_event_flags_set(&event_flags_usb, FLG_USB_PSOC_ERROR, TX_OR);

                        goto RX_COMPLETE;
                    }
                    //Lengthまで読み出し
                    err = g_sf_comms4.p_api->read(g_sf_comms4.p_ctrl, (uint8_t *)&tensou.pPSoC2USB->command , 4, 50);
                    if(err != SSP_SUCCESS){
                        Printf("\r\n  PSoC communication Error1 !! \r\n");
                        goto RX_COMPLETE;
                    }
                    //残りバイト読み込み
                    err = g_sf_comms4.p_api->read(g_sf_comms4.p_ctrl, (uint8_t *)tensou.pPSoC2USB->data, (uint32_t)(tensou.pPSoC2USB->length + 2), 200); //データ部+CRC読み込み
                    if(err != SSP_SUCCESS){
                        Printf("\r\n  PSoC communication Error2 !! \r\n");
                        goto RX_COMPLETE;
                    }
                    Printf(" BLE SOH/STX RX Start !!  %02X,%02X, (%d Byte)\r\n", tensou.pPSoC2USB->header, tensou.pPSoC2USB->command,(tensou.pPSoC2USB->length + 7));

                    //スキャンデータは直接USB送信スレッドに送る
                    if(tensou.pPSoC2USB->command == STX_SCAN_DATA){

                        if(during_scan == true){//スキャン中
                            usbTxQueue.pTxBuf = &tensou.pPSoC2USB->header;        //送信データへのポインタ
                            usbTxQueue.length = (uint32_t)(tensou.pPSoC2USB->length + 7);   //送信データ長

                            tx_queue_send(&g_usb_tx_queue, &usbTxQueue, TX_WAIT_FOREVER);                 //メッセージキューセット
                            tx_event_flags_set( &event_flags_usb, FLG_USB_TX_REQUEST, TX_OR);     //USB送信リクエスト イベントシグナル
                            status = tx_thread_resume(&usb_tx_thread); //USB TX スレッド起動
                        }

                        //スキャンデータを受信してしまったので
                        goto CLIENT_RX_WAIT;        //受信からやりなおし
                    }
                    else//スキャンデータ以外はUSB 受信スレッドに渡す
                    {
                        tx_event_flags_set(&event_flags_usb, FLG_USB_PSOC_COMPLETE, TX_OR);
                    }
#if 0
                    //スキャンデータ等 STXの残りデータを受信する事がある
                    if(tensou.pPSoC2USB->command != tensou.pUSB2PSoC->command){
//                        goto CLIENT_RX_WAIT;        //受信からやりなおし
                        goto CLIENT_TX_RETRY;       //送信からやり直し
                    }
#endif

#if 0
  //USB送信時にセットする
                   if(tensou.pPSoC2USB->command == STX_START_SCAN){  // スキャン開始コマンド 0x20        //PSoCから応答が連続で来る
                      tx_event_flags_set( &g_ble_event_flags, FLG_BLE_RX_REQUEST, TX_OR);     //引き続きPSoC受信リクエスト
                   }
                   else{
                       ;
                   }
#endif
//                   goto RX_COMPLETE;
                }
                else
                {
                  ; //送信失敗 send_PSoC()  USB切断 or PC側のドライバが死んでいる
                }
            }
            else        //サーバで送信要求が来た(通常あり得ない)
            {
                ;
            }
        }
        else if(actual_events & FLG_PSOC_UPDATE_REQUEST)     //FW 更新要求
        {
            //通常サーバーのみ
            if(isBleRole() == ROLE_SERVER){
                if(1 == fPSoCUpdateMode)
                {
                    //ブランクのデータフラッシュは不定値でアクセスのたびに値が変わるので一致してしまうことがある 2020.Sep.15
                    if(FLASH_RESULT_NOT_BLANK == CheckBlank_data_flash(FLASH_UPDATE_EXEC,4)){
                    if(0x00000003 == *(uint32_t *)FLASH_UPDATE_EXEC)        //PSoC FW 更新実行フラグ
                    {
                        tx_thread_sleep (10);       //コマンド応答の処理をしている場合がるのでしばらく待つ
                        PSoC_program_FW();        //PSoC FW更新実行
                    }
                    else   //ファーム更新実行フラグが3以外の場合、ファーム更新完了する前に別種のファーム更新で上書きされた状態
                    {
                            if(FLASH_RESULT_NOT_BLANK == CheckBlank_data_flash(DATA_FLASH_ADR_PSoC_WRITE_MODE, 4)){
                        if(0x00000001 != *(uint32_t *)DATA_FLASH_ADR_PSoC_WRITE_MODE)    //PSoC ファーム書き込み中フラグチェック
                        {
                            fPSoCUpdateMode = 0;                    //書き込み途中ではないので更新実行フラグを消す
                                }
                            }
                        }
                    }
                }
             }
             else   //クライアントで要求が来た
             {
                ;
             }
        }
        else if(actual_events & FLG_PSOC_RESET_REQUEST)    //PSoC リセット要求
        {
            //サーバー、クライアント共通
            tx_event_flags_get( &g_ble_event_flags, FLG_BLE_ALL_EVENT, TX_OR_CLEAR, &actual_events, TX_NO_WAIT );   //BLEイベントシグナルをすべてクリア

            g_external_irq2.p_api->disable(g_external_irq2.p_ctrl);                          //IRQ2ディセーブル
            ble_reset();    //PSoCリセット
            g_external_irq2.p_api->enable(g_external_irq2.p_ctrl);                          //IRQ2イネーブル
            LED_Set( LED_BLEON, LED_ON );
            Printf("\r\n BLE Reset Request \r\n");

        }
        else if(actual_events & FLG_PSOC_FREEZ_REQUEST)    //PSoC FREEZ要求
        {
            //サーバー、クライアント共通
            tx_event_flags_get( &g_ble_event_flags, FLG_BLE_ALL_EVENT, TX_OR_CLEAR, &actual_events, TX_NO_WAIT );   //BLEイベントシグナルをすべてクリア

            g_ioport.p_api->pinWrite(PORT_BLE_FREEZE, IOPORT_LEVEL_LOW);     // FREEZE する
            g_ioport.p_api->pinWrite(PORT_BLE_ROLE_CHANGE, ROLE_SERVER);     // SERVER(ペリフェラル)
            LED_Set( LED_BLEOFF, LED_OFF );
            Printf("\r\n BLE Freeze Request \r\n");
        }
        else   //これ以外のイベント(通常発生しない)
        {
            Printf("\r\n BLE Illegal Event!! \r\n");
        }

RX_COMPLETE:
        if(isBleRole() == ROLE_SERVER){ //サーバーの場合ブレーク割り込みを有効にする
            during_scan = false;
            g_external_irq2.p_api->enable(g_external_irq2.p_ctrl);                          //IRQ2イネーブル
        }
//if(isBleConnect() == BLE_CONNECT)
        if( BLE_DISCONNECT == ble_connect_check())       //とりあえずここに入れておく 2020.06.10
        {
            Printf("BLE Disconnect device \r\n");
            if(ble_connect == 1){                   // sakaguchi 2021.01.28 ログ追加
                PutLog(LOG_SYS,"BLE Disconnect");
                ble_connect = 0;
            }
        }else{
            __NOP();//デバッグ用
            Printf(" BLE Connect device \r\n");
            if(ble_connect == 0){                   // sakaguchi 2021.01.28 ログ追加
                PutLog(LOG_SYS,"BLE Connect");
                ble_connect = 1;
            }
        }
    }//スレッドループ for(;;)
}



/**
 * @brief   PSoC UART送信
 * @param pData     送信データへのポインタ
 * @param length    送信データ長
 * @param timeout   TX タイムアウト時間[ms] TX_WAIT_FOREVER使用可
 * @return  エラーコード  SSP_SUCCESS = 0, SSP_ERR_TIMEOUT等
 * @note    2020.Jun.09
 */
static uint32_t send_PSoC(uint8_t *pData, uint32_t length, uint32_t timeout)
{
    ssp_err_t err;
    uint32_t ret = SSP_ERR_TIMEOUT;
    static uint8_t null_break = 0x00;

    err = g_sf_comms4.p_api->lock(g_sf_comms4.p_ctrl, SF_COMMS_LOCK_TX, timeout);    //comm4ロック

    if(isBleRole() == ROLE_CLIENT){ // BLEがクライアント（セントラル）の場合
        if(SSP_SUCCESS == err ){
            //クライアント時、子機へのNULL ブレークはRTR500BWが出す
            if(pData[0] == CODE_SOH){
                err = g_sf_comms4.p_api->write(g_sf_comms4.p_ctrl, (uint8_t *)&null_break, 1, 5);       //NULLブレーク
                tx_thread_sleep (2);
            }
        }
    }

    if(SSP_SUCCESS == err){
        Printf(" Send PSoC: 0x%02X,0x%02X, (%d Byte)\r\n", pData[0], pData[1], length);

        err = g_sf_comms4.p_api->write(g_sf_comms4.p_ctrl, pData, length, timeout );        //データ本体送信
    }
//#if 0
    if(err == SSP_SUCCESS){
//        Printf(" Success\r\n");
    }else{
        Printf(" fail  %ld\r\n", err);
    }
//#endif
    ret = err;
    err = g_sf_comms4.p_api->unlock(g_sf_comms4.p_ctrl, SF_COMMS_LOCK_TX);    //comm4ロック解除

    return (ret);

}

/**
 * @brief   BLE ブレーク信号コールバック
 * @note    P203    IRQ2    立下りエッジ
 * @note    OS管理外
 * @note    新基板用（旧基板はP203未接続）
 * @param p_args
 */
void BLE_break_callback(external_irq_callback_args_t *p_args)
{
    if(p_args->channel == 2)
    {
        g_external_irq2.p_api->disable(g_external_irq2.p_ctrl); //IRQ2割り込み禁止

        if(isBleRole() == ROLE_SERVER){ //サーバーの場合ブレーク割り込みでBLE受信スタート
            during_scan = false;
            tx_event_flags_set( &g_ble_event_flags, FLG_BLE_RX_REQUEST, TX_OR);     //PSoC受信リクエスト イベントフラグ セット
            tx_thread_resume(&ble_thread);     //BLE スレッド起動
            Printf(" BLE Break(IRQ2)\r\n");
        }
    }

}





/**
 * @brief SOHコマンド処理(0x9Eコマンド処理)
 * SOH 0x9Eコマンドのみ
 * @param   sohcmd  SOHコマンド
 * @return  コマンド送受信結果
 * @note    BLEのCAP TRIM、アドバタイズ更新周期が固定値だったのを修正
 */
static int soh_command_exe(char sohcmd)
{
    uint32_t sz;
    uint16_t crc;
    ssp_err_t err;

    memset(BCOM.txbuf.data, 0, sizeof(BCOM.txbuf.data));
    switch (sohcmd)
    {
        case 0x9e:

            BCOM.txbuf.header       = CODE_SOH;
            BCOM.txbuf.command      = 0x9e;
            BCOM.txbuf.subcommand   = CODE_ACK;

            if(BCOM.rxbuf.subcommand == 0x01){

                if(BCOM.rxbuf.length == 18)                 // ＢＬＥデバイスアドレスとチップＩＤが添付された０ｘ９Ｅコマンドの場合
                {
                    psoc.chip_id = 0;
                    memcpy((char *)&psoc.device_address, BCOM.rxbuf.data, 8);
                    psoc.device_address &= 0x0000ffffffffffff;
                    memcpy((char *)&psoc.chip_id, (char *)&BCOM.rxbuf.data[6], 8);
                    memcpy((char *)&psoc.revision, (char *)&BCOM.rxbuf.data[14], 4);
                    psoc.revision[4] = 0x00;
                }

                BCOM.txbuf.length     = 8;
                BCOM.txbuf.data[0]    = fact_config.ble.cap_trim;       //500BW default=0x63
                BCOM.txbuf.data[1]    = fact_config.ble.interval_0x9e_command;             // 9e command interval

                sz = (uint32_t)(BCOM.txbuf.length + 5 + 2);
            }
            else if(BCOM.rxbuf.subcommand == 0x00)
            {
                BCOM.txbuf.length       = 63;
                ble_advertise_packet();     //BLEのアドバタイズパケットデータの更新
                memcpy(&BCOM.txbuf.data, (char *)&BLE_ADV_Packet, 63);  //62Byteコピー

                sz = (uint32_t)(BCOM.txbuf.length + 5 + 2);
            }

            break;

        default:    //BLE経由でメインＣＰＵ宛てにSOHやT2コマンドが来る可能性あり。注意
            Printf(" Invalid SOH command %02X \r\n", BCOM.rxbuf.command);
            sz = 0;
            break;
    }



    crc = crc16_bfr(&BCOM.txbuf.header, (uint32_t)(BCOM.txbuf.length + 5) );
    //エンディアン注意
    BCOM.txbuf.data[BCOM.txbuf.length+0]    = (uint8_t)(crc / 256);
    BCOM.txbuf.data[BCOM.txbuf.length+1]    = (uint8_t)crc;

//    Printf("BLE SOH(%02X) \r\n", BCOM.txbuf.command);

    err = send_PSoC((uint8_t*)&BCOM.txbuf.header, sz, TX_WAIT_FOREVER);

    if(BCOM.rxbuf.subcommand == 0x01){
        DebugLog(LOG_DBG, "PSoC SC:0x01\r\n");
    }
    return(err);
}

/**
 * @brief   ble reset
 * @note    PSoCのFREEZはリセットで復帰させる
 * @note    2020.Jun.24 リセット時はサーバーに戻す様に変更
 *
 */
void ble_reset(void)
{
    g_ioport.p_api->pinWrite(PORT_BLE_RESET, IOPORT_LEVEL_LOW);

    g_ioport.p_api->pinWrite(PORT_BLE_ROLE_CHANGE, ROLE_SERVER);     // SERVER(ペリフェラル)
    g_ioport.p_api->pinWrite(PORT_BLE_FREEZE, IOPORT_LEVEL_HIGH);    // FREEZE 解除

    tx_thread_sleep (2);
    g_ioport.p_api->pinWrite(PORT_BLE_RESET, IOPORT_LEVEL_HIGH);

    Printf("BLE Reset! \r\n");
}

/**
 * @brief   ble connect check
 * @return  1 = 接続中  0= 切断中
 * @note    BleStatusやLEDの処理がなくなったのでisBleConnect()と同じになった
 */
int ble_connect_check(void)
{
    ioport_level_t level;

        g_ioport.p_api-> pinRead(PORT_BLE_CMONI, &level);
/*
        if(level == IOPORT_LEVEL_HIGH){      // Connect
            if(BleStatus == BLE_DISCONNECT){
                BleStatus = BLE_CONNECT;
                //LED_Set( LED_BLECOM, LED_ON );
               Printf("### BLE Connect \r\n");
            }
        }
        else
        {
            if(BleStatus == BLE_CONNECT){
                BleStatus = BLE_DISCONNECT;
                BlePass = BLE_PASS_NG;
                //LED_Set( LED_BLEON, LED_ON );
                Printf("### BLE DisConnect \r\n");
            }
        }
*/
    return (level);
}

/**
 * @brief   ble advertise packet
 *          BLEのアドバタイズパケットデータの更新
 * @note   AdvPacket[]は64Byteだが0x9Eでの応答は63Byteをセットする
 * @note    BLE_ADV_Packet構造体を利用に変更
 */
static void ble_advertise_packet(void){

//  uint32_t sn;
    int         len;
    uint8_t       ucState = 0;
    uint8_t       ucTemp = 0;
    uint16_t    usCrc = 0x0000;      //CRC初期値 0x00000
    char CrcBuf[18+sizeof(my_config.network.NetPass)];     //CRC演算用バッファ CompanyIDから18Byte + パスコード64Byte

    memset((char *)&BLE_ADV_Packet, 0x00, sizeof(BLE_ADV_Packet));      //アドバタイズパケットクリア

    BLE_ADV_Packet.reserve = 0x00;//予備 /bit0=0のときDeepSleep保持、リセットで解除 bit0=1のとき通常動作
  //アドバタイズ パケット
    BLE_ADV_Packet.length = 0x02;
    BLE_ADV_Packet.AD_Type = 0x01;
    BLE_ADV_Packet.AD_Data = 0x06;

    //UUID
    BLE_ADV_Packet.Value.RTR500BW.Uuid.length = 0x05;
    BLE_ADV_Packet.Value.RTR500BW.Uuid.AD_Type = 0x05;
    BLE_ADV_Packet.Value.RTR500BW.Uuid.Data = 0x0000180A;// デバイスインフォメーション 32bit UUID

    //Manufacturer Specificデータ
    BLE_ADV_Packet.Value.RTR500BW.Cid.length = 0x15;
    BLE_ADV_Packet.Value.RTR500BW.Cid.AD_Type = 0xFF;
    BLE_ADV_Packet.Value.RTR500BW.Cid.CompanyID = COMPANY_ID;    //Company ID 0x0392
    BLE_ADV_Packet.Value.RTR500BW.Cid.SerialNo = fact_config.SerialNumber;         //機器シリアル番号

    BLE_ADV_Packet.Value.RTR500BW.Cid.code = 0x00;               //制御コード 0x00固定

    // sakaguchi UT-0035 ↓

    ucTemp = 0;
    len = (int)strlen(my_config.network.NetPass);
    if(len>sizeof(my_config.network.NetPass))  len = sizeof(my_config.network.NetPass);     // 2020.12.04  64byteまで入っていると、終端が不正になる

    if(0 < len){
        ucTemp = 1;
    }
    ucState = ucTemp;                                   // bit7:BLEセキュリティ(0:無効,1:有効)

    ucTemp = 0;                                         // bit6:警報有無
    if(0 < (uint8_t)Warning_Check()){
        ucTemp = 1;
    }
    ucState = (uint8_t)((ucState << 1) + ucTemp);
    ucState = (uint8_t)(ucState << 1);                    // bit5:予備

    ucTemp = 0;                                         // bit4:WLAN通信エラー(0:なし,1:あり)
    if(LAN_DISCONNECT == lan_connect_check(WIFI)){
        ucTemp = 1;
    }
    ucState = (uint8_t)((ucState << 1) + ucTemp);

    ucTemp = 0;                                         // bit3:設定エラー(0:なし,1:あり)
    if(false == set_state_check()){
        ucTemp = 1;
    }
    ucState = (uint8_t)((ucState << 1) + ucTemp);

    ucTemp = 0;                                         // bit2:チャンネルBUSY(0:なし,1:あり)
    if(CH_BUSY == RfCh_Status){
        ucTemp = 1;
    }
    ucState = (uint8_t)((ucState << 1) + ucTemp);

    ucTemp = 0;                                         // bit1:LAN通信エラー(0:なし,1:あり)
    if(LAN_DISCONNECT == lan_connect_check(ETHERNET)){
        ucTemp = 1;
    }
    ucState = (uint8_t)((ucState << 1) + ucTemp);

    ucTemp = 0;                                         // bit0:自律動作(0:OFF,1:ON)
    if(STATE_OK == UnitState){
        ucTemp = 1;
    }
    ucState = (uint8_t)((ucState << 1) + ucTemp);

    BLE_ADV_Packet.Value.RTR500BW.Cid.State  = ucState;                         //状態コード
    BLE_ADV_Packet.Value.RTR500BW.Cid.SysCnt= (uint8_t)my_config.device.SysCnt; //設定CNT（親機設定）
    BLE_ADV_Packet.Value.RTR500BW.Cid.ScanCnt = 0x00;                           // SCANCNT（対応せず）
    BLE_ADV_Packet.Value.RTR500BW.Cid.WarnCnt = auto_control.warcnt;            // 警報CNT
    BLE_ADV_Packet.Value.RTR500BW.Cid.RegCnt = (uint8_t)my_config.device.RegCnt;// 設定CNT（登録情報）;         // 設定CNT（登録情報）
    BLE_ADV_Packet.Value.RTR500BW.Cid.reserve = 0x00;                           // 空き
    BLE_ADV_Packet.Value.RTR500BW.Cid.BattLevel = 0x10;                         // 電池レベル BWは0x10固定
    BLE_ADV_Packet.Value.RTR500BW.Cid.data[0] =  0x0000;                        //測定値   予備 uint16_t
    BLE_ADV_Packet.Value.RTR500BW.Cid.data[1] =  0x0000;                        //測定値   予備 uint16_t

    memcpy(CrcBuf, (char *)&BLE_ADV_Packet.Value.RTR5_TR7.Cid.CompanyID, 18 );    //CompanyIDから18Byte(CRCの手前まで）
    memcpy(&CrcBuf[18], my_config.network.NetPass, sizeof(my_config.network.NetPass));                       //パスコード64Byte

    usCrc = crc16_bfr(CrcBuf, sizeof(CrcBuf));      //CRC演算

    BLE_ADV_Packet.Value.RTR500BW.Cid.PassCRC_H = (uint8_t)(usCrc >> 8);      //パスコードCRC  2Byte ビッグエンディアン
    BLE_ADV_Packet.Value.RTR500BW.Cid.PassCRC_L = (uint8_t)usCrc;

// sakaguchi UT-0035 ↑


    //デバイス名 0x08
    // UFT8の文字コード境目の対応
    len = str_utf8_len(my_config.device.Name, 26);
    if(len > 26){
        len = 26;
    }

    BLE_ADV_Packet.DevName.length = 0x1B;       //1+26
    BLE_ADV_Packet.DevName.AD_Type = 0x08;
    memcpy(BLE_ADV_Packet.DevName.Data, my_config.device.Name , (uint32_t)len);        //デバイス名文字列 26バイト固定長 UTF-8

    /*
    BLE_ADV_Packet.DevName.dummy[0] = 0x00;
    BLE_ADV_Packet.DevName.dummy[1] = 0x00;
    BLE_ADV_Packet.DevName.dummy[2] = 0x00;
    */

}

/**
 * @brief BLE BLEクライアント、サーバーのチェック
 * @retval  0   Client
 * @retval  1   Server
 */
int isBleRole(void){
    ioport_level_t level;
    g_ioport.p_api->pinRead(PORT_BLE_ROLE_CHANGE, &level);
    return (level);
}

/**
 * @brief BLE 接続状態ののチェック
 * @retval  0   切断
 * @retval  1   接続
 */
int isBleConnect(void){
    ioport_level_t level;
    g_ioport.p_api-> pinRead(PORT_BLE_CMONI, &level);
    return (level);
}




/*** end of file ***/

