/**
 * @file DateTime.c
 *
 *  @date   Created on: 2018/12/20
 *  @author haya
 *  @note       2019.Dec.25  t.saito コメントをdoxygen形式に変更
 *  @note   2020.01.30  v6 0128 ソース反映済み
 */
#include "_r500_config.h"


#include <stdio.h>
#include <stdarg.h>
#include <limits.h>     //ULONG_MAX
#include <math.h>
#include <string.h>
#include <stdlib.h>

//#include "system_time.h"
//#include "system_thread.h"
#include "MyDefine.h"
//#include "Config.h"
#include "General.h"

/*
*/

#include "r500_defs.h"

#define _DATETIME_C_
#include "DateTime.h"





#define year_n      60L*60L*24L*365L    ///< 通常の1年の秒数
#define year_u      60L*60L*24L*366L    ///< 閏年の1年の秒数

#define month_28    60L*60L*24L*28L     ///< 普通の2月の秒数
#define month_29    60L*60L*24L*29L     ///< 閏年の2月の秒数
#define month_30    60L*60L*24L*30L     ///< 1ヶ月30日の秒数
#define month_31    60L*60L*24L*31L     ///< 1ヶ月31日の秒数

#define SEC_DAY(X)  ( 60L*60L*24L*(X) )     ///< X日の秒数



/***********************************************************************************************************************
 * Private global variables
 **********************************************************************************************************************/

const int32_t monList_n[12] = {
		month_31,	// 1
		month_28,	// 2
		month_31,	// 3
		month_30,	// 4
		month_31,	// 5
		month_30,	// 6
		month_31,	// 7
		month_31,	// 8
		month_30,	// 9
		month_31,	// 10
		month_30,	// 11
		month_31	// 12
};

const int32_t monList_u[12] = {
		month_31,	// 1
		month_29,	// 2
		month_31,	// 3
		month_30,	// 4
		month_31,	// 5
		month_30,	// 6
		month_31,	// 7
		month_31,	// 8
		month_30,	// 9
		month_31,	// 10
		month_30,	// 11
		month_31,	// 12
};


static const uint8_t MonthDay[][12] = {
//     1   2   3   4   5   6   7   8   9  10  11  12
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },	// 通常
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }	// 閏年
};


static const uint16_t MonthDayTotal[][12] = {
    {
        0,
        31 ,                                    // 1
        28 + 31 ,                               // 2
        31 + 31+28 ,                            // 3
        30 + 31+28+31 ,                         // 4
        31 + 31+28+31+30 ,                      // 5
        30 + 31+28+31+30+31 ,                   // 6
        31 + 31+28+31+30+31+30 ,                // 7
        31 + 31+28+31+30+31+30+31 ,             // 8
        30 + 31+28+31+30+31+30+31+31 ,          // 9
        31 + 31+28+31+30+31+30+31+31+30 ,       // 10
        30 + 31+28+31+30+31+30+31+31+30+31 ,    // 11
//      31 + 31+28+31+30+31+30+31+31+30+31+30 , // 12
    },  // 通常
    {
        0,
        31 ,                                    // 1
        29 + 31 ,                               // 2
        31 + 31+29 ,                            // 3
        30 + 31+29+31 ,                         // 4
        31 + 31+29+31+30 ,                      // 5
        30 + 31+29+31+30+31 ,                   // 6
        31 + 31+29+31+30+31+30 ,                // 7
        31 + 31+29+31+30+31+30+31 ,             // 8
        30 + 31+29+31+30+31+30+31+31 ,          // 9
        31 + 31+29+31+30+31+30+31+31+30 ,       // 10
        30 + 31+29+31+30+31+30+31+31+30+31 ,    // 11
//      31 + 31+29+31+30+31+30+31+31+30+31+30 , // 12
    }   // 閏年
};


// # RTCへの時間の設定は、SNTPが、system_time.c adjust_time() で行う
//   RTCは、UTCで動作している

//プロトタイプ
static bool RTC_GDay2Date(int days, rtc_time_t *Tm);



