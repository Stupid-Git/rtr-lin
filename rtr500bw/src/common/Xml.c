/**
 *  @brief  XML処理
 *  @file    Xml.c
 *   *  @date   Created on: 2019/09/03
 *  @author tooru.hayashi
 *  @attention  ARMはコンパイラオプションで指定しない限り char は符号無し（uint8_tとの代入でワーニングは出る）
 */

#define _XML_C_
#include <string.h> //note: include ‘<string.h>’ or provide a declaration of ‘memset’
//#include "common_data.h"
#include "Globals.h"
#include "Xml.h"

static void	XML_Create( int );	
static void	XML_OpenTag( char * );
static void	XML_CloseTag(void);
static void	XML_CloseAll(void);
static void XML_OpenTagAttrib(char *TagName, char *Attrib);
static void XML_TagOnce( char *TagName, const char *fmt, ... );
static void XML_TagOnce2( char *TagName, const char *fmt, ... );
static void XML_TagOnceAttrib( char *TagName, char *Attrib, const char *fmt, ... );
static void XML_TagOnceEmpty( bool Flag, char *TagName, const char *fmt, ... );
static void XML_Entity( const char *fmt, ... );
static void XML_Plain( const char *fmt, ... );

static void XML_Base64( bool flag, char *Src, int Size );
static int	XML_Encode( char *Dst, char *Src );
static int	XML_Encode2( char *Dst, char *Src );
//static int	XML_EncodeSjis( char *Dst, char *Src );

static void XML_WriteRAM(void);



//********************************************************
//	関数のプロトタイプ宣言と外部変数
//  （このソースの内部のみ記述）
//********************************************************


/**
 * @brief   XMLオブジェクト
 * @attention   オブジェクト定義の関数以外、外部から呼び出さないこと！！
 */
OBJ_XML	XML = {

	XML_Create,				// コンストラクタ
	XML_OpenTag,
	XML_CloseTag,
	XML_OpenTagAttrib,
	XML_TagOnce,
	XML_TagOnce2,
	XML_TagOnceAttrib,
	XML_TagOnceEmpty,
	XML_Entity,
	XML_Plain,
	XML_Base64,
	XML_CloseAll,
	XML_Encode,
	XML_Encode2,
//	XML_Output,
	XML_WriteRAM,       //XML_Outputの代わりにとりあえず  2020.01.17
	0,      //XML_OutFlag = 0
    0,      // *TagPos現在オープンされているタグの格納アドレス
    0,      // *BuffPos現在書き込まれているバッファの先頭
    0,      // *FileNameXMLファイル名
    0       //Size バッファにある有効データ数
};


#define F_OK 0    //debug


/**
 * @brief   XML開始
 * @param Flag
 */
static void XML_Create( int Flag )
{
//	int		Temp;       ///2019.Dec.26 コメントアウト
//	SSP_PARAMETER_NOT_USED(Flag);
	(void)Flag;

	memset(xml_buffer, 0 , sizeof(xml_buffer)); //32768*4 Byte

	XML.TagPos = TagTemp;				// タグ作成用
	XML.BuffPos = XMLBuff;  			// XML送信用
	*XML.TagPos++ = (char)0xFF;			// 最初に0xFFを入れておく
	XML.OutFlag = F_OK;
	XML.FileName = XMLName;
//	XMLName[0] = 0;						// 念のため初期化

	//if ( MyProductID & MY_PID_JP )		// 日本語？
	//	XML.Encode = XML_EncodeSjis;	// SJIS -> UTF8
	//else
		XML.Encode = XML_Encode;		// Windows1252 -> UTF8

	XML.Output = XML_WriteRAM;			// 出力用関数へのポインタ

	XML_Plain( XML_HEADER );			// <?xml>
	XML.Size = 0;

	Printf( "\nXML:Start\n" );
}



