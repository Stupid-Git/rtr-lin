/*
 * Cmd_func.h
 *
 *  Created on: Nov 25, 2024
 *      Author: karel
 */

#ifndef CMD_FUNC_H_
#define CMD_FUNC_H_

#include <stdint.h>

#ifdef EDF
#undef EDF
#endif

#ifdef _CMD_FUNC_C_
#define EDF
#else
#define EDF extern
#endif

/**
 *  パラメータリスト構造体
 *  @note   2020.01.17  UINT定義をintに変更
 */
typedef struct{
    struct{
        char   *Name;
        char   *Data;
        int    Size;
        //int32_t    Size;
        //int64_t    Size;
    } __attribute__((packed)) Arg[40];// __attribute__((aligned(4)));
    int Count;
} __attribute__((packed)) PARA_LIST;// __attribute__((aligned(4)));

// I think the problem we get without the attributes is because there
// are various
// #pragma pack(1) and #pragma pack(4) statements floating around
// So depending where this header file is called, we get a different packing size
// and so each "Arg" is either (8+8+4=20) bytes or (8+8+8=24) bytes.

///パラメータリスト構造体
EDF PARA_LIST   PLIST;

#if 0
//cmd_thread_entry.c/hに移動
EDF void StatusPrintf( char *Name, const char *fmt, ... );                  // 返送パラメータのセット
EDF void StatusPrintf_S(uint16_t max, char *Name, const char *fmt,  ...  ); // 返送パラメータのセット
EDF void StatusPrintf_v2s(char *Name, char *Data, int size, const char *fmt);
EDF void StatusPrintfB(char *, char *, int);                         // 返送パラメータのセット（バイナリ版）
#endif





EDF int32_t ParamInt( char *Key );                                      // パラメータ名に対応したデータ(10進数)を取得する
EDF int32_t ParamHex( char *Key );                                      // パラメータ文字列(Ｈｅｘ)を数値に変換する
EDF int32_t ParamInt32( char *Key );                                    // パラメータ名に対応したデータ(10進数)を取得する
EDF int32_t ParamAdrs( char *Key, char **Adrs );                        // パラメータ名に対応したデータ(文字列)を取得する
EDF uint8_t Hash( char *Key );                                            // パラメータ名に対応したデータ位置を検索する
EDF int split( char *str, const char *delim, char *outlist[] );         // 文字列を分割する
EDF int comma_separate_array_make( char *str, int size );           // カンマ区切りの文字列を作成する



#endif /* CMD_FUNC_H_ */
