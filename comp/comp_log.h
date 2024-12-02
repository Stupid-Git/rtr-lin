/*
 * comp_log.h
 *
 *  Created on: Nov 21, 2024
 *      Author: karel
 */

#ifndef COMP_LOG_H_
#define COMP_LOG_H_


#ifdef EDF
#undef EDF
#endif

#ifndef _LOG_C_

#define EDF extern
#else
#define EDF
#endif

//********************************************************

#define	LOG_SYS			0
#define	LOG_CMD			1
#define	LOG_RF			2
#define	LOG_GSM			3
#define	LOG_IR			4
#define	LOG_GPS			5
#define	LOG_LAN			6

#define	LOG_DBG			9

#define	LOG_NUMBER		9		///<  最終値



#define LOG_LINE_SIZE	115

#define LOG_SEND_CNT 10			// ログ送信件数	// 2023.08.01

//********************************************************


//********************************************************
//	オブジェクトの宣言
//********************************************************
/// ログ１ラインの情報（254以下の事）    128 byte
typedef struct {

	uint8_t	    Size;			///<  データサイズ(メンバの先頭＋uint8_tであること)
	uint8_t	    Category;		///<  エラーカテゴリ (GSM or RF or ...)
	uint16_t	Number;			///<  シリアル番号
	uint32_t	GSec;			///<  グローバル秒
	uint32_t	LSec;			///<  ローカル秒          ここまで　12byte
	char	    Data[LOG_LINE_SIZE];		///<  実データ
    uint8_t     gap[1];
} LOG_LINE;

///ログ制御用
/// @attention   未使用
typedef struct {

	uint32_t	Size;
	uint32_t	pStart;			///<  開始ポインタ(最古データへのポインタ)
	uint32_t	pWrite;			///<  次に書き込むポインタ
	uint16_t	Count;			///<  ログ数
	uint16_t	Number;			///<  シリアル番号（次に書かれる番号）
//	uint8_t	*Buff;			// バッファ開始アドレス
//	uint8_t	Data[256];		//

} LOG_CTRL;

/// ログ取得用
typedef struct {

	uint32_t	SaveSize;
	uint32_t	pRead;
	int			StartNum;		///<  読み込み開始ログ番号
//	uint16_t	Count;			// ログ数

} LOG_GET;

/**
 * ログ情報
 * @note    パディングあり
 */
typedef struct {

	uint32_t	Size;
	uint32_t	pStart;			///< 開始ポインタ(最古データへのポインタ)
	uint32_t	pWrite;			///< 次に書き込むポインタ
	uint16_t	Count;			///< ログ数
	uint16_t	NextNumber;		///< シリアル番号（次に書かれる番号）
	uint16_t	FirstNumber;	///< 最も古いログ番号
} LOG_INFO;


//EDF LOG_CTRL	LogCtrl;		///< 制御情報


EDF int32_t GetLogInfo(int);
EDF void ResetLog(void);
EDF void  PutLog( uint8_t Flag, const char *fmt, ... );
EDF void  DebugLog( uint8_t Flag, const char *fmt, ... );
//EDF uint16_t GetLog( char *Adr, int16_t Number, int16_t MaxSize );
EDF uint16_t GetLog( char *Adr, int Number, int MaxSize, int area );
//EDF uint16_t GetLog_http( char *Adr, int Number, int MaxSize, int area );		// sakaguchi 2020.12.07
EDF uint16_t GetLog_http( char *Adr, int Number, int MaxSize, int area, int file );		// 2023.03.01
EDF uint16_t GetLogCnt_http(int area);	// 2023.08.01
EDF void Net_LOG_Write( uint32_t stat, const char *fmt, uint32_t msg);





#endif /* COMP_LOG_H_ */
