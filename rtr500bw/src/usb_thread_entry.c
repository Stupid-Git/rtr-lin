/**
 *  @brief USB処理メイン スレッド（受信スレッド）
 *          USBのイニシャライズ、受信、エラー処理等主要な処理を行う。
 *          送信処理のみ別スレッド
 *  @file   usb_thread_entry.c
 *
 *
 *  @note   スタックサイズ              8192 Byte → 2048Byte
 *  @note   USBX プールメモリ     18432 Byte(USBX CDC-ACM Default)
 *  @note   USB受信バッファ1   8192 Byte  usb_buffer → usb_rx_bufferに変更
 *  @note   USB受信バッファ２      12480 Byte  def_com_u UCOM  → 廃止
 *  @note   USB送信バッファ1   8192 Byte  StsArea   T2コマンド時  LAN/WiFiと共通
 *  @note   USB送信バッファ2   8192 Byte 新設 usb_tx_buffer → 2048Byte→ 4096+64Byte
 *
 *  @note   2020.Jun.15 USB送信部をusb_tx_threadに分けた
 *
 *  @note   StsAreaは各スレッドから排他制御無しで書き込まれるため、USB,BLEからは使用しない→USB専用の送受信バッファに分けた
 *  @todo   UCOM廃止  → 廃止済み
 *  @todo   StsArea共有をやめる
 *  @todo   USBスレッドを送受信で分ける → 分けた
 *  @attention  2020.Jul.16 Limitation, Issue ID:15801対策でux_dcd_synergy_transfer_abort.cのux_dcd_synergy_transfer_abort（）関数修正済み
 *  @attention  ホスト側はUSBの応答を受信してもすぐにシリアルポートをクローズしてはいけない（数秒待つ事）

 */
#define _USB_THREAD_ENTRY_C_


#include "General.h"

#include "common_data.h"
#include "system_thread.h"

#include "usb_thread_entry.h"
#include "led_thread_entry.h"
#include "cmd_thread_entry.h"
#include "ble_thread_entry.h"
#include "event_thread_entry.h"
#include "usb_thread_entry.h"
#include "usb_tx_thread.h"
extern TX_THREAD usb_tx_thread;


/** CDC-ACM reception data buffer. USB受信バッファ 1 */
static uint8_t usb_rx_buffer[UX_BUFFER_SIZE+64];          //8192+64Byte
static uint8_t usb_tx_buffer[UX_TX_BUFFER_SIZE*2+64];       //とりあえず、受信と同容量 → 2048Byte→ 4096+64Byte

/** A pointer to store CDC-ACM device instance. */
UX_SLAVE_CLASS_CDC_ACM   * g_cdc;


//プロトタイプ




/**
 * *********************************************************************************************************************
 * USB String Descriptor
 *  Manufacturer string descriptor  : Index 1
 *  Product string descriptor       : Index 2
 *  Serial Number string descriptor : Index 3
 *********************************************************************************************************************
*/
volatile const UCHAR g_string_framework[] =
{ (uint8_t) (0x0409), /* 0 Supported Language Code */
  (uint8_t) (0x0409 >> 8), /* 1 Supported Language Code */
  0x01, /* 2 Index */
  //18, // 3 bLength 
  //'R', 'e', 'n', 'e', 's', 'a', 's', 'E', 'l', 'e', 'c', 't', 'r', 'o', 'n', 'i', 'c', 's',
  12, // 3 bLength 
  'T', '&', 'D', ' ', 'C', 'D', 'C', ' ', 'P', 'o', 'r', 't',   

  (uint8_t) (0x0409), /* 0 Supported Language Code */
  (uint8_t) (0x0409 >> 8), /* 1 Supported Language Code */
  0x02, /* 2 Index */
  //10, // 3 bLength
  //'S', 'y', 'n', 'e', 'r', 'g', 'y', 'C', 'D', 'C',
   7, // 3 bLength 
  'T', '&', 'D', ' ', 'C', 'D', 'C',   

  (uint8_t) (0x0409), /* 0 Supported Language Code */
  (uint8_t) (0x0409 >> 8), /* 1 Supported Language Code */
  0x03, /* 2 Index */
  4, /* 3 bLength */
  '1', '2', '0', '0' };

volatile const int g_string_framework_size = sizeof(g_string_framework);

const UCHAR g_language_id_framework[] =
{ (uint8_t) (0x0409), /* Supported Language Code */
  (uint8_t) (0x0409 >> 8) /* US English as the default */
};


