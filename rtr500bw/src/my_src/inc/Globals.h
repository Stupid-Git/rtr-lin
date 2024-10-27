/**
 * @file	 Globals.h
 *
 * @date	 Created on: 2019/01/23
 * @author	haya
 * @note	2020.01.30	v6ソースマージ済み
 * @note	2020.Jul.01 GitHub 0701ソース反映済み

 */

#ifndef GLOBALS_H_
#define GLOBALS_H_


//#include "bsp_api.h"
#include "tx_api.h"
#include "nx_api.h"
//#include "hal_data.h"
#ifdef EDF
#undef EDF
#endif
#ifndef GLOBALS_C_
    #define EDF extern
#else
    #define EDF
#endif


// 外部RAMは、バイトアクセス ！！  
// xram にアサイン時は注意のこと

EDF int16_t	HttpFile;			///<  0: Monitor 1: Suction
EDF int16_t Http_Use;           //    0: Warning,Motior,Suction ともに、HTTPで送信しない    X:何れかをHTTPで送信する    2020.10.16

EDF	int16_t	UnitState;			///<  自律動作中かどうか


EDF int16_t	NetStatus;			///<  0: 未接続 	1: 接続完了
EDF int16_t NetCmdStatus;		///<  0: non	1: Busy	2: Success	3: Error
EDF int16_t NetReboot;			//
EDF int16_t Link_Status;		//  Ethrnet Link Status  1:Link up	0:Link down
EDF int16_t DHCP_Status;		//  DHCP Status  0: 静的IPアドレス 1:DHCP取得成功 2:DHCP取得失敗 3:DHCP取得中

EDF int16_t Sntp_Status;		//  Sntp Status  0: 未接続 	1: 接続完了    //sakaguchi UT-0035
EDF int16_t RfCh_Status;		//  RfCH Busy  0: OK 	1:Busy             //sakaguchi UT-0035

//EDF int16_t UsbState;			///<  0: Down	1: UP   ->usb_thread_entr.h
EDF int16_t CmdMode;            //  0: Run  1:Comannd Mode (USB接続 BLE-Socket Command実行中)  Test送信の為に仕様
EDF int16_t WaitRequest;        //  0: Run  1:Wait      EWAITコマンドでの自律禁止を行います

EDF int16_t TestMode;			// 1:Test Mode


//EDF volatile int CmdStatusSize;        ///<コマンドステータスのサイズ  @attention 注意：複数の関数から書き込まれる @note 2020.01.17 intに変更 cmd_thread_entry.hに移動

EDF int32_t oneday_sec;		    ///<  1日の秒数
//EDF int32_t TimeDiff;           // 時差情報　RTC.Diffから変更

EDF int     http_count;			///<  debug
EDF int     success_count;		///<  debug
EDF int 	http_error;	
EDF uint16_t    tls_error;          // 		
//EDF int32_t RtcDiff_Backup;     // debug
//EDF int     Rtc_err;            // debug
EDF uint16_t body_get_count;        // debug
EDF UINT    body_status;            // debug

EDF struct {
	uint32_t	NetUtc;			///< 最後にネットワーク通信した時刻（UTIM）
	uint32_t	NetStat;		///< 最後のネットワーク通信の結果（コード）
	char		NetMsg[30];		///< 最後のネットワーク通信の結果（メッセージ）
} NetLogInfo;

EDF int16_t  NetTest;            //    0: 接続OK　1:接続NG　2:接続実行中  
EDF uint16_t NetTestCount;       //   

EDF NXD_ADDRESS target_ip;          // ip address of host

//=============  その他 =================================
EDF char	ConvertTemp[50];		///<  単位変換用
EDF char	ScaleString[64+2];		///<  スケール変換用(1データ)
EDF char	FormatScale[64+6];		///<  スケール変換最終データ
EDF char	PreScale[50];			///<  スケール変換用
EDF char	MulTemp[30];			///<  10進演算用
EDF char	MulAddTemp[30];			///<  10進演算用


EDF uint8_t inspection_switch;      ///< @bug   セットしている個所が居ない

EDF uint8_t KGC_ON;			        // RTR505BでKGCモード(L5_L10)なら=1

// 10msec and 1msec and 1sec
EDF volatile struct def_wait {
//--- 10 msec callback  timer1
//    struct {
//        uint32_t ble;
//        uint32_t scom;
//        uint32_t pow;
//        uint32_t psoc_restert;
//    } stop_mode;
    uint32_t sfm;
//    struct {
//        uint32_t comm;
//        uint32_t tx;
//    } usb;
//    uint32_t ble_comm;
//    uint32_t scom_comm;
//    uint32_t opt_comm;    //→不使用に
    uint32_t rfm_comm;
//    uint32_t r500_comm;       //未使用
    uint32_t channel_busy;
    uint32_t radio_icon_indicate;
    uint32_t battery_measure_interval;
    uint32_t client;
    uint32_t inproc_rfm;
    uint32_t passcode_denial;
//    uint32_t update_timeout;

//--- 1 msec   callback timer2
    struct {
        uint32_t ble_reset_tm;  ///関数名と重複しているので修正 2020.01.20
        uint32_t rf_wait;
        uint32_t sfm_wait;
    } ms_1;

//--- 1 sec  use RTC
    struct{
        uint32_t soc_time;              ///<  socket通信 login後の接続監視
        uint32_t mate_command;         //  自律動作中断
        uint32_t ext_warn;              //  一時警報（監視期間のOn Off）の出力Off   
    } sec_1;
    
} wait;

EDF volatile int soc_time_flag;         ///<  1:socket timer再セット
EDF volatile int mate_time_flag;        //  1:mate timer再セット     自律停止タイマー
EDF volatile int warn_time_flag;        //  1:セット

EDF volatile int sfm_flag;

EDF volatile int mate_time;             //  mate time値             自律停止タイマー

EDF volatile int mate_at_start;         //  AT_START停止

EDF volatile int mate_at_start_reset;   //  1=AT_START実行時に警報samクリアさせる

/**
 * rf timer
 * @note    パディングあり
 */
EDF volatile  struct def_timer
{
    bool latch_enable;                                          ///<  到達カウントのラッチ許可
    uint16_t    arrival;                                        ///<  子機へのコマンド到達カウント
    uint16_t    int125;                                         ///<  125msカウンタ
    uint16_t    int1000;                                        ///<  1000msカウンタ
} timer;

//EDF NX_DNS  *g_active_dns;
//EDF NX_IP   *g_active_ip;                /* Pointer to active IP interface */
//EDF NX_DHCP *g_active_dhcp;              /* Pointer to active DHCP client */

EDF struct def_net_addres{

    struct{
        uint32_t    address;
        uint32_t    mask;
        uint32_t    gateway;
        uint32_t    dns1;
        uint32_t    dns2;
// 2023.02.20 ↓
        uint32_t    dns3;
        uint32_t    dns4;
        uint32_t    dns5;
// 2023.02.20 ↑
        //uint64_t    mac;
        uint32_t    mac1;
        uint32_t    mac2;
    } active;


    struct{
        uint32_t    address;
        uint32_t    mask;
        uint32_t    gateway;
        uint32_t    dns1;
        uint32_t    dns2;
// 2023.02.20 ↓
        uint32_t    dns3;
        uint32_t    dns4;
        uint32_t    dns5;
// 2023.02.20 ↑
        //uint64_t    mac;
        uint32_t    mac1;
        uint32_t    mac2;
    } eth;

    struct{
        uint32_t    address;
        uint32_t    mask;
        uint32_t    gateway;
        uint32_t    dns1;
        uint32_t    dns2;
// 2023.02.20 ↓
        uint32_t    dns3;
        uint32_t    dns4;
        uint32_t    dns5;
// 2023.02.20 ↑
        //uint64_t    mac;
        uint32_t    mac1;
        uint32_t    mac2;
    } wifi;

    struct{
        uint32_t    get_ip;
        CHAR        target_name[64];
    } dns;

}net_address;

typedef struct netif_static_mode
{
    ULONG address;
    ULONG mask;
    ULONG gw;
    ULONG dns;
}netif_static_mode_t;

/**
 * @note    パディングあり
 */
EDF struct def_net_cfg
{
    uint8_t netif_valid;
    uint8_t netif_select[16];
    uint8_t netif_addr_mode;
    uint8_t interface_index;
	netif_static_mode_t netif_static;
}net_cfg;


/**
 * R500通信 SOHフレーム構造体定義
 * @note    パディングあり
 * @note    T2コマンドにも使用
 */
typedef struct {
    char     for_packing;
    char     header;
    char     command;
    char     subcommand;
    uint16_t    length;
    char     data[1024+128];
//    uint32_t    start;        //未使用なのでコメントアウト
//   uint32_t    end;        //未使用なのでコメントアウト
//    uint16_t    capacity;        //未使用なのでコメントアウト
//    char     *rwPtr;        //未使用なのでコメントアウト
}__attribute__ ((__packed__)) def_comformat;

/**
 * @note    パディングあり
 */
typedef struct {
    char     for_packing;
    char     header;
    char     command;
    char     subcommand;
    uint16_t    length;
//    char     data[4096 + 64 +128];
//    char     data[1024*12 + 64 +128];
    char     data[1024*5];                  // 5KB sakaguchi cg
//    uint32_t    start;        //未使用なのでコメントアウト
//    uint32_t    end;        //未使用なのでコメントアウト
//    uint16_t    capacity;        //未使用なのでコメントアウト
//    char     *rwPtr;        //未使用なのでコメントアウト
}__attribute__ ((__packed__)) def_comformat_u;



