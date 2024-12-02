/*
 * comp_led_thread.h
 *
 *  Created on: Dec 2, 2024
 *      Author: karel
 */

#ifndef COMP_OPTO_THREAD_H_
#define COMP_OPTO_THREAD_H_

#include "_r500_config.h"

#include <stdint.h>
#include "Globals.h" // for comSOH_t

#ifdef EDF
#undef EDF
#endif

#ifdef _COMP_OPTO_THREAD_C_
#define EDF
#else
#define EDF extern
#endif


#define OPTICOM_BAUDRATE    2400

///光通信コマンド番号　optc_msg[] メッセージキューで受けるコマンド
enum {
    ID_IR_64BYTE_READ = 0,
    ID_IR_64BYTE_WRITE,
    ID_IR_HEADER_READ,
    ID_IR_DATA_DOWNLOAD_T,
    ID_IR_TYPE_READ,
    ID_IR_SIMPLE_COMMAND,
    ID_IR_SIMPLE_COMMAND_RETRY,
    ID_IR_REGIST,
    ID_IR_SOH_DIRECT,

};


//optc_event_flagsのイベントフラグ
#define FLG_OPTC_REQUEST        0x00000001      ///< optc_event_flagsのイベントフラグ    コマンド実行要求  2020.Jul.03 追加(まだ未使用)
#define FLG_OPTC_END            0x00000002      ///< //optc_event_flagsのイベントフラグ コマンド処理終了


/// Thread に渡すパラメータ
EDF struct {
//    char     *out_poi;           ///< 吸い上げデータを入れるポインタ     //未使用
    uint32_t    result;
    uint32_t    cmd_id[4];
    uint32_t    GetTime;            ///< 0x47:個数[個]指定    0x48:吸い上げ期間[分]指定    0x49:吸い上げデータ番号

    char     *buf;

    uint16_t    DataSize;

    uint16_t len;                ///< command length
    char     cm;                 ///< command
    char     scm;                ///< sub comannd
    char     sw;                 ///< 0:吸い上げ開始        1:次データ要求
    char     Cmd;                ///< 0x47:個数指定       0x48:期間指定指定         0x49:データ番号指定

    char     gid[8];             ///< group id
    char     name[8];            ///< name
    char     num;                ///<
    char     ch;                 ///< channel
    char     band;               ///< freg band


} optc;

//光通信用バッファ  ->static
//EDF def_com OCOM;                   ///< 光通信用バッファ  Opti Comm Globals.hから移動 → static

EDF comSOH_t *pOptUartTx;           //光通信 SOH構造体ポインタ 送信用　cmd_thread_entryで利用する
EDF comSOH_t *pOptUartRx;           //光通信 SOH構造体ポインタ 受信用　cmd_thread_entryで利用する


/* 関数宣言 */
/*
//cmd_thread_entry()から呼ばれる（修正予定）　→　cmd_thread_entry.cに移動
EDF uint32_t IR_Nomal_command(uint8_t Cmd, uint16_t length, char *pTxData);      // 光通信　通常コマンド
EDF uint32_t IR_data_download_t(char *, uint8_t, uint8_t, uint64_t);     // 光通信　データ吸い上げ(1024byte毎)
//EDF uint32_t IR_data_block_read(void);                          // 光通信　データブロック要求
EDF uint32_t IR_header_read(uint8_t, uint16_t);                 // 光通信　データ吸い上げ要求
EDF uint32_t IR_64byte_write(char, char *);               // 光通信　６４バイト情報を書き込む
EDF uint32_t IR_64byte_read(char, char *);                // 光通信　６４バイト情報を読み込
EDF uint32_t IR_read_type(void);
EDF uint32_t IR_soh_direct(comSOH_t *pTx, uint32_t length);                               // 光通信　SOHダイレクトコマンド
EDF uint32_t IR_Regist(void);                                   // 光通信　子機登録
*/


#endif /* COMP_OPTO_THREAD_H_ */
