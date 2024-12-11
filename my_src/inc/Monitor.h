/**
 * @file    Monitor.h
 *
 * @date    2019/09/05
 * @author  tooru.hayashi
 */

#ifndef _MONITOR_H_
#define _MONITOR_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef EDF
#undef EDF
#endif

#ifndef _MONITOR_C_
    #define EDF extern
#else
    #define EDF
#endif

//********************************************************
//	フォーマットの定義
//********************************************************
/**
 * モニタフォーマットの定義
 */
typedef struct {				// 64byte
// 0
	uint8_t	group_no;			///< グループ番号
	uint8_t	format_no;			///< DATA FORMAT No.
	uint8_t	grobal_time[4];		///< グローバル時間
	uint8_t	rf_err;				///< 無線通信NGの場合 0xCC or 0xCB
	uint8_t	wr_flags;			///< 自律動作フラグ
// 8
	uint8_t	kind;				///< 設定ファイルの子機種類(0xFE, 0xFA)
	uint8_t	scale;				///< スケール変換フラグ　__gap_1_[1];
	uint8_t	attrib[2];			///< CH属性
	uint8_t	serial[4];			///< シリアル番号
// 16
	uint8_t	unit_name[8];		///< 子機名称
// 24
	uint8_t	kisyu505;			///< 505機種コード
	uint8_t	disp_unit;			///< 表示単位(505)
	uint8_t	kgc_time;			// KGCモードの演算間隔
	uint8_t	kgc_mode;			// KGCモードの演算方式
	uint8_t	__gap_2_[2];		
//	uint8_t	__gap_2_[6];
//	uint8_t	__gap_2_[4];		
	uint8_t	rep_no;				///< 末端の中継機番号 2017-12-12 Ver3.00より
	uint8_t	__gap_4_[1];
// 32
	uint8_t	unit_no;			///< 子機番号
	uint8_t	rssi;				///< RSSIレベル
	uint8_t	keika_time[2];		///< 最新データからの経過秒数
	uint8_t	batt;				///< 電池残量(下位4bit)/警報flag
	uint8_t	data_number[2];		///< データ番号(最新データの番号)
	uint8_t	alarm_cnt;			///< 警報カウント(CH1 + CH2)
// 40
	uint8_t	data_intt;			///< モニタデータ間隔
	uint8_t	data_kind;			///< データ種類

	union {

		struct {
		    char	temp[20];	///< 記録データ(温度)
		} ex501;
		struct {
		    char	temp[10];	///< 記録データ(温度)
		    char	hum[10];	///< 記録データ(湿度)
		} ex503;
		struct {
		    char	mp_lx[2];	///< 積算照度
		    char	lx[8];		///< 照度
		    char	mp_uv[2];	///< 積算紫外線
		    char	uv[8];		///< 紫外線
		} ex574;
		struct {
		    char	pulse[20];	///< 505(パルス数など）
		} ex505;
	} rec;

	char	__gap_3_[2];

} MON_FORM;

EDF int32_t SendMonitorFile(int32_t Test);

EDF void MakeMonitorFTP(void);
EDF void MakeMonitorXML(int32_t Test );
EDF char *MakeMonitorBody(void);
EDF void MakeMonitorMail(bool Test);
EDF void MakeMonitorHTTP(void);


#endif /* _MONITOR_H_ */
