/**
 * @file    MyDefine.h
 *
 * @date    Created on: 2019/02/12
 * @author      haya
 * @note	2020.Jul.01 GitHub 0701ソース反映済み
 */

#ifndef _MYDEFINE_H_
#define _MYDEFINE_H_

#include "tx_api.h"

//デバック出力制御
//#define  DBG_TERM   0           // debug out off
// release 時は、上の定義をコメントアウトを外す

#define CONFIG_NEW_STATE_CTRL   1                          // 2023.05.26
#define CONFIG_USE_WARNINGPORT_CLEAR_ONOFF_IN_BACKUP    1  // 2023.05.26
#define CONFIG_USE_WARNINGPORT_SET_FROM_W_CONFIG_ON     1  // 2023.05.26

#define UN_USE  0

typedef int error_t;

//#define LONG_MAX        INT64_MAX   //0x7fffffffL
//#define INT32_MAX       0x7fffffffL
//#define UINT32_MAX      0xffffffffL
//#define UINT16_MAX      0xfffff
//#define INT16_MAX       0x7ffff

//=====================================================
///　機種名
//=====================================================
// 2023.05.31 ↓ 移動
#define	UNIT_BASE_TANDD		    "RTR500BW"
#define	UNIT_BASE_ESPEC		    "RT24BN"
#define	UNIT_BASE_HITACHI		"RTR500BW-H"

#define	TEST_MODEL_TANDD		"RTR502B"
#define	TEST_MODEL_ESPEC		"RTW32S"
// 2023.05.31 ↑

#define VENDER_TD       0
#define VENDER_ESPEC    1
#define VENDER_HIT      9

#define COMPRESS_ALGORITHM_NOTICE 251

#define CONNECT_HTTPS   1
#define CONNECT_HTTP    0

// Network Status
#define NETWORK_UP          1
#define NETWORK_DOWN        0

// Link Status                  //sakaguchi UT-0035
#define LINK_UP         1
#define LINK_DOWN       0

// HTTP送信ファイル
typedef enum{
    FILE_C = 0,         // 機器設定
    FILE_W,             // 警報
    FILE_S,             // 吸い上げ
    FILE_M,             // モニター
    FILE_R,             // 操作リクエスト
    FILE_A,             // 操作リクエスト結果
    FILE_I,             // 機器状態
    FILE_L,             // ログ
    FILE_L_T,           // ログ送信テスト
    HTTP_FILE_MAX,      // HTTPファイル最大数 
}HTTP_FILE;

// 装置ステータス　UnitState	
#define	STATE_INIT			0		///< 装置ステータス　UnitState 初期化中
#define	STATE_OK			1		///< 装置ステータス　UnitState OK
#define	STATE_ERROR			2		///< 装置ステータス　UnitState エラー停止
#define	STATE_COMMAND		3		///< 装置ステータス　UnitState 要再起動
/*  ->usb_thread_entry.h
// USB UsbStaus
#define USB_CONNECT         1
#define USB_DISCONNECT      0
*/
/*  -> ble_thread_entry.h
// BLE BleStatus
#define BLE_CONNECT         1
#define BLE_DISCONNECT      0
*/
// LAN LanStatus                // sakaguchi UT-0035
#define LAN_CONNECT         1
#define LAN_DISCONNECT      0

// SNTP Status                  // sakaguchi UT-0035
#define SNTP_OK             1
#define SNTP_ERROR          0

// RFCH Status                  // sakaguchi UT-0035
#define CH_BUSY          	1
#define CH_OK           	0

// CmdMode   Local Commmad      // HTTPでテスト送信を行う場合に使用 
#define COMMAND_LOCAL       1
#define COMMAND_NON         0 

// WaitRequest                  // 自律動作の一時停止要求
#define WAIT_REQUEST        1
#define WAIT_CANCEL         0

// コマンド通信先
#define CMD_NON     0
#define CMD_USB     1
#define CMD_BLE     2
#define CMD_TCP     3
#define CMD_HTTP    4