/**
 ******************************************************************************
    @brief      XMLタグオープン
                     "abc" -> "/abc"+size(1byte) で格納していく
    @param      *TagName    XMLタグ
    @return     none
    @see            OBJ_XML XML
    @note       呼び出し元
 ******************************************************************************
*/
static void XML_OpenTag( char *TagName )
{
    char	*Pos = XML.TagPos;
	int		Size;

	*Pos++ = '/';								    // タグ閉じる準備
	XML.TagPos = StrCopyN( Pos, TagName, 32 );	    // タグ名称は最大32byteとする
	Size = (int)( XML.TagPos - Pos );			    // 文字列サイズ
	*( XML.TagPos - 1 ) = (char)Size;			    // 

	Size = sprintf( XML.BuffPos, "<%.20s>\n", TagName );		//
//	Printf( XML.BuffPos );
	XML.BuffPos += Size;						    // バッファを次へ
}


/**
 ******************************************************************************
    @brief      XMLタグオープン(属性付き）
                    "abc" -> "/abc"+size(1byte) で格納していく
    @param      *TagName    XMLタグ
    @param      Attrib      属性
    @see            OBJ_XML XML
    @note       呼び出し元
 ******************************************************************************
*/
void XML_OpenTagAttrib(char *TagName, char *Attrib)
{
    //va_list args;       //可変引数リスト

    char *Pos = XML.TagPos;
    int     Size;

    //va_start( args, fmt );      //可変引数リスト初期化

    *Pos++ = '/';                               // タグ閉じる準備
    XML.TagPos = StrCopyN(Pos, TagName, 32);    // タグ名称は最大32byteとする
    Size = (int16_t) (XML.TagPos - Pos);            // 文字列サイズ
    *(XML.TagPos - 1) = (uint8_t) Size;         //

    Size = sprintf( XML.BuffPos, "<%.20s %.30s>\n", TagName, Attrib);       //
    XML.BuffPos += Size;                        // バッファを次へ


//    vsprintf( EntityTemp,  fmt, args);    // 実体を作成

//    Size = XML_Encode(XML.BuffPos, EntityTemp);    // エンコード展開(quote処理なし）
//    XML.BuffPos += Size;                        // バッファを次へ

    //strcpy( XML.BuffPos, ">" CRLF );
    //XML.BuffPos += 3;                           // タグを閉じる


//  *XML.BuffPos++ = '>';

    //va_end( args ); //可変引数リスト終了
}


/**
 ******************************************************************************
    @brief      XMLClose（タグ取得）
    @note       呼び出し元
 ******************************************************************************
*/
static void XML_CloseTag(void)
{
    int Size;

	XML.TagPos--;					// ポジションデクリメント
	Size = (int)*XML.TagPos;		// 文字列サイズ
	if ( Size < 32 ) {

		*XML.TagPos = 0;			// nullセット
		XML.TagPos -= Size;			// タグの先頭ポインタ

//		Printf( "\nXML:<%s>", XML.TagPos );
		Size = sprintf( XML.BuffPos, "<%.20s>\n", XML.TagPos );		//
//		Printf( XML.BuffPos );
		XML.BuffPos += Size;						// バッファを次へ

	}
	else {
		Size = sprintf( XML.BuffPos, "<noTags>" );		//
//		Printf( XML.BuffPos );
//		XML.BuffPos += Size;						// バッファを次へ

//		XML_Create( XML.Output );					// 強制初期化
	}

	XML_WriteRAM();
	//XML.Output();					// バッファ吐き出し
}

/**
 ******************************************************************************
    @brief      XML 1タグ吐き出し
    @param      *TagName    XMLタグ
    @param      *fmt, ...   可変長引数
    @return     none
    @see            OBJ_XML XML
    @note       呼び出し元
 ******************************************************************************
*/
static void XML_TagOnce( char *TagName, const char *fmt, ... )
{
	va_list args;

//	char	*Pos = XML.BuffPos;
	int		Size;

	va_start( args, fmt );

	Size = sprintf( XML.BuffPos, "<%.20s>", TagName );		//
	XML.BuffPos += Size;

//	vsprintf( XML.BuffPos, fmt, args );		// いったん実体を作成
//	XML_Entity( XML.BuffPos );				// 変換へ

	vsprintf( EntityTemp, fmt, args );				// 実体を作成
	Size = XML.Encode( XML.BuffPos, EntityTemp );	// エンコード展開
	XML.BuffPos += Size;

	Size = sprintf( XML.BuffPos, "</%.20s>\n", TagName );	//
	XML.BuffPos += Size;

//	Printf( Pos );

	XML_WriteRAM();
//	XML.Output();					// バッファ吐き出し

	va_end( args );
}