typedef struct{
    def_comformat   rxbuf;
    def_comformat   txbuf;
} def_com;

typedef struct{
    def_comformat_u   rxbuf;
    def_comformat_u   txbuf;        // sakaguchi 2021.04.21
} def_com_u;



//EDF def_com_u UCOM;                  ///<  USB 廃止
//EDF def_com OCOM;                   ///<  Opti Comm → opt_cmd_thread_entry.h ローカル変数に変更
//EDF def_com RCOM;                   ///<  RF ローカル変数に変更
//EDF def_com SCOM;                   ///<  Socket
//EDF def_com BCOM;                   ///<  BLE ローカル変数に変更
EDF def_com_u HTTP;                 ///<  HTTP			sakaguchi cg




/**
 * @note    パディングあり
 */
EDF struct def_property {
    char     protocol;
    char     result;
    char     model;
    char     ch_name[4][8];
    char     name[16];
    char     channel;
    char     attribute[4];
    uint16_t    interval;
    uint32_t    start_time;
    uint32_t    last_time;
    uint8_t     record_start;
    uint8_t     record_mode;
    uint16_t    sendon_number;
    uint16_t    record_number;
    uint16_t    acquire_number;
    uint16_t    transferred_number;
    uint16_t    max_data[4];
    uint16_t    max_point[4];
    uint16_t    min_data[4];
    uint16_t    min_point[4];
    int32_t     intercept;
    int32_t     slope;
    uint32_t    serial;
}__attribute__ ((__packed__)) property;


#pragma pack(1)
/// 登録情報ヘッダ  16 byte
//EDF struct def_root_registry{
typedef struct {
    uint16_t    length;                                             ///<  0x10  <-- 0x40から変更
    char     header;                                             ///<  0xfb
    char     group;												///<  グループ数
    char        auto_route;										///<  自動無線ルート（0:無効,1:有効）
    uint8_t     rssi_limit1;                                    ///<  電波強度閾値１
    uint8_t     rssi_limit2;                                    ///<  電波強度閾値２
    char        dummy[8];									    ///<  予備
    char     version;
} def_root_registry;// __attribute__((aligned(4)));
def_root_registry root_registry;


/// グループ登録情報 48 byte
//EDF struct def_group_registry{
typedef struct {
    uint16_t    length;                                               ///<  0x30
    char     header;                                               ///<  0xfc
    char     altogether;                                           ///<  中継機の総数 ０～
    char     remote_unit;                                          ///<  登録子機数
    char     frequency;
    char     band;
    char     max_unit_no;        //uint8_t     dummy;
    char     id[8];
    char     name[32];

} def_group_registry;//  __attribute__((aligned(4)));
def_group_registry group_registry;

/// 中継機登録情報
EDF struct def_rpt_registry{
    uint16_t    length;                                               ///<  0x40
    char     header;                                               ///<  0xfd
    char     number;                                               ///<  中継機の番号 １～
    char     superior;                                             ///<  上位の中継機番号 ０：親機
    char     dummy;
    uint32_t    serial;
    char     abbr[3];
    char     settings;
    char     remain[12];
    struct def_rpt_registry_ble{
        char     name[26];
        uint32_t    passcode;
        uint64_t    device_address;
    }__attribute__ ((__packed__)) ble;
}__attribute__ ((__packed__)) rpt_registry;//  __attribute__((aligned(4)));


/// 子機登録情報
//EDF union def_ru_registry{
typedef union {

    struct def_501{

        uint16_t length;                                        // 0  ０ｘ６０
        char header;                                            // 2  ０ｘｆｅ
        char number;                                            // 3  子機の番号 １～
        char superior;                                          // 4  上位の中継機番号 ０：親機
        char set_flag;                                          // 5  自律動作フラグなど
        uint32_t serial;                                        // 6  子機シリアル番号
        uint16_t interval;                                      // 10

        char attribute[2];                                      // 12 CH1、CH2属性
        char judge_time[2];                                     // 14
        char name[8];                                           // 16  予備
        uint16_t limiter1[2];                                   // 24  CH1下上限値
        uint16_t limiter2[2];                                   // 28  CH2下上限値
        char limiter_flag;                                      // 32
        char data_intt;                                         // 33  モニタテータ間隔[分]
        char rec_mode;                                          // 34  記録モード
        char remain[23];                                        // 35  予備
        struct def_501_ble{
            char name[26];                                      // 58        
            uint32_t passcode;                                  // 84
            uint64_t device_address;                            // 88-95
        } ble;

    }__attribute__ ((__packed__)) rtr501;	//  __attribute__((aligned(4)));

    struct def_574{

        uint16_t length;                                        // 0  ０ｘ４０
        char header;                                            // 2  ０ｘｆａ
        char number;                                            // 3  子機の番号 １～
        char superior;                                          // 4  上位の中継機番号 ０：親機
        char set_flag;                                          // 5
        uint32_t serial;                                        // 6  子機シリアル番号
        uint16_t interval;                                      // 10

        char name[8];                                           // 12  子機名
        char attribute[4];                                      // 20  CH1、CH2、CH3、CH4属性
        char judge_time[4];                                     // 24
        uint16_t limiter1[2];                                   // 28  CH1下上限値
        uint16_t limiter2[2];                                   // 32  CH2下上限値
        uint16_t limiter3[2];                                   // 36  CH3下上限値
        uint16_t limiter4[2];                                   // 40  CH4下上限値
        char limiter_flag;                                      // 44
        char data_intt;                                         // 45  モニタテータ間隔[分]
        char rec_mode;                                          // 46  記録モード
        char sekisan_flag;                                      // 47  積算上限値有効フラグ 
        uint16_t sekisan_lu;                                    // 48  積算照度上限値
        uint16_t sekisan_uv;                                    // 50  積算紫外線量上限値
        char sosa_lock;                                         // 52  操作スイッチロック
        char lcd_mode;                                          // 53  液晶表示設定
        char gap2[4];                                           // 54                                           
        char name2[26];                                         // 58                                             
        char gap3[12];                                          // 84-95

    }__attribute__ ((__packed__)) rtr574;	//  __attribute__((aligned(4)));

    struct def_576{

        uint16_t length;                                        // 0  ０ｘ４０
        char header;                                            // 2  ０ｘｆ９
        char number;                                            // 3  子機の番号 １～
        char superior;                                          // 4  上位の中継機番号 ０：親機
        char set_flag;                                          // 5
        uint32_t serial;                                        // 6  子機シリアル番号
        uint16_t interval;                                      // 10

        char name[8];                                           // 12  子機名
        char attribute[4];                                      // 20  CH1、CH2、CH3、CH4属性
        char judge_time[4];                                     // 24
        uint16_t limiter1[2];                                   // 28  CH1下上限値
        uint16_t limiter2[2];                                   // 32  CH2下上限値
        uint16_t limiter3[2];                                   // 36  CH3下上限値
        uint16_t limiter4[2];                                   // 40  CH4下上限値
        char limiter_flag;                                      // 44
        char data_intt;                                         // 45  モニタテータ間隔[分]
        char rec_mode;                                          // 46  記録モード
        char aki;                                               // 47
        uint16_t comp_error;                                    // 48  気圧補正 誤差
        //uint16_t comp_gain;                                     // 50  気圧補正 ゲイン
        //uint16_t comp_offset;                                   // 52  気圧補正 オフセット
        //uint16_t yobi1;                                         // 54
        //char yobi2;                                             // 56
        char sosa_lock;                                         // 50  操作スイッチロック
        char lcd_mode;                                          // 51  液晶表示設定
        char gap2[6];                                           // 52     
        char name2[26];                                         // 58                                             
        char gap3[12];                                          // 84-95
        
        //char function;                                          // 59  機能設定
        //char yobi3;                                             // 60    
        //char channel;                                           // 61  記録チャンネル数
        //uint16_t fulldata;                                      // 62 -63   フルデータ数

    }__attribute__ ((__packed__)) rtr576;	//  __attribute__((aligned(4)));

    struct def_505{

        uint16_t length;                                        // 0  ０ｘｃ０
        char header;                                            // 2  ０ｘｆ８
        char number;                                            // 3  子機の番号 １～
        char superior;                                          // 4  上位の中継機番号 ０：親機
        char set_flag;                                          // 5   
        uint32_t serial;                                        // 6  子機シリアル番号
        uint16_t interval;                                      // 10

        char attribute[2];                                      // 12  CH1、CH2属性
        char judge_time[2];                                     // 14
        char gap01[8];                                          // 16  予備
        uint16_t limiter1[2];                                   // 24  CH1下上限値
        uint16_t limiter2[2];                                   // 28  CH2下上限値
        char limiter_flag;                                      // 32
        char data_intt;                                         // 33  モニタテータ間隔[分]
        char rec_mode;                                          // 34  記録モード

        char gap02; /*scale_flag;*/                             // 35  スケール変換
        char model;                                             // 36  １バイト機種コード
        char recode_setting;                                    // 37  記録方式
        char display_unit;                                      // 38  表示単位
        char preheat_type;                                      // 39  プレヒート設定
        uint16_t preheat_time;                                  // 40  プレヒート時間
        char watch_pulse;                                       // 42  パルス警報信号種別
        char gap03[15];                                         // 43    
        struct def_505_ble{                                     // 58                                        
			char name[26];      
			uint32_t passcode;                                  // 84    
			uint64_t device_address;                            // 88
		} ble;
		
        char scale_setting[64];								// 96 スケール変換、単位設定

        /*
        char remain2[18];                                       // 43
        char record_channel;                                    // 61  記録データチャンネル数
        uint16_t record_capacity;                                 ///<  記録容量（データ数）
        char scale_setting[64];                                ///<  スケール変換、単位設定
        char remain3[26];
        struct def_505_ble{
            char name[26];									  ///<  子機名
            uint32_t passcode;
            uint64_t device_address;
        } ble;
        */
    }__attribute__ ((__packed__)) rtr505;	//  __attribute__((aligned(4)));

} def_ru_registry;
def_ru_registry ru_registry;