/**
 *　USBX CDC-ACM Instance Activate User Callback Function
 * @param cdc_instance
 * @note    USBX CDCのミドルウェアを使用すると自動生成される
 * @note    FLG_USB_UPイベントフラグは待っている個所が居ない
 */
void ux_cdc_device0_instance_activate (VOID * cdc_instance)
{
    ULONG actual_events = 0;
    //Printf("### USB Connect\r\n");
    /* Save the CDC instance.  */
    g_cdc = (UX_SLAVE_CLASS_CDC_ACM *) cdc_instance;

    tx_event_flags_get (&event_flags_usb, 0xffffffff,  TX_OR_CLEAR, &actual_events, TX_NO_WAIT);       //USBイベントフラグクリア
    tx_event_flags_get (&event_flags_cycle, FLG_EVENT_DISCONNECT_USB , TX_OR_CLEAR, &actual_events, TX_NO_WAIT);       // USB Connect clear // sakaguchi 2021.04.07
    tx_event_flags_set (&event_flags_cycle, FLG_EVENT_CONNECT_USB, TX_OR);//FLG_USB_UPイベントを待っている個所がない→ イベントフラグ待ちに追加
}

/**
 * USBX CDC-ACM Instance Deactivate User Callback Function
 * @param cdc_instance
 * @note    USBX CDCのミドルウェアを使用すると自動生成される
 * @note    FLG_USB_DOWNイベントフラグは event_thread_entry()で待っている
 */
void ux_cdc_device0_instance_deactivate (VOID * cdc_instance)
{
    SSP_PARAMETER_NOT_USED(cdc_instance);
    ULONG actual_events = 0;

    //Printf("### USB Disconnect\r\n");

    g_cdc = UX_NULL;

    tx_event_flags_get (&event_flags_usb, 0xffffffff,  TX_OR_CLEAR, &actual_events, TX_NO_WAIT);       //USBイベントフラグクリア
    tx_event_flags_get (&event_flags_cycle, FLG_EVENT_CONNECT_USB , TX_OR_CLEAR, &actual_events, TX_NO_WAIT);       // USB Connect clear // sakaguchi 2021.04.07
    tx_event_flags_set (&event_flags_cycle, FLG_EVENT_DISCONNECT_USB, TX_OR);// FLG_USB_DOWNイベントフラグセット @note event_thread_entry()で待っている
}



/**
 *  @brief  USB Rx Thread entry function
 *
 * @note    UCOM廃止
 * @note    def_comformatを使わず、comSOH_tにキャストして使う
 * @note    def_comformatは先頭にダミーバイトがあるのでずらさないといけない
 * @note    2020.Jul.16 キューメッセージの送受を専用の構造体に変更
 */