/**
 * @brief   RTC 初期化
 * @note  RTCへの時間の設定は、SNTPが、system_time.c adjust_time() で行う
 * @note  RTCは、UTCで動作している
 */
void RTC_Create(void)
{
	int32_t Min;
	rtc_time_t ct;

	char Temp[sizeof(my_config.device.TimeDiff)+2];
	
	//g_rtc.p_api->calendarTimeGet(g_rtc.p_ctrl, &ct);
	with_retry_calendarTimeGet("RTC_Create", &ct);		// 2022.09.09
//



//


	memset(Temp,0,sizeof(Temp));
	memcpy( Temp, my_config.device.TimeDiff, sizeof(my_config.device.TimeDiff) );

	Min = ( AtoI(Temp, 3) * 60 ) + AtoI( (char *)&Temp[3], 2);
	Printf("   Time Diff %d \r\n", Min);

	if( Min >= 24*60 || Min <= -24*60)
		Min = 0;
		
	RTC.Diff = Min * 60;		// 秒で格納
	//RtcDiff_Backup = RTC.Diff; 
	//TimeDiff = RTC.Diff;

	Printf("RTC_CreateT %d/%d/%d %d:%d:%d (%d)\r\n", ct.tm_year,ct.tm_mon,ct.tm_mday,ct.tm_hour,ct.tm_min,ct.tm_sec, RTC.Diff);
	//SetDST( ct.tm_year );	// これを呼ぶ前に時差情報が確定している事
	SetDST( ct.tm_year-100 );	// これを呼ぶ前に時差情報が確定している事		// sakaguchi 2021.05.17



	
}


/**
 * GMT時刻の取得(RTCレジスタ読み込み)
 * @param ct    保存先ポインタ
 * @note    (RTCにはGMT時刻が書かれている)
 * @note    ※セマフォによるブロックあり Synergyは必要ないかも？？
 * @note   2020.01.16 ポインタ渡しに変更
 */
void RTC_GetGMT( rtc_time_t *ct )
{
	//bool	status = false;
//    rtc_time_t ct;
    //g_rtc.p_api->calendarTimeGet(g_rtc.p_ctrl, ct);
    //Printf("RTC_GetGMT %d/%d/%d %d:%d:%d \r\n", ct.tm_year,ct.tm_mon,ct.tm_mday,ct.tm_hour,ct.tm_min,ct.tm_sec);
	with_retry_calendarTimeGet("RTC_GetGMT", ct);		// 2022.09.09

    ct->tm_year -= 100;
    ct->tm_mon += 1;
    //status = true;

//	return ct;
}


/**
 * GMT時刻の取得(UNIX秒)
 * @return
 * @note    RTC_GetGMTを使う
 */
uint32_t RTC_GetGMTSec( void )
{
//	rtc_time_t	Tm;     //2019.Dec.26 コメントアウト
    rtc_time_t ct;
	uint32_t		Gsec;

    RTC_GetGMT(&ct);
    Gsec = RTC_Date2GSec( &ct );
	//Printf("RTC_GetGMTSec %ld\r\n", Gsec);

   /*
	if ( RTC_GetGMT( &Tm ) == true ){
		Gsec = RTC_Date2GSec( &Tm );
    }
	else{
		Gsec = 0;
        Printf("GetGMTSec error \r\n");
    }
    */
	return (Gsec);
}


/**
 * 現在のLocal時刻の取得(RTC_Struct)
 * GMT時刻(UNIX秒)を取得して時差とサマータイムを加算後、RTC_Structへ入れる
 * @param Val
 * @return
 */
bool RTC_GetLCT( rtc_time_t *Val )
{
	bool Status = false;
	uint32_t GSec;

    GSec = RTC_GetGMTSec();
	if ( GSec ) {				// UNIX秒を取得して
		//
		// サマータイム中はAdjustSecを加算
		//
		GSec = (uint32_t)((int64_t)GSec + RTC.Diff + GetSummerAdjust( GSec ));
		Status = RTC_GSec2Date( (uint32_t)GSec, Val );	// RTC_Structへ変換
	}

	return (Status);
}