// Network接続                  // sakaguchi UT-0035
#define ETHERNET    0
#define WIFI        1

// HTTP送信設定             Waring Monitor Suction で、HTTP送信
#define HTTP_SEND       1
#define HTTP_NOSEND     0

// Port 定義
#define SFM_CS          IOPORT_PORT_04_PIN_05               // (out) Serial Flash CS
#define OPT_PWR         IOPORT_PORT_06_PIN_15               // (out) 光通信電源　　                ON:Low Off:High
#if 0
//ble_thread_entry.hに移動
#define PORT_BLE_RESET       IOPORT_PORT_02_PIN_11               ///< (out) BLE Reset          Low： Reset
#define PORT_BLE_FREEZE      IOPORT_PORT_02_PIN_10               ///< (out) BLE Freeze         Low: Freeze          High: Normal
#define PORT_BLE_ROLE_CHANGE IOPORT_PORT_07_PIN_05               ///< (out) BLE Role Change    Low:クライアント（セントラル）　　High:サーバ（ペリフェラル）  アドバタイズを出すほう
#define PORT_BLE_CMONI        IOPORT_PORT_09_PIN_06               ///< (in ) BLE Connection Monitor
#endif

#define WIFI_RESET      IOPORT_PORT_07_PIN_04               ///< (out) WIFI Reset         Low Reset
#define WIFI_RESET_ACTIVE     g_ioport.p_api->pinWrite(WIFI_RESET, IOPORT_LEVEL_LOW)
#define WIFI_RESET_INACTIVE   g_ioport.p_api->pinWrite(WIFI_RESET, IOPORT_LEVEL_HIGH)

#define WIFI_RTS        IOPORT_PORT_04_PIN_06               ///< (out) WTFI CTS / CPU RTS
#define WIFI_CTS        IOPORT_PORT_0B_PIN_01               ///< (in ) WIFI RTS / CPU CTS

#if 0
//rf_thread_entry.hに移動
#define RF_RESET        IOPORT_PORT_05_PIN_00               ///< (out) RF Reset
#define RF_CS           IOPORT_PORT_08_PIN_04               ///< (out) RF CS
#define RF_BUSY         IOPORT_PORT_05_PIN_11               ///< (in ) RF BUSY  Low:busy




#define RF_CS_ACTIVE        g_ioport.p_api->pinWrite(RF_CS, IOPORT_LEVEL_LOW)
#define RF_CS_INACTIVE      g_ioport.p_api->pinWrite(RF_CS, IOPORT_LEVEL_HIGH)
#define RF_RESET_ACTIVE     g_ioport.p_api->pinWrite(RF_RESET, IOPORT_LEVEL_LOW)
#define RF_RESET_INACTIVE   g_ioport.p_api->pinWrite(RF_RESET, IOPORT_LEVEL_HIGH)
#endif

#define EX_WRAN_ON          g_ioport.p_api->pinWrite(IOPORT_PORT_00_PIN_14, IOPORT_LEVEL_LOW)
#define EX_WRAN_OFF         g_ioport.p_api->pinWrite(IOPORT_PORT_00_PIN_14, IOPORT_LEVEL_HIGH)
#define EX_WRAN				IOPORT_PORT_00_PIN_14

#define ETH_RESET           IOPORT_PORT_09_PIN_03
#define ETH_RESET_ACTIVE     g_ioport.p_api->pinWrite(ETH_RESET, IOPORT_LEVEL_LOW)
#define ETH_RESET_INACTIVE   g_ioport.p_api->pinWrite(ETH_RESET, IOPORT_LEVEL_HIGH)


// 10msec 分解能タイマー定義
#define WAIT_10MSEC     1
#define WAIT_20MSEC     2
#define WAIT_30MSEC     3
#define WAIT_40MSEC     4
#define WAIT_50MSEC     5
#define WAIT_60MSEC     6
#define WAIT_70MSEC     7
#define WAIT_80MSEC     8
#define WAIT_90MSEC     9

