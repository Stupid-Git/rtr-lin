/**
 * @file    usb_thread_entry.h
 *
 * @date    2020/06/10
 * @author  t.saito
 * @note    2020.06.10  他ソースから分離
 */
//#include "usb_thread.h"
extern TX_THREAD usb_thread;
extern TX_THREAD usb_tx_thread;

#ifndef _USB_THREAD_ENTRY_H_
#define _USB_THREAD_ENTRY_H_

#ifdef EDF
#undef EDF
#endif

#ifdef _USB_THREAD_ENTRY_C_
#define EDF
#else
#define EDF extern
#endif


// USB event_flags_usb
//#define FLG_USB_UP              BIT_0         //-> event_threadに移動＆名称変更   待っている個所が居ない
//#define FLG_USB_DOWN            BIT_1         // -> event_threadに移動＆名称変更
#define FLG_USB_READY           BIT_0
#define FLG_USB_PSOC_COMPLETE   BIT_1          ///<PSoC通信完了 BLEスレッドからの通知
#define FLG_USB_PSOC_ERROR      BIT_2          ///<PSoC通信エラー BLEスレッドからの通知
#define FLG_USB_TX_REQUEST      BIT_3           ///<USB 送信要求
#define FLG_USB_TX_COMPLETE     BIT_4           ///<USB 送信完了    2020.Jul.09
#define FLG_USB_TX_ERROR        BIT_5           ///< USB送信エラー 送信中の切断、PC側のドライバ死亡等

#define UX_BUFFER_SIZE 8192  //12480
#define UX_TX_BUFFER_SIZE   4096
#define CHECK_NONE 0
#define CHECK_CRC  1
#define CHECK_SUM  2

///USB送信用メッセージキューの型定義
typedef struct{
    char *pTxBuf;        ///<送信データへのポインタ
    uint32_t length;        ///<送信サイズ
}usb_tx_t;

// USB UsbStaus
#define USB_CONNECT         1
#define USB_DISCONNECT      0

//EDF int16_t UsbState;           ///<  0: Down   1: UP →　廃止 isUSBState()マクロへ
///USBの接続状態をチェックする
#define isUSBState()    ((g_cdc == UX_NULL) ? USB_DISCONNECT : USB_CONNECT)


extern UX_SLAVE_CLASS_CDC_ACM   * g_cdc;

#endif
