/**
 * @file    Convert.c
 *
 * @date   Created on: 2019/09/05
 * @author tooru.hayashi
 * @note	2020.01.30	V6 0130 ソース反映済み
 * @note	2020.Aug.07	0807ソース反映済み
 */

#define _CONVERT_C_

#include "Globals.h"
#include "General.h"
#include "Convert.h"
//#include "cmd_thread_entry.h"
#include <math.h>       //pow()等 2020.01.21
#include <stdlib.h>     //abs()等


//-------------------------------------------------------------------
// 単位変換関数（変換は必ずこれを使うこと
//-------------------------------------------------------------------
static int16_t ConvertTempC( char *Dst, uint32_t Data );
static int16_t ConvertTempF( char *Dst, uint32_t Data );
static int16_t ConvertHum( char *Dst, uint32_t Data );
static int16_t ConvertHum2( char *Dst, uint32_t Data );
static int16_t ConvertLx( char *Dst, uint32_t Data );
static int16_t ConvertSumLx( char *Dst, uint32_t Data );
static int16_t ConvertUV( char *Dst, uint32_t Data );
static int16_t ConvertSumUV( char *Dst, uint32_t Data );
static int16_t ConvertNone( char *Dst, uint32_t Data );
static int16_t ConvertAmp( char *Dst, uint32_t Data );
static int16_t ConvertAmp_S( char *Dst, uint32_t Data );
static int16_t ConvertVolt( char *Dst, uint32_t Data );
static int16_t ConvertVolt_S( char *Dst, uint32_t Data );
static int16_t ConvertPSDW( char *Dst, uint32_t Data );
static int16_t ConvertPSDW_S( char *Dst, uint32_t Data );
static int16_t ConvertPSUP( char *Dst, uint32_t Data );
static int16_t ConvertPSUP_S( char *Dst, uint32_t Data );
static int16_t ConvertCO2( char *Dst, uint32_t Data );


const OBJ_METHOD MeTable[] =
{
	{ ConvertNone,	0x0000, "N/A"	  },	// METHOD_NONE		// 該当無し
	{ ConvertTempC,	0x000D, "C"		  },	// METHOD_C		1	// 摂氏
	{ ConvertTempF,	0x000D, "F"		  },	// METHOD_F		2	// 華氏
	{ ConvertHum,	0x00D0, "%"		  },	// METHOD_HUM	3	// 湿度
	{ ConvertHum2,	0x00D1, "%"		  },	// METHOD_HUM2	4	// 湿度(0.1%精度)
	{ ConvertLx,	0x0049, "lx"	  },	// METHOD_LX	5	// 照度
	{ ConvertSumLx,	0x0149, "lxh"	  },	// METHOD_LXH	6	// 積算照度
	{ ConvertUV,	0x0055, "mW/cm2"  },	// METHOD_MW	7	// 紫外線強度
	{ ConvertSumUV,	0x0155, "mW/cm2h" },	// METHOD_MWH	8	// 積算紫外線強度
	{ ConvertAmp,	0x0081, "mA"	  },	// METHOD_MA	9	// 電流
	{ ConvertAmp_S,	0x0081, "mA"	  },	// METHOD_MA	9	// 電流
	{ ConvertVolt,	0x0091, "mV"	  },	// METHOD_MV1  10	// 電圧(0.01mV)
	{ ConvertVolt_S,0x0091, "mV"	  },	// METHOD_MV1  10	// 電圧(0.01mV)
	{ ConvertVolt,	0x0092, "mV"	  },	// METHOD_MV2  11	// 電圧(0.1mV)
	{ ConvertVolt_S,0x0092, "mV"	  },	// METHOD_MV2  11	// 電圧(0.1mV)
	{ ConvertVolt,	0x0093, "mV"	  },	// METHOD_MV3  12	// 電圧(1mV)
	{ ConvertVolt_S,0x0093, "mV"	  },	// METHOD_MV3  12	// 電圧(1mV)
	{ ConvertPSDW,	0x004D, "pulse"	  },	// METHOD_PSDW 13	// パルス
	{ ConvertPSDW_S,0x004D, "pulse"	  },	// METHOD_PSDW 13	// パルス
	{ ConvertPSDW,	0x014D, "pulse"	  },	// METHOD_PSDW 13	// パルス
	{ ConvertPSDW_S,0x014D, "pulse"	  },	// METHOD_PSDW 13	// パルス
	{ ConvertPSDW,	0x024D, "pulse"	  },	// METHOD_PSDW 13	// パルス
	{ ConvertPSDW_S,0x024D, "pulse"	  },	// METHOD_PSDW 13	// パルス
	{ ConvertPSUP,	0x004E, "pulse"	  },	// METHOD_PSUP 14	// パルス
	{ ConvertPSUP_S,0x004E, "pulse"	  },	// METHOD_PSUP 14	// パルス
	{ ConvertPSUP,	0x014E, "pulse"	  },	// METHOD_PSUP 14	// パルス
	{ ConvertPSUP_S,0x014E, "pulse"	  },	// METHOD_PSUP 14	// パルス
	{ ConvertPSUP,	0x024E, "pulse"	  },	// METHOD_PSUP 14	// パルス
	{ ConvertPSUP_S,0x024E, "pulse"	  },	// METHOD_PSUP 14	// パルス
	{ ConvertCO2,	0x0042, "ppm"	  },	// METHOD_CO2  15	// CO2
};
// ※Code=0x0000は該当無しのチェックとして用いている事に注意！

///// 上記の構造体の順番にあわせること(８ビット以下とする)
enum {
	METHOD_NONE = 0,	// 該当無し
	METHOD_C,			// 摂氏
	METHOD_F,			// 華氏
	METHOD_HUM,			// 湿度
	METHOD_HUM2,		// 湿度(0.1%精度)
	METHOD_LX,			// 照度
	METHOD_LXH,			// 積算照度
	METHOD_MW,			// 紫外線強度
	METHOD_MWH,			// 積算紫外線強度
	METHOD_MA,			// 電流
	METHOD_MA_S,		// 電流
	METHOD_MV1,			// 電圧(0.01)
	METHOD_MV1_S,		// 電圧(0.01)
	METHOD_MV2,			// 電圧(0.1)
	METHOD_MV2_S,		// 電圧(0.1)
	METHOD_MV3,			// 電圧(1)
	METHOD_MV3_S,		// 電圧(1)
	METHOD_PSDW,		// 立ち下がりパルス
	METHOD_PSDW_S,		// 立ち下がりパルス
	METHOD_PSDW2,		// 総立ち下がりパルス
	METHOD_PSDW_S2,		// 総立ち下がりパルス
	METHOD_PSDW3,		// 時刻付き総立ち下がりパルス
	METHOD_PSDW_S3,		// 時刻付き総立ち下がりパルス
	METHOD_PSUP,		// 立ち上がりパルス
	METHOD_PSUP_S,		// 立ち上がりパルス
	METHOD_PSUP2,		// 総立ち上がりパルス
	METHOD_PSUP_S2,		// 総立ち上がりパルス
	METHOD_PSUP3,		// 時刻付き総立ち上がりパルス
	METHOD_PSUP_S3,		// 時刻付き総立ち上がりパルス
	METHOD_CO2,			// CO2
	METHOD_KGC,			// 騒音振動　KGC
};



