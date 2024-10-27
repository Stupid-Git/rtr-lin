/**
 * @file	General.c
 *
 * @date	 Created on: 2019/02/26
 * @author	 haya
 * @note    2020.01.30  v6 0130 ソース反映済み
 * @note	2020.02.05	v6 0204日立ソース反映済み
 * @note	2020.Jul.27 GitHub 0722ソース反映済み
 */


#define _GENERAL_C_

#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include "General.h"
#include "RTT/SEGGER_RTT.h"
#include "cmd_thread_entry.h"
#include "system_thread.h"

#if 1
#ifdef printf
#undef printf
#endif
#define printf(...)     SEGGER_RTT_printf(0, __VA_ARGS__)
#endif

#define CRC16_SEED 0x00000000       ///< CRC16(CCITT)の初期値
#define CRC32_SEED  0xffffffff      ///< CRC32の初期値


static void   crc32_init(uint32_t *pCRC);
static void   crc32_add(uint32_t *pCRC, uint8_t val8);
static void   crc32_end(uint32_t *pCRC);

uint32_t SanitizeString( char *Dst, char *Src, uint32_t Max );

///CRC32用バッファ
uint32_t crc32_table[256];
#if 1
///Printfのテンポラリ
char PrintfTemp[300];
#endif

///Base64 encoding table
static const int8_t base64EncTable[64] =
{
   'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
   'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
   'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
   'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
};