#pragma pack(4)


/// 子機 ヘッダ 64Byte
EDF struct def_ru_header
{
    char intt[2];                                              ///<  記録インターバル
    char start_time[8];                                        ///<  記録開始日時(GMT)
    char start_mode;                                           ///<  記録開始方法
    char rec_mode;                                             ///<  記録モード
    uint32_t wait_time;                                            ///<  最終記録からの秒数
    char disp_unit;                                            ///<  LCD表示単位
    char rec_protect;                                          ///<  記録開始プロテクト
    char data_intt;                                            ///<  モニタテータ間隔[分]
    char ch1_comp_time;                                        ///<  CH1(CH3)警報判定時間
    char ch1_lower[2];                                         ///<  CH1(CH3)下限値
    char ch1_upper[2];                                         ///<  CH1(CH3)上限値
    char ch2_comp_time;                                        ///<  CH2(CH4)警報判定時間
    char ch2_lower[2];                                         ///<  CH2(CH4)下限値
    char ch2_upper[2];                                         ///<  CH2(CH4)上限値
    char warning_flag;                                         ///<  警報フラグ
    char data_byte[2];                                         ///<  吸上げデータバイト数
    char all_data_byte[2];                                     ///<  全記録データバイト数
    char end_no[2];                                            ///<  最終記録データ番号
    uint32_t serial_no;                                            ///<  子機のシリアル番号
    char batt;                                                 ///<  電池残量
    char aki2;                                                 ///<
    char ch_zoku[2];                                           ///<  CH1(CH3)、CH2(CH4)記録属性
    char group_id[8];                                          ///<  グループ名(8バイト)
    char unit_name[8];                                         ///<  子機名(8バイト)
    char unit_no;                                              ///<  子機番号
    char freq_ch;                                              ///<  周波数チャンネル
    char kisyu_code;                                           ///<  機種コード
    char ul_protect;                                           ///<  上下限書き換えプロテクト
}__attribute__ ((__packed__)) ru_header;


/**
 * 制御部とやり取りする為の構造体
 * @note    パディングあり
 */
EDF union def_buff
{
    // USB、RS-232Cでコマンド入力 18byte
    struct def_rf_req{
        char cmd1;
        char cmd2;
        uint32_t time;             ///<  使用しないこと
        uint16_t data_size;
        //uint8_t *data_poi;
        //uint8_t freq_no;
        //uint rpt_cnt;
        //uint8_t *rpt_poi;
        //uint8_t group_id[8];
        //uint8_t node;
        //uint8_t mode;
        char cancel;                                           ///<  キャンセル要求。無線通信開始時は、ここをクリアする
        char current_group;									///<  現在通信対象となっているグループ番号を示す(自律動作時に自動的に設定される)
        char data_format;									///<  リアルスキャンのDATA
        char short_cut;                                        ///<  ショートカットさせる場合に1,させない場合0
        uint16_t timeout_time;                                      ///<  無線通信時のタイムアウト時間
    }__attribute__ ((__packed__)) rf_req ;

    /// USB、RS-232Cでコマンド応答 9byte
    struct def_rf_res{
        char cmd1;
        char status;
        uint32_t time;
        uint16_t data_size;
        //uint8_t *data_poi;
    }__attribute__ ((__packed__)) rf_res ;

} RF_buff;

/// R500とのコマンド送受信バッファ 24byte
struct def_r500
{
    char     dummy;                                                ///<  偶数バイトにする為のダミー
    char     soh;
    char     cmd1;
    char     cmd2;
    uint16_t    len;
    char     end_rpt;
//    char     start_rpt;
	union def_route {
        char start_rpt;
        char rpt_max;											///<  ＥＷＲＳＲコマンド追加、検索子機最大番号を格納する
	} route;
	char     my_rpt;
	char     next_rpt;
    uint16_t    time_cnt;
    uint16_t    data_size;                                             ///<  追加データ長
    struct
    {
        char rpt_cnt;
        char cmd_id;
    }__attribute__ ((__packed__)) para1;                                                    ///<  中継情報
    char     para2[2];
    char     rf_cmd1;
    char     rf_cmd2;
    char     node;
    char     rssi;
    uint16_t    sum;
}__attribute__((aligned(1)));

EDF struct def_r500 r500, r500_backup, r500_dummy;

//extern uint16_t CurrentWarning;                                     // 警報の状態 XRAM

EDF uint32_t regf_rfm_serial_number;                            ///<  無線モジュール シリアル番号
EDF uint32_t regf_rfm_version_number;                           ///<  無線モジュール バージョン番号
EDF uint32_t regf_cmd_id_back;                                  ///<  無線コマンドIDバックアップ上り（親機のとき下位のみ使用 ／ 中継機のとき、上り（子機へ）：下位 下り（子機から）：上位）
EDF uint32_t regf_battery_voltage;                              ///<  電池電圧［ｍＶ］  @bug    セットしている個所が居ない

EDF uint32_t regf_shortcut_info0;                               ///<  ショートカット情報
EDF uint32_t regf_shortcut_info1;
EDF uint32_t regf_shortcut_info2;
EDF uint32_t regf_shortcut_info3;


EDF char rpt_sequence[256];
EDF uint8_t rpt_sequence_cnt;

EDF bool repeater_is_engaged;                                   ///<  無線ＬＥＤ点滅期間を決めるフラグ

// 現在値送信、自動データ吸上げのリトライ回数カウンタ
EDF uint8_t	Download_ReTry;										///<  自動データ吸上げのリトライをさせるフラグ @bug 複数スレッドで使用している
EDF uint8_t CurrRead_ReTry;										///<  現在値送信リトライカウンタ @bug 複数スレッドで使用している
//EDF char unit_name_buff[26];									///<  子機名を登録ファイルから読み出し、モニタリングデータに渡す為のバッファ segi//未使用

#if 0
//未使用
/**
 * @note    パディングあり
 */
EDF union{
    struct {
        uint8_t     TxBuff[512];
    };
    struct{
        uint8_t     header;
        uint8_t     command;
        uint8_t     subcommand;
        uint16_t    length;
        uint8_t     data[512-5];
    }__attribute__((aligned(1)));
}__attribute__ ((__packed__)) RCOM_TX;


/**
 * @note    パディングあり
 */
EDF union{
    struct {
        uint8_t     TxBuff[512];
    };
    struct{
        uint8_t     header;
        uint8_t     command;
        uint8_t     subcommand;
        uint16_t    length;
        uint8_t     data[512-5];
    };
}__attribute__ ((__packed__)) RCOM_RX;

#endif



#if 0
//未使用
typedef struct {
        //UINT    Size;                             ///<  バッファにある有効データ数
        uint16_t    pRD;                            ///<  次に読み込むポインタ
        uint16_t    pWR;                            ///<  次に書き込むポインタ
        uint16_t    Capacity;                       ///<  バッファ容量
        //uint8_t   *Buff;                            ///<  バッファ開始アドレス
        uint8_t     Buff[1024];
} def_dcom;

//EDF def_dcom DCOM;                                  // debug 出力
#endif

//=====  以下は RF通信用  ================================================



typedef struct st_def_a_download{		                        // 4byte
//    int8_t group_no;				                    ///<  グループ番号
    uint8_t group_no;				                    ///<  グループ番号      // 2023.05.26
	uint8_t unit_no;				                    ///<  子機番号
	uint8_t siri[4];				                    ///<  子機のシリアル番号を保存する		子機の記録データ継続はシリアル番号の一致も確認する
	uint8_t data_no[2];			                        ///<  (前回の)最終データのデータ番号
}__attribute__ ((__packed__))   def_a_download;

typedef struct st_def_a_warning{		                        // 8byte
	uint8_t group_no;				                    ///<  グループ番号
	uint8_t unit_no;				                    ///<  子機番号
	uint8_t batt;					                    ///<  電池エラー回数
	uint8_t rfng;					                    ///<  無線エラー回数
	uint8_t ch1_counter;			                    ///<  CH1警報カウンタ
	uint8_t ch2_counter;			                    ///<  CH2警報カウンタ
	uint8_t counter;				                    ///<  警報カウンタ
	uint8_t yobi;					                    ///<  予備
}__attribute__ ((__packed__))   def_a_warning;