//************************************************************

//====================================================================
// Method番号の取得
// ・属性(データ種別) 0x0D, 0xD0, 0x49 ...
// ・温度種別 0=oC, 1=oF (EEPROMの設定値と同じ）
// ・戻りはMethod。 MeTable[Method] で取得できる
//
//     Attrib    Type   Add   内容
//		0x0D      0      -     温度(oC)
//		0x0D      1      -     温度(oF)
//		0xD0	  -      -     湿度
//		0xD1	  -      -     湿度(0.1%精度)
//		0x49      -      0     照度
//		0x49      -      1     積算照度
//		0x55      -      0     紫外線強度
//		0x55      -      1     積算紫外線強度
//		0x81      -     0/1    電流
//		0x91      -     0/1    電圧
//		0x92      -     0/1    電圧
//		0x93      -     0/1    電圧
//		0x4D      -     0/1    パルス
//		0x4E      -     0/1    パルス
//		0x4D      -     2/3    総パルス
//		0x4E      -     2/3    総パルス
//		0x4D      -     4/5    時刻付き総パルス
//		0x4E      -     4/5    時刻付き総パルス
//		0x42      -      -     CO2
//		default	  -      -     該当無し
//====================================================================
int16_t	GetConvertMethod( uint8_t Attrib, uint8_t Type, uint8_t Add )
{
	int16_t		method;

	switch( Attrib ) {
		case 0x0D:	// 温度 oC または oF
            method = ( Type == 0 ) ? METHOD_C : METHOD_F ;
            break;

		case 0xD0:
            method = METHOD_HUM;		// 湿度(1%精度)
            break;

		case 0xD1:
		    method = METHOD_HUM2;		// 湿度(0.1%精度)
		    break;

		case 0x49:
		    method = ( Add == 0 ) ? METHOD_LX : METHOD_LXH;	// 照度 or 積算照度
		    break;

		case 0x55:
		    method = ( Add == 0 ) ? METHOD_MW : METHOD_MWH;	// 紫外線強度 or 積算紫外線強度
		    break;

		case 0x81:
		    method = ( Add == 0 ) ? METHOD_MA : METHOD_MA_S;		// 電流
		    break;

		case 0x91:
		    method = ( Add == 0 ) ? METHOD_MV1 : METHOD_MV1_S;		// 電圧
		    break;

		case 0x92:													// これしか使用していないかも
		    method = ( Add == 0 ) ? METHOD_MV2 : METHOD_MV2_S;		// 電圧
		    break;

		case 0x93:
		    method = ( Add == 0 ) ? METHOD_MV3 : METHOD_MV3_S;		// 電圧
		    break;

		case 0x9a:													// KGC
			//method = METHOD_MV1;
		    //method = ( Add == 0 ) ? METHOD_MV1 : METHOD_MV1_S;		// KGC 電圧と同じ計算
		    method = ( Add == 0 ) ? METHOD_MV2 : METHOD_MV2_S;		// KGC 電圧と同じ計算			// 505BLV 測定値の表示精度修正 sakaguchi 2021.04.07
			break;

		case 0x4D:
		    if ( Add & 0x04 ){			// 時刻付き総パルス
				method = ( Add & 1 ) ? METHOD_PSDW_S3 : METHOD_PSDW3;	// 総パルス
		    }
		    else if ( Add & 0x02 ){		// 総パルス
                method = ( Add & 1 ) ? METHOD_PSDW_S2 : METHOD_PSDW2;	// パルス
            }
            else{
                method = ( Add & 1 ) ? METHOD_PSDW_S : METHOD_PSDW;		// パルス
            }
            break;

		case 0x4E:
		    if ( Add & 0x04 ){			// 時刻付き総パルス
				method = ( Add & 1 ) ? METHOD_PSUP_S3 : METHOD_PSUP3;	// 総パルス
		    }else if ( Add & 0x02 ){		// 総パルス
				method = ( Add & 1 ) ? METHOD_PSUP_S2 : METHOD_PSUP2;	// パルス
		    }else{
		        method = ( Add & 1 ) ? METHOD_PSUP_S : METHOD_PSUP;		// パルス
		    }
			break;

		case 0x42:
		    method = METHOD_CO2;		// CO2
			break;

		default:
		    method = METHOD_NONE;		// 該当無し
			break;
	}

	return (method);
}


// ※unit_all を使用するところは、rtr501の前半を使用
/**
 * 子機の種類とチャネル数
 * @return
 * @note    regist 構造体が確定していること
 */
uint8_t GetChannels(void)
{
	uint8_t	ch = 0;

	if ( ru_registry.rtr501.header == 0xFA ) {		// 574 ?
		ch = 6;
	}
	if ( ru_registry.rtr501.header == 0xF9 ) {		// 576 ?
		ch = 6;		// 子機情報の読み込み位置を決定するだけなので574にあわせておく
	}
	else {
		if ( ru_registry.rtr501.attribute[1] == 0 ){			// RTR-501/502
			ch = 1;
		}
		else if ( ru_registry.rtr501.attribute[0] == 0x0D ){	// RTR-503
			ch = 2;
		}
	}

	return (ch);
}


