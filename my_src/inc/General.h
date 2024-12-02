#ifndef _GENERAL_H_
#define _GENERAL_H_

#include "_r500_config.h"

#include <stdint.h>
#include <stddef.h>

//#include "hal_data.h"
#include "flag_def.h"
#include "MyDefine.h"
//#include "Config.h"
//#include "DateTime.h"
////#include "cmd_thread.h"
////#include "system_thread.h"
#include "Globals.h"
//#include "new_g.h"			// 2023.05.26


#ifdef EDF
#undef EDF
#endif

#ifdef _GENERAL_C_
#define EDF
#else
#define EDF extern
#endif

#define	DIMSIZE(dim)	( sizeof(dim)/sizeof(dim[0]) )


#define BIT(X)		( 1UL << (X) )

#if 0
#define	ABS(X)		( ((X)<0) ? -(X) : (X) )
#endif

#if 0
///バイト連結マクロ uint8_t + uint8_t = uint16_t
#define	SRD_W(P)	( (((uint16_t)*((uint8_t *)((int32_t)(P)+1UL))) << 8) | (*(uint8_t *)(P)) )

///バイト連結マクロ uint8_t + uint8_t + uint8_t + uint8_t = uint32_t
#define	SRD_L(P)	( (((uint32_t)*((uint8_t *)((uint32_t)(P)+3UL))) << 24 ) | (((uint32_t)*((uint8_t *)((uint32_t)(P)+2UL))) << 16 ) | (((uint32_t)*((uint8_t *)((uint32_t)(P)+1UL))) << 8 ) | (*(uint8_t *)(P)) )
#endif

#define	SWR_W(P,X)	*((uint8_t *)(P))=(uint8_t)X, *((uint8_t *)((uint32_t)(P)+1UL))=(uint8_t)((X)>>8)

#define	CRLF	"\x0d\x0a"


//*************************************************
//**  コマンド実行用マクロ
//*************************************************

#define	CMD_NET_STATUS	0

#define	CMD_NET_CRNT	11		// 現在値
#define	CMD_NET_WARN	12		// 警報
#define	CMD_NET_SUC		13		// 吸い上げ
#define	CMD_NET_SWON	14		// 即時警報（接点入力ON）
#define	CMD_NET_SWOFF	15		// 即時警報（接点入力OFF）


#define	CMD_TEST_CRNT	21		// 現在値
#define	CMD_TEST_WARN	22		// 警報
#define	CMD_TEST_SUC	23		// 吸い上げ




//********************************************************
//	シリアル番号テーブル	上位=連番, 下位=CH数
//********************************************************

enum {
	SPEC_501	= 0x0101,		// 上位=連番, 下位=CH数
	SPEC_502	= 0x0201,
	SPEC_502PT	= 0x0301,
	SPEC_503	= 0x0402,
	SPEC_574	= 0x0506,		// CH5,6は積算
	SPEC_505PS	= 0x0602,		// CH2は総パルス数
	SPEC_505TC	= 0x0701,		// 熱電対
	SPEC_505PT	= 0x0801,		// Pt
	SPEC_505V	= 0x0901,		// 電圧
	SPEC_505A	= 0x0A01,		// 電流
	SPEC_576	= 0x0B03,
	SPEC_507	= 0x0C02,		// 高精度湿度センサ
	SPEC_601	= 0x0F01,		// PUSH 601
	SPEC_602	= 0x1001,		// PUSH 602

	SPEC_501B	= 0x1101,		// BLE対応 501
	SPEC_502B	= 0x1201,		// BLE対応 502
	SPEC_503B	= 0x1302,		// BLE対応 503
	SPEC_507B	= 0x1402,		// BLE対応 507
	SPEC_505BPS	= 0x1602,		// BLE対応 CH2は総パルス数
	SPEC_505BTC	= 0x1701,		// BLE対応 熱電対
	SPEC_505BPT	= 0x1801,		// BLE対応 Pt
	SPEC_505BV	= 0x1901,		// BLE対応 電圧
	SPEC_505BA	= 0x1A01,		// BLE対応 電流
	SPEC_505BLX = 0x1B01,		// BLE対応 LX 騒音、振動

	SPEC_NONE	= 0				// 該当無し
};


/**
 * MODEL_OBJ
 * @note    パディングあり
 */
typedef struct {
	const uint16_t	Code;
	const char	*Name;
}__attribute__ ((__packed__)) MODEL_OBJ ;

#ifdef	_GENERAL_C_