#define WAIT_100MSEC    10
#define WAIT_200MSEC    20
#define WAIT_300MSEC    30
#define WAIT_400MSEC    40
#define WAIT_500MSEC    50

#define WAIT_1SEC       100
#define WAIT_2SEC       200
#define WAIT_3SEC       300
#define WAIT_4SEC       400
#define WAIT_5SEC       500
#define WAIT_10SEC      1000
#define WAIT_30SEC      3000
#define WAIT_60SEC      6000

#define WAIT_1MIN       6000
#define WAIT_2MIN       12000
#define WAIT_3MIN       18000


///@note バイトスワップ用途以外ではなるべく使わない
typedef union
{
    uint16_t word;
    struct
    {
        uint8_t byte_lo;
        uint8_t byte_hi;
    };
} LO_HI;

///@note バイトスワップ用途以外ではなるべく使わない
typedef union
{
    uint32_t longword;
    struct
    {
        uint16_t lo;
        uint16_t hi;
    } word;
    struct
    {
        uint8_t lo;
        uint8_t ml;
        uint8_t mh;
        uint8_t hi;
    } byte;
} LO_HI_LONG;


#define DEFAULT_PART 0
#define DEFAULT_ALL  1


typedef enum {
    CODE_SOH = 0x01,                                            ///< ＳＯＨ　ＡＳＣＩＩコード
    CODE_STX = 0x02,                                            ///< ＳＴＸ
    CODE_ETX = 0x03,                                            ///< ＥＴＸ
    CODE_EOT = 0x04,                                            ///< ＥＯＴ
    CODE_ENQ = 0x05,                                            ///< ＥＮＱ
    CODE_ACK = 0x06,                                            ///< ＡＣＫ
    CODE_LF  = 0x0a,                                            ///< ＬＦ
    CODE_CR  = 0x0d,                                            ///< ＣＲ
    CODE_REFUSE = 0x0f,                                         ///< コマンド拒否
    CODE_DLE = 0x10,                                            ///< ＤＥＬ
    CODE_CH_BUSY = 0x12,                                        ///< チャンネルＢＵＳＹ（ＪＰ版で追加）
    CODE_BUSY = 0x13,                                           ///< ＢＵＳＹ
    CODE_NAK = 0x15,                                            ///< ＮＡＫ
    CODE_ETB = 0x17,                                            ///< ETB
    CODE_CAN = 0x18,                                            ///< ＣＡＮ
//    CODE_ESC = 0x1b,                                            ///< ＥＳＣ
    CODE_CRC = 0x1b,                                            ///< ＣＲＣエラー   // 2024 01 15 D.00.03.184
} CONTROLL_CHARACTER;


#define TEST_MODE   0x01

// for T comannd
#define MD_OK           0x00
#define MD_ERROR        0x80

#define SOURCE_USB    0x00
#define SOURCE_SERIAL 0x01
#define SOURCE_BLE    0x02

#define USE_REG_FRQ 255

// ETLAN 実行状態
#define ETLAN_INIT      0
#define ETLAN_BUSY      1
#define ETLAN_SUCCESS   2
#define ETLAN_ERROR     3

// BLE Pass Code Status
#define BLE_PASS_NG     0
#define BLE_PASS_OK     1


#define RFM_BAUDRATE 19200


#define RFM_NORMAL_END  0x00                                    ///< 正常終了

#define RFM_LOW_BATT    0xe0                                    ///< 電池電圧が低い
#define RFM_PASS_ERR    0xe1                                    ///< パスコード不正エラー

#define RFM_SERIAL_TOUT 0xf0                                    ///< シリアル通信のタイムアウト
#define RFM_DEADLINE    0xf1                                    ///< コマンド終了の締め切り時間切れ
#define RFM_CANCEL      0xf2                                    ///< キャンセル