void usb_thread_entry(void)
{
    uint16_t error = 0;
    uint32_t status = 0;
    uint32_t actual_events = 0;
    uint32_t actual_length = 0;
    uint32_t i;                     //
    static usb_psoc_t tensou;       ///< キューでBLE_Threadに送るメッセージ
    static CmdQueue_t cmd_msg;      ///< キューでcmd_threadに送るメッセージ
    static usb_tx_t usbTxQueue;            ///< キューでusb_tx_threadに送るメッセージ

   uint32_t rx_wait;                ///< PSoC転送時の時のタイムアウト

   comSOH_t *pComRx = (comSOH_t *)&usb_rx_buffer;      //pComRx はSOH,STXフレーム型のポインタ
   comSOH_t *pComTx = (comSOH_t *)&usb_tx_buffer;

   //スレッド初期化
   tx_event_flags_get (&event_flags_usb, 0xffffffff,  TX_OR_CLEAR, &actual_events, TX_NO_WAIT);        //USBイベントフラグをすべてクリア

    Printf("USB Thread Start\n");
    ux_device_class_cdc_acm_init0();        //デバイス初期化


    //スレッド ループ
    for(;;){
USB_CHECK:
//        tx_event_flags_get (&event_flags_usb, 0xffffffff,  TX_OR_CLEAR, &actual_events, TX_NO_WAIT);        //USBイベントフラグをすべてクリア

        if(g_cdc != UX_NULL){      //切断されている時点でg_cdc == NULL
            LED_Set(LED_DIAG, LED_OFF);
            LED_Set( LED_WARNING, LED_OFF );
            EX_WRAN_OFF;
        }else{
            tx_event_flags_get (&event_flags_usb, 0xffffffff,  TX_OR_CLEAR, &actual_events, TX_NO_WAIT);        //USBイベントフラグをすべてクリア
        }


        //        tx_event_flags_set( &event_flags_usb, FLG_USB_TX_COMPLETE, TX_OR);
        tx_event_flags_set( &event_flags_usb, FLG_USB_READY, TX_OR);
        //USB接続中ループ
        while(g_cdc != UX_NULL){

           // 先頭のSOH, STX ,T を受信
USB_RECEIVE:
            //受信待ちループ
            for(;;){

                while(TX_EVENT_FLAG != usb_tx_thread.tx_thread_state){
                    //ここに来た場合、RXとTXスレッドでセマフォの待ちが競合している場合がある（USB TXスレッドは完了を待っているので実際の送信は終わっている）
                    //ホスト側がシリアルポートをクローズしている場合（応答を受信してもすぐクローズしない）
                    //前回の受信時に全データを取得していない場合、
                    //USB送信前に受信データが来ている場合など(NULLブレーク等)。
                    //受信データがある場合ux_device_class_cdc_acm_read()関数を呼ぶと、USB Txのセマフォ待ちが完了しux_device_class_cdc_acm_write()関数が完了する
                    //シリアルポートクローズ時はシリアルポートオープンでUSB Txのセマフォ待ちが完了しux_device_class_cdc_acm_write()関数が完了する
                    Printf(" ##Wait USB TX thread Ready\r\n");
                    tx_thread_sleep (10);
                }
                // Read from the CDC class.
                status = ux_device_class_cdc_acm_read (g_cdc, usb_rx_buffer, UX_BUFFER_SIZE, &actual_length);

                Printf("\r\n>> USB Recive Start %02X,%02X, (%d Byte)\r\n", usb_rx_buffer[0], usb_rx_buffer[1], actual_length);

                if(status != UX_SUCCESS)
                {
                    //UX_CONFIGURATION_HANDLE_UNKNOWN , UX_TRANSFER_NO_ANSWER 切断された？
                    error++;
                    Printf("usb recv err = %d/%d\n", error,status);
                    goto USB_CHECK;        // USB再受信
                }

                //NULLブレークの場合があるのでヘッダを探す
                for (i = 0; i < actual_length; i++){
                    if((usb_rx_buffer[i] == CODE_SOH) || (usb_rx_buffer[i] == CODE_STX) || (usb_rx_buffer[i] == 'T')){
                        if((actual_length - i) < ((usb_rx_buffer[i] == 'T') ? 6 :7 )){
                            continue; //6Byte未満 USB再受信
                        }else{
                            goto START_RX; //ヘッダが見つかった 受信完了 ループを抜ける
                        }
                    }
                }
                tx_thread_sleep (1);
            }//for 受信待ちループ

START_RX:
            //SOH,STX,T2から始まるデータを受信した
            switch(usb_rx_buffer[i])   //i分オフセットしている場合あり
            {
                case CODE_SOH:
                case CODE_STX:
                    pComRx = (comSOH_t *)&usb_rx_buffer[i];   //ポインタを再セット （以降はポインタでアクセスする）
                    if((actual_length - i) < (uint32_t)(pComRx->length + 7))
                    {
                         Printf("=======>>>  USB receve error len %d %d\r\n", actual_length, pComRx->length );
                         goto USB_RECEIVE;        // USB再受信
                     }
                    break;
                case 'T':
                    pComRx = (comSOH_t *)((char *)(&usb_rx_buffer[i] - 1));   //   //ポインタを再セット SOHコマンドと length位置を合わせる
                    pComRx->header = pComRx->command;
                    if((actual_length - i) < (uint32_t)(pComRx->length + 6))
                    {
                        Printf("=======>>>  USB receve error len %d %d\r\n", actual_length, pComRx->length );
                        goto USB_RECEIVE;        // USB再受信
                    }
                    break;
                default:
                    Printf("USB receve Other command\r\n");
                    goto USB_RECEIVE;        //5Byte未満 USB再受信
            }//switch


            // コマンド解析・処理
            // SOHは転送、STXはPSoC制御コマンド
            if((pComRx->header == CODE_SOH) || (pComRx->header == CODE_STX))  //SOH,STXコマンド
            {
                if(isBleRole() == ROLE_CLIENT)  // BLEがクライアント（セントラル）の場合
                {
                    //PSoCへコマンド転送
                    //転送用メッセージキューにバッファのポインタをセットする
                    tensou.pUSB2PSoC = (comSOH_t *)&pComRx->header;     //USBからPSoCへ転送する受信データフレームの先頭ポインタ
                    tensou.pPSoC2USB = (comSOH_t *)&pComTx->header;     //PSoCからの応答をUSBへ転送する応答フレームの先頭ポインタ

                    tx_queue_send(&g_ble_usb_queue, &tensou, TX_WAIT_FOREVER);
                    tx_event_flags_set( &g_ble_event_flags, FLG_BLE_TX_REQUEST, TX_OR);     //PSoC送信リクエスト(BLE RX スレッド起動)
                    status = tx_thread_resume(&ble_thread);                                 //BLE スレッド起動

                    //クライアント時の子機への接続および子機へのSOHコマンドはタイムアウトを長くする
                    if((isBleConnect() == BLE_CONNECT) || (tensou.pUSB2PSoC->command == STX_CONNECTION) || (tensou.pUSB2PSoC->header == CODE_SOH))
                    {
                        rx_wait = 1010; //10.1秒
                    }else{
                        rx_wait = 210;   //2.1秒
                    }

                    //イベントフラグ発生まで待ち
                    status = tx_event_flags_get( &event_flags_usb, FLG_USB_PSOC_COMPLETE|FLG_USB_PSOC_ERROR, TX_OR_CLEAR, &actual_events, rx_wait );   //PSoCから受信完了 待ち

                    //USBで転送
                    if(TX_NO_EVENTS != status){
                        if(during_scan == false){
                            if(actual_events & FLG_USB_PSOC_COMPLETE){
                                usbTxQueue.pTxBuf = (char *)tensou.pPSoC2USB;        //送信データへのポインタ
                                usbTxQueue.length = (uint32_t)(tensou.pPSoC2USB->length + 7);   //送信データ長
                            }
                            else if(actual_events & FLG_USB_PSOC_ERROR){
                                //読み飛ばす
                                Printf(" PSoC Error\r\n");
                                continue;
                            }
                        }

                    }else{
                        __NOP();   //デバッグ用
                        Printf(" PSoC Timeout\r\n");
                        continue;
                    }

                }
                else    //ＢＬＥサーバー SOH,STX（基本的に来ない）
                {
                    //読み飛ばす
                    Printf("Error! Receive Client command\r\n");
                    continue;
                }

            }
            else if(pComRx->header == 'T')   //T2コマンド
            {
                //コマンドスレッド実行中は待つ 2020.Jul.15
                while((TX_SUSPENDED != cmd_thread.tx_thread_state) /*&& (TX_EVENT_FLAG != cmd_thread.tx_thread_state)*/){
                    Printf("#Wait Command Thread complete\r\n");
                    tx_thread_sleep (1);
                }
                cmd_msg.CmdRoute = CMD_USB;                    //コマンド キュー  コマンド実行要求元
                cmd_msg.pT2Command = (char *)&pComRx->command;    //コマンド処理する受信データフレームの先頭ポインタ
                cmd_msg.pT2Status = (char *)&pComTx->header;     //2020.Jun.15まだ未対応
                cmd_msg.pStatusSize = (int32_t *)&usbTxQueue.length;

                tx_queue_send(&cmd_queue, &cmd_msg, TX_WAIT_FOREVER);
                tx_event_flags_set (&g_command_event_flags, FLG_CMD_EXEC,TX_OR);
                status = tx_thread_resume(&cmd_thread);     //コマンドスレッド起動

                //コマンド処理完了待ち
                status = tx_event_flags_get (&g_command_event_flags, FLG_CMD_END ,TX_OR_CLEAR, &actual_events,TX_WAIT_FOREVER);
                /*
                usb_tx_threadでZLP処理している
                if((CmdStatusSize % 64 ) == 0){      //  64の倍数のときパケットが送信されない現象があり！
                    CmdStatusSize++;            // こうしないとパケットが送信されない
                }
*/
                int wai_sem_count = 0;
                while(TX_SEMAPHORE_SUSP == usb_tx_thread.tx_thread_state){
                    Printf("Wait Usb Tx Semaphore\r\n");
                    tx_thread_sleep (10);       //100ms待ってみる
                    wai_sem_count++;
                    if(wai_sem_count > 20) //2秒ロックされていたら
                    {
                        //送信中断処理(正常に動作するかは未確認)
                        Printf(">>>>>>>>>>UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_STOP\r\n");
                        status = ux_device_class_cdc_acm_ioctl(g_cdc, UX_SLAVE_CLASS_CDC_ACM_IOCTL_TRANSMISSION_STOP, UX_NULL); //通信停止
                         if(status == UX_ERROR){
                             __NOP();//送信中ではない
                         }
                        //パイプ中止(正常に動作するかは未確認)
                         Printf(">>>>>>>>>>UX_SLAVE_CLASS_CDC_ACM_IOCTL_ABORT_PIPE\r\n");
                       uint32_t ioparam = UX_SLAVE_CLASS_CDC_ACM_ENDPOINT_XMIT;
                       status = ux_device_class_cdc_acm_ioctl(g_cdc, UX_SLAVE_CLASS_CDC_ACM_IOCTL_ABORT_PIPE,&ioparam);
                       if(status == UX_ENDPOINT_HANDLE_UNKNOWN){
                           __NOP();
                       }

                       //ux_dcd_synergy_transfer_abort(ux_slave_dcd_controller_hardware);
                       tx_thread_suspend(&usb_tx_thread);       //スレッドサスペンドでセマフォ等がクリーンナップされるはず
                    }
                }
                usbTxQueue.pTxBuf = cmd_msg.pT2Status;        //送信データへのポインタ
  //              pTx.length = (uint32_t)CmdStatusSize;   //送信データ長

            }
            else    //SCPIコマンド
            {
                //読み飛ばす
                Printf("Receive SCPI, skip\r\n");
                __NOP();
                continue;
            }
          status = tx_event_flags_get (&event_flags_usb, FLG_USB_READY|FLG_USB_TX_COMPLETE|FLG_USB_TX_ERROR ,TX_OR_CLEAR, &actual_events, TX_NO_WAIT);	//フラグクリア（不要）
            //T2/STX/SOH 共通送信処理
            tx_queue_send(&g_usb_tx_queue, &usbTxQueue, TX_WAIT_FOREVER);                 //メッセージキューセット
            tx_event_flags_set( &event_flags_usb, FLG_USB_TX_REQUEST, TX_OR);     //USB送信リクエスト イベントシグナル
            status = tx_thread_resume(&usb_tx_thread);                            //USB TX スレッド起動

            //USB送信完了待ち 2020.Jul.09
            status = tx_event_flags_get (&event_flags_usb, FLG_USB_TX_COMPLETE|FLG_USB_TX_ERROR ,TX_OR_CLEAR, &actual_events, 500);
            if(status == TX_SUCCESS){
                if(actual_events & FLG_USB_TX_ERROR){   //USB送信エラー
                    Printf(">>>USB Tx Error\r\n");
                    break;  //USB接続チェックからやり直し
                }
                else if(FLG_USB_TX_COMPLETE & actual_events){
                    Printf(">>>USB Tx Success\r\n");
                }
                else{
                    __NOP();
                }
            }
            else
            {
                //送信タイムアウトだが、USBホスト側の問題の可能性が高い
                //ここに来た場合、RXとTXスレッドでセマフォの待ちが競合している場合がある（USB TXスレッドは完了を待っているので実際の送信は終わっている）
                //ホスト側がシリアルポートをクローズしている場合（応答を受信してもすぐクローズしない）　<---- 一番可能性が高い 動作している方のUSBスレッドでセマフォ待ちになる
                //前回の受信時に全データを取得していない場合、
                //USB送信前に受信データが来ている場合など(NULLブレーク等)。
                //受信データがある場合ux_device_class_cdc_acm_read()関数を呼ぶと、USB Txのセマフォ待ちが完了しux_device_class_cdc_acm_write()関数が完了する
                //シリアルポートクローズ時はシリアルポートオープンでUSB Txのセマフォ待ちが完了しux_device_class_cdc_acm_write()関数が完了する

                Printf(">>>>USB Tx Error. Tx Timeout.\r\n");
                break;  //USB接続チェックからやり直し
            }

            Reboot_check();     //REBOOT要求のチェック 2020.Jun.10 各タスクに入っていたチェック部を関数にして分離(とりあえずcmd_thread_entry.c)

        } // end of while 1  再受信

        tx_thread_sleep(10);   //100ms USB未接続なので接続待ち
    } // end of for スレッドループ
}