typedef struct st_def_w_config{		                        // 4byte
	uint8_t group_no;				                    ///<  グループ番号
	uint8_t unit_no;				                    ///<  子機番号
	uint8_t now[2];				                        ///<  今回の警報状態	[0]の部分が8bitの時の内容
	uint8_t before[2];			                        ///<  前回の警報状態	[1]の部分が16bit化の拡張部分
	uint8_t on_off[2];			                        ///<  RF通信間隔の間に警報ON→OFFしたフラグ	[1]の部分が16bit化の拡張部分	w_onoff対応
}__attribute__ ((__packed__))   def_w_config;












/// 警報監視データのバックアップ
EDF  struct def_ALM_bak{		                        // 114byte
	uint8_t group_no;				                    ///<  1 グループ番号(0～)
	uint8_t unit_no;				                    ///<  1 子機番号(1～)

	uint8_t f1_grobal_time[4];	                        ///<  4 無線通信時刻unixtime
	uint8_t f1_rf_ng;				                    ///<  1 無線通信エラー
	uint8_t f1_unit_flags;		                        ///<  1 自律動作フラグ
	uint8_t f1_unit_kind;			                    ///<  1 子機情報種類(登録ファイルの情報種類の値)
	uint8_t f1_scale_flags;		                        ///<  1 bit0:電圧、パルスのスケール変換(しない/する)=(0/1)
	uint8_t f1_ch_zoku[2];		                        ///<  2 [0]Ch1記録属性    [1]Ch2記録属性
	uint8_t f1_unit_sirial[4];	                        ///<  4 子機シリアル番号
	uint8_t f1_unit_name[8];		                    ///<  8 子機名
	uint8_t f1_yobi1[8];			                    ///<  8 予備
	uint8_t f1_yobi2;				                    ///<  1 予備
	uint8_t f1_rssi;				                    ///<  1 電波強度RSSI
	uint8_t f1_keika_time[2];		                    ///<  2 最終モニタリングからの経過秒数[秒]
	uint8_t f1_batt;				                    ///<  1 bit4-7:警報フラグ    bit0-3:電池残量
	uint8_t f1_data_number[2];	                        ///<  2 最終モニタリングデータ番号(0-0xffffでエンドレス)
	uint8_t f1_ALM_count;			                    ///<  1 ALMカウンタ(2ch機の場合、ch1+ch2)
	uint8_t f1_data_intt;			                    ///<  1 モニタデータ間隔[分]
	uint8_t f1_data_kind;			                    ///<  1 記録状態・データ種類
	uint8_t f1_data[20];			                    ///<  20 記録データ

	uint8_t f2_grobal_time[4];	                        ///<  4 無線通信時刻unixtime
	uint8_t f2_ALM_count1;		                        ///<  1 ALMカウンタ 1(or 3)
	uint8_t f2_ALM_count2;		                        ///<  1 ALMカウンタ 2(or 4)
	uint8_t f2_data[20];			                    ///<  20 警報データ

	uint8_t f3_grobal_time[4];	                        ///<  4 無線通信時刻unixtime
	uint8_t f3_ALM_count1;		                        ///<  1 ALMカウンタ 照度
	uint8_t f3_ALM_count2;		                        ///<  1 ALMカウンタ UV
	uint8_t f3_data[20];			                    ///<  20 積算警報データ

    uint8_t warntype[6];                                ///<  6 CH1～6 警報種別          // sakaguchi 2021.03.09

}__attribute__ ((__packed__))    ALM_bak[100], AlmXML;			                        //

EDF uint8_t ALM_bak_cnt;			                    ///<  ALM_bakの有効データ個数(添え字番号の個数)

//==========================================

EDF	uint8_t	TrendData[32];		                        ///<  現在値情報

///	他の中継機電波の受信履歴(RTR-50互換動作の時に使う) @note 2020.06.18 現在未使用
EDF struct def_rx_log{	                                // 20byte
	uint8_t Rep_No[10];	                                ///<  中継機番号保存
	uint8_t Rep_RF[10];	                                ///<  中継機電波強度
}rx_log;


//	上り通信時に前の中継機からの受信レベルを記憶する(リアルスキャンの応答に中継機電波強度を付加する為)
//EDF struct def_rssi{
//	uint8_t rpt;
//	uint8_t level;
//}rssi;






// これは定義
typedef struct st_def_format0{	                                        // 32byte
	uint8_t     unit_no;            				    ///<  子機番号
	uint8_t     rssi;				            	    ///<  RSSIレベル
//	uint8_t     keika_time[2];		                    ///<  現在値測定からの経過秒数
	uint8_t     rec_data_No[2];		                    ///<  記録データ番号            // 2023.05.26
	uint8_t     batt;	            				    ///<  電池残量/警報flag
	uint8_t     data_byte[2];		            	    ///<  記録データバイト(データカウント)数
	uint8_t     data_intt;          			        ///<  モニタデータ間隔
	uint8_t     warn_count;			                    ///<  警報カウント
	uint8_t     data_kind;			                    ///<  データ種類
	uint8_t     rec_intt[2];			                ///<  記録インターバル
	uint8_t     wern_time[2];			                ///<  警報持続時間
	uint8_t     ch1_lower[2];			                ///<  ch1 下限値
	uint8_t     ch1_upper[2];			                ///<  ch1 上限値
	uint8_t     ch2_lower[2];			                ///<  ch2 下限値
	uint8_t     ch2_upper[2];			                ///<  ch2 上限値
	uint8_t     unit_name[8];			                ///<  子機名(8バイト)
}__attribute__ ((__packed__))   def_format0/* def_format0*/;

typedef struct  st_def_format1{	                                    // 32byte
	uint8_t     unit_no;	            			    ///<  子機番号
	uint8_t     rssi;					                ///<  RSSIレベル
	uint8_t     keika_time[2];              	        ///<  最新データからの経過秒数
	uint8_t     batt;					                ///<  電池残量/警報flag
	uint8_t     data_number[2];		                    ///<  データ番号
//	uint8_t     data_intt;			                    ///<  モニタデータ間隔      // sakaguchi 2021.01.28 ↓に移動
	uint8_t     warn_count;			                    ///<  警報カウント
	uint8_t     data_intt;			                    ///<  モニタデータ間隔      // sakaguchi 2021.01.28
	uint8_t     data_kind;			                    ///<  データ種類
	uint8_t     data[20];				                ///<
}__attribute__ ((__packed__))   def_format1/* def_format1*/;

typedef struct st_def_format2{	                                    // 32byte
	uint8_t     unit_no;			            	    ///<  子機番号
	uint8_t     rssi;           					    ///<  RSSIレベル
	uint8_t     keika_time[2];	            	        ///<  最新データからの経過秒数
	uint8_t     batt;					                ///<  電池残量/警報flag
	uint8_t     data_number[2];         		        ///<  データ番号
	uint8_t     data_intt;			                    ///<  モニタデータ間隔
	uint8_t     warn_count;	            		        ///<  警報カウント
	uint8_t     data_kind;			                    ///<  データ種類
	uint8_t     ch1_max_data[2];            		    ///<  警報MAX値(ch1)
	uint8_t     ch1_wern_sec[2];		                ///<  最新警報からの経過秒(ch1)
	uint8_t     ch1_first_sec[2];           		    ///<  最古警報からの経過秒(ch1)
	uint8_t     ch1_data1[2];           			    ///<  記録データ1(ch1)
	uint8_t     ch1_data2[2];			                ///<  記録データ2(ch1)
	uint8_t     ch2_max_data[2];        	    	    ///<  警報MAX値(ch2)
	uint8_t     ch2_wern_sec[2];		                ///<  最新警報からの経過秒(ch2)
	uint8_t     ch2_first_sec[2];           		    ///<  最古警報からの経過秒(ch2)
	uint8_t     ch2_data1[2];			                ///<  記録データ1(ch2)
	uint8_t     ch2_data2[2];			                ///<  記録データ2(ch2)
}__attribute__ ((__packed__))   def_format2/* def_format2*/;

typedef struct st_def_a_format1{	                                    // 64byte
	uint8_t     group_no;           				    ///<  グループ番号
	uint8_t     format_no;          			        ///<  DATA FORMAT No.
	uint8_t     grobal_time[4];		                    ///<  グローバル時間
	uint8_t     rf_ng;	            			        ///<  無線通信NGの場合 0xcc
	uint8_t     yobi_0;				                    ///<  未使用
	uint8_t     wr_flags;			            	    ///<  警報フラグ bit1=ch2    bit0=ch1
	uint8_t     yobi[23];
	uint8_t     unit_no;				                ///<  子機番号
	uint8_t     rssi;					                ///<  RSSIレベル
	uint8_t     keika_time[2];		                    ///<  最新データからの経過秒数
	uint8_t     batt;					                ///<  電池残量/警報flag
	uint8_t     data_number[2];		                    ///<  データ番号(最新データの番号)
	uint8_t     ALM_count;			                    ///<  警報カウント(CH1 + CH2)
	uint8_t     data_intt;			                    ///<  モニタデータ間隔
	uint8_t     data_kind;			                    ///<  データ種類
	uint8_t     data[20];				                ///<  記録データ
	uint8_t     yobi2[2];
}__attribute__ ((__packed__))   def_a_format1/* def_a_format1*/;

