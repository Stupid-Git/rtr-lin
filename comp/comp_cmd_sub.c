/*
 * comp_cmd_sub.c
 *
 *  Created on: Nov 25, 2024
 *      Author: karel
 */


#include "_r500_config.h"

#define COMP_CMD_SUB_C_
#include "comp_cmd_sub.h"

#include "r500_defs.h"

#include "comp_cmd.h"
#include "Error.h"

#if 1
//cmd_thread_entry.c/hに移動
EDF void StatusPrintf( char *Name, const char *fmt, ... );                  // 返送パラメータのセット
EDF void StatusPrintf_S(uint16_t max, char *Name, const char *fmt,  ...  ); // 返送パラメータのセット
EDF void StatusPrintf_v2s(char *Name, char *Data, int size, const char *fmt);
EDF void StatusPrintfB(char *, char *, int);                         // 返送パラメータのセット（バイナリ版）
#endif

#define NULL_CODE   0x00

#define CMD_NOP         0x01    //(0b00000001)        // パラメータ無し
#define CMD_PER         0x02    //(0b00000010)        // パラメータエラー

#define CMD_RES(X)  (( X & 0x02 ) ? ERR(CMD,FORMAT) : ERR(CMD,NOERROR) )

// g_command_event_flagsのイベントフラグ
#define FLG_CMD_EXEC            0x00000001       ///< g_command_event_flagsのイベントフラグ コマンド実行要求
#define FLG_CMD_END             0x00000002       ///< g_command_event_flagsのイベントフラグ コマンド完了

bool ble_passcode_lock;


//=============================================================================
tdx_flags_t g_wmd_event_flags;

#include <pthread.h>

pthread_mutex_t mutex_log;
pthread_mutex_t mutex_sfmem;
pthread_mutex_t g_rf_mutex;

#define WAIT_100MSEC 10
//=============================================================================
extern uint32_t Cmd_Route;          //コマンドの発給元  USB or BLE or TCP or HTTP
static char CmdTemp[300];           //Globals.hから移動

uint16_t g_ftp_server_control_port_user; //TODO


/** Date and time structure defined in C standard library <time.h> */
typedef struct tm rtc_time_t;





//関数本体未定義 ここから 2020.01.17
/*static*/ uint32_t CFWriteInfo(void);                                          // 機器情報の設定
/*static*/ uint32_t CFReadInfo(void);                                           // 機器情報の取得

/*static*/ uint32_t CFWriteNetwork(void);                                       // ネットワークの設定
/*static*/ uint32_t CFReadNetwork(void);                                        // ネットワークの取得
/*static*/ uint32_t CFReadWalnAp(void);                                         // WLAM Ap List

/*static*/ uint32_t CFWriteNetParam(void);
/*static*/ uint32_t CFReadNetParam(void);
/*static*/ uint32_t CFWriteWlanParam(void);
/*static*/ uint32_t CFReadWlanParam(void);
/*static*/ uint32_t CFWProxy(void);
/*static*/ uint32_t CFRProxy(void);
/*static*/ uint32_t CFWriteBleParam(void);
/*static*/ uint32_t CFReadBleParam(void);
/*static*/ uint32_t CFReadBleAdv(void);
/*static*/ uint32_t CFBleAuth(void);


/*static*/ uint32_t CFWriteDtime(void);                                     // 時刻情報の設定
/*static*/ uint32_t CFReadDtime(void);                                      // 時刻情報の取得


/*static*/ uint32_t CFWriteSettingsFile(void);                              // 設定（登録）ファイル書き込み
/*static*/ uint32_t CFReadSettingsFile(void);                               // 設定（登録）ファイル読み込み

/*static*/ uint32_t CFWriteCertFile(void);                                  // ルート証明書書き込み
/*static*/ uint32_t CFReadCertFile(void);                                   // ルート証明書読み込み


/*static*/ uint32_t CFWriteWarning(void);                                       // 警報設定の書き込み
/*static*/ uint32_t CFReadWarning(void);                                        // 警報設定の読み込み
/*static*/ uint32_t CFWriteMonitor(void);
/*static*/ uint32_t CFReadMonitor(void);
/*static*/ uint32_t CFWriteSuction(void);
/*static*/ uint32_t CFReadSuction(void);
/*static*/ uint32_t CFWriteEncode(void);
/*static*/ uint32_t CFReadEncode(void);

/*static*/ uint32_t CFWriteUnitSettingsFile(void);                          // 設定（登録）ファイルに含まれる子機の設定書き込み

/*static*/ uint32_t CFReadVGroup(void);
/*static*/ uint32_t CFWriteVGroup(void);

/*static*/ uint32_t CFReadSettingsGroup(void);
/*static*/ uint32_t CFStartMonitor(void);
/*static*/ uint32_t CFClearMonitor(void);
/*static*/ uint32_t CFReadMonitorResult(void);

/*static*/ uint32_t CFWriteUserDefine(void);                                // ユーザ定義情報の書き込み
/*static*/ uint32_t CFReadUserDefine(void);                                 // ユーザ定義情報の読み込み

/*static*/ uint32_t CFInitilize(void);                                      // 本体の初期化
/*static*/ uint32_t CFBSStatus(void);                                       // 本体の状態
/*static*/ uint32_t CFNetworkStatus(void);                                  // ネットワークの状態
/*static*/ uint32_t CFReadLog(void);

/*static*/ uint32_t CFExtentionReg(void);                                   // 光通信 子機登録
/*static*/ uint32_t CFExtentionSerial(void);                                // 光通信 シリアル番号の取得
/*static*/ uint32_t CFExtSettings(void);                                    // 光通信 設定値読み書き
/*static*/ uint32_t CFExtSuction(void);                                     // 光通信 データ吸い上げ
/*static*/ uint32_t CFExtWirelessOFF(void);                                 // 光通信 電波制御
/*static*/ uint32_t CFExtRecStop(void);                                     // 光通信 記録停止
/*static*/ uint32_t CFExtAdjust(void);                                      // 光通信 アジャストメント
/*static*/ uint32_t CFExtScale(void);                                       // 光通信 スケール変換
/*static*/ uint32_t CFExtSOH(void);                                         // 光通信 ＳＯＨコマンドの送受信
/*static*/ uint32_t CFExtBlp(void);                                         // 光通信 Bluetooth設定の設定と取得


/*static*/ uint32_t CFExWirelessLevel(void);                              // 子機の電波強度＆電池残量
/*static*/ uint32_t CFRelayWirelessLevel(void);                           // 中継機の電波残量＆電池残量
/*static*/ uint32_t CFWirelessSettingsRead(void);                         // 設定値読み込み
/*static*/ uint32_t CFWirelessSettingsWrite(void);                        // 設定値書き込み
/*static*/ uint32_t CFWirelessMonitor(void);                              // 現在値取得
/*static*/ uint32_t CFWirelessSuction(void);                              // データ吸い上げコマンド
/*static*/ uint32_t CFSearchKoki(void);                                   // 子機検索
/*static*/ uint32_t CFSearchRelay(void);                                  // 中継機検索
/*static*/ uint32_t CFExRealscan(void);                                   // リアルスキャン
/*static*/ uint32_t CFThroughoutSearch(void);                             // 子機と中継機の一斉検索
/*static*/ uint32_t CFRecStartGroup(void);                                // グループ一斉記録開始
/*static*/ uint32_t CFWirelessRecStop(void);                              // 記録停止
/*static*/ uint32_t CFSetProtection(void);                                // 子機のプロテクト設定
/*static*/ uint32_t CFExInterval(void);                                   // 子機のモニタリング間隔設定
/*static*/ uint32_t CFWirelessRegistration(void);                         // 子機登録変更

/*static*/ uint32_t CFBleSettingsWrite(void);                             // BLE設定の書き込み
/*static*/ uint32_t CFBleSettingsRead(void);                              // BLE設定の読み出し
/*static*/ uint32_t CFScaleSettingsWrite(void);                           // スケール変換式の書き込み
/*static*/ uint32_t CFScaleSettingsRead(void);                            // スケール変換式の読み出し


/*static*/ uint32_t CFPushSettingsRead(void);                           // ＰＵＳＨ 子機の設定値読み込み
/*static*/ uint32_t CFPushSettingsWrite(void);                          // ＰＵＳＨ 子機の設定値書き込み
/*static*/ uint32_t CFPushSuction(void);                                // ＰＵＳＨ データ吸い上げ
/*static*/ uint32_t CFPushRecErase(void);                               // ＰＵＳＨ 子機の記録データ消去
/*static*/ uint32_t CFPushItemListRead(void);                           // ＰＵＳＨ 子機の品目リスト読み込み
/*static*/ uint32_t CFPushItemListWrite(void);                          // ＰＵＳＨ 子機の品目リスト書き込み
/*static*/ uint32_t CFPushPersonListRead(void);                         // ＰＵＳＨ 作業者リスト読み込み
/*static*/ uint32_t CFPushPersonListWrite(void);                        // ＰＵＳＨ 作業者リスト書き込み
/*static*/ uint32_t CFPushWorkgrpRead(void);                            // ＰＵＳＨ ワークグループ読み込み
/*static*/ uint32_t CFPushWorkgrpWrite(void);                           // ＰＵＳＨ ワークグループ書き込み
/*static*/ uint32_t CFPushMessage(void);                                // ＰＵＳＨ メッセージ表示
/*static*/ uint32_t CFPushRemoteMeasure(void);                          // ＰＵＳＨ リモート測定
/*static*/ uint32_t CFPushClock(void);                                  // ＰＵＳＨ 時計設定

/*static*/ uint32_t  CFTestLAN(void);                                   // LAN通信テスト for USB
/*static*/ uint32_t  CFTestWarning(void);                               // 警報の通信テスト
/*static*/ uint32_t  CFWait(void);                                      // 自律動作の一時停止

/*static*/ uint32_t  CFEventDownload(void);                             // データ吸い上げイベント


/*static*/ uint32_t CFKensa(void);
/*static*/ uint32_t CFInspect(void);
//未使用/*static*/ uint32_t CFDebug(void);
/*static*/ uint32_t CFReadSystemInfo(void);
/*static*/ uint32_t CFReadSFlash(void);

uint16_t sram_inspc(void);

/*static*/ uint32_t CFR500Special(void);        // R500無線モジュール直接SOHコマンド送受信

/*static*/ uint32_t CFWriteFirmData(void);          // [EFIRM] ファームデータ書き込み（キャッシュ）   t.saito
/*static*/ uint32_t CFRoleChange(void);         //[EBROL]   BLEクライアント、サーバー切り替え


//各関数の共通部をサブ関数化 t.saito
/*static*/ uint32_t Sub_CFSearch(int data_size, int print_sw);
/*static*/ uint32_t Sub_Act_2(void);
/*static*/ void uncmpressData(int sw);
/*static*/ void Sub_CF_DataProcess(int maxDataSize, int sw);

//光通信 呼び出し関数　opt_cmd_thread_entry.hから移動、static化
/*static*/ uint32_t IR_Nomal_command(uint8_t Cmd, uint8_t SubCmd, uint16_t length, char *pTxData);     // 光通信　通常コマンド
/*static*/ uint32_t IR_data_download_t(char *, uint8_t, uint8_t, uint64_t);     // 光通信　データ吸い上げ(1024byte毎)
///*static*/ uint32_t IR_data_block_read(void);                          // 光通信　データブロック要求
/*static*/ uint32_t IR_header_read(uint8_t, uint16_t);                 // 光通信　データ吸い上げ要求
/*static*/ uint32_t IR_64byte_write(char, char *);               // 光通信　６４バイト情報を書き込む
/*static*/ uint32_t IR_64byte_read(char, char *);                // 光通信　６４バイト情報を読み込
///*static*/ uint32_t IR_read_type(void);
/*static*/ uint32_t IR_soh_direct(comSOH_t *pTx, uint32_t length);                               // 光通信　SOHダイレクトコマンド
/*static*/ uint32_t IR_Regist(void);

/*static*/ uint32_t opt_command_exec(void);                         //T2からの光通信コマンド実行

//Cmd_func.c/hから移動
/*static*/ void StatusPrintf( char *Name, const char *fmt, ... );                  // 返送パラメータのセット
/*static*/ void StatusPrintf_S(uint16_t max, char *Name, const char *fmt,  ...  ); // 返送パラメータのセット
/*static*/ void StatusPrintf_v2s(char *Name, char *Data, int size, const char *fmt);
//===================================================================================================================
// コマンドの定義
//===================================================================================================================
/**
 * コマンドの定義
 * RF実行系コマンドは、コマンド実行時にRF-Mutexの確認を行う
 * 0x28BC Byte
 */
#if 0
CMD_TABLE HostCmd[]=
{
    //----- 本機の情報 -----
    {"WUINF:", {"NAME", "DESC", "UNITS", "PASC", "REGC", "REQ",                0}, CFWriteInfo, 0, 0},            // 機器情報の設定
    {"RUINF:", {                                                               0}, CFReadInfo, 0, 0},


    //----- ネットワークサーバ情報 ------
    { "WNSRV:", { "FTPSV","FTPID","FTPPW","FTPPT","PASV","SMTPSV","SMTPPT","AUTHEN","AUTHID","AUTHPW","DESCF","FROM",
                            "HTTPSV","HTTPPT","HTTPINT","HTTPPATH","HTTPS","SNTP1","SNTP2","SOCPT","UDPPT","RESPT",0 },  CFWriteNetwork , 1, 0  },
    { "RNSRV:", { /*"FTPSV","FTPID","FTPPW","FTPPT","PASV","WEBSV","WEBPT","SNTP1","SNTP2","SOCPT","UDPPT","RESPT",*/ 0 },    CFReadNetwork , 0, 0 },
    { "EWLAP:", {                                                               0 }, CFReadWalnAp , 0, 0  },            // WIFI AP List




    //----- ネットワークパラメータ -------
    { "WNETP:", {"IP","MASK","GW","DNS1","DNS2","DHCP","PW","PHY","MTU","INTV", 0}, CFWriteNetParam , 1, 0  },       // MTU追加 sakaguchi 2021.04.07 // sakaguchi 2021.05.28 MBL
    { "RNETP:", {                                                              0}, CFReadNetParam  , 0, 0  },


    //----- 無線LANパラメータ --------
    { "WWLAN:", { "WLEN,","BAND","SSID","SEC","WEP","PSK",                      0}, CFWriteWlanParam , 1, 0  },
    { "RWLAN:", {                                                               0}, CFReadWlanParam  , 0, 0  },

    //----- PROXY設定 ------------
    { "WPRXY:", { "FTPEN","FTPIP","FTPPT","MAILEN","MAILIP","MAILPT","HTTPEN","HTTPIP","HTTPPT","HTTPSV",        0 },  CFWProxy, 1, 0  },
    { "RPRXY:", { /*"FTPEN","FTPIP","FTPPT","MAILEN","MAILIP","MAILPT","HTTPEN","HTTPIP","HTTPPT",*/    0 },  CFRProxy, 0, 0  },


    //----- httpsルート証明書 ------------
    { "WRTCE:", { "AREA","DATA","OFFSET","SAVE","SIZE","PW",                    0 },  CFWriteCertFile,  1, 0  },
    { "RRTCE:", { "AREA","OFFSET","PW",                                         0 },  CFReadCertFile,   0, 0  },


    //----- BLE設定 ------------
    { "WBLEP:", { "ENABLE",                                                     0 },  CFWriteBleParam,  1, 0  },
    { "RBLEP:", {                                                               0 },  CFReadBleParam,   0, 0  },
    { "EBADV:", {                                                               0 },  CFReadBleAdv,     0, 0  },
    { "EAUTH:", { "PW",                                                         0 },  CFBleAuth,   0, 0  },

    //----- 時刻情報 -----
    { "WDTIM:", {"DTIME", "DIFF", "DST", "UTC", "FORM", "ZONE", "SYNC" ,        0}, CFWriteDtime, 0, 0 },           // 時刻情報の設定
    { "RDTIM:", {                                                               0}, CFReadDtime , 0, 0 },            // 時刻情報の取得

    //----- 警報設定 -----
    { "WWARP:", { "ENABLE","ROUTE","SBJ","BODY","TO1","TO2","TO3","TO4","INT","FN1","FN2","FN3","FN4","EXT", 0  },  CFWriteWarning , 1, 0  },
    { "RWARP:", {                                                                                            0  },  CFReadWarning  , 0, 0  },

    //----- 現在値送信設定 -----
    { "WCURP:", { "ENABLE","ROUTE","INT","SBJ","TO1","TO2","ATTACH","DIR","FNAME","BIND", 0 },    CFWriteMonitor , 1, 0 },
    { "RCURP:", {                                                                         0 },    CFReadMonitor  , 0, 0 },

    //----- 吸い上げ設定 -----
    { "WSUCP:", { "ENABLE","ROUTE","TYPE","EVT0","EVT1","EVT2","EVT3","EVT4","EVT5","EVT6","EVT7","EVT8","SBJ","TO1","ATTACH","DIR","FNAME", 0 },     CFWriteSuction , 1, 0  },
    { "RSUCP:", {                                                                                                                            0 },     CFReadSuction  , 0, 0  },

    //----- 設定ファイル ------
    { "WSETF:", {"DATA", "OFFSET","SAVE","SIZE",                                0}, CFWriteSettingsFile, 0, 0 },    // 設定（登録）ファイル書き込み   // sakaguchi size追加
    { "RSETF:", {"SIZE", "OFFSET",                                              0}, CFReadSettingsFile , 0, 0 },     // 設定（登録） ファイル読み込み

    //----- 表示グループファイル ------
    { "WVGRP:", { "DATA",                                                        0}, CFWriteVGroup, 0, 0 },              //
    { "RVGRP:", {                                                                0}, CFReadVGroup,  0, 0 },               //

    //----- ユーザ定義 -----
    { "WUSRD:", { "AREA","DATA",                                                0}, CFWriteUserDefine, 0, 0 },      // ユーザ定義情報の書き込み
    { "RUSRD:", { "AREA",                                                       0}, CFReadUserDefine,  0, 0 },       // ユーザ定義情報の読み込み

    //----- エンコード設定 -----
    { "WENCD:", { "MAIL","FTP",                                                 0}, CFWriteEncode, 0, 0 },              //
    { "RENCD:", { "MAIL","FTP",                                                 0}, CFReadEncode,  0, 0 },               //

    //----- 設定ファイル（子機設定） ------
    { "WREGI:", {"DATA",                                                        0}, CFWriteUnitSettingsFile, 0, 0 },    // 設定（登録）ファイルに含まれる子機の設定書き込み


    //----- 本体の初期化 -----
    { "EINIT:",  {"MODE","CNT",                                                 0}, CFInitilize, 0, 0 },         // 本体の初期化

    //----- 本体の状態 -----
    { "EBSTS:", { "SWOUT",                                                      0}, CFBSStatus, 0, 0 },             // 本体の状態
    { "ENSTS:", {                                                               0}, CFNetworkStatus, 0, 0 },        // ネットワークの状態
    { "ELOGS:", { "MODE","ACT","AREA",                                          0 }, CFReadLog , 0, 0 },           // ログの取得
    { "ETLAN:", { "MODE","ACT",                                                 0 }, CFTestLAN , 0 , 0 },           // テストコマンド(LAN)
    { "ETWAR:", { "UPPER","LOWER","SENSOR","COMM","BATT","EXTPS",               0 }, CFTestWarning , 0 , 0 },       // 警報のテスト送信(LAN)
//    { "EWAIT:", {                                                               0 }, CFWait , 0 , 0 },              // 自律動作の一時停止
    { "EWAIT:", { "SEC",                                                        0 }, CFWait , 0 , 0 },              // 自律動作の一時停止

    //---- データ吸い上げイベント発生
    { "EDWLS:", {                                                               0}, CFEventDownload,     0, 0 },    // データ吸い上げイベント
    //----- 指定グループの無線通信設定を取得する -----
    { "ERGRP:", { "GRP","SER",                                                  0}, CFReadSettingsGroup, 0, 0 },   // 指定グループの無線通信設定を取得する
//    { "ESMON:", {                                                             0}, CFStartMonitor,      0, 0 },    // 指定したグループのモニタリングを開始する
//    { "ERMON:", { "GRP",                                                      0}, CFReadMonitorResult, 0, 0 },    // 指定したグループのモニタリング結果を取得する
//    { "ECMON:", { "GRP","RPT","PU",                                           0}, CFClearMonitor,      0, 0 },    // 指定したグループのモニタリング結果を削除する
    { "EMONS:", { "MODE",                                                       0}, CFStartMonitor,      0, 0 },    // 指定したグループのモニタリングを開始する
    { "EMONR:", { "GRP",                                                        0}, CFReadMonitorResult, 0, 0 },    // 指定したグループのモニタリング結果を取得する
    { "EMONC:", { "GRP","PARENT","RU",                                          0}, CFClearMonitor,      0, 0 },    // 指定したグループのモニタリング結果を削除する

    //----- 光通信コマンド -----
    { "EIREG:", {"ID", "NAME", "NUM", "CH","BAND",                              0}, CFExtentionReg,    0, 0 },      // 子機登録
    { "EISER:", {                                                               0}, CFExtentionSerial, 0, 0 },      // シリアル番号の取得
    { "EISET:", {"DATA", "ACT",                                                 0}, CFExtSettings,     0, 0 },      // 設定値読み書き
    { "EISUC:", {"MODE", "RANGE",                                               0}, CFExtSuction,      0, 0 },      // 吸い上げ
    { "EIWRS:", {"MODE",                                                        0}, CFExtWirelessOFF,  0, 0 },      // 電波制御
    { "EIRSP:", {"MODE",                                                        0}, CFExtRecStop,      0, 0 },      // 記録停止
    { "EIADJ:", {"ACT", "DATA",                                                 0}, CFExtAdjust,       0, 0 },      // アジャストメント
    { "EISCL:", {"ACT", "DATA",                                                 0}, CFExtScale,        0, 0 },      // スケール変換
    { "EISOH:", {"TIME", "DATA",                                                0}, CFExtSOH,          0, 0 },      // ＳＯＨコマンドの送受信
    { "EIBLP:", {"ACT", "ENABLE","NAME", "PASC",                                0}, CFExtBlp,          0, 0 },      // Bluetooth設定の設定と取得

    //----- RFコマンド -----
    { "EWLEX:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "SER", "NTH", "CH", "MAX", "GRP",                          0}, CFExWirelessLevel,          0, RF_CMD },    // 子機の電波強度＆電池残量
    { "EWLRU:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "CH", "MAX", "GRP",                                 0}, CFRelayWirelessLevel,       0, RF_CMD },    // 中継機の電波強度＆電池残量
    { "EWSTR:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "SER", "NTH", "CH", "NUM", "GRP",                          0}, CFWirelessSettingsRead,     0, RF_CMD },    // 設定値読み込み
    { "EWSTW:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "SER", "NTH", "CH", "NUM", "GRP", "DATA", "FORMAT", "UTC", 0}, CFWirelessSettingsWrite,    0, RF_CMD },    // 設定値書き込み
    { "EWCUR:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "SER", "NTH", "CH", "NUM", "GRP", "FORMAT",                0}, CFWirelessMonitor,          0, RF_CMD },    // 現在値取得

    { "EWSUC:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "SER", "NTH", "CH", "NUM", "GRP", "MODE", "RANGE",         0}, CFWirelessSuction,          0, RF_CMD },    // データ吸い上げ

    { "EWSCE:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "CH", "MAX", "GRP",                                 0}, CFSearchKoki,               0, RF_CMD },    // 子機検索
    { "EWSCR:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "CH", "MAX", "GRP",                                 0}, CFSearchRelay,              0, RF_CMD },    // 中継機検索
    { "EWRSC:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "CH", "MAX", "GRP", "FORMAT",                       0}, CFExRealscan,               0, RF_CMD },    // リアルスキャン
    { "EWRSR:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "CH", "RUMAX", "RPTMAX", "GRP", "FORMAT",           0}, CFThroughoutSearch,         0, RF_CMD },    // 子機と中継機の一斉検索

    { "EWBSW:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "CH", "MAX", "GRP", "DATA",                         0}, CFRecStartGroup,            0, RF_CMD },    // グループ一斉記録開始

    { "EWRSP:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "SER", "NTH", "CH", "NUM", "GRP", "MODE",                  0}, CFWirelessRecStop,          0, RF_CMD },    // 記録停止
    { "EWPRO:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "SER", "NTH", "CH", "NUM", "GRP", "REC", "WARN", "INT",    0}, CFSetProtection,            0, RF_CMD },    // 子機のプロテクト設定
    { "EWINT:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "SER", "NTH", "CH", "NUM", "GRP", "TIME",                  0}, CFExInterval,               0, RF_CMD },     // 子機のモニタリング間隔設定
    { "EWREG:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "SER", "NTH", "CH", "NUM", "GRP", "MODE", "XID", "XBAND", "XCH", "XNUM", "XNAME", "XBLEN", 0}, CFWirelessRegistration, 0, RF_CMD },        // 子機登録変更

    { "EWBLW:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "SER", "NTH", "CH", "NUM", "GRP", "ENABLE", "NAME", "SECURITY",  "NEWPASC" , "OLDPASC", 0}, CFBleSettingsWrite, 0, RF_CMD },    // BLE設定書き込み
    { "EWBLR:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "SER", "NTH", "CH", "NUM", "GRP",                           0}, CFBleSettingsRead,          0, RF_CMD },    // BLE設定読み出し
    { "EWSLW:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "SER", "NTH", "CH", "NUM", "GRP", "DATA",                   0}, CFScaleSettingsWrite,       0, RF_CMD },    // スケール変換式書き込み
    { "EWSLR:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "SER", "NTH", "CH", "NUM", "GRP",                           0}, CFScaleSettingsRead,        0, RF_CMD },    // スケール変換式読み出し

    { "EWBTE:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "SER", "NTH", "CH", "NUM", "GRP", "MODE",                   0}, CFSearchKoki,               0, RF_CMD },    // 子機BLE On Off


    //----- PUSHコマンド -----
    { "EWPSR:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "CH", "NUM", "GRP",                      0}, CFPushSettingsRead,    0, RF_CMD },    // ＰＵＳＨ 子機の設定値読み込み
    { "EWPSW:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "CH", "NUM", "GRP", "DATA",              0}, CFPushSettingsWrite,   0, RF_CMD },    // ＰＵＳＨ 子機の設定値書き込み
    { "EWPSU:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "CH", "NUM", "GRP", "MODE", "RANGE",         0}, CFPushSuction,         0, RF_CMD },    // ＰＵＳＨ データ吸い上げ
    { "EWPSP:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "CH", "NUM", "GRP", "MODE",              0}, CFPushRecErase,        0, RF_CMD },    // ＰＵＳＨ 子機の記録データ消去
    { "EWOLR:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "CH", "NUM", "GRP", "DATA",              0}, CFPushItemListRead,    0, RF_CMD },    // ＰＵＳＨ 子機の品目リスト読み込み
    { "EWOLW:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "CH", "NUM", "GRP", "DATA",              0}, CFPushItemListWrite,   0, RF_CMD },    // ＰＵＳＨ 子機の品目リスト書き込み
    { "EWPLR:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "CH", "NUM", "GRP", "DATA",              0}, CFPushPersonListRead,  0, RF_CMD },    // ＰＵＳＨ 作業者リスト読み込み
    { "EWPLW:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "CH", "NUM", "GRP", "DATA",              0}, CFPushPersonListWrite, 0, RF_CMD },    // ＰＵＳＨ 作業者リスト書き込み
    { "EWPWR:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "CH", "NUM", "GRP", "DATA",              0}, CFPushWorkgrpRead,     0, RF_CMD },    // ＰＵＳＨ ワークグループ読み込み
    { "EWPWW:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "CH", "NUM", "GRP", "DATA",              0}, CFPushWorkgrpWrite,    0, RF_CMD },    // ＰＵＳＨ ワークグループ書き込み
    { "EWPMG:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "CH", "NUM", "GRP", "DATA",              0}, CFPushMessage,         0, RF_CMD },    // ＰＵＳＨ メッセージ表示             ***
    { "EWPRQ:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "CH", "NUM", "GRP", "DATA",              0}, CFPushRemoteMeasure,   0, RF_CMD },    // ＰＵＳＨ リモート測定指示
    { "EWPTM:", {"ACT", "ID", "RELAY", "ROUTE", "BAND", "CH", "NUM", "GRP", "DATA",              0}, CFPushClock,           0, RF_CMD },    // ＰＵＳＨ 時計設定                    ***


    //----- 検査コマンド -----
    { "KENSA:", { "CMD","DATA","P1","P2","P3","P4","P5","P6","P7","P8","P9",                    0 },    CFKensa ,  0, 0 },          // 検査用特殊コマンド

    { "INSPC:", { "CMD","DATA","P1","P2","P3","P4","P5","P6","P7","P8","P9",                    0 },    CFInspect , 0, 0 },         // 検査用特殊コマンド

    { "DPRNT:", {                                                                               0 },    CFInspect , 0, 0 },         // 特殊コマンド

    { "DRSYS:", {                                                                               0 },    CFReadSystemInfo , 0, 0 },          // 特殊コマンド

    { "DRSFM:", { "OFFSET",                                                                     0 },    CFReadSFlash , 0, 0 },          // 特殊コマンド

    //BLE サーバー、クライアント切り替えコマンド
    { "EBROL:",  {"MODE",                                                                       0}, CFRoleChange, 0, 0 },         // BLEクライアント、サーバー切り替え


    //----- 無線モジュール ダイレクトコマンド -----
    { "R500C:", { "MODE","DATA","TIME",                                                         0 },    CFR500Special, 0,  RF_CMD },    // 無線モジュール ダイレクトコマンド


    //----- ファームウェアキャッシュコマンド -----
    { "EFIRM:",  {"DATA","PW",                                                                  0 }, CFWriteFirmData, 0, 0 },         // ファームウェアキャッシュ t.saito // sakaguchi PW追加


    {0, {0 },NULL, 0, 0}      // 終了コード

};
#endif

//***************************************************
//     コマンドの実体
//***************************************************
/**
 * [EINIT] 本体の初期化
 * @return
 */
uint32_t CFInitilize(void)
{
    char    *mode;
    int     size;
    uint32_t    Err;
    uint32_t actual_events = 0;

    Printf("[EINIT]\r\n");
    Err = ERR( CMD, FORMAT );
    size = ParamAdrs( "MODE", &mode ) ;
    if ( size ) {
        if( (size == 7) && !strncmp( "FACTORY", mode, 7 ) ) {

            //RF_power_on(0);           // 2010.07.06関数仕様変更により引数追加

            Printf("Factory default\r\n");
// 2023.05.31 ↓
//            if(fact_config.Vender == VENDER_HIT){
//                init_factory_default(1, 1);
//            }
//            else{
//                init_factory_default(0, 1);
//            }
            init_factory_default(fact_config.Vender,1);
// 2023.05.31 ↑
            init_regist_file_default();                // 子機登録ファイル初期化
            init_group_file_default();                  // グループファイル初期化

            Err = ERR( CMD, NOERROR );
        }
        else if( (size == 7) && !strncmp( "RESTART", mode, 7 ) ) {

            Printf("Restart\r\n");
// 2023.05.26 ↓
#if CONFIG_NEW_STATE_CTRL
#else
            RF_power_on_init(1);      // 2021.03.10 暫定
#endif
// 2023.05.26 ↑
            // reboot処理
            PutLog( LOG_CMD, "Restart by command" );
//TODO            tx_event_flags_get (&event_flags_reboot, 0xffffffff,  TX_OR_CLEAR, &actual_events, TX_NO_WAIT);
//TODO            tx_event_flags_set (&event_flags_reboot, FLG_REBOOT_REQUEST, TX_OR);        //リブートリクエストイベントシグナル セット
            Err = ERR( CMD, NOERROR );
        }
        else if( (size == 7) && !strncmp( "REGFILE", mode, 7 ) ) {

            Printf("RegFile Clear\r\n");
            init_regist_file_default();                // 子機登録ファイル初期化
            Err = ERR( CMD, NOERROR );
        }
        else if( (size == 8) && !strncmp( "BLERESET", mode, 8 ) ) { //@ToDo
            Printf("BLE Reset \r\n");
            if(my_config.ble.active != 0){
                //init_regist_file_default();                // BLE Reset
//TODO                tx_event_flags_set( &g_ble_event_flags, FLG_PSOC_RESET_REQUEST, TX_OR); //PSoCリセット要求    2020.Jun.30追加
//TODO                tx_thread_resume(&ble_thread);
            }                                          //BLEスレッド起動    2020.Jun.30追加
            Err = ERR( CMD, NOERROR );
        }
        else if( (size == 7) && !strncmp( "WARNING", mode, 7 ) ){   // 2021.03.09  warning buffer clear
            Printf("Warning buffer clear\r\n");
            RF_power_on_init(1);
            Err = ERR( CMD, NOERROR );
        }
    }

    return (Err);
}





/**
 * [RUINF] ユニット情報の読み込み
 * @return
 @note  LO_HIマクロ排除
 */
uint32_t CFReadInfo(void)
{
 //   uint16_t sp;      //2019.Dec.26 コメントアウト
    char str[5];
 //   LO_HI a;          //2019.Dec.26 コメントアウト


    // 親機名称
    StatusPrintf_S(sizeof(my_config.device.Name),  "NAME", "%s", my_config.device.Name);
    // 本体シリアル番号
    //fact_config.SerialNumber = 0x5f58ffff;
    StatusPrintf("SER", "%.8lX", fact_config.SerialNumber);
     // 本機の説明
    StatusPrintf_S(sizeof(my_config.device.Description),  "DESC", "%s", my_config.device.Description);
    // 本体ファームウェアバージョン
    //StatusPrintf( "FWV", "%s %s", VERSION_FW, __DATE__ );  //debug
    StatusPrintf( "FWV", "%s", VERSION_FW);     // release
    // 無線モジュール ファームウェアバージョン
    StatusPrintf( "RFV", "%.4X", regf_rfm_version_number & 0x0000ffff);

    // debug
    //StatusPrintf( "NETV", "%s", VERSION_FW);

    // 温度単位
    StatusPrintf_v2s( "UNITS", my_config.device.TempUnits, sizeof(my_config.device.TempUnits), "%lu");

    // 販売者
//    StatusPrintf("VENDER", "%d", fact_config.Vender);
    /*
    memcpy((char *)&(StsArea[CmdStatusSize]), "NAME=00", 7);
    sp = CmdStatusSize + 5;                                     // データ数書き込み位置ラッチ
    CmdStatusSize += 7;



    memcpy(&StsArea[CmdStatusSize], my_config.device.Name, 26);
    StsArea[CmdStatusSize + 26] = 0x00;                         // 最大２６バイトとするため、２６バイト目をＮＵＬＬにする

    word_data_a = strlen((char *)&StsArea[CmdStatusSize]);           // ＮＵＬＬコードまでの長さ
    if(word_data_a > 26) word_data_a = 26;
    StsArea[sp] = (uint8_t)a.byte_lo;
    StsArea[sp + 1] = a.byte_hi;
    CmdStatusSize += word_data_a;

    // 本機の説明
    memcpy(&StsArea[CmdStatusSize], "DESC=00", 7);
    sp = CmdStatusSize + 5;                                     // データ数書き込み位置ラッチ
    CmdStatusSize += 7;

    memcpy(&StsArea[CmdStatusSize], my_config.device.Description, 64);
    StsArea[CmdStatusSize + 64] = 0x00;                         // 最大６４バイトとするため、６４バイト目をＮＵＬＬにする

    word_data_a = strlen((char *)&StsArea[CmdStatusSize]);           // ＮＵＬＬコードまでの長さ
    if(word_data_a > 64) word_data_a = 64;
    StsArea[sp] = a.byte_lo;
    StsArea[sp + 1] = a.byte_hi;
    CmdStatusSize += word_data_a;
    */

    // シリアル通信速度
    //StatusPrintf("BAUD", "%u", my_settings.scom_baudrate);

    // パスコード
    //StatusPrintfB("PASC", (char *)&my_config.ble.passcode, 4);

    // ＢＬＥデバイスアドレス
    //StatusPrintfB("DADR", (char *)&psoc.device_address, 8);

    // 登録コード
    //StatusPrintfB("REGC", (char *)&my_config.device.registration_code, 4);



    switch(fact_config.SerialNumber & 0xf0000000){
        case 0x30000000:    // US
            StatusPrintf( "WCER", "%s", "1");
            break;
        case 0x40000000:    // EU
            StatusPrintf( "WCER", "%s", "2");
            break;
        case 0x50000000:    // 日本
            StatusPrintf( "WCER", "%s", "0");
            break;
        case 0xE0000000:    // 日本(ESPEC)
            StatusPrintf( "WCER", "%s", "0");
            break;
        default:            //
            StatusPrintf( "WCER", "%s", "0");
            break;

    }



    // 無線モジュール シリアル番号
    StatusPrintf("RFS", "%.8lX", regf_rfm_serial_number);



    // ＢＬＥ ファームウェアバージョン
    //taskENTER_CRITICAL();
    memcpy(str, psoc.revision, 4);
    str[4] = 0x00;
    //taskEXIT_CRITICAL();
    StatusPrintf( "BLV", "%s", &str);
    //rfm_reset();


    return(ERR(CMD, NOERROR));
}



/**
 * [WUINF] ユニット情報の書き込み
 * @return
 */
uint32_t CFWriteInfo(void)
{
    int Value;
    uint16_t max;
    int Size;
    char *name, *desc, chg = 0x00, *bin, *req;
    uint8_t err = 0x00;

    Size = ParamAdrs("NAME", &name);                            // 機器名称（親機名称）
    max = sizeof(my_config.device.Name);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }
        else if((Memcmp(my_config.device.Name, name, (int)strlen(my_config.device.Name), Size) != 0))
        {
            chg |= 0x01;
            memset(my_config.device.Name, 0x00, sizeof(my_config.device.Name));
            memcpy(my_config.device.Name, name, (size_t)Size);
        }
    }

    Size = ParamAdrs("DESC", &desc);                            // 本機の説明
    max = sizeof(my_config.device.Description);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }
        else if((Memcmp(my_config.device.Description, desc, (int)strlen(my_config.device.Description) ,Size) != 0))
        {
            chg |= 0x02;
            memset(my_config.device.Description, 0x00, sizeof(my_config.device.Description));
            memcpy(my_config.device.Description, desc, (size_t)Size);
        }
    }


//----↓↓↓↓↓-------------------------------------------------------------------------------------------------
    // UNITSの保存していなかったので対応した    2019/11/27 segi
    Value = ParamInt("UNITS");
    if((Value == 0) || (Value == 1))
    {
        if(my_config.device.TempUnits[0] != (char)Value)
        {
            chg = 0x04;
            my_config.device.TempUnits[0] = (char)Value;
        }
    }
    else if(Value == INT32_MAX){
        ;//err |= CMD_NOP;

    }
//sakaguchi add UT-0025 ↓
    else{
        err |= CMD_PER;
    }
//sakaguchi add UT-0025 ↑
//-----↑↑↑↑↑------------------------------------------------------------------------------------------------


    if(ParamAdrs("PASC", &bin) != -1)                           // ＢＬＥパスコード
    {
        if(my_config.ble.passcode != *(uint32_t *)bin)        // 今までのＢＬＥパスコードと違う
        {
            chg |= 0x10;
            my_config.ble.passcode = *(uint32_t *)bin;
        }

        if(*(uint32_t *)bin == 0)
        {
            if(ble_passcode_lock != false){     ///< if不要  2020.Feb.04
                ble_passcode_lock = false;   // 解錠
            }

            if(my_config.ble.utilization != 0)
            {
                chg |= 0x10;
                my_config.ble.utilization = 0;                // ＢＬＥ パスコード利用 無効
            }
        }
        else
        {
            if(ble_passcode_lock != true){    ///< if不要  2020.Feb.04
                ble_passcode_lock = true; // 施錠
            }

            if(my_config.ble.utilization != 1)
            {
                chg |= 0x10;
                my_config.ble.utilization = 1;                // ＢＬＥ パスコード利用 有効
            }
        }
    }
    else{
        ;//err |= CMD_NOP;
    }

    if(ParamAdrs("REGC", &bin) != -1)                           // 登録コード
    {
        if(memcmp(&my_config.device.registration_code, bin, 4) != 0)
        {
            chg |= 0x20;
            my_config.device.registration_code = *(uint32_t *)bin;
        }
    }
    else{
        ;//err |= CMD_NOP;
    }

    if((chg != 0x00) && (err == 0x00))                                             // 内容の変更があったら、データを書き込む
    {
        my_config.device.SysCnt++;                              //親機設定変更カウンタ更新 sakaguchi UT-0027

        rewrite_settings();     //本体設定書き換え（関数内でmutex_sfmemを取得返却している)
        read_my_settings();     //本体設定をＳＲＡＭへ読み込み

        //vhttp_sysset_sndon();                                   //親機設定送信 sakaguchi cg UT-0026
        if(HTTP_SEND == Http_Use) vhttp_sysset_sndon();         // 親機設定送信 // sakaguchi 2020.12.24
    }
    else{
        read_my_settings();     //本体設定をＳＲＡＭへ読み込み
    }

    Printf("err = %02X\r\n", err);

    Size = ParamAdrs("REQ", &req);
    if(ParamAdrs("REQ", &bin) != -1){
    if(memcmp("RESET", (char *)req, (size_t)Size) == 0){
        post.WUINF = true;  // ＢＬＥリセット
    }
    else{
        post.WUINF = false;
            err |= CMD_PER;                                         //sakaguchi add UT-0025
        }
    }

    Printf("err = %02X\r\n", err);

    return CMD_RES(err);
}




/**
 * [RSETF] 設定（登録）ファイル読み込み
 * @return
 */
uint32_t CFReadSettingsFile(void)
{

    int Size;
    int Offset;
    uint32_t adr;
    uint16_t Save = 0;                   // sakaguchi 2021.06.15
    uint16_t Save_tmp = 0;               // sakaguchi 2021.06.15


    Printf("CMD:RSETF\r\n");

    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS){
        // LONG_MAX:0x7fffffffL INT_MAX:0x7fffffffL
        Offset = ParamInt32("OFFSET");
        if(Offset < 0 ){
            Offset = 0;                      // 未指定時オフセット０
        }

        Printf("RSETF offset = %d\r\n", Offset);

        adr = (uint32_t)(SFM_REGIST_START + (uint32_t)Offset);
        if(adr > (SFM_REGIST_END + 1)){
            tx_mutex_put(&mutex_sfmem);
            return(ERR(CMD, FORMAT));   // 範囲内であること
        }

        Size = ParamInt("SIZE");
        //Size = 2048;  // debug
        //Size -= 256;
        //if((Size > 1024) || (Size < 0)){
        if((Size > 4096) || (Size < 0)){            // 1024->4096byteに拡張     // sakaguchi 2021.06.15
            tx_mutex_put(&mutex_sfmem);
            return(ERR(CMD, FORMAT));               // サイズ指定が２０４８越えていたらエラー
        }

        memcpy((char *)&StsArea[CmdStatusSize], "DATA=", 5);
        CmdStatusSize += 5;

        StsArea[CmdStatusSize++] = (uint8_t)Size;
        StsArea[CmdStatusSize++] = (uint8_t)(Size / 256);

        serial_flash_multbyte_read(adr, (uint32_t)Size, (char *)&StsArea[CmdStatusSize]); // 読み込み

        CmdStatusSize += Size;

        Printf("Size=%ld\r\n", CmdStatusSize);

// sakaguchi 2021.06.15 ↓
        // SAVE（登録情報のサイズ）
        memset(work_buffer, 0x00, sizeof(work_buffer));
        serial_flash_multbyte_read(SFM_REGIST_START, SFM_REGIST_SIZE_32, work_buffer); // 読み込み  work_buffer size 32K
        while(1){
            Save_tmp = *(uint16_t *)&work_buffer[Save];
            if((Save_tmp == 0xFFFF) || (Save_tmp == 0x0000)){
                break;
            }
            if((uint16_t)(Save + Save_tmp) > sizeof(work_buffer)){
                break;
            }
            Save = (uint16_t)(Save + Save_tmp);
        }
        StatusPrintf_v2s( "SAVE", (char *)&Save, sizeof(Save), "%d");
        Printf("Save=%d\r\n", Save);
// sakaguchi 2021.06.15 ↑

        tx_mutex_put(&mutex_sfmem);
        return ERR( CMD, NOERROR );
    }


    return ERR( CMD, RUNNING );
}


//※ SAVEパラメータを追加のこと
/**
 * @brief  [WSETF] 設定（登録）ファイル書き込み
 * @return
 * @attention   SAVEパラメータを追加のこと
 */
uint32_t CFWriteSettingsFile(void)
{
    char *data;

    uint32_t Offset = 0, Size;
    int Save;
    uint32_t adr;

    Save = ParamInt32("SAVE");

    Printf("[WSETF] save=%ld %08X\r\n", Save, Save);

    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS){

        if(Save == 0)                                                               // 保護解除
        {
            serial_flash_block_unlock();                                            // グローバルブロックプロテクション解除

            serial_flash_erase(SFM_TEMP_START, SFM_TEMP_END);
        }
        else if(Save == INT32_MAX){
            Printf("### WSETF  temp \r\n");
            // LONG_MAX:0x7fffffffL INT_MAX:0x7fffffffL
            Offset = (uint32_t)ParamInt32("OFFSET");
            if(Offset == INT32_MAX){
                Offset = 0;                                                         // 未指定時オフセット０
            }

            Printf("WSETF offset = %d\r\n", Offset);
            adr = (uint32_t)(SFM_TEMP_START + Offset);
            if(adr > (SFM_TEMP_END + 1)){
                tx_mutex_put(&mutex_sfmem);
                return(ERR(CMD, FORMAT));   // 範囲内であること
            }
            Size = (uint32_t)ParamAdrs("DATA", &data);                              // 設定（登録）データ
            Printf(" Size = %ld\r\n", Size);
//sakaguchi UT-0043 ↓
//            if(Size > 1024){
//                tx_mutex_put(&mutex_sfmem);
//                return(ERR(CMD, FORMAT));                                           // 範囲内であること
//            }
//sakaguchi UT-0043 ↑
            if((adr + Size) > (SFM_TEMP_END + 1)){
                tx_mutex_put(&mutex_sfmem);
                return(ERR(CMD, FORMAT));                                           // 範囲内であること
            }
            serial_flash_block_unlock();                                                // グローバルブロックプロテクション解除
            serial_flash_multbyte_write(adr, (size_t)Size, data);                       // 書き込み

        }
        else{
            if(Save > 0 && Save < 8192*4){
                serial_flash_multbyte_read(SFM_TEMP_START, (size_t)Save, work_buffer); // 読み込み  work_buffer size 32K

                serial_flash_block_unlock();                        // グローバルブロックプロテクション解除

                serial_flash_erase(SFM_REGIST_START, SFM_REGIST_END);
                serial_flash_multbyte_write(SFM_REGIST_START, (size_t)Save, work_buffer);
                serial_flash_erase(SFM_REGIST_START_B, SFM_REGIST_END_B);
                serial_flash_multbyte_write(SFM_REGIST_START_B, (size_t)Save, work_buffer);

                Printf("### WSETF \r\n");
                auto_control.crc = 0;                               // AT_start();で警報カウンタをクリアさせる

// sakaguchi UT-0043 ↓
//                if( VENDER_HIT != fact_config.Vender){                // Hitachi以外
//                    G_HttpFile[FILE_C].sndflg = SND_ON;                     // [FILE]機器設定：送信有りをセット
//                    G_HttpCmd[HTTP_CMD_RSETF].sndflg = SND_ON;              // [CMD] 機器設定(RSETF)：送信有りをセット
//                    G_HttpCmd[HTTP_CMD_RSETF].save = (uint32_t)Save;        // [CMD] 機器設定(RSETF)：保護サイズをセット
//                    G_HttpCmd[HTTP_CMD_RSETF].offset = Offset;              // [CMD] 機器設定(RSETF)：オフセットをセット
//                }

// sakaguchi 2021.07.20 del ↓
                //my_config.device.RegCnt++;                                  // 登録情報変更カウンタ更新 sakaguchi UT-0027
                //RF_full_moni_init();                                      // モニタリング用テーブルの初期化       // sakaguchi 2020.08.31
                //if(WaitRequest != WAIT_REQUEST){
                //    Printf("WSETF AT Start (FLG_EVENT_INITIAL)\r\n");
                //    tx_event_flags_set (&g_wmd_event_flags, FLG_EVENT_INITIAL, TX_OR);        // AT Start起動（登録情報変更時）
                //    mate_at_start = 0;
                //}else{
                //    mate_at_start = 1;
                //}
                //PutLog(LOG_CMD,"Unit Registration file Update");

                //if((VENDER_HIT != fact_config.Vender) && (Http_Use == HTTP_SEND)){  // Hitachi以外    // sakaguchi 2020.08.31  haya 2020.10.16
                //    G_HttpFile[FILE_C].sndflg = SND_ON;                             // [FILE]機器設定：送信有りをセット
                //    G_HttpCmd[HTTP_CMD_RSETF].sndflg = SND_ON;                      // [CMD] 機器設定(RSETF)：送信有りをセット
                //}
// sakaguchi 2021.07.20 del ↑
// sakaguchi UT-0043 ↑
            }

        }

        tx_mutex_put(&mutex_sfmem);

// sakaguchi 2021.07.20 ↓
        if(Save > 0 && Save < 8192*4){

            my_config.device.RegCnt++;      // 登録情報変更カウンタ更新

            rewrite_settings();             // 本体設定書き換え（関数内でmutex_sfmemを取得返却している)
            read_my_settings();             // 本体設定をＳＲＡＭへ読み込み

            if((VENDER_HIT != fact_config.Vender) && (Http_Use == HTTP_SEND)){      // Hitachi以外
                G_HttpFile[FILE_C].sndflg = SND_ON;                                 // [FILE]機器設定：送信有りをセット
                G_HttpCmd[HTTP_CMD_RSETF].sndflg = SND_ON;                          // [CMD] 機器設定(RSETF)：送信有りをセット
            }

            RF_full_moni_init();            // モニタリング用テーブルの初期化
            if(WaitRequest != WAIT_REQUEST){
                Printf("WSETF AT Start (FLG_EVENT_INITIAL)\r\n");
                tx_event_flags_set (&g_wmd_event_flags, FLG_EVENT_INITIAL, TX_OR);  // AT Start起動（登録情報変更時）
                mate_at_start = 0;
            }else{
                mate_at_start = 1;
            }
            PutLog(LOG_CMD,"Unit Registration file Update");
        }
// sakaguchi 2021.07.20 ↑
        return ERR( CMD, NOERROR );
    }


    return ERR( CMD, NOERROR );
}


/**
 * @brief  [WREGI] 設定（登録）ファイルに含まれる子機設定の書き込み
 * @return
 * @attention
 */
uint32_t CFWriteUnitSettingsFile(void)
{

    char    *data;
    int32_t sz_data;
    uint32_t serial;                        // 子機シリアル番号
    uint8_t   setinfo;                        // 子機設定
    uint8_t   keiho;                          // 警報監視設定
    uint8_t   kiroku;                         // 記録データ送信設定
    int i;
    LO_HI   size;

    typedef struct{
        uint8_t bit0 : 1;
        uint8_t bit1 : 1;
        uint8_t bit2 : 1;
        uint8_t bit3 : 1;
        uint8_t bit4 : 1;
        uint8_t bit5 : 1;
        uint8_t bit6 : 1;
        uint8_t bit7 : 1;
    }Bit_t;

    typedef union{
        Bit_t b;
        uint8_t byte;
    }Setinfo_t;

    Setinfo_t   unitinfo;
    Setinfo_t   unitinfo_back;              // 2023.05.26
    uint8_t       wflg = 0;                   // フラッシュ書込フラグ
    uint32_t    Err;



    Err = ERR( CMD, NOERROR );

    sz_data = ParamAdrs("DATA",&data);

    if(( sz_data == -1 ) || ( sz_data == 0 ) ){

        Err = ERR( CMD, FORMAT );           // フォーマットエラー
        return(Err);                        // 処理終了
    }


    // 設定（登録）ファイルの読込
    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS){

//        serial_flash_multbyte_read(SFM_TEMP_START, SFM_REGIST_SIZE_32, work_buffer); // 読み込み  work_buffer size 32K
        serial_flash_multbyte_read(SFM_REGIST_START, SFM_REGIST_SIZE_32, work_buffer); // 読み込み  work_buffer size 32K

        Printf("FLASH_Read\r\n");

        tx_mutex_put(&mutex_sfmem);
    }
    else{
        Err = ERR( CMD, RUNNING );           // 処理中エラー
        return(Err);                         // 処理終了
    }


    // 子機のシリアル番号取得
    for(i=0; i<(sz_data/5); i++){

        memcpy(&serial, data+5*i, 4);       // シリアル番号
        memcpy(&setinfo, data+5*i+4, 1);    // 子機設定情報

        if(0xff != get_regist_group_adr_ser(serial)){           // 開始アドレス(EWSTW_Adr)、対象子機の設定ファイル情報(Regist_Data)をセット

            keiho = setinfo & 0x03;       // 警報監視設定
            kiroku = setinfo & 0x0C;      // 記録データ送信

            if(( 0 == keiho ) && ( 0 == kiroku ))   // 警報監視も記録データ送信も変更無し
            {
                continue;           // 次の子機へ
            }

            unitinfo.byte = Regist_Data[5];         // 子機設定情報取得
            unitinfo_back.byte = unitinfo.byte;          // 子機設定情報バックアップ // 2023.05.26

            // 警報監視=有り
            if( 0x01 == keiho ){
                unitinfo.b.bit0 = 1;
            }

            // 警報監視=無し
            if( 0x02 == keiho ){
                unitinfo.b.bit0 = 0;
            }

            // 記録データ送信=有り
            if( 0x04 == kiroku ){

                unitinfo.b.bit2 = 1;
                if((0xFA == Regist_Data[2]) || (0xF9 == Regist_Data[2])){         // RTR-574,RTR-576の場合
                    unitinfo.b.bit3 = 1;
                }
            }

            // 記録データ送信=無し
            if( 0x08 == kiroku ){

                unitinfo.b.bit2 = 0;
                if((0xFA == Regist_Data[2]) || (0xF9 == Regist_Data[2])){         // RTR-574,RTR-576の場合
                    unitinfo.b.bit3 = 0;
                }
            }

            Regist_Data[5] = unitinfo.byte;      // 子機設定情報更新
            size.byte_lo = Regist_Data[0];
            size.byte_hi = Regist_Data[1];
            memcpy(&work_buffer[EWSTW_Adr], &Regist_Data[0], size.word);
            wflg = 1;                           // 書込フラグON

// 2023.05.26 ↓
            if(kiroku){                         // 記録データ送信有無に変更がある場合
                if(unitinfo_back.byte != unitinfo.byte){    // 子機設定に変更がある場合
                    kk_delta_set(serial, 0x01);     // 対象子機の記録データを次回全データ吸い上げとする
                }
            }
// 2023.05.26 ↑
        }
        // 登録されていない子機がある場合
        else{
            Err = ERR( CMD, FORMAT );           // フォーマットエラー
            return(Err);                        // 書込みせずここで処理終了
        }
    }

    if(1 == wflg){
        // 設定（登録）ファイルの書込
        if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS){

            // 書き込み
            serial_flash_block_unlock();                        // グローバルブロックプロテクション解除

            serial_flash_erase(SFM_REGIST_START, SFM_REGIST_END);

            serial_flash_multbyte_write(SFM_REGIST_START, SFM_REGIST_SIZE_32, work_buffer);                 // 32kbyte書き込み

            serial_flash_erase(SFM_REGIST_START_B, SFM_REGIST_END_B);

            serial_flash_multbyte_write(SFM_REGIST_START_B, SFM_REGIST_SIZE_32, work_buffer);               // 32kbyte書き込み

            tx_mutex_put(&mutex_sfmem);
        }
        else{
            Err = ERR( CMD, RUNNING );           // 処理中エラー
        }

        if((VENDER_HIT != fact_config.Vender) && (Http_Use == HTTP_SEND)){          // Hitachi以外  2020.10.16
            G_HttpFile[FILE_C].sndflg = SND_ON;                                     // [FILE]機器設定：送信有りをセット
            G_HttpCmd[HTTP_CMD_RSETF].sndflg = SND_ON;                              // [CMD] 機器設定(RSETF)：送信有りをセット
        }
        my_config.device.RegCnt++;

        PutLog(LOG_CMD,"Uint Registration File Update");

        if(WaitRequest != WAIT_REQUEST){
            Printf("WSETF AT Start (FLG_EVENT_INITIAL) 2\r\n");
            tx_event_flags_set (&g_wmd_event_flags, FLG_EVENT_INITIAL, TX_OR);      // AT Start起動（登録情報変更時）
            mate_at_start = 0;
        }else{
            mate_at_start = 1;
        }
    }

    return(Err);

}


/**
 * [WRTCE] HTTPSルート証明書の書き込み
 * @return
 * @note    LO_HIマクロ廃止
 */
uint32_t CFWriteCertFile(void)
{
    int a_size, d_size,Save,Size;
    int Offset;
    char *data;
    char *area;
    char *dst;
    int windex, warea;
   uint32_t Err = ERR( CMD, FORMAT );

    int i;
 //   LO_HI a;

    Printf("[WRTCE] \r\n");

    Save = ParamInt32("SAVE");
    a_size = ParamAdrs( "AREA", &area );
    Size = ParamInt32("SIZE");
    Offset = ParamInt32("OFFSET");


    Printf("WRTCE Area:%ld Save:%ld  Offset:%ld Size:%ld \r\n", a_size,Save,Offset,Size);

    if(Save == 0)                   // サイズ指定
    {
        CertFileSize_Temp =  (uint16_t)0;
        Printf("[WRTCE] 1 size(%d) %d \r\n", Size, a_size);
//        if(0 < Size && Size <= 2048){
        if(0 < Size && Size <= 4094){                 // 4KB確保されているが2byteはサイズ領域で使用する
            if(a_size == 4 || a_size == 5){
                if(a_size == 4){
                    Err = ERR( CMD, NOERROR );
                    if(!strncmp( "WSS1", area, 4)){
                        //my_config.websrv.CertSizeW[0] = (uint16_t)Size;
                        CertFileSize_Temp =  (uint16_t)Size;
                    }
                    else if(!strncmp( "WSS2", area, 4)){
                        //my_config.websrv.CertSizeW[1] = (uint16_t)Size;
                        CertFileSize_Temp =  (uint16_t)Size;
                    }
                    else if(!strncmp( "WSS3", area, 4)){
                        //my_config.websrv.CertSizeW[2] = (uint16_t)Size;
                        CertFileSize_Temp =  (uint16_t)Size;
                    }
                    else if(!strncmp( "WSS4", area, 4)){
                        //my_config.websrv.CertSizeW[3] = (uint16_t)Size;
                        CertFileSize_Temp =  (uint16_t)Size;
                    }
                    else if(!strncmp( "WSS5", area, 4)){
                        //my_config.websrv.CertSizeW[4] = (uint16_t)Size;
                        CertFileSize_Temp =  (uint16_t)Size;
                    }
                    else if(!strncmp( "WSS6", area, 4)){
                        //my_config.websrv.CertSizeW[5] = (uint16_t)Size;
                        CertFileSize_Temp =  (uint16_t)Size;
                    }
                    else{
                        Err = ERR( CMD, FORMAT );
                    }
                }
                else{
                    Err = ERR( CMD, NOERROR );
                    if(!strncmp( "USER1", area, 5)){
                        //my_config.websrv.CertSizeU[0] = (uint16_t)Size;
                        CertFileSize_Temp =  (uint16_t)Size;
                    }
                    else if(!strncmp( "USER2", area, 5)){
                        //my_config.websrv.CertSizeU[1] = (uint16_t)Size;
                        CertFileSize_Temp =  (uint16_t)Size;
                    }
                    else{
                        Err = ERR( CMD, FORMAT );
                    }
                }

                return (Err);

            }
            else{
                return (Err);
            }

            }
            else{
            Printf(" Size Error \r\n");
            return ERR( CMD, FORMAT );
        }
    }
//    else if(0 < Save && Save <= 2048){
    else if(0 < Save && Save <= 4094){                          // 4KB確保されているが2byteはサイズ領域で使用する
        Printf("[WRTCE] 2 size(%d)\r\n", CertFileSize_Temp);

        if(a_size == 4 || a_size == 5){
            if(a_size == 4){
                warea = 0;
                if(!strncmp( "WSS1", area, 4)){
                    windex = 0;
                }
                else if(!strncmp( "WSS2", area, 4)){
                    windex = 1;
                }
                else if(!strncmp( "WSS3", area, 4)){
                    windex = 2;
                }
                else if(!strncmp( "WSS4", area, 4)){
                    windex = 3;
                }
                else if(!strncmp( "WSS5", area, 4)){
                    windex = 4;
                }
                else if(!strncmp( "WSS6", area, 4)){
                    windex = 5;
                }
                else{
                    return ERR( CMD, FORMAT );
                }
            }
            else
            {
                warea = 1;
                if(!strncmp( "USER1", area, 5)){
                    windex = 0;
                }
                else if(!strncmp( "USER2", area, 5)){
                    windex = 1;
                }
                else{
                    return ERR( CMD, FORMAT );
                }
            }

            if( ceat_file_write(warea, windex, CertFileSize_Temp) == 0){
                PutLog(LOG_CMD, "der file write success %d-%d", warea, windex);
                rewrite_settings();     //本体設定書き換え（関数内でmutex_sfmemを取得返却している)// sizeを書き換え
                read_my_settings();
                return ERR( CMD, NOERROR );
            }
            else{
                PutLog(LOG_CMD, "der file write error %d", a_size);
                read_my_settings();         // sizeを戻す SFM Busy なのに効果ある？？？？
                return ERR( CMD, RUNNING );
            }
        }
        else
        {
            return ERR( CMD, FORMAT );
        }



    }
    else{
        // ここはRAMアクセスのみ
        Printf("[WRTCE] 3 \r\n");
//        if(0 <= Offset  && Offset <= 2048){
        if(0 <= Offset  && Offset < 4094){
            d_size = (uint16_t)ParamAdrs("DATA", &data);                        // 設定（登録）データ
            Printf(" Size = %ld\r\n", d_size);
//            if(d_size + Offset > 4096){
            if(d_size + Offset > 4094){
                Printf(" Size Error Total Size %ld\r\n", d_size + Offset );
                Err = ERR( CMD, FORMAT );
            }
            else{
                if(a_size == 4 && !strncmp( "WSS", area, 3)){
                    dst = &CertFile_WS[2] + Offset;
                for(i=0;i<d_size;i++){
                    *dst++ = *data++;
                }
                    Err = ERR( CMD, NOERROR );
            }
                else if(a_size == 5 && !strncmp( "USER", area, 4)){
                    dst = &CertFile_USER[2] + Offset;
                for(i=0;i<d_size;i++){
                    *dst++ = *data++;
                }
                    Err = ERR( CMD, NOERROR );
            }
                else{
                    Err = ERR( CMD, FORMAT );
                }
            }
        }
        else{
            Printf(" Offset Error \r\n");
            Err = ERR( CMD, FORMAT );
        }
    }

    return (Err);   //ERR( CMD, NOERROR );     //return ERR( CMD, RUNNING );

}


/**
 * [RRTCE] ルート証明書の読み込み
 * @return
 */
uint32_t CFReadCertFile(void)
{
    uint32_t Err =  ERR( CMD, RUNNING );
    char *area;
    int32_t Size;
    int32_t a_size;
    int Offset;
 //   uint16_t word_data;
    uint32_t adr, adr_end,adr_start;
    int rindex;
//    int warea;

    Printf("[RRTCE] \r\n");

    a_size = ParamAdrs( "AREA", &area );
    //Size = ParamInt("SIZE");

    //if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS){

     Offset = ParamInt32("OFFSET");
    if(Offset == INT32_MAX){
        Offset = 0;                      // 未指定時オフセット０
    }
    if(Offset<2048){

        //if((Size > 1024) || (Size < 0)){
        //    //tx_mutex_put(&mutex_sfmem);
        //    return ERR(CMD, FORMAT);
        //}

        Printf("RRTCE Area= %d Offset= %d \r\n", a_size, Offset);

        memcpy((char *)&StsArea[CmdStatusSize], "DATA=", 5);
        CmdStatusSize += 5;

        if(a_size == 4 || a_size == 5 || a_size == -1) {
            if(a_size == 4){
  //              warea = 0;
                if(!strncmp( "WSS1", area, 4)){
                    rindex = 0;
                }
                else if(!strncmp( "WSS2", area, 4)){
                    rindex = 1;
                }
                else if(!strncmp( "WSS3", area, 4)){
                    rindex = 2;
                }
                else if(!strncmp( "WSS4", area, 4)){
                    rindex = 3;
                }
                else if(!strncmp( "WSS5", area, 4)){
                    rindex = 4;
                }
                else if(!strncmp( "WSS6", area, 4)){
                    rindex = 5;
                }
                else{
                    Size = 0;
                    StsArea[CmdStatusSize++] = (uint8_t)Size;
                    StsArea[CmdStatusSize++] = (uint8_t)(Size / 256);
                    return ERR( CMD, FORMAT );
                }

                adr       = (uint32_t)(SFM_CERT_W1_START + (uint32_t)(4096 * rindex));
                adr_start = (uint32_t)(SFM_CERT_W1_START + (uint32_t)(4096 * rindex + Offset));
                adr_end   = (uint32_t)(SFM_CERT_W1_START + (uint32_t)(4096 * rindex + 1));
                //Size = my_config.websrv.CertSizeW[rindex];
            }
            else
            {
  //              warea = 1;
                if((!strncmp( "USER1", area, 5)) || (a_size == -1)){
                    rindex = 0;
                }
                else if(!strncmp( "USER2", area, 5)){
                    rindex = 1;
                }
                else{
                    Size = 0;
                    StsArea[CmdStatusSize++] = (uint8_t)Size;
                    StsArea[CmdStatusSize++] = (uint8_t)(Size / 256);
                    return ERR( CMD, FORMAT );
                }

                adr       = (uint32_t)(SFM_CERT_U1_START + (uint32_t)(4096 * rindex));
                adr_start = (uint32_t)(SFM_CERT_U1_START + (uint32_t)(4096 * rindex + Offset));
                adr_end   = (uint32_t)(SFM_CERT_U1_START + (uint32_t)(4096 * rindex + 1));
                //Size = my_config.websrv.CertSizeU[rindex];

            }

            if(adr_start > (adr_end + 1)){      // これ以上無い場合
                Size = 0;
                StsArea[CmdStatusSize++] = (uint8_t)Size;
                StsArea[CmdStatusSize++] = (uint8_t)(Size / 256);
                //tx_mutex_put(&mutex_sfmem);
                //return(ERR(CMD, FORMAT));   // 範囲内であること
                return (ERR( CMD, NOERROR ));
            }

// sakaguchi del UT-0036 ↓
//            if( 0 < Size && Size < 4096){

                if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS){

                    serial_flash_multbyte_read(adr , 4096, work_buffer);                        // 読み込み

                    Size = (uint16_t)work_buffer[0] + (uint16_t)work_buffer[1] * 256;
                    Printf("Size %d\r\n",Size);

                    if((Size == 0xffff) || (Size == 0)){
                        Size = 0;
                    }
                    else{
                      Size = (Size >= Offset) ? (Size - Offset) : 0;
                    }

                    StsArea[CmdStatusSize++] = (uint8_t)Size;
                    StsArea[CmdStatusSize++] = (uint8_t)(Size / 256);

                    if(Size !=0){
                        memcpy((char *)&StsArea[CmdStatusSize], (char *)&work_buffer[2 + Offset], (size_t)Size);
                    }

                CmdStatusSize += Size;
                    tx_mutex_put(&mutex_sfmem);
                }
                else{
                    return ERR( CMD, RUNNING );;
                }
//            }
//            else{
//                Size = 0;
//                StsArea[CmdStatusSize++] = (uint8_t)Size;
//                StsArea[CmdStatusSize++] = (uint8_t)(Size / 256);
//            }
// sakaguchi del UT-0036 ↑

            Err = ERR( CMD, NOERROR );

        }
        else{
            Err = ERR( CMD, FORMAT );
        }
    }
    else{
        Err = ERR( CMD, FORMAT );
        }


    return (Err);       //ERR( CMD, NOERROR );


}



/**
 * @brief   [WUSRD] ユーザ定義情報の書き込み
 * @return
 */
uint32_t CFWriteUserDefine(void)
{


    int Size;
    char *data;


    Size = ParamAdrs("DATA", &data);                            // ユーザ定義情報
    if(Size != -1)
    {
        if(Size > 64){
            Size = 64;
        }

#if 1 // new    // 2021.06.16
        int changed;

        changed = memcmp(my_config.user_area, data, (size_t)Size);
        if(changed == 0) { // same data
            size_t i;
            for(i = (size_t)Size; i < 64 ; i++) {
                if(my_config.user_area[i] != 0) {
                    changed = 1;
                    break;
                }
            }
        }

        Printf("WUSRD Size = %d\r\n", Size);
        Printf("WUSRD changed = %d\r\n", changed);

        if(changed != 0)
#else
        if(((Size < 64) && (my_config.user_area[Size] != 0x00)) || (Memcmp(my_config.user_area, data, (int)strlenABS(my_config.user_area) ,Size) != 0))
#endif
        {
            memset(my_config.user_area, 0x00, sizeof(my_config.user_area));
            memcpy(my_config.user_area, data, (size_t)Size);
            rewrite_settings();     //本体設定書き換え（関数内でmutex_sfmemを取得返却している)
            read_my_settings();     //本体設定をＳＲＡＭへ読み込み
        }
    }
    else{
        return ERR(CMD,FORMAT);
    }

    return(ERR(CMD, NOERROR));
}


/**
 * @brief   [RUSRD] ユーザ定義情報の読み込み
 * @return
 * @note    LO_HIマクロ廃止
 */
uint32_t CFReadUserDefine(void)
{
#if 1 // NEW return 64bytes of binary data  // 2021.06.16
    int sp;
    uint16_t word_data;

    memcpy(&StsArea[CmdStatusSize], "DATA=00", 7);
    sp = CmdStatusSize + 5;                                     // データ数書き込み位置ラッチ
    CmdStatusSize += 7;

    memcpy(&StsArea[CmdStatusSize], my_config.user_area, 64);
    word_data = 64;
    *(uint16_t *)&StsArea[sp] = word_data;
    CmdStatusSize += word_data;

    return(ERR(CMD, NOERROR));
#else
    int sp;
//    LO_HI a;
    uint16_t word_data;

    memcpy(&StsArea[CmdStatusSize], "DATA=00", 7);
    sp = CmdStatusSize + 5;                                     // データ数書き込み位置ラッチ
    CmdStatusSize += 7;

    memcpy(&StsArea[CmdStatusSize], my_config.user_area, 64);
    StsArea[CmdStatusSize + 64] = 0x00;                         // 最大６４バイトとするため、６４バイト目をＮＵＬＬにする

    word_data = (uint16_t)strlen((char *)&StsArea[CmdStatusSize]);           // ＮＵＬＬコードまでの長さ
    if(word_data > 64){
        word_data = 64;
    }
    *(uint16_t *)&StsArea[sp] = word_data;
    CmdStatusSize += word_data;

    return(ERR(CMD, NOERROR));
#endif
}


/**
 * @brief   [WVGRP] 表示グループ情報の書き込み
 * @return
 */
uint32_t CFWriteVGroup(void)
{
    int Size,i;
    char *data;
    char *dst;


    Size = ParamAdrs("DATA", &data);                            // ユーザ定義情報
    if(Size != -1)
    {
        if((Size>=0) && (Size<=1000)){
            memset(work_buffer, 0xff, 1024);
            work_buffer[0] = (uint8_t)Size;
            work_buffer[1] = (uint8_t)(Size/256);

            dst = &work_buffer[2];
            for(i=0;i<Size;i++){
                *dst++ = *data++;
            }


            for(i=0;i<Size+2;i++)
            {
                Printf("%02X ", work_buffer[i]);
            }
            Printf("\r\n");
            if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS){
                serial_flash_block_unlock();                                // グローバルブロックプロテクション解除
                serial_flash_erase(SFM_GROUP_START, SFM_GROUP_END);
                serial_flash_multbyte_write(SFM_GROUP_START, (uint32_t)1024, (char *)&work_buffer[0]);
                tx_mutex_put(&mutex_sfmem);
            }
            else{
                return ERR( CMD, RUNNING );
            }
        }
        else
        {
            return ERR(CMD,FORMAT);
        }

    }
    else{
        return ERR(CMD,FORMAT);
    }

    my_config.device.SysCnt++;                              //親機設定変更カウンタ更新 sakaguchi UT-0027
    rewrite_settings();     //本体設定書き換え（関数内でmutex_sfmemを取得返却している)//sakaguchi UT-0027
    read_my_settings();     //本体設定をＳＲＡＭへ読み込み                                     //sakaguchi UT-0027
    //vhttp_sysset_sndon();                                   //親機設定送信 sakaguchi cg UT-0026
    if(HTTP_SEND == Http_Use) vhttp_sysset_sndon();         // 親機設定送信 // sakaguchi 2020.12.24

    return(ERR(CMD, NOERROR));



}

/**
 * @brief   [RVGRP] 表示グループの読み込み
 * @return
 */
uint32_t CFReadVGroup(void)
{
//    int i;
    int size;

    size = 1024;

    Printf("[RVGRP]\r\n");

    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS){
        memcpy((char *)&StsArea[CmdStatusSize], "DATA=", 5);
        CmdStatusSize += 5;

        serial_flash_multbyte_read(SFM_GROUP_START, (size_t)SFM_GROUP_SIZE, work_buffer); // 読み込み 4k to work_buffer size 32K

        size = work_buffer[0] + work_buffer[1]*256;

// sakaguchi UT-0041 ↓
        if(size == 0xFFFF){
            size = 0;
        }else if(size > 1000){
            size = 1000;
        }
// sakaguchi UT-0041 ↑

        StsArea[CmdStatusSize++] = (uint8_t)size;
        StsArea[CmdStatusSize++] = (uint8_t)(size >> 8);
        memcpy((char *)&StsArea[CmdStatusSize+0], &work_buffer[2], (size_t)size);
        CmdStatusSize += size;
        tx_mutex_put(&mutex_sfmem);

        return(ERR(CMD, NOERROR));
    }

    return ERR( CMD, RUNNING );
}




/**
 * @brief   [EBSTS]本体の状態
 * @return
 */
uint32_t CFBSStatus(void)
{
    //uint16_t val = 1;
    uint16_t phy;
    int swo;
//    uint32_t rutc = 1563269700;
    uint32_t   val;
//TODO    ioport_level_t level;

    phy = (uint16_t)my_config.network.Phy[0];

    swo = ParamInt("SWOUT");
    if(swo == 0){
        EX_WRAN_OFF;
    }
    else if(swo == 1){
        EX_WRAN_ON;
    }

    if(my_config.network.DhcpEnable[0] == 0x00){
        StatusPrintf_S(sizeof(my_config.network.IpAddrss),  "IP", "%s", my_config.network.IpAddrss );
        StatusPrintf_S(sizeof(my_config.network.SnMask),  "MASK", "%s",  my_config.network.SnMask);
        StatusPrintf_S(sizeof(my_config.network.GateWay),  "GW", "%s",    my_config.network.GateWay);
        StatusPrintf_S(sizeof(my_config.network.Dns1),  "DNS1", "%s",  my_config.network.Dns1);
        StatusPrintf_S(sizeof(my_config.network.Dns2),  "DNS2", "%s",  my_config.network.Dns2);
    }
    else{
        val = net_address.active.address;
        StatusPrintf("IP", "%d.%d.%d.%d", (int)(val>>24), (int)(val>>16)&0xFF, (int)(val>>8)&0xFF, (int)(val)&0xFF);
        val = net_address.active.mask;
        StatusPrintf("MASK", "%d.%d.%d.%d", (int)(val>>24), (int)(val>>16)&0xFF, (int)(val>>8)&0xFF, (int)(val)&0xFF);
        val = net_address.active.gateway;
        StatusPrintf("GW", "%d.%d.%d.%d", (int)(val>>24), (int)(val>>16)&0xFF, (int)(val>>8)&0xFF, (int)(val)&0xFF);
        val = net_address.active.dns1;
        StatusPrintf("DNS1", "%d.%d.%d.%d", (int)(val>>24), (int)(val>>16)&0xFF, (int)(val>>8)&0xFF, (int)(val)&0xFF);
        val = 0;
        StatusPrintf("DNS2", "%d.%d.%d.%d", (int)(val>>24), (int)(val>>16)&0xFF, (int)(val>>8)&0xFF, (int)(val)&0xFF);
    }
// mac
    Printf("MAC %04X-%08X\r\n",  fact_config.mac_h, fact_config.mac_l) ;  //net_address.eth.mac1,net_address.eth.mac2);
    Printf("MAC %04X-%08X\r\n",  net_address.eth.mac1,net_address.eth.mac2);

    //StatusPrintf("MAC", "%04X%08X",  net_address.eth.mac1,net_address.eth.mac2);
    if(phy==0){     // ethernet
        StatusPrintf("MAC", "%04X%08X", fact_config.mac_h, fact_config.mac_l);
    }
    else{           // wifi
        StatusPrintf("MAC", "%04X%08X", net_address.eth.mac1, net_address.eth.mac2);
    }
//    StatusPrintf_v2s( "DHCP", my_config.network.DhcpEnable, sizeof(my_config.network.DhcpEnable), "%d");
    StatusPrintf("DHCP", "%d", DHCP_Status);                            // DHCP取得状態

    StatusPrintf_v2s( "PHY", (char *)&phy, sizeof(phy), "%u");
    StatusPrintf_v2s( "STATE", (char *)&UnitState, sizeof(UnitState), "%u");

    val = (uint32_t)isUSBState();
    StatusPrintf_v2s( "USB", (char *)&val, sizeof(val), "%u");

/*TODO
    g_ioport.p_api-> pinRead(EX_WRAN, &level);
    if(level == IOPORT_LEVEL_HIGH)  val = 0;
    else                            val = 1;
*/  val = 0;
    StatusPrintf_v2s( "SWOUT", (char *)&val, sizeof(val), "%u");

    val = 1;
    StatusPrintf_v2s( "PWR", (char *)&val, sizeof(val), "%lu");         // 外部電源の有無(500BWはON固定)

    StatusPrintf("NETUTC", "%lu", NetLogInfo.NetUtc);                   // 最後にネットワーク通信した時刻(UTIM)

    StatusPrintf("NETSTAT", "%lu", NetLogInfo.NetStat);                 // 最後のネットワーク通信の結果(コード)

    StatusPrintf_S(sizeof(NetLogInfo.NetMsg),  "NETMSG", "%s", NetLogInfo.NetMsg );     // 最後のネットワーク通信の結果(メッセージ)

    StatusPrintf("REGCNT", "%lu", my_config.device.RegCnt);             // 登録情報カウンタ

    StatusPrintf("SYSCNT", "%lu", my_config.device.SysCnt);             // 親機設定カウンタ

    return ERR(CMD,NOERROR);
}

/**
 * @brief   [ENSTS]ネットワークの状態
 * @return
 */
uint32_t CFNetworkStatus(void)
{
    uint32_t   val;

    val = net_address.active.address;
    StatusPrintf("IP", "%d.%d.%d.%d", (int)(val>>24), (int)(val>>16)&0xFF, (int)(val>>8)&0xFF, (int)(val)&0xFF);
    val = net_address.active.mask;
    StatusPrintf("MASK", "%d.%d.%d.%d", (int)(val>>24), (int)(val>>16)&0xFF, (int)(val>>8)&0xFF, (int)(val)&0xFF);
    val = net_address.active.gateway;
    StatusPrintf("GW", "%d.%d.%d.%d", (int)(val>>24), (int)(val>>16)&0xFF, (int)(val>>8)&0xFF, (int)(val)&0xFF);
    val = net_address.active.dns1;
    StatusPrintf("DNS", "%d.%d.%d.%d", (int)(val>>24), (int)(val>>16)&0xFF, (int)(val>>8)&0xFF, (int)(val)&0xFF);

    return ERR(CMD,NOERROR);
}


/**
 * @brief   [ELOGS] ログの読み出し
 * @return
 */
uint32_t CFReadLog(void)
{
    uint16_t cmd;
    int size, total, max;
    int mode, act,area;
    uint32_t Err;
    char    *sts;
    int i = 0;

    Err = ERR( CMD, FORMAT );

    max = sizeof(StsArea)-1000;      // 8192
    if((Cmd_Route == CMD_BLE) || (Cmd_Route == CMD_TCP)){
        max = 1024;
    }

    mode = ParamInt("MODE");
    act  = ParamInt("ACT");
    area  = ParamInt("AREA");
    if(area==INT32_MAX){
        area = 0;
    }
    else if(area != 1){
        area = 0;
    }

    Printf("[ELOGS] mode=%ld act=%ld \r\n", mode,act);

    if ( mode == 1 ) {
        ResetLog();                         // 全消去
        PutLog(LOG_SYS,"Log clear");
        Err = ERR(CMD,NOERROR);
    }
    else if ( mode == 0 && act <= 1 ) {

        if ( act == 0 ) {
            //num = ParamInt("NUM");
            //if ( num == INT32_MAX )           // 指定されていない？
            //  num = -1;
            //GetLogInfo(0);
            //GetLog( 0, -1, 0 );           // ポインタセット
            GetLogInfo(area);
            GetLog( 0, -1, 0, area );           // ポインタセット
        }

        cmd = (uint16_t)CmdStatusSize;                  // DATA=の次の場所
        StatusPrintfB( "DATA", CmdTemp, 0 );    // dummy
        sts = (char *)&StsArea[cmd+7];
        total = 0;

        do {
            //size = GetLog( sts, 0, sizeof(StsArea)-1000, area );  // Read
            Printf("Read log %d\r\n", i);
            i++;
            size = GetLog( sts, 0, max, area ); // Read
            if ( size ) {
                total += size;
                Printf("log size %d/%d \r\n", size, total);
                //if ( total < (int)(sizeof(StsArea)-1000) ){// バッファ-1000
                if ( total < (int)(max) ){// バッファ-1000
                    sts += size;                    // ポインタ移動
                }
                else{
                    break;
                }
            }
            else{
                break;
            }

        } while(1);

        Printf("log size end %d\r\n", total);

        StsArea[cmd+5] = (uint8_t)( total );
        StsArea[cmd+6] = (uint8_t)( total >> 8 );
        CmdStatusSize += total;

        Err = ERR(CMD,NOERROR);
    }

    return (Err);
}


//-----↓↓↓↓↓---- 2019/11/28 segi ------------------------------------------------------------------
// [WBLEP] と [RBLEP] の実装


/**
 * [WBLEP] BLE設定の書き込み
 * @attention    BLE有効、無効しか設定できない
 * @return
 */
uint32_t CFWriteBleParam(void)
{
    int Value;
//    int Size, max;
//    char *name, *desc, *bin, *req;
//    char *pw;
    char chg = 0x00;
    uint8_t err = 0x00;

//------------
    Value = ParamInt("ENABLE");
    if((Value == 0) || (Value == 1))
    {
        if(my_config.ble.active != (uint8_t)Value)
        {
            chg = 0x01;
            my_config.ble.active = (uint8_t)Value;
        }
    }
    else if(Value == INT32_MAX){
        ;//err |= CMD_NOP;
    }
//sakaguchi add UT-0025 ↓
    else{
        err |= CMD_PER;
    }
//sakaguchi add UT-0025 ↑

/*------------
    Value = ParamInt("SECURITY");
    if((Value == 0) || (Value == 1))
    {
        if(my_config.ble.utilization != (uint8_t)Value)
        {
            chg = 0x01;
            my_config.ble.utilization = (uint8_t)Value;
        }
    }
    else if(Value == INT32_MAX){
        err |= CMD_NOP;
    }
*/
//------------
/*
    if(ParamAdrs("PW", &bin) != -1)                           // ＢＬＥパスコード
    {
        if(my_config.ble.passcode != *(uint32_t *)bin)        // 今までのＢＬＥパスコードと違う
        {
            chg |= 0x10;
            my_config.ble.passcode = *(uint32_t *)bin;
        }
    }
    else
        err |= CMD_NOP;
*/
/*
    Size = ParamAdrs("PW", &pw);
    max = sizeof(my_config.network.NetPass);
    if(Size >=6 && Size <=max)
    {
        if(Memcmp(my_config.network.NetPass, pw, (int)strlen(my_config.network.NetPass) ,Size) != 0){
            chg |= 0x01;
            memset(my_config.network.NetPass, 0x00, sizeof(my_config.network.NetPass));
            memcpy(my_config.network.NetPass, pw, (size_t)Size);
        }
    }
    else if(Size == -1){
        ;//err |= CMD_NOP;
    }
    else{
        err |= CMD_PER;
    }
*/




//------------
    if((chg != 0x00) && (err == 0x00))                                             // 内容の変更があったら、データを書き込む
    {
        my_config.device.SysCnt++;                              //親機設定変更カウンタ更新 sakaguchi UT-0027
        rewrite_settings();     //本体設定書き換え（関数内でmutex_sfmemを取得返却している)
        read_my_settings();     //本体設定をＳＲＡＭへ読み込み
        //vhttp_sysset_sndon();                                   //親機設定送信 sakaguchi cg UT-0026
        if(HTTP_SEND == Http_Use) vhttp_sysset_sndon();         // 親機設定送信 // sakaguchi 2020.12.24

        if(my_config.ble.active == 0){
//          g_ioport.p_api->pinWrite(PORT_BLE_FREEZE, IOPORT_LEVEL_LOW);        // FREEZE する
            Printf("[WBLEP] Ble Freeze\r\n");
/*TODO
            tx_event_flags_set( &g_ble_event_flags, FLG_PSOC_FREEZ_REQUEST, TX_OR); //PSoC FREEZ要求
            tx_thread_resume(&ble_thread);     //BLEスレッド起動
*/
            LED_Set( LED_BLEOFF, LED_OFF );
        }
        else{
//          g_ioport.p_api->pinWrite(PORT_BLE_FREEZE, IOPORT_LEVEL_HIGH);   // FREEZE 解除
//          ble_reset();// FREEZE 解除
            Printf("[WBLEP] Ble Reset\r\n");
/*TODO
            tx_event_flags_set( &g_ble_event_flags, FLG_PSOC_RESET_REQUEST, TX_OR); //PSoCリセット要求
            tx_thread_resume(&ble_thread);     //BLEスレッド起動
*/
            LED_Set( LED_BLEON, LED_ON );
        }


    }
    else{
        read_my_settings();     //本体設定をＳＲＡＭへ読み込み
    }

//----------------------------------



//    Size = ParamAdrs("REQ", &req);
//    if(memcmp("RESET", (char *)req, Size) == 0) post.WUINF = true;  // ＢＬＥリセット
//    else post.WUINF = false;

    return CMD_RES(err);
}


/**
 * @brief  [RBLEP] BLE設定の読み出し
 * @return
 */
uint32_t CFReadBleParam(void)
{
    StatusPrintf_v2s( "ENABLE", &my_config.ble.active, sizeof(my_config.ble.active), "%d");
//  StatusPrintf_v2s( "SECURITY", (char *)&my_config.ble.utilization, sizeof(my_config.ble.utilization), "%d"); // 2019.12.25 削除
//  StatusPrintfB("PW", (char *)&my_config.ble.passcode, 4);
//  StatusPrintf_S(sizeof(my_config.network.NetPass), "PW", "%s",    my_config.network.NetPass);                    // 2019.12.25 削除
    return ERR(CMD,NOERROR);
}

//-----↑↑↑↑↑---------------------------------------------------------------------------

/**
 * @brief  [EBADV] アドバタイズデータの取得
 * @return
 */
uint32_t CFReadBleAdv(void)
{
    int Size = 63;

//    ble_advertise_packet();     //アドバタイズパケット生成    排他制御がないのでここでは生成させない 2020.Jun.05t.saito

    memcpy((char *)&StsArea[CmdStatusSize], "DATA=", 5);
    CmdStatusSize += 5;

    StsArea[CmdStatusSize++] = (uint8_t)Size;
    StsArea[CmdStatusSize++] = (uint8_t)(Size / 256);


    memcpy((char *)&StsArea[CmdStatusSize], (char *)&BLE_ADV_Packet, (size_t)Size); // 読み込み

    CmdStatusSize += Size;

    return ERR(CMD,NOERROR);
}

/**
 * @brief  [EAUTH] パスワード認証
 * @return
 */
uint32_t CFBleAuth(void)
{
    uint32_t    Err = ERR(CMD, PASS);
    int Size;
    int max,len,i;
    char *pw;

    Printf("[EAUTH] \r\n");

    Size = ParamAdrs("PW", &pw);
    max = sizeof(my_config.network.NetPass);

    Printf("[EAUTH] %d %d \r\n", Size, max);

    BlePass = BLE_PASS_NG;

    len = (int)strlen(my_config.network.NetPass);
    if(len>sizeof(my_config.network.NetPass))  len = sizeof(my_config.network.NetPass);     // 2020.12.04  64byteまで入っていると、終端が不正になる

    for(i=0;i<len;i++){
        Printf("%02X ", my_config.network.NetPass[i]);
    }
    Printf("\r\n");



    if(Size >=6 && Size <=max)
    {
        if(Memcmp(my_config.network.NetPass, pw, (int)strlen(my_config.network.NetPass) ,Size) == 0){
            Printf("PassCode OK !!\r\n");
            BlePass = BLE_PASS_OK;
            Err = ERR(CMD,NOERROR);
        }
    }
    else{
        PutLog( LOG_CMD, "BLE Auth Error");
    }


    return (Err);         // ERR(CMD,NOERROR);
}



//#############  Net Work Setting #######################################


/**
 * [WNSRV] ネットワークサーバ情報の書き込み
 * @return
 * @note    パラメータが無いときは無視する
 * @note    LO_HIマクロ廃止
 */
uint32_t CFWriteNetwork(void)
{
    int Size, Value, max;
    char *srv, *id, *pw, *tmp;
    uint16_t word_data_a,word_data_b;
    uint16_t chg = 0x00;
    uint8_t err = 0x00;
    uint8_t linkdown = 0x00;                                    // ネットワークの再接続指示 // sakaguchi 2020.09.24

    // FTP Server Name
    Size = ParamAdrs("FTPSV", &srv);
    max = sizeof(my_config.ftp.Server);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }

        else if((Memcmp(my_config.ftp.Server, srv, (int)strlen(my_config.ftp.Server) ,Size) != 0))
        {
            chg |= 0x0001;
            memset(my_config.ftp.Server, 0x00, sizeof(my_config.ftp.Server));
            memcpy(my_config.ftp.Server, srv, (size_t)Size);
        }
    }


    // FTP User ID
    Size = ParamAdrs("FTPID", &id);
    max = sizeof(my_config.ftp.UserID);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }

        else if((Memcmp(my_config.ftp.UserID, id, (int)strlen(my_config.ftp.UserID), Size) != 0))
        {
            chg |= 0x0002;
            memset(my_config.ftp.UserID, 0x00, sizeof(my_config.ftp.UserID));
            memcpy(my_config.ftp.UserID, id, (size_t)Size);
        }
    }

    // FTP Password
    Size = ParamAdrs("FTPPW", &pw);
    max= sizeof(my_config.ftp.UserPW);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }

        if((Memcmp(my_config.ftp.UserPW, pw, (int)strlen(my_config.ftp.UserPW) ,Size) != 0))
        {
            chg |= 0x0004;
            memset(my_config.ftp.UserPW, 0x00, sizeof(my_config.ftp.UserPW));
            memcpy(my_config.ftp.UserPW, pw, (size_t)Size);
        }
    }

    // FTP Port
    Value = ParamInt("FTPPT");
    Printf("              FTPPT = %04X\r\n", Value);
    if(Value != INT32_MAX)
    {
        word_data_a = (uint16_t)Value;
        word_data_b = *(uint16_t *)&my_config.ftp.Port[0];
        if(word_data_a != word_data_b)
        {
            chg |= 0x0008;
            *(uint16_t *)&my_config.ftp.Port[0] = word_data_a;
        }
    }

    g_ftp_server_control_port_user = *(uint16_t *)&my_config.ftp.Port[0];

    // FTP mode
    Value = ParamInt("PASV");
    if((Value == 0) || (Value == 1))
    {
        if(my_config.ftp.Pasv[0] != (uint8_t)Value)
        {
            chg = 0x0008;
            my_config.ftp.Pasv[0] = (uint8_t)Value;
        }
    }
    else if(Value == INT32_MAX){
        ;//err |= CMD_NOP;
    }
    else{
        err |= CMD_PER;
    }


   // SMTP Server Name
    Size = ParamAdrs("SMTPSV", &srv);
    max = sizeof(my_config.smtp.Server);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }

        //else if((memcmp(my_config.smtp.Server, srv, (size_t)Size) != 0))
        else if((Memcmp(my_config.smtp.Server, srv, (int)strlen(my_config.smtp.Server),   Size) != 0))
        {
            chg |= 0x0010;
            memset(my_config.smtp.Server, 0x00, sizeof(my_config.smtp.Server));
            memcpy(my_config.smtp.Server, srv, (size_t)Size);
        }
    }

    // SMTP User ID
    Size = ParamAdrs("AUTHID", &id);
    max = sizeof(my_config.smtp.UserID);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }

        else if((Memcmp(my_config.smtp.UserID, id, (int)strlen(my_config.smtp.UserID),Size) != 0))
        {
            chg |= 0x0010;
            memset(my_config.smtp.UserID, 0x00, sizeof(my_config.smtp.UserID));
            memcpy(my_config.smtp.UserID, id, (size_t)Size);
        }
    }

    // SMTP Password
    Size = ParamAdrs("AUTHPW", &pw);
    max= sizeof(my_config.smtp.UserPW);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }

        if((Memcmp(my_config.smtp.UserPW, pw, (int)strlen(my_config.smtp.UserPW),Size) != 0))
        {
            chg |= 0x0010;
            memset(my_config.smtp.UserPW, 0x00, sizeof(my_config.smtp.UserPW));
            memcpy(my_config.smtp.UserPW, pw, (size_t)Size);
        }
    }

    // SMTP Port
    Value = ParamInt("SMTPPT");
    if(Value != INT32_MAX)
    {
        word_data_a = (uint16_t)Value;
        word_data_b = *(uint16_t *)&my_config.smtp.Port[0];
        if(word_data_a != word_data_b)
        {
            chg |= 0x0010;
            *(uint16_t *)&my_config.smtp.Port[0] = word_data_a;
        }
    }

    // SMTP From
    Size = ParamAdrs("FROM", &tmp);
    max= sizeof(my_config.smtp.From);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }

        if((Memcmp(my_config.smtp.From, tmp, (int)strlen(my_config.smtp.From), Size) != 0))
        {
            chg |= 0x0010;
            memset(my_config.smtp.From, 0x00, sizeof(my_config.smtp.From));
            memcpy(my_config.smtp.From, tmp, (size_t)Size);
        }
    }

    // flg2 = JudgeEncode( "DESCF", 64, 12);
    // SMTP Sender
    Size = ParamAdrs("DESCF", &tmp);
    max= sizeof(my_config.smtp.Sender);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }

        if((Memcmp(my_config.smtp.Sender, tmp, (int)strlen(my_config.smtp.Sender), Size) != 0))
        {
            chg |= 0x0010;
            memset(my_config.smtp.Sender, 0x00, sizeof(my_config.smtp.Sender));
            memcpy(my_config.smtp.Sender, tmp, (size_t)Size);
        }
    }

    // SMTP Auth enable ?
    Value = ParamInt("AUTHEN");
    if((Value == 0) || (Value == 1))
    {
        if(my_config.smtp.AuthMode[0] != (uint8_t)Value)
        {
            chg = 0x0010;
            my_config.smtp.AuthMode[0] = (uint8_t)Value;
        }
    }
    else if(Value == INT32_MAX){
        ;//err |= CMD_NOP;
    }
    else{
        err |= CMD_PER;
    }




    // HTTP Server(etc. webstorage) Name
    Size = ParamAdrs("HTTPSV", &srv);
    max = sizeof(my_config.websrv.Server);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }

        else if((Memcmp(my_config.websrv.Server, srv, (int)strlen(my_config.websrv.Server), Size) != 0))
        {
            chg |= 0x0100;
            memset(my_config.websrv.Server, 0x00, sizeof(my_config.websrv.Server));
            memcpy(my_config.websrv.Server, srv, (size_t)Size);
        }
    }


   // HTTP Server Api
    Size = ParamAdrs("HTTPPATH", &srv);
    max = sizeof(my_config.websrv.Api);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }

        else if((Memcmp(my_config.websrv.Api, srv, (int)strlen(my_config.websrv.Api), Size) != 0))
        {
            chg |= 0x0100;
            memset(my_config.websrv.Api, 0x00, sizeof(my_config.websrv.Api));
            memcpy(my_config.websrv.Api, srv, (size_t)Size);
        }
    }


    // HTTP Server Port
    Value = ParamInt("HTTPPT");
    Printf("    HTTP port  %d \r\n", Value);
    if(Value != INT32_MAX)
    {
        word_data_a = (uint16_t)Value;
        word_data_b = *(uint16_t *)&my_config.websrv.Port[0];
        if(word_data_a != word_data_b)
        {
            chg = 0x0200;
            word_data_a = (uint16_t)Value;
            *(uint16_t *)&my_config.websrv.Port[0] = word_data_a;
        }
    }

    // HTTP Access interval
    Value = ParamInt("HTTPINT");
    Printf("    HTTP accsess interval  %d \r\n", Value);
    if(Value != INT32_MAX)
    {
        word_data_a = (uint16_t)Value;
        word_data_b = *(uint16_t *)&my_config.websrv.Interval[0];
        if(word_data_a != word_data_b)
        {
            chg = 0x0200;
            word_data_a = (uint16_t)Value;
            *(uint16_t *)&my_config.websrv.Interval[0] = word_data_a;
//sakaguchi ↓
            ReqOpe_time = Value*60;           //操作リクエスト送信タイマ更新（秒）
//sakaguchi ↑
        }
    }

    // HTTP access type  HTTP or HTTPS ?
    Value = ParamInt("HTTPS");
    if(( 0 <= Value ) && (Value <= 3))
    {
        if(my_config.websrv.Mode[0] != (uint8_t)Value)
        {
            chg = 0x0200;
            my_config.websrv.Mode[0] = (uint8_t)Value;
        }
    }
    else if(Value == INT32_MAX){
        ;//err |= CMD_NOP;
    }
    else{
        err |= CMD_PER;
    }





    // SNTP1 Server Name
    Size = ParamAdrs("SNTP1", &srv);
    max = sizeof(my_config.network.SntpServ1);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }

        else if((Memcmp(my_config.network.SntpServ1, srv, (int)strlen(my_config.network.SntpServ1), Size) != 0))
        {
            chg |= 0x2000;
            memset(my_config.network.SntpServ1, 0x00, sizeof(my_config.network.SntpServ1));
            memcpy(my_config.network.SntpServ1, srv, (size_t)Size);
        }
    }

    // STNP2 Server Name
    Size = ParamAdrs("SNTP2", &srv);
    max = sizeof(my_config.network.SntpServ2);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }

        else if((Memcmp(my_config.network.SntpServ2, srv, (int)strlen(my_config.network.SntpServ2), Size) != 0))
        {
            chg |= 0x2000;
            memset(my_config.network.SntpServ2, 0x00, sizeof(my_config.network.SntpServ2));
            memcpy(my_config.network.SntpServ2, srv, (size_t)Size);
        }
    }

    // Socket Port Number
    Value = ParamInt("SOCPT");
    if(Value != INT32_MAX){
        word_data_a = (uint16_t)Value;
        word_data_b = *(uint16_t *)&my_config.network.SocketPort[0];
        if( word_data_a !=  word_data_b)
        {
            chg = 0x4000;
            *(uint16_t *)&my_config.network.SocketPort[0] = (uint16_t)Value;
            linkdown = 1;       // sakaguchi 2020.09.24
        }
    }

    // UDP Port Number
    Value = ParamInt("UDPPT");
    if(Value != INT32_MAX){
        word_data_a = (uint16_t)Value;
        word_data_b = *(uint16_t *)&my_config.network.UdpRxPort[0];
        if(word_data_a != word_data_b)
        {
            chg = 0x4000;
            *(uint16_t *)&my_config.network.UdpRxPort[0] = (uint16_t)Value;
            linkdown = 1;       // sakaguchi 2020.09.24
        }
    }

    // UDP Response  Port Number
    Value = ParamInt("RESPT");
    if(Value != INT32_MAX){
        word_data_a = (uint16_t)Value;
        word_data_b = *(uint16_t *)&my_config.network.UdpTxPort[0];
        if(word_data_a != word_data_b)
        {
            chg = 0x4000;
            *(uint16_t *)&my_config.network.UdpTxPort[0] =  (uint16_t)Value;
            linkdown = 1;       // sakaguchi 2020.09.24
        }
    }


    if((chg != 0x0000) && (err == 0x00))                                           // 内容の変更があったら、データを書き込む
    {
        rewrite_settings();     //本体設定書き換え（関数内でmutex_sfmemを取得返却している)
        read_my_settings();     //本体設定をＳＲＡＭへ読み込み
// sakaguchi 2020.09.24 ↓
        if(0 != linkdown){
            // アプリからの指示で再起動する
            Net_LOG_Write(0,"%04X",0);                                          // ネットワーク通信結果の初期化
            UnitState = STATE_COMMAND;                                          // 要再起動
        }
// sakaguchi 2020.09.24 ↑
    }
    else{
        read_my_settings();     //本体設定をＳＲＡＭへ読み込み
    }
    Printf("WNSRV %02X %04X \r\n", err, chg);

    return CMD_RES(err);
    //if(err!=0x00)
    //    return ERR(CMD,FORMAT);
    //else
    //    return ERR(CMD,NOERROR);

}



/**
 *  [RNSRV] ネットワークサーバ情報の読み込み
 * @return
 */
uint32_t CFReadNetwork(void)
{

    // ftp
    StatusPrintf_S(sizeof(my_config.ftp.Server),  "FTPSV", "%s", my_config.ftp.Server);
    StatusPrintf_S(sizeof(my_config.ftp.UserID),  "FTPID", "%s", my_config.ftp.UserID);
    StatusPrintf_S(sizeof(my_config.ftp.UserPW),  "FTPPW", "%s", my_config.ftp.UserPW);
    StatusPrintf_v2s( "FTPPT", my_config.ftp.Port, sizeof(my_config.ftp.Port), "%d");
    StatusPrintf_v2s( "PASV", my_config.ftp.Pasv, sizeof(my_config.ftp.Pasv), "%d");
    //StatusPrintfB("PASV", (char *)&my_config.ftp.Pasv, 1);

    /*
    StatusPrintf_S(sizeof(my_config.smtp.Server),  "SMTPSV", "%s", my_config.smtp.Server);
    StatusPrintf_S(sizeof(my_config.smtp.UserID),  "AUTHID", "%s", my_config.smtp.UserID);
    StatusPrintf_S(sizeof(my_config.smtp.UserPW),  "AUTHPW", "%s", my_config.smtp.UserPW);
    StatusPrintf_S(sizeof(my_config.smtp.Sender),  "DESCF",  "%s", my_config.smtp.Sender);
    StatusPrintf_S(sizeof(my_config.smtp.From),    "FROM",   "%s", my_config.smtp.From);
    StatusPrintf_v2s( "SMTPPT", my_config.smtp.Port, sizeof(my_config.smtp.Port), "%d");
    StatusPrintf_v2s( "AUTHEN", my_config.smtp.AuthMode, sizeof(my_config.smtp.AuthMode), "%d");
    */



    // http webstrage
    StatusPrintf_S(sizeof(my_config.websrv.Server), "HTTPSV"  , "%s", my_config.websrv.Server);
    StatusPrintf_S(sizeof(my_config.websrv.Api),    "HTTPPATH", "%s", my_config.websrv.Api);
    StatusPrintf_v2s( "HTTPPT", my_config.websrv.Port, sizeof(my_config.websrv.Port), "%d");
    StatusPrintf_v2s( "HTTPINT", my_config.websrv.Interval, sizeof(my_config.websrv.Interval), "%d");
    StatusPrintf_v2s( "HTTPS", my_config.websrv.Mode, sizeof(my_config.websrv.Mode), "%d");
    //StatusPrintfB("WEBPT", (char *)&my_config.websrv.Port,2);

    // sntp
    StatusPrintf_S(sizeof(my_config.network.SntpServ1),  "SNTP1", "%s", my_config.network.SntpServ1);
    StatusPrintf_S(sizeof(my_config.network.SntpServ2),  "SNTP2", "%s", my_config.network.SntpServ2);

    // socket
    StatusPrintf_v2s( "SOCPT", my_config.network.SocketPort, sizeof(my_config.network.SocketPort), "%d");
    StatusPrintf_v2s( "UDPPT", my_config.network.UdpRxPort, sizeof(my_config.network.UdpRxPort), "%d");
    StatusPrintf_v2s( "RESPT", my_config.network.UdpTxPort, sizeof(my_config.network.UdpTxPort), "%d");
    //StatusPrintfB("SOCPT", (char *)&my_config.network.SocketPort, 2);
    //StatusPrintfB("UDPPT", (char *)&my_config.network.UdpRxPort, 2);
    //StatusPrintfB("RESPT", (char *)&my_config.network.UdpTxPort, 2);


    return ERR(CMD,NOERROR);
}




/**
 * @brief   [WNETP] ネットワークパラメータの設定
 *
 * サイズ違いは、PMエラー
 * 同じ内容の場合は、書き換えない
 * @return
 */
uint32_t CFWriteNetParam(void)
{
    int Size, max;
    int Value;
    char *ip, *mask, *gw, *dns1, *dns2, *pw;
    //uint8_t chg = 0x00;
    uint16_t chg = 0x00;                                        // sakaguchi 2021.04.07
    uint8_t err = 0x00;
    uint8_t linkdown = 0x00;                                    // ネットワークの再接続指示 // sakaguchi 2020.09.02

    Size = ParamAdrs("IP", &ip);
    max = sizeof(my_config.network.IpAddrss);
    if(Size != -1)   // no param ??
    {
        if(Size > max){
            err |= CMD_PER;
        }

        // 内容同じ？？
        else if(/*((Size < sz) && (my_config.network.IpAddrss[Size] != 0x00)) ||*/ (Memcmp(my_config.network.IpAddrss, ip, (int)strlen(my_config.network.IpAddrss), Size) != 0))
        {
                chg |= 0x01;
                linkdown = 1;                                   // ネットワークの再接続指示 // sakaguchi 2020.09.02
                memset(my_config.network.IpAddrss, 0x00, sizeof(my_config.network.IpAddrss));
                memcpy(my_config.network.IpAddrss, ip, (size_t)Size);
        }
    }
    //else
    //    err |= CMD_NOP;

    Printf("WNETP IP   %02X %02X \r\n", err, chg);

    Size = ParamAdrs("MASK", &mask);
    max = sizeof(my_config.network.SnMask);

    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }

        else if((Memcmp(my_config.network.SnMask, mask, (int)strlen(my_config.network.SnMask), Size) != 0))
        {
            chg |= 0x02;
            linkdown = 1;                                       // ネットワークの再接続指示 // sakaguchi 2020.09.02
            memset(my_config.network.SnMask, 0x00, sizeof(my_config.network.SnMask));
            memcpy(my_config.network.SnMask, mask, (size_t)Size);
        }
    }

    Printf("WNETP MASK %02X %02X \r\n", err, chg);

    Size = ParamAdrs("GW", &gw);
    max = sizeof(my_config.network.GateWay);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }

        else if((Memcmp(my_config.network.GateWay, gw, (int)strlen(my_config.network.GateWay), Size) != 0))
        {
            chg |= 0x04;
            linkdown = 1;                                       // ネットワークの再接続指示 // sakaguchi 2020.09.02
            memset(my_config.network.GateWay, 0x00, sizeof(my_config.network.GateWay));
            memcpy(my_config.network.GateWay, gw, (size_t)Size);
        }
     }

    Printf("WNETP GW   %02X %02X \r\n", err, chg);

    Size = ParamAdrs("DNS1", &dns1);
    max = sizeof(my_config.network.Dns1);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }

        else if((Memcmp(my_config.network.Dns1, dns1, (int)strlen(my_config.network.Dns1), Size) != 0))
        {
            chg |= 0x08;
            linkdown = 1;                                       // ネットワークの再接続指示 // sakaguchi 2020.09.02
            memset(my_config.network.Dns1, 0x00, sizeof(my_config.network.Dns1));
            memcpy(my_config.network.Dns1, dns1, (size_t)Size);
        }
    }

    Printf("WNETP DNS1  %02X %02X \r\n", err, chg);

    Size = ParamAdrs("DNS2", &dns2);
    max = sizeof(my_config.network.Dns2);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }

        else if((Memcmp(my_config.network.Dns2, dns2, (int)strlen(my_config.network.Dns2), Size) != 0))
        {
            chg |= 0x10;
            linkdown = 1;                                       // ネットワークの再接続指示 // sakaguchi 2020.09.02
            memset(my_config.network.Dns2, 0x00, sizeof(my_config.network.Dns2));
            memcpy(my_config.network.Dns2, dns2, (size_t)Size);
        }
    }


    Printf("WNETP DNS2  %02X %02X \r\n", err, chg);

    Value = ParamInt("DHCP");
    if((Value == 0) || (Value == 1))
    {
        if(my_config.network.DhcpEnable[0] != Value){
//            chg = 0x20;
            chg |= 0x20;
            linkdown = 1;                                       // ネットワークの再接続指示 // sakaguchi 2020.09.02
            my_config.network.DhcpEnable[0] = (char)Value;
            DHCP_Status = 3;                                    // DHCPステータスを取得中にする
        }
    }
    else if(Value == INT32_MAX){
        ;//err |= CMD_NOP;
    }
    else{
        err |= CMD_PER;
    }

    Printf("WNETP DHCP  %02X %02X %d\r\n", err, chg, Value);

    Size = ParamAdrs("PW", &pw);
    max = sizeof(my_config.network.NetPass);
    if(Size >=6 && Size <=max)
    {
        if(Memcmp(my_config.network.NetPass, pw, (int)strlen(my_config.network.NetPass), Size) != 0){
            chg |= 0x40;
            memset(my_config.network.NetPass, 0x00, sizeof(my_config.network.NetPass));    // NetPassを使用する時は、65byteとして
            memcpy(my_config.network.NetPass, pw, (size_t)Size);

            my_config.device.SysCnt++;                              //親機設定変更カウンタ更新 sakaguchi add UT-0033
            //vhttp_sysset_sndon();                                   //親機設定送信 sakaguchi add UT-0033
            if(HTTP_SEND == Http_Use) vhttp_sysset_sndon();         // 親機設定送信 // sakaguchi 2020.12.24
        }
    }
    else if(Size == -1){
        ;//err |= CMD_NOP;
    }
    else{
        err |= CMD_PER;
    }

    Printf("WNETP PW %02X %02X \r\n", err, chg);

    Value = ParamInt("PHY");
    if((Value == 0) || (Value == 1))
    {
        if(my_config.network.Phy[0] != (char)Value){
            chg = 0x80;
            linkdown = 1;                                       // ネットワークの再接続指示 // sakaguchi 2020.09.02
            my_config.network.Phy[0] = (char)Value;
            DHCP_Status = 3;                                    // DHCPステータスを取得中にする
        }
    }
    else if(Value == INT32_MAX){
        ;//err |= CMD_NOP;
    }
    else{
        err |= CMD_PER;
    }

    Printf("WNETP PHY  %02X %02X %d %d\r\n", err, chg, Value, linkdown);            // sakaguchi 2020.09.02

    // MTU追加 sakaguchi 2021.04.07
    Value = ParamInt("MTU");
    if((576 <= Value) && (Value <= 1400))
    {
        if( *(uint16_t *)&my_config.network.Mtu[0] != (uint16_t)Value ){
            chg |= 0x0100;
            linkdown = 1;                                       // ネットワークの再接続指示
            *(uint16_t *)&my_config.network.Mtu[0] = (uint16_t)Value;
        }
    }
    else if(Value == INT32_MAX){
        ;//err |= CMD_NOP;
    }
    else{
        err |= CMD_PER;
    }
    Printf("WNETP MTU  %02X %02X %d\r\n", err, chg, Value);

// sakaguchi 2021.05.28 ↓
    // 通信間隔
    Value = ParamInt("INTV");
    if((0 <= Value) && (Value <= 200))
    {
        if(my_config.network.Interval[0] != (char)Value){
            chg |= 0x0200;
            my_config.network.Interval[0] = (char)Value;
        }
    }
    else if(Value == INT32_MAX){
        ;//err |= CMD_NOP;
    }
    else{
        err |= CMD_PER;
    }
// sakaguchi 2021.05.28 ↑

    if((chg != 0x00) && (err == 0x00))                                             // 内容の変更があったら、データを書き込む
    {
        rewrite_settings();     //本体設定書き換え（関数内でmutex_sfmemを取得返却している)
        read_my_settings();     //本体設定をＳＲＡＭへ読み込み
// sakaguchi 2020.09.16 ↓
        if(0 != linkdown){      // sakaguchi 2020.09.02
        // アプリからの指示で再起動する
            //tx_event_flags_set (&event_flags_link, 0x00000000, TX_AND);
            //tx_event_flags_set (&event_flags_link, FLG_LINK_DOWN, TX_OR);     // ネットワークの再接続実行
        Net_LOG_Write(0,"%04X",0);                                          // ネットワーク通信結果の初期化
            UnitState = STATE_COMMAND;                                          // 要再起動
        }
// sakaguchi 2020.09.16 ↑
    }
    else{
        read_my_settings();     //本体設定をＳＲＡＭへ読み込み
    }

    Printf("WNETP %02X %02X \r\n", err, chg);

    return CMD_RES(err);

}



/**
 * [RNETP] ネットワークパラメータの設定
 * @return
 */
uint32_t CFReadNetParam(void)
{

    StatusPrintf_S(sizeof(my_config.network.IpAddrss),  "IP", "%s", my_config.network.IpAddrss );
    StatusPrintf_S(sizeof(my_config.network.SnMask),  "MASK", "%s",  my_config.network.SnMask);
    StatusPrintf_S(sizeof(my_config.network.GateWay),  "GW", "%s",    my_config.network.GateWay);
    StatusPrintf_S(sizeof(my_config.network.Dns1),  "DNS1", "%s",  my_config.network.Dns1);
    StatusPrintf_S(sizeof(my_config.network.Dns2),  "DNS2", "%s",  my_config.network.Dns2);

    StatusPrintf_v2s( "DHCP", (char *)&my_config.network.DhcpEnable, sizeof(my_config.network.DhcpEnable), "%d");

    StatusPrintf_S(sizeof(my_config.network.NetPass), "PW", "%s",    my_config.network.NetPass);

    StatusPrintf_v2s( "PHY", (char *)&my_config.network.Phy, sizeof(my_config.network.Phy), "%d");

    StatusPrintf_v2s( "MTU", (uint16_t *)&my_config.network.Mtu, sizeof(my_config.network.Mtu), "%d");

    StatusPrintf_v2s( "INTV", (char *)&my_config.network.Interval, sizeof(my_config.network.Interval), "%d");   // sakaguchi 2021.07.16

    return ERR(CMD,NOERROR);
}


/**
 * @brief    [EWLAP] ネットワークパラメータの設定
 * @return
 * @note    AP_Count使用
 */
uint32_t CFReadWalnAp(void)
{
    char ap[16];
    char temp[8];
    int i;

    if(my_config.network.Phy[0]== 1){

        for(i = 0; i < 5; i++){
            wifi_module_scan();
            if(0 < AP_Count){                       // sakaguchi 2020.09.07-2
                break;
            }
        }
        Printf("AP list count %d\r\n", AP_Count);
        if(0 == AP_Count){  return ERR(CMD,NOERROR); }      // APが発見できなかった場合ここで処理終了 // sakaguchi 2020.09.07-2

        for(i = 0; i < AP_Count; i++){
            Printf("AP-%d : %.32s   (%d)\r\n", i, AP_List[i].ssid, (int8_t)AP_List[i].rssi);
        }

        for(i = 0; i < AP_Count; i++)
        {
            sprintf(ap,"AP%d",i+1);
            //sprintf(temp, "%.32s",AP_List[i]);
            StatusPrintf_S(sizeof(AP_List[i].ssid),  ap, "%s", AP_List[i].ssid );
            sprintf(ap,"RSSI%d",i+1);
            sprintf(temp,"%d",(int8_t)AP_List[i].rssi);
            StatusPrintf_S(sizeof(temp),  ap, "%s", temp);
            sprintf(ap,"SEC%d",i+1);
            sprintf(temp,"%d",(int8_t)AP_List[i].sec);
            StatusPrintf_S(sizeof(temp),  ap, "%s", temp);
        }

    }
    return ERR(CMD,NOERROR);
}




/**
 * @brief   [WWLAN] 無線LANの設定
 * @return
 */
uint32_t CFWriteWlanParam(void)
{
    int Size, Value, max;
    char *ssid, *wep, *psk;
    uint16_t chg = 0x0000;
    uint8_t err = 0x00;

    Value = ParamInt("SEC");
    if(Value>=0 && Value <= 4){
        if(my_config.wlan.SEC[0] != (uint8_t)Value){
            chg |= 0x0010;
            my_config.wlan.SEC[0] = (uint8_t)Value;
            Printf("WIFI SEC Mode %d\r\n", Value);
        }
    }
    else if(Value == INT32_MAX){
        ;//err |= CMD_NOP;
    }
    else{
        err |= CMD_PER;
    }


    Size = ParamAdrs("WEP", &wep);
    max = sizeof(my_config.wlan.WEP);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }

        else if((Memcmp(my_config.wlan.WEP, wep, (int)strlen(my_config.wlan.WEP),  Size) != 0))
        {
            chg |= 0x0040;
            memset(my_config.wlan.WEP, 0x00, sizeof(my_config.wlan.WEP));
            memcpy(my_config.wlan.WEP, wep, (size_t)Size);
        }
    }

    Size = ParamAdrs("PSK", &psk);
    max = sizeof(my_config.wlan.PSK);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }

        else if((Memcmp(my_config.wlan.PSK, psk, (int)strlen(my_config.wlan.PSK), Size) != 0))
        {
            chg |= 0x0080;
            memset(my_config.wlan.PSK, 0x00, sizeof(my_config.wlan.PSK));
            memcpy(my_config.wlan.PSK, psk, (size_t)Size);
        }
    }


    Size = ParamAdrs("SSID", &ssid);
    max = sizeof(my_config.wlan.SSID);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }

        else if((Memcmp(my_config.wlan.SSID, ssid, (int)strlen(my_config.wlan.SSID), Size) != 0))
        {
            chg |= 0x0100;
            memset(my_config.wlan.SSID, 0x00, sizeof(my_config.wlan.SSID));
            memcpy(my_config.wlan.SSID, ssid, (size_t)Size);
        }
    }

    Printf("WWLAN %02X %04X \r\n", err, chg);

    if((chg != 0x0000) && (err == 0x00))                                       // 内容の変更があったら、データを書き込む
    {
        rewrite_settings();     //本体設定書き換え（関数内でmutex_sfmemを取得返却している)
        read_my_settings();     //本体設定をＳＲＡＭへ読み込み

// sakaguchi 2020.09.16 ↓
        // アプリからの指示で再起動する Net_LOG_Writeの値は0⇒？（再起動）に変更する
        //tx_event_flags_set (&event_flags_link, 0x00000000, TX_AND);
        //tx_event_flags_set (&event_flags_link, FLG_LINK_DOWN, TX_OR);       // ネットワークの再接続実行
        Net_LOG_Write(0,"%04X",0);                                          // ネットワーク通信結果の初期化
        UnitState = STATE_COMMAND;                                          // 要再起動
// sakaguchi 2020.09.16 ↑
        DHCP_Status = 3;                                                    // DHCPステータスを取得中にする// sakaguchi 2020.09.14
    }
    else{
        read_my_settings();     //本体設定をＳＲＡＭへ読み込み
    }
    return CMD_RES(err);

}




/**
 *  [RWLAN] 無線LANの設定
 * @return
 */
uint32_t CFReadWlanParam(void)
{

    //StatusPrintfB("WLEN",       (char *)&my_config.wlan.Enable, 1);
    //StatusPrintfB("BAND",       (char *)&my_config.wlan.BAND, 1);
    //StatusPrintfB("CH2",        (char *)&my_config.wlan.CH2G, 2);
    //StatusPrintfB("CH5",        (char *)&my_config.wlan.CH5G, 2);

//    StatusPrintf_v2s( "WLEN", (char *)&my_config.wlan.Enable, sizeof(my_config.wlan.Enable), "%d");       // このパラメータは廃止
    StatusPrintf_v2s( "SEC", my_config.wlan.SEC, sizeof(my_config.wlan.SEC), "%d");
    StatusPrintf_S(sizeof(my_config.wlan.SSID), "SSID", "%s",   my_config.wlan.SSID);
    //StatusPrintfB("SEC",        (char *)&my_config.wlan.SEC, 1);

    StatusPrintf_S(sizeof(my_config.wlan.WEP), "WEP", "%s",   my_config.wlan.WEP);
    StatusPrintf_S(sizeof(my_config.wlan.PSK), "PSK", "%s",   my_config.wlan.PSK);

    return ERR(CMD,NOERROR);
}




/**
 * [WDTIM] 時刻・日付の設定
 * @return
 */
uint32_t CFWriteDtime(void)
{

    int Size, Value, max;
    char *summer, *diff, *form, *zone ,*para;

    uint32_t     Secs = 0;
    int Min,Year;
    int     sz_dtime, sz_utc;
    rtc_time_t  tm;

    uint16_t chg = 0x0000;
    uint8_t err = 0x00;

    uint32_t Err;

    Err = ERR(CMD, NOERROR);

    RTC_GetLCT( &tm );              // DTIMEが無い場合は
    Year = 2000 + tm.tm_year;           // 現在のローカル時刻をセット



    Value = ParamInt("SYNC");
    if((Value >= 0) && (Value <= 2))
    {
        if(my_config.device.TimeSync[0] != (uint8_t)Value){
            chg |= 0x0001;
            my_config.device.TimeSync[0] = (uint8_t)Value;
        }
    }
    else if(Value == INT32_MAX){
        ;//err |= CMD_NOP;
    }
    else{
        err |= CMD_PER;
    }

    Size = ParamAdrs("DST", &summer);
    max = sizeof(my_config.device.Summer);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }

        else if((Memcmp(my_config.device.Summer, summer, (int)strlen(my_config.device.Summer), Size) != 0))
        {
            chg |= 0x0002;
            memset(my_config.device.Summer, 0x00, sizeof(my_config.device.Summer));
            memcpy(my_config.device.Summer, summer, (size_t)Size);
        }
    }

    Size = ParamAdrs("DIFF", &diff);
    max = sizeof(my_config.device.TimeDiff);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }


        else if((Memcmp(my_config.device.TimeDiff, diff, (int)strlen(my_config.device.TimeDiff), Size) != 0))
        {
            chg |= 0x0004;
            memset(my_config.device.TimeDiff, 0x00, sizeof(my_config.device.TimeDiff));
            memcpy(my_config.device.TimeDiff, diff, (size_t)Size);
        }
    }

    Size = ParamAdrs("FORM", &form);
    max = sizeof(my_config.device.TimeForm);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }

        else if((Memcmp(my_config.device.TimeForm, form, (int)strlen(my_config.device.TimeForm), Size) != 0))
        {
            chg |= 0x0008;
            memset(my_config.device.TimeForm, 0x00, sizeof(my_config.device.TimeForm));
            memcpy(my_config.device.TimeForm, form, (size_t)Size);
        }
    }


    Size = ParamAdrs("ZONE", &zone);
    max = sizeof(my_config.device.TimeZone);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }

        else if((Memcmp(my_config.device.TimeZone, zone, (int)strlen(my_config.device.TimeZone), Size) != 0))
        {
            chg |= 0x0010;
            memset(my_config.device.TimeZone, 0x00, sizeof(my_config.device.TimeZone));
            memcpy(my_config.device.TimeZone, zone, (size_t)Size);
        }
    }

    // SMEMに書き込みが必要なのはここまで

    if((chg != 0x0000) && (err == 0x00))                                             // 内容の変更があったら、データを書き込む
    {
        Printf("[WDTIM] Updata %02X\r\n", chg);

        my_config.device.SysCnt++;                              //親機設定変更カウンタ更新 sakaguchi UT-0027

        rewrite_settings();     //本体設定書き換え（関数内でmutex_sfmemを取得返却している)
        read_my_settings();     //本体設定をＳＲＡＭへ読み込み

        //vhttp_sysset_sndon();                                   //親機設定送信 sakaguchi cg UT-0026
        if(HTTP_SEND == Http_Use) vhttp_sysset_sndon();         // 親機設定送信 // sakaguchi 2020.12.24
        RTC_Create();           // 2020.09.16
    }
    else{
        read_my_settings();     //本体設定をＳＲＡＭへ読み込み
    }



    sz_utc = ParamAdrs("UTC",&para);
    if ( sz_utc > 0 ) {
        Secs = (uint32_t)AtoL( para, sz_utc );
        RTC_GSec2LCT( Secs, &tm );
        Year = tm.tm_year + 2000;
    }

    sz_dtime = ParamAdrs("DTIME",&para) ;
    if ( sz_dtime >= 0 ) {
        if ( (sz_utc < 0) && (sz_dtime == 14) ) {
            Year = AtoL( para, 4 );
            tm.tm_mon   = AtoL( para +  4, 2 );
            tm.tm_mday  = AtoL( para +  6, 2 );
            tm.tm_hour  = AtoL( para +  8, 2 );
            tm.tm_min   = AtoL( para + 10, 2 );
            tm.tm_sec   = AtoL( para + 12, 2 );
        }
        else{
            err |= CMD_PER;             // パラメータエラー  ※エラー処理検討のこと --> 結局全てFORMATエラー （動作中等を除くと）
        }
    }

    Printf("UTC %d /DTIME %d\r\n", sz_utc, sz_dtime);

    memcpy( CmdTemp, my_config.device.TimeDiff, sizeof(my_config.device.TimeDiff) );
    Min = ( AtoI( CmdTemp, 3 ) * 60 ) + AtoI( &CmdTemp[3], 2 );
    if ( Min >= 24*60 || Min <= -24*60 ){        //2020.01.17 条件式が常にtrueになるのでMInをintに変更
        Min = 0;
    }
    RTC.Diff = Min * 60;        // 秒で格納     // sakaguchi add UT-0020 時差の更新が必要
    //TimeDiff = RTC.Diff;
if((sz_utc != -1) || (sz_dtime != -1) ){ //指定している場合のみ実行
    if ( Year > 2019 && Year < 2100 ) {     //2020.01.17 条件式 || を &&に変更
        tm.tm_year = ( Year - 2000 );

        if ( Secs ) {               // UTCによる指定？

            SetDST( tm.tm_year );                   // サマータイム再構築   // sakaguchi 2021.04.13

// sakaguchi add UT-0023 ↓
            if( true == RTC_GSec2Date( Secs, &tm )){            // UTCによる時計セット
            //RTC.Create( &tm );                    // Diffなども再セットされる

// sakaguchi add UT-0021 ↓
                tm.tm_year += (2000-1900);
                tm.tm_mon -= 1;
                adjust_time( &tm );
// sakaguchi add UT-0021 ↑

            Err = ERR(CMD,NOERROR);
            }else{
                Err = ERR(CMD,FORMAT);      // フォーマットエラー
            }
// sakaguchi add UT-0023 ↑
        }
        else{
            Secs = RTC_Date2GSec( &tm ) ;
            if ( Secs ) {   // 日付チェック

                Secs = (uint32_t)(Secs - (uint32_t)(Min * 60));                 // 時差
                SetDST( tm.tm_year );                   // サマータイム再構築
                RTC_GSec2Date( Secs, &tm );         // 一度構築して

                if ( GetSummerAdjust( Secs ) ){     // サマータイム中なら？
                    RTC_GSec2Date( (uint32_t)((int64_t)Secs - RTC.AdjustSec), &tm );    // Bias値を減算して再構築
                }
                //RTC.Create( &tm );                    // Diffなども再セットされる

// sakaguchi add UT-0021 ↓
                tm.tm_year += (2000-1900);
                tm.tm_mon -= 1;
                adjust_time( &tm );
// sakaguchi add UT-0021 ↑

                Err = ERR(CMD,NOERROR);
// sakaguchi add UT-0023 ↓
            }else{
                Err = ERR(CMD,FORMAT);      // フォーマットエラー
            }
// sakaguchi add UT-0023 ↑
        }
    }
    else{
        Err = ERR(CMD,FORMAT);      // フォーマットエラー
    }
}

    if( CMD_RES(err) != ERR(CMD,NOERROR)){
        Err = CMD_RES(err);
    }

    return (Err);

}



/**
 * [RDTIM] 時刻・日付の読み込み
 * @return
 */
uint32_t CFReadDtime(void)
{

    //char form[sizeof(my_config.device.TimeForm)+1];
    rtc_time_t tm;
    uint32_t GSec;

    GSec = RTC_GetGMTSec();

    RTC_GSec2LCT( GSec, &tm );

    RTC_GetFormStr( &tm, CmdTemp , "*Y*m*d*H*i*s" );
    StatusPrintf("DTIME", "%s", CmdTemp);
    StatusPrintf("UTC", "%lu", GSec);

    GSec = GetSummerAdjust( GSec ) / 60;    // サマータイム補正（秒）
    StatusPrintf("BIAS", "%+.02d", (uint16_t)GSec);

    StatusPrintf_S(sizeof(my_config.device.Summer),  "DST", "%s",  my_config.device.Summer);

    StatusPrintf_S(sizeof(my_config.device.TimeForm),  "FORM", "%s",  my_config.device.TimeForm);

    StatusPrintf_S(sizeof(my_config.device.TimeDiff),  "DIFF", "%s",  my_config.device.TimeDiff);

    StatusPrintf_S(sizeof(my_config.device.TimeZone),  "ZONE", "%s",  my_config.device.TimeZone);

//    StatusPrintfB("SYNC",       (char *)&my_config.device.TimeSync, 1);
    StatusPrintf_v2s( "SYNC", my_config.device.TimeSync, sizeof(my_config.device.TimeSync), "%d");  // 復活 20/09/29　応答パラメタ修正 2019/11/27 segi

    return ERR(CMD,NOERROR);
}




/**
 * [WENCD] エンコードの設定
 * @return
 */
uint32_t CFWriteEncode(void)
{
    int Value;
//    int Size, max;
//    char *summer, *diff, *form, *zone ,*para;


    uint16_t chg = 0x0000;
    uint8_t err = 0x00;

 //   uint32_t Err;     2020.Apr.07 コメントアウト

 //   Err = ERR(CMD, NOERROR);      2020.Apr.07 コメントアウト

    Value = ParamInt("MAIL");
    if((Value == 0) || (Value == 1))
    {
        if(my_config.smtp.Encode[0] != (uint8_t)Value){
            chg |= 0x0001;
            my_config.smtp.Encode[0] = (uint8_t)Value;
        }
    }
    else if(Value == INT32_MAX){
        ;
    }
    else{
        err |= CMD_PER;
    }

    Value = ParamInt("FTP");
    if((Value >= 0) && (Value <= 2))
    {
        if(my_config.ftp.Encode[0] != (uint8_t)Value){
            chg |= 0x0002;
            my_config.ftp.Encode[0] = (uint8_t)Value;
        }
    }
    else if(Value == INT32_MAX){
        ;
    }
    else{
        err |= CMD_PER;
    }

    if((chg != 0x0000) && (err == 0x00))                                           // 内容の変更があったら、データを書き込む
    {
        rewrite_settings();     //本体設定書き換え（関数内でmutex_sfmemを取得返却している)
        read_my_settings();     //本体設定をＳＲＡＭへ読み込み
    }
    else{
        read_my_settings();     //本体設定をＳＲＡＭへ読み込み
    }

    Printf("WENCD %02X %04X \r\n", err, chg);

    return CMD_RES(err);
}




/**
 * [RENCD] エンコードの読み込み
 * @return
 */
uint32_t CFReadEncode(void)
{

    //StatusPrintf_v2s( "MAIL", my_config.smtp.Encode, sizeof(my_config.smtp.Encode), "%d");
    StatusPrintf_v2s( "FTP",  my_config.ftp.Encode, sizeof(my_config.ftp.Encode), "%d");

    return ERR(CMD,NOERROR);
}




/**
 * [WPRXY] PROXYの設定
 * @return
 * @note    LO_HIマクロ廃止
 */
uint32_t CFWProxy(void)
{
    int Size, Value, max;
    char *srv;
    uint16_t word_data_a, word_data_b;

    uint8_t chg = 0x00;
    uint8_t err = 0x00;

    Value = ParamInt("FTPEN");
    if((Value==0) || (Value == 1)){
        chg |= 0x01;
        my_config.network.ProxyFtp[0] = (uint8_t)Value;
    }
    else if(Value == INT32_MAX){
        ;//err |= CMD_NOP;
    }
    else{
        err |= CMD_PER;
    }


    Value = ParamInt("HTTPEN");
    if((Value==0) || (Value == 1)){
        chg |= 0x02;
        my_config.network.ProxyHttp[0] = (uint8_t)Value;
    }
    else if(Value == INT32_MAX){
        ;//err |= CMD_NOP;
    }
    else{
        err |= CMD_PER;
    }


    Value = ParamInt("FTPPT");
    if(Value == INT32_MAX){
        ;//err |= CMD_NOP;
    }
    else {
        word_data_a = (uint16_t)Value;
        word_data_b = *(uint16_t *)&my_config.network.ProxyFtpPort[0];
        if(word_data_a != word_data_b)
        {
            chg |= 0x04;
//            word_data_a = (uint16_t)Value;
            *(uint16_t *)&my_config.network.ProxyFtpPort[0] = (uint16_t)Value;
        }
    }

    Value = ParamInt("HTTPPT");
    if(Value == INT32_MAX){
        ;//err |= CMD_NOP;
    }
    else {
        word_data_a = (uint16_t)Value;
        word_data_b = *(uint16_t *)&my_config.network.ProxyHttpPort[0];
        if(word_data_a != word_data_b)
        {
            chg |= 0x08;
            *(uint16_t *)&my_config.network.ProxyHttpPort[0] = (uint16_t)Value;
        }
    }

    Size = ParamAdrs("FTPSV", &srv);
    max = sizeof(my_config.network.ProxyFtpServ);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }
        else if((Memcmp(my_config.network.ProxyFtpServ, srv, (int)strlen(my_config.network.ProxyFtpServ), Size) != 0))
        {
            chg |= 0x10;
            memset(my_config.network.ProxyFtpServ, 0x00, sizeof(my_config.network.ProxyFtpServ));
            memcpy(my_config.network.ProxyFtpServ, srv, (size_t)Size);
        }
    }


    Size = ParamAdrs("HTTPSV", &srv);
    max = sizeof(my_config.network.ProxyHttpServ);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }
        else if((Memcmp(my_config.network.ProxyHttpServ, srv, (int)strlen(my_config.network.ProxyHttpServ) ,Size) != 0))
        {
            chg |= 0x20;
            memset(my_config.network.ProxyHttpServ, 0x00, sizeof(my_config.network.ProxyHttpServ));
            memcpy(my_config.network.ProxyHttpServ, srv, (size_t)Size);
        }
    }

    if((chg != 0x00) && (err == 0x00))                                           // 内容の変更があったら、データを書き込む
    {
        rewrite_settings();     //本体設定書き換え（関数内でmutex_sfmemを取得返却している)
        read_my_settings();     //本体設定をＳＲＡＭへ読み込み
        Net_LOG_Write(0,"%04X",0);                             // ネットワーク通信結果の初期化
    }
    else{
        read_my_settings();     //本体設定をＳＲＡＭへ読み直し
    }

    return CMD_RES(err);

}




/**
 * [RPRXY] PROXYの読み込み
 * @return
 */
uint32_t CFRProxy(void)
{

//     FTP Proxyは対応しない
//    StatusPrintfB("FTPEN", my_config.network.ProxyFtp,1);
//    StatusPrintf_S(sizeof(my_config.network.ProxyFtpServ), "FTPSV", "%s",    my_config.network.ProxyFtpServ);
//    StatusPrintfB("FTPPT", my_config.network.ProxyFtpPort, 2);

    StatusPrintf_v2s( "HTTPEN", my_config.network.ProxyHttp, sizeof(my_config.network.ProxyHttp), "%d");
    StatusPrintf_S(sizeof(my_config.network.ProxyHttpServ),  "HTTPSV", "%s",    my_config.network.ProxyHttpServ);
    StatusPrintf_v2s( "HTTPPT", my_config.network.ProxyHttpPort, sizeof(my_config.network.ProxyHttpPort), "%d");

    return ERR(CMD,NOERROR);
}




//##########  自律動作設定 ####################################################


/**
 * [WWARP] 警報の設定
 * @return
 * @note    LO_HIマクロ廃止
 */
uint32_t CFWriteWarning(void)
{
    int Size, Value, max;
    char *tmp;
    uint16_t word_data_a, word_data_b;
    uint16_t chg = 0x0000;
    uint16_t err = 0x0000;

    Value = ParamInt("ENABLE");
    if(Value==0 || Value == 1){
        if(my_config.warning.Enable[0] != (uint8_t)Value)
        {
            chg |= 0x0001;
            my_config.warning.Enable[0] = (uint8_t)Value;
        }
    }
    else if (Value == INT32_MAX){
        ;//err |= CMD_NOP;
    }
    else{
        err |= CMD_PER;
    }

    Value = ParamInt("INT");
    if(Value != INT32_MAX)
    {
        if( 1 <= Value && Value <= 1440 ){
            word_data_a = (uint16_t)Value;
            word_data_b = *(uint16_t *)&my_config.warning.Interval[0];
            if(word_data_a != word_data_b){
                chg = 0x0002;
                *(uint16_t *)&my_config.warning.Interval[0] = word_data_a;
            }
        }
        else{
            err |= CMD_PER;
        }
    }

    Size = ParamAdrs("SBJ", &tmp);
    max = sizeof(my_config.warning.Subject);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }
        else if((Memcmp(my_config.warning.Subject, tmp, (int)strlen(my_config.warning.Subject), Size) != 0))
        {
            chg |= 0x0004;
            memset(my_config.warning.Subject, 0x00, sizeof(my_config.warning.Subject));
            memcpy(my_config.warning.Subject, tmp, (size_t)Size);
        }
    }

    Size = ParamAdrs("BODY", &tmp);
    max = sizeof(my_config.warning.Body);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }
        else if((Memcmp(my_config.warning.Body, tmp, (int)strlen(my_config.warning.Body) ,Size) != 0))
        {
            chg |= 0x0008;
            memset(my_config.warning.Body, 0x00, sizeof(my_config.warning.Body));
            memcpy(my_config.warning.Body, tmp, (size_t)Size);
        }
    }

    Size = ParamAdrs("TO1", &tmp);
    max = sizeof(my_config.warning.To1);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }
        else if((Memcmp(my_config.warning.To1, tmp, (int)strlen(my_config.warning.To1), Size) != 0))
        {
            chg |= 0x00010;
            memset(my_config.warning.To1, 0x00, sizeof(my_config.warning.To1));
            memcpy(my_config.warning.To1, tmp, (size_t)Size);
        }
    }

    Size = ParamAdrs("TO2", &tmp);
    max = sizeof(my_config.warning.To2);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }
        else if((Memcmp(my_config.warning.To2, tmp, (int)strlen(my_config.warning.To2), Size) != 0))
        {
            chg |= 0x00020;
            memset(my_config.warning.To2, 0x00, sizeof(my_config.warning.To2));
            memcpy(my_config.warning.To2, tmp, (size_t)Size);
        }
    }

    Size = ParamAdrs("TO3", &tmp);
    max = sizeof(my_config.warning.To3);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }
        else if((Memcmp(my_config.warning.To3, tmp, (int)strlen(my_config.warning.To3), Size) != 0))
        {
            chg |= 0x00040;
            memset(my_config.warning.To3, 0x00, sizeof(my_config.warning.To3));
            memcpy(my_config.warning.To3, tmp, (size_t)Size);
        }
    }

    Size = ParamAdrs("TO4", &tmp);
    max = sizeof(my_config.warning.To4);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }
        else if((Memcmp(my_config.warning.To4, tmp, (int)strlen(my_config.warning.To4), Size) != 0))
        {
            chg |= 0x00080;
            memset(my_config.warning.To4, 0x00, sizeof(my_config.warning.To4));
            memcpy(my_config.warning.To4, tmp, (size_t)Size);
        }
    }

    Value = ParamInt("FN1");
    if(Value>=0 && Value <= 255){ //2020.01.17 条件式 || を &&に変更
        if(my_config.warning.Fn1[0] != (char)Value)
        {
            chg |= 0x0100;
            my_config.warning.Fn1[0] = (char)Value;
        }
    }
    else if (Value == INT32_MAX){
        ;//err |= CMD_NOP;
    }
    else{
        err |= CMD_PER;    // != -> |= 修正 2020.01.20
    }

    Value = ParamInt("FN2");
    if(Value>=0 && Value <= 255){ //2020.01.17 条件式 || を &&に変更
        if(my_config.warning.Fn2[0] != (char)Value)
        {
            chg |= 0x0200;
            my_config.warning.Fn2[0] = (char)Value;
        }
    }
    else if (Value == INT32_MAX){
        ;//err |= CMD_NOP;
    }
    else{
        err |= CMD_PER;
    }

    Value = ParamInt("FN3");
    if(Value>=0 && Value <= 255){ //2020.01.17 条件式 || を &&に変更
        if(my_config.warning.Fn3[0] != (char)Value)
        {
            chg |= 0x0400;
            my_config.warning.Fn3[0] = (char)Value;
        }
    }
    else if (Value == INT32_MAX){
        ;//err |= CMD_NOP;
    }
    else{
        err |= CMD_PER;
    }

    Value = ParamInt("FN4");
    if(Value>=0 && Value <= 255){ //2020.01.17 条件式 || を &&に変更
        if(my_config.warning.Fn4[0] != (char)Value)
        {
            chg |= 0x0800;
            my_config.warning.Fn4[0] = (char)Value;
        }
    }
    else if (Value == INT32_MAX){
        ;//err |= CMD_NOP;
    }
    else{
        err |= CMD_PER;
    }

    Value = ParamInt("EXT");
    if(Value>=0 && Value <= 255){ //2020.01.17 条件式 || を &&に変更
        if(my_config.warning.Ext[0] != (char)Value)
        {
            chg |= 0x1000;
            my_config.warning.Ext[0] = (char)Value;
        }
    }
    else if (Value == INT32_MAX){
        ;//err |= CMD_NOP;
    }
    else{
        err |= CMD_PER;
    }

    Value = ParamInt("ROUTE");
    if(/*Value==0 || */Value == 2 || Value == 3){
        if(my_config.warning.Route[0] != (char)Value)
        {
            chg |= 0x2000;
            my_config.warning.Route[0] = (char)Value;
        }
    }
    else if (Value == INT32_MAX){
        ;//err |= CMD_NOP;
    }
    else{
        err |= CMD_PER;
    }

    if((err == 0x0000) && (my_config.warning.Enable[0] == 0)){
        LED_Set( LED_WARNING, LED_OFF );
        EX_WRAN_OFF;
    }

    if((chg != 0x0000) && (err == 0x0000) )                      // 内容の変更があったら、データを書き込む
     {
        my_config.device.SysCnt++;                              //親機設定変更カウンタ更新 sakaguchi UT-0027

        rewrite_settings();     //本体設定書き換え（関数内でmutex_sfmemを取得返却している)
        read_my_settings();     //本体設定をＳＲＡＭへ読み込み

        //vhttp_sysset_sndon();                                   //親機設定送信 sakaguchi cg UT-0026
        if(HTTP_SEND == Http_Use) vhttp_sysset_sndon();         // 親機設定送信 // sakaguchi 2020.12.24
     }
     else{
         read_my_settings();     //本体設定をＳＲＡＭに読み直し
     }

    return CMD_RES(err);

}




/**
 * [RWARP] 警報設定の読み込み
 * @return
 */
uint32_t CFReadWarning(void)
{
    Printf("    [RWARP exec] \r\n");

    //my_config.warning.Enable[0] = 0;      // debug
    //my_config.warning.Interval[0] = 5;
    //my_config.warning.Route[0] = 3;

    StatusPrintf_v2s( "ENABLE", my_config.warning.Enable,   sizeof(my_config.warning.Enable),   "%d");
    StatusPrintf_v2s( "ROUTE",  my_config.warning.Route,    sizeof(my_config.warning.Route),    "%d");
    StatusPrintf_v2s( "INT",    my_config.warning.Interval, sizeof(my_config.warning.Interval), "%d");

    //StatusPrintf_S(sizeof(my_config.warning.Subject),   "SBJ",      "%s",    my_config.warning.Subject);
    //StatusPrintf_S(sizeof(my_config.warning.Body),      "BODY",     "%s",    my_config.warning.Body);

    //StatusPrintf_S(sizeof(my_config.warning.To1),       "TO1",      "%s",    my_config.warning.To1);
    //StatusPrintf_S(sizeof(my_config.warning.To2),       "TO2",      "%s",    my_config.warning.To2);
    //StatusPrintf_S(sizeof(my_config.warning.To3),       "TO3",      "%s",    my_config.warning.To3);
    //StatusPrintf_S(sizeof(my_config.warning.To4),       "TO4",      "%s",    my_config.warning.To4);

    //StatusPrintf_v2s( "FN1",    my_config.warning.Fn1,      sizeof(my_config.warning.Fn1), "%d");
    //StatusPrintf_v2s( "FN2",    my_config.warning.Fn2,      sizeof(my_config.warning.Fn2), "%d");
    //StatusPrintf_v2s( "FN3",    my_config.warning.Fn3,      sizeof(my_config.warning.Fn3), "%d");
    //StatusPrintf_v2s( "FN4",    my_config.warning.Fn4,      sizeof(my_config.warning.Fn4), "%d");
    StatusPrintf_v2s( "EXT",    my_config.warning.Ext,      sizeof(my_config.warning.Ext), "%d");



    return ERR(CMD,NOERROR);
}


/**
 *  [WCURP] モニタ設定
 * @return
 * @note    LO_HIマクロ廃止
 */
uint32_t CFWriteMonitor(void)
{
    int Size, Value, max;
    char *dir, *fname, *tmp;
    uint16_t word_data_a, word_data_b;

    uint16_t chg = 0x0000;
    uint16_t err = 0x0000;

    Value = ParamInt("ENABLE");
    if(Value==0 || Value == 1){
        if(my_config.monitor.Enable[0] != (char)Value)
        {
            chg |= 0x0001;
            my_config.monitor.Enable[0] = (char)Value;
        }
    }
    else if(Value == INT32_MAX){
        ;//err |= CMD_NOP;
    }
    else{
        err |= CMD_PER;
    }

    Value = ParamInt("ROUTE");
    if(Value >= 1 && Value <= 3){                       //2020.04.08 0:Emailを削除
        if(my_config.monitor.Route[0] != (char)Value)
        {
            chg |= 0x0002;
            my_config.monitor.Route[0] = (char)Value;
        }
    }
    else if(Value == INT32_MAX){
        ;//err |= CMD_NOP;
    }
    else{
        err |= CMD_PER;
    }


    Value = ParamInt("BIND");
    if(Value==0 || Value == 1){
        if(my_config.monitor.Bind[0] != (char)Value)
        {
            chg |= 0x0004;
            my_config.monitor.Bind[0] = (char)Value;
        }
    }
    else if(Value == INT32_MAX){
        ;//err |= CMD_NOP;
    }
    else{
        err |= CMD_PER;
    }

    Value = ParamInt("INT");
    if(Value != INT32_MAX)
    {
        if( 1 <= Value && Value <= 1440 ){
            word_data_a = (uint16_t)Value;
            word_data_b = *(uint16_t *)&my_config.monitor.Interval[0];
            if(word_data_a != word_data_b){
                chg |= 0x0008;
                *(uint16_t *)&my_config.monitor.Interval[0] = word_data_a;
            }
        }
        else{
            err |= CMD_PER;
        }
    }

    Size = ParamAdrs("DIR", &dir);
    max = sizeof(my_config.monitor.Dir);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }
        else if((Memcmp(my_config.monitor.Dir, dir, (int)strlen(my_config.monitor.Dir) ,Size) != 0))
        {
            chg |= 0x0010;
            memset(my_config.monitor.Dir, 0x00, sizeof(my_config.monitor.Dir));
            memcpy(my_config.monitor.Dir, dir, (size_t)Size);
        }
    }

    Size = ParamAdrs("FNAME", &fname);
    max = sizeof(my_config.monitor.Fname);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }
        else if((Memcmp(my_config.monitor.Fname, fname, (int)strlen(my_config.monitor.Fname), Size) != 0))
        {
            chg |= 0x0020;
            memset(my_config.monitor.Fname, 0x00, sizeof(my_config.monitor.Fname));
            memcpy(my_config.monitor.Fname, fname, (size_t)Size);
        }
    }

    Size = ParamAdrs("SBJ", &tmp);
    max = sizeof(my_config.monitor.Subject);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }
        else if((Memcmp(my_config.monitor.Subject, tmp, (int)strlen(my_config.monitor.Subject), Size) != 0))
        {
            chg |= 0x0100;
            memset(my_config.monitor.Subject, 0x00, sizeof(my_config.monitor.Subject));
            memcpy(my_config.monitor.Subject, tmp, (size_t)Size);
        }
    }

    Size = ParamAdrs("TO1", &tmp);
    max = sizeof(my_config.monitor.To1);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }
        else if((Memcmp(my_config.monitor.To1, tmp, (int)strlen(my_config.monitor.To1), Size) != 0))
        {
            chg |= 0x0200;
            memset(my_config.monitor.To1, 0x00, sizeof(my_config.monitor.To1));
            memcpy(my_config.monitor.To1, tmp, (size_t)Size);
        }
    }

    /*
    Size = ParamAdrs("TO2", &tmp);
    max = sizeof(my_config.monitor.To2);
    if(Size != -1)
    {
        if(Size > max)
            err |= CMD_PER;
        else if((memcmp(my_config.monitor.To2, tmp, (size_t)Size) != 0))
        {
            chg |= 0x0400;
            memset(my_config.monitor.To2, 0x00, sizeof(my_config.monitor.To2));
            memcpy(my_config.monitor.To2, tmp, (size_t)Size);
        }
    }
    */

    Size = ParamAdrs("ATTACH", &tmp);
    max = sizeof(my_config.monitor.Attach);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }
        else if((Memcmp(my_config.monitor.Attach, tmp, (int)strlen(my_config.monitor.Attach), Size) != 0))
        {
            chg |= 0x0800;
            memset(my_config.monitor.Attach, 0x00, sizeof(my_config.monitor.Attach));
            memcpy(my_config.monitor.Attach, tmp, (size_t)Size);
        }
    }



    if((chg != 0x0000) && (err == 0x0000))                                           // 内容の変更があったら、データを書き込む
     {
        my_config.device.SysCnt++;                              //親機設定変更カウンタ更新 sakaguchi UT-0027

         rewrite_settings();     //本体設定書き換え（関数内でmutex_sfmemを取得返却している)
         read_my_settings();     //本体設定をＳＲＡＭへ読み込み

        //vhttp_sysset_sndon();                                   //親機設定送信 sakaguchi cg UT-0026
        if(HTTP_SEND == Http_Use) vhttp_sysset_sndon();         // 親機設定送信 // sakaguchi 2020.12.24

     }
     else{
          read_my_settings();     //本体設定をＳＲＡＭへ読み込み
     }

    return CMD_RES(err);

}




/**
 * [RCURP] モニタ設定の読み込み
 * @return
 */
uint32_t CFReadMonitor(void)
{

    StatusPrintf_v2s( "ENABLE", my_config.monitor.Enable,   sizeof(my_config.monitor.Enable), "%d");
    StatusPrintf_v2s( "ROUTE",  my_config.monitor.Route,    sizeof(my_config.monitor.Route), "%d");
    StatusPrintf_v2s( "INT",    my_config.monitor.Interval, sizeof(my_config.monitor.Interval), "%d");

    //StatusPrintf_v2s( "BIND",   my_config.monitor.Bind, sizeof(my_config.monitor.Bind), "%d");

    //StatusPrintf_S(sizeof(my_config.monitor.Subject),   "SBJ",      "%s",    my_config.monitor.Subject);
    //StatusPrintf_S(sizeof(my_config.monitor.To1),       "TO1",      "%s",    my_config.monitor.To1);
    //StatusPrintf_S(sizeof(my_config.monitor.To2),       "TO2",      "%s",    my_config.monitor.To2);

    //StatusPrintf_S(sizeof(my_config.monitor.Attach),    "ATTACH",   "%s",    my_config.monitor.Attach);
    StatusPrintf_S(sizeof(my_config.monitor.Dir),       "DIR",      "%s",    my_config.monitor.Dir);
    StatusPrintf_S(sizeof(my_config.monitor.Fname),     "FNAME",    "%s",    my_config.monitor.Fname);

    return ERR(CMD,NOERROR);
}



/**
 * [WSUCP] 吸い上げの設定
 * @return
 */
uint32_t CFWriteSuction(void)
{
    int Size, Value,max;
    char *evt, *sbj, *to, *tmp;
 //   LO_HI a;

    uint16_t chg = 0x0000;
    uint8_t err = 0x00;

    Value = ParamInt("ENABLE");
    if((Value==0) || (Value == 1)){
        if(my_config.suction.Enable[0] != (char)Value)
        {
            chg |= 0x0001;
            my_config.suction.Enable[0] = (char)Value;
        }
    }
    else if(Value == INT32_MAX){
        ;//err |= CMD_NOP;
    }
    else{
        err |= CMD_PER;
    }

    Value = ParamInt("ROUTE");
    if(/*(Value==0) || */(Value == 1) || (Value == 3)){     // Emailを削除
        if(my_config.suction.Route[0] != (char)Value)
        {
            chg |= 0x0002;
            my_config.suction.Route[0] = (char)Value;
        }
    }
    else if(Value == INT32_MAX){
        ;//err |= CMD_NOP;
    }
    else{
        err |= CMD_PER;
    }

    /*
    Value = ParamInt("TYPE");
    if(Value==0 || Value == 1){
        if(my_config.suction.FileType[0] != (uint8_t)Value)
        {
            chg |= 0x0004;
            my_config.suction.FileType[0] = (uint8_t)Value;
        }
    }
    else if(Value == INT32_MAX)
        err |= CMD_NOP;
    else
        err |= CMD_PER;
    */

    Size = ParamAdrs("EVT0", &evt);
    max = sizeof(my_config.suction.Event0);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }
        else if((Memcmp(my_config.suction.Event0, evt, (int)strlen(my_config.suction.Event0), Size) != 0))
        {
           chg |= 0x0008;
           memset(my_config.suction.Event0, 0x00, sizeof(my_config.suction.Event0));
           memcpy(my_config.suction.Event0, evt, (size_t)Size);
       }
    }


    Size = ParamAdrs("EVT1", &evt);
    max = sizeof(my_config.suction.Event1);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }
        else if((Memcmp(my_config.suction.Event1, evt, (int)strlen(my_config.suction.Event1), Size) != 0))
        {
           chg |= 0x0008;
           memset(my_config.suction.Event1, 0x00, sizeof(my_config.suction.Event1));
           memcpy(my_config.suction.Event1, evt, (size_t)Size);
        }
    }

    Size = ParamAdrs("EVT2", &evt);
    max = sizeof(my_config.suction.Event2);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }
        else if((Memcmp(my_config.suction.Event2, evt, (int)strlen(my_config.suction.Event2), Size) != 0))
        {
            chg |= 0x0020;
            memset(my_config.suction.Event2, 0x00, sizeof(my_config.suction.Event2));
            memcpy(my_config.suction.Event2, evt, (size_t)Size);
        }
    }

    Size = ParamAdrs("EVT3", &evt);
    max = sizeof(my_config.suction.Event3);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }
        else if((Memcmp(my_config.suction.Event3, evt, (int)strlen(my_config.suction.Event3), Size) != 0))
        {
            chg |= 0x0040;
            memset(my_config.suction.Event3, 0x00, sizeof(my_config.suction.Event3));
            memcpy(my_config.suction.Event3, evt, (size_t)Size);
        }
    }

    Size = ParamAdrs("EVT4", &evt);
    max = sizeof(my_config.suction.Event4);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;     //2020.01.17 修正
        }
        else if((Memcmp(my_config.suction.Event4, evt, (int)strlen(my_config.suction.Event4), Size) != 0))
        {
            chg |= 0x0080;
            memset(my_config.suction.Event4, 0x00, sizeof(my_config.suction.Event4));
            memcpy(my_config.suction.Event4, evt, (size_t)Size);
        }
    }

    Size = ParamAdrs("EVT5", &evt);
    max = sizeof(my_config.suction.Event5);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }
        else if((Memcmp(my_config.suction.Event5, evt, (int)strlen(my_config.suction.Event5), Size) != 0))
        {
            chg |= 0x0100;
            memset(my_config.suction.Event5, 0x00, sizeof(my_config.suction.Event5));
            memcpy(my_config.suction.Event5, evt, (size_t)Size);
        }
    }

    Size = ParamAdrs("EVT6", &evt);
    max = sizeof(my_config.suction.Event6);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }
        else if((Memcmp(my_config.suction.Event6, evt, (int)strlen(my_config.suction.Event6), Size) != 0))
        {
            chg |= 0x0200;
            memset(my_config.suction.Event6, 0x00, sizeof(my_config.suction.Event6));
            memcpy(my_config.suction.Event6, evt, (size_t)Size);
        }
    }

    Size = ParamAdrs("EVT7", &evt);
    max = sizeof(my_config.suction.Event7);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }
        else if((Memcmp(my_config.suction.Event7, evt, (int)strlen(my_config.suction.Event7), Size) != 0))
        {
            chg |= 0x0400;
            memset(my_config.suction.Event7, 0x00, sizeof(my_config.suction.Event7));
            memcpy(my_config.suction.Event7, evt, (size_t)Size);
        }
    }

    Size = ParamAdrs("EVT8", &evt);
    max = sizeof(my_config.suction.Event8);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }
        else if((Memcmp(my_config.suction.Event8, evt, (int)strlen(my_config.suction.Event8), Size) != 0))
        {
            chg |= 0x0800;
            memset(my_config.suction.Event8, 0x00, sizeof(my_config.suction.Event8));
            memcpy(my_config.suction.Event8, evt, (size_t)Size);
        }
    }

    Size = ParamAdrs("SBJ", &sbj);
    max = sizeof(my_config.suction.Subject);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }
        else if((Memcmp(my_config.suction.Subject, sbj, (int)strlen(my_config.suction.Subject), Size) != 0))
        {
           chg |= 0x1000;
           memset(my_config.suction.Subject, 0x00, sizeof(my_config.suction.Subject));
           memcpy(my_config.suction.Subject, sbj, (size_t)Size);
        }
    }

    Size = ParamAdrs("TO1", &to);
    max = sizeof(my_config.suction.To1);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }
        else if((Memcmp(my_config.suction.To1, to, (int)strlen(my_config.suction.To1), Size) != 0))
        {
           chg |= 0x2000;
           memset(my_config.suction.To1, 0x00, sizeof(my_config.suction.To1));
           memcpy(my_config.suction.To1, to, (size_t)Size);
        }
    }

    Size = ParamAdrs("ATTACH", &tmp);
    max = sizeof(my_config.suction.Attach);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }
        else if((Memcmp(my_config.suction.Attach, tmp, (int)strlen(my_config.suction.Attach), Size) != 0))
        {
           chg |= 0x2000;
           memset(my_config.suction.Attach, 0x00, sizeof(my_config.suction.Attach));
           memcpy(my_config.suction.Attach, tmp, (size_t)Size);
        }
    }

    Size = ParamAdrs("DIR", &tmp);
    max = sizeof(my_config.suction.Dir);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }
        else if((Memcmp(my_config.suction.Dir, tmp, (int)strlen(my_config.suction.Dir), Size) != 0))
        {
           chg |= 0x2000;
           memset(my_config.suction.Dir, 0x00, sizeof(my_config.suction.Dir));
           memcpy(my_config.suction.Dir, tmp, (size_t)Size);
        }
    }

    Size = ParamAdrs("FNAME", &tmp);
    max = sizeof(my_config.suction.Fname);
    if(Size != -1)
    {
        if(Size > max){
            err |= CMD_PER;
        }
        else if((Memcmp(my_config.suction.Fname, tmp, (int)strlen(my_config.suction.Fname), Size) != 0))
        {
           chg |= 0x2000;
           memset(my_config.suction.Fname, 0x00, sizeof(my_config.suction.Fname));
           memcpy(my_config.suction.Fname, tmp, (size_t)Size);
        }
    }


    Printf("WNETP %02X %04X \r\n", err, chg);

    if((chg != 0x00) && (err == 0x00))                                           // 内容の変更があったら、データを書き込む
    {
        my_config.device.SysCnt++;      //親機設定変更カウンタ更新 sakaguchi UT-0027
        rewrite_settings();             //本体設定書き換え（関数内でmutex_sfmemを取得返却している)
        read_my_settings();             //本体設定をＳＲＡＭへ読み込み
// 2023.05.26 ↓
        if(chg & 0x0001){               // 記録データ送信有無に変更がある場合
            kk_delta_set_all(0x01);     // 全子機の記録データを次回全データ吸い上げとする
            if(WaitRequest != WAIT_REQUEST){
                Printf("WSETF AT Start (FLG_EVENT_INITIAL) 2\r\n");
                tx_event_flags_set (&g_wmd_event_flags, FLG_EVENT_INITIAL, TX_OR);      // AT Start起動（登録情報変更時）
                mate_at_start = 0;
            }else{
                mate_at_start = 1;
            }
        }
// 2023.05.26 ↑
        //vhttp_sysset_sndon();           //親機設定送信
        if(HTTP_SEND == Http_Use) vhttp_sysset_sndon();         // 親機設定送信 // sakaguchi 2020.12.24

    }
    else{
        read_my_settings();             //本体設定をＳＲＡＭへ読み込み
    }

    return CMD_RES(err);

}



/**
 * [RSUCP] 吸い上げ設定の読み込み
 * @return
 */
uint32_t CFReadSuction(void)
{

    StatusPrintf_v2s( "ENABLE", my_config.suction.Enable, sizeof(my_config.suction.Enable), "%d");
    StatusPrintf_v2s( "ROUTE",  my_config.suction.Route, sizeof(my_config.suction.Route), "%d");
    //StatusPrintf_v2s( "TYPE",   (char *)&my_config.suction.FileType, sizeof(my_config.suction.FileType), "%d");

    //StatusPrintf_S(sizeof(my_config.suction.Subject),   "SBJ",    "%s",    my_config.suction.Subject);

    //StatusPrintf_S(sizeof(my_config.suction.To1),       "TO1",    "%s",    my_config.suction.To1);

    StatusPrintf_S(sizeof(my_config.suction.Event0),    "EVT0",   "%s",    my_config.suction.Event0);
    StatusPrintf_S(sizeof(my_config.suction.Event1),    "EVT1",   "%s",    my_config.suction.Event1);
    StatusPrintf_S(sizeof(my_config.suction.Event2),    "EVT2",   "%s",    my_config.suction.Event2);
    StatusPrintf_S(sizeof(my_config.suction.Event3),    "EVT3",   "%s",    my_config.suction.Event3);
    StatusPrintf_S(sizeof(my_config.suction.Event4),    "EVT4",   "%s",    my_config.suction.Event4);
    StatusPrintf_S(sizeof(my_config.suction.Event5),    "EVT5",   "%s",    my_config.suction.Event5);
    StatusPrintf_S(sizeof(my_config.suction.Event6),    "EVT6",   "%s",    my_config.suction.Event6);
    StatusPrintf_S(sizeof(my_config.suction.Event7),    "EVT7",   "%s",    my_config.suction.Event7);
    StatusPrintf_S(sizeof(my_config.suction.Event8),    "EVT8",   "%s",    my_config.suction.Event8);

    //StatusPrintf_S(sizeof(my_config.suction.Attach),    "ATTACH",   "%s",  my_config.suction.Attach);     // 2019/11/27 追加segi

    StatusPrintf_S(sizeof(my_config.suction.Dir),       "DIR",    "%s",    my_config.suction.Dir);
    StatusPrintf_S(sizeof(my_config.suction.Fname),     "FNAME",  "%s",    my_config.suction.Fname);



    return ERR(CMD,NOERROR);
}






//########## 光通信 ###########################################################

/**
 * 光通信 子機登録  [EIREG]
 * @return
 * @note    AnalyzeCmd() に戻る
 */
uint32_t CFExtentionReg(void)
{
    int     sz_id, sz_name;
    uint16_t num, ch, band;
    char     *id, *name;
    uint32_t    Err = ERR(CMD, FORMAT);

    sz_id = ParamAdrs("ID", &id);
    sz_name = ParamAdrs("NAME", &name);
    num = (uint16_t)ParamInt("NUM");
    ch = (uint16_t)ParamInt("CH");
    band = (uint16_t)ParamInt("BAND");
    if(band == INT16_MAX){
        band = 0;                               // 省略時は0
    }
    if(band > 7){
        band = 0xFF;
    }

    if((sz_id == 8) && (sz_name <= 8) && (ch < 256) && ((1 <= num) && (num <= 250)))
    {
        memcpy(optc.gid, id, sizeof(optc.gid));
        memset(optc.name, 0x00, sizeof(optc.name));
        memcpy(optc.name, name, (size_t)sz_name);

        optc.num = (char)num;
        optc.ch = (char)ch;
        optc.band = (char)band;

        Printf("EIREG id:%02X ch:%d num:%d band:%d %s\r\n", id[0],ch,num,band,name);

        Err = IR_Regist();      //子機登録

        //Err = ERR(IR, NOERROR);
    }



    return (Err);
}

/**
 * 光通信 シリアル番号の取得  [EISER]
 * @return
 * @note    AnalyzeCmd() に戻る
 */
uint32_t CFExtentionSerial(void)
{
    uint32_t Err;

    Printf("CMD:EISER \r\n");


//    Err = IR_Nomal_command(0xb3, 0, NULL);        //光通信 SOH　通常コマンド
    Err = IR_Nomal_command(0xb3, 0, 0, NULL);        //光通信 SOH　通常コマンド
    if(Err == ERR(IR, NOERROR))
    {

        StatusPrintf( "SER", "%.2X%.2X%.2X%.2X", pOptUartRx->data[3] & 0xff, pOptUartRx->data[2] & 0xff, pOptUartRx->data[1] & 0xff, pOptUartRx->data[0] & 0xff);

    }

    return (Err);
}

/**
 * 光通信 設定値の読み書き  [EISET]
 * @return
 * @note    AnalyzeCmd() に戻る
 */
uint32_t CFExtSettings(void)
{

    uint32_t    Err;
    char        *data;
    int32_t     sz_data, act;
    uint16_t    spec;                          // 2023.08.07

    Err = ERR(CMD, FORMAT);

    Printf("\r\n EISET exec \r\n");

    act = ParamInt("ACT");
    sz_data = ParamAdrs("DATA", &data);

    switch(act)
    {

        case 0: // 書き込み

            if(sz_data <= 64){
                Err = IR_64byte_write(0x3c, data);  //64Byte書き込み（設定テーブル）
            }
// 2023.08.07 ↓
            if(Err == ERR(IR, NOERROR)){
                Printf("EISET Read!\r\n");
                Err = IR_header_read(0x48, 0);      //データ吸い上げ要求(設定テーブル読み出し)
                if(Err == ERR(IR, NOERROR)){
                    spec = GetSpecNumber(ru_header.serial_no,ru_header.kisyu_code);
                    switch(spec){
                        case SPEC_501:
                        case SPEC_502:
                        case SPEC_502PT:
                        case SPEC_503:
                        case SPEC_507:
                        case SPEC_574:
                        case SPEC_576:
                        case SPEC_501B:
                        case SPEC_502B:
                        case SPEC_503B:
                        case SPEC_507B:
                            if((*(data+17) & 0x01) == 0){       // 設定変更フラグ（記録開始）
                                kk_delta_set(ru_header.serial_no, 0x01);     // 変化あり
                            }
                            break;
                        case SPEC_505PS:
                        case SPEC_505TC:
                        case SPEC_505PT:
                        case SPEC_505V:
                        case SPEC_505A:
                        case SPEC_505BPS:
                        case SPEC_505BTC:
                        case SPEC_505BPT:
                        case SPEC_505BV:
                        case SPEC_505BA:
                        case SPEC_505BLX:
                            if(((*(data+30) & 0x01) != 0)       // 設定変更フラグ（記録開始）
                            || ((*(data+30) & 0x10) != 0)){     // 設定変更フラグ（表示設定）
                                kk_delta_set(ru_header.serial_no, 0x01);     // 変化あり
                            }
                            break;
                        case SPEC_601:          // RTR601
                        case SPEC_602:          // RTR602
                        default:
                            break;
                    }
                }
            }
// 2023.08.07 ↑
            break;

        case 1: // 読み込み

            Printf("EISET Read!\r\n");
            Err = IR_header_read(0x48, 0);      //データ吸い上げ要求(設定テーブル読み出し)
            if(Err == ERR(IR, NOERROR)){
                StatusPrintfB("DATA", ru_header.intt, 64);      //子機 ヘッダ 64Byte
            }

            break;

        default:
            break;
    }

    Printf("EISET exec end %ld \r\n", Err);

    return (Err);

}


/**
 *  光通信 データ吸い上げ [EISUC]
 * @return
 * @note  AnalyzeCmd() に戻る
 */
uint32_t CFExtSuction(void)
{
    uint32_t    Err;
    int32_t     range,mode;
    uint8_t     sw, para;
    uint16_t    cmd;


    Err = ERR(CMD, FORMAT);

    mode = ParamInt("MODE");
    range = ParamInt32("RANGE");

    Printf("[[ EISUC ]] mode %d  %d \r\n", mode, range);

    if(mode == INT32_MAX){
        mode = 9;
    }
    if((mode != 9) && (range == INT32_MAX)){         // パラメータ無し？
        return (Err);
    }

    cmd = (uint16_t)CmdStatusSize;                                        // DATA=の次の場所
    StatusPrintfB("DATA", huge_buffer, 0);                      // dummy

    Printf(" cmd = %d \r\n", cmd);

    switch(mode)
    {
        case 0:                 // データ個数
            para = 0x47;
            sw = 0;
            break;

        case 1:                 // データ番号
            para = 0x49;
            sw = 0;
            break;

        case 2:                 // 期間指定
            para = 0x48;
            sw = 0;
            break;

        case 9:                 // 記録開始情報
            para = 0x00;
            sw = 1;
            range = 0;
            break;

        default:
            sw = 2;
            break;
    }

    Printf(" cmd = %02X sw = %d range = %d mode = %d \r\n", cmd, sw, range, mode);

    if(sw <= 1)                                                 // コマンドOK?
    {
        Err = IR_data_download_t(&StsArea[cmd+7], sw, para, (uint64_t)range);

        if(Err < ERRCAT_BASE_SYS)                               // エラーステータスではない？
        {
            if(sw == 0)                                         // 初回
            {
                StsArea[cmd+5] = 64;
                StsArea[cmd+6] = 0;                             // 返送データは64バイト
                CmdStatusSize += 64;
                StatusPrintf("SIZE", "%u", (uint32_t)Err);         // SIZEパラメータを返す
            }
            else                                                // ２回目以降
            {
                StsArea[cmd+5] = (char)Err;
                StsArea[cmd+6] = (char)(Err >> 8);
                CmdStatusSize += (int)Err;
            }

            Err = ERR(IR, NOERROR);
        }
    }

    if (Err != ERR(IR, NOERROR)){
        CmdStatusSize = cmd;           // エラーなら元に戻す
    }

    return (Err);
}

/**
 * 光通信 電波制御 [EIWRS]
 * @return
 * @note    AnalyzeCmd() に戻る
 */
uint32_t CFExtWirelessOFF(void)
{
    uint32_t Err = ERR(CMD, FORMAT);
    uint8_t mode;


    mode = (uint8_t)ParamInt("MODE");

    if((mode == 0) || (mode == 1))
    {
        Err = ERR(IR, NORES);

//        Err = IR_Nomal_command(0x20, 0, NULL);    //光通信 SOH　通常コマンド
        Err = IR_Nomal_command(0x20, 0, 0, NULL);    //光通信 SOH　通常コマンド
    }

    return (Err);
}

/**
 *  光通信 記録停止 [EIESP]
 * @return
 * @note    AnalyzeCmd() に戻る
 */
uint32_t CFExtRecStop(void)
{
    uint32_t Err = ERR(CMD, FORMAT);


    if(ParamInt("MODE") == 1)
    {
        Err = ERR(IR, NORES);
//        Err = IR_Nomal_command(0x32, 0, NULL);    //光通信 SOH　通常コマンド
        Err = IR_Nomal_command(0x32, 0, 0, NULL);    //光通信 SOH　通常コマンド
     }

    return (Err);
}

/**
 *  光通信 アジャストメント [EIADJ]
 * @return
 * @note    AnalyzeCmd() に戻る
 */
uint32_t CFExtAdjust(void)
{
    uint32_t   Err;
    char   *data;
    int     sz_data, act;


    Err = ERR(CMD, FORMAT);

    act = ParamInt("ACT");
    sz_data = ParamAdrs("DATA", &data);

    switch(act)
    {
        case 0:             // 書き込み

            if(sz_data == 64){
                Err = IR_64byte_write(0x58, data);
            }
            break;

        case 1:             // 読み込み

            Err = IR_64byte_read(0x59, huge_buffer);
            if(Err == ERR(IR, NOERROR)){
                StatusPrintfB("DATA", huge_buffer, 64);
            }
            break;

        default:
            break;
    }

    return (Err);
}

/**
 *  光通信 スケール変換 [EISCL]
 * @return
 * @note    AnalyzeCmd() に戻る
 */
uint32_t CFExtScale(void)
{
    uint32_t   Err;
    char   *data;
    int    sz_data, act;


    Err = ERR(CMD, FORMAT);

    act = ParamInt("ACT");
    sz_data = ParamAdrs("DATA", &data);

    switch(act)
    {

        case 0:             // 書き込み

            if(sz_data == 64){
                Err = IR_64byte_write(0x6b, data);
            }
            break;

        case 1:             // 読み込み

            Err = IR_64byte_read(0x6c, huge_buffer);
            if(Err == ERR(IR, NOERROR)){
                StatusPrintfB("DATA", huge_buffer, 64);
            }
            break;

        default:
            break;

    }

    return (Err);
}


/**
 *  光通信 ＳＯＨコマンドの送受信 [EIBLP]
 * @return
 * @note    AnalyzeCmd() に戻る
 */
uint32_t CFExtBlp(void)
{
    uint32_t   Err;
    int act, mode;
    int sz_id, sz_name;
    char *name, *id;
    int len;
//    int i;
    uint16_t chg = 0x0000;
    char txData[26];
    memset(txData, 0x00, sizeof(txData));

    Printf("===>[EIBLP]  Start \r\n");

    Err = ERR(CMD, FORMAT);

    act = ParamInt("ACT");
    mode = ParamInt("ENABLE");

    sz_id = ParamAdrs("PASC", &id);     //未使用
    sz_name = ParamAdrs("NAME", &name);

    switch(act)
    {
        case 0:                                         // 設定
            // 機種名
            if(sz_name != -1){
                if(0<=sz_name && sz_name <= 26)
                {
                    chg |= 0x0001;
                }
                else{
                    goto Exit;
                }
            }
            // Bluetooth機能 有効・無効
            if(mode!=INT32_MAX){
                if(mode==0 || mode==1)
                {
                    chg |= 0x0002;
                }
                else{
                    goto Exit;
                }
            }
Printf("[EIBLP] chg %d %04X \r\n", sz_id, chg);
            // PASS code 現在未使用
            if(sz_id != -1){
                if(sz_id == 4)
                {
                    chg |= 0x0004;
                }
                else{
                    goto Exit;
                }
            }

            Printf("[EIBLP] chg %d %04X \r\n", sz_id, chg);

       //光コマンド送受信処理
            if(chg & 0x0001){  //機種名変更
 //               memset(OCOM.txbuf.data, 0x00, 26);
                memset(txData, 0x00, sizeof(txData));

                if(sz_name > 0){    //念のため
 //                   memcpy(OCOM.txbuf.data, name, (size_t)sz_name);
                    memcpy(txData, name, (size_t)sz_name);
                }

                for(int i = 0 ;i < 26; i++){
//                    Printf("%02X ",OCOM.txbuf.data[i]);
                    Printf("%02X ",txData[i]);
                }
                Printf("\r\n");

//                Err = IR_Nomal_command(0x61, 26, txData);            //光通信 SOH　通常コマンド 0x61 機器名設定 コマンド
                Err = IR_Nomal_command(0x61, 0, 26, txData);            //光通信 SOH　通常コマンド 0x61 機器名設定 コマンド
                if(Err != ERR(IR, NOERROR)){
                    goto Exit;
                }
            }

            if(chg & 0x0002){   // Bluetooth機能 有効・無効
//                memset(OCOM.txbuf.data, 0x00, 4);
                memset(txData, 0x00, 4);
                if(mode!=1){
//                    OCOM.txbuf.data[0] = 0xff;
                    txData[0] = 0xff;
                }
//                Err = IR_Nomal_command(0x23, 4, txData);           //光通信 SOH　通常コマンド 0x23 コマンド
                Err = IR_Nomal_command(0x23, 0, 4, txData);           //光通信 SOH　通常コマンド 0x23 コマンド
                if(Err != ERR(IR, NOERROR)){
                    goto Exit;
                }
            }

            if(chg & 0x0004){   // Bluetooth Pass Code設定
                Printf("[EIBLP] Pass Code Set\r\n");
 //               memcpy(OCOM.txbuf.data, id, 4);
//                Err = IR_Nomal_command(0x75, 4, id);            //光通信 SOH　通常コマンド 0x75  コマンド
                Err = IR_Nomal_command(0x75, 0, 4, id);            //光通信 SOH　通常コマンド 0x75  コマンド
                if(Err != ERR(IR, NOERROR)){
                    goto Exit;
                }
             }


            break;

        case 1:                                         // 読み込み
//            Err = IR_Nomal_command(0x62, 0, txData);            //光通信 SOH　通常コマンド 0x62 機器名取得コマンド
            Err = IR_Nomal_command(0x62, 0, 0, txData);            //光通信 SOH　通常コマンド 0x62 機器名取得コマンド

            if(Err == ERR(IR, NOERROR))
            {
//                len = OCOM.rxbuf.length;
                len = pOptUartRx->length;
                if(len > 26){
                    len = 26;
                }
//                StatusPrintfB("NAME", (char *)&OCOM.rxbuf.data, len);

                Printf("EIBLP Name \r\n");
                for(int i=0;i<len;i++){
                    Printf("%02X ", pOptUartRx->data[i]);
                }
                Printf("\r\n");
                StatusPrintfB("NAME", pOptUartRx->data, len);

//                Err = IR_Nomal_command(0x9E, 0, txData);            //光通信 SOH　通常コマンド 0x9E BLE状態取得
                Err = IR_Nomal_command(0x9E, 1, 0, txData);            //光通信 SOH　通常コマンド 0x9E サブコマンド 0x01 BLE状態取得
                if(Err == ERR(IR, NOERROR))
                {
 //                   if(OCOM.rxbuf.data[2] == 0x00){         // 有効
//                    if(rxData[2] == 0x00){
                    if(pOptUartRx->data[2] == 0x00){
                        StatusPrintf( "ENABLE", "%s", "1");
                    }
                    else{
                        StatusPrintf( "ENABLE", "%s", "0");
                    }
                }
            }

            break;
        default:
            break;
    }


 Exit:;

    return (Err);
}


/**
 *  光通信 ＳＯＨコマンドの送受信 [EISOH]
 * @return
 * @note    AnalyzeCmd() に戻る
 */
uint32_t CFExtSOH(void)
{
    int32_t sz_data, time;
    char *data;
  //  char *pt;
    uint32_t   Err;
    int i;

    Printf("===>[EISOH]  Start \r\n");

    Err = ERR(CMD, FORMAT);

    time = ParamInt("TIME");
    sz_data = ParamAdrs("DATA", &data);
 //   ParamAdrs("DATA", &pt);

 //1024で切っているが、SUMが切れるので必ずエラーになる →　sizeof(def_comformat)までサイズ拡張(エラーは変わらず)
    if(sz_data > (int32_t)sizeof(def_comformat)){
        sz_data = (int32_t)sizeof(def_comformat);
    }

    //デバッグ用の表示
    for(i=0;i<sz_data;i++){
        Printf("%02X ", data[i]);
    }
    Printf("\r\n");


    if((time == 0) || (time == INT32_MAX)){
        time = 5000;           // 省略時は5000msec
    }

    if(data != 0)
    {
 //       memcpy(&OCOM.txbuf.header, data, (size_t)sz_data);

        Err = IR_soh_direct((comSOH_t *)data, (uint32_t)sz_data);
        Printf("IR soh %02X \r\n", Err);

        switch (Err)
        {
            case ERR(IR, NOERROR):
            case ERR(IR, RESERR):
//              StatusPrintfB("DATA", &OCOM.rxbuf.header, OCOM.rxbuf.length + 7);
                StatusPrintfB("DATA", &pOptUartRx->header, pOptUartRx->length + 7);
                break;

            default:
                break;
        }
    }

    Printf("===>[EISOH]  End \r\n");

    return (Err);
}




/**
 *  子機の電波強度＆電池残量コマンド [EWLEX]
 * @return
 * @note    AnalyzeCmd() に戻る
 */
uint32_t CFExWirelessLevel(void)
{
    char *id, *relay;

    int i, num;
    int32_t act;

    uint32_t Err = ERR(RF, NOERROR);

    act = ParamInt("ACT");
    Printf("[EWLEX ] %ld\r\n", act);

    //switch(ParamInt("ACT"))
    switch(act)
    {
        case 0:

            Err = start_rftask(RF_COMM_REAL_SCAN);              // ＴコマンドパラメータからＲＦタスク起動
            break;

        case 1:

            if((ERRORCODE.ACT0 == ERR(RF, NOERROR)) && (RF_buff.rf_res.status == END_OK) && (memcmp(latest, PLIST.Arg[0].Name, 5) == 0))
            {

                uncmpressData( 0 );    // 圧縮データを解凍 サブ関数化 2020.01.29

                num = (huge_buffer[0] + (uint16_t)huge_buffer[1] * 256) / 30 * 3; // 1データ30byte
                StatusPrintfB("LEVEL", huge_buffer, num);              // いったん出力
                relay = &StsArea[CmdStatusSize - num];
                id = (char *)&huge_buffer[2];

                for (i = 0; i < num; i += 3)
                {
                    relay[i + 0] = id[0];                       // 子機番号
                    relay[i + 1] = ExLevelWL(id[1]);            // 電波レベル
                    relay[i + 2] = ExLevelBatt(id[4] & 0x0F);   // バッテリ残量
                    id += 30;
                }
            }

            Err = ERRORCODE.ACT0;
            ERRORCODE.ACT0 = ERR(RF, NOERROR);

            RF_buff.rf_res.status = NULL_CODE;
            break;

        case 2:
            Err = Sub_Act_2();    //ACT=2の処理 2020.01.29
            break;

        default:
            Err = ERR(CMD, FORMAT);
            break;
    }

    return (Err);
}

/**
 *  中継機の電波強度＆電池残量コマンド [EWLRU]
 * @return
 * @note    AnalyzeCmd() に戻る
 */
uint32_t CFRelayWirelessLevel(void)
{
    char *id, *relay;

    int i, num;

    uint32_t Err = ERR(RF, NOERROR);


    switch(ParamInt("ACT"))
    {
        case 0:

            Err = start_rftask(RF_COMM_REPEATER_SEARCH);        // ＴコマンドパラメータからＲＦタスク起動
            break;

        case 1:

            if((ERRORCODE.ACT0 == ERR(RF, NOERROR)) && (RF_buff.rf_res.status == END_OK) && (memcmp(latest, PLIST.Arg[0].Name, 5) == 0))
            {
                uncmpressData( 0 );    // 圧縮データを解凍 サブ関数化 2020.01.29

                num = r500.data_size / 19 * 3;                      // 1データ19byte
                StatusPrintfB("LEVEL", huge_buffer, num);  // いったん出力
                relay = (char *)&StsArea[CmdStatusSize - num];
                id = huge_buffer;

                for (i = 0; i < num; i += 3)
                {
                    relay[i + 0] = id[0];                   // 子機番号
                    relay[i + 1] = ExLevelWL(id[1]);        // 電波レベル
                    relay[i + 2] = id[18];                  // バッテリ残量
                    id += 19;
                }
            }

            Err = ERRORCODE.ACT0;
            ERRORCODE.ACT0 = ERR(RF, NOERROR);

            RF_buff.rf_res.status = NULL_CODE;
            break;                                              // ＡＣＴ＝１の結果

        case 2:
            Err = Sub_Act_2();    //ACT=2の処理 2020.01.29
            break;

        default:
            Err = ERR(CMD, FORMAT);
            break;
    }

    return(Err);
}


/**
 * 設定値読み込みコマンド[EWSTR]
 * @return
 * @note    AnalyzeCmd() に戻る
 */
uint32_t CFWirelessSettingsRead(void)
{
    uint32_t Err = ERR(RF, NOERROR);


    switch(ParamInt("ACT"))
    {
        case 0:

            Err = start_rftask(RF_COMM_SETTING_READ);           // ＴコマンドパラメータからＲＦタスク起動
            break;

        case 1:

            Printf("Err %ld rf status %02X\r\n", ERRORCODE.ACT0, RF_buff.rf_res.status);

            //if((ERRORCODE.ACT0 == ERR(RF, NOERROR)) && (RF_buff.rf_res.status == END_OK) && (memcmp(latest, PLIST.Arg[0].Name, 5) == 0))
            //if((ERRORCODE.ACT0 == ERR(RF, NOERROR)) && (RF_buff.rf_res.status == RFM_NORMAL_END) && (memcmp(latest, PLIST.Arg[0].Name, 5) == 0))
            if(RF_buff.rf_res.status == END_OK)             // 全データ確保
            {
                uncmpressData( 0 );    // 圧縮データを解凍 サブ関数化 2020.01.29

                Printf("CFWirelessSettingsRead size %d \r\n", r500.data_size);
                StatusPrintfB("DATA", huge_buffer, r500.data_size);    // 取得した設定値
            }

            Err = ERRORCODE.ACT0;
            ERRORCODE.ACT0 = ERR(RF, NOERROR);

            RF_buff.rf_res.status = NULL_CODE;
            break;

        case 2:
            Err = Sub_Act_2();    //ACT=2の処理 2020.01.29
            break;

        default:

            Err = ERR(CMD, FORMAT);
            break;
    }

    return(Err);
}

/**
 * @brief   設定値書き込み[EWSTW]
 * @return  実行結果、エラーコード
 * @note    AnalyzeCmd() に戻る
 */
uint32_t CFWirelessSettingsWrite(void)
{
    uint32_t Err = ERR(RF, NOERROR);
    int32_t act = ParamInt("ACT");

    Printf("[EWSTW] act=%d\r\n", act );
    switch(ParamInt("ACT"))
    {
        case 0:
            EWSTW_utc = 0;                              // [EWSTW]コマンドで開始日時UTCを返す為の変数。応答時0のまま返信不要。  2020/08/29 segi
            Err = start_rftask(RF_COMM_SETTING_WRITE);          // ＴコマンドパラメータからＲＦタスク起動
            break;

        case 1:

            Err = ERRORCODE.ACT0;
            ERRORCODE.ACT0 = ERR(RF, NOERROR);

            RF_buff.rf_res.status = NULL_CODE;

            if(Err == 0x40000){
                WR_set(EWSTW_GNo,ru_registry.rtr501.number,2,0);
                WR_set(EWSTW_GNo,ru_registry.rtr501.number,3,0);
                WR_set(EWSTW_GNo,ru_registry.rtr501.number,4,0);
                RegistTableWrite();         // 無線通信成功したら登録パラメータを更新する
            }

            if(EWSTW_utc != 0){                         // 記録開始UTCを応答する処理   2020/08/29 segi
                StatusPrintf("UTC", "%lu", EWSTW_utc);  // 記録開始UTCを応答する処理   2020/08/29 segi
            }                                           // 記録開始UTCを応答する処理   2020/08/29 segi

            break;

        case 2:
            Err = Sub_Act_2();    //ACT=2の処理 2020.01.29
            break;

        default:

            Err = ERR(CMD, FORMAT);
            break;
    }
    Printf("Err = %04X\r\n");
    return(Err);
}


/**
 * @brief   現在値取得コマンド [EWCUR]
 * @return  実行結果、エラーコード
 * @note    AnalyzeCmd() に戻る
 */
uint32_t CFWirelessMonitor(void)
{
    uint32_t Err = ERR(RF, NOERROR);

    Printf("EWCUR start \r\n");

    switch(ParamInt("ACT"))
    {
        case 0:
            Printf("EWCUR act 0 \r\n");
            Err = start_rftask(RF_COMM_CURRENT_READINGS);       // ＴコマンドパラメータからＲＦタスク起動
            break;

        case 1:
            Printf("EWCUR act 1 %d\r\n", RF_buff.rf_res.status);
            //if((ERRORCODE.ACT0 == ERR(RF, NOERROR)) && (RF_buff.rf_res.status == END_OK) && (memcmp(latest, PLIST.Arg[0].Name, 5) == 0))
            //if((ERRORCODE.ACT0 == ERR(RF, NOERROR)) && (RF_buff.rf_res.status == RFM_NORMAL_END) && (memcmp(latest, PLIST.Arg[0].Name, 5) == 0))
            if(RF_buff.rf_res.status == END_OK)             // 全データ確保
            {
                StatusPrintf("TIME", "%lu", RF_buff.rf_res.time);       // コマンド受信から無線通信開始までの秒数［秒］
                StatusPrintf("LAST", "%lu", timer.int125 * 125uL);      // 無線通信後の経過秒数［ｍｓ］（まだカウント中、次のコマンドが実行されるまで止まらない）

                if(r500.node == COMPRESS_ALGORITHM_NOTICE)      // 圧縮データを解凍
                {
                    memcpy(work_buffer, huge_buffer, r500.data_size);
                    r500.data_size = (uint16_t)LZSS_Decode((uint8_t *)work_buffer, (uint8_t *)huge_buffer);     // 2022.06.09
                    StatusPrintfB("DATA", huge_buffer, 30);    // 取得した現在値
                }
                else{
                    StatusPrintfB("DATA", (char *)&huge_buffer[2], 30);   // 取得した現在値
                }


            }

            Err = ERRORCODE.ACT0;
            ERRORCODE.ACT0 = ERR(RF, NOERROR);

            RF_buff.rf_res.status = NULL_CODE;
            break;

        //case 2:
        //    Err = Sub_Act_2();    //ACT=2の処理 2020.01.29
        //    break;

        default:
            Printf("EWCUR act x \r\n");
            Err = ERR(CMD, FORMAT);
            break;
    }
Printf("EWCUR end %d  \r\n", Err);
    return(Err);
}


/**
 * @brief   データ吸い上げコマンド[EWSUC]
 * @return  実行結果、エラーコード
 * @note    AnalyzeCmd() に戻る
 */
uint32_t CFWirelessSuction(void)
{
//    uint16_t i;

    uint32_t Err = ERR(RF, NOERROR);

//    uint16_t acquire_number;
    uint32_t data_size = 0;
    int act = ParamInt("ACT");

    Printf("[EWSUC] act=%d \r\n",act);
    switch(act)
    {
        case 0:

            Err = start_rftask(RF_COMM_GATHER_DATA);            // ＴコマンドパラメータからＲＦタスク起動
            break;

        case 1:                                                 // ＲＦタスク終了後にデータを返す
            process_in_action1:;

//            if(in_rf_process == false){
/*TODO
        if(TX_SUSPENDED != rf_thread.tx_thread_state){
                Err = ERR(RF, BUSY);     // ＲＦタスクまだ起動してない
            }
            else TODO*/if(tx_mutex_get(&g_rf_mutex, WAIT_100MSEC) == TX_SUCCESS)    // ＲＦタスク終了
            {

                tx_mutex_put(&g_rf_mutex);

                if(RF_buff.rf_res.status == END_OK)             // 全データ確保
                {
//                  StatusPrintf("TIME", "%lu", RF_buff.rf_res.time);       // コマンド受信から無線通信開始までの秒数［秒］
//                  StatusPrintf("LAST", "%lu", timer.int125 * 125uL);      // 無線通信後の経過秒数［ｍｓ］（まだカウント中、次のコマンドが実行されるまで止まらない）

                    uncmpressData( 1 );        //圧縮データの回答処理（データ＋Propertyセット） 2020.01.29 サブ関数化

                    goto process_in_action3;
                }
            }
            else{
                Err = ERR(RF, BUSY);                           // ＲＦタスク実行中
            }

            break;

        case 3:                                                 // ＲＦタスク実行中でもデータを返す
            process_in_action3:;

            // property.acquire_number      ：ＲＦから受信済みのデータバイト数
            // property.transferred_number  ：ホストへ転送済みのデータバイト数
            // r500.data_size               ：ＰＣへ転送予定のデータバイト数

            if((ERRORCODE.ACT0 == ERR(RF, NOERROR)) && (memcmp(latest, PLIST.Arg[0].Name, 5) == 0))
            {
                if(r500.node == COMPRESS_ALGORITHM_NOTICE){
                    goto process_in_action1; // 圧縮処理されている場合
                }

                Printf("property.acquire_number=%d    property.transferred_number=%d \r\n", property.acquire_number,property.transferred_number);

                Sub_CF_DataProcess((32064 + 250 + 2), 2 );       //データ転送処理 2020.Feb.03 サブ関数化
                /*
                acquire_number = property.acquire_number;       // ＲＦから受信済みのデータバイト数ラッチ


                if((acquire_number > 0) && (acquire_number >= property.transferred_number))
                {

                    data_size = (r500.data_size == 0) ? (32064 + 250 + 2) : r500.data_size;

                    i = (data_size <= property.transferred_number) ? 0 : (uint16_t)(data_size - property.transferred_number);


                    StatusPrintf("SIZE", "%u", i);              // ＡＣＴ＝１未転送の吸い上げデータバイト数

                    if(i > (uint32_t)(acquire_number - property.transferred_number)){
                        i = (uint16_t)((uint32_t)(acquire_number - property.transferred_number));    // 受信済みバイト数で制限
                    }

                    if(i > 1024){
                        i = 1024;                      // 最大バイト数制限
                    }

                    if(data_size > acquire_number){
                        i &= (uint16_t)(~0x3f);    // ６４バイト区切り 全受信済みなら端数あり
                    }

                    // ここまで吸い上げたデータの転送処理

                    // データ数ゼロでもDATAパラメータを出力する
                    //if(i > 0)
                    //{
                        memcpy((char *)&(StsArea[CmdStatusSize]), "DATA=", 5);

                        StsArea[CmdStatusSize + 5] = (char)i;
                        StsArea[CmdStatusSize + 6] = (char)(i / 256);

                        CmdStatusSize += 7;                     // 転送データ格納域

                        memcpy(&StsArea[CmdStatusSize], &huge_buffer[property.transferred_number], (size_t)i);  // 受信データコピー

                        CmdStatusSize += i;
                        property.transferred_number = (uint16_t)( property.transferred_number + i);
                    //}
                }
                */

            }

            data_size = (r500.data_size == 0) ? (32064 + 250 + 2) : r500.data_size;
            Printf("[EWSUC] size %ld(act %d)\r\n", data_size,act);

            Err = Sub_CFSearch((int)data_size, 1);  //TIME,LAST,END表示 2020.01.30 サブ関数化
            break;

        case 2:
            Err = Sub_Act_2();    //ACT=2の処理 2020.01.29
            break;

        default:

            Err = ERR(CMD, FORMAT);
            break;
    }

    return(Err);
}



/**
 * @brief   子機検索コマンド [EWSCE]
 * @return  実行結果、エラーコード
 * @note    AnalyzeCmd() に戻る
 */
uint32_t CFSearchKoki(void)
{
//    int acquire_number, i;

    uint32_t Err = ERR(RF, NOERROR);

    int data_size = 0;


    switch(ParamInt("ACT"))
    {
        case 0:

            Err = start_rftask(RF_COMM_SEARCH);                 // ＴコマンドパラメータからＲＦタスク起動
            break;

        case 1:                                                 // ＲＦタスク終了後にデータを返す
            process_in_action1:;

//            if(in_rf_process == false){
/*TODO
            if(TX_SUSPENDED != rf_thread.tx_thread_state){
                Err = ERR(RF, BUSY);     // ＲＦタスクまだ起動してない
            }
            else TODO*/if(tx_mutex_get(&g_rf_mutex, WAIT_100MSEC) == TX_SUCCESS)    // ＲＦタスク終了
            {
                tx_mutex_put(&g_rf_mutex);

                if(RF_buff.rf_res.status == END_OK)             // 全データ確保
                {
                    uncmpressData( 1 );        //圧縮データの回答処理（データ＋Propertyセット） 2020.01.29 サブ関数化

                    goto process_in_action3;
                }
            }
            else{
                Err = ERR(RF, BUSY);                           // ＲＦタスク実行中
            }

            break;

        case 3:
            process_in_action3:;

            // property.acquire_number      ：ＲＦから受信済みのデータバイト数
            // property.transferred_number  ：ホストへ転送済みのデータバイト数
            // r500.data_size               ：ＰＣへ転送予定のデータバイト数
            // property.sendon_number       ：最大転送予定のデータバイト数

            if((ERRORCODE.ACT0 == ERR(RF, NOERROR)) && (memcmp(latest, PLIST.Arg[0].Name, 5) == 0))
            {
                if(r500.node == COMPRESS_ALGORITHM_NOTICE){
                    goto process_in_action1; // 圧縮処理されている場合
                }

                Sub_CF_DataProcess(250 * 19, 0 );       //データ転送処理 2020.Feb.03 サブ関数化
                /*
                acquire_number = property.acquire_number;       // ＲＦから受信済みのデータバイト数ラッチ

                if((acquire_number > 0) && (acquire_number >= property.transferred_number))
                {

                    data_size = (r500.data_size == 0) ? (250 * 19) : r500.data_size;

                    i = (data_size <= property.transferred_number) ? 0 : (data_size - property.transferred_number);

                    if(i > (acquire_number - property.transferred_number)){
                        i = acquire_number - property.transferred_number;    // 受信済みバイト数で制限
                    }

                    if(i > 1024){
                        i = 1024;                      // 最大バイト数制限
                    }

                    // ここまで受信したデータの転送処理

                    if(i > 0)
                    {
                        memcpy((char *)&(StsArea[CmdStatusSize]), "DATA=", 5);

                        StsArea[CmdStatusSize + 5] = (uint8_t)i;
                        StsArea[CmdStatusSize + 6] = (uint8_t)(i / 256);

                        CmdStatusSize += 7;                     // 転送データ格納域

                        memcpy(&StsArea[CmdStatusSize], &huge_buffer[property.transferred_number], (size_t)i);  // 受信データコピー

                        CmdStatusSize += i;
                        property.transferred_number = (uint16_t)(property.transferred_number + i);
                    }
                }
                */
            }

            data_size = (r500.data_size == 0) ? (250 * 19) : r500.data_size;
            Err = Sub_CFSearch(data_size, 0);  //2020.01.29    各関数の共通部をサブ関数化
            break;

        case 2:
            Err = Sub_Act_2();    //ACT=2の処理 2020.01.29

            break;

        default:
            Err = ERR(CMD, FORMAT);
            break;
    }

    return(Err);
}



/**
 * @brief   中継機検索コマンド [EWSCR]
 * @return  実行結果、エラーコード
 * @note    AnalyzeCmd() に戻る
 */
uint32_t CFSearchRelay(void)
{
//    uint16_t i,acquire_number;
    uint16_t data_size = 0;;

    uint32_t Err = ERR(RF, NOERROR);

//    uint32_t acquire_number, data_size = 0;


    switch(ParamInt("ACT"))
    {
        case 0:

            Err = start_rftask(RF_COMM_REPEATER_SEARCH);        // ＴコマンドパラメータからＲＦタスク起動
            break;

        case 1:                                                 // ＲＦタスク終了後にデータを返す
            process_in_action1:;

//            if(in_rf_process == false){
/*TODO
            if(TX_SUSPENDED != rf_thread.tx_thread_state){
                Err = ERR(RF, BUSY);     // ＲＦタスクまだ起動してない
            }
            else TODO*/if(tx_mutex_get(&g_rf_mutex, WAIT_100MSEC) == TX_SUCCESS)    // ＲＦタスク終了
            {
                tx_mutex_put(&g_rf_mutex);

                if(RF_buff.rf_res.status == END_OK)             // 全データ確保
                {
                    uncmpressData( 1 );        //圧縮データの回答処理（データ＋Propertyセット） 2020.01.29 サブ関数化

                    goto process_in_action3;
                }
            }
            else{
                Err = ERR(RF, BUSY);                           // ＲＦタスク実行中
            }

            break;

        case 3:
            process_in_action3:;

            // property.acquire_number      ：ＲＦから受信済みのデータバイト数
            // property.transferred_number  ：ホストへ転送済みのデータバイト数
            // r500.data_size               ：ＰＣへ転送予定のデータバイト数
            // property.sendon_number       ：最大転送予定のデータバイト数

            if((ERRORCODE.ACT0 == ERR(RF, NOERROR)) && (memcmp(latest, PLIST.Arg[0].Name, 5) == 0))
            {
                if(r500.node == COMPRESS_ALGORITHM_NOTICE){
                    goto process_in_action1; // 圧縮処理されている場合
                }

                Sub_CF_DataProcess(250 * 19, 0 );       //データ転送処理 2020.Feb.03 サブ関数化
 /*
                acquire_number = property.acquire_number;       // ＲＦから受信済みのデータバイト数ラッチ

                if((acquire_number > 0) && (acquire_number >= property.transferred_number))
                {
                    data_size = (r500.data_size == 0) ? (250 * 19) : r500.data_size;

                    i = (data_size <= property.transferred_number) ? 0 : (uint16_t)(data_size - property.transferred_number);

                    if(i > (uint16_t)(acquire_number - property.transferred_number)){
                        i = (uint16_t)(acquire_number - property.transferred_number);    // 受信済みバイト数で制限
                    }

                    if(i > 1024){
                        i = 1024;                      // 最大バイト数制限
                    }

                    // ここまで受信したデータの転送処理

                    if(i > 0)
                    {
                        memcpy((char *)&(StsArea[CmdStatusSize]), "DATA=", 5);

                        StsArea[CmdStatusSize + 5] = (char)i;
                        StsArea[CmdStatusSize + 6] = (char)(i / 256);

                        CmdStatusSize += 7;                     // 転送データ格納域

                        memcpy(&StsArea[CmdStatusSize], &huge_buffer[property.transferred_number], i);  // 受信データコピー

                        CmdStatusSize += i;
                        property.transferred_number = (uint16_t)(property.transferred_number + i);
                    }
                }
                */
            }

            data_size = (r500.data_size == 0) ? (250 * 19) : r500.data_size;
            Err = Sub_CFSearch(data_size, 0);  //2020.01.29    各関数の共通部をサブ関数化
            break;

        case 2:
            Err = Sub_Act_2();    //ACT=2の処理 2020.01.29
            break;

        default:
            Err = ERR(CMD, FORMAT);
            break;
    }

    return(Err);
}


/**
 * @brief   リアルスキャンコマンド [EWRSC]
 * @return  実行結果、エラーコード
 * @note    AnalyzeCmd() に戻る
 */
uint32_t CFExRealscan(void)
{
    uint16_t i;

    uint32_t Err = ERR(RF, NOERROR);

//    uint32_t acquire_number;
    int data_size = 0;
    int act = ParamInt("ACT");


    switch(act)
    {
        case 0:

            Err = start_rftask(RF_COMM_REAL_SCAN);              // ＴコマンドパラメータからＲＦタスク起動
            break;

        case 1:                                                 // ＲＦタスク終了後にデータを返す
            process_in_action1:;

//            if(in_rf_process == false){
/*TODO
            if(TX_SUSPENDED != rf_thread.tx_thread_state){
                Err = ERR(RF, BUSY);     // ＲＦタスクまだ起動してない
            }
            else iTODO*/if(tx_mutex_get(&g_rf_mutex, WAIT_100MSEC) == TX_SUCCESS)    // ＲＦタスク終了
            {
                tx_mutex_put(&g_rf_mutex);

                Printf("Real Scan status=%02X  size=%d \r\n", RF_buff.rf_res.status,r500.data_size);

                if(RF_buff.rf_res.status == END_OK)             // 全データ確保
                {
                    if(r500.node == COMPRESS_ALGORITHM_NOTICE)  // 圧縮データを解凍(huge_buffer先頭は0xe0)
                    {
                        i = (uint16_t)(r500.data_size - *(uint16_t *)&huge_buffer[2]);

                        memcpy(work_buffer, huge_buffer, r500.data_size);
                        r500.data_size = (uint16_t)LZSS_Decode((uint8_t *)work_buffer, (uint8_t *)huge_buffer);     // 2022.06.09

                        if(i > 0)                               // 非圧縮の中継機データ情報があるとき
                        {
                            memcpy(&huge_buffer[r500.data_size], &work_buffer[*(uint16_t *)&work_buffer[2]], i);
                        }

                        r500.data_size = (uint16_t)(r500.data_size + i);
                        property.acquire_number = r500.data_size;
                        property.transferred_number = 0;

                        r500.node = 0xff;                       // 解凍が重複しないようにする
                    }

                    goto process_in_action3;
                }
            }
            else{
                Err = ERR(RF, BUSY);                           // ＲＦタスク実行中
            }

            break;

        case 3:
            process_in_action3:;

            // property.acquire_number      ：ＲＦから受信済みのデータバイト数
            // property.transferred_number  ：ホストへ転送済みのデータバイト数
            // r500.data_size               ：ＰＣへ転送予定のデータバイト数
            // property.sendon_number       ：最大転送予定のデータバイト数

            if((ERRORCODE.ACT0 == ERR(RF, NOERROR)) && (memcmp(latest, PLIST.Arg[0].Name, 5) == 0))
            {
                if(r500.node == COMPRESS_ALGORITHM_NOTICE){
                    goto process_in_action1; // 圧縮処理されている場合
                }

                Printf("property.acquire_number=%d    property.transferred_number=%d \r\n", property.acquire_number,property.transferred_number);

                Sub_CF_DataProcess(250 * 30 + 2 + 251 * 2, 0 );       //データ転送処理 2020.Feb.03 サブ関数化
                /*
                acquire_number = property.acquire_number;       // ＲＦから受信済みのデータバイト数ラッチ
                if((acquire_number > 0) && (acquire_number >= property.transferred_number))
                {
                    data_size = (r500.data_size == 0) ? (250 * 30 + 2 + 251 * 2) : r500.data_size;  //8004Byte

                    i = (data_size <= property.transferred_number) ? 0 : (uint16_t)(data_size - property.transferred_number);

                    if(i > (acquire_number - property.transferred_number)){
                        i = (uint16_t)(acquire_number - property.transferred_number);    // 受信済みバイト数で制限
                    }

                    //if((act == 3) && (i > 2048)) i = 2048;        // 最大バイト制限
                    if(i > 1024){
                        i = 1024;                      // 最大バイト数制限
                    }

                    // ここまで受信したデータの転送処理

                    if(i > 0)
                    {
                        memcpy((char *)&(StsArea[CmdStatusSize]), "DATA=", 5);

                        StsArea[CmdStatusSize + 5] = (uint8_t)i;
                        StsArea[CmdStatusSize + 6] = (uint8_t)(i / 256);

                        CmdStatusSize += 7;                     // 転送データ格納域

                        memcpy(&StsArea[CmdStatusSize], &huge_buffer[property.transferred_number], i);  // 受信データコピー

                        CmdStatusSize += i;
                        property.transferred_number = (uint16_t)(property.transferred_number + i);
                    }
                    for(int j=0;j<34;j++){
                        Printf("%02X ", huge_buffer[j]);
                    }
                    Printf("\r\n");
                }
                */

/*
                for(int j=0;j<34;j++){
                    StatusPrintf("TIME", "%lu", RF_buff.rf_res.time);       // コマンド受信から無線通信開始までの秒数［秒］
                    StatusPrintf("LAST", "%lu", timer.int125 * 125uL);      // 無線通信後の経過秒数［ｍｓ］（まだカウント中、次のコマンドが実行されるまで止まらない）
                    StatusPrintf("END", "%u", 1);   // 転送終了
                }
*/

                Err = ERRORCODE.ACT0;
                ERRORCODE.ACT0 = ERR(RF, NOERROR);
            }

            data_size = (r500.data_size == 0) ? (250 * 30 + 2 + 251 * 2) : r500.data_size;  //8004Byte              2020/02/27追加瀬木
            Err = Sub_CFSearch(data_size, 1);  //TIME,LAST,END表示 2020.01.29    各関数の共通部をサブ関数化

            break;

        case 2:
            Err = Sub_Act_2();    //ACT=2の処理 2020.01.29
            break;

        default:

            Err = ERR(CMD, FORMAT);
            break;
    }

    return(Err);
}


/**
 * @brief   子機と中継機の一斉検索コマンド
 * @return  実行結果、エラーコード
 * @note    AnalyzeCmd() に戻る
 */
uint32_t CFThroughoutSearch(void)
{
    int i;

    uint32_t Err = ERR(RF, NOERROR);
//  int acquire_number;

    union{
        uint8_t byte[2];
        uint16_t word;
    }b_w_chg;

    uint8_t grp_no,grp_max;
    uint8_t parent_no;
    char rf_id[8];
    memset(rf_id,0x00,sizeof(rf_id));


    int data_size = 0, act = ParamInt("ACT");

//    Printf("[EWRTR]\r\n");
    Printf("[EWRSR]\r\n");
    switch(act)
    {
        case 0:
            Err = start_rftask(RF_COMM_WHOLE_SCAN);             // ＴコマンドパラメータからＲＦタスク起動
            break;

        case 1:                                                 // ＲＦタスク終了後にデータを返す
            process_in_action1:;

//          if(in_rf_process == false){
/*TODO
            if(TX_SUSPENDED != rf_thread.tx_thread_state){
                Err = ERR(RF, BUSY);        // ＲＦタスクまだ起動してない
            }
            else TODO*/if(tx_mutex_get(&g_rf_mutex, WAIT_100MSEC) == TX_SUCCESS)    // ＲＦタスク終了
            {
                tx_mutex_put(&g_rf_mutex);

                if(RF_buff.rf_res.status == END_OK)             // 全データ確保
                {
                    if(r500.node == COMPRESS_ALGORITHM_NOTICE)  // 圧縮データを解凍(huge_buffer先頭は0xe0)
                    {
                        i = r500.data_size - *(uint16_t *)&huge_buffer[2];

                        memcpy(work_buffer, huge_buffer, r500.data_size);
                        r500.data_size = (uint16_t)LZSS_Decode((uint8_t *)work_buffer, (uint8_t *)huge_buffer);     // 2022.06.09
                        Printf("[EWRSR] r500.data_size %ld\r\n", r500.data_size);
                        if(i > 0)                               // 非圧縮の中継機データ情報があるとき
                        {
                            memcpy((char *)&huge_buffer[r500.data_size], (char *)&work_buffer[*(uint16_t *)&work_buffer[2]], (size_t)i);
                        }

                        r500.data_size = (uint16_t)(r500.data_size + i);
                        property.acquire_number = r500.data_size;
                        property.transferred_number = 0;

                        r500.node = 0xff;                       // 解凍が重複しないようにする
                    }

                    goto process_in_action3;
                }
            }
            else{
                Err = ERR(RF, BUSY);                            // ＲＦタスク実行中
            }

            break;

        case 3:
            process_in_action3:;

            // property.acquire_number      ：ＲＦから受信済みのデータバイト数
            // property.transferred_number  ：ホストへ転送済みのデータバイト数
            // r500.data_size               ：ＰＣへ転送予定のデータバイト数
            // property.sendon_number       ：最大転送予定のデータバイト数

            if((ERRORCODE.ACT0 == ERR(RF, NOERROR)) && (memcmp(latest, PLIST.Arg[0].Name, 5) == 0))
            {
                if(r500.node == COMPRESS_ALGORITHM_NOTICE){
                    goto process_in_action1;    // 圧縮処理されている場合
                }
                //  最大 3324 byte 子機100 中10 非圧縮
                Sub_CF_DataProcess(250 * 30 + 2 + 251 * 2, 0 );       //データ転送処理 2020.Feb.03 サブ関数化
                /*
                acquire_number = property.acquire_number;       // ＲＦから受信済みのデータバイト数ラッチ

                if((acquire_number > 0) && (acquire_number >= property.transferred_number))
                {
                    data_size = (r500.data_size == 0) ? (250 * 30 + 2 + 251 * 30) : r500.data_size;     //8004Byte

                    i = (data_size <= property.transferred_number) ? 0 : (data_size - property.transferred_number);

                    if(i > (acquire_number - property.transferred_number)){
                        i = acquire_number - property.transferred_number;   // 受信済みバイト数で制限
                    }

                    //if((act == 3) && (i > 2048)) i = 2048;        // 最大バイト制限
                    if(i > 1024){
                        i = 1024;                       // 最大バイト数制限
                    }

                    // ここまで受信したデータの転送処理

                    if(i > 0)
                    {
                        memcpy((char *)&(StsArea[CmdStatusSize]), "DATA=", 5);

                        StsArea[CmdStatusSize + 5] = (uint8_t)i;
                        StsArea[CmdStatusSize + 6] = (uint8_t)(i / 256);

                        CmdStatusSize += 7;                     // 転送データ格納域

                        memcpy((char *)&StsArea[CmdStatusSize], (char *)&huge_buffer[property.transferred_number], (size_t)i);  // 受信データコピー

                        CmdStatusSize += i;
                        property.transferred_number = (uint16_t)(property.transferred_number + i);
                    }
                }
                */
            }

            data_size = (r500.data_size == 0) ? (250 * 30 + 2 + 251 * 30) : r500.data_size;     //8004Byte
            Err = Sub_CFSearch(data_size, 1);  //2020.01.29    各関数の共通部をサブ関数化
            Printf("[EWRSR] data size %ld / r500.data_size %ld\r\n", data_size, r500.data_size);

//          if(1 == (root_registry.auto_route & 0x01)){     // 自動ルート機能ONの場合のみ実行
            if((ERR(RF, NOERROR) == Err) && (1 == (root_registry.auto_route & 0x01))){      // 自動ルート機能ONの場合のみ実行

                memcpy(rf_id, group_registry.id,sizeof(group_registry.id));     // 無線通信IDを退避
                parent_no = rpt_sequence[group_registry.altogether-1];          // 親番号を退避（ルートの末端）
                rpt_sequence_cnt = group_registry.altogether;                   // ルート段数

                grp_max = get_regist_group_size();
                for(grp_no=1; grp_no<=grp_max; grp_no++){          // 登録されているグループ数分ループ
                    if(0 != get_regist_group_adr(grp_no)){                      // グループ情報読込（無線通信IDを取得する）
                        if(0 == memcmp(group_registry.id, rf_id, sizeof(rf_id))){      // 無線通信IDが一致するグループの検索
                            RF_get_regist_RegisterRemoteUnit(grp_no);                       // 電波強度テーブルに子機を登録
                            b_w_chg.byte[0] = huge_buffer[0];                               // 無線通信結果メモリを自律処理用変数にコピーする
                            b_w_chg.byte[1] = huge_buffer[1];                               // 無線通信結果メモリを自律処理用変数にコピーする
//                            b_w_chg.word = (uint16_t)(b_w_chg.word + 2);
                            b_w_chg.word = (uint16_t)(b_w_chg.word + rpt_sequence_cnt*2);   // 個別中継機データ分を加算

                            memcpy(rf_ram.realscan.data_byte,&huge_buffer[2],b_w_chg.word); // MAX=(30byte×128)+2 = 3842
                            RF_table_make(grp_no, parent_no, REGU_MONI);                    // 無線通信の結果で電波強度更新

//                            RF_Group[grp_no-1].max_unit_no = RF_get_regist_RegisterRemoteUnit(grp_no);        // 最大子機番号取得
//                            RF_Group[grp_no-1].max_rpt_no = (uint8_t)RF_get_rpt_registry_full(grp_no);                // 最大中継機番号更新

                            RF_WirelessGroup_RefreshParentTable(&RF_Group[grp_no-1], root_registry.rssi_limit1, root_registry.rssi_limit2);        // 無線ルート更新
                            RF_oya_log_last(grp_no);                                                        // 最後に通信した親番号のみ更新
                            if((VENDER_HIT != fact_config.Vender) && (Http_Use == HTTP_SEND)){      // 2020.10.16
                                G_HttpFile[FILE_I].sndflg = SND_ON;             // [FILE]機器状態送信：ON
                            }
                            break;
                        }
                    }
                }
            }

            break;

        case 2:
            Err = Sub_Act_2();    //ACT=2の処理 2020.01.29
            break;

        default:

            Err = ERR(CMD, FORMAT);
            break;
    }

    return(Err);
}




/**
 * @brief   グループ一斉記録開始 [EWBSW]
 * @return  実行結果、エラーコード
 * @note    AnalyzeCmd() に戻る
 */
uint32_t CFRecStartGroup(void)
{
//    int acquire_number, i;

    uint32_t Err = ERR(RF, NOERROR);

    uint32_t data_size = 0;


    switch(ParamInt("ACT"))
    {
        case 0:
            Err = start_rftask(RF_COMM_SETTING_WRITE_GROUP);    // ＴコマンドパラメータからＲＦタスク起動

            break;

        case 1:                                                 // ＲＦタスク終了後にデータを返す
            process_in_action1:;

//            if(in_rf_process == false){
/*TODO
            if(TX_SUSPENDED != rf_thread.tx_thread_state){
                Err = ERR(RF, BUSY);     // ＲＦタスクまだ起動してない
            }
            else TODO*/ if(tx_mutex_get(&g_rf_mutex, WAIT_100MSEC) == TX_SUCCESS)    // ＲＦタスク終了
            {
                tx_mutex_put(&g_rf_mutex);

                if(RF_buff.rf_res.status == END_OK)             // 全データ確保
                {
                    uncmpressData( 1 );        //圧縮データの回答処理（データ＋Propertyセット） 2020.01.29 サブ関数化

                    goto process_in_action3;
                }
            }
            else{
                Err = ERR(RF, BUSY);                           // ＲＦタスク実行中
            }

            break;

        case 3:
            process_in_action3:;

            // property.acquire_number      ：ＲＦから受信済みのデータバイト数
            // property.transferred_number  ：ホストへ転送済みのデータバイト数
            // r500.data_size               ：ＰＣへ転送予定のデータバイト数
            // property.sendon_number       ：最大転送予定のデータバイト数

            if((ERRORCODE.ACT0 == ERR(RF, NOERROR)) && (memcmp(latest, PLIST.Arg[0].Name, 5) == 0))
            {
                if(r500.node == COMPRESS_ALGORITHM_NOTICE){
                    goto process_in_action1; // 圧縮処理されている場合
                }

                Sub_CF_DataProcess(250 * 19, 0 );       //データ転送処理 2020.Feb.03 サブ関数化
                /*
                acquire_number = property.acquire_number;       // ＲＦから受信済みのデータバイト数ラッチ

                if((acquire_number > 0) && (acquire_number >= property.transferred_number))
                {
                    data_size = (r500.data_size == 0) ? (250 * 19) : r500.data_size;

                    i = (data_size <= property.transferred_number) ? 0 : (uint16_t)(data_size - property.transferred_number);

                    if(i > (uint16_t)(acquire_number - property.transferred_number)){
                        i = (uint16_t)(acquire_number - property.transferred_number);    // 受信済みバイト数で制限
                    }

                    if(i > 1024){
                        i = 1024;                      // 最大バイト数制限
                    }

                    // ここまで受信したデータの転送処理

                    if(i > 0)
                    {
                        memcpy((char *)&(StsArea[CmdStatusSize]), "DATA=", 5);

                        StsArea[CmdStatusSize + 5] = (uint8_t)i;
                        StsArea[CmdStatusSize + 6] = (uint8_t)(i / 256);

                        CmdStatusSize += 7;                     // 転送データ格納域

                        memcpy((char *)&StsArea[CmdStatusSize], (char *)&huge_buffer[property.transferred_number], (size_t)i);  // 受信データコピー

                        CmdStatusSize += i;
                        property.transferred_number = (uint16_t)(property.transferred_number + i);
                    }
                }
                */
            }

            data_size = (r500.data_size == 0) ? (250 * 19) : r500.data_size;
            Err = Sub_CFSearch((int)data_size, 0);  //END表示 2020.01.30 サブ関数化
            break;

        case 2:
            Err = Sub_Act_2();    //ACT=2の処理 2020.01.29
            break;

        default:

            Err = (ERR(CMD, FORMAT));
            break;
    }

    return(Err);
}


/**
 * @brief  記録停止コマンド [EWRSP]
 * @return  実行結果、エラーコード
 * @note    AnalyzeCmd() に戻る
 */
uint32_t CFWirelessRecStop(void)
{
    uint32_t Err = ERR(RF, NOERROR);


    switch(ParamInt("ACT"))
    {
        case 0:

            Err = start_rftask(RF_COMM_RECORD_STOP);            // ＴコマンドパラメータからＲＦタスク起動
            break;

        case 1:

            Err = ERRORCODE.ACT0;
            ERRORCODE.ACT0 = ERR(RF, NOERROR);

            RF_buff.rf_res.status = NULL_CODE;
            break;

        case 2:
            Err = Sub_Act_2();    //ACT=2の処理 2020.01.29
            break;

        default:
            Err = ERR(CMD, FORMAT);
            break;
    }

    return(Err);
}

/**
 * @brief  子機のプロテクト設定 [EWPRO]
 * @return  実行結果、エラーコード
 * @note    AnalyzeCmd() に戻る
 * @attention case 0の パラメータ違いだけのコピペ関数が13個ある
 */
uint32_t CFSetProtection(void)
{
    uint32_t Err = ERR(RF, NOERROR);


    switch(ParamInt("ACT"))
    {
        case 0:

            Err = start_rftask(RF_COMM_PROTECTION);             // ＴコマンドパラメータからＲＦタスク起動
            break;

        case 1:

            Err = ERRORCODE.ACT0;
            ERRORCODE.ACT0 = ERR(RF, NOERROR);

            RF_buff.rf_res.status = NULL_CODE;
            break;

        case 2:
            Err = Sub_Act_2();    //ACT=2の処理 2020.01.29
            break;

        default:
            Err = ERR(CMD, FORMAT);
            break;
    }

    return(Err);
}

/**
 * @brief  子機のモニタリング間隔設定 [EWINT]
 * @return  実行結果、エラーコード
 * @note    AnalyzeCmd() に戻る
 * @attention case 0の パラメータ違いだけのコピペ関数が13個ある
 */
uint32_t CFExInterval(void)
{
    uint32_t Err = ERR(RF, NOERROR);


    switch(ParamInt("ACT"))
    {
        case 0:

            Err = start_rftask(RF_COMM_MONITOR_INTERVAL);       // ＴコマンドパラメータからＲＦタスク起動
            break;

        case 1:

            Err = ERRORCODE.ACT0;
            ERRORCODE.ACT0 = ERR(RF, NOERROR);

            RF_buff.rf_res.status = NULL_CODE;
            break;

        case 2:
            Err = Sub_Act_2();    //ACT=2の処理 2020.01.29
            break;

        default:
            Err = ERR(CMD, FORMAT);
            break;
    }

    return(Err);
}

/**
 * @brief  子機登録変更 [EWREG]
 * @return  実行結果、エラーコード
 * @note    AnalyzeCmd() に戻る
 * @attention case 0の パラメータ違いだけのコピペ関数が13個ある
 */
uint32_t CFWirelessRegistration(void)
{
    uint32_t Err = ERR(RF, NOERROR);


    switch(ParamInt("ACT"))
    {
        case 0:

            Err = start_rftask(RF_COMM_REGISTRATION);           // ＴコマンドパラメータからＲＦタスク起動
            break;

        case 1:

            Err = ERRORCODE.ACT0;
            ERRORCODE.ACT0 = ERR(RF, NOERROR);

            RF_buff.rf_res.status = NULL_CODE;
            break;

        case 2:
            Err = Sub_Act_2();    //ACT=2の処理 2020.01.29
            break;

        default:
            Err = ERR(CMD, FORMAT);
            break;
    }

    return(Err);
}





/**
 * @brief  子機のBluetooth設定値Read[EWBLR]
 * @return  実行結果、エラーコード
 * @note    AnalyzeCmd() に戻る
 */
uint32_t CFBleSettingsRead(void)
{
    uint32_t Err = ERR(RF, NOERROR);
    char str[32];

    Printf("[EWBLR]\r\n");
    switch(ParamInt("ACT"))
    {
        case 0:

            Err = start_rftask(RF_COMM_BLE_SETTING_READ);           // ＴコマンドパラメータからＲＦタスク起動
            break;

        case 1:

            Printf("Err %ld rf status %02X\r\n", ERRORCODE.ACT0, RF_buff.rf_res.status);

            //if((ERRORCODE.ACT0 == ERR(RF, NOERROR)) && (RF_buff.rf_res.status == END_OK) && (memcmp(latest, PLIST.Arg[0].Name, 5) == 0))
            //if((ERRORCODE.ACT0 == ERR(RF, NOERROR)) && (RF_buff.rf_res.status == RFM_NORMAL_END) && (memcmp(latest, PLIST.Arg[0].Name, 5) == 0))
            if(RF_buff.rf_res.status == END_OK)             // 全データ確保
            {
                uncmpressData( 0 );    // 圧縮データを解凍 サブ関数化 2020.01.29

                Printf("CFWirelessSettingsRead size %d \r\n", r500.data_size);

                if(huge_buffer[36] == 0xFF){            // 0xFF:OFF     それ以外なら:ON
                    StatusPrintf("ENABLE", "%u", 0);                        // ENABLE OFF
                }
                else{
                    StatusPrintf("ENABLE", "%u", 1);                        // ENABLE ON
                }

                memset(str, 0x00, sizeof(str));                 // 子機機器名
                memcpy(str, &huge_buffer[1], 26);

                StatusPrintf("NAME","%s", str);
                //StatusPrintfB("NAME", (char *)&huge_buffer[1],26);        // NAME
        //      StatusPrintfB("PASC", (char *)&huge_buffer[32],4);      // SECURITY

/*
                if(huge_buffer[27] == 0){
                    StatusPrintf("SECURITY", "%u", 0);                          // PASC
                }
                else{
                    StatusPrintf("SECURITY", "%u", 1);                          // PASC
                }
                StatusPrintfB("DATA", huge_buffer, r500.data_size);    // 取得した設定値
*/
            }

            Err = ERRORCODE.ACT0;
            ERRORCODE.ACT0 = ERR(RF, NOERROR);

            RF_buff.rf_res.status = NULL_CODE;
            break;

        case 2:
            Err = Sub_Act_2();    //ACT=2の処理 2020.01.29
            break;

        default:

            Err = ERR(CMD, FORMAT);
            break;
    }

    return(Err);
}


/**
 * @brief  子機のBluetooth設定値Write[EWBLW]
 * @return  実行結果、エラーコード
 * @note    AnalyzeCmd() に戻る
 * @attention case 0の パラメータ違いだけのコピペ関数が13個ある
 */
uint32_t CFBleSettingsWrite(void)
{
    uint32_t Err = ERR(RF, NOERROR);

    Printf("[EWBLW] \r\n");
    switch(ParamInt("ACT"))
    {
        case 0:

            Err = start_rftask(RF_COMM_BLE_SETTING_WRITE);          // ＴコマンドパラメータからＲＦタスク起動
            break;

        case 1:

            Err = ERRORCODE.ACT0;
            ERRORCODE.ACT0 = ERR(RF, NOERROR);

            RF_buff.rf_res.status = NULL_CODE;

            if(Err == 0x40000){
                RegistTableBleWrite();
            }
            break;

        case 2:
            Err = Sub_Act_2();    //ACT=2の処理 2020.01.29
            break;

        default:

            Err = ERR(CMD, FORMAT);
            break;
    }
    Printf("Err %04X\r\n", Err);
    return(Err);
}




/**
 * @brief  子機のスケール変換式Read[EWSLR]
 * @return  実行結果、エラーコード
 * @note    AnalyzeCmd() に戻る
 * @attention case 0の パラメータ違いだけのコピペ関数が13個ある
 */
uint32_t CFScaleSettingsRead(void)
{
    uint32_t Err = ERR(RF, NOERROR);

    Printf("[EWSLR] \r\n");
    switch(ParamInt("ACT"))
    {
        case 0:

            Err = start_rftask(RF_COMM_MEMO_READ);           // ＴコマンドパラメータからＲＦタスク起動
            break;

        case 1:

            Printf("Err %ld rf status %02X\r\n", ERRORCODE.ACT0, RF_buff.rf_res.status);

            //if((ERRORCODE.ACT0 == ERR(RF, NOERROR)) && (RF_buff.rf_res.status == END_OK) && (memcmp(latest, PLIST.Arg[0].Name, 5) == 0))
            //if((ERRORCODE.ACT0 == ERR(RF, NOERROR)) && (RF_buff.rf_res.status == RFM_NORMAL_END) && (memcmp(latest, PLIST.Arg[0].Name, 5) == 0))
            if(RF_buff.rf_res.status == END_OK)             // 全データ確保
            {
                uncmpressData( 0 );    // 圧縮データを解凍 サブ関数化 2020.01.29

                Printf("CFWirelessSettingsRead size %d \r\n", r500.data_size);
                StatusPrintfB("DATA", huge_buffer, r500.data_size);    // 取得した設定値
            }

            Err = ERRORCODE.ACT0;
            ERRORCODE.ACT0 = ERR(RF, NOERROR);

            RF_buff.rf_res.status = NULL_CODE;
            break;

        case 2:
            Err = Sub_Act_2();    //ACT=2の処理 2020.01.29
            break;

        default:

            Err = ERR(CMD, FORMAT);
            break;
    }

    return(Err);
}

/**
 * @brief  子機のスケール変換式Write[EWSLW]
 * @return  実行結果、エラーコード
 * @note    AnalyzeCmd() に戻る
 * @attention case 0の パラメータ違いだけのコピペ関数が13個ある
 */
uint32_t CFScaleSettingsWrite(void)
{
    uint32_t Err = ERR(RF, NOERROR);

    Printf("[EWSLW] \r\n");
    switch(ParamInt("ACT"))
    {
        case 0:

            Err = start_rftask(RF_COMM_MEMO_WRITE);          // ＴコマンドパラメータからＲＦタスク起動
            break;

        case 1:

            Err = ERRORCODE.ACT0;
            ERRORCODE.ACT0 = ERR(RF, NOERROR);

            RF_buff.rf_res.status = NULL_CODE;

            if(Err == 0x40000){
                RegistTableSlWrite();
            }

            break;

        case 2:
            Err = Sub_Act_2();    //ACT=2の処理 2020.01.29
            break;

        default:

            Err = ERR(CMD, FORMAT);
            break;
    }

    return(Err);
}





/**
 * @brief   [EWPSR] PUSHの設定値読み込み（無線）
 * @return
 */
uint32_t CFPushSettingsRead(void)
{
    uint32_t Err = ERR(RF, NOERROR);

    Printf("[EWPSR] \r\n");
    switch(ParamInt("ACT"))
    {
        case 0:

            Err = start_rftask(RF_COMM_PS_SETTING_READ);        // ＴコマンドパラメータからＲＦタスク起動
            break;

        case 1:
            if((ERRORCODE.ACT0 == ERR(RF, NOERROR)) && (RF_buff.rf_res.status == END_OK) && (memcmp(latest, PLIST.Arg[0].Name, 5) == 0))
            {
                uncmpressData( 0 );    // 圧縮データを解凍 サブ関数化 2020.01.29

                StatusPrintfB("DATA", huge_buffer, 64); // 取得した設定値
            }

            Err = ERRORCODE.ACT0;
            ERRORCODE.ACT0 = ERR(RF, NOERROR);

            RF_buff.rf_res.status = NULL_CODE;
            break;

        case 2:
            Err = Sub_Act_2();    //ACT=2の処理 2020.01.29
            break;

    default:

        Err = ERR(CMD, FORMAT);
        break;
    }

    return(Err);
}


/**
 * @brief   [EWPSW] PUSHの設定値書き込み（無線）
 * @return
 * @attention case 0の パラメータ違いだけのコピペ関数が13個ある
 */
uint32_t CFPushSettingsWrite(void)
{
    uint32_t Err = ERR(RF, NOERROR);

    Printf("[EWPSW] \r\n");
    switch(ParamInt("ACT"))
    {
        case 0:

            Err = start_rftask(RF_COMM_PS_SETTING_WRITE);       // ＴコマンドパラメータからＲＦタスク起動
            break;

        case 1:

            Err = ERRORCODE.ACT0;
            ERRORCODE.ACT0 = ERR(RF, NOERROR);

            RF_buff.rf_res.status = NULL_CODE;
            break;

        case 2:
            Err = Sub_Act_2();    //ACT=2の処理 2020.01.29
            break;

        default:

            Err = ERR(CMD, FORMAT);
            break;
    }

    return(Err);
}



/**
 * @brief   [EWPSU] PUSHからデータ吸い上げ
 * @return
 * @note    このコマンドはデータサイズの制限がない
 */
uint32_t CFPushSuction(void)
{
//    int i;

    uint32_t Err = ERR(RF, NOERROR);

//  uint16_t acquire_number;
    int act = ParamInt("ACT");

    Printf("[EWPSU] \r\n");
    switch(act)
    {
        case 0:

            Err = start_rftask(RF_COMM_PS_GATHER_DATA);         // ＴコマンドパラメータからＲＦタスク起動
            break;

        case 1:                                                 // ＲＦタスク終了後にデータを返す
            process_in_action1:;

//            if(in_rf_process == false){
/*TODO
            if(TX_SUSPENDED != rf_thread.tx_thread_state){
                Err = ERR(RF, BUSY);     // ＲＦタスクまだ起動してない
            }
            else TODO*/if(tx_mutex_get(&g_rf_mutex, WAIT_100MSEC) == TX_SUCCESS)    // ＲＦタスク終了
            {
                tx_mutex_put(&g_rf_mutex);

                if(RF_buff.rf_res.status == END_OK)             // 全データ確保
                {
                    //StatusPrintf("TIME", "%lu", RF_buff.rf_res.time);       // コマンド受信から無線通信開始までの秒数［秒］
                    //StatusPrintf("LAST", "%lu", timer.int125 * 125uL);      // 無線通信後の経過秒数［ｍｓ］（まだカウント中、次のコマンドが実行されるまで止まらない）

                    uncmpressData( 1 );        //圧縮データの回答処理（データ＋Propertyセット） 2020.01.29 サブ関数化

                    goto process_in_action3;
                }
            }
            else{
                Err = ERR(RF, BUSY);                            // ＲＦタスク実行中
            }

            break;

        case 3:                                                 // ＲＦタスク実行中でもデータを返す
            process_in_action3:;

            // property.acquire_number      ：ＲＦから受信済みのデータバイト数
            // property.transferred_number  ：ホストへ転送済みのデータバイト数
            // r500.data_size               ：ＰＣへ転送予定のデータバイト数

            if((ERRORCODE.ACT0 == ERR(RF, NOERROR)) && (memcmp(latest, PLIST.Arg[0].Name, 5) == 0))
            {
                if(r500.node == COMPRESS_ALGORITHM_NOTICE){
                    goto process_in_action1;    // 圧縮処理されている場合
                }

                //Sub_CF_DataProcess(r500.data_size, 1);       //データ転送処理 2020.Feb.03 サブ関数化
                Sub_CF_DataProcess(r500.data_size, 2);       //データ転送処理 2020.Feb.03 サブ関数化

                /*
                acquire_number = property.acquire_number;       // ＲＦから受信済みのデータバイト数ラッチ

                if((acquire_number > 0) && (acquire_number >= property.transferred_number))
                {
                    i = (r500.data_size <= property.transferred_number) ? 0 : r500.data_size - property.transferred_number;


                    StatusPrintf("SIZE", "%u", i);              // ＡＣＴ＝１未転送の吸い上げデータバイト数

                    if(i > (uint16_t)(acquire_number - property.transferred_number)){
                        i = (uint16_t)(acquire_number - property.transferred_number);   // 受信済みバイト数で制限
                    }

                    if(i > 1024){
                        i = 1024;                       // 最大バイト数制限
                    }

                    if(r500.data_size > acquire_number){
                        i &= (uint16_t)(~0x3f); // ６４バイト区切り 全受信済みなら端数あり
                    }

                    // ここまで吸い上げたデータの転送処理

                    if(i > 0)
                    {
                        memcpy((char *)&(StsArea[CmdStatusSize]), "DATA=", 5);

                        StsArea[CmdStatusSize + 5] = (uint8_t)i;
                        StsArea[CmdStatusSize + 6] = (uint8_t)(i / 256);

                        CmdStatusSize += 7;                     // 転送データ格納域

                        memcpy((char *)&StsArea[CmdStatusSize], (char *)&huge_buffer[property.transferred_number], (size_t)i);  // 受信データコピー

                        CmdStatusSize += i;
                        property.transferred_number = (uint16_t)(property.transferred_number + i);
                    }
                }
                */
            }

//            if(in_rf_process == false){
/*TODO
            if(TX_SUSPENDED != rf_thread.tx_thread_state){
                Err = ERR(RF, BUSY);     // ＲＦタスクまだ起動してない
            }
            else TODO*/if(tx_mutex_get(&g_rf_mutex, WAIT_100MSEC) == TX_SUCCESS)
            {
                tx_mutex_put(&g_rf_mutex);

                if(r500.data_size == property.transferred_number)
                {
                    StatusPrintf("TIME", "%lu", RF_buff.rf_res.time);   // コマンド受信から無線通信開始までの秒数［秒］
                    StatusPrintf("LAST", "%lu", timer.int125 * 125uL);  // 無線通信後の経過秒数［ｍｓ］（まだカウント中、次のコマンドが実行されるまで止まらない）
                    //StatusPrintf("END", "%u", 1); // 転送終了
                }

                Err = ERRORCODE.ACT0;
                ERRORCODE.ACT0 = ERR(RF, NOERROR);
            }
            else{
                Err = ERR(RF, BUSY);                            // ＲＦタスク実行中
            }

            break;

        case 2:
            Err = Sub_Act_2();    //ACT=2の処理 2020.01.29
            break;

        default:

            Err = ERR(CMD, FORMAT);
            break;
    }

    return(Err);
}




/**
 * @brief   [EWPSP] PUSH 記録データ消去
 * @return
 * @attention case 0の パラメータ違いだけのコピペ関数が13個ある
 */
uint32_t CFPushRecErase(void)
{
    uint32_t Err = ERR(RF, NOERROR);

    switch(ParamInt("ACT"))
    {
        case 0:

            Err = start_rftask(RF_COMM_PS_DATA_ERASE);          // ＴコマンドパラメータからＲＦタスク起動
            break;

        case 1:

            Err = ERRORCODE.ACT0;
            ERRORCODE.ACT0 = ERR(RF, NOERROR);

            RF_buff.rf_res.status = NULL_CODE;
            break;

        case 2:
            Err = Sub_Act_2();    //ACT=2の処理 2020.01.29
            break;

        default:

            Err = ERR(CMD, FORMAT);
            break;
    }

    return(Err);
}


/**
 * @brief   [EWOLR] PUSHの品目リスト読み込み（無線）
 * @return
 */
uint32_t CFPushItemListRead(void)
{
    uint32_t  Err = ERR(RF, NOERROR);

    switch(ParamInt("ACT"))
    {
        case 0:

            Err = start_rftask(RF_COMM_PS_ITEM_LIST_READ);          // ＴコマンドパラメータからＲＦタスク起動
            break;

        case 1:

            if((ERRORCODE.ACT0 == ERR(RF, NOERROR)) && (RF_buff.rf_res.status == END_OK) && (memcmp(latest, PLIST.Arg[0].Name, 5) == 0))
            {
                uncmpressData( 0 );    // 圧縮データを解凍 サブ関数化 2020.01.29

                StatusPrintfB("DATA", huge_buffer, r500.data_size);
            }

            Err = ERRORCODE.ACT0;
            ERRORCODE.ACT0 = ERR(RF, NOERROR);

            RF_buff.rf_res.status = NULL_CODE;
            break;

        case 2:
            Err = Sub_Act_2();    //ACT=2の処理 2020.01.29
            break;

        default:

            Err = ERR(CMD, FORMAT);
            break;
    }

    return(Err);
}



/**
 * @brief   [EWOLW] PUSHの品目リスト書き込み（無線）
 * @return
 * @attention case 0の パラメータ違いだけのコピペ関数が13個ある
 */
uint32_t CFPushItemListWrite(void)
{
    uint32_t  Err = ERR(RF, NOERROR);

    switch(ParamInt("ACT"))
    {
        case 0:

            Err = start_rftask(RF_COMM_PS_ITEM_LIST_WRITE);     // ＴコマンドパラメータからＲＦタスク起動
            break;

        case 1:

            Err = ERRORCODE.ACT0;
            ERRORCODE.ACT0 = ERR(RF, NOERROR);

            RF_buff.rf_res.status = NULL_CODE;
            break;

        case 2:
            Err = Sub_Act_2();    //ACT=2の処理 2020.01.29
            break;

        default:

            Err = ERR(CMD, FORMAT);
            break;
    }

    return(Err);
}


/**
 * [EWPLR] PUSHの作業者リスト読み込み（無線）
 * @return
 */
uint32_t CFPushPersonListRead(void)
{
    uint32_t Err = ERR(RF, NOERROR);

    switch(ParamInt("ACT"))
    {
        case 0:

            Err = start_rftask(RF_COMM_PS_WORKER_LIST_READ);    // ＴコマンドパラメータからＲＦタスク起動
            break;

        case 1:

            if((ERRORCODE.ACT0 == ERR(RF, NOERROR)) && (RF_buff.rf_res.status == END_OK) && (memcmp(latest, PLIST.Arg[0].Name, 5) == 0))
            {
                uncmpressData( 0 );    // 圧縮データを解凍 サブ関数化 2020.01.29
                StatusPrintfB("DATA", huge_buffer, r500.data_size);
            }

            Err = ERRORCODE.ACT0;
            ERRORCODE.ACT0 = ERR(RF, NOERROR);

            RF_buff.rf_res.status = NULL_CODE;
            break;

        case 2:
            Err = Sub_Act_2();    //ACT=2の処理 2020.01.29
            break;

        default:

            Err = ERR(CMD, FORMAT);
            break;
    }

    return(Err);
}


/**
 * [EWPLW] PUSHの作業者リスト書き込み（無線）
 * @return
 * @attention case 0の パラメータ違いだけのコピペ関数が13個ある
 */
uint32_t CFPushPersonListWrite(void)
{
    uint32_t Err = ERR(RF, NOERROR);

    switch(ParamInt("ACT"))
    {
        case 0:

            Err = start_rftask(RF_COMM_PS_WORKER_LIST_WRITE);   // ＴコマンドパラメータからＲＦタスク起動
            break;

        case 1:

            Err = ERRORCODE.ACT0;
            ERRORCODE.ACT0 = ERR(RF, NOERROR);

            RF_buff.rf_res.status = NULL_CODE;
            break;

        case 2:
            Err = Sub_Act_2();    //ACT=2の処理 2020.01.29
            break;

        default:

            Err = ERR(CMD, FORMAT);
            break;
    }

    return(Err);
}


/**
 * @brief   [EWPWR] PUSHのワークグループ読み込み（無線）
 * @return
 */
uint32_t CFPushWorkgrpRead(void)
{
    uint32_t  Err = ERR(RF, NOERROR);

    switch(ParamInt("ACT"))
    {
        case 0:

            Err = start_rftask(RF_COMM_PS_WORK_GROUP_READ);     // ＴコマンドパラメータからＲＦタスク起動
            break;

        case 1:

            if((ERRORCODE.ACT0 == ERR(RF, NOERROR)) && (RF_buff.rf_res.status == END_OK) && (memcmp(latest, PLIST.Arg[0].Name, 5) == 0))
            {
                uncmpressData( 0 );    // 圧縮データを解凍 サブ関数化 2020.01.29

                StatusPrintfB("DATA", huge_buffer, r500.data_size);
            }

            Err = ERRORCODE.ACT0;
            ERRORCODE.ACT0 = ERR(RF, NOERROR);

            RF_buff.rf_res.status = NULL_CODE;
            break;

        case 2:
            Err = Sub_Act_2();    //ACT=2の処理 2020.01.29
            break;

        default:

            Err = ERR(CMD, FORMAT);
            break;
    }

    return(Err);
}


/**
 * @brief   [EWOLW] PUSHのワークグループ書き込み（無線）
 * @return
 * @attention case 0の パラメータ違いだけのコピペ関数が13個ある
 */
uint32_t CFPushWorkgrpWrite(void)
{
    uint32_t Err = ERR(RF, NOERROR);

    switch(ParamInt("ACT"))
    {
        case 0:

            Err = start_rftask(RF_COMM_PS_WORK_GROUP_WRITE);    // ＴコマンドパラメータからＲＦタスク起動
            break;

        case 1:

            Err = ERRORCODE.ACT0;
            ERRORCODE.ACT0 = ERR(RF, NOERROR);

            RF_buff.rf_res.status = NULL_CODE;
            break;

        case 2:
            Err = Sub_Act_2();    //ACT=2の処理 2020.01.29
            break;

        default:

            Err = ERR(CMD, FORMAT);
            break;
    }

    return(Err);
}



/**
 * @brief   [EWPMG] PUSHのメッセージ表示
 * @return
 * @attention case 0の パラメータ違いだけのコピペ関数が13個ある
 */
uint32_t CFPushMessage(void)
{
    uint32_t Err = ERR(RF, NOERROR);


    switch(ParamInt("ACT"))
    {
        case 0:

            Err = start_rftask(RF_COMM_PS_DISPLAY_MESSAGE);     // ＴコマンドパラメータからＲＦタスク起動
            break;

        case 1:

            Err = ERRORCODE.ACT0;
            ERRORCODE.ACT0 = ERR(RF, NOERROR);

            RF_buff.rf_res.status = NULL_CODE;
            break;

        case 2:
            Err = Sub_Act_2();    //ACT=2の処理 2020.01.29
            break;

        default:

            Err = ERR(CMD, FORMAT);
            break;
    }

    return(Err);
}


/**
 * @brief   [EWPRQ] PUSHのリモート測定
 * @return
 * @attention case 0の パラメータ違いだけのコピペ関数が13個ある
 */
uint32_t CFPushRemoteMeasure(void)
{
    uint32_t Err = ERR(RF, NOERROR);


    switch(ParamInt("ACT"))
    {
        case 0:
            Err = start_rftask(RF_COMM_PS_REMOTE_MEASURE_REQUEST); // ＴコマンドパラメータからＲＦタスク起動
            break;

        case 1:

            Err = ERRORCODE.ACT0;
            ERRORCODE.ACT0 = ERR(RF, NOERROR);

            RF_buff.rf_res.status = NULL_CODE;
            break;

        case 2:
            Err = Sub_Act_2();    //ACT=2の処理 2020.01.29
            break;

        default:

            Err = ERR(CMD, FORMAT);
            break;
    }

    return(Err);
}


/**
 * @brief   [EWPTM] PUSHの時計設定
 * @return
 * @attention case 0の パラメータ違いだけのコピペ関数が13個ある
 */
uint32_t CFPushClock(void)
{
    uint32_t Err = ERR(RF, NOERROR);


    switch(ParamInt("ACT"))
    {
        case 0:

            Err = start_rftask(RF_COMM_PS_SET_TIME);            // ＴコマンドパラメータからＲＦタスク起動
            break;

        case 1:

            Err = ERRORCODE.ACT0;
            ERRORCODE.ACT0 = ERR(RF, NOERROR);

            RF_buff.rf_res.status = NULL_CODE;
            break;

        case 2:
            Err = Sub_Act_2();    //ACT=2の処理 2020.01.29
            break;

        default:

            Err = ERR(CMD, FORMAT);
            break;
    }

    return(Err);
}


/**
 * @brief   [ETLAN] LAN通信試験コマンド
 * @return
 * @todo    戻り値が違う
 */
uint32_t CFTestLAN(void)
{

    int act,mode;

    uint32_t Err = ERR(LAN, NOERROR);

    act = ParamInt("ACT");
    mode = ParamInt("MODE");            // 0: Monitor 1:Warnig 2:Suction Data

    Printf("[ETLAN] act %d mode %d \r\n", act, mode);

    switch(ParamInt("ACT"))
    {
        case 0:         // 実行開始

            // sakaguchi 2020.09.16
            if(STATE_COMMAND == UnitState){     // 要再起動
                NetCmdStatus = ETLAN_INIT;
                Err = ERR(LAN, REBOOT);
                break;
            }

// sakaguchi 2020.09.17 ↓
#if 0
            if(TX_SUSPENDED != wifi_thread.tx_thread_state){    // ネットワーク接続中       // sakaguchi 2020.09.14
                NetCmdStatus = ETLAN_INIT;
                Err = ERR(LAN, INIT);
                break;
            }
#endif
            if(NETWORK_DOWN == NetStatus){         // ネットワーク未接続
                Printf("NetStatus NG[%d] %d\n",NetStatus, NetLogInfo.NetStat);
                NetCmdStatus = ETLAN_INIT;
                switch(NetLogInfo.NetStat){
                    case 400:
                    case 401:
                    case 402:
                    case 100:
                    case 101:
                    case 102:
                    case 103:
                        Err = ERR(LAN, OTHER);
                        break;
                    default:
                        Err = ERR(LAN, INIT);
                }
                break;
            }
// sakaguchi 2020.09.17 ↑

            if(EWAIT_res == 0){                                 // 自律動作中などは無効にする        2020/09/10 segi

            if(tx_mutex_get(&mutex_network_init, TX_NO_WAIT) == TX_SUCCESS){
                tx_mutex_put(&mutex_network_init);
                NetCmdStatus = ETLAN_INIT;
                switch(mode){
                    case 0:
                        SendMonitorFile(1);         // 70040 error
                        break;
                    case 1:
                        SendWarningFile(1, 0);
                        break;
                    case 2:
                        SendSuctionData(1);
                        break;
                    case 3:
                        SendLogData();
                        break;
                    default:
                        Err = ERR(CMD, FORMAT);
                        break;
                }
            }else{
                if(0 == NetLogInfo.NetStat){        // 起動直後でまだネットワーク接続していない
                    NetCmdStatus = ETLAN_INIT;
                    Err = ERR(LAN, INIT);
                }else{
                    NetCmdStatus = ETLAN_ERROR;
                    Err = ERR(LAN, OTHER);
                }
            }

            }                                                   // 自律動作中などは無効にする        2020/09/10 segi
            else{
                NetCmdStatus = ETLAN_BUSY;                      // 自律動作中などは無効にする        2020/09/10 segi
                //Err = ERR(LAN, BUSY);                         // ACT=0は正常でACT=1はBUSYにする   2020/09/10 segi
            }

            break;

        case 1:         // ステータス取得
            switch (NetCmdStatus)
            {
                case ETLAN_BUSY:
                    Err = ERR(LAN, BUSY);
                    break;
                case ETLAN_SUCCESS:
                    Err = ERR(LAN, NOERROR);
                    break;
                case ETLAN_ERROR:
                    Err = ERR(LAN, OTHER);
                    break;
                case ETLAN_INIT:
                    Err = ERR(LAN, INIT);
                    break;
                default:
                    break;
            }

            break;

        case 2:         // キャンセル

            Err = ERR(LAN, NOERROR);
            break;

        default:

            Err = ERR(CMD, FORMAT);
            break;
    }

    return(Err);
}

//extern void SendWarningTestNew(uint32_t pat);
/**
 * @brief   [ETWAR] LAN通信試験コマンド
 * @return  結果（常にNOERROR）
 */
uint32_t CFTestWarning(void)
{
    int upper,lower,sensor,comm,batt;
    uint32_t pat = 0;

    //uint32_t Err = ERR(CMD, NOERROR);
    uint32_t Err = ERR(LAN, NOERROR);                   // sakaguchi 2020.09.09

    // sakaguchi 2020.09.16
    if(STATE_COMMAND == UnitState){                     // 要再起動
        NetCmdStatus = ETLAN_INIT;
        Err = ERR(LAN, REBOOT);
        return(Err);
    }

    upper  = ParamInt("UPPER");                          // 0: 無効　1:有効
    lower  = ParamInt("LOWER");
    sensor = ParamInt("SENSOR");
    comm   = ParamInt("COMM");
    batt   = ParamInt("BATT");

    if(upper){
        pat = pat | WTEST_UPPER;
    }
    if(lower){
        pat = pat | WTEST_LOWER;
    }
    if(sensor){
        pat = pat | WTEST_SENSOR;
    }
    if(comm){
        pat = pat | WTEST_COMM;
    }
    if(batt){
        pat = pat | WTEST_BATT;
    }

    Printf("[ETWAR] pat %02X \r\n", pat);

    pat = 0xffffffff;     //テストパターン＝全ビット
    XML.Create(0);

    if(tx_mutex_get(&mutex_network_init, TX_NO_WAIT) == TX_SUCCESS){
        tx_mutex_put(&mutex_network_init);
        if(SendWarningFile(2, pat) != 0){
            Err = ERR(LAN, OTHER);
        }
    }else{
         Err = ERR(LAN, OTHER);         // sakaguchi 2020.09.09
    }
    return(Err);
}



/**
 * @brief   [EWAIT]  自律動作の一時停止
 * @return 実行結果
 * @note    2020.07.01  修正
 */
uint32_t  CFWait(void)
{
    uint32_t Err = ERR(CMD, NOERROR);
    int32_t sec;

    sec = ParamInt("SEC");

    if((0 <= sec) && (sec <= 250)){
        WaitRequest = WAIT_REQUEST;             // 一時停止要求
        mate_time_flag = 1;                     // 自律動作の一時停止フラグON
        mate_time = sec;                        // 自律動作の一時停止時間


        StatusPrintf("AUTOSTAT", "%u", EWAIT_res);
        //DebugLog(LOG_DBG, "EWAIT sec=%ld st=%u", sec, EWAIT_res);

    }else{
        Err = ERR(CMD, FORMAT);
    }

    return(Err);
}


/**
 * @brief   [EDWLS]  記録データ吸い上げイベント
 * @return  実行結果
 * @note    2021.01.21  追加
 */
uint32_t  CFEventDownload(void)
{
    uint32_t Err = ERR(CMD, NOERROR);
    uint32_t Flag = FLG_EVENT_SUCTION;

    tx_event_flags_set (&g_wmd_event_flags, Flag, TX_OR);       // 自律動作用イベントフラグ

    return(Err);
}


/**
 * @brief   [ERGRP] 指定グループの無線通信設定を取得する
 * @return  実行結果
 * @note    2020.01.20  修正
 */
uint32_t CFReadSettingsGroup(void)
{

    int32_t group;

    uint32_t seri,num,chk_seri;
    uint32_t sz_seri;

    char seri_tmp[2];
    uint32_t cnt,i,j;
    char *para, *poi;

    uint32_t Err = ERR(CMD, NOERROR);

    RouteArray_t route;                                     // 中継ルート
    memset(&route, 0x00, sizeof(RouteArray_t));

    sz_seri = (uint32_t)ParamAdrs("SER",&para) ;
//  if ( sz_seri > 0 ) {    // パラメータにSERがあるか
    if (( sz_seri > 0 )&&( sz_seri < 9 )) { // パラメータにSERがあるか
        seri = 0;
        poi = para;

        for(cnt = 0 ; cnt < sz_seri ; cnt++){
            seri = seri * 16;
            seri_tmp[0] = *poi;
            seri_tmp[1] = 0x00;
            seri += (uint32_t)strtol(seri_tmp,NULL,16);
            poi++;
        }
    }
    else{
        seri = UINT32_MAX;
    }

    group = ParamInt("GRP");

    if(seri != UINT32_MAX){
        chk_seri = get_regist_group_adr_ser(seri);          // シリアルNo.からグループ、子機登録を読み出す。
        if(250 >= chk_seri){
            group = (int32_t)chk_seri;              // グループNo.
            num = ru_registry.rtr501.number;    // 子機番号
            group_registry.altogether = get_regist_relay_inf((uint8_t)group, (uint8_t)num, rpt_sequence);   // 中継機情報を組み立て（登録ファイルから指定されたグループ番号のグループ情報をグループ情報構造体にセット）
            StatusPrintfB("ID", group_registry.id, 8);
            StatusPrintf_v2s( "NUM", &ru_registry.rtr501.number, sizeof(ru_registry.rtr501.number), "%d");
            StatusPrintf_v2s( "CH", &group_registry.frequency, sizeof(group_registry.frequency), "%d");
            StatusPrintf_v2s( "BAND", &group_registry.band, sizeof(group_registry.band), "%d");

            if( 1 == (root_registry.auto_route & 0x01) ){     // 自動ルート機能ONの場合
                GetRoot_from_ResultTable((uint8_t)group, 1, (int)num, &route);
                StatusPrintfB("RELAY", (char *)route.value, route.count);
            }else{
                StatusPrintfB("RELAY", rpt_sequence, group_registry.altogether);
            }

            Err = ERR(CMD,NOERROR);
        }                       //### シリアルNo.が存在しない場合 ###
        else{//
//          return(ERR(RF, PARAMISS));

// ----------↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓----------
            chk_seri = get_regist_group_adr_rpt_ser(seri);      // シリアルNo.からグループ、中継機登録を読み出す。
            if(250 >= chk_seri){
                group = (int32_t)chk_seri;              // グループNo.
                num = rpt_registry.number;          // 中継機番号
                group_registry.altogether = get_regist_relay_inf_relay((uint8_t)group, (uint8_t)num, rpt_sequence);
                StatusPrintfB("ID", group_registry.id, 8);                                              // 2020.05.22 追加
                StatusPrintf_v2s( "NUM", &rpt_registry.number, sizeof(rpt_registry.number), "%d");
                StatusPrintf_v2s( "CH", &group_registry.frequency, sizeof(group_registry.frequency), "%d");
                StatusPrintf_v2s( "BAND", &group_registry.band, sizeof(group_registry.band), "%d");

                if( 1 == (root_registry.auto_route & 0x01) ){     // 自動ルート機能ONの場合
                    GetRoot_from_ResultTable((uint8_t)group, 0, (int)num, &route);
                    StatusPrintfB("RELAY", (char *)route.value, (int)route.count);
                }else{
                    StatusPrintfB("RELAY", rpt_sequence, group_registry.altogether);
                }

                get_regist_scan_unit((uint8_t)group, 0);
                StatusPrintf_v2s( "RUMAX", &group_registry.max_unit_no, sizeof(group_registry.max_unit_no), "%d");
                StatusPrintf_v2s( "RPTMAX",    &max_rpt_no, sizeof(max_rpt_no), "%d");

                Err = ERR(CMD,NOERROR);
            }
            else{
// ----------↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑----------

            Err = ERR(RF, PARAMISS);

// ----------↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓----------
        }
// ----------↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑----------

        }


    }
    else if(group != INT32_MAX){

        if(0 == get_regist_group_adr((uint8_t)group)){          // 最初にグループ番号が存在するかを調べる
            //return (ERR(CMD, FORMAT));
            return (ERR(RF, PARAMISS));         // 存在しないGRPを指定された場合はパラメータエラーにする。2020/07/08 segi
        }


        get_regist_scan_unit((uint8_t)group, 0);


        j = 0;
        for(i=0;i<sizeof(group_registry.id);i++){
            j += (uint32_t)group_registry.id[i];
        }
        if(j!=0){

            StatusPrintfB("ID", group_registry.id, 8);
                StatusPrintf_v2s( "RUMAX", &group_registry.max_unit_no, sizeof(group_registry.max_unit_no), "%d");

            StatusPrintf_v2s( "RPTMAX",    &max_rpt_no, sizeof(max_rpt_no), "%d");
            StatusPrintf_v2s( "CH",    &group_registry.frequency, sizeof(group_registry.frequency), "%d");
            StatusPrintf_v2s( "BAND",  &group_registry.band, sizeof(group_registry.band), "%d");

            Err = ERR(CMD,NOERROR);
        }
        else
        {
            Err = ERR(CMD, FORMAT);     // SERもGRPもない場合はフォーマットエラー
        }

    }
    else{
        Err = ERR(CMD, FORMAT);     // SERもGRPもない場合はフォーマットエラー
    }

//  Err = ERR(CMD,NOERROR);

    return (Err);


}


/**
 * @brief   [EMONS] モニタリング結果を開始する
 * @return  実行結果
 * @note
 */
uint32_t CFStartMonitor(void)
{

    uint32_t Err = ERR(CMD, NOERROR);
    int32_t mode;                       // モード

    mode = ParamInt("MODE");
    if(INT32_MAX == mode){          // モード指定無し
//        Err = ERR(CMD, FORMAT);
        Err = ERR(RF, PARAMISS);    // パラメータエラー
        return(Err);                // 処理終了
    }

    if(tx_mutex_get(&g_rf_mutex, WAIT_100MSEC) == TX_SUCCESS)
    {
        tx_mutex_put(&g_rf_mutex);

        // レギュラーモニタリング
        if(0 == mode){
//            RF_regu_moni();
            tx_event_flags_set (&g_wmd_event_flags, FLG_EVENT_REGUMONI, TX_OR);
        }
        // フルモニタリング
        else if(1 == mode){
//            RF_full_moni();
            tx_event_flags_set (&g_wmd_event_flags, FLG_EVENT_FULLMONI, TX_OR);
        }
        else{
//            Err = ERR(CMD, FORMAT);                     // コマンドフォーマットエラー
            Err = ERR(RF, PARAMISS);    // パラメータエラー
        }
    }
    else{
        Err = ERR(RF, BUSY);                            // ＲＦタスク実行中
    }

    return (Err);
}


/**
 * @brief   [EMONR] 指定グループのモニタリング結果を取得する
 * @return  実行結果
 * @note    2020.01.20  修正
 */
uint32_t CFReadMonitorResult(void)
{

    uint32_t Err = ERR(CMD, NOERROR);                   // ステータス
    int32_t group;                                      // グループ番号
    char    outpara[REPEATER_MAX*REMOTE_UNIT_MAX+2];    // モニタリング結果出力用
    int size;                                           // テーブルサイズ
    uint16_t    adr,len;                                // アドレス
    uint8_t     i;                                      // ループカウンタ
    uint8_t     max_unit_no;                            // 最大子機番号
    uint8_t     read_rpt_no;                            // 中継機番号
    def_ru_registry     read_ru_regist;                 // 子機情報             // sakaguchi 2020.09.01
    def_group_registry  read_group_registry;            // グループ情報         // sakaguchi 2020.09.01
    uint8_t max_repeater_no;                            // 最大中継機番号       // sakaguchi 2020.09.01

    group = ParamInt("GRP");
    if(INT32_MAX == group){         // グループ指定無し
//        Err = ERR(CMD, FORMAT);
        Err = ERR(RF, PARAMISS);    // パラメータエラー
        return(Err);                // 処理終了
    }

// sakaguchi 2020.09.01 ↓
//    if(0 == get_regist_group_adr((uint8_t)group)){    // グループ番号が存在するかを調べる
//    adr = get_regist_group_adr((uint8_t)group);         // グループ番号が存在するかを調べる
    adr = RF_get_regist_group_adr((uint8_t)group, &read_group_registry);         // グループ番号が存在するかを調べる  // sakaguchi 2020.09.01
    if(0 == adr){
//      Err = ERR(CMD, FORMAT);
        Err = ERR(RF, PARAMISS);    // パラメータエラー
        return(Err);               // 処理終了
    }

    adr = (uint16_t)(adr + read_group_registry.length);                 // グループヘッダ分アドレスＵＰ     // sakaguchi 2020.09.01
    max_unit_no = 0;                                                    // 登録されている最大子機番号把握する為、最初は0としておく

    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS)          // シリアルフラッシュ ミューテックス確保 スレッド確定
    {
        // 最大中継機番号の取得
        max_repeater_no = 0;
        for(i = 0; i < read_group_registry.altogether; i++)             // 中継機情報分アドレスＵＰ         // sakaguchi 2020.09.01
        {
            serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), 2, (char *)&len);                // 中継機情報サイズを読み込む
            serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr + 3), 1, (char *)&read_rpt_no);    // 最大中継機番号を把握
            if(max_repeater_no < read_rpt_no){
                max_repeater_no = read_rpt_no;       // 最大中継機番号を更新
            }
            adr = (uint16_t)(adr + len);
        }
        for(i = 0; i < read_group_registry.remote_unit; i++)
        {
            serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), sizeof(read_ru_regist), (char *)&read_ru_regist);    // 子機情報構造体に読み込む

            if(max_unit_no < read_ru_regist.rtr501.number){
                max_unit_no = read_ru_regist.rtr501.number; // 最大子機番号を把握する
            }

            if((read_ru_regist.rtr501.header == 0xfa)||(read_ru_regist.rtr501.header == 0xf9)){
                max_unit_no = (uint8_t)(max_unit_no + 1);               // 574,576なら子機番号を2個使うのでインクリメント
            }

            adr = (uint16_t)(adr + read_ru_regist.rtr501.length);
        }

        tx_mutex_put(&mutex_sfmem);

        RF_Group[group-1].max_rpt_no = max_repeater_no;     // 最大中継機番号格納
        RF_Group[group-1].max_unit_no = max_unit_no;        // 最大子機番号格納
    }
// sakaguchi 2020.09.01 ↑

    StatusPrintf("GRP", "%ld", group);                                  // グループ番号

    StatusPrintf("UTC", "%lu", RF_Group[group-1].moni_time);            // モニタリングした時刻

    StatusPrintf("RUMAX", "%d", RF_Group[group-1].max_unit_no);         // 最大子機番号

//    if( 0 != RF_Group[group-1].max_unit_no ){                           // 子機が存在しない場合でも以下のパラメータは出力する

        memset(outpara,0x00,sizeof(outpara));
        size = RF_ParentTable_Get(outpara, group, 1, 0);
        StatusPrintfB("RUPARENT", outpara, size);                       // 子機の親番号（最新）

        memset(outpara,0x00,sizeof(outpara));
        size = RF_ParentTable_Get(outpara, group, 1, 1);
        StatusPrintfB("RUPARENT1", outpara, size);                      // 子機の親番号 (最新より1回前)

        memset(outpara,0x00,sizeof(outpara));
        size = RF_ParentTable_Get(outpara, group, 1, 2);
        StatusPrintfB("RUPARENT2", outpara, size);                      // 子機の親番号 (最新より2回前)

        memset(outpara,0x00,sizeof(outpara));
        size = RF_ParentTable_Get(outpara, group, 1, 3);
        StatusPrintfB("RUPARENT3", outpara, size);                      // 子機の親番号 (最新より3回前)

        memset(outpara,0x00,sizeof(outpara));
        size = RF_ParentTable_Get(outpara, group, 1, 5);
        StatusPrintfB("RUPARENTLAST", outpara, size);                   // 子機の親番号 (最後に通信できた親番号)

        memset(outpara,0x00,sizeof(outpara));
        size = RF_RssiTable_Get(outpara, group, 1);
        StatusPrintfB("RURSSI", outpara, size);                         // 子機と親番号の機器との電波強度

        memset(outpara,0x00,sizeof(outpara));
        size = RF_BatteryTable_Get(outpara, group, 1);
        StatusPrintfB("RUBATT", outpara, size);                         // 子機の電池残量
//    }



    StatusPrintf("RPTMAX", "%d", RF_Group[group-1].max_rpt_no);         // 最大中継機番号

//    if(0 != RF_Group[group-1].max_rpt_no){                              // 中継機が存在しない場合でも以下のパラメータは出力する

        memset(outpara,0x00,sizeof(outpara));
        size = RF_ParentTable_Get(outpara, group, 0, 0);
        StatusPrintfB("RPTPARENT", outpara, size);                      // 中継機の親番号（最新）

        memset(outpara,0x00,sizeof(outpara));
        size = RF_ParentTable_Get(outpara, group, 0, 1);
        StatusPrintfB("RPTPARENT1", outpara, size);                     // 中継機の親番号 (最新より1回前)

        memset(outpara,0x00,sizeof(outpara));
        size = RF_ParentTable_Get(outpara, group, 0, 2);
        StatusPrintfB("RPTPARENT2", outpara, size);                     // 中継機の親番号 (最新より2回前)

        memset(outpara,0x00,sizeof(outpara));
        size = RF_ParentTable_Get(outpara, group, 0, 3);
        StatusPrintfB("RPTPARENT3", outpara, size);                     // 中継機の親番号 (最新より3回前)

        memset(outpara,0x00,sizeof(outpara));
        size = RF_ParentTable_Get(outpara, group, 0, 5);
        StatusPrintfB("RPTPARENTLAST", outpara, size);                  // 中継機の親番号 (最後に通信できた親番号)

        memset(outpara,0x00,sizeof(outpara));
        size = RF_RssiTable_Get(outpara, group, 0);
        StatusPrintfB("RPTRSSI", outpara, size);                        // 中継機と親番号の機器との電波強度

        memset(outpara,0x00,sizeof(outpara));
        size = RF_BatteryTable_Get(outpara, group, 0);
        StatusPrintfB("RPTBATT", outpara, size);                        // 中継機の電池残量
//    }

    return (Err);
}


/**
 * @brief   [EMONC] モニタリング結果を削除する
 * @return  実行結果
 * @note    指定した機器の電波強度情報を削除する
 */
uint32_t CFClearMonitor(void)
{

    int32_t group;
    char    *para;
    int32_t sz_parent;
//    int32_t sz_ru;
    uint32_t Err = ERR(CMD, NOERROR);
    int     iRet;
    char    inpara[REMOTE_UNIT_MAX];
//    char    *outpara[REMOTE_UNIT_MAX];
//    int     psize;
    int     i;
    uint8_t trmno;
    uint32_t moni_time;

    memset(inpara, 0x00, sizeof(inpara));

    memset(realscan_buff.over_rpt,0x00,sizeof(realscan_buff.over_rpt));
    memset(realscan_buff.over_unit,0x00,sizeof(realscan_buff.over_unit));

    group = ParamInt("GRP");
    if(INT32_MAX == group){     // グループ指定無し
        iRet = RF_clear_rssi(GROUP_ALL, 0, 0);    // 全グループ、全機器の電波強度を削除
        if(iRet != 0){
            //Err = ERR(CMD, FORMAT);
            Err = ERR(RF, PARAMISS);    // パラメータエラー
        }

        if(Err == ERR(CMD, NOERROR)){
            // 全グループのモニタリング時刻を更新する
            moni_time = RTC_GetGMTSec();
            for( i=1; i<=GROUP_MAX; i++ ){
                RF_Group[i-1].moni_time = moni_time;
            }
        }

        return (Err);           // 全消去のためここで処理終了
    }

    if(GROUP_MAX < group){      // グループ番号異常
        //Err = ERR(CMD, FORMAT);
        Err = ERR(RF, PARAMISS);    // パラメータエラー
        return (Err);           // ここで処理終了
    }



    sz_parent = ParamAdrs("PARENT",&para);
    if(sz_parent == -1){        // 親機中継機指定無しの場合
        iRet = RF_clear_rssi((int)group, 0, REPEATER_ALL);      // グループ内の全中継機の電波強度を削除
        if(iRet != 0){
//            Err = ERR(CMD, FORMAT);
            Err = ERR(RF, PARAMISS);    // パラメータエラー
        }
    }
    else if(sz_parent == 0){    // 要素数0の場合は処理なし
        // 処理なし
    }
    else{                       // 親機中継機指定有り
        memcpy(inpara, para, (size_t)sz_parent);
//        psize = split(inpara,"[,]", outpara);                   // 不要な記号を削除
        for(i=0; i<(int)sz_parent; i++){
//            trmno = (uint8_t)atoi(outpara[i]);                  // 削除する中継機番号を取得
            trmno = inpara[i];
            iRet = RF_clear_rssi((int)group, 0, trmno);         // 指定した中継機番号の電波強度を削除
            if(iRet != 0){
//                Err = ERR(CMD, FORMAT);
                Err = ERR(RF, PARAMISS);    // パラメータエラー
                break;                                          // エラーの時点で終了
            }
        }
    }

// RUパラメータは仕様から削除
#if 0
    sz_ru = ParamAdrs("RU", &para);
    if(sz_ru == -1){            // 子機指定無しの場合
        iRet = RF_clear_rssi((int)group, 1, REMOTE_UNIT_ALL);   // グループ内の全子機の電波強度を削除
        if(iRet != 0){
//            Err = ERR(CMD, FORMAT);
            Err = ERR(RF, PARAMISS);    // パラメータエラー
        }
    }
    else if(sz_ru == 0){        // 要素数0の場合は処理なし
        // 処理なし
    }
    else{
        memset(inpara, 0x00, sizeof(inpara));
        memcpy(inpara, para, (size_t)sz_ru);
//        psize = split(inpara,"[,]", outpara);                   // 不要な記号を削除
        for(i=0; i<(int)sz_ru; i++){
//            trmno = (uint8_t)atoi(outpara[i]);                  // 削除する子機番号を取得
            trmno = inpara[i];
            iRet = RF_clear_rssi((int)group, 1, trmno);         // 指定した子機番号の電波強度を削除
            if(iRet != 0){
//                Err = ERR(CMD, FORMAT);
                Err = ERR(RF, PARAMISS);    // パラメータエラー
                break;                                          // エラーの時点で終了
            }
        }
    }
#endif

    if(Err == ERR(CMD, NOERROR)){
        RF_Group[group-1].moni_time = RTC_GetGMTSec();                                 // モニタリングした時刻を保存
    }

    return (Err);
}


/**
 * @brief   [KENSA] 検査コマンド
 * @return  実行結果
 */
uint32_t CFKensa(void)
{

    int16_t cmd;

    uint32_t    Err;

    Err = ERR( CMD, NOERROR );

    cmd = (int16_t)ParamInt("CMD");



//----- segi ------------------------------------------------------------------------------------

//  if(cmd = 0){
//      init_factory_system_default(0x5fa00001);
//  }
//  else if(cmr == 1){
        // ＲＦタスク実行
        if(tx_mutex_get(&g_rf_mutex, WAIT_100MSEC) == TX_SUCCESS)   // ＲＦタスク実行中ではないこと
        {
                tx_mutex_put(&g_rf_mutex);

                rf_command = RF_EVENT_EXECUTE;                                  // コマンド番号セット

            //  status = tx_thread_resume(&rf_thread);
            //  tx_thread_sleep (1);
            //  rf_thread_entry();


            dbg_cmd = 0;

            switch(cmd){
                case 0:
                    RF_monitor_execute(0);  // モニタリングと警報監視
                    XML.Create(0);
                    MakeMonitorXML( 0 );
                    break;

                case 1:
                    RF_monitor_execute(1);  // モニタリングのみ
                    MakeMonitorFTP();
                    XML.Create(0);
                    MakeMonitorXML( 0 );
//sakaguchi ↓
                    G_HttpFile[FILE_M].sndflg = SND_ON;                                 //[FILE]現在値データ送信
                    tx_event_flags_set (&event_flags_http, FLG_HTTP_READY, TX_OR);      //HTTPスレッド動作許可
                    //tx_thread_resume(&http_thread);
//sakaguchi ↑
                    //tx_thread_resume(&ftp_thread);
                    break;

                case 2:

                    break;

                case 3:
                    get_regist_group_adr_ser(0x5EB80002);       // 登録ファイルからシリアルNo.で情報取得
                    break;

                case 4:
                    RF_monitor_execute(0);  // モニタリングと警報監視
                    SendWarningFile(0, 0);

                    break;

                case 5:
                    RF_event_execute(1);                        // 自動データ吸上げ(リトライ)
                    break;


                case 6:
                    dbg_cmd = 6;                                // 自動データ吸上げ(全データ)
                    RF_event_execute(0);
                    XML.Create(0);
                    break;


                case 7:
                    for(int i=0;i<999;i++){
                        PutLog(LOG_DBG, "Test Log %d", i);
                    }
                    break;

                case 8:
                    break;

                case 9:
                    AUTO_control(0);                                // 自律動作 総合入り口の実験
                    break;



            }
        }

    return (Err);
}







/**
 *
 * @return
 */
uint16_t sram_inspc(void)
{
    char *poi;
    uint32_t cnt;
    uint16_t rtn = 0;

//  poi = (0x84000000);
    poi = &huge_buffer[0];
//  for(cnt = 0 ; cnt < 0x80000 ; cnt++){
    for(cnt = 0 ; cnt < 0x80 ; cnt++){
        *poi = (char)((0x80000 - cnt) % 0x100);
        poi++;
    }

//  poi = (0x84000000);
    poi = &huge_buffer[0];
//  for(cnt = 0 ; cnt < 0x80000 ; cnt++){
    for(cnt = 0 ; cnt < 0x80 ; cnt++){
        if(*poi != (char)((0x80000 - cnt) % 0x0100)){
            rtn = 0xff;
            break;
        }
        poi++;
    }

    return (rtn);
}


/**
 * [INSPC] 検査コマンド
 * @return
 */
uint32_t CFInspect(void)
{
    int16_t cmd;
    int pos;
    uint16_t res;
    char    *data, *buff;

    uint16_t sz_buff;
    uint16_t sz_data;

    uint32_t    Err;

    Err = ERR( CMD, NOERROR );

    cmd = (int16_t)ParamInt("CMD");
    sz_data = (uint16_t)ParamAdrs("DATA",&data);

    if ( cmd != INT16_MAX ) {

        if ( !data ){
            sz_data = 0;
        }

        pos = CmdStatusSize;                    // DATA=の次の場所
        StatusPrintfB( "DATA", CmdTemp, 0 );    // dummy

        buff = &StsArea[pos+7];                 // デフォルト設定
        sz_buff = 0;

        memcpy( (char *)&XMLTemp[200], StsArea, (size_t)(pos+7) );      // 退避
        res = KensaMain( cmd, data, sz_data, &buff, &sz_buff ); // 実行
        memcpy( StsArea, (char *)&XMLTemp[200], (size_t)(pos+7) );      // 戻す

        Printf("[INSPC]  sz_buff %ld\r\n",sz_buff);
        if ( sz_buff > (sizeof(StsArea) - 100) ){
            sz_buff = sizeof(StsArea) - 100;
        }

        if ( buff != &StsArea[pos+7] )                  // ポインタ変更？
            memcpy( &StsArea[pos+7], buff, (size_t)sz_buff );   // コピー

        StsArea[pos+5] = (uint8_t)( sz_buff );
        StsArea[pos+6] = (uint8_t)( sz_buff >> 8 );
        CmdStatusSize += sz_buff;

        StatusPrintf( "RES", "%04X", res );     // ステータス

    }
    else{
        Err = ERR( CMD, FORMAT );
    }

    return (Err);
}


#if 0
//未使用
/**
 * @brief   [DPRNT] 検査コマンド
 * @return
 * @note    未使用
 */
uint32_t CFDebug(void)
{
//  uint16_t    cmd, pos, sz_buff, res;
//  uint8_t *data, *buff, *mem;
    uint32_t    Err;

    Err = ERR( CMD, NOERROR );

    //StatusPrintf("HTTPCT", "%lu", http_count);              // HTTPS実行回数
    //StatusPrintf("HTTPSC", "%lu", success_count);           // HTTPS成功回数

    return (Err);
}
#endif


/**
 * @brief   [DRSYS] 検査コマンド
 * @note    未公開　本体情報の読み出し
 * @return
 */
uint32_t CFReadSystemInfo(void)
{
    char str[5];

    uint32_t sn = fact_config.SerialNumber;
    uint8_t pd;

    sn = sn >> 28;
    pd = (uint8_t)sn;

    Printf("SN: %ld(%02X)\r\n", sn,pd);


    // 本体シリアル番号
    //fact_config.SerialNumber = 0x5f58ffff;
    StatusPrintf("SER", "%.8lX", fact_config.SerialNumber);
    StatusPrintf("TEST", "%.8lX", fact_config.TestNumber);

    // 本体ファームウェアバージョン
    StatusPrintf( "FWV", "%s", VERSION_FW);     // release

    // 販売者
    StatusPrintf("VENDER", "%02X", fact_config.Vender);
    StatusPrintf("COUNTRY", "%02X", fact_config.RfBand);

    StatusPrintf("BCAP", "%02X", fact_config.ble.cap_trim);
    StatusPrintf("BINT", "%02X", fact_config.ble.interval_0x9e_command);

    StatusPrintf("MAC1", "%04X%08X", fact_config.mac_h, fact_config.mac_l);
    StatusPrintf("MAC2", "%04X%08X", net_address.eth.mac1,net_address.eth.mac2);

    StatusPrintf("BDA","%12X", psoc.device_address);
    StatusPrintf("BID","%16X", psoc.chip_id);
     // ＢＬＥ ファームウェアバージョン
    memcpy(str, psoc.revision, 4);
    str[4] = 0x00;
    StatusPrintf( "BLV", "%s", &str);

    StatusPrintf_S(sizeof(my_config.device.TimeDiff),  "TDIFF", "%s",  my_config.device.TimeDiff);

    return(ERR(CMD, NOERROR));
}

/**
 * @brief   [DRSFM]
 * @note    未公開　SFMの読み出し 1Kbyte
 * @return
 */
uint32_t CFReadSFlash(void)
{

    int Size;
    int Offset;
    uint32_t adr;

    Printf("CMD:DRSFM\r\n");

    Offset = ParamInt32("OFFSET");
    Printf("Offset = %ld\r\n", Offset);
    if(!((0 <= Offset) && ( Offset <= 832))) {
        return(ERR(CMD, FORMAT));
    }
    adr = (uint32_t)(SFM_FACTORY_START + (uint32_t)Offset*1024);
    if(adr > (SFM_TD_LOG_END + 1)){
        Printf("Address Over Flow\r\n");
        return(ERR(CMD, FORMAT));   // 範囲内であること
    }

    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS){



        Size = 1024;

        memcpy((char *)&StsArea[CmdStatusSize], "DATA=", 5);
        CmdStatusSize += 5;

        StsArea[CmdStatusSize++] = (uint8_t)Size;
        StsArea[CmdStatusSize++] = (uint8_t)(Size / 256);

        serial_flash_multbyte_read(adr, (uint32_t)Size, (char *)&StsArea[CmdStatusSize]); // 読み込み

        CmdStatusSize += Size;

        Printf("Size=%ld\r\n", CmdStatusSize);

        tx_mutex_put(&mutex_sfmem);
        return ERR( CMD, NOERROR );
    }


     return(ERR(CMD, NOERROR));
}



/**
 * @brief   [R500C] スペシャル
 * 無線モジュールダイレクトコマンド
 *
 * @return
 * @note    2020.Feb.12 追加修正
 * @note    2020.Jun.   処理部を関数R500C_Direct()に分離
 */
uint32_t CFR500Special(void)
{
    int sz_data, mode, time;
    char *data;
    uint32_t    Err;

    Err = ERR(CMD, FORMAT);

    mode = ParamInt("MODE");
    time = ParamInt("TIME");
    sz_data = ParamAdrs("DATA", &data);

    switch(mode)
    {
        case 0:

            if((data == 0) || (time == INT32_MAX) || (sz_data == 0)){
                break;
            }

            Err = R500C_Direct(mode, time, data);       //コマンド処理部
            //処理部を関数R500C_Direct()に分離してrf_thread_entry.cに移動
#if 0
            RF_CS_ACTIVE;                                       // ＣＳ Ｌｏｗ（無線モジュール アクティブ）
            rf_delay(20);

            calc_checksum_data(data);                               // チェックサム計算、チェックサム付加

            rfm_send(data, (uint32_t)(data[3] + ((uint16_t)data[4] << 8) + 7));
            if(rfm_recv((uint32_t)time) == RFM_NORMAL_END)
            {
                StatusPrintfB("DATA", (char *)&RCOM.rxbuf.header, RCOM.rxbuf.length + 5);   // チェックサム部は送らない
                Err = ERR(RF,NOERROR);
            }
            else{
                     Err = ERR(RF, OTHER_ERR);
                 }

            //RF_CS_INACTIVE;                                           // ＣＳ Ｌｏｗ（無線モジュール スタンバイ）
#endif
            break;

        default:

            Err = ERR(CMD, FORMAT);
            break;
    }

    return (Err);
}


/**
 * @brief  [EFIRM] ファームデータ書き込み（キャッシュ）
 * @note データ構造 ヘッダ32Byte＋ファームデータ本体4KB すべてバイナリ
 * 64KBブロック の先頭4KB受信時にブロック消去64KB
 * 4KBサブブロック書き込み時にサブブロックのCRCチェック&サブブロックのCRCをデータフラッシュ保存する
 *
 * シリアルフラッシュ ROM書き換え用記録領域 (2.9375MByte)
 * 0x00100000-0x003effff  2f0000 3080192 Byte            <64KB block>
 * SPIフラッシュは64KBブロックエリア
 * S5D9のフラッシュは32KBブロックエリア
 * T2コマンドの仕様により一回のコマンド長は8192Byteに制限されるため4096Byteのサブブロックに分けて処理を行う
 * @note   2019.Dec.19 コード整理
 * @note   2020.Jun.18 PSoCファーム更新対応
 * @note   2020.Jun.19 無線モジュールファーム更新対応
 */
static uint16_t    block_no = 0xFFFF;    // 書込成功ブロックNOをクリア     // sakaguchi 2021.08.02 del
uint32_t CFWriteFirmData(void)
{
#if 0 // TODO
    char *pData;           ///<データへのポインタ
    uint16_t DataSize;
    uint16_t crc16 = 0;
    uint32_t sflash_adr;       ///<書き込み先アドレス（SPIフラッシュ）
    uint32_t df_address;        ///<データフラッシュアドレス
    uint32_t exec_flag;
    uint32_t ret_code;
    char readBack[4096];    ///<リードバック用バッファ（仮）

    ///ファームブロックヘッダ
    struct{
    //ブロック情報
       uint16_t firm_type;      ///<ファームウェア種類 1 = main, 2 = Wifi, 3 = PsoC, 4 = radio
       uint16_t cache_area;     ///<キャッシュエリア    1 = SPIフラッシュ
       uint16_t sub_block_no;   ///<サブブロック番号 4096Byteごとのブロック番号 0-511(0-15はブートローダなので書かない）
       uint16_t sub_block_size; ///<書き込みサブブロックサイズ 通常 4096Byte
       uint16_t sub_block_crc;  ///<サブブロック(4KB)のCRC
       uint16_t firm_flag;      ///<管理フラグ  0x0001=先頭フレーム（データフラッシュCRCクリア、パラメータ書き込み）
    //FW全体の情報
       uint32_t Version;        ///<ファームウェアバージョン
       uint32_t src_address;    ///<書き込み元アドレス
       uint32_t dst_address;    ///<書き込み先アドレス
       uint32_t write_size;     ///<全書き込みサイズ
       uint32_t fw_crc;         ///<FW全体のＣＲＣ

    }firm_header;

    // uint16_t    block_no = 0;       // 書込成功ブロックNO
    // uint16_t    block_no = 0xFFFF;    // 書込成功ブロックNOをクリア     // sakaguchi 2021.08.02 del

    memset(readBack, 0, sizeof(readBack)); //バッファクリア

    Printf("### EFIRM Command Exec \r\n");

    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS){     //シリアルフラッシュ mutex取得

        while(1){

            DataSize = (uint16_t)ParamAdrs("DATA", &pData);      // データ 32Byte + 4KBのはず
            Printf(" Size = %ld\r\n", DataSize);
            if((DataSize > 4096 + sizeof(firm_header)) || (DataSize < sizeof(firm_header)) ){   //範囲内であること ヘッダ32Byte必須 データは4096Byteまで
                ret_code = ERR(CMD, FORMAT);
                break;      //ループ抜ける
            }

            //ファームデータヘッダ取得
            memcpy((char *)&firm_header , pData, sizeof(firm_header));

// sakaguchi 2021.08.02 ↓
            //Printf("Rx Header SubBlock No = %d \n", firm_header.sub_block_no);
            //Printf("Rx Header SubBlock Size = %d \n", firm_header.sub_block_size);
            //Printf("Rx Header SubBlock CRC = %04X \n", firm_header.sub_block_crc);
            if(firm_header.firm_type == 0x0001){
                if(16 == firm_header.sub_block_no){  // メインファームの開始ブロック
                    PutLog(LOG_SYS,"Firmware Update Start");
                }else{
                    if(0xFFFF == block_no){         // 初回はメインファームの開始ブロック以外はエラーにする
                        ret_code = ERR(CMD, FORMAT);
                        PutLog(LOG_SYS,"Firmware Update Error[%d]",firm_header.sub_block_no);
                        break;      //ループ抜ける
                    }
                }
            }
// sakaguchi 2021.08.02 ↑
// sakaguchi 2021.05.28 ↓
            if(block_no == firm_header.sub_block_no){
                ret_code = ERR( CMD, NOERROR );
                Printf("Skip Block No = %d \n", block_no);
                break;      //ループ抜ける
            }
// sakaguchi 2021.05.28 ↑

            //メインファームは通常ブートローダ分の64KBオフセットしている
            if( (0 == firm_header.src_address) && (firm_header.firm_type == 0x0001)){
                firm_header.src_address = 0x00100000;       //キャッシュエリアの先頭アドレス
            }
            //SPIフラッシュSPIフラッシュアドレス
            sflash_adr = (uint32_t)(firm_header.src_address + (uint32_t)(firm_header.sub_block_no * 4096));
            //Printf("Flash Write Address = %04X \n", sflash_adr);      // sakaguchi 2021.08.02 del

            if((sflash_adr + firm_header.sub_block_size) > (0x03effff + 1)){
                ret_code = ERR(CMD, FORMAT);
                break;      //ループ抜ける
            }

            crc16 = crc16_bfr(&pData[sizeof(firm_header)], firm_header.sub_block_size);     //受信データ（FWデータ部分）のCRC16(CCITT計算）

            //Printf("Rx FW Data CRC = %04X \n", crc16);    // sakaguchi 2021.08.02 del
            if(crc16 != firm_header.sub_block_crc ){        //CRCエラー
                Printf("CRC Error 1 [%04X][%04X]\n", crc16, firm_header.sub_block_crc);        // sakaguchi 2021.08.02
                ret_code = ERR(CMD, FORMAT);
                break;      //ループ抜ける
            }
            if(0x0001 == firm_header.firm_flag){    //先頭ブロック
                flash_hp_open();
                eraseBlock_data_flash(FLASH_DF_64B_BLOCK0, 34);     //ブロック0からブロック33まで消去
                write_data_flash((uint32_t)&firm_header.Version ,FLASH_DF_64B_BLOCK0, 20);        //20Byte保存
                flash_hp_close();
            }

            //シリアルフラッシュ操作
            serial_flash_block_unlock();             // グローバルブロックプロテクション解除

            //セクタ消去
            Printf("SPI Flash Sector Erase \n");
            serial_flash_sector_erase(sflash_adr);    //4KBを消去する

            //FWデータ書き込み 4KB
            Printf("SPI Flash Sector Write \n");
            serial_flash_multbyte_write(sflash_adr, firm_header.sub_block_size, &pData[sizeof(firm_header)]);           //4KB 書き込み

            //リードバック CRCチェック
            crc16 = 0;
            serial_flash_multbyte_read(sflash_adr, firm_header.sub_block_size, readBack);

            crc16 = crc16_bfr(readBack, firm_header.sub_block_size);  //キャッシュデータのCRC16計算
            //Printf("Cache FW Data CRC = %04X \n", crc16);     // sakaguchi 2021.08.02 del

            if(crc16 != firm_header.sub_block_crc ){        //CRCエラー
                Printf("CRC Error 2 [%04X][%04X]\n", crc16, firm_header.sub_block_crc);         // sakaguchi 2021.08.02
                ret_code = ERR(CMD, FORMAT);
                break;      //ループ抜ける
            }


            //データフラッシュにCRC保存
            df_address = (uint32_t)(FLASH_DF_CRC_BLOCK2 + (firm_header.sub_block_no * 4));
            flash_hp_open();
            write_data_flash((uint32_t)&firm_header.sub_block_size ,df_address, 4);        //4Byte保存(size+crc)
            flash_hp_close();

            Printf("DF Address %08X\r\n", df_address);
            //リードバック CRCチェック
             if(*(uint32_t *)&firm_header.sub_block_size != *(uint32_t *)df_address){
                 Printf("Data Flash CRC Write Error \n");

                ret_code = ERR(CMD, FORMAT);
                break;      //ループ抜ける
             }
             if(0x0002 == firm_header.firm_flag){    //最終ブロック
                 if(firm_header.firm_type == 0x0001)    //メインファーム
                 {
                     exec_flag = 0x00000001;        //メインファームウェア更新実行（次回リセット時）
                 }
                 else if(firm_header.firm_type == 0x0003)   //BLE PSoCファームウェア
                 {
                     exec_flag = 0x00000003;        //PSoCファームウェア更新実行
                 }
                 else if(firm_header.firm_type == 0x0004)   //無線モジュール ファームウェア
                 {
                     exec_flag = 0x00000004;        //RFMファームウェア更新実行
                 }
                 else
                 {
                     exec_flag = 0;
                 }

                 flash_hp_open();
                 write_data_flash((uint32_t)&exec_flag ,FLASH_UPDATE_EXEC, 4);        //4Byte保存
                 flash_hp_close();

                 //ブランクのデータフラッシュは不定値でアクセスのたびに値が変わるので一致してしまうことがある 2020.Sep.15
//                 if(FLASH_RESULT_NOT_BLANK == CheckBlank_data_flash(FLASH_UPDATE_EXEC,4)){
                     //FW更新の実際の処理は各タスクに任せる
                     if(0x00000003 == *(uint32_t *)FLASH_UPDATE_EXEC)    //PSoC(BLE)ファーム更新
                     {
                         fPSoCUpdateMode = 1;             // PSoC PSoC FW更新モード ﾞ（直接通信モード）
                         tx_event_flags_set( &g_ble_event_flags, FLG_PSOC_UPDATE_REQUEST, TX_OR);
                         tx_thread_resume(&ble_thread);     //BLEスレッド起動
                     }
                     else if(0x00000004 == *(uint32_t *)FLASH_UPDATE_EXEC)        //無線モジュールファーム更新
                     {
                         fRadioUpdateMode = 1;      //RFM FW更新モード
                         tx_thread_resume(&rf_thread);     //RFスレッド起動
                     }
//                 }


             }

            Printf("Rx Header SubBlock No[%d] Size[%d] CRC[%04X] \n", firm_header.sub_block_no, firm_header.sub_block_size, firm_header.sub_block_crc); // sakaguchi 2021.08.02

 //            tx_mutex_put(&mutex_sfmem);
            block_no = firm_header.sub_block_no;    // 成功ブロックNO更新   // sakaguchi 2021.05.28
// sakaguchi 2021.06.15 ↓
            if(0x0002 == firm_header.firm_flag){    // 最終ブロック
                block_no = 0xFFFF;                  // 成功ブロックNOを初期値に戻す
// sakaguchi 2021.08.02 ↓
                if(firm_header.firm_type == 0x0001){    // メインファームの終了ブロックでログ出力
                    PutLog(LOG_SYS,"Firmware Update Success");
                }
// sakaguchi 2021.08.02 ↑
            }
// sakaguchi 2021.06.15 ↑
            ret_code = ERR( CMD, NOERROR );         //正常終了
            break;      //ループ抜ける 正常終了
        }//whileループ

        tx_mutex_put(&mutex_sfmem);     //ここでmutex返却

    }else{
        ret_code = ERR( CMD, RUNNING );    //シリアルフラッシュ実行中
    }
#else
    uint32_t ret_code;
    ret_code = ERR( CMD, NOERROR );
#endif // 0
    return (ret_code);
}

// sakaguchi 2021.08.02 ↓
/**
 * ファームアップデート中のチェック
 * @return  true/false（アップデート中／待受）
 */
bool Update_check(void)
{
    bool ret = false;

    if(block_no != 0xFFFF){
        ret = true;
    }

    return(ret);
}
// sakaguchi 2021.08.02 ↑

/**
 * CFSearchKoki(), CFSearchRelay(), CFExRealscan(),CFThroughoutSearch() の共通処理部
 * @param data_size データサイズ
 * @param print_sw  ステータス表示のスイッチ 0= ENDのみ、 0以外TIME.LAST.END
 * @return  エラーコード
 * @note     2020.01.29 CFSearchKoki(), CFSearchRelay(), CFExRealscan(),CFThroughoutSearch() の共通処理部を関数化
 * @note    2020.01.30  END以外のステータス表示もスイッチで対応
 */
uint32_t Sub_CFSearch(int data_size, int print_sw)
{
    uint32_t Err = ERR(RF, NOERROR);

//    if(in_rf_process == false){
/*TODO
    if(TX_SUSPENDED != rf_thread.tx_thread_state){
        Err = ERR(RF, BUSY);     // ＲＦタスクまだ起動してない
    }
    else TODO*/if(tx_mutex_get(&g_rf_mutex, WAIT_100MSEC) == TX_SUCCESS)    // ＲＦタスク実行中か？
    {
        tx_mutex_put(&g_rf_mutex);

        if((uint16_t)data_size == property.transferred_number)
        {
            if( 0 != print_sw){
                StatusPrintf("TIME", "%lu", RF_buff.rf_res.time);       // コマンド受信から無線通信開始までの秒数［秒］
                StatusPrintf("LAST", "%lu", timer.int125 * 125uL);      // 無線通信後の経過秒数［ｍｓ］（まだカウント中、次のコマンドが実行されるまで止まらない）
            }
            StatusPrintf("END", "%u", 1);   // 転送終了
        }

        Err = ERRORCODE.ACT0;
        ERRORCODE.ACT0 = ERR(RF, NOERROR);
    }
    else{
        Err = ERR(RF, BUSY);                           // ＲＦタスク実行中
    }

    return (Err);
 }

/**
 * 各コマンドのACT=2の処理
 * 基本的に各コマンド共通
 * @return  エラーコード
 * @note    2020.01.29  分離サブ関数化
 */
uint32_t Sub_Act_2(void)
{

    ERRORCODE.ACT0 = ERR(RF, NOERROR);
    RF_buff.rf_res.status = NULL_CODE;

    RF_buff.rf_req.cancel = 1;     // キャンセルフラグセット

    return(ERRORCODE.ACT0);

}

/**
 * R500 圧縮データの解凍処理
 * LZSS Decode
 * @param   sw  0 = データ解凍処理のみ  0以外= データ解凍処理＋Propertyセット
 * @note    2020.01.29  サブ関数化
 * @note    2020.01.30  Propertyのセットを引数でできるように変更
 */
void uncmpressData(int sw){
    if(r500.node == COMPRESS_ALGORITHM_NOTICE)      // 圧縮データを解凍
    {
        memcpy(work_buffer, huge_buffer, r500.data_size);
        r500.data_size = (uint16_t)LZSS_Decode((uint8_t *)work_buffer, (uint8_t *)huge_buffer);     // 2022.06.09

        if(0 != sw){
            property.acquire_number = r500.data_size;
            property.transferred_number = 0;

            r500.node = 0xff;                       // 解凍が重複しないようにする
        }

    }
}


/**
 * 各コマンドのデータ転送処理 DATA= のセット
 * CFWirelessSuction(), CFExRealscan(), CFThroughoutSearch(),
 * CFSearchKoki(), CFSearchRelay(), CFRecStartGroup(), CFPushSuction()の共通処理
 * @param maxDataSize   データサイズの最大値(データサイズ＝0の時のデフォルト値)
 * @param   sw  1 = 64Byte端数処理あり CFPushSuction()、 CFWirelessSuction() 用  2= SIZE出力あり CFWirelessSuction() 用
 * @note    2020.Feb.03 分離サブ関数化
 * @note  グローバル変数  CmdStatusSize, StsArea, huge_buffer, property, r500 使用
 * @see CFWirelessSuction(), CFExRealscan(), CFThroughoutSearch(), CFSearchKoki(), CFSearchRelay(), CFRecStartGroup(), CFPushSuction()
 */
void Sub_CF_DataProcess(int maxDataSize, int sw)
{
    int i;
    int data_size;
    int acquire_number = property.acquire_number;       // ＲＦから受信済みのデータバイト数ラッチ

//sakaguchi xxxxx ↓
    if(0 == memcmp(latest,"EWRSR", 5)){
        if(property.transferred_number == 0) property.transferred_number = 2;   // 先頭の２バイトを削除する（ＥＷＲＳＲのみ特別 先頭２バイトは中継機がＲＳＳＩと電池残量を付加するため、データバイト数が入っている）
    }
//sakaguchi xxxxx ↑


    if((acquire_number > 0) && (acquire_number >= property.transferred_number))
    {

        data_size = (r500.data_size == 0) ? maxDataSize : r500.data_size;

        i = (data_size <= property.transferred_number) ? 0 : (data_size - property.transferred_number);

        //CFWirelessSuction()コマンド用
        if(2 == sw){
           StatusPrintf("SIZE", "%u", i);              // ＡＣＴ＝１未転送の吸い上げデータバイト数
        }

        if(i > (acquire_number - property.transferred_number)){
            i = acquire_number - property.transferred_number;    // 受信済みバイト数で制限
        }

/*  2021.03.10  EWRSRにDATAが1024byteを超えた場合に送れない件   最大 3324 byte 子機100 中10 非圧縮
        if(i > 1024){
            i = 1024;                      // 最大バイト数制限
        }
*/
        if(i > 4096){
            i = 4096;                      // 最大バイト数制限
        }

        //64Byte区切り単数処理 CFPushSuction()用
        if(sw != 0){
            if(r500.data_size > acquire_number){
                i &= (uint16_t)(~0x3f); // ６４バイト区切り 全受信済みなら端数あり
            }
        }
        // ここまで受信したデータの転送処理
        if((i > 0) || (2 == sw))   //CFWirelessSuction()はデータ数ゼロでもDATAパラメータを出力する
        {
            memcpy((char *)&(StsArea[CmdStatusSize]), "DATA=", 5);

            StsArea[CmdStatusSize + 5] = (uint8_t)i;
            StsArea[CmdStatusSize + 6] = (uint8_t)(i / 256);

            CmdStatusSize += 7;                     // 転送データ格納域

            memcpy(&StsArea[CmdStatusSize], &huge_buffer[property.transferred_number], (size_t)i);  // 受信データコピー

            CmdStatusSize += i;
            property.transferred_number = (uint16_t)(property.transferred_number + i);
        }
    }
}


/**
 * [EBROL]  BLEクライアント、サーバー切り替え
 */
uint32_t CFRoleChange(void)          //
{
    uint32_t Err;
//  ssp_err_t err;
    Err = ERR(CMD, FORMAT);
    int mode;
    uint32_t actual_events = 0;

    mode = ParamInt("MODE");

    Err = ERR(CMD, NOERROR);

#if 0 //not in Linux?
    if(mode == 0)   //サーバ（ペリフェラル）モード
    {
        if(isBleRole() == ROLE_SERVER){
            // すでにサーバ設定済み
            during_scan = false;
            Err = ERR(CMD, RUNNING);    //コマンド実行中
        }
        else{   //クライアントからサーバへ切り替え

            g_sf_comms4.p_api->lock(g_sf_comms4.p_ctrl, SF_COMMS_LOCK_ALL, TX_WAIT_FOREVER);    //comm4ロック
            g_sf_comms4.p_api->close(g_sf_comms4.p_ctrl);
            during_scan = false;        //スキャン中フラグクリア

            g_ioport.p_api->pinWrite(PORT_BLE_ROLE_CHANGE, ROLE_SERVER);    // サーバ（ペリフェラル）役に変更
            Printf("### BLE Client to Server \r\n");

            tx_event_flags_get( &g_ble_event_flags, 0xFFFFFFFF, TX_OR_CLEAR, &actual_events, TX_NO_WAIT );  //全BLEイベントをクリアしておく

            g_sf_comms4.p_api->open(g_sf_comms4.p_ctrl, g_sf_comms4.p_cfg);
            g_sf_comms4.p_api->unlock(g_sf_comms4.p_ctrl, SF_COMMS_LOCK_ALL);    //comm4ロック解除

//            tx_event_flags_set( &g_ble_event_flags, FLG_PSOC_RESET_REQUEST, TX_OR); //PSoCリセット要求念のためPSoCリセット
            tx_thread_resume(&ble_thread);     //BLEスレッド起動

            g_external_irq2.p_api->enable(g_external_irq2.p_ctrl);                          //IRQ2イネーブル
        }
    }
    else    //クライアント（セントラル）
    {
        if(my_rpt_number > 0){
            Err = ERR(CMD, RUNNING);      //中継器設定中/ 中継機はクライアントになれない
        }
        else if(isBleRole() == ROLE_CLIENT){        //クライアント（セントラル）
            Err = ERR(CMD, RUNNING);    //コマンド実行中/ すでにクライアント設定済み
        }
        else{   //サーバからクライアント切り替え
            //サーバーからクライアントへの切り替えは時間が数秒かかる

            g_sf_comms4.p_api->lock(g_sf_comms4.p_ctrl, SF_COMMS_LOCK_ALL, TX_WAIT_FOREVER);    //comm4ロック
            g_sf_comms4.p_api->close(g_sf_comms4.p_ctrl);
            during_scan = false;        //スキャン中フラグクリア

            g_ioport.p_api->pinWrite(PORT_BLE_ROLE_CHANGE, ROLE_CLIENT);// クライアント（セントラル）役に変更
            Printf("### BLE Server to Client \r\n");

            tx_event_flags_get( &g_ble_event_flags, 0xFFFFFFFF, TX_OR_CLEAR, &actual_events, TX_NO_WAIT );  //全BLEイベントをクリアしておく

            g_sf_comms4.p_api->open(g_sf_comms4.p_ctrl, g_sf_comms4.p_cfg);
            g_sf_comms4.p_api->unlock(g_sf_comms4.p_ctrl, SF_COMMS_LOCK_ALL);    //comm4ロック解除
            tx_thread_resume(&ble_thread);     //BLEスレッド起動
            g_external_irq2.p_api->disable(g_external_irq2.p_ctrl);                          //IRQ2ディセーブル
        }
    }
#endif // 0

  return (Err);

}


/**
 * @brief   REBOOT要求のチェック
 * REBOOT要求イベントが発生している場合、REBOOT実行イベントフラグをセットする
 * @note    2020.Jun.10 各タスクに入っていたチェック部を関数にして分離
 */
void Reboot_check(void)
{
    uint32_t status = 0;
    uint32_t actual_events = 0;

    status = tx_event_flags_get (&event_flags_reboot, FLG_REBOOT_REQUEST ,TX_OR_CLEAR, &actual_events,TX_NO_WAIT);
//    Printf("\r\n   (%04X) !!!!!\r\n", status);
    // TX_NO_WAITなので要求フラグが無い場合は、TX_NO_EVENTS(0x07)になる為、この場合の判断にはstatusのチェックも必要！
    if((status == TX_SUCCESS ) && (actual_events & FLG_REBOOT_REQUEST)){
        Printf("\r\n   Reboot Request (%04X) !!!!!\r\n\r\n", actual_events);
        tx_thread_sleep (50);
        tx_event_flags_set(&event_flags_reboot, FLG_REBOOT_EXEC, TX_OR);        //リブート実行イベントシグナル セット
    }

}


//光通信
/**
 * 光通信 データ吸い上げ要求する関数
 * @param Cmd       0x48, 0x59, 0x6c command
 * @param DataSize
 * @return          ERR(IR, NOERROR) 失敗:ERR(IR, NORES)
 * @note   T commad
 * @note    2020.Jul.06 6個所あったキュー送信、イベントフラグ取得をopt_command_exec()に共通化
 */
uint32_t IR_header_read(uint8_t Cmd , uint16_t DataSize)
{

    optc.Cmd= Cmd;
    optc.DataSize = DataSize;
    optc.cmd_id[0] = ID_IR_HEADER_READ;

    opt_command_exec();         //T2からの光通信コマンド実行 (メッセージキューのセット及び「イベントフラグ待ち)

    return (optc.result);
}




/**
 * 光通信 SOH　通常コマンド
 *  データ長 4バイトの普通のコマンド
 * @param   Cmd     コマンド
 * @param   SubCmd
 * @param   length  データ長
 * @param   pTxData     フレームへセットとするデータへのポインタ
 * @return      成功:     失敗:
 * @note        T command
 *              length=0の場合は、実際の送信データは4byteのNULLを送る
 * @note    2020.Jul.06 6個所あったキュー送信、イベントフラグ取得をopt_command_exec()に共通化
 * @note    フレームへのデータセットを追加
 */
uint32_t IR_Nomal_command(uint8_t Cmd, uint8_t SubCmd, uint16_t length, char *pTxData)
{
    optc.cm = Cmd;
    optc.scm = SubCmd;
    optc.cmd_id[0] = ID_IR_SIMPLE_COMMAND;
    optc.len = (length > 64) ? 64 : length;

    optc.buf = pTxData;

    opt_command_exec();         //T2からの光通信コマンド実行 (メッセージキューのセット及びイベントフラグ待ち)

    return (optc.result);
}


/**
 * 光通信 ６４バイト情報を書き込む関数
 * @param c
 * @param buf
 * @return      成功:     失敗:
 * @note    T command
 * @note    2020.Jul.06 6個所あったキュー送信、イベントフラグ取得をopt_command_exec()に共通化
 */
uint32_t IR_64byte_write(char c, char *buf)
{
    optc.cm = c;
    optc.buf = buf;
    optc.cmd_id[0] = ID_IR_64BYTE_WRITE;

    opt_command_exec();         //T2からの光通信コマンド実行 (メッセージキューのセット及び「イベントフラグ待ち)
    return (optc.result);

}


/**
 * 光通信 ６４バイト情報を読み込む関数
 * @note    T command
 * @param c
 * @param buf
 * @return  成功:     失敗:
 * @note    2020.Jul.06 6個所あったキュー送信、イベントフラグ取得をopt_command_exec()に共通化
 */
uint32_t IR_64byte_read(char c, char *buf)
{

    optc.cm = c;
    optc.buf = buf;
    optc.cmd_id[0] = ID_IR_64BYTE_READ;

    opt_command_exec();         //T2からの光通信コマンド実行 (メッセージキューのセット及び「イベントフラグ待ち)

    return (optc.result);
}



/**
 * @brief           光通信 データ吸い上げ(1024byte毎)
 * @param out_poi   吸い上げデータを入れるポインタ
 * @param sw        0:吸い上げ開始        1:次データ要求
 * @param Cmd       0x48:個数指定        0x49:データ番号指定
 * @param GetTime   0x48:個数[個]指定    0x49:吸い上げデータ番号
 * @return          成功:吸い上げ全データ数、今回転送データ数 失敗:ERR(IR,NORES) 引き数異常:ERR(IR,PARAMISS)
 * @note            T comannd    / 0x47は無い
 * @note    2020.Jul.06 6個所あったキュー送信、イベントフラグ取得をopt_command_exec()に共通化
 */
uint32_t IR_data_download_t(char *out_poi, uint8_t sw, uint8_t Cmd, uint64_t GetTime)
{

    optc.Cmd = Cmd;
    optc.buf = out_poi;
    optc.GetTime = (uint32_t)GetTime;
    optc.sw = sw;
    optc.cmd_id[0] = ID_IR_DATA_DOWNLOAD_T;

    opt_command_exec();         //T2からの光通信コマンド実行 (メッセージキューのセット及び「イベントフラグ待ち) //2020.08.07 sakaguchi add

    return (optc.result);



}



/**
 * 光通信 T-Command SOH Direct
 * @param   pTx     転送データへのポインタ
 * @param   length  転送するデータバイト数
 * @return  成功:     失敗:
 * @note       T command
 * @note    2020.Jul.06 6個所あったキュー送信、イベントフラグ取得をopt_command_exec()に共通化
 * @note    2020.Jul.06 バッファ
 */
uint32_t IR_soh_direct(comSOH_t *pTx, uint32_t length)
{

    memcpy(&pOptUartTx->header, pTx, length);

    optc.cmd_id[0] = ID_IR_SOH_DIRECT;

    opt_command_exec();         //T2からの光通信コマンド実行 (メッセージキューのセット及び「イベントフラグ待ち)


    return (optc.result);

}



/**
 * 光通信 子機登録関数
 * @return      成功:     失敗:
 * @note    T command
 * @note    2020.Jul.06 6個所あったキュー送信、イベントフラグ取得をopt_command_exec()に共通化
 */
uint32_t IR_Regist(void)
{

    optc.cmd_id[0] = ID_IR_REGIST;

    opt_command_exec();         //T2からの光通信コマンド実行 (メッセージキューのセット及び「イベントフラグ待ち)

    return (optc.result);

}


/**
 * @brief   T2からの光通信コマンド実行
 * 各コマンド処理のメッセージキューのセット及び「イベントフラグ待ちを関数化
 * @return  イベントフラグ取得のステータス
 * @note    2020.Jul.06 6個所あったキュー送信、イベントフラグ取得を共通化
 */
uint32_t opt_command_exec(void){

    uint32_t   actual_events;
    uint32_t status;

    tx_queue_send(&optc_queue, optc.cmd_id, TX_WAIT_FOREVER);

    status = tx_event_flags_get (&optc_event_flags, FLG_OPTC_END, TX_OR_CLEAR, &actual_events,TX_WAIT_FOREVER); //光通信コマンド終了イベント待ち

    return(status);

}


//Cmd_func.c/hから移動
/**
 * @brief   返送パラメータのセット
 * @param Name
 * @param fmt
 * @note    グローバル変数使用
 *          StsArea[]       返送パラメータエリア
 *          CmdStatusSize   返送パラメータのサイズ（ポインタ）
 */
void StatusPrintf( char *Name, const char *fmt, ... )
{
    va_list args;

    int len;
    char    *sz, *ps = &(StsArea[CmdStatusSize]);

    va_start( args, fmt );

    len = (int)strlen( Name );
    strcpy( ps, Name );
    ps += len;
    *ps++ = '=';
    sz = ps;
    ps += 2;
    CmdStatusSize += len + 1 + 2;       // '=' + size area

    len = vsprintf( ps, fmt, args );
    CmdStatusSize += len;
    *sz++ = (uint8_t)len;
    *sz   = (uint8_t)(len>>8);

    va_end( args );
}


/**
 * @brief   返送パラメータのセット 最大サイズ指定
 * @param max
 * @param Name
 * @param fmt
 * @note    グローバル変数使用
 *          StsArea[]       返送パラメータエリア
 *          CmdStatusSize   返送パラメータのサイズ（ポインタ）
 */
void StatusPrintf_S(uint16_t max,  char *Name, const char *fmt,  ... )
{
    va_list args;

    int     len;
    char   *sz, *ps = &(StsArea[CmdStatusSize]);

    va_start( args, fmt );

    len = (int)strlen( Name );
    strcpy( ps, Name );
    ps += len;
    *ps++ = '=';
    sz = ps;
    ps += 2;
    CmdStatusSize += len + 1 + 2;       // '=' + size area

    len = vsprintf( ps, fmt, args );
    if(len > max)
        len = max;
    CmdStatusSize += len;
    *sz++ = (uint8_t)len;
    *sz   = (uint8_t)(len>>8);

    va_end( args );
}

/**
 * @brief   返送パラメータのセット 最大サイズ指定
 * @param Name
 * @param Data
 * @param size
 * @param fmt
 * @note    グローバル変数使用
 *          StsArea[]       返送パラメータエリア
 *          CmdStatusSize   返送パラメータのサイズ（ポインタ）
 */
void StatusPrintf_v2s(char *Name, char *Data, int size, const char *fmt)
{

    char   *Csz;
    int    len,sz;
    int i;
    char tmp[8];
    uint32_t val = 0;


    Csz = (char *)&(StsArea[CmdStatusSize]);

    len = (int16_t)strlen( Name );
    memcpy( Csz, Name, (size_t)len );
    Csz += len;

    *Csz++ = '=';

    memcpy( tmp, Data, (size_t)size );
    for(i= size -1; i>=0;i--){
        val = (val <<8 ) | (uint8_t)tmp[i];
    }

    sz = sprintf(tmp,fmt, val);

    *Csz++ = (uint8_t)( sz % 256 );         // Size L
    *Csz++ = (uint8_t)( sz / 256 );         // Size H

    memcpy( Csz, tmp, (size_t)sz );

    CmdStatusSize += len + 1 + 2 + sz;

}



/**
 * 返送パラメータのセット（バイナリ版）
 * @param Name
 * @param Data
 * @param Size
 * @note    グローバル変数使用
 *          StsArea[]       返送パラメータエリア
 *          CmdStatusSize   返送パラメータのサイズ（ポインタ）
 */
void StatusPrintfB( char *Name, char *Data, int Size )
{
    char   *Csz;
    int    len;

    Csz = (char *)&(StsArea[CmdStatusSize]);

    len = (int)strlen( Name );
    memcpy( Csz, Name, (size_t)len );
    Csz += len;

    *Csz++ = '=';

    *Csz++ = (uint8_t)( Size % 256 );         // Size L
    *Csz++ = (uint8_t)( Size / 256 );         // Size H

    if ( Size ){
        memcpy( Csz, Data, (size_t)Size );
    }

    CmdStatusSize += len + 1 + 2 + Size;
}




//###################################   End of file ########################################