#define RFM_R500_BUSY   0xf3                                    ///< R500無線モジュールがBUSY応答の場合
#define RFM_R500_NAK    0xf4                                    ///< R500無線モジュールがNAK応答の場合
#define RFM_R500_CH_BUSY 0xf5                                   ///< R500無線モジュールがCH_BUSY応答の場合

#define RFM_PRTCOL_ERR  0xf8                                    ///< R500無線モジュール通信で不正プロトコルであった
#define RFM_SERIAL_ERR  0xf9                                    ///< R500無線モジュールとのシリアル通信エラー
#define RFM_RU_PROTECT  0xfa                                    ///< 子機にプロテクトかかっていて記録開始できない
#define RFM_RT_SHORTAGE 0xfb                                    ///< 記録開始までの秒数足りない
#define RFM_REFUSE      0xfc                                    ///< コマンド処理が拒否された
#define RFM_KOKI_ERROR  0xfd                                    ///< 子機通信のエラー
#define RFM_RPT_ERROR   0xfe                                    ///< 中継機通信のエラー
#define RFM_SRAM_OVER   0xff                                    ///< ＳＲＡＭオーバー

#define RFM_INUSE       0xf6                                    ///< チャンネル空き無し(国内版で追加したステータス)
#define RFM_SERIAL_ERR2 0xf7                                    ///< R500無線モジュールとのシリアル通信エラー  api time out

// buff.rf_res.status の内容
#define BUSY_RF         0x01                                    ///< 無線通信中
#define END_REC_T_ERR   0x06                                    ///< 記録開始開始までの秒数足りず
#define END_PRO         0x07                                    ///< 記録開始プロテクト
#define END_CANCEL      0x08                                    ///< 無線キャンセル終了
#define END_RPT_ERR_H   0x09                                    ///< 中継機間エラー
#define END_RPT_ERR_L   0x0A                                    ///< 中継機間エラー
#define END_KOKI_ERR    0x0B                                    ///< 子機間エラー
#define END_OTHER_ERR   0x0C                                    ///< その他のエラー
#define END_TIMEOUT     0x0D                                    ///< タイムアウトエラー
#define END_OK          0x03                                    ///< 正常終了
#define END_OK_rpt      0x04                                    ///< 正常終了(中継機情報を追記しながら返る)
#define END_VARIOUS     0x05                                    ///< 汎用的な応答

// 無線コマンド応答で cmd2の内容
#define cmd2_R2R            0x00
#define cmd2_OUKA1          0x01
#define cmd2_OUKA2          0x02
#define cmd2_END_OK         0x03
#define cmd2_END_OK_rpt     0x04
#define cmd2_END_VARIOUS    0x05

// 無線コマンド応答で cmd2=[END_VARIOUS] 時の内容
#define para2_LOG_GET       0x0e                                //
#define para2_LOG_CLR       0x0f                                //
#define para2_REC_T_ERR     0x06                                //
#define para2_PRO           0x07                                //
#define para2_CANCEL        0x08                                ///< 無線キャンセル終了
#define para2_RPT_ERR_H     0x09                                //
#define para2_RPT_ERR_L     0x0A                                //
#define para2_KOKI_ERR      0x0B                                ///< 子機間エラー
#define para2_BUSY          0x0C                                ///< ＢＵＳＹエラー
#define para2_INUSE         0x02                                ///< チャンネル空き無しエラー

// ＲＦタスク　コマンド番号
#define RF_COMM_REPEATER_CLEAR      0x01                        ///< 無線子機　中継機の受信履歴読み込みとクリア
#define RF_COMM_MONITOR_INTERVAL    0x02                        ///< 無線子機　子機のモニタリング間隔設定
#define RF_COMM_PROTECTION          0x03                        ///< 無線子機　子機のプロテクト設定
#define RF_COMM_BROADCAST           0x04                        ///< 無線子機　子機検索（ブロードキャスト：ユニークＩＤ制限なし）