typedef struct st_def_a_format2{	                                    // 64byte
	uint8_t     group_no;			    	            ///<  グループ番号
	uint8_t     format_no;			                    ///<  DATA FORMAT No.
	uint8_t     grobal_time[4];		                    ///<  グローバル時間
	uint8_t     yobi_0[4];
	uint8_t     ch_zoku[2];			                    ///<  登録ファイルから記録属性を抜き出して付加する
	uint8_t     yobi[20];
	uint8_t     unit_no;				                ///<  子機番号
	uint8_t     rssi;   					            ///<  RSSIレベル
	uint8_t     keika_time[2];  		                ///<  最新データからの経過秒数
	uint8_t     batt;			    		            ///<  電池残量/警報flag
	uint8_t     data_number[2];		                    ///<  データ番号
	uint8_t     ALM_count1;			                    ///<  警報カウント(CH1)
	uint8_t     ALM_count2;			                    ///<  警報カウント(CH2)
	uint8_t     data_kind;			                    ///<  データ種類
	uint8_t     ch1_max_data[2];		                ///<  警報MAX値(ch1)
	uint8_t     ch1_wern_sec[2];    		            ///<  最新警報からの経過秒(ch1)
	uint8_t     ch1_first_sec[2];	    	            ///<  最古警報からの経過秒(ch1)
	uint8_t     ch1_data1[2];			                ///<  記録データ1(ch1)
	uint8_t     ch1_data2[2];			                ///<  記録データ2(ch1)
	uint8_t     ch2_max_data[2];		                ///<  警報MAX値(ch2)
	uint8_t     ch2_wern_sec[2];		                ///<  最新警報からの経過秒(ch2)
	uint8_t     ch2_first_sec[2];		                ///<  最古警報からの経過秒(ch2)
	uint8_t     ch2_data1[2];			                ///<  記録データ1(ch2)
	uint8_t     ch2_data2[2];			                ///<  記録データ2(ch2)
	uint8_t     yobi2[2];
}__attribute__ ((__packed__))   def_a_format2/* def_a_format2*/;


typedef struct st_def_a_format3{    	                                // 64byte
	uint8_t     group_no;   				            ///<  グループ番号
	uint8_t     format_no;	    		                ///<  DATA FORMAT No.
	uint8_t     grobal_time[4];	    	                ///<  グローバル時間
	uint8_t     yobi_0[4];  
	uint8_t     ch_zoku[2];	    		                ///<  (登録ファイルから記録属性を抜き出して付加する)
	uint8_t     yobi[20];
	uint8_t     unit_no;            				    ///<  子機番号
	uint8_t     rssi;			            		    ///<  RSSIレベル
	uint8_t     keika_time[2];		                    ///<  最新データからの経過秒数
	uint8_t     batt;					                ///<  電池残量/警報flag
	uint8_t     data_number[2];		                    ///<  データ番号
	uint8_t     ALM_count1;			                    ///<  警報カウント(照度)
	uint8_t     ALM_count2; 			                ///<  警報カウント(UV)
	uint8_t     data_kind;	    		                ///<  データ種類
	uint8_t     ch1_max_data[2];    		            ///<  最新積算照度
	uint8_t     ch1_wern_sec[2];	    	            ///<  積算照度警報経過秒数
	uint8_t     ch1_first_sec[2];		                ///<  積算照度上限値
	uint8_t     ch1_data1[2];			                ///<  未使用
	uint8_t     ch1_data2[2];			                ///<  積算照度(新)
	uint8_t     ch2_max_data[2];		                ///<  最新積算UV量
	uint8_t     ch2_wern_sec[2];    		            ///<  積算UV量警報経過秒数
	uint8_t     ch2_first_sec[2];	    	            ///<  積算UV量上限値
	uint8_t     ch2_data1[2];			                ///<  未使用
	uint8_t     ch2_data2[2];			                ///<  積算UV量(新)
	uint8_t     yobi2[2];
}__attribute__ ((__packed__))   def_a_format3/* def_a_format3*/;


typedef struct st_def_a_rptlog{    	                                // 64byte
	uint8_t     group_no;   				            ///<  グループ番号
	uint8_t     format_no;	    		            ///<  未使用
	uint8_t     grobal_time[4];	    	            ///<  未使用
	uint8_t     yobi_0[4];							///<  未使用
	uint8_t     ch_zoku[2];	    		            ///<  未使用
	uint8_t     yobi[20];								///<  未使用
	uint8_t     unit_no;            				    ///<  中継機番号
	uint8_t     rssi;			            		    ///<  RSSIレベル
	uint8_t     check_sum[2];		                    ///<  受信履歴のチェックサム
	uint8_t     batt;					                ///<  電池残量
	uint8_t     unit_cnt;		                    	///<  子機台数
	uint8_t     rpt_data[24];							///<  子機番号とRSSI
}__attribute__ ((__packed__))   def_a_rptlog/* def_a_rptlog*/;




uint16_t CertFileSize_WS;   // 未使用
uint16_t CertFileSize_USER; // 未使用
uint16_t CertFileSize_Temp;












// ↓↓↓↓↓ segi ------------------------------------------------------------------------------------

EDF	uint32_t ALM_bak_utc;
EDF	uint32_t DL_start_utc;

EDF char End_Rpt_No;					// 自律リアルスキャンの末端中継機番号を記憶
EDF char max_rpt_no;					// 中継機の最大番号を把握。get_regist_scan_unit()で設定する



EDF uint8_t EWSTW_Data[64];		// コマンド入力されたDATA			グローバルに移動
EDF uint8_t EWSTW_Format;		// コマンド入力されたFORMAT			グローバルに移動
EDF uint8_t EWSTW_GNo;			// コマンドから導いたGNo			グローバルに移動
//EDF uint8_t EWSTW_UNo;			// コマンドから導いたUNo			グローバルに移動
EDF uint32_t EWSTW_Ser;			// コマンドから導いたシリアルNo		グローバルに移動
EDF uint32_t EWSTW_Adr;			// FLASHに書き込むアドレス
EDF uint8_t Regist_Data[96+64];	// FLASHに書き込むデータ			グローバルに移動
EDF uint8_t NTH_2in1;
EDF uint8_t EWSTR_time;			// EWSTWコマンド実行後にEWSTRをやるタイマ

EDF int ReqOpe_time;			// 操作リクエスト用タイマ（秒）
EDF uint32_t RfMonit_time;      //フルモニタリング用カウンタ 24h
EDF uint8_t FirstAlarmMoni;		// 警報監視１回目フラグ     1:１回目 0:通常     // sakaguchi 2021.03.01

EDF uint8_t RF_Err_cnt;			// 無線通信が失敗した回数をカウントする(繰り返し失敗の場合にモジュール再起動する)

EDF uint8_t EWAIT_res;			// EWAITコマンドの状態レスポンス用　0:何もしていない	1:モニタリング	2:吸い上げ	3:WSSコマンド実行中
EDF uint32_t EWSTW_utc;			// [EWSTW]コマンドでUTCパラが含まれていた場合にUTCを返す変数。返信不要の時は0を入れる。



	// 自動リアルスキャンの結果から1子機分データを抜き出す変数
	EDF union def_get_a_format
	{
		def_a_format1 format1;
		def_a_format2 format2;
	}get_a_format;