/**
 * 現在のLocal秒の取得
 * @param GSec  引数が0の時は現在の時刻 それ以外は指定されたものを変換する
 * @return
 */
uint32_t RTC_GetLCTSec( uint32_t GSec )
{
//	long	GSec;
    uint32_t GNTSec;

	if ( !GSec ){						// 引数が０の場合は
	    GNTSec = RTC_GetGMTSec();			// 現在の時刻を取得
	}else{
	    GNTSec = GSec;
	}
	//
	// サマータイム中はAdjustSecを加算
	//
	GNTSec = (uint32_t)((int64_t)GNTSec + RTC.Diff + GetSummerAdjust( GNTSec ));

	return (GNTSec);
}


/**
 * GMT時刻(UNIX秒)からLocal時刻の取得(RTC_Struct)
 * サマータイムを加算後、RTC_Structへ入れる
 * @param GSec
 * @param Val
 * @return
 */
bool RTC_GSec2LCT( uint32_t GSec, rtc_time_t *Val )
{
	bool Status = false;

	if ( GSec ) {				// UNIX秒
		//
		// サマータイム中はAdjustSecを加算
		//
		GSec = (uint32_t)((int64_t)GSec + RTC.Diff + GetSummerAdjust( GSec ));
		Status = RTC_GSec2Date( GSec, Val );	// RTC_Structへ変換
	}

	return (Status);
}


/**
 * @brief   日付・時刻をグローバル秒(GMT)に変換
 *
 * RTC_Struct の形式の時刻を、2000年1月1日00時00分00秒からのトータル秒に変換する
 * @param Tm    変換する日付構造体へのポインタ(2000年は指定不可)
 * @return  変換後のグローバル秒 (2000/01/01を0とした秒数)  ※エラーのときは0を返す
 */
uint32_t RTC_Date2GSec( rtc_time_t *Tm )
{
	uint32_t  totalTime;
	uint32_t totalDays;

	if( Tm->tm_hour > 23 || Tm->tm_min > 59 || Tm->tm_sec > 59 )
		return ( 0 );		// 時分秒エラー

	totalDays = RTC_Date2GDay( Tm );			// 前日までの日数を出す
	if ( !totalDays )
		return ( 0 );		// 日付エラー

	// 秒数の計算 -----------------------------------------------
	// 年
	totalTime = SEC_DAY( (uint32_t)totalDays );		// 経年の秒数

	// 時
	totalTime += 3600L * (uint32_t)Tm ->tm_hour;

	// 分
	totalTime += 60L * (uint32_t)Tm->tm_min;

	// 秒
	totalTime += (uint32_t)Tm->tm_sec;

	//Printf("\r\n: GSM total time[%d]",totalTime);
	return ((uint32_t)( totalTime + UNIX_BASE_SEC ));		// 最後にUNIX-TIMEを加算
}


/**
 * @brief   日付をグローバル日に変換
 *
 * 指定日付を2000年1月1日からのトータル日に変換する
 * @param Tm    変換する日付構造体へのポインタ(2000年は指定不可)
 * @return      変換後のトータル日 (2000/01/01を0とした日数)   ※エラーのときは0を返す
 */