#define RF_COMM_SETTING_READ        0x05                        ///< 無線子機　設定値読み込み
#define RF_COMM_SETTING_WRITE       0x06                        ///< 無線子機　設定値書き込み
#define RF_COMM_CURRENT_READINGS    0x07                        ///< 無線子機　現在値読み込み
#define RF_COMM_GATHER_DATA         0x08                        ///< 無線子機　データ吸い上げ
#define RF_COMM_SEARCH              0x09                        ///< 無線子機　子機検索
#define RF_COMM_REPEATER_SEARCH     0x0a                        ///< 無線子機　中継機検索
#define RF_COMM_REAL_SCAN           0x0b                        ///< 無線子機　リアルスキャン
#define RF_COMM_RECORD_STOP         0x0c                        ///< 無線子機　記録停止
#define RF_COMM_SETTING_WRITE_GROUP 0x0d                        ///< 無線子機　グループ一斉記録開始
#define RF_COMM_REGISTRATION        0x0e                        ///< 無線子機　子機登録変更

#define RF_COMM_BLE_SETTING_READ    0x0f                        ///< 無線子機　ＢＬＥ設定読み出し
#define RF_COMM_BLE_SETTING_WRITE   0x10                        ///< 無線子機　ＢＬＥ設定書き込み
#define RF_COMM_MEMO_READ			0x11                        ///< 無線子機　スケール変換式読み出し
#define RF_COMM_MEMO_WRITE			0x12                        ///< 無線子機　スケール変換式書き込み

#define RF_COMM_WHOLE_SCAN			0x13						///< 無線子機　子機と中継機の全検索



#define RF_COMM_GET_MODULE_VERSION  0x20                        ///< 無線モジュールバージョン取得
#define RF_COMM_GET_MODULE_SERIAL   0x21                        ///< 無線モジュールシリアル番号取得
#define RF_COMM_DIRECT_COMMAND      0x22                        ///< 無線モジュールダイレクトコマンド



#define RF_COMM_SETTING_WRITE_FORMAT0 0x30                      ///< 無線子機　設定値書き込み　ＦＯＲＭＡＴ０
#define RF_COMM_SETTING_WRITE_FORMAT1 0x31                      ///< 無線子機　設定値書き込み　ＦＯＲＭＡＴ１

#define RF_COMM_PS_SETTING_READ      0x50                       ///< ＰＵＳＨ　設定値読み込み
#define RF_COMM_PS_SETTING_WRITE     0x51                       ///< ＰＵＳＨ　設定値書き込み
#define RF_COMM_PS_GATHER_DATA       0x52                       ///< ＰＵＳＨ　データ吸い上げ
#define RF_COMM_PS_DATA_ERASE        0x53                       ///< ＰＵＳＨ　データ消去
#define RF_COMM_PS_ITEM_LIST_READ    0x54                       ///< ＰＵＳＨ　品目リスト読み込み
#define RF_COMM_PS_ITEM_LIST_WRITE   0x55                       ///< ＰＵＳＨ　品目リスト書き込み
#define RF_COMM_PS_WORKER_LIST_READ  0x56                       ///< ＰＵＳＨ　作業者リスト読み込み
#define RF_COMM_PS_WORKER_LIST_WRITE 0x57                       ///< ＰＵＳＨ　作業者リスト書き込み
#define RF_COMM_PS_SET_TIME          0x58                       ///< ＰＵＳＨ　時刻設定
#define RF_COMM_PS_DISPLAY_MESSAGE   0x59                       ///< ＰＵＳＨ　メッセージ表示
#define RF_COMM_PS_REMOTE_MEASURE_REQUEST 0x5a                  ///< ＰＵＳＨ　リモート測定指示
#define RF_COMM_PS_WORK_GROUP_READ   0x5b                       ///< ＰＵＳＨ　ワークグループ読み込み
#define RF_COMM_PS_WORK_GROUP_WRITE  0x5c                       ///< ＰＵＳＨ　ワークグループ書き込み


