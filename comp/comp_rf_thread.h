/*
 * comp_rf_thread.h
 *
 *  Created on: Dec 2, 2024
 *      Author: karel
 */

#ifndef COMP_RF_THREAD_H_
#define COMP_RF_THREAD_H_


#include "_r500_config.h"
#include <stdint.h>

#ifdef EDF
#undef EDF
#endif

#ifndef _COMP_RF_THREAD_C_
    #define EDF extern
#else
    #define EDF
#endif

// 警報bitの定義
#define WR_OYA_INPUT    0x0080  // 親 ---- 外部接点入力
#define WR_BATT         0x0040  // 親,子 - 電池少ない
#define WR_UNIT_RFNG    0x0020  // 子 ---- 無線通信エラー
#define WR_UNIT_SENS    0x0010  // 子 ---- センサー異常（CH1,CH2）
#define WR_OYA_AC       0x0008  // 親 ---- 外部電源抜きエラー
//#define WR_UNIT_xx      0xCC04                              // 予備の領域
#define WR_UNIT_xx      0xC804                              // 予備の領域   // センサー異常（CH3,CH4）を予備から除外 sakaguchi 2021.04.13
#define WR_UNIT_CH2     0x0002  // 子 ---- ch1上下限警報
#define WR_UNIT_CH1     0x0001  // 子 ---- ch2上下限警報

#define WR_UNIT_SENS2   0x0400  // 子 ---- センサー異常（CH3,CH4）  // sakaguchi 2021.04.07

#define WR_UNIT_CH4     0x0200  // 子 ---- ch4上下限警報
#define WR_UNIT_CH3     0x0100  // 子 ---- ch3上下限警報

#define WR_UNIT_MP2     0x2000  // 子 ---- ch2積算警報
#define WR_UNIT_MP1     0x1000  // 子 ---- ch1積算警報

//ポート定義
/* R500Sポート定義
 * 8    CS   P804(OD out)   (RFM内部47Kプルアップ)
 * 7    CK   nouse   (RFM内部47Kプルアップ)
 * 6    RxD  SCI5 TxD_MOSI  (P501)   (RFM内部47Kプルアップ)
 * 5    TxD  SCI5 RxD_MISO  (P502)
 * 4    Busy P511 (Input)
 * 3    Status/Reset   P500(OD out)
 * 2    3V
 * 1    GND
 */
#if 0 //TODO
#define RFM_RESET_PORT        IOPORT_PORT_05_PIN_00               ///< (out) RF Reset status
#define RFM_CS_PORT           IOPORT_PORT_08_PIN_04               ///< (out) RF CS
#define RFM_BUSY_PORT         IOPORT_PORT_05_PIN_11               ///< (in ) RF BUSY  Low:busy

#define RF_CS_ACTIVE()        g_ioport.p_api->pinWrite(RFM_CS_PORT, IOPORT_LEVEL_LOW)
#define RF_CS_INACTIVE()      g_ioport.p_api->pinWrite(RFM_CS_PORT, IOPORT_LEVEL_HIGH)
#define RF_RESET_ACTIVE()     g_ioport.p_api->pinWrite(RFM_RESET_PORT, IOPORT_LEVEL_LOW)
#define RF_RESET_INACTIVE()   g_ioport.p_api->pinWrite(RFM_RESET_PORT, IOPORT_LEVEL_HIGH)
#else
#define RF_CS_ACTIVE()        //TODO
#define RF_CS_INACTIVE()      //TODO
#define RF_RESET_ACTIVE()     //TODO
#define RF_RESET_INACTIVE()   //TODO
#endif // TODO

//Globals.hから移動
#define NORMAL_END      0x00            // 正常終了
#define ERROR_END       0xf0            // エラー終了
#define AUTO_END        0xe0            // 現在指定されているグループではリアルスキャン対象がなくなった。
#define RF_CANCEL       0xe1
#define NOT_UNIT        0xe2


//関数プロトタイプ

EDF uint8_t RF_command_execute(uint8_t);                            // ＲＦコマンド実行

