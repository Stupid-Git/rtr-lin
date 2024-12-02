/**
 * @file    Unicode.c
 * @brief   ユニコード処理
 * @date     Created on: 2019/09/03
 * @author tooru.hayashi
 * @note   第1バイトが 0x81-0x9F or 0xE0-0xFC
 *  第2バイトが 0x40-0x7E or 0x80-0xFC
 *  第1バイトに対して 0x40～0xFCのテーブルを作成(0x7Fはダミー) : [189]
 *  第一水準は　0x9872 まで
 * @attention  gcc ARMはコンパイラオプションで指定しない限り char は符号無し（uint8_tとの代入でワーニングは出る）
 *              e2 studioのデフォルトは　オプションでデフォルトが符号無しになっている
 */


#define _UNICODE_C_

#include <string.h> //note: include ‘<string.h>’ or provide a declaration of ‘strlen’
#include "Unicode.h"


#define		UTF_BLOCK			(uint16_t)( 0xFC - 0x40 + 1U )	///< 1 Block の個数

/** 半角カナ変換用テーブル構造体 */
typedef const struct {
	uint32_t	UTF8;			// UTF-8
} OBJ_KNTBL;
/** 漢字変換用テーブル構造体 */
typedef const struct {
//	UWORD	Sjis;			// Shift-Jis (CP932)
	uint16_t	Unicode;		///< Unicode
} OBJ_CDTBL;

static char  EncodeTemp[200];        ///< Encode用

extern OBJ_KNTBL KanaTable[];
extern OBJ_CDTBL CodeTable[];
extern OBJ_CDTBL CodeTable2[];      // sakaguchi 2020.12.14
extern OBJ_CDTBL CodeTable3[];      // sakaguchi 2020.12.14
extern OBJ_CDTBL CodeTable4[];      // sakaguchi 2020.12.14


/**
 ******************************************************************************
    @brief  SJIS(CP932) から UTF-8 へ
    @param  *Dst        変換後の保存先ポインタ
    @param  *Src        変換元へのポインタ
    @param  Max     変換後の最大バイト数(null含む)
    @param  **Next  次に読み込むべきソースポインタ（終了は0）
    @return 書き込まれたサイズ(null含まず)
    @note
 ******************************************************************************
*/
int Sjis2UTF8( char *Dst, char *Src, int Max, char **Next )	// *nextは次の開始場所(終わりなら0)
{
	int		sz, Size = 0;
	uint16_t	Code;

	*Next = Src;					// 次のポインタ

//	while( Max > 3 )				// 面倒なので１文字最大値(3)以上で
	while( Max >= 3 )				// 面倒なので１文字最大値(3)以上で	// sakaguchi 2020.11.20
	{

		Code = (uint16_t)*Src++;
		sz = 0;

		if ( Code == 0 )
		{
			*Next = 0;				// 終了
			break;
		}
		else if ( Code < 0x007F )
		{
			*Dst = (char)Code;
			sz = 1;
		}
		else if ( Code < 0x0081 )
		{
			sz = 0;							// 7F, 80 は無視
		}
		else if ( Code < 0x00A0 )			// 漢字エリア
		{
			Code = (uint16_t)(( Code << 8 ) | *Src);
			Src++;
			sz = ChangeUTF8( Dst, Code );
		}
		else if ( Code < 0x00A1 )
		{
			*Dst = (char)Code;			// 0x20の方がいいかな？
			sz = 1;
		}
		else if ( Code < 0x00E0 )		// 半角カナエリア
		{
			sz = ChangeUTF8( Dst, Code );
		}
		else if ( Code < 0x00FD )		// 漢字エリア
		{
			Code = (uint16_t)(( Code << 8 ) | *Src);
			Src++;
			sz = ChangeUTF8( Dst, Code );
		}
		else{
			sz = 0;						// 無視
		}
		Size += sz;
		Max  -= sz;
		Dst  += sz;

		*Next = Src;				// 次のポインタ

	}

	if ( *Next && ( **Next == 0 ) ){		// 次がnullから始まる場合は
		*Next = 0;						// 終了とする
	}
	*Dst = 0;			// 最後はnull

	return (Size);
}

/**
 ******************************************************************************
    @brief  SJIS(CP932) から EUC へ
    @param  *Dst        変換後の保存先ポインタ
    @param  *Src        変換元へのポインタ
    @param  Max     変換後の最大バイト数(null含む)
    @param  Next
    @return 書き込まれたサイズ(null含まず)
    @note
 ******************************************************************************
*/
int Sjis2EUC( char *Dst, char *Src, int Max, char **Next )	// Next未使用
{
	int		size = 0;
	char	code;
	char high, low;


	(void)(Next);  //2019.Dec.26

	while( Max > 1 )
	{
		code = *Src++;				// １文字読み込み

		if ( ( 0x81 <= code && code <= 0x9F )
			|| ( 0xE0 <= code && code <= 0xFC ) )		// SJISの範囲？
		{
			high = code;
			low = *Src++;			// 次の１バイトを読む

			if ( 0x40 <= low && low <= 0xFC && low != 0x7F )	// 範囲内？
			{
				high = (char)(high -  (( high <= (char)0x9F ) ? 0x71 : 0xB1) );		// 0x71 or 0xB1 を引く
				high  = (char)(( high << 1 ) + 1);					// ２倍して +1

				if ( low > 0x7F ){
				    low--;
				}

				if ( low >= 0x9E ) {
					low = (char)(low - (char)0x7D);
					high++;
				}
				else{
				    low = (char)(low - 0x1F);
				}

				if ( Max > 2 ) {
					*Dst++ = (char)(high | 0x80);	// EUCはJIS+最上位ビットが立つだけ
					*Dst++ = (char)(low | 0x80);	// EUCはJIS+最上位ビットが立つだけ
					size += 2;
					Max -= 2;
				}
				else{
				    break;
				}
			}
			else if ( low == 0 ){		// ここで null なら
				break;					// 終了
			}
			// 範囲外は2バイト捨てられる
		}
		else if ( 0xA1 <= code && code <= 0xDF )
		{
			if ( Max > 2 ) {
				*Dst++ = (char)0x8E;			// 半角カナは 0x8E に続いて
				*Dst++ = (char)code;			// カナコードを入れる
				size += 2;
				Max -= 2;
			}
		}
		else if ( code != 0 )
		{
			if ( Max > 1 ) {
				*Dst++ = (char)code;
				size++;
				Max--;
			}
			else{
				break;
			}
		}
		else{
			break;
		}

	}

	if ( Max > 0 ){
		*Dst = 0;		// 最後は必ず null
	}

	return (size);		// 書き込んだサイズ
}

/**
 ******************************************************************************
    @brief  Unicode(UCS) から UTF-8 へ
    @param  *Dst        変換したコードの保存先ポインタ
    @param  Code        変換するコード
    @return サイズ
    @see        KanaTable
    @note   2017.Jun.1  ローカル変数UTF8をUTF8_dに変更
 ******************************************************************************
*/
int ChangeUTF8( char *Dst, uint16_t Code )
{
	uint16_t	Low, High;
	int Size = 0;
	uint32_t	UTF8_d;
	uint8_t		Change=0;           // sakaguchi 2020.12.14 

	if ( 0x00A1 <= Code && Code <= 0x00DF )		// 半角カナ？
	{
		Code = (uint16_t)(Code - 0x00A1);
		UTF8_d = KanaTable[ Code ].UTF8;			// UTF-8へ
		*Dst++ = (char)( UTF8_d >> 16 );
		*Dst++ = (char)( UTF8_d >> 8 );
		*Dst++ = (char)UTF8_d;
		Size = 3;
	}
	else	// 漢字エリア
	{

		Low = Code & 0xFF;
		High = Code >> 8;

		if ( 0x40 <= Low && Low <= 0xFC )			// 範囲内？
		{
			if ( 0x81 <= High && High <= 0x9F )
			{
				Size = (( High - (char)0x81 ) * UTF_BLOCK + Low - (char)0x40);	// 開始位置の計算
				Code = CodeTable[Size].Unicode;									// UCS(Unicode)へ   // sakaguchi 2020.12.14
				Change = 1;														// コード変換OK     // sakaguchi 2020.12.14
			}
// sakaguchi 2020.12.14 ↓
			else if ( 0xE0 <= High && High <= 0xEA )
			{
				Size = (( High - (char)0xE0 ) * UTF_BLOCK + Low - (char)0x40);	// 開始位置の計算
				Code = CodeTable2[Size].Unicode;								// UCS(Unicode)へ
				Change = 1;														// コード変換OK
			}
			else if( 0xED <= High && High <= 0xEF )
			{
				Size = (( High - (char)0xED ) * UTF_BLOCK + Low - (char)0x40);	// 開始位置の計算
				Code = CodeTable3[Size].Unicode;								// UCS(Unicode)へ
				Change = 1;														// コード変換OK
			}
			else if( 0xFA <= High && High <= 0xFC )
			{
				Size = (( High - (char)0xFA ) * UTF_BLOCK + Low - (char)0x40);	// 開始位置の計算
				Code = CodeTable4[Size].Unicode;								// UCS(Unicode)へ
				Change = 1;														// コード変換OK
			}
// sakaguchi 2020.12.14 ↑
		}

//		if ( Size )		// コードは正しい
		if ( Change )	// コード変換OK         // sakaguchi 2020.12.14
		{

			//Code = CodeTable[Size].Unicode;			// UCS(Unicode)へ           // sakaguchi 2020.12.14 del

			// 以下 UTF-8 へエンコード
			if ( Code >= 0x0800 )
			{
                Low = (uint16_t)(0xE0 /*0b11100000*/ | ( Code >> 12 ));             // 1110**** を付加
                *Dst++ = (char)Low;
                Low = (uint16_t)(0x80 /*0b10000000*/ | ( ( Code >> 6 ) & 0x003F )); // 10****** を付加
                *Dst++ = (char)Low;
                Low = (uint16_t)(0x80 /*0b10000000*/ | ( Code & 0x003F ));          // 10****** を付加
                *Dst++ = (char)Low;
                Size = 3;
			}
			else if ( Code >= 0x80 )
			{
                Low = 0xC0 /*0b11000000*/ | ( Code >> 6 );              // 110***** を付加
                *Dst++ = (char)Low;
                Low = (char)(0x80 /*0b10000000*/ | ( ( Code >> 6 ) & 0x003F )); // 10****** を付加
                *Dst++ = (char)Low;
                Size = 2;
			}
			else
			{
//				*Dst++ = (UBYTE)Low;		// 未対応は何もしない
//				Size = 1;
				Size = 0;
			}
		}
	}

	return (Size);
}

/**
 ******************************************************************************
    @brief  SJIS(CP932) から JIS(ISO-2022-JP) へ
    @param  *Dst        変換後の保存先ポインタ
    @param  *Src        変換元へのポインタ
    @param  Max     変換後の最大バイト数(null含む)
    @param  Next  カットされた場合の次に始める先頭アドレス
    @return 書き込まれたサイズ(null含まず)
    @attention  ※Maxを超えるとき必ずASCIIモードに戻してカットされる
 ******************************************************************************
*/
int Sjis2Jis( char *Dst, char *Src, int Max, char **Next )	// *nextは次の開始場所(終わりなら0)
{
	static const char *const ESC_KNJ = "\x1B$B";
	static const char *const ESC_ASC = "\x1B(B";

	int		size = 0;
	char high, low, code, knj=0;

	*Next = Src;					// 次のポインタ

	while( Max > 1 )
	{
		*Next = Src;				// 次のポインタ
		code = *Src++;				// １文字読み込み

		if ( ( 0x81 <= code && code <= 0x9F )
			|| ( 0xE0 <= code && code <= 0xFC ) )		// SJISの範囲？
		{
			high = code;
			low = *Src++;			// 次の１バイトを読む

			if ( 0x40 <= low && low <= 0xFC && low != 0x7F )	// 範囲内？
			{
				high = (char)(high -  (( high <= 0x9F ) ? 0x71 : 0xB1) );		// 0x71 or 0xB1 を引く
				high  = (char)(( high << 1 ) + 1);					// ２倍して +1

				if ( low > 0x7F ){
				    low--;
				}

				if ( low >= 0x9E ) {
					low = (char)(low - 0x7D);
					high++;
				}
				else
					low = (char)(low - 0x1F);

				if ( !knj )			// ASCIIの後？
				{
					if ( Max > 3+3+2 )	// 漢字に入るとき抜ける分も考慮
					{					// KIN(3)+KNJ(2)+KOUT(3)
						Dst = StrCopyN( Dst, (char *)ESC_KNJ, 3 ) - 1;
						knj = 1;
						size += 3;
						Max -= 3;
					}
					else{
						break;			// 漢字に入れず
					}
				}

				if ( Max > 2+3 ) {
					*Dst++ = high;
					*Dst++ = low;
					size += 2;
					Max -= 2;
					*Next = Src;				// 次のポインタ
				}
				else{
				    break;				// KOUTが残っていない
				}
			}
			else if ( low == 0 )		// ここで null なら (本当はだめだけど)
			{
				*Next = 0;				// 最後まで行きました
				break;					// 終了
			}

			// 範囲外は2バイト捨てられる
		}
		else if ( code != 0 )
		{
			if ( knj )			// 漢字の後？
			{
				if ( Max > 3 ) {
					Dst = StrCopyN( Dst, (char *)ESC_ASC, 3 ) - 1;
					knj = 0;
					size += 3;
					Max -= 3;
				}
				else{
				    break;				// 来ないはずだが
				}
			}

			if ( Max > 1 ) {
				*Dst++ = code;
				size++;
				Max--;
				*Next = Src;				// 次のポインタ
			}
			else{
				break;					// ASCIIのまま終わり
			}
		}
		else	// nullはこちら
		{
			*Next = 0;				// 最後まで行きました
			break;
		}

	}

	if ( knj )			// 最後が漢字で終わっている場合
	{
		if ( Max > 3 ) {
			Dst = StrCopyN( Dst, (char *)ESC_ASC, 3 ) - 1;
			size += 3;
			Max -= 3;
		}
	}

	if ( *Next && ( **Next == 0 ) ){		// 次がnullから始まる場合は
		*Next = 0;						// 終了とする
	}
	if ( Max > 0 ){
	    *Dst = 0;		// 最後は必ず null
	}

	return (size);		// 書き込んだサイズ
}



/**
 ******************************************************************************
    @brief  ヘッダ用 SJIS(CP932) から JIS(IS-2022-JP) へ
    @param  *Dst        変換後の保存先ポインタ
    @param  *Src        変換元へのポインタ
    @param  Max     変換後の最大バイト数(null含む)
    @return サイズ
    @attention  １行は75文字までという制約がある(RFC)
                    さらにiChipは64(Subjectは96?)バイトまで
                    Maxには最後のnullを含むこと
    @note
 ******************************************************************************
*/
int Sjis2JisHeader( char *Dst, char *Src, int Max )
{
	static const char *const ENC_START = "=?ISO-2022-JP?B?";	// 16文字
	static const char *const ENC_END = "?=";					// 2文字
	int		size, part; /*, ichip;*/
	char	data;
	char *start;

	for ( size=0; size<Max; size++ )
	{
		data = Src[size];

		if ( ( 0x81 <= data && data <= 0x9F )
			|| ( 0xE0 <= data && data <= 0xFC ) )		// SJISの範囲？
		{
			size = -1;				// エンコードする
			break;
		}
		else if ( data == 0 ){		// 終端？
			break;
		}
	}

	if ( size >= 0 )					// エンコード不要？
	{
//		size = ( size > ichip ) ? ichip : size;
		StrCopyN( Dst, Src, size );		// そのままコピー
	}
	else
	{
		while ( Max > 0 )
		{
//			part = ( part > 33 ) ? 33 : part;	// 最大64バイト　(64-(16+2))/4*3 = 33

			part = ( Max - (16+2) ) / 4 * 3;	// エンコード後のサイズを算出
		
			if ( part > 0 ) {
				part = ( part > 33 ) ? 33 : part;	// 最大64バイト　(64-(16+2))/4*3 = 33
				size = Sjis2Jis( EncodeTemp, Src, part, &start );
			} else
				break;		// 余裕無し

			Src = Dst;		// 間借り

			Dst = StrCopyN( Dst, (char *)ENC_START, 16 ) - 1;
			Dst += ToMIME( Dst, EncodeTemp, size );

			Dst = StrCopyN( Dst, (char *)ENC_END, 2 ) - 1;

			size = Dst - Src;

			Max -= size;

			if ( ( Max > 16+2+9+2 ) && start )		// エンコードヘッダより大きいこと
			{										// なおかつソースの残りがある
				Dst = StrCopyN( Dst, "\\x0D\\x0A ", 9 ) - 1;		// 次の行へ
				Src = start;
				Max -= 9;
			}
			else{
				Max = 0;		// 終了
			}
		}
	}

	return (size);		// 意味なし
}


/**
 ******************************************************************************
    @brief  ヘッダ用 SJIS(CP932) から UTF-8 へ
    @param  *Dst        変換後の保存先ポインタ
    @param  *Src        変換元へのポインタ
    @param  Max     変換後の最大バイト数(null含む)
    @return サイズ
    @attention  １行は75文字までという制約がある(RFC)
                    さらにiChipは64(Subjectは96?)バイトまで
    @note
 ******************************************************************************
*/
int Sjis2UTF8Header( char *Dst, char *Src, int Max )
{
	static const char *const ENC_START = "=?UTF-8?B?";		// 10文字
	static const char *const ENC_END = "?=";				// 2文字
	int		size, part; /*, ichip;*/
	char	data, *start;

	for ( size=0; size<Max; size++ )
	{
		data = Src[size];
		if ( data >= 0x80 )
		{
			size = -1;				// エンコードする
			break;
		}
		else if ( data == 0 )		// 終端？
			break;
	}

	if ( size >= 0 )					// エンコード不要？
	{
//		size = ( size > ichip ) ? ichip : size;
		StrCopyN( Dst, Src, size );		// そのままコピー
	}
	else
	{
		while ( Max > 0 )
		{
//			part = ( Max > 39 ) ? 39 : Max;		// 最大64バイト　(64-(10+2))/4*3 = 39

			part = ( Max - (10+2) ) / 4 * 3;	// エンコード後のサイズを算出
		
			if ( part > 0 ) {
				part = ( part > 39 ) ? 39 : part;	// 最大64バイト　(64-(10+2))/4*3 = 39
				size = Sjis2UTF8( EncodeTemp, Src, part, &start );
			} else
				break;		// 余裕無し

			Src = Dst;		// 間借り

			Dst = StrCopyN( Dst, (char *)ENC_START, 10 ) - 1;
			Dst += ToMIME( Dst, EncodeTemp, size );

			Dst = StrCopyN( Dst, (char *)ENC_END, 2 ) - 1;

			size = Dst - Src;

			Max -= size;

			if ( ( Max > 10+2+9+3 ) && start )		// エンコードヘッダより大きいこと
			{										// なおかつソースの残りがある
				Dst = StrCopyN( Dst, "\\x0D\\x0A ", 9 ) - 1;		// 次の行へ
				Src = start;
				Max -= 9;
			}
			else{
			    Max = 0;		// 終了
			}
		}
	}

	return (size);		// 意味なし
}



/**
 * MIME(Base64)
 * @param Dst       変換後の保存先ポインタ
 * @param Src       変換元へのポインタ
 * @param Size      サイズ
 * @return
 * @note    4/3倍になることに注意
 * @note    Size = 0 の時は strlen まで
 */
uint16_t ToMIME( char *Dst, char *Src, int Size )
{
	uint16_t	x, y, z, rest;
	int total = 0;

	if ( Size == 0 ){
	    Size = (int)strlen( Src );
	}

	rest = (uint16_t)(Size % 3);
	Size -= rest;

	while( Size > 0 ) {

		x = (uint16_t)*Src++;
		y = (uint16_t)(( x << 8 ) | *Src++);
		z = (uint16_t)(( y << 8 ) | *Src++);

		*Dst++ = Encode[ ( x >> 2 ) ];
		*Dst++ = Encode[ ( y >> 4 ) & 0x3F ];
		*Dst++ = Encode[ ( z >> 6 ) & 0x3F ];
		*Dst++ = Encode[ z & 0x3F  ];

		Size -= 3;
		total += 4;
	}

	if ( rest > 0 ) {

		y = z = 0;
		x = (uint16_t)*Src++;				// 余り 1byte
		y = (uint16_t)(( x << 8 ) | ( ( rest == 2 ) ? *Src : 0 ));	// 余り 2byteは次を読む
		z = (uint16_t)(( y << 8 ));

		*Dst++ = Encode[ ( x >> 2 ) ];
		*Dst++ = Encode[ ( y >> 4 ) & 0x3F ];

		if ( rest == 2 ){
		    *Dst++ = Encode[ ( z >> 6 ) & 0x3F ];
		}
		else{
		    *Dst++ = '=';
		}

		*Dst++ = '=';

		total += 4;
	}

	return ((uint16_t)total);
}



/**
 * UTF-8 変換テーブル
 */
static const struct
{
	char size;
	char code[3];
} UTF8[] = {
// 0x00 - 0x7F
            { 0, { 0x00 }}, { 1, { 0x01 }}, { 1, { 0x02 }}, { 1, { 0x03 }}, { 1, { 0x04 }}, { 1, { 0x05 }}, { 1, { 0x06 }}, { 1, { 0x07 }},
            { 1, { 0x08 }}, { 1, { 0x09 }}, { 1, { 0x0A }}, { 1, { 0x0B }}, { 1, { 0x0C }}, { 1, { 0x0D }}, { 1, { 0x0E }}, { 1, { 0x0F }},
            { 1, { 0x10 }}, { 1, { 0x11 }}, { 1, { 0x12 }}, { 1, { 0x13 }}, { 1, { 0x14 }}, { 1, { 0x15 }}, { 1, { 0x16 }}, { 1, { 0x17 }},
            { 1, { 0x18 }}, { 1, { 0x19 }}, { 1, { 0x1A }}, { 1, { 0x1B }}, { 1, { 0x1C }}, { 1, { 0x1D }}, { 1, { 0x1E }}, { 1, { 0x1F }},
            { 1, { 0x20 }}, { 1, { 0x21 }}, { 1, { 0x22 }}, { 1, { 0x23 }}, { 1, { 0x24 }}, { 1, { 0x25 }}, { 1, { 0x26 }}, { 1, { 0x27 }},
            { 1, { 0x28 }}, { 1, { 0x29 }}, { 1, { 0x2A }}, { 1, { 0x2B }}, { 1, { 0x2C }}, { 1, { 0x2D }}, { 1, { 0x2E }}, { 1, { 0x2F }},
            { 1, { 0x30 }}, { 1, { 0x31 }}, { 1, { 0x32 }}, { 1, { 0x33 }}, { 1, { 0x34 }}, { 1, { 0x35 }}, { 1, { 0x36 }}, { 1, { 0x37 }},
            { 1, { 0x38 }}, { 1, { 0x39 }}, { 1, { 0x3A }}, { 1, { 0x3B }}, { 1, { 0x3C }}, { 1, { 0x3D }}, { 1, { 0x3E }}, { 1, { 0x3F }},
            { 1, { 0x40 }}, { 1, { 0x41 }}, { 1, { 0x42 }}, { 1, { 0x43 }}, { 1, { 0x44 }}, { 1, { 0x45 }}, { 1, { 0x46 }}, { 1, { 0x47 }},
            { 1, { 0x48 }}, { 1, { 0x49 }}, { 1, { 0x4A }}, { 1, { 0x4B }}, { 1, { 0x4C }}, { 1, { 0x4D }}, { 1, { 0x4E }}, { 1, { 0x4F }},
            { 1, { 0x50 }}, { 1, { 0x51 }}, { 1, { 0x52 }}, { 1, { 0x53 }}, { 1, { 0x54 }}, { 1, { 0x55 }}, { 1, { 0x56 }}, { 1, { 0x57 }},
            { 1, { 0x58 }}, { 1, { 0x59 }}, { 1, { 0x5A }}, { 1, { 0x5B }}, { 1, { 0x5C }}, { 1, { 0x5D }}, { 1, { 0x5E }}, { 1, { 0x5F }},
            { 1, { 0x60 }}, { 1, { 0x61 }}, { 1, { 0x62 }}, { 1, { 0x63 }}, { 1, { 0x64 }}, { 1, { 0x65 }}, { 1, { 0x66 }}, { 1, { 0x67 }},
            { 1, { 0x68 }}, { 1, { 0x69 }}, { 1, { 0x6A }}, { 1, { 0x6B }}, { 1, { 0x6C }}, { 1, { 0x6D }}, { 1, { 0x6E }}, { 1, { 0x6F }},
            { 1, { 0x70 }}, { 1, { 0x71 }}, { 1, { 0x72 }}, { 1, { 0x73 }}, { 1, { 0x74 }}, { 1, { 0x75 }}, { 1, { 0x76 }}, { 1, { 0x77 }},
            { 1, { 0x78 }}, { 1, { 0x79 }}, { 1, { 0x7A }}, { 1, { 0x7B }}, { 1, { 0x7C }}, { 1, { 0x7D }}, { 1, { 0x7E }}, { 1, { 0x7F }},

        // 0x80 - 0x9F

            { 3, { 0xE2,0x82,0xAC }}, { 0, { 0x00 }}, { 3, { 0xE2,0x80,0x9A }}, { 2, { 0xC6,0x92 }},  { 3, { 0xE2,0x80,0x9E }}, { 3, { 0xE2,0x80,0xA6 }}, { 3, { 0xE2,0x80,0xA0 }}, { 3, { 0xE2,0x80,0xA1 }},
            { 2, { 0xCB,0x86 }}, { 3, { 0xE2,0x80,0xB0 }}, { 2, { 0xC5,0xA0 }}, { 3, { 0xE2,0x80,0xB9 }},   { 2, { 0xC5,0x92 }}, { 0, { 0x00 }}, { 2, { 0xC5,0xBD }}, { 0, { 0x00 }},

            { 0, { 0x00 }}, { 3, { 0xE2,0x80,0x98 }}, { 3, { 0xE2,0x80,0x99 }}, { 3, { 0xE2,0x80,0x9C }},   { 3, { 0xE2,0x80,0x9D }}, { 3, { 0xE2,0x80,0xA2 }}, { 3, { 0xE2,0x80,0x93 }}, { 3, { 0xE2,0x80,0x94 }},
            { 2, { 0xCB,0x9C }}, { 3, { 0xE2,0x84,0xA2 }}, { 2, { 0xC5,0xA1 }}, { 3, { 0xE2,0x80,0xBA }},   { 3, { 0xC5,0x93 }}, { 0, { 0x00 }}, { 2, { 0xC5,0xBE }}, { 2, { 0xC5,0xB8 }},

        // 0xA0 - 0xFF

            { 2, { 0xC2,0xA0 }}, { 2, { 0xC2,0xA1 }}, { 2, { 0xC2,0xA2 }}, { 2, { 0xC2,0xA3 }}, { 2, { 0xC2,0xA4 }}, { 2, { 0xC2,0xA5 }}, { 2, { 0xC2,0xA6 }}, { 2, { 0xC2,0xA7 }},
            { 2, { 0xC2,0xA8 }}, { 2, { 0xC2,0xA9 }}, { 2, { 0xC2,0xAA }}, { 2, { 0xC2,0xAB }}, { 2, { 0xC2,0xAC }}, { 2, { 0xC2,0xAD }}, { 2, { 0xC2,0xAE }}, { 2, { 0xC2,0xAF }},

            { 2, { 0xC2,0xB0 }}, { 2, { 0xC2,0xB1 }}, { 2, { 0xC2,0xB2 }}, { 2, { 0xC2,0xB3 }}, { 2, { 0xC2,0xB4 }}, { 2, { 0xC2,0xB5 }}, { 2, { 0xC2,0xB6 }}, { 2, { 0xC2,0xB7 }},
            { 2, { 0xC2,0xB8 }}, { 2, { 0xC2,0xB9 }}, { 2, { 0xC2,0xBA }}, { 2, { 0xC2,0xBB }}, { 2, { 0xC2,0xBC }}, { 2, { 0xC2,0xBD }}, { 2, { 0xC2,0xBE }}, { 2, { 0xC2,0xBF }},

            { 2, { 0xC3,0x80 }}, { 2, { 0xC3,0x81 }}, { 2, { 0xC3,0x82 }}, { 2, { 0xC3,0x83 }}, { 2, { 0xC3,0x84 }}, { 2, { 0xC3,0x85 }}, { 2, { 0xC3,0x86 }}, { 2, { 0xC3,0x87 }},
            { 2, { 0xC3,0x88 }}, { 2, { 0xC3,0x89 }}, { 2, { 0xC3,0x8A }}, { 2, { 0xC3,0x8B }}, { 2, { 0xC3,0x8C }}, { 2, { 0xC3,0x8D }}, { 2, { 0xC3,0x8E }}, { 2, { 0xC3,0x8F }},

            { 2, { 0xC3,0x90 }}, { 2, { 0xC3,0x91 }}, { 2, { 0xC3,0x92 }}, { 2, { 0xC3,0x93 }}, { 2, { 0xC3,0x94 }}, { 2, { 0xC3,0x95 }}, { 2, { 0xC3,0x96 }}, { 2, { 0xC3,0x97 }},
            { 2, { 0xC3,0x98 }}, { 2, { 0xC3,0x99 }}, { 2, { 0xC3,0x9A }}, { 2, { 0xC3,0x9B }}, { 2, { 0xC3,0x9C }}, { 2, { 0xC3,0x9D }}, { 2, { 0xC3,0x9E }}, { 2, { 0xC3,0x9F }},

            { 2, { 0xC3,0xA0 }}, { 2, { 0xC3,0xA1 }}, { 2, { 0xC3,0xA2 }}, { 2, { 0xC3,0xA3 }}, { 2, { 0xC3,0xA4 }}, { 2, { 0xC3,0xA5 }}, { 2, { 0xC3,0xA6 }}, { 2, { 0xC3,0xA7 }},
            { 2, { 0xC3,0xA8 }}, { 2, { 0xC3,0xA9 }}, { 2, { 0xC3,0xAA }}, { 2, { 0xC3,0xAB }}, { 2, { 0xC3,0xAC }}, { 2, { 0xC3,0xAD }}, { 2, { 0xC3,0xAE }}, { 2, { 0xC3,0xAF }},

            { 2, { 0xC3,0xB0 }}, { 2, { 0xC3,0xB1 }}, { 2, { 0xC3,0xB2 }}, { 2, { 0xC3,0xB3 }}, { 2, { 0xC3,0xB4 }}, { 2, { 0xC3,0xB5 }}, { 2, { 0xC3,0xB6 }}, { 2, { 0xC3,0xB7 }},
            { 2, { 0xC3,0xB8 }}, { 2, { 0xC3,0xB9 }}, { 2, { 0xC3,0xBA }}, { 2, { 0xC3,0xBB }}, { 2, { 0xC3,0xBC }}, { 2, { 0xC3,0xBD }}, { 2, { 0xC3,0xBE }}, { 2, { 0xC3,0xBF }},
};

/**
 ******************************************************************************
    @brief  Windows-1252 から UTF-8 へ
    @param  *Dst    変換先ポインタ
    @param  Data    変換データ
    @return カウント
    @note   呼び出し元   XML_EncodeWin()
 ******************************************************************************
*/
int Win2UTF8( char *Dst, char Data )
{
	int		i, cnt;

	for ( cnt = i = 0; i < UTF8[(int)Data].size; i++ ) {
		*Dst++ = UTF8[(int)Data].code[i];
		cnt++;
	}

	return (cnt);
}

/**
 ******************************************************************************
    @brief  Windows-1252 から UTF-8 へ

                0x00～0xFFの1バイトコードをUTF-8へ
                制御コードと未定義コードは無視する
                Sizeまたはnullが現れるまでとする
    @param  *Dst    変換先ポインタ
    @param  *Src    変換元データへのポインタ
    @param  Size
    @return カウント
    @note   呼び出し元   XML_EncodeWin()
 ******************************************************************************
*/
int WinStr2UTF8(char *Dst, char *Src, int Size)	// Next未使用
{
	int		i, cnt = 0;
	char	data;

	if ( Size <= 0 )
		Size = 256;		// Max256

	do {

		data = *Src++;

		if ( data ) {

			if ( UTF8[(int)data].size <= Size )
			{
				for ( i=0; i < UTF8[(int)data].size; i++ ) {
					*Dst++ = UTF8[(int)data].code[i];
					cnt++;
					Size--;
				}
			}
			else{
				break;
			}
		}
		else{
			break;
		}

	} while( Size > 0 );

	if ( Size > 0 )
		*Dst = 0;		// 最後は必ず null

	return (cnt);
}


//===================================================

/**
 ******************************************************************************
    @brief  Unicode(UCS) から UTF-8 変換用テーブル
                0xA1 ～ 0xDF　半角カナエリア
    @note       0x21 ～ 0x7E のうち（これはとりあえずやめておく）
        0x5C --> 0xC2A5
        0x7E --> 0xE280BE
    @note       0xA1 ～ 0xDF　半角カナエリア
 ******************************************************************************
*/
OBJ_KNTBL KanaTable[] =
{
	{ /* 0xA1, */ 0xEFBDA1 },{ /* 0xA2, */ 0xEFBDA2 },{ /* 0xA3, */ 0xEFBDA3 },	{ /* 0xA4, */ 0xEFBDA4 },
	{ /* 0xA5, */ 0xEFBDA5 },{ /* 0xA6, */ 0xEFBDA6 },{ /* 0xA7, */ 0xEFBDA7 },	{ /* 0xA8, */ 0xEFBDA8 },
	{ /* 0xA9, */ 0xEFBDA9 },{ /* 0xAA, */ 0xEFBDAA },{ /* 0xAB, */ 0xEFBDAB },	{ /* 0xAC, */ 0xEFBDAC },
	{ /* 0xAD, */ 0xEFBDAD },{ /* 0xAE, */ 0xEFBDAE },{ /* 0xAF, */ 0xEFBDAF },	{ /* 0xB0, */ 0xEFBDB0 },
	{ /* 0xB1, */ 0xEFBDB1 },{ /* 0xB2, */ 0xEFBDB2 },{ /* 0xB3, */ 0xEFBDB3 },	{ /* 0xB4, */ 0xEFBDB4 },
	{ /* 0xB5, */ 0xEFBDB5 },{ /* 0xB6, */ 0xEFBDB6 },{ /* 0xB7, */ 0xEFBDB7 },	{ /* 0xB8, */ 0xEFBDB8 },
	{ /* 0xB9, */ 0xEFBDB9 },{ /* 0xBA, */ 0xEFBDBA },{ /* 0xBB, */ 0xEFBDBB },	{ /* 0xBC, */ 0xEFBDBC },
	{ /* 0xBD, */ 0xEFBDBD },{ /* 0xBE, */ 0xEFBDBE },{ /* 0xBF, */ 0xEFBDBF },	{ /* 0xC0, */ 0xEFBE80 },
	{ /* 0xC1, */ 0xEFBE81 },{ /* 0xC2, */ 0xEFBE82 },{ /* 0xC3, */ 0xEFBE83 },	{ /* 0xC4, */ 0xEFBE84 },
	{ /* 0xC5, */ 0xEFBE85 },{ /* 0xC6, */ 0xEFBE86 },{ /* 0xC7, */ 0xEFBE87 },	{ /* 0xC8, */ 0xEFBE88 },
	{ /* 0xC9, */ 0xEFBE89 },{ /* 0xCA, */ 0xEFBE8A },{ /* 0xCB, */ 0xEFBE8B },	{ /* 0xCC, */ 0xEFBE8C },
	{ /* 0xCD, */ 0xEFBE8D },{ /* 0xCE, */ 0xEFBE8E },{ /* 0xCF, */ 0xEFBE8F },	{ /* 0xD0, */ 0xEFBE90 },
	{ /* 0xD1, */ 0xEFBE91 },{ /* 0xD2, */ 0xEFBE92 },{ /* 0xD3, */ 0xEFBE93 },	{ /* 0xD4, */ 0xEFBE94 },
	{ /* 0xD5, */ 0xEFBE95 },{ /* 0xD6, */ 0xEFBE96 },{ /* 0xD7, */ 0xEFBE97 },	{ /* 0xD8, */ 0xEFBE98 },
	{ /* 0xD9, */ 0xEFBE99 },{ /* 0xDA, */ 0xEFBE9A },{ /* 0xDB, */ 0xEFBE9B },	{ /* 0xDC, */ 0xEFBE9C },
	{ /* 0xDD, */ 0xEFBE9D },{ /* 0xDE, */ 0xEFBE9E },{ /* 0xDF, */ 0xEFBE9F },
};

OBJ_CDTBL CodeTable[] =
{
//===== 0x81 40～FC ( テーブル数 [189]  ）====================
{ /* 0x8140, */ 0x3000 },	// 8140 E38080 [　]  #IDEOGRAPHIC SPACE
{ /* 0x8141, */ 0x3001 },	// 8141 E38081 [、]  #IDEOGRAPHIC COMMA
{ /* 0x8142, */ 0x3002 },	// 8142 E38082 [。]  #IDEOGRAPHIC FULL STOP
{ /* 0x8143, */ 0xFF0C },	// 8143 EFBC8C [，]  #FULLWIDTH COMMA
{ /* 0x8144, */ 0xFF0E },	// 8144 EFBC8E [．]  #FULLWIDTH FULL STOP
{ /* 0x8145, */ 0x30FB },	// 8145 E383BB [・]  #KATAKANA MIDDLE DOT
{ /* 0x8146, */ 0xFF1A },	// 8146 EFBC9A [：]  #FULLWIDTH COLON
{ /* 0x8147, */ 0xFF1B },	// 8147 EFBC9B [；]  #FULLWIDTH SEMICOLON
{ /* 0x8148, */ 0xFF1F },	// 8148 EFBC9F [？]  #FULLWIDTH QUESTION MARK
{ /* 0x8149, */ 0xFF01 },	// 8149 EFBC81 [！]  #FULLWIDTH EXCLAMATION MARK
{ /* 0x814A, */ 0x309B },	// 814A E3829B [゛]  #KATAKANA-HIRAGANA VOICED SOUND MARK
{ /* 0x814B, */ 0x309C },	// 814B E3829C [゜]  #KATAKANA-HIRAGANA SEMI-VOICED SOUND MARK
{ /* 0x814C, */ 0x00B4 },	// 814C C2B4   [´]  #ACUTE ACCENT
{ /* 0x814D, */ 0xFF40 },	// 814D EFBD80 [｀]  #FULLWIDTH GRAVE ACCENT
{ /* 0x814E, */ 0x00A8 },	// 814E C2A8   [¨]  #DIAERESIS
{ /* 0x814F, */ 0xFF3E },	// 814F EFBCBE [＾]  #FULLWIDTH CIRCUMFLEX ACCENT
{ /* 0x8150, */ 0xFFE3 },	// 8150 EFBFA3 [￣]  #FULLWIDTH MACRON
{ /* 0x8151, */ 0xFF3F },	// 8151 EFBCBF [＿]  #FULLWIDTH LOW LINE
{ /* 0x8152, */ 0x30FD },	// 8152 E383BD [ヽ]  #KATAKANA ITERATION MARK
{ /* 0x8153, */ 0x30FE },	// 8153 E383BE [ヾ]  #KATAKANA VOICED ITERATION MARK
{ /* 0x8154, */ 0x309D },	// 8154 E3829D [ゝ]  #HIRAGANA ITERATION MARK
{ /* 0x8155, */ 0x309E },	// 8155 E3829E [ゞ]  #HIRAGANA VOICED ITERATION MARK
{ /* 0x8156, */ 0x3003 },	// 8156 E38083 [〃]  #DITTO MARK
{ /* 0x8157, */ 0x4EDD },	// 8157 E4BB9D [仝]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8158, */ 0x3005 },	// 8158 E38085 [々]  #IDEOGRAPHIC ITERATION MARK
{ /* 0x8159, */ 0x3006 },	// 8159 E38086 [〆]  #IDEOGRAPHIC CLOSING MARK
{ /* 0x815A, */ 0x3007 },	// 815A E38087 [〇]  #IDEOGRAPHIC NUMBER ZERO
{ /* 0x815B, */ 0x30FC },	// 815B E383BC [ー]  #KATAKANA-HIRAGANA PROLONGED SOUND MARK
{ /* 0x815C, */ 0x2015 },	// 815C E28095 [―]  #HORIZONTAL BAR
{ /* 0x815D, */ 0x2010 },	// 815D E28090 [‐]  #HYPHEN
{ /* 0x815E, */ 0xFF0F },	// 815E EFBC8F [／]  #FULLWIDTH SOLIDUS
{ /* 0x815F, */ 0xFF3C },	// 815F 5C     [＼]  #FULLWIDTH REVERSE SOLIDUS
{ /* 0x8160, */ 0xFF5E },	// 8160 E3809C [～]  #FULLWIDTH TILDE
{ /* 0x8161, */ 0x2225 },	// 8161 E28096 [∥]  #PARALLEL TO
{ /* 0x8162, */ 0xFF5C },	// 8162 EFBD9C [｜]  #FULLWIDTH VERTICAL LINE
{ /* 0x8163, */ 0x2026 },	// 8163 E280A6 […]  #HORIZONTAL ELLIPSIS
{ /* 0x8164, */ 0x2025 },	// 8164 E280A5 [‥]  #TWO DOT LEADER
{ /* 0x8165, */ 0x2018 },	// 8165 E28098 [‘]  #LEFT SINGLE QUOTATION MARK
{ /* 0x8166, */ 0x2019 },	// 8166 E28099 [’]  #RIGHT SINGLE QUOTATION MARK
{ /* 0x8167, */ 0x201C },	// 8167 E2809C [“]  #LEFT DOUBLE QUOTATION MARK
{ /* 0x8168, */ 0x201D },	// 8168 E2809D [”]  #RIGHT DOUBLE QUOTATION MARK
{ /* 0x8169, */ 0xFF08 },	// 8169 EFBC88 [（]  #FULLWIDTH LEFT PARENTHESIS
{ /* 0x816A, */ 0xFF09 },	// 816A EFBC89 [）]  #FULLWIDTH RIGHT PARENTHESIS
{ /* 0x816B, */ 0x3014 },	// 816B E38094 [〔]  #LEFT TORTOISE SHELL BRACKET
{ /* 0x816C, */ 0x3015 },	// 816C E38095 [〕]  #RIGHT TORTOISE SHELL BRACKET
{ /* 0x816D, */ 0xFF3B },	// 816D EFBCBB [［]  #FULLWIDTH LEFT SQUARE BRACKET
{ /* 0x816E, */ 0xFF3D },	// 816E EFBCBD [］]  #FULLWIDTH RIGHT SQUARE BRACKET
{ /* 0x816F, */ 0xFF5B },	// 816F EFBD9B [｛]  #FULLWIDTH LEFT CURLY BRACKET
{ /* 0x8170, */ 0xFF5D },	// 8170 EFBD9D [｝]  #FULLWIDTH RIGHT CURLY BRACKET
{ /* 0x8171, */ 0x3008 },	// 8171 E38088 [〈]  #LEFT ANGLE BRACKET
{ /* 0x8172, */ 0x3009 },	// 8172 E38089 [〉]  #RIGHT ANGLE BRACKET
{ /* 0x8173, */ 0x300A },	// 8173 E3808A [《]  #LEFT DOUBLE ANGLE BRACKET
{ /* 0x8174, */ 0x300B },	// 8174 E3808B [》]  #RIGHT DOUBLE ANGLE BRACKET
{ /* 0x8175, */ 0x300C },	// 8175 E3808C [「]  #LEFT CORNER BRACKET
{ /* 0x8176, */ 0x300D },	// 8176 E3808D [」]  #RIGHT CORNER BRACKET
{ /* 0x8177, */ 0x300E },	// 8177 E3808E [『]  #LEFT WHITE CORNER BRACKET
{ /* 0x8178, */ 0x300F },	// 8178 E3808F [』]  #RIGHT WHITE CORNER BRACKET
{ /* 0x8179, */ 0x3010 },	// 8179 E38090 [【]  #LEFT BLACK LENTICULAR BRACKET
{ /* 0x817A, */ 0x3011 },	// 817A E38091 [】]  #RIGHT BLACK LENTICULAR BRACKET
{ /* 0x817B, */ 0xFF0B },	// 817B EFBC8B [＋]  #FULLWIDTH PLUS SIGN
{ /* 0x817C, */ 0xFF0D },	// 817C E28892 [－]  #FULLWIDTH HYPHEN-MINUS
{ /* 0x817D, */ 0x00B1 },	// 817D C2B1   [±]  #PLUS-MINUS SIGN
{ /* 0x817E, */ 0x00D7 },	// 817E C397   [×]  #MULTIPLICATION SIGN
{ /* 0x817F, */ 0x0000 },	// 		  #ダミー
{ /* 0x8180, */ 0x00F7 },	// 8180 C3B7   [÷]  #DIVISION SIGN
{ /* 0x8181, */ 0xFF1D },	// 8181 EFBC9D [＝]  #FULLWIDTH EQUALS SIGN
{ /* 0x8182, */ 0x2260 },	// 8182 E289A0 [≠]  #NOT EQUAL TO
{ /* 0x8183, */ 0xFF1C },	// 8183 EFBC9C [＜]  #FULLWIDTH LESS-THAN SIGN
{ /* 0x8184, */ 0xFF1E },	// 8184 EFBC9E [＞]  #FULLWIDTH GREATER-THAN SIGN
{ /* 0x8185, */ 0x2266 },	// 8185 E289A6 [≦]  #LESS-THAN OVER EQUAL TO
{ /* 0x8186, */ 0x2267 },	// 8186 E289A7 [≧]  #GREATER-THAN OVER EQUAL TO
{ /* 0x8187, */ 0x221E },	// 8187 E2889E [∞]  #INFINITY
{ /* 0x8188, */ 0x2234 },	// 8188 E288B4 [∴]  #THEREFORE
{ /* 0x8189, */ 0x2642 },	// 8189 E29982 [♂]  #MALE SIGN
{ /* 0x818A, */ 0x2640 },	// 818A E29980 [♀]  #FEMALE SIGN
{ /* 0x818B, */ 0x00B0 },	// 818B C2B0   [°]  #DEGREE SIGN
{ /* 0x818C, */ 0x2032 },	// 818C E280B2 [′]  #PRIME
{ /* 0x818D, */ 0x2033 },	// 818D E280B3 [″]  #DOUBLE PRIME
{ /* 0x818E, */ 0x2103 },	// 818E E28483 [℃]  #DEGREE CELSIUS
{ /* 0x818F, */ 0xFFE5 },	// 818F EFBFA5 [￥]  #FULLWIDTH YEN SIGN
{ /* 0x8190, */ 0xFF04 },	// 8190 EFBC84 [＄]  #FULLWIDTH DOLLAR SIGN
{ /* 0x8191, */ 0xFFE0 },	// 8191 C2A2   [￠]  #FULLWIDTH CENT SIGN
{ /* 0x8192, */ 0xFFE1 },	// 8192 C2A3   [￡]  #FULLWIDTH POUND SIGN
{ /* 0x8193, */ 0xFF05 },	// 8193 EFBC85 [％]  #FULLWIDTH PERCENT SIGN
{ /* 0x8194, */ 0xFF03 },	// 8194 EFBC83 [＃]  #FULLWIDTH NUMBER SIGN
{ /* 0x8195, */ 0xFF06 },	// 8195 EFBC86 [＆]  #FULLWIDTH AMPERSAND
{ /* 0x8196, */ 0xFF0A },	// 8196 EFBC8A [＊]  #FULLWIDTH ASTERISK
{ /* 0x8197, */ 0xFF20 },	// 8197 EFBCA0 [＠]  #FULLWIDTH COMMERCIAL AT
{ /* 0x8198, */ 0x00A7 },	// 8198 C2A7   [§]  #SECTION SIGN
{ /* 0x8199, */ 0x2606 },	// 8199 E29886 [☆]  #WHITE STAR
{ /* 0x819A, */ 0x2605 },	// 819A E29885 [★]  #BLACK STAR
{ /* 0x819B, */ 0x25CB },	// 819B E2978B [○]  #WHITE CIRCLE
{ /* 0x819C, */ 0x25CF },	// 819C E2978F [●]  #BLACK CIRCLE
{ /* 0x819D, */ 0x25CE },	// 819D E2978E [◎]  #BULLSEYE
{ /* 0x819E, */ 0x25C7 },	// 819E E29787 [◇]  #WHITE DIAMOND
{ /* 0x819F, */ 0x25C6 },	// 819F E29786 [◆]  #BLACK DIAMOND
{ /* 0x81A0, */ 0x25A1 },	// 81A0 E296A1 [□]  #WHITE SQUARE
{ /* 0x81A1, */ 0x25A0 },	// 81A1 E296A0 [■]  #BLACK SQUARE
{ /* 0x81A2, */ 0x25B3 },	// 81A2 E296B3 [△]  #WHITE UP-POINTING TRIANGLE
{ /* 0x81A3, */ 0x25B2 },	// 81A3 E296B2 [▲]  #BLACK UP-POINTING TRIANGLE
{ /* 0x81A4, */ 0x25BD },	// 81A4 E296BD [▽]  #WHITE DOWN-POINTING TRIANGLE
{ /* 0x81A5, */ 0x25BC },	// 81A5 E296BC [▼]  #BLACK DOWN-POINTING TRIANGLE
{ /* 0x81A6, */ 0x203B },	// 81A6 E280BB [※]  #REFERENCE MARK
{ /* 0x81A7, */ 0x3012 },	// 81A7 E38092 [〒]  #POSTAL MARK
{ /* 0x81A8, */ 0x2192 },	// 81A8 E28692 [→]  #RIGHTWARDS ARROW
{ /* 0x81A9, */ 0x2190 },	// 81A9 E28690 [←]  #LEFTWARDS ARROW
{ /* 0x81AA, */ 0x2191 },	// 81AA E28691 [↑]  #UPWARDS ARROW
{ /* 0x81AB, */ 0x2193 },	// 81AB E28693 [↓]  #DOWNWARDS ARROW
{ /* 0x81AC, */ 0x3013 },	// 81AC E38093 [〓]  #GETA MARK
{ /* 0x81AD, */ 0x0000 },	// 81AD ------ [　]  
{ /* 0x81AE, */ 0x0000 },	// 81AE ------ [　]  
{ /* 0x81AF, */ 0x0000 },	// 81AF ------ [　]  
{ /* 0x81B0, */ 0x0000 },	// 81B0 ------ [　]  
{ /* 0x81B1, */ 0x0000 },	// 81B1 ------ [　]  
{ /* 0x81B2, */ 0x0000 },	// 81B2 ------ [　]  
{ /* 0x81B3, */ 0x0000 },	// 81B3 ------ [　]  
{ /* 0x81B4, */ 0x0000 },	// 81B4 ------ [　]  
{ /* 0x81B5, */ 0x0000 },	// 81B5 ------ [　]  
{ /* 0x81B6, */ 0x0000 },	// 81B6 ------ [　]  
{ /* 0x81B7, */ 0x0000 },	// 81B7 ------ [　]  
{ /* 0x81B8, */ 0x2208 },	// 81B8 E28888 [∈]  #ELEMENT OF
{ /* 0x81B9, */ 0x220B },	// 81B9 E2888B [∋]  #CONTAINS AS MEMBER
{ /* 0x81BA, */ 0x2286 },	// 81BA E28A86 [⊆]  #SUBSET OF OR EQUAL TO
{ /* 0x81BB, */ 0x2287 },	// 81BB E28A87 [⊇]  #SUPERSET OF OR EQUAL TO
{ /* 0x81BC, */ 0x2282 },	// 81BC E28A82 [⊂]  #SUBSET OF
{ /* 0x81BD, */ 0x2283 },	// 81BD E28A83 [⊃]  #SUPERSET OF
{ /* 0x81BE, */ 0x222A },	// 81BE E288AA [∪]  #UNION
{ /* 0x81BF, */ 0x2229 },	// 81BF E288A9 [∩]  #INTERSECTION
{ /* 0x81C0, */ 0x0000 },	// 81C0 ------ [　]  
{ /* 0x81C1, */ 0x0000 },	// 81C1 ------ [　]  
{ /* 0x81C2, */ 0x0000 },	// 81C2 ------ [　]  
{ /* 0x81C3, */ 0x0000 },	// 81C3 ------ [　]  
{ /* 0x81C4, */ 0x0000 },	// 81C4 ------ [　]  
{ /* 0x81C5, */ 0x0000 },	// 81C5 ------ [　]  
{ /* 0x81C6, */ 0x0000 },	// 81C6 ------ [　]  
{ /* 0x81C7, */ 0x0000 },	// 81C7 ------ [　]  
{ /* 0x81C8, */ 0x2227 },	// 81C8 E288A7 [∧]  #LOGICAL AND
{ /* 0x81C9, */ 0x2228 },	// 81C9 E288A8 [∨]  #LOGICAL OR
{ /* 0x81CA, */ 0xFFE2 },	// 81CA C2AC   [￢]  #FULLWIDTH NOT SIGN
{ /* 0x81CB, */ 0x21D2 },	// 81CB E28792 [⇒]  #RIGHTWARDS DOUBLE ARROW
{ /* 0x81CC, */ 0x21D4 },	// 81CC E28794 [⇔]  #LEFT RIGHT DOUBLE ARROW
{ /* 0x81CD, */ 0x2200 },	// 81CD E28880 [∀]  #FOR ALL
{ /* 0x81CE, */ 0x2203 },	// 81CE E28883 [∃]  #THERE EXISTS
{ /* 0x81CF, */ 0x0000 },	// 81CF ------ [　]  
{ /* 0x81D0, */ 0x0000 },	// 81D0 ------ [　]  
{ /* 0x81D1, */ 0x0000 },	// 81D1 ------ [　]  
{ /* 0x81D2, */ 0x0000 },	// 81D2 ------ [　]  
{ /* 0x81D3, */ 0x0000 },	// 81D3 ------ [　]  
{ /* 0x81D4, */ 0x0000 },	// 81D4 ------ [　]  
{ /* 0x81D5, */ 0x0000 },	// 81D5 ------ [　]  
{ /* 0x81D6, */ 0x0000 },	// 81D6 ------ [　]  
{ /* 0x81D7, */ 0x0000 },	// 81D7 ------ [　]  
{ /* 0x81D8, */ 0x0000 },	// 81D8 ------ [　]  
{ /* 0x81D9, */ 0x0000 },	// 81D9 ------ [　]  
{ /* 0x81DA, */ 0x2220 },	// 81DA E288A0 [∠]  #ANGLE
{ /* 0x81DB, */ 0x22A5 },	// 81DB E28AA5 [⊥]  #UP TACK
{ /* 0x81DC, */ 0x2312 },	// 81DC E28C92 [⌒]  #ARC
{ /* 0x81DD, */ 0x2202 },	// 81DD E28882 [∂]  #PARTIAL DIFFERENTIAL
{ /* 0x81DE, */ 0x2207 },	// 81DE E28887 [∇]  #NABLA
{ /* 0x81DF, */ 0x2261 },	// 81DF E289A1 [≡]  #IDENTICAL TO
{ /* 0x81E0, */ 0x2252 },	// 81E0 E28992 [≒]  #APPROXIMATELY EQUAL TO OR THE IMAGE OF
{ /* 0x81E1, */ 0x226A },	// 81E1 E289AA [≪]  #MUCH LESS-THAN
{ /* 0x81E2, */ 0x226B },	// 81E2 E289AB [≫]  #MUCH GREATER-THAN
{ /* 0x81E3, */ 0x221A },	// 81E3 E2889A [√]  #SQUARE ROOT
{ /* 0x81E4, */ 0x223D },	// 81E4 E288BD [∽]  #REVERSED TILDE
{ /* 0x81E5, */ 0x221D },	// 81E5 E2889D [∝]  #PROPORTIONAL TO
{ /* 0x81E6, */ 0x2235 },	// 81E6 E288B5 [∵]  #BECAUSE
{ /* 0x81E7, */ 0x222B },	// 81E7 E288AB [∫]  #INTEGRAL
{ /* 0x81E8, */ 0x222C },	// 81E8 E288AC [∬]  #DOUBLE INTEGRAL
{ /* 0x81E9, */ 0x0000 },	// 81E9 ------ [　]  
{ /* 0x81EA, */ 0x0000 },	// 81EA ------ [　]  
{ /* 0x81EB, */ 0x0000 },	// 81EB ------ [　]  
{ /* 0x81EC, */ 0x0000 },	// 81EC ------ [　]  
{ /* 0x81ED, */ 0x0000 },	// 81ED ------ [　]  
{ /* 0x81EE, */ 0x0000 },	// 81EE ------ [　]  
{ /* 0x81EF, */ 0x0000 },	// 81EF ------ [　]  
{ /* 0x81F0, */ 0x212B },	// 81F0 E284AB [Å]  #ANGSTROM SIGN
{ /* 0x81F1, */ 0x2030 },	// 81F1 E280B0 [‰]  #PER MILLE SIGN
{ /* 0x81F2, */ 0x266F },	// 81F2 E299AF [♯]  #MUSIC SHARP SIGN
{ /* 0x81F3, */ 0x266D },	// 81F3 E299AD [♭]  #MUSIC FLAT SIGN
{ /* 0x81F4, */ 0x266A },	// 81F4 E299AA [♪]  #EIGHTH NOTE
{ /* 0x81F5, */ 0x2020 },	// 81F5 E280A0 [†]  #DAGGER
{ /* 0x81F6, */ 0x2021 },	// 81F6 E280A1 [‡]  #DOUBLE DAGGER
{ /* 0x81F7, */ 0x00B6 },	// 81F7 C2B6   [¶]  #PILCROW SIGN
{ /* 0x81F8, */ 0x0000 },	// 81F8 ------ [　]  
{ /* 0x81F9, */ 0x0000 },	// 81F9 ------ [　]  
{ /* 0x81FA, */ 0x0000 },	// 81FA ------ [　]  
{ /* 0x81FB, */ 0x0000 },	// 81FB ------ [　]  
{ /* 0x81FC, */ 0x25EF },	// 81FC E297AF [◯]  #LARGE CIRCLE
//===== 0x82 40～FC ( テーブル数 [189]  ）====================
{ /* 0x8240, */ 0x0000 },	// 8240 ------ [　]  
{ /* 0x8241, */ 0x0000 },	// 8241 ------ [　]  
{ /* 0x8242, */ 0x0000 },	// 8242 ------ [　]  
{ /* 0x8243, */ 0x0000 },	// 8243 ------ [　]  
{ /* 0x8244, */ 0x0000 },	// 8244 ------ [　]  
{ /* 0x8245, */ 0x0000 },	// 8245 ------ [　]  
{ /* 0x8246, */ 0x0000 },	// 8246 ------ [　]  
{ /* 0x8247, */ 0x0000 },	// 8247 ------ [　]  
{ /* 0x8248, */ 0x0000 },	// 8248 ------ [　]  
{ /* 0x8249, */ 0x0000 },	// 8249 ------ [　]  
{ /* 0x824A, */ 0x0000 },	// 824A ------ [　]  
{ /* 0x824B, */ 0x0000 },	// 824B ------ [　]  
{ /* 0x824C, */ 0x0000 },	// 824C ------ [　]  
{ /* 0x824D, */ 0x0000 },	// 824D ------ [　]  
{ /* 0x824E, */ 0x0000 },	// 824E ------ [　]  
{ /* 0x824F, */ 0xFF10 },	// 824F EFBC90 [０]  #FULLWIDTH DIGIT ZERO
{ /* 0x8250, */ 0xFF11 },	// 8250 EFBC91 [１]  #FULLWIDTH DIGIT ONE
{ /* 0x8251, */ 0xFF12 },	// 8251 EFBC92 [２]  #FULLWIDTH DIGIT TWO
{ /* 0x8252, */ 0xFF13 },	// 8252 EFBC93 [３]  #FULLWIDTH DIGIT THREE
{ /* 0x8253, */ 0xFF14 },	// 8253 EFBC94 [４]  #FULLWIDTH DIGIT FOUR
{ /* 0x8254, */ 0xFF15 },	// 8254 EFBC95 [５]  #FULLWIDTH DIGIT FIVE
{ /* 0x8255, */ 0xFF16 },	// 8255 EFBC96 [６]  #FULLWIDTH DIGIT SIX
{ /* 0x8256, */ 0xFF17 },	// 8256 EFBC97 [７]  #FULLWIDTH DIGIT SEVEN
{ /* 0x8257, */ 0xFF18 },	// 8257 EFBC98 [８]  #FULLWIDTH DIGIT EIGHT
{ /* 0x8258, */ 0xFF19 },	// 8258 EFBC99 [９]  #FULLWIDTH DIGIT NINE
{ /* 0x8259, */ 0x0000 },	// 8259 ------ [　]  
{ /* 0x825A, */ 0x0000 },	// 825A ------ [　]  
{ /* 0x825B, */ 0x0000 },	// 825B ------ [　]  
{ /* 0x825C, */ 0x0000 },	// 825C ------ [　]  
{ /* 0x825D, */ 0x0000 },	// 825D ------ [　]  
{ /* 0x825E, */ 0x0000 },	// 825E ------ [　]  
{ /* 0x825F, */ 0x0000 },	// 825F ------ [　]  
{ /* 0x8260, */ 0xFF21 },	// 8260 EFBCA1 [Ａ]  #FULLWIDTH LATIN CAPITAL LETTER A
{ /* 0x8261, */ 0xFF22 },	// 8261 EFBCA2 [Ｂ]  #FULLWIDTH LATIN CAPITAL LETTER B
{ /* 0x8262, */ 0xFF23 },	// 8262 EFBCA3 [Ｃ]  #FULLWIDTH LATIN CAPITAL LETTER C
{ /* 0x8263, */ 0xFF24 },	// 8263 EFBCA4 [Ｄ]  #FULLWIDTH LATIN CAPITAL LETTER D
{ /* 0x8264, */ 0xFF25 },	// 8264 EFBCA5 [Ｅ]  #FULLWIDTH LATIN CAPITAL LETTER E
{ /* 0x8265, */ 0xFF26 },	// 8265 EFBCA6 [Ｆ]  #FULLWIDTH LATIN CAPITAL LETTER F
{ /* 0x8266, */ 0xFF27 },	// 8266 EFBCA7 [Ｇ]  #FULLWIDTH LATIN CAPITAL LETTER G
{ /* 0x8267, */ 0xFF28 },	// 8267 EFBCA8 [Ｈ]  #FULLWIDTH LATIN CAPITAL LETTER H
{ /* 0x8268, */ 0xFF29 },	// 8268 EFBCA9 [Ｉ]  #FULLWIDTH LATIN CAPITAL LETTER I
{ /* 0x8269, */ 0xFF2A },	// 8269 EFBCAA [Ｊ]  #FULLWIDTH LATIN CAPITAL LETTER J
{ /* 0x826A, */ 0xFF2B },	// 826A EFBCAB [Ｋ]  #FULLWIDTH LATIN CAPITAL LETTER K
{ /* 0x826B, */ 0xFF2C },	// 826B EFBCAC [Ｌ]  #FULLWIDTH LATIN CAPITAL LETTER L
{ /* 0x826C, */ 0xFF2D },	// 826C EFBCAD [Ｍ]  #FULLWIDTH LATIN CAPITAL LETTER M
{ /* 0x826D, */ 0xFF2E },	// 826D EFBCAE [Ｎ]  #FULLWIDTH LATIN CAPITAL LETTER N
{ /* 0x826E, */ 0xFF2F },	// 826E EFBCAF [Ｏ]  #FULLWIDTH LATIN CAPITAL LETTER O
{ /* 0x826F, */ 0xFF30 },	// 826F EFBCB0 [Ｐ]  #FULLWIDTH LATIN CAPITAL LETTER P
{ /* 0x8270, */ 0xFF31 },	// 8270 EFBCB1 [Ｑ]  #FULLWIDTH LATIN CAPITAL LETTER Q
{ /* 0x8271, */ 0xFF32 },	// 8271 EFBCB2 [Ｒ]  #FULLWIDTH LATIN CAPITAL LETTER R
{ /* 0x8272, */ 0xFF33 },	// 8272 EFBCB3 [Ｓ]  #FULLWIDTH LATIN CAPITAL LETTER S
{ /* 0x8273, */ 0xFF34 },	// 8273 EFBCB4 [Ｔ]  #FULLWIDTH LATIN CAPITAL LETTER T
{ /* 0x8274, */ 0xFF35 },	// 8274 EFBCB5 [Ｕ]  #FULLWIDTH LATIN CAPITAL LETTER U
{ /* 0x8275, */ 0xFF36 },	// 8275 EFBCB6 [Ｖ]  #FULLWIDTH LATIN CAPITAL LETTER V
{ /* 0x8276, */ 0xFF37 },	// 8276 EFBCB7 [Ｗ]  #FULLWIDTH LATIN CAPITAL LETTER W
{ /* 0x8277, */ 0xFF38 },	// 8277 EFBCB8 [Ｘ]  #FULLWIDTH LATIN CAPITAL LETTER X
{ /* 0x8278, */ 0xFF39 },	// 8278 EFBCB9 [Ｙ]  #FULLWIDTH LATIN CAPITAL LETTER Y
{ /* 0x8279, */ 0xFF3A },	// 8279 EFBCBA [Ｚ]  #FULLWIDTH LATIN CAPITAL LETTER Z
{ /* 0x827A, */ 0x0000 },	// 827A ------ [　]  
{ /* 0x827B, */ 0x0000 },	// 827B ------ [　]  
{ /* 0x827C, */ 0x0000 },	// 827C ------ [　]  
{ /* 0x827D, */ 0x0000 },	// 827D ------ [　]  
{ /* 0x827E, */ 0x0000 },	// 827E ------ [　]  
{ /* 0x827F, */ 0x0000 },	// ダミー
{ /* 0x8280, */ 0x0000 },	// 8280 ------ [　]  
{ /* 0x8281, */ 0xFF41 },	// 8281 EFBD81 [ａ]  #FULLWIDTH LATIN SMALL LETTER A
{ /* 0x8282, */ 0xFF42 },	// 8282 EFBD82 [ｂ]  #FULLWIDTH LATIN SMALL LETTER B
{ /* 0x8283, */ 0xFF43 },	// 8283 EFBD83 [ｃ]  #FULLWIDTH LATIN SMALL LETTER C
{ /* 0x8284, */ 0xFF44 },	// 8284 EFBD84 [ｄ]  #FULLWIDTH LATIN SMALL LETTER D
{ /* 0x8285, */ 0xFF45 },	// 8285 EFBD85 [ｅ]  #FULLWIDTH LATIN SMALL LETTER E
{ /* 0x8286, */ 0xFF46 },	// 8286 EFBD86 [ｆ]  #FULLWIDTH LATIN SMALL LETTER F
{ /* 0x8287, */ 0xFF47 },	// 8287 EFBD87 [ｇ]  #FULLWIDTH LATIN SMALL LETTER G
{ /* 0x8288, */ 0xFF48 },	// 8288 EFBD88 [ｈ]  #FULLWIDTH LATIN SMALL LETTER H
{ /* 0x8289, */ 0xFF49 },	// 8289 EFBD89 [ｉ]  #FULLWIDTH LATIN SMALL LETTER I
{ /* 0x828A, */ 0xFF4A },	// 828A EFBD8A [ｊ]  #FULLWIDTH LATIN SMALL LETTER J
{ /* 0x828B, */ 0xFF4B },	// 828B EFBD8B [ｋ]  #FULLWIDTH LATIN SMALL LETTER K
{ /* 0x828C, */ 0xFF4C },	// 828C EFBD8C [ｌ]  #FULLWIDTH LATIN SMALL LETTER L
{ /* 0x828D, */ 0xFF4D },	// 828D EFBD8D [ｍ]  #FULLWIDTH LATIN SMALL LETTER M
{ /* 0x828E, */ 0xFF4E },	// 828E EFBD8E [ｎ]  #FULLWIDTH LATIN SMALL LETTER N
{ /* 0x828F, */ 0xFF4F },	// 828F EFBD8F [ｏ]  #FULLWIDTH LATIN SMALL LETTER O
{ /* 0x8290, */ 0xFF50 },	// 8290 EFBD90 [ｐ]  #FULLWIDTH LATIN SMALL LETTER P
{ /* 0x8291, */ 0xFF51 },	// 8291 EFBD91 [ｑ]  #FULLWIDTH LATIN SMALL LETTER Q
{ /* 0x8292, */ 0xFF52 },	// 8292 EFBD92 [ｒ]  #FULLWIDTH LATIN SMALL LETTER R
{ /* 0x8293, */ 0xFF53 },	// 8293 EFBD93 [ｓ]  #FULLWIDTH LATIN SMALL LETTER S
{ /* 0x8294, */ 0xFF54 },	// 8294 EFBD94 [ｔ]  #FULLWIDTH LATIN SMALL LETTER T
{ /* 0x8295, */ 0xFF55 },	// 8295 EFBD95 [ｕ]  #FULLWIDTH LATIN SMALL LETTER U
{ /* 0x8296, */ 0xFF56 },	// 8296 EFBD96 [ｖ]  #FULLWIDTH LATIN SMALL LETTER V
{ /* 0x8297, */ 0xFF57 },	// 8297 EFBD97 [ｗ]  #FULLWIDTH LATIN SMALL LETTER W
{ /* 0x8298, */ 0xFF58 },	// 8298 EFBD98 [ｘ]  #FULLWIDTH LATIN SMALL LETTER X
{ /* 0x8299, */ 0xFF59 },	// 8299 EFBD99 [ｙ]  #FULLWIDTH LATIN SMALL LETTER Y
{ /* 0x829A, */ 0xFF5A },	// 829A EFBD9A [ｚ]  #FULLWIDTH LATIN SMALL LETTER Z
{ /* 0x829B, */ 0x0000 },	// 829B ------ [　]  
{ /* 0x829C, */ 0x0000 },	// 829C ------ [　]  
{ /* 0x829D, */ 0x0000 },	// 829D ------ [　]  
{ /* 0x829E, */ 0x0000 },	// 829E ------ [　]  
{ /* 0x829F, */ 0x3041 },	// 829F E38181 [ぁ]  #HIRAGANA LETTER SMALL A
{ /* 0x82A0, */ 0x3042 },	// 82A0 E38182 [あ]  #HIRAGANA LETTER A
{ /* 0x82A1, */ 0x3043 },	// 82A1 E38183 [ぃ]  #HIRAGANA LETTER SMALL I
{ /* 0x82A2, */ 0x3044 },	// 82A2 E38184 [い]  #HIRAGANA LETTER I
{ /* 0x82A3, */ 0x3045 },	// 82A3 E38185 [ぅ]  #HIRAGANA LETTER SMALL U
{ /* 0x82A4, */ 0x3046 },	// 82A4 E38186 [う]  #HIRAGANA LETTER U
{ /* 0x82A5, */ 0x3047 },	// 82A5 E38187 [ぇ]  #HIRAGANA LETTER SMALL E
{ /* 0x82A6, */ 0x3048 },	// 82A6 E38188 [え]  #HIRAGANA LETTER E
{ /* 0x82A7, */ 0x3049 },	// 82A7 E38189 [ぉ]  #HIRAGANA LETTER SMALL O
{ /* 0x82A8, */ 0x304A },	// 82A8 E3818A [お]  #HIRAGANA LETTER O
{ /* 0x82A9, */ 0x304B },	// 82A9 E3818B [か]  #HIRAGANA LETTER KA
{ /* 0x82AA, */ 0x304C },	// 82AA E3818C [が]  #HIRAGANA LETTER GA
{ /* 0x82AB, */ 0x304D },	// 82AB E3818D [き]  #HIRAGANA LETTER KI
{ /* 0x82AC, */ 0x304E },	// 82AC E3818E [ぎ]  #HIRAGANA LETTER GI
{ /* 0x82AD, */ 0x304F },	// 82AD E3818F [く]  #HIRAGANA LETTER KU
{ /* 0x82AE, */ 0x3050 },	// 82AE E38190 [ぐ]  #HIRAGANA LETTER GU
{ /* 0x82AF, */ 0x3051 },	// 82AF E38191 [け]  #HIRAGANA LETTER KE
{ /* 0x82B0, */ 0x3052 },	// 82B0 E38192 [げ]  #HIRAGANA LETTER GE
{ /* 0x82B1, */ 0x3053 },	// 82B1 E38193 [こ]  #HIRAGANA LETTER KO
{ /* 0x82B2, */ 0x3054 },	// 82B2 E38194 [ご]  #HIRAGANA LETTER GO
{ /* 0x82B3, */ 0x3055 },	// 82B3 E38195 [さ]  #HIRAGANA LETTER SA
{ /* 0x82B4, */ 0x3056 },	// 82B4 E38196 [ざ]  #HIRAGANA LETTER ZA
{ /* 0x82B5, */ 0x3057 },	// 82B5 E38197 [し]  #HIRAGANA LETTER SI
{ /* 0x82B6, */ 0x3058 },	// 82B6 E38198 [じ]  #HIRAGANA LETTER ZI
{ /* 0x82B7, */ 0x3059 },	// 82B7 E38199 [す]  #HIRAGANA LETTER SU
{ /* 0x82B8, */ 0x305A },	// 82B8 E3819A [ず]  #HIRAGANA LETTER ZU
{ /* 0x82B9, */ 0x305B },	// 82B9 E3819B [せ]  #HIRAGANA LETTER SE
{ /* 0x82BA, */ 0x305C },	// 82BA E3819C [ぜ]  #HIRAGANA LETTER ZE
{ /* 0x82BB, */ 0x305D },	// 82BB E3819D [そ]  #HIRAGANA LETTER SO
{ /* 0x82BC, */ 0x305E },	// 82BC E3819E [ぞ]  #HIRAGANA LETTER ZO
{ /* 0x82BD, */ 0x305F },	// 82BD E3819F [た]  #HIRAGANA LETTER TA
{ /* 0x82BE, */ 0x3060 },	// 82BE E381A0 [だ]  #HIRAGANA LETTER DA
{ /* 0x82BF, */ 0x3061 },	// 82BF E381A1 [ち]  #HIRAGANA LETTER TI
{ /* 0x82C0, */ 0x3062 },	// 82C0 E381A2 [ぢ]  #HIRAGANA LETTER DI
{ /* 0x82C1, */ 0x3063 },	// 82C1 E381A3 [っ]  #HIRAGANA LETTER SMALL TU
{ /* 0x82C2, */ 0x3064 },	// 82C2 E381A4 [つ]  #HIRAGANA LETTER TU
{ /* 0x82C3, */ 0x3065 },	// 82C3 E381A5 [づ]  #HIRAGANA LETTER DU
{ /* 0x82C4, */ 0x3066 },	// 82C4 E381A6 [て]  #HIRAGANA LETTER TE
{ /* 0x82C5, */ 0x3067 },	// 82C5 E381A7 [で]  #HIRAGANA LETTER DE
{ /* 0x82C6, */ 0x3068 },	// 82C6 E381A8 [と]  #HIRAGANA LETTER TO
{ /* 0x82C7, */ 0x3069 },	// 82C7 E381A9 [ど]  #HIRAGANA LETTER DO
{ /* 0x82C8, */ 0x306A },	// 82C8 E381AA [な]  #HIRAGANA LETTER NA
{ /* 0x82C9, */ 0x306B },	// 82C9 E381AB [に]  #HIRAGANA LETTER NI
{ /* 0x82CA, */ 0x306C },	// 82CA E381AC [ぬ]  #HIRAGANA LETTER NU
{ /* 0x82CB, */ 0x306D },	// 82CB E381AD [ね]  #HIRAGANA LETTER NE
{ /* 0x82CC, */ 0x306E },	// 82CC E381AE [の]  #HIRAGANA LETTER NO
{ /* 0x82CD, */ 0x306F },	// 82CD E381AF [は]  #HIRAGANA LETTER HA
{ /* 0x82CE, */ 0x3070 },	// 82CE E381B0 [ば]  #HIRAGANA LETTER BA
{ /* 0x82CF, */ 0x3071 },	// 82CF E381B1 [ぱ]  #HIRAGANA LETTER PA
{ /* 0x82D0, */ 0x3072 },	// 82D0 E381B2 [ひ]  #HIRAGANA LETTER HI
{ /* 0x82D1, */ 0x3073 },	// 82D1 E381B3 [び]  #HIRAGANA LETTER BI
{ /* 0x82D2, */ 0x3074 },	// 82D2 E381B4 [ぴ]  #HIRAGANA LETTER PI
{ /* 0x82D3, */ 0x3075 },	// 82D3 E381B5 [ふ]  #HIRAGANA LETTER HU
{ /* 0x82D4, */ 0x3076 },	// 82D4 E381B6 [ぶ]  #HIRAGANA LETTER BU
{ /* 0x82D5, */ 0x3077 },	// 82D5 E381B7 [ぷ]  #HIRAGANA LETTER PU
{ /* 0x82D6, */ 0x3078 },	// 82D6 E381B8 [へ]  #HIRAGANA LETTER HE
{ /* 0x82D7, */ 0x3079 },	// 82D7 E381B9 [べ]  #HIRAGANA LETTER BE
{ /* 0x82D8, */ 0x307A },	// 82D8 E381BA [ぺ]  #HIRAGANA LETTER PE
{ /* 0x82D9, */ 0x307B },	// 82D9 E381BB [ほ]  #HIRAGANA LETTER HO
{ /* 0x82DA, */ 0x307C },	// 82DA E381BC [ぼ]  #HIRAGANA LETTER BO
{ /* 0x82DB, */ 0x307D },	// 82DB E381BD [ぽ]  #HIRAGANA LETTER PO
{ /* 0x82DC, */ 0x307E },	// 82DC E381BE [ま]  #HIRAGANA LETTER MA
{ /* 0x82DD, */ 0x307F },	// 82DD E381BF [み]  #HIRAGANA LETTER MI
{ /* 0x82DE, */ 0x3080 },	// 82DE E38280 [む]  #HIRAGANA LETTER MU
{ /* 0x82DF, */ 0x3081 },	// 82DF E38281 [め]  #HIRAGANA LETTER ME
{ /* 0x82E0, */ 0x3082 },	// 82E0 E38282 [も]  #HIRAGANA LETTER MO
{ /* 0x82E1, */ 0x3083 },	// 82E1 E38283 [ゃ]  #HIRAGANA LETTER SMALL YA
{ /* 0x82E2, */ 0x3084 },	// 82E2 E38284 [や]  #HIRAGANA LETTER YA
{ /* 0x82E3, */ 0x3085 },	// 82E3 E38285 [ゅ]  #HIRAGANA LETTER SMALL YU
{ /* 0x82E4, */ 0x3086 },	// 82E4 E38286 [ゆ]  #HIRAGANA LETTER YU
{ /* 0x82E5, */ 0x3087 },	// 82E5 E38287 [ょ]  #HIRAGANA LETTER SMALL YO
{ /* 0x82E6, */ 0x3088 },	// 82E6 E38288 [よ]  #HIRAGANA LETTER YO
{ /* 0x82E7, */ 0x3089 },	// 82E7 E38289 [ら]  #HIRAGANA LETTER RA
{ /* 0x82E8, */ 0x308A },	// 82E8 E3828A [り]  #HIRAGANA LETTER RI
{ /* 0x82E9, */ 0x308B },	// 82E9 E3828B [る]  #HIRAGANA LETTER RU
{ /* 0x82EA, */ 0x308C },	// 82EA E3828C [れ]  #HIRAGANA LETTER RE
{ /* 0x82EB, */ 0x308D },	// 82EB E3828D [ろ]  #HIRAGANA LETTER RO
{ /* 0x82EC, */ 0x308E },	// 82EC E3828E [ゎ]  #HIRAGANA LETTER SMALL WA
{ /* 0x82ED, */ 0x308F },	// 82ED E3828F [わ]  #HIRAGANA LETTER WA
{ /* 0x82EE, */ 0x3090 },	// 82EE E38290 [ゐ]  #HIRAGANA LETTER WI
{ /* 0x82EF, */ 0x3091 },	// 82EF E38291 [ゑ]  #HIRAGANA LETTER WE
{ /* 0x82F0, */ 0x3092 },	// 82F0 E38292 [を]  #HIRAGANA LETTER WO
{ /* 0x82F1, */ 0x3093 },	// 82F1 E38293 [ん]  #HIRAGANA LETTER N
{ /* 0x82F2, */ 0x0000 },	// 82F2 ------ [　]  
{ /* 0x82F3, */ 0x0000 },	// 82F3 ------ [　]  
{ /* 0x82F4, */ 0x0000 },	// 82F4 ------ [　]  
{ /* 0x82F5, */ 0x0000 },	// 82F5 ------ [　]  
{ /* 0x82F6, */ 0x0000 },	// 82F6 ------ [　]  
{ /* 0x82F7, */ 0x0000 },	// 82F7 ------ [　]  
{ /* 0x82F8, */ 0x0000 },	// 82F8 ------ [　]  
{ /* 0x82F9, */ 0x0000 },	// 82F9 ------ [　]  
{ /* 0x82FA, */ 0x0000 },	// 82FA ------ [　]  
{ /* 0x82FB, */ 0x0000 },	// 82FB ------ [　]  
{ /* 0x82FC, */ 0x0000 },	// 82FC ------ [　]  
//===== 0x83 40～FC ( テーブル数 [189]  ）====================
{ /* 0x8340, */ 0x30A1 },	// 8340 E382A1 [ァ]  #KATAKANA LETTER SMALL A
{ /* 0x8341, */ 0x30A2 },	// 8341 E382A2 [ア]  #KATAKANA LETTER A
{ /* 0x8342, */ 0x30A3 },	// 8342 E382A3 [ィ]  #KATAKANA LETTER SMALL I
{ /* 0x8343, */ 0x30A4 },	// 8343 E382A4 [イ]  #KATAKANA LETTER I
{ /* 0x8344, */ 0x30A5 },	// 8344 E382A5 [ゥ]  #KATAKANA LETTER SMALL U
{ /* 0x8345, */ 0x30A6 },	// 8345 E382A6 [ウ]  #KATAKANA LETTER U
{ /* 0x8346, */ 0x30A7 },	// 8346 E382A7 [ェ]  #KATAKANA LETTER SMALL E
{ /* 0x8347, */ 0x30A8 },	// 8347 E382A8 [エ]  #KATAKANA LETTER E
{ /* 0x8348, */ 0x30A9 },	// 8348 E382A9 [ォ]  #KATAKANA LETTER SMALL O
{ /* 0x8349, */ 0x30AA },	// 8349 E382AA [オ]  #KATAKANA LETTER O
{ /* 0x834A, */ 0x30AB },	// 834A E382AB [カ]  #KATAKANA LETTER KA
{ /* 0x834B, */ 0x30AC },	// 834B E382AC [ガ]  #KATAKANA LETTER GA
{ /* 0x834C, */ 0x30AD },	// 834C E382AD [キ]  #KATAKANA LETTER KI
{ /* 0x834D, */ 0x30AE },	// 834D E382AE [ギ]  #KATAKANA LETTER GI
{ /* 0x834E, */ 0x30AF },	// 834E E382AF [ク]  #KATAKANA LETTER KU
{ /* 0x834F, */ 0x30B0 },	// 834F E382B0 [グ]  #KATAKANA LETTER GU
{ /* 0x8350, */ 0x30B1 },	// 8350 E382B1 [ケ]  #KATAKANA LETTER KE
{ /* 0x8351, */ 0x30B2 },	// 8351 E382B2 [ゲ]  #KATAKANA LETTER GE
{ /* 0x8352, */ 0x30B3 },	// 8352 E382B3 [コ]  #KATAKANA LETTER KO
{ /* 0x8353, */ 0x30B4 },	// 8353 E382B4 [ゴ]  #KATAKANA LETTER GO
{ /* 0x8354, */ 0x30B5 },	// 8354 E382B5 [サ]  #KATAKANA LETTER SA
{ /* 0x8355, */ 0x30B6 },	// 8355 E382B6 [ザ]  #KATAKANA LETTER ZA
{ /* 0x8356, */ 0x30B7 },	// 8356 E382B7 [シ]  #KATAKANA LETTER SI
{ /* 0x8357, */ 0x30B8 },	// 8357 E382B8 [ジ]  #KATAKANA LETTER ZI
{ /* 0x8358, */ 0x30B9 },	// 8358 E382B9 [ス]  #KATAKANA LETTER SU
{ /* 0x8359, */ 0x30BA },	// 8359 E382BA [ズ]  #KATAKANA LETTER ZU
{ /* 0x835A, */ 0x30BB },	// 835A E382BB [セ]  #KATAKANA LETTER SE
{ /* 0x835B, */ 0x30BC },	// 835B E382BC [ゼ]  #KATAKANA LETTER ZE
{ /* 0x835C, */ 0x30BD },	// 835C E382BD [ソ]  #KATAKANA LETTER SO
{ /* 0x835D, */ 0x30BE },	// 835D E382BE [ゾ]  #KATAKANA LETTER ZO
{ /* 0x835E, */ 0x30BF },	// 835E E382BF [タ]  #KATAKANA LETTER TA
{ /* 0x835F, */ 0x30C0 },	// 835F E38380 [ダ]  #KATAKANA LETTER DA
{ /* 0x8360, */ 0x30C1 },	// 8360 E38381 [チ]  #KATAKANA LETTER TI
{ /* 0x8361, */ 0x30C2 },	// 8361 E38382 [ヂ]  #KATAKANA LETTER DI
{ /* 0x8362, */ 0x30C3 },	// 8362 E38383 [ッ]  #KATAKANA LETTER SMALL TU
{ /* 0x8363, */ 0x30C4 },	// 8363 E38384 [ツ]  #KATAKANA LETTER TU
{ /* 0x8364, */ 0x30C5 },	// 8364 E38385 [ヅ]  #KATAKANA LETTER DU
{ /* 0x8365, */ 0x30C6 },	// 8365 E38386 [テ]  #KATAKANA LETTER TE
{ /* 0x8366, */ 0x30C7 },	// 8366 E38387 [デ]  #KATAKANA LETTER DE
{ /* 0x8367, */ 0x30C8 },	// 8367 E38388 [ト]  #KATAKANA LETTER TO
{ /* 0x8368, */ 0x30C9 },	// 8368 E38389 [ド]  #KATAKANA LETTER DO
{ /* 0x8369, */ 0x30CA },	// 8369 E3838A [ナ]  #KATAKANA LETTER NA
{ /* 0x836A, */ 0x30CB },	// 836A E3838B [ニ]  #KATAKANA LETTER NI
{ /* 0x836B, */ 0x30CC },	// 836B E3838C [ヌ]  #KATAKANA LETTER NU
{ /* 0x836C, */ 0x30CD },	// 836C E3838D [ネ]  #KATAKANA LETTER NE
{ /* 0x836D, */ 0x30CE },	// 836D E3838E [ノ]  #KATAKANA LETTER NO
{ /* 0x836E, */ 0x30CF },	// 836E E3838F [ハ]  #KATAKANA LETTER HA
{ /* 0x836F, */ 0x30D0 },	// 836F E38390 [バ]  #KATAKANA LETTER BA
{ /* 0x8370, */ 0x30D1 },	// 8370 E38391 [パ]  #KATAKANA LETTER PA
{ /* 0x8371, */ 0x30D2 },	// 8371 E38392 [ヒ]  #KATAKANA LETTER HI
{ /* 0x8372, */ 0x30D3 },	// 8372 E38393 [ビ]  #KATAKANA LETTER BI
{ /* 0x8373, */ 0x30D4 },	// 8373 E38394 [ピ]  #KATAKANA LETTER PI
{ /* 0x8374, */ 0x30D5 },	// 8374 E38395 [フ]  #KATAKANA LETTER HU
{ /* 0x8375, */ 0x30D6 },	// 8375 E38396 [ブ]  #KATAKANA LETTER BU
{ /* 0x8376, */ 0x30D7 },	// 8376 E38397 [プ]  #KATAKANA LETTER PU
{ /* 0x8377, */ 0x30D8 },	// 8377 E38398 [ヘ]  #KATAKANA LETTER HE
{ /* 0x8378, */ 0x30D9 },	// 8378 E38399 [ベ]  #KATAKANA LETTER BE
{ /* 0x8379, */ 0x30DA },	// 8379 E3839A [ペ]  #KATAKANA LETTER PE
{ /* 0x837A, */ 0x30DB },	// 837A E3839B [ホ]  #KATAKANA LETTER HO
{ /* 0x837B, */ 0x30DC },	// 837B E3839C [ボ]  #KATAKANA LETTER BO
{ /* 0x837C, */ 0x30DD },	// 837C E3839D [ポ]  #KATAKANA LETTER PO
{ /* 0x837D, */ 0x30DE },	// 837D E3839E [マ]  #KATAKANA LETTER MA
{ /* 0x837E, */ 0x30DF },	// 837E E3839F [ミ]  #KATAKANA LETTER MI
{ /* 0x837F, */ 0x0000 },	// 		　#ダミー
{ /* 0x8380, */ 0x30E0 },	// 8380 E383A0 [ム]  #KATAKANA LETTER MU
{ /* 0x8381, */ 0x30E1 },	// 8381 E383A1 [メ]  #KATAKANA LETTER ME
{ /* 0x8382, */ 0x30E2 },	// 8382 E383A2 [モ]  #KATAKANA LETTER MO
{ /* 0x8383, */ 0x30E3 },	// 8383 E383A3 [ャ]  #KATAKANA LETTER SMALL YA
{ /* 0x8384, */ 0x30E4 },	// 8384 E383A4 [ヤ]  #KATAKANA LETTER YA
{ /* 0x8385, */ 0x30E5 },	// 8385 E383A5 [ュ]  #KATAKANA LETTER SMALL YU
{ /* 0x8386, */ 0x30E6 },	// 8386 E383A6 [ユ]  #KATAKANA LETTER YU
{ /* 0x8387, */ 0x30E7 },	// 8387 E383A7 [ョ]  #KATAKANA LETTER SMALL YO
{ /* 0x8388, */ 0x30E8 },	// 8388 E383A8 [ヨ]  #KATAKANA LETTER YO
{ /* 0x8389, */ 0x30E9 },	// 8389 E383A9 [ラ]  #KATAKANA LETTER RA
{ /* 0x838A, */ 0x30EA },	// 838A E383AA [リ]  #KATAKANA LETTER RI
{ /* 0x838B, */ 0x30EB },	// 838B E383AB [ル]  #KATAKANA LETTER RU
{ /* 0x838C, */ 0x30EC },	// 838C E383AC [レ]  #KATAKANA LETTER RE
{ /* 0x838D, */ 0x30ED },	// 838D E383AD [ロ]  #KATAKANA LETTER RO
{ /* 0x838E, */ 0x30EE },	// 838E E383AE [ヮ]  #KATAKANA LETTER SMALL WA
{ /* 0x838F, */ 0x30EF },	// 838F E383AF [ワ]  #KATAKANA LETTER WA
{ /* 0x8390, */ 0x30F0 },	// 8390 E383B0 [ヰ]  #KATAKANA LETTER WI
{ /* 0x8391, */ 0x30F1 },	// 8391 E383B1 [ヱ]  #KATAKANA LETTER WE
{ /* 0x8392, */ 0x30F2 },	// 8392 E383B2 [ヲ]  #KATAKANA LETTER WO
{ /* 0x8393, */ 0x30F3 },	// 8393 E383B3 [ン]  #KATAKANA LETTER N
{ /* 0x8394, */ 0x30F4 },	// 8394 E383B4 [ヴ]  #KATAKANA LETTER VU
{ /* 0x8395, */ 0x30F5 },	// 8395 E383B5 [ヵ]  #KATAKANA LETTER SMALL KA
{ /* 0x8396, */ 0x30F6 },	// 8396 E383B6 [ヶ]  #KATAKANA LETTER SMALL KE
{ /* 0x8397, */ 0x0000 },	// 8397 ------ [　]  
{ /* 0x8398, */ 0x0000 },	// 8398 ------ [　]  
{ /* 0x8399, */ 0x0000 },	// 8399 ------ [　]  
{ /* 0x839A, */ 0x0000 },	// 839A ------ [　]  
{ /* 0x839B, */ 0x0000 },	// 839B ------ [　]  
{ /* 0x839C, */ 0x0000 },	// 839C ------ [　]  
{ /* 0x839D, */ 0x0000 },	// 839D ------ [　]  
{ /* 0x839E, */ 0x0000 },	// 839E ------ [　]  
{ /* 0x839F, */ 0x0391 },	// 839F CE91   [Α]  #GREEK CAPITAL LETTER ALPHA
{ /* 0x83A0, */ 0x0392 },	// 83A0 CE92   [Β]  #GREEK CAPITAL LETTER BETA
{ /* 0x83A1, */ 0x0393 },	// 83A1 CE93   [Γ]  #GREEK CAPITAL LETTER GAMMA
{ /* 0x83A2, */ 0x0394 },	// 83A2 CE94   [Δ]  #GREEK CAPITAL LETTER DELTA
{ /* 0x83A3, */ 0x0395 },	// 83A3 CE95   [Ε]  #GREEK CAPITAL LETTER EPSILON
{ /* 0x83A4, */ 0x0396 },	// 83A4 CE96   [Ζ]  #GREEK CAPITAL LETTER ZETA
{ /* 0x83A5, */ 0x0397 },	// 83A5 CE97   [Η]  #GREEK CAPITAL LETTER ETA
{ /* 0x83A6, */ 0x0398 },	// 83A6 CE98   [Θ]  #GREEK CAPITAL LETTER THETA
{ /* 0x83A7, */ 0x0399 },	// 83A7 CE99   [Ι]  #GREEK CAPITAL LETTER IOTA
{ /* 0x83A8, */ 0x039A },	// 83A8 CE9A   [Κ]  #GREEK CAPITAL LETTER KAPPA
{ /* 0x83A9, */ 0x039B },	// 83A9 CE9B   [Λ]  #GREEK CAPITAL LETTER LAMDA
{ /* 0x83AA, */ 0x039C },	// 83AA CE9C   [Μ]  #GREEK CAPITAL LETTER MU
{ /* 0x83AB, */ 0x039D },	// 83AB CE9D   [Ν]  #GREEK CAPITAL LETTER NU
{ /* 0x83AC, */ 0x039E },	// 83AC CE9E   [Ξ]  #GREEK CAPITAL LETTER XI
{ /* 0x83AD, */ 0x039F },	// 83AD CE9F   [Ο]  #GREEK CAPITAL LETTER OMICRON
{ /* 0x83AE, */ 0x03A0 },	// 83AE CEA0   [Π]  #GREEK CAPITAL LETTER PI
{ /* 0x83AF, */ 0x03A1 },	// 83AF CEA1   [Ρ]  #GREEK CAPITAL LETTER RHO
{ /* 0x83B0, */ 0x03A3 },	// 83B0 CEA3   [Σ]  #GREEK CAPITAL LETTER SIGMA
{ /* 0x83B1, */ 0x03A4 },	// 83B1 CEA4   [Τ]  #GREEK CAPITAL LETTER TAU
{ /* 0x83B2, */ 0x03A5 },	// 83B2 CEA5   [Υ]  #GREEK CAPITAL LETTER UPSILON
{ /* 0x83B3, */ 0x03A6 },	// 83B3 CEA6   [Φ]  #GREEK CAPITAL LETTER PHI
{ /* 0x83B4, */ 0x03A7 },	// 83B4 CEA7   [Χ]  #GREEK CAPITAL LETTER CHI
{ /* 0x83B5, */ 0x03A8 },	// 83B5 CEA8   [Ψ]  #GREEK CAPITAL LETTER PSI
{ /* 0x83B6, */ 0x03A9 },	// 83B6 CEA9   [Ω]  #GREEK CAPITAL LETTER OMEGA
{ /* 0x83B7, */ 0x0000 },	// 83B7 ------ [　]  
{ /* 0x83B8, */ 0x0000 },	// 83B8 ------ [　]  
{ /* 0x83B9, */ 0x0000 },	// 83B9 ------ [　]  
{ /* 0x83BA, */ 0x0000 },	// 83BA ------ [　]  
{ /* 0x83BB, */ 0x0000 },	// 83BB ------ [　]  
{ /* 0x83BC, */ 0x0000 },	// 83BC ------ [　]  
{ /* 0x83BD, */ 0x0000 },	// 83BD ------ [　]  
{ /* 0x83BE, */ 0x0000 },	// 83BE ------ [　]  
{ /* 0x83BF, */ 0x03B1 },	// 83BF CEB1   [α]  #GREEK SMALL LETTER ALPHA
{ /* 0x83C0, */ 0x03B2 },	// 83C0 CEB2   [β]  #GREEK SMALL LETTER BETA
{ /* 0x83C1, */ 0x03B3 },	// 83C1 CEB3   [γ]  #GREEK SMALL LETTER GAMMA
{ /* 0x83C2, */ 0x03B4 },	// 83C2 CEB4   [δ]  #GREEK SMALL LETTER DELTA
{ /* 0x83C3, */ 0x03B5 },	// 83C3 CEB5   [ε]  #GREEK SMALL LETTER EPSILON
{ /* 0x83C4, */ 0x03B6 },	// 83C4 CEB6   [ζ]  #GREEK SMALL LETTER ZETA
{ /* 0x83C5, */ 0x03B7 },	// 83C5 CEB7   [η]  #GREEK SMALL LETTER ETA
{ /* 0x83C6, */ 0x03B8 },	// 83C6 CEB8   [θ]  #GREEK SMALL LETTER THETA
{ /* 0x83C7, */ 0x03B9 },	// 83C7 CEB9   [ι]  #GREEK SMALL LETTER IOTA
{ /* 0x83C8, */ 0x03BA },	// 83C8 CEBA   [κ]  #GREEK SMALL LETTER KAPPA
{ /* 0x83C9, */ 0x03BB },	// 83C9 CEBB   [λ]  #GREEK SMALL LETTER LAMDA
{ /* 0x83CA, */ 0x03BC },	// 83CA CEBC   [μ]  #GREEK SMALL LETTER MU
{ /* 0x83CB, */ 0x03BD },	// 83CB CEBD   [ν]  #GREEK SMALL LETTER NU
{ /* 0x83CC, */ 0x03BE },	// 83CC CEBE   [ξ]  #GREEK SMALL LETTER XI
{ /* 0x83CD, */ 0x03BF },	// 83CD CEBF   [ο]  #GREEK SMALL LETTER OMICRON
{ /* 0x83CE, */ 0x03C0 },	// 83CE CF80   [π]  #GREEK SMALL LETTER PI
{ /* 0x83CF, */ 0x03C1 },	// 83CF CF81   [ρ]  #GREEK SMALL LETTER RHO
{ /* 0x83D0, */ 0x03C3 },	// 83D0 CF83   [σ]  #GREEK SMALL LETTER SIGMA
{ /* 0x83D1, */ 0x03C4 },	// 83D1 CF84   [τ]  #GREEK SMALL LETTER TAU
{ /* 0x83D2, */ 0x03C5 },	// 83D2 CF85   [υ]  #GREEK SMALL LETTER UPSILON
{ /* 0x83D3, */ 0x03C6 },	// 83D3 CF86   [φ]  #GREEK SMALL LETTER PHI
{ /* 0x83D4, */ 0x03C7 },	// 83D4 CF87   [χ]  #GREEK SMALL LETTER CHI
{ /* 0x83D5, */ 0x03C8 },	// 83D5 CF88   [ψ]  #GREEK SMALL LETTER PSI
{ /* 0x83D6, */ 0x03C9 },	// 83D6 CF89   [ω]  #GREEK SMALL LETTER OMEGA
{ /* 0x83D7, */ 0x0000 },	// 83D7 ------ [　]  
{ /* 0x83D8, */ 0x0000 },	// 83D8 ------ [　]  
{ /* 0x83D9, */ 0x0000 },	// 83D9 ------ [　]  
{ /* 0x83DA, */ 0x0000 },	// 83DA ------ [　]  
{ /* 0x83DB, */ 0x0000 },	// 83DB ------ [　]  
{ /* 0x83DC, */ 0x0000 },	// 83DC ------ [　]  
{ /* 0x83DD, */ 0x0000 },	// 83DD ------ [　]  
{ /* 0x83DE, */ 0x0000 },	// 83DE ------ [　]  
{ /* 0x83DF, */ 0x0000 },	// 83DF ------ [　]  
{ /* 0x83E0, */ 0x0000 },	// 83E0 ------ [　]  
{ /* 0x83E1, */ 0x0000 },	// 83E1 ------ [　]  
{ /* 0x83E2, */ 0x0000 },	// 83E2 ------ [　]  
{ /* 0x83E3, */ 0x0000 },	// 83E3 ------ [　]  
{ /* 0x83E4, */ 0x0000 },	// 83E4 ------ [　]  
{ /* 0x83E5, */ 0x0000 },	// 83E5 ------ [　]  
{ /* 0x83E6, */ 0x0000 },	// 83E6 ------ [　]  
{ /* 0x83E7, */ 0x0000 },	// 83E7 ------ [　]  
{ /* 0x83E8, */ 0x0000 },	// 83E8 ------ [　]  
{ /* 0x83E9, */ 0x0000 },	// 83E9 ------ [　]  
{ /* 0x83EA, */ 0x0000 },	// 83EA ------ [　]  
{ /* 0x83EB, */ 0x0000 },	// 83EB ------ [　]  
{ /* 0x83EC, */ 0x0000 },	// 83EC ------ [　]  
{ /* 0x83ED, */ 0x0000 },	// 83ED ------ [　]  
{ /* 0x83EE, */ 0x0000 },	// 83EE ------ [　]  
{ /* 0x83EF, */ 0x0000 },	// 83EF ------ [　]  
{ /* 0x83F0, */ 0x0000 },	// 83F0 ------ [　]  
{ /* 0x83F1, */ 0x0000 },	// 83F1 ------ [　]  
{ /* 0x83F2, */ 0x0000 },	// 83F2 ------ [　]  
{ /* 0x83F3, */ 0x0000 },	// 83F3 ------ [　]  
{ /* 0x83F4, */ 0x0000 },	// 83F4 ------ [　]  
{ /* 0x83F5, */ 0x0000 },	// 83F5 ------ [　]  
{ /* 0x83F6, */ 0x0000 },	// 83F6 ------ [　]  
{ /* 0x83F7, */ 0x0000 },	// 83F7 ------ [　]  
{ /* 0x83F8, */ 0x0000 },	// 83F8 ------ [　]  
{ /* 0x83F9, */ 0x0000 },	// 83F9 ------ [　]  
{ /* 0x83FA, */ 0x0000 },	// 83FA ------ [　]  
{ /* 0x83FB, */ 0x0000 },	// 83FB ------ [　]  
{ /* 0x83FC, */ 0x0000 },	// 83FC ------ [　]  
//===== 0x84 40～FC ( テーブル数 [189]  ）====================
{ /* 0x8440, */ 0x0410 },	// 8440 D090   [А]  #CYRILLIC CAPITAL LETTER A
{ /* 0x8441, */ 0x0411 },	// 8441 D091   [Б]  #CYRILLIC CAPITAL LETTER BE
{ /* 0x8442, */ 0x0412 },	// 8442 D092   [В]  #CYRILLIC CAPITAL LETTER VE
{ /* 0x8443, */ 0x0413 },	// 8443 D093   [Г]  #CYRILLIC CAPITAL LETTER GHE
{ /* 0x8444, */ 0x0414 },	// 8444 D094   [Д]  #CYRILLIC CAPITAL LETTER DE
{ /* 0x8445, */ 0x0415 },	// 8445 D095   [Е]  #CYRILLIC CAPITAL LETTER IE
{ /* 0x8446, */ 0x0401 },	// 8446 D081   [Ё]  #CYRILLIC CAPITAL LETTER IO
{ /* 0x8447, */ 0x0416 },	// 8447 D096   [Ж]  #CYRILLIC CAPITAL LETTER ZHE
{ /* 0x8448, */ 0x0417 },	// 8448 D097   [З]  #CYRILLIC CAPITAL LETTER ZE
{ /* 0x8449, */ 0x0418 },	// 8449 D098   [И]  #CYRILLIC CAPITAL LETTER I
{ /* 0x844A, */ 0x0419 },	// 844A D099   [Й]  #CYRILLIC CAPITAL LETTER SHORT I
{ /* 0x844B, */ 0x041A },	// 844B D09A   [К]  #CYRILLIC CAPITAL LETTER KA
{ /* 0x844C, */ 0x041B },	// 844C D09B   [Л]  #CYRILLIC CAPITAL LETTER EL
{ /* 0x844D, */ 0x041C },	// 844D D09C   [М]  #CYRILLIC CAPITAL LETTER EM
{ /* 0x844E, */ 0x041D },	// 844E D09D   [Н]  #CYRILLIC CAPITAL LETTER EN
{ /* 0x844F, */ 0x041E },	// 844F D09E   [О]  #CYRILLIC CAPITAL LETTER O
{ /* 0x8450, */ 0x041F },	// 8450 D09F   [П]  #CYRILLIC CAPITAL LETTER PE
{ /* 0x8451, */ 0x0420 },	// 8451 D0A0   [Р]  #CYRILLIC CAPITAL LETTER ER
{ /* 0x8452, */ 0x0421 },	// 8452 D0A1   [С]  #CYRILLIC CAPITAL LETTER ES
{ /* 0x8453, */ 0x0422 },	// 8453 D0A2   [Т]  #CYRILLIC CAPITAL LETTER TE
{ /* 0x8454, */ 0x0423 },	// 8454 D0A3   [У]  #CYRILLIC CAPITAL LETTER U
{ /* 0x8455, */ 0x0424 },	// 8455 D0A4   [Ф]  #CYRILLIC CAPITAL LETTER EF
{ /* 0x8456, */ 0x0425 },	// 8456 D0A5   [Х]  #CYRILLIC CAPITAL LETTER HA
{ /* 0x8457, */ 0x0426 },	// 8457 D0A6   [Ц]  #CYRILLIC CAPITAL LETTER TSE
{ /* 0x8458, */ 0x0427 },	// 8458 D0A7   [Ч]  #CYRILLIC CAPITAL LETTER CHE
{ /* 0x8459, */ 0x0428 },	// 8459 D0A8   [Ш]  #CYRILLIC CAPITAL LETTER SHA
{ /* 0x845A, */ 0x0429 },	// 845A D0A9   [Щ]  #CYRILLIC CAPITAL LETTER SHCHA
{ /* 0x845B, */ 0x042A },	// 845B D0AA   [Ъ]  #CYRILLIC CAPITAL LETTER HARD SIGN
{ /* 0x845C, */ 0x042B },	// 845C D0AB   [Ы]  #CYRILLIC CAPITAL LETTER YERU
{ /* 0x845D, */ 0x042C },	// 845D D0AC   [Ь]  #CYRILLIC CAPITAL LETTER SOFT SIGN
{ /* 0x845E, */ 0x042D },	// 845E D0AD   [Э]  #CYRILLIC CAPITAL LETTER E
{ /* 0x845F, */ 0x042E },	// 845F D0AE   [Ю]  #CYRILLIC CAPITAL LETTER YU
{ /* 0x8460, */ 0x042F },	// 8460 D0AF   [Я]  #CYRILLIC CAPITAL LETTER YA
{ /* 0x8461, */ 0x0000 },	// 8461 ------ [　]  
{ /* 0x8462, */ 0x0000 },	// 8462 ------ [　]  
{ /* 0x8463, */ 0x0000 },	// 8463 ------ [　]  
{ /* 0x8464, */ 0x0000 },	// 8464 ------ [　]  
{ /* 0x8465, */ 0x0000 },	// 8465 ------ [　]  
{ /* 0x8466, */ 0x0000 },	// 8466 ------ [　]  
{ /* 0x8467, */ 0x0000 },	// 8467 ------ [　]  
{ /* 0x8468, */ 0x0000 },	// 8468 ------ [　]  
{ /* 0x8469, */ 0x0000 },	// 8469 ------ [　]  
{ /* 0x846A, */ 0x0000 },	// 846A ------ [　]  
{ /* 0x846B, */ 0x0000 },	// 846B ------ [　]  
{ /* 0x846C, */ 0x0000 },	// 846C ------ [　]  
{ /* 0x846D, */ 0x0000 },	// 846D ------ [　]  
{ /* 0x846E, */ 0x0000 },	// 846E ------ [　]  
{ /* 0x846F, */ 0x0000 },	// 846F ------ [　]  
{ /* 0x8470, */ 0x0430 },	// 8470 D0B0   [а]  #CYRILLIC SMALL LETTER A
{ /* 0x8471, */ 0x0431 },	// 8471 D0B1   [б]  #CYRILLIC SMALL LETTER BE
{ /* 0x8472, */ 0x0432 },	// 8472 D0B2   [в]  #CYRILLIC SMALL LETTER VE
{ /* 0x8473, */ 0x0433 },	// 8473 D0B3   [г]  #CYRILLIC SMALL LETTER GHE
{ /* 0x8474, */ 0x0434 },	// 8474 D0B4   [д]  #CYRILLIC SMALL LETTER DE
{ /* 0x8475, */ 0x0435 },	// 8475 D0B5   [е]  #CYRILLIC SMALL LETTER IE
{ /* 0x8476, */ 0x0451 },	// 8476 D191   [ё]  #CYRILLIC SMALL LETTER IO
{ /* 0x8477, */ 0x0436 },	// 8477 D0B6   [ж]  #CYRILLIC SMALL LETTER ZHE
{ /* 0x8478, */ 0x0437 },	// 8478 D0B7   [з]  #CYRILLIC SMALL LETTER ZE
{ /* 0x8479, */ 0x0438 },	// 8479 D0B8   [и]  #CYRILLIC SMALL LETTER I
{ /* 0x847A, */ 0x0439 },	// 847A D0B9   [й]  #CYRILLIC SMALL LETTER SHORT I
{ /* 0x847B, */ 0x043A },	// 847B D0BA   [к]  #CYRILLIC SMALL LETTER KA
{ /* 0x847C, */ 0x043B },	// 847C D0BB   [л]  #CYRILLIC SMALL LETTER EL
{ /* 0x847D, */ 0x043C },	// 847D D0BC   [м]  #CYRILLIC SMALL LETTER EM
{ /* 0x847E, */ 0x043D },	// 847E D0BD   [н]  #CYRILLIC SMALL LETTER EN
{ /* 0x847F, */ 0x0000 },	// 		　#ダミー
{ /* 0x8480, */ 0x043E },	// 8480 D0BE   [о]  #CYRILLIC SMALL LETTER O
{ /* 0x8481, */ 0x043F },	// 8481 D0BF   [п]  #CYRILLIC SMALL LETTER PE
{ /* 0x8482, */ 0x0440 },	// 8482 D180   [р]  #CYRILLIC SMALL LETTER ER
{ /* 0x8483, */ 0x0441 },	// 8483 D181   [с]  #CYRILLIC SMALL LETTER ES
{ /* 0x8484, */ 0x0442 },	// 8484 D182   [т]  #CYRILLIC SMALL LETTER TE
{ /* 0x8485, */ 0x0443 },	// 8485 D183   [у]  #CYRILLIC SMALL LETTER U
{ /* 0x8486, */ 0x0444 },	// 8486 D184   [ф]  #CYRILLIC SMALL LETTER EF
{ /* 0x8487, */ 0x0445 },	// 8487 D185   [х]  #CYRILLIC SMALL LETTER HA
{ /* 0x8488, */ 0x0446 },	// 8488 D186   [ц]  #CYRILLIC SMALL LETTER TSE
{ /* 0x8489, */ 0x0447 },	// 8489 D187   [ч]  #CYRILLIC SMALL LETTER CHE
{ /* 0x848A, */ 0x0448 },	// 848A D188   [ш]  #CYRILLIC SMALL LETTER SHA
{ /* 0x848B, */ 0x0449 },	// 848B D189   [щ]  #CYRILLIC SMALL LETTER SHCHA
{ /* 0x848C, */ 0x044A },	// 848C D18A   [ъ]  #CYRILLIC SMALL LETTER HARD SIGN
{ /* 0x848D, */ 0x044B },	// 848D D18B   [ы]  #CYRILLIC SMALL LETTER YERU
{ /* 0x848E, */ 0x044C },	// 848E D18C   [ь]  #CYRILLIC SMALL LETTER SOFT SIGN
{ /* 0x848F, */ 0x044D },	// 848F D18D   [э]  #CYRILLIC SMALL LETTER E
{ /* 0x8490, */ 0x044E },	// 8490 D18E   [ю]  #CYRILLIC SMALL LETTER YU
{ /* 0x8491, */ 0x044F },	// 8491 D18F   [я]  #CYRILLIC SMALL LETTER YA
{ /* 0x8492, */ 0x0000 },	// 8492 ------ [　]  
{ /* 0x8493, */ 0x0000 },	// 8493 ------ [　]  
{ /* 0x8494, */ 0x0000 },	// 8494 ------ [　]  
{ /* 0x8495, */ 0x0000 },	// 8495 ------ [　]  
{ /* 0x8496, */ 0x0000 },	// 8496 ------ [　]  
{ /* 0x8497, */ 0x0000 },	// 8497 ------ [　]  
{ /* 0x8498, */ 0x0000 },	// 8498 ------ [　]  
{ /* 0x8499, */ 0x0000 },	// 8499 ------ [　]  
{ /* 0x849A, */ 0x0000 },	// 849A ------ [　]  
{ /* 0x849B, */ 0x0000 },	// 849B ------ [　]  
{ /* 0x849C, */ 0x0000 },	// 849C ------ [　]  
{ /* 0x849D, */ 0x0000 },	// 849D ------ [　]  
{ /* 0x849E, */ 0x0000 },	// 849E ------ [　]  
{ /* 0x849F, */ 0x2500 },	// 849F E29480 [─]  #BOX DRAWINGS LIGHT HORIZONTAL
{ /* 0x84A0, */ 0x2502 },	// 84A0 E29482 [│]  #BOX DRAWINGS LIGHT VERTICAL
{ /* 0x84A1, */ 0x250C },	// 84A1 E2948C [┌]  #BOX DRAWINGS LIGHT DOWN AND RIGHT
{ /* 0x84A2, */ 0x2510 },	// 84A2 E29490 [┐]  #BOX DRAWINGS LIGHT DOWN AND LEFT
{ /* 0x84A3, */ 0x2518 },	// 84A3 E29498 [┘]  #BOX DRAWINGS LIGHT UP AND LEFT
{ /* 0x84A4, */ 0x2514 },	// 84A4 E29494 [└]  #BOX DRAWINGS LIGHT UP AND RIGHT
{ /* 0x84A5, */ 0x251C },	// 84A5 E2949C [├]  #BOX DRAWINGS LIGHT VERTICAL AND RIGHT
{ /* 0x84A6, */ 0x252C },	// 84A6 E294AC [┬]  #BOX DRAWINGS LIGHT DOWN AND HORIZONTAL
{ /* 0x84A7, */ 0x2524 },	// 84A7 E294A4 [┤]  #BOX DRAWINGS LIGHT VERTICAL AND LEFT
{ /* 0x84A8, */ 0x2534 },	// 84A8 E294B4 [┴]  #BOX DRAWINGS LIGHT UP AND HORIZONTAL
{ /* 0x84A9, */ 0x253C },	// 84A9 E294BC [┼]  #BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL
{ /* 0x84AA, */ 0x2501 },	// 84AA E29481 [━]  #BOX DRAWINGS HEAVY HORIZONTAL
{ /* 0x84AB, */ 0x2503 },	// 84AB E29483 [┃]  #BOX DRAWINGS HEAVY VERTICAL
{ /* 0x84AC, */ 0x250F },	// 84AC E2948F [┏]  #BOX DRAWINGS HEAVY DOWN AND RIGHT
{ /* 0x84AD, */ 0x2513 },	// 84AD E29493 [┓]  #BOX DRAWINGS HEAVY DOWN AND LEFT
{ /* 0x84AE, */ 0x251B },	// 84AE E2949B [┛]  #BOX DRAWINGS HEAVY UP AND LEFT
{ /* 0x84AF, */ 0x2517 },	// 84AF E29497 [┗]  #BOX DRAWINGS HEAVY UP AND RIGHT
{ /* 0x84B0, */ 0x2523 },	// 84B0 E294A3 [┣]  #BOX DRAWINGS HEAVY VERTICAL AND RIGHT
{ /* 0x84B1, */ 0x2533 },	// 84B1 E294B3 [┳]  #BOX DRAWINGS HEAVY DOWN AND HORIZONTAL
{ /* 0x84B2, */ 0x252B },	// 84B2 E294AB [┫]  #BOX DRAWINGS HEAVY VERTICAL AND LEFT
{ /* 0x84B3, */ 0x253B },	// 84B3 E294BB [┻]  #BOX DRAWINGS HEAVY UP AND HORIZONTAL
{ /* 0x84B4, */ 0x254B },	// 84B4 E2958B [╋]  #BOX DRAWINGS HEAVY VERTICAL AND HORIZONTAL
{ /* 0x84B5, */ 0x2520 },	// 84B5 E294A0 [┠]  #BOX DRAWINGS VERTICAL HEAVY AND RIGHT LIGHT
{ /* 0x84B6, */ 0x252F },	// 84B6 E294AF [┯]  #BOX DRAWINGS DOWN LIGHT AND HORIZONTAL HEAVY
{ /* 0x84B7, */ 0x2528 },	// 84B7 E294A8 [┨]  #BOX DRAWINGS VERTICAL HEAVY AND LEFT LIGHT
{ /* 0x84B8, */ 0x2537 },	// 84B8 E294B7 [┷]  #BOX DRAWINGS UP LIGHT AND HORIZONTAL HEAVY
{ /* 0x84B9, */ 0x253F },	// 84B9 E294BF [┿]  #BOX DRAWINGS VERTICAL LIGHT AND HORIZONTAL HEAVY
{ /* 0x84BA, */ 0x251D },	// 84BA E2949D [┝]  #BOX DRAWINGS VERTICAL LIGHT AND RIGHT HEAVY
{ /* 0x84BB, */ 0x2530 },	// 84BB E294B0 [┰]  #BOX DRAWINGS DOWN HEAVY AND HORIZONTAL LIGHT
{ /* 0x84BC, */ 0x2525 },	// 84BC E294A5 [┥]  #BOX DRAWINGS VERTICAL LIGHT AND LEFT HEAVY
{ /* 0x84BD, */ 0x2538 },	// 84BD E294B8 [┸]  #BOX DRAWINGS UP HEAVY AND HORIZONTAL LIGHT
{ /* 0x84BE, */ 0x2542 },	// 84BE E29582 [╂]  #BOX DRAWINGS VERTICAL HEAVY AND HORIZONTAL LIGHT
{ /* 0x84BF, */ 0x0000 },	// 84BF ------ [　]  
{ /* 0x84C0, */ 0x0000 },	// 84C0 ------ [　]  
{ /* 0x84C1, */ 0x0000 },	// 84C1 ------ [　]  
{ /* 0x84C2, */ 0x0000 },	// 84C2 ------ [　]  
{ /* 0x84C3, */ 0x0000 },	// 84C3 ------ [　]  
{ /* 0x84C4, */ 0x0000 },	// 84C4 ------ [　]  
{ /* 0x84C5, */ 0x0000 },	// 84C5 ------ [　]  
{ /* 0x84C6, */ 0x0000 },	// 84C6 ------ [　]  
{ /* 0x84C7, */ 0x0000 },	// 84C7 ------ [　]  
{ /* 0x84C8, */ 0x0000 },	// 84C8 ------ [　]  
{ /* 0x84C9, */ 0x0000 },	// 84C9 ------ [　]  
{ /* 0x84CA, */ 0x0000 },	// 84CA ------ [　]  
{ /* 0x84CB, */ 0x0000 },	// 84CB ------ [　]  
{ /* 0x84CC, */ 0x0000 },	// 84CC ------ [　]  
{ /* 0x84CD, */ 0x0000 },	// 84CD ------ [　]  
{ /* 0x84CE, */ 0x0000 },	// 84CE ------ [　]  
{ /* 0x84CF, */ 0x0000 },	// 84CF ------ [　]  
{ /* 0x84D0, */ 0x0000 },	// 84D0 ------ [　]  
{ /* 0x84D1, */ 0x0000 },	// 84D1 ------ [　]  
{ /* 0x84D2, */ 0x0000 },	// 84D2 ------ [　]  
{ /* 0x84D3, */ 0x0000 },	// 84D3 ------ [　]  
{ /* 0x84D4, */ 0x0000 },	// 84D4 ------ [　]  
{ /* 0x84D5, */ 0x0000 },	// 84D5 ------ [　]  
{ /* 0x84D6, */ 0x0000 },	// 84D6 ------ [　]  
{ /* 0x84D7, */ 0x0000 },	// 84D7 ------ [　]  
{ /* 0x84D8, */ 0x0000 },	// 84D8 ------ [　]  
{ /* 0x84D9, */ 0x0000 },	// 84D9 ------ [　]  
{ /* 0x84DA, */ 0x0000 },	// 84DA ------ [　]  
{ /* 0x84DB, */ 0x0000 },	// 84DB ------ [　]  
{ /* 0x84DC, */ 0x0000 },	// 84DC ------ [　]  
{ /* 0x84DD, */ 0x0000 },	// 84DD ------ [　]  
{ /* 0x84DE, */ 0x0000 },	// 84DE ------ [　]  
{ /* 0x84DF, */ 0x0000 },	// 84DF ------ [　]  
{ /* 0x84E0, */ 0x0000 },	// 84E0 ------ [　]  
{ /* 0x84E1, */ 0x0000 },	// 84E1 ------ [　]  
{ /* 0x84E2, */ 0x0000 },	// 84E2 ------ [　]  
{ /* 0x84E3, */ 0x0000 },	// 84E3 ------ [　]  
{ /* 0x84E4, */ 0x0000 },	// 84E4 ------ [　]  
{ /* 0x84E5, */ 0x0000 },	// 84E5 ------ [　]  
{ /* 0x84E6, */ 0x0000 },	// 84E6 ------ [　]  
{ /* 0x84E7, */ 0x0000 },	// 84E7 ------ [　]  
{ /* 0x84E8, */ 0x0000 },	// 84E8 ------ [　]  
{ /* 0x84E9, */ 0x0000 },	// 84E9 ------ [　]  
{ /* 0x84EA, */ 0x0000 },	// 84EA ------ [　]  
{ /* 0x84EB, */ 0x0000 },	// 84EB ------ [　]  
{ /* 0x84EC, */ 0x0000 },	// 84EC ------ [　]  
{ /* 0x84ED, */ 0x0000 },	// 84ED ------ [　]  
{ /* 0x84EE, */ 0x0000 },	// 84EE ------ [　]  
{ /* 0x84EF, */ 0x0000 },	// 84EF ------ [　]  
{ /* 0x84F0, */ 0x0000 },	// 84F0 ------ [　]  
{ /* 0x84F1, */ 0x0000 },	// 84F1 ------ [　]  
{ /* 0x84F2, */ 0x0000 },	// 84F2 ------ [　]  
{ /* 0x84F3, */ 0x0000 },	// 84F3 ------ [　]  
{ /* 0x84F4, */ 0x0000 },	// 84F4 ------ [　]  
{ /* 0x84F5, */ 0x0000 },	// 84F5 ------ [　]  
{ /* 0x84F6, */ 0x0000 },	// 84F6 ------ [　]  
{ /* 0x84F7, */ 0x0000 },	// 84F7 ------ [　]  
{ /* 0x84F8, */ 0x0000 },	// 84F8 ------ [　]  
{ /* 0x84F9, */ 0x0000 },	// 84F9 ------ [　]  
{ /* 0x84FA, */ 0x0000 },	// 84FA ------ [　]  
{ /* 0x84FB, */ 0x0000 },	// 84FB ------ [　]  
{ /* 0x84FC, */ 0x0000 },	// 84FC ------ [　]  
//===== 0x85 40～FC ( テーブル数 [189]  ）====================
{ /* 0x8540, */ 0x0000 },	// 8540 ------ [　]  
{ /* 0x8541, */ 0x0000 },	// 8541 ------ [　]  
{ /* 0x8542, */ 0x0000 },	// 8542 ------ [　]  
{ /* 0x8543, */ 0x0000 },	// 8543 ------ [　]  
{ /* 0x8544, */ 0x0000 },	// 8544 ------ [　]  
{ /* 0x8545, */ 0x0000 },	// 8545 ------ [　]  
{ /* 0x8546, */ 0x0000 },	// 8546 ------ [　]  
{ /* 0x8547, */ 0x0000 },	// 8547 ------ [　]  
{ /* 0x8548, */ 0x0000 },	// 8548 ------ [　]  
{ /* 0x8549, */ 0x0000 },	// 8549 ------ [　]  
{ /* 0x854A, */ 0x0000 },	// 854A ------ [　]  
{ /* 0x854B, */ 0x0000 },	// 854B ------ [　]  
{ /* 0x854C, */ 0x0000 },	// 854C ------ [　]  
{ /* 0x854D, */ 0x0000 },	// 854D ------ [　]  
{ /* 0x854E, */ 0x0000 },	// 854E ------ [　]  
{ /* 0x854F, */ 0x0000 },	// 854F ------ [　]  
{ /* 0x8550, */ 0x0000 },	// 8550 ------ [　]  
{ /* 0x8551, */ 0x0000 },	// 8551 ------ [　]  
{ /* 0x8552, */ 0x0000 },	// 8552 ------ [　]  
{ /* 0x8553, */ 0x0000 },	// 8553 ------ [　]  
{ /* 0x8554, */ 0x0000 },	// 8554 ------ [　]  
{ /* 0x8555, */ 0x0000 },	// 8555 ------ [　]  
{ /* 0x8556, */ 0x0000 },	// 8556 ------ [　]  
{ /* 0x8557, */ 0x0000 },	// 8557 ------ [　]  
{ /* 0x8558, */ 0x0000 },	// 8558 ------ [　]  
{ /* 0x8559, */ 0x0000 },	// 8559 ------ [　]  
{ /* 0x855A, */ 0x0000 },	// 855A ------ [　]  
{ /* 0x855B, */ 0x0000 },	// 855B ------ [　]  
{ /* 0x855C, */ 0x0000 },	// 855C ------ [　]  
{ /* 0x855D, */ 0x0000 },	// 855D ------ [　]  
{ /* 0x855E, */ 0x0000 },	// 855E ------ [　]  
{ /* 0x855F, */ 0x0000 },	// 855F ------ [　]  
{ /* 0x8560, */ 0x0000 },	// 8560 ------ [　]  
{ /* 0x8561, */ 0x0000 },	// 8561 ------ [　]  
{ /* 0x8562, */ 0x0000 },	// 8562 ------ [　]  
{ /* 0x8563, */ 0x0000 },	// 8563 ------ [　]  
{ /* 0x8564, */ 0x0000 },	// 8564 ------ [　]  
{ /* 0x8565, */ 0x0000 },	// 8565 ------ [　]  
{ /* 0x8566, */ 0x0000 },	// 8566 ------ [　]  
{ /* 0x8567, */ 0x0000 },	// 8567 ------ [　]  
{ /* 0x8568, */ 0x0000 },	// 8568 ------ [　]  
{ /* 0x8569, */ 0x0000 },	// 8569 ------ [　]  
{ /* 0x856A, */ 0x0000 },	// 856A ------ [　]  
{ /* 0x856B, */ 0x0000 },	// 856B ------ [　]  
{ /* 0x856C, */ 0x0000 },	// 856C ------ [　]  
{ /* 0x856D, */ 0x0000 },	// 856D ------ [　]  
{ /* 0x856E, */ 0x0000 },	// 856E ------ [　]  
{ /* 0x856F, */ 0x0000 },	// 856F ------ [　]  
{ /* 0x8570, */ 0x0000 },	// 8570 ------ [　]  
{ /* 0x8571, */ 0x0000 },	// 8571 ------ [　]  
{ /* 0x8572, */ 0x0000 },	// 8572 ------ [　]  
{ /* 0x8573, */ 0x0000 },	// 8573 ------ [　]  
{ /* 0x8574, */ 0x0000 },	// 8574 ------ [　]  
{ /* 0x8575, */ 0x0000 },	// 8575 ------ [　]  
{ /* 0x8576, */ 0x0000 },	// 8576 ------ [　]  
{ /* 0x8577, */ 0x0000 },	// 8577 ------ [　]  
{ /* 0x8578, */ 0x0000 },	// 8578 ------ [　]  
{ /* 0x8579, */ 0x0000 },	// 8579 ------ [　]  
{ /* 0x857A, */ 0x0000 },	// 857A ------ [　]  
{ /* 0x857B, */ 0x0000 },	// 857B ------ [　]  
{ /* 0x857C, */ 0x0000 },	// 857C ------ [　]  
{ /* 0x857D, */ 0x0000 },	// 857D ------ [　]  
{ /* 0x857E, */ 0x0000 },	// 857E ------ [　]  
{ /* 0x857F, */ 0x0000 },	// 		　#ダミー
{ /* 0x8580, */ 0x0000 },	// 8580 ------ [　]  
{ /* 0x8581, */ 0x0000 },	// 8581 ------ [　]  
{ /* 0x8582, */ 0x0000 },	// 8582 ------ [　]  
{ /* 0x8583, */ 0x0000 },	// 8583 ------ [　]  
{ /* 0x8584, */ 0x0000 },	// 8584 ------ [　]  
{ /* 0x8585, */ 0x0000 },	// 8585 ------ [　]  
{ /* 0x8586, */ 0x0000 },	// 8586 ------ [　]  
{ /* 0x8587, */ 0x0000 },	// 8587 ------ [　]  
{ /* 0x8588, */ 0x0000 },	// 8588 ------ [　]  
{ /* 0x8589, */ 0x0000 },	// 8589 ------ [　]  
{ /* 0x858A, */ 0x0000 },	// 858A ------ [　]  
{ /* 0x858B, */ 0x0000 },	// 858B ------ [　]  
{ /* 0x858C, */ 0x0000 },	// 858C ------ [　]  
{ /* 0x858D, */ 0x0000 },	// 858D ------ [　]  
{ /* 0x858E, */ 0x0000 },	// 858E ------ [　]  
{ /* 0x858F, */ 0x0000 },	// 858F ------ [　]  
{ /* 0x8590, */ 0x0000 },	// 8590 ------ [　]  
{ /* 0x8591, */ 0x0000 },	// 8591 ------ [　]  
{ /* 0x8592, */ 0x0000 },	// 8592 ------ [　]  
{ /* 0x8593, */ 0x0000 },	// 8593 ------ [　]  
{ /* 0x8594, */ 0x0000 },	// 8594 ------ [　]  
{ /* 0x8595, */ 0x0000 },	// 8595 ------ [　]  
{ /* 0x8596, */ 0x0000 },	// 8596 ------ [　]  
{ /* 0x8597, */ 0x0000 },	// 8597 ------ [　]  
{ /* 0x8598, */ 0x0000 },	// 8598 ------ [　]  
{ /* 0x8599, */ 0x0000 },	// 8599 ------ [　]  
{ /* 0x859A, */ 0x0000 },	// 859A ------ [　]  
{ /* 0x859B, */ 0x0000 },	// 859B ------ [　]  
{ /* 0x859C, */ 0x0000 },	// 859C ------ [　]  
{ /* 0x859D, */ 0x0000 },	// 859D ------ [　]  
{ /* 0x859E, */ 0x0000 },	// 859E ------ [　]  
{ /* 0x859F, */ 0x0000 },	// 859F ------ [　]  
{ /* 0x85A0, */ 0x0000 },	// 85A0 ------ [　]  
{ /* 0x85A1, */ 0x0000 },	// 85A1 ------ [　]  
{ /* 0x85A2, */ 0x0000 },	// 85A2 ------ [　]  
{ /* 0x85A3, */ 0x0000 },	// 85A3 ------ [　]  
{ /* 0x85A4, */ 0x0000 },	// 85A4 ------ [　]  
{ /* 0x85A5, */ 0x0000 },	// 85A5 ------ [　]  
{ /* 0x85A6, */ 0x0000 },	// 85A6 ------ [　]  
{ /* 0x85A7, */ 0x0000 },	// 85A7 ------ [　]  
{ /* 0x85A8, */ 0x0000 },	// 85A8 ------ [　]  
{ /* 0x85A9, */ 0x0000 },	// 85A9 ------ [　]  
{ /* 0x85AA, */ 0x0000 },	// 85AA ------ [　]  
{ /* 0x85AB, */ 0x0000 },	// 85AB ------ [　]  
{ /* 0x85AC, */ 0x0000 },	// 85AC ------ [　]  
{ /* 0x85AD, */ 0x0000 },	// 85AD ------ [　]  
{ /* 0x85AE, */ 0x0000 },	// 85AE ------ [　]  
{ /* 0x85AF, */ 0x0000 },	// 85AF ------ [　]  
{ /* 0x85B0, */ 0x0000 },	// 85B0 ------ [　]  
{ /* 0x85B1, */ 0x0000 },	// 85B1 ------ [　]  
{ /* 0x85B2, */ 0x0000 },	// 85B2 ------ [　]  
{ /* 0x85B3, */ 0x0000 },	// 85B3 ------ [　]  
{ /* 0x85B4, */ 0x0000 },	// 85B4 ------ [　]  
{ /* 0x85B5, */ 0x0000 },	// 85B5 ------ [　]  
{ /* 0x85B6, */ 0x0000 },	// 85B6 ------ [　]  
{ /* 0x85B7, */ 0x0000 },	// 85B7 ------ [　]  
{ /* 0x85B8, */ 0x0000 },	// 85B8 ------ [　]  
{ /* 0x85B9, */ 0x0000 },	// 85B9 ------ [　]  
{ /* 0x85BA, */ 0x0000 },	// 85BA ------ [　]  
{ /* 0x85BB, */ 0x0000 },	// 85BB ------ [　]  
{ /* 0x85BC, */ 0x0000 },	// 85BC ------ [　]  
{ /* 0x85BD, */ 0x0000 },	// 85BD ------ [　]  
{ /* 0x85BE, */ 0x0000 },	// 85BE ------ [　]  
{ /* 0x85BF, */ 0x0000 },	// 85BF ------ [　]  
{ /* 0x85C0, */ 0x0000 },	// 85C0 ------ [　]  
{ /* 0x85C1, */ 0x0000 },	// 85C1 ------ [　]  
{ /* 0x85C2, */ 0x0000 },	// 85C2 ------ [　]  
{ /* 0x85C3, */ 0x0000 },	// 85C3 ------ [　]  
{ /* 0x85C4, */ 0x0000 },	// 85C4 ------ [　]  
{ /* 0x85C5, */ 0x0000 },	// 85C5 ------ [　]  
{ /* 0x85C6, */ 0x0000 },	// 85C6 ------ [　]  
{ /* 0x85C7, */ 0x0000 },	// 85C7 ------ [　]  
{ /* 0x85C8, */ 0x0000 },	// 85C8 ------ [　]  
{ /* 0x85C9, */ 0x0000 },	// 85C9 ------ [　]  
{ /* 0x85CA, */ 0x0000 },	// 85CA ------ [　]  
{ /* 0x85CB, */ 0x0000 },	// 85CB ------ [　]  
{ /* 0x85CC, */ 0x0000 },	// 85CC ------ [　]  
{ /* 0x85CD, */ 0x0000 },	// 85CD ------ [　]  
{ /* 0x85CE, */ 0x0000 },	// 85CE ------ [　]  
{ /* 0x85CF, */ 0x0000 },	// 85CF ------ [　]  
{ /* 0x85D0, */ 0x0000 },	// 85D0 ------ [　]  
{ /* 0x85D1, */ 0x0000 },	// 85D1 ------ [　]  
{ /* 0x85D2, */ 0x0000 },	// 85D2 ------ [　]  
{ /* 0x85D3, */ 0x0000 },	// 85D3 ------ [　]  
{ /* 0x85D4, */ 0x0000 },	// 85D4 ------ [　]  
{ /* 0x85D5, */ 0x0000 },	// 85D5 ------ [　]  
{ /* 0x85D6, */ 0x0000 },	// 85D6 ------ [　]  
{ /* 0x85D7, */ 0x0000 },	// 85D7 ------ [　]  
{ /* 0x85D8, */ 0x0000 },	// 85D8 ------ [　]  
{ /* 0x85D9, */ 0x0000 },	// 85D9 ------ [　]  
{ /* 0x85DA, */ 0x0000 },	// 85DA ------ [　]  
{ /* 0x85DB, */ 0x0000 },	// 85DB ------ [　]  
{ /* 0x85DC, */ 0x0000 },	// 85DC ------ [　]  
{ /* 0x85DD, */ 0x0000 },	// 85DD ------ [　]  
{ /* 0x85DE, */ 0x0000 },	// 85DE ------ [　]  
{ /* 0x85DF, */ 0x0000 },	// 85DF ------ [　]  
{ /* 0x85E0, */ 0x0000 },	// 85E0 ------ [　]  
{ /* 0x85E1, */ 0x0000 },	// 85E1 ------ [　]  
{ /* 0x85E2, */ 0x0000 },	// 85E2 ------ [　]  
{ /* 0x85E3, */ 0x0000 },	// 85E3 ------ [　]  
{ /* 0x85E4, */ 0x0000 },	// 85E4 ------ [　]  
{ /* 0x85E5, */ 0x0000 },	// 85E5 ------ [　]  
{ /* 0x85E6, */ 0x0000 },	// 85E6 ------ [　]  
{ /* 0x85E7, */ 0x0000 },	// 85E7 ------ [　]  
{ /* 0x85E8, */ 0x0000 },	// 85E8 ------ [　]  
{ /* 0x85E9, */ 0x0000 },	// 85E9 ------ [　]  
{ /* 0x85EA, */ 0x0000 },	// 85EA ------ [　]  
{ /* 0x85EB, */ 0x0000 },	// 85EB ------ [　]  
{ /* 0x85EC, */ 0x0000 },	// 85EC ------ [　]  
{ /* 0x85ED, */ 0x0000 },	// 85ED ------ [　]  
{ /* 0x85EE, */ 0x0000 },	// 85EE ------ [　]  
{ /* 0x85EF, */ 0x0000 },	// 85EF ------ [　]  
{ /* 0x85F0, */ 0x0000 },	// 85F0 ------ [　]  
{ /* 0x85F1, */ 0x0000 },	// 85F1 ------ [　]  
{ /* 0x85F2, */ 0x0000 },	// 85F2 ------ [　]  
{ /* 0x85F3, */ 0x0000 },	// 85F3 ------ [　]  
{ /* 0x85F4, */ 0x0000 },	// 85F4 ------ [　]  
{ /* 0x85F5, */ 0x0000 },	// 85F5 ------ [　]  
{ /* 0x85F6, */ 0x0000 },	// 85F6 ------ [　]  
{ /* 0x85F7, */ 0x0000 },	// 85F7 ------ [　]  
{ /* 0x85F8, */ 0x0000 },	// 85F8 ------ [　]  
{ /* 0x85F9, */ 0x0000 },	// 85F9 ------ [　]  
{ /* 0x85FA, */ 0x0000 },	// 85FA ------ [　]  
{ /* 0x85FB, */ 0x0000 },	// 85FB ------ [　]  
{ /* 0x85FC, */ 0x0000 },	// 85FC ------ [　]  
//===== 0x86 40～FC ( テーブル数 [189]  ）====================
{ /* 0x8640, */ 0x0000 },	// 8640 ------ [　]  
{ /* 0x8641, */ 0x0000 },	// 8641 ------ [　]  
{ /* 0x8642, */ 0x0000 },	// 8642 ------ [　]  
{ /* 0x8643, */ 0x0000 },	// 8643 ------ [　]  
{ /* 0x8644, */ 0x0000 },	// 8644 ------ [　]  
{ /* 0x8645, */ 0x0000 },	// 8645 ------ [　]  
{ /* 0x8646, */ 0x0000 },	// 8646 ------ [　]  
{ /* 0x8647, */ 0x0000 },	// 8647 ------ [　]  
{ /* 0x8648, */ 0x0000 },	// 8648 ------ [　]  
{ /* 0x8649, */ 0x0000 },	// 8649 ------ [　]  
{ /* 0x864A, */ 0x0000 },	// 864A ------ [　]  
{ /* 0x864B, */ 0x0000 },	// 864B ------ [　]  
{ /* 0x864C, */ 0x0000 },	// 864C ------ [　]  
{ /* 0x864D, */ 0x0000 },	// 864D ------ [　]  
{ /* 0x864E, */ 0x0000 },	// 864E ------ [　]  
{ /* 0x864F, */ 0x0000 },	// 864F ------ [　]  
{ /* 0x8650, */ 0x0000 },	// 8650 ------ [　]  
{ /* 0x8651, */ 0x0000 },	// 8651 ------ [　]  
{ /* 0x8652, */ 0x0000 },	// 8652 ------ [　]  
{ /* 0x8653, */ 0x0000 },	// 8653 ------ [　]  
{ /* 0x8654, */ 0x0000 },	// 8654 ------ [　]  
{ /* 0x8655, */ 0x0000 },	// 8655 ------ [　]  
{ /* 0x8656, */ 0x0000 },	// 8656 ------ [　]  
{ /* 0x8657, */ 0x0000 },	// 8657 ------ [　]  
{ /* 0x8658, */ 0x0000 },	// 8658 ------ [　]  
{ /* 0x8659, */ 0x0000 },	// 8659 ------ [　]  
{ /* 0x865A, */ 0x0000 },	// 865A ------ [　]  
{ /* 0x865B, */ 0x0000 },	// 865B ------ [　]  
{ /* 0x865C, */ 0x0000 },	// 865C ------ [　]  
{ /* 0x865D, */ 0x0000 },	// 865D ------ [　]  
{ /* 0x865E, */ 0x0000 },	// 865E ------ [　]  
{ /* 0x865F, */ 0x0000 },	// 865F ------ [　]  
{ /* 0x8660, */ 0x0000 },	// 8660 ------ [　]  
{ /* 0x8661, */ 0x0000 },	// 8661 ------ [　]  
{ /* 0x8662, */ 0x0000 },	// 8662 ------ [　]  
{ /* 0x8663, */ 0x0000 },	// 8663 ------ [　]  
{ /* 0x8664, */ 0x0000 },	// 8664 ------ [　]  
{ /* 0x8665, */ 0x0000 },	// 8665 ------ [　]  
{ /* 0x8666, */ 0x0000 },	// 8666 ------ [　]  
{ /* 0x8667, */ 0x0000 },	// 8667 ------ [　]  
{ /* 0x8668, */ 0x0000 },	// 8668 ------ [　]  
{ /* 0x8669, */ 0x0000 },	// 8669 ------ [　]  
{ /* 0x866A, */ 0x0000 },	// 866A ------ [　]  
{ /* 0x866B, */ 0x0000 },	// 866B ------ [　]  
{ /* 0x866C, */ 0x0000 },	// 866C ------ [　]  
{ /* 0x866D, */ 0x0000 },	// 866D ------ [　]  
{ /* 0x866E, */ 0x0000 },	// 866E ------ [　]  
{ /* 0x866F, */ 0x0000 },	// 866F ------ [　]  
{ /* 0x8670, */ 0x0000 },	// 8670 ------ [　]  
{ /* 0x8671, */ 0x0000 },	// 8671 ------ [　]  
{ /* 0x8672, */ 0x0000 },	// 8672 ------ [　]  
{ /* 0x8673, */ 0x0000 },	// 8673 ------ [　]  
{ /* 0x8674, */ 0x0000 },	// 8674 ------ [　]  
{ /* 0x8675, */ 0x0000 },	// 8675 ------ [　]  
{ /* 0x8676, */ 0x0000 },	// 8676 ------ [　]  
{ /* 0x8677, */ 0x0000 },	// 8677 ------ [　]  
{ /* 0x8678, */ 0x0000 },	// 8678 ------ [　]  
{ /* 0x8679, */ 0x0000 },	// 8679 ------ [　]  
{ /* 0x867A, */ 0x0000 },	// 867A ------ [　]  
{ /* 0x867B, */ 0x0000 },	// 867B ------ [　]  
{ /* 0x867C, */ 0x0000 },	// 867C ------ [　]  
{ /* 0x867D, */ 0x0000 },	// 867D ------ [　]  
{ /* 0x867E, */ 0x0000 },	// 867E ------ [　]  
{ /* 0x867F, */ 0x0000 },	// 		　#ダミー
{ /* 0x8680, */ 0x0000 },	// 8680 ------ [　]  
{ /* 0x8681, */ 0x0000 },	// 8681 ------ [　]  
{ /* 0x8682, */ 0x0000 },	// 8682 ------ [　]  
{ /* 0x8683, */ 0x0000 },	// 8683 ------ [　]  
{ /* 0x8684, */ 0x0000 },	// 8684 ------ [　]  
{ /* 0x8685, */ 0x0000 },	// 8685 ------ [　]  
{ /* 0x8686, */ 0x0000 },	// 8686 ------ [　]  
{ /* 0x8687, */ 0x0000 },	// 8687 ------ [　]  
{ /* 0x8688, */ 0x0000 },	// 8688 ------ [　]  
{ /* 0x8689, */ 0x0000 },	// 8689 ------ [　]  
{ /* 0x868A, */ 0x0000 },	// 868A ------ [　]  
{ /* 0x868B, */ 0x0000 },	// 868B ------ [　]  
{ /* 0x868C, */ 0x0000 },	// 868C ------ [　]  
{ /* 0x868D, */ 0x0000 },	// 868D ------ [　]  
{ /* 0x868E, */ 0x0000 },	// 868E ------ [　]  
{ /* 0x868F, */ 0x0000 },	// 868F ------ [　]  
{ /* 0x8690, */ 0x0000 },	// 8690 ------ [　]  
{ /* 0x8691, */ 0x0000 },	// 8691 ------ [　]  
{ /* 0x8692, */ 0x0000 },	// 8692 ------ [　]  
{ /* 0x8693, */ 0x0000 },	// 8693 ------ [　]  
{ /* 0x8694, */ 0x0000 },	// 8694 ------ [　]  
{ /* 0x8695, */ 0x0000 },	// 8695 ------ [　]  
{ /* 0x8696, */ 0x0000 },	// 8696 ------ [　]  
{ /* 0x8697, */ 0x0000 },	// 8697 ------ [　]  
{ /* 0x8698, */ 0x0000 },	// 8698 ------ [　]  
{ /* 0x8699, */ 0x0000 },	// 8699 ------ [　]  
{ /* 0x869A, */ 0x0000 },	// 869A ------ [　]  
{ /* 0x869B, */ 0x0000 },	// 869B ------ [　]  
{ /* 0x869C, */ 0x0000 },	// 869C ------ [　]  
{ /* 0x869D, */ 0x0000 },	// 869D ------ [　]  
{ /* 0x869E, */ 0x0000 },	// 869E ------ [　]  
{ /* 0x869F, */ 0x0000 },	// 869F ------ [　]  
{ /* 0x86A0, */ 0x0000 },	// 86A0 ------ [　]  
{ /* 0x86A1, */ 0x0000 },	// 86A1 ------ [　]  
{ /* 0x86A2, */ 0x0000 },	// 86A2 ------ [　]  
{ /* 0x86A3, */ 0x0000 },	// 86A3 ------ [　]  
{ /* 0x86A4, */ 0x0000 },	// 86A4 ------ [　]  
{ /* 0x86A5, */ 0x0000 },	// 86A5 ------ [　]  
{ /* 0x86A6, */ 0x0000 },	// 86A6 ------ [　]  
{ /* 0x86A7, */ 0x0000 },	// 86A7 ------ [　]  
{ /* 0x86A8, */ 0x0000 },	// 86A8 ------ [　]  
{ /* 0x86A9, */ 0x0000 },	// 86A9 ------ [　]  
{ /* 0x86AA, */ 0x0000 },	// 86AA ------ [　]  
{ /* 0x86AB, */ 0x0000 },	// 86AB ------ [　]  
{ /* 0x86AC, */ 0x0000 },	// 86AC ------ [　]  
{ /* 0x86AD, */ 0x0000 },	// 86AD ------ [　]  
{ /* 0x86AE, */ 0x0000 },	// 86AE ------ [　]  
{ /* 0x86AF, */ 0x0000 },	// 86AF ------ [　]  
{ /* 0x86B0, */ 0x0000 },	// 86B0 ------ [　]  
{ /* 0x86B1, */ 0x0000 },	// 86B1 ------ [　]  
{ /* 0x86B2, */ 0x0000 },	// 86B2 ------ [　]  
{ /* 0x86B3, */ 0x0000 },	// 86B3 ------ [　]  
{ /* 0x86B4, */ 0x0000 },	// 86B4 ------ [　]  
{ /* 0x86B5, */ 0x0000 },	// 86B5 ------ [　]  
{ /* 0x86B6, */ 0x0000 },	// 86B6 ------ [　]  
{ /* 0x86B7, */ 0x0000 },	// 86B7 ------ [　]  
{ /* 0x86B8, */ 0x0000 },	// 86B8 ------ [　]  
{ /* 0x86B9, */ 0x0000 },	// 86B9 ------ [　]  
{ /* 0x86BA, */ 0x0000 },	// 86BA ------ [　]  
{ /* 0x86BB, */ 0x0000 },	// 86BB ------ [　]  
{ /* 0x86BC, */ 0x0000 },	// 86BC ------ [　]  
{ /* 0x86BD, */ 0x0000 },	// 86BD ------ [　]  
{ /* 0x86BE, */ 0x0000 },	// 86BE ------ [　]  
{ /* 0x86BF, */ 0x0000 },	// 86BF ------ [　]  
{ /* 0x86C0, */ 0x0000 },	// 86C0 ------ [　]  
{ /* 0x86C1, */ 0x0000 },	// 86C1 ------ [　]  
{ /* 0x86C2, */ 0x0000 },	// 86C2 ------ [　]  
{ /* 0x86C3, */ 0x0000 },	// 86C3 ------ [　]  
{ /* 0x86C4, */ 0x0000 },	// 86C4 ------ [　]  
{ /* 0x86C5, */ 0x0000 },	// 86C5 ------ [　]  
{ /* 0x86C6, */ 0x0000 },	// 86C6 ------ [　]  
{ /* 0x86C7, */ 0x0000 },	// 86C7 ------ [　]  
{ /* 0x86C8, */ 0x0000 },	// 86C8 ------ [　]  
{ /* 0x86C9, */ 0x0000 },	// 86C9 ------ [　]  
{ /* 0x86CA, */ 0x0000 },	// 86CA ------ [　]  
{ /* 0x86CB, */ 0x0000 },	// 86CB ------ [　]  
{ /* 0x86CC, */ 0x0000 },	// 86CC ------ [　]  
{ /* 0x86CD, */ 0x0000 },	// 86CD ------ [　]  
{ /* 0x86CE, */ 0x0000 },	// 86CE ------ [　]  
{ /* 0x86CF, */ 0x0000 },	// 86CF ------ [　]  
{ /* 0x86D0, */ 0x0000 },	// 86D0 ------ [　]  
{ /* 0x86D1, */ 0x0000 },	// 86D1 ------ [　]  
{ /* 0x86D2, */ 0x0000 },	// 86D2 ------ [　]  
{ /* 0x86D3, */ 0x0000 },	// 86D3 ------ [　]  
{ /* 0x86D4, */ 0x0000 },	// 86D4 ------ [　]  
{ /* 0x86D5, */ 0x0000 },	// 86D5 ------ [　]  
{ /* 0x86D6, */ 0x0000 },	// 86D6 ------ [　]  
{ /* 0x86D7, */ 0x0000 },	// 86D7 ------ [　]  
{ /* 0x86D8, */ 0x0000 },	// 86D8 ------ [　]  
{ /* 0x86D9, */ 0x0000 },	// 86D9 ------ [　]  
{ /* 0x86DA, */ 0x0000 },	// 86DA ------ [　]  
{ /* 0x86DB, */ 0x0000 },	// 86DB ------ [　]  
{ /* 0x86DC, */ 0x0000 },	// 86DC ------ [　]  
{ /* 0x86DD, */ 0x0000 },	// 86DD ------ [　]  
{ /* 0x86DE, */ 0x0000 },	// 86DE ------ [　]  
{ /* 0x86DF, */ 0x0000 },	// 86DF ------ [　]  
{ /* 0x86E0, */ 0x0000 },	// 86E0 ------ [　]  
{ /* 0x86E1, */ 0x0000 },	// 86E1 ------ [　]  
{ /* 0x86E2, */ 0x0000 },	// 86E2 ------ [　]  
{ /* 0x86E3, */ 0x0000 },	// 86E3 ------ [　]  
{ /* 0x86E4, */ 0x0000 },	// 86E4 ------ [　]  
{ /* 0x86E5, */ 0x0000 },	// 86E5 ------ [　]  
{ /* 0x86E6, */ 0x0000 },	// 86E6 ------ [　]  
{ /* 0x86E7, */ 0x0000 },	// 86E7 ------ [　]  
{ /* 0x86E8, */ 0x0000 },	// 86E8 ------ [　]  
{ /* 0x86E9, */ 0x0000 },	// 86E9 ------ [　]  
{ /* 0x86EA, */ 0x0000 },	// 86EA ------ [　]  
{ /* 0x86EB, */ 0x0000 },	// 86EB ------ [　]  
{ /* 0x86EC, */ 0x0000 },	// 86EC ------ [　]  
{ /* 0x86ED, */ 0x0000 },	// 86ED ------ [　]  
{ /* 0x86EE, */ 0x0000 },	// 86EE ------ [　]  
{ /* 0x86EF, */ 0x0000 },	// 86EF ------ [　]  
{ /* 0x86F0, */ 0x0000 },	// 86F0 ------ [　]  
{ /* 0x86F1, */ 0x0000 },	// 86F1 ------ [　]  
{ /* 0x86F2, */ 0x0000 },	// 86F2 ------ [　]  
{ /* 0x86F3, */ 0x0000 },	// 86F3 ------ [　]  
{ /* 0x86F4, */ 0x0000 },	// 86F4 ------ [　]  
{ /* 0x86F5, */ 0x0000 },	// 86F5 ------ [　]  
{ /* 0x86F6, */ 0x0000 },	// 86F6 ------ [　]  
{ /* 0x86F7, */ 0x0000 },	// 86F7 ------ [　]  
{ /* 0x86F8, */ 0x0000 },	// 86F8 ------ [　]  
{ /* 0x86F9, */ 0x0000 },	// 86F9 ------ [　]  
{ /* 0x86FA, */ 0x0000 },	// 86FA ------ [　]  
{ /* 0x86FB, */ 0x0000 },	// 86FB ------ [　]  
{ /* 0x86FC, */ 0x0000 },	// 86FC ------ [　]  
//===== 0x87 40～FC ( テーブル数 [189]  ）====================
{ /* 0x8740, */ 0x2460 },	// 8740 ------ [①]  #CIRCLED DIGIT ONE
{ /* 0x8741, */ 0x2461 },	// 8741 ------ [②]  #CIRCLED DIGIT TWO
{ /* 0x8742, */ 0x2462 },	// 8742 ------ [③]  #CIRCLED DIGIT THREE
{ /* 0x8743, */ 0x2463 },	// 8743 ------ [④]  #CIRCLED DIGIT FOUR
{ /* 0x8744, */ 0x2464 },	// 8744 ------ [⑤]  #CIRCLED DIGIT FIVE
{ /* 0x8745, */ 0x2465 },	// 8745 ------ [⑥]  #CIRCLED DIGIT SIX
{ /* 0x8746, */ 0x2466 },	// 8746 ------ [⑦]  #CIRCLED DIGIT SEVEN
{ /* 0x8747, */ 0x2467 },	// 8747 ------ [⑧]  #CIRCLED DIGIT EIGHT
{ /* 0x8748, */ 0x2468 },	// 8748 ------ [⑨]  #CIRCLED DIGIT NINE
{ /* 0x8749, */ 0x2469 },	// 8749 ------ [⑩]  #CIRCLED NUMBER TEN
{ /* 0x874A, */ 0x246A },	// 874A ------ [⑪]  #CIRCLED NUMBER ELEVEN
{ /* 0x874B, */ 0x246B },	// 874B ------ [⑫]  #CIRCLED NUMBER TWELVE
{ /* 0x874C, */ 0x246C },	// 874C ------ [⑬]  #CIRCLED NUMBER THIRTEEN
{ /* 0x874D, */ 0x246D },	// 874D ------ [⑭]  #CIRCLED NUMBER FOURTEEN
{ /* 0x874E, */ 0x246E },	// 874E ------ [⑮]  #CIRCLED NUMBER FIFTEEN
{ /* 0x874F, */ 0x246F },	// 874F ------ [⑯]  #CIRCLED NUMBER SIXTEEN
{ /* 0x8750, */ 0x2470 },	// 8750 ------ [⑰]  #CIRCLED NUMBER SEVENTEEN
{ /* 0x8751, */ 0x2471 },	// 8751 ------ [⑱]  #CIRCLED NUMBER EIGHTEEN
{ /* 0x8752, */ 0x2472 },	// 8752 ------ [⑲]  #CIRCLED NUMBER NINETEEN
{ /* 0x8753, */ 0x2473 },	// 8753 ------ [⑳]  #CIRCLED NUMBER TWENTY
{ /* 0x8754, */ 0x2160 },	// 8754 ------ [Ⅰ]  #ROMAN NUMERAL ONE
{ /* 0x8755, */ 0x2161 },	// 8755 ------ [Ⅱ]  #ROMAN NUMERAL TWO
{ /* 0x8756, */ 0x2162 },	// 8756 ------ [Ⅲ]  #ROMAN NUMERAL THREE
{ /* 0x8757, */ 0x2163 },	// 8757 ------ [Ⅳ]  #ROMAN NUMERAL FOUR
{ /* 0x8758, */ 0x2164 },	// 8758 ------ [Ⅴ]  #ROMAN NUMERAL FIVE
{ /* 0x8759, */ 0x2165 },	// 8759 ------ [Ⅵ]  #ROMAN NUMERAL SIX
{ /* 0x875A, */ 0x2166 },	// 875A ------ [Ⅶ]  #ROMAN NUMERAL SEVEN
{ /* 0x875B, */ 0x2167 },	// 875B ------ [Ⅷ]  #ROMAN NUMERAL EIGHT
{ /* 0x875C, */ 0x2168 },	// 875C ------ [Ⅸ]  #ROMAN NUMERAL NINE
{ /* 0x875D, */ 0x2169 },	// 875D ------ [Ⅹ]  #ROMAN NUMERAL TEN
{ /* 0x875E, */ 0x0000 },	// 875E ------ [　]  
{ /* 0x875F, */ 0x3349 },	// 875F ------ [㍉]  #SQUARE MIRI
{ /* 0x8760, */ 0x3314 },	// 8760 ------ [㌔]  #SQUARE KIRO
{ /* 0x8761, */ 0x3322 },	// 8761 ------ [㌢]  #SQUARE SENTI
{ /* 0x8762, */ 0x334D },	// 8762 ------ [㍍]  #SQUARE MEETORU
{ /* 0x8763, */ 0x3318 },	// 8763 ------ [㌘]  #SQUARE GURAMU
{ /* 0x8764, */ 0x3327 },	// 8764 ------ [㌧]  #SQUARE TON
{ /* 0x8765, */ 0x3303 },	// 8765 ------ [㌃]  #SQUARE AARU
{ /* 0x8766, */ 0x3336 },	// 8766 ------ [㌶]  #SQUARE HEKUTAARU
{ /* 0x8767, */ 0x3351 },	// 8767 ------ [㍑]  #SQUARE RITTORU
{ /* 0x8768, */ 0x3357 },	// 8768 ------ [㍗]  #SQUARE WATTO
{ /* 0x8769, */ 0x330D },	// 8769 ------ [㌍]  #SQUARE KARORII
{ /* 0x876A, */ 0x3326 },	// 876A ------ [㌦]  #SQUARE DORU
{ /* 0x876B, */ 0x3323 },	// 876B ------ [㌣]  #SQUARE SENTO
{ /* 0x876C, */ 0x332B },	// 876C ------ [㌫]  #SQUARE PAASENTO
{ /* 0x876D, */ 0x334A },	// 876D ------ [㍊]  #SQUARE MIRIBAARU
{ /* 0x876E, */ 0x333B },	// 876E ------ [㌻]  #SQUARE PEEZI
{ /* 0x876F, */ 0x339C },	// 876F ------ [㎜]  #SQUARE MM
{ /* 0x8770, */ 0x339D },	// 8770 ------ [㎝]  #SQUARE CM
{ /* 0x8771, */ 0x339E },	// 8771 ------ [㎞]  #SQUARE KM
{ /* 0x8772, */ 0x338E },	// 8772 ------ [㎎]  #SQUARE MG
{ /* 0x8773, */ 0x338F },	// 8773 ------ [㎏]  #SQUARE KG
{ /* 0x8774, */ 0x33C4 },	// 8774 ------ [㏄]  #SQUARE CC
{ /* 0x8775, */ 0x33A1 },	// 8775 ------ [㎡]  #SQUARE M SQUARED
{ /* 0x8776, */ 0x0000 },	// 8776 ------ [　]  
{ /* 0x8777, */ 0x0000 },	// 8777 ------ [　]  
{ /* 0x8778, */ 0x0000 },	// 8778 ------ [　]  
{ /* 0x8779, */ 0x0000 },	// 8779 ------ [　]  
{ /* 0x877A, */ 0x0000 },	// 877A ------ [　]  
{ /* 0x877B, */ 0x0000 },	// 877B ------ [　]  
{ /* 0x877C, */ 0x0000 },	// 877C ------ [　]  
{ /* 0x877D, */ 0x0000 },	// 877D ------ [　]  
{ /* 0x877E, */ 0x337B },	// 877E ------ [㍻]  #SQUARE ERA NAME HEISEI
{ /* 0x877F, */ 0x0000 },	// 		　#ダミー
{ /* 0x8780, */ 0x301D },	// 8780 ------ [〝]  #REVERSED DOUBLE PRIME QUOTATION MARK
{ /* 0x8781, */ 0x301F },	// 8781 ------ [〟]  #LOW DOUBLE PRIME QUOTATION MARK
{ /* 0x8782, */ 0x2116 },	// 8782 ------ [№]  #NUMERO SIGN
{ /* 0x8783, */ 0x33CD },	// 8783 ------ [㏍]  #SQUARE KK
{ /* 0x8784, */ 0x2121 },	// 8784 ------ [℡]  #TELEPHONE SIGN
{ /* 0x8785, */ 0x32A4 },	// 8785 ------ [㊤]  #CIRCLED IDEOGRAPH HIGH
{ /* 0x8786, */ 0x32A5 },	// 8786 ------ [㊥]  #CIRCLED IDEOGRAPH CENTRE
{ /* 0x8787, */ 0x32A6 },	// 8787 ------ [㊦]  #CIRCLED IDEOGRAPH LOW
{ /* 0x8788, */ 0x32A7 },	// 8788 ------ [㊧]  #CIRCLED IDEOGRAPH LEFT
{ /* 0x8789, */ 0x32A8 },	// 8789 ------ [㊨]  #CIRCLED IDEOGRAPH RIGHT
{ /* 0x878A, */ 0x3231 },	// 878A ------ [㈱]  #PARENTHESIZED IDEOGRAPH STOCK
{ /* 0x878B, */ 0x3232 },	// 878B ------ [㈲]  #PARENTHESIZED IDEOGRAPH HAVE
{ /* 0x878C, */ 0x3239 },	// 878C ------ [㈹]  #PARENTHESIZED IDEOGRAPH REPRESENT
{ /* 0x878D, */ 0x337E },	// 878D ------ [㍾]  #SQUARE ERA NAME MEIZI
{ /* 0x878E, */ 0x337D },	// 878E ------ [㍽]  #SQUARE ERA NAME TAISYOU
{ /* 0x878F, */ 0x337C },	// 878F ------ [㍼]  #SQUARE ERA NAME SYOUWA
{ /* 0x8790, */ 0x2252 },	// 8790 ------ [≒]  #APPROXIMATELY EQUAL TO OR THE IMAGE OF
{ /* 0x8791, */ 0x2261 },	// 8791 ------ [≡]  #IDENTICAL TO
{ /* 0x8792, */ 0x222B },	// 8792 ------ [∫]  #INTEGRAL
{ /* 0x8793, */ 0x222E },	// 8793 ------ [∮]  #CONTOUR INTEGRAL
{ /* 0x8794, */ 0x2211 },	// 8794 ------ [∑]  #N-ARY SUMMATION
{ /* 0x8795, */ 0x221A },	// 8795 ------ [√]  #SQUARE ROOT
{ /* 0x8796, */ 0x22A5 },	// 8796 ------ [⊥]  #UP TACK
{ /* 0x8797, */ 0x2220 },	// 8797 ------ [∠]  #ANGLE
{ /* 0x8798, */ 0x221F },	// 8798 ------ [∟]  #RIGHT ANGLE
{ /* 0x8799, */ 0x22BF },	// 8799 ------ [⊿]  #RIGHT TRIANGLE
{ /* 0x879A, */ 0x2235 },	// 879A ------ [∵]  #BECAUSE
{ /* 0x879B, */ 0x2229 },	// 879B ------ [∩]  #INTERSECTION
{ /* 0x879C, */ 0x222A },	// 879C ------ [∪]  #UNION
{ /* 0x879D, */ 0x0000 },	// 879D ------ [　]  
{ /* 0x879E, */ 0x0000 },	// 879E ------ [　]  
{ /* 0x879F, */ 0x0000 },	// 879F ------ [　]  
{ /* 0x87A0, */ 0x0000 },	// 87A0 ------ [　]  
{ /* 0x87A1, */ 0x0000 },	// 87A1 ------ [　]  
{ /* 0x87A2, */ 0x0000 },	// 87A2 ------ [　]  
{ /* 0x87A3, */ 0x0000 },	// 87A3 ------ [　]  
{ /* 0x87A4, */ 0x0000 },	// 87A4 ------ [　]  
{ /* 0x87A5, */ 0x0000 },	// 87A5 ------ [　]  
{ /* 0x87A6, */ 0x0000 },	// 87A6 ------ [　]  
{ /* 0x87A7, */ 0x0000 },	// 87A7 ------ [　]  
{ /* 0x87A8, */ 0x0000 },	// 87A8 ------ [　]  
{ /* 0x87A9, */ 0x0000 },	// 87A9 ------ [　]  
{ /* 0x87AA, */ 0x0000 },	// 87AA ------ [　]  
{ /* 0x87AB, */ 0x0000 },	// 87AB ------ [　]  
{ /* 0x87AC, */ 0x0000 },	// 87AC ------ [　]  
{ /* 0x87AD, */ 0x0000 },	// 87AD ------ [　]  
{ /* 0x87AE, */ 0x0000 },	// 87AE ------ [　]  
{ /* 0x87AF, */ 0x0000 },	// 87AF ------ [　]  
{ /* 0x87B0, */ 0x0000 },	// 87B0 ------ [　]  
{ /* 0x87B1, */ 0x0000 },	// 87B1 ------ [　]  
{ /* 0x87B2, */ 0x0000 },	// 87B2 ------ [　]  
{ /* 0x87B3, */ 0x0000 },	// 87B3 ------ [　]  
{ /* 0x87B4, */ 0x0000 },	// 87B4 ------ [　]  
{ /* 0x87B5, */ 0x0000 },	// 87B5 ------ [　]  
{ /* 0x87B6, */ 0x0000 },	// 87B6 ------ [　]  
{ /* 0x87B7, */ 0x0000 },	// 87B7 ------ [　]  
{ /* 0x87B8, */ 0x0000 },	// 87B8 ------ [　]  
{ /* 0x87B9, */ 0x0000 },	// 87B9 ------ [　]  
{ /* 0x87BA, */ 0x0000 },	// 87BA ------ [　]  
{ /* 0x87BB, */ 0x0000 },	// 87BB ------ [　]  
{ /* 0x87BC, */ 0x0000 },	// 87BC ------ [　]  
{ /* 0x87BD, */ 0x0000 },	// 87BD ------ [　]  
{ /* 0x87BE, */ 0x0000 },	// 87BE ------ [　]  
{ /* 0x87BF, */ 0x0000 },	// 87BF ------ [　]  
{ /* 0x87C0, */ 0x0000 },	// 87C0 ------ [　]  
{ /* 0x87C1, */ 0x0000 },	// 87C1 ------ [　]  
{ /* 0x87C2, */ 0x0000 },	// 87C2 ------ [　]  
{ /* 0x87C3, */ 0x0000 },	// 87C3 ------ [　]  
{ /* 0x87C4, */ 0x0000 },	// 87C4 ------ [　]  
{ /* 0x87C5, */ 0x0000 },	// 87C5 ------ [　]  
{ /* 0x87C6, */ 0x0000 },	// 87C6 ------ [　]  
{ /* 0x87C7, */ 0x0000 },	// 87C7 ------ [　]  
{ /* 0x87C8, */ 0x0000 },	// 87C8 ------ [　]  
{ /* 0x87C9, */ 0x0000 },	// 87C9 ------ [　]  
{ /* 0x87CA, */ 0x0000 },	// 87CA ------ [　]  
{ /* 0x87CB, */ 0x0000 },	// 87CB ------ [　]  
{ /* 0x87CC, */ 0x0000 },	// 87CC ------ [　]  
{ /* 0x87CD, */ 0x0000 },	// 87CD ------ [　]  
{ /* 0x87CE, */ 0x0000 },	// 87CE ------ [　]  
{ /* 0x87CF, */ 0x0000 },	// 87CF ------ [　]  
{ /* 0x87D0, */ 0x0000 },	// 87D0 ------ [　]  
{ /* 0x87D1, */ 0x0000 },	// 87D1 ------ [　]  
{ /* 0x87D2, */ 0x0000 },	// 87D2 ------ [　]  
{ /* 0x87D3, */ 0x0000 },	// 87D3 ------ [　]  
{ /* 0x87D4, */ 0x0000 },	// 87D4 ------ [　]  
{ /* 0x87D5, */ 0x0000 },	// 87D5 ------ [　]  
{ /* 0x87D6, */ 0x0000 },	// 87D6 ------ [　]  
{ /* 0x87D7, */ 0x0000 },	// 87D7 ------ [　]  
{ /* 0x87D8, */ 0x0000 },	// 87D8 ------ [　]  
{ /* 0x87D9, */ 0x0000 },	// 87D9 ------ [　]  
{ /* 0x87DA, */ 0x0000 },	// 87DA ------ [　]  
{ /* 0x87DB, */ 0x0000 },	// 87DB ------ [　]  
{ /* 0x87DC, */ 0x0000 },	// 87DC ------ [　]  
{ /* 0x87DD, */ 0x0000 },	// 87DD ------ [　]  
{ /* 0x87DE, */ 0x0000 },	// 87DE ------ [　]  
{ /* 0x87DF, */ 0x0000 },	// 87DF ------ [　]  
{ /* 0x87E0, */ 0x0000 },	// 87E0 ------ [　]  
{ /* 0x87E1, */ 0x0000 },	// 87E1 ------ [　]  
{ /* 0x87E2, */ 0x0000 },	// 87E2 ------ [　]  
{ /* 0x87E3, */ 0x0000 },	// 87E3 ------ [　]  
{ /* 0x87E4, */ 0x0000 },	// 87E4 ------ [　]  
{ /* 0x87E5, */ 0x0000 },	// 87E5 ------ [　]  
{ /* 0x87E6, */ 0x0000 },	// 87E6 ------ [　]  
{ /* 0x87E7, */ 0x0000 },	// 87E7 ------ [　]  
{ /* 0x87E8, */ 0x0000 },	// 87E8 ------ [　]  
{ /* 0x87E9, */ 0x0000 },	// 87E9 ------ [　]  
{ /* 0x87EA, */ 0x0000 },	// 87EA ------ [　]  
{ /* 0x87EB, */ 0x0000 },	// 87EB ------ [　]  
{ /* 0x87EC, */ 0x0000 },	// 87EC ------ [　]  
{ /* 0x87ED, */ 0x0000 },	// 87ED ------ [　]  
{ /* 0x87EE, */ 0x0000 },	// 87EE ------ [　]  
{ /* 0x87EF, */ 0x0000 },	// 87EF ------ [　]  
{ /* 0x87F0, */ 0x0000 },	// 87F0 ------ [　]  
{ /* 0x87F1, */ 0x0000 },	// 87F1 ------ [　]  
{ /* 0x87F2, */ 0x0000 },	// 87F2 ------ [　]  
{ /* 0x87F3, */ 0x0000 },	// 87F3 ------ [　]  
{ /* 0x87F4, */ 0x0000 },	// 87F4 ------ [　]  
{ /* 0x87F5, */ 0x0000 },	// 87F5 ------ [　]  
{ /* 0x87F6, */ 0x0000 },	// 87F6 ------ [　]  
{ /* 0x87F7, */ 0x0000 },	// 87F7 ------ [　]  
{ /* 0x87F8, */ 0x0000 },	// 87F8 ------ [　]  
{ /* 0x87F9, */ 0x0000 },	// 87F9 ------ [　]  
{ /* 0x87FA, */ 0x0000 },	// 87FA ------ [　]  
{ /* 0x87FB, */ 0x0000 },	// 87FB ------ [　]  
{ /* 0x87FC, */ 0x0000 },	// 87FC ------ [　]  
//===== 0x88 40～FC ( テーブル数 [189]  ）====================
{ /* 0x8840, */ 0x0000 },	// 8840 ------ [　]  
{ /* 0x8841, */ 0x0000 },	// 8841 ------ [　]  
{ /* 0x8842, */ 0x0000 },	// 8842 ------ [　]  
{ /* 0x8843, */ 0x0000 },	// 8843 ------ [　]  
{ /* 0x8844, */ 0x0000 },	// 8844 ------ [　]  
{ /* 0x8845, */ 0x0000 },	// 8845 ------ [　]  
{ /* 0x8846, */ 0x0000 },	// 8846 ------ [　]  
{ /* 0x8847, */ 0x0000 },	// 8847 ------ [　]  
{ /* 0x8848, */ 0x0000 },	// 8848 ------ [　]  
{ /* 0x8849, */ 0x0000 },	// 8849 ------ [　]  
{ /* 0x884A, */ 0x0000 },	// 884A ------ [　]  
{ /* 0x884B, */ 0x0000 },	// 884B ------ [　]  
{ /* 0x884C, */ 0x0000 },	// 884C ------ [　]  
{ /* 0x884D, */ 0x0000 },	// 884D ------ [　]  
{ /* 0x884E, */ 0x0000 },	// 884E ------ [　]  
{ /* 0x884F, */ 0x0000 },	// 884F ------ [　]  
{ /* 0x8850, */ 0x0000 },	// 8850 ------ [　]  
{ /* 0x8851, */ 0x0000 },	// 8851 ------ [　]  
{ /* 0x8852, */ 0x0000 },	// 8852 ------ [　]  
{ /* 0x8853, */ 0x0000 },	// 8853 ------ [　]  
{ /* 0x8854, */ 0x0000 },	// 8854 ------ [　]  
{ /* 0x8855, */ 0x0000 },	// 8855 ------ [　]  
{ /* 0x8856, */ 0x0000 },	// 8856 ------ [　]  
{ /* 0x8857, */ 0x0000 },	// 8857 ------ [　]  
{ /* 0x8858, */ 0x0000 },	// 8858 ------ [　]  
{ /* 0x8859, */ 0x0000 },	// 8859 ------ [　]  
{ /* 0x885A, */ 0x0000 },	// 885A ------ [　]  
{ /* 0x885B, */ 0x0000 },	// 885B ------ [　]  
{ /* 0x885C, */ 0x0000 },	// 885C ------ [　]  
{ /* 0x885D, */ 0x0000 },	// 885D ------ [　]  
{ /* 0x885E, */ 0x0000 },	// 885E ------ [　]  
{ /* 0x885F, */ 0x0000 },	// 885F ------ [　]  
{ /* 0x8860, */ 0x0000 },	// 8860 ------ [　]  
{ /* 0x8861, */ 0x0000 },	// 8861 ------ [　]  
{ /* 0x8862, */ 0x0000 },	// 8862 ------ [　]  
{ /* 0x8863, */ 0x0000 },	// 8863 ------ [　]  
{ /* 0x8864, */ 0x0000 },	// 8864 ------ [　]  
{ /* 0x8865, */ 0x0000 },	// 8865 ------ [　]  
{ /* 0x8866, */ 0x0000 },	// 8866 ------ [　]  
{ /* 0x8867, */ 0x0000 },	// 8867 ------ [　]  
{ /* 0x8868, */ 0x0000 },	// 8868 ------ [　]  
{ /* 0x8869, */ 0x0000 },	// 8869 ------ [　]  
{ /* 0x886A, */ 0x0000 },	// 886A ------ [　]  
{ /* 0x886B, */ 0x0000 },	// 886B ------ [　]  
{ /* 0x886C, */ 0x0000 },	// 886C ------ [　]  
{ /* 0x886D, */ 0x0000 },	// 886D ------ [　]  
{ /* 0x886E, */ 0x0000 },	// 886E ------ [　]  
{ /* 0x886F, */ 0x0000 },	// 886F ------ [　]  
{ /* 0x8870, */ 0x0000 },	// 8870 ------ [　]  
{ /* 0x8871, */ 0x0000 },	// 8871 ------ [　]  
{ /* 0x8872, */ 0x0000 },	// 8872 ------ [　]  
{ /* 0x8873, */ 0x0000 },	// 8873 ------ [　]  
{ /* 0x8874, */ 0x0000 },	// 8874 ------ [　]  
{ /* 0x8875, */ 0x0000 },	// 8875 ------ [　]  
{ /* 0x8876, */ 0x0000 },	// 8876 ------ [　]  
{ /* 0x8877, */ 0x0000 },	// 8877 ------ [　]  
{ /* 0x8878, */ 0x0000 },	// 8878 ------ [　]  
{ /* 0x8879, */ 0x0000 },	// 8879 ------ [　]  
{ /* 0x887A, */ 0x0000 },	// 887A ------ [　]  
{ /* 0x887B, */ 0x0000 },	// 887B ------ [　]  
{ /* 0x887C, */ 0x0000 },	// 887C ------ [　]  
{ /* 0x887D, */ 0x0000 },	// 887D ------ [　]  
{ /* 0x887E, */ 0x0000 },	// 887E ------ [　]  
{ /* 0x887F, */ 0x0000 },	// 		　#ダミー
{ /* 0x8880, */ 0x0000 },	// 8880 ------ [　]  
{ /* 0x8881, */ 0x0000 },	// 8881 ------ [　]  
{ /* 0x8882, */ 0x0000 },	// 8882 ------ [　]  
{ /* 0x8883, */ 0x0000 },	// 8883 ------ [　]  
{ /* 0x8884, */ 0x0000 },	// 8884 ------ [　]  
{ /* 0x8885, */ 0x0000 },	// 8885 ------ [　]  
{ /* 0x8886, */ 0x0000 },	// 8886 ------ [　]  
{ /* 0x8887, */ 0x0000 },	// 8887 ------ [　]  
{ /* 0x8888, */ 0x0000 },	// 8888 ------ [　]  
{ /* 0x8889, */ 0x0000 },	// 8889 ------ [　]  
{ /* 0x888A, */ 0x0000 },	// 888A ------ [　]  
{ /* 0x888B, */ 0x0000 },	// 888B ------ [　]  
{ /* 0x888C, */ 0x0000 },	// 888C ------ [　]  
{ /* 0x888D, */ 0x0000 },	// 888D ------ [　]  
{ /* 0x888E, */ 0x0000 },	// 888E ------ [　]  
{ /* 0x888F, */ 0x0000 },	// 888F ------ [　]  
{ /* 0x8890, */ 0x0000 },	// 8890 ------ [　]  
{ /* 0x8891, */ 0x0000 },	// 8891 ------ [　]  
{ /* 0x8892, */ 0x0000 },	// 8892 ------ [　]  
{ /* 0x8893, */ 0x0000 },	// 8893 ------ [　]  
{ /* 0x8894, */ 0x0000 },	// 8894 ------ [　]  
{ /* 0x8895, */ 0x0000 },	// 8895 ------ [　]  
{ /* 0x8896, */ 0x0000 },	// 8896 ------ [　]  
{ /* 0x8897, */ 0x0000 },	// 8897 ------ [　]  
{ /* 0x8898, */ 0x0000 },	// 8898 ------ [　]  
{ /* 0x8899, */ 0x0000 },	// 8899 ------ [　]  
{ /* 0x889A, */ 0x0000 },	// 889A ------ [　]  
{ /* 0x889B, */ 0x0000 },	// 889B ------ [　]  
{ /* 0x889C, */ 0x0000 },	// 889C ------ [　]  
{ /* 0x889D, */ 0x0000 },	// 889D ------ [　]  
{ /* 0x889E, */ 0x0000 },	// 889E ------ [　]  
{ /* 0x889F, */ 0x4E9C },	// 889F E4BA9C [亜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88A0, */ 0x5516 },	// 88A0 E59496 [唖]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88A1, */ 0x5A03 },	// 88A1 E5A883 [娃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88A2, */ 0x963F },	// 88A2 E998BF [阿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88A3, */ 0x54C0 },	// 88A3 E59380 [哀]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88A4, */ 0x611B },	// 88A4 E6849B [愛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88A5, */ 0x6328 },	// 88A5 E68CA8 [挨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88A6, */ 0x59F6 },	// 88A6 E5A7B6 [姶]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88A7, */ 0x9022 },	// 88A7 E980A2 [逢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88A8, */ 0x8475 },	// 88A8 E891B5 [葵]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88A9, */ 0x831C },	// 88A9 E88C9C [茜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88AA, */ 0x7A50 },	// 88AA E7A990 [穐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88AB, */ 0x60AA },	// 88AB E682AA [悪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88AC, */ 0x63E1 },	// 88AC E68FA1 [握]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88AD, */ 0x6E25 },	// 88AD E6B8A5 [渥]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88AE, */ 0x65ED },	// 88AE E697AD [旭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88AF, */ 0x8466 },	// 88AF E891A6 [葦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88B0, */ 0x82A6 },	// 88B0 E88AA6 [芦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88B1, */ 0x9BF5 },	// 88B1 E9AFB5 [鯵]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88B2, */ 0x6893 },	// 88B2 E6A293 [梓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88B3, */ 0x5727 },	// 88B3 E59CA7 [圧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88B4, */ 0x65A1 },	// 88B4 E696A1 [斡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88B5, */ 0x6271 },	// 88B5 E689B1 [扱]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88B6, */ 0x5B9B },	// 88B6 E5AE9B [宛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88B7, */ 0x59D0 },	// 88B7 E5A790 [姐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88B8, */ 0x867B },	// 88B8 E899BB [虻]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88B9, */ 0x98F4 },	// 88B9 E9A3B4 [飴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88BA, */ 0x7D62 },	// 88BA E7B5A2 [絢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88BB, */ 0x7DBE },	// 88BB E7B6BE [綾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88BC, */ 0x9B8E },	// 88BC E9AE8E [鮎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88BD, */ 0x6216 },	// 88BD E68896 [或]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88BE, */ 0x7C9F },	// 88BE E7B29F [粟]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88BF, */ 0x88B7 },	// 88BF E8A2B7 [袷]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88C0, */ 0x5B89 },	// 88C0 E5AE89 [安]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88C1, */ 0x5EB5 },	// 88C1 E5BAB5 [庵]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88C2, */ 0x6309 },	// 88C2 E68C89 [按]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88C3, */ 0x6697 },	// 88C3 E69A97 [暗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88C4, */ 0x6848 },	// 88C4 E6A188 [案]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88C5, */ 0x95C7 },	// 88C5 E99787 [闇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88C6, */ 0x978D },	// 88C6 E99E8D [鞍]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88C7, */ 0x674F },	// 88C7 E69D8F [杏]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88C8, */ 0x4EE5 },	// 88C8 E4BBA5 [以]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88C9, */ 0x4F0A },	// 88C9 E4BC8A [伊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88CA, */ 0x4F4D },	// 88CA E4BD8D [位]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88CB, */ 0x4F9D },	// 88CB E4BE9D [依]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88CC, */ 0x5049 },	// 88CC E58189 [偉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88CD, */ 0x56F2 },	// 88CD E59BB2 [囲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88CE, */ 0x5937 },	// 88CE E5A4B7 [夷]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88CF, */ 0x59D4 },	// 88CF E5A794 [委]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88D0, */ 0x5A01 },	// 88D0 E5A881 [威]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88D1, */ 0x5C09 },	// 88D1 E5B089 [尉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88D2, */ 0x60DF },	// 88D2 E6839F [惟]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88D3, */ 0x610F },	// 88D3 E6848F [意]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88D4, */ 0x6170 },	// 88D4 E685B0 [慰]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88D5, */ 0x6613 },	// 88D5 E69893 [易]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88D6, */ 0x6905 },	// 88D6 E6A485 [椅]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88D7, */ 0x70BA },	// 88D7 E782BA [為]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88D8, */ 0x754F },	// 88D8 E7958F [畏]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88D9, */ 0x7570 },	// 88D9 E795B0 [異]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88DA, */ 0x79FB },	// 88DA E7A7BB [移]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88DB, */ 0x7DAD },	// 88DB E7B6AD [維]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88DC, */ 0x7DEF },	// 88DC E7B7AF [緯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88DD, */ 0x80C3 },	// 88DD E88383 [胃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88DE, */ 0x840E },	// 88DE E8908E [萎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88DF, */ 0x8863 },	// 88DF E8A1A3 [衣]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88E0, */ 0x8B02 },	// 88E0 E8AC82 [謂]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88E1, */ 0x9055 },	// 88E1 E98195 [違]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88E2, */ 0x907A },	// 88E2 E981BA [遺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88E3, */ 0x533B },	// 88E3 E58CBB [医]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88E4, */ 0x4E95 },	// 88E4 E4BA95 [井]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88E5, */ 0x4EA5 },	// 88E5 E4BAA5 [亥]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88E6, */ 0x57DF },	// 88E6 E59F9F [域]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88E7, */ 0x80B2 },	// 88E7 E882B2 [育]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88E8, */ 0x90C1 },	// 88E8 E98381 [郁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88E9, */ 0x78EF },	// 88E9 E7A3AF [磯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88EA, */ 0x4E00 },	// 88EA E4B880 [一]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88EB, */ 0x58F1 },	// 88EB E5A3B1 [壱]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88EC, */ 0x6EA2 },	// 88EC E6BAA2 [溢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88ED, */ 0x9038 },	// 88ED E980B8 [逸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88EE, */ 0x7A32 },	// 88EE E7A8B2 [稲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88EF, */ 0x8328 },	// 88EF E88CA8 [茨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88F0, */ 0x828B },	// 88F0 E88A8B [芋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88F1, */ 0x9C2F },	// 88F1 E9B0AF [鰯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88F2, */ 0x5141 },	// 88F2 E58581 [允]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88F3, */ 0x5370 },	// 88F3 E58DB0 [印]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88F4, */ 0x54BD },	// 88F4 E592BD [咽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88F5, */ 0x54E1 },	// 88F5 E593A1 [員]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88F6, */ 0x56E0 },	// 88F6 E59BA0 [因]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88F7, */ 0x59FB },	// 88F7 E5A7BB [姻]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88F8, */ 0x5F15 },	// 88F8 E5BC95 [引]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88F9, */ 0x98F2 },	// 88F9 E9A3B2 [飲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88FA, */ 0x6DEB },	// 88FA E6B7AB [淫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88FB, */ 0x80E4 },	// 88FB E883A4 [胤]  #CJK UNIFIED IDEOGRAPH
{ /* 0x88FC, */ 0x852D },	// 88FC E894AD [蔭]  #CJK UNIFIED IDEOGRAPH
//===== 0x89 40～FC ( テーブル数 [189]  ）====================
{ /* 0x8940, */ 0x9662 },	// 8940 E999A2 [院]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8941, */ 0x9670 },	// 8941 E999B0 [陰]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8942, */ 0x96A0 },	// 8942 E99AA0 [隠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8943, */ 0x97FB },	// 8943 E99FBB [韻]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8944, */ 0x540B },	// 8944 E5908B [吋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8945, */ 0x53F3 },	// 8945 E58FB3 [右]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8946, */ 0x5B87 },	// 8946 E5AE87 [宇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8947, */ 0x70CF },	// 8947 E7838F [烏]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8948, */ 0x7FBD },	// 8948 E7BEBD [羽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8949, */ 0x8FC2 },	// 8949 E8BF82 [迂]  #CJK UNIFIED IDEOGRAPH
{ /* 0x894A, */ 0x96E8 },	// 894A E99BA8 [雨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x894B, */ 0x536F },	// 894B E58DAF [卯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x894C, */ 0x9D5C },	// 894C E9B59C [鵜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x894D, */ 0x7ABA },	// 894D E7AABA [窺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x894E, */ 0x4E11 },	// 894E E4B891 [丑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x894F, */ 0x7893 },	// 894F E7A293 [碓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8950, */ 0x81FC },	// 8950 E887BC [臼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8951, */ 0x6E26 },	// 8951 E6B8A6 [渦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8952, */ 0x5618 },	// 8952 E59898 [嘘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8953, */ 0x5504 },	// 8953 E59484 [唄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8954, */ 0x6B1D },	// 8954 E6AC9D [欝]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8955, */ 0x851A },	// 8955 E8949A [蔚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8956, */ 0x9C3B },	// 8956 E9B0BB [鰻]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8957, */ 0x59E5 },	// 8957 E5A7A5 [姥]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8958, */ 0x53A9 },	// 8958 E58EA9 [厩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8959, */ 0x6D66 },	// 8959 E6B5A6 [浦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x895A, */ 0x74DC },	// 895A E7939C [瓜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x895B, */ 0x958F },	// 895B E9968F [閏]  #CJK UNIFIED IDEOGRAPH
{ /* 0x895C, */ 0x5642 },	// 895C E59982 [噂]  #CJK UNIFIED IDEOGRAPH
{ /* 0x895D, */ 0x4E91 },	// 895D E4BA91 [云]  #CJK UNIFIED IDEOGRAPH
{ /* 0x895E, */ 0x904B },	// 895E E9818B [運]  #CJK UNIFIED IDEOGRAPH
{ /* 0x895F, */ 0x96F2 },	// 895F E99BB2 [雲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8960, */ 0x834F },	// 8960 E88D8F [荏]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8961, */ 0x990C },	// 8961 E9A48C [餌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8962, */ 0x53E1 },	// 8962 E58FA1 [叡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8963, */ 0x55B6 },	// 8963 E596B6 [営]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8964, */ 0x5B30 },	// 8964 E5ACB0 [嬰]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8965, */ 0x5F71 },	// 8965 E5BDB1 [影]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8966, */ 0x6620 },	// 8966 E698A0 [映]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8967, */ 0x66F3 },	// 8967 E69BB3 [曳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8968, */ 0x6804 },	// 8968 E6A084 [栄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8969, */ 0x6C38 },	// 8969 E6B0B8 [永]  #CJK UNIFIED IDEOGRAPH
{ /* 0x896A, */ 0x6CF3 },	// 896A E6B3B3 [泳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x896B, */ 0x6D29 },	// 896B E6B4A9 [洩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x896C, */ 0x745B },	// 896C E7919B [瑛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x896D, */ 0x76C8 },	// 896D E79B88 [盈]  #CJK UNIFIED IDEOGRAPH
{ /* 0x896E, */ 0x7A4E },	// 896E E7A98E [穎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x896F, */ 0x9834 },	// 896F E9A0B4 [頴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8970, */ 0x82F1 },	// 8970 E88BB1 [英]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8971, */ 0x885B },	// 8971 E8A19B [衛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8972, */ 0x8A60 },	// 8972 E8A9A0 [詠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8973, */ 0x92ED },	// 8973 E98BAD [鋭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8974, */ 0x6DB2 },	// 8974 E6B6B2 [液]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8975, */ 0x75AB },	// 8975 E796AB [疫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8976, */ 0x76CA },	// 8976 E79B8A [益]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8977, */ 0x99C5 },	// 8977 E9A785 [駅]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8978, */ 0x60A6 },	// 8978 E682A6 [悦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8979, */ 0x8B01 },	// 8979 E8AC81 [謁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x897A, */ 0x8D8A },	// 897A E8B68A [越]  #CJK UNIFIED IDEOGRAPH
{ /* 0x897B, */ 0x95B2 },	// 897B E996B2 [閲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x897C, */ 0x698E },	// 897C E6A68E [榎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x897D, */ 0x53AD },	// 897D E58EAD [厭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x897E, */ 0x5186 },	// 897E E58686 [円]  #CJK UNIFIED IDEOGRAPH
{ /* 0x897F, */ 0x0000 },	// 		　#ダミー
{ /* 0x8980, */ 0x5712 },	// 8980 E59C92 [園]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8981, */ 0x5830 },	// 8981 E5A0B0 [堰]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8982, */ 0x5944 },	// 8982 E5A584 [奄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8983, */ 0x5BB4 },	// 8983 E5AEB4 [宴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8984, */ 0x5EF6 },	// 8984 E5BBB6 [延]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8985, */ 0x6028 },	// 8985 E680A8 [怨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8986, */ 0x63A9 },	// 8986 E68EA9 [掩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8987, */ 0x63F4 },	// 8987 E68FB4 [援]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8988, */ 0x6CBF },	// 8988 E6B2BF [沿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8989, */ 0x6F14 },	// 8989 E6BC94 [演]  #CJK UNIFIED IDEOGRAPH
{ /* 0x898A, */ 0x708E },	// 898A E7828E [炎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x898B, */ 0x7114 },	// 898B E78494 [焔]  #CJK UNIFIED IDEOGRAPH
{ /* 0x898C, */ 0x7159 },	// 898C E78599 [煙]  #CJK UNIFIED IDEOGRAPH
{ /* 0x898D, */ 0x71D5 },	// 898D E78795 [燕]  #CJK UNIFIED IDEOGRAPH
{ /* 0x898E, */ 0x733F },	// 898E E78CBF [猿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x898F, */ 0x7E01 },	// 898F E7B881 [縁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8990, */ 0x8276 },	// 8990 E889B6 [艶]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8991, */ 0x82D1 },	// 8991 E88B91 [苑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8992, */ 0x8597 },	// 8992 E89697 [薗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8993, */ 0x9060 },	// 8993 E981A0 [遠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8994, */ 0x925B },	// 8994 E9899B [鉛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8995, */ 0x9D1B },	// 8995 E9B49B [鴛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8996, */ 0x5869 },	// 8996 E5A1A9 [塩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8997, */ 0x65BC },	// 8997 E696BC [於]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8998, */ 0x6C5A },	// 8998 E6B19A [汚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8999, */ 0x7525 },	// 8999 E794A5 [甥]  #CJK UNIFIED IDEOGRAPH
{ /* 0x899A, */ 0x51F9 },	// 899A E587B9 [凹]  #CJK UNIFIED IDEOGRAPH
{ /* 0x899B, */ 0x592E },	// 899B E5A4AE [央]  #CJK UNIFIED IDEOGRAPH
{ /* 0x899C, */ 0x5965 },	// 899C E5A5A5 [奥]  #CJK UNIFIED IDEOGRAPH
{ /* 0x899D, */ 0x5F80 },	// 899D E5BE80 [往]  #CJK UNIFIED IDEOGRAPH
{ /* 0x899E, */ 0x5FDC },	// 899E E5BF9C [応]  #CJK UNIFIED IDEOGRAPH
{ /* 0x899F, */ 0x62BC },	// 899F E68ABC [押]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89A0, */ 0x65FA },	// 89A0 E697BA [旺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89A1, */ 0x6A2A },	// 89A1 E6A8AA [横]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89A2, */ 0x6B27 },	// 89A2 E6ACA7 [欧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89A3, */ 0x6BB4 },	// 89A3 E6AEB4 [殴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89A4, */ 0x738B },	// 89A4 E78E8B [王]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89A5, */ 0x7FC1 },	// 89A5 E7BF81 [翁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89A6, */ 0x8956 },	// 89A6 E8A596 [襖]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89A7, */ 0x9D2C },	// 89A7 E9B4AC [鴬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89A8, */ 0x9D0E },	// 89A8 E9B48E [鴎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89A9, */ 0x9EC4 },	// 89A9 E9BB84 [黄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89AA, */ 0x5CA1 },	// 89AA E5B2A1 [岡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89AB, */ 0x6C96 },	// 89AB E6B296 [沖]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89AC, */ 0x837B },	// 89AC E88DBB [荻]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89AD, */ 0x5104 },	// 89AD E58484 [億]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89AE, */ 0x5C4B },	// 89AE E5B18B [屋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89AF, */ 0x61B6 },	// 89AF E686B6 [憶]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89B0, */ 0x81C6 },	// 89B0 E88786 [臆]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89B1, */ 0x6876 },	// 89B1 E6A1B6 [桶]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89B2, */ 0x7261 },	// 89B2 E789A1 [牡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89B3, */ 0x4E59 },	// 89B3 E4B999 [乙]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89B4, */ 0x4FFA },	// 89B4 E4BFBA [俺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89B5, */ 0x5378 },	// 89B5 E58DB8 [卸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89B6, */ 0x6069 },	// 89B6 E681A9 [恩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89B7, */ 0x6E29 },	// 89B7 E6B8A9 [温]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89B8, */ 0x7A4F },	// 89B8 E7A98F [穏]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89B9, */ 0x97F3 },	// 89B9 E99FB3 [音]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89BA, */ 0x4E0B },	// 89BA E4B88B [下]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89BB, */ 0x5316 },	// 89BB E58C96 [化]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89BC, */ 0x4EEE },	// 89BC E4BBAE [仮]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89BD, */ 0x4F55 },	// 89BD E4BD95 [何]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89BE, */ 0x4F3D },	// 89BE E4BCBD [伽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89BF, */ 0x4FA1 },	// 89BF E4BEA1 [価]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89C0, */ 0x4F73 },	// 89C0 E4BDB3 [佳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89C1, */ 0x52A0 },	// 89C1 E58AA0 [加]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89C2, */ 0x53EF },	// 89C2 E58FAF [可]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89C3, */ 0x5609 },	// 89C3 E59889 [嘉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89C4, */ 0x590F },	// 89C4 E5A48F [夏]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89C5, */ 0x5AC1 },	// 89C5 E5AB81 [嫁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89C6, */ 0x5BB6 },	// 89C6 E5AEB6 [家]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89C7, */ 0x5BE1 },	// 89C7 E5AFA1 [寡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89C8, */ 0x79D1 },	// 89C8 E7A791 [科]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89C9, */ 0x6687 },	// 89C9 E69A87 [暇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89CA, */ 0x679C },	// 89CA E69E9C [果]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89CB, */ 0x67B6 },	// 89CB E69EB6 [架]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89CC, */ 0x6B4C },	// 89CC E6AD8C [歌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89CD, */ 0x6CB3 },	// 89CD E6B2B3 [河]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89CE, */ 0x706B },	// 89CE E781AB [火]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89CF, */ 0x73C2 },	// 89CF E78F82 [珂]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89D0, */ 0x798D },	// 89D0 E7A68D [禍]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89D1, */ 0x79BE },	// 89D1 E7A6BE [禾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89D2, */ 0x7A3C },	// 89D2 E7A8BC [稼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89D3, */ 0x7B87 },	// 89D3 E7AE87 [箇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89D4, */ 0x82B1 },	// 89D4 E88AB1 [花]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89D5, */ 0x82DB },	// 89D5 E88B9B [苛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89D6, */ 0x8304 },	// 89D6 E88C84 [茄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89D7, */ 0x8377 },	// 89D7 E88DB7 [荷]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89D8, */ 0x83EF },	// 89D8 E88FAF [華]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89D9, */ 0x83D3 },	// 89D9 E88F93 [菓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89DA, */ 0x8766 },	// 89DA E89DA6 [蝦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89DB, */ 0x8AB2 },	// 89DB E8AAB2 [課]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89DC, */ 0x5629 },	// 89DC E598A9 [嘩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89DD, */ 0x8CA8 },	// 89DD E8B2A8 [貨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89DE, */ 0x8FE6 },	// 89DE E8BFA6 [迦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89DF, */ 0x904E },	// 89DF E9818E [過]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89E0, */ 0x971E },	// 89E0 E99C9E [霞]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89E1, */ 0x868A },	// 89E1 E89A8A [蚊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89E2, */ 0x4FC4 },	// 89E2 E4BF84 [俄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89E3, */ 0x5CE8 },	// 89E3 E5B3A8 [峨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89E4, */ 0x6211 },	// 89E4 E68891 [我]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89E5, */ 0x7259 },	// 89E5 E78999 [牙]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89E6, */ 0x753B },	// 89E6 E794BB [画]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89E7, */ 0x81E5 },	// 89E7 E887A5 [臥]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89E8, */ 0x82BD },	// 89E8 E88ABD [芽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89E9, */ 0x86FE },	// 89E9 E89BBE [蛾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89EA, */ 0x8CC0 },	// 89EA E8B380 [賀]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89EB, */ 0x96C5 },	// 89EB E99B85 [雅]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89EC, */ 0x9913 },	// 89EC E9A493 [餓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89ED, */ 0x99D5 },	// 89ED E9A795 [駕]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89EE, */ 0x4ECB },	// 89EE E4BB8B [介]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89EF, */ 0x4F1A },	// 89EF E4BC9A [会]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89F0, */ 0x89E3 },	// 89F0 E8A7A3 [解]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89F1, */ 0x56DE },	// 89F1 E59B9E [回]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89F2, */ 0x584A },	// 89F2 E5A18A [塊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89F3, */ 0x58CA },	// 89F3 E5A38A [壊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89F4, */ 0x5EFB },	// 89F4 E5BBBB [廻]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89F5, */ 0x5FEB },	// 89F5 E5BFAB [快]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89F6, */ 0x602A },	// 89F6 E680AA [怪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89F7, */ 0x6094 },	// 89F7 E68294 [悔]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89F8, */ 0x6062 },	// 89F8 E681A2 [恢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89F9, */ 0x61D0 },	// 89F9 E68790 [懐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89FA, */ 0x6212 },	// 89FA E68892 [戒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89FB, */ 0x62D0 },	// 89FB E68B90 [拐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x89FC, */ 0x6539 },	// 89FC E694B9 [改]  #CJK UNIFIED IDEOGRAPH
//===== 0x8A 40～FC ( テーブル数 [189]  ）====================
{ /* 0x8A40, */ 0x9B41 },	// 8A40 E9AD81 [魁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A41, */ 0x6666 },	// 8A41 E699A6 [晦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A42, */ 0x68B0 },	// 8A42 E6A2B0 [械]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A43, */ 0x6D77 },	// 8A43 E6B5B7 [海]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A44, */ 0x7070 },	// 8A44 E781B0 [灰]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A45, */ 0x754C },	// 8A45 E7958C [界]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A46, */ 0x7686 },	// 8A46 E79A86 [皆]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A47, */ 0x7D75 },	// 8A47 E7B5B5 [絵]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A48, */ 0x82A5 },	// 8A48 E88AA5 [芥]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A49, */ 0x87F9 },	// 8A49 E89FB9 [蟹]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A4A, */ 0x958B },	// 8A4A E9968B [開]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A4B, */ 0x968E },	// 8A4B E99A8E [階]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A4C, */ 0x8C9D },	// 8A4C E8B29D [貝]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A4D, */ 0x51F1 },	// 8A4D E587B1 [凱]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A4E, */ 0x52BE },	// 8A4E E58ABE [劾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A4F, */ 0x5916 },	// 8A4F E5A496 [外]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A50, */ 0x54B3 },	// 8A50 E592B3 [咳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A51, */ 0x5BB3 },	// 8A51 E5AEB3 [害]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A52, */ 0x5D16 },	// 8A52 E5B496 [崖]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A53, */ 0x6168 },	// 8A53 E685A8 [慨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A54, */ 0x6982 },	// 8A54 E6A682 [概]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A55, */ 0x6DAF },	// 8A55 E6B6AF [涯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A56, */ 0x788D },	// 8A56 E7A28D [碍]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A57, */ 0x84CB },	// 8A57 E8938B [蓋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A58, */ 0x8857 },	// 8A58 E8A197 [街]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A59, */ 0x8A72 },	// 8A59 E8A9B2 [該]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A5A, */ 0x93A7 },	// 8A5A E98EA7 [鎧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A5B, */ 0x9AB8 },	// 8A5B E9AAB8 [骸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A5C, */ 0x6D6C },	// 8A5C E6B5AC [浬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A5D, */ 0x99A8 },	// 8A5D E9A6A8 [馨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A5E, */ 0x86D9 },	// 8A5E E89B99 [蛙]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A5F, */ 0x57A3 },	// 8A5F E59EA3 [垣]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A60, */ 0x67FF },	// 8A60 E69FBF [柿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A61, */ 0x86CE },	// 8A61 E89B8E [蛎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A62, */ 0x920E },	// 8A62 E9888E [鈎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A63, */ 0x5283 },	// 8A63 E58A83 [劃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A64, */ 0x5687 },	// 8A64 E59A87 [嚇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A65, */ 0x5404 },	// 8A65 E59084 [各]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A66, */ 0x5ED3 },	// 8A66 E5BB93 [廓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A67, */ 0x62E1 },	// 8A67 E68BA1 [拡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A68, */ 0x64B9 },	// 8A68 E692B9 [撹]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A69, */ 0x683C },	// 8A69 E6A0BC [格]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A6A, */ 0x6838 },	// 8A6A E6A0B8 [核]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A6B, */ 0x6BBB },	// 8A6B E6AEBB [殻]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A6C, */ 0x7372 },	// 8A6C E78DB2 [獲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A6D, */ 0x78BA },	// 8A6D E7A2BA [確]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A6E, */ 0x7A6B },	// 8A6E E7A9AB [穫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A6F, */ 0x899A },	// 8A6F E8A69A [覚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A70, */ 0x89D2 },	// 8A70 E8A792 [角]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A71, */ 0x8D6B },	// 8A71 E8B5AB [赫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A72, */ 0x8F03 },	// 8A72 E8BC83 [較]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A73, */ 0x90ED },	// 8A73 E983AD [郭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A74, */ 0x95A3 },	// 8A74 E996A3 [閣]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A75, */ 0x9694 },	// 8A75 E99A94 [隔]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A76, */ 0x9769 },	// 8A76 E99DA9 [革]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A77, */ 0x5B66 },	// 8A77 E5ADA6 [学]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A78, */ 0x5CB3 },	// 8A78 E5B2B3 [岳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A79, */ 0x697D },	// 8A79 E6A5BD [楽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A7A, */ 0x984D },	// 8A7A E9A18D [額]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A7B, */ 0x984E },	// 8A7B E9A18E [顎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A7C, */ 0x639B },	// 8A7C E68E9B [掛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A7D, */ 0x7B20 },	// 8A7D E7ACA0 [笠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A7E, */ 0x6A2B },	// 8A7E E6A8AB [樫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A7F, */ 0x0000 },	// 		　#ダミー
{ /* 0x8A80, */ 0x6A7F },	// 8A80 E6A9BF [橿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A81, */ 0x68B6 },	// 8A81 E6A2B6 [梶]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A82, */ 0x9C0D },	// 8A82 E9B08D [鰍]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A83, */ 0x6F5F },	// 8A83 E6BD9F [潟]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A84, */ 0x5272 },	// 8A84 E589B2 [割]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A85, */ 0x559D },	// 8A85 E5969D [喝]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A86, */ 0x6070 },	// 8A86 E681B0 [恰]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A87, */ 0x62EC },	// 8A87 E68BAC [括]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A88, */ 0x6D3B },	// 8A88 E6B4BB [活]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A89, */ 0x6E07 },	// 8A89 E6B887 [渇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A8A, */ 0x6ED1 },	// 8A8A E6BB91 [滑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A8B, */ 0x845B },	// 8A8B E8919B [葛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A8C, */ 0x8910 },	// 8A8C E8A490 [褐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A8D, */ 0x8F44 },	// 8A8D E8BD84 [轄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A8E, */ 0x4E14 },	// 8A8E E4B894 [且]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A8F, */ 0x9C39 },	// 8A8F E9B0B9 [鰹]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A90, */ 0x53F6 },	// 8A90 E58FB6 [叶]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A91, */ 0x691B },	// 8A91 E6A49B [椛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A92, */ 0x6A3A },	// 8A92 E6A8BA [樺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A93, */ 0x9784 },	// 8A93 E99E84 [鞄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A94, */ 0x682A },	// 8A94 E6A0AA [株]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A95, */ 0x515C },	// 8A95 E5859C [兜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A96, */ 0x7AC3 },	// 8A96 E7AB83 [竃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A97, */ 0x84B2 },	// 8A97 E892B2 [蒲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A98, */ 0x91DC },	// 8A98 E9879C [釜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A99, */ 0x938C },	// 8A99 E98E8C [鎌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A9A, */ 0x565B },	// 8A9A E5999B [噛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A9B, */ 0x9D28 },	// 8A9B E9B4A8 [鴨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A9C, */ 0x6822 },	// 8A9C E6A0A2 [栢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A9D, */ 0x8305 },	// 8A9D E88C85 [茅]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A9E, */ 0x8431 },	// 8A9E E890B1 [萱]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8A9F, */ 0x7CA5 },	// 8A9F E7B2A5 [粥]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AA0, */ 0x5208 },	// 8AA0 E58888 [刈]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AA1, */ 0x82C5 },	// 8AA1 E88B85 [苅]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AA2, */ 0x74E6 },	// 8AA2 E793A6 [瓦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AA3, */ 0x4E7E },	// 8AA3 E4B9BE [乾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AA4, */ 0x4F83 },	// 8AA4 E4BE83 [侃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AA5, */ 0x51A0 },	// 8AA5 E586A0 [冠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AA6, */ 0x5BD2 },	// 8AA6 E5AF92 [寒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AA7, */ 0x520A },	// 8AA7 E5888A [刊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AA8, */ 0x52D8 },	// 8AA8 E58B98 [勘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AA9, */ 0x52E7 },	// 8AA9 E58BA7 [勧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AAA, */ 0x5DFB },	// 8AAA E5B7BB [巻]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AAB, */ 0x559A },	// 8AAB E5969A [喚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AAC, */ 0x582A },	// 8AAC E5A0AA [堪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AAD, */ 0x59E6 },	// 8AAD E5A7A6 [姦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AAE, */ 0x5B8C },	// 8AAE E5AE8C [完]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AAF, */ 0x5B98 },	// 8AAF E5AE98 [官]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AB0, */ 0x5BDB },	// 8AB0 E5AF9B [寛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AB1, */ 0x5E72 },	// 8AB1 E5B9B2 [干]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AB2, */ 0x5E79 },	// 8AB2 E5B9B9 [幹]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AB3, */ 0x60A3 },	// 8AB3 E682A3 [患]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AB4, */ 0x611F },	// 8AB4 E6849F [感]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AB5, */ 0x6163 },	// 8AB5 E685A3 [慣]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AB6, */ 0x61BE },	// 8AB6 E686BE [憾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AB7, */ 0x63DB },	// 8AB7 E68F9B [換]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AB8, */ 0x6562 },	// 8AB8 E695A2 [敢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AB9, */ 0x67D1 },	// 8AB9 E69F91 [柑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ABA, */ 0x6853 },	// 8ABA E6A193 [桓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ABB, */ 0x68FA },	// 8ABB E6A3BA [棺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ABC, */ 0x6B3E },	// 8ABC E6ACBE [款]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ABD, */ 0x6B53 },	// 8ABD E6AD93 [歓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ABE, */ 0x6C57 },	// 8ABE E6B197 [汗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ABF, */ 0x6F22 },	// 8ABF E6BCA2 [漢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AC0, */ 0x6F97 },	// 8AC0 E6BE97 [澗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AC1, */ 0x6F45 },	// 8AC1 E6BD85 [潅]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AC2, */ 0x74B0 },	// 8AC2 E792B0 [環]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AC3, */ 0x7518 },	// 8AC3 E79498 [甘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AC4, */ 0x76E3 },	// 8AC4 E79BA3 [監]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AC5, */ 0x770B },	// 8AC5 E79C8B [看]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AC6, */ 0x7AFF },	// 8AC6 E7ABBF [竿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AC7, */ 0x7BA1 },	// 8AC7 E7AEA1 [管]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AC8, */ 0x7C21 },	// 8AC8 E7B0A1 [簡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AC9, */ 0x7DE9 },	// 8AC9 E7B7A9 [緩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ACA, */ 0x7F36 },	// 8ACA E7BCB6 [缶]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ACB, */ 0x7FF0 },	// 8ACB E7BFB0 [翰]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ACC, */ 0x809D },	// 8ACC E8829D [肝]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ACD, */ 0x8266 },	// 8ACD E889A6 [艦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ACE, */ 0x839E },	// 8ACE E88E9E [莞]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ACF, */ 0x89B3 },	// 8ACF E8A6B3 [観]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AD0, */ 0x8ACC },	// 8AD0 E8AB8C [諌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AD1, */ 0x8CAB },	// 8AD1 E8B2AB [貫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AD2, */ 0x9084 },	// 8AD2 E98284 [還]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AD3, */ 0x9451 },	// 8AD3 E99191 [鑑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AD4, */ 0x9593 },	// 8AD4 E99693 [間]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AD5, */ 0x9591 },	// 8AD5 E99691 [閑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AD6, */ 0x95A2 },	// 8AD6 E996A2 [関]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AD7, */ 0x9665 },	// 8AD7 E999A5 [陥]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AD8, */ 0x97D3 },	// 8AD8 E99F93 [韓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AD9, */ 0x9928 },	// 8AD9 E9A4A8 [館]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ADA, */ 0x8218 },	// 8ADA E88898 [舘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ADB, */ 0x4E38 },	// 8ADB E4B8B8 [丸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ADC, */ 0x542B },	// 8ADC E590AB [含]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ADD, */ 0x5CB8 },	// 8ADD E5B2B8 [岸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ADE, */ 0x5DCC },	// 8ADE E5B78C [巌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ADF, */ 0x73A9 },	// 8ADF E78EA9 [玩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AE0, */ 0x764C },	// 8AE0 E7998C [癌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AE1, */ 0x773C },	// 8AE1 E79CBC [眼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AE2, */ 0x5CA9 },	// 8AE2 E5B2A9 [岩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AE3, */ 0x7FEB },	// 8AE3 E7BFAB [翫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AE4, */ 0x8D0B },	// 8AE4 E8B48B [贋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AE5, */ 0x96C1 },	// 8AE5 E99B81 [雁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AE6, */ 0x9811 },	// 8AE6 E9A091 [頑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AE7, */ 0x9854 },	// 8AE7 E9A194 [顔]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AE8, */ 0x9858 },	// 8AE8 E9A198 [願]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AE9, */ 0x4F01 },	// 8AE9 E4BC81 [企]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AEA, */ 0x4F0E },	// 8AEA E4BC8E [伎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AEB, */ 0x5371 },	// 8AEB E58DB1 [危]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AEC, */ 0x559C },	// 8AEC E5969C [喜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AED, */ 0x5668 },	// 8AED E599A8 [器]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AEE, */ 0x57FA },	// 8AEE E59FBA [基]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AEF, */ 0x5947 },	// 8AEF E5A587 [奇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AF0, */ 0x5B09 },	// 8AF0 E5AC89 [嬉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AF1, */ 0x5BC4 },	// 8AF1 E5AF84 [寄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AF2, */ 0x5C90 },	// 8AF2 E5B290 [岐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AF3, */ 0x5E0C },	// 8AF3 E5B88C [希]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AF4, */ 0x5E7E },	// 8AF4 E5B9BE [幾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AF5, */ 0x5FCC },	// 8AF5 E5BF8C [忌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AF6, */ 0x63EE },	// 8AF6 E68FAE [揮]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AF7, */ 0x673A },	// 8AF7 E69CBA [机]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AF8, */ 0x65D7 },	// 8AF8 E69797 [旗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AF9, */ 0x65E2 },	// 8AF9 E697A2 [既]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AFA, */ 0x671F },	// 8AFA E69C9F [期]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AFB, */ 0x68CB },	// 8AFB E6A38B [棋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8AFC, */ 0x68C4 },	// 8AFC E6A384 [棄]  #CJK UNIFIED IDEOGRAPH
//===== 0x8B 40～FC ( テーブル数 [189]  ）====================
{ /* 0x8B40, */ 0x6A5F },	// 8B40 E6A99F [機]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B41, */ 0x5E30 },	// 8B41 E5B8B0 [帰]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B42, */ 0x6BC5 },	// 8B42 E6AF85 [毅]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B43, */ 0x6C17 },	// 8B43 E6B097 [気]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B44, */ 0x6C7D },	// 8B44 E6B1BD [汽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B45, */ 0x757F },	// 8B45 E795BF [畿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B46, */ 0x7948 },	// 8B46 E7A588 [祈]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B47, */ 0x5B63 },	// 8B47 E5ADA3 [季]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B48, */ 0x7A00 },	// 8B48 E7A880 [稀]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B49, */ 0x7D00 },	// 8B49 E7B480 [紀]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B4A, */ 0x5FBD },	// 8B4A E5BEBD [徽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B4B, */ 0x898F },	// 8B4B E8A68F [規]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B4C, */ 0x8A18 },	// 8B4C E8A898 [記]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B4D, */ 0x8CB4 },	// 8B4D E8B2B4 [貴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B4E, */ 0x8D77 },	// 8B4E E8B5B7 [起]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B4F, */ 0x8ECC },	// 8B4F E8BB8C [軌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B50, */ 0x8F1D },	// 8B50 E8BC9D [輝]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B51, */ 0x98E2 },	// 8B51 E9A3A2 [飢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B52, */ 0x9A0E },	// 8B52 E9A88E [騎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B53, */ 0x9B3C },	// 8B53 E9ACBC [鬼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B54, */ 0x4E80 },	// 8B54 E4BA80 [亀]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B55, */ 0x507D },	// 8B55 E581BD [偽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B56, */ 0x5100 },	// 8B56 E58480 [儀]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B57, */ 0x5993 },	// 8B57 E5A693 [妓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B58, */ 0x5B9C },	// 8B58 E5AE9C [宜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B59, */ 0x622F },	// 8B59 E688AF [戯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B5A, */ 0x6280 },	// 8B5A E68A80 [技]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B5B, */ 0x64EC },	// 8B5B E693AC [擬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B5C, */ 0x6B3A },	// 8B5C E6ACBA [欺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B5D, */ 0x72A0 },	// 8B5D E78AA0 [犠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B5E, */ 0x7591 },	// 8B5E E79691 [疑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B5F, */ 0x7947 },	// 8B5F E7A587 [祇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B60, */ 0x7FA9 },	// 8B60 E7BEA9 [義]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B61, */ 0x87FB },	// 8B61 E89FBB [蟻]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B62, */ 0x8ABC },	// 8B62 E8AABC [誼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B63, */ 0x8B70 },	// 8B63 E8ADB0 [議]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B64, */ 0x63AC },	// 8B64 E68EAC [掬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B65, */ 0x83CA },	// 8B65 E88F8A [菊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B66, */ 0x97A0 },	// 8B66 E99EA0 [鞠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B67, */ 0x5409 },	// 8B67 E59089 [吉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B68, */ 0x5403 },	// 8B68 E59083 [吃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B69, */ 0x55AB },	// 8B69 E596AB [喫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B6A, */ 0x6854 },	// 8B6A E6A194 [桔]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B6B, */ 0x6A58 },	// 8B6B E6A998 [橘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B6C, */ 0x8A70 },	// 8B6C E8A9B0 [詰]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B6D, */ 0x7827 },	// 8B6D E7A0A7 [砧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B6E, */ 0x6775 },	// 8B6E E69DB5 [杵]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B6F, */ 0x9ECD },	// 8B6F E9BB8D [黍]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B70, */ 0x5374 },	// 8B70 E58DB4 [却]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B71, */ 0x5BA2 },	// 8B71 E5AEA2 [客]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B72, */ 0x811A },	// 8B72 E8849A [脚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B73, */ 0x8650 },	// 8B73 E89990 [虐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B74, */ 0x9006 },	// 8B74 E98086 [逆]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B75, */ 0x4E18 },	// 8B75 E4B898 [丘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B76, */ 0x4E45 },	// 8B76 E4B985 [久]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B77, */ 0x4EC7 },	// 8B77 E4BB87 [仇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B78, */ 0x4F11 },	// 8B78 E4BC91 [休]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B79, */ 0x53CA },	// 8B79 E58F8A [及]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B7A, */ 0x5438 },	// 8B7A E590B8 [吸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B7B, */ 0x5BAE },	// 8B7B E5AEAE [宮]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B7C, */ 0x5F13 },	// 8B7C E5BC93 [弓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B7D, */ 0x6025 },	// 8B7D E680A5 [急]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B7E, */ 0x6551 },	// 8B7E E69591 [救]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B7F, */ 0x0000 },	// 		　#ダミー
{ /* 0x8B80, */ 0x673D },	// 8B80 E69CBD [朽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B81, */ 0x6C42 },	// 8B81 E6B182 [求]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B82, */ 0x6C72 },	// 8B82 E6B1B2 [汲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B83, */ 0x6CE3 },	// 8B83 E6B3A3 [泣]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B84, */ 0x7078 },	// 8B84 E781B8 [灸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B85, */ 0x7403 },	// 8B85 E79083 [球]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B86, */ 0x7A76 },	// 8B86 E7A9B6 [究]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B87, */ 0x7AAE },	// 8B87 E7AAAE [窮]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B88, */ 0x7B08 },	// 8B88 E7AC88 [笈]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B89, */ 0x7D1A },	// 8B89 E7B49A [級]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B8A, */ 0x7CFE },	// 8B8A E7B3BE [糾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B8B, */ 0x7D66 },	// 8B8B E7B5A6 [給]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B8C, */ 0x65E7 },	// 8B8C E697A7 [旧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B8D, */ 0x725B },	// 8B8D E7899B [牛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B8E, */ 0x53BB },	// 8B8E E58EBB [去]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B8F, */ 0x5C45 },	// 8B8F E5B185 [居]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B90, */ 0x5DE8 },	// 8B90 E5B7A8 [巨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B91, */ 0x62D2 },	// 8B91 E68B92 [拒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B92, */ 0x62E0 },	// 8B92 E68BA0 [拠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B93, */ 0x6319 },	// 8B93 E68C99 [挙]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B94, */ 0x6E20 },	// 8B94 E6B8A0 [渠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B95, */ 0x865A },	// 8B95 E8999A [虚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B96, */ 0x8A31 },	// 8B96 E8A8B1 [許]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B97, */ 0x8DDD },	// 8B97 E8B79D [距]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B98, */ 0x92F8 },	// 8B98 E98BB8 [鋸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B99, */ 0x6F01 },	// 8B99 E6BC81 [漁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B9A, */ 0x79A6 },	// 8B9A E7A6A6 [禦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B9B, */ 0x9B5A },	// 8B9B E9AD9A [魚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B9C, */ 0x4EA8 },	// 8B9C E4BAA8 [亨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B9D, */ 0x4EAB },	// 8B9D E4BAAB [享]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B9E, */ 0x4EAC },	// 8B9E E4BAAC [京]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8B9F, */ 0x4F9B },	// 8B9F E4BE9B [供]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BA0, */ 0x4FA0 },	// 8BA0 E4BEA0 [侠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BA1, */ 0x50D1 },	// 8BA1 E58391 [僑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BA2, */ 0x5147 },	// 8BA2 E58587 [兇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BA3, */ 0x7AF6 },	// 8BA3 E7ABB6 [競]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BA4, */ 0x5171 },	// 8BA4 E585B1 [共]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BA5, */ 0x51F6 },	// 8BA5 E587B6 [凶]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BA6, */ 0x5354 },	// 8BA6 E58D94 [協]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BA7, */ 0x5321 },	// 8BA7 E58CA1 [匡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BA8, */ 0x537F },	// 8BA8 E58DBF [卿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BA9, */ 0x53EB },	// 8BA9 E58FAB [叫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BAA, */ 0x55AC },	// 8BAA E596AC [喬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BAB, */ 0x5883 },	// 8BAB E5A283 [境]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BAC, */ 0x5CE1 },	// 8BAC E5B3A1 [峡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BAD, */ 0x5F37 },	// 8BAD E5BCB7 [強]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BAE, */ 0x5F4A },	// 8BAE E5BD8A [彊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BAF, */ 0x602F },	// 8BAF E680AF [怯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BB0, */ 0x6050 },	// 8BB0 E68190 [恐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BB1, */ 0x606D },	// 8BB1 E681AD [恭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BB2, */ 0x631F },	// 8BB2 E68C9F [挟]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BB3, */ 0x6559 },	// 8BB3 E69599 [教]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BB4, */ 0x6A4B },	// 8BB4 E6A98B [橋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BB5, */ 0x6CC1 },	// 8BB5 E6B381 [況]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BB6, */ 0x72C2 },	// 8BB6 E78B82 [狂]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BB7, */ 0x72ED },	// 8BB7 E78BAD [狭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BB8, */ 0x77EF },	// 8BB8 E79FAF [矯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BB9, */ 0x80F8 },	// 8BB9 E883B8 [胸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BBA, */ 0x8105 },	// 8BBA E88485 [脅]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BBB, */ 0x8208 },	// 8BBB E88888 [興]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BBC, */ 0x854E },	// 8BBC E8958E [蕎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BBD, */ 0x90F7 },	// 8BBD E983B7 [郷]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BBE, */ 0x93E1 },	// 8BBE E98FA1 [鏡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BBF, */ 0x97FF },	// 8BBF E99FBF [響]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BC0, */ 0x9957 },	// 8BC0 E9A597 [饗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BC1, */ 0x9A5A },	// 8BC1 E9A99A [驚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BC2, */ 0x4EF0 },	// 8BC2 E4BBB0 [仰]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BC3, */ 0x51DD },	// 8BC3 E5879D [凝]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BC4, */ 0x5C2D },	// 8BC4 E5B0AD [尭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BC5, */ 0x6681 },	// 8BC5 E69A81 [暁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BC6, */ 0x696D },	// 8BC6 E6A5AD [業]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BC7, */ 0x5C40 },	// 8BC7 E5B180 [局]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BC8, */ 0x66F2 },	// 8BC8 E69BB2 [曲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BC9, */ 0x6975 },	// 8BC9 E6A5B5 [極]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BCA, */ 0x7389 },	// 8BCA E78E89 [玉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BCB, */ 0x6850 },	// 8BCB E6A190 [桐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BCC, */ 0x7C81 },	// 8BCC E7B281 [粁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BCD, */ 0x50C5 },	// 8BCD E58385 [僅]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BCE, */ 0x52E4 },	// 8BCE E58BA4 [勤]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BCF, */ 0x5747 },	// 8BCF E59D87 [均]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BD0, */ 0x5DFE },	// 8BD0 E5B7BE [巾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BD1, */ 0x9326 },	// 8BD1 E98CA6 [錦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BD2, */ 0x65A4 },	// 8BD2 E696A4 [斤]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BD3, */ 0x6B23 },	// 8BD3 E6ACA3 [欣]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BD4, */ 0x6B3D },	// 8BD4 E6ACBD [欽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BD5, */ 0x7434 },	// 8BD5 E790B4 [琴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BD6, */ 0x7981 },	// 8BD6 E7A681 [禁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BD7, */ 0x79BD },	// 8BD7 E7A6BD [禽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BD8, */ 0x7B4B },	// 8BD8 E7AD8B [筋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BD9, */ 0x7DCA },	// 8BD9 E7B78A [緊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BDA, */ 0x82B9 },	// 8BDA E88AB9 [芹]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BDB, */ 0x83CC },	// 8BDB E88F8C [菌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BDC, */ 0x887F },	// 8BDC E8A1BF [衿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BDD, */ 0x895F },	// 8BDD E8A59F [襟]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BDE, */ 0x8B39 },	// 8BDE E8ACB9 [謹]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BDF, */ 0x8FD1 },	// 8BDF E8BF91 [近]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BE0, */ 0x91D1 },	// 8BE0 E98791 [金]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BE1, */ 0x541F },	// 8BE1 E5909F [吟]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BE2, */ 0x9280 },	// 8BE2 E98A80 [銀]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BE3, */ 0x4E5D },	// 8BE3 E4B99D [九]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BE4, */ 0x5036 },	// 8BE4 E580B6 [倶]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BE5, */ 0x53E5 },	// 8BE5 E58FA5 [句]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BE6, */ 0x533A },	// 8BE6 E58CBA [区]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BE7, */ 0x72D7 },	// 8BE7 E78B97 [狗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BE8, */ 0x7396 },	// 8BE8 E78E96 [玖]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BE9, */ 0x77E9 },	// 8BE9 E79FA9 [矩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BEA, */ 0x82E6 },	// 8BEA E88BA6 [苦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BEB, */ 0x8EAF },	// 8BEB E8BAAF [躯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BEC, */ 0x99C6 },	// 8BEC E9A786 [駆]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BED, */ 0x99C8 },	// 8BED E9A788 [駈]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BEE, */ 0x99D2 },	// 8BEE E9A792 [駒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BEF, */ 0x5177 },	// 8BEF E585B7 [具]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BF0, */ 0x611A },	// 8BF0 E6849A [愚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BF1, */ 0x865E },	// 8BF1 E8999E [虞]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BF2, */ 0x55B0 },	// 8BF2 E596B0 [喰]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BF3, */ 0x7A7A },	// 8BF3 E7A9BA [空]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BF4, */ 0x5076 },	// 8BF4 E581B6 [偶]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BF5, */ 0x5BD3 },	// 8BF5 E5AF93 [寓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BF6, */ 0x9047 },	// 8BF6 E98187 [遇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BF7, */ 0x9685 },	// 8BF7 E99A85 [隅]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BF8, */ 0x4E32 },	// 8BF8 E4B8B2 [串]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BF9, */ 0x6ADB },	// 8BF9 E6AB9B [櫛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BFA, */ 0x91E7 },	// 8BFA E987A7 [釧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BFB, */ 0x5C51 },	// 8BFB E5B191 [屑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8BFC, */ 0x5C48 },	// 8BFC E5B188 [屈]  #CJK UNIFIED IDEOGRAPH
//===== 0x8C 40～FC ( テーブル数 [189]  ）====================
{ /* 0x8C40, */ 0x6398 },	// 8C40 E68E98 [掘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C41, */ 0x7A9F },	// 8C41 E7AA9F [窟]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C42, */ 0x6C93 },	// 8C42 E6B293 [沓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C43, */ 0x9774 },	// 8C43 E99DB4 [靴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C44, */ 0x8F61 },	// 8C44 E8BDA1 [轡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C45, */ 0x7AAA },	// 8C45 E7AAAA [窪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C46, */ 0x718A },	// 8C46 E7868A [熊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C47, */ 0x9688 },	// 8C47 E99A88 [隈]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C48, */ 0x7C82 },	// 8C48 E7B282 [粂]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C49, */ 0x6817 },	// 8C49 E6A097 [栗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C4A, */ 0x7E70 },	// 8C4A E7B9B0 [繰]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C4B, */ 0x6851 },	// 8C4B E6A191 [桑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C4C, */ 0x936C },	// 8C4C E98DAC [鍬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C4D, */ 0x52F2 },	// 8C4D E58BB2 [勲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C4E, */ 0x541B },	// 8C4E E5909B [君]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C4F, */ 0x85AB },	// 8C4F E896AB [薫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C50, */ 0x8A13 },	// 8C50 E8A893 [訓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C51, */ 0x7FA4 },	// 8C51 E7BEA4 [群]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C52, */ 0x8ECD },	// 8C52 E8BB8D [軍]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C53, */ 0x90E1 },	// 8C53 E983A1 [郡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C54, */ 0x5366 },	// 8C54 E58DA6 [卦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C55, */ 0x8888 },	// 8C55 E8A288 [袈]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C56, */ 0x7941 },	// 8C56 E7A581 [祁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C57, */ 0x4FC2 },	// 8C57 E4BF82 [係]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C58, */ 0x50BE },	// 8C58 E582BE [傾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C59, */ 0x5211 },	// 8C59 E58891 [刑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C5A, */ 0x5144 },	// 8C5A E58584 [兄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C5B, */ 0x5553 },	// 8C5B E59593 [啓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C5C, */ 0x572D },	// 8C5C E59CAD [圭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C5D, */ 0x73EA },	// 8C5D E78FAA [珪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C5E, */ 0x578B },	// 8C5E E59E8B [型]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C5F, */ 0x5951 },	// 8C5F E5A591 [契]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C60, */ 0x5F62 },	// 8C60 E5BDA2 [形]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C61, */ 0x5F84 },	// 8C61 E5BE84 [径]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C62, */ 0x6075 },	// 8C62 E681B5 [恵]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C63, */ 0x6176 },	// 8C63 E685B6 [慶]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C64, */ 0x6167 },	// 8C64 E685A7 [慧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C65, */ 0x61A9 },	// 8C65 E686A9 [憩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C66, */ 0x63B2 },	// 8C66 E68EB2 [掲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C67, */ 0x643A },	// 8C67 E690BA [携]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C68, */ 0x656C },	// 8C68 E695AC [敬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C69, */ 0x666F },	// 8C69 E699AF [景]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C6A, */ 0x6842 },	// 8C6A E6A182 [桂]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C6B, */ 0x6E13 },	// 8C6B E6B893 [渓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C6C, */ 0x7566 },	// 8C6C E795A6 [畦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C6D, */ 0x7A3D },	// 8C6D E7A8BD [稽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C6E, */ 0x7CFB },	// 8C6E E7B3BB [系]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C6F, */ 0x7D4C },	// 8C6F E7B58C [経]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C70, */ 0x7D99 },	// 8C70 E7B699 [継]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C71, */ 0x7E4B },	// 8C71 E7B98B [繋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C72, */ 0x7F6B },	// 8C72 E7BDAB [罫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C73, */ 0x830E },	// 8C73 E88C8E [茎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C74, */ 0x834A },	// 8C74 E88D8A [荊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C75, */ 0x86CD },	// 8C75 E89B8D [蛍]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C76, */ 0x8A08 },	// 8C76 E8A888 [計]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C77, */ 0x8A63 },	// 8C77 E8A9A3 [詣]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C78, */ 0x8B66 },	// 8C78 E8ADA6 [警]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C79, */ 0x8EFD },	// 8C79 E8BBBD [軽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C7A, */ 0x981A },	// 8C7A E9A09A [頚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C7B, */ 0x9D8F },	// 8C7B E9B68F [鶏]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C7C, */ 0x82B8 },	// 8C7C E88AB8 [芸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C7D, */ 0x8FCE },	// 8C7D E8BF8E [迎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C7E, */ 0x9BE8 },	// 8C7E E9AFA8 [鯨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C7F, */ 0x0000 },	// 		　#ダミー
{ /* 0x8C80, */ 0x5287 },	// 8C80 E58A87 [劇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C81, */ 0x621F },	// 8C81 E6889F [戟]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C82, */ 0x6483 },	// 8C82 E69283 [撃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C83, */ 0x6FC0 },	// 8C83 E6BF80 [激]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C84, */ 0x9699 },	// 8C84 E99A99 [隙]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C85, */ 0x6841 },	// 8C85 E6A181 [桁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C86, */ 0x5091 },	// 8C86 E58291 [傑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C87, */ 0x6B20 },	// 8C87 E6ACA0 [欠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C88, */ 0x6C7A },	// 8C88 E6B1BA [決]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C89, */ 0x6F54 },	// 8C89 E6BD94 [潔]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C8A, */ 0x7A74 },	// 8C8A E7A9B4 [穴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C8B, */ 0x7D50 },	// 8C8B E7B590 [結]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C8C, */ 0x8840 },	// 8C8C E8A180 [血]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C8D, */ 0x8A23 },	// 8C8D E8A8A3 [訣]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C8E, */ 0x6708 },	// 8C8E E69C88 [月]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C8F, */ 0x4EF6 },	// 8C8F E4BBB6 [件]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C90, */ 0x5039 },	// 8C90 E580B9 [倹]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C91, */ 0x5026 },	// 8C91 E580A6 [倦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C92, */ 0x5065 },	// 8C92 E581A5 [健]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C93, */ 0x517C },	// 8C93 E585BC [兼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C94, */ 0x5238 },	// 8C94 E588B8 [券]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C95, */ 0x5263 },	// 8C95 E589A3 [剣]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C96, */ 0x55A7 },	// 8C96 E596A7 [喧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C97, */ 0x570F },	// 8C97 E59C8F [圏]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C98, */ 0x5805 },	// 8C98 E5A085 [堅]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C99, */ 0x5ACC },	// 8C99 E5AB8C [嫌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C9A, */ 0x5EFA },	// 8C9A E5BBBA [建]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C9B, */ 0x61B2 },	// 8C9B E686B2 [憲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C9C, */ 0x61F8 },	// 8C9C E687B8 [懸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C9D, */ 0x62F3 },	// 8C9D E68BB3 [拳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C9E, */ 0x6372 },	// 8C9E E68DB2 [捲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8C9F, */ 0x691C },	// 8C9F E6A49C [検]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CA0, */ 0x6A29 },	// 8CA0 E6A8A9 [権]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CA1, */ 0x727D },	// 8CA1 E789BD [牽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CA2, */ 0x72AC },	// 8CA2 E78AAC [犬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CA3, */ 0x732E },	// 8CA3 E78CAE [献]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CA4, */ 0x7814 },	// 8CA4 E7A094 [研]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CA5, */ 0x786F },	// 8CA5 E7A1AF [硯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CA6, */ 0x7D79 },	// 8CA6 E7B5B9 [絹]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CA7, */ 0x770C },	// 8CA7 E79C8C [県]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CA8, */ 0x80A9 },	// 8CA8 E882A9 [肩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CA9, */ 0x898B },	// 8CA9 E8A68B [見]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CAA, */ 0x8B19 },	// 8CAA E8AC99 [謙]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CAB, */ 0x8CE2 },	// 8CAB E8B3A2 [賢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CAC, */ 0x8ED2 },	// 8CAC E8BB92 [軒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CAD, */ 0x9063 },	// 8CAD E981A3 [遣]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CAE, */ 0x9375 },	// 8CAE E98DB5 [鍵]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CAF, */ 0x967A },	// 8CAF E999BA [険]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CB0, */ 0x9855 },	// 8CB0 E9A195 [顕]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CB1, */ 0x9A13 },	// 8CB1 E9A893 [験]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CB2, */ 0x9E78 },	// 8CB2 E9B9B8 [鹸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CB3, */ 0x5143 },	// 8CB3 E58583 [元]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CB4, */ 0x539F },	// 8CB4 E58E9F [原]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CB5, */ 0x53B3 },	// 8CB5 E58EB3 [厳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CB6, */ 0x5E7B },	// 8CB6 E5B9BB [幻]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CB7, */ 0x5F26 },	// 8CB7 E5BCA6 [弦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CB8, */ 0x6E1B },	// 8CB8 E6B89B [減]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CB9, */ 0x6E90 },	// 8CB9 E6BA90 [源]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CBA, */ 0x7384 },	// 8CBA E78E84 [玄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CBB, */ 0x73FE },	// 8CBB E78FBE [現]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CBC, */ 0x7D43 },	// 8CBC E7B583 [絃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CBD, */ 0x8237 },	// 8CBD E888B7 [舷]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CBE, */ 0x8A00 },	// 8CBE E8A880 [言]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CBF, */ 0x8AFA },	// 8CBF E8ABBA [諺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CC0, */ 0x9650 },	// 8CC0 E99990 [限]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CC1, */ 0x4E4E },	// 8CC1 E4B98E [乎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CC2, */ 0x500B },	// 8CC2 E5808B [個]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CC3, */ 0x53E4 },	// 8CC3 E58FA4 [古]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CC4, */ 0x547C },	// 8CC4 E591BC [呼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CC5, */ 0x56FA },	// 8CC5 E59BBA [固]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CC6, */ 0x59D1 },	// 8CC6 E5A791 [姑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CC7, */ 0x5B64 },	// 8CC7 E5ADA4 [孤]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CC8, */ 0x5DF1 },	// 8CC8 E5B7B1 [己]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CC9, */ 0x5EAB },	// 8CC9 E5BAAB [庫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CCA, */ 0x5F27 },	// 8CCA E5BCA7 [弧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CCB, */ 0x6238 },	// 8CCB E688B8 [戸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CCC, */ 0x6545 },	// 8CCC E69585 [故]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CCD, */ 0x67AF },	// 8CCD E69EAF [枯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CCE, */ 0x6E56 },	// 8CCE E6B996 [湖]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CCF, */ 0x72D0 },	// 8CCF E78B90 [狐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CD0, */ 0x7CCA },	// 8CD0 E7B38A [糊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CD1, */ 0x88B4 },	// 8CD1 E8A2B4 [袴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CD2, */ 0x80A1 },	// 8CD2 E882A1 [股]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CD3, */ 0x80E1 },	// 8CD3 E883A1 [胡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CD4, */ 0x83F0 },	// 8CD4 E88FB0 [菰]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CD5, */ 0x864E },	// 8CD5 E8998E [虎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CD6, */ 0x8A87 },	// 8CD6 E8AA87 [誇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CD7, */ 0x8DE8 },	// 8CD7 E8B7A8 [跨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CD8, */ 0x9237 },	// 8CD8 E988B7 [鈷]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CD9, */ 0x96C7 },	// 8CD9 E99B87 [雇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CDA, */ 0x9867 },	// 8CDA E9A1A7 [顧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CDB, */ 0x9F13 },	// 8CDB E9BC93 [鼓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CDC, */ 0x4E94 },	// 8CDC E4BA94 [五]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CDD, */ 0x4E92 },	// 8CDD E4BA92 [互]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CDE, */ 0x4F0D },	// 8CDE E4BC8D [伍]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CDF, */ 0x5348 },	// 8CDF E58D88 [午]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CE0, */ 0x5449 },	// 8CE0 E59189 [呉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CE1, */ 0x543E },	// 8CE1 E590BE [吾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CE2, */ 0x5A2F },	// 8CE2 E5A8AF [娯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CE3, */ 0x5F8C },	// 8CE3 E5BE8C [後]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CE4, */ 0x5FA1 },	// 8CE4 E5BEA1 [御]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CE5, */ 0x609F },	// 8CE5 E6829F [悟]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CE6, */ 0x68A7 },	// 8CE6 E6A2A7 [梧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CE7, */ 0x6A8E },	// 8CE7 E6AA8E [檎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CE8, */ 0x745A },	// 8CE8 E7919A [瑚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CE9, */ 0x7881 },	// 8CE9 E7A281 [碁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CEA, */ 0x8A9E },	// 8CEA E8AA9E [語]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CEB, */ 0x8AA4 },	// 8CEB E8AAA4 [誤]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CEC, */ 0x8B77 },	// 8CEC E8ADB7 [護]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CED, */ 0x9190 },	// 8CED E98690 [醐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CEE, */ 0x4E5E },	// 8CEE E4B99E [乞]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CEF, */ 0x9BC9 },	// 8CEF E9AF89 [鯉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CF0, */ 0x4EA4 },	// 8CF0 E4BAA4 [交]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CF1, */ 0x4F7C },	// 8CF1 E4BDBC [佼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CF2, */ 0x4FAF },	// 8CF2 E4BEAF [侯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CF3, */ 0x5019 },	// 8CF3 E58099 [候]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CF4, */ 0x5016 },	// 8CF4 E58096 [倖]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CF5, */ 0x5149 },	// 8CF5 E58589 [光]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CF6, */ 0x516C },	// 8CF6 E585AC [公]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CF7, */ 0x529F },	// 8CF7 E58A9F [功]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CF8, */ 0x52B9 },	// 8CF8 E58AB9 [効]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CF9, */ 0x52FE },	// 8CF9 E58BBE [勾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CFA, */ 0x539A },	// 8CFA E58E9A [厚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CFB, */ 0x53E3 },	// 8CFB E58FA3 [口]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8CFC, */ 0x5411 },	// 8CFC E59091 [向]  #CJK UNIFIED IDEOGRAPH
//===== 0x8D 40～FC ( テーブル数 [189]  ）====================
{ /* 0x8D40, */ 0x540E },	// 8D40 E5908E [后]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D41, */ 0x5589 },	// 8D41 E59689 [喉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D42, */ 0x5751 },	// 8D42 E59D91 [坑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D43, */ 0x57A2 },	// 8D43 E59EA2 [垢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D44, */ 0x597D },	// 8D44 E5A5BD [好]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D45, */ 0x5B54 },	// 8D45 E5AD94 [孔]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D46, */ 0x5B5D },	// 8D46 E5AD9D [孝]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D47, */ 0x5B8F },	// 8D47 E5AE8F [宏]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D48, */ 0x5DE5 },	// 8D48 E5B7A5 [工]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D49, */ 0x5DE7 },	// 8D49 E5B7A7 [巧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D4A, */ 0x5DF7 },	// 8D4A E5B7B7 [巷]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D4B, */ 0x5E78 },	// 8D4B E5B9B8 [幸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D4C, */ 0x5E83 },	// 8D4C E5BA83 [広]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D4D, */ 0x5E9A },	// 8D4D E5BA9A [庚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D4E, */ 0x5EB7 },	// 8D4E E5BAB7 [康]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D4F, */ 0x5F18 },	// 8D4F E5BC98 [弘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D50, */ 0x6052 },	// 8D50 E68192 [恒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D51, */ 0x614C },	// 8D51 E6858C [慌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D52, */ 0x6297 },	// 8D52 E68A97 [抗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D53, */ 0x62D8 },	// 8D53 E68B98 [拘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D54, */ 0x63A7 },	// 8D54 E68EA7 [控]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D55, */ 0x653B },	// 8D55 E694BB [攻]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D56, */ 0x6602 },	// 8D56 E69882 [昂]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D57, */ 0x6643 },	// 8D57 E69983 [晃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D58, */ 0x66F4 },	// 8D58 E69BB4 [更]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D59, */ 0x676D },	// 8D59 E69DAD [杭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D5A, */ 0x6821 },	// 8D5A E6A0A1 [校]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D5B, */ 0x6897 },	// 8D5B E6A297 [梗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D5C, */ 0x69CB },	// 8D5C E6A78B [構]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D5D, */ 0x6C5F },	// 8D5D E6B19F [江]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D5E, */ 0x6D2A },	// 8D5E E6B4AA [洪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D5F, */ 0x6D69 },	// 8D5F E6B5A9 [浩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D60, */ 0x6E2F },	// 8D60 E6B8AF [港]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D61, */ 0x6E9D },	// 8D61 E6BA9D [溝]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D62, */ 0x7532 },	// 8D62 E794B2 [甲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D63, */ 0x7687 },	// 8D63 E79A87 [皇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D64, */ 0x786C },	// 8D64 E7A1AC [硬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D65, */ 0x7A3F },	// 8D65 E7A8BF [稿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D66, */ 0x7CE0 },	// 8D66 E7B3A0 [糠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D67, */ 0x7D05 },	// 8D67 E7B485 [紅]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D68, */ 0x7D18 },	// 8D68 E7B498 [紘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D69, */ 0x7D5E },	// 8D69 E7B59E [絞]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D6A, */ 0x7DB1 },	// 8D6A E7B6B1 [綱]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D6B, */ 0x8015 },	// 8D6B E88095 [耕]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D6C, */ 0x8003 },	// 8D6C E88083 [考]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D6D, */ 0x80AF },	// 8D6D E882AF [肯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D6E, */ 0x80B1 },	// 8D6E E882B1 [肱]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D6F, */ 0x8154 },	// 8D6F E88594 [腔]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D70, */ 0x818F },	// 8D70 E8868F [膏]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D71, */ 0x822A },	// 8D71 E888AA [航]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D72, */ 0x8352 },	// 8D72 E88D92 [荒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D73, */ 0x884C },	// 8D73 E8A18C [行]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D74, */ 0x8861 },	// 8D74 E8A1A1 [衡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D75, */ 0x8B1B },	// 8D75 E8AC9B [講]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D76, */ 0x8CA2 },	// 8D76 E8B2A2 [貢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D77, */ 0x8CFC },	// 8D77 E8B3BC [購]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D78, */ 0x90CA },	// 8D78 E9838A [郊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D79, */ 0x9175 },	// 8D79 E985B5 [酵]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D7A, */ 0x9271 },	// 8D7A E989B1 [鉱]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D7B, */ 0x783F },	// 8D7B E7A0BF [砿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D7C, */ 0x92FC },	// 8D7C E98BBC [鋼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D7D, */ 0x95A4 },	// 8D7D E996A4 [閤]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D7E, */ 0x964D },	// 8D7E E9998D [降]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D7F, */ 0x0000 },	// 		　#ダミー
{ /* 0x8D80, */ 0x9805 },	// 8D80 E9A085 [項]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D81, */ 0x9999 },	// 8D81 E9A699 [香]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D82, */ 0x9AD8 },	// 8D82 E9AB98 [高]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D83, */ 0x9D3B },	// 8D83 E9B4BB [鴻]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D84, */ 0x525B },	// 8D84 E5899B [剛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D85, */ 0x52AB },	// 8D85 E58AAB [劫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D86, */ 0x53F7 },	// 8D86 E58FB7 [号]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D87, */ 0x5408 },	// 8D87 E59088 [合]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D88, */ 0x58D5 },	// 8D88 E5A395 [壕]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D89, */ 0x62F7 },	// 8D89 E68BB7 [拷]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D8A, */ 0x6FE0 },	// 8D8A E6BFA0 [濠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D8B, */ 0x8C6A },	// 8D8B E8B1AA [豪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D8C, */ 0x8F5F },	// 8D8C E8BD9F [轟]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D8D, */ 0x9EB9 },	// 8D8D E9BAB9 [麹]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D8E, */ 0x514B },	// 8D8E E5858B [克]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D8F, */ 0x523B },	// 8D8F E588BB [刻]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D90, */ 0x544A },	// 8D90 E5918A [告]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D91, */ 0x56FD },	// 8D91 E59BBD [国]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D92, */ 0x7A40 },	// 8D92 E7A980 [穀]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D93, */ 0x9177 },	// 8D93 E985B7 [酷]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D94, */ 0x9D60 },	// 8D94 E9B5A0 [鵠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D95, */ 0x9ED2 },	// 8D95 E9BB92 [黒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D96, */ 0x7344 },	// 8D96 E78D84 [獄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D97, */ 0x6F09 },	// 8D97 E6BC89 [漉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D98, */ 0x8170 },	// 8D98 E885B0 [腰]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D99, */ 0x7511 },	// 8D99 E79491 [甑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D9A, */ 0x5FFD },	// 8D9A E5BFBD [忽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D9B, */ 0x60DA },	// 8D9B E6839A [惚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D9C, */ 0x9AA8 },	// 8D9C E9AAA8 [骨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D9D, */ 0x72DB },	// 8D9D E78B9B [狛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D9E, */ 0x8FBC },	// 8D9E E8BEBC [込]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8D9F, */ 0x6B64 },	// 8D9F E6ADA4 [此]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DA0, */ 0x9803 },	// 8DA0 E9A083 [頃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DA1, */ 0x4ECA },	// 8DA1 E4BB8A [今]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DA2, */ 0x56F0 },	// 8DA2 E59BB0 [困]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DA3, */ 0x5764 },	// 8DA3 E59DA4 [坤]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DA4, */ 0x58BE },	// 8DA4 E5A2BE [墾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DA5, */ 0x5A5A },	// 8DA5 E5A99A [婚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DA6, */ 0x6068 },	// 8DA6 E681A8 [恨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DA7, */ 0x61C7 },	// 8DA7 E68787 [懇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DA8, */ 0x660F },	// 8DA8 E6988F [昏]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DA9, */ 0x6606 },	// 8DA9 E69886 [昆]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DAA, */ 0x6839 },	// 8DAA E6A0B9 [根]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DAB, */ 0x68B1 },	// 8DAB E6A2B1 [梱]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DAC, */ 0x6DF7 },	// 8DAC E6B7B7 [混]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DAD, */ 0x75D5 },	// 8DAD E79795 [痕]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DAE, */ 0x7D3A },	// 8DAE E7B4BA [紺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DAF, */ 0x826E },	// 8DAF E889AE [艮]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DB0, */ 0x9B42 },	// 8DB0 E9AD82 [魂]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DB1, */ 0x4E9B },	// 8DB1 E4BA9B [些]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DB2, */ 0x4F50 },	// 8DB2 E4BD90 [佐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DB3, */ 0x53C9 },	// 8DB3 E58F89 [叉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DB4, */ 0x5506 },	// 8DB4 E59486 [唆]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DB5, */ 0x5D6F },	// 8DB5 E5B5AF [嵯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DB6, */ 0x5DE6 },	// 8DB6 E5B7A6 [左]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DB7, */ 0x5DEE },	// 8DB7 E5B7AE [差]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DB8, */ 0x67FB },	// 8DB8 E69FBB [査]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DB9, */ 0x6C99 },	// 8DB9 E6B299 [沙]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DBA, */ 0x7473 },	// 8DBA E791B3 [瑳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DBB, */ 0x7802 },	// 8DBB E7A082 [砂]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DBC, */ 0x8A50 },	// 8DBC E8A990 [詐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DBD, */ 0x9396 },	// 8DBD E98E96 [鎖]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DBE, */ 0x88DF },	// 8DBE E8A39F [裟]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DBF, */ 0x5750 },	// 8DBF E59D90 [坐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DC0, */ 0x5EA7 },	// 8DC0 E5BAA7 [座]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DC1, */ 0x632B },	// 8DC1 E68CAB [挫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DC2, */ 0x50B5 },	// 8DC2 E582B5 [債]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DC3, */ 0x50AC },	// 8DC3 E582AC [催]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DC4, */ 0x518D },	// 8DC4 E5868D [再]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DC5, */ 0x6700 },	// 8DC5 E69C80 [最]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DC6, */ 0x54C9 },	// 8DC6 E59389 [哉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DC7, */ 0x585E },	// 8DC7 E5A19E [塞]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DC8, */ 0x59BB },	// 8DC8 E5A6BB [妻]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DC9, */ 0x5BB0 },	// 8DC9 E5AEB0 [宰]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DCA, */ 0x5F69 },	// 8DCA E5BDA9 [彩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DCB, */ 0x624D },	// 8DCB E6898D [才]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DCC, */ 0x63A1 },	// 8DCC E68EA1 [採]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DCD, */ 0x683D },	// 8DCD E6A0BD [栽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DCE, */ 0x6B73 },	// 8DCE E6ADB3 [歳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DCF, */ 0x6E08 },	// 8DCF E6B888 [済]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DD0, */ 0x707D },	// 8DD0 E781BD [災]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DD1, */ 0x91C7 },	// 8DD1 E98787 [采]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DD2, */ 0x7280 },	// 8DD2 E78A80 [犀]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DD3, */ 0x7815 },	// 8DD3 E7A095 [砕]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DD4, */ 0x7826 },	// 8DD4 E7A0A6 [砦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DD5, */ 0x796D },	// 8DD5 E7A5AD [祭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DD6, */ 0x658E },	// 8DD6 E6968E [斎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DD7, */ 0x7D30 },	// 8DD7 E7B4B0 [細]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DD8, */ 0x83DC },	// 8DD8 E88F9C [菜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DD9, */ 0x88C1 },	// 8DD9 E8A381 [裁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DDA, */ 0x8F09 },	// 8DDA E8BC89 [載]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DDB, */ 0x969B },	// 8DDB E99A9B [際]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DDC, */ 0x5264 },	// 8DDC E589A4 [剤]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DDD, */ 0x5728 },	// 8DDD E59CA8 [在]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DDE, */ 0x6750 },	// 8DDE E69D90 [材]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DDF, */ 0x7F6A },	// 8DDF E7BDAA [罪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DE0, */ 0x8CA1 },	// 8DE0 E8B2A1 [財]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DE1, */ 0x51B4 },	// 8DE1 E586B4 [冴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DE2, */ 0x5742 },	// 8DE2 E59D82 [坂]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DE3, */ 0x962A },	// 8DE3 E998AA [阪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DE4, */ 0x583A },	// 8DE4 E5A0BA [堺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DE5, */ 0x698A },	// 8DE5 E6A68A [榊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DE6, */ 0x80B4 },	// 8DE6 E882B4 [肴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DE7, */ 0x54B2 },	// 8DE7 E592B2 [咲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DE8, */ 0x5D0E },	// 8DE8 E5B48E [崎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DE9, */ 0x57FC },	// 8DE9 E59FBC [埼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DEA, */ 0x7895 },	// 8DEA E7A295 [碕]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DEB, */ 0x9DFA },	// 8DEB E9B7BA [鷺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DEC, */ 0x4F5C },	// 8DEC E4BD9C [作]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DED, */ 0x524A },	// 8DED E5898A [削]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DEE, */ 0x548B },	// 8DEE E5928B [咋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DEF, */ 0x643E },	// 8DEF E690BE [搾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DF0, */ 0x6628 },	// 8DF0 E698A8 [昨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DF1, */ 0x6714 },	// 8DF1 E69C94 [朔]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DF2, */ 0x67F5 },	// 8DF2 E69FB5 [柵]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DF3, */ 0x7A84 },	// 8DF3 E7AA84 [窄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DF4, */ 0x7B56 },	// 8DF4 E7AD96 [策]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DF5, */ 0x7D22 },	// 8DF5 E7B4A2 [索]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DF6, */ 0x932F },	// 8DF6 E98CAF [錯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DF7, */ 0x685C },	// 8DF7 E6A19C [桜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DF8, */ 0x9BAD },	// 8DF8 E9AEAD [鮭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DF9, */ 0x7B39 },	// 8DF9 E7ACB9 [笹]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DFA, */ 0x5319 },	// 8DFA E58C99 [匙]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DFB, */ 0x518A },	// 8DFB E5868A [冊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8DFC, */ 0x5237 },	// 8DFC E588B7 [刷]  #CJK UNIFIED IDEOGRAPH
//===== 0x8E 40～FC ( テーブル数 [189]  ）====================
{ /* 0x8E40, */ 0x5BDF },	// 8E40 E5AF9F [察]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E41, */ 0x62F6 },	// 8E41 E68BB6 [拶]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E42, */ 0x64AE },	// 8E42 E692AE [撮]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E43, */ 0x64E6 },	// 8E43 E693A6 [擦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E44, */ 0x672D },	// 8E44 E69CAD [札]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E45, */ 0x6BBA },	// 8E45 E6AEBA [殺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E46, */ 0x85A9 },	// 8E46 E896A9 [薩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E47, */ 0x96D1 },	// 8E47 E99B91 [雑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E48, */ 0x7690 },	// 8E48 E79A90 [皐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E49, */ 0x9BD6 },	// 8E49 E9AF96 [鯖]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E4A, */ 0x634C },	// 8E4A E68D8C [捌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E4B, */ 0x9306 },	// 8E4B E98C86 [錆]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E4C, */ 0x9BAB },	// 8E4C E9AEAB [鮫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E4D, */ 0x76BF },	// 8E4D E79ABF [皿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E4E, */ 0x6652 },	// 8E4E E69992 [晒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E4F, */ 0x4E09 },	// 8E4F E4B889 [三]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E50, */ 0x5098 },	// 8E50 E58298 [傘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E51, */ 0x53C2 },	// 8E51 E58F82 [参]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E52, */ 0x5C71 },	// 8E52 E5B1B1 [山]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E53, */ 0x60E8 },	// 8E53 E683A8 [惨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E54, */ 0x6492 },	// 8E54 E69292 [撒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E55, */ 0x6563 },	// 8E55 E695A3 [散]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E56, */ 0x685F },	// 8E56 E6A19F [桟]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E57, */ 0x71E6 },	// 8E57 E787A6 [燦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E58, */ 0x73CA },	// 8E58 E78F8A [珊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E59, */ 0x7523 },	// 8E59 E794A3 [産]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E5A, */ 0x7B97 },	// 8E5A E7AE97 [算]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E5B, */ 0x7E82 },	// 8E5B E7BA82 [纂]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E5C, */ 0x8695 },	// 8E5C E89A95 [蚕]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E5D, */ 0x8B83 },	// 8E5D E8AE83 [讃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E5E, */ 0x8CDB },	// 8E5E E8B39B [賛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E5F, */ 0x9178 },	// 8E5F E985B8 [酸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E60, */ 0x9910 },	// 8E60 E9A490 [餐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E61, */ 0x65AC },	// 8E61 E696AC [斬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E62, */ 0x66AB },	// 8E62 E69AAB [暫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E63, */ 0x6B8B },	// 8E63 E6AE8B [残]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E64, */ 0x4ED5 },	// 8E64 E4BB95 [仕]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E65, */ 0x4ED4 },	// 8E65 E4BB94 [仔]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E66, */ 0x4F3A },	// 8E66 E4BCBA [伺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E67, */ 0x4F7F },	// 8E67 E4BDBF [使]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E68, */ 0x523A },	// 8E68 E588BA [刺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E69, */ 0x53F8 },	// 8E69 E58FB8 [司]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E6A, */ 0x53F2 },	// 8E6A E58FB2 [史]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E6B, */ 0x55E3 },	// 8E6B E597A3 [嗣]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E6C, */ 0x56DB },	// 8E6C E59B9B [四]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E6D, */ 0x58EB },	// 8E6D E5A3AB [士]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E6E, */ 0x59CB },	// 8E6E E5A78B [始]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E6F, */ 0x59C9 },	// 8E6F E5A789 [姉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E70, */ 0x59FF },	// 8E70 E5A7BF [姿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E71, */ 0x5B50 },	// 8E71 E5AD90 [子]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E72, */ 0x5C4D },	// 8E72 E5B18D [屍]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E73, */ 0x5E02 },	// 8E73 E5B882 [市]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E74, */ 0x5E2B },	// 8E74 E5B8AB [師]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E75, */ 0x5FD7 },	// 8E75 E5BF97 [志]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E76, */ 0x601D },	// 8E76 E6809D [思]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E77, */ 0x6307 },	// 8E77 E68C87 [指]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E78, */ 0x652F },	// 8E78 E694AF [支]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E79, */ 0x5B5C },	// 8E79 E5AD9C [孜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E7A, */ 0x65AF },	// 8E7A E696AF [斯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E7B, */ 0x65BD },	// 8E7B E696BD [施]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E7C, */ 0x65E8 },	// 8E7C E697A8 [旨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E7D, */ 0x679D },	// 8E7D E69E9D [枝]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E7E, */ 0x6B62 },	// 8E7E E6ADA2 [止]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E7F, */ 0x0000 },	// 		　#ダミー
{ /* 0x8E80, */ 0x6B7B },	// 8E80 E6ADBB [死]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E81, */ 0x6C0F },	// 8E81 E6B08F [氏]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E82, */ 0x7345 },	// 8E82 E78D85 [獅]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E83, */ 0x7949 },	// 8E83 E7A589 [祉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E84, */ 0x79C1 },	// 8E84 E7A781 [私]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E85, */ 0x7CF8 },	// 8E85 E7B3B8 [糸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E86, */ 0x7D19 },	// 8E86 E7B499 [紙]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E87, */ 0x7D2B },	// 8E87 E7B4AB [紫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E88, */ 0x80A2 },	// 8E88 E882A2 [肢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E89, */ 0x8102 },	// 8E89 E88482 [脂]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E8A, */ 0x81F3 },	// 8E8A E887B3 [至]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E8B, */ 0x8996 },	// 8E8B E8A696 [視]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E8C, */ 0x8A5E },	// 8E8C E8A99E [詞]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E8D, */ 0x8A69 },	// 8E8D E8A9A9 [詩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E8E, */ 0x8A66 },	// 8E8E E8A9A6 [試]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E8F, */ 0x8A8C },	// 8E8F E8AA8C [誌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E90, */ 0x8AEE },	// 8E90 E8ABAE [諮]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E91, */ 0x8CC7 },	// 8E91 E8B387 [資]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E92, */ 0x8CDC },	// 8E92 E8B39C [賜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E93, */ 0x96CC },	// 8E93 E99B8C [雌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E94, */ 0x98FC },	// 8E94 E9A3BC [飼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E95, */ 0x6B6F },	// 8E95 E6ADAF [歯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E96, */ 0x4E8B },	// 8E96 E4BA8B [事]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E97, */ 0x4F3C },	// 8E97 E4BCBC [似]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E98, */ 0x4F8D },	// 8E98 E4BE8D [侍]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E99, */ 0x5150 },	// 8E99 E58590 [児]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E9A, */ 0x5B57 },	// 8E9A E5AD97 [字]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E9B, */ 0x5BFA },	// 8E9B E5AFBA [寺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E9C, */ 0x6148 },	// 8E9C E68588 [慈]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E9D, */ 0x6301 },	// 8E9D E68C81 [持]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E9E, */ 0x6642 },	// 8E9E E69982 [時]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8E9F, */ 0x6B21 },	// 8E9F E6ACA1 [次]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EA0, */ 0x6ECB },	// 8EA0 E6BB8B [滋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EA1, */ 0x6CBB },	// 8EA1 E6B2BB [治]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EA2, */ 0x723E },	// 8EA2 E788BE [爾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EA3, */ 0x74BD },	// 8EA3 E792BD [璽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EA4, */ 0x75D4 },	// 8EA4 E79794 [痔]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EA5, */ 0x78C1 },	// 8EA5 E7A381 [磁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EA6, */ 0x793A },	// 8EA6 E7A4BA [示]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EA7, */ 0x800C },	// 8EA7 E8808C [而]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EA8, */ 0x8033 },	// 8EA8 E880B3 [耳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EA9, */ 0x81EA },	// 8EA9 E887AA [自]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EAA, */ 0x8494 },	// 8EAA E89294 [蒔]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EAB, */ 0x8F9E },	// 8EAB E8BE9E [辞]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EAC, */ 0x6C50 },	// 8EAC E6B190 [汐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EAD, */ 0x9E7F },	// 8EAD E9B9BF [鹿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EAE, */ 0x5F0F },	// 8EAE E5BC8F [式]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EAF, */ 0x8B58 },	// 8EAF E8AD98 [識]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EB0, */ 0x9D2B },	// 8EB0 E9B4AB [鴫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EB1, */ 0x7AFA },	// 8EB1 E7ABBA [竺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EB2, */ 0x8EF8 },	// 8EB2 E8BBB8 [軸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EB3, */ 0x5B8D },	// 8EB3 E5AE8D [宍]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EB4, */ 0x96EB },	// 8EB4 E99BAB [雫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EB5, */ 0x4E03 },	// 8EB5 E4B883 [七]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EB6, */ 0x53F1 },	// 8EB6 E58FB1 [叱]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EB7, */ 0x57F7 },	// 8EB7 E59FB7 [執]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EB8, */ 0x5931 },	// 8EB8 E5A4B1 [失]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EB9, */ 0x5AC9 },	// 8EB9 E5AB89 [嫉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EBA, */ 0x5BA4 },	// 8EBA E5AEA4 [室]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EBB, */ 0x6089 },	// 8EBB E68289 [悉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EBC, */ 0x6E7F },	// 8EBC E6B9BF [湿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EBD, */ 0x6F06 },	// 8EBD E6BC86 [漆]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EBE, */ 0x75BE },	// 8EBE E796BE [疾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EBF, */ 0x8CEA },	// 8EBF E8B3AA [質]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EC0, */ 0x5B9F },	// 8EC0 E5AE9F [実]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EC1, */ 0x8500 },	// 8EC1 E89480 [蔀]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EC2, */ 0x7BE0 },	// 8EC2 E7AFA0 [篠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EC3, */ 0x5072 },	// 8EC3 E581B2 [偲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EC4, */ 0x67F4 },	// 8EC4 E69FB4 [柴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EC5, */ 0x829D },	// 8EC5 E88A9D [芝]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EC6, */ 0x5C61 },	// 8EC6 E5B1A1 [屡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EC7, */ 0x854A },	// 8EC7 E8958A [蕊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EC8, */ 0x7E1E },	// 8EC8 E7B89E [縞]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EC9, */ 0x820E },	// 8EC9 E8888E [舎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ECA, */ 0x5199 },	// 8ECA E58699 [写]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ECB, */ 0x5C04 },	// 8ECB E5B084 [射]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ECC, */ 0x6368 },	// 8ECC E68DA8 [捨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ECD, */ 0x8D66 },	// 8ECD E8B5A6 [赦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ECE, */ 0x659C },	// 8ECE E6969C [斜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ECF, */ 0x716E },	// 8ECF E785AE [煮]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ED0, */ 0x793E },	// 8ED0 E7A4BE [社]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ED1, */ 0x7D17 },	// 8ED1 E7B497 [紗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ED2, */ 0x8005 },	// 8ED2 E88085 [者]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ED3, */ 0x8B1D },	// 8ED3 E8AC9D [謝]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ED4, */ 0x8ECA },	// 8ED4 E8BB8A [車]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ED5, */ 0x906E },	// 8ED5 E981AE [遮]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ED6, */ 0x86C7 },	// 8ED6 E89B87 [蛇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ED7, */ 0x90AA },	// 8ED7 E982AA [邪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ED8, */ 0x501F },	// 8ED8 E5809F [借]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8ED9, */ 0x52FA },	// 8ED9 E58BBA [勺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EDA, */ 0x5C3A },	// 8EDA E5B0BA [尺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EDB, */ 0x6753 },	// 8EDB E69D93 [杓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EDC, */ 0x707C },	// 8EDC E781BC [灼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EDD, */ 0x7235 },	// 8EDD E788B5 [爵]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EDE, */ 0x914C },	// 8EDE E9858C [酌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EDF, */ 0x91C8 },	// 8EDF E98788 [釈]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EE0, */ 0x932B },	// 8EE0 E98CAB [錫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EE1, */ 0x82E5 },	// 8EE1 E88BA5 [若]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EE2, */ 0x5BC2 },	// 8EE2 E5AF82 [寂]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EE3, */ 0x5F31 },	// 8EE3 E5BCB1 [弱]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EE4, */ 0x60F9 },	// 8EE4 E683B9 [惹]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EE5, */ 0x4E3B },	// 8EE5 E4B8BB [主]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EE6, */ 0x53D6 },	// 8EE6 E58F96 [取]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EE7, */ 0x5B88 },	// 8EE7 E5AE88 [守]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EE8, */ 0x624B },	// 8EE8 E6898B [手]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EE9, */ 0x6731 },	// 8EE9 E69CB1 [朱]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EEA, */ 0x6B8A },	// 8EEA E6AE8A [殊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EEB, */ 0x72E9 },	// 8EEB E78BA9 [狩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EEC, */ 0x73E0 },	// 8EEC E78FA0 [珠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EED, */ 0x7A2E },	// 8EED E7A8AE [種]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EEE, */ 0x816B },	// 8EEE E885AB [腫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EEF, */ 0x8DA3 },	// 8EEF E8B6A3 [趣]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EF0, */ 0x9152 },	// 8EF0 E98592 [酒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EF1, */ 0x9996 },	// 8EF1 E9A696 [首]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EF2, */ 0x5112 },	// 8EF2 E58492 [儒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EF3, */ 0x53D7 },	// 8EF3 E58F97 [受]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EF4, */ 0x546A },	// 8EF4 E591AA [呪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EF5, */ 0x5BFF },	// 8EF5 E5AFBF [寿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EF6, */ 0x6388 },	// 8EF6 E68E88 [授]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EF7, */ 0x6A39 },	// 8EF7 E6A8B9 [樹]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EF8, */ 0x7DAC },	// 8EF8 E7B6AC [綬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EF9, */ 0x9700 },	// 8EF9 E99C80 [需]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EFA, */ 0x56DA },	// 8EFA E59B9A [囚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EFB, */ 0x53CE },	// 8EFB E58F8E [収]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8EFC, */ 0x5468 },	// 8EFC E591A8 [周]  #CJK UNIFIED IDEOGRAPH
//===== 0x8F 40～FC ( テーブル数 [189]  ）====================
{ /* 0x8F40, */ 0x5B97 },	// 8F40 E5AE97 [宗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F41, */ 0x5C31 },	// 8F41 E5B0B1 [就]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F42, */ 0x5DDE },	// 8F42 E5B79E [州]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F43, */ 0x4FEE },	// 8F43 E4BFAE [修]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F44, */ 0x6101 },	// 8F44 E68481 [愁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F45, */ 0x62FE },	// 8F45 E68BBE [拾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F46, */ 0x6D32 },	// 8F46 E6B4B2 [洲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F47, */ 0x79C0 },	// 8F47 E7A780 [秀]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F48, */ 0x79CB },	// 8F48 E7A78B [秋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F49, */ 0x7D42 },	// 8F49 E7B582 [終]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F4A, */ 0x7E4D },	// 8F4A E7B98D [繍]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F4B, */ 0x7FD2 },	// 8F4B E7BF92 [習]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F4C, */ 0x81ED },	// 8F4C E887AD [臭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F4D, */ 0x821F },	// 8F4D E8889F [舟]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F4E, */ 0x8490 },	// 8F4E E89290 [蒐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F4F, */ 0x8846 },	// 8F4F E8A186 [衆]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F50, */ 0x8972 },	// 8F50 E8A5B2 [襲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F51, */ 0x8B90 },	// 8F51 E8AE90 [讐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F52, */ 0x8E74 },	// 8F52 E8B9B4 [蹴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F53, */ 0x8F2F },	// 8F53 E8BCAF [輯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F54, */ 0x9031 },	// 8F54 E980B1 [週]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F55, */ 0x914B },	// 8F55 E9858B [酋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F56, */ 0x916C },	// 8F56 E985AC [酬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F57, */ 0x96C6 },	// 8F57 E99B86 [集]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F58, */ 0x919C },	// 8F58 E9869C [醜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F59, */ 0x4EC0 },	// 8F59 E4BB80 [什]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F5A, */ 0x4F4F },	// 8F5A E4BD8F [住]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F5B, */ 0x5145 },	// 8F5B E58585 [充]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F5C, */ 0x5341 },	// 8F5C E58D81 [十]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F5D, */ 0x5F93 },	// 8F5D E5BE93 [従]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F5E, */ 0x620E },	// 8F5E E6888E [戎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F5F, */ 0x67D4 },	// 8F5F E69F94 [柔]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F60, */ 0x6C41 },	// 8F60 E6B181 [汁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F61, */ 0x6E0B },	// 8F61 E6B88B [渋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F62, */ 0x7363 },	// 8F62 E78DA3 [獣]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F63, */ 0x7E26 },	// 8F63 E7B8A6 [縦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F64, */ 0x91CD },	// 8F64 E9878D [重]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F65, */ 0x9283 },	// 8F65 E98A83 [銃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F66, */ 0x53D4 },	// 8F66 E58F94 [叔]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F67, */ 0x5919 },	// 8F67 E5A499 [夙]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F68, */ 0x5BBF },	// 8F68 E5AEBF [宿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F69, */ 0x6DD1 },	// 8F69 E6B791 [淑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F6A, */ 0x795D },	// 8F6A E7A59D [祝]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F6B, */ 0x7E2E },	// 8F6B E7B8AE [縮]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F6C, */ 0x7C9B },	// 8F6C E7B29B [粛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F6D, */ 0x587E },	// 8F6D E5A1BE [塾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F6E, */ 0x719F },	// 8F6E E7869F [熟]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F6F, */ 0x51FA },	// 8F6F E587BA [出]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F70, */ 0x8853 },	// 8F70 E8A193 [術]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F71, */ 0x8FF0 },	// 8F71 E8BFB0 [述]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F72, */ 0x4FCA },	// 8F72 E4BF8A [俊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F73, */ 0x5CFB },	// 8F73 E5B3BB [峻]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F74, */ 0x6625 },	// 8F74 E698A5 [春]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F75, */ 0x77AC },	// 8F75 E79EAC [瞬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F76, */ 0x7AE3 },	// 8F76 E7ABA3 [竣]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F77, */ 0x821C },	// 8F77 E8889C [舜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F78, */ 0x99FF },	// 8F78 E9A7BF [駿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F79, */ 0x51C6 },	// 8F79 E58786 [准]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F7A, */ 0x5FAA },	// 8F7A E5BEAA [循]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F7B, */ 0x65EC },	// 8F7B E697AC [旬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F7C, */ 0x696F },	// 8F7C E6A5AF [楯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F7D, */ 0x6B89 },	// 8F7D E6AE89 [殉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F7E, */ 0x6DF3 },	// 8F7E E6B7B3 [淳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F7F, */ 0x0000 },	// 		　#ダミー
{ /* 0x8F80, */ 0x6E96 },	// 8F80 E6BA96 [準]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F81, */ 0x6F64 },	// 8F81 E6BDA4 [潤]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F82, */ 0x76FE },	// 8F82 E79BBE [盾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F83, */ 0x7D14 },	// 8F83 E7B494 [純]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F84, */ 0x5DE1 },	// 8F84 E5B7A1 [巡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F85, */ 0x9075 },	// 8F85 E981B5 [遵]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F86, */ 0x9187 },	// 8F86 E98687 [醇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F87, */ 0x9806 },	// 8F87 E9A086 [順]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F88, */ 0x51E6 },	// 8F88 E587A6 [処]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F89, */ 0x521D },	// 8F89 E5889D [初]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F8A, */ 0x6240 },	// 8F8A E68980 [所]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F8B, */ 0x6691 },	// 8F8B E69A91 [暑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F8C, */ 0x66D9 },	// 8F8C E69B99 [曙]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F8D, */ 0x6E1A },	// 8F8D E6B89A [渚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F8E, */ 0x5EB6 },	// 8F8E E5BAB6 [庶]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F8F, */ 0x7DD2 },	// 8F8F E7B792 [緒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F90, */ 0x7F72 },	// 8F90 E7BDB2 [署]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F91, */ 0x66F8 },	// 8F91 E69BB8 [書]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F92, */ 0x85AF },	// 8F92 E896AF [薯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F93, */ 0x85F7 },	// 8F93 E897B7 [藷]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F94, */ 0x8AF8 },	// 8F94 E8ABB8 [諸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F95, */ 0x52A9 },	// 8F95 E58AA9 [助]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F96, */ 0x53D9 },	// 8F96 E58F99 [叙]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F97, */ 0x5973 },	// 8F97 E5A5B3 [女]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F98, */ 0x5E8F },	// 8F98 E5BA8F [序]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F99, */ 0x5F90 },	// 8F99 E5BE90 [徐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F9A, */ 0x6055 },	// 8F9A E68195 [恕]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F9B, */ 0x92E4 },	// 8F9B E98BA4 [鋤]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F9C, */ 0x9664 },	// 8F9C E999A4 [除]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F9D, */ 0x50B7 },	// 8F9D E582B7 [傷]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F9E, */ 0x511F },	// 8F9E E5849F [償]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8F9F, */ 0x52DD },	// 8F9F E58B9D [勝]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FA0, */ 0x5320 },	// 8FA0 E58CA0 [匠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FA1, */ 0x5347 },	// 8FA1 E58D87 [升]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FA2, */ 0x53EC },	// 8FA2 E58FAC [召]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FA3, */ 0x54E8 },	// 8FA3 E593A8 [哨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FA4, */ 0x5546 },	// 8FA4 E59586 [商]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FA5, */ 0x5531 },	// 8FA5 E594B1 [唱]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FA6, */ 0x5617 },	// 8FA6 E59897 [嘗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FA7, */ 0x5968 },	// 8FA7 E5A5A8 [奨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FA8, */ 0x59BE },	// 8FA8 E5A6BE [妾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FA9, */ 0x5A3C },	// 8FA9 E5A8BC [娼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FAA, */ 0x5BB5 },	// 8FAA E5AEB5 [宵]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FAB, */ 0x5C06 },	// 8FAB E5B086 [将]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FAC, */ 0x5C0F },	// 8FAC E5B08F [小]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FAD, */ 0x5C11 },	// 8FAD E5B091 [少]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FAE, */ 0x5C1A },	// 8FAE E5B09A [尚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FAF, */ 0x5E84 },	// 8FAF E5BA84 [庄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FB0, */ 0x5E8A },	// 8FB0 E5BA8A [床]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FB1, */ 0x5EE0 },	// 8FB1 E5BBA0 [廠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FB2, */ 0x5F70 },	// 8FB2 E5BDB0 [彰]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FB3, */ 0x627F },	// 8FB3 E689BF [承]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FB4, */ 0x6284 },	// 8FB4 E68A84 [抄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FB5, */ 0x62DB },	// 8FB5 E68B9B [招]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FB6, */ 0x638C },	// 8FB6 E68E8C [掌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FB7, */ 0x6377 },	// 8FB7 E68DB7 [捷]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FB8, */ 0x6607 },	// 8FB8 E69887 [昇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FB9, */ 0x660C },	// 8FB9 E6988C [昌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FBA, */ 0x662D },	// 8FBA E698AD [昭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FBB, */ 0x6676 },	// 8FBB E699B6 [晶]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FBC, */ 0x677E },	// 8FBC E69DBE [松]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FBD, */ 0x68A2 },	// 8FBD E6A2A2 [梢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FBE, */ 0x6A1F },	// 8FBE E6A89F [樟]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FBF, */ 0x6A35 },	// 8FBF E6A8B5 [樵]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FC0, */ 0x6CBC },	// 8FC0 E6B2BC [沼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FC1, */ 0x6D88 },	// 8FC1 E6B688 [消]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FC2, */ 0x6E09 },	// 8FC2 E6B889 [渉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FC3, */ 0x6E58 },	// 8FC3 E6B998 [湘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FC4, */ 0x713C },	// 8FC4 E784BC [焼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FC5, */ 0x7126 },	// 8FC5 E784A6 [焦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FC6, */ 0x7167 },	// 8FC6 E785A7 [照]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FC7, */ 0x75C7 },	// 8FC7 E79787 [症]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FC8, */ 0x7701 },	// 8FC8 E79C81 [省]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FC9, */ 0x785D },	// 8FC9 E7A19D [硝]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FCA, */ 0x7901 },	// 8FCA E7A481 [礁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FCB, */ 0x7965 },	// 8FCB E7A5A5 [祥]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FCC, */ 0x79F0 },	// 8FCC E7A7B0 [称]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FCD, */ 0x7AE0 },	// 8FCD E7ABA0 [章]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FCE, */ 0x7B11 },	// 8FCE E7AC91 [笑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FCF, */ 0x7CA7 },	// 8FCF E7B2A7 [粧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FD0, */ 0x7D39 },	// 8FD0 E7B4B9 [紹]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FD1, */ 0x8096 },	// 8FD1 E88296 [肖]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FD2, */ 0x83D6 },	// 8FD2 E88F96 [菖]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FD3, */ 0x848B },	// 8FD3 E8928B [蒋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FD4, */ 0x8549 },	// 8FD4 E89589 [蕉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FD5, */ 0x885D },	// 8FD5 E8A19D [衝]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FD6, */ 0x88F3 },	// 8FD6 E8A3B3 [裳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FD7, */ 0x8A1F },	// 8FD7 E8A89F [訟]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FD8, */ 0x8A3C },	// 8FD8 E8A8BC [証]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FD9, */ 0x8A54 },	// 8FD9 E8A994 [詔]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FDA, */ 0x8A73 },	// 8FDA E8A9B3 [詳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FDB, */ 0x8C61 },	// 8FDB E8B1A1 [象]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FDC, */ 0x8CDE },	// 8FDC E8B39E [賞]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FDD, */ 0x91A4 },	// 8FDD E986A4 [醤]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FDE, */ 0x9266 },	// 8FDE E989A6 [鉦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FDF, */ 0x937E },	// 8FDF E98DBE [鍾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FE0, */ 0x9418 },	// 8FE0 E99098 [鐘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FE1, */ 0x969C },	// 8FE1 E99A9C [障]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FE2, */ 0x9798 },	// 8FE2 E99E98 [鞘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FE3, */ 0x4E0A },	// 8FE3 E4B88A [上]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FE4, */ 0x4E08 },	// 8FE4 E4B888 [丈]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FE5, */ 0x4E1E },	// 8FE5 E4B89E [丞]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FE6, */ 0x4E57 },	// 8FE6 E4B997 [乗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FE7, */ 0x5197 },	// 8FE7 E58697 [冗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FE8, */ 0x5270 },	// 8FE8 E589B0 [剰]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FE9, */ 0x57CE },	// 8FE9 E59F8E [城]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FEA, */ 0x5834 },	// 8FEA E5A0B4 [場]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FEB, */ 0x58CC },	// 8FEB E5A38C [壌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FEC, */ 0x5B22 },	// 8FEC E5ACA2 [嬢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FED, */ 0x5E38 },	// 8FED E5B8B8 [常]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FEE, */ 0x60C5 },	// 8FEE E68385 [情]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FEF, */ 0x64FE },	// 8FEF E693BE [擾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FF0, */ 0x6761 },	// 8FF0 E69DA1 [条]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FF1, */ 0x6756 },	// 8FF1 E69D96 [杖]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FF2, */ 0x6D44 },	// 8FF2 E6B584 [浄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FF3, */ 0x72B6 },	// 8FF3 E78AB6 [状]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FF4, */ 0x7573 },	// 8FF4 E795B3 [畳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FF5, */ 0x7A63 },	// 8FF5 E7A9A3 [穣]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FF6, */ 0x84B8 },	// 8FF6 E892B8 [蒸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FF7, */ 0x8B72 },	// 8FF7 E8ADB2 [譲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FF8, */ 0x91B8 },	// 8FF8 E986B8 [醸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FF9, */ 0x9320 },	// 8FF9 E98CA0 [錠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FFA, */ 0x5631 },	// 8FFA E598B1 [嘱]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FFB, */ 0x57F4 },	// 8FFB E59FB4 [埴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x8FFC, */ 0x98FE },	// 8FFC E9A3BE [飾]  #CJK UNIFIED IDEOGRAPH
//===== 0x90 40～FC ( テーブル数 [189]  ）====================
{ /* 0x9040, */ 0x62ED },	// 9040 E68BAD [拭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9041, */ 0x690D },	// 9041 E6A48D [植]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9042, */ 0x6B96 },	// 9042 E6AE96 [殖]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9043, */ 0x71ED },	// 9043 E787AD [燭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9044, */ 0x7E54 },	// 9044 E7B994 [織]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9045, */ 0x8077 },	// 9045 E881B7 [職]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9046, */ 0x8272 },	// 9046 E889B2 [色]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9047, */ 0x89E6 },	// 9047 E8A7A6 [触]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9048, */ 0x98DF },	// 9048 E9A39F [食]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9049, */ 0x8755 },	// 9049 E89D95 [蝕]  #CJK UNIFIED IDEOGRAPH
{ /* 0x904A, */ 0x8FB1 },	// 904A E8BEB1 [辱]  #CJK UNIFIED IDEOGRAPH
{ /* 0x904B, */ 0x5C3B },	// 904B E5B0BB [尻]  #CJK UNIFIED IDEOGRAPH
{ /* 0x904C, */ 0x4F38 },	// 904C E4BCB8 [伸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x904D, */ 0x4FE1 },	// 904D E4BFA1 [信]  #CJK UNIFIED IDEOGRAPH
{ /* 0x904E, */ 0x4FB5 },	// 904E E4BEB5 [侵]  #CJK UNIFIED IDEOGRAPH
{ /* 0x904F, */ 0x5507 },	// 904F E59487 [唇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9050, */ 0x5A20 },	// 9050 E5A8A0 [娠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9051, */ 0x5BDD },	// 9051 E5AF9D [寝]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9052, */ 0x5BE9 },	// 9052 E5AFA9 [審]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9053, */ 0x5FC3 },	// 9053 E5BF83 [心]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9054, */ 0x614E },	// 9054 E6858E [慎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9055, */ 0x632F },	// 9055 E68CAF [振]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9056, */ 0x65B0 },	// 9056 E696B0 [新]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9057, */ 0x664B },	// 9057 E6998B [晋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9058, */ 0x68EE },	// 9058 E6A3AE [森]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9059, */ 0x699B },	// 9059 E6A69B [榛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x905A, */ 0x6D78 },	// 905A E6B5B8 [浸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x905B, */ 0x6DF1 },	// 905B E6B7B1 [深]  #CJK UNIFIED IDEOGRAPH
{ /* 0x905C, */ 0x7533 },	// 905C E794B3 [申]  #CJK UNIFIED IDEOGRAPH
{ /* 0x905D, */ 0x75B9 },	// 905D E796B9 [疹]  #CJK UNIFIED IDEOGRAPH
{ /* 0x905E, */ 0x771F },	// 905E E79C9F [真]  #CJK UNIFIED IDEOGRAPH
{ /* 0x905F, */ 0x795E },	// 905F E7A59E [神]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9060, */ 0x79E6 },	// 9060 E7A7A6 [秦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9061, */ 0x7D33 },	// 9061 E7B4B3 [紳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9062, */ 0x81E3 },	// 9062 E887A3 [臣]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9063, */ 0x82AF },	// 9063 E88AAF [芯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9064, */ 0x85AA },	// 9064 E896AA [薪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9065, */ 0x89AA },	// 9065 E8A6AA [親]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9066, */ 0x8A3A },	// 9066 E8A8BA [診]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9067, */ 0x8EAB },	// 9067 E8BAAB [身]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9068, */ 0x8F9B },	// 9068 E8BE9B [辛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9069, */ 0x9032 },	// 9069 E980B2 [進]  #CJK UNIFIED IDEOGRAPH
{ /* 0x906A, */ 0x91DD },	// 906A E9879D [針]  #CJK UNIFIED IDEOGRAPH
{ /* 0x906B, */ 0x9707 },	// 906B E99C87 [震]  #CJK UNIFIED IDEOGRAPH
{ /* 0x906C, */ 0x4EBA },	// 906C E4BABA [人]  #CJK UNIFIED IDEOGRAPH
{ /* 0x906D, */ 0x4EC1 },	// 906D E4BB81 [仁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x906E, */ 0x5203 },	// 906E E58883 [刃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x906F, */ 0x5875 },	// 906F E5A1B5 [塵]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9070, */ 0x58EC },	// 9070 E5A3AC [壬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9071, */ 0x5C0B },	// 9071 E5B08B [尋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9072, */ 0x751A },	// 9072 E7949A [甚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9073, */ 0x5C3D },	// 9073 E5B0BD [尽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9074, */ 0x814E },	// 9074 E8858E [腎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9075, */ 0x8A0A },	// 9075 E8A88A [訊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9076, */ 0x8FC5 },	// 9076 E8BF85 [迅]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9077, */ 0x9663 },	// 9077 E999A3 [陣]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9078, */ 0x976D },	// 9078 E99DAD [靭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9079, */ 0x7B25 },	// 9079 E7ACA5 [笥]  #CJK UNIFIED IDEOGRAPH
{ /* 0x907A, */ 0x8ACF },	// 907A E8AB8F [諏]  #CJK UNIFIED IDEOGRAPH
{ /* 0x907B, */ 0x9808 },	// 907B E9A088 [須]  #CJK UNIFIED IDEOGRAPH
{ /* 0x907C, */ 0x9162 },	// 907C E985A2 [酢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x907D, */ 0x56F3 },	// 907D E59BB3 [図]  #CJK UNIFIED IDEOGRAPH
{ /* 0x907E, */ 0x53A8 },	// 907E E58EA8 [厨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x907F, */ 0x0000 },	// 		　#ダミー
{ /* 0x9080, */ 0x9017 },	// 9080 E98097 [逗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9081, */ 0x5439 },	// 9081 E590B9 [吹]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9082, */ 0x5782 },	// 9082 E59E82 [垂]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9083, */ 0x5E25 },	// 9083 E5B8A5 [帥]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9084, */ 0x63A8 },	// 9084 E68EA8 [推]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9085, */ 0x6C34 },	// 9085 E6B0B4 [水]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9086, */ 0x708A },	// 9086 E7828A [炊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9087, */ 0x7761 },	// 9087 E79DA1 [睡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9088, */ 0x7C8B },	// 9088 E7B28B [粋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9089, */ 0x7FE0 },	// 9089 E7BFA0 [翠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x908A, */ 0x8870 },	// 908A E8A1B0 [衰]  #CJK UNIFIED IDEOGRAPH
{ /* 0x908B, */ 0x9042 },	// 908B E98182 [遂]  #CJK UNIFIED IDEOGRAPH
{ /* 0x908C, */ 0x9154 },	// 908C E98594 [酔]  #CJK UNIFIED IDEOGRAPH
{ /* 0x908D, */ 0x9310 },	// 908D E98C90 [錐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x908E, */ 0x9318 },	// 908E E98C98 [錘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x908F, */ 0x968F },	// 908F E99A8F [随]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9090, */ 0x745E },	// 9090 E7919E [瑞]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9091, */ 0x9AC4 },	// 9091 E9AB84 [髄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9092, */ 0x5D07 },	// 9092 E5B487 [崇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9093, */ 0x5D69 },	// 9093 E5B5A9 [嵩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9094, */ 0x6570 },	// 9094 E695B0 [数]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9095, */ 0x67A2 },	// 9095 E69EA2 [枢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9096, */ 0x8DA8 },	// 9096 E8B6A8 [趨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9097, */ 0x96DB },	// 9097 E99B9B [雛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9098, */ 0x636E },	// 9098 E68DAE [据]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9099, */ 0x6749 },	// 9099 E69D89 [杉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x909A, */ 0x6919 },	// 909A E6A499 [椙]  #CJK UNIFIED IDEOGRAPH
{ /* 0x909B, */ 0x83C5 },	// 909B E88F85 [菅]  #CJK UNIFIED IDEOGRAPH
{ /* 0x909C, */ 0x9817 },	// 909C E9A097 [頗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x909D, */ 0x96C0 },	// 909D E99B80 [雀]  #CJK UNIFIED IDEOGRAPH
{ /* 0x909E, */ 0x88FE },	// 909E E8A3BE [裾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x909F, */ 0x6F84 },	// 909F E6BE84 [澄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90A0, */ 0x647A },	// 90A0 E691BA [摺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90A1, */ 0x5BF8 },	// 90A1 E5AFB8 [寸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90A2, */ 0x4E16 },	// 90A2 E4B896 [世]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90A3, */ 0x702C },	// 90A3 E780AC [瀬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90A4, */ 0x755D },	// 90A4 E7959D [畝]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90A5, */ 0x662F },	// 90A5 E698AF [是]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90A6, */ 0x51C4 },	// 90A6 E58784 [凄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90A7, */ 0x5236 },	// 90A7 E588B6 [制]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90A8, */ 0x52E2 },	// 90A8 E58BA2 [勢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90A9, */ 0x59D3 },	// 90A9 E5A793 [姓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90AA, */ 0x5F81 },	// 90AA E5BE81 [征]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90AB, */ 0x6027 },	// 90AB E680A7 [性]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90AC, */ 0x6210 },	// 90AC E68890 [成]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90AD, */ 0x653F },	// 90AD E694BF [政]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90AE, */ 0x6574 },	// 90AE E695B4 [整]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90AF, */ 0x661F },	// 90AF E6989F [星]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90B0, */ 0x6674 },	// 90B0 E699B4 [晴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90B1, */ 0x68F2 },	// 90B1 E6A3B2 [棲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90B2, */ 0x6816 },	// 90B2 E6A096 [栖]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90B3, */ 0x6B63 },	// 90B3 E6ADA3 [正]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90B4, */ 0x6E05 },	// 90B4 E6B885 [清]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90B5, */ 0x7272 },	// 90B5 E789B2 [牲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90B6, */ 0x751F },	// 90B6 E7949F [生]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90B7, */ 0x76DB },	// 90B7 E79B9B [盛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90B8, */ 0x7CBE },	// 90B8 E7B2BE [精]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90B9, */ 0x8056 },	// 90B9 E88196 [聖]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90BA, */ 0x58F0 },	// 90BA E5A3B0 [声]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90BB, */ 0x88FD },	// 90BB E8A3BD [製]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90BC, */ 0x897F },	// 90BC E8A5BF [西]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90BD, */ 0x8AA0 },	// 90BD E8AAA0 [誠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90BE, */ 0x8A93 },	// 90BE E8AA93 [誓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90BF, */ 0x8ACB },	// 90BF E8AB8B [請]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90C0, */ 0x901D },	// 90C0 E9809D [逝]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90C1, */ 0x9192 },	// 90C1 E98692 [醒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90C2, */ 0x9752 },	// 90C2 E99D92 [青]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90C3, */ 0x9759 },	// 90C3 E99D99 [静]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90C4, */ 0x6589 },	// 90C4 E69689 [斉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90C5, */ 0x7A0E },	// 90C5 E7A88E [税]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90C6, */ 0x8106 },	// 90C6 E88486 [脆]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90C7, */ 0x96BB },	// 90C7 E99ABB [隻]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90C8, */ 0x5E2D },	// 90C8 E5B8AD [席]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90C9, */ 0x60DC },	// 90C9 E6839C [惜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90CA, */ 0x621A },	// 90CA E6889A [戚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90CB, */ 0x65A5 },	// 90CB E696A5 [斥]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90CC, */ 0x6614 },	// 90CC E69894 [昔]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90CD, */ 0x6790 },	// 90CD E69E90 [析]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90CE, */ 0x77F3 },	// 90CE E79FB3 [石]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90CF, */ 0x7A4D },	// 90CF E7A98D [積]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90D0, */ 0x7C4D },	// 90D0 E7B18D [籍]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90D1, */ 0x7E3E },	// 90D1 E7B8BE [績]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90D2, */ 0x810A },	// 90D2 E8848A [脊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90D3, */ 0x8CAC },	// 90D3 E8B2AC [責]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90D4, */ 0x8D64 },	// 90D4 E8B5A4 [赤]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90D5, */ 0x8DE1 },	// 90D5 E8B7A1 [跡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90D6, */ 0x8E5F },	// 90D6 E8B99F [蹟]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90D7, */ 0x78A9 },	// 90D7 E7A2A9 [碩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90D8, */ 0x5207 },	// 90D8 E58887 [切]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90D9, */ 0x62D9 },	// 90D9 E68B99 [拙]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90DA, */ 0x63A5 },	// 90DA E68EA5 [接]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90DB, */ 0x6442 },	// 90DB E69182 [摂]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90DC, */ 0x6298 },	// 90DC E68A98 [折]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90DD, */ 0x8A2D },	// 90DD E8A8AD [設]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90DE, */ 0x7A83 },	// 90DE E7AA83 [窃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90DF, */ 0x7BC0 },	// 90DF E7AF80 [節]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90E0, */ 0x8AAC },	// 90E0 E8AAAC [説]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90E1, */ 0x96EA },	// 90E1 E99BAA [雪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90E2, */ 0x7D76 },	// 90E2 E7B5B6 [絶]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90E3, */ 0x820C },	// 90E3 E8888C [舌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90E4, */ 0x8749 },	// 90E4 E89D89 [蝉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90E5, */ 0x4ED9 },	// 90E5 E4BB99 [仙]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90E6, */ 0x5148 },	// 90E6 E58588 [先]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90E7, */ 0x5343 },	// 90E7 E58D83 [千]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90E8, */ 0x5360 },	// 90E8 E58DA0 [占]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90E9, */ 0x5BA3 },	// 90E9 E5AEA3 [宣]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90EA, */ 0x5C02 },	// 90EA E5B082 [専]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90EB, */ 0x5C16 },	// 90EB E5B096 [尖]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90EC, */ 0x5DDD },	// 90EC E5B79D [川]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90ED, */ 0x6226 },	// 90ED E688A6 [戦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90EE, */ 0x6247 },	// 90EE E68987 [扇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90EF, */ 0x64B0 },	// 90EF E692B0 [撰]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90F0, */ 0x6813 },	// 90F0 E6A093 [栓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90F1, */ 0x6834 },	// 90F1 E6A0B4 [栴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90F2, */ 0x6CC9 },	// 90F2 E6B389 [泉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90F3, */ 0x6D45 },	// 90F3 E6B585 [浅]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90F4, */ 0x6D17 },	// 90F4 E6B497 [洗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90F5, */ 0x67D3 },	// 90F5 E69F93 [染]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90F6, */ 0x6F5C },	// 90F6 E6BD9C [潜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90F7, */ 0x714E },	// 90F7 E7858E [煎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90F8, */ 0x717D },	// 90F8 E785BD [煽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90F9, */ 0x65CB },	// 90F9 E6978B [旋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90FA, */ 0x7A7F },	// 90FA E7A9BF [穿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90FB, */ 0x7BAD },	// 90FB E7AEAD [箭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x90FC, */ 0x7DDA },	// 90FC E7B79A [線]  #CJK UNIFIED IDEOGRAPH
//===== 0x91 40～FC ( テーブル数 [189]  ）====================
{ /* 0x9140, */ 0x7E4A },	// 9140 E7B98A [繊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9141, */ 0x7FA8 },	// 9141 E7BEA8 [羨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9142, */ 0x817A },	// 9142 E885BA [腺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9143, */ 0x821B },	// 9143 E8889B [舛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9144, */ 0x8239 },	// 9144 E888B9 [船]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9145, */ 0x85A6 },	// 9145 E896A6 [薦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9146, */ 0x8A6E },	// 9146 E8A9AE [詮]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9147, */ 0x8CCE },	// 9147 E8B38E [賎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9148, */ 0x8DF5 },	// 9148 E8B7B5 [践]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9149, */ 0x9078 },	// 9149 E981B8 [選]  #CJK UNIFIED IDEOGRAPH
{ /* 0x914A, */ 0x9077 },	// 914A E981B7 [遷]  #CJK UNIFIED IDEOGRAPH
{ /* 0x914B, */ 0x92AD },	// 914B E98AAD [銭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x914C, */ 0x9291 },	// 914C E98A91 [銑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x914D, */ 0x9583 },	// 914D E99683 [閃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x914E, */ 0x9BAE },	// 914E E9AEAE [鮮]  #CJK UNIFIED IDEOGRAPH
{ /* 0x914F, */ 0x524D },	// 914F E5898D [前]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9150, */ 0x5584 },	// 9150 E59684 [善]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9151, */ 0x6F38 },	// 9151 E6BCB8 [漸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9152, */ 0x7136 },	// 9152 E784B6 [然]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9153, */ 0x5168 },	// 9153 E585A8 [全]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9154, */ 0x7985 },	// 9154 E7A685 [禅]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9155, */ 0x7E55 },	// 9155 E7B995 [繕]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9156, */ 0x81B3 },	// 9156 E886B3 [膳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9157, */ 0x7CCE },	// 9157 E7B38E [糎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9158, */ 0x564C },	// 9158 E5998C [噌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9159, */ 0x5851 },	// 9159 E5A191 [塑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x915A, */ 0x5CA8 },	// 915A E5B2A8 [岨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x915B, */ 0x63AA },	// 915B E68EAA [措]  #CJK UNIFIED IDEOGRAPH
{ /* 0x915C, */ 0x66FE },	// 915C E69BBE [曾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x915D, */ 0x66FD },	// 915D E69BBD [曽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x915E, */ 0x695A },	// 915E E6A59A [楚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x915F, */ 0x72D9 },	// 915F E78B99 [狙]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9160, */ 0x758F },	// 9160 E7968F [疏]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9161, */ 0x758E },	// 9161 E7968E [疎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9162, */ 0x790E },	// 9162 E7A48E [礎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9163, */ 0x7956 },	// 9163 E7A596 [祖]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9164, */ 0x79DF },	// 9164 E7A79F [租]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9165, */ 0x7C97 },	// 9165 E7B297 [粗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9166, */ 0x7D20 },	// 9166 E7B4A0 [素]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9167, */ 0x7D44 },	// 9167 E7B584 [組]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9168, */ 0x8607 },	// 9168 E89887 [蘇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9169, */ 0x8A34 },	// 9169 E8A8B4 [訴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x916A, */ 0x963B },	// 916A E998BB [阻]  #CJK UNIFIED IDEOGRAPH
{ /* 0x916B, */ 0x9061 },	// 916B E981A1 [遡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x916C, */ 0x9F20 },	// 916C E9BCA0 [鼠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x916D, */ 0x50E7 },	// 916D E583A7 [僧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x916E, */ 0x5275 },	// 916E E589B5 [創]  #CJK UNIFIED IDEOGRAPH
{ /* 0x916F, */ 0x53CC },	// 916F E58F8C [双]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9170, */ 0x53E2 },	// 9170 E58FA2 [叢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9171, */ 0x5009 },	// 9171 E58089 [倉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9172, */ 0x55AA },	// 9172 E596AA [喪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9173, */ 0x58EE },	// 9173 E5A3AE [壮]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9174, */ 0x594F },	// 9174 E5A58F [奏]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9175, */ 0x723D },	// 9175 E788BD [爽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9176, */ 0x5B8B },	// 9176 E5AE8B [宋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9177, */ 0x5C64 },	// 9177 E5B1A4 [層]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9178, */ 0x531D },	// 9178 E58C9D [匝]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9179, */ 0x60E3 },	// 9179 E683A3 [惣]  #CJK UNIFIED IDEOGRAPH
{ /* 0x917A, */ 0x60F3 },	// 917A E683B3 [想]  #CJK UNIFIED IDEOGRAPH
{ /* 0x917B, */ 0x635C },	// 917B E68D9C [捜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x917C, */ 0x6383 },	// 917C E68E83 [掃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x917D, */ 0x633F },	// 917D E68CBF [挿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x917E, */ 0x63BB },	// 917E E68EBB [掻]  #CJK UNIFIED IDEOGRAPH
{ /* 0x917F, */ 0x0000 },	// 		　#ダミー
{ /* 0x9180, */ 0x64CD },	// 9180 E6938D [操]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9181, */ 0x65E9 },	// 9181 E697A9 [早]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9182, */ 0x66F9 },	// 9182 E69BB9 [曹]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9183, */ 0x5DE3 },	// 9183 E5B7A3 [巣]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9184, */ 0x69CD },	// 9184 E6A78D [槍]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9185, */ 0x69FD },	// 9185 E6A7BD [槽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9186, */ 0x6F15 },	// 9186 E6BC95 [漕]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9187, */ 0x71E5 },	// 9187 E787A5 [燥]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9188, */ 0x4E89 },	// 9188 E4BA89 [争]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9189, */ 0x75E9 },	// 9189 E797A9 [痩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x918A, */ 0x76F8 },	// 918A E79BB8 [相]  #CJK UNIFIED IDEOGRAPH
{ /* 0x918B, */ 0x7A93 },	// 918B E7AA93 [窓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x918C, */ 0x7CDF },	// 918C E7B39F [糟]  #CJK UNIFIED IDEOGRAPH
{ /* 0x918D, */ 0x7DCF },	// 918D E7B78F [総]  #CJK UNIFIED IDEOGRAPH
{ /* 0x918E, */ 0x7D9C },	// 918E E7B69C [綜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x918F, */ 0x8061 },	// 918F E881A1 [聡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9190, */ 0x8349 },	// 9190 E88D89 [草]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9191, */ 0x8358 },	// 9191 E88D98 [荘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9192, */ 0x846C },	// 9192 E891AC [葬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9193, */ 0x84BC },	// 9193 E892BC [蒼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9194, */ 0x85FB },	// 9194 E897BB [藻]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9195, */ 0x88C5 },	// 9195 E8A385 [装]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9196, */ 0x8D70 },	// 9196 E8B5B0 [走]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9197, */ 0x9001 },	// 9197 E98081 [送]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9198, */ 0x906D },	// 9198 E981AD [遭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9199, */ 0x9397 },	// 9199 E98E97 [鎗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x919A, */ 0x971C },	// 919A E99C9C [霜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x919B, */ 0x9A12 },	// 919B E9A892 [騒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x919C, */ 0x50CF },	// 919C E5838F [像]  #CJK UNIFIED IDEOGRAPH
{ /* 0x919D, */ 0x5897 },	// 919D E5A297 [増]  #CJK UNIFIED IDEOGRAPH
{ /* 0x919E, */ 0x618E },	// 919E E6868E [憎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x919F, */ 0x81D3 },	// 919F E88793 [臓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91A0, */ 0x8535 },	// 91A0 E894B5 [蔵]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91A1, */ 0x8D08 },	// 91A1 E8B488 [贈]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91A2, */ 0x9020 },	// 91A2 E980A0 [造]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91A3, */ 0x4FC3 },	// 91A3 E4BF83 [促]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91A4, */ 0x5074 },	// 91A4 E581B4 [側]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91A5, */ 0x5247 },	// 91A5 E58987 [則]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91A6, */ 0x5373 },	// 91A6 E58DB3 [即]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91A7, */ 0x606F },	// 91A7 E681AF [息]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91A8, */ 0x6349 },	// 91A8 E68D89 [捉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91A9, */ 0x675F },	// 91A9 E69D9F [束]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91AA, */ 0x6E2C },	// 91AA E6B8AC [測]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91AB, */ 0x8DB3 },	// 91AB E8B6B3 [足]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91AC, */ 0x901F },	// 91AC E9809F [速]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91AD, */ 0x4FD7 },	// 91AD E4BF97 [俗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91AE, */ 0x5C5E },	// 91AE E5B19E [属]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91AF, */ 0x8CCA },	// 91AF E8B38A [賊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91B0, */ 0x65CF },	// 91B0 E6978F [族]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91B1, */ 0x7D9A },	// 91B1 E7B69A [続]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91B2, */ 0x5352 },	// 91B2 E58D92 [卒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91B3, */ 0x8896 },	// 91B3 E8A296 [袖]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91B4, */ 0x5176 },	// 91B4 E585B6 [其]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91B5, */ 0x63C3 },	// 91B5 E68F83 [揃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91B6, */ 0x5B58 },	// 91B6 E5AD98 [存]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91B7, */ 0x5B6B },	// 91B7 E5ADAB [孫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91B8, */ 0x5C0A },	// 91B8 E5B08A [尊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91B9, */ 0x640D },	// 91B9 E6908D [損]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91BA, */ 0x6751 },	// 91BA E69D91 [村]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91BB, */ 0x905C },	// 91BB E9819C [遜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91BC, */ 0x4ED6 },	// 91BC E4BB96 [他]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91BD, */ 0x591A },	// 91BD E5A49A [多]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91BE, */ 0x592A },	// 91BE E5A4AA [太]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91BF, */ 0x6C70 },	// 91BF E6B1B0 [汰]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91C0, */ 0x8A51 },	// 91C0 E8A991 [詑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91C1, */ 0x553E },	// 91C1 E594BE [唾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91C2, */ 0x5815 },	// 91C2 E5A095 [堕]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91C3, */ 0x59A5 },	// 91C3 E5A6A5 [妥]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91C4, */ 0x60F0 },	// 91C4 E683B0 [惰]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91C5, */ 0x6253 },	// 91C5 E68993 [打]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91C6, */ 0x67C1 },	// 91C6 E69F81 [柁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91C7, */ 0x8235 },	// 91C7 E888B5 [舵]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91C8, */ 0x6955 },	// 91C8 E6A595 [楕]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91C9, */ 0x9640 },	// 91C9 E99980 [陀]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91CA, */ 0x99C4 },	// 91CA E9A784 [駄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91CB, */ 0x9A28 },	// 91CB E9A8A8 [騨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91CC, */ 0x4F53 },	// 91CC E4BD93 [体]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91CD, */ 0x5806 },	// 91CD E5A086 [堆]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91CE, */ 0x5BFE },	// 91CE E5AFBE [対]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91CF, */ 0x8010 },	// 91CF E88090 [耐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91D0, */ 0x5CB1 },	// 91D0 E5B2B1 [岱]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91D1, */ 0x5E2F },	// 91D1 E5B8AF [帯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91D2, */ 0x5F85 },	// 91D2 E5BE85 [待]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91D3, */ 0x6020 },	// 91D3 E680A0 [怠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91D4, */ 0x614B },	// 91D4 E6858B [態]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91D5, */ 0x6234 },	// 91D5 E688B4 [戴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91D6, */ 0x66FF },	// 91D6 E69BBF [替]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91D7, */ 0x6CF0 },	// 91D7 E6B3B0 [泰]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91D8, */ 0x6EDE },	// 91D8 E6BB9E [滞]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91D9, */ 0x80CE },	// 91D9 E8838E [胎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91DA, */ 0x817F },	// 91DA E885BF [腿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91DB, */ 0x82D4 },	// 91DB E88B94 [苔]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91DC, */ 0x888B },	// 91DC E8A28B [袋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91DD, */ 0x8CB8 },	// 91DD E8B2B8 [貸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91DE, */ 0x9000 },	// 91DE E98080 [退]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91DF, */ 0x902E },	// 91DF E980AE [逮]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91E0, */ 0x968A },	// 91E0 E99A8A [隊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91E1, */ 0x9EDB },	// 91E1 E9BB9B [黛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91E2, */ 0x9BDB },	// 91E2 E9AF9B [鯛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91E3, */ 0x4EE3 },	// 91E3 E4BBA3 [代]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91E4, */ 0x53F0 },	// 91E4 E58FB0 [台]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91E5, */ 0x5927 },	// 91E5 E5A4A7 [大]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91E6, */ 0x7B2C },	// 91E6 E7ACAC [第]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91E7, */ 0x918D },	// 91E7 E9868D [醍]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91E8, */ 0x984C },	// 91E8 E9A18C [題]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91E9, */ 0x9DF9 },	// 91E9 E9B7B9 [鷹]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91EA, */ 0x6EDD },	// 91EA E6BB9D [滝]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91EB, */ 0x7027 },	// 91EB E780A7 [瀧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91EC, */ 0x5353 },	// 91EC E58D93 [卓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91ED, */ 0x5544 },	// 91ED E59584 [啄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91EE, */ 0x5B85 },	// 91EE E5AE85 [宅]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91EF, */ 0x6258 },	// 91EF E68998 [托]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91F0, */ 0x629E },	// 91F0 E68A9E [択]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91F1, */ 0x62D3 },	// 91F1 E68B93 [拓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91F2, */ 0x6CA2 },	// 91F2 E6B2A2 [沢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91F3, */ 0x6FEF },	// 91F3 E6BFAF [濯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91F4, */ 0x7422 },	// 91F4 E790A2 [琢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91F5, */ 0x8A17 },	// 91F5 E8A897 [託]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91F6, */ 0x9438 },	// 91F6 E990B8 [鐸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91F7, */ 0x6FC1 },	// 91F7 E6BF81 [濁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91F8, */ 0x8AFE },	// 91F8 E8ABBE [諾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91F9, */ 0x8338 },	// 91F9 E88CB8 [茸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91FA, */ 0x51E7 },	// 91FA E587A7 [凧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91FB, */ 0x86F8 },	// 91FB E89BB8 [蛸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x91FC, */ 0x53EA },	// 91FC E58FAA [只]  #CJK UNIFIED IDEOGRAPH
//===== 0x92 40～FC ( テーブル数 [189]  ）====================
{ /* 0x9240, */ 0x53E9 },	// 9240 E58FA9 [叩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9241, */ 0x4F46 },	// 9241 E4BD86 [但]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9242, */ 0x9054 },	// 9242 E98194 [達]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9243, */ 0x8FB0 },	// 9243 E8BEB0 [辰]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9244, */ 0x596A },	// 9244 E5A5AA [奪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9245, */ 0x8131 },	// 9245 E884B1 [脱]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9246, */ 0x5DFD },	// 9246 E5B7BD [巽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9247, */ 0x7AEA },	// 9247 E7ABAA [竪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9248, */ 0x8FBF },	// 9248 E8BEBF [辿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9249, */ 0x68DA },	// 9249 E6A39A [棚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x924A, */ 0x8C37 },	// 924A E8B0B7 [谷]  #CJK UNIFIED IDEOGRAPH
{ /* 0x924B, */ 0x72F8 },	// 924B E78BB8 [狸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x924C, */ 0x9C48 },	// 924C E9B188 [鱈]  #CJK UNIFIED IDEOGRAPH
{ /* 0x924D, */ 0x6A3D },	// 924D E6A8BD [樽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x924E, */ 0x8AB0 },	// 924E E8AAB0 [誰]  #CJK UNIFIED IDEOGRAPH
{ /* 0x924F, */ 0x4E39 },	// 924F E4B8B9 [丹]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9250, */ 0x5358 },	// 9250 E58D98 [単]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9251, */ 0x5606 },	// 9251 E59886 [嘆]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9252, */ 0x5766 },	// 9252 E59DA6 [坦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9253, */ 0x62C5 },	// 9253 E68B85 [担]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9254, */ 0x63A2 },	// 9254 E68EA2 [探]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9255, */ 0x65E6 },	// 9255 E697A6 [旦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9256, */ 0x6B4E },	// 9256 E6AD8E [歎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9257, */ 0x6DE1 },	// 9257 E6B7A1 [淡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9258, */ 0x6E5B },	// 9258 E6B99B [湛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9259, */ 0x70AD },	// 9259 E782AD [炭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x925A, */ 0x77ED },	// 925A E79FAD [短]  #CJK UNIFIED IDEOGRAPH
{ /* 0x925B, */ 0x7AEF },	// 925B E7ABAF [端]  #CJK UNIFIED IDEOGRAPH
{ /* 0x925C, */ 0x7BAA },	// 925C E7AEAA [箪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x925D, */ 0x7DBB },	// 925D E7B6BB [綻]  #CJK UNIFIED IDEOGRAPH
{ /* 0x925E, */ 0x803D },	// 925E E880BD [耽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x925F, */ 0x80C6 },	// 925F E88386 [胆]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9260, */ 0x86CB },	// 9260 E89B8B [蛋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9261, */ 0x8A95 },	// 9261 E8AA95 [誕]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9262, */ 0x935B },	// 9262 E98D9B [鍛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9263, */ 0x56E3 },	// 9263 E59BA3 [団]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9264, */ 0x58C7 },	// 9264 E5A387 [壇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9265, */ 0x5F3E },	// 9265 E5BCBE [弾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9266, */ 0x65AD },	// 9266 E696AD [断]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9267, */ 0x6696 },	// 9267 E69A96 [暖]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9268, */ 0x6A80 },	// 9268 E6AA80 [檀]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9269, */ 0x6BB5 },	// 9269 E6AEB5 [段]  #CJK UNIFIED IDEOGRAPH
{ /* 0x926A, */ 0x7537 },	// 926A E794B7 [男]  #CJK UNIFIED IDEOGRAPH
{ /* 0x926B, */ 0x8AC7 },	// 926B E8AB87 [談]  #CJK UNIFIED IDEOGRAPH
{ /* 0x926C, */ 0x5024 },	// 926C E580A4 [値]  #CJK UNIFIED IDEOGRAPH
{ /* 0x926D, */ 0x77E5 },	// 926D E79FA5 [知]  #CJK UNIFIED IDEOGRAPH
{ /* 0x926E, */ 0x5730 },	// 926E E59CB0 [地]  #CJK UNIFIED IDEOGRAPH
{ /* 0x926F, */ 0x5F1B },	// 926F E5BC9B [弛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9270, */ 0x6065 },	// 9270 E681A5 [恥]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9271, */ 0x667A },	// 9271 E699BA [智]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9272, */ 0x6C60 },	// 9272 E6B1A0 [池]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9273, */ 0x75F4 },	// 9273 E797B4 [痴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9274, */ 0x7A1A },	// 9274 E7A89A [稚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9275, */ 0x7F6E },	// 9275 E7BDAE [置]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9276, */ 0x81F4 },	// 9276 E887B4 [致]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9277, */ 0x8718 },	// 9277 E89C98 [蜘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9278, */ 0x9045 },	// 9278 E98185 [遅]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9279, */ 0x99B3 },	// 9279 E9A6B3 [馳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x927A, */ 0x7BC9 },	// 927A E7AF89 [築]  #CJK UNIFIED IDEOGRAPH
{ /* 0x927B, */ 0x755C },	// 927B E7959C [畜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x927C, */ 0x7AF9 },	// 927C E7ABB9 [竹]  #CJK UNIFIED IDEOGRAPH
{ /* 0x927D, */ 0x7B51 },	// 927D E7AD91 [筑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x927E, */ 0x84C4 },	// 927E E89384 [蓄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x927F, */ 0x0000 },	// 		　#ダミー
{ /* 0x9280, */ 0x9010 },	// 9280 E98090 [逐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9281, */ 0x79E9 },	// 9281 E7A7A9 [秩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9282, */ 0x7A92 },	// 9282 E7AA92 [窒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9283, */ 0x8336 },	// 9283 E88CB6 [茶]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9284, */ 0x5AE1 },	// 9284 E5ABA1 [嫡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9285, */ 0x7740 },	// 9285 E79D80 [着]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9286, */ 0x4E2D },	// 9286 E4B8AD [中]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9287, */ 0x4EF2 },	// 9287 E4BBB2 [仲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9288, */ 0x5B99 },	// 9288 E5AE99 [宙]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9289, */ 0x5FE0 },	// 9289 E5BFA0 [忠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x928A, */ 0x62BD },	// 928A E68ABD [抽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x928B, */ 0x663C },	// 928B E698BC [昼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x928C, */ 0x67F1 },	// 928C E69FB1 [柱]  #CJK UNIFIED IDEOGRAPH
{ /* 0x928D, */ 0x6CE8 },	// 928D E6B3A8 [注]  #CJK UNIFIED IDEOGRAPH
{ /* 0x928E, */ 0x866B },	// 928E E899AB [虫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x928F, */ 0x8877 },	// 928F E8A1B7 [衷]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9290, */ 0x8A3B },	// 9290 E8A8BB [註]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9291, */ 0x914E },	// 9291 E9858E [酎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9292, */ 0x92F3 },	// 9292 E98BB3 [鋳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9293, */ 0x99D0 },	// 9293 E9A790 [駐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9294, */ 0x6A17 },	// 9294 E6A897 [樗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9295, */ 0x7026 },	// 9295 E780A6 [瀦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9296, */ 0x732A },	// 9296 E78CAA [猪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9297, */ 0x82E7 },	// 9297 E88BA7 [苧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9298, */ 0x8457 },	// 9298 E89197 [著]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9299, */ 0x8CAF },	// 9299 E8B2AF [貯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x929A, */ 0x4E01 },	// 929A E4B881 [丁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x929B, */ 0x5146 },	// 929B E58586 [兆]  #CJK UNIFIED IDEOGRAPH
{ /* 0x929C, */ 0x51CB },	// 929C E5878B [凋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x929D, */ 0x558B },	// 929D E5968B [喋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x929E, */ 0x5BF5 },	// 929E E5AFB5 [寵]  #CJK UNIFIED IDEOGRAPH
{ /* 0x929F, */ 0x5E16 },	// 929F E5B896 [帖]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92A0, */ 0x5E33 },	// 92A0 E5B8B3 [帳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92A1, */ 0x5E81 },	// 92A1 E5BA81 [庁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92A2, */ 0x5F14 },	// 92A2 E5BC94 [弔]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92A3, */ 0x5F35 },	// 92A3 E5BCB5 [張]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92A4, */ 0x5F6B },	// 92A4 E5BDAB [彫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92A5, */ 0x5FB4 },	// 92A5 E5BEB4 [徴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92A6, */ 0x61F2 },	// 92A6 E687B2 [懲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92A7, */ 0x6311 },	// 92A7 E68C91 [挑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92A8, */ 0x66A2 },	// 92A8 E69AA2 [暢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92A9, */ 0x671D },	// 92A9 E69C9D [朝]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92AA, */ 0x6F6E },	// 92AA E6BDAE [潮]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92AB, */ 0x7252 },	// 92AB E78992 [牒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92AC, */ 0x753A },	// 92AC E794BA [町]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92AD, */ 0x773A },	// 92AD E79CBA [眺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92AE, */ 0x8074 },	// 92AE E881B4 [聴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92AF, */ 0x8139 },	// 92AF E884B9 [脹]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92B0, */ 0x8178 },	// 92B0 E885B8 [腸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92B1, */ 0x8776 },	// 92B1 E89DB6 [蝶]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92B2, */ 0x8ABF },	// 92B2 E8AABF [調]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92B3, */ 0x8ADC },	// 92B3 E8AB9C [諜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92B4, */ 0x8D85 },	// 92B4 E8B685 [超]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92B5, */ 0x8DF3 },	// 92B5 E8B7B3 [跳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92B6, */ 0x929A },	// 92B6 E98A9A [銚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92B7, */ 0x9577 },	// 92B7 E995B7 [長]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92B8, */ 0x9802 },	// 92B8 E9A082 [頂]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92B9, */ 0x9CE5 },	// 92B9 E9B3A5 [鳥]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92BA, */ 0x52C5 },	// 92BA E58B85 [勅]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92BB, */ 0x6357 },	// 92BB E68D97 [捗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92BC, */ 0x76F4 },	// 92BC E79BB4 [直]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92BD, */ 0x6715 },	// 92BD E69C95 [朕]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92BE, */ 0x6C88 },	// 92BE E6B288 [沈]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92BF, */ 0x73CD },	// 92BF E78F8D [珍]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92C0, */ 0x8CC3 },	// 92C0 E8B383 [賃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92C1, */ 0x93AE },	// 92C1 E98EAE [鎮]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92C2, */ 0x9673 },	// 92C2 E999B3 [陳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92C3, */ 0x6D25 },	// 92C3 E6B4A5 [津]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92C4, */ 0x589C },	// 92C4 E5A29C [墜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92C5, */ 0x690E },	// 92C5 E6A48E [椎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92C6, */ 0x69CC },	// 92C6 E6A78C [槌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92C7, */ 0x8FFD },	// 92C7 E8BFBD [追]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92C8, */ 0x939A },	// 92C8 E98E9A [鎚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92C9, */ 0x75DB },	// 92C9 E7979B [痛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92CA, */ 0x901A },	// 92CA E9809A [通]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92CB, */ 0x585A },	// 92CB E5A19A [塚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92CC, */ 0x6802 },	// 92CC E6A082 [栂]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92CD, */ 0x63B4 },	// 92CD E68EB4 [掴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92CE, */ 0x69FB },	// 92CE E6A7BB [槻]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92CF, */ 0x4F43 },	// 92CF E4BD83 [佃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92D0, */ 0x6F2C },	// 92D0 E6BCAC [漬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92D1, */ 0x67D8 },	// 92D1 E69F98 [柘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92D2, */ 0x8FBB },	// 92D2 E8BEBB [辻]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92D3, */ 0x8526 },	// 92D3 E894A6 [蔦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92D4, */ 0x7DB4 },	// 92D4 E7B6B4 [綴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92D5, */ 0x9354 },	// 92D5 E98D94 [鍔]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92D6, */ 0x693F },	// 92D6 E6A4BF [椿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92D7, */ 0x6F70 },	// 92D7 E6BDB0 [潰]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92D8, */ 0x576A },	// 92D8 E59DAA [坪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92D9, */ 0x58F7 },	// 92D9 E5A3B7 [壷]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92DA, */ 0x5B2C },	// 92DA E5ACAC [嬬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92DB, */ 0x7D2C },	// 92DB E7B4AC [紬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92DC, */ 0x722A },	// 92DC E788AA [爪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92DD, */ 0x540A },	// 92DD E5908A [吊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92DE, */ 0x91E3 },	// 92DE E987A3 [釣]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92DF, */ 0x9DB4 },	// 92DF E9B6B4 [鶴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92E0, */ 0x4EAD },	// 92E0 E4BAAD [亭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92E1, */ 0x4F4E },	// 92E1 E4BD8E [低]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92E2, */ 0x505C },	// 92E2 E5819C [停]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92E3, */ 0x5075 },	// 92E3 E581B5 [偵]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92E4, */ 0x5243 },	// 92E4 E58983 [剃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92E5, */ 0x8C9E },	// 92E5 E8B29E [貞]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92E6, */ 0x5448 },	// 92E6 E59188 [呈]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92E7, */ 0x5824 },	// 92E7 E5A0A4 [堤]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92E8, */ 0x5B9A },	// 92E8 E5AE9A [定]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92E9, */ 0x5E1D },	// 92E9 E5B89D [帝]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92EA, */ 0x5E95 },	// 92EA E5BA95 [底]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92EB, */ 0x5EAD },	// 92EB E5BAAD [庭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92EC, */ 0x5EF7 },	// 92EC E5BBB7 [廷]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92ED, */ 0x5F1F },	// 92ED E5BC9F [弟]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92EE, */ 0x608C },	// 92EE E6828C [悌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92EF, */ 0x62B5 },	// 92EF E68AB5 [抵]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92F0, */ 0x633A },	// 92F0 E68CBA [挺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92F1, */ 0x63D0 },	// 92F1 E68F90 [提]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92F2, */ 0x68AF },	// 92F2 E6A2AF [梯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92F3, */ 0x6C40 },	// 92F3 E6B180 [汀]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92F4, */ 0x7887 },	// 92F4 E7A287 [碇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92F5, */ 0x798E },	// 92F5 E7A68E [禎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92F6, */ 0x7A0B },	// 92F6 E7A88B [程]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92F7, */ 0x7DE0 },	// 92F7 E7B7A0 [締]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92F8, */ 0x8247 },	// 92F8 E88987 [艇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92F9, */ 0x8A02 },	// 92F9 E8A882 [訂]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92FA, */ 0x8AE6 },	// 92FA E8ABA6 [諦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92FB, */ 0x8E44 },	// 92FB E8B984 [蹄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x92FC, */ 0x9013 },	// 92FC E98093 [逓]  #CJK UNIFIED IDEOGRAPH
//===== 0x93 40～FC ( テーブル数 [189]  ）====================
{ /* 0x9340, */ 0x90B8 },	// 9340 E982B8 [邸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9341, */ 0x912D },	// 9341 E984AD [鄭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9342, */ 0x91D8 },	// 9342 E98798 [釘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9343, */ 0x9F0E },	// 9343 E9BC8E [鼎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9344, */ 0x6CE5 },	// 9344 E6B3A5 [泥]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9345, */ 0x6458 },	// 9345 E69198 [摘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9346, */ 0x64E2 },	// 9346 E693A2 [擢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9347, */ 0x6575 },	// 9347 E695B5 [敵]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9348, */ 0x6EF4 },	// 9348 E6BBB4 [滴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9349, */ 0x7684 },	// 9349 E79A84 [的]  #CJK UNIFIED IDEOGRAPH
{ /* 0x934A, */ 0x7B1B },	// 934A E7AC9B [笛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x934B, */ 0x9069 },	// 934B E981A9 [適]  #CJK UNIFIED IDEOGRAPH
{ /* 0x934C, */ 0x93D1 },	// 934C E98F91 [鏑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x934D, */ 0x6EBA },	// 934D E6BABA [溺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x934E, */ 0x54F2 },	// 934E E593B2 [哲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x934F, */ 0x5FB9 },	// 934F E5BEB9 [徹]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9350, */ 0x64A4 },	// 9350 E692A4 [撤]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9351, */ 0x8F4D },	// 9351 E8BD8D [轍]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9352, */ 0x8FED },	// 9352 E8BFAD [迭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9353, */ 0x9244 },	// 9353 E98984 [鉄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9354, */ 0x5178 },	// 9354 E585B8 [典]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9355, */ 0x586B },	// 9355 E5A1AB [填]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9356, */ 0x5929 },	// 9356 E5A4A9 [天]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9357, */ 0x5C55 },	// 9357 E5B195 [展]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9358, */ 0x5E97 },	// 9358 E5BA97 [店]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9359, */ 0x6DFB },	// 9359 E6B7BB [添]  #CJK UNIFIED IDEOGRAPH
{ /* 0x935A, */ 0x7E8F },	// 935A E7BA8F [纏]  #CJK UNIFIED IDEOGRAPH
{ /* 0x935B, */ 0x751C },	// 935B E7949C [甜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x935C, */ 0x8CBC },	// 935C E8B2BC [貼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x935D, */ 0x8EE2 },	// 935D E8BBA2 [転]  #CJK UNIFIED IDEOGRAPH
{ /* 0x935E, */ 0x985B },	// 935E E9A19B [顛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x935F, */ 0x70B9 },	// 935F E782B9 [点]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9360, */ 0x4F1D },	// 9360 E4BC9D [伝]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9361, */ 0x6BBF },	// 9361 E6AEBF [殿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9362, */ 0x6FB1 },	// 9362 E6BEB1 [澱]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9363, */ 0x7530 },	// 9363 E794B0 [田]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9364, */ 0x96FB },	// 9364 E99BBB [電]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9365, */ 0x514E },	// 9365 E5858E [兎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9366, */ 0x5410 },	// 9366 E59090 [吐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9367, */ 0x5835 },	// 9367 E5A0B5 [堵]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9368, */ 0x5857 },	// 9368 E5A197 [塗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9369, */ 0x59AC },	// 9369 E5A6AC [妬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x936A, */ 0x5C60 },	// 936A E5B1A0 [屠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x936B, */ 0x5F92 },	// 936B E5BE92 [徒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x936C, */ 0x6597 },	// 936C E69697 [斗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x936D, */ 0x675C },	// 936D E69D9C [杜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x936E, */ 0x6E21 },	// 936E E6B8A1 [渡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x936F, */ 0x767B },	// 936F E799BB [登]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9370, */ 0x83DF },	// 9370 E88F9F [菟]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9371, */ 0x8CED },	// 9371 E8B3AD [賭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9372, */ 0x9014 },	// 9372 E98094 [途]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9373, */ 0x90FD },	// 9373 E983BD [都]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9374, */ 0x934D },	// 9374 E98D8D [鍍]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9375, */ 0x7825 },	// 9375 E7A0A5 [砥]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9376, */ 0x783A },	// 9376 E7A0BA [砺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9377, */ 0x52AA },	// 9377 E58AAA [努]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9378, */ 0x5EA6 },	// 9378 E5BAA6 [度]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9379, */ 0x571F },	// 9379 E59C9F [土]  #CJK UNIFIED IDEOGRAPH
{ /* 0x937A, */ 0x5974 },	// 937A E5A5B4 [奴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x937B, */ 0x6012 },	// 937B E68092 [怒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x937C, */ 0x5012 },	// 937C E58092 [倒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x937D, */ 0x515A },	// 937D E5859A [党]  #CJK UNIFIED IDEOGRAPH
{ /* 0x937E, */ 0x51AC },	// 937E E586AC [冬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x937F, */ 0x0000 },	// 		　#ダミー
{ /* 0x9380, */ 0x51CD },	// 9380 E5878D [凍]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9381, */ 0x5200 },	// 9381 E58880 [刀]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9382, */ 0x5510 },	// 9382 E59490 [唐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9383, */ 0x5854 },	// 9383 E5A194 [塔]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9384, */ 0x5858 },	// 9384 E5A198 [塘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9385, */ 0x5957 },	// 9385 E5A597 [套]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9386, */ 0x5B95 },	// 9386 E5AE95 [宕]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9387, */ 0x5CF6 },	// 9387 E5B3B6 [島]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9388, */ 0x5D8B },	// 9388 E5B68B [嶋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9389, */ 0x60BC },	// 9389 E682BC [悼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x938A, */ 0x6295 },	// 938A E68A95 [投]  #CJK UNIFIED IDEOGRAPH
{ /* 0x938B, */ 0x642D },	// 938B E690AD [搭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x938C, */ 0x6771 },	// 938C E69DB1 [東]  #CJK UNIFIED IDEOGRAPH
{ /* 0x938D, */ 0x6843 },	// 938D E6A183 [桃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x938E, */ 0x68BC },	// 938E E6A2BC [梼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x938F, */ 0x68DF },	// 938F E6A39F [棟]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9390, */ 0x76D7 },	// 9390 E79B97 [盗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9391, */ 0x6DD8 },	// 9391 E6B798 [淘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9392, */ 0x6E6F },	// 9392 E6B9AF [湯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9393, */ 0x6D9B },	// 9393 E6B69B [涛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9394, */ 0x706F },	// 9394 E781AF [灯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9395, */ 0x71C8 },	// 9395 E78788 [燈]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9396, */ 0x5F53 },	// 9396 E5BD93 [当]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9397, */ 0x75D8 },	// 9397 E79798 [痘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9398, */ 0x7977 },	// 9398 E7A5B7 [祷]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9399, */ 0x7B49 },	// 9399 E7AD89 [等]  #CJK UNIFIED IDEOGRAPH
{ /* 0x939A, */ 0x7B54 },	// 939A E7AD94 [答]  #CJK UNIFIED IDEOGRAPH
{ /* 0x939B, */ 0x7B52 },	// 939B E7AD92 [筒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x939C, */ 0x7CD6 },	// 939C E7B396 [糖]  #CJK UNIFIED IDEOGRAPH
{ /* 0x939D, */ 0x7D71 },	// 939D E7B5B1 [統]  #CJK UNIFIED IDEOGRAPH
{ /* 0x939E, */ 0x5230 },	// 939E E588B0 [到]  #CJK UNIFIED IDEOGRAPH
{ /* 0x939F, */ 0x8463 },	// 939F E891A3 [董]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93A0, */ 0x8569 },	// 93A0 E895A9 [蕩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93A1, */ 0x85E4 },	// 93A1 E897A4 [藤]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93A2, */ 0x8A0E },	// 93A2 E8A88E [討]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93A3, */ 0x8B04 },	// 93A3 E8AC84 [謄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93A4, */ 0x8C46 },	// 93A4 E8B186 [豆]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93A5, */ 0x8E0F },	// 93A5 E8B88F [踏]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93A6, */ 0x9003 },	// 93A6 E98083 [逃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93A7, */ 0x900F },	// 93A7 E9808F [透]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93A8, */ 0x9419 },	// 93A8 E99099 [鐙]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93A9, */ 0x9676 },	// 93A9 E999B6 [陶]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93AA, */ 0x982D },	// 93AA E9A0AD [頭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93AB, */ 0x9A30 },	// 93AB E9A8B0 [騰]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93AC, */ 0x95D8 },	// 93AC E99798 [闘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93AD, */ 0x50CD },	// 93AD E5838D [働]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93AE, */ 0x52D5 },	// 93AE E58B95 [動]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93AF, */ 0x540C },	// 93AF E5908C [同]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93B0, */ 0x5802 },	// 93B0 E5A082 [堂]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93B1, */ 0x5C0E },	// 93B1 E5B08E [導]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93B2, */ 0x61A7 },	// 93B2 E686A7 [憧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93B3, */ 0x649E },	// 93B3 E6929E [撞]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93B4, */ 0x6D1E },	// 93B4 E6B49E [洞]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93B5, */ 0x77B3 },	// 93B5 E79EB3 [瞳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93B6, */ 0x7AE5 },	// 93B6 E7ABA5 [童]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93B7, */ 0x80F4 },	// 93B7 E883B4 [胴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93B8, */ 0x8404 },	// 93B8 E89084 [萄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93B9, */ 0x9053 },	// 93B9 E98193 [道]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93BA, */ 0x9285 },	// 93BA E98A85 [銅]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93BB, */ 0x5CE0 },	// 93BB E5B3A0 [峠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93BC, */ 0x9D07 },	// 93BC E9B487 [鴇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93BD, */ 0x533F },	// 93BD E58CBF [匿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93BE, */ 0x5F97 },	// 93BE E5BE97 [得]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93BF, */ 0x5FB3 },	// 93BF E5BEB3 [徳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93C0, */ 0x6D9C },	// 93C0 E6B69C [涜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93C1, */ 0x7279 },	// 93C1 E789B9 [特]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93C2, */ 0x7763 },	// 93C2 E79DA3 [督]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93C3, */ 0x79BF },	// 93C3 E7A6BF [禿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93C4, */ 0x7BE4 },	// 93C4 E7AFA4 [篤]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93C5, */ 0x6BD2 },	// 93C5 E6AF92 [毒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93C6, */ 0x72EC },	// 93C6 E78BAC [独]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93C7, */ 0x8AAD },	// 93C7 E8AAAD [読]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93C8, */ 0x6803 },	// 93C8 E6A083 [栃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93C9, */ 0x6A61 },	// 93C9 E6A9A1 [橡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93CA, */ 0x51F8 },	// 93CA E587B8 [凸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93CB, */ 0x7A81 },	// 93CB E7AA81 [突]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93CC, */ 0x6934 },	// 93CC E6A4B4 [椴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93CD, */ 0x5C4A },	// 93CD E5B18A [届]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93CE, */ 0x9CF6 },	// 93CE E9B3B6 [鳶]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93CF, */ 0x82EB },	// 93CF E88BAB [苫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93D0, */ 0x5BC5 },	// 93D0 E5AF85 [寅]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93D1, */ 0x9149 },	// 93D1 E98589 [酉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93D2, */ 0x701E },	// 93D2 E7809E [瀞]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93D3, */ 0x5678 },	// 93D3 E599B8 [噸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93D4, */ 0x5C6F },	// 93D4 E5B1AF [屯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93D5, */ 0x60C7 },	// 93D5 E68387 [惇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93D6, */ 0x6566 },	// 93D6 E695A6 [敦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93D7, */ 0x6C8C },	// 93D7 E6B28C [沌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93D8, */ 0x8C5A },	// 93D8 E8B19A [豚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93D9, */ 0x9041 },	// 93D9 E98181 [遁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93DA, */ 0x9813 },	// 93DA E9A093 [頓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93DB, */ 0x5451 },	// 93DB E59191 [呑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93DC, */ 0x66C7 },	// 93DC E69B87 [曇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93DD, */ 0x920D },	// 93DD E9888D [鈍]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93DE, */ 0x5948 },	// 93DE E5A588 [奈]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93DF, */ 0x90A3 },	// 93DF E982A3 [那]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93E0, */ 0x5185 },	// 93E0 E58685 [内]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93E1, */ 0x4E4D },	// 93E1 E4B98D [乍]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93E2, */ 0x51EA },	// 93E2 E587AA [凪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93E3, */ 0x8599 },	// 93E3 E89699 [薙]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93E4, */ 0x8B0E },	// 93E4 E8AC8E [謎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93E5, */ 0x7058 },	// 93E5 E78198 [灘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93E6, */ 0x637A },	// 93E6 E68DBA [捺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93E7, */ 0x934B },	// 93E7 E98D8B [鍋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93E8, */ 0x6962 },	// 93E8 E6A5A2 [楢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93E9, */ 0x99B4 },	// 93E9 E9A6B4 [馴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93EA, */ 0x7E04 },	// 93EA E7B884 [縄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93EB, */ 0x7577 },	// 93EB E795B7 [畷]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93EC, */ 0x5357 },	// 93EC E58D97 [南]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93ED, */ 0x6960 },	// 93ED E6A5A0 [楠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93EE, */ 0x8EDF },	// 93EE E8BB9F [軟]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93EF, */ 0x96E3 },	// 93EF E99BA3 [難]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93F0, */ 0x6C5D },	// 93F0 E6B19D [汝]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93F1, */ 0x4E8C },	// 93F1 E4BA8C [二]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93F2, */ 0x5C3C },	// 93F2 E5B0BC [尼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93F3, */ 0x5F10 },	// 93F3 E5BC90 [弐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93F4, */ 0x8FE9 },	// 93F4 E8BFA9 [迩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93F5, */ 0x5302 },	// 93F5 E58C82 [匂]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93F6, */ 0x8CD1 },	// 93F6 E8B391 [賑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93F7, */ 0x8089 },	// 93F7 E88289 [肉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93F8, */ 0x8679 },	// 93F8 E899B9 [虹]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93F9, */ 0x5EFF },	// 93F9 E5BBBF [廿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93FA, */ 0x65E5 },	// 93FA E697A5 [日]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93FB, */ 0x4E73 },	// 93FB E4B9B3 [乳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x93FC, */ 0x5165 },	// 93FC E585A5 [入]  #CJK UNIFIED IDEOGRAPH
//===== 0x94 40～FC ( テーブル数 [189]  ）====================
{ /* 0x9440, */ 0x5982 },	// 9440 E5A682 [如]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9441, */ 0x5C3F },	// 9441 E5B0BF [尿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9442, */ 0x97EE },	// 9442 E99FAE [韮]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9443, */ 0x4EFB },	// 9443 E4BBBB [任]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9444, */ 0x598A },	// 9444 E5A68A [妊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9445, */ 0x5FCD },	// 9445 E5BF8D [忍]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9446, */ 0x8A8D },	// 9446 E8AA8D [認]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9447, */ 0x6FE1 },	// 9447 E6BFA1 [濡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9448, */ 0x79B0 },	// 9448 E7A6B0 [禰]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9449, */ 0x7962 },	// 9449 E7A5A2 [祢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x944A, */ 0x5BE7 },	// 944A E5AFA7 [寧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x944B, */ 0x8471 },	// 944B E891B1 [葱]  #CJK UNIFIED IDEOGRAPH
{ /* 0x944C, */ 0x732B },	// 944C E78CAB [猫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x944D, */ 0x71B1 },	// 944D E786B1 [熱]  #CJK UNIFIED IDEOGRAPH
{ /* 0x944E, */ 0x5E74 },	// 944E E5B9B4 [年]  #CJK UNIFIED IDEOGRAPH
{ /* 0x944F, */ 0x5FF5 },	// 944F E5BFB5 [念]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9450, */ 0x637B },	// 9450 E68DBB [捻]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9451, */ 0x649A },	// 9451 E6929A [撚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9452, */ 0x71C3 },	// 9452 E78783 [燃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9453, */ 0x7C98 },	// 9453 E7B298 [粘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9454, */ 0x4E43 },	// 9454 E4B983 [乃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9455, */ 0x5EFC },	// 9455 E5BBBC [廼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9456, */ 0x4E4B },	// 9456 E4B98B [之]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9457, */ 0x57DC },	// 9457 E59F9C [埜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9458, */ 0x56A2 },	// 9458 E59AA2 [嚢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9459, */ 0x60A9 },	// 9459 E682A9 [悩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x945A, */ 0x6FC3 },	// 945A E6BF83 [濃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x945B, */ 0x7D0D },	// 945B E7B48D [納]  #CJK UNIFIED IDEOGRAPH
{ /* 0x945C, */ 0x80FD },	// 945C E883BD [能]  #CJK UNIFIED IDEOGRAPH
{ /* 0x945D, */ 0x8133 },	// 945D E884B3 [脳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x945E, */ 0x81BF },	// 945E E886BF [膿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x945F, */ 0x8FB2 },	// 945F E8BEB2 [農]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9460, */ 0x8997 },	// 9460 E8A697 [覗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9461, */ 0x86A4 },	// 9461 E89AA4 [蚤]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9462, */ 0x5DF4 },	// 9462 E5B7B4 [巴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9463, */ 0x628A },	// 9463 E68A8A [把]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9464, */ 0x64AD },	// 9464 E692AD [播]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9465, */ 0x8987 },	// 9465 E8A687 [覇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9466, */ 0x6777 },	// 9466 E69DB7 [杷]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9467, */ 0x6CE2 },	// 9467 E6B3A2 [波]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9468, */ 0x6D3E },	// 9468 E6B4BE [派]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9469, */ 0x7436 },	// 9469 E790B6 [琶]  #CJK UNIFIED IDEOGRAPH
{ /* 0x946A, */ 0x7834 },	// 946A E7A0B4 [破]  #CJK UNIFIED IDEOGRAPH
{ /* 0x946B, */ 0x5A46 },	// 946B E5A986 [婆]  #CJK UNIFIED IDEOGRAPH
{ /* 0x946C, */ 0x7F75 },	// 946C E7BDB5 [罵]  #CJK UNIFIED IDEOGRAPH
{ /* 0x946D, */ 0x82AD },	// 946D E88AAD [芭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x946E, */ 0x99AC },	// 946E E9A6AC [馬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x946F, */ 0x4FF3 },	// 946F E4BFB3 [俳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9470, */ 0x5EC3 },	// 9470 E5BB83 [廃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9471, */ 0x62DD },	// 9471 E68B9D [拝]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9472, */ 0x6392 },	// 9472 E68E92 [排]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9473, */ 0x6557 },	// 9473 E69597 [敗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9474, */ 0x676F },	// 9474 E69DAF [杯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9475, */ 0x76C3 },	// 9475 E79B83 [盃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9476, */ 0x724C },	// 9476 E7898C [牌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9477, */ 0x80CC },	// 9477 E8838C [背]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9478, */ 0x80BA },	// 9478 E882BA [肺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9479, */ 0x8F29 },	// 9479 E8BCA9 [輩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x947A, */ 0x914D },	// 947A E9858D [配]  #CJK UNIFIED IDEOGRAPH
{ /* 0x947B, */ 0x500D },	// 947B E5808D [倍]  #CJK UNIFIED IDEOGRAPH
{ /* 0x947C, */ 0x57F9 },	// 947C E59FB9 [培]  #CJK UNIFIED IDEOGRAPH
{ /* 0x947D, */ 0x5A92 },	// 947D E5AA92 [媒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x947E, */ 0x6885 },	// 947E E6A285 [梅]  #CJK UNIFIED IDEOGRAPH
{ /* 0x947F, */ 0x0000 },	// 		　#ダミー
{ /* 0x9480, */ 0x6973 },	// 9480 E6A5B3 [楳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9481, */ 0x7164 },	// 9481 E785A4 [煤]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9482, */ 0x72FD },	// 9482 E78BBD [狽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9483, */ 0x8CB7 },	// 9483 E8B2B7 [買]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9484, */ 0x58F2 },	// 9484 E5A3B2 [売]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9485, */ 0x8CE0 },	// 9485 E8B3A0 [賠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9486, */ 0x966A },	// 9486 E999AA [陪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9487, */ 0x9019 },	// 9487 E98099 [這]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9488, */ 0x877F },	// 9488 E89DBF [蝿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9489, */ 0x79E4 },	// 9489 E7A7A4 [秤]  #CJK UNIFIED IDEOGRAPH
{ /* 0x948A, */ 0x77E7 },	// 948A E79FA7 [矧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x948B, */ 0x8429 },	// 948B E890A9 [萩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x948C, */ 0x4F2F },	// 948C E4BCAF [伯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x948D, */ 0x5265 },	// 948D E589A5 [剥]  #CJK UNIFIED IDEOGRAPH
{ /* 0x948E, */ 0x535A },	// 948E E58D9A [博]  #CJK UNIFIED IDEOGRAPH
{ /* 0x948F, */ 0x62CD },	// 948F E68B8D [拍]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9490, */ 0x67CF },	// 9490 E69F8F [柏]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9491, */ 0x6CCA },	// 9491 E6B38A [泊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9492, */ 0x767D },	// 9492 E799BD [白]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9493, */ 0x7B94 },	// 9493 E7AE94 [箔]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9494, */ 0x7C95 },	// 9494 E7B295 [粕]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9495, */ 0x8236 },	// 9495 E888B6 [舶]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9496, */ 0x8584 },	// 9496 E89684 [薄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9497, */ 0x8FEB },	// 9497 E8BFAB [迫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9498, */ 0x66DD },	// 9498 E69B9D [曝]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9499, */ 0x6F20 },	// 9499 E6BCA0 [漠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x949A, */ 0x7206 },	// 949A E78886 [爆]  #CJK UNIFIED IDEOGRAPH
{ /* 0x949B, */ 0x7E1B },	// 949B E7B89B [縛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x949C, */ 0x83AB },	// 949C E88EAB [莫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x949D, */ 0x99C1 },	// 949D E9A781 [駁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x949E, */ 0x9EA6 },	// 949E E9BAA6 [麦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x949F, */ 0x51FD },	// 949F E587BD [函]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94A0, */ 0x7BB1 },	// 94A0 E7AEB1 [箱]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94A1, */ 0x7872 },	// 94A1 E7A1B2 [硲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94A2, */ 0x7BB8 },	// 94A2 E7AEB8 [箸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94A3, */ 0x8087 },	// 94A3 E88287 [肇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94A4, */ 0x7B48 },	// 94A4 E7AD88 [筈]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94A5, */ 0x6AE8 },	// 94A5 E6ABA8 [櫨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94A6, */ 0x5E61 },	// 94A6 E5B9A1 [幡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94A7, */ 0x808C },	// 94A7 E8828C [肌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94A8, */ 0x7551 },	// 94A8 E79591 [畑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94A9, */ 0x7560 },	// 94A9 E795A0 [畠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94AA, */ 0x516B },	// 94AA E585AB [八]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94AB, */ 0x9262 },	// 94AB E989A2 [鉢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94AC, */ 0x6E8C },	// 94AC E6BA8C [溌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94AD, */ 0x767A },	// 94AD E799BA [発]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94AE, */ 0x9197 },	// 94AE E98697 [醗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94AF, */ 0x9AEA },	// 94AF E9ABAA [髪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94B0, */ 0x4F10 },	// 94B0 E4BC90 [伐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94B1, */ 0x7F70 },	// 94B1 E7BDB0 [罰]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94B2, */ 0x629C },	// 94B2 E68A9C [抜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94B3, */ 0x7B4F },	// 94B3 E7AD8F [筏]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94B4, */ 0x95A5 },	// 94B4 E996A5 [閥]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94B5, */ 0x9CE9 },	// 94B5 E9B3A9 [鳩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94B6, */ 0x567A },	// 94B6 E599BA [噺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94B7, */ 0x5859 },	// 94B7 E5A199 [塙]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94B8, */ 0x86E4 },	// 94B8 E89BA4 [蛤]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94B9, */ 0x96BC },	// 94B9 E99ABC [隼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94BA, */ 0x4F34 },	// 94BA E4BCB4 [伴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94BB, */ 0x5224 },	// 94BB E588A4 [判]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94BC, */ 0x534A },	// 94BC E58D8A [半]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94BD, */ 0x53CD },	// 94BD E58F8D [反]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94BE, */ 0x53DB },	// 94BE E58F9B [叛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94BF, */ 0x5E06 },	// 94BF E5B886 [帆]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94C0, */ 0x642C },	// 94C0 E690AC [搬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94C1, */ 0x6591 },	// 94C1 E69691 [斑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94C2, */ 0x677F },	// 94C2 E69DBF [板]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94C3, */ 0x6C3E },	// 94C3 E6B0BE [氾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94C4, */ 0x6C4E },	// 94C4 E6B18E [汎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94C5, */ 0x7248 },	// 94C5 E78988 [版]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94C6, */ 0x72AF },	// 94C6 E78AAF [犯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94C7, */ 0x73ED },	// 94C7 E78FAD [班]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94C8, */ 0x7554 },	// 94C8 E79594 [畔]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94C9, */ 0x7E41 },	// 94C9 E7B981 [繁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94CA, */ 0x822C },	// 94CA E888AC [般]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94CB, */ 0x85E9 },	// 94CB E897A9 [藩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94CC, */ 0x8CA9 },	// 94CC E8B2A9 [販]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94CD, */ 0x7BC4 },	// 94CD E7AF84 [範]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94CE, */ 0x91C6 },	// 94CE E98786 [釆]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94CF, */ 0x7169 },	// 94CF E785A9 [煩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94D0, */ 0x9812 },	// 94D0 E9A092 [頒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94D1, */ 0x98EF },	// 94D1 E9A3AF [飯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94D2, */ 0x633D },	// 94D2 E68CBD [挽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94D3, */ 0x6669 },	// 94D3 E699A9 [晩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94D4, */ 0x756A },	// 94D4 E795AA [番]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94D5, */ 0x76E4 },	// 94D5 E79BA4 [盤]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94D6, */ 0x78D0 },	// 94D6 E7A390 [磐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94D7, */ 0x8543 },	// 94D7 E89583 [蕃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94D8, */ 0x86EE },	// 94D8 E89BAE [蛮]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94D9, */ 0x532A },	// 94D9 E58CAA [匪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94DA, */ 0x5351 },	// 94DA E58D91 [卑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94DB, */ 0x5426 },	// 94DB E590A6 [否]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94DC, */ 0x5983 },	// 94DC E5A683 [妃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94DD, */ 0x5E87 },	// 94DD E5BA87 [庇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94DE, */ 0x5F7C },	// 94DE E5BDBC [彼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94DF, */ 0x60B2 },	// 94DF E682B2 [悲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94E0, */ 0x6249 },	// 94E0 E68989 [扉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94E1, */ 0x6279 },	// 94E1 E689B9 [批]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94E2, */ 0x62AB },	// 94E2 E68AAB [披]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94E3, */ 0x6590 },	// 94E3 E69690 [斐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94E4, */ 0x6BD4 },	// 94E4 E6AF94 [比]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94E5, */ 0x6CCC },	// 94E5 E6B38C [泌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94E6, */ 0x75B2 },	// 94E6 E796B2 [疲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94E7, */ 0x76AE },	// 94E7 E79AAE [皮]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94E8, */ 0x7891 },	// 94E8 E7A291 [碑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94E9, */ 0x79D8 },	// 94E9 E7A798 [秘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94EA, */ 0x7DCB },	// 94EA E7B78B [緋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94EB, */ 0x7F77 },	// 94EB E7BDB7 [罷]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94EC, */ 0x80A5 },	// 94EC E882A5 [肥]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94ED, */ 0x88AB },	// 94ED E8A2AB [被]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94EE, */ 0x8AB9 },	// 94EE E8AAB9 [誹]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94EF, */ 0x8CBB },	// 94EF E8B2BB [費]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94F0, */ 0x907F },	// 94F0 E981BF [避]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94F1, */ 0x975E },	// 94F1 E99D9E [非]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94F2, */ 0x98DB },	// 94F2 E9A39B [飛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94F3, */ 0x6A0B },	// 94F3 E6A88B [樋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94F4, */ 0x7C38 },	// 94F4 E7B0B8 [簸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94F5, */ 0x5099 },	// 94F5 E58299 [備]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94F6, */ 0x5C3E },	// 94F6 E5B0BE [尾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94F7, */ 0x5FAE },	// 94F7 E5BEAE [微]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94F8, */ 0x6787 },	// 94F8 E69E87 [枇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94F9, */ 0x6BD8 },	// 94F9 E6AF98 [毘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94FA, */ 0x7435 },	// 94FA E790B5 [琵]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94FB, */ 0x7709 },	// 94FB E79C89 [眉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x94FC, */ 0x7F8E },	// 94FC E7BE8E [美]  #CJK UNIFIED IDEOGRAPH
//===== 0x95 40～FC ( テーブル数 [189]  ）====================
{ /* 0x9540, */ 0x9F3B },	// 9540 E9BCBB [鼻]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9541, */ 0x67CA },	// 9541 E69F8A [柊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9542, */ 0x7A17 },	// 9542 E7A897 [稗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9543, */ 0x5339 },	// 9543 E58CB9 [匹]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9544, */ 0x758B },	// 9544 E7968B [疋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9545, */ 0x9AED },	// 9545 E9ABAD [髭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9546, */ 0x5F66 },	// 9546 E5BDA6 [彦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9547, */ 0x819D },	// 9547 E8869D [膝]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9548, */ 0x83F1 },	// 9548 E88FB1 [菱]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9549, */ 0x8098 },	// 9549 E88298 [肘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x954A, */ 0x5F3C },	// 954A E5BCBC [弼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x954B, */ 0x5FC5 },	// 954B E5BF85 [必]  #CJK UNIFIED IDEOGRAPH
{ /* 0x954C, */ 0x7562 },	// 954C E795A2 [畢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x954D, */ 0x7B46 },	// 954D E7AD86 [筆]  #CJK UNIFIED IDEOGRAPH
{ /* 0x954E, */ 0x903C },	// 954E E980BC [逼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x954F, */ 0x6867 },	// 954F E6A1A7 [桧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9550, */ 0x59EB },	// 9550 E5A7AB [姫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9551, */ 0x5A9B },	// 9551 E5AA9B [媛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9552, */ 0x7D10 },	// 9552 E7B490 [紐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9553, */ 0x767E },	// 9553 E799BE [百]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9554, */ 0x8B2C },	// 9554 E8ACAC [謬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9555, */ 0x4FF5 },	// 9555 E4BFB5 [俵]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9556, */ 0x5F6A },	// 9556 E5BDAA [彪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9557, */ 0x6A19 },	// 9557 E6A899 [標]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9558, */ 0x6C37 },	// 9558 E6B0B7 [氷]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9559, */ 0x6F02 },	// 9559 E6BC82 [漂]  #CJK UNIFIED IDEOGRAPH
{ /* 0x955A, */ 0x74E2 },	// 955A E793A2 [瓢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x955B, */ 0x7968 },	// 955B E7A5A8 [票]  #CJK UNIFIED IDEOGRAPH
{ /* 0x955C, */ 0x8868 },	// 955C E8A1A8 [表]  #CJK UNIFIED IDEOGRAPH
{ /* 0x955D, */ 0x8A55 },	// 955D E8A995 [評]  #CJK UNIFIED IDEOGRAPH
{ /* 0x955E, */ 0x8C79 },	// 955E E8B1B9 [豹]  #CJK UNIFIED IDEOGRAPH
{ /* 0x955F, */ 0x5EDF },	// 955F E5BB9F [廟]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9560, */ 0x63CF },	// 9560 E68F8F [描]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9561, */ 0x75C5 },	// 9561 E79785 [病]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9562, */ 0x79D2 },	// 9562 E7A792 [秒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9563, */ 0x82D7 },	// 9563 E88B97 [苗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9564, */ 0x9328 },	// 9564 E98CA8 [錨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9565, */ 0x92F2 },	// 9565 E98BB2 [鋲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9566, */ 0x849C },	// 9566 E8929C [蒜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9567, */ 0x86ED },	// 9567 E89BAD [蛭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9568, */ 0x9C2D },	// 9568 E9B0AD [鰭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9569, */ 0x54C1 },	// 9569 E59381 [品]  #CJK UNIFIED IDEOGRAPH
{ /* 0x956A, */ 0x5F6C },	// 956A E5BDAC [彬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x956B, */ 0x658C },	// 956B E6968C [斌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x956C, */ 0x6D5C },	// 956C E6B59C [浜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x956D, */ 0x7015 },	// 956D E78095 [瀕]  #CJK UNIFIED IDEOGRAPH
{ /* 0x956E, */ 0x8CA7 },	// 956E E8B2A7 [貧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x956F, */ 0x8CD3 },	// 956F E8B393 [賓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9570, */ 0x983B },	// 9570 E9A0BB [頻]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9571, */ 0x654F },	// 9571 E6958F [敏]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9572, */ 0x74F6 },	// 9572 E793B6 [瓶]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9573, */ 0x4E0D },	// 9573 E4B88D [不]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9574, */ 0x4ED8 },	// 9574 E4BB98 [付]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9575, */ 0x57E0 },	// 9575 E59FA0 [埠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9576, */ 0x592B },	// 9576 E5A4AB [夫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9577, */ 0x5A66 },	// 9577 E5A9A6 [婦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9578, */ 0x5BCC },	// 9578 E5AF8C [富]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9579, */ 0x51A8 },	// 9579 E586A8 [冨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x957A, */ 0x5E03 },	// 957A E5B883 [布]  #CJK UNIFIED IDEOGRAPH
{ /* 0x957B, */ 0x5E9C },	// 957B E5BA9C [府]  #CJK UNIFIED IDEOGRAPH
{ /* 0x957C, */ 0x6016 },	// 957C E68096 [怖]  #CJK UNIFIED IDEOGRAPH
{ /* 0x957D, */ 0x6276 },	// 957D E689B6 [扶]  #CJK UNIFIED IDEOGRAPH
{ /* 0x957E, */ 0x6577 },	// 957E E695B7 [敷]  #CJK UNIFIED IDEOGRAPH
{ /* 0x957F, */ 0x0000 },	// 		　#ダミー
{ /* 0x9580, */ 0x65A7 },	// 9580 E696A7 [斧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9581, */ 0x666E },	// 9581 E699AE [普]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9582, */ 0x6D6E },	// 9582 E6B5AE [浮]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9583, */ 0x7236 },	// 9583 E788B6 [父]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9584, */ 0x7B26 },	// 9584 E7ACA6 [符]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9585, */ 0x8150 },	// 9585 E88590 [腐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9586, */ 0x819A },	// 9586 E8869A [膚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9587, */ 0x8299 },	// 9587 E88A99 [芙]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9588, */ 0x8B5C },	// 9588 E8AD9C [譜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9589, */ 0x8CA0 },	// 9589 E8B2A0 [負]  #CJK UNIFIED IDEOGRAPH
{ /* 0x958A, */ 0x8CE6 },	// 958A E8B3A6 [賦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x958B, */ 0x8D74 },	// 958B E8B5B4 [赴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x958C, */ 0x961C },	// 958C E9989C [阜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x958D, */ 0x9644 },	// 958D E99984 [附]  #CJK UNIFIED IDEOGRAPH
{ /* 0x958E, */ 0x4FAE },	// 958E E4BEAE [侮]  #CJK UNIFIED IDEOGRAPH
{ /* 0x958F, */ 0x64AB },	// 958F E692AB [撫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9590, */ 0x6B66 },	// 9590 E6ADA6 [武]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9591, */ 0x821E },	// 9591 E8889E [舞]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9592, */ 0x8461 },	// 9592 E891A1 [葡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9593, */ 0x856A },	// 9593 E895AA [蕪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9594, */ 0x90E8 },	// 9594 E983A8 [部]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9595, */ 0x5C01 },	// 9595 E5B081 [封]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9596, */ 0x6953 },	// 9596 E6A593 [楓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9597, */ 0x98A8 },	// 9597 E9A2A8 [風]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9598, */ 0x847A },	// 9598 E891BA [葺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9599, */ 0x8557 },	// 9599 E89597 [蕗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x959A, */ 0x4F0F },	// 959A E4BC8F [伏]  #CJK UNIFIED IDEOGRAPH
{ /* 0x959B, */ 0x526F },	// 959B E589AF [副]  #CJK UNIFIED IDEOGRAPH
{ /* 0x959C, */ 0x5FA9 },	// 959C E5BEA9 [復]  #CJK UNIFIED IDEOGRAPH
{ /* 0x959D, */ 0x5E45 },	// 959D E5B985 [幅]  #CJK UNIFIED IDEOGRAPH
{ /* 0x959E, */ 0x670D },	// 959E E69C8D [服]  #CJK UNIFIED IDEOGRAPH
{ /* 0x959F, */ 0x798F },	// 959F E7A68F [福]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95A0, */ 0x8179 },	// 95A0 E885B9 [腹]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95A1, */ 0x8907 },	// 95A1 E8A487 [複]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95A2, */ 0x8986 },	// 95A2 E8A686 [覆]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95A3, */ 0x6DF5 },	// 95A3 E6B7B5 [淵]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95A4, */ 0x5F17 },	// 95A4 E5BC97 [弗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95A5, */ 0x6255 },	// 95A5 E68995 [払]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95A6, */ 0x6CB8 },	// 95A6 E6B2B8 [沸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95A7, */ 0x4ECF },	// 95A7 E4BB8F [仏]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95A8, */ 0x7269 },	// 95A8 E789A9 [物]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95A9, */ 0x9B92 },	// 95A9 E9AE92 [鮒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95AA, */ 0x5206 },	// 95AA E58886 [分]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95AB, */ 0x543B },	// 95AB E590BB [吻]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95AC, */ 0x5674 },	// 95AC E599B4 [噴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95AD, */ 0x58B3 },	// 95AD E5A2B3 [墳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95AE, */ 0x61A4 },	// 95AE E686A4 [憤]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95AF, */ 0x626E },	// 95AF E689AE [扮]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95B0, */ 0x711A },	// 95B0 E7849A [焚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95B1, */ 0x596E },	// 95B1 E5A5AE [奮]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95B2, */ 0x7C89 },	// 95B2 E7B289 [粉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95B3, */ 0x7CDE },	// 95B3 E7B39E [糞]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95B4, */ 0x7D1B },	// 95B4 E7B49B [紛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95B5, */ 0x96F0 },	// 95B5 E99BB0 [雰]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95B6, */ 0x6587 },	// 95B6 E69687 [文]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95B7, */ 0x805E },	// 95B7 E8819E [聞]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95B8, */ 0x4E19 },	// 95B8 E4B899 [丙]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95B9, */ 0x4F75 },	// 95B9 E4BDB5 [併]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95BA, */ 0x5175 },	// 95BA E585B5 [兵]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95BB, */ 0x5840 },	// 95BB E5A180 [塀]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95BC, */ 0x5E63 },	// 95BC E5B9A3 [幣]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95BD, */ 0x5E73 },	// 95BD E5B9B3 [平]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95BE, */ 0x5F0A },	// 95BE E5BC8A [弊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95BF, */ 0x67C4 },	// 95BF E69F84 [柄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95C0, */ 0x4E26 },	// 95C0 E4B8A6 [並]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95C1, */ 0x853D },	// 95C1 E894BD [蔽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95C2, */ 0x9589 },	// 95C2 E99689 [閉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95C3, */ 0x965B },	// 95C3 E9999B [陛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95C4, */ 0x7C73 },	// 95C4 E7B1B3 [米]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95C5, */ 0x9801 },	// 95C5 E9A081 [頁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95C6, */ 0x50FB },	// 95C6 E583BB [僻]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95C7, */ 0x58C1 },	// 95C7 E5A381 [壁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95C8, */ 0x7656 },	// 95C8 E79996 [癖]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95C9, */ 0x78A7 },	// 95C9 E7A2A7 [碧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95CA, */ 0x5225 },	// 95CA E588A5 [別]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95CB, */ 0x77A5 },	// 95CB E79EA5 [瞥]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95CC, */ 0x8511 },	// 95CC E89491 [蔑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95CD, */ 0x7B86 },	// 95CD E7AE86 [箆]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95CE, */ 0x504F },	// 95CE E5818F [偏]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95CF, */ 0x5909 },	// 95CF E5A489 [変]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95D0, */ 0x7247 },	// 95D0 E78987 [片]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95D1, */ 0x7BC7 },	// 95D1 E7AF87 [篇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95D2, */ 0x7DE8 },	// 95D2 E7B7A8 [編]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95D3, */ 0x8FBA },	// 95D3 E8BEBA [辺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95D4, */ 0x8FD4 },	// 95D4 E8BF94 [返]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95D5, */ 0x904D },	// 95D5 E9818D [遍]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95D6, */ 0x4FBF },	// 95D6 E4BEBF [便]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95D7, */ 0x52C9 },	// 95D7 E58B89 [勉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95D8, */ 0x5A29 },	// 95D8 E5A8A9 [娩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95D9, */ 0x5F01 },	// 95D9 E5BC81 [弁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95DA, */ 0x97AD },	// 95DA E99EAD [鞭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95DB, */ 0x4FDD },	// 95DB E4BF9D [保]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95DC, */ 0x8217 },	// 95DC E88897 [舗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95DD, */ 0x92EA },	// 95DD E98BAA [鋪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95DE, */ 0x5703 },	// 95DE E59C83 [圃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95DF, */ 0x6355 },	// 95DF E68D95 [捕]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95E0, */ 0x6B69 },	// 95E0 E6ADA9 [歩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95E1, */ 0x752B },	// 95E1 E794AB [甫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95E2, */ 0x88DC },	// 95E2 E8A39C [補]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95E3, */ 0x8F14 },	// 95E3 E8BC94 [輔]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95E4, */ 0x7A42 },	// 95E4 E7A982 [穂]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95E5, */ 0x52DF },	// 95E5 E58B9F [募]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95E6, */ 0x5893 },	// 95E6 E5A293 [墓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95E7, */ 0x6155 },	// 95E7 E68595 [慕]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95E8, */ 0x620A },	// 95E8 E6888A [戊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95E9, */ 0x66AE },	// 95E9 E69AAE [暮]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95EA, */ 0x6BCD },	// 95EA E6AF8D [母]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95EB, */ 0x7C3F },	// 95EB E7B0BF [簿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95EC, */ 0x83E9 },	// 95EC E88FA9 [菩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95ED, */ 0x5023 },	// 95ED E580A3 [倣]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95EE, */ 0x4FF8 },	// 95EE E4BFB8 [俸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95EF, */ 0x5305 },	// 95EF E58C85 [包]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95F0, */ 0x5446 },	// 95F0 E59186 [呆]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95F1, */ 0x5831 },	// 95F1 E5A0B1 [報]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95F2, */ 0x5949 },	// 95F2 E5A589 [奉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95F3, */ 0x5B9D },	// 95F3 E5AE9D [宝]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95F4, */ 0x5CF0 },	// 95F4 E5B3B0 [峰]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95F5, */ 0x5CEF },	// 95F5 E5B3AF [峯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95F6, */ 0x5D29 },	// 95F6 E5B4A9 [崩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95F7, */ 0x5E96 },	// 95F7 E5BA96 [庖]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95F8, */ 0x62B1 },	// 95F8 E68AB1 [抱]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95F9, */ 0x6367 },	// 95F9 E68DA7 [捧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95FA, */ 0x653E },	// 95FA E694BE [放]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95FB, */ 0x65B9 },	// 95FB E696B9 [方]  #CJK UNIFIED IDEOGRAPH
{ /* 0x95FC, */ 0x670B },	// 95FC E69C8B [朋]  #CJK UNIFIED IDEOGRAPH
//===== 0x96 40～FC ( テーブル数 [189]  ）====================
{ /* 0x9640, */ 0x6CD5 },	// 9640 E6B395 [法]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9641, */ 0x6CE1 },	// 9641 E6B3A1 [泡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9642, */ 0x70F9 },	// 9642 E783B9 [烹]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9643, */ 0x7832 },	// 9643 E7A0B2 [砲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9644, */ 0x7E2B },	// 9644 E7B8AB [縫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9645, */ 0x80DE },	// 9645 E8839E [胞]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9646, */ 0x82B3 },	// 9646 E88AB3 [芳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9647, */ 0x840C },	// 9647 E8908C [萌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9648, */ 0x84EC },	// 9648 E893AC [蓬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9649, */ 0x8702 },	// 9649 E89C82 [蜂]  #CJK UNIFIED IDEOGRAPH
{ /* 0x964A, */ 0x8912 },	// 964A E8A492 [褒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x964B, */ 0x8A2A },	// 964B E8A8AA [訪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x964C, */ 0x8C4A },	// 964C E8B18A [豊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x964D, */ 0x90A6 },	// 964D E982A6 [邦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x964E, */ 0x92D2 },	// 964E E98B92 [鋒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x964F, */ 0x98FD },	// 964F E9A3BD [飽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9650, */ 0x9CF3 },	// 9650 E9B3B3 [鳳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9651, */ 0x9D6C },	// 9651 E9B5AC [鵬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9652, */ 0x4E4F },	// 9652 E4B98F [乏]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9653, */ 0x4EA1 },	// 9653 E4BAA1 [亡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9654, */ 0x508D },	// 9654 E5828D [傍]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9655, */ 0x5256 },	// 9655 E58996 [剖]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9656, */ 0x574A },	// 9656 E59D8A [坊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9657, */ 0x59A8 },	// 9657 E5A6A8 [妨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9658, */ 0x5E3D },	// 9658 E5B8BD [帽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9659, */ 0x5FD8 },	// 9659 E5BF98 [忘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x965A, */ 0x5FD9 },	// 965A E5BF99 [忙]  #CJK UNIFIED IDEOGRAPH
{ /* 0x965B, */ 0x623F },	// 965B E688BF [房]  #CJK UNIFIED IDEOGRAPH
{ /* 0x965C, */ 0x66B4 },	// 965C E69AB4 [暴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x965D, */ 0x671B },	// 965D E69C9B [望]  #CJK UNIFIED IDEOGRAPH
{ /* 0x965E, */ 0x67D0 },	// 965E E69F90 [某]  #CJK UNIFIED IDEOGRAPH
{ /* 0x965F, */ 0x68D2 },	// 965F E6A392 [棒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9660, */ 0x5192 },	// 9660 E58692 [冒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9661, */ 0x7D21 },	// 9661 E7B4A1 [紡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9662, */ 0x80AA },	// 9662 E882AA [肪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9663, */ 0x81A8 },	// 9663 E886A8 [膨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9664, */ 0x8B00 },	// 9664 E8AC80 [謀]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9665, */ 0x8C8C },	// 9665 E8B28C [貌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9666, */ 0x8CBF },	// 9666 E8B2BF [貿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9667, */ 0x927E },	// 9667 E989BE [鉾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9668, */ 0x9632 },	// 9668 E998B2 [防]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9669, */ 0x5420 },	// 9669 E590A0 [吠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x966A, */ 0x982C },	// 966A E9A0AC [頬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x966B, */ 0x5317 },	// 966B E58C97 [北]  #CJK UNIFIED IDEOGRAPH
{ /* 0x966C, */ 0x50D5 },	// 966C E58395 [僕]  #CJK UNIFIED IDEOGRAPH
{ /* 0x966D, */ 0x535C },	// 966D E58D9C [卜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x966E, */ 0x58A8 },	// 966E E5A2A8 [墨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x966F, */ 0x64B2 },	// 966F E692B2 [撲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9670, */ 0x6734 },	// 9670 E69CB4 [朴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9671, */ 0x7267 },	// 9671 E789A7 [牧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9672, */ 0x7766 },	// 9672 E79DA6 [睦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9673, */ 0x7A46 },	// 9673 E7A986 [穆]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9674, */ 0x91E6 },	// 9674 E987A6 [釦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9675, */ 0x52C3 },	// 9675 E58B83 [勃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9676, */ 0x6CA1 },	// 9676 E6B2A1 [没]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9677, */ 0x6B86 },	// 9677 E6AE86 [殆]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9678, */ 0x5800 },	// 9678 E5A080 [堀]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9679, */ 0x5E4C },	// 9679 E5B98C [幌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x967A, */ 0x5954 },	// 967A E5A594 [奔]  #CJK UNIFIED IDEOGRAPH
{ /* 0x967B, */ 0x672C },	// 967B E69CAC [本]  #CJK UNIFIED IDEOGRAPH
{ /* 0x967C, */ 0x7FFB },	// 967C E7BFBB [翻]  #CJK UNIFIED IDEOGRAPH
{ /* 0x967D, */ 0x51E1 },	// 967D E587A1 [凡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x967E, */ 0x76C6 },	// 967E E79B86 [盆]  #CJK UNIFIED IDEOGRAPH
{ /* 0x967F, */ 0x0000 },	// 		　#ダミー
{ /* 0x9680, */ 0x6469 },	// 9680 E691A9 [摩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9681, */ 0x78E8 },	// 9681 E7A3A8 [磨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9682, */ 0x9B54 },	// 9682 E9AD94 [魔]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9683, */ 0x9EBB },	// 9683 E9BABB [麻]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9684, */ 0x57CB },	// 9684 E59F8B [埋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9685, */ 0x59B9 },	// 9685 E5A6B9 [妹]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9686, */ 0x6627 },	// 9686 E698A7 [昧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9687, */ 0x679A },	// 9687 E69E9A [枚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9688, */ 0x6BCE },	// 9688 E6AF8E [毎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9689, */ 0x54E9 },	// 9689 E593A9 [哩]  #CJK UNIFIED IDEOGRAPH
{ /* 0x968A, */ 0x69D9 },	// 968A E6A799 [槙]  #CJK UNIFIED IDEOGRAPH
{ /* 0x968B, */ 0x5E55 },	// 968B E5B995 [幕]  #CJK UNIFIED IDEOGRAPH
{ /* 0x968C, */ 0x819C },	// 968C E8869C [膜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x968D, */ 0x6795 },	// 968D E69E95 [枕]  #CJK UNIFIED IDEOGRAPH
{ /* 0x968E, */ 0x9BAA },	// 968E E9AEAA [鮪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x968F, */ 0x67FE },	// 968F E69FBE [柾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9690, */ 0x9C52 },	// 9690 E9B192 [鱒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9691, */ 0x685D },	// 9691 E6A19D [桝]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9692, */ 0x4EA6 },	// 9692 E4BAA6 [亦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9693, */ 0x4FE3 },	// 9693 E4BFA3 [俣]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9694, */ 0x53C8 },	// 9694 E58F88 [又]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9695, */ 0x62B9 },	// 9695 E68AB9 [抹]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9696, */ 0x672B },	// 9696 E69CAB [末]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9697, */ 0x6CAB },	// 9697 E6B2AB [沫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9698, */ 0x8FC4 },	// 9698 E8BF84 [迄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9699, */ 0x4FAD },	// 9699 E4BEAD [侭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x969A, */ 0x7E6D },	// 969A E7B9AD [繭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x969B, */ 0x9EBF },	// 969B E9BABF [麿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x969C, */ 0x4E07 },	// 969C E4B887 [万]  #CJK UNIFIED IDEOGRAPH
{ /* 0x969D, */ 0x6162 },	// 969D E685A2 [慢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x969E, */ 0x6E80 },	// 969E E6BA80 [満]  #CJK UNIFIED IDEOGRAPH
{ /* 0x969F, */ 0x6F2B },	// 969F E6BCAB [漫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96A0, */ 0x8513 },	// 96A0 E89493 [蔓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96A1, */ 0x5473 },	// 96A1 E591B3 [味]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96A2, */ 0x672A },	// 96A2 E69CAA [未]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96A3, */ 0x9B45 },	// 96A3 E9AD85 [魅]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96A4, */ 0x5DF3 },	// 96A4 E5B7B3 [巳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96A5, */ 0x7B95 },	// 96A5 E7AE95 [箕]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96A6, */ 0x5CAC },	// 96A6 E5B2AC [岬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96A7, */ 0x5BC6 },	// 96A7 E5AF86 [密]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96A8, */ 0x871C },	// 96A8 E89C9C [蜜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96A9, */ 0x6E4A },	// 96A9 E6B98A [湊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96AA, */ 0x84D1 },	// 96AA E89391 [蓑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96AB, */ 0x7A14 },	// 96AB E7A894 [稔]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96AC, */ 0x8108 },	// 96AC E88488 [脈]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96AD, */ 0x5999 },	// 96AD E5A699 [妙]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96AE, */ 0x7C8D },	// 96AE E7B28D [粍]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96AF, */ 0x6C11 },	// 96AF E6B091 [民]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96B0, */ 0x7720 },	// 96B0 E79CA0 [眠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96B1, */ 0x52D9 },	// 96B1 E58B99 [務]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96B2, */ 0x5922 },	// 96B2 E5A4A2 [夢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96B3, */ 0x7121 },	// 96B3 E784A1 [無]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96B4, */ 0x725F },	// 96B4 E7899F [牟]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96B5, */ 0x77DB },	// 96B5 E79F9B [矛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96B6, */ 0x9727 },	// 96B6 E99CA7 [霧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96B7, */ 0x9D61 },	// 96B7 E9B5A1 [鵡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96B8, */ 0x690B },	// 96B8 E6A48B [椋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96B9, */ 0x5A7F },	// 96B9 E5A9BF [婿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96BA, */ 0x5A18 },	// 96BA E5A898 [娘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96BB, */ 0x51A5 },	// 96BB E586A5 [冥]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96BC, */ 0x540D },	// 96BC E5908D [名]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96BD, */ 0x547D },	// 96BD E591BD [命]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96BE, */ 0x660E },	// 96BE E6988E [明]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96BF, */ 0x76DF },	// 96BF E79B9F [盟]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96C0, */ 0x8FF7 },	// 96C0 E8BFB7 [迷]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96C1, */ 0x9298 },	// 96C1 E98A98 [銘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96C2, */ 0x9CF4 },	// 96C2 E9B3B4 [鳴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96C3, */ 0x59EA },	// 96C3 E5A7AA [姪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96C4, */ 0x725D },	// 96C4 E7899D [牝]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96C5, */ 0x6EC5 },	// 96C5 E6BB85 [滅]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96C6, */ 0x514D },	// 96C6 E5858D [免]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96C7, */ 0x68C9 },	// 96C7 E6A389 [棉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96C8, */ 0x7DBF },	// 96C8 E7B6BF [綿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96C9, */ 0x7DEC },	// 96C9 E7B7AC [緬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96CA, */ 0x9762 },	// 96CA E99DA2 [面]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96CB, */ 0x9EBA },	// 96CB E9BABA [麺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96CC, */ 0x6478 },	// 96CC E691B8 [摸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96CD, */ 0x6A21 },	// 96CD E6A8A1 [模]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96CE, */ 0x8302 },	// 96CE E88C82 [茂]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96CF, */ 0x5984 },	// 96CF E5A684 [妄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96D0, */ 0x5B5F },	// 96D0 E5AD9F [孟]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96D1, */ 0x6BDB },	// 96D1 E6AF9B [毛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96D2, */ 0x731B },	// 96D2 E78C9B [猛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96D3, */ 0x76F2 },	// 96D3 E79BB2 [盲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96D4, */ 0x7DB2 },	// 96D4 E7B6B2 [網]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96D5, */ 0x8017 },	// 96D5 E88097 [耗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96D6, */ 0x8499 },	// 96D6 E89299 [蒙]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96D7, */ 0x5132 },	// 96D7 E584B2 [儲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96D8, */ 0x6728 },	// 96D8 E69CA8 [木]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96D9, */ 0x9ED9 },	// 96D9 E9BB99 [黙]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96DA, */ 0x76EE },	// 96DA E79BAE [目]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96DB, */ 0x6762 },	// 96DB E69DA2 [杢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96DC, */ 0x52FF },	// 96DC E58BBF [勿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96DD, */ 0x9905 },	// 96DD E9A485 [餅]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96DE, */ 0x5C24 },	// 96DE E5B0A4 [尤]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96DF, */ 0x623B },	// 96DF E688BB [戻]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96E0, */ 0x7C7E },	// 96E0 E7B1BE [籾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96E1, */ 0x8CB0 },	// 96E1 E8B2B0 [貰]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96E2, */ 0x554F },	// 96E2 E5958F [問]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96E3, */ 0x60B6 },	// 96E3 E682B6 [悶]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96E4, */ 0x7D0B },	// 96E4 E7B48B [紋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96E5, */ 0x9580 },	// 96E5 E99680 [門]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96E6, */ 0x5301 },	// 96E6 E58C81 [匁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96E7, */ 0x4E5F },	// 96E7 E4B99F [也]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96E8, */ 0x51B6 },	// 96E8 E586B6 [冶]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96E9, */ 0x591C },	// 96E9 E5A49C [夜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96EA, */ 0x723A },	// 96EA E788BA [爺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96EB, */ 0x8036 },	// 96EB E880B6 [耶]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96EC, */ 0x91CE },	// 96EC E9878E [野]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96ED, */ 0x5F25 },	// 96ED E5BCA5 [弥]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96EE, */ 0x77E2 },	// 96EE E79FA2 [矢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96EF, */ 0x5384 },	// 96EF E58E84 [厄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96F0, */ 0x5F79 },	// 96F0 E5BDB9 [役]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96F1, */ 0x7D04 },	// 96F1 E7B484 [約]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96F2, */ 0x85AC },	// 96F2 E896AC [薬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96F3, */ 0x8A33 },	// 96F3 E8A8B3 [訳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96F4, */ 0x8E8D },	// 96F4 E8BA8D [躍]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96F5, */ 0x9756 },	// 96F5 E99D96 [靖]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96F6, */ 0x67F3 },	// 96F6 E69FB3 [柳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96F7, */ 0x85AE },	// 96F7 E896AE [薮]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96F8, */ 0x9453 },	// 96F8 E99193 [鑓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96F9, */ 0x6109 },	// 96F9 E68489 [愉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96FA, */ 0x6108 },	// 96FA E68488 [愈]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96FB, */ 0x6CB9 },	// 96FB E6B2B9 [油]  #CJK UNIFIED IDEOGRAPH
{ /* 0x96FC, */ 0x7652 },	// 96FC E79992 [癒]  #CJK UNIFIED IDEOGRAPH
//===== 0x97 40～FC ( テーブル数 [189]  ）====================
{ /* 0x9740, */ 0x8AED },	// 9740 E8ABAD [諭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9741, */ 0x8F38 },	// 9741 E8BCB8 [輸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9742, */ 0x552F },	// 9742 E594AF [唯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9743, */ 0x4F51 },	// 9743 E4BD91 [佑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9744, */ 0x512A },	// 9744 E584AA [優]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9745, */ 0x52C7 },	// 9745 E58B87 [勇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9746, */ 0x53CB },	// 9746 E58F8B [友]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9747, */ 0x5BA5 },	// 9747 E5AEA5 [宥]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9748, */ 0x5E7D },	// 9748 E5B9BD [幽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9749, */ 0x60A0 },	// 9749 E682A0 [悠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x974A, */ 0x6182 },	// 974A E68682 [憂]  #CJK UNIFIED IDEOGRAPH
{ /* 0x974B, */ 0x63D6 },	// 974B E68F96 [揖]  #CJK UNIFIED IDEOGRAPH
{ /* 0x974C, */ 0x6709 },	// 974C E69C89 [有]  #CJK UNIFIED IDEOGRAPH
{ /* 0x974D, */ 0x67DA },	// 974D E69F9A [柚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x974E, */ 0x6E67 },	// 974E E6B9A7 [湧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x974F, */ 0x6D8C },	// 974F E6B68C [涌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9750, */ 0x7336 },	// 9750 E78CB6 [猶]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9751, */ 0x7337 },	// 9751 E78CB7 [猷]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9752, */ 0x7531 },	// 9752 E794B1 [由]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9753, */ 0x7950 },	// 9753 E7A590 [祐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9754, */ 0x88D5 },	// 9754 E8A395 [裕]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9755, */ 0x8A98 },	// 9755 E8AA98 [誘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9756, */ 0x904A },	// 9756 E9818A [遊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9757, */ 0x9091 },	// 9757 E98291 [邑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9758, */ 0x90F5 },	// 9758 E983B5 [郵]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9759, */ 0x96C4 },	// 9759 E99B84 [雄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x975A, */ 0x878D },	// 975A E89E8D [融]  #CJK UNIFIED IDEOGRAPH
{ /* 0x975B, */ 0x5915 },	// 975B E5A495 [夕]  #CJK UNIFIED IDEOGRAPH
{ /* 0x975C, */ 0x4E88 },	// 975C E4BA88 [予]  #CJK UNIFIED IDEOGRAPH
{ /* 0x975D, */ 0x4F59 },	// 975D E4BD99 [余]  #CJK UNIFIED IDEOGRAPH
{ /* 0x975E, */ 0x4E0E },	// 975E E4B88E [与]  #CJK UNIFIED IDEOGRAPH
{ /* 0x975F, */ 0x8A89 },	// 975F E8AA89 [誉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9760, */ 0x8F3F },	// 9760 E8BCBF [輿]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9761, */ 0x9810 },	// 9761 E9A090 [預]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9762, */ 0x50AD },	// 9762 E582AD [傭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9763, */ 0x5E7C },	// 9763 E5B9BC [幼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9764, */ 0x5996 },	// 9764 E5A696 [妖]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9765, */ 0x5BB9 },	// 9765 E5AEB9 [容]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9766, */ 0x5EB8 },	// 9766 E5BAB8 [庸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9767, */ 0x63DA },	// 9767 E68F9A [揚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9768, */ 0x63FA },	// 9768 E68FBA [揺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9769, */ 0x64C1 },	// 9769 E69381 [擁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x976A, */ 0x66DC },	// 976A E69B9C [曜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x976B, */ 0x694A },	// 976B E6A58A [楊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x976C, */ 0x69D8 },	// 976C E6A798 [様]  #CJK UNIFIED IDEOGRAPH
{ /* 0x976D, */ 0x6D0B },	// 976D E6B48B [洋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x976E, */ 0x6EB6 },	// 976E E6BAB6 [溶]  #CJK UNIFIED IDEOGRAPH
{ /* 0x976F, */ 0x7194 },	// 976F E78694 [熔]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9770, */ 0x7528 },	// 9770 E794A8 [用]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9771, */ 0x7AAF },	// 9771 E7AAAF [窯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9772, */ 0x7F8A },	// 9772 E7BE8A [羊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9773, */ 0x8000 },	// 9773 E88080 [耀]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9774, */ 0x8449 },	// 9774 E89189 [葉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9775, */ 0x84C9 },	// 9775 E89389 [蓉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9776, */ 0x8981 },	// 9776 E8A681 [要]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9777, */ 0x8B21 },	// 9777 E8ACA1 [謡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9778, */ 0x8E0A },	// 9778 E8B88A [踊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9779, */ 0x9065 },	// 9779 E981A5 [遥]  #CJK UNIFIED IDEOGRAPH
{ /* 0x977A, */ 0x967D },	// 977A E999BD [陽]  #CJK UNIFIED IDEOGRAPH
{ /* 0x977B, */ 0x990A },	// 977B E9A48A [養]  #CJK UNIFIED IDEOGRAPH
{ /* 0x977C, */ 0x617E },	// 977C E685BE [慾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x977D, */ 0x6291 },	// 977D E68A91 [抑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x977E, */ 0x6B32 },	// 977E E6ACB2 [欲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x977F, */ 0x0000 },	// 		　#ダミー
{ /* 0x9780, */ 0x6C83 },	// 9780 E6B283 [沃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9781, */ 0x6D74 },	// 9781 E6B5B4 [浴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9782, */ 0x7FCC },	// 9782 E7BF8C [翌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9783, */ 0x7FFC },	// 9783 E7BFBC [翼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9784, */ 0x6DC0 },	// 9784 E6B780 [淀]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9785, */ 0x7F85 },	// 9785 E7BE85 [羅]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9786, */ 0x87BA },	// 9786 E89EBA [螺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9787, */ 0x88F8 },	// 9787 E8A3B8 [裸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9788, */ 0x6765 },	// 9788 E69DA5 [来]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9789, */ 0x83B1 },	// 9789 E88EB1 [莱]  #CJK UNIFIED IDEOGRAPH
{ /* 0x978A, */ 0x983C },	// 978A E9A0BC [頼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x978B, */ 0x96F7 },	// 978B E99BB7 [雷]  #CJK UNIFIED IDEOGRAPH
{ /* 0x978C, */ 0x6D1B },	// 978C E6B49B [洛]  #CJK UNIFIED IDEOGRAPH
{ /* 0x978D, */ 0x7D61 },	// 978D E7B5A1 [絡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x978E, */ 0x843D },	// 978E E890BD [落]  #CJK UNIFIED IDEOGRAPH
{ /* 0x978F, */ 0x916A },	// 978F E985AA [酪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9790, */ 0x4E71 },	// 9790 E4B9B1 [乱]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9791, */ 0x5375 },	// 9791 E58DB5 [卵]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9792, */ 0x5D50 },	// 9792 E5B590 [嵐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9793, */ 0x6B04 },	// 9793 E6AC84 [欄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9794, */ 0x6FEB },	// 9794 E6BFAB [濫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9795, */ 0x85CD },	// 9795 E8978D [藍]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9796, */ 0x862D },	// 9796 E898AD [蘭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9797, */ 0x89A7 },	// 9797 E8A6A7 [覧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9798, */ 0x5229 },	// 9798 E588A9 [利]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9799, */ 0x540F },	// 9799 E5908F [吏]  #CJK UNIFIED IDEOGRAPH
{ /* 0x979A, */ 0x5C65 },	// 979A E5B1A5 [履]  #CJK UNIFIED IDEOGRAPH
{ /* 0x979B, */ 0x674E },	// 979B E69D8E [李]  #CJK UNIFIED IDEOGRAPH
{ /* 0x979C, */ 0x68A8 },	// 979C E6A2A8 [梨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x979D, */ 0x7406 },	// 979D E79086 [理]  #CJK UNIFIED IDEOGRAPH
{ /* 0x979E, */ 0x7483 },	// 979E E79283 [璃]  #CJK UNIFIED IDEOGRAPH
{ /* 0x979F, */ 0x75E2 },	// 979F E797A2 [痢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97A0, */ 0x88CF },	// 97A0 E8A38F [裏]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97A1, */ 0x88E1 },	// 97A1 E8A3A1 [裡]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97A2, */ 0x91CC },	// 97A2 E9878C [里]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97A3, */ 0x96E2 },	// 97A3 E99BA2 [離]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97A4, */ 0x9678 },	// 97A4 E999B8 [陸]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97A5, */ 0x5F8B },	// 97A5 E5BE8B [律]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97A6, */ 0x7387 },	// 97A6 E78E87 [率]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97A7, */ 0x7ACB },	// 97A7 E7AB8B [立]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97A8, */ 0x844E },	// 97A8 E8918E [葎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97A9, */ 0x63A0 },	// 97A9 E68EA0 [掠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97AA, */ 0x7565 },	// 97AA E795A5 [略]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97AB, */ 0x5289 },	// 97AB E58A89 [劉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97AC, */ 0x6D41 },	// 97AC E6B581 [流]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97AD, */ 0x6E9C },	// 97AD E6BA9C [溜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97AE, */ 0x7409 },	// 97AE E79089 [琉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97AF, */ 0x7559 },	// 97AF E79599 [留]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97B0, */ 0x786B },	// 97B0 E7A1AB [硫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97B1, */ 0x7C92 },	// 97B1 E7B292 [粒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97B2, */ 0x9686 },	// 97B2 E99A86 [隆]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97B3, */ 0x7ADC },	// 97B3 E7AB9C [竜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97B4, */ 0x9F8D },	// 97B4 E9BE8D [龍]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97B5, */ 0x4FB6 },	// 97B5 E4BEB6 [侶]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97B6, */ 0x616E },	// 97B6 E685AE [慮]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97B7, */ 0x65C5 },	// 97B7 E69785 [旅]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97B8, */ 0x865C },	// 97B8 E8999C [虜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97B9, */ 0x4E86 },	// 97B9 E4BA86 [了]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97BA, */ 0x4EAE },	// 97BA E4BAAE [亮]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97BB, */ 0x50DA },	// 97BB E5839A [僚]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97BC, */ 0x4E21 },	// 97BC E4B8A1 [両]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97BD, */ 0x51CC },	// 97BD E5878C [凌]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97BE, */ 0x5BEE },	// 97BE E5AFAE [寮]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97BF, */ 0x6599 },	// 97BF E69699 [料]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97C0, */ 0x6881 },	// 97C0 E6A281 [梁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97C1, */ 0x6DBC },	// 97C1 E6B6BC [涼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97C2, */ 0x731F },	// 97C2 E78C9F [猟]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97C3, */ 0x7642 },	// 97C3 E79982 [療]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97C4, */ 0x77AD },	// 97C4 E79EAD [瞭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97C5, */ 0x7A1C },	// 97C5 E7A89C [稜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97C6, */ 0x7CE7 },	// 97C6 E7B3A7 [糧]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97C7, */ 0x826F },	// 97C7 E889AF [良]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97C8, */ 0x8AD2 },	// 97C8 E8AB92 [諒]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97C9, */ 0x907C },	// 97C9 E981BC [遼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97CA, */ 0x91CF },	// 97CA E9878F [量]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97CB, */ 0x9675 },	// 97CB E999B5 [陵]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97CC, */ 0x9818 },	// 97CC E9A098 [領]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97CD, */ 0x529B },	// 97CD E58A9B [力]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97CE, */ 0x7DD1 },	// 97CE E7B791 [緑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97CF, */ 0x502B },	// 97CF E580AB [倫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97D0, */ 0x5398 },	// 97D0 E58E98 [厘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97D1, */ 0x6797 },	// 97D1 E69E97 [林]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97D2, */ 0x6DCB },	// 97D2 E6B78B [淋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97D3, */ 0x71D0 },	// 97D3 E78790 [燐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97D4, */ 0x7433 },	// 97D4 E790B3 [琳]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97D5, */ 0x81E8 },	// 97D5 E887A8 [臨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97D6, */ 0x8F2A },	// 97D6 E8BCAA [輪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97D7, */ 0x96A3 },	// 97D7 E99AA3 [隣]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97D8, */ 0x9C57 },	// 97D8 E9B197 [鱗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97D9, */ 0x9E9F },	// 97D9 E9BA9F [麟]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97DA, */ 0x7460 },	// 97DA E791A0 [瑠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97DB, */ 0x5841 },	// 97DB E5A181 [塁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97DC, */ 0x6D99 },	// 97DC E6B699 [涙]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97DD, */ 0x7D2F },	// 97DD E7B4AF [累]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97DE, */ 0x985E },	// 97DE E9A19E [類]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97DF, */ 0x4EE4 },	// 97DF E4BBA4 [令]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97E0, */ 0x4F36 },	// 97E0 E4BCB6 [伶]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97E1, */ 0x4F8B },	// 97E1 E4BE8B [例]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97E2, */ 0x51B7 },	// 97E2 E586B7 [冷]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97E3, */ 0x52B1 },	// 97E3 E58AB1 [励]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97E4, */ 0x5DBA },	// 97E4 E5B6BA [嶺]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97E5, */ 0x601C },	// 97E5 E6809C [怜]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97E6, */ 0x73B2 },	// 97E6 E78EB2 [玲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97E7, */ 0x793C },	// 97E7 E7A4BC [礼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97E8, */ 0x82D3 },	// 97E8 E88B93 [苓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97E9, */ 0x9234 },	// 97E9 E988B4 [鈴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97EA, */ 0x96B7 },	// 97EA E99AB7 [隷]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97EB, */ 0x96F6 },	// 97EB E99BB6 [零]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97EC, */ 0x970A },	// 97EC E99C8A [霊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97ED, */ 0x9E97 },	// 97ED E9BA97 [麗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97EE, */ 0x9F62 },	// 97EE E9BDA2 [齢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97EF, */ 0x66A6 },	// 97EF E69AA6 [暦]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97F0, */ 0x6B74 },	// 97F0 E6ADB4 [歴]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97F1, */ 0x5217 },	// 97F1 E58897 [列]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97F2, */ 0x52A3 },	// 97F2 E58AA3 [劣]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97F3, */ 0x70C8 },	// 97F3 E78388 [烈]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97F4, */ 0x88C2 },	// 97F4 E8A382 [裂]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97F5, */ 0x5EC9 },	// 97F5 E5BB89 [廉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97F6, */ 0x604B },	// 97F6 E6818B [恋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97F7, */ 0x6190 },	// 97F7 E68690 [憐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97F8, */ 0x6F23 },	// 97F8 E6BCA3 [漣]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97F9, */ 0x7149 },	// 97F9 E78589 [煉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97FA, */ 0x7C3E },	// 97FA E7B0BE [簾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97FB, */ 0x7DF4 },	// 97FB E7B7B4 [練]  #CJK UNIFIED IDEOGRAPH
{ /* 0x97FC, */ 0x806F },	// 97FC E881AF [聯]  #CJK UNIFIED IDEOGRAPH
//===== 0x98 40～FC ( テーブル数 [189]  ）====================
{ /* 0x9840, */ 0x84EE },	// 9840 E893AE [蓮]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9841, */ 0x9023 },	// 9841 E980A3 [連]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9842, */ 0x932C },	// 9842 E98CAC [錬]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9843, */ 0x5442 },	// 9843 E59182 [呂]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9844, */ 0x9B6F },	// 9844 E9ADAF [魯]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9845, */ 0x6AD3 },	// 9845 E6AB93 [櫓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9846, */ 0x7089 },	// 9846 E78289 [炉]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9847, */ 0x8CC2 },	// 9847 E8B382 [賂]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9848, */ 0x8DEF },	// 9848 E8B7AF [路]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9849, */ 0x9732 },	// 9849 E99CB2 [露]  #CJK UNIFIED IDEOGRAPH
{ /* 0x984A, */ 0x52B4 },	// 984A E58AB4 [労]  #CJK UNIFIED IDEOGRAPH
{ /* 0x984B, */ 0x5A41 },	// 984B E5A981 [婁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x984C, */ 0x5ECA },	// 984C E5BB8A [廊]  #CJK UNIFIED IDEOGRAPH
{ /* 0x984D, */ 0x5F04 },	// 984D E5BC84 [弄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x984E, */ 0x6717 },	// 984E E69C97 [朗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x984F, */ 0x697C },	// 984F E6A5BC [楼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9850, */ 0x6994 },	// 9850 E6A694 [榔]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9851, */ 0x6D6A },	// 9851 E6B5AA [浪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9852, */ 0x6F0F },	// 9852 E6BC8F [漏]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9853, */ 0x7262 },	// 9853 E789A2 [牢]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9854, */ 0x72FC },	// 9854 E78BBC [狼]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9855, */ 0x7BED },	// 9855 E7AFAD [篭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9856, */ 0x8001 },	// 9856 E88081 [老]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9857, */ 0x807E },	// 9857 E881BE [聾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9858, */ 0x874B },	// 9858 E89D8B [蝋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9859, */ 0x90CE },	// 9859 E9838E [郎]  #CJK UNIFIED IDEOGRAPH
{ /* 0x985A, */ 0x516D },	// 985A E585AD [六]  #CJK UNIFIED IDEOGRAPH
{ /* 0x985B, */ 0x9E93 },	// 985B E9BA93 [麓]  #CJK UNIFIED IDEOGRAPH
{ /* 0x985C, */ 0x7984 },	// 985C E7A684 [禄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x985D, */ 0x808B },	// 985D E8828B [肋]  #CJK UNIFIED IDEOGRAPH
{ /* 0x985E, */ 0x9332 },	// 985E E98CB2 [録]  #CJK UNIFIED IDEOGRAPH
{ /* 0x985F, */ 0x8AD6 },	// 985F E8AB96 [論]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9860, */ 0x502D },	// 9860 E580AD [倭]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9861, */ 0x548C },	// 9861 E5928C [和]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9862, */ 0x8A71 },	// 9862 E8A9B1 [話]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9863, */ 0x6B6A },	// 9863 E6ADAA [歪]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9864, */ 0x8CC4 },	// 9864 E8B384 [賄]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9865, */ 0x8107 },	// 9865 E88487 [脇]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9866, */ 0x60D1 },	// 9866 E68391 [惑]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9867, */ 0x67A0 },	// 9867 E69EA0 [枠]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9868, */ 0x9DF2 },	// 9868 E9B7B2 [鷲]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9869, */ 0x4E99 },	// 9869 E4BA99 [亙]  #CJK UNIFIED IDEOGRAPH
{ /* 0x986A, */ 0x4E98 },	// 986A E4BA98 [亘]  #CJK UNIFIED IDEOGRAPH
{ /* 0x986B, */ 0x9C10 },	// 986B E9B090 [鰐]  #CJK UNIFIED IDEOGRAPH
{ /* 0x986C, */ 0x8A6B },	// 986C E8A9AB [詫]  #CJK UNIFIED IDEOGRAPH
{ /* 0x986D, */ 0x85C1 },	// 986D E89781 [藁]  #CJK UNIFIED IDEOGRAPH
{ /* 0x986E, */ 0x8568 },	// 986E E895A8 [蕨]  #CJK UNIFIED IDEOGRAPH
{ /* 0x986F, */ 0x6900 },	// 986F E6A480 [椀]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9870, */ 0x6E7E },	// 9870 E6B9BE [湾]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9871, */ 0x7897 },	// 9871 E7A297 [碗]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9872, */ 0x8155 },	// 9872 E88595 [腕]  #CJK UNIFIED IDEOGRAPH
{ /* 0x9873, */ 0x0000 },	//  9873 ------ [　]  
{ /* 0x9874, */ 0x0000 },	//  9874 ------ [　]  
{ /* 0x9875, */ 0x0000 },	//  9875 ------ [　]  
{ /* 0x9876, */ 0x0000 },	//  9876 ------ [　]  
{ /* 0x9877, */ 0x0000 },	//  9877 ------ [　]  
{ /* 0x9878, */ 0x0000 },	//  9878 ------ [　]  
{ /* 0x9879, */ 0x0000 },	//  9879 ------ [　]  
{ /* 0x987A, */ 0x0000 },	//  987A ------ [　]  
{ /* 0x987B, */ 0x0000 },	//  987B ------ [　]  
{ /* 0x987C, */ 0x0000 },	//  987C ------ [　]  
{ /* 0x987D, */ 0x0000 },	//  987D ------ [　]  
{ /* 0x987E, */ 0x0000 },	//  987E ------ [　]  
{ /* 0x987F, */ 0x0000 },	// 		　#ダミー
{ /* 0x9880, */ 0x0000 },	//  9880 ------ [　]  
{ /* 0x9881, */ 0x0000 },	//  9881 ------ [　]  
{ /* 0x9882, */ 0x0000 },	//  9882 ------ [　]  
{ /* 0x9883, */ 0x0000 },	//  9883 ------ [　]  
{ /* 0x9884, */ 0x0000 },	//  9884 ------ [　]  
{ /* 0x9885, */ 0x0000 },	//  9885 ------ [　]  
{ /* 0x9886, */ 0x0000 },	//  9886 ------ [　]  
{ /* 0x9887, */ 0x0000 },	//  9887 ------ [　]  
{ /* 0x9888, */ 0x0000 },	//  9888 ------ [　]  
{ /* 0x9889, */ 0x0000 },	//  9889 ------ [　]  
{ /* 0x988A, */ 0x0000 },	//  988A ------ [　]  
{ /* 0x988B, */ 0x0000 },	//  988B ------ [　]  
{ /* 0x988C, */ 0x0000 },	//  988C ------ [　]  
{ /* 0x988D, */ 0x0000 },	//  988D ------ [　]  
{ /* 0x988E, */ 0x0000 },	//  988E ------ [　]  
{ /* 0x988F, */ 0x0000 },	//  988F ------ [　]  
{ /* 0x9890, */ 0x0000 },	//  9890 ------ [　]  
{ /* 0x9891, */ 0x0000 },	//  9891 ------ [　]  
{ /* 0x9892, */ 0x0000 },	//  9892 ------ [　]  
{ /* 0x9893, */ 0x0000 },	//  9893 ------ [　]  
{ /* 0x9894, */ 0x0000 },	//  9894 ------ [　]  
{ /* 0x9895, */ 0x0000 },	//  9895 ------ [　]  
{ /* 0x9896, */ 0x0000 },	//  9896 ------ [　]  
{ /* 0x9897, */ 0x0000 },	//  9897 ------ [　]  
{ /* 0x9898, */ 0x0000 },	//  9898 ------ [　]  
{ /* 0x9899, */ 0x0000 },	//  9899 ------ [　]  
{ /* 0x989A, */ 0x0000 },	//  989A ------ [　]  
{ /* 0x989B, */ 0x0000 },	//  989B ------ [　]  
{ /* 0x989C, */ 0x0000 },	//  989C ------ [　]  
{ /* 0x989D, */ 0x0000 },	//  989D ------ [　]  
{ /* 0x989E, */ 0x0000 },	//  989E ------ [　]  
{ /* 0x989F, */ 0x5F0C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98A0, */ 0x4E10 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98A1, */ 0x4E15 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98A2, */ 0x4E2A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98A3, */ 0x4E31 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98A4, */ 0x4E36 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98A5, */ 0x4E3C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98A6, */ 0x4E3F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98A7, */ 0x4E42 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98A8, */ 0x4E56 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98A9, */ 0x4E58 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98AA, */ 0x4E82 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98AB, */ 0x4E85 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98AC, */ 0x8C6B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98AD, */ 0x4E8A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98AE, */ 0x8212 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98AF, */ 0x5F0D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98B0, */ 0x4E8E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98B1, */ 0x4E9E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98B2, */ 0x4E9F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98B3, */ 0x4EA0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98B4, */ 0x4EA2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98B5, */ 0x4EB0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98B6, */ 0x4EB3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98B7, */ 0x4EB6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98B8, */ 0x4ECE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98B9, */ 0x4ECD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98BA, */ 0x4EC4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98BB, */ 0x4EC6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98BC, */ 0x4EC2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98BD, */ 0x4ED7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98BE, */ 0x4EDE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98BF, */ 0x4EED },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98C0, */ 0x4EDF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98C1, */ 0x4EF7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98C2, */ 0x4F09 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98C3, */ 0x4F5A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98C4, */ 0x4F30 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98C5, */ 0x4F5B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98C6, */ 0x4F5D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98C7, */ 0x4F57 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98C8, */ 0x4F47 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98C9, */ 0x4F76 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98CA, */ 0x4F88 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98CB, */ 0x4F8F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98CC, */ 0x4F98 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98CD, */ 0x4F7B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98CE, */ 0x4F69 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98CF, */ 0x4F70 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98D0, */ 0x4F91 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98D1, */ 0x4F6F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98D2, */ 0x4F86 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98D3, */ 0x4F96 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98D4, */ 0x5118 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98D5, */ 0x4FD4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98D6, */ 0x4FDF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98D7, */ 0x4FCE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98D8, */ 0x4FD8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98D9, */ 0x4FDB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98DA, */ 0x4FD1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98DB, */ 0x4FDA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98DC, */ 0x4FD0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98DD, */ 0x4FE4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98DE, */ 0x4FE5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98DF, */ 0x501A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98E0, */ 0x5028 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98E1, */ 0x5014 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98E2, */ 0x502A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98E3, */ 0x5025 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98E4, */ 0x5005 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98E5, */ 0x4F1C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98E6, */ 0x4FF6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98E7, */ 0x5021 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98E8, */ 0x5029 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98E9, */ 0x502C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98EA, */ 0x4FFE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98EB, */ 0x4FEF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98EC, */ 0x5011 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98ED, */ 0x5006 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98EE, */ 0x5043 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98EF, */ 0x5047 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98F0, */ 0x6703 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98F1, */ 0x5055 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98F2, */ 0x5050 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98F3, */ 0x5048 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98F4, */ 0x505A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98F5, */ 0x5056 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98F6, */ 0x506C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98F7, */ 0x5078 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98F8, */ 0x5080 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98F9, */ 0x509A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98FA, */ 0x5085 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98FB, */ 0x50B4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x98FC, */ 0x50B2 },	// #CJK UNIFIED IDEOGRAPH
//===== 0x99 40～FC ( テーブル数 [189]  ）====================
{ /* 0x9940, */ 0x50C9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9941, */ 0x50CA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9942, */ 0x50B3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9943, */ 0x50C2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9944, */ 0x50D6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9945, */ 0x50DE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9946, */ 0x50E5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9947, */ 0x50ED },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9948, */ 0x50E3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9949, */ 0x50EE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x994A, */ 0x50F9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x994B, */ 0x50F5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x994C, */ 0x5109 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x994D, */ 0x5101 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x994E, */ 0x5102 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x994F, */ 0x5116 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9950, */ 0x5115 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9951, */ 0x5114 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9952, */ 0x511A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9953, */ 0x5121 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9954, */ 0x513A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9955, */ 0x5137 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9956, */ 0x513C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9957, */ 0x513B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9958, */ 0x513F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9959, */ 0x5140 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x995A, */ 0x5152 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x995B, */ 0x514C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x995C, */ 0x5154 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x995D, */ 0x5162 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x995E, */ 0x7AF8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x995F, */ 0x5169 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9960, */ 0x516A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9961, */ 0x516E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9962, */ 0x5180 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9963, */ 0x5182 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9964, */ 0x56D8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9965, */ 0x518C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9966, */ 0x5189 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9967, */ 0x518F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9968, */ 0x5191 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9969, */ 0x5193 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x996A, */ 0x5195 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x996B, */ 0x5196 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x996C, */ 0x51A4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x996D, */ 0x51A6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x996E, */ 0x51A2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x996F, */ 0x51A9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9970, */ 0x51AA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9971, */ 0x51AB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9972, */ 0x51B3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9973, */ 0x51B1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9974, */ 0x51B2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9975, */ 0x51B0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9976, */ 0x51B5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9977, */ 0x51BD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9978, */ 0x51C5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9979, */ 0x51C9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x997A, */ 0x51DB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x997B, */ 0x51E0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x997C, */ 0x8655 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x997D, */ 0x51E9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x997E, */ 0x51ED },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x997F, */ 0x0000 },	// 		　#ダミー
{ /* 0x9980, */ 0x51F0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9981, */ 0x51F5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9982, */ 0x51FE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9983, */ 0x5204 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9984, */ 0x520B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9985, */ 0x5214 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9986, */ 0x520E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9987, */ 0x5227 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9988, */ 0x522A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9989, */ 0x522E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x998A, */ 0x5233 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x998B, */ 0x5239 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x998C, */ 0x524F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x998D, */ 0x5244 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x998E, */ 0x524B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x998F, */ 0x524C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9990, */ 0x525E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9991, */ 0x5254 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9992, */ 0x526A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9993, */ 0x5274 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9994, */ 0x5269 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9995, */ 0x5273 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9996, */ 0x527F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9997, */ 0x527D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9998, */ 0x528D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9999, */ 0x5294 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x999A, */ 0x5292 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x999B, */ 0x5271 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x999C, */ 0x5288 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x999D, */ 0x5291 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x999E, */ 0x8FA8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x999F, */ 0x8FA7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99A0, */ 0x52AC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99A1, */ 0x52AD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99A2, */ 0x52BC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99A3, */ 0x52B5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99A4, */ 0x52C1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99A5, */ 0x52CD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99A6, */ 0x52D7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99A7, */ 0x52DE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99A8, */ 0x52E3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99A9, */ 0x52E6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99AA, */ 0x98ED },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99AB, */ 0x52E0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99AC, */ 0x52F3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99AD, */ 0x52F5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99AE, */ 0x52F8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99AF, */ 0x52F9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99B0, */ 0x5306 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99B1, */ 0x5308 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99B2, */ 0x7538 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99B3, */ 0x530D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99B4, */ 0x5310 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99B5, */ 0x530F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99B6, */ 0x5315 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99B7, */ 0x531A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99B8, */ 0x5323 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99B9, */ 0x532F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99BA, */ 0x5331 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99BB, */ 0x5333 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99BC, */ 0x5338 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99BD, */ 0x5340 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99BE, */ 0x5346 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99BF, */ 0x5345 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99C0, */ 0x4E17 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99C1, */ 0x5349 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99C2, */ 0x534D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99C3, */ 0x51D6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99C4, */ 0x535E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99C5, */ 0x5369 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99C6, */ 0x536E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99C7, */ 0x5918 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99C8, */ 0x537B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99C9, */ 0x5377 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99CA, */ 0x5382 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99CB, */ 0x5396 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99CC, */ 0x53A0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99CD, */ 0x53A6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99CE, */ 0x53A5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99CF, */ 0x53AE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99D0, */ 0x53B0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99D1, */ 0x53B6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99D2, */ 0x53C3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99D3, */ 0x7C12 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99D4, */ 0x96D9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99D5, */ 0x53DF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99D6, */ 0x66FC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99D7, */ 0x71EE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99D8, */ 0x53EE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99D9, */ 0x53E8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99DA, */ 0x53ED },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99DB, */ 0x53FA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99DC, */ 0x5401 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99DD, */ 0x543D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99DE, */ 0x5440 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99DF, */ 0x542C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99E0, */ 0x542D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99E1, */ 0x543C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99E2, */ 0x542E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99E3, */ 0x5436 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99E4, */ 0x5429 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99E5, */ 0x541D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99E6, */ 0x544E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99E7, */ 0x548F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99E8, */ 0x5475 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99E9, */ 0x548E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99EA, */ 0x545F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99EB, */ 0x5471 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99EC, */ 0x5477 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99ED, */ 0x5470 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99EE, */ 0x5492 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99EF, */ 0x547B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99F0, */ 0x5480 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99F1, */ 0x5476 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99F2, */ 0x5484 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99F3, */ 0x5490 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99F4, */ 0x5486 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99F5, */ 0x54C7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99F6, */ 0x54A2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99F7, */ 0x54B8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99F8, */ 0x54A5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99F9, */ 0x54AC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99FA, */ 0x54C4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99FB, */ 0x54C8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x99FC, */ 0x54A8 },	// #CJK UNIFIED IDEOGRAPH
//===== 0x9A 40～FC ( テーブル数 [189]  ）====================
{ /* 0x9A40, */ 0x54AB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A41, */ 0x54C2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A42, */ 0x54A4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A43, */ 0x54BE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A44, */ 0x54BC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A45, */ 0x54D8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A46, */ 0x54E5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A47, */ 0x54E6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A48, */ 0x550F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A49, */ 0x5514 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A4A, */ 0x54FD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A4B, */ 0x54EE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A4C, */ 0x54ED },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A4D, */ 0x54FA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A4E, */ 0x54E2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A4F, */ 0x5539 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A50, */ 0x5540 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A51, */ 0x5563 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A52, */ 0x554C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A53, */ 0x552E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A54, */ 0x555C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A55, */ 0x5545 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A56, */ 0x5556 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A57, */ 0x5557 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A58, */ 0x5538 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A59, */ 0x5533 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A5A, */ 0x555D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A5B, */ 0x5599 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A5C, */ 0x5580 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A5D, */ 0x54AF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A5E, */ 0x558A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A5F, */ 0x559F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A60, */ 0x557B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A61, */ 0x557E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A62, */ 0x5598 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A63, */ 0x559E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A64, */ 0x55AE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A65, */ 0x557C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A66, */ 0x5583 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A67, */ 0x55A9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A68, */ 0x5587 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A69, */ 0x55A8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A6A, */ 0x55DA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A6B, */ 0x55C5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A6C, */ 0x55DF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A6D, */ 0x55C4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A6E, */ 0x55DC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A6F, */ 0x55E4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A70, */ 0x55D4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A71, */ 0x5614 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A72, */ 0x55F7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A73, */ 0x5616 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A74, */ 0x55FE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A75, */ 0x55FD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A76, */ 0x561B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A77, */ 0x55F9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A78, */ 0x564E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A79, */ 0x5650 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A7A, */ 0x71DF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A7B, */ 0x5634 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A7C, */ 0x5636 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A7D, */ 0x5632 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A7E, */ 0x5638 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A7F, */ 0x0000 },	// 		　#ダミー
{ /* 0x9A80, */ 0x566B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A81, */ 0x5664 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A82, */ 0x562F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A83, */ 0x566C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A84, */ 0x566A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A85, */ 0x5686 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A86, */ 0x5680 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A87, */ 0x568A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A88, */ 0x56A0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A89, */ 0x5694 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A8A, */ 0x568F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A8B, */ 0x56A5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A8C, */ 0x56AE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A8D, */ 0x56B6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A8E, */ 0x56B4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A8F, */ 0x56C2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A90, */ 0x56BC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A91, */ 0x56C1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A92, */ 0x56C3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A93, */ 0x56C0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A94, */ 0x56C8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A95, */ 0x56CE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A96, */ 0x56D1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A97, */ 0x56D3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A98, */ 0x56D7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A99, */ 0x56EE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A9A, */ 0x56F9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A9B, */ 0x5700 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A9C, */ 0x56FF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A9D, */ 0x5704 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A9E, */ 0x5709 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9A9F, */ 0x5708 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AA0, */ 0x570B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AA1, */ 0x570D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AA2, */ 0x5713 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AA3, */ 0x5718 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AA4, */ 0x5716 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AA5, */ 0x55C7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AA6, */ 0x571C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AA7, */ 0x5726 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AA8, */ 0x5737 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AA9, */ 0x5738 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AAA, */ 0x574E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AAB, */ 0x573B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AAC, */ 0x5740 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AAD, */ 0x574F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AAE, */ 0x5769 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AAF, */ 0x57C0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AB0, */ 0x5788 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AB1, */ 0x5761 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AB2, */ 0x577F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AB3, */ 0x5789 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AB4, */ 0x5793 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AB5, */ 0x57A0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AB6, */ 0x57B3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AB7, */ 0x57A4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AB8, */ 0x57AA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AB9, */ 0x57B0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ABA, */ 0x57C3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ABB, */ 0x57C6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ABC, */ 0x57D4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ABD, */ 0x57D2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ABE, */ 0x57D3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ABF, */ 0x580A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AC0, */ 0x57D6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AC1, */ 0x57E3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AC2, */ 0x580B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AC3, */ 0x5819 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AC4, */ 0x581D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AC5, */ 0x5872 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AC6, */ 0x5821 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AC7, */ 0x5862 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AC8, */ 0x584B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AC9, */ 0x5870 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ACA, */ 0x6BC0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ACB, */ 0x5852 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ACC, */ 0x583D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ACD, */ 0x5879 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ACE, */ 0x5885 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ACF, */ 0x58B9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AD0, */ 0x589F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AD1, */ 0x58AB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AD2, */ 0x58BA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AD3, */ 0x58DE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AD4, */ 0x58BB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AD5, */ 0x58B8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AD6, */ 0x58AE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AD7, */ 0x58C5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AD8, */ 0x58D3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AD9, */ 0x58D1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ADA, */ 0x58D7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ADB, */ 0x58D9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ADC, */ 0x58D8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ADD, */ 0x58E5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ADE, */ 0x58DC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ADF, */ 0x58E4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AE0, */ 0x58DF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AE1, */ 0x58EF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AE2, */ 0x58FA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AE3, */ 0x58F9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AE4, */ 0x58FB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AE5, */ 0x58FC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AE6, */ 0x58FD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AE7, */ 0x5902 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AE8, */ 0x590A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AE9, */ 0x5910 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AEA, */ 0x591B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AEB, */ 0x68A6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AEC, */ 0x5925 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AED, */ 0x592C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AEE, */ 0x592D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AEF, */ 0x5932 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AF0, */ 0x5938 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AF1, */ 0x593E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AF2, */ 0x7AD2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AF3, */ 0x5955 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AF4, */ 0x5950 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AF5, */ 0x594E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AF6, */ 0x595A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AF7, */ 0x5958 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AF8, */ 0x5962 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AF9, */ 0x5960 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AFA, */ 0x5967 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AFB, */ 0x596C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9AFC, */ 0x5969 },	// #CJK UNIFIED IDEOGRAPH
//===== 0x9B 40～FC ( テーブル数 [189]  ）====================
{ /* 0x9B40, */ 0x5978 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B41, */ 0x5981 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B42, */ 0x599D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B43, */ 0x4F5E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B44, */ 0x4FAB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B45, */ 0x59A3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B46, */ 0x59B2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B47, */ 0x59C6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B48, */ 0x59E8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B49, */ 0x59DC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B4A, */ 0x598D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B4B, */ 0x59D9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B4C, */ 0x59DA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B4D, */ 0x5A25 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B4E, */ 0x5A1F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B4F, */ 0x5A11 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B50, */ 0x5A1C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B51, */ 0x5A09 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B52, */ 0x5A1A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B53, */ 0x5A40 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B54, */ 0x5A6C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B55, */ 0x5A49 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B56, */ 0x5A35 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B57, */ 0x5A36 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B58, */ 0x5A62 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B59, */ 0x5A6A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B5A, */ 0x5A9A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B5B, */ 0x5ABC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B5C, */ 0x5ABE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B5D, */ 0x5ACB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B5E, */ 0x5AC2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B5F, */ 0x5ABD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B60, */ 0x5AE3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B61, */ 0x5AD7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B62, */ 0x5AE6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B63, */ 0x5AE9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B64, */ 0x5AD6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B65, */ 0x5AFA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B66, */ 0x5AFB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B67, */ 0x5B0C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B68, */ 0x5B0B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B69, */ 0x5B16 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B6A, */ 0x5B32 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B6B, */ 0x5AD0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B6C, */ 0x5B2A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B6D, */ 0x5B36 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B6E, */ 0x5B3E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B6F, */ 0x5B43 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B70, */ 0x5B45 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B71, */ 0x5B40 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B72, */ 0x5B51 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B73, */ 0x5B55 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B74, */ 0x5B5A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B75, */ 0x5B5B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B76, */ 0x5B65 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B77, */ 0x5B69 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B78, */ 0x5B70 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B79, */ 0x5B73 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B7A, */ 0x5B75 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B7B, */ 0x5B78 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B7C, */ 0x6588 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B7D, */ 0x5B7A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B7E, */ 0x5B80 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B7F, */ 0x0000 },	// 		　#ダミー
{ /* 0x9B80, */ 0x5B83 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B81, */ 0x5BA6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B82, */ 0x5BB8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B83, */ 0x5BC3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B84, */ 0x5BC7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B85, */ 0x5BC9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B86, */ 0x5BD4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B87, */ 0x5BD0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B88, */ 0x5BE4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B89, */ 0x5BE6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B8A, */ 0x5BE2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B8B, */ 0x5BDE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B8C, */ 0x5BE5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B8D, */ 0x5BEB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B8E, */ 0x5BF0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B8F, */ 0x5BF6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B90, */ 0x5BF3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B91, */ 0x5C05 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B92, */ 0x5C07 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B93, */ 0x5C08 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B94, */ 0x5C0D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B95, */ 0x5C13 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B96, */ 0x5C20 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B97, */ 0x5C22 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B98, */ 0x5C28 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B99, */ 0x5C38 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B9A, */ 0x5C39 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B9B, */ 0x5C41 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B9C, */ 0x5C46 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B9D, */ 0x5C4E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B9E, */ 0x5C53 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9B9F, */ 0x5C50 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BA0, */ 0x5C4F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BA1, */ 0x5B71 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BA2, */ 0x5C6C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BA3, */ 0x5C6E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BA4, */ 0x4E62 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BA5, */ 0x5C76 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BA6, */ 0x5C79 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BA7, */ 0x5C8C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BA8, */ 0x5C91 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BA9, */ 0x5C94 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BAA, */ 0x599B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BAB, */ 0x5CAB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BAC, */ 0x5CBB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BAD, */ 0x5CB6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BAE, */ 0x5CBC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BAF, */ 0x5CB7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BB0, */ 0x5CC5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BB1, */ 0x5CBE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BB2, */ 0x5CC7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BB3, */ 0x5CD9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BB4, */ 0x5CE9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BB5, */ 0x5CFD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BB6, */ 0x5CFA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BB7, */ 0x5CED },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BB8, */ 0x5D8C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BB9, */ 0x5CEA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BBA, */ 0x5D0B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BBB, */ 0x5D15 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BBC, */ 0x5D17 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BBD, */ 0x5D5C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BBE, */ 0x5D1F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BBF, */ 0x5D1B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BC0, */ 0x5D11 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BC1, */ 0x5D14 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BC2, */ 0x5D22 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BC3, */ 0x5D1A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BC4, */ 0x5D19 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BC5, */ 0x5D18 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BC6, */ 0x5D4C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BC7, */ 0x5D52 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BC8, */ 0x5D4E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BC9, */ 0x5D4B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BCA, */ 0x5D6C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BCB, */ 0x5D73 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BCC, */ 0x5D76 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BCD, */ 0x5D87 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BCE, */ 0x5D84 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BCF, */ 0x5D82 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BD0, */ 0x5DA2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BD1, */ 0x5D9D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BD2, */ 0x5DAC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BD3, */ 0x5DAE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BD4, */ 0x5DBD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BD5, */ 0x5D90 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BD6, */ 0x5DB7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BD7, */ 0x5DBC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BD8, */ 0x5DC9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BD9, */ 0x5DCD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BDA, */ 0x5DD3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BDB, */ 0x5DD2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BDC, */ 0x5DD6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BDD, */ 0x5DDB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BDE, */ 0x5DEB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BDF, */ 0x5DF2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BE0, */ 0x5DF5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BE1, */ 0x5E0B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BE2, */ 0x5E1A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BE3, */ 0x5E19 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BE4, */ 0x5E11 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BE5, */ 0x5E1B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BE6, */ 0x5E36 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BE7, */ 0x5E37 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BE8, */ 0x5E44 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BE9, */ 0x5E43 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BEA, */ 0x5E40 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BEB, */ 0x5E4E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BEC, */ 0x5E57 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BED, */ 0x5E54 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BEE, */ 0x5E5F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BEF, */ 0x5E62 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BF0, */ 0x5E64 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BF1, */ 0x5E47 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BF2, */ 0x5E75 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BF3, */ 0x5E76 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BF4, */ 0x5E7A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BF5, */ 0x9EBC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BF6, */ 0x5E7F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BF7, */ 0x5EA0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BF8, */ 0x5EC1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BF9, */ 0x5EC2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BFA, */ 0x5EC8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BFB, */ 0x5ED0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9BFC, */ 0x5ECF },	// #CJK UNIFIED IDEOGRAPH
//===== 0x9C 40～FC ( テーブル数 [189]  ）====================
{ /* 0x9C40, */ 0x5ED6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C41, */ 0x5EE3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C42, */ 0x5EDD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C43, */ 0x5EDA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C44, */ 0x5EDB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C45, */ 0x5EE2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C46, */ 0x5EE1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C47, */ 0x5EE8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C48, */ 0x5EE9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C49, */ 0x5EEC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C4A, */ 0x5EF1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C4B, */ 0x5EF3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C4C, */ 0x5EF0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C4D, */ 0x5EF4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C4E, */ 0x5EF8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C4F, */ 0x5EFE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C50, */ 0x5F03 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C51, */ 0x5F09 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C52, */ 0x5F5D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C53, */ 0x5F5C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C54, */ 0x5F0B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C55, */ 0x5F11 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C56, */ 0x5F16 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C57, */ 0x5F29 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C58, */ 0x5F2D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C59, */ 0x5F38 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C5A, */ 0x5F41 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C5B, */ 0x5F48 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C5C, */ 0x5F4C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C5D, */ 0x5F4E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C5E, */ 0x5F2F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C5F, */ 0x5F51 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C60, */ 0x5F56 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C61, */ 0x5F57 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C62, */ 0x5F59 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C63, */ 0x5F61 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C64, */ 0x5F6D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C65, */ 0x5F73 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C66, */ 0x5F77 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C67, */ 0x5F83 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C68, */ 0x5F82 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C69, */ 0x5F7F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C6A, */ 0x5F8A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C6B, */ 0x5F88 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C6C, */ 0x5F91 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C6D, */ 0x5F87 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C6E, */ 0x5F9E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C6F, */ 0x5F99 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C70, */ 0x5F98 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C71, */ 0x5FA0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C72, */ 0x5FA8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C73, */ 0x5FAD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C74, */ 0x5FBC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C75, */ 0x5FD6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C76, */ 0x5FFB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C77, */ 0x5FE4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C78, */ 0x5FF8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C79, */ 0x5FF1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C7A, */ 0x5FDD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C7B, */ 0x60B3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C7C, */ 0x5FFF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C7D, */ 0x6021 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C7E, */ 0x6060 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C7F, */ 0x0000 },	// 		　#ダミー
{ /* 0x9C80, */ 0x6019 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C81, */ 0x6010 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C82, */ 0x6029 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C83, */ 0x600E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C84, */ 0x6031 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C85, */ 0x601B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C86, */ 0x6015 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C87, */ 0x602B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C88, */ 0x6026 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C89, */ 0x600F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C8A, */ 0x603A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C8B, */ 0x605A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C8C, */ 0x6041 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C8D, */ 0x606A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C8E, */ 0x6077 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C8F, */ 0x605F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C90, */ 0x604A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C91, */ 0x6046 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C92, */ 0x604D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C93, */ 0x6063 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C94, */ 0x6043 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C95, */ 0x6064 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C96, */ 0x6042 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C97, */ 0x606C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C98, */ 0x606B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C99, */ 0x6059 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C9A, */ 0x6081 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C9B, */ 0x608D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C9C, */ 0x60E7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C9D, */ 0x6083 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C9E, */ 0x609A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9C9F, */ 0x6084 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CA0, */ 0x609B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CA1, */ 0x6096 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CA2, */ 0x6097 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CA3, */ 0x6092 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CA4, */ 0x60A7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CA5, */ 0x608B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CA6, */ 0x60E1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CA7, */ 0x60B8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CA8, */ 0x60E0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CA9, */ 0x60D3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CAA, */ 0x60B4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CAB, */ 0x5FF0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CAC, */ 0x60BD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CAD, */ 0x60C6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CAE, */ 0x60B5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CAF, */ 0x60D8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CB0, */ 0x614D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CB1, */ 0x6115 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CB2, */ 0x6106 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CB3, */ 0x60F6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CB4, */ 0x60F7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CB5, */ 0x6100 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CB6, */ 0x60F4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CB7, */ 0x60FA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CB8, */ 0x6103 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CB9, */ 0x6121 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CBA, */ 0x60FB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CBB, */ 0x60F1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CBC, */ 0x610D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CBD, */ 0x610E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CBE, */ 0x6147 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CBF, */ 0x613E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CC0, */ 0x6128 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CC1, */ 0x6127 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CC2, */ 0x614A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CC3, */ 0x613F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CC4, */ 0x613C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CC5, */ 0x612C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CC6, */ 0x6134 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CC7, */ 0x613D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CC8, */ 0x6142 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CC9, */ 0x6144 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CCA, */ 0x6173 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CCB, */ 0x6177 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CCC, */ 0x6158 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CCD, */ 0x6159 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CCE, */ 0x615A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CCF, */ 0x616B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CD0, */ 0x6174 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CD1, */ 0x616F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CD2, */ 0x6165 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CD3, */ 0x6171 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CD4, */ 0x615F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CD5, */ 0x615D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CD6, */ 0x6153 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CD7, */ 0x6175 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CD8, */ 0x6199 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CD9, */ 0x6196 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CDA, */ 0x6187 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CDB, */ 0x61AC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CDC, */ 0x6194 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CDD, */ 0x619A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CDE, */ 0x618A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CDF, */ 0x6191 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CE0, */ 0x61AB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CE1, */ 0x61AE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CE2, */ 0x61CC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CE3, */ 0x61CA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CE4, */ 0x61C9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CE5, */ 0x61F7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CE6, */ 0x61C8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CE7, */ 0x61C3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CE8, */ 0x61C6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CE9, */ 0x61BA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CEA, */ 0x61CB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CEB, */ 0x7F79 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CEC, */ 0x61CD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CED, */ 0x61E6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CEE, */ 0x61E3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CEF, */ 0x61F6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CF0, */ 0x61FA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CF1, */ 0x61F4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CF2, */ 0x61FF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CF3, */ 0x61FD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CF4, */ 0x61FC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CF5, */ 0x61FE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CF6, */ 0x6200 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CF7, */ 0x6208 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CF8, */ 0x6209 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CF9, */ 0x620D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CFA, */ 0x620C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CFB, */ 0x6214 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9CFC, */ 0x621B },	// #CJK UNIFIED IDEOGRAPH
//===== 0x9D 40～FC ( テーブル数 [189]  ）====================
{ /* 0x9D40, */ 0x621E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D41, */ 0x6221 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D42, */ 0x622A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D43, */ 0x622E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D44, */ 0x6230 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D45, */ 0x6232 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D46, */ 0x6233 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D47, */ 0x6241 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D48, */ 0x624E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D49, */ 0x625E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D4A, */ 0x6263 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D4B, */ 0x625B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D4C, */ 0x6260 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D4D, */ 0x6268 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D4E, */ 0x627C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D4F, */ 0x6282 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D50, */ 0x6289 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D51, */ 0x627E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D52, */ 0x6292 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D53, */ 0x6293 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D54, */ 0x6296 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D55, */ 0x62D4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D56, */ 0x6283 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D57, */ 0x6294 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D58, */ 0x62D7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D59, */ 0x62D1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D5A, */ 0x62BB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D5B, */ 0x62CF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D5C, */ 0x62FF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D5D, */ 0x62C6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D5E, */ 0x64D4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D5F, */ 0x62C8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D60, */ 0x62DC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D61, */ 0x62CC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D62, */ 0x62CA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D63, */ 0x62C2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D64, */ 0x62C7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D65, */ 0x629B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D66, */ 0x62C9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D67, */ 0x630C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D68, */ 0x62EE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D69, */ 0x62F1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D6A, */ 0x6327 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D6B, */ 0x6302 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D6C, */ 0x6308 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D6D, */ 0x62EF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D6E, */ 0x62F5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D6F, */ 0x6350 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D70, */ 0x633E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D71, */ 0x634D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D72, */ 0x641C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D73, */ 0x634F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D74, */ 0x6396 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D75, */ 0x638E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D76, */ 0x6380 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D77, */ 0x63AB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D78, */ 0x6376 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D79, */ 0x63A3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D7A, */ 0x638F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D7B, */ 0x6389 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D7C, */ 0x639F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D7D, */ 0x63B5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D7E, */ 0x636B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D7F, */ 0x0000 },	// 		　#ダミー
{ /* 0x9D80, */ 0x6369 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D81, */ 0x63BE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D82, */ 0x63E9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D83, */ 0x63C0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D84, */ 0x63C6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D85, */ 0x63E3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D86, */ 0x63C9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D87, */ 0x63D2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D88, */ 0x63F6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D89, */ 0x63C4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D8A, */ 0x6416 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D8B, */ 0x6434 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D8C, */ 0x6406 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D8D, */ 0x6413 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D8E, */ 0x6426 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D8F, */ 0x6436 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D90, */ 0x651D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D91, */ 0x6417 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D92, */ 0x6428 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D93, */ 0x640F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D94, */ 0x6467 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D95, */ 0x646F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D96, */ 0x6476 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D97, */ 0x644E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D98, */ 0x652A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D99, */ 0x6495 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D9A, */ 0x6493 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D9B, */ 0x64A5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D9C, */ 0x64A9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D9D, */ 0x6488 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D9E, */ 0x64BC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9D9F, */ 0x64DA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DA0, */ 0x64D2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DA1, */ 0x64C5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DA2, */ 0x64C7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DA3, */ 0x64BB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DA4, */ 0x64D8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DA5, */ 0x64C2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DA6, */ 0x64F1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DA7, */ 0x64E7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DA8, */ 0x8209 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DA9, */ 0x64E0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DAA, */ 0x64E1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DAB, */ 0x62AC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DAC, */ 0x64E3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DAD, */ 0x64EF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DAE, */ 0x652C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DAF, */ 0x64F6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DB0, */ 0x64F4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DB1, */ 0x64F2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DB2, */ 0x64FA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DB3, */ 0x6500 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DB4, */ 0x64FD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DB5, */ 0x6518 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DB6, */ 0x651C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DB7, */ 0x6505 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DB8, */ 0x6524 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DB9, */ 0x6523 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DBA, */ 0x652B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DBB, */ 0x6534 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DBC, */ 0x6535 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DBD, */ 0x6537 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DBE, */ 0x6536 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DBF, */ 0x6538 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DC0, */ 0x754B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DC1, */ 0x6548 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DC2, */ 0x6556 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DC3, */ 0x6555 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DC4, */ 0x654D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DC5, */ 0x6558 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DC6, */ 0x655E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DC7, */ 0x655D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DC8, */ 0x6572 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DC9, */ 0x6578 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DCA, */ 0x6582 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DCB, */ 0x6583 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DCC, */ 0x8B8A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DCD, */ 0x659B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DCE, */ 0x659F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DCF, */ 0x65AB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DD0, */ 0x65B7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DD1, */ 0x65C3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DD2, */ 0x65C6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DD3, */ 0x65C1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DD4, */ 0x65C4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DD5, */ 0x65CC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DD6, */ 0x65D2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DD7, */ 0x65DB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DD8, */ 0x65D9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DD9, */ 0x65E0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DDA, */ 0x65E1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DDB, */ 0x65F1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DDC, */ 0x6772 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DDD, */ 0x660A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DDE, */ 0x6603 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DDF, */ 0x65FB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DE0, */ 0x6773 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DE1, */ 0x6635 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DE2, */ 0x6636 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DE3, */ 0x6634 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DE4, */ 0x661C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DE5, */ 0x664F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DE6, */ 0x6644 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DE7, */ 0x6649 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DE8, */ 0x6641 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DE9, */ 0x665E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DEA, */ 0x665D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DEB, */ 0x6664 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DEC, */ 0x6667 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DED, */ 0x6668 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DEE, */ 0x665F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DEF, */ 0x6662 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DF0, */ 0x6670 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DF1, */ 0x6683 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DF2, */ 0x6688 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DF3, */ 0x668E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DF4, */ 0x6689 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DF5, */ 0x6684 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DF6, */ 0x6698 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DF7, */ 0x669D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DF8, */ 0x66C1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DF9, */ 0x66B9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DFA, */ 0x66C9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DFB, */ 0x66BE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9DFC, */ 0x66BC },	// #CJK UNIFIED IDEOGRAPH
//===== 0x9E 40～FC ( テーブル数 [189]  ）====================
{ /* 0x9E40, */ 0x66C4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E41, */ 0x66B8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E42, */ 0x66D6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E43, */ 0x66DA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E44, */ 0x66E0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E45, */ 0x663F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E46, */ 0x66E6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E47, */ 0x66E9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E48, */ 0x66F0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E49, */ 0x66F5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E4A, */ 0x66F7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E4B, */ 0x670F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E4C, */ 0x6716 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E4D, */ 0x671E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E4E, */ 0x6726 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E4F, */ 0x6727 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E50, */ 0x9738 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E51, */ 0x672E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E52, */ 0x673F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E53, */ 0x6736 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E54, */ 0x6741 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E55, */ 0x6738 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E56, */ 0x6737 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E57, */ 0x6746 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E58, */ 0x675E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E59, */ 0x6760 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E5A, */ 0x6759 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E5B, */ 0x6763 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E5C, */ 0x6764 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E5D, */ 0x6789 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E5E, */ 0x6770 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E5F, */ 0x67A9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E60, */ 0x677C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E61, */ 0x676A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E62, */ 0x678C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E63, */ 0x678B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E64, */ 0x67A6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E65, */ 0x67A1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E66, */ 0x6785 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E67, */ 0x67B7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E68, */ 0x67EF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E69, */ 0x67B4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E6A, */ 0x67EC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E6B, */ 0x67B3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E6C, */ 0x67E9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E6D, */ 0x67B8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E6E, */ 0x67E4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E6F, */ 0x67DE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E70, */ 0x67DD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E71, */ 0x67E2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E72, */ 0x67EE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E73, */ 0x67B9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E74, */ 0x67CE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E75, */ 0x67C6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E76, */ 0x67E7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E77, */ 0x6A9C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E78, */ 0x681E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E79, */ 0x6846 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E7A, */ 0x6829 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E7B, */ 0x6840 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E7C, */ 0x684D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E7D, */ 0x6832 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E7E, */ 0x684E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E7F, */ 0x0000 },	// 		　#ダミー
{ /* 0x9E80, */ 0x68B3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E81, */ 0x682B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E82, */ 0x6859 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E83, */ 0x6863 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E84, */ 0x6877 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E85, */ 0x687F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E86, */ 0x689F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E87, */ 0x688F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E88, */ 0x68AD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E89, */ 0x6894 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E8A, */ 0x689D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E8B, */ 0x689B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E8C, */ 0x6883 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E8D, */ 0x6AAE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E8E, */ 0x68B9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E8F, */ 0x6874 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E90, */ 0x68B5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E91, */ 0x68A0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E92, */ 0x68BA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E93, */ 0x690F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E94, */ 0x688D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E95, */ 0x687E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E96, */ 0x6901 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E97, */ 0x68CA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E98, */ 0x6908 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E99, */ 0x68D8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E9A, */ 0x6922 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E9B, */ 0x6926 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E9C, */ 0x68E1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E9D, */ 0x690C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E9E, */ 0x68CD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9E9F, */ 0x68D4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EA0, */ 0x68E7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EA1, */ 0x68D5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EA2, */ 0x6936 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EA3, */ 0x6912 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EA4, */ 0x6904 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EA5, */ 0x68D7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EA6, */ 0x68E3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EA7, */ 0x6925 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EA8, */ 0x68F9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EA9, */ 0x68E0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EAA, */ 0x68EF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EAB, */ 0x6928 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EAC, */ 0x692A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EAD, */ 0x691A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EAE, */ 0x6923 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EAF, */ 0x6921 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EB0, */ 0x68C6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EB1, */ 0x6979 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EB2, */ 0x6977 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EB3, */ 0x695C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EB4, */ 0x6978 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EB5, */ 0x696B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EB6, */ 0x6954 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EB7, */ 0x697E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EB8, */ 0x696E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EB9, */ 0x6939 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EBA, */ 0x6974 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EBB, */ 0x693D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EBC, */ 0x6959 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EBD, */ 0x6930 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EBE, */ 0x6961 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EBF, */ 0x695E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EC0, */ 0x695D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EC1, */ 0x6981 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EC2, */ 0x696A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EC3, */ 0x69B2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EC4, */ 0x69AE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EC5, */ 0x69D0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EC6, */ 0x69BF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EC7, */ 0x69C1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EC8, */ 0x69D3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EC9, */ 0x69BE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ECA, */ 0x69CE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ECB, */ 0x5BE8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ECC, */ 0x69CA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ECD, */ 0x69DD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ECE, */ 0x69BB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ECF, */ 0x69C3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ED0, */ 0x69A7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ED1, */ 0x6A2E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ED2, */ 0x6991 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ED3, */ 0x69A0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ED4, */ 0x699C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ED5, */ 0x6995 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ED6, */ 0x69B4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ED7, */ 0x69DE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ED8, */ 0x69E8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9ED9, */ 0x6A02 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EDA, */ 0x6A1B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EDB, */ 0x69FF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EDC, */ 0x6B0A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EDD, */ 0x69F9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EDE, */ 0x69F2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EDF, */ 0x69E7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EE0, */ 0x6A05 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EE1, */ 0x69B1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EE2, */ 0x6A1E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EE3, */ 0x69ED },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EE4, */ 0x6A14 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EE5, */ 0x69EB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EE6, */ 0x6A0A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EE7, */ 0x6A12 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EE8, */ 0x6AC1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EE9, */ 0x6A23 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EEA, */ 0x6A13 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EEB, */ 0x6A44 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EEC, */ 0x6A0C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EED, */ 0x6A72 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EEE, */ 0x6A36 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EEF, */ 0x6A78 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EF0, */ 0x6A47 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EF1, */ 0x6A62 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EF2, */ 0x6A59 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EF3, */ 0x6A66 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EF4, */ 0x6A48 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EF5, */ 0x6A38 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EF6, */ 0x6A22 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EF7, */ 0x6A90 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EF8, */ 0x6A8D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EF9, */ 0x6AA0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EFA, */ 0x6A84 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EFB, */ 0x6AA2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9EFC, */ 0x6AA3 },	// #CJK UNIFIED IDEOGRAPH
//===== 0x9F 40～FC ( テーブル数 [189]  ）====================
{ /* 0x9F40, */ 0x6A97 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F41, */ 0x8617 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F42, */ 0x6ABB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F43, */ 0x6AC3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F44, */ 0x6AC2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F45, */ 0x6AB8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F46, */ 0x6AB3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F47, */ 0x6AAC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F48, */ 0x6ADE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F49, */ 0x6AD1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F4A, */ 0x6ADF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F4B, */ 0x6AAA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F4C, */ 0x6ADA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F4D, */ 0x6AEA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F4E, */ 0x6AFB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F4F, */ 0x6B05 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F50, */ 0x8616 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F51, */ 0x6AFA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F52, */ 0x6B12 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F53, */ 0x6B16 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F54, */ 0x9B31 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F55, */ 0x6B1F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F56, */ 0x6B38 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F57, */ 0x6B37 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F58, */ 0x76DC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F59, */ 0x6B39 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F5A, */ 0x98EE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F5B, */ 0x6B47 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F5C, */ 0x6B43 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F5D, */ 0x6B49 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F5E, */ 0x6B50 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F5F, */ 0x6B59 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F60, */ 0x6B54 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F61, */ 0x6B5B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F62, */ 0x6B5F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F63, */ 0x6B61 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F64, */ 0x6B78 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F65, */ 0x6B79 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F66, */ 0x6B7F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F67, */ 0x6B80 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F68, */ 0x6B84 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F69, */ 0x6B83 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F6A, */ 0x6B8D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F6B, */ 0x6B98 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F6C, */ 0x6B95 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F6D, */ 0x6B9E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F6E, */ 0x6BA4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F6F, */ 0x6BAA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F70, */ 0x6BAB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F71, */ 0x6BAF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F72, */ 0x6BB2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F73, */ 0x6BB1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F74, */ 0x6BB3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F75, */ 0x6BB7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F76, */ 0x6BBC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F77, */ 0x6BC6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F78, */ 0x6BCB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F79, */ 0x6BD3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F7A, */ 0x6BDF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F7B, */ 0x6BEC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F7C, */ 0x6BEB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F7D, */ 0x6BF3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F7E, */ 0x6BEF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F7F, */ 0x0000 },	// 		　#ダミー
{ /* 0x9F80, */ 0x9EBE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F81, */ 0x6C08 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F82, */ 0x6C13 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F83, */ 0x6C14 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F84, */ 0x6C1B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F85, */ 0x6C24 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F86, */ 0x6C23 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F87, */ 0x6C5E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F88, */ 0x6C55 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F89, */ 0x6C62 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F8A, */ 0x6C6A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F8B, */ 0x6C82 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F8C, */ 0x6C8D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F8D, */ 0x6C9A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F8E, */ 0x6C81 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F8F, */ 0x6C9B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F90, */ 0x6C7E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F91, */ 0x6C68 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F92, */ 0x6C73 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F93, */ 0x6C92 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F94, */ 0x6C90 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F95, */ 0x6CC4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F96, */ 0x6CF1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F97, */ 0x6CD3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F98, */ 0x6CBD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F99, */ 0x6CD7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F9A, */ 0x6CC5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F9B, */ 0x6CDD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F9C, */ 0x6CAE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F9D, */ 0x6CB1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F9E, */ 0x6CBE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9F9F, */ 0x6CBA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FA0, */ 0x6CDB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FA1, */ 0x6CEF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FA2, */ 0x6CD9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FA3, */ 0x6CEA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FA4, */ 0x6D1F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FA5, */ 0x884D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FA6, */ 0x6D36 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FA7, */ 0x6D2B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FA8, */ 0x6D3D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FA9, */ 0x6D38 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FAA, */ 0x6D19 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FAB, */ 0x6D35 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FAC, */ 0x6D33 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FAD, */ 0x6D12 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FAE, */ 0x6D0C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FAF, */ 0x6D63 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FB0, */ 0x6D93 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FB1, */ 0x6D64 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FB2, */ 0x6D5A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FB3, */ 0x6D79 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FB4, */ 0x6D59 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FB5, */ 0x6D8E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FB6, */ 0x6D95 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FB7, */ 0x6FE4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FB8, */ 0x6D85 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FB9, */ 0x6DF9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FBA, */ 0x6E15 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FBB, */ 0x6E0A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FBC, */ 0x6DB5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FBD, */ 0x6DC7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FBE, */ 0x6DE6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FBF, */ 0x6DB8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FC0, */ 0x6DC6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FC1, */ 0x6DEC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FC2, */ 0x6DDE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FC3, */ 0x6DCC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FC4, */ 0x6DE8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FC5, */ 0x6DD2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FC6, */ 0x6DC5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FC7, */ 0x6DFA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FC8, */ 0x6DD9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FC9, */ 0x6DE4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FCA, */ 0x6DD5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FCB, */ 0x6DEA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FCC, */ 0x6DEE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FCD, */ 0x6E2D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FCE, */ 0x6E6E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FCF, */ 0x6E2E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FD0, */ 0x6E19 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FD1, */ 0x6E72 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FD2, */ 0x6E5F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FD3, */ 0x6E3E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FD4, */ 0x6E23 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FD5, */ 0x6E6B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FD6, */ 0x6E2B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FD7, */ 0x6E76 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FD8, */ 0x6E4D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FD9, */ 0x6E1F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FDA, */ 0x6E43 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FDB, */ 0x6E3A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FDC, */ 0x6E4E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FDD, */ 0x6E24 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FDE, */ 0x6EFF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FDF, */ 0x6E1D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FE0, */ 0x6E38 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FE1, */ 0x6E82 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FE2, */ 0x6EAA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FE3, */ 0x6E98 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FE4, */ 0x6EC9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FE5, */ 0x6EB7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FE6, */ 0x6ED3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FE7, */ 0x6EBD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FE8, */ 0x6EAF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FE9, */ 0x6EC4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FEA, */ 0x6EB2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FEB, */ 0x6ED4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FEC, */ 0x6ED5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FED, */ 0x6E8F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FEE, */ 0x6EA5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FEF, */ 0x6EC2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FF0, */ 0x6E9F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FF1, */ 0x6F41 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FF2, */ 0x6F11 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FF3, */ 0x704C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FF4, */ 0x6EEC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FF5, */ 0x6EF8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FF6, */ 0x6EFE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FF7, */ 0x6F3F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FF8, */ 0x6EF2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FF9, */ 0x6F31 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FFA, */ 0x6EEF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FFB, */ 0x6F32 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0x9FFC, */ 0x6ECC },	// #CJK UNIFIED IDEOGRAPH
// sakaguchi 2020.12.14 ↓ テーブルを４つに分割
};

//===== 0xE0～0xEA ====================
OBJ_CDTBL CodeTable2[] =
{

//===== 0xE0 40～FC ( テーブル数 [189]  ）====================
{ /* 0xE040, */ 0x6F3E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE041, */ 0x6F13 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE042, */ 0x6EF7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE043, */ 0x6F86 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE044, */ 0x6F7A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE045, */ 0x6F78 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE046, */ 0x6F81 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE047, */ 0x6F80 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE048, */ 0x6F6F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE049, */ 0x6F5B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE04A, */ 0x6FF3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE04B, */ 0x6F6D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE04C, */ 0x6F82 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE04D, */ 0x6F7C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE04E, */ 0x6F58 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE04F, */ 0x6F8E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE050, */ 0x6F91 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE051, */ 0x6FC2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE052, */ 0x6F66 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE053, */ 0x6FB3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE054, */ 0x6FA3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE055, */ 0x6FA1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE056, */ 0x6FA4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE057, */ 0x6FB9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE058, */ 0x6FC6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE059, */ 0x6FAA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE05A, */ 0x6FDF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE05B, */ 0x6FD5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE05C, */ 0x6FEC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE05D, */ 0x6FD4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE05E, */ 0x6FD8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE05F, */ 0x6FF1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE060, */ 0x6FEE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE061, */ 0x6FDB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE062, */ 0x7009 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE063, */ 0x700B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE064, */ 0x6FFA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE065, */ 0x7011 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE066, */ 0x7001 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE067, */ 0x700F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE068, */ 0x6FFE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE069, */ 0x701B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE06A, */ 0x701A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE06B, */ 0x6F74 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE06C, */ 0x701D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE06D, */ 0x7018 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE06E, */ 0x701F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE06F, */ 0x7030 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE070, */ 0x703E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE071, */ 0x7032 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE072, */ 0x7051 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE073, */ 0x7063 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE074, */ 0x7099 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE075, */ 0x7092 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE076, */ 0x70AF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE077, */ 0x70F1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE078, */ 0x70AC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE079, */ 0x70B8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE07A, */ 0x70B3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE07B, */ 0x70AE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE07C, */ 0x70DF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE07D, */ 0x70CB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE07E, */ 0x70DD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE07F, */ 0x0000 },	// 		　#ダミー
{ /* 0xE080, */ 0x70D9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE081, */ 0x7109 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE082, */ 0x70FD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE083, */ 0x711C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE084, */ 0x7119 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE085, */ 0x7165 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE086, */ 0x7155 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE087, */ 0x7188 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE088, */ 0x7166 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE089, */ 0x7162 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE08A, */ 0x714C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE08B, */ 0x7156 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE08C, */ 0x716C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE08D, */ 0x718F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE08E, */ 0x71FB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE08F, */ 0x7184 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE090, */ 0x7195 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE091, */ 0x71A8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE092, */ 0x71AC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE093, */ 0x71D7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE094, */ 0x71B9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE095, */ 0x71BE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE096, */ 0x71D2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE097, */ 0x71C9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE098, */ 0x71D4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE099, */ 0x71CE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE09A, */ 0x71E0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE09B, */ 0x71EC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE09C, */ 0x71E7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE09D, */ 0x71F5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE09E, */ 0x71FC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE09F, */ 0x71F9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0A0, */ 0x71FF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0A1, */ 0x720D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0A2, */ 0x7210 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0A3, */ 0x721B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0A4, */ 0x7228 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0A5, */ 0x722D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0A6, */ 0x722C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0A7, */ 0x7230 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0A8, */ 0x7232 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0A9, */ 0x723B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0AA, */ 0x723C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0AB, */ 0x723F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0AC, */ 0x7240 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0AD, */ 0x7246 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0AE, */ 0x724B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0AF, */ 0x7258 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0B0, */ 0x7274 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0B1, */ 0x727E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0B2, */ 0x7282 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0B3, */ 0x7281 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0B4, */ 0x7287 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0B5, */ 0x7292 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0B6, */ 0x7296 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0B7, */ 0x72A2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0B8, */ 0x72A7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0B9, */ 0x72B9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0BA, */ 0x72B2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0BB, */ 0x72C3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0BC, */ 0x72C6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0BD, */ 0x72C4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0BE, */ 0x72CE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0BF, */ 0x72D2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0C0, */ 0x72E2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0C1, */ 0x72E0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0C2, */ 0x72E1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0C3, */ 0x72F9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0C4, */ 0x72F7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0C5, */ 0x500F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0C6, */ 0x7317 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0C7, */ 0x730A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0C8, */ 0x731C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0C9, */ 0x7316 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0CA, */ 0x731D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0CB, */ 0x7334 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0CC, */ 0x732F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0CD, */ 0x7329 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0CE, */ 0x7325 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0CF, */ 0x733E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0D0, */ 0x734E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0D1, */ 0x734F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0D2, */ 0x9ED8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0D3, */ 0x7357 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0D4, */ 0x736A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0D5, */ 0x7368 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0D6, */ 0x7370 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0D7, */ 0x7378 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0D8, */ 0x7375 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0D9, */ 0x737B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0DA, */ 0x737A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0DB, */ 0x73C8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0DC, */ 0x73B3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0DD, */ 0x73CE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0DE, */ 0x73BB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0DF, */ 0x73C0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0E0, */ 0x73E5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0E1, */ 0x73EE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0E2, */ 0x73DE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0E3, */ 0x74A2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0E4, */ 0x7405 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0E5, */ 0x746F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0E6, */ 0x7425 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0E7, */ 0x73F8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0E8, */ 0x7432 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0E9, */ 0x743A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0EA, */ 0x7455 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0EB, */ 0x743F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0EC, */ 0x745F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0ED, */ 0x7459 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0EE, */ 0x7441 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0EF, */ 0x745C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0F0, */ 0x7469 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0F1, */ 0x7470 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0F2, */ 0x7463 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0F3, */ 0x746A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0F4, */ 0x7476 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0F5, */ 0x747E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0F6, */ 0x748B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0F7, */ 0x749E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0F8, */ 0x74A7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0F9, */ 0x74CA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0FA, */ 0x74CF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0FB, */ 0x74D4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE0FC, */ 0x73F1 },	// #CJK UNIFIED IDEOGRAPH
//===== 0xE1 40～FC ( テーブル数 [189]  ）====================
{ /* 0xE140, */ 0x74E0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE141, */ 0x74E3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE142, */ 0x74E7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE143, */ 0x74E9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE144, */ 0x74EE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE145, */ 0x74F2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE146, */ 0x74F0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE147, */ 0x74F1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE148, */ 0x74F8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE149, */ 0x74F7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE14A, */ 0x7504 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE14B, */ 0x7503 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE14C, */ 0x7505 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE14D, */ 0x750C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE14E, */ 0x750E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE14F, */ 0x750D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE150, */ 0x7515 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE151, */ 0x7513 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE152, */ 0x751E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE153, */ 0x7526 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE154, */ 0x752C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE155, */ 0x753C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE156, */ 0x7544 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE157, */ 0x754D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE158, */ 0x754A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE159, */ 0x7549 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE15A, */ 0x755B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE15B, */ 0x7546 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE15C, */ 0x755A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE15D, */ 0x7569 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE15E, */ 0x7564 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE15F, */ 0x7567 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE160, */ 0x756B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE161, */ 0x756D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE162, */ 0x7578 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE163, */ 0x7576 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE164, */ 0x7586 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE165, */ 0x7587 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE166, */ 0x7574 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE167, */ 0x758A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE168, */ 0x7589 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE169, */ 0x7582 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE16A, */ 0x7594 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE16B, */ 0x759A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE16C, */ 0x759D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE16D, */ 0x75A5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE16E, */ 0x75A3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE16F, */ 0x75C2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE170, */ 0x75B3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE171, */ 0x75C3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE172, */ 0x75B5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE173, */ 0x75BD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE174, */ 0x75B8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE175, */ 0x75BC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE176, */ 0x75B1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE177, */ 0x75CD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE178, */ 0x75CA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE179, */ 0x75D2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE17A, */ 0x75D9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE17B, */ 0x75E3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE17C, */ 0x75DE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE17D, */ 0x75FE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE17E, */ 0x75FF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE17F, */ 0x0000 },	// 		　#ダミー
{ /* 0xE180, */ 0x75FC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE181, */ 0x7601 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE182, */ 0x75F0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE183, */ 0x75FA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE184, */ 0x75F2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE185, */ 0x75F3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE186, */ 0x760B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE187, */ 0x760D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE188, */ 0x7609 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE189, */ 0x761F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE18A, */ 0x7627 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE18B, */ 0x7620 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE18C, */ 0x7621 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE18D, */ 0x7622 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE18E, */ 0x7624 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE18F, */ 0x7634 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE190, */ 0x7630 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE191, */ 0x763B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE192, */ 0x7647 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE193, */ 0x7648 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE194, */ 0x7646 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE195, */ 0x765C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE196, */ 0x7658 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE197, */ 0x7661 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE198, */ 0x7662 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE199, */ 0x7668 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE19A, */ 0x7669 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE19B, */ 0x766A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE19C, */ 0x7667 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE19D, */ 0x766C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE19E, */ 0x7670 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE19F, */ 0x7672 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1A0, */ 0x7676 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1A1, */ 0x7678 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1A2, */ 0x767C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1A3, */ 0x7680 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1A4, */ 0x7683 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1A5, */ 0x7688 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1A6, */ 0x768B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1A7, */ 0x768E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1A8, */ 0x7696 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1A9, */ 0x7693 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1AA, */ 0x7699 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1AB, */ 0x769A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1AC, */ 0x76B0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1AD, */ 0x76B4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1AE, */ 0x76B8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1AF, */ 0x76B9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1B0, */ 0x76BA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1B1, */ 0x76C2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1B2, */ 0x76CD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1B3, */ 0x76D6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1B4, */ 0x76D2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1B5, */ 0x76DE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1B6, */ 0x76E1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1B7, */ 0x76E5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1B8, */ 0x76E7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1B9, */ 0x76EA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1BA, */ 0x862F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1BB, */ 0x76FB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1BC, */ 0x7708 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1BD, */ 0x7707 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1BE, */ 0x7704 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1BF, */ 0x7729 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1C0, */ 0x7724 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1C1, */ 0x771E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1C2, */ 0x7725 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1C3, */ 0x7726 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1C4, */ 0x771B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1C5, */ 0x7737 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1C6, */ 0x7738 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1C7, */ 0x7747 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1C8, */ 0x775A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1C9, */ 0x7768 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1CA, */ 0x776B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1CB, */ 0x775B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1CC, */ 0x7765 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1CD, */ 0x777F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1CE, */ 0x777E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1CF, */ 0x7779 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1D0, */ 0x778E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1D1, */ 0x778B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1D2, */ 0x7791 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1D3, */ 0x77A0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1D4, */ 0x779E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1D5, */ 0x77B0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1D6, */ 0x77B6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1D7, */ 0x77B9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1D8, */ 0x77BF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1D9, */ 0x77BC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1DA, */ 0x77BD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1DB, */ 0x77BB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1DC, */ 0x77C7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1DD, */ 0x77CD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1DE, */ 0x77D7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1DF, */ 0x77DA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1E0, */ 0x77DC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1E1, */ 0x77E3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1E2, */ 0x77EE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1E3, */ 0x77FC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1E4, */ 0x780C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1E5, */ 0x7812 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1E6, */ 0x7926 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1E7, */ 0x7820 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1E8, */ 0x792A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1E9, */ 0x7845 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1EA, */ 0x788E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1EB, */ 0x7874 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1EC, */ 0x7886 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1ED, */ 0x787C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1EE, */ 0x789A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1EF, */ 0x788C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1F0, */ 0x78A3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1F1, */ 0x78B5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1F2, */ 0x78AA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1F3, */ 0x78AF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1F4, */ 0x78D1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1F5, */ 0x78C6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1F6, */ 0x78CB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1F7, */ 0x78D4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1F8, */ 0x78BE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1F9, */ 0x78BC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1FA, */ 0x78C5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1FB, */ 0x78CA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE1FC, */ 0x78EC },	// #CJK UNIFIED IDEOGRAPH
//===== 0xE2 40～FC ( テーブル数 [189]  ）====================
{ /* 0xE240, */ 0x78E7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE241, */ 0x78DA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE242, */ 0x78FD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE243, */ 0x78F4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE244, */ 0x7907 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE245, */ 0x7912 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE246, */ 0x7911 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE247, */ 0x7919 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE248, */ 0x792C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE249, */ 0x792B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE24A, */ 0x7940 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE24B, */ 0x7960 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE24C, */ 0x7957 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE24D, */ 0x795F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE24E, */ 0x795A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE24F, */ 0x7955 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE250, */ 0x7953 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE251, */ 0x797A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE252, */ 0x797F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE253, */ 0x798A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE254, */ 0x799D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE255, */ 0x79A7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE256, */ 0x9F4B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE257, */ 0x79AA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE258, */ 0x79AE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE259, */ 0x79B3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE25A, */ 0x79B9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE25B, */ 0x79BA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE25C, */ 0x79C9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE25D, */ 0x79D5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE25E, */ 0x79E7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE25F, */ 0x79EC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE260, */ 0x79E1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE261, */ 0x79E3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE262, */ 0x7A08 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE263, */ 0x7A0D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE264, */ 0x7A18 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE265, */ 0x7A19 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE266, */ 0x7A20 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE267, */ 0x7A1F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE268, */ 0x7980 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE269, */ 0x7A31 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE26A, */ 0x7A3B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE26B, */ 0x7A3E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE26C, */ 0x7A37 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE26D, */ 0x7A43 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE26E, */ 0x7A57 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE26F, */ 0x7A49 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE270, */ 0x7A61 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE271, */ 0x7A62 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE272, */ 0x7A69 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE273, */ 0x9F9D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE274, */ 0x7A70 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE275, */ 0x7A79 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE276, */ 0x7A7D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE277, */ 0x7A88 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE278, */ 0x7A97 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE279, */ 0x7A95 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE27A, */ 0x7A98 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE27B, */ 0x7A96 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE27C, */ 0x7AA9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE27D, */ 0x7AC8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE27E, */ 0x7AB0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE27F, */ 0x0000 },	// 		　#ダミー
{ /* 0xE280, */ 0x7AB6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE281, */ 0x7AC5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE282, */ 0x7AC4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE283, */ 0x7ABF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE284, */ 0x9083 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE285, */ 0x7AC7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE286, */ 0x7ACA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE287, */ 0x7ACD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE288, */ 0x7ACF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE289, */ 0x7AD5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE28A, */ 0x7AD3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE28B, */ 0x7AD9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE28C, */ 0x7ADA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE28D, */ 0x7ADD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE28E, */ 0x7AE1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE28F, */ 0x7AE2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE290, */ 0x7AE6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE291, */ 0x7AED },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE292, */ 0x7AF0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE293, */ 0x7B02 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE294, */ 0x7B0F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE295, */ 0x7B0A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE296, */ 0x7B06 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE297, */ 0x7B33 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE298, */ 0x7B18 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE299, */ 0x7B19 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE29A, */ 0x7B1E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE29B, */ 0x7B35 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE29C, */ 0x7B28 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE29D, */ 0x7B36 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE29E, */ 0x7B50 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE29F, */ 0x7B7A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2A0, */ 0x7B04 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2A1, */ 0x7B4D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2A2, */ 0x7B0B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2A3, */ 0x7B4C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2A4, */ 0x7B45 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2A5, */ 0x7B75 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2A6, */ 0x7B65 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2A7, */ 0x7B74 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2A8, */ 0x7B67 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2A9, */ 0x7B70 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2AA, */ 0x7B71 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2AB, */ 0x7B6C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2AC, */ 0x7B6E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2AD, */ 0x7B9D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2AE, */ 0x7B98 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2AF, */ 0x7B9F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2B0, */ 0x7B8D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2B1, */ 0x7B9C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2B2, */ 0x7B9A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2B3, */ 0x7B8B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2B4, */ 0x7B92 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2B5, */ 0x7B8F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2B6, */ 0x7B5D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2B7, */ 0x7B99 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2B8, */ 0x7BCB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2B9, */ 0x7BC1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2BA, */ 0x7BCC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2BB, */ 0x7BCF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2BC, */ 0x7BB4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2BD, */ 0x7BC6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2BE, */ 0x7BDD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2BF, */ 0x7BE9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2C0, */ 0x7C11 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2C1, */ 0x7C14 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2C2, */ 0x7BE6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2C3, */ 0x7BE5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2C4, */ 0x7C60 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2C5, */ 0x7C00 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2C6, */ 0x7C07 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2C7, */ 0x7C13 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2C8, */ 0x7BF3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2C9, */ 0x7BF7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2CA, */ 0x7C17 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2CB, */ 0x7C0D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2CC, */ 0x7BF6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2CD, */ 0x7C23 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2CE, */ 0x7C27 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2CF, */ 0x7C2A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2D0, */ 0x7C1F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2D1, */ 0x7C37 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2D2, */ 0x7C2B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2D3, */ 0x7C3D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2D4, */ 0x7C4C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2D5, */ 0x7C43 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2D6, */ 0x7C54 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2D7, */ 0x7C4F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2D8, */ 0x7C40 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2D9, */ 0x7C50 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2DA, */ 0x7C58 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2DB, */ 0x7C5F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2DC, */ 0x7C64 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2DD, */ 0x7C56 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2DE, */ 0x7C65 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2DF, */ 0x7C6C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2E0, */ 0x7C75 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2E1, */ 0x7C83 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2E2, */ 0x7C90 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2E3, */ 0x7CA4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2E4, */ 0x7CAD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2E5, */ 0x7CA2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2E6, */ 0x7CAB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2E7, */ 0x7CA1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2E8, */ 0x7CA8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2E9, */ 0x7CB3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2EA, */ 0x7CB2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2EB, */ 0x7CB1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2EC, */ 0x7CAE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2ED, */ 0x7CB9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2EE, */ 0x7CBD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2EF, */ 0x7CC0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2F0, */ 0x7CC5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2F1, */ 0x7CC2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2F2, */ 0x7CD8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2F3, */ 0x7CD2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2F4, */ 0x7CDC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2F5, */ 0x7CE2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2F6, */ 0x9B3B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2F7, */ 0x7CEF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2F8, */ 0x7CF2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2F9, */ 0x7CF4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2FA, */ 0x7CF6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2FB, */ 0x7CFA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE2FC, */ 0x7D06 },	// #CJK UNIFIED IDEOGRAPH
//===== 0xE3 40～FC ( テーブル数 [189]  ）====================
{ /* 0xE340, */ 0x7D02 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE341, */ 0x7D1C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE342, */ 0x7D15 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE343, */ 0x7D0A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE344, */ 0x7D45 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE345, */ 0x7D4B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE346, */ 0x7D2E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE347, */ 0x7D32 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE348, */ 0x7D3F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE349, */ 0x7D35 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE34A, */ 0x7D46 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE34B, */ 0x7D73 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE34C, */ 0x7D56 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE34D, */ 0x7D4E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE34E, */ 0x7D72 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE34F, */ 0x7D68 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE350, */ 0x7D6E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE351, */ 0x7D4F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE352, */ 0x7D63 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE353, */ 0x7D93 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE354, */ 0x7D89 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE355, */ 0x7D5B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE356, */ 0x7D8F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE357, */ 0x7D7D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE358, */ 0x7D9B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE359, */ 0x7DBA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE35A, */ 0x7DAE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE35B, */ 0x7DA3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE35C, */ 0x7DB5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE35D, */ 0x7DC7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE35E, */ 0x7DBD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE35F, */ 0x7DAB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE360, */ 0x7E3D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE361, */ 0x7DA2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE362, */ 0x7DAF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE363, */ 0x7DDC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE364, */ 0x7DB8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE365, */ 0x7D9F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE366, */ 0x7DB0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE367, */ 0x7DD8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE368, */ 0x7DDD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE369, */ 0x7DE4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE36A, */ 0x7DDE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE36B, */ 0x7DFB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE36C, */ 0x7DF2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE36D, */ 0x7DE1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE36E, */ 0x7E05 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE36F, */ 0x7E0A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE370, */ 0x7E23 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE371, */ 0x7E21 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE372, */ 0x7E12 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE373, */ 0x7E31 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE374, */ 0x7E1F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE375, */ 0x7E09 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE376, */ 0x7E0B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE377, */ 0x7E22 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE378, */ 0x7E46 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE379, */ 0x7E66 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE37A, */ 0x7E3B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE37B, */ 0x7E35 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE37C, */ 0x7E39 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE37D, */ 0x7E43 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE37E, */ 0x7E37 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE37F, */ 0x0000 },	// 		　#ダミー
{ /* 0xE380, */ 0x7E32 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE381, */ 0x7E3A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE382, */ 0x7E67 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE383, */ 0x7E5D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE384, */ 0x7E56 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE385, */ 0x7E5E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE386, */ 0x7E59 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE387, */ 0x7E5A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE388, */ 0x7E79 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE389, */ 0x7E6A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE38A, */ 0x7E69 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE38B, */ 0x7E7C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE38C, */ 0x7E7B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE38D, */ 0x7E83 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE38E, */ 0x7DD5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE38F, */ 0x7E7D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE390, */ 0x8FAE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE391, */ 0x7E7F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE392, */ 0x7E88 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE393, */ 0x7E89 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE394, */ 0x7E8C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE395, */ 0x7E92 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE396, */ 0x7E90 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE397, */ 0x7E93 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE398, */ 0x7E94 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE399, */ 0x7E96 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE39A, */ 0x7E8E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE39B, */ 0x7E9B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE39C, */ 0x7E9C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE39D, */ 0x7F38 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE39E, */ 0x7F3A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE39F, */ 0x7F45 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3A0, */ 0x7F4C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3A1, */ 0x7F4D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3A2, */ 0x7F4E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3A3, */ 0x7F50 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3A4, */ 0x7F51 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3A5, */ 0x7F55 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3A6, */ 0x7F54 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3A7, */ 0x7F58 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3A8, */ 0x7F5F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3A9, */ 0x7F60 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3AA, */ 0x7F68 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3AB, */ 0x7F69 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3AC, */ 0x7F67 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3AD, */ 0x7F78 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3AE, */ 0x7F82 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3AF, */ 0x7F86 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3B0, */ 0x7F83 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3B1, */ 0x7F88 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3B2, */ 0x7F87 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3B3, */ 0x7F8C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3B4, */ 0x7F94 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3B5, */ 0x7F9E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3B6, */ 0x7F9D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3B7, */ 0x7F9A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3B8, */ 0x7FA3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3B9, */ 0x7FAF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3BA, */ 0x7FB2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3BB, */ 0x7FB9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3BC, */ 0x7FAE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3BD, */ 0x7FB6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3BE, */ 0x7FB8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3BF, */ 0x8B71 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3C0, */ 0x7FC5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3C1, */ 0x7FC6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3C2, */ 0x7FCA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3C3, */ 0x7FD5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3C4, */ 0x7FD4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3C5, */ 0x7FE1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3C6, */ 0x7FE6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3C7, */ 0x7FE9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3C8, */ 0x7FF3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3C9, */ 0x7FF9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3CA, */ 0x98DC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3CB, */ 0x8006 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3CC, */ 0x8004 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3CD, */ 0x800B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3CE, */ 0x8012 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3CF, */ 0x8018 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3D0, */ 0x8019 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3D1, */ 0x801C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3D2, */ 0x8021 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3D3, */ 0x8028 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3D4, */ 0x803F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3D5, */ 0x803B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3D6, */ 0x804A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3D7, */ 0x8046 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3D8, */ 0x8052 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3D9, */ 0x8058 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3DA, */ 0x805A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3DB, */ 0x805F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3DC, */ 0x8062 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3DD, */ 0x8068 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3DE, */ 0x8073 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3DF, */ 0x8072 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3E0, */ 0x8070 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3E1, */ 0x8076 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3E2, */ 0x8079 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3E3, */ 0x807D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3E4, */ 0x807F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3E5, */ 0x8084 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3E6, */ 0x8086 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3E7, */ 0x8085 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3E8, */ 0x809B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3E9, */ 0x8093 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3EA, */ 0x809A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3EB, */ 0x80AD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3EC, */ 0x5190 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3ED, */ 0x80AC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3EE, */ 0x80DB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3EF, */ 0x80E5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3F0, */ 0x80D9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3F1, */ 0x80DD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3F2, */ 0x80C4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3F3, */ 0x80DA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3F4, */ 0x80D6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3F5, */ 0x8109 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3F6, */ 0x80EF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3F7, */ 0x80F1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3F8, */ 0x811B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3F9, */ 0x8129 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3FA, */ 0x8123 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3FB, */ 0x812F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE3FC, */ 0x814B },	// #CJK UNIFIED IDEOGRAPH
//===== 0xE4 40～FC ( テーブル数 [189]  ）====================
{ /* 0xE440, */ 0x968B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE441, */ 0x8146 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE442, */ 0x813E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE443, */ 0x8153 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE444, */ 0x8151 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE445, */ 0x80FC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE446, */ 0x8171 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE447, */ 0x816E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE448, */ 0x8165 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE449, */ 0x8166 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE44A, */ 0x8174 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE44B, */ 0x8183 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE44C, */ 0x8188 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE44D, */ 0x818A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE44E, */ 0x8180 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE44F, */ 0x8182 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE450, */ 0x81A0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE451, */ 0x8195 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE452, */ 0x81A4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE453, */ 0x81A3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE454, */ 0x815F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE455, */ 0x8193 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE456, */ 0x81A9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE457, */ 0x81B0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE458, */ 0x81B5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE459, */ 0x81BE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE45A, */ 0x81B8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE45B, */ 0x81BD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE45C, */ 0x81C0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE45D, */ 0x81C2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE45E, */ 0x81BA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE45F, */ 0x81C9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE460, */ 0x81CD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE461, */ 0x81D1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE462, */ 0x81D9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE463, */ 0x81D8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE464, */ 0x81C8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE465, */ 0x81DA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE466, */ 0x81DF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE467, */ 0x81E0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE468, */ 0x81E7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE469, */ 0x81FA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE46A, */ 0x81FB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE46B, */ 0x81FE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE46C, */ 0x8201 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE46D, */ 0x8202 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE46E, */ 0x8205 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE46F, */ 0x8207 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE470, */ 0x820A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE471, */ 0x820D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE472, */ 0x8210 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE473, */ 0x8216 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE474, */ 0x8229 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE475, */ 0x822B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE476, */ 0x8238 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE477, */ 0x8233 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE478, */ 0x8240 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE479, */ 0x8259 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE47A, */ 0x8258 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE47B, */ 0x825D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE47C, */ 0x825A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE47D, */ 0x825F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE47E, */ 0x8264 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE47F, */ 0x0000 },	// 		　#ダミー
{ /* 0xE480, */ 0x8262 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE481, */ 0x8268 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE482, */ 0x826A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE483, */ 0x826B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE484, */ 0x822E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE485, */ 0x8271 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE486, */ 0x8277 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE487, */ 0x8278 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE488, */ 0x827E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE489, */ 0x828D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE48A, */ 0x8292 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE48B, */ 0x82AB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE48C, */ 0x829F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE48D, */ 0x82BB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE48E, */ 0x82AC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE48F, */ 0x82E1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE490, */ 0x82E3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE491, */ 0x82DF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE492, */ 0x82D2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE493, */ 0x82F4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE494, */ 0x82F3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE495, */ 0x82FA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE496, */ 0x8393 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE497, */ 0x8303 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE498, */ 0x82FB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE499, */ 0x82F9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE49A, */ 0x82DE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE49B, */ 0x8306 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE49C, */ 0x82DC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE49D, */ 0x8309 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE49E, */ 0x82D9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE49F, */ 0x8335 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4A0, */ 0x8334 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4A1, */ 0x8316 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4A2, */ 0x8332 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4A3, */ 0x8331 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4A4, */ 0x8340 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4A5, */ 0x8339 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4A6, */ 0x8350 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4A7, */ 0x8345 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4A8, */ 0x832F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4A9, */ 0x832B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4AA, */ 0x8317 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4AB, */ 0x8318 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4AC, */ 0x8385 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4AD, */ 0x839A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4AE, */ 0x83AA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4AF, */ 0x839F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4B0, */ 0x83A2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4B1, */ 0x8396 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4B2, */ 0x8323 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4B3, */ 0x838E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4B4, */ 0x8387 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4B5, */ 0x838A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4B6, */ 0x837C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4B7, */ 0x83B5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4B8, */ 0x8373 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4B9, */ 0x8375 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4BA, */ 0x83A0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4BB, */ 0x8389 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4BC, */ 0x83A8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4BD, */ 0x83F4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4BE, */ 0x8413 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4BF, */ 0x83EB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4C0, */ 0x83CE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4C1, */ 0x83FD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4C2, */ 0x8403 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4C3, */ 0x83D8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4C4, */ 0x840B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4C5, */ 0x83C1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4C6, */ 0x83F7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4C7, */ 0x8407 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4C8, */ 0x83E0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4C9, */ 0x83F2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4CA, */ 0x840D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4CB, */ 0x8422 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4CC, */ 0x8420 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4CD, */ 0x83BD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4CE, */ 0x8438 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4CF, */ 0x8506 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4D0, */ 0x83FB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4D1, */ 0x846D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4D2, */ 0x842A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4D3, */ 0x843C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4D4, */ 0x855A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4D5, */ 0x8484 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4D6, */ 0x8477 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4D7, */ 0x846B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4D8, */ 0x84AD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4D9, */ 0x846E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4DA, */ 0x8482 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4DB, */ 0x8469 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4DC, */ 0x8446 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4DD, */ 0x842C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4DE, */ 0x846F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4DF, */ 0x8479 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4E0, */ 0x8435 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4E1, */ 0x84CA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4E2, */ 0x8462 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4E3, */ 0x84B9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4E4, */ 0x84BF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4E5, */ 0x849F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4E6, */ 0x84D9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4E7, */ 0x84CD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4E8, */ 0x84BB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4E9, */ 0x84DA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4EA, */ 0x84D0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4EB, */ 0x84C1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4EC, */ 0x84C6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4ED, */ 0x84D6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4EE, */ 0x84A1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4EF, */ 0x8521 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4F0, */ 0x84FF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4F1, */ 0x84F4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4F2, */ 0x8517 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4F3, */ 0x8518 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4F4, */ 0x852C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4F5, */ 0x851F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4F6, */ 0x8515 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4F7, */ 0x8514 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4F8, */ 0x84FC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4F9, */ 0x8540 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4FA, */ 0x8563 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4FB, */ 0x8558 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE4FC, */ 0x8548 },	// #CJK UNIFIED IDEOGRAPH
//===== 0xE5 40～FC ( テーブル数 [189]  ）====================
{ /* 0xE540, */ 0x8541 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE541, */ 0x8602 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE542, */ 0x854B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE543, */ 0x8555 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE544, */ 0x8580 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE545, */ 0x85A4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE546, */ 0x8588 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE547, */ 0x8591 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE548, */ 0x858A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE549, */ 0x85A8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE54A, */ 0x856D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE54B, */ 0x8594 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE54C, */ 0x859B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE54D, */ 0x85EA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE54E, */ 0x8587 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE54F, */ 0x859C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE550, */ 0x8577 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE551, */ 0x857E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE552, */ 0x8590 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE553, */ 0x85C9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE554, */ 0x85BA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE555, */ 0x85CF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE556, */ 0x85B9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE557, */ 0x85D0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE558, */ 0x85D5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE559, */ 0x85DD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE55A, */ 0x85E5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE55B, */ 0x85DC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE55C, */ 0x85F9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE55D, */ 0x860A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE55E, */ 0x8613 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE55F, */ 0x860B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE560, */ 0x85FE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE561, */ 0x85FA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE562, */ 0x8606 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE563, */ 0x8622 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE564, */ 0x861A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE565, */ 0x8630 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE566, */ 0x863F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE567, */ 0x864D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE568, */ 0x4E55 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE569, */ 0x8654 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE56A, */ 0x865F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE56B, */ 0x8667 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE56C, */ 0x8671 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE56D, */ 0x8693 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE56E, */ 0x86A3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE56F, */ 0x86A9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE570, */ 0x86AA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE571, */ 0x868B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE572, */ 0x868C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE573, */ 0x86B6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE574, */ 0x86AF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE575, */ 0x86C4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE576, */ 0x86C6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE577, */ 0x86B0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE578, */ 0x86C9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE579, */ 0x8823 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE57A, */ 0x86AB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE57B, */ 0x86D4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE57C, */ 0x86DE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE57D, */ 0x86E9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE57E, */ 0x86EC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE57F, */ 0x0000 },	// 		　#ダミー
{ /* 0xE580, */ 0x86DF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE581, */ 0x86DB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE582, */ 0x86EF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE583, */ 0x8712 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE584, */ 0x8706 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE585, */ 0x8708 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE586, */ 0x8700 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE587, */ 0x8703 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE588, */ 0x86FB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE589, */ 0x8711 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE58A, */ 0x8709 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE58B, */ 0x870D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE58C, */ 0x86F9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE58D, */ 0x870A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE58E, */ 0x8734 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE58F, */ 0x873F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE590, */ 0x8737 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE591, */ 0x873B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE592, */ 0x8725 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE593, */ 0x8729 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE594, */ 0x871A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE595, */ 0x8760 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE596, */ 0x875F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE597, */ 0x8778 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE598, */ 0x874C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE599, */ 0x874E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE59A, */ 0x8774 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE59B, */ 0x8757 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE59C, */ 0x8768 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE59D, */ 0x876E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE59E, */ 0x8759 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE59F, */ 0x8753 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5A0, */ 0x8763 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5A1, */ 0x876A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5A2, */ 0x8805 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5A3, */ 0x87A2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5A4, */ 0x879F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5A5, */ 0x8782 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5A6, */ 0x87AF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5A7, */ 0x87CB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5A8, */ 0x87BD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5A9, */ 0x87C0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5AA, */ 0x87D0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5AB, */ 0x96D6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5AC, */ 0x87AB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5AD, */ 0x87C4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5AE, */ 0x87B3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5AF, */ 0x87C7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5B0, */ 0x87C6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5B1, */ 0x87BB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5B2, */ 0x87EF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5B3, */ 0x87F2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5B4, */ 0x87E0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5B5, */ 0x880F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5B6, */ 0x880D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5B7, */ 0x87FE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5B8, */ 0x87F6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5B9, */ 0x87F7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5BA, */ 0x880E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5BB, */ 0x87D2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5BC, */ 0x8811 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5BD, */ 0x8816 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5BE, */ 0x8815 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5BF, */ 0x8822 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5C0, */ 0x8821 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5C1, */ 0x8831 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5C2, */ 0x8836 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5C3, */ 0x8839 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5C4, */ 0x8827 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5C5, */ 0x883B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5C6, */ 0x8844 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5C7, */ 0x8842 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5C8, */ 0x8852 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5C9, */ 0x8859 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5CA, */ 0x885E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5CB, */ 0x8862 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5CC, */ 0x886B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5CD, */ 0x8881 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5CE, */ 0x887E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5CF, */ 0x889E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5D0, */ 0x8875 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5D1, */ 0x887D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5D2, */ 0x88B5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5D3, */ 0x8872 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5D4, */ 0x8882 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5D5, */ 0x8897 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5D6, */ 0x8892 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5D7, */ 0x88AE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5D8, */ 0x8899 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5D9, */ 0x88A2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5DA, */ 0x888D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5DB, */ 0x88A4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5DC, */ 0x88B0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5DD, */ 0x88BF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5DE, */ 0x88B1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5DF, */ 0x88C3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5E0, */ 0x88C4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5E1, */ 0x88D4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5E2, */ 0x88D8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5E3, */ 0x88D9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5E4, */ 0x88DD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5E5, */ 0x88F9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5E6, */ 0x8902 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5E7, */ 0x88FC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5E8, */ 0x88F4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5E9, */ 0x88E8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5EA, */ 0x88F2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5EB, */ 0x8904 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5EC, */ 0x890C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5ED, */ 0x890A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5EE, */ 0x8913 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5EF, */ 0x8943 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5F0, */ 0x891E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5F1, */ 0x8925 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5F2, */ 0x892A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5F3, */ 0x892B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5F4, */ 0x8941 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5F5, */ 0x8944 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5F6, */ 0x893B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5F7, */ 0x8936 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5F8, */ 0x8938 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5F9, */ 0x894C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5FA, */ 0x891D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5FB, */ 0x8960 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE5FC, */ 0x895E },	// #CJK UNIFIED IDEOGRAPH
//===== 0xE6 40～FC ( テーブル数 [189]  ）====================
{ /* 0xE640, */ 0x8966 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE641, */ 0x8964 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE642, */ 0x896D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE643, */ 0x896A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE644, */ 0x896F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE645, */ 0x8974 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE646, */ 0x8977 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE647, */ 0x897E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE648, */ 0x8983 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE649, */ 0x8988 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE64A, */ 0x898A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE64B, */ 0x8993 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE64C, */ 0x8998 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE64D, */ 0x89A1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE64E, */ 0x89A9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE64F, */ 0x89A6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE650, */ 0x89AC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE651, */ 0x89AF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE652, */ 0x89B2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE653, */ 0x89BA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE654, */ 0x89BD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE655, */ 0x89BF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE656, */ 0x89C0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE657, */ 0x89DA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE658, */ 0x89DC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE659, */ 0x89DD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE65A, */ 0x89E7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE65B, */ 0x89F4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE65C, */ 0x89F8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE65D, */ 0x8A03 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE65E, */ 0x8A16 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE65F, */ 0x8A10 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE660, */ 0x8A0C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE661, */ 0x8A1B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE662, */ 0x8A1D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE663, */ 0x8A25 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE664, */ 0x8A36 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE665, */ 0x8A41 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE666, */ 0x8A5B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE667, */ 0x8A52 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE668, */ 0x8A46 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE669, */ 0x8A48 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE66A, */ 0x8A7C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE66B, */ 0x8A6D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE66C, */ 0x8A6C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE66D, */ 0x8A62 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE66E, */ 0x8A85 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE66F, */ 0x8A82 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE670, */ 0x8A84 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE671, */ 0x8AA8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE672, */ 0x8AA1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE673, */ 0x8A91 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE674, */ 0x8AA5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE675, */ 0x8AA6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE676, */ 0x8A9A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE677, */ 0x8AA3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE678, */ 0x8AC4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE679, */ 0x8ACD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE67A, */ 0x8AC2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE67B, */ 0x8ADA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE67C, */ 0x8AEB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE67D, */ 0x8AF3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE67E, */ 0x8AE7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE67F, */ 0x0000 },	// 		　#ダミー
{ /* 0xE680, */ 0x8AE4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE681, */ 0x8AF1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE682, */ 0x8B14 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE683, */ 0x8AE0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE684, */ 0x8AE2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE685, */ 0x8AF7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE686, */ 0x8ADE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE687, */ 0x8ADB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE688, */ 0x8B0C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE689, */ 0x8B07 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE68A, */ 0x8B1A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE68B, */ 0x8AE1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE68C, */ 0x8B16 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE68D, */ 0x8B10 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE68E, */ 0x8B17 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE68F, */ 0x8B20 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE690, */ 0x8B33 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE691, */ 0x97AB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE692, */ 0x8B26 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE693, */ 0x8B2B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE694, */ 0x8B3E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE695, */ 0x8B28 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE696, */ 0x8B41 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE697, */ 0x8B4C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE698, */ 0x8B4F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE699, */ 0x8B4E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE69A, */ 0x8B49 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE69B, */ 0x8B56 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE69C, */ 0x8B5B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE69D, */ 0x8B5A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE69E, */ 0x8B6B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE69F, */ 0x8B5F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6A0, */ 0x8B6C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6A1, */ 0x8B6F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6A2, */ 0x8B74 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6A3, */ 0x8B7D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6A4, */ 0x8B80 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6A5, */ 0x8B8C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6A6, */ 0x8B8E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6A7, */ 0x8B92 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6A8, */ 0x8B93 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6A9, */ 0x8B96 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6AA, */ 0x8B99 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6AB, */ 0x8B9A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6AC, */ 0x8C3A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6AD, */ 0x8C41 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6AE, */ 0x8C3F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6AF, */ 0x8C48 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6B0, */ 0x8C4C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6B1, */ 0x8C4E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6B2, */ 0x8C50 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6B3, */ 0x8C55 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6B4, */ 0x8C62 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6B5, */ 0x8C6C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6B6, */ 0x8C78 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6B7, */ 0x8C7A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6B8, */ 0x8C82 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6B9, */ 0x8C89 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6BA, */ 0x8C85 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6BB, */ 0x8C8A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6BC, */ 0x8C8D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6BD, */ 0x8C8E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6BE, */ 0x8C94 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6BF, */ 0x8C7C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6C0, */ 0x8C98 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6C1, */ 0x621D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6C2, */ 0x8CAD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6C3, */ 0x8CAA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6C4, */ 0x8CBD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6C5, */ 0x8CB2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6C6, */ 0x8CB3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6C7, */ 0x8CAE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6C8, */ 0x8CB6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6C9, */ 0x8CC8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6CA, */ 0x8CC1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6CB, */ 0x8CE4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6CC, */ 0x8CE3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6CD, */ 0x8CDA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6CE, */ 0x8CFD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6CF, */ 0x8CFA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6D0, */ 0x8CFB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6D1, */ 0x8D04 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6D2, */ 0x8D05 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6D3, */ 0x8D0A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6D4, */ 0x8D07 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6D5, */ 0x8D0F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6D6, */ 0x8D0D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6D7, */ 0x8D10 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6D8, */ 0x9F4E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6D9, */ 0x8D13 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6DA, */ 0x8CCD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6DB, */ 0x8D14 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6DC, */ 0x8D16 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6DD, */ 0x8D67 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6DE, */ 0x8D6D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6DF, */ 0x8D71 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6E0, */ 0x8D73 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6E1, */ 0x8D81 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6E2, */ 0x8D99 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6E3, */ 0x8DC2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6E4, */ 0x8DBE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6E5, */ 0x8DBA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6E6, */ 0x8DCF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6E7, */ 0x8DDA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6E8, */ 0x8DD6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6E9, */ 0x8DCC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6EA, */ 0x8DDB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6EB, */ 0x8DCB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6EC, */ 0x8DEA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6ED, */ 0x8DEB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6EE, */ 0x8DDF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6EF, */ 0x8DE3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6F0, */ 0x8DFC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6F1, */ 0x8E08 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6F2, */ 0x8E09 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6F3, */ 0x8DFF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6F4, */ 0x8E1D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6F5, */ 0x8E1E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6F6, */ 0x8E10 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6F7, */ 0x8E1F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6F8, */ 0x8E42 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6F9, */ 0x8E35 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6FA, */ 0x8E30 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6FB, */ 0x8E34 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE6FC, */ 0x8E4A },	// #CJK UNIFIED IDEOGRAPH
//===== 0xE7 40～FC ( テーブル数 [189]  ）====================
{ /* 0xE740, */ 0x8E47 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE741, */ 0x8E49 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE742, */ 0x8E4C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE743, */ 0x8E50 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE744, */ 0x8E48 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE745, */ 0x8E59 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE746, */ 0x8E64 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE747, */ 0x8E60 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE748, */ 0x8E2A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE749, */ 0x8E63 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE74A, */ 0x8E55 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE74B, */ 0x8E76 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE74C, */ 0x8E72 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE74D, */ 0x8E7C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE74E, */ 0x8E81 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE74F, */ 0x8E87 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE750, */ 0x8E85 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE751, */ 0x8E84 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE752, */ 0x8E8B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE753, */ 0x8E8A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE754, */ 0x8E93 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE755, */ 0x8E91 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE756, */ 0x8E94 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE757, */ 0x8E99 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE758, */ 0x8EAA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE759, */ 0x8EA1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE75A, */ 0x8EAC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE75B, */ 0x8EB0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE75C, */ 0x8EC6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE75D, */ 0x8EB1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE75E, */ 0x8EBE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE75F, */ 0x8EC5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE760, */ 0x8EC8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE761, */ 0x8ECB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE762, */ 0x8EDB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE763, */ 0x8EE3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE764, */ 0x8EFC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE765, */ 0x8EFB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE766, */ 0x8EEB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE767, */ 0x8EFE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE768, */ 0x8F0A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE769, */ 0x8F05 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE76A, */ 0x8F15 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE76B, */ 0x8F12 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE76C, */ 0x8F19 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE76D, */ 0x8F13 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE76E, */ 0x8F1C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE76F, */ 0x8F1F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE770, */ 0x8F1B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE771, */ 0x8F0C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE772, */ 0x8F26 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE773, */ 0x8F33 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE774, */ 0x8F3B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE775, */ 0x8F39 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE776, */ 0x8F45 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE777, */ 0x8F42 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE778, */ 0x8F3E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE779, */ 0x8F4C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE77A, */ 0x8F49 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE77B, */ 0x8F46 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE77C, */ 0x8F4E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE77D, */ 0x8F57 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE77E, */ 0x8F5C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE77F, */ 0x0000 },	// 		　#ダミー
{ /* 0xE780, */ 0x8F62 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE781, */ 0x8F63 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE782, */ 0x8F64 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE783, */ 0x8F9C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE784, */ 0x8F9F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE785, */ 0x8FA3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE786, */ 0x8FAD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE787, */ 0x8FAF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE788, */ 0x8FB7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE789, */ 0x8FDA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE78A, */ 0x8FE5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE78B, */ 0x8FE2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE78C, */ 0x8FEA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE78D, */ 0x8FEF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE78E, */ 0x9087 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE78F, */ 0x8FF4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE790, */ 0x9005 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE791, */ 0x8FF9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE792, */ 0x8FFA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE793, */ 0x9011 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE794, */ 0x9015 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE795, */ 0x9021 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE796, */ 0x900D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE797, */ 0x901E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE798, */ 0x9016 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE799, */ 0x900B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE79A, */ 0x9027 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE79B, */ 0x9036 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE79C, */ 0x9035 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE79D, */ 0x9039 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE79E, */ 0x8FF8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE79F, */ 0x904F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7A0, */ 0x9050 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7A1, */ 0x9051 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7A2, */ 0x9052 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7A3, */ 0x900E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7A4, */ 0x9049 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7A5, */ 0x903E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7A6, */ 0x9056 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7A7, */ 0x9058 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7A8, */ 0x905E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7A9, */ 0x9068 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7AA, */ 0x906F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7AB, */ 0x9076 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7AC, */ 0x96A8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7AD, */ 0x9072 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7AE, */ 0x9082 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7AF, */ 0x907D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7B0, */ 0x9081 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7B1, */ 0x9080 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7B2, */ 0x908A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7B3, */ 0x9089 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7B4, */ 0x908F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7B5, */ 0x90A8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7B6, */ 0x90AF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7B7, */ 0x90B1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7B8, */ 0x90B5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7B9, */ 0x90E2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7BA, */ 0x90E4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7BB, */ 0x6248 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7BC, */ 0x90DB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7BD, */ 0x9102 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7BE, */ 0x9112 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7BF, */ 0x9119 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7C0, */ 0x9132 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7C1, */ 0x9130 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7C2, */ 0x914A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7C3, */ 0x9156 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7C4, */ 0x9158 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7C5, */ 0x9163 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7C6, */ 0x9165 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7C7, */ 0x9169 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7C8, */ 0x9173 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7C9, */ 0x9172 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7CA, */ 0x918B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7CB, */ 0x9189 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7CC, */ 0x9182 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7CD, */ 0x91A2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7CE, */ 0x91AB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7CF, */ 0x91AF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7D0, */ 0x91AA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7D1, */ 0x91B5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7D2, */ 0x91B4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7D3, */ 0x91BA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7D4, */ 0x91C0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7D5, */ 0x91C1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7D6, */ 0x91C9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7D7, */ 0x91CB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7D8, */ 0x91D0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7D9, */ 0x91D6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7DA, */ 0x91DF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7DB, */ 0x91E1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7DC, */ 0x91DB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7DD, */ 0x91FC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7DE, */ 0x91F5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7DF, */ 0x91F6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7E0, */ 0x921E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7E1, */ 0x91FF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7E2, */ 0x9214 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7E3, */ 0x922C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7E4, */ 0x9215 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7E5, */ 0x9211 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7E6, */ 0x925E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7E7, */ 0x9257 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7E8, */ 0x9245 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7E9, */ 0x9249 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7EA, */ 0x9264 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7EB, */ 0x9248 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7EC, */ 0x9295 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7ED, */ 0x923F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7EE, */ 0x924B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7EF, */ 0x9250 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7F0, */ 0x929C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7F1, */ 0x9296 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7F2, */ 0x9293 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7F3, */ 0x929B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7F4, */ 0x925A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7F5, */ 0x92CF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7F6, */ 0x92B9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7F7, */ 0x92B7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7F8, */ 0x92E9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7F9, */ 0x930F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7FA, */ 0x92FA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7FB, */ 0x9344 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE7FC, */ 0x932E },	// #CJK UNIFIED IDEOGRAPH
//===== 0xE8 40～FC ( テーブル数 [189]  ）====================
{ /* 0xE840, */ 0x9319 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE841, */ 0x9322 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE842, */ 0x931A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE843, */ 0x9323 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE844, */ 0x933A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE845, */ 0x9335 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE846, */ 0x933B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE847, */ 0x935C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE848, */ 0x9360 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE849, */ 0x937C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE84A, */ 0x936E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE84B, */ 0x9356 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE84C, */ 0x93B0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE84D, */ 0x93AC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE84E, */ 0x93AD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE84F, */ 0x9394 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE850, */ 0x93B9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE851, */ 0x93D6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE852, */ 0x93D7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE853, */ 0x93E8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE854, */ 0x93E5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE855, */ 0x93D8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE856, */ 0x93C3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE857, */ 0x93DD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE858, */ 0x93D0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE859, */ 0x93C8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE85A, */ 0x93E4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE85B, */ 0x941A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE85C, */ 0x9414 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE85D, */ 0x9413 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE85E, */ 0x9403 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE85F, */ 0x9407 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE860, */ 0x9410 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE861, */ 0x9436 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE862, */ 0x942B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE863, */ 0x9435 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE864, */ 0x9421 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE865, */ 0x943A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE866, */ 0x9441 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE867, */ 0x9452 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE868, */ 0x9444 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE869, */ 0x945B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE86A, */ 0x9460 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE86B, */ 0x9462 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE86C, */ 0x945E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE86D, */ 0x946A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE86E, */ 0x9229 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE86F, */ 0x9470 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE870, */ 0x9475 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE871, */ 0x9477 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE872, */ 0x947D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE873, */ 0x945A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE874, */ 0x947C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE875, */ 0x947E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE876, */ 0x9481 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE877, */ 0x947F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE878, */ 0x9582 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE879, */ 0x9587 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE87A, */ 0x958A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE87B, */ 0x9594 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE87C, */ 0x9596 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE87D, */ 0x9598 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE87E, */ 0x9599 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE87F, */ 0x0000 },	// 		　#ダミー
{ /* 0xE880, */ 0x95A0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE881, */ 0x95A8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE882, */ 0x95A7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE883, */ 0x95AD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE884, */ 0x95BC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE885, */ 0x95BB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE886, */ 0x95B9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE887, */ 0x95BE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE888, */ 0x95CA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE889, */ 0x6FF6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE88A, */ 0x95C3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE88B, */ 0x95CD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE88C, */ 0x95CC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE88D, */ 0x95D5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE88E, */ 0x95D4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE88F, */ 0x95D6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE890, */ 0x95DC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE891, */ 0x95E1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE892, */ 0x95E5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE893, */ 0x95E2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE894, */ 0x9621 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE895, */ 0x9628 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE896, */ 0x962E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE897, */ 0x962F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE898, */ 0x9642 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE899, */ 0x964C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE89A, */ 0x964F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE89B, */ 0x964B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE89C, */ 0x9677 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE89D, */ 0x965C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE89E, */ 0x965E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE89F, */ 0x965D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8A0, */ 0x965F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8A1, */ 0x9666 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8A2, */ 0x9672 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8A3, */ 0x966C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8A4, */ 0x968D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8A5, */ 0x9698 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8A6, */ 0x9695 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8A7, */ 0x9697 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8A8, */ 0x96AA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8A9, */ 0x96A7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8AA, */ 0x96B1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8AB, */ 0x96B2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8AC, */ 0x96B0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8AD, */ 0x96B4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8AE, */ 0x96B6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8AF, */ 0x96B8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8B0, */ 0x96B9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8B1, */ 0x96CE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8B2, */ 0x96CB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8B3, */ 0x96C9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8B4, */ 0x96CD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8B5, */ 0x894D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8B6, */ 0x96DC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8B7, */ 0x970D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8B8, */ 0x96D5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8B9, */ 0x96F9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8BA, */ 0x9704 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8BB, */ 0x9706 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8BC, */ 0x9708 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8BD, */ 0x9713 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8BE, */ 0x970E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8BF, */ 0x9711 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8C0, */ 0x970F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8C1, */ 0x9716 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8C2, */ 0x9719 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8C3, */ 0x9724 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8C4, */ 0x972A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8C5, */ 0x9730 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8C6, */ 0x9739 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8C7, */ 0x973D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8C8, */ 0x973E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8C9, */ 0x9744 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8CA, */ 0x9746 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8CB, */ 0x9748 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8CC, */ 0x9742 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8CD, */ 0x9749 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8CE, */ 0x975C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8CF, */ 0x9760 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8D0, */ 0x9764 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8D1, */ 0x9766 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8D2, */ 0x9768 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8D3, */ 0x52D2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8D4, */ 0x976B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8D5, */ 0x9771 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8D6, */ 0x9779 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8D7, */ 0x9785 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8D8, */ 0x977C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8D9, */ 0x9781 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8DA, */ 0x977A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8DB, */ 0x9786 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8DC, */ 0x978B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8DD, */ 0x978F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8DE, */ 0x9790 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8DF, */ 0x979C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8E0, */ 0x97A8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8E1, */ 0x97A6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8E2, */ 0x97A3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8E3, */ 0x97B3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8E4, */ 0x97B4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8E5, */ 0x97C3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8E6, */ 0x97C6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8E7, */ 0x97C8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8E8, */ 0x97CB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8E9, */ 0x97DC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8EA, */ 0x97ED },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8EB, */ 0x9F4F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8EC, */ 0x97F2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8ED, */ 0x7ADF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8EE, */ 0x97F6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8EF, */ 0x97F5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8F0, */ 0x980F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8F1, */ 0x980C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8F2, */ 0x9838 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8F3, */ 0x9824 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8F4, */ 0x9821 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8F5, */ 0x9837 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8F6, */ 0x983D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8F7, */ 0x9846 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8F8, */ 0x984F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8F9, */ 0x984B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8FA, */ 0x986B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8FB, */ 0x986F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE8FC, */ 0x9870 },	// #CJK UNIFIED IDEOGRAPH
//===== 0xE9 40～FC ( テーブル数 [189]  ）====================
{ /* 0xE940, */ 0x9871 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE941, */ 0x9874 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE942, */ 0x9873 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE943, */ 0x98AA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE944, */ 0x98AF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE945, */ 0x98B1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE946, */ 0x98B6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE947, */ 0x98C4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE948, */ 0x98C3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE949, */ 0x98C6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE94A, */ 0x98E9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE94B, */ 0x98EB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE94C, */ 0x9903 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE94D, */ 0x9909 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE94E, */ 0x9912 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE94F, */ 0x9914 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE950, */ 0x9918 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE951, */ 0x9921 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE952, */ 0x991D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE953, */ 0x991E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE954, */ 0x9924 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE955, */ 0x9920 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE956, */ 0x992C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE957, */ 0x992E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE958, */ 0x993D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE959, */ 0x993E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE95A, */ 0x9942 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE95B, */ 0x9949 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE95C, */ 0x9945 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE95D, */ 0x9950 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE95E, */ 0x994B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE95F, */ 0x9951 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE960, */ 0x9952 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE961, */ 0x994C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE962, */ 0x9955 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE963, */ 0x9997 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE964, */ 0x9998 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE965, */ 0x99A5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE966, */ 0x99AD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE967, */ 0x99AE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE968, */ 0x99BC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE969, */ 0x99DF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE96A, */ 0x99DB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE96B, */ 0x99DD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE96C, */ 0x99D8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE96D, */ 0x99D1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE96E, */ 0x99ED },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE96F, */ 0x99EE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE970, */ 0x99F1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE971, */ 0x99F2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE972, */ 0x99FB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE973, */ 0x99F8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE974, */ 0x9A01 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE975, */ 0x9A0F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE976, */ 0x9A05 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE977, */ 0x99E2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE978, */ 0x9A19 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE979, */ 0x9A2B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE97A, */ 0x9A37 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE97B, */ 0x9A45 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE97C, */ 0x9A42 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE97D, */ 0x9A40 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE97E, */ 0x9A43 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE97F, */ 0x0000 },	// 		　#ダミー
{ /* 0xE980, */ 0x9A3E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE981, */ 0x9A55 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE982, */ 0x9A4D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE983, */ 0x9A5B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE984, */ 0x9A57 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE985, */ 0x9A5F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE986, */ 0x9A62 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE987, */ 0x9A65 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE988, */ 0x9A64 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE989, */ 0x9A69 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE98A, */ 0x9A6B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE98B, */ 0x9A6A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE98C, */ 0x9AAD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE98D, */ 0x9AB0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE98E, */ 0x9ABC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE98F, */ 0x9AC0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE990, */ 0x9ACF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE991, */ 0x9AD1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE992, */ 0x9AD3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE993, */ 0x9AD4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE994, */ 0x9ADE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE995, */ 0x9ADF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE996, */ 0x9AE2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE997, */ 0x9AE3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE998, */ 0x9AE6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE999, */ 0x9AEF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE99A, */ 0x9AEB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE99B, */ 0x9AEE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE99C, */ 0x9AF4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE99D, */ 0x9AF1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE99E, */ 0x9AF7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE99F, */ 0x9AFB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9A0, */ 0x9B06 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9A1, */ 0x9B18 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9A2, */ 0x9B1A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9A3, */ 0x9B1F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9A4, */ 0x9B22 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9A5, */ 0x9B23 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9A6, */ 0x9B25 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9A7, */ 0x9B27 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9A8, */ 0x9B28 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9A9, */ 0x9B29 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9AA, */ 0x9B2A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9AB, */ 0x9B2E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9AC, */ 0x9B2F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9AD, */ 0x9B32 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9AE, */ 0x9B44 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9AF, */ 0x9B43 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9B0, */ 0x9B4F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9B1, */ 0x9B4D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9B2, */ 0x9B4E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9B3, */ 0x9B51 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9B4, */ 0x9B58 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9B5, */ 0x9B74 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9B6, */ 0x9B93 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9B7, */ 0x9B83 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9B8, */ 0x9B91 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9B9, */ 0x9B96 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9BA, */ 0x9B97 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9BB, */ 0x9B9F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9BC, */ 0x9BA0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9BD, */ 0x9BA8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9BE, */ 0x9BB4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9BF, */ 0x9BC0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9C0, */ 0x9BCA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9C1, */ 0x9BB9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9C2, */ 0x9BC6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9C3, */ 0x9BCF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9C4, */ 0x9BD1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9C5, */ 0x9BD2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9C6, */ 0x9BE3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9C7, */ 0x9BE2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9C8, */ 0x9BE4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9C9, */ 0x9BD4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9CA, */ 0x9BE1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9CB, */ 0x9C3A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9CC, */ 0x9BF2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9CD, */ 0x9BF1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9CE, */ 0x9BF0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9CF, */ 0x9C15 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9D0, */ 0x9C14 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9D1, */ 0x9C09 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9D2, */ 0x9C13 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9D3, */ 0x9C0C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9D4, */ 0x9C06 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9D5, */ 0x9C08 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9D6, */ 0x9C12 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9D7, */ 0x9C0A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9D8, */ 0x9C04 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9D9, */ 0x9C2E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9DA, */ 0x9C1B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9DB, */ 0x9C25 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9DC, */ 0x9C24 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9DD, */ 0x9C21 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9DE, */ 0x9C30 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9DF, */ 0x9C47 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9E0, */ 0x9C32 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9E1, */ 0x9C46 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9E2, */ 0x9C3E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9E3, */ 0x9C5A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9E4, */ 0x9C60 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9E5, */ 0x9C67 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9E6, */ 0x9C76 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9E7, */ 0x9C78 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9E8, */ 0x9CE7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9E9, */ 0x9CEC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9EA, */ 0x9CF0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9EB, */ 0x9D09 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9EC, */ 0x9D08 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9ED, */ 0x9CEB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9EE, */ 0x9D03 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9EF, */ 0x9D06 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9F0, */ 0x9D2A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9F1, */ 0x9D26 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9F2, */ 0x9DAF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9F3, */ 0x9D23 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9F4, */ 0x9D1F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9F5, */ 0x9D44 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9F6, */ 0x9D15 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9F7, */ 0x9D12 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9F8, */ 0x9D41 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9F9, */ 0x9D3F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9FA, */ 0x9D3E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9FB, */ 0x9D46 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xE9FC, */ 0x9D48 },	// #CJK UNIFIED IDEOGRAPH
//===== 0xE9 40～FC ( テーブル数 [189]  ）====================
{ /* 0xEA40, */ 0x9D5D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA41, */ 0x9D5E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA42, */ 0x9D64 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA43, */ 0x9D51 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA44, */ 0x9D50 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA45, */ 0x9D59 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA46, */ 0x9D72 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA47, */ 0x9D89 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA48, */ 0x9D87 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA49, */ 0x9DAB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA4A, */ 0x9D6F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA4B, */ 0x9D7A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA4C, */ 0x9D9A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA4D, */ 0x9DA4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA4E, */ 0x9DA9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA4F, */ 0x9DB2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA50, */ 0x9DC4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA51, */ 0x9DC1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA52, */ 0x9DBB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA53, */ 0x9DB8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA54, */ 0x9DBA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA55, */ 0x9DC6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA56, */ 0x9DCF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA57, */ 0x9DC2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA58, */ 0x9DD9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA59, */ 0x9DD3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA5A, */ 0x9DF8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA5B, */ 0x9DE6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA5C, */ 0x9DED },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA5D, */ 0x9DEF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA5E, */ 0x9DFD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA5F, */ 0x9E1A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA60, */ 0x9E1B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA61, */ 0x9E1E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA62, */ 0x9E75 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA63, */ 0x9E79 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA64, */ 0x9E7D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA65, */ 0x9E81 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA66, */ 0x9E88 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA67, */ 0x9E8B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA68, */ 0x9E8C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA69, */ 0x9E92 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA6A, */ 0x9E95 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA6B, */ 0x9E91 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA6C, */ 0x9E9D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA6D, */ 0x9EA5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA6E, */ 0x9EA9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA6F, */ 0x9EB8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA70, */ 0x9EAA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA71, */ 0x9EAD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA72, */ 0x9761 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA73, */ 0x9ECC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA74, */ 0x9ECE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA75, */ 0x9ECF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA76, */ 0x9ED0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA77, */ 0x9ED4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA78, */ 0x9EDC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA79, */ 0x9EDE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA7A, */ 0x9EDD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA7B, */ 0x9EE0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA7C, */ 0x9EE5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA7D, */ 0x9EE8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA7E, */ 0x9EEF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA7F, */ 0x0000 },	// 		　#ダミー
{ /* 0xEA80, */ 0x9EF4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA81, */ 0x9EF6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA82, */ 0x9EF7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA83, */ 0x9EF9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA84, */ 0x9EFB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA85, */ 0x9EFC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA86, */ 0x9EFD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA87, */ 0x9F07 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA88, */ 0x9F08 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA89, */ 0x76B7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA8A, */ 0x9F15 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA8B, */ 0x9F21 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA8C, */ 0x9F2C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA8D, */ 0x9F3E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA8E, */ 0x9F4A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA8F, */ 0x9F52 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA90, */ 0x9F54 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA91, */ 0x9F63 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA92, */ 0x9F5F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA93, */ 0x9F60 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA94, */ 0x9F61 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA95, */ 0x9F66 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA96, */ 0x9F67 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA97, */ 0x9F6C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA98, */ 0x9F6A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA99, */ 0x9F77 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA9A, */ 0x9F72 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA9B, */ 0x9F76 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA9C, */ 0x9F95 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA9D, */ 0x9F9C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA9E, */ 0x9FA0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEA9F, */ 0x582F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEAA0, */ 0x69C7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEAA1, */ 0x9059 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEAA2, */ 0x7464 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEAA3, */ 0x51DC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEAA4, */ 0x7199 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEAA5, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAA6, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAA7, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAA8, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAA9, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAAA, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAAB, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAAC, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAAD, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAAE, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAAF, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAB0, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAB1, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAB2, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAB3, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAB4, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAB5, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAB6, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAB7, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAB8, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAB9, */ 0x0000 },	// 		　#ダミー
{ /* 0xEABA, */ 0x0000 },	// 		　#ダミー
{ /* 0xEABB, */ 0x0000 },	// 		　#ダミー
{ /* 0xEABC, */ 0x0000 },	// 		　#ダミー
{ /* 0xEABD, */ 0x0000 },	// 		　#ダミー
{ /* 0xEABE, */ 0x0000 },	// 		　#ダミー
{ /* 0xEABF, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAC0, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAC1, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAC2, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAC3, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAC4, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAC5, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAC6, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAC7, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAC8, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAC9, */ 0x0000 },	// 		　#ダミー
{ /* 0xEACA, */ 0x0000 },	// 		　#ダミー
{ /* 0xEACB, */ 0x0000 },	// 		　#ダミー
{ /* 0xEACC, */ 0x0000 },	// 		　#ダミー
{ /* 0xEACD, */ 0x0000 },	// 		　#ダミー
{ /* 0xEACE, */ 0x0000 },	// 		　#ダミー
{ /* 0xEACF, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAD0, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAD1, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAD2, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAD3, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAD4, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAD5, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAD6, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAD7, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAD8, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAD9, */ 0x0000 },	// 		　#ダミー
{ /* 0xEADA, */ 0x0000 },	// 		　#ダミー
{ /* 0xEADB, */ 0x0000 },	// 		　#ダミー
{ /* 0xEADC, */ 0x0000 },	// 		　#ダミー
{ /* 0xEADD, */ 0x0000 },	// 		　#ダミー
{ /* 0xEADE, */ 0x0000 },	// 		　#ダミー
{ /* 0xEADF, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAE0, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAE1, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAE2, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAE3, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAE4, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAE5, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAE6, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAE7, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAE8, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAE9, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAEA, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAEB, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAEC, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAED, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAEE, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAEF, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAF0, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAF1, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAF2, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAF3, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAF4, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAF5, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAF6, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAF7, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAF8, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAF9, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAFA, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAFB, */ 0x0000 },	// 		　#ダミー
{ /* 0xEAFC, */ 0x0000 },	// 		　#ダミー

};

//===== 0xED～0xEE====================
OBJ_CDTBL CodeTable3[] =
{
//===== 0xED 40～FC ( テーブル数 [189]  ）====================
{ /* 0xED40, */ 0x7E8A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED41, */ 0x891C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED42, */ 0x9348 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED43, */ 0x9288 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED44, */ 0x84DC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED45, */ 0x4FC9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED46, */ 0x70BB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED47, */ 0x6631 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED48, */ 0x68C8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED49, */ 0x92F9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED4A, */ 0x66FB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED4B, */ 0x5F45 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED4C, */ 0x4E28 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED4D, */ 0x4EE1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED4E, */ 0x4EFC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED4F, */ 0x4F00 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED50, */ 0x4F03 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED51, */ 0x4F39 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED52, */ 0x4F56 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED53, */ 0x4F92 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED54, */ 0x4F8A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED55, */ 0x4F9A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED56, */ 0x4F94 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED57, */ 0x4FCD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED58, */ 0x5040 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED59, */ 0x5022 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED5A, */ 0x4FFF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED5B, */ 0x501E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED5C, */ 0x5046 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED5D, */ 0x5070 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED5E, */ 0x5042 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED5F, */ 0x5094 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED60, */ 0x50F4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED61, */ 0x50D8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED62, */ 0x514A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED63, */ 0x5164 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED64, */ 0x519D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED65, */ 0x51BE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED66, */ 0x51EC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED67, */ 0x5215 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED68, */ 0x529C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED69, */ 0x52A6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED6A, */ 0x52C0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED6B, */ 0x52DB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED6C, */ 0x5300 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED6D, */ 0x5307 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED6E, */ 0x5324 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED6F, */ 0x5372 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED70, */ 0x5393 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED71, */ 0x53B2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED72, */ 0x53DD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED73, */ 0xFA0E },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xED74, */ 0x549C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED75, */ 0x548A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED76, */ 0x54A9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED77, */ 0x54FF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED78, */ 0x5586 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED79, */ 0x5759 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED7A, */ 0x5765 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED7B, */ 0x57AC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED7C, */ 0x57C8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED7D, */ 0x57C7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED7E, */ 0xFA0F },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xED7F, */ 0x0000 },	// 		　#ダミー
{ /* 0xED80, */ 0xFA10 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xED81, */ 0x589E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED82, */ 0x58B2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED83, */ 0x590B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED84, */ 0x5953 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED85, */ 0x595B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED86, */ 0x595D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED87, */ 0x5963 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED88, */ 0x59A4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED89, */ 0x59BA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED8A, */ 0x5B56 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED8B, */ 0x5BC0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED8C, */ 0x752F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED8D, */ 0x5BD8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED8E, */ 0x5BEC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED8F, */ 0x5C1E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED90, */ 0x5CA6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED91, */ 0x5CBA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED92, */ 0x5CF5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED93, */ 0x5D27 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED94, */ 0x5D53 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED95, */ 0xFA11 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xED96, */ 0x5D42 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED97, */ 0x5D6D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED98, */ 0x5DB8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED99, */ 0x5DB9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED9A, */ 0x5DD0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED9B, */ 0x5F21 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED9C, */ 0x5F34 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED9D, */ 0x5F67 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED9E, */ 0x5FB7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xED9F, */ 0x5FDE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDA0, */ 0x605D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDA1, */ 0x6085 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDA2, */ 0x608A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDA3, */ 0x60DE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDA4, */ 0x60D5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDA5, */ 0x6120 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDA6, */ 0x60F2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDA7, */ 0x6111 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDA8, */ 0x6137 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDA9, */ 0x6130 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDAA, */ 0x6198 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDAB, */ 0x6213 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDAC, */ 0x62A6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDAD, */ 0x63F5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDAE, */ 0x6460 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDAF, */ 0x649D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDB0, */ 0x64CE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDB1, */ 0x654E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDB2, */ 0x6600 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDB3, */ 0x6615 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDB4, */ 0x663B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDB5, */ 0x6609 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDB6, */ 0x662E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDB7, */ 0x661E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDB8, */ 0x6624 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDB9, */ 0x6665 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDBA, */ 0x6657 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDBB, */ 0x6659 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDBC, */ 0xFA12 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xEDBD, */ 0x6673 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDBE, */ 0x6699 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDBF, */ 0x66A0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDC0, */ 0x66B2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDC1, */ 0x66BF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDC2, */ 0x66FA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDC3, */ 0x670E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDC4, */ 0xF929 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xEDC5, */ 0x6766 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDC6, */ 0x67BB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDC7, */ 0x6852 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDC8, */ 0x67C0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDC9, */ 0x6801 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDCA, */ 0x6844 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDCB, */ 0x68CF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDCC, */ 0xFA13 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xEDCD, */ 0x6968 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDCE, */ 0xFA14 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xEDCF, */ 0x6998 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDD0, */ 0x69E2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDD1, */ 0x6A30 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDD2, */ 0x6A6B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDD3, */ 0x6A46 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDD4, */ 0x6A73 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDD5, */ 0x6A7E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDD6, */ 0x6AE2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDD7, */ 0x6AE4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDD8, */ 0x6BD6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDD9, */ 0x6C3F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDDA, */ 0x6C5C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDDB, */ 0x6C86 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDDC, */ 0x6C6F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDDD, */ 0x6CDA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDDE, */ 0x6D04 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDDF, */ 0x6D87 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDE0, */ 0x6D6F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDE1, */ 0x6D96 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDE2, */ 0x6DAC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDE3, */ 0x6DCF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDE4, */ 0x6DF8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDE5, */ 0x6DF2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDE6, */ 0x6DFC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDE7, */ 0x6E39 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDE8, */ 0x6E5C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDE9, */ 0x6E27 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDEA, */ 0x6E3C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDEB, */ 0x6EBF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDEC, */ 0x6F88 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDED, */ 0x6FB5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDEE, */ 0x6FF5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDEF, */ 0x7005 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDF0, */ 0x7007 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDF1, */ 0x7028 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDF2, */ 0x7085 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDF3, */ 0x70AB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDF4, */ 0x710F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDF5, */ 0x7104 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDF6, */ 0x715C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDF7, */ 0x7146 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDF8, */ 0x7147 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDF9, */ 0xFA15 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xEDFA, */ 0x71C1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDFB, */ 0x71FE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEDFC, */ 0x72B1 },	// #CJK UNIFIED IDEOGRAPH
//===== 0xEE 40～FC ( テーブル数 [189]  ）====================
{ /* 0xEE40, */ 0x72BE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE41, */ 0x7324 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE42, */ 0xFA16 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xEE43, */ 0x7377 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE44, */ 0x73BD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE45, */ 0x73C9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE46, */ 0x73D6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE47, */ 0x73E3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE48, */ 0x73D2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE49, */ 0x7407 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE4A, */ 0x73F5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE4B, */ 0x7426 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE4C, */ 0x742A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE4D, */ 0x7429 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE4E, */ 0x742E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE4F, */ 0x7462 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE50, */ 0x7489 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE51, */ 0x749F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE52, */ 0x7501 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE53, */ 0x756F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE54, */ 0x7682 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE55, */ 0x769C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE56, */ 0x769E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE57, */ 0x769B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE58, */ 0x76A6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE59, */ 0xFA17 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xEE5A, */ 0x7746 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE5B, */ 0x52AF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE5C, */ 0x7821 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE5D, */ 0x784E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE5E, */ 0x7864 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE5F, */ 0x787A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE60, */ 0x7930 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE61, */ 0xFA18 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xEE62, */ 0xFA19 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xEE63, */ 0xFA1A },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xEE64, */ 0x7994 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE65, */ 0xFA1B },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xEE66, */ 0x799B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE67, */ 0x7AD1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE68, */ 0x7AE7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE69, */ 0xFA1C },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xEE6A, */ 0x7AEB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE6B, */ 0x7B9E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE6C, */ 0xFA1D },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xEE6D, */ 0x7D48 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE6E, */ 0x7D5C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE6F, */ 0x7DB7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE70, */ 0x7DA0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE71, */ 0x7DD6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE72, */ 0x7E52 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE73, */ 0x7F47 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE74, */ 0x7FA1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE75, */ 0xFA1E },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xEE76, */ 0x8301 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE77, */ 0x8362 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE78, */ 0x837F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE79, */ 0x83C7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE7A, */ 0x83F6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE7B, */ 0x8448 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE7C, */ 0x84B4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE7D, */ 0x8553 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE7E, */ 0x8559 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE7F, */ 0x0000 },	// 		　#ダミー
{ /* 0xEE80, */ 0x856B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE81, */ 0xFA1F },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xEE82, */ 0x85B0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE83, */ 0xFA20 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xEE84, */ 0xFA21 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xEE85, */ 0x8807 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE86, */ 0x88F5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE87, */ 0x8A12 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE88, */ 0x8A37 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE89, */ 0x8A79 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE8A, */ 0x8AA7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE8B, */ 0x8ABE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE8C, */ 0x8ADF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE8D, */ 0xFA22 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xEE8E, */ 0x8AF6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE8F, */ 0x8B53 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE90, */ 0x8B7F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE91, */ 0x8CF0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE92, */ 0x8CF4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE93, */ 0x8D12 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE94, */ 0x8D76 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE95, */ 0xFA23 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xEE96, */ 0x8ECF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE97, */ 0xFA24 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xEE98, */ 0xFA25 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xEE99, */ 0x9067 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE9A, */ 0x90DE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE9B, */ 0xFA26 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xEE9C, */ 0x9115 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE9D, */ 0x9127 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE9E, */ 0x91DA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEE9F, */ 0x91D7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEA0, */ 0x91DE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEA1, */ 0x91ED },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEA2, */ 0x91EE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEA3, */ 0x91E4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEA4, */ 0x91E5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEA5, */ 0x9206 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEA6, */ 0x9210 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEA7, */ 0x920A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEA8, */ 0x923A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEA9, */ 0x9240 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEAA, */ 0x923C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEAB, */ 0x924E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEAC, */ 0x9259 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEAD, */ 0x9251 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEAE, */ 0x9239 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEAF, */ 0x9267 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEB0, */ 0x92A7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEB1, */ 0x9277 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEB2, */ 0x9278 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEB3, */ 0x92E7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEB4, */ 0x92D7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEB5, */ 0x92D9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEB6, */ 0x92D0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEB7, */ 0xFA27 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xEEB8, */ 0x92D5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEB9, */ 0x92E0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEBA, */ 0x92D3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEBB, */ 0x9325 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEBC, */ 0x9321 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEBD, */ 0x92FB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEBE, */ 0xFA28 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xEEBF, */ 0x931E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEC0, */ 0x92FF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEC1, */ 0x931D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEC2, */ 0x9302 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEC3, */ 0x9370 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEC4, */ 0x9357 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEC5, */ 0x93A4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEC6, */ 0x93C6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEC7, */ 0x93DE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEC8, */ 0x93F8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEC9, */ 0x9431 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEECA, */ 0x9445 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEECB, */ 0x9448 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEECC, */ 0x9592 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEECD, */ 0xF9DC },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xEECE, */ 0xFA29 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xEECF, */ 0x969D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEED0, */ 0x96AF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEED1, */ 0x9733 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEED2, */ 0x973B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEED3, */ 0x9743 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEED4, */ 0x974D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEED5, */ 0x974F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEED6, */ 0x9751 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEED7, */ 0x9755 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEED8, */ 0x9857 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEED9, */ 0x9865 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEDA, */ 0xFA2A },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xEEDB, */ 0xFA2B },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xEEDC, */ 0x9927 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEDD, */ 0xFA2C },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xEEDE, */ 0x999E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEDF, */ 0x9A4E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEE0, */ 0x9AD9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEE1, */ 0x9ADC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEE2, */ 0x9B75 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEE3, */ 0x9B72 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEE4, */ 0x9B8F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEE5, */ 0x9BB1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEE6, */ 0x9BBB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEE7, */ 0x9C00 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEE8, */ 0x9D70 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEE9, */ 0x9D6B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEEA, */ 0xFA2D },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xEEEB, */ 0x9E19 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEEC, */ 0x9ED1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xEEED, */ 0x0000 },	// #ダミー
{ /* 0xEEEE, */ 0x0000 },	// #ダミー
{ /* 0xEEEF, */ 0x2170 },	// #SMALL ROMAN NUMERAL ONE
{ /* 0xEEF0, */ 0x2171 },	// #SMALL ROMAN NUMERAL TWO
{ /* 0xEEF1, */ 0x2172 },	// #SMALL ROMAN NUMERAL THREE
{ /* 0xEEF2, */ 0x2173 },	// #SMALL ROMAN NUMERAL FOUR
{ /* 0xEEF3, */ 0x2174 },	// #SMALL ROMAN NUMERAL FIVE
{ /* 0xEEF4, */ 0x2175 },	// #SMALL ROMAN NUMERAL SIX
{ /* 0xEEF5, */ 0x2176 },	// #SMALL ROMAN NUMERAL SEVEN
{ /* 0xEEF6, */ 0x2177 },	// #SMALL ROMAN NUMERAL EIGHT
{ /* 0xEEF7, */ 0x2178 },	// #SMALL ROMAN NUMERAL NINE
{ /* 0xEEF8, */ 0x2179 },	// #SMALL ROMAN NUMERAL TEN
{ /* 0xEEF9, */ 0xFFE2 },	// #FULLWIDTH NOT SIGN
{ /* 0xEEFA, */ 0xFFE4 },	// #FULLWIDTH BROKEN BAR
{ /* 0xEEFB, */ 0xFF07 },	// #FULLWIDTH APOSTROPHE
{ /* 0xEEFC, */ 0xFF02 },	// #FULLWIDTH QUOTATION MARK

};

//===== 0xFA～0xFC====================
OBJ_CDTBL CodeTable4[] =
{
//===== 0xFA 40～FC ( テーブル数 [189]  ）====================
{ /* 0xFA40, */ 0x2170 },	// #SMALL ROMAN NUMERAL ONE
{ /* 0xFA41, */ 0x2171 },	// #SMALL ROMAN NUMERAL TWO
{ /* 0xFA42, */ 0x2172 },	// #SMALL ROMAN NUMERAL THREE
{ /* 0xFA43, */ 0x2173 },	// #SMALL ROMAN NUMERAL FOUR
{ /* 0xFA44, */ 0x2174 },	// #SMALL ROMAN NUMERAL FIVE
{ /* 0xFA45, */ 0x2175 },	// #SMALL ROMAN NUMERAL SIX
{ /* 0xFA46, */ 0x2176 },	// #SMALL ROMAN NUMERAL SEVEN
{ /* 0xFA47, */ 0x2177 },	// #SMALL ROMAN NUMERAL EIGHT
{ /* 0xFA48, */ 0x2178 },	// #SMALL ROMAN NUMERAL NINE
{ /* 0xFA49, */ 0x2179 },	// #SMALL ROMAN NUMERAL TEN
{ /* 0xFA4A, */ 0x2160 },	// #ROMAN NUMERAL ONE
{ /* 0xFA4B, */ 0x2161 },	// #ROMAN NUMERAL TWO
{ /* 0xFA4C, */ 0x2162 },	// #ROMAN NUMERAL THREE
{ /* 0xFA4D, */ 0x2163 },	// #ROMAN NUMERAL FOUR
{ /* 0xFA4E, */ 0x2164 },	// #ROMAN NUMERAL FIVE
{ /* 0xFA4F, */ 0x2165 },	// #ROMAN NUMERAL SIX
{ /* 0xFA50, */ 0x2166 },	// #ROMAN NUMERAL SEVEN
{ /* 0xFA51, */ 0x2167 },	// #ROMAN NUMERAL EIGHT
{ /* 0xFA52, */ 0x2168 },	// #ROMAN NUMERAL NINE
{ /* 0xFA53, */ 0x2169 },	// #ROMAN NUMERAL TEN
{ /* 0xFA54, */ 0xFFE2 },	// #FULLWIDTH NOT SIGN
{ /* 0xFA55, */ 0xFFE4 },	// #FULLWIDTH BROKEN BAR
{ /* 0xFA56, */ 0xFF07 },	// #FULLWIDTH APOSTROPHE
{ /* 0xFA57, */ 0xFF02 },	// #FULLWIDTH QUOTATION MARK
{ /* 0xFA58, */ 0x3231 },	// #PARENTHESIZED IDEOGRAPH STOCK
{ /* 0xFA59, */ 0x2116 },	// #NUMERO SIGN
{ /* 0xFA5A, */ 0x2121 },	// #TELEPHONE SIGN
{ /* 0xFA5B, */ 0x2235 },	// #BECAUSE
{ /* 0xFA5C, */ 0x7E8A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA5D, */ 0x891C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA5E, */ 0x9348 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA5F, */ 0x9288 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA60, */ 0x84DC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA61, */ 0x4FC9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA62, */ 0x70BB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA63, */ 0x6631 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA64, */ 0x68C8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA65, */ 0x92F9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA66, */ 0x66FB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA67, */ 0x5F45 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA68, */ 0x4E28 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA69, */ 0x4EE1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA6A, */ 0x4EFC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA6B, */ 0x4F00 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA6C, */ 0x4F03 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA6D, */ 0x4F39 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA6E, */ 0x4F56 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA6F, */ 0x4F92 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA70, */ 0x4F8A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA71, */ 0x4F9A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA72, */ 0x4F94 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA73, */ 0x4FCD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA74, */ 0x5040 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA75, */ 0x5022 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA76, */ 0x4FFF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA77, */ 0x501E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA78, */ 0x5046 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA79, */ 0x5070 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA7A, */ 0x5042 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA7B, */ 0x5094 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA7C, */ 0x50F4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA7D, */ 0x50D8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA7E, */ 0x514A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA7F, */ 0x0000 },	// #ダミー
{ /* 0xFA80, */ 0x5164 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA81, */ 0x519D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA82, */ 0x51BE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA83, */ 0x51EC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA84, */ 0x5215 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA85, */ 0x529C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA86, */ 0x52A6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA87, */ 0x52C0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA88, */ 0x52DB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA89, */ 0x5300 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA8A, */ 0x5307 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA8B, */ 0x5324 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA8C, */ 0x5372 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA8D, */ 0x5393 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA8E, */ 0x53B2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA8F, */ 0x53DD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA90, */ 0xFA0E },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFA91, */ 0x549C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA92, */ 0x548A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA93, */ 0x54A9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA94, */ 0x54FF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA95, */ 0x5586 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA96, */ 0x5759 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA97, */ 0x5765 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA98, */ 0x57AC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA99, */ 0x57C8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA9A, */ 0x57C7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA9B, */ 0xFA0F },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFA9C, */ 0xFA10 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFA9D, */ 0x589E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA9E, */ 0x58B2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFA9F, */ 0x590B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAA0, */ 0x5953 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAA1, */ 0x595B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAA2, */ 0x595D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAA3, */ 0x5963 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAA4, */ 0x59A4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAA5, */ 0x59BA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAA6, */ 0x5B56 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAA7, */ 0x5BC0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAA8, */ 0x752F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAA9, */ 0x5BD8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAAA, */ 0x5BEC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAAB, */ 0x5C1E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAAC, */ 0x5CA6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAAD, */ 0x5CBA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAAE, */ 0x5CF5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAAF, */ 0x5D27 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAB0, */ 0x5D53 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAB1, */ 0xFA11 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFAB2, */ 0x5D42 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAB3, */ 0x5D6D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAB4, */ 0x5DB8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAB5, */ 0x5DB9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAB6, */ 0x5DD0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAB7, */ 0x5F21 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAB8, */ 0x5F34 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAB9, */ 0x5F67 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFABA, */ 0x5FB7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFABB, */ 0x5FDE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFABC, */ 0x605D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFABD, */ 0x6085 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFABE, */ 0x608A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFABF, */ 0x60DE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAC0, */ 0x60D5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAC1, */ 0x6120 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAC2, */ 0x60F2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAC3, */ 0x6111 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAC4, */ 0x6137 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAC5, */ 0x6130 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAC6, */ 0x6198 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAC7, */ 0x6213 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAC8, */ 0x62A6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAC9, */ 0x63F5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFACA, */ 0x6460 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFACB, */ 0x649D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFACC, */ 0x64CE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFACD, */ 0x654E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFACE, */ 0x6600 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFACF, */ 0x6615 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAD0, */ 0x663B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAD1, */ 0x6609 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAD2, */ 0x662E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAD3, */ 0x661E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAD4, */ 0x6624 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAD5, */ 0x6665 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAD6, */ 0x6657 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAD7, */ 0x6659 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAD8, */ 0xFA12 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFAD9, */ 0x6673 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFADA, */ 0x6699 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFADB, */ 0x66A0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFADC, */ 0x66B2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFADD, */ 0x66BF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFADE, */ 0x66FA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFADF, */ 0x670E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAE0, */ 0xF929 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFAE1, */ 0x6766 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAE2, */ 0x67BB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAE3, */ 0x6852 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAE4, */ 0x67C0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAE5, */ 0x6801 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAE6, */ 0x6844 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAE7, */ 0x68CF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAE8, */ 0xFA13 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFAE9, */ 0x6968 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAEA, */ 0xFA14 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFAEB, */ 0x6998 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAEC, */ 0x69E2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAED, */ 0x6A30 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAEE, */ 0x6A6B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAEF, */ 0x6A46 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAF0, */ 0x6A73 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAF1, */ 0x6A7E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAF2, */ 0x6AE2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAF3, */ 0x6AE4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAF4, */ 0x6BD6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAF5, */ 0x6C3F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAF6, */ 0x6C5C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAF7, */ 0x6C86 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAF8, */ 0x6C6F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAF9, */ 0x6CDA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAFA, */ 0x6D04 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAFB, */ 0x6D87 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFAFC, */ 0x6D6F },	// #CJK UNIFIED IDEOGRAPH
//===== 0xFB 40～FC ( テーブル数 [189]  ）====================
{ /* 0xFB40, */ 0x6D96 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB41, */ 0x6DAC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB42, */ 0x6DCF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB43, */ 0x6DF8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB44, */ 0x6DF2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB45, */ 0x6DFC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB46, */ 0x6E39 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB47, */ 0x6E5C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB48, */ 0x6E27 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB49, */ 0x6E3C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB4A, */ 0x6EBF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB4B, */ 0x6F88 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB4C, */ 0x6FB5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB4D, */ 0x6FF5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB4E, */ 0x7005 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB4F, */ 0x7007 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB50, */ 0x7028 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB51, */ 0x7085 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB52, */ 0x70AB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB53, */ 0x710F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB54, */ 0x7104 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB55, */ 0x715C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB56, */ 0x7146 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB57, */ 0x7147 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB58, */ 0xFA15 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFB59, */ 0x71C1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB5A, */ 0x71FE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB5B, */ 0x72B1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB5C, */ 0x72BE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB5D, */ 0x7324 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB5E, */ 0xFA16 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFB5F, */ 0x7377 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB60, */ 0x73BD },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB61, */ 0x73C9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB62, */ 0x73D6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB63, */ 0x73E3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB64, */ 0x73D2 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB65, */ 0x7407 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB66, */ 0x73F5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB67, */ 0x7426 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB68, */ 0x742A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB69, */ 0x7429 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB6A, */ 0x742E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB6B, */ 0x7462 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB6C, */ 0x7489 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB6D, */ 0x749F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB6E, */ 0x7501 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB6F, */ 0x756F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB70, */ 0x7682 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB71, */ 0x769C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB72, */ 0x769E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB73, */ 0x769B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB74, */ 0x76A6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB75, */ 0xFA17 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFB76, */ 0x7746 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB77, */ 0x52AF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB78, */ 0x7821 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB79, */ 0x784E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB7A, */ 0x7864 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB7B, */ 0x787A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB7C, */ 0x7930 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB7D, */ 0xFA18 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFB7E, */ 0xFA19 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFB7F, */ 0x0000 },	// #ダミー
{ /* 0xFB80, */ 0xFA1A },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFB81, */ 0x7994 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB82, */ 0xFA1B },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFB83, */ 0x799B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB84, */ 0x7AD1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB85, */ 0x7AE7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB86, */ 0xFA1C },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFB87, */ 0x7AEB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB88, */ 0x7B9E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB89, */ 0xFA1D },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFB8A, */ 0x7D48 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB8B, */ 0x7D5C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB8C, */ 0x7DB7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB8D, */ 0x7DA0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB8E, */ 0x7DD6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB8F, */ 0x7E52 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB90, */ 0x7F47 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB91, */ 0x7FA1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB92, */ 0xFA1E },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFB93, */ 0x8301 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB94, */ 0x8362 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB95, */ 0x837F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB96, */ 0x83C7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB97, */ 0x83F6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB98, */ 0x8448 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB99, */ 0x84B4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB9A, */ 0x8553 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB9B, */ 0x8559 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB9C, */ 0x856B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB9D, */ 0xFA1F },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFB9E, */ 0x85B0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFB9F, */ 0xFA20 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFBA0, */ 0xFA21 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFBA1, */ 0x8807 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBA2, */ 0x88F5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBA3, */ 0x8A12 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBA4, */ 0x8A37 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBA5, */ 0x8A79 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBA6, */ 0x8AA7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBA7, */ 0x8ABE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBA8, */ 0x8ADF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBA9, */ 0xFA22 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFBAA, */ 0x8AF6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBAB, */ 0x8B53 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBAC, */ 0x8B7F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBAD, */ 0x8CF0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBAE, */ 0x8CF4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBAF, */ 0x8D12 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBB0, */ 0x8D76 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBB1, */ 0xFA23 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFBB2, */ 0x8ECF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBB3, */ 0xFA24 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFBB4, */ 0xFA25 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFBB5, */ 0x9067 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBB6, */ 0x90DE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBB7, */ 0xFA26 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFBB8, */ 0x9115 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBB9, */ 0x9127 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBBA, */ 0x91DA },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBBB, */ 0x91D7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBBC, */ 0x91DE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBBD, */ 0x91ED },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBBE, */ 0x91EE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBBF, */ 0x91E4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBC0, */ 0x91E5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBC1, */ 0x9206 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBC2, */ 0x9210 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBC3, */ 0x920A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBC4, */ 0x923A },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBC5, */ 0x9240 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBC6, */ 0x923C },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBC7, */ 0x924E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBC8, */ 0x9259 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBC9, */ 0x9251 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBCA, */ 0x9239 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBCB, */ 0x9267 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBCC, */ 0x92A7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBCD, */ 0x9277 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBCE, */ 0x9278 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBCF, */ 0x92E7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBD0, */ 0x92D7 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBD1, */ 0x92D9 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBD2, */ 0x92D0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBD3, */ 0xFA27 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFBD4, */ 0x92D5 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBD5, */ 0x92E0 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBD6, */ 0x92D3 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBD7, */ 0x9325 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBD8, */ 0x9321 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBD9, */ 0x92FB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBDA, */ 0xFA28 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFBDB, */ 0x931E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBDC, */ 0x92FF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBDD, */ 0x931D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBDE, */ 0x9302 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBDF, */ 0x9370 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBE0, */ 0x9357 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBE1, */ 0x93A4 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBE2, */ 0x93C6 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBE3, */ 0x93DE },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBE4, */ 0x93F8 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBE5, */ 0x9431 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBE6, */ 0x9445 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBE7, */ 0x9448 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBE8, */ 0x9592 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBE9, */ 0xF9DC },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFBEA, */ 0xFA29 },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFBEB, */ 0x969D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBEC, */ 0x96AF },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBED, */ 0x9733 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBEE, */ 0x973B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBEF, */ 0x9743 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBF0, */ 0x974D },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBF1, */ 0x974F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBF2, */ 0x9751 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBF3, */ 0x9755 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBF4, */ 0x9857 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBF5, */ 0x9865 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBF6, */ 0xFA2A },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFBF7, */ 0xFA2B },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFBF8, */ 0x9927 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBF9, */ 0xFA2C },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFBFA, */ 0x999E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBFB, */ 0x9A4E },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFBFC, */ 0x9AD9 },	// #CJK UNIFIED IDEOGRAPH
//===== 0xFC 40～FC ( テーブル数 []  ）====================
{ /* 0xFC40, */ 0x9ADC },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFC41, */ 0x9B75 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFC42, */ 0x9B72 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFC43, */ 0x9B8F },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFC44, */ 0x9BB1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFC45, */ 0x9BBB },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFC46, */ 0x9C00 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFC47, */ 0x9D70 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFC48, */ 0x9D6B },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFC49, */ 0xFA2D },	// #CJK COMPATIBILITY IDEOGRAPH
{ /* 0xFC4A, */ 0x9E19 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFC4B, */ 0x9ED1 },	// #CJK UNIFIED IDEOGRAPH
{ /* 0xFC4C, */ 0x0000 },	// #ダミー
{ /* 0xFC4D, */ 0x0000 },	// #ダミー
{ /* 0xFC4E, */ 0x0000 },	// #ダミー
{ /* 0xFC4F, */ 0x0000 },	// #ダミー
{ /* 0xFC50, */ 0x0000 },	// #ダミー
{ /* 0xFC51, */ 0x0000 },	// #ダミー
{ /* 0xFC52, */ 0x0000 },	// #ダミー
{ /* 0xFC53, */ 0x0000 },	// #ダミー
{ /* 0xFC54, */ 0x0000 },	// #ダミー
{ /* 0xFC55, */ 0x0000 },	// #ダミー
{ /* 0xFC56, */ 0x0000 },	// #ダミー
{ /* 0xFC57, */ 0x0000 },	// #ダミー
{ /* 0xFC58, */ 0x0000 },	// #ダミー
{ /* 0xFC59, */ 0x0000 },	// #ダミー
{ /* 0xFC5A, */ 0x0000 },	// #ダミー
{ /* 0xFC5B, */ 0x0000 },	// #ダミー
{ /* 0xFC5C, */ 0x0000 },	// #ダミー
{ /* 0xFC5D, */ 0x0000 },	// #ダミー
{ /* 0xFC5E, */ 0x0000 },	// #ダミー
{ /* 0xFC5F, */ 0x0000 },	// #ダミー
{ /* 0xFC60, */ 0x0000 },	// #ダミー
{ /* 0xFC61, */ 0x0000 },	// #ダミー
{ /* 0xFC62, */ 0x0000 },	// #ダミー
{ /* 0xFC63, */ 0x0000 },	// #ダミー
{ /* 0xFC64, */ 0x0000 },	// #ダミー
{ /* 0xFC65, */ 0x0000 },	// #ダミー
{ /* 0xFC66, */ 0x0000 },	// #ダミー
{ /* 0xFC67, */ 0x0000 },	// #ダミー
{ /* 0xFC68, */ 0x0000 },	// #ダミー
{ /* 0xFC69, */ 0x0000 },	// #ダミー
{ /* 0xFC6A, */ 0x0000 },	// #ダミー
{ /* 0xFC6B, */ 0x0000 },	// #ダミー
{ /* 0xFC6C, */ 0x0000 },	// #ダミー
{ /* 0xFC6D, */ 0x0000 },	// #ダミー
{ /* 0xFC6E, */ 0x0000 },	// #ダミー
{ /* 0xFC6F, */ 0x0000 },	// #ダミー
{ /* 0xFC70, */ 0x0000 },	// #ダミー
{ /* 0xFC71, */ 0x0000 },	// #ダミー
{ /* 0xFC72, */ 0x0000 },	// #ダミー
{ /* 0xFC73, */ 0x0000 },	// #ダミー
{ /* 0xFC74, */ 0x0000 },	// #ダミー
{ /* 0xFC75, */ 0x0000 },	// #ダミー
{ /* 0xFC76, */ 0x0000 },	// #ダミー
{ /* 0xFC77, */ 0x0000 },	// #ダミー
{ /* 0xFC78, */ 0x0000 },	// #ダミー
{ /* 0xFC79, */ 0x0000 },	// #ダミー
{ /* 0xFC7A, */ 0x0000 },	// #ダミー
{ /* 0xFC7B, */ 0x0000 },	// #ダミー
{ /* 0xFC7C, */ 0x0000 },	// #ダミー
{ /* 0xFC7D, */ 0x0000 },	// #ダミー
{ /* 0xFC7E, */ 0x0000 },	// #ダミー
{ /* 0xFC7F, */ 0x0000 },	// #ダミー
{ /* 0xFC80, */ 0x0000 },	// #ダミー
{ /* 0xFC81, */ 0x0000 },	// #ダミー
{ /* 0xFC82, */ 0x0000 },	// #ダミー
{ /* 0xFC83, */ 0x0000 },	// #ダミー
{ /* 0xFC84, */ 0x0000 },	// #ダミー
{ /* 0xFC85, */ 0x0000 },	// #ダミー
{ /* 0xFC86, */ 0x0000 },	// #ダミー
{ /* 0xFC87, */ 0x0000 },	// #ダミー
{ /* 0xFC88, */ 0x0000 },	// #ダミー
{ /* 0xFC89, */ 0x0000 },	// #ダミー
{ /* 0xFC8A, */ 0x0000 },	// #ダミー
{ /* 0xFC8B, */ 0x0000 },	// #ダミー
{ /* 0xFC8C, */ 0x0000 },	// #ダミー
{ /* 0xFC8D, */ 0x0000 },	// #ダミー
{ /* 0xFC8E, */ 0x0000 },	// #ダミー
{ /* 0xFC8F, */ 0x0000 },	// #ダミー
{ /* 0xFC90, */ 0x0000 },	// #ダミー
{ /* 0xFC91, */ 0x0000 },	// #ダミー
{ /* 0xFC92, */ 0x0000 },	// #ダミー
{ /* 0xFC93, */ 0x0000 },	// #ダミー
{ /* 0xFC94, */ 0x0000 },	// #ダミー
{ /* 0xFC95, */ 0x0000 },	// #ダミー
{ /* 0xFC96, */ 0x0000 },	// #ダミー
{ /* 0xFC97, */ 0x0000 },	// #ダミー
{ /* 0xFC98, */ 0x0000 },	// #ダミー
{ /* 0xFC99, */ 0x0000 },	// #ダミー
{ /* 0xFC9A, */ 0x0000 },	// #ダミー
{ /* 0xFC9B, */ 0x0000 },	// #ダミー
{ /* 0xFC9C, */ 0x0000 },	// #ダミー
{ /* 0xFC9D, */ 0x0000 },	// #ダミー
{ /* 0xFC9E, */ 0x0000 },	// #ダミー
{ /* 0xFC9F, */ 0x0000 },	// #ダミー
{ /* 0xFCA0, */ 0x0000 },	// #ダミー
{ /* 0xFCA1, */ 0x0000 },	// #ダミー
{ /* 0xFCA2, */ 0x0000 },	// #ダミー
{ /* 0xFCA3, */ 0x0000 },	// #ダミー
{ /* 0xFCA4, */ 0x0000 },	// #ダミー
{ /* 0xFCA5, */ 0x0000 },	// #ダミー
{ /* 0xFCA6, */ 0x0000 },	// #ダミー
{ /* 0xFCA7, */ 0x0000 },	// #ダミー
{ /* 0xFCA8, */ 0x0000 },	// #ダミー
{ /* 0xFCA9, */ 0x0000 },	// #ダミー
{ /* 0xFCAA, */ 0x0000 },	// #ダミー
{ /* 0xFCAB, */ 0x0000 },	// #ダミー
{ /* 0xFCAC, */ 0x0000 },	// #ダミー
{ /* 0xFCAD, */ 0x0000 },	// #ダミー
{ /* 0xFCAE, */ 0x0000 },	// #ダミー
{ /* 0xFCAF, */ 0x0000 },	// #ダミー
{ /* 0xFCB0, */ 0x0000 },	// #ダミー
{ /* 0xFCB1, */ 0x0000 },	// #ダミー
{ /* 0xFCB2, */ 0x0000 },	// #ダミー
{ /* 0xFCB3, */ 0x0000 },	// #ダミー
{ /* 0xFCB4, */ 0x0000 },	// #ダミー
{ /* 0xFCB5, */ 0x0000 },	// #ダミー
{ /* 0xFCB6, */ 0x0000 },	// #ダミー
{ /* 0xFCB7, */ 0x0000 },	// #ダミー
{ /* 0xFCB8, */ 0x0000 },	// #ダミー
{ /* 0xFCB9, */ 0x0000 },	// #ダミー
{ /* 0xFCBA, */ 0x0000 },	// #ダミー
{ /* 0xFCBB, */ 0x0000 },	// #ダミー
{ /* 0xFCBC, */ 0x0000 },	// #ダミー
{ /* 0xFCBD, */ 0x0000 },	// #ダミー
{ /* 0xFCBE, */ 0x0000 },	// #ダミー
{ /* 0xFCBF, */ 0x0000 },	// #ダミー
{ /* 0xFCC0, */ 0x0000 },	// #ダミー
{ /* 0xFCC1, */ 0x0000 },	// #ダミー
{ /* 0xFCC2, */ 0x0000 },	// #ダミー
{ /* 0xFCC3, */ 0x0000 },	// #ダミー
{ /* 0xFCC4, */ 0x0000 },	// #ダミー
{ /* 0xFCC5, */ 0x0000 },	// #ダミー
{ /* 0xFCC6, */ 0x0000 },	// #ダミー
{ /* 0xFCC7, */ 0x0000 },	// #ダミー
{ /* 0xFCC8, */ 0x0000 },	// #ダミー
{ /* 0xFCC9, */ 0x0000 },	// #ダミー
{ /* 0xFCCA, */ 0x0000 },	// #ダミー
{ /* 0xFCCB, */ 0x0000 },	// #ダミー
{ /* 0xFCCC, */ 0x0000 },	// #ダミー
{ /* 0xFCCD, */ 0x0000 },	// #ダミー
{ /* 0xFCCE, */ 0x0000 },	// #ダミー
{ /* 0xFCCF, */ 0x0000 },	// #ダミー
{ /* 0xFCD0, */ 0x0000 },	// #ダミー
{ /* 0xFCD1, */ 0x0000 },	// #ダミー
{ /* 0xFCD2, */ 0x0000 },	// #ダミー
{ /* 0xFCD3, */ 0x0000 },	// #ダミー
{ /* 0xFCD4, */ 0x0000 },	// #ダミー
{ /* 0xFCD5, */ 0x0000 },	// #ダミー
{ /* 0xFCD6, */ 0x0000 },	// #ダミー
{ /* 0xFCD7, */ 0x0000 },	// #ダミー
{ /* 0xFCD8, */ 0x0000 },	// #ダミー
{ /* 0xFCD9, */ 0x0000 },	// #ダミー
{ /* 0xFCDA, */ 0x0000 },	// #ダミー
{ /* 0xFCDB, */ 0x0000 },	// #ダミー
{ /* 0xFCDC, */ 0x0000 },	// #ダミー
{ /* 0xFCDD, */ 0x0000 },	// #ダミー
{ /* 0xFCDE, */ 0x0000 },	// #ダミー
{ /* 0xFCDF, */ 0x0000 },	// #ダミー
{ /* 0xFCE0, */ 0x0000 },	// #ダミー
{ /* 0xFCE1, */ 0x0000 },	// #ダミー
{ /* 0xFCE2, */ 0x0000 },	// #ダミー
{ /* 0xFCE3, */ 0x0000 },	// #ダミー
{ /* 0xFCE4, */ 0x0000 },	// #ダミー
{ /* 0xFCE5, */ 0x0000 },	// #ダミー
{ /* 0xFCE6, */ 0x0000 },	// #ダミー
{ /* 0xFCE7, */ 0x0000 },	// #ダミー
{ /* 0xFCE8, */ 0x0000 },	// #ダミー
{ /* 0xFCE9, */ 0x0000 },	// #ダミー
{ /* 0xFCEA, */ 0x0000 },	// #ダミー
{ /* 0xFCEB, */ 0x0000 },	// #ダミー
{ /* 0xFCEC, */ 0x0000 },	// #ダミー
{ /* 0xFCED, */ 0x0000 },	// #ダミー
{ /* 0xFCEE, */ 0x0000 },	// #ダミー
{ /* 0xFCEF, */ 0x0000 },	// #ダミー
{ /* 0xFCF0, */ 0x0000 },	// #ダミー
{ /* 0xFCF1, */ 0x0000 },	// #ダミー
{ /* 0xFCF2, */ 0x0000 },	// #ダミー
{ /* 0xFCF3, */ 0x0000 },	// #ダミー
{ /* 0xFCF4, */ 0x0000 },	// #ダミー
{ /* 0xFCF5, */ 0x0000 },	// #ダミー
{ /* 0xFCF6, */ 0x0000 },	// #ダミー
{ /* 0xFCF7, */ 0x0000 },	// #ダミー
{ /* 0xFCF8, */ 0x0000 },	// #ダミー
{ /* 0xFCF9, */ 0x0000 },	// #ダミー
{ /* 0xFCFA, */ 0x0000 },	// #ダミー
{ /* 0xFCFB, */ 0x0000 },	// #ダミー
{ /* 0xFCFC, */ 0x0000 },	// #ダミー

};