///Base64 decoding table
static const uint8_t base64DecTable[128] =
{
   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3E, 0xFF, 0xFF, 0xFF, 0x3F,
   0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
   0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
   0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
   0xFF, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
   0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

///月名
static const char *Moname[] = {
	"000","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
};

///メッセージのカテゴリ
const char *CatMsg[] = {
	"SYS","CMD","RF ","GSM","IR ","GPS","LAN","***","***","***"
};



/**
 * CRCの演算 CRC16
 * 初期値 0
 * 多項式 16ビットCRC-CCITT（X16＋X12＋X5＋1）
 * MSBファースト （）
 * @param pBuf      バッファ番地
 * @param length    サイズ
 * @return          CRC
 *
 */
uint16_t crc16_bfr(char *pBuf, uint32_t length)
{
    uint32_t crc = 0;

    if(tx_mutex_get(&mutex_crc, WAIT_100MSEC) == TX_SUCCESS){

        g_crc.p_api->open(g_crc.p_ctrl, g_crc.p_cfg);

        g_crc.p_api->calculate(g_crc.p_ctrl, pBuf, length, CRC16_SEED, &crc);

        g_crc.p_api->close(g_crc.p_ctrl);

        tx_mutex_put(&mutex_crc);
    }else{
        Printf("CRC BUSY\n");

    }
    return ((uint16_t)(crc & 0x0000FFFF));

}
#if 0
//未使用
/**
 *
 * @param sz
 */
void crc32_test(uint32_t sz)
{
    uint32_t i, crc32;
    crc32_init(&crc32);

    for(i=0;i<sz-4;i++)
        crc32_add(&crc32, 0xff);

    crc32_end(&crc32);

    Printf("crc = %08X / %d \r\n", crc32, i);
}

#endif


/**
 * @brief   Begin CRC calculation.
 * @param pCRC      pointer to the CRC area
 */
void crc32_init(uint32_t *pCRC)
{
    *pCRC = CRC32_SEED;
}


/**
 * @brief   Calculate CRC value one at a time
 * @param pCRC      pointer to the CRC area
 * @param val8      passing value to get the CRC
 */
void crc32_add(uint32_t *pCRC, uint8_t val8)
{
    uint32_t i, poly;
    uint32_t entry;
    uint32_t crc_in;
    uint32_t crc_out;

    crc_in = *pCRC;
    poly = 0xEDB88320L;
    entry = (crc_in ^ ((uint32_t) val8)) & 0xFF;
    for (i = 0; i < 8; i++)
    {
        if (entry & 1){
            entry = (entry >> 1) ^ poly;
        }
        else{
            entry >>= 1;
        }
    }
    crc_out = ((crc_in>>8) & 0x00FFFFFF) ^ entry;
    *pCRC = crc_out;
    return;
}


/**
 * @brief       Finish CRC calculation
 * @param pCRC      pointer to the CRC area
 */
void crc32_end(uint32_t *pCRC)
{
    *pCRC ^= 0xffffffff;
}



/**
 * @brief   Get the CRC value based on size of the string.
 * 初期値  0xffffffff
 * 生成多項式    0xEDB88320L X^32+X^26+X^23+X^22+X^16+X^12+X^11+X^10+X^8+X^7+X^5+X^4+X^2+X^1+X^0
 * 出力   XOR
 * @param pBfr      Pointer to the string
 * @param size      size of the string
 * @return          CRC value

 */
uint32_t crc32_bfr(void *pBfr, uint32_t size)
{
    uint32_t crc32;
    uint8_t  *pu8;

    crc32_init(&crc32);
    pu8 = (uint8_t *) pBfr;
    while (size-- != 0)
    {
        crc32_add(&crc32, *pu8);
        pu8++ ;
    }
    crc32_end(&crc32);
    return ( crc32 );
}





/**
 * @brief   チェックサム計算、付加(SOH/STXフレーム専用 ）
 * 計算したチェックサムはバッファの最後に付加される
 *
 * @param pData     SOH/STXヘッダへのポインタ
 * @return          演算したデータ長
 * @note    2020.Apr.16 SOHフレーム専用に変更
 * @note    2020.Jul.03 def_comm型をやめる(calc_checksum_data()と統合)
 *
 */
uint16_t calc_checksum(char *pData)
{
    uint16_t sum = 0;
    int i;

    comSOH_t *pBuf = (comSOH_t *)pData;

    for(i = 0; i < pBuf->length + 5; i++){
        sum = (uint16_t)(sum + pData[i]);
    }

//    pBuf->data[pBuf->length] = (uint8_t)sum;
//    pBuf->data[pBuf->length + 1] = (uint8_t)(sum / 256);
    *(uint16_t *)&pBuf->data[pBuf->length] = sum;

    return ((uint16_t)(pBuf->length + 7));
}


/**
 * @brief   CRC計算、付加(SOHフレーム専用）
 * 計算したCRCはバッファの最後に付加される
 * comSOH_t 型のポインタに対して演算
 *
 * @param pData     SOH/STXヘッダへのポインタ
 * @return          演算したデータ長
 * @note    エンディアン注意
 */

uint16_t calc_soh_crc(char *pData)
{
    uint16_t crc;
    comSOH_t *pBuf = (comSOH_t *)pData;

    crc = crc16_bfr(&pBuf->header, (uint32_t)(pBuf->length + 5));

    pBuf->data[pBuf->length] = (uint8_t)crc;
    pBuf->data[pBuf->length + 1] = (uint8_t)(crc / 256);

    return ((uint16_t)(pBuf->length + 7));
}


#if 0
/**
 * @brief   チェックサム計算、付加(SOHフレーム専用 データ列直接）
 * 計算したチェックサムはバッファの最後に付加される
 * @param rsbuf     SOHフレーム先頭へのポインタ
 * @return          演算したデータ長
 */
uint16_t calc_checksum_data(char *rsbuf)
{
    uint16_t i, sum= 0;

    comSOH_t *pCom = (comSOH_t *)rsbuf;

    for( i = 0; i < (pCom->length + 5); i++){
        sum = (uint16_t)(sum + (uint8_t)rsbuf[i]);
    }

    *(uint16_t *)&rsbuf[i] = sum;

    return ((uint16_t)(pCom->length + 7));
}
#endif


/**
 * @brief   受信チェックサム判定 SOHフレーム用 def_com型
 *          ヘッダ（SOH/STX）からのチェックサムを判定する
 * @param   pData   SOH/STXヘッダへのポインタ
 * @retval  0 = SUM一致
 * @retval  1 = CRC一致
 * @retval  -1 = 不一致
 * @note    2020.Juｌ.03 CRC/SUM両対応
 * @note    2020.Jul.03 def_com型をやめた
 */
int judge_checksum(char *pData)
{
    uint16_t sum = 0;
    int i;
    int ret = -1;

    comSOH_t *pBuf = (comSOH_t *)pData;
    //SUM計算
    for(i = 0; i < pBuf->length + 5; i++){
        sum = (uint16_t)(sum + pData[i]);
    }
    //SUM比較
    if(sum == *(uint16_t *)&pData[i]){
        ret = 0;
    }
    else   //CRCチェック(エンディアン注意)
    {
        sum = crc16_bfr(&pBuf->header, (uint32_t)(pBuf->length+7));
        if(sum == 0){
            ret = 1;
        }
    }

    return(ret);
}

#if 0
/**
 * @brief   受信チェックサム判定 SOHフレーム用 データ列直接
 * @param   rsbuf 受信データアドレス
 * @return  判定
 */
uint8_t judge_checksum_data(char *rsbuf)
{
    uint16_t i, sum, len;
//    volatile uint16_t sum2;       //2019.Dec.25 コメントアウト

    len = (uint16_t)(rsbuf[3] + rsbuf[4] * 256 + 5);
//    sum2 = (uint16_t)(rsbuf[len] + rsbuf[len + 1] * 256);     //19.Dec.25 コメントアウト・・

    for(sum = 0, i = 0; i < len; i++){
        sum = (uint16_t)(sum + rsbuf[i]);
    }

    //printf("\n%04X \n", sum);
    if(sum == (rsbuf[len] + (uint16_t)rsbuf[len + 1] * 256)){
        return(MD_OK);
    }

    return(MD_ERROR);
}
#endif

/**
 * @brief   T2コマンドのチェックサム判定
 * SOH,T2フレームのデータ部のみのチェックサムを判定する
 * @param pBuf  受信フレームヘッダへのポインタ
 * @retval  0 = SUM一致
 * @retval  1 = CRC一致
 * @retval  -1 = 不一致
 * @note    2020.Juｌ.03 CRC/SUM両対応
 */
int judge_T2checksum(comT2_t *pBuf)
{
    int i;
    uint16_t sum = 0;
    int ret = -1;
	//Printf("(%ld)\r\n", pBuf->length);
    //SUM計算
    for(i = 0; i < pBuf->length; i++){
		sum = (uint16_t)(sum + (uint8_t)pBuf->data[i]);
        //sum = sum + (uint16_t)((uint8_t)pBuf->data[i]); 
		//Printf("%02X(%04X) ", (uint8_t)pBuf->data[i], sum);
    }
	Printf("\r\n\r\n");
	//Printf("%02X %02X %04X %04X (%ld)\r\n", (uint8_t)pBuf->data[i],(uint8_t)pBuf->data[i+1], sum,sum2,i);
    //SUM比較
    if(sum == *(uint16_t *)&pBuf->data[i]){
        ret = 0;
    }
    else   //CRCチェック(エンディアン注意)
    {
        sum = crc16_bfr(pBuf->data, (uint32_t)(pBuf->length+2));
        if(sum == 0){
            ret = 1;
        }
    }

    return(ret);
}

/**
 * @brief   本機の電池残量０～５レベルを返す
 * @param   batt
 * @return
 */
uint8_t inherent_BAT_level_0to5(uint16_t batt)
{
    uint8_t ret = 5;
    if(batt == 0xeeee){
        ret = 0;
    }
    else if (batt < 2200){
        ret = 0;
    }
    else if(batt < 2350){
        ret = 1;
    }
    else if(batt < 2500){
        ret = 2;
    }
    else if(batt < 2600){
        ret = 3;
    }
    else if(batt < 2700){
        ret = 4;
    }
    else{
        ret = 5;
    }
    return (ret);
}

/**
 * BAT_level_0to5
 * @param batt
 * @return
 */
uint8_t BAT_level_0to5(uint16_t batt)
{
    //if(EXT_POW == POW_HIGH) return(5);                          // 外部電源ありなら５

    return(inherent_BAT_level_0to5(batt));
}


/**
 * @brief   子機の電波強度レベルを返す
 * @param rssi
 * @return
 * @note    48～145程度なのでその間を5分割
 * @note    2020.01.31  修正
 * @note    同様の動きをする関数がある rtr500_RSSI_1to5()
 * @see     rtr500_RSSI_1to5()
 */
uint8_t ExLevelWL( uint8_t rssi )
{
    uint8_t ret = rssi;

     if ( rssi > 120 ){
         ret = 5;
     }
    else if ( rssi > 100 ){
        ret = 4;
    }
    else if ( rssi >  80 ){
        ret = 3;
    }
    else if ( rssi >  60 ){
        ret = 2;
    }
    else{
        ret = 1;
    }

    return ( ret );
}


/**
 * 子機の電池残量レベルを返す
 * @param val
 * @return  電池残量レベル
 */
/*
uint8_t ExLevelBatt( uint8_t val )
{
		uint8_t batt = val;
		
    if ( batt <= 2 ){
        batt = 5;   // 充分
    }
    else if ( batt <= 3 ){
        batt = 4;   // 少なくなり始め
    }
    else if ( batt <= 5 ){
        batt = 3;   // 半分以下
    }
    else if ( batt <= 7 ){
        batt = 2;   // 早めに交換を
    }
    else if ( batt <= 9 ){
        batt = 1;   // 今すぐ交換
    }
    else{
        batt = 0;   // データ消去おそれ有り
    }

    return (batt);
}
*/


/**
 * 子機の電池残量レベルを返す
 * @param val
 * @return  電池残量レベル
 */
uint8_t ExLevelBatt( uint8_t val )
{
		uint8_t batt = val;
		
    if ( batt <= 2 ){
        batt = 5;   // [0,1,2]充分
    }
    else if ( batt <= 4 ){
        batt = 4;   // [3,4]少なくなり始め
    }
    else if ( batt <= 6 ){
        batt = 3;   // [5,6]半分以下
    }
    else if ( batt <= 7 ){
        batt = 2;   // [7]早めに交換を
    }
    else if ( batt <= 8 ){
        batt = 1;   // [8]今すぐ交換
    }
    else{
        batt = 0;   // [9-15]データ消去おそれ有り
    }

    return (batt);
}










/**
 * @brief   Stringに指定文字をコピーする
 * @param Dst
 * @param Src
 * @param Size
 * @param Max
 */
void WriteText(char *Dst, char *Src, size_t Size, uint32_t Max)
{
    if(Size > 256 || Size > Max){
        return;
    }
    if( Src == 0){
        return;
    }

    memcpy(Dst, Src, Size);
    memset(Dst+Size, 0, Max-Size);

}




/**
 * IPアドレス(文字列) -> 4byteIPアドレス
 * @param Adrs
 * @param IP
 * @return
 * @note 2020.01.24  i,lenをintに変更
 */
bool Str2IP( char *Adrs, char *IP )
{
	char	*src = Adrs;				// データへのポインタ
	uint32_t size = strlen( Adrs );		// データサイズ
	char	*wd;
	int	i, len;

//	int		Term[4];
	char	dmy[4];
	int Period;
	bool	err = false;

	if ( (size >= 7) && (size <= 15) ) {		// *.*.*.*

		Period = 0;

		for ( i = 0; i < 3; i++ ) {				// ピリオド3つ
		    wd = memchr( src, '.', size );
			if ( wd ) {
				len = wd - src;				// 文字数
				if ( len ) {
					IP[Period++] = (char)AtoI( src, 3 );
					src = wd + 1;
					size = (uint32_t)(size - (uint32_t)( len + 1 ));
				}
				else{
				    goto IPerr;
				}
			}
			else{
			    goto IPerr;
			}
		}

		if ( (Period == 3) && (size <= 3) ) {
			memcpy( dmy, src, size );
			dmy[size] = '\0';
			IP[3] = (char)AtoI( dmy, 3 );	// 最後の１セグメント
		}
		else{
			goto IPerr;
		}

#if 0
		//uint8_tなので常にfalse 2019.Dec.25
		for ( i=0; i<4; i++ ) {
			if ( IP[i] < 0 || IP[i] > 255 ){
			    goto IPerr;
			}
		}
#endif
	}
	else{
	    goto IPerr;
	}

	err = true;

IPerr:

	if ( err == false ){
	    IP[0] = IP[1] = IP[2] = IP[3] = 0;
	}

	return (err);

}  



/**
 * @brief uint32を -> 4byte uint8に変換
 * @param val
 * @param RET
 */
void uint32to8(uint32_t val, char *RET)
{
    char	dmy[4];

    dmy[3] = (uint8_t)(val & 0x000000ff);      
    dmy[2] = (uint8_t)((val >> 8 ) & 0x000000ff);
    dmy[1] = (uint8_t)((val >> 16 ) & 0x000000ff);
    dmy[0] = (uint8_t)((val >> 24 ) & 0x000000ff);

    for(int i = 0; i < 4; i++){
        RET[i] = dmy[i];
    }
}


/**
 * @brief uint32のアドレスを、xxx.xxx.xxx.xxxに変換
 * @param adr
 * @param val
 */
void address_in2str(uint32_t adr, char *val)
{
    char str[16];
    int len,i;
    len = sprintf(str, "%d.%d.%d.%d", (int)(adr>>24), (int)(adr>>16)&0xFF, (int)(adr>>8)&0xFF, (int)(adr)&0xFF );
    
    if(len > 15){
        len = 15;
    }
    for(i=0;i<len;i++){
        val[i] = str[i];
    }
}

/**
 * @brief Base64 encoding algorithm
 * @param[in] input Input data to encode
 * @param[in] inputLen Length of the data to encode
 * @param[out] output NULL-terminated string encoded with Base64 algorithm
 * @param[out] outputLen Length of the encoded string (optional parameter)
 **/
void base64Encode(const void *input, size_t inputLen,  int8_t *output, size_t *outputLen)
{
   size_t n;
   uint8_t a;
   uint8_t b;
   uint8_t c;
   uint8_t d;
   const uint8_t *p;

   //Point to the first byte of the input data
   p = (const uint8_t *) input;

   //Divide the input stream into blocks of 3 bytes
   n = inputLen / 3;

   //A full encoding quantum is always completed at the end of a quantity
   if(inputLen == (n * 3 + 1))
   {
      //The final quantum of encoding input is exactly 8 bits
      if(input != NULL && output != NULL)
      {
         //Read input data
         a = (p[n * 3] & 0xFC) >> 2;
         b = (uint8_t)((p[n * 3] & 0x03) << 4);

         //The final unit of encoded output will be two characters followed
         //by two "=" padding characters
         output[n * 4] = base64EncTable[a];
         output[n * 4 + 1] = base64EncTable[b];
         output[n * 4 + 2] = '=';
         output[n * 4 + 3] = '=';
         output[n * 4 + 4] = '\0';
      }

      //Length of the encoded string (excluding the terminating NULL)
      if(outputLen != NULL){
          *outputLen = n * 4 + 4;
      }
   }
   else if(inputLen == (n * 3 + 2))
   {
      //The final quantum of encoding input is exactly 16 bits
      if(input != NULL && output != NULL)
      {
         //Read input data
         a = (p[n * 3] & 0xFC) >> 2;
         b = (uint8_t)(((p[n * 3] & 0x03) << 4) | ((p[n * 3 + 1] & 0xF0) >> 4));
         c = (uint8_t)((p[n * 3 + 1] & 0x0F) << 2);

         //The final unit of encoded output will be three characters followed
         //by one "=" padding character
         output[n * 4] = base64EncTable[a];
         output[n * 4 + 1] = base64EncTable[b];
         output[n * 4 + 2] = base64EncTable[c];
         output[n * 4 + 3] = '=';
         output[n * 4 + 4] = '\0';
      }

      //Length of the encoded string (excluding the terminating NULL)
      if(outputLen != NULL){
          *outputLen = n * 4 + 4;
      }
   }
   else
   {
      //The final quantum of encoding input is an integral multiple of 24 bits
      if(output != NULL)
      {
         //The final unit of encoded output will be an integral multiple of 4
         //characters with no "=" padding
         output[n * 4] = '\0';
      }

      //Length of the encoded string (excluding the terminating NULL)
      if(outputLen != NULL){
          *outputLen = n * 4;
      }
   }

   //If the output parameter is NULL, then the function calculates the
   //length of the resulting Base64 string without copying any data
   if(input != NULL && output != NULL)
   {
      //The input data is processed block by block
      while(n-- > 0)
      {
         //Read input data
         a = (p[n * 3] & 0xFC) >> 2;
         b = (uint8_t)(((p[n * 3] & 0x03) << 4) | ((p[n * 3 + 1] & 0xF0) >> 4));
         c = (uint8_t)(((p[n * 3 + 1] & 0x0F) << 2) | ((p[n * 3 + 2] & 0xC0) >> 6));
         d = p[n * 3 + 2] & 0x3F;

         //Map each 3-byte block to 4 printable characters using the Base64
         //character set
         output[n * 4] = base64EncTable[a];
         output[n * 4 + 1] = base64EncTable[b];
         output[n * 4 + 2] = base64EncTable[c];
         output[n * 4 + 3] = base64EncTable[d];
      }
   }
}


/**
 * @brief Base64 decoding algorithm
 * @param[in] input Base64-encoded string
 * @param[in] inputLen Length of the encoded string
 * @param[out] output Resulting decoded data
 * @param[out] outputLen Length of the decoded data
 * @return Error code
 **/
//karel
#define NO_ERROR                0x00
#define ERROR_INVALID_PARAMETER 0x01
#define ERROR_INVALID_LENGTH    0x02
#define ERROR_INVALID_CHARACTER 0x03
//
//karel

error_t base64Decode(const int8_t *input, size_t inputLen, void *output, size_t *outputLen)
{
   size_t i;
   size_t j;
   uint32_t value;
   uint8_t *p;

   //Check parameters
   if((input == NULL) && (inputLen != 0)){
       return (ERROR_INVALID_PARAMETER);
   }
   if(outputLen == NULL){
       return (ERROR_INVALID_PARAMETER);
   }

   //Point to the buffer where to write the decoded data
   p = (uint8_t *) output;
   //Length of the decoded data
   i = 0;

   //The length of the string to decode must be a multiple of 4
   if((inputLen % 4) != 0){
       return (ERROR_INVALID_LENGTH);
   }

   //Process the Base64-encoded string
   while(inputLen >= 4)
   {
      //Divide the input stream into blocks of 4 characters
      for(value = 0, j = 0; j < 4; j++)
      {
         //The "==" sequence indicates that the last block contains only 1 byte
         if(inputLen == 2 && input[0] == '=' && input[1] == '=')
         {
            //Decode the last byte
            if(p != NULL){
                p[i] = (value >> 4) & 0xFF;
            }

            //Return the length of the decoded data
            *outputLen = i + 1;

            //Decoding is now complete
            return (NO_ERROR);
         }
         //The "=" sequence indicates that the last block contains only 2 bytes
         else if(inputLen == 1 && input[0] == '=')
         {
            //Decode the last two bytes
            if(p != NULL)
            {
               p[i] = (value >> 10) & 0xFF;
               p[i + 1] = (value >> 2) & 0xFF;
            }

            //Return the length of the decoded data
            *outputLen = i + 2;

            //Decoding is now complete
            return (NO_ERROR);
         }

         //Ensure the current character belongs to the Base64 character set
         if(((uint8_t) *input) > 127 || base64DecTable[(uint8_t) *input] > 63)
         {
            //Decoding failed
            return (ERROR_INVALID_CHARACTER);
         }

         //Decode the current character
         value = (value << 6) | base64DecTable[(uint8_t) *input];

         //Point to the next character to decode
         input++;
         //Remaining bytes to process
         inputLen--;
      }

      //Map each 4-character block to 3 bytes
      if(p != NULL)
      {
         p[i] = (value >> 16) & 0xFF;
         p[i + 1] = (value >> 8) & 0xFF;
         p[i + 2] = value  & 0xFF;
      }

      //Next block
      i += 3;
   }

   //Return the length of the decoded data
   *outputLen = i;

   //Decoding is now complete
   return (NO_ERROR);
}




/**
 * サイズ付き strcpy
 * 最後にnullが入る
 * @param dst
 * @param src
 * @param size
 * @return  戻り値はコピー後のポインタ(dst)位置（nullの次）
 * @note    サイズ0およびsrc=0の場合は先頭番地に0が入る
 */
char *StrCopyN( char *dst, char *src, int size )
{
	char data;

	if ( src ) {
		while ( size > 0 ) {
			data = *src++;
			*dst++ = data;
			if ( !data ){
			    goto exit_cpyn;		// nullがあったら終了
			}
			size--;					// sakaguchi 2020.11.20
		}
	}
	*dst++ = 0;

exit_cpyn:

	return (dst);		// nullの次のポインタ

}

/**
 *
 * @param dst
 * @param src
 * @param size_d
 * @param size_s
 * @return
 */
int Memcmp(char *dst, char *src, int size_d, int size_s)
{

	if(size_d != size_s){
	    return (-1);
	}

	if(memcmp(dst, src, (size_t)size_d) != 0){
	    return (-1);
	}
	else{
	    return (0);
	}

}

/**
 *
 * @param src
 * @param dst
 * @param max
 */
void Read_StrParam(char *src , char *dst, uint32_t max)
{
//	uint16_t len,i;        //2019.Dec.25 コメントアウト
    uint32_t i;
//	len = (uint16_t)strlen(src);        //2019.Dec.25 コメントアウト
	for(i = 0; i < max; i++)
	{
		if(*src == 0x00){
		    break;
		}
		*dst++ = *src++;
	}
	*dst = 0x00;

	//Printf(" len = %d %d \r\n", len, max);

}




/**
 * シリアル番号から機種コードを生成
 * @param SerialCode
 * @param Attrib
 * @return
 * @note    子機のみ
 */
uint16_t GetSpecNumber( uint32_t SerialCode, uint8_t Attrib )
{
	uint16_t	Code;

	// 31**0001
	Code = (uint16_t)( ( SerialCode >> 16 ) & 0x00FF );

	Printf(" GetSpecNumber Code %04X Att %02X\r\n", Code, Attrib );

	switch ( Code ) {
		case 0xB8:
		case 0xB9:
			Code = SPEC_501;
			break;

		case 0xBA:
		case 0xBB:
			Code = SPEC_502;
			break;

		case 0xBC:
		case 0xBD:
			Code = SPEC_503;
			break;

		case 0x80:
		case 0x81:
			Code = SPEC_501B;
			break;

		case 0x82:
		case 0x83:
			Code = SPEC_502B;
			break;

		case 0x84:
		case 0x85:
			Code = SPEC_503B;
			break;

		case 0xBE:
		case 0xBF:
			Code = SPEC_502PT;
			break;

		case 0xC0:
		case 0xC1:
			Code = SPEC_574;
			break;

		case 0xC2:		// 505
		//case 0xC3:

			switch ( Attrib ) {
				case 0xA1:	Code = SPEC_505TC;
							break;
				case 0xA2:	Code = SPEC_505PT;
							break;
				case 0xA3:	Code = SPEC_505A;
							break;
				case 0xA4:	Code = SPEC_505V;
							break;
				case 0xA5:	Code = SPEC_505PS;
							break;
				default:
					Code = SPEC_505TC;
					break;
			}
			break;

		case 0xC3:		// 505B

			switch ( Attrib ) {
				case 0xA1:	Code = SPEC_505BTC;
							break;
				case 0xA2:	Code = SPEC_505BPT;
							break;
				case 0xA3:	Code = SPEC_505BA;
							break;
				case 0xA4:	Code = SPEC_505BV;
							break;
				case 0xA5:	Code = SPEC_505BPS;
							break;
				case 0xA6:	Code = SPEC_505BLX;
							break;
				default:
					Code = SPEC_505BTC;
					break;
			}
			break;


		case 0xC4:		// 576
		case 0xC5:
			Code = SPEC_576;
			break;

		case 0xC6:		// 507
//		case 0xC7:
			Code = SPEC_507;
			break;

		case 0xC7:		// 507B
			Code = SPEC_507B;
			break;

		case 0x7C:		// 601i
		case 0x7D:	
		case 0x7E:		// 601
			Code = SPEC_601;
			break;
		case 0x7F:		// 602
			Code = SPEC_602;
			break;

		default:
			Code = 0;
			break;
	}

	Printf(" GetSpecNumber Code %04X Att %02X\r\n", Code, Attrib );
	return (Code);
}

/************************************************************************
 * 機種コードからチャンネル数を返す
 * @param Code  機種コード
 * @return  チャンネル数
 * @note    子機のみ
 ************************************************************************/
uint16_t GetChannelNumber( uint16_t Code )
{
	uint16_t ch = 0;

	switch ( Code ) {
		case SPEC_501:
		case SPEC_502:
		case SPEC_501B:
		case SPEC_502B:

		case SPEC_502PT:
		case SPEC_505TC:
		case SPEC_505PT:
		case SPEC_505A:
		case SPEC_505V:
		case SPEC_505PS:		//????

		case SPEC_505BTC:
		case SPEC_505BPT:
		case SPEC_505BA:
		case SPEC_505BV:
		case SPEC_505BLX: 
		case SPEC_505BPS:		//????
			ch = 1;
			break;
		case SPEC_503:
		case SPEC_503B:
		case SPEC_507:
		case SPEC_507B:
			ch = 2;
			break;
		
		case SPEC_574:
			ch = 6;
			break;
		case SPEC_576:
			ch = 3;
			break;

		case SPEC_601:
			ch = 0;
			break;
		default:
			ch = 0;
			break;
	}


	return (ch);
}

/**
 * 機種コードからモデル名を探す
 * @param Serial
 * @param Code
 * @return
 */
char *GetModelName( uint32_t Serial, uint16_t Code )
{
	uint32_t		i;
	uint16_t	spec;
	char	*name;

	spec = (uint16_t)( Serial >> 28 );		// 1桁目(仕向け)

	if ( spec != 0x0E )			// ESPEC以外 ?
	{
		for ( i = 0; i < DIMSIZE(ModelName); i++ )			// 今まで通り
		{
			spec = ModelName[i].Code;
			name = (char *)ModelName[i].Name;

			if ( (spec == Code) || (spec == SPEC_NONE) ){
				Printf("GetModelName code = %02X spec = %02X\r\n", Code,spec);
			    break;
			}
		}
	}
	else
	{
		for ( i = 0; i < DIMSIZE(ModelNameEspec); i++ )			// ESPECテーブルから拾う
		{
			spec = ModelNameEspec[i].Code;
			name = (char *)ModelNameEspec[i].Name;

			if ( (spec == Code) || (spec == SPEC_NONE) ){
			    break;
			}
		}
	}

	return (name);						// モデル名を返す
}


//====================================================================
// 自身のシリアル番号からNW/AW(US)/AW(EU)を判別
// ・上位4桁のコードを渡す
//====================================================================
//int GetMyProduct( UWORD SerialCode )



/**
 * 書式による日付と時刻の文字列の取得
 *
 * RTC_GetFormStr()と同等だがこちらで
 * 最大Max文字+'\0'までで最後は\0が必ず入る
 * @param GSec      グローバル秒を引数とする
 * @param Dst
 * @param TForm
 * @param Max
 * @param Exp
 * @return          戻り値は変換後の文字数(最後のnullは除く)
 * @note    親機名の \/:*?'"<>| は _ へ強制変換
 */
uint32_t GetFormatString( uint32_t GSec, char *Dst, char *TForm, uint32_t Max, char *Exp )
{
//	RTC_Struct Tm;
    uint32_t Point, size;
	char	data, *Base = Dst;
	char	tmp[sizeof(my_config.device.Name)+2];
	char	name[sizeof(my_config.device.Name)+2];				// sakaguchi add 2020.08.27
    rtc_time_t Tm;

	if ( GSec ){
	    RTC_GSec2LCT( GSec, &Tm );
	}
	else{
	    RTC_GetLCT( &Tm );			// 現在時刻取得
	}

	Point = 0;
	while ( Point < Max ) {

		data = *TForm++;
		if ( data == '\0' ){
		    break;
		}
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
					sprintf( tmp, "20%02d", Tm.tm_year );
					break;
				case 'y' :		// 年 2桁
					size = 2;
					sprintf( tmp, "%02d", Tm.tm_year );
					break;
				case 'M' :		// 月 英語表記 3桁
					size = 3;
					sprintf( tmp, "%.3s", Moname[Tm.tm_mon] );
					break;
				case 'm' :		// 月 02桁
					size = 2;
					sprintf( tmp, "%02d", Tm.tm_mon );
					break;
				case 'n' :		// 月 1～2桁
					size = (uint32_t)sprintf( tmp, "%-d", Tm.tm_mon );
					if ( size > 2 )
						size = 2;
					break;
				case 'd' :		// 日 02桁
					size = 2;
					sprintf( tmp, "%02d", Tm.tm_mday );
					break;
				case 'j' :		// 日 1～2桁
					size = (uint32_t)sprintf( tmp, "%-d", Tm.tm_mday );
					if ( size > 2 )
						size = 2;
					break;
				case 'H' :		// 時 02桁
					size = 2;
					sprintf( tmp, "%02d", Tm.tm_hour );
					break;
				case 'i' :		// 分 02桁
					size = 2;
					sprintf( tmp, "%02d", Tm.tm_min );
					break;
				case 's' :		// 秒 02桁
					size = 2;
					sprintf( tmp, "%02d", Tm.tm_sec );
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
					//EEP_RDP( InstName, SaniTemp );
					//size = SanitizeString( tmp, SaniTemp, 32 );
				    //size = SanitizeString( tmp, my_config.device.Name, 32);
					//size = (uint32_t)sprintf(tmp,"%s", my_config.device.Name );			// sakaguchi cg 2020.08.27
					memset(name, 0x00, sizeof(name));
					memcpy(name, my_config.device.Name, sizeof(my_config.device.Name));
					size = (uint32_t)sprintf(tmp,"%s",name);
					break;
				case 'C' :		// 子機名称 *Expを使う 最大8文字
					if ( Exp ) {
						//size = SanitizeString( tmp, Exp, 26 );		// ??? 修正必要
// sakaguchi 2021.05.17 ↓
						//size = (uint32_t)sprintf(tmp,"%s", Exp );
						memset(name, 0x00, sizeof(name));
						memcpy(name, Exp, sizeof(ru_registry.rtr501.ble.name));
						size = (uint32_t)sprintf( tmp, "%s", name);
// sakaguchi 2021.05.17 ↑
					}
					else{
					    size = (uint32_t)sprintf( tmp, "C" );
					}
					break;

				default:
					size = 0;
					TForm--;
					break;
			}
		}

		if ( (Point + size) <= Max ) {
			memcpy ( Dst, tmp, size );
			Point += size;
			Dst += size;
		}
		else{
		    break;
		}

	}
	*Dst++ = '\0';

	return ((uint32_t)( Dst - Base - 1 ));
}