const MODEL_OBJ ModelName[] = {
	{ SPEC_501,		"RTR-501"	 },
	{ SPEC_502,		"RTR-502"	 },
	{ SPEC_502PT,	"RTR-502Pt"	 },
	{ SPEC_503,		"RTR-503"	 },
	{ SPEC_574,		"RTR-574"	 },
	{ SPEC_505PS,	"RTR-505-P"	 },
	{ SPEC_505TC,	"RTR-505-TC" },
	{ SPEC_505PT,	"RTR-505-Pt" },
	{ SPEC_505V,	"RTR-505-V"	 },
	{ SPEC_505A,	"RTR-505-mA" },
	{ SPEC_576,		"RTR-576"	 },
	{ SPEC_507,		"RTR-507"	 },
	{ SPEC_601,		"RTR-601"	 },
	{ SPEC_602,		"RTR-602"	 },
	{ SPEC_501B,	"RTR501B"	 },
	{ SPEC_502B,	"RTR502B"	 },
	{ SPEC_503B,	"RTR503B"	 },
	{ SPEC_507B,	"RTR507B"	 },
	{ SPEC_505BPS,	"RTR505B-P"	 },
	{ SPEC_505BTC,	"RTR505B-TC" },
	{ SPEC_505BPT,	"RTR505B-Pt" },
	{ SPEC_505BV,	"RTR505B-V"	 },
	{ SPEC_505BA,	"RTR505B-mA" },
	{ SPEC_505BLX,	"RTR505B-Lx" },


	{ SPEC_NONE,	"N/A"		 },	// 最後に置く
	{	0,			""			 },	// 予備
};
// 2023.05.31 ↓
const MODEL_OBJ ModelNameEspec[] = {
	{ SPEC_501,		"RTW-21S"	 },
	{ SPEC_502,		"RTW-31S"	 },
	{ SPEC_502PT,	"RTW-31S"	 },	// なし？
	{ SPEC_503,		"RSW-21S"	 },
	{ SPEC_574,		"RSW-13L"	 },
	{ SPEC_505PS,	"RUW-21-P"	 },
	{ SPEC_505TC,	"RUW-21-TC" },
	{ SPEC_505PT,	"RUW-21-Pt" },
	{ SPEC_505V,	"RUW-21-V"	 },
	{ SPEC_505A,	"RUW-21-mA" },
	{ SPEC_576,		"RTR-576"	 },
	{ SPEC_507,		"RTR-507"	 },
	{ SPEC_601,		"RTR-601"	 },
	{ SPEC_602,		"RTR-602"	 },
	{ SPEC_501B,	"RTW22S"	 },
	{ SPEC_502B,	"RTW32S"	 },
	{ SPEC_503B,	"RSW22S"	 },
	{ SPEC_507B,	"RTR507B"	 },
	{ SPEC_505BPS,	"RUW22S-P"	 },
	{ SPEC_505BTC,	"RUW22S-TC" },
	{ SPEC_505BPT,	"RUW22S-Pt" },
	{ SPEC_505BV,	"RUW22S-V"	 },
	{ SPEC_505BA,	"RUW22S-mA" },
	{ SPEC_505BLX,	"RUW22S-Lx" },

	{ SPEC_NONE,	"N/A"		 },	// 最後に置く
	{	0,			""			 },	// 予備
};
// 2023.05.31 ↑
#else

extern MODEL_OBJ ModelName[];
extern MODEL_OBJ ModelNameEspec[];

#endif


// moved to comp_log.c EDF const char *CatMsg[];




EDF uint16_t crc16_bfr(char *pBuf, uint32_t length);                //CRC16(CCITT)計算
EDF uint16_t calc_checksum(char *pData);                         // チェックサム計算 SOHフレーム用
EDF uint16_t calc_soh_crc(char *pData);                          // CRC計算 SOHフレーム用

//EDF uint16_t calc_checksum_data(char * rsbuf);                   // チェックサム計算 SOHフレーム用 データ列直接
//EDF uint8_t judge_checksum_data(char *);                         // チェックサム判定 SOHフレーム用 データ列直接
EDF int judge_checksum(char *pData);                                // チェックサム判定 SOHフレーム用
EDF int judge_T2checksum(comT2_t *pBuf);                         // チェックサム判定 T2コマンド用

EDF uint8_t BAT_level_0to5(uint16_t batt);                          // 本機の電池残量レベル０～５を返す（外部電源ありなら５）

EDF uint8_t ExLevelWL(uint8_t);                                     // 子機の電波強度レベルを返す
EDF uint8_t ExLevelBatt(uint8_t);                                   // 子機の電池残量レベルを返す


//static EDF void   crc32_init(uint32_t *pCRC);
//static EDF void   crc32_add(uint32_t *pCRC, uint8_t val8);
//static EDF void   crc32_end(uint32_t *pCRC);
EDF uint32_t  crc32_bfr(void *pBfr, uint32_t size);
//未使用EDF void 	crc32_test(uint32_t sz);

EDF void WriteText(char *Dst, char *Src, size_t Size, uint32_t Max);

EDF bool Str2IP( char *Adrs, char *IP );
EDF void uint32to8(uint32_t val, char *RET);
EDF void address_in2str(uint32_t adr, char *val);

EDF void base64Encode(const void *input, size_t inputLen,  int8_t *output, size_t *outputLen);
EDF error_t base64Decode(const int8_t *input, size_t inputLen, void *output, size_t *outputLen);

EDF char *StrCopyN( char *dst, char *src, int size );


EDF uint16_t GetSpecNumber( uint32_t SerialCode, uint8_t Attrib );
EDF uint16_t GetChannelNumber( uint16_t Code );

EDF int GetTimeString( uint32_t GSec, char *Dst, char *TForm );

EDF char *GetModelName( uint32_t Serial, uint16_t Code );


EDF int Str2U( char *src, uint32_t size );

EDF void Read_StrParam(char *src , char *dst, uint32_t max);

//DO NOT USE EDF char *ExMemcpy( uint32_t dst, uint32_t src, uint32_t size );

EDF int Memcmp(char *dst, char *src, int size_d, int size_s); 

EDF int strlen_utf8(char *s);
EDF int str_utf8_len(char *s, int size);

EDF int AtoI( char *src, int size );                            // パラメータ名に対応したデータ位置を検索する
EDF int AtoL( char *src, int size );                           // サイズ付き atol() ※オーバーフローチェック無し

//EDF void Printf( const char *fmt, ... );

EDF void Print_warning_buffer(int x);
	

EDF uint8_t inherent_BAT_level_0to5(uint16_t batt);
EDF uint32_t GetFormatString( uint32_t GSec, char *Dst, char *TForm, uint32_t Max, char *Exp );
EDF int GetTimeFormat( int GSec, char *Dst, char *TForm, int Flag );	
#endif /* GENERAL_H_ */
