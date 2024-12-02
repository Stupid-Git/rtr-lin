/*
 * Convert.h
 *
 *  Created on: 2019/09/05
 *      Author: tooru.hayashi
 */

#ifndef _CONVERT_H_
#define _CONVERT_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>



#undef EDF

#ifndef _CONVERT_C_
    #define EDF extern
#else
    #define EDF
#endif

//============================================================
// マクロ
//============================================================

#define	SCALE_DEFAULT		(const char *)"\x0A\x0A\x0A\x0A"		// スケール変換のデフォルト(変換無し）
//#define	SCALE_FAIL			(const char *)"1\x0A0\x0A0\x0A\ \x0A"		// スケール変換のデフォルト(変換無し）

#define	SCALE_TERM			0x0A

#define	SCALE_505V			(const char *)"0.001\x0A" "0\x0A" "0\x0A" "V\x0A" "\0"	// 1/1000[V]
#define	SCALE_505Lx			(const char *)"1.0\x0A" "0\x0A" "4\x0A" "dB\x0A" "\0"	// 1[dB]

//============================================================

///@note    パディングあり
typedef struct {

	int16_t			( *const Convert )( char *, uint32_t/*ULONG*/ );	// 変換関数へのポインタ
	uint16_t		DataCode;		// 0x0D, 0x49, 0x149 など
	const char	*TypeChar;		// 温度 "C", "F", 湿度 "%" 文字列へのポインタ

} OBJ_METHOD;


EDF int16_t		GetConvertMethod( uint8_t Attrib, uint8_t Type, uint8_t Add );
EDF uint8_t	GetChannels(void);
//EDF int		ScaleChange( char *Dst, char *Src1, char *Data, char *Src2, int Valid );
EDF int16_t		CheckScaleStr( char **A, char **B, char **Unit );
EDF int16_t		Mul10( char *Dst, char *Src1, char *Src2, int Valid, int Dec );
EDF int16_t		Mul10Add( char *Dst, char *Src1, char *Src2, char *Src3, int Valid );
EDF bool	GetScaleStr( char *Src, int Num );
EDF void	FormatScaleStr( char *Dst, char *Src );


#ifndef	_CONVERT_C_

extern const OBJ_METHOD MeTable[];

#endif


#endif /* _CONVERT_H_ */