/**
 * 書式による日付と時刻の文字列の取得（ローカル時刻固定）
 *
 * 最大Max文字+'\0'までで最後は\0が必ず入る
 * "*"の直後の文字で該当しない物は消える
 * @param GSec
 * @param Dst
 * @param TForm
 * @return      戻り値は変換後の文字数(最後のnullは除く)
 */
int GetTimeString( uint32_t GSec, char *Dst, char *TForm )
{
//	RTC_Struct Tm;
	int		size;
	char	data, *Base = Dst;
	rtc_time_t Tm;

	if ( GSec ){
	    RTC_GSec2Date( GSec, &Tm );
	}
	else{
	    RTC_GetLCT( &Tm );			// 現在時刻取得
	}

	while ( 1 ) {

		data = *TForm++;
		if ( data == '\0' ){
		    break;
		}
		else if ( data == (char)0xFF ) {	// 0xFFはスキップする
			continue;
		}
		else if ( data != '*' ) {
			*Dst++ = data;
			continue;
		}
		else
		{
			data = *TForm++;
			switch ( data ) {
				case 'Y' :		// 年 4桁
					size = sprintf( Dst, "20%02d", Tm.tm_year );
					break;
				case 'y' :		// 年 2桁
					size = sprintf( Dst, "%02d", Tm.tm_year );
					break;
				case 'M' :		// 月 英語表記 3桁
					size = sprintf( Dst, "%.3s", Moname[Tm.tm_mon] );
					break;
				case 'm' :		// 月 02桁
					size = sprintf( Dst, "%02d", Tm.tm_mon );
					break;
				case 'n' :		// 月 1～2桁
					size = sprintf( Dst, "%-d", Tm.tm_mon );
					if ( size > 2 )
						size = 2;
					break;
				case 'd' :		// 日 02桁
					size = sprintf( Dst, "%02d", Tm.tm_mday );
					break;
				case 'j' :		// 日 1～2桁
					size = sprintf( Dst, "%-d", Tm.tm_mday );
					if ( size > 2 )
						size = 2;
					break;
				case 'H' :		// 時 02桁
					size = sprintf( Dst, "%02d", Tm.tm_hour );
					break;
				case 'i' :		// 分 02桁
					size = sprintf( Dst, "%02d", Tm.tm_min );
					break;
				case 's' :		// 秒 02桁
					size = sprintf( Dst, "%02d", Tm.tm_sec );
					break;
				case 'P' :		// 時差 +00:00
					size = sprintf( Dst, "%+03ld:%02ld", RTC.Diff/3600L,labs(RTC.Diff)%3600L );
					break;
				case 'O' :		// 時差 +0000
					size = sprintf( Dst, "%+03ld%02ld", RTC.Diff/3600L,labs(RTC.Diff)%3600L );
					break;

				default:		// *B(親機名称), *C(子機名称)など該当無しは全て消す
					size = 0;
					break;
			}
		}

		Dst += size;

	}
	*Dst = '\0';

	return ( Dst - Base );		// 文字数を返す
}