#pragma pack(1)
	EDF  union def_rf_ram
	{
		//---------------------------------------------------------
		struct def_realscan{			// ########## 自律リアルスキャンデータを保存する場所 ##################################
			uint8_t data_byte[2];						// 子機データのバイト数
			union{
				def_format0 format0[128];	// リアルスキャン取得データ
				def_format1 format1[128];	// リアルスキャン取得データ
				def_format2 format2[128];	// リアルスキャン取得データ
			}data;
		}realscan;
		//---------------------------------------------------------
		struct def_auto_format1{		// ########## 現在値(モニタリング)で使うデータ ########################################
			uint8_t dummy[4096];
			uint8_t data_byte[2];					// AUTOリアルスキャンバイト数
			def_a_format1 kdata[128];	// AUTOリアルスキャン取得データ(64byte x 128)
		}auto_format1;
		//---------------------------------------------------------
		struct def_auto_format2{		// ########## 警報メールなどで使うデータ ##############################################
			uint8_t dummy[12290];
			uint8_t data_byte[2];					// AUTOリアルスキャンバイト数
			def_a_format2 kdata[128];	// AUTOリアルスキャン取得データ(64byte x 128)
		}auto_format2;
		//---------------------------------------------------------
		struct def_auto_format3{		// ########## 積算警報メールなどで使うデータ ##############################################
			uint8_t dummy[20484];
			uint8_t data_byte[2];					// AUTOリアルスキャンバイト数
			def_a_format3 kdata[128];	// AUTOリアルスキャン取得データ(64byte x 128)
		}auto_format3;
		//---------------------------------------------------------
		struct def_auto_rpt_info{		// ########## 中継機スキャン情報の保存データ ##############################################
			uint8_t dummy[28678];
			uint8_t data_byte[2];					// AUTOリアルスキャンバイト数
			def_a_rptlog kdata[128];	// AUTOリアルスキャン取得データ(64byte x 128)
		}auto_rpt_info;
		//---------------------------------------------------------
		struct def_auto_download{		// ########## 自動吸上げで使うデータ(データ本体はsram.adr[0]～参照) ###################
			uint8_t dummy[32128];
			uint8_t dummy2[250];
			uint8_t group_no;						// AUTOデータ吸上げグループ番号
			uint8_t unit_no;						// 子機番号
			uint8_t start_time[8];				// データ吸い上げ日時(GMT)
		}auto_download;
		//---------------------------------------------------------
		uint8_t adr[32378];	// 128 + 32000 + 250 byte					// xxx04 2009/12/04 RTR-574対応の為増やした
		//---------------------------------------------------------
		struct def_rpt_buff{
			uint8_t d[32128];				// ダミー名						// xxx04 2009/12/04 RTR-574対応の為増やした
			uint8_t adr[250];				// 中継データのバックアップ用
		}rpt_buff;
		//---------------------------------------------------------
		struct def_header{
			uint8_t intt[2];				// 記録インターバル
			uint8_t start_time[8];			// 記録開始日時(GMT)
			uint8_t start_mode;				// 記録開始方法
			uint8_t rec_mode;				// 記録モード
			uint8_t wait_time[4];			// 最終記録からの秒数
			uint8_t disp_unit;				// LCD表示単位
			uint8_t rec_protect;			// 記録開始プロテクト
			uint8_t data_intt;				// モニタテータ間隔[分]
			uint8_t ch1_comp_time;			// CH1(CH3)警報判定時間
			uint8_t ch1_lower[2];			// CH1(CH3)下限値
			uint8_t ch1_upper[2];			// CH1(CH3)上限値
			uint8_t ch2_comp_time;			// CH2(CH4)警報判定時間
			uint8_t ch2_lower[2];			// CH2(CH4)下限値
			uint8_t ch2_upper[2];			// CH2(CH4)上限値
			uint8_t aki1;					// 
			uint8_t data_byte[2];			// 吸上げデータバイト数
			uint8_t all_data_byte[2];		// 全記録データバイト数
			uint8_t end_no[2];				// 最終記録データ番号
			uint8_t serial_no[4];			// 子機のシリアル番号
			uint8_t batt;					// 電池残量
			uint8_t aki2;					// 
			uint8_t ch_zoku[2];				// CH1(CH3)、CH2(CH4)記録属性
			uint8_t group_id[8];			// グループ名(8バイト)
			uint8_t unit_name[8];			// 子機名(8バイト)
			uint8_t unit_no;				// 子機番号
			uint8_t freq_ch;				// 周波数チャンネル
			uint8_t kisyu_code;				// 機種コード
			uint8_t ul_protect;				// 上下限書き換えプロテクト
			uint8_t data[32064];			// ロギングデータ				// xxx04 2009/12/04 RTR-574対応の為増やした
		}header;
		//---------------------------------------------------------
		struct def_header574{
			uint8_t intt[2];				// 記録インターバル
			uint8_t start_time[8];			// 記録開始日時(GMT)
			uint8_t start_mode;				// 記録開始方法
			uint8_t rec_mode;				// 記録モード
			uint8_t wait_time[4];			// 最終記録からの秒数
			uint8_t disp_unit;				// LCD表示単位
			uint8_t rec_protect;			// 記録開始プロテクト
			uint8_t data_intt;				// モニタテータ間隔[分]
			uint8_t ch1_comp_time;			// CH1(CH3)警報判定時間
			uint8_t ch1_lower[2];			// CH1(CH3)下限値
			uint8_t ch1_upper[2];			// CH1(CH3)上限値
			uint8_t ch2_comp_time;			// CH2(CH4)警報判定時間
			uint8_t ch2_lower[2];			// CH2(CH4)下限値
			uint8_t ch2_upper[2];			// CH2(CH4)上限値
			uint8_t aki1;					// 
			uint8_t data_byte[2];			// 吸上げデータバイト数
			uint8_t all_data_byte[2];		// 全記録データバイト数
			uint8_t end_no[2];				// 最終記録データ番号
			uint8_t serial_no[4];			// 子機のシリアル番号
			uint8_t batt;					// 電池残量
			uint8_t aki2;					// 
			uint8_t ch_zoku[2];				// CH1(CH3)、CH2(CH4)記録属性
			uint8_t group_id[8];			// グループ名(8バイト)
			uint8_t unit_name[8];			// 子機名(8バイト)
			uint8_t unit_no;				// 子機番号
			uint8_t freq_ch;				// 周波数チャンネル
			uint8_t kisyu_code;				// 機種コード
			uint8_t ul_protect;				// bit0-3:無線通信(停止/動作)=(0/1)  bit4-7:追加テーブル数

			uint8_t disp_select;			// LCD表示切替設定
			uint8_t disp_item;				// LCD切り替え表示時の表示項目
			uint8_t key_lock;				// スイッチロック設定
			uint8_t ch1_name[16];			// CH1(CH3)チャンネル名
			uint8_t ch2_name[16];			// CH2(CH4)チャンネル名
			uint8_t RF_enable;				// 無線通信(停止/動作)=(0/1)
			uint8_t IR_enable;				// 赤外線通信(停止/動作)=(0/1)
			uint8_t calib_date[4];			// CH1/CH2(CH3/CH4)センサキャリブレーション日時
			uint8_t calib_ch1_a[2];			// CH1(CH3)センサキャリブレーション傾き(返り値=a×生データ + b)
			uint8_t calib_ch1_b[2];			// CH1(CH3)センサキャリブレーション切片(返り値=a×生データ + b)
			uint8_t calib_ch2_a[2];			// CH2(CH4)センサキャリブレーション傾き(返り値=a×生データ + b)
			uint8_t calib_ch2_b[2];			// CH2(CH4)センサキャリブレーション切片(返り値=a×生データ + b)
			uint8_t ch1_mp_upper[2];		// CH1積算警報上限値
			uint8_t ch2_mp_upper[2];		// CH2積算警報上限値
			uint8_t ch1_mp[2];				// CH1積算値()
			uint8_t ch2_mp[2];				// CH2積算値()
			uint8_t aki3[7];				// 
			uint8_t data[32000];			// ロギングデータ				// xxx04 2009/12/04 RTR-574対応の為増やした
		}header574;
		//---------------------------------------------------------

	}rf_ram		__attribute__((section(".xram")));


	EDF  union def_rf_ram2
	{
		//---------------------------------------------------------
		struct def_realscan realscan;			// ########## 自律リアルスキャンデータを保存する場所 ##################################
		//---------------------------------------------------------
		struct def_auto_format1 auto_format1;	// ########## 現在値(モニタリング)で使うデータ ########################################
		//---------------------------------------------------------
		struct def_auto_format2 auto_format2;	// ########## 警報メールなどで使うデータ ##############################################
		//---------------------------------------------------------
		struct def_auto_download auto_download;	// ########## 自動吸上げで使うデータ(データ本体はsram.adr[0]～参照) ###################
		//---------------------------------------------------------
		uint8_t adr[32378];	// 128 + 32000 + 250 byte					// xxx04 2009/12/04 RTR-574対応の為増やした
		//---------------------------------------------------------
		struct def_rpt_buff rpt_buff;
		//---------------------------------------------------------
		struct def_header header;
		//---------------------------------------------------------
		struct def_header574 header574;
		//---------------------------------------------------------
	}rf_ram2		__attribute__((section(".xram")));





// 自動吸い上げデータ受け渡し用変数 ワードアクセスがあるため、外部RAMには置けない！！
	EDF union def_Download{
		struct def_Attach{
			uint8_t group_no;			//	1	子機登録ファイル上のグループ番号
			uint8_t unit_no;			//	1	子機登録ファイル上の子機番号
			uint32_t GMT;				//	4	無線通信開始時刻
			uint32_t serial_no;		//	4	シリアル番号
			char group_id[8];		//	8	グループID
			char unit_name[8];		//	8	子機名
			uint8_t kisyu_code;		//	1	機種コード
			uint8_t yobi1;			//	1	予備
			uint16_t intt;				//	2	記録間隔[秒]
			uint32_t start_time[2];	//	8	記録開始日時(子機保存値)
			uint32_t wait_time;		//	4	最終記録からの経過秒数
			uint16_t end_no;			//	2	最終記録のデータ番号
			uint8_t zoku[4];			//	4	記録属性	[0]ch1,[1]ch2,[2]ch3,[3]ch4
			uint16_t multiply[4];		//	8	積算値		[0]ch1,[1]ch2,[2]ch3,[3]ch4
			char *data_poi[2];		//	8	記録データ本体先頭ポインタ [0]ch1_ch2,[1]ch3_ch4

			uint16_t data_byte[2];		//	4	記録データバイト数	[0]ch1&ch2,[1]ch3&ch4

			uint16_t lower_limit[4];	//	8	下限値		[0]ch1,[1]ch2,[2]ch3,[3]ch4
			uint16_t upper_limit[4];	//	8	上限値		[0]ch1,[1]ch2,[2]ch3,[3]ch4
			uint16_t mp_limit[4];		//	8	積算上限	[0]ch1,[1]ch2,[2]ch3,[3]ch4

//			uint8_t ud_flags;			//	1	上下限値(有効/無効)フラグ
			struct {
				uint8_t lo_1:1;
				uint8_t up_1:1;
				uint8_t lo_2:1;
				uint8_t up_2:1;
				uint8_t lo_3:1;
				uint8_t up_3:1;
				uint8_t lo_4:1;
				uint8_t up_4:1;
			}ud_flags;


			uint8_t mp_flags;			//	1	積算上限値(有効/無効)フラグ

			uint8_t rec_sett;			//	1	記録方式(瞬時/平均、測定間隔)
			uint8_t sett_flags;		//	1	機能設定フラグ				// xxx28 2011.02.28追加
			char scale_conv[64];	//	64	スケール・単位変換情報		// xxx28 2011.02.28追加

			uint16_t header_size[2];	//	4	記録データのヘッダーサイズ(バイト数)	[0]ch1&ch2,[1]ch3&ch4
			uint32_t total_pulse[2];	//	8	積算パルス  [0]ch1     [1]ch2

			uint8_t extension[4];		//	4	RTR-601自動吸い上げファイルの拡張子	2013/04/23追加 
			//uint32_t time_diff;		//	4	RTR-601の時差情報	2013/04/26追加
			int32_t time_diff;		//	4	RTR-601の時差情報	2021.12.22

		}Attach;

		uint8_t adr[159];
	}Download;	/* 		__attribute__((section(".xram")));*/



