/**
 * @file   event_thread_entry.h
 *
 *  @date   2019/01/29
 *  @author haya
 */



#ifndef _THREAD_ENTRY_H_
#define _THREAD_ENTRY_H_

//#include "event_thread.h"
extern TX_THREAD event_thread;

#ifdef EDF
#undef EDF
#endif

#ifdef  _EVENT_THREAD_ENTRY_C_

#define EDF
#else
#define EDF extern
#endif



// event thread flag cycle_event_flag　　Event Thread の動作許可　再起動
#define FLG_EVENT_READY         BIT_0       // event thread 動作許可
#define FLG_EVENT_RESTART       BIT_1       // event thread 再起動（初期化実行）
#define FLG_EVENT_CONNECT_USB    BIT_2      //USB接続された  2020.Jun.26
#define FLG_EVENT_DISCONNECT_USB    BIT_3   //USB切断された  2020.Jun.26

//#############################
//#include "Globals.h"


#define EVENT_OBJ( X ) \
volatile OBJ_EVENT X = { \
    X ## _Create,   \
}

#define TIMER_OBJ( X ) \
volatile OBJ_TIMER X = { \
    X ## _Create,   \
}


/**
 * @note    パディングあり
 */
typedef struct {

    void ( * const Create)(void);

    int     MinCount;           ///< 1分に1回インクリメント

	struct {
		UCHAR	Enable;			///< ON/OFF
		UCHAR	Route;			///< 0:Email 1:FTP
		int16_t	Interval;		///< 警報周期(分) 現状10分固定
	} Warn;

	struct {
		UCHAR	Enable;			///< ON/OFF
		UCHAR	Route;			///< 0:Email 1:FTP
		int16_t	Interval;		///< モニタ周期(分)
	} Mon;

	struct {
		UCHAR	Enable;			///< ON/OFF
		UCHAR	Route;			///< 0:Email 1:FTP
		UCHAR	EnFlag[9];		///< 個別のEnableFlag
		UCHAR	Week[9];		///< 曜日（0は毎日）
		int16_t	DayMin[9];		///< 00:00からの分数
		int16_t	EventCnt;		///< イベント数
	} Suc;
	/// トレンド情報用
	struct {
		uint32_t	StartUTC;		///< 開始時刻
		int16_t	Interval;		///< 取得周期(分)
	} Trend;

 } OBJ_EVENT;

 typedef struct {

     void ( * const Create)(void);

     UINT   SEC;				///< 1秒でカウント    インクリメントだけ
     UINT   HOUR;				///< 1秒でカウント	１時間で戻す
     UINT   MIN;				///< 1秒でカウント	１分で戻す

 } OBJ_TIMER;



extern volatile OBJ_EVENT  EVT;
extern volatile OBJ_TIMER  TIMER;



EDF bool CheckEventParam(int log);


#endif /* EVENT_H_ */