//====================================================================
// 書式による日付と時刻の文字列の取得
//  ・最大Max文字+'\0'までで最後は\0が必ず入る
//  ・戻り値は変換後の文字数(最後のnullは除く)
//  ・"*"の直後の文字で該当しない物は消える
//  ・Flag: bit0:年  bit1:秒 が消える
//====================================================================
int GetTimeFormat( int GSec, char *Dst, char *TForm, int Flag )
{
//	RTC_Struct Tm;
	int		size, cnt=0;
	char	data, *Base = Dst;
	rtc_time_t	Tm;
	const char *erase = (const char *)"/-:";		//	while( pos = strpbrk( pos, "\\/:;,*?\"<>|&" ) ) {

	if ( GSec )
		RTC_GSec2Date( (uint32_t)GSec, &Tm );
	else
		RTC_GetLCT( &Tm );			// 現在時刻取得

	while ( 1 ) {

		data = *TForm++;
		if ( data == '\0' )
			break;
		else if ( data != '*' ) {
			*Dst++ = data;
			cnt++;
			continue;
		}
		else
		{
			data = *TForm++;
			switch ( data ) {
// Y は消し込み
				case 'Y' :		// 年 4桁
					if ( Flag & 0x0001 ) {
						if ( strchr( erase, *TForm ) )
							TForm++;			// 後ろの１つを消す
						else if ( cnt && strchr( erase, *(Dst-1) ) )
							Dst--;
						size = 0;
					}
					else
						size = sprintf( Dst, "20%02d", Tm.tm_year );
					break;
				case 'y' :		// 年 2桁
					if ( Flag & 0x0001 ) {
						if ( strchr( erase, *TForm ) )
							TForm++;			// 後ろの１つを消す
						else if ( cnt && strchr( erase, *(Dst-1) ) )
							Dst--;
						size = 0;
					}
					else
						size = sprintf( Dst, "%02d", Tm.tm_year );
					break;
//
				case 'M' :		// 月 英語表記 3桁
					size = sprintf( Dst, "%.3s", Moname[Tm.tm_mon] );
					break;
				case 'm' :		// 月 02桁
					size = sprintf( Dst, "%02d", Tm.tm_mon );
					break;
				case 'n' :		// 月 1～2桁
					size = sprintf( Dst, "%-d", Tm.tm_mon );
					if ( size > 2 )
						size = 2;
					break;
				case 'd' :		// 日 02桁
					size = sprintf( Dst, "%02d", Tm.tm_mday );
					break;
				case 'j' :		// 日 1～2桁
					size = sprintf( Dst, "%-d", Tm.tm_mday );
					if ( size > 2 )
						size = 2;
					break;
				case 'H' :		// 時 02桁
					size = sprintf( Dst, "%02d", Tm.tm_hour );
					break;
				case 'i' :		// 分 02桁
					size = sprintf( Dst, "%02d", Tm.tm_min );
					break;
// s は消し込み
				case 's' :		// 秒 02桁
					if ( Flag & 0x0002 ) {
						if ( cnt && strchr( erase, *(Dst-1) ) )
							Dst--;
						size = 0;
					}
					else
						size = sprintf( Dst, "%02d", Tm.tm_sec );
					break;
/////
/*				case 'P' :		// 時差 +00:00
					size = sprintf( Dst, "%+03ld:%02ld", RTC.Diff/3600L,labs(RTC.Diff)%3600L );
					break;
				case 'O' :		// 時差 +0000
					size = sprintf( Dst, "%+03ld%02ld", RTC.Diff/3600L,labs(RTC.Diff)%3600L );
					break;
*/
				default:		// *B(親機名称), *C(子機名称)など該当無しは全て消す
					size = 0;
					break;
			}
		}

		Dst += size;
		cnt += size;

	}
	*Dst = '\0';

	return ( Dst - Base );		// 文字数を返す
}