uint32_t RTC_Date2GDay( rtc_time_t *Tm )
{
	int	totalDays, UruCount;
	int	UruFlag;
	int	Year, Month, Day;

	Year  = Tm->tm_year;			// 求めるのは2000年からだけど2000年は指定できないよ
	Month = Tm->tm_mon - 1;		// 0～11とする
	Day   = Tm->tm_mday - 1;		// 0～30(or27or29)とする

	if ( !Year ){
	    // 求めるのは2000年からだけど2000年は指定できないよ
		return ( 0 );				// 年エラー
	}


	UruFlag = ( Year % 4 == 0 ) ? 1 : 0 ;		// 閏年＝１

	if( Month < 12 ) {

		if( Day >= MonthDay[UruFlag][Month] ){
		    return ( 0 );		// 日数エラー
		}

	}
	else{
	    return ( 0 );			// 月数のエラー
	}

	// 日数の計算 -----------------------------------------------
	// 年
	UruCount = ( (Year-1) >> 2 ) + 1;					// 前年までの閏年の回数

	totalDays  = UruCount * 366;						// 閏年の日数
	totalDays += ( Year - UruCount ) * 365;		// その他の年の日数

	// 月
	totalDays += MonthDayTotal[UruFlag][Month];

	// 日
	totalDays += Day;

	return((uint32_t)totalDays);
}


/**
 * @brief   グローバル日を日付に変換
 *
 * 2000年1月1日からのトータル日を日付に変換する
 * @param   days    グローバル日,
 * @param   *Tm   変換後の格納エリア（時刻は何もしない）
 * @return     true / false
 */
static bool RTC_GDay2Date(int days, rtc_time_t *Tm)
{
    bool flag = true;
//    uint16_t year;
    int year;
    int Days = days;        //計算用変数
    int16_t UruFlag;
    int i;
//    uint8_t  Week;
    int Week;
                                    // 0  1  2  3  4 5  6  2000/1/1が土曜日なので＋６
    Week = (Days + 6) % 7; // 日 月 火 水 木 金 土

    year = ( Days / 1461 ) * 4;     // 4年分 (366 + 365 + 365 + 365 = 1461)
    Days = Days % 1461;             // 4年までの余り
    if ( Days >= 1096 ) {           // 366+365+365
        Days -= 1096;
        year += 3;                  // 3年
    } else if ( Days >= 731 ) {     // 366+365
        Days -= 731;
        year += 2;                  // 2年
    } else if ( Days >= 366 ) {     // 366
        Days -= 366;
        year += 1;                  // 1年
    }

    if ( !year || year >= 100 ) {   // 求めるのは2000年からだけど2000年はNGよ
        flag = false;               // 年エラー
        Tm->tm_year = 0;
        Tm->tm_mon = 0;
        Tm->tm_mday = 0;
        Tm->tm_wday = 0;
    }
    else
    {

        UruFlag = ( year % 4 == 0 ) ? 1 : 0 ;       // 閏年＝１

        for ( i = 11; i > 0; i-- ) {
            if ( MonthDayTotal[UruFlag][i] <= Days ) {
                Days -= MonthDayTotal[UruFlag][i];
                break;
            }
        }

        Tm->tm_year  = year;
        Tm->tm_mon =  i + 1 ;
        Tm->tm_mday  = ( Days + 1 );
        Tm->tm_wday  = Week;
    }

    return (flag);
}


/**
 * @brief   グローバル秒を日付に変換
 *
 * 2000年1月1日からのグローバル秒を日付に変換する
 * @param Secs  グローバル秒
 * @param Tm    変換後の格納エリア
 * @return      TRUE / FALSE
 */
bool RTC_GSec2Date( uint32_t Secs, rtc_time_t *Tm )
{
	bool	Flag;
	int	Temp;

	Secs -= UNIX_BASE_SEC;						// UNIX-TIME減算

	Temp = (int)(Secs / 86400);						// 1日( 24 * 60 * 60 )
	Flag = RTC_GDay2Date( Temp, Tm );	// グローバル日数を求める

	if ( Flag == true ) {

		Temp = (int)(Secs % 86400);					// 余りの秒数

		Tm->tm_hour = ( Temp / 3600 );		// 60*60秒
		Temp -= (Tm->tm_hour * 3600L);
		Tm->tm_min  = ( Temp / 60 );		// 分
		Tm->tm_sec  = ( Temp % 60 );		// 秒

	}

	return (Flag);
}


/**
 * @brief   書式による日付と時刻の文字列の取得
 *
 * 最大255文字+'\0'までで最後は\0が必ず入る
 * @param Tm
 * @param Dst
 * @param TForm
 * @return      戻り値は変換後の文字数
 */
