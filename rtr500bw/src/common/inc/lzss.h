
/*----------------------------------------------------------------------------*
    < STM32L476 >
    file            : lzss.h
    object computer : STM32L476
    programer       : Takashi.S
    date            : Jun.5 2018
    description     : Rev.0
*----------------------------------------------------------------------------*/

#ifndef _LZSS_H_
#define _LZSS_H_


#include "tx_api.h"

#undef EDF

#ifdef _LZSS_C_
#define EDF                                                     // ソース中のみ extern を削除
#else
#define EDF extern                                              // 外部変数
#endif

// 圧縮データの情報構造体
struct LZSS_ENCODE_INFO
{
    //int8_t EncodeType;                                          // 圧縮方式
    //int8_t EncodeCode;                                          // 圧縮情報の開始を示す数値
    //int16_t PressSize ;                                         // 圧縮後のデータのサイズ(この構造体のサイズを含む）
    uint8_t EncodeType;                                         // 圧縮方式                     // 2022.05.24 cg
    uint8_t EncodeCode;                                         // 圧縮情報の開始を示す数値      // 2022.05.24 cg
    uint16_t PressSize ;                                         // 圧縮後のデータのサイズ(この構造体のサイズを含む）   // 2022.06.09
};

//EDF int32_t LZSS_Encode(char *, int32_t, char *);
//EDF int32_t LZSS_Decode(char *, char *);
EDF uint32_t LZSS_Encode(uint8_t *Src, uint32_t SrcSize, uint8_t *Dest);    // 2022.06.09
EDF uint32_t LZSS_Decode(uint8_t *PressPoint, uint8_t *DestPoint);          // 2022.06.09


#endif /* _LZSS_H_ */
