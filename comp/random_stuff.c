/*
 * random_stuff.c
 *
 *  Created on: Nov 25, 2024
 *      Author: karel
 */

#include "_r500_config.h"

#include <string.h>

#include "r500_defs.h"

#include "random_stuff.h"

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
        res = (int32_t)0x7FFFFFFF; //LONG_MAX;        //ユーザー定義ULONG_MAX=0x7FFFFFFF→limits.h LONG_MAX = 0x7FFFFFFF
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
        res = (int32_t)0x7FFFFFFF; //LONG_MAX;     //ユーザー定義ULONG_MAX=0x7FFFFFFF→limits.h LONG_MAX = 0x7FFFFFFF
    }

    return ((int)res);
}