/**
 ******************************************************************************
    @brief      温度の素データを°Cへ変換する（小数第一位まで）
                    " Data -> Big:整数部(符号付き) Small:小数部(0～9のみ) "
    @param      *Dst        変換先ポインタ
    @param      Src     元データ
    @return     変換した文字数
    @attention  この変換は必ずこれを通す事
    @note   温度0を割る可能性ありのため修正 2018.Jan.08
    @note       呼び出し元       ConvertAttrib()
 ******************************************************************************
*/
static int16_t ConvertTempC( char *Dst, uint32_t Src )
{
	uint16_t	Data = (uint16_t)Src;
	int16_t		Temp = (int16_t)Data;

    Temp = (int16_t)( Temp - 1000);
    if( Temp == 0 ) {       //0の時
        Data = 0;
        Temp = 0;
        Temp = (int16_t)sprintf( Dst, "%hu.%1hu", (uint16_t)Temp, Data);
    }
    else if ( Temp > 0 )	// プラス値
	{
		Data = (uint16_t)( Temp % 10 );		// 小数部
		Temp /=  10;					// 整数部
		Temp = (int16_t)sprintf( Dst, "%u.%1u", (uint16_t)Temp, Data );
	}
	else				// マイナス値
	{
		Temp = (int16_t)(-Temp);						// 符号反転（絶対値）
		Data = (uint16_t)( Temp % 10 );		// 小数部
		Temp /=  10;					// 整数部
		Temp = (int16_t)sprintf( Dst, "-%u.%1u", (uint16_t)Temp, Data );
	}

	return (Temp);
}

/**
 ******************************************************************************
    @brief      温度の素データを°Fへ変換する（小数第一位まで）
                    " Data -> Big:整数部(符号付き) Small:小数部(0～9のみ) "
    @param      *Dst        変換先ポインタ
    @param      Src     元データ
    @return     変換した文字数
    @attention  この変換は必ずこれを通す事
    @note       呼び出し元       ConvertAttrib()
 ******************************************************************************
*/
static int16_t ConvertTempF( char *Dst, uint32_t Src )
{
	uint16_t	Data = (uint16_t)Src;
	int32_t	Temp;
	int16_t	Big;
	char	sign;

	// Temp = ( Data - 1000 ) / 10
	// Temp = Temp * 90 + 1600		// ( Temp = Temp * 1.8 + 32 の50倍 )
	// Temp = Temp / 50;

	Temp = ( ((int32_t)( (int16_t)Data - 1000 )) * 9 ) + 1600;		// 整数で計算するために5倍値で行う
	
	sign = ( Temp >= 0 ) ? 0 : 1;		// マイナス値フラグ

	if ( sign )
		Temp = -Temp;					// 符号反転（絶対値）

	Big  = (int16_t)( Temp / 50 );			// 整数部
	Data = (uint16_t)( Temp % 50 );
	Data = Data / 5;					// 小数部

	if ( Big == 0 && Data == 0 ){
		sign = 0;						// 0.0 はプラス扱い
	}

	if ( sign ){
		Big = (int16_t)sprintf( Dst, "-%u.%1u", (uint16_t)Big, Data );
	}else{
		Big = (int16_t)sprintf( Dst, "%u.%1u", (uint16_t)Big, Data );
	}
	return (Big);
}


/**
 ******************************************************************************
    @brief      湿度の素データを%へ変換する（小数部なし）
    @param      *Dst        変換先ポインタ
    @param      Src     元データ
    @return     変換した文字数
    @attention  この変換は必ずこれを通す事
    @note       呼び出し元       ConvertAttrib()
 ******************************************************************************
*/
static int16_t ConvertHum( char *Dst, uint32_t Src )
{
	uint16_t	Data = (uint16_t)Src;

	Data = (uint16_t)(( Data - 1000 ) / 10);

	Data = (uint16_t)sprintf( Dst, "%d", Data );

	return ((int16_t)Data);
}

/**
 ******************************************************************************
    @brief      湿度の素データを%へ変換する（小数第一位まで）
    @param      *Dst        変換先ポインタ
    @param      Src     元データ
    @return     変換した文字数
    @attention  この変換は必ずこれを通す事
    @note       呼び出し元       ConvertAttrib()
 ******************************************************************************
*/
static int16_t ConvertHum2( char *Dst, uint32_t Src )
{
	uint16_t	Data = (uint16_t)Src;
	int16_t		Temp;

	Temp = (int16_t)(Data - 1000);
	Data = (uint16_t)( Temp % 10 );		// 小数部
	Temp = Temp / 10;				// 整数部

	Temp = (int16_t)sprintf( Dst, "%d.%1u", Temp, Data );

	return (Temp);
}



/**
 * 照度の素データを変換する（有効桁数4桁・小数第2位まで）
 * Data -> Big:整数部 Small:小数部(00～99 ※100分の1であることに注意)
 * @param Dst
 * @param Src
 * @return
 * @attention   この変換は必ずこれを通す事
 */
static int16_t ConvertLx( char *Dst, uint32_t Src )
{
	uint16_t	Data = (uint16_t)Src;
	uint16_t	man, rad;
	int16_t		num, pos;
	uint32_t	tmp;

	rad = Data >> 12;				// 指数部(この数分左へシフト）
	man = Data & 0x0FFF;			// 仮数部

	tmp = (uint32_t)man << rad;
	rad = (uint16_t)( tmp % 100 );		// 小数部
	tmp = tmp / 100;				// 整数部

	if ( tmp >= 1000 ) {			// 4桁以上？

		num = (int16_t)sprintf( Dst, "%lu", tmp );	// 文字数保存
		pos = (int16_t)(num - 1);
		while( pos >= 4 ) {			// 有効桁数を超えた？
			Dst[ pos ] = '0';		// 切り捨て
			pos--;
		}

//		tmp = AtoL( ConvertTemp, 10 );	// 数値へ変換
//		rad = 0;						// 小数部 = 0

	}
	else if ( tmp >= 100 ) {		// 3桁以上？
		rad /= 10;					// 小数部1桁切り捨て
		num = (int16_t)sprintf( Dst, "%lu.%.1u", tmp, rad );
//		rad *= 10;					// 例) 56 -> 50
	}
	else {
		num = (int16_t)sprintf( Dst, "%lu.%.2u", tmp, rad );
	}

//	*Big = tmp;						// 整数部
//	*Small = rad;					// 小数部

	return (num);
}



/**
 * 積算照度の素データを変換する（有効4桁・小数なし）
 * Data -> Big:整数部のみ
 * @param Dst
 * @param Src
 * @return
 * @attention   この変換は必ずこれを通す事
 */