static void XML_TagOnce2( char *TagName, const char *fmt, ... )
{
	va_list args;

//	char	*Pos = XML.BuffPos;
	int		Size;

	va_start( args, fmt );

	Size = sprintf( XML.BuffPos, "<%.70s>", TagName );		//
	XML.BuffPos += Size;

//	vsprintf( XML.BuffPos, fmt, args );		// いったん実体を作成
//	XML_Entity( XML.BuffPos );				// 変換へ

	vsprintf( EntityTemp, fmt, args );				// 実体を作成
	Size = XML.Encode2( XML.BuffPos, EntityTemp );	// エンコード展開
	XML.BuffPos += Size;

	Size = sprintf( XML.BuffPos, "</%.70s>\n", TagName );	//
	XML.BuffPos += Size;

//	Printf( Pos );

	XML_WriteRAM();
//	XML.Output();					// バッファ吐き出し

	va_end( args );
}


/**
 * @brief   XML 1タグ吐き出し(属性付き)
 * @param TagName
 * @param Attrib
 * @param fmt
 */
static void XML_TagOnceAttrib( char *TagName, char *Attrib, const char *fmt, ... )
{
	va_list args;

//	char	*Pos = XML.BuffPos;
	int		Size;

	va_start( args, fmt );

	Size = sprintf( XML.BuffPos, "<%.20s %.30s>", TagName, Attrib );	//
	XML.BuffPos += Size;

//	vsprintf( XML.BuffPos, fmt, args );		// いったん実体を作成
//	XML_Entity( XML.BuffPos );				// 変換へ

	vsprintf( EntityTemp, fmt, args );				// 実体を作成
	Size = XML.Encode( XML.BuffPos, EntityTemp );	// エンコード展開
	XML.BuffPos += Size;

	Size = sprintf( XML.BuffPos, "</%.20s>\n", TagName );	//
	XML.BuffPos += Size;

//	Printf( Pos );

//XML_WriteRAM();
	XML.Output();					// バッファ吐き出し

	va_end( args );
}




/**
 ******************************************************************************
    @brief      XML 1タグ吐き出し（ FALSE時は空タグにする ）
    @param      Flag    フラグ
    @param      *TagName    XMLタグ
    @param      *fmt, ...   可変長引数
    @return     none
    @see            OBJ_XML XML
    @note       呼び出し元
 ******************************************************************************
*/
static void XML_TagOnceEmpty( bool Flag, char *TagName, const char *fmt, ... )
{
	va_list args;

//	char	*Pos = XML.BuffPos;
	int		Size;

	va_start( args, fmt );

	if ( Flag ) {

		Size = sprintf( XML.BuffPos, "<%.20s>", TagName );		//
		XML.BuffPos += Size;

		vsprintf( EntityTemp, fmt, args );				// 実体を作成
		Size = XML.Encode( XML.BuffPos, EntityTemp );	// エンコード展開
		XML.BuffPos += Size;

		Size = sprintf( XML.BuffPos, "</%.20s>\n", TagName );	//
		XML.BuffPos += Size;

		//	Printf( Pos );

	}
	else
	{
		Size = sprintf( XML.BuffPos, "<%.20s></%.20s>\n", TagName, TagName );	//
		XML.BuffPos += Size;
	}

//XML_WriteRAM();
	XML.Output();					// バッファ吐き出し

	va_end( args );
}

/**
 ******************************************************************************
    @brief      XML 実体生成
    @param      *fmt, ...   可変長引数
    @return     none
    @see            OBJ_XML XML
    @note       呼び出し元
 ******************************************************************************
*/
static void XML_Entity( const char *fmt, ... )
{
	va_list args;

	int		Size;

	va_start( args, fmt );

	vsprintf( EntityTemp, fmt, args );				// 実体を作成
	Size = XML.Encode( XML.BuffPos, EntityTemp );	// エンコード展開

//	Printf( XML.BuffPos );

	XML.BuffPos += Size;

	va_end( args );

//	return Size;
}

