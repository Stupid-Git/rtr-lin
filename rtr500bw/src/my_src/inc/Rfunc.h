/**
 * @file	Rfunc.h
 *
 * @date	2019/07/29
 * @author	tooru.hayashi
 * @note	2020.Jul.27 GitHub 0727ソース反映済み 

 */

#ifndef _RFUNC_H_
#define _RFUNC_H_


#include "Globals.h"

#ifdef EDF
#undef EDF
#endif

#ifndef _RFUNC_C_
    #define EDF extern
#else
    #define EDF
#endif

EDF uint32_t     Same_ID_Check(void);									                // 中継機が受信したコマンドIDが既中継か判断する

EDF void        rpt_data_reverse(void);                                                 // 中継情報の保存エリアに保存されている中継情報を反転する
EDF void        extract_property(void);								                     // 子機ヘッダからプロパティ情報を読み取る



EDF uint8_t     check_regist_unit(void);                                                // 子機登録ファイルの中に、自律動作(警報,現在値,吸上げ)対象の子機のある/なしを検出
EDF uint8_t     get_regist_group_size(void);							                // 登録ファイルからグループ数を得る
EDF uint16_t    get_regist_group_adr(uint8_t);							                // 登録ファイルからグループ番号指定してグループ情報アドレスを取得する
EDF uint8_t     get_regist_relay_inf(uint8_t, uint8_t, char *);                      //登録ファイルからグループ番号と子機番号を指定して中継機情報を作成する

EDF uint8_t     get_regist_gather_unit(uint8_t gnum);									// 登録ファイルから自動吸上げ対象子機フラグを立てる
EDF uint8_t     get_regist_relay_inf_relay(uint8_t gnum, uint8_t knum, char *relay);	// 登録ファイルからグループ番号と中継機番号を指定して中継機情報を作成する
EDF uint8_t     get_regist_scan_unit(uint8_t gnum, uint8_t PARA);                       // 登録ファイルからグループ番号を指定してリアルスキャン対象子機リストを作成する
/*
EDF void        shortcut_info_clear(void);                                  // ショートカット情報　クリア
EDF void        shortcut_info_update(void);                                 // ショートカット情報　更新
EDF uint8_t     shortcut_info_number_read(uint8_t);                         // ショートカット情報　中継機番号　読み込み
EDF void        shortcut_info_number_write(uint8_t, uint8_t);               // ショートカット情報　中継機番号　書き込み

EDF uint8_t     shortcut_info_rssi_read(uint8_t);                           // ショートカット情報　ＲＳＳＩ　読み込み
EDF void        shortcut_info_rssi_write(uint8_t, uint8_t);                 // ショートカット情報　ＲＳＳＩ　書き込み
EDF bool        shortcut_info_search(uint8_t);                              // ショートカット情報　指定された中継器番号を探す
*/
EDF uint8_t     shortcut_info_redesignation(void);                          // ショートカット情報　次の中継機を指定する

EDF uint32_t get_regist_group_adr_ser(uint32_t serial);

EDF int         get_regist_old_relay(void);                                 // 登録されている中継機の新旧混在を検索する

EDF uint8_t rtr500_RSSI_1to5(uint8_t OrgData);                              // RTR-500が受信したRSSIレベルから1-5段階を返す

EDF uint8_t get_rpt_registry(uint8_t gnum, uint8_t knum);


EDF bool RF_full_moni_start_check(uint8_t flg);     //auto_thread
EDF void RF_full_moni_init(void);                    //auto_thread
EDF void RF_full_moni(void);           //auto_thread, cmd_thread
EDF void RF_full_moni_group(uint8_t grp_no);

EDF void RF_table_make(uint8_t grp_no, uint8_t parent_no, uint8_t sw);
EDF void RF_regu_moni_init(void);
EDF void RF_regu_Make_Required_ScanList(void);
EDF void RF_regu_moni(void);                    //cmd_thread
EDF void RF_regu_moni_group(uint8_t grp_no, uint8_t data_format);
EDF void RF_regu_moni_group_manual(uint8_t grp_no, uint8_t data_format);

EDF void RF_oya_log_make(uint8_t grp_no);
EDF void RF_oya_log_last(uint8_t grp_no);
EDF void RF_oya_log_clear(void);

EDF int RF_GetRepeaterRoute(uint8_t grp_no, int repeaterNum, RouteArray_t *route);
EDF int RF_GetRemoteRoute(uint8_t grp_no, int remoteUnitNum, RouteArray_t *route);
EDF int RF_clear_rssi(int grp_no, int flg, uint8_t no);
EDF int RF_ParentTable_Get(char *parent, int grp_no, int flg, uint8_t log);
EDF int RF_RssiTable_Get(char *parent, int grp_no, int flg);
EDF int RF_BatteryTable_Get(char *parent, int grp_no, int flg);
EDF uint8_t RF_get_regist_RegisterRemoteUnit(uint8_t gnum);
EDF void RF_WirelessGroup_RefreshParentTable(WirelessGroup_t *grp, uint8_t rssiLimit1, uint8_t rssiLimit2);


EDF uint32_t get_regist_group_adr_rpt_ser(uint32_t serial);
EDF uint8_t RF_get_rpt_registry_full(uint8_t gnum);

//EDF uint8_t realscan_err_data_add(void);
EDF void realscan_data_para(uint8_t format_no);

EDF uint8_t RF_get_max_unitno(int gnum, uint8_t *max_repeater_no);

EDF void GetRoot_from_ResultTable(uint8_t grp_no, int flg, int no, RouteArray_t* route);

EDF uint8_t RF_get_regist_group_size(uint32_t *adr);
EDF uint16_t RF_get_regist_group_adr(uint8_t gnum, def_group_registry *read_group_registry);
EDF uint8_t RF_get_regist_group_no(char *id);           // 2023.05.26

#endif /* _RFUNC_H_ */