static int16_t ConvertSumLx( char *Dst, uint32_t Src )
{
	uint16_t	Data = (uint16_t)Src;
	uint32_t	tmp;
	int16_t		num, pos;

//	rad = Data >> 12;				// 指数部(この数分左へシフト）
//	man = Data & 0x0FFF;			// 仮数部

	tmp = (uint32_t)( Data & 0x0FFF ) << ( Data >> 12 );

	if ( tmp >= 10000 ) {			// 5桁以上？

		num = (int16_t)sprintf( Dst, "%lu", tmp );
		pos = (int16_t)(num - 1);
		while( pos >= 4 ) {				// 有効桁数を超えた？
			Dst[ pos ] = '0';	// 切り捨て
			pos--;
		}

//		tmp = AtoL( ConvertTemp, 10 );	// 数値へ変換

	}
	else
		num = (int16_t)sprintf( Dst, "%lu", tmp );

//	*Big = tmp;						// 整数部

	return (num);
}



/**
 * 紫外線の素データを変換する（有効桁数4桁・小数第3位まで）
 * Data -> Big:整数部 Small:小数部(000～999 ※1000分の1であることに注意)
 * @param Dst
 * @param Src
 * @return
 *  * @attention   この変換は必ずこれを通す事
 */
static int16_t ConvertUV( char *Dst, uint32_t Src )
{
	uint16_t 	Data = (uint16_t )Src;
	uint16_t	man, rad;
	int16_t		num;
	uint32_t	tmp;

	rad = Data >> 12;				// 指数部(この数分左へシフト）
	man = Data & 0x0FFF;			// 仮数部

	tmp = (uint32_t)man << rad;
	rad = (uint16_t)( tmp % 1000 );		// 小数部
	tmp = tmp / 1000;				// 整数部

	if ( tmp >= 1000 ) {			// 4桁以上？

		num = (int16_t)sprintf( Dst, "%lu", tmp );
//		pos = (int16_t)(num - 1);                       //未使用コメントアウト 2020.Jan.156
		while( num >= 4 ) {				// 有効桁数を超えた？
			Dst[ num ] = '0';	// 切り捨て
			num--;
		}

//		tmp = AtoL( ConvertTemp, 10 );	// 数値へ変換
//		rad = 0;						// 小数部 = 0

	}
	else if ( tmp >= 100 ) {		// 3桁以上？
		rad /= 100;					// 小数部2桁切り捨て
		num = (int16_t)sprintf( Dst, "%lu.%.1u", tmp, rad );
//		rad *= 100;					// 例) 987 -> 900
	}
	else if ( tmp >= 10 ) {			// 2桁以上？
		rad /= 10;					// 小数部1桁切り捨て
		num = (int16_t)sprintf( Dst, "%lu.%.2u", tmp, rad );
//		rad *= 10;					// 例) 987 -> 980
	}
	else{
		num = (int16_t)sprintf( Dst, "%lu.%.3u", tmp, rad );
	}
//	*Big = tmp;						// 整数部
//	*Small = rad;					// 小数部

	return (num);
}


/**
 * 積算照度の素データを変換する（小数第3位まで）
 * Data -> Big:整数部 Small:小数部(000～999)
 * @param Dst
 * @param Src
 * @return
 * @attention   この変換は必ずこれを通す事
 */
static int16_t ConvertSumUV( char *Dst, uint32_t Src )
{
	uint16_t 	Data = (uint16_t )Src;

	return (ConvertUV( Dst, Data ));
}


/**
 * 電流の素データを変換する（小数第2位まで）
 * @param Dst
 * @param Src
 * @return
 * @attention   この変換は必ずこれを通す事
 */
static int16_t ConvertAmp( char *Dst, uint32_t Src )
{
	uint16_t 	Data = (uint16_t )Src;
	uint16_t	rad, sig, man;
	int16_t		num;
	int32_t	tmp;

	sig = Data & 0x8000;			// 符号部
	rad = ( Data >> 12 ) & 0x0007;	// 指数部(この数分左へシフト）
	man = Data & 0x0FFF;			// 仮数部(0～4095)

	tmp = ( sig ) ? 4096 - man : man;	// ＋の値へ
	tmp <<= rad;						// 指数部シフト

	man = (uint16_t)( tmp / 100 );		// 整数部
	rad = (uint16_t)( tmp % 100 );		// 小数部

//	if ( sig )
//		man = -man;					// マイナス値へ
/*
	if ( sig ){
		num = (int16_t)sprintf( Dst, "-%u.%02u", man, rad );
	}else{
		num = (int16_t)sprintf( Dst, "%u.%02u", man, rad );
	}
	*/
	num = (int16_t)(( sig ) ? sprintf( Dst, "-%u.%02u", man, rad ) : sprintf( Dst, "%u.%02u", man, rad ));
	return (num);

}


/**
 * 電流の素データを変換する（スケール変換付き）
 * @param Dst
 * @param Src
 * @return
 * @attention   この変換は必ずこれを通す事
 */
static int16_t ConvertAmp_S( char *Dst, uint32_t Src )
{
	int16_t	num;
	char	*a, *b, *unit;
	int16_t		val;

	// regist.unit505.scale_conv が確定していること

	val = CheckScaleStr( &a, &b, &unit );

	if ( val < 0 )	{						// 変換無し
		return (ConvertAmp( Dst, Src ));		// 変換無しで処理して終了
	}
	// 変換あり
	num = ConvertAmp( PreScale, Src );		// まずはソース変換
	if ( num ){
		num = Mul10Add( Dst, a, PreScale, b, val );		// 変換
//		num = ScaleChange( Dst, a, PreScale, b, val );	// 実変換
	}
	return (num);
}


/**
 * 電圧の素データを変換する（小数第1位まで）
 * 0.01mV, 0.1mV, 1mV 単位
 * @param Dst
 * @param Src
 * @return
 */
static int16_t ConvertVolt( char *Dst, uint32_t Src )
{
	uint16_t 	Data = (uint16_t )Src;
	uint16_t	rad, sig, man;
	int16_t		num;
	int32_t	tmp;

	sig = Data & 0x8000;			// 符号部
	rad = ( Data >> 12 ) & 0x0007;	// 指数部(この数分左へシフト）
	man = Data & 0x0FFF;			// 仮数部(0～4095)

	tmp = ( sig ) ? 4096 - man : man;	// ＋の値へ
	tmp <<= rad;						// 指数部シフト

	man = (uint16_t)( tmp / 10 );		// 整数部
	rad = (uint16_t)( tmp % 10 );		// 小数部

//	if ( sig )
//		man = -man;					// マイナス値へ

	if ( sig ){						// マイナス?
		num = (int16_t)sprintf( Dst, "-%u.%u", man, rad );
	}
	else{
	    num = (int16_t)sprintf( Dst, "%u.%u", man, rad );
	}

	return (num);
}