int RTC_GetFormStr( rtc_time_t *Tm, char *Dst, char *TForm )
{
//	RTC_Struct Tm;
	int	Point, size;
	char	tmp[sizeof(my_config.device.Name)+2], data, *Base = Dst;
	static const char *Moname[] = {
		"   ","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
	};

//	RTC_GetGMT( &Tm );			// 現在時刻取得

	Point=0;
	while ( Point<255 ) {

		data = *TForm++;
		if ( data == '\0' )
			break;
		else if ( data != '*' ) {
			*Dst++ = data;
			Point++;
			continue;
		}
		else
		{
			data = *TForm++;
			switch ( data ) {
				case 'Y' :		// 年 4桁
					size = 4;
					sprintf( tmp, "20%02d", Tm->tm_year );
					break;
				case 'y' :		// 年 2桁
					size = 2;
					sprintf( tmp, "%02d", Tm->tm_year );
					break;
				case 'M' :		// 月 英語表記 3桁
					size = 3;
					sprintf( tmp, "%.3s", Moname[Tm->tm_mon] );
					break;
				case 'm' :		// 月 02桁
					size = 2;
					sprintf( tmp, "%02d", Tm->tm_mon );
					break;
				case 'n' :		// 月 1～2桁
					size = sprintf( tmp, "%-d", Tm->tm_mon );
					if ( size > 2 )
						size = 2;
					break;
				case 'd' :		// 日 02桁
					size = 2;
					sprintf( tmp, "%02d", Tm->tm_mday );
					break;
				case 'j' :		// 日 1～2桁
					size = sprintf( tmp, "%-d", Tm->tm_mday );
					if ( size > 2 )
						size = 2;
					break;
				case 'H' :		// 時 02桁
					size = 2;
					sprintf( tmp, "%02d", Tm->tm_hour );
					break;
				case 'i' :		// 分 02桁
					size = 2;
					sprintf( tmp, "%02d", Tm->tm_min );
					break;
				case 's' :		// 秒 02桁
					size = 2;
					sprintf( tmp, "%02d", Tm->tm_sec );
					break;
				case 'P' :		// 時差 +00:00
					size = 6;
					sprintf( tmp, "%+03ld:%02ld", RTC.Diff/3600L,labs(RTC.Diff)%3600L );
					break;
				case 'O' :		// 時差 +0000
					size = 5;
					sprintf( tmp, "%+03ld%02ld", RTC.Diff/3600L,labs(RTC.Diff)%3600L );
					break;
				case 'B' :		// 親機名称
                    memcpy(&tmp,  &my_config.device.Name, sizeof(my_config.device.Name));
					size = (int)strlen( tmp );
					break;
				case 'C' :		// 子機名称

//#ifdef	_RTR500_GSM_
//					size = strlen(Gsm.uname);
//					if(size)
//						sprintf( tmp, "%s", Gsm.uname);
//#endif
					break;		// ちょっと後回し

				default:
					size = 0;
					TForm--;
					break;
			}
		}

		if ( Point + size <= 255 ) {
			memcpy ( Dst, tmp, (uint32_t)size );
			Point += size;
			Dst += size;
		}
		else{
		    break;
		}

	}
	*Dst++ = '\0';

	return ( Dst - Base - 1 );
}




/**
 * サマータイムかどうか調べる
 * @param GSecs UNIX秒(GMT秒)でチェックする時間を渡す
 * @return  戻り値は加算すべき秒数
 */