/**
 * 禁止文字を置換
 * @param Dst
 * @param Src
 * @param Max
 * @return
 */
uint32_t SanitizeString( char *Dst, char *Src, uint32_t Max )
{
    uint32_t		size, len;
	char	code;
	const char *rep_str = "\\/:;,*?\"<>|&";		// 禁止文字列

	size = strlen( Src );
	if ( size > Max ){
	    size = Max;
	}

	len = 0;


	if ( 1 /*MyProductID & MY_PID_JP*/ )				// 日本語版は Shift-JIS
	{
		while( size > 0 )
		{
			code = *Src++;
			size--;

			if ( code == 0 )
			{
				break;
			}
			else if ( code < 0x007F )
			{
				if( strchr( rep_str, code ) ){	// このエリアのみチェック
					code = '_';					// 変換
				}
				*Dst++ = code;
				len++;
			}
			else if ( code < 0x0081 )
			{
//				*Dst = (UBYTE)Code;				// 7F, 80 は無視
			}
			else if ( code < 0x00A0 )			// 漢字エリア 0x81～0x9F
			{
				*Dst++ = code;
				code = *Src++;
				*Dst++ = code;
				size--;
				len += 2;
			}
			else if ( code < 0x00E0 )			// 半角カナエリア
			{
				*Dst++ = code;
				len++;
			}
			else if ( code < 0x00FD )			// 漢字エリア
			{
				*Dst++ = code;
				code = *Src++;
				*Dst++ = code;
				size--;
				len += 2;
			}
		}
	}
	else		// US,EU
	{
		while( size > 0 )
		{
			code = *Src++;
			if ( code )
			{
				if( strchr( rep_str, code ) ){
				    code = '_';					// 変換
				}
				*Dst++ = code;
				size--;
				len++;
			}
			else{
			    break;
			}
		}
	}

	*Dst = 0;			// 最後はnull

	return (len);
}


