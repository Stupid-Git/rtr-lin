/**
 * @file	Warning.h
 *
 * @date	Created on: 2019/09/09
 * @author	tooru.hayashi
 */

#ifndef _WARNING_H_
#define _WARNING_H_



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


#ifdef EDF
#undef EDF
#endif

#ifndef _WARNING_C_
    #define EDF extern
#else
    #define EDF
#endif

//========================================================
// 現在の警報は何があるのかフラグ
// WarningPort()関数で使う

#define	WR_FLG_LIMIT_1	BIT(0)
#define	WR_FLG_LIMIT_2	BIT(1)
#define	WR_FLG_LIMIT_3	BIT(2)
#define	WR_FLG_LIMIT_4	BIT(3)
#define	WR_FLG_LIMIT_5	BIT(4)
#define	WR_FLG_LIMIT_6	BIT(5)

#define	WR_FLG_SENSOR	BIT(12)
#define	WR_FLG_RF		BIT(13)
#define	WR_FLG_BAT		BIT(14)
#define	WR_FLG_SWON		BIT(15)


#define	WR_MASK_HILO	BIT(0)
#define	WR_MASK_SENSOR	BIT(1)		// 0と1はセットで
#define	WR_MASK_RF		BIT(2)
#define	WR_MASK_BATT	BIT(3)
#define	WR_MASK_INPUT	BIT(4)

//======= Warning Test 
#define WTEST_UPPER		BIT(0)
#define WTEST_LOWER		BIT(1)
#define WTEST_SENSOR	BIT(2)         ///< @bug   二重定義 → WTEST_** に変更
#define WTEST_COMM		BIT(3)
#define WTEST_BATT		BIT(4)
#define WTEST_EXTPS		BIT(5)




//********************************************************
//	フォーマットの定義
//********************************************************

typedef struct {				// 64byte
// 0
	uint8_t	group_no;			///< グループ番号
	uint8_t	format_no;			///< DATA FORMAT No.
	uint8_t	grobal_time[4];		///< グローバル時間
	uint8_t	__gap_0_[2];
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
	uint8_t	__gap_2_[6];
// 32
	uint8_t	unit_no;			///< 子機番号
	uint8_t	rssi;				///< RSSIレベル
	uint8_t	keika_time[2];		///< 最新データからの経過秒数
	uint8_t	batt;				///< 電池残量(下位4bit)/警報flag   Full=0   Empty=15   
	uint8_t	data_number[2];		///< データ番号(最新データの番号)
	uint8_t	alarm_cnt;			///< 警報カウント(CH1 + CH2)
// 40
	uint8_t	interval;			///< 記録間隔
	uint8_t	rec_mode;			///< 記録状況

	union {

		struct {
			uint8_t	warn_temp[2];	///< 警報値
			uint8_t	past_temp[2];	///< 経過秒
		} ex501;
		struct {
			uint8_t	warn_temp[2];	///< 警報値
			uint8_t	past_temp[2];	///< 経過秒
			uint8_t	__gap_4a_[6];

			uint8_t	warn_hum[2];	///< 警報値
			uint8_t	past_hum[2];	///< 経過秒
			uint8_t	__gap_4b_[6];
		} ex503;
/*
		struct {
			uint8_t	warn_lx[2];	// 警報値
			uint8_t	past_lx[2];	// 経過秒
			uint8_t	__gap_5_[6];

			uint8_t	warn_uv[2];	// 警報値
			uint8_t	past_uv[2];	// 経過秒
		} ex574;
*/
	} rec;

	uint8_t	__gap_6_[2];

} WARN_FORM;

EDF uint32_t CurrentWarning;      // 警報の状態 Globals.cのXRAM定義から移動


//プロトタイプ
EDF int SendWarningFile(int Test, uint32_t pat); 



#endif /* _WARNING_H_ */