uint32_t GetSummerAdjust( uint32_t GSecs )
{
	uint32_t	Gsec1, Gsec2;
	bool	Status = 0;

	if ( RTC.AdjustSec && RTC.SummerBase ) {		// SummerTime ON ?

		Gsec1 = RTC_Date2GSec( &RTC.SummerIN );		// 年を同じにして
		Gsec2 = RTC_Date2GSec( &RTC.SummerOUT );	// それぞれを取得

		if ( Gsec1 && Gsec2 && GSecs ) {
			if ( Gsec1 < Gsec2 ) {						// 北半球
				if ( Gsec1 <= GSecs && GSecs < Gsec2 )
					Status = 1;
			}
			else if ( Gsec1 <= GSecs || GSecs < Gsec2 ) {	// 南半球
					Status = 1;
			}
		}
	}

	GSecs = ( Status ) ? (uint32_t)RTC.AdjustSec : 0;

	return (GSecs);
}


/**
 * 指定された年のサマータイムを算出してセット
 * @param Year
 * @return
 */
bool SetDST( int Year )
{
	uint32_t	GSecs;
	uint32_t	GDays;
	char	Temp[sizeof(my_config.device.Summer)+2];
	int	week, week_in, week_out;

	memcpy( Temp, my_config.device.Summer, sizeof(my_config.device.Summer));

	RTC.SummerIN.tm_year = RTC.SummerOUT.tm_year = Year;		// 年
	RTC.SummerIN.tm_sec = RTC.SummerOUT.tm_sec = 0;			// 秒=0

	RTC.SummerIN.tm_mon = AtoI( &Temp[ 1], 2 );	// 月
	RTC.SummerIN.tm_mday   = AtoI( &Temp[ 3], 2 );	// 日
	week_in   =  AtoI( &Temp[ 6], 2 );			// 週(絶対値)
	RTC.SummerIN.tm_hour  = AtoI( &Temp[ 8], 2 );	// 時
	RTC.SummerIN.tm_min   = AtoI( &Temp[10], 2 );	// 分

	RTC.SummerOUT.tm_mon = AtoI( &Temp[12], 2 );	// 月
	RTC.SummerOUT.tm_mday = AtoI( &Temp[14], 2 );	// 日
	week_out   =  AtoI( &Temp[17], 2 );			// 週(絶対値)
	RTC.SummerOUT.tm_hour = AtoI( &Temp[19], 2 );	// 時
	RTC.SummerOUT.tm_min   = AtoI( &Temp[21], 2 );	// 分

	RTC.SummerBase = (char)(Temp[0] - '0');					// 0:none 1:LOCAL 2:GMT

	RTC.AdjustSec = AtoL( &Temp[23], 3 );			// Bias(秒)
	RTC.AdjustSec = ( RTC.AdjustSec == INT32_MAX ) ? 0 : (RTC.AdjustSec * 60);

	if ( !RTC.SummerIN.tm_mday ) {					// 日が0?
		RTC.SummerIN.tm_mday = 1;
		GDays = RTC_Date2GDay( &RTC.SummerIN );		// グローバル日
		if ( GDays ) {								// 日 月 火 水 木 金 土 
			week = (int)( ( GDays + 6 ) % 7 );	//  0  1  2  3  4  5  6  2000/1/1が土曜日なので+6
			week = (( (week_in-1) % 7 ) + 1 ) - week;
			if ( week <= 0 ){
			    week += 7;		// 第１＊曜日の日付が出る
			}

			if ( Temp[5] == '-' ) {				// 最終から？
				week += 28;						// 7*4週間足してみる
				RTC.SummerIN.tm_mday = week;
				if ( !RTC_Date2GDay( &RTC.SummerIN ) )	// 存在しない?
				{
				    week -= 7;					// 1週間手前
				}

				week -= ( ( week_in - 1 ) / 7 ) * 7;	// 第＊週の日付が出る
			}
			else if ( Temp[5] == '+' ) {			// 最初から？
				week += ( ( week_in - 1 ) / 7 ) * 7;	// 第＊週の日付が出る
				RTC.SummerIN.tm_mday = week;
				if ( !RTC_Date2GDay( &RTC.SummerIN ) )	// 存在しない?
				{
				    week = 0;
				}
			}
			else{
			    week = 0;
			}

			RTC.SummerIN.tm_mday = week;

		}
	}

	if ( !RTC.SummerOUT.tm_mday ) {					// 日が0?
		RTC.SummerOUT.tm_mday = 1;
		GDays = RTC_Date2GDay( &RTC.SummerOUT );		// グローバル日
		if ( GDays ) {							// 日 月 火 水 木 金 土 
			week = (int)( ( GDays + 6 ) % 7 );	//  0  1  2  3  4  5  6  2000/1/1が土曜日なので+6
			week = (( (week_out-1) % 7 ) + 1 ) - week;
			if ( week <= 0 )	week += 7;		// 第１＊曜日の日付が出る

			if ( Temp[16] == '-' ) {			// 最終から？
				week += 28;						// 7*4週間足してみる
				RTC.SummerOUT.tm_mday = week;
				if ( !RTC_Date2GDay( &RTC.SummerOUT ) )	// 存在しない?
				{
				    week -= 7;					// 1週間手前
				}

				week -= ( ( week_out - 1 ) / 7 ) * 7;	// 第＊週の日付が出る
			}
			else if ( Temp[16] == '+' ) {			// 最初から？
				week += ( ( week_out - 1 ) / 7 ) * 7;	// 第＊週の日付が出る
				RTC.SummerOUT.tm_mday = week;
				if ( !RTC_Date2GDay( &RTC.SummerOUT ) )	// 存在しない?
				{
				    week = 0;
				}
			}
			else{
			    week = 0;
			}

			RTC.SummerOUT.tm_mday = week;

		}
	}

	GSecs = 0;
	if ( RTC.SummerBase == 1 ) {		// 現地時間基準？
	    GSecs = RTC_Date2GSec( &RTC.SummerIN );
		if ( GSecs ) {
			GSecs = (uint32_t)((int64_t)GSecs - RTC.Diff);						// 時差情報を引く(GMTへ)

			RTC_GSec2Date( GSecs, &RTC.SummerIN );

			GSecs = RTC_Date2GSec( &RTC.SummerOUT ) ;
			if ( GSecs ) {
				GSecs = (uint32_t)((int64_t)GSecs - RTC.Diff);                  // 時差情報を引く(GMTへ)
				GSecs = (uint32_t)((int64_t)GSecs - RTC.AdjustSec);         	// Bias(秒)も引く(GMTへ)
				RTC_GSec2Date( GSecs, &RTC.SummerOUT );
			}
		}
	}
	else if ( RTC.SummerBase == 2 ) {		// GMT基準？
		if ( RTC_Date2GSec( &RTC.SummerIN ) ){
		    GSecs = RTC_Date2GSec( &RTC.SummerOUT );	// 時刻チェックのみ
		}
	}
	else
	{
		RTC.SummerBase = 0;
		RTC.AdjustSec = 0;			// Bias(秒)は0にする
	}

	return ( GSecs ? true : false);
}

// 2022.09.09
ssp_err_t with_retry_calendarTimeGet(char* caption, rtc_time_t *p_rtc_t)
{
    ssp_err_t ssp_err;
    int       i;
    TX_INTERRUPT_SAVE_AREA

    for(i=0; i<5; i++){
		TX_DISABLE
        ssp_err = g_rtc.p_api->calendarTimeGet(g_rtc.p_ctrl, p_rtc_t);
        TX_RESTORE
        if(ssp_err == SSP_SUCCESS){
            //Printf("%s OK[%d] %d/%d/%d %d:%d:%d \r\n", caption, ssp_err, p_rtc_t->tm_year+1900, p_rtc_t->tm_mon+1,p_rtc_t->tm_mday,p_rtc_t->tm_hour,p_rtc_t->tm_min,p_rtc_t->tm_sec);
            break;
        }else{
            Printf("%s err[%d] %d/%d/%d %d:%d:%d \r\n", caption, ssp_err, p_rtc_t->tm_year+1900, p_rtc_t->tm_mon+1,p_rtc_t->tm_mday,p_rtc_t->tm_hour,p_rtc_t->tm_min,p_rtc_t->tm_sec);
        }
    }
    return ssp_err;
}