/**
 * 電圧の素データを変換する（小数第1位まで）
 * 0.01mV, 0.1mV, 1mV 単位
 * @param Dst
 * @param Src
 * @return
 */
static int16_t ConvertVolt_S( char *Dst, uint32_t Src )
{
	int16_t		num;
	char	*a, *b, *unit;
	int16_t		val;

	// regist.unit505.scale_conv が確定していること

	val = CheckScaleStr( &a, &b, &unit );

	if ( val < 0 ){							// 変換無し
		return ( ConvertVolt( Dst, Src ));		// 変換無しで処理して終了
	}
	// 変換あり
	num = ConvertVolt( PreScale, Src );		// まずはソース変換
	if ( num ){
		num = Mul10Add( Dst, a, PreScale, b, val );		// 変換
//		num = ScaleChange( Dst, a, PreScale, b, val );	// 実変換
	}
	return (num);
}


/**
 * パルス（ダウン）
 * @param Dst
 * @param Src
 * @return
 */
static int16_t ConvertPSDW( char *Dst, uint32_t Src )
{
	return ((int16_t)sprintf( Dst, "%lu", Src ));
}


/**
 * パルス（ダウン）
 * @param Dst
 * @param Src
 * @return
 */
static int16_t ConvertPSDW_S( char *Dst, uint32_t Src )
{
	int16_t		num;
	char	*a, *b, *unit;
	int16_t		val;

	// regist.unit505.scale_conv が確定していること

	val = CheckScaleStr( &a, &b, &unit );

	if ( val < 0 ){	    // 変換無し
		return (ConvertPSDW( Dst, Src ));		// 変換無しで処理して終了
	}
	// 変換あり
	num = ConvertPSDW( PreScale, Src );		// まずはソース変換
	if ( num ){
		num = Mul10Add( Dst, a, PreScale, b, val );		// 変換
//		num = ScaleChange( Dst, a, PreScale, b, val );	// 実変換
	}
	return (num);
}


/**
 * パルス（ダウン）
 * @param Dst
 * @param Src
 * @return
 */
static int16_t ConvertPSUP( char *Dst, uint32_t Src )
{
	return ((int16_t)sprintf( Dst, "%lu", Src ));
}


/**
 * パルス（ダウン）
 * @param Dst
 * @param Src
 * @return
 */
static int16_t ConvertPSUP_S( char *Dst, uint32_t Src )
{
	int16_t		num;
	char	*a, *b, *unit;
	int16_t		val;

	// regist.unit505.scale_conv が確定していること

	val = CheckScaleStr( &a, &b, &unit );

	if ( val < 0 ){							// 変換無し
		return (ConvertPSUP( Dst, Src ));		// 変換無しで処理して終了
	}
	// 変換あり
	num = ConvertPSUP( PreScale, Src );		// まずはソース変換
	if ( num ){
		num = Mul10Add( Dst, a, PreScale, b, val );		// 変換
//		num = ScaleChange( Dst, a, PreScale, b, val );	// 実変換
	}
	return (num);
}


/**
 * CO2
 * @param Dst
 * @param Src
 * @return
 */
static int16_t ConvertCO2( char *Dst, uint32_t Src )
{
	uint16_t 	Data = (uint16_t )Src;
	uint16_t	rad, sig, man;
	int16_t		num;
	int32_t	tmp;

	sig = Data & 0x8000;			// 符号部
	rad = ( Data >> 12 ) & 0x0007;	// 指数部(この数分左へシフト）
	man = Data & 0x0FFF;			// 仮数部(0～4095)

	tmp = ( sig ) ? 4096 - man : man;	// ＋の値へ
	tmp <<= rad;						// 指数部シフト

	if ( sig )
		tmp = -tmp;					// マイナス値へ

	//num = (int16_t)sprintf( Dst, "%lu", tmp );  ///< @bug sprintf　で int -> unsigned　long
    num = (int16_t)sprintf( Dst, "%ld", tmp );  // CO2は子機の設定や校正によりマイナスになる場合があります	// 2023.06.19

	return (num);
}

/**
 * どこにも該当しない場合はここ
 * @param Dst
 * @param Src
 * @return
 */
static int16_t ConvertNone( char *Dst, uint32_t Src )
{
	return ((int16_t)sprintf( Dst, "0x%08lX", Src ));
}



/**
 * スケール変換のデータを１つ取り出す
 * ScaleString[]へ入れる
 * 最後の\nはnullになる
 * 最初は Num=0
 * @param Src
 * @param Num
 * @return
 */
bool GetScaleStr( char *Src, int Num )
{
	char	part, data, *ptr = ScaleString;

	int pos = 0;

	// 何個目のスケール変換文字列だろうか？
	if ( Num > 0 )
	{
		part = 0;
		for ( pos= 0; pos< (int)(sizeof(ru_registry.rtr505.scale_setting)-1); pos++ )
		{
			if ( SCALE_TERM == *Src++ )
			{
				part++;
				if ( part >= 4 ) {
					part=0;
					Num--;
					if ( Num <= 0 )
						break;
				}
			}
		}
	}

	if ( Num == 0 )
	{	// posは初期化しないでおく
		for ( part=0; pos<(int)(sizeof(ru_registry.rtr505.scale_setting)-1); pos++ )
		{
			data = *Src++;
			*ptr++ = data;					// コピー
			if ( SCALE_TERM == data )
			{
				part++;
				if ( part >= 4 ) {
					*( ptr - 1 ) = 0;		// 最後の\nはnull
					pos = 0;				// OKフラグ
					break;
				}
			}
		}
	}

	if ( pos == 0 ){
		return (true);
	}
	else{
		strcpy( ScaleString, SCALE_DEFAULT );
		return (false);
	}
}


/**
 * スケール変換のチェック
 *  y = A * x + B  [Unit] のポインタへ入れる
 * @param A
 * @param B
 * @param Unit
 * @return      戻り値は有効桁数(0-9)。マイナスは変換無しまたはエラー
 * @note    ScaleString[]を使う
 * @attention   呼び出し前にGetScaleStr()を呼んでおくこと
 */