//----- segi ------------------------------------------------------------------------------------
#define RF_EVENT_EXECUTE			0xB0						// 自律動作実行
//----- segi ------------------------------------------------------------------------------------

#define RF_RSSI_LIMIT   20                                      // 無線通信可能とするRSSIレベル

#define FULL_MONI   0                                           // フルモニタリング
#define REGU_MONI   1                                           // レギュラーモニタリング

//#define FULL_MONI_INTERVAL          3600*24                     // フルモニタリングタイマ(24時間)
#define FULL_MONI_INTERVAL          3600                     // フルモニタリングタイマ(1時間)


#define IT_STOP   1
#define IT_PERMIT 0

#define RS_PROC_START   0x00
#define RS_IRDA_JOINED  0x01
#define RS_SENDON_ZERO  0x02
#define RS_SRAM_OVER    0x03
#define RS_NUMB_OVER    0x04
#define RS_COMM_ERROR   0x05
#define RS_CANCELED     0x06
#define RS_RECORDING    0x07
#define RS_LIMIT_OVER   0x08

#define RS_MUST_BE_R5XX 0x10
#define RS_UNREGISTERED 0x11
#define RS_EEPROM_ERROR 0x12
#define RS_DUPLICATIN   0x13
#define RS_REFUSE       0x14

#define RS_INCOMPATIBLE 0x15

#define RS_PROC_END     0xff

#define PSTV_OFF 0
#define PSTV_ON  1





// 仕向け先
#define DST_US  0x30
#define DST_EU  0x40
#define DST_JP  0x50
#define DST_ES  0xe0


// 機種コードの設定

#define TR51    0x51f0
#define TR52    0x52f0
#define TR51A   0x50f0

#define TR51S   0x50f0
#define TR52S   0x52f0

#define TR51i   0x7104
#define TR52i   0x7204

#define RTR51   0x2404
#define RTR52   0x2504
#define RTR53   0x3504
#define RVR52   0x3304
#define RVR52A  0x4604

#define RTR501  0x5904
#define RTR502  0x6004
#define RTR503  0x6104

#define RTR507  0x9104

#define RTR501VA 0x005c                                         ///< Web Storage対応ファイル作成のためオリジナルヘッダテーブルを付加するときの機種コード
#define RTR502VA 0x005d
#define RTR503VA 0x005e
#define RTR507VA 0x0055

#define RTR505   0x00a0
#define RTR505TC 0x00a1
#define RTR505Pt 0x00a2
#define RTR505mA 0x00a3
#define RTR505V  0x00a4
#define RTR505P  0x00a5
#define RTR505IT 0x00a6
#define RTR505TH 0x00af

//#define RTR574  0x0467        //2020.01.20    仮でコメントアウト
//#define RTR576  0x0480        //2020.01.20    仮でコメントアウト

#define RTR576  0x56        //1Ｂｙｔｅ機種コード    2020.01.20
#define RTR574  0x57        //1Ｂｙｔｅ機種コード    2020.01.20

#define TR55i   0x5500
#define TR55iTC 0x5501
#define TR55iPt 0x5502
#define TR55imA 0x5503
#define TR55iV  0x5504
#define TR55iP  0x5505
#define TR55iIT 0x5506
#define TR55iHP 0x550e
#define TR55iTH 0x550f

//#define RTR505    0x7604
//#define RTR506    0x0480

#define TR71    0x61f0
#define TR72    0x62f0

#define TR71S   0x71f0
#define TR72S   0x72f0

#define TR71U   0x71f0
#define TR72U   0x72f0

#define TR71Ui  0x71f0
#define TR72Ui  0x72f0

#define TR77Ui  0x77f0

#define TR73U   0x0448
#define TR73UB  0x0486

#define TR74Ui  0x0457
#define TR76Ui  0x0464

#define VR71    0x22f4

#define RTR57C  0x0426
#define TR57C   0x0429

#define RTR57U  0x0445