/**
 * ******************************************************************************
	@brief	サイズ付き strtoui()
	@param 	*src
	@param	size
	@retval	size	-1はエラー
	@note	※数字以外が含まれるとエラー（-1を返す)
				・0から始まっても10進数扱いです。
				・マイナス値もエラー
				 ・４桁まで
	@note 	呼び出し元	JudgeIPAdrs()
	@note	汎用
******************************************************************************
*/
int Str2U( char *src, uint32_t size )
{
	uint32_t i;		//uint8_t → uint16_t → uint32_t
	char	data;
	int res = 0;

	if ( size <= 4 )
	{
		for ( i = 0; i < size; i++ )
		{
			data = *src++;

			if ( (data == 0) || (data == '.') )		// NULL or '.'
			{
				if ( i == 0 ) {
					res = -1;					// いきなり出現はエラー
					break;
				}
				else{
					break;
				}
			}

			data = (char)(data - '0');
			if ( data <= 9 ) {
				res = (res * 10 + data);
			}
			else{
				res = -1;
				break;
			}
		}
	}
	else{
		res = -1;
	}
	return (res);
}


/**
 * 拡張 memcpy()
 * 64KByte以上のサイズが可能
 * 標準関数では64KByte境界で0に戻る仕様をクリアするための関数
 * ポインタがfarポインタであることが条件
 * @param dst
 * @param src
 * @param size
 * @return
 */