int16_t CheckScaleStr( char **A, char **B, char **Unit )
{
	char	*ptr, part, pos, data, *src = ScaleString;
	int		count, val = -1;
	static const char *const def = " \x00";
//	double	x;

	if ( !strcmp( SCALE_DEFAULT, src ) ) {
		*Unit = (char *)def;
		return (-1);							// スケール変換無し
	}

	ptr = src;
	*A = *B = *Unit = 0;
	pos = part = 0;

	for ( count=0; count<(int)(sizeof(ru_registry.rtr505.scale_setting)-1); count++ )
	{
		data = *src++;
		if ( SCALE_TERM == data || data == 0 ) {		// \n or \0 ?
//			*( start - 1 ) = 0;			// null置き換え
			pos++;
			switch ( pos ) {
				case 1:
					if ( part > 0 ){
					    *A = ptr;			// 傾き
					}
					else{
					    goto exit_check;	// エラー
					}
					break;
				case 2:
					if ( part > 0 ){
					    *B = ptr;			// 切片
					}
					else{
					    goto exit_check;	// エラー
					}
					break;
				case 3:
					if ( part > 0 && part <= 3 ){		// 3桁以上はエラーとする(0003とかもダメ)
						val = AtoI( ptr, 3 );			// 3桁まで
					}
					else{
					    goto exit_check;				// エラー
					}
					break;
				case 4:						// 単位無しもそのまま扱う
					*Unit = ptr;			// 傾き
					*( src - 1 ) = 0;		// 最後の\nをnullに
					goto exit_check;		// 4つで終了
			}
			part = 0;
			ptr = src;		// 次の項目の先頭
		}
		else{
		    part++;
		}

		if ( data == 0 ){
		    break;			// nullは終了
		}
	}

exit_check:

	count = -1;

	if ( pos == 4 && val >= 0 && val <= 9 ) {		// 9桁までしかダメとする
		// チェックは呼び出し側でやる
		count = val;						// 正常終了
	}

	return ((int16_t)count);		// 文字数
}	


/**
 * スケール変換のフォーマット化
 * \n 4つに適用させる
 * @param Dst
 * @param Src
 */
void FormatScaleStr( char *Dst, char *Src )
{
	char	data;
	uint32_t pos, i;

	for ( i = pos = 0; i < sizeof( FormatScale )-6; i++ )
	{
		data = *Src++;
		*Dst++ = data;

		if ( data == SCALE_TERM )
			pos++;
		else if ( data == 0 ) {
			*( Dst - 1 ) = SCALE_TERM;
			pos++;
			break;
		}

		if ( pos >= 4 ){
		    break;
		}
	}

	for ( i=pos; i<4; i++ ){		// 4つに足りない分を加算
		*Dst++ = SCALE_TERM;
	}
	*Dst = 0;						// 終端
}

/**
 * 10進 掛け算（文字列×文字列）用構造体
 */
static struct {
	char	Src[8+1+1];				// 8桁 + null + 予備
	char	Targ[8+1+1];			// 8桁 + null + 予備
//	int		DecLen;					// 小数以下の桁数
//	uint32_t	MulAns[2][2];
	char	Ans[8*2+1+1+1+1];
} MUL;


/**
 * @brief   10進 掛け算（文字列×文字列）
 * ・doubleの精度不足のためこちら。
 * ・指数表現は不可
 * ・8桁×8桁に対応
 * ・スタックメモリ消費に注意！！（結構食ってる）
 * ・指定した桁数(Valid)まで0が必ず入る
 * ・Valid = 0のときは計算された桁まで出力
 * @param Dst   展開先
 * @param Src1  src1
 * @param Src2  src2
 * @param Valid 全体の有効桁数
 * @param Dec   少数以下桁数
 * @return
 * @note     小数以下桁数を指定すると常にその桁分の小数が付加される。 ただし、有効桁数以降は０となる
 */