#define TR57DCi 0x0463
#define RTR500DC 0x0478

#define UC_TR71U    0x0437                                      ///< ＩｒＤＡ　ＯＮでＲＳ２３２Ｃ吸い上げしたとき設計用機種コードが帰ってくるため準備（問い合わせ時に戻す）
#define UC_TR72U    0x0438

#define UC_TR71Ui   0x0465
#define UC_TR72Ui   0x0466

#define UC_TR74Ui   0x0457
#define UC_TR77Ui   0x0453

// 通信プロトコル
#define TR5_OLD 0x01
#define TR5_NEW 0x02
#define TR7_OLD 0x03
#define TR7_NEW 0x04
#define VR7_NEW 0x05

#define COM_RX      0
#define COM_TX      1
#define COM_TX_RX   3


//http(s)通信用
#define POST_MAX_SIZE       51200          //POSTデータサイズ(50KB)

//http(s)通信状態フラグ
#define SND_OFF     0       //送信無し
#define SND_ON      1       //送信有り
#define SND_DO      2       //送信中
#define SND_ACT0    3       //ACT0送信中
#define RCV_ACT0    4       //ACT0受信
#define SND_ACT1    5       //ACT1送信中
#define RCV_ACT1    6       //ACT1受信
#define SND_STEP1   7       //STEP1送信中
#define RCV_STEP1   8       //STEP1受信
#define SND_STEP2   9       //STEP2送信中
#define RCV_STEP2   10      //STEP2受信
#define SND_STEP3   11      //STEP3送信中
#define RCV_STEP3   12      //STEP3受信



//http(s)結果
#define E_OK        0       //成功
#define E_NG        1       //失敗
#define E_OVER      2       //サイズオーバー
#define E_RF_BUSY   3       //無線通信中