char *ExMemcpy( uint32_t dst, uint32_t src, uint32_t size )
{
	while( size > 0 ) {
		*( (char *)dst ) = *( (char *)src );
		dst++;
		src++;
		size--;
	}

	return ((char *)dst);
}



/**
 * UTF8の文字列のサイズ
 * @param s
 * @return
 * @note    0x00-0x7fのはカウントされない
 */
int strlen_utf8(char *s)
{
	int i = 0, j = 0;
  	while (s[i])
  	{
    	if ((s[i] & 0xc0) != 0x80){
    	    j++;
    	}
    	i++; 
	}
  	return (j);
}


/**
 * UTF8の文字列を指定されたバッファのサイズに適合させるサイズを
 * バッファサイズが小さい場合は、バッファサイズ以上の文字列はカットする
 * @param s
 * @param size
 * @return
 */
int str_utf8_len(char *s, int size)
{
	int i = 0; 
// sakaguchi 2020.11.10 ↓
	int end = 0;			// UTF8の終端ポインタ
	if(size <= 0){
		size = 0;
	}
	else{
		while(s[i])
		{
			if(i >= size){
				i = end+1;					// バッファサイズを算出
				break;
			}

			if ((s[i+1] & 0xc0) != 0x80){	// 次バイトがUTF8の先頭？
				end = i;					// UTF8の終端ポインタを更新
			}
			i++;
		}
	}

#if 0
	int top = 0;
	if(size < 0){
		size = 0;
	}
	else{
	while(s[i])
	{
		if ((s[i] & 0xc0) != 0x80){	//  UTF-8の先頭 
				//Printf("UTF8 Start %d \r\n", i);
			if(i > size){
					i--;
			    break;
			}
				top = i;
		}
			else if(i > size){
			    break;
			}
			i++; 
		}	

		if(i > size)
		i = top;
	}
#endif
// sakaguchi 2020.11.10 ↑

	//Printf("\r\n --- i=%d top=%d\r\n", i, top);
	return (i);
}