int16_t Mul10( char *Dst, char *Src1, char *Src2, int Valid, int Dec )
{
	int16_t		pos, len, size = 0, dec_len = 0;
	char	data, pod, sign, *mem;
	uint32_t	targ, ans1, ans2, ans3, ans4, src1, src2;
	uint16_t 	out1, out2, out3, out4;

	sign = 0;
	if ( *Src1 == '-' ){
	    sign = 1, Src1++;					// マイナス
	}
	else if ( *Src1 == '+' ){
	    sign = 0, Src1++;					// プラス
	}

	if ( *Src2 == '-' ) {
		sign = sign ? 0 : 1;				// 掛け算後の符号
//		*Src2++;
		Src2++;     //2020.01.21 ポインタのみ進める
	}
	else if ( *Src2 == '+' ){
	    Src2++;								// プラス
	}

	out1 = 0;			// 小数点以下の総桁数（間借り）

	// 元の数
	mem = Src1;
	for ( len=0; len<8+1+1; len++ )			// 8桁まで(ピリオド含む)
	{
		data = *mem++;
		if ( data == 0 || data == SCALE_TERM ){
		    break;				// 終了
		}
	}
	if ( len > 8+1 || len == 0 ){
	    goto exit_mul;					// 抜ける
	}

	sprintf( MUL.Src, "%.8u", 0 );		// まずは0で埋めておく
	mem -= 2;							// 最後の１文字にあるはず
	Src1 = &MUL.Src[8-1];				// 最後の位置へ
	dec_len = 0;

	for ( pos=pod=0; pos<len; pos++ )	// 8桁までへ(ピリオド削除)
	{
		data = *mem--;

		if ( data == '.' ){
		    pod = 1;					// 小数点以上
		}

		else if ( data >= '0' && data <= '9' ) {
			*Src1-- = data;
			if ( !pod )
				dec_len++;
		}
		else{
		    goto exit_mul;					// 抜ける
		}
	}
	if ( !pod )	{
	    dec_len = 0;			// ピリオド無しの場合
	}

	out1 = (uint16_t)dec_len;
	
	// かける数
	mem = Src2;
	for ( len=0; len<8+1+1; len++ )			// 8桁まで(ピリオド含む)
	{
		data = *mem++;
		if ( data == 0 || data == SCALE_TERM ){
		    break;				// 終了
		}
	}
	if ( len > 8+1 || len == 0 ){
	    goto exit_mul;					// 抜ける
	}

	sprintf( MUL.Targ, "%.8u", 0 );		// まずは0で埋めておく
	mem -= 2;							// 最後の１文字にあるはず
	Src2 = &MUL.Targ[8-1];				// 最後の位置へ
	dec_len = 0;

	for ( pos=pod=0; pos<len; pos++ )	// 8桁までへ(ピリオド削除)
	{
		data = *mem--;

		if ( data == '.' ){
		    pod = 1;					// 小数点以上
		}
		else if ( data >= '0' && data <= '9' ) {
			*Src2-- = data;
			if ( !pod ){
			    dec_len++;
			}
		}
		else{
		    goto exit_mul;					// 抜ける
		}
	}
	if ( !pod )	{
	    dec_len = 0;			// ピリオド無しの場合
	}

	dec_len = (int16_t)(dec_len + out1);					// 小数点以下の総桁数
	

	// ここまでで
	// 01234567 * 0000234 みたいになってるはず

	src1 = (uint32_t)AtoL( &MUL.Src[4], 4 );
	src2 = (uint32_t)AtoL( &MUL.Src[0], 4 );

	// 最後の４桁
	targ = (uint32_t)AtoL( &MUL.Targ[4], 4 );		// ４桁ずつ
	ans1 = src1 * targ;
	ans2 = src2 * targ;

	// 最初の４桁
	targ = (uint32_t)AtoL( &MUL.Targ[0], 4 );		// ４桁ずつ
	ans3 = src1 * targ;
	ans4 = src2 * targ;

	// 結果を加算する（４桁区切りとなる）
	out1  = (uint16_t )( ans1 % 10000UL );

	out2  = (uint16_t )( ans1 / 10000UL );
	out2 = (uint16_t )(out2 +  ans2 % 10000UL );
	out2 = (uint16_t )(out2 + ans3 % 10000UL );

	out3  = (uint16_t )( ans2 / 10000UL );
	out3 = (uint16_t )(out3 + ans3 / 10000UL );
	out3 = (uint16_t )(out3 + ans4 % 10000UL );

	out4  = (uint16_t )( ans4 / 10000UL );

	out3 = (uint16_t )(out3 + out2 / 10000UL );		// 繰り上がり
	out4 = (uint16_t )(out4 + out3 / 10000UL );		// 繰り上がり

	out2 = (uint16_t )( out2 % 10000UL );
	out3 = (uint16_t )( out3 % 10000UL );

	if ( !( out1 | out2 | out3 | out4 ) ) {		// 0?
		*Dst++ = '0';							// 0
		size++;
		goto exit_mul;							// ゼロ
	}

	if ( sign ) {
		*Dst++ = '-';				// 符号
		size++;
	}

	// いったん展開
	sprintf( MUL.Ans, "%.4u%.4u%.4u%.4u", out4,out3,out2,out1 );

	for ( pos=0; pos<dec_len; pos++ ) {		// 小数点以下の
		if ( MUL.Ans[16-1-pos] == '0' ){		// 最後の０を除去
			MUL.Ans[16-1-pos] = 0;
		}
		else{
		    break;
		}
	}

	mem = MUL.Ans;
	pod = 0;
	dec_len = (int16_t)(16 - dec_len);					// ピリオドの場所

	for ( pos=0; pos<dec_len; pos++ ) {		// ピリオド位置まで０検索
		if ( *mem != '0' ){
		    break;
		}
		else{
		    mem++;
		}
	}
	if ( pos == dec_len ) {				// 最後まで到達？
		*Dst++ = '0';					// 0
		size++;
	}

	// 実体の最終展開

	Valid = ( Valid <= 0 ) ? -1 : Valid;	// 全体の有効桁数
	Dec = ( Dec <= 0 ) ? -1 : Dec;			// 少数以下桁数

	for ( ; pos<16; pos++ ) {

		data = *mem++;
		if ( data == 0 ){			// 途中終了
			break;
		}
		if ( pos == dec_len ) {
			if ( !Valid ){
			    break;
			}
			pod = 1;
			*Dst++ = '.';
			size++;
		}

		if ( Valid == 0 ){			// 有効桁数以上は０
			data = '0';
		}
		*Dst++ = data;
		size++;

		if ( pod && Dec > 0 ){
		    Dec--;
		}

		if ( Valid > 0 ){
		    Valid--;				// 0で止める
		}

		if ( ( !Valid && pod ) || ( !Dec && pod ) ){
		    break;
		}
	}
/*
	if ( Dec > 0 )
		Valid = Dec;
	else if ( Dec == 0 )
		Valid = 0;
	else
		Valid = Valid;
*/

	Valid = ( Dec >= 0 ) ? Dec : Valid;

	if ( Valid > 0 ) {		// 残っていれば0を付加

		if ( !pod ) {		// ピリオド無し？
			*Dst++ = '.';			// 
			size++;
		}
		while ( Valid > 0 ) {
			*Dst++ = '0';			// 0
			size++;
			Valid--;
		}
	}

exit_mul:

	*Dst++ = 0;		// エラーでも必ずNULLが入る

	return (size);
}



/**
 * @brief   10進 掛け算＋足し算（文字列×文字列＋文字列）
 * @param Dst   計算結果（文字列）
 * @param Src1     ソース1（文字列）
 * @param Src2     ソース2（文字列）
 * @param Src3     ソース3（文字列）
 * @param Valid 桁数
 * @return      計算結果のサイズ
 * @note    doubleの精度不足のためこちら。
 * @note    指数表現は不可
 * @note    正当性はチェックしていない
 * @note    指定した桁数(Valid)まで0が必ず入る
 * @note    Valid=0は最後の0が付かない
 * @attention   スタックメモリ消費に注意！！（結構食ってる）
 * @note    2020.01.21  pow(),abs()を標準ライブラリ使用に変更
 */