/**
 ******************************************************************************
    @brief      XML そのまま出力
    @param      *fmt, ...   可変長引数
    @return     none
    @see            OBJ_XML XML
    @note       呼び出し元
 ******************************************************************************
*/
static void XML_Plain( const char *fmt, ... )
{
	va_list args;

	int		Size;

	va_start( args, fmt );

	Size = vsprintf( XML.BuffPos, fmt, args );				// 実体を作成

	XML.BuffPos += Size;

	va_end( args );

//	return Size;
}


/**
 ******************************************************************************
    @brief      XML終了
                    残り全てのタグをクローズする
    @note       呼び出し元       SendMonitorData(), SendWarningData()
 ******************************************************************************
*/
static void XML_CloseAll(void)
{
	while( XML.TagPos > &TagTemp[1] ) {

		XML_CloseTag();

	}

//XML_WriteRAM();
	XML.Output();					// バッファ吐き出し

}


//========================================================

/**
 ******************************************************************************
    @brief      XML Base64
                    *SrcのデータをBASE64でエンコードしてXML.BuffPosの位置に保存する
    @param      Flag
        @arg        true    そのまま
        @arg        false   2byteとびとび
    @param      *Src    変換元へのポインタ
    @param      Size
    @return     none
    @attention  4/3倍になることに注意
    @see            OBJ_XML XML
    @note       呼び出し元
 ******************************************************************************
*/
static void XML_Base64( bool Flag, char *Src, int Size )
{
	uint16_t	x, y, z;
	int rest;
	char *Dst;
	uint8_t flag;

//	Flag が TRUE はそのまま FLASE は2byteでとびとび

	Dst = XML.BuffPos;			// バッファの先頭
	
	rest = Size % 3;
	Size -= rest;

	//int xx = 0;

	if ( Flag ) {

		while( Size > 0 ) {

			x = (uint16_t)(*Src++ & 0x00ff);
			y = (uint16_t)( ( x << 8 ) | (*Src++ & 0x00ff));
			z = (uint16_t)(( y << 8 ) | (*Src++ & 0x00ff));

			*Dst++ = Encode[ ( x >> 2 ) & 0x003F ];
			*Dst++ = Encode[ ( y >> 4 ) & 0x003F ];
			*Dst++ = Encode[ ( z >> 6 ) & 0x003F ];
			*Dst++ = Encode[ z & 0x003F  ];

			Size -= 3;

	//		if(xx < 2)
	//			Printf("        XML [%04X], [%04X], [%04X]\r\n",x,y,z);
	//		xx++;	
		}

		if ( rest > 0 ) {

			y = z = 0;
			x = (uint16_t)(*Src++ & 0x00ff);				// 余り 1byte
			y = (uint16_t)(( x << 8 ) | ( ( rest == 2 ) ? (*Src & 0x00ff): 0 ));	// 余り 2byteは次を読む
			z = (uint16_t)( y << 8 );

			*Dst++ = Encode[ ( x >> 2 ) & 0x003F ];
			*Dst++ = Encode[ ( y >> 4 ) & 0x003F ];

			if ( rest == 2 ){
				*Dst++ = Encode[ ( z >> 6 ) & 0x003F ];
			}else{
				*Dst++ = '=';
			}
			*Dst++ = '=';
		}

	}
	else		// 2CHバッファ
	{

		flag = 0;

		while( Size > 0 ) {

			x = (uint16_t)(*Src++ & 0x00ff);
			if ( flag++ & 0x01 ){
			    Src += 2;	// 2回に1回2byte進める
			}
			y =(uint16_t)( ( x << 8 ) | (*Src++ & 0x00ff));
			if ( flag++ & 0x01 ){
			    Src += 2;
			}
			z =(uint16_t)( ( y << 8 ) | (*Src++ & 0x00ff));
			if ( flag++ & 0x01 ){
			    Src += 2;
			}

			*Dst++ = Encode[ ( x >> 2 ) & 0x003F ];
			*Dst++ = Encode[ ( y >> 4 ) & 0x003F ];
			*Dst++ = Encode[ ( z >> 6 ) & 0x003F ];
			*Dst++ = Encode[ z & 0x003F  ];

			Size -= 3;
		}

		if ( rest > 0 ) {

			y = z = 0;
			x = (uint16_t)(*Src++ & 0x00ff);					// 余り 1byte
			if ( flag++ & 0x01 ){
			    Src += 2;	// 2回に1回2byte進める
			}
			y = (uint16_t)( ( x << 8 ) | ( ( rest == 2 ) ? (*Src & 0x00ff) : 0 ));	// 余り 2byteは次を読む
			z = (uint16_t)( y << 8 );

			*Dst++ = Encode[ ( x >> 2 ) & 0x003F ];
			*Dst++ = Encode[ ( y >> 4 ) & 0x003F ];

			if ( rest == 2 ){
				*Dst++ = Encode[ ( z >> 6 ) & 0x003F ];
			}else{
				*Dst++ = '=';
			}
			*Dst++ = '=';
		}

	}
	XML.BuffPos = Dst;			// バッファの最終アドレスをセット

	XML.Output();					// バッファ吐き出し
}

