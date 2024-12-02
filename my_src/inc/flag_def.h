/*
 * flag_def.h
 *
 *  Created on: Dec 2, 2024
 *      Author: karel
 */

#ifndef FLAG_DEF_H_
#define FLAG_DEF_H_

#define BIT_0    0x00000001
#define BIT_1    0x00000002
#define BIT_2    0x00000004
#define BIT_3    0x00000008
#define BIT_4    0x00000010
#define BIT_5    0x00000020
#define BIT_6    0x00000040
#define BIT_7    0x00000080
#define BIT_8    0x00000100
#define BIT_9    0x00000200
#define BIT_10   0x00000400
#define BIT_11   0x00000800
#define BIT_12   0x00001000
#define BIT_13   0x00002000
#define BIT_14   0x00004000
#define BIT_15   0x00008000
#define BIT_16   0x00010000
#define BIT_17   0x00020000
#define BIT_18   0x00040000
#define BIT_19   0x00080000
#define BIT_20   0x00100000
#define BIT_21   0x00200000
#define BIT_22   0x00400000
#define BIT_23   0x00800000
#define BIT_24   0x01000000
#define BIT_25   0x02000000
#define BIT_26   0x04000000
#define BIT_27   0x08000000
#define BIT_28   0x10000000
#define BIT_29   0x20000000
#define BIT_30   0x40000000
#define BIT_31   0x80000000


#define FLG_CLEAR    0x00000000
#define FLG_ALL  0xffffffff

#define FLG_RTC_INT         BIT_0


#define FLG_DHCP_SUCCESS    BIT_4
#define FLG_DHCP_ERROR      BIT_5
#define FLG_LINK_ERROR      BIT_6

//#define FLG_SNTP_SUCCESS    BIT_8     //2019.Dec.24 二重定義コメントアウト
//#define FLG_SNTP_ERROR      BIT_9     //2019.Dec.24 二重定義コメントアウト


#define FLG_DNS_SUCCESS     BIT_0
#define FLG_DNS_ERROR       BIT_1




#define FLG_NET_INIT_SUCCESS    BIT_0
#define FLG_NET_INIT_ERROR      BIT_1

#define FLG_NET_HTTP_BUSY       BIT_0
#define FLG_NET_HTTP_END        BIT_1
#define FLG_NET_FTP_BUSY        BIT_2
#define FLG_NET_SMTP_BUSY       BIT_3

/* cmd_thread_entry.hに移動
// g_command_event_flagsのイベントフラグ
#define FLG_CMD_EXEC            BIT_0
#define FLG_CMD_END             BIT_1
*/
//#define FLG_CMD_EXEC_UCOM       BIT_9
//#define FLG_CMD_EXEC_SCOM       BIT_10
//#define FLG_CMD_EXEC_BCOM       BIT_11
//#define FLG_CMD_END_UCOM        BIT_12
//#define FLG_CMD_END_SCOM        BIT_13
//#define FLG_CMD_END_BCOM        BIT_14

/* opt_cmd_thread_entry.hに移動
//optc_event_flagsのイベントフラグ
#define FLG_OPTC_END            BIT_0
*/



#define DHCP_READY      "0"
#define DHCP_EXEC       "1"
#define DHCP_SUCCESS    "2"
#define DHCP_ERROR      "3"

//==========  Event Flag Define  =============================================================
// tcp thread flag
#define FLG_TCP_INIT_SUCCESS    BIT_0
#define FLG_TCP_CONNECT         BIT_1       // for login send
#define FLG_TCP_DISCONNECT      BIT_2
#define FLG_TCP_RECEIVE_END     BIT_3

/* event.hに移動
// event thread flag cycle_event_flag　　Event Thread の動作許可　再起動
#define FLG_EVENT_READY         BIT_0       // event thread 動作許可
#define FLG_EVENT_RESTART       BIT_1       // event thread 再起動（初期化実行）
#define FLG_EVENT_CONNECT_USB    BIT_2      //USB接続された  2020.Jun.26
#define FLG_EVENT_DISCONNECT_USB    BIT_3   //USB切断された  2020.Jun.26
*/
// 自律動作用フラグ
#define FLG_EVENT_KEIHO         BIT_0
#define FLG_EVENT_MONITOR       BIT_1
#define FLG_EVENT_SUCTION       BIT_2
#define FLG_EVENT_INITIAL       BIT_3
#define FLG_EVENT_REGUMONI      BIT_4
#define FLG_EVENT_FULLMONI      BIT_5
#define FLG_EVENT_MASK          0x0000003f

// HTTP  event_flags_http
#define FLG_HTTP_SUCCESS         BIT_0
#define FLG_HTTP_ERROR           BIT_1
#define FLG_HTTP_READY           BIT_2      // http thread 動作許可 sakaguchi
#define FLG_HTTP_CANCEL          BIT_3

// FTP  event_flags_ftp
#define FLG_FTP_SUCCESS         BIT_0
#define FLG_FTP_ERROR           BIT_1

// SMTP event_flags_smtp
#define FLG_SMTP_SUCCESS        BIT_0
#define FLG_SMTP_ERROR          BIT_1

// HTTP FTP SMTP 全部共通の位置
#define FLG_SEND_ERROR          BIT_1


// Ethrnet Link event_flags_link
#define FLG_LINK_UP             BIT_0
#define FLG_LINK_DOWN           BIT_1
#define FLG_LINK_REBOOT         BIT_2       // Reboot 要求

// WIFI Link event_flags_wifi
#define FLG_WIFI_UP             BIT_0
#define FLG_WIFI_DOWN           BIT_1

// SNTP event_flags_sntp
#define FLG_SNTP_SUCCESS        BIT_0
#define FLG_SNTP_ERROR          BIT_1
#if 0
// USB event_flags_usb
#define FLG_USB_UP              BIT_0
#define FLG_USB_DOWN            BIT_1
#define FLG_USB_PSOC_COMPLETE   BIT_2
#define FLG_USB_PSOC_ERROR      BIT_3          ///<PSoC通信エラー
#endif
// Reboot event_flags_reboot
#define FLG_REBOOT_REQUEST      BIT_0   // コマンドで、Reboot要求を受信、　応答送信後に　FLG_REBOOT_EXECをセットし実行させる
#define FLG_REBOOT_EXEC         BIT_1   //

//
#define FLG_POWER_DOWN          BIT_0



#endif /* FLAG_DEF_H_ */
