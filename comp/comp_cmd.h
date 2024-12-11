/*
 * comp_cmd.h
 *
 *  Created on: Nov 25, 2024
 *      Author: karel
 */

#ifndef COMP_CMD_H_
#define COMP_CMD_H_


#ifdef EDF
#undef EDF
#endif
#ifndef COMP_CMD_C_
#define EDF extern
//#define EDF_EXTERN 1
//#warning "Setting EDF to extern"
#else
#define EDF
//#warning "Setting EDF to Nothing"
#endif

#include <stdint.h>
#include <stdbool.h>
#include <string.h>


// g_command_event_flagsのイベントフラグ
#define FLG_CMD_EXEC            0x00000001       ///< g_command_event_flagsのイベントフラグ コマンド実行要求
#define FLG_CMD_END             0x00000002       ///< g_command_event_flagsのイベントフラグ コマンド完了




// コマンド通信先
#define CMD_NON     0
#define CMD_USB     1
#define CMD_BLE     2
#define CMD_TCP     3
#define CMD_HTTP    4

///T2コマンド処理メッセージキューの定義
typedef struct{
    uint32_t CmdRoute;      ///<T2コマンドの要求元
    uint8_t *pT2Command;    ///<T2コマンドの先頭ポインタ
    uint8_t *pT2Status;     ///<T2コマンド応答の先頭ポインタ
    int32_t *pStatusSize;     ///<T2コマンド応答のサイズへのポインタ
}CmdQueue_t;

EDF char latest[32];         ///<  最終実行コマンド

///コマンド処理応答データ用バッファ
EDF char StsArea[8 * 1024L];
EDF volatile int CmdStatusSize;        ///<コマンドステータスのサイズ  @attention 注意：複数の関数から書き込まれる @note 2020.01.17 intに変更


#define CMD_NOP         0x01    //(0b00000001)        // パラメータ無し
#define CMD_PER         0x02    //(0b00000010)        // パラメータエラー

#define CMD_RES(X)  (( X & 0x02 ) ? ERR(CMD,FORMAT) : ERR(CMD,NOERROR) )



// g_command_event_flagsのイベントフラグ
#define FLG_CMD_EXEC            0x00000001       ///< g_command_event_flagsのイベントフラグ コマンド実行要求
#define FLG_CMD_END             0x00000002       ///< g_command_event_flagsのイベントフラグ コマンド完了




//EDF int32_t ParamInt( char *Key );                                        // パラメータ名に対応したデータ(10進数)を取得する
//EDF int32_t ParamHex( char *Key );                                        // パラメータ文字列(Ｈｅｘ)を数値に変換する
//EDF int32_t ParamInt32( char *Key );                                        // パラメータ名に対応したデータ(10進数)を取得する
//EDF int32_t ParamAdrs( char *Key, char **Adrs );                      // パラメータ名に対応したデータ(文字列)を取得する
//EDF uint8_t Hash( char *Key );                                            // パラメータ名に対応したデータ位置を検索する


//EDF uint8_t Rec_StartTime_set(uint32_t Secs);                 // [EWSTW]にUTCが含まれていた場合、指定時刻で記録開始する

EDF bool ble_passcode_lock;

/**
 * CFWriteInfo()で使用
 * @bug post.WUINFは値セットのみ
 * @bug post.EINITは未使用
 */
EDF struct def_post {
    bool WUINF;
    bool EINIT;
} post;


//Globals.hから移動
EDF uint8_t dbg_cmd;




//関数
EDF void Reboot_check(void);

EDF void StatusPrintfB(char *, char *, int);                         // 返送パラメータのセット（バイナリ版）

EDF bool Update_check(void);                                         // sakaguchi 2021.08.02

//EDF uint8_t WR_set(uint8_t G_No ,uint8_t U_No ,uint8_t pogi ,uint8_t setdata);    // sakaguchi 2021.03.01


#endif /* COMP_CMD_H_ */