/**
 ******************************************************************************
    @brief      XML エンコード  (UTF-8用)

                    使用できないキャラクタ[",<,>等]を変換
        " <tag>*****</tag>  ***** " の文字列をSrcとして指定
    @param      *Dst        変換データ保存先ポインタ
    @param      *Src        変換元へのポインタ
    @return     展開後のサイズ
    @attention  Srcはnullで終端している事
    @note       2020.01.17  whileの条件判断の代入式を修正
 ******************************************************************************
*/
static int XML_Encode( char *Dst, char *Src )
{
//	static const char *Hex = (const char *)"0123456789ABCDEF";
	char	data, *base = Dst;

	while( true ) {

	    data = *Src++;      //1Byte 読み出し
        if ( data == 0 ) {       //NULL
            break;
        }
        else if ( data >= 0x80 ) {

/*
			 *Dst++ = '&';
			 *Dst++ = '#';
			 *Dst++ = 'x';
			 *Dst++ = Hex[data >> 4];
			 *Dst++ = Hex[data & 0x0F];
			 *Dst++ = ';';
*/

			Dst += Win2UTF8( Dst, data );	// 1文字をUTF-8へ変換
		}
		else//ASCII文字列
		{

			switch ( data ) {
				case '&' :
				    Dst = StrCopyN( Dst, "&amp;", 5 ) - 1;
					break;

				case '<' :
				    Dst = StrCopyN( Dst, "&lt;", 4 ) - 1;
					break;

				case '>' :
				    Dst = StrCopyN( Dst, "&gt;", 4 ) - 1;
					break;

				case 0x22 :         // 「 " 」
				    Dst = StrCopyN( Dst, "&quot;", 6 ) - 1;
					break;

				case 0x27 :         // 「 ' 」
				    Dst = StrCopyN( Dst, "&apos;", 6 ) - 1;
					break;

				default ://通常のASCII文字列
				    *Dst++ = data;
					break;
			}
		}
	}

	*Dst = 0;			// 最後はnullを強制的に入れる

	return ( Dst - base );			// 展開後のサイズ

}


static int XML_Encode2( char *Dst, char *Src )
{
//	static const char *Hex = (const char *)"0123456789ABCDEF";
	char	data, *base = Dst;

	while( true ) {

	    data = *Src++;      //1Byte 読み出し
        if ( data == 0 ) {       //NULL
            break;
        }
        else if ( data >= 0x80 ) {
			//Dst += Win2UTF8( Dst, data );	// 1文字をUTF-8へ変換
			*Dst++ = data;
		}
		else//ASCII文字列
		{

			switch ( data ) {
				case '&' :
				    Dst = StrCopyN( Dst, "&amp;", 5 ) - 1;
					break;

				case '<' :
				    Dst = StrCopyN( Dst, "&lt;", 4 ) - 1;
					break;

				case '>' :
				    Dst = StrCopyN( Dst, "&gt;", 4 ) - 1;
					break;

				case 0x22 :         // 「 " 」
				    Dst = StrCopyN( Dst, "&quot;", 6 ) - 1;
					break;

				case 0x27 :         // 「 ' 」
				    Dst = StrCopyN( Dst, "&apos;", 6 ) - 1;
					break;

				default ://通常のASCII文字列
				    *Dst++ = data;
					break;
			}
		}
	}

	*Dst = 0;			// 最後はnullを強制的に入れる

	return ( Dst - base );			// 展開後のサイズ

}