//----- segi ------------------------------------------------------------------------------------
//EDF uint32_t AUTO_control(uint32_t actual_events);
EDF uint16_t Warning_Check(void);
/*
EDF uint32_t RF_event_execute(uint8_t);                            // ＲＦコマンド実行
EDF uint8_t RF_monitor_execute(uint8_t order);        //auto_thread_entry.c
*/
EDF void RF_power_on_init(uint8_t sw);
/*
EDF void WR_clr(void);
EDF void set_flag(char *poi , uint8_t No);
EDF uint8_t check_flag(char *poi , uint8_t No);
EDF void copy_realscan(uint8_t format_no);
*/
//----- segi ------------------------------------------------------------------------------------
EDF uint8_t rpt_cnt_search(uint8_t);                                // 自局の中継機番号が中継機テーブルの先頭から何番目にいるか返す
//EDF char regu_moni_scan(uint8_t data_format);       //Rfunc.cに移動2020.06.18
EDF uint32_t R500C_Direct(int mode, int time, char *pData);

EDF int Chreck_RFM_Busy(void);      //RFMのビジーチェック
EDF int Chreck_RFM_Status(void);    //無線モジュールのステータスをチェックする

/*
EDF uint8_t auto_realscan_New(uint8_t DATA_FORMAT);
extern void get_regist_scan_unit_retry(void);
*/



//グローバル変数

EDF uint32_t    rfm_country;                                    ///<  無線モジュールシリアル番号最上位バイト（仕向け先情報）
//EDF bool        rfm_ready;        //未使用
//EDF volatile bool        in_rf_process;      //フラグ管理不要 OSの機能を使うべき
//EDF bool        rfmup_mode;                                     ///<  RFMアップデートモード

/* static
EDF struct def_rssi{
    char rpt;
    char level_to_koki;                                        ///<  子機方向へのコマンドのRSSIレベル
    char level_0x34;                                           ///<  0x34応答時のRSSIレベルを保存する
} relay_rssi;
*/




EDF struct def_sram {
    uint16_t    record;                                         ///<  記録レコード（ブロック内のアドレス）
} sram;

#if 0
//auto_thread_entry.hに移動
/// 自律動作の自動吸上げをリトライする子機、中継機情報
typedef struct st_def_download_buff_back{                       // 32byte
    char do_rpt[16];                                    ///<  自動データ吸上げに使用する中継機リスト
    char do_unit[16];                               ///<  自動吸上げする子機リスト
}def_download_buff_back;


/// 自律動作の自動吸上げリトライ対象子機、中継機を示す用変数
EDF  struct def_retry_buff{                             // 256byte
    def_download_buff_back download_buff[8];
}retry_buff;


/// 自律動作のリアルスキャン対象子機、中継機を示す変数(1グループの最大子機番号は128とする)
EDF  struct def_realscan_buff{                          // 64byte
    char do_rpt[16];                                    ///<  通信すべき中継機リスト
    char do_unit[16];                               ///<  通信すべき子機リスト
    char over_rpt[16];                              ///<  通信が終わった中継機リスト
    char over_unit[16];                             ///<  通信できた子機リスト
}realscan_buff;


/// 自律動作の自動吸上げ対象子機、中継機を示す用変数(1グループの最大子機番号は128とする)
EDF  struct def_download_buff{                          // 64byte
    char do_rpt[16];                                    ///<  通信すべき　中継機リスト
    char do_unit[16];                               ///<  通信すべき　子機リスト
    char over_rpt[16];                              ///<  通信成功　通信が終わった中継機リスト
    char over_unit[16];                             ///<  通信成功　通信できた子機リスト
    char ng_rpt[16];                                    ///<  通信エラー　通信が終わった中継機リスト
    char ng_unit[16];                               ///<  通信エラー　通信できた子機リスト
}download_buff;
#endif


//Globals.hから移動
EDF uint8_t complement_info[4];

EDF uint8_t     rf_command;
EDF uint8_t     my_rpt_number;



#endif /* COMP_RF_THREAD_H_ */