int16_t Mul10Add( char *Dst, char *Src1, char *Src2, char *Src3, int Valid )
{
	int64_t	src1_int = 0, src1_dec = 0, src2_int = 0, src2_dec = 0, tmp;     //2020.01.21 uint32_t -> int64_t
	double exp;     //2020.01.21 uint32_t -> double
	int		dec1_len = 0, dec2_len = 0, pos, size = 0;
	char	data, pod, sign1, sign2;


	Mul10( MulAddTemp, Src1, Src2, 0, 0 );		// まず掛け算

	Src1 = MulAddTemp;
	Src2 = Src3;

	sign1 = ( *Src1 == '-' ) ? 1 : 0 ;
	sign2 = ( *Src2 == '-' ) ? 1 : 0 ;

//	src1_int = (uint32_t)AtoL( Src1, 10 );				// ピリオドまで
	src1_int = AtoL( Src1, 10 );				// ピリオドまで			// sakaguchi 2021.11.09 負の値もキャストしてしまうため(uint32_t)を削除

	for ( pos=0; pos<12; pos++ )		// 整数部10桁まで(符号+ピリオド含む)
	{
		data = *Src1++;

		if ( data == 0 || data == SCALE_TERM ) {
			pod = 1;
			break;						// 終了
		}

		if ( data == '.' ) {
			pod = 2;					// 小数点以下を処理
			break;
		}
	}

	if ( pod == 2 )
	{
		src1_dec = AtoL( Src1, 10 );		// ピリオド以降

		for ( pos=0; pos<10+1; pos++ )		// 小数部＝8桁まで
		{
			data = *Src1++;

			if ( data == 0 || data == SCALE_TERM ) {
				pod = 3;			// 正常終了
				break;
			}

			dec1_len++;
		}
	}

	if ( pod == 1 || pod == 3 )		// 正常終了
	{

		src2_int = AtoL( Src2, 10 );		// ピリオドまで
		pod = 0;

		for ( pos=0; pos<12; pos++ )		// 整数部10桁まで(符号+ピリオド含む)
		{
			data = *Src2++;

			if ( data == 0 || data == SCALE_TERM ) {
				pod = 1;
				break;				// 終了
			}

			if ( data == '.' ) {
				pod = 2;			// 小数点以下を処理
				break;
			}
		}
		
		if ( pod == 2 )
		{
			src2_dec = AtoL( Src2, 10 );	// ピリオド以降

			for ( pos=0; pos<10+1; pos++ )		// 小数部＝10桁まで
			{
				data = *Src2++;

				if ( data == 0 || data == SCALE_TERM ) {
					pod = 3;		// 正常終了
					break;
				}

				dec2_len++;
			}
		}

		if ( pod == 1 || pod == 3 )				// 足し算の数
		{

//			src1_int = ABS(((LONG)src1_int));	// 絶対値へ
//			src2_int = ABS(((LONG)src2_int));	// 絶対値へ
            src1_int = llabs(src1_int);   //stdlib.hの標準abs()に変更してみた 2020.01.21
            src2_int = llabs(src2_int);   //stdlib.hの標準abs()に変更してみた 2020.01.21


			if ( !src1_int && !src1_dec ){			// 0は符号を＋へ
				sign1 = 0;
			}
			if ( !src2_int && !src2_dec ){
			    sign2 = 0;
			}

			pos = (int16_t)(dec2_len - dec1_len);
//			pos = ABS( pos );
			pos = abs( pos);    //stdlib.hの標準abs()に変更してみた 2020.01.21
			if ( pos ) {						// 桁数に差がある？
				exp = pow(  (double)10, (double)pos );
				if ( dec2_len > dec1_len ) {
					src1_dec *= (uint32_t)exp;			// 桁数をあわせる
					dec1_len = dec2_len;
				}
				else{
				    src2_dec *= (uint32_t)exp;			// 桁数をあわせる
				}
			}

//			if ( dec1_len ) {

				if ( sign1 == sign2 )				// 符号が同じなら加算
				{
					src1_int += src2_int;			// 整数部加算
					src1_dec += src2_dec;			// 加算
					pos = (int16_t)sprintf( MulAddTemp, "%lu", (uint32_t)src1_dec );	// 一度展開
					if ( dec1_len && pos > dec1_len ) {			// 繰り上がり？
						src1_int++;					// 整数部インクリメント
						src1_dec = (uint32_t)AtoL( &MulAddTemp[1], 10 );		// 最上位桁削除
					}
				}
				else	// 符号が違う場合は減算
				{
					if ( ( src2_int > src1_int ) || 
						 ( src2_int == src1_int && src2_dec > src1_dec ) )
					{
						sign1 = !sign1;				// 符号が反転する場合は
						tmp = src1_int;				// 入れ替えて演算
						src1_int = src2_int;
						src2_int = tmp;				// swap
						tmp = src1_dec;
						src1_dec = src2_dec;
						src2_dec = tmp;				// swap
					}

					src1_int -= src2_int;			// 整数部減算
					src1_dec -= src2_dec;			// 減算
//                    if ( (LONG)src1_dec < 0 ) {     // 繰り下がり
                    if ( (signed long)src1_dec < 0 ) {     // 繰り下がり  //TODO for 64 bit linux_?
						src1_int--;					// 整数部デクリメント
						exp = pow( (double)10, (double)dec1_len );
						src1_dec += (uint32_t)exp;			// プラス値へ
					}

				}
//			}

			// 結果で判断
			if ( !src1_int && !src1_dec ){			// 0は符号を＋へ
				sign1 = 0;
			}
			// 文字列へ展開
			if ( sign1 ) {
				*Dst++ = '-';
				size++;
			}

			pos = (int16_t)sprintf( Dst, "%lu", (uint32_t)src1_int );	// 整数部
			Dst += pos;
			size += pos;

			if ( !Valid ) {
				pod = 0;
				Valid = 10;
			}
			else {
				pod = 1;
				if ( src1_int == 0 ){				// 整数部が0のとき
					Valid++;						// 桁数を+1
				}
			}

			if ( /*dec1_len &&*/ pos < Valid ) {

				*Dst++ = '.';
				size++;
				Valid =(int16_t)((int32_t)Valid - pos);
//				dec1_len = ( dec1_len ) ? dec1_len : 10;	// 0は10へ

				if ( dec1_len )
				{
					dec2_len = (int16_t)sprintf( MulAddTemp, "%.10lu", (uint32_t)src1_dec );	// 0埋め

					if ( src1_int == 0 )				// 整数部が0のとき
					{
						for ( data=(char)(dec2_len-dec1_len); data<(char)dec2_len; data++ )	// dataは間借り
						{
							if ( MulAddTemp[(int)data] == '0' )	{					// 0はスキップ
								Valid++;
							}
							else{
							    break;
							}
						}
					}
					size += ( pos = (int16_t)sprintf( Dst, "%.*s", Valid, &MulAddTemp[dec2_len-dec1_len] ) );
				}
				else{
				    pos = 0;
				}

				if ( pod ) {
					size += (int16_t)sprintf( Dst+pos, "%.*u", Valid-pos, 0 );	// 0付加
				}
				else {
					while ( pos >= 0 ) {		// 最後の0およびピリオド消す
						pos--;
						if ( Dst[pos] == '0' || Dst[pos] == '.' ) {
							Dst[pos] = 0;
							size--;
						}
						else{
						    break;
						}
					}
				}

			}
			else if ( pos > Valid ) {
				while ( pos > Valid ) {
					Dst--;
					*Dst = '0';
					pos--;
				}
			}
		
//			size += Valid;		// 文字数
		}
	}

	return ((int16_t)size);
}
