/**
 *  @brief USB 送信スレッド
 *  @file   usb_tx_thread_entry.c
 *  @date   2020.Jun.15
 *
 *  @note   スタックサイズ              8192 Byte → 2048Byte
 *  @note   USBX プールメモリ     18432 Byte(USBX CDC-ACM Default)
 *  @note   USB受信バッファ1   8192 Byte  usb_buffer → usb_rx_bufferに変更
 *  @note   USB受信バッファ２      12480 Byte  def_com_u UCOM  → 廃止
 *  @note   USB送信バッファ1   8192 Byte  StsArea   T2コマンド時  LAN/WiFiと共通
 *  @note   USB送信バッファ2   8192 Byte 新設 usb_tx_buffer → 2048Byte → 4096+32Byte
 *
 *  @note   2020.Jun.15 USB送信部をusb_tx_threadに分けた
 *
 *  @attention   USB送信時、PC側が受信できない場合、エラーにならず送信可能になるまで待ちになってしまう(RTS/CTS設定しなくても)
 *          その状態でUSBX プールメモリがあふれるとosエラーになってしまう
 *  @attention  2020.Jul.16 Limitation, Issue ID:15801対策でux_dcd_synergy_transfer_abort.cのux_dcd_synergy_transfer_abort（）関数修正済み
*/


#define _USB_TX_THREAD_ENTRY_C_



#include "General.h"
#include "usb_tx_thread.h"
#include "usb_thread_entry.h"
#include "ble_thread_entry.h"

//extern UX_SLAVE_CLASS_CDC_ACM   * g_cdc;

/**
 *  USB TX Thread entry function
 *
 * FLG_USB_TX_REQUESTイベントフラグで起床、
 * 終了時はFLG_USB_TX_COMPLETEもしくはFLG_USB_TX_ERRORイベントイベントフラグがセットされる
 *
 *
 */
void usb_tx_thread_entry(void)
{
    uint32_t status = 0;
    uint32_t actual_events = 0;
    uint32_t actual_length = 0;
    static usb_tx_t usbTxQueue;


    while (1)
    {
        if( g_cdc != UX_NULL) //USB CDC未接続
        {
/*
            //実験用 USBプールメモリにデータがあるか確認できるか？　USBは接続されているが、シリアルポートがクローズの状態を検出できるか
             * //USBは接続されているが、シリアルポートがクローズの状態(ホストがDTR有効にしていないと認識できない)
            if(UX_TRUE == g_cdc->ux_slave_class_cdc_acm_data_dtr_state){
                __NOP();//デバッグ用
            }else{
                __NOP();//デバッグ用
            }
            if(UX_TRUE == g_cdc->ux_slave_class_cdc_acm_data_rts_state){
                __NOP();//デバッグ用
            }else{
                __NOP();//デバッグ用
            }
            */

            //イベントフラグ発生まで待ち
            status = tx_event_flags_get( &event_flags_usb, FLG_USB_TX_REQUEST, TX_OR_CLEAR, &actual_events, TX_WAIT_FOREVER );   //USB送信要求 待ち
            if(actual_events & FLG_USB_TX_REQUEST){

                status = tx_queue_receive(&g_usb_tx_queue, &usbTxQueue, TX_WAIT_FOREVER);      //メッセージキュー受け取り
                Printf(">> Start Send USB\r\n");

                while(TX_READY != usb_tx_thread.tx_thread_state){
                    Printf(" ####Wait USB TX thread Ready\r\n");
                    tx_thread_sleep (1);
                }
                status = ux_device_class_cdc_acm_write(g_cdc, (uint8_t *)usbTxQueue.pTxBuf, usbTxQueue.length, &actual_length);
				//ux_device_class_cdc_acm_write()を呼んだ後にシリアルポートがクローズされるとセマフォ待ちのまま関数から帰ってこない（シリアルポート再オープンまで）
                if(UX_SUCCESS == status){   //送信成功
                    Printf(">> Send USB: 0x%02X,0x%02X, (%d Byte)\r\n",usbTxQueue.pTxBuf[0], usbTxQueue.pTxBuf[1], actual_length);
                    if(usbTxQueue.length % 64 == 0){
                        status = ux_device_class_cdc_acm_write(g_cdc, (uint8_t *)usbTxQueue.pTxBuf, 0, &actual_length);    //ZLP
                        Printf(">>> Send ZLP, (%d Byte) \r\n", actual_length);
                        if(UX_SUCCESS != status){   //送信失敗はUSB切断されている UX_CONFIGURATION_HANDLE_UNKNOWN , UX_TRANSFER_NO_ANSWER
                            Printf(">>> Send Fail. Disconnect USB.\r\n");
                            tx_event_flags_set( &event_flags_usb, FLG_USB_TX_ERROR, TX_OR);   // USB送信エラー 送信中の切断、PC側のドライバ死亡等
                        }
                    }

                    if(isBleRole() == ROLE_CLIENT){ // BLEがクライアント（セントラル）の場合
                        if(usbTxQueue.pTxBuf[1] == STX_START_SCAN){  // スキャン開始コマンド 0x20        //PSoCから応答が連続で来る
                            during_scan = true;       //コマンド応答後セットする
                        }
                        else if(usbTxQueue.pTxBuf[1] == STX_STOP_SCAN){
                            during_scan = false;      //コマンド応答後セットする
                        }
                        //BLE スキャン中
                        if(during_scan == true){
                            if((usbTxQueue.pTxBuf[1] == STX_START_SCAN) || (usbTxQueue.pTxBuf[1] == STX_SCAN_DATA)){
                                tx_event_flags_set( &g_ble_event_flags, FLG_BLE_RX_REQUEST, TX_OR);     //引き続きPSoC受信リクエスト
                            }
                        }
                    }
                    //ux_device_class_cdc_acm_write()がUX_SUCCESSでも実転送は完了していない
                    while(TX_READY != usb_tx_thread.tx_thread_state){
                        tx_thread_sleep (1);
                    }
                    tx_event_flags_set( &event_flags_usb, FLG_USB_TX_COMPLETE, TX_OR);      //送信完了
                }
                else   //送信失敗はUSB切断されている UX_CONFIGURATION_HANDLE_UNKNOWN , UX_TRANSFER_NO_ANSWER
                {
                    Printf(">>> Send Fail. Disconnect USB.\r\n");
                    tx_event_flags_set( &event_flags_usb, FLG_USB_TX_ERROR, TX_OR);   // USB送信エラー 送信中の切断、PC側のドライバ死亡等
                }
            }

 //           tx_thread_suspend(&usb_tx_thread);         //自分自身でサスペンドする
        }
        else{
             tx_thread_sleep(30); /* Sleep until USB peripheral initialisation */
        }
    }
}