//http(s)コマンド
typedef enum{
    HTTP_CMD_NONE  = 0x00,  //0  定義なし
    HTTP_CMD_WUINF,         //1  機器設定（変更）
    HTTP_CMD_RUINF,         //2  機器設定（取得）
    HTTP_CMD_WBLEP,         //3  Bluetooth設定（変更）
    HTTP_CMD_RBLEP,         //4  Bluetooth設定（取得）
    HTTP_CMD_WNSRV,         //5  ネットワークサーバ設定（変更）
    HTTP_CMD_RNSRV,         //6  ネットワークサーバ設定（取得）
    HTTP_CMD_WNETP,         //7  ネットワーク情報設定（変更）
    HTTP_CMD_RNETP,         //8  ネットワーク情報設定（取得）
    HTTP_CMD_WWLAN,         //9  無線LAN情報設定（変更）
    HTTP_CMD_RWLAN,         //10 無線LAN情報設定（取得）
    HTTP_CMD_WDTIM,         //11 時刻設定（変更）
    HTTP_CMD_RDTIM,         //12 時刻設定（取得）
    HTTP_CMD_WENCD,         //13 エンコード方式（変更）
    HTTP_CMD_RENCD,         //14 エンコード方式（取得）
    HTTP_CMD_WWARP,         //15 警報設定（変更）
    HTTP_CMD_RWARP,         //16 警報設定（取得）
    HTTP_CMD_WCURP,         //17 モニタリング設定（変更）
    HTTP_CMD_RCURP,         //18 モニタリング設定（取得）
    HTTP_CMD_WSUCP,         //19 記録データ送信設定（変更）
    HTTP_CMD_RSUCP,         //20 記録データ送信設定（取得）
    HTTP_CMD_WUSRD,         //21 ユーザ定義情報（変更）
    HTTP_CMD_RUSRD,         //22 ユーザ定義情報（取得）
    HTTP_CMD_WRTCE,         //23 httpsルート証明書（変更）
    HTTP_CMD_RRTCE,         //24 httpsルート証明書（取得）
    HTTP_CMD_WVGRP,         //25 グループ情報（変更）
    HTTP_CMD_RVGRP,         //26 グループ情報（取得）
    HTTP_CMD_WPRXY,         //27 プロキシ設定（変更）
    HTTP_CMD_RPRXY,         //28 プロキシ設定（取得）
    HTTP_CMD_WSETF,         //29 登録情報設定（変更）
    HTTP_CMD_RSETF,         //30 登録情報設定（取得）
    HTTP_CMD_EINIT,         //31 初期化と再起動
    HTTP_CMD_ETGSM,         //32 テスト（3GおよびGPSモジュール）
    HTTP_CMD_ETLAN,         //33 テスト
    HTTP_CMD_EBSTS,         //34 ステータスの取得と設定
    HTTP_CMD_ELOGS,         //35 ログの取得および消去
    HTTP_CMD_ETRND,         //36 トレンド情報（モニタリングデータ）取得
    HTTP_CMD_EMONS,         //37 モニタリングを開始する
    HTTP_CMD_EDWLS,         //38 記録データ送信開始
    HTTP_CMD_ERGRP,         //39 指定グループの無線通知設定取得
    HTTP_CMD_EWLEX,         //40 子機の電波強度を取得する(無線通信)
    HTTP_CMD_EWLRU,         //41 中継機の電波強度を取得する(無線通信)
    HTTP_CMD_EWSTR,         //42 子機の設定を取得する(無線通信)
    HTTP_CMD_EWSTW,         //43 子機の設定を変更する(記録開始)(無線通信)
    HTTP_CMD_EWCUR,         //44 子機の現在値を取得する(無線通信)
    HTTP_CMD_EWLAP,         //45 無線LANのアクセスポイントを検索する
    HTTP_CMD_EWSCE,         //46 子機の検索（無線通信）
    HTTP_CMD_EWSCR,         //47 中継機の一斉検索（無線通信）
    HTTP_CMD_EWRSC,         //48 子機のモニタリングデータを取得する（無線通信）
    HTTP_CMD_EWBSW,         //49 一斉記録開始（無線通信）
    HTTP_CMD_EWRSP,         //50 子機の記録を停止する（無線通信）
    HTTP_CMD_EWPRO,         //51 プロテクト設定（無線通信）
    HTTP_CMD_EWINT,         //52 子機のモニタリング間隔変更（無線通信）
    HTTP_CMD_EWBLW,         //53 子機のBluetooth設定を設定する(無線通信)
    HTTP_CMD_EWBLR,         //54 子機のBluetooth設定を取得する(無線通信)
    HTTP_CMD_EWSLW,         //55 子機のスケール変換式を設定する(無線通信)
    HTTP_CMD_EWSLR,         //56 子機のスケール変換式を取得する(無線通信)
    HTTP_CMD_DMY57,         //57 ダミー
    HTTP_CMD_EAUTH,         //58 パスワード認証
    HTTP_CMD_EBADV,         //59 アドバタイジングデータの取得
    HTTP_CMD_EWRSR,         //60 子機と中継機の一斉検索（無線通信）
    HTTP_CMD_EWREG,         //61 子機登録変更（無線通信）
    HTTP_CMD_ETWAR,         //62 警報テストを開始する
    HTTP_CMD_EMONR,         //63 モニタリングで収集した無線通信情報と電池残量を取得する
    HTTP_CMD_EMONC,         //64 モニタリングで取得した電波強度を削除する
    HTTP_CMD_WREGI,         //65 登録情報の設定を変更する（子機）
    HTTP_CMD_EWAIT,         //66 自律動作の開始しない時間を設定する
    HTTP_CMD_EFIRM,         //67 ファームウェア更新
    HTTP_CMD_DMY68,         //68 ダミー
    HTTP_CMD_MAX,           //128byte以上に増やす場合はG_HttpCmd[]のバッファサイズ変更
                            //HTTP_T2Cmd[]の定義と順番をあわせること
}HTTP_CMD;



#endif /* _MYDEFINE_H_ */