#if 0
//未使用
/**
 * @brief   XML エンコード (SJIS対応)
 * 使用できないキャラクタを変換
 * "<tag>*****</tag>"  ***** の文字列をSrcとして指定
 * @param Dst
 * @param Src
 * @return
 * @note    Srcはnullで終端している事
 */
static int XML_EncodeSjis( char *Dst, char *Src )
{
	int		sz, Size = 0, Max = 200;	// 200文字まで
	uint16_t	Code;

	while( Max > 3 )			// 面倒なので１文字最大値(3)以上で
	{
		Code = (uint16_t)*Src++;
		sz = 0;

		if ( Code == 0 )
		{
			break;
		}
		else if ( Code < 0x007F )
		{
			switch ( Code )
			{
				case '&' :
				    StrCopyN( Dst, "&amp;", 5 );
                    sz = 5;
                    break;

				case '<' :
				    StrCopyN( Dst, "&lt;", 4 );
                    sz = 4;
                    break;

				case '>' :
				    StrCopyN( Dst, "&gt;", 4 );
                    sz = 4;
                    break;

				case 0x22 :
				    StrCopyN( Dst, "&quot;", 6 );
                    sz = 6;
                    break;			// 「 " 」

				case 0x27 :
				    StrCopyN( Dst, "&apos;", 6 );
                    sz = 6;
                    break;			// 「 ' 」

				default :
				    *Dst = (uint8_t)Code;
					sz = 1;
			}
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
			*Dst = (uint8_t)Code;			// 0x20の方がいいかな？
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

	}

	*Dst = 0;			// 最後はnull

	return (Size);

}

#endif



/**
 * @briefデータ送信（RAM）
 * データ生成の中で適宜この関数が呼ばれる
 */
static void XML_WriteRAM(void)
{

	int	size;
//	int  res;      //2019.Dec.26 コメントアウト
	char *src;
	char *dst;
//	int16_t size2;      //2019.Dec.26 コメントアウト
	char *end;      //2020.01.17 ポインタ変数にしてみた

    end = (xml_buffer + sizeof(xml_buffer));        // ?????
//    end = (uint32_t)xml_buffer + (uint32_t)sizeof(xml_buffer);
	if ( XML.OutFlag == F_OK ) {

		size = XML.BuffPos - XMLBuff;
//		size2 = size;       //2019.Dec.26 コメントアウト
		src = XMLBuff; //(uint8_t *)&XMLBuff;
		//dst = (uint32_t)xml_buffer + XML.Size;
		dst = (char *)(xml_buffer + XML.Size);  //(uint8_t *)&xml_buffer + XML.Size;
		if ( size ) {
			while( size > 0 ) {

				if( dst == end){
					//PutLog( LOG_GSM, "XML Buffer Full" );
					break;
				}

				//*( (char *)dst ) = *src++;
				*dst = *src++;
				dst++;
				XML.Size++;
				size--;
			}
			 XML.OutFlag = F_OK;

			if ( F_OK == XML.OutFlag ){
			    XML.BuffPos = XMLBuff;
			}
		}
	}
	//printf("\r\nXML Size= %ld / size = %d", XML.Size, size2);
}


#if 0

/**
 ******************************************************************************
    @brief      XMLデータ送信
                    データ生成の中で適宜この関数が呼ばれる
                    XML.ContentMode=1の時 実データ送信
                    XML.ContentMode=0の時XML.ContentLengthに値がセットされる
    @param      none
    @return     none
    @note       呼び出し元
    @see            XML
 ******************************************************************************
*/
void XML_Output( void )
{


    }
}

#endif