#pragma pack(4)

//////////////////////////////////////////
//	無線自動ルート作成で使用する部分
//////////////////////////////////////////

   /// @brief  使用する子機の台数（子機番号の最大値）
    #define REMOTE_UNIT_MAX 100
    #define REMOTE_UNIT_ALL REMOTE_UNIT_MAX+1               // 全子機指定用

    /// @brief  使用する中継機の台数（中継機番号の最大値）
    /// @note   内部でint8_tで保持するので127まで可
    #define REPEATER_MAX 10
    #define REPEATER_ALL REPEATER_MAX+1                     // 全中継機指定用

    /// @brief 使用するグループの数（グループ番号の最大値）
    #define GROUP_MAX 4
    #define GROUP_ALL GROUP_MAX+1                           // 全グループ指定用

    /// @brief  電波強度が不定
    #define RSSI_VOID 0
    /// @brief  無線通信不可
    #define RSSI_NONE 1


    /// @brief  スキャンリストの最大登録数
    #define SCANLIST_MAX REPEATER_MAX


    /// @brief  無線ルートを保存するため配列
    typedef struct {

        /// @brief  無線ルート（親番号の配列）
        /// @note   親機→中継機3→中継機1の場合、[0, 3, 1, 0, 0, 0, .... ]
        uint8_t value[REPEATER_MAX + 1];

        /// @brief  無線ルートの段数
        /// @note   親機→中継機3→中継機1の場合、3
        uint8_t count;

    } RouteArray_t;


    /// @brief  親機番号のリストをやりとりするための配列
    ///         中継機番号の最大値分の要素があれば足りる
    /// @note   value   親機番号のリスト（count以降の値は不定）
    ///         count   親機番号のリストで有効な値の数
    typedef RouteArray_t ParentArray_t;


    /// @brief  親機（中継機）－中継機間の電波強度テーブル
    ///
    /// @note   電波の往路復路を区別しません（中1→中2, 中2→中1 ともに[中1→中2]と解釈する）
    ///
    //  例）中継機の最大が5台の場合
    //      親 １   ２   ３   ４   ５  
    //  親  － ○ 0 ○ 1 ○ 2 ○ 3 ○ 4
    //  1   － － 5 ○ 6 ○ 7 ○ 8 ○ 9
    //  2   － －10 －11 ○12 ○13 ○14
    //  3   － －15 －16 －17 ○18 ○19
    //  4   － －20 －21 －22 －23 ○24
    //  5   － －   －   －   －   －
    //
    typedef struct {
        uint8_t value[REPEATER_MAX * REPEATER_MAX];
    } RepeaterRssiTable_t;


    /// @brief  親機（中継機）－子機間の電波強度リスト
    typedef struct {
        
        //  例）中継機の最大が5台の場合
        //      親 １   ２   ３   ４   ５  
        uint8_t value[REPEATER_MAX + 1];

        //valueが有効か判断するフラグ
        uint8_t enable;

    } RemoteUnitRssiArray_t;


    /// @brief  親機（中継機）－子機間の電波強度テーブル
    typedef struct {
        RemoteUnitRssiArray_t units[REMOTE_UNIT_MAX];
    } RemoteUnitRssiTable_t;


    /// @brief  中継機の親番号テーブル
    /// インデックス
    ///     0 : 未使用
    ///     1 : 中継機番号1
    ///     2 : 中継機番号2
    ///             ：
    ///             ：
    ///
    /// 値
    ///    -1 : 親なし
    ///     0 : 親機
    ///     1 : 中継機番号1
    ///     2 : 中継機番号2
    ///             ：
    ///             ：
    ///   127 : 中継機番号127
    ///
    /// @note 中継機番号で配列にアクセスできるように先頭に未使用領域があります。
    typedef struct {
        int8_t value[REPEATER_MAX + 1];
    } RepeaterParentArray_t;


    /// @brief  子機の親番号テーブル
    /// インデックス
    ///     0 : 未使用
    ///     1 : 子機番号1
    ///     2 : 子機番号2
    ///             ：
    ///             ：
    ///
    /// 値
    ///    -1 : 親なし
    ///     0 : 親機
    ///     1 : 中継機番号1
    ///     2 : 中継機番号2
    ///             ：
    ///             ：
    ///   127 : 中継機番号127
    ///
    /// @note 中継機番号で配列にアクセスできるように先頭に未使用領域があります。
    typedef struct {
        int8_t value[REMOTE_UNIT_MAX + 1];
    } RemoteUnitParentArray_t;




    /// @brief  無線グループ
    typedef struct {

        /// @brief  すべてのグループで共有する子機の電波強度テーブル
        /// @note   すべてのグループで共有するのでstaticです。
	//	static RemoteUnitRssiTable_t *allRemoteUnitRssiTable;
        RemoteUnitRssiTable_t *allRemoteUnitRssiTable;

        /// @brief  中継機の電波強度テーブル
        /// @note   無線グループ毎に独立しています。
        RepeaterRssiTable_t repeaterRssiTable;

        /// @brief  子機の電波強度テーブル
        /// @note   remoteUnitRssiTable[0] : 未使用
        ///         remoteUnitRssiTable[1] : 子機番号1の電波強度リスト
        ///         remoteUnitRssiTable[2] : 子機番号2の電波強度リスト
        ///                 :
        ///
        /// Nullポインタなら、この無線グループで未使用の子機です。        
        RemoteUnitRssiArray_t* remoteUnitRssiTable[REMOTE_UNIT_MAX + 1];

        /// @brief  中継機の親番号リスト
        RepeaterParentArray_t repeaterParents;

        /// @brief  子機の親番号リスト
        RemoteUnitParentArray_t remoteUnitParents;

        /// @brief  最大子機番号
        uint8_t max_unit_no;
        /// @brief  最大中継機番号
        uint8_t max_rpt_no;

        /// @brief  子機電池残量
        uint8_t unit_battery[REMOTE_UNIT_MAX + 1];
        /// @brief  親機中継機電池残量
        uint8_t rpt_battery[REPEATER_MAX + 1];

        /// @brief  モニタリングした時刻(UTIM)
        uint32_t moni_time;

    } WirelessGroup_t;


	WirelessGroup_t RF_Group[4];



	uint8_t realscan_order[12];			// 自動ルート作成フルスキャンのやるべきテーブル
	uint8_t realscan_cnt;				// 自動ルート作成フルスキャンのやるべきテーブルの位置



    /// @note 親番号リストの履歴
    typedef struct {
        /// @brief  中継機の親番号リスト
        RepeaterParentArray_t repeaterParents;
        /// @brief  子機の親番号リスト
        RemoteUnitParentArray_t remoteUnitParents;
    } OYA_No_log_t;

    OYA_No_log_t RF_OYALOG_Group[4][6];     //親番号リストの履歴[グループ番号-1][履歴] 履歴：[0]今回、[1]～[4]1～4回前、[5]最後に通信できた親番号

    typedef struct{
        uint8_t repno;
        uint8_t rssi;
        uint8_t batteryLevel;
    } ScanRelayStatus_t;

	uint8_t moni_err[4];			// 0以外:直近のモニタリングで取得できない子機があった。[グループNo]
	uint8_t download_err[4];		// 0以外:直近のデータ吸い上げで取得できない子機あった。[グループNo]

    uint8_t NOISE_CH[GROUP_MAX];        // CH-BUSYフラグ[グループNo] // sakaguchi 2021.01.25

	uint8_t chk_download_null;












// ↑↑↑↑↑ segi ------------------------------------------------------------------------------------


//=============  FTP =================================

EDF	struct {

	char	PORT[2];				// ポート番号
	char	__gap0__[2];
	char	SRV[64];				// FTP Server Name
	char	__gap1__[2];
	char	ID[64];					// User ID
	char	__gap2__[2];
	char	PW[64];					// Password
	char	__gap3__[2];
	char	PATH[256];				// Directory
	char	__gap4__[2];
	char	FNAME[256];				// FileName(エンコード後)
	char	__gap5__[2];

} FtpHeader __attribute__((section(".xram")));

//=============  Email =================================