/**
  ******************************************************************************
    @brief  サイズ付き atoI()
    @param  *src    変換元数値文字列へのポインタ
    @param  size    サイズ
    @return サイズ LONG_MAXの場合10文字以上なので未変換
    @attention  オーバーフローチェック無し
                    0から始まっても10進数扱いです。
    @note   汎用
  ******************************************************************************
*/
int AtoI( char *src, int size )
{
    uint8_t   i;
    char   data;
    int32_t   res = 0;
    uint8_t   sign = 0;
//    uint16_t  sz = size;
    uint8_t flg=1;
//    uint8_t *pt = src;        2019.Dec.26 コメントアウト

    //for(i=0;i<sz;i++)
    //    Printf("   %02X  ", *pt++);

    if ( size ) {

        data = *src;
        if ( data == '+' ) {
            size--;
//            *src++;
            src++;      //ポインタのみ進める 2017.Jun.1
        }
        else if ( data == '-' ) {
            sign = 1;
            size--;
 //           *src++;
            src++;      //ポインタのみ進める 2017.Jun.1
        }

        if ( size <= 10 ) {
            for ( i = 0; i < size; i++ ) {
                data = (char)(*src++ - '0');
                if ( data <= 9 ) {
                    res = res * 10 + data;
                    flg = 0;
                }
                else{
                    break;
                }
            }
        }
    }

    if ( !flg ) {
        if ( sign ) {
            res = -res;
        }
    }
    else{
        res = LONG_MAX;        //ユーザー定義ULONG_MAX=0x7FFFFFFF→limits.h LONG_MAX = 0x7FFFFFFF
    }
    return (res);
}


/**
 * @brief   サイズ付き atol() ※オーバーフローチェック無し
 * @param src
 * @param size
 * @return
 * @note    0から始まっても10進数扱いです。
 */
int AtoL( char *src, int size )
{
    uint8_t   i; 
    uint8_t   data;
    int  res = 0;
    uint8_t   sign = 0, flg=1;;
    int  sz = size;

    if ( sz ) {

        data = *src;
        if ( data == '+' ) {
            sz--;
 //           *src++;
            src++;      //ポインタのみ進める 2020.01.20
        }
        else if ( data == '-' ) {
            sign = 1;
            sz--;
//            *src++;
            src++;      //ポインタのみ進める 2020.01.20
        }

        if ( sz <= 10 ) {
            for ( i=0; i<sz; i++ ) {
                data = (char)(*src++ - '0');
                if ( data <= 9 ) {
                    res = res * 10 + data;
                    flg = 0;
                }
                else{
                    break;
                }
            }
        }
    }

    if ( !flg ) {
        if ( sign ){
            res = -res;
        }
    }
    else{
        res = LONG_MAX;     //ユーザー定義ULONG_MAX=0x7FFFFFFF→limits.h LONG_MAX = 0x7FFFFFFF
    }

    return ((int)res);
}

void Print_warning_buffer(int x)
{
	int cnt;
	Printf("auto_control.w_config[cnt].group_no %d\r\n",x);
	for(cnt = 0 ; cnt < 128 ; cnt++){		
		Printf("%02X ", auto_control.w_config[cnt].group_no);
		if((cnt+1)%32 == 0)
			Printf("\r\n");
	}
	Printf("\r\n");
}

/**
 * @brief   TERMへprintfの結果を出力
 * @param fmt
 * @note        2020.Jan.27  SEGGER_RTT専用に修正
 * @attention   使用前に必ずSEGGER_RTT_Init()をコールすること(1回のみ)
 */
void Printf( const char *fmt, ... )
{
	va_list args;
//	int		Size;
	va_start( args, fmt );

//********** TERM未定義の場合、以下は消えます ******************
#ifdef DBG_TERM
    //SEGGER_RTT_printf(0, fmt);
	
	vsprintf( PrintfTemp, fmt ,args);
	printf("%s", PrintfTemp);

#endif
//********** ここまで ******************

	va_end( args );
}