typedef	struct st_def_EmHeader {

    char	Sbj[190];				// SBJ[96]; Subject
	char	__gap2[2];

	char	TOA1[64+2];			// To Address ( ***@aaa.com )
	char	TOA2[64+2];			// To Address ( ***@aaa.com )
	char	TOA3[64+2];			// To Address ( ***@aaa.com )
	char	TOA4[64+2];			// To Address ( ***@aaa.com )

	//uint8_t	POP3[64];				// POP3サーバ名
	//uint8_t	__gap21[2];
	//uint8_t	MID[64];				// POP3 ID
	//uint8_t	__gap22[2];
	//uint8_t	MPW[64];				// POP3 PW
	//uint8_t	__gap23[2];

	char	SRV[64];				// SMTPサーバ名
	char	__gap24[2];
	char	ID[64];					// SMTP認証 ID
	char	__gap25[2];
	char	PW[64];					// SMTP認証 PW
	char	__gap26[2];

	char	TO[96];					// To Desc (address )
	char	__gap7[2];
	char	From[64];				// Return(FROM) Address ( ***@bbb.com )
	char	__gap8[2];
	char	Sender[96];				// Sender(FROM) Desc ( t_and_d )
	char	__gap9[2];
	char	ATA[190];				// ATA[64]; // Attach file name
	char	__gap10[2];
	char	BDY[2400UL];			// Body ※iChipレジスタは96byteだが
	char	__gap11[2];
	char	ENCBDY[2400UL];			// Body ※iChipレジスタは96byteだが
	char	__gap12[2];

} def_EmHeader;  //EmHeader __attribute__((section(".xram")));

extern def_EmHeader EmHeader;

//============ Warning XML用 構造体
typedef  struct def_Warn {
	char		Ch_Name[16];
	char		Val[16];
//	char		Unit[16];
//	char		Unit[23];               // sakaguchi 2021.01.18 サイズ拡張 505BP スケール変換単位(max16文字)+/記録間隔(max6文字)
	char		Unit[71];               // sakaguchi 2021.04.16 スケール変換単位(全角max16文字)+/記録間隔(max6文字)
	char		U_limit[16];
	char		L_limit[16];
    char        Judge_time[8];

	uint32_t 	utime;
	uint32_t	id;
	uint32_t	comm;
	uint32_t	utime_c;
    uint8_t     limit_flag;         // 下位２ビットを使用
    uint8_t     batt;

    uint32_t    Val_raw;            // 警報値（素データ）    // sakaguchi 2021.03.01
    uint32_t    U_limit_raw;        // 上限値（素データ）    // sakaguchi 2021.03.01
    uint32_t    L_limit_raw;        // 下限値（素データ）    // sakaguchi 2021.03.01
    uint16_t    AttribCode;         // データ種別           // sakaguchi 2021.03.01
    uint8_t     warnType;           // 警報種別             // sakaguchi 2021.03.01
    uint8_t     edge;               // 立上り・立下り       // sakaguchi 2021.04.16
} __attribute__ ((__packed__)) def_Warn;

extern def_Warn WarnVal;

// 仮想グループ
typedef struct def_VGoup 
{
	uint32_t serial;		// 検索対象のシリアル番号
	char     Name[65];     	// 仮想グループ名	無し場合は、Null 
    //--------------------------------------------
    char    code[4];        // VGRP固定のはず
    uint8_t ver;
    uint8_t g_num;
    char    gap[10];        // ここまで16byte

    char    data[1001];     // max 1000 byte

} __attribute__ ((__packed__))   def_VGroup;

extern def_VGroup VGroup;

///WiFiスキャン時の RSSSI,SSID構造体定義
typedef struct def_wifi_scan
{
    uint8_t                    rssi;                          // Signal Strength
	char					 sec;							// security type
    char                     ssid[32 + 1]; 					// SSID name
} def_wifi_scan_t;

//WiFiスキャン時の RSSSI,SSID構造体
extern def_wifi_scan_t AP_List[16];


//============ HTTP通信用 構造体
typedef  struct def_http_file_snd {
	uint8_t		sndflg;
	uint32_t 	reqid;
} def_http_file_snd_t;	//Warn __attribute__((section(".xram")));

typedef  struct def_http_cmd_snd {
    uint8_t     sndflg;     // 機器設定送信
    uint8_t     ansflg;     // 操作リクエスト応答送信
    uint32_t    reqid;      // リクエストID
    uint32_t    save;       // 一時格納用
    uint16_t    offset;     // オフセット
    uint16_t    size;       // サイズ
    ULONG       status;     // 実行結果
} def_http_cmd_snd_t;	//Warn __attribute__((section(".xram")));

extern def_http_file_snd_t G_HttpFile[10];
extern def_http_cmd_snd_t G_HttpCmd[128];



// 以下は CS4の外部アドレス
// XRAM CY62148  0x80000    512K
// 外部RAMは、バイトアクセス ！！
//####  ここは外部RAM #####################################################
//####  ここは外部RAM #####################################################
//####  ここは外部RAM #####################################################

//#pragma pack(1)

extern char    BackUpTest[100];

//=============  XML ====================================

extern char    XMLBuff[ 1500 ];     ///<  XML送信用
extern char    XMLTemp[ 300 ];         ///<  XMLテンポラリ
extern char    XMLTForm[ 100 ];     ///<  XML作成
extern char    TagTemp[ 200 ];         ///<  XMLタグ作成用
extern char    XMLName[ 300 ];         ///<  XMLファイル名
extern char    EntityTemp[ 200 ];     ///<  XML実体作成用


// ＃実態＃
//EDF def_RFB RFB __attribute__((section(".xram")));     // 101382 byte

///作業用バッファ
extern char huge_buffer[32768];
///作業用バッファ
extern char work_buffer[32768];
///XML作業用バッファ
extern char xml_buffer[32768*4];
//EDF uint8_t xml_buffer[4096]; //  __attribute__((section(".xram")));

//sakaguchi ↓
//HTTP作業用バッファ
extern char http_buffer[51200];
//sakaguchi ↑

///証明書作業用バッファ
extern char CertFile_WS[4096];
///証明書作業用バッファ
extern char CertFile_USER[4096];
//
extern char Group_Buffer[1024];


//EDF uint8_t log_test[1024]  __attribute__((section(".xram")));


//####  ここは外部RAM #####################################################
//####  ここは外部RAM #####################################################


//EDF uint8_t XramTest[1024*1] __attribute__((section(".xram")));

//#pragma pack(4)

//-----------------------------------------------------------------
//-----------------------------------------------------------------
//-----------------------------------------------------------------







EDF uint16_t AP_Count;	





//####################################################################################################################
// ここからは、バックアップＲＡＭ   ８ＫＢ  外RAMに変更

//###################################################################################################################
// ＃実態＃ これはバックアップが必要
/**
 * 自律動作の前回の情報(警報)記憶用変数
 * @note    パディングあり
 */
typedef struct /*def_auto_control*/{					        // 3072byte
    	def_a_download   download[128];  		// 自動吸上げの前回データ番号
	    def_a_warning    warning[128];			// 警報判定の為の前回情報
	    def_a_warning    warning_back[128];		// 警報判定の為の前回情報(GSM通信エラー時に元に戻す為のバックアップ)
	    def_w_config     w_config[128];			// 子機の前回と今回の警報状態フラグ
	    def_w_config     w_config_back[128];		// 子機の前回と今回の警報状態フラグ(GSM通信エラー時に元に戻す為のバックアップ)
	    uint8_t m_batt;					    			// 親機の電池警報カウンタ
		uint8_t warcnt;					    			// 警報カウンタ
		uint32_t crc;									// 電源断時、rf処理終了時このエリアのCRCを生成		

		uint16_t backup_cur;
		uint16_t backup_now;
		uint16_t backup_warn;
		uint16_t backup_sum;

} def_auto_control;

EDF def_auto_control auto_control		__attribute__((section(".xram")));;
/*
typedef struct {

	uint32_t	Size;
	uint32_t	pStart;			// 開始ポインタ(最古データへのポインタ)
	uint32_t	pWrite;			// 次に書き込むポインタ
	uint16_t	Count;			// ログ数
	uint16_t	Number;			// シリアル番号（次に書かれる番号）
//	uint8_t	*Buff;			// バッファ開始アドレス
//	uint8_t	Data[256];		// 

} LOG_CTRL;
*/

// 72 byte
/*
typedef struct {	

	char		MagicCode[32];		// マジックコード
	char		VersionCode[16];	// ファームバージョン
	//LOG_CTRL	Control;			// 制御構造体
	//uint16_t	LogSum;				// LOG_CTRLのチェックサム
	uint32_t	LogSize;			// ログのサイズ
	uint32_t	LogAdrs;			// ログ先頭アドレス

	uint32_t	Size;
	uint32_t	pStart;				// 開始ポインタ(最古データへのポインタ)
	uint32_t	pWrite;				// 次に書き込むポインタ
	uint16_t	Count;				// ログ数
	uint16_t	Number;				// シリアル番号（次に書かれる番号


} def_LogInfo;
*/

//EDF def_LogInfo LogInfo		__attribute__((section(".xram")));	//			__attribute__((section(".bram")));
//EDF uint16_t	LogSum		__attribute__((section(".xram")));	// 			__attribute__((section(".bram")));				// チェックサム













//#####################################################################################################################



/**
 * SOHフレームの型定義
 */
typedef struct {
    char     header;
    char     command;
    char     subcommand;
    uint16_t    length;
    //char     data[2048];
    char     data[4096];        // 2048->4096に拡張     // sakaguchi 2021.06.15
}__attribute__ ((__packed__)) comSOH_t;

/**
 * T2フレームの型定義
 */
typedef struct {
    char     command;       //T
    char     subcommand;    //2
    uint16_t    length;
    char     data[8192-4];

}__attribute__ ((__packed__)) comT2_t;










// 関数定義

EDF void bsp_exbus_init(void);              // SRAM Setting

#endif /* GLOBALS_H_ */
