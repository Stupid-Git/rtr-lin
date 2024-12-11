/*
 * comp_cmd.c
 *
 *  Created on: Nov 25, 2024
 *      Author: karel
 */

#include "_r500_config.h"

//-----------------------------------------------------------------------------
#define COMP_CMD_C_
#include "comp_cmd.h"
//-----------------------------------------------------------------------------

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include <stdbool.h>

#include "tdx.h"



#include "random_stuff.h"
#include "file_structs.h"
#include "Error.h"
#include "comp_log.h"


#include "r500_defs.h"
#include "comp_cmd_sub.h"


/*static*/ void StatusPrintf( char *Name, const char *fmt, ... );                  // 返送パラメータのセット
/*static*/ void StatusPrintf_S(uint16_t max, char *Name, const char *fmt,  ...  ); // 返送パラメータのセット
/*static*/ void StatusPrintf_v2s(char *Name, char *Data, int size, const char *fmt);

#define     RF_CMD  1

typedef const struct{
    char    *Code;
    char    *Param[30];
    uint32_t    (* const Func)(void);
    int16_t     RestartFlag;        //
    int16_t     RF_Com;             // RF Command
} CMD_TABLE;

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


tdx_flags_t g_command_event_flags;
tdx_queue_t cmd_queue;


uint32_t Cmd_Route;          //コマンドの発給元  USB or BLE or TCP or HTTP

//プロトタイプ
static int AnalyzeCmd(char *pCmd, int Size, uint32_t route);                           // Ｔコマンド解析


static void* cmd_thread(void *socket_desc);
static void* cmd_thread(void *socket_desc)
{
//    uint32_t status = 0;
    uint32_t actual_events = 0;
    int i;
    uint16_t ParaSize;
    uint16_t Sum;
    int sz = 2;
    //    static uint32_t cmd_msg[4];        //メッセージキュー受信用
    static CmdQueue_t cmd_msg;        //メッセージキュー受信用
    uint8_t ucIdx;               //sakaguchi

    comT2_t *pComRx;      //コマンド処理する受信フレームのヘッダポインタ
    char *pComTx;      //コマンド処理した応答フレームのヘッダポインタ(まだ未使用)
    int Sum_Crc = 0;        //SUMの場合0 CRC16の場合1

    //LED_Set( LED_USBCOM, LED_OFF );
    //LED_Set( LED_LANCOM, LED_OFF );
Printf("cmd_thread Started\n");
    for(; ;) {
        /*status = *///tx_event_flags_get(&g_command_event_flags, FLG_CMD_EXEC, TX_OR_CLEAR, &actual_events, TX_WAIT_FOREVER);
        /*status = */tdx_flags_get(&g_command_event_flags, FLG_CMD_EXEC, /*TX_OR_CLEAR,*/ &actual_events, TX_WAIT_FOREVER);
Printf("cmd_thread Got Exec\n");
        if(actual_events & FLG_CMD_EXEC) {
            tdx_queue_receive(&cmd_queue, &cmd_msg, 0, 10);	//TX_NO_WAIT);
            //Printf("CMD %d\r\n", cmd_msg[0]);
            //SET_ERR(CMD, NOERROR);

            //            memset(StsArea, 0, sizeof(StsArea));
            memset(StsArea, 0, 6);  //先頭から6Byteだけクリア（念のため）

            CmdStatusSize = sprintf(StsArea, "T200");
            Sum = 0;
            //メッセージキューで受け取ったメッセージ
            Cmd_Route = cmd_msg.CmdRoute;         //コマンド要求元保存 (一部のコマンドで参照する)
            pComRx = (comT2_t*)cmd_msg.pT2Command;   //コマンド処理する受信データフレームの先頭ポインタ
            //Printf("%02X %02X %d\r\n", pComRx->command, pComRx->subcommand, pComRx->length);

            pComTx = cmd_msg.pT2Status;    //StsAreaでコマンド応答データ処理後、データをコピーする
            Printf("### CMD Route %d\r\n", Cmd_Route);
            //LED処理
            if(cmd_msg.CmdRoute == CMD_TCP) {
                if(my_config.network.Phy[0] == 0) {   // ETH
                    //LED_Set(LED_LANCOM, LED_ON);
                    //LED_Set(LED_LANON, LED_ON);
                } else {                                // WIFI
                    //LED_Set(LED_WIFICOM, LED_ON);
                    //LED_Set(LED_WIFION, LED_ON);
                    //wifi_module_scan();   削除 debugで使用
                }

            } else if(cmd_msg.CmdRoute == CMD_BLE) {
                //LED_Set(LED_BLECOM, LED_ON);
                //LED_Set(LED_BLEON, LED_ON);

            } else if(cmd_msg.CmdRoute == CMD_HTTP) {
                //TODO EWAIT_res = 3;                           // WSSコマンド実行中
                if(my_config.network.Phy[0] == 0) {
                    //LED_Set(LED_LANCOM, LED_ON);
                } else {
                    //LED_Set(LED_WIFICOM, LED_ON);
                }
            } else        // USB
            {
                //LED_Set(LED_USBCOM, LED_ON);
                //LED_Set(LED_USBON, LED_ON);
            }

            //コマンド解析(共通処理)
            Sum_Crc = judge_T2checksum(pComRx);
            if(-1 == Sum_Crc) {
                ERRORCODE.ERR = ERR(CMD, CHECKSUM);
                CmdStatusSize = (int32_t)(CmdStatusSize + sprintf(&StsArea[CmdStatusSize], "%.5s:", &pComRx->data[0]));
                DebugLog(LOG_DBG, "[%.5s] %04hd-%04hd (%d)", pComRx->data, (uint32_t)(ERRORCODE.ERR >> 16), (uint32_t)ERRORCODE.ERR, cmd_msg.CmdRoute);
            } else {
                AnalyzeCmd(pComRx->data, pComRx->length, cmd_msg.CmdRoute); //コマンド解析、コマンド実行
                if(cmd_msg.CmdRoute != CMD_USB) {
                    DebugLog(LOG_DBG, "[%.5s] %04hd-%04hd (%d)", pComRx->data, (uint32_t)(ERRORCODE.ERR >> 16), (uint32_t)ERRORCODE.ERR, cmd_msg.CmdRoute);
                }
            }

            //HTTPの場合追加処理
            if(cmd_msg.CmdRoute == CMD_HTTP) {
                //DebugLog(LOG_DBG,"HTTP:[%.5s] %04hd-%04hd", pComRx->data, (uint32_t)(ERRORCODE.ERR>>16), (uint32_t)ERRORCODE.ERR);
                // sakaguchi 2021.08.02 EFIRMはログを圧迫するため出力しない ↓
                if(memcmp(pComRx->data, "EFIRM", 5) != 0) {
                    PutLog(LOG_SYS, "HTTP:[%.5s] %04hd-%04hd", pComRx->data, (uint32_t)(ERRORCODE.ERR >> 16), (uint32_t)ERRORCODE.ERR);
                }
                // sakaguchi 2021.08.02 ↑
                ucIdx = (uint8_t)pComRx->command;                // 受信コマンドIndex
#if 0 // TODO HTTP
                // 操作リクエスト結果送信フラグ更新
                if(SND_OFF == G_HttpCmd[ucIdx].ansflg) {

                    G_HttpCmd[ucIdx].ansflg = SND_ON;       // [CMD] 操作リクエスト結果送信
                    G_HttpFile[FILE_A].sndflg = SND_ON;    // [FILE] 操作リクエスト結果送信

                } else if(SND_ACT0 == G_HttpCmd[ucIdx].ansflg) {

                    G_HttpCmd[ucIdx].ansflg = RCV_ACT0;        // [CMD] ACT0応答受信

                } else if(SND_ACT1 == G_HttpCmd[ucIdx].ansflg) {

                    G_HttpCmd[ucIdx].ansflg = RCV_ACT1;        // [CMD] ACT1応答受信

                } else if(SND_STEP1 == G_HttpCmd[ucIdx].ansflg) {

                    G_HttpCmd[ucIdx].ansflg = RCV_STEP1;      // [CMD] STEP1応答受信

                } else if(SND_STEP2 == G_HttpCmd[ucIdx].ansflg) {

                    G_HttpCmd[ucIdx].ansflg = RCV_STEP2;      // [CMD] STEP2応答受信

                } else if(SND_STEP3 == G_HttpCmd[ucIdx].ansflg) {

                    G_HttpCmd[ucIdx].ansflg = RCV_STEP3;      // [CMD] STEP3応答受信
                }

                G_HttpCmd[ucIdx].status = (ULONG)ERRORCODE.ERR;   // [CMD] 結果格納
                tx_event_flags_set(&event_flags_http, FLG_HTTP_READY, TX_OR); // HTTPスレッド動作許可ON
                EWAIT_res = 0;                                    // WSSコマンド実行終了
#endif
            }

            //共通処理
            StatusPrintf("STATUS", "%04hd-%04hd", (uint32_t)(ERRORCODE.ERR >> 16), (uint32_t)ERRORCODE.ERR);

            ParaSize = (uint16_t)(CmdStatusSize - 2 - sz);

            *(uint16_t*)&StsArea[sz] = ParaSize;

            Sum = 0;
            for(i = 2 + sz ; i < CmdStatusSize ; i++) {
                Sum = (uint16_t)(Sum + StsArea[i]);
            }

            *(uint16_t*)&StsArea[CmdStatusSize] = Sum;
            CmdStatusSize += 2;

            //メッセージキューにポインタがセットされている場合、コマンド応答結果をコピーする
            if(cmd_msg.pStatusSize != (int32_t*)&CmdStatusSize) {
                *cmd_msg.pStatusSize = CmdStatusSize;
            }
            if(pComTx != StsArea) {
                memcpy(pComTx, StsArea, (uint32_t)CmdStatusSize); //応答データをコピーして返す(2020.Jul.16)
            }

            //Printf("CMD Thread (%ld)  (%ld)\r\n", CmdStatusSize, *cmd_msg.pStatusSize);
            Printf("### CMD Thread End (%d)  %04hd-%04hd \n", CmdStatusSize, (uint32_t)(ERRORCODE.ERR >> 16), (uint32_t)ERRORCODE.ERR);

//            tx_event_flags_set(&g_command_event_flags, FLG_CMD_END, TX_OR); //コマンド処理完了
            tdx_flags_set(&g_command_event_flags, FLG_CMD_END/*, TX_OR*/); //コマンド処理完了

            /*status =  */
            //TODO tx_thread_suspend(&cmd_thread);         //自分自身でサスペンドする

        }
    }         //スレッドループ

    return NULL;
}




/**
 * @brief   コマンド解析
 * @param pCmd      コマンドバイト配列へのポインタ T2+size 以降
 * @param Size      サイズ
 * @param route     コマンド要求元
 * @return
 */
static int AnalyzeCmd(char *pCmd, int Size, uint32_t route)
{
    int    Count, len, paralen;
    uint32_t  sz;
    int   i,j,k, ParNum;
    char   find, *Term;
//    int     restart = 0;    //2020.01.20 とりあえずコメントアウト
    int     rf_cmd = 0;
    char   cmdBuff[6];
    uint32_t   Err = ERR( CMD, NOERROR );

    uint32_t   (*Func)(void) = 0;              // 呼び出し先関数

    CMD_TABLE   *Table;

    //char xx[2];

    Printf("\nStart AnalyzeCmd   %.5s\n", &pCmd[0] );
    sprintf(cmdBuff, "%.5s", &pCmd[0]);

    CmdStatusSize += sprintf((char *)&StsArea[CmdStatusSize],"%.5s:", &pCmd[0] );

    Count = PLIST.Count = 0;
    Term = pCmd + Size;                 // コマンドの終了アドレス

    PLIST.Arg[0].Name = PLIST.Arg[0].Data = NULL;
    PLIST.Arg[0].Size = 0;

    Table = &HostCmd[0];

    for(i=0;Table->Code !=0; i++){
        //xx[0] = Table->Code[0];
        //xx[1] = *pCmd;

        if( *(Table->Code) == *pCmd ){                      // 最初の１バイト
            len = (int)strlen( (char *)Table->Code );            // コマンド文字数
            if( memcmp( Table->Code, pCmd, (size_t)len ) == 0){     // 一致？？

                PLIST.Arg[0].Name = pCmd;

                Func = Table->Func;                         // 対応する関数へのポインタ
                pCmd += len;
//                restart = Table->RestartFlag;     //2020.01.20 とりあえずコメントアウト
                rf_cmd = Table->RF_Com;

                for( ParNum=0; Table->Param[ParNum] !=0; ParNum++ );       // パラメータ数

                for( k=0; k<ParNum; k++){
                    if( Term == pCmd ){                      // 最後まで読み込んだ？？
                        //Printf("AnalyzeCmd 01\n");
                        goto ExitScanCmd;
                    }
                    else if ( Term < pCmd){
                        Err = ERR( CMD, FORMAT );                            // コマンドエラー
                        Printf("AnalyzeCmd 02\n");
                        goto ExitScanCmd;
                    }

                    find = 0;
                    // パラメータのスキャン
                    for( j=0; j<ParNum; j++){

                        paralen = (int)strlen(Table->Param[j]);      // コマンド文字数
                        if ( memcmp( Table->Param[j], pCmd, (size_t)paralen ) == 0 ) {  // 一致？

                            Count++;                            // パラメータ数
                            PLIST.Arg[Count].Name = pCmd;
//Printf("### pCmd = %s\n", pCmd);
                            pCmd += paralen;                    // パラメータ文字数分送る
                            if ( *pCmd != '=' ) {
                                Printf("FOMAT ERROR 2(%d/%d)\r\n", k,j); //debug
                                Err = ERR( CMD, FORMAT );       // コマンドエラー
                                goto ExitScanCmd;
                            }
                            *pCmd++ = '\0';                     // nullセット
Printf("### PLIST.Arg[%d].Name = %s\n", Count, PLIST.Arg[Count].Name);
                            sz  = *pCmd++;
                            sz = (sz | ( (uint32_t/*UINT*/)*pCmd++ << 8 ));      // パラメータサイズ
//Printf("### sz = %d\n", sz);
                            PLIST.Arg[Count].Size = (int)sz;         // パラメータサイズ
                            PLIST.Arg[Count].Data = pCmd;       // 最後はnullとは限らないよ
                            pCmd += sz;                         // 次のパラメータの先頭へ
                            if ( pCmd > Term ) {                // コマンドサイズを越えてる？
                                Printf("FOMAT ERROR 3(%d/%d)\r\n", k,j); //debug
                                Err = ERR( CMD, FORMAT );       // コマンドエラー
                                goto ExitScanCmd;
                            }
                            find = 1;                           // 検出フラグ
                            break;
                        }
                    }

                    if ( !find ) {                  // １つも見つからなければエラー
                            Printf("FOMAT ERROR 4(%d/%d)\r\n",k,j); //debug
                            Err = ERR( CMD, FORMAT );       // コマンドエラー
                            goto ExitScanCmd;
                    }
                }
            }
        }
        Table++;
    }

ExitScanCmd:

    if( !Func ){
        Err = ERR( CMD, FORMAT );       // コマンドエラー
        Printf("   Command Not Found \r\n");
    }
    PLIST.Count = Count;                // パラメータ数
Printf("### PLIST.Count = %d\n", PLIST.Count);
    ERRORCODE.ERR = Err;

//    if((Err == ERR(CMD, NOERROR)) && (opponent == CLIENT_BLE) && (ble_passcode_lock == true)) ERRORCODE.ERR = Err = ERR(BLE, REFUSE);   // ＢＬＥ通信の場合、パスコードロック中なら、Ｔコマンドは受け付けない

    //Printf("exec command 1-%08X \n", ERRORCODE.ERR);
    if( Err == ERR( CMD, NOERROR )){

        if(rf_cmd == RF_CMD){
            i = (uint8_t)ParamInt("ACT");                             // ＡＣＴパラメータ（パラメータが含まれていないとき０ｘｆｆ）

            if(i == 0){
//                in_rf_process = false;                       // ＲＦタスク実行中を知るためクリアしておく    ←フラグ管理不要 OSの機能を使うべき
            }

            if((i == 0) || (i == 1) || (i == 0xff))                 // ＡＣＴ＝０または１の場合、またはＡＣＴパラメータのないコマンドの場合（例外：ＥＩＳＥＴ／ＥＩＡＤＪコマンドはＡＣＴ＝０，ＡＣＴ＝１で該当する）
            {
                /*TODO
                if(tx_mutex_get(&g_rf_mutex, WAIT_100MSEC) == TX_SUCCESS){   // ＲＦタスク実行中ではないこと
                    tx_mutex_put(&g_rf_mutex);
                }
                else{
                    ERRORCODE.ERR = ERR(RF, BUSY);                                         // ＲＦタスク実行中動作中
                }
                */
            }
        }

        //Printf("exec commnnd 2-%08X \n", ERRORCODE.ERR);

        if(route == CMD_BLE){
            if(BlePass != BLE_PASS_OK)
            {
                for(i=0;i<5;i++){
                    Printf("%02X ", cmdBuff[i]);
                }
                Printf("\r\n");
                if(memcmp(cmdBuff, "EAUTH", 5) !=  0){
                    ERRORCODE.ERR = ERR(CMD, PASS);
                }
            }
        }

        if(ERRORCODE.ERR == ERR(CMD, NOERROR)) {
            if(route != CMD_HTTP){
                CmdMode = COMMAND_LOCAL;
            }
            ERRORCODE.ERR = Func();  // ＲＦコマンド実行
            CmdMode = COMMAND_NON;
        }

        //Printf("exec command 3-%08X \n", ERRORCODE.ERR);

        if(i == 0){
            memcpy(latest, PLIST.Arg[0].Name, 5);        // 最終実行コマンド５文字を記憶
        }
        Printf("exec command func end / %.5s  %08X \r\n", cmdBuff, ERRORCODE.ERR);
    }


    return (0);
}



int cmd_thread_start(void);
int cmd_thread_start(void)
{
    pthread_t id;
    pthread_create(&id, NULL, (void*)cmd_thread, (void*) NULL);

    //pthread_detach(id); //?

    return 0;
}

int cmd_thread_init(void);
int cmd_thread_init(void)
{
    //int tdx_queue_create(tdx_queue_t *queue, void *queue_memory, size_t queue_size, size_t message_size)

    static uint8_t Qbuf[4 * 32];
    tdx_queue_create(&cmd_queue, Qbuf, 4, 32);
    tdx_flags_create(&g_command_event_flags, "");

    return 0;
}



int cmd_thread_fakeT2(void);
int cmd_thread_fakeT2(void)
{
    static CmdQueue_t cmd_msg;

    uint32_t txLen;

    fact_config.SerialNumber = 0x5F580123;
    init_factory_default(0,0);
/*
    txLen = (int32_t)SCOM.rxbuf.length;
    cmd_msg.CmdRoute = CMD_TCP;            //コマンド キュー  コマンド実行要求元
    cmd_msg.pT2Command = &SCOM.rxbuf.command;    //コマンド処理する受信データフレームの先頭ポインタ
    cmd_msg.pT2Status = &SCOM.txbuf.header;    //コマンド処理された応答データフレームの先頭ポインタ
    cmd_msg.pStatusSize = (int32_t *)&txLen;              //コマンド処理された応答データフレームサイズ
*/
    static uint8_t T[1024];
    static uint8_t R[1024];

    txLen = 6;
    //54 32 06 00 52 55 49 4E  46 3A BE 01              |  T2..RUINF:..
    T[0] = 0x54;
    T[1] = 0x32;
    T[2] = 0x06;
    T[3] = 0x00;
    T[4] = 0x52;
    T[5] = 0x55;
    T[6] = 0x49;
    T[7] = 0x4E;
    T[8] = 0x46;
    T[9] = 0x3A;
    T[10] = 0xBE;
    T[11] = 0x01;
    //|  T2..RUINF:..


    cmd_msg.CmdRoute = CMD_TCP;            //コマンド キュー  コマンド実行要求元
    cmd_msg.pT2Command = &T[0]; //SCOM.rxbuf.command;    //コマンド処理する受信データフレームの先頭ポインタ
    cmd_msg.pT2Status = &R[0]; //&SCOM.txbuf.header;    //コマンド処理された応答データフレームの先頭ポインタ
    cmd_msg.pStatusSize = (int32_t *)&txLen;              //コマンド処理された応答データフレームサイズ


    tdx_queue_send(&cmd_queue, &cmd_msg, 0, 10);    //TX_NO_WAIT);
    tdx_flags_set(&g_command_event_flags, FLG_CMD_EXEC);
    //
    uint32_t actual_events;
    tdx_flags_get(&g_command_event_flags, FLG_CMD_END, /*TX_OR_CLEAR,*/ &actual_events, TX_WAIT_FOREVER);

    Printf("*cmd_msg.pStatusSize = %d\n", *cmd_msg.pStatusSize);
    PrintHex("", R, txLen);

    return 0;
}

int DoCmd(uint8_t *REQbuf, uint8_t *RSPbuf, uint32_t REQlen,  uint32_t *pRSPlen);
int DoCmd(uint8_t *REQbuf, uint8_t *RSPbuf, uint32_t REQlen,  uint32_t *pRSPlen)
{
    int rtn = 0;
    static CmdQueue_t cmd_msg;

    *pRSPlen = REQlen;
    cmd_msg.CmdRoute = CMD_TCP;            //コマンド キュー  コマンド実行要求元
    cmd_msg.pT2Command = REQbuf; //&T[0]; //SCOM.rxbuf.command;    //コマンド処理する受信データフレームの先頭ポインタ
    cmd_msg.pT2Status = RSPbuf; //&R[0]; //&SCOM.txbuf.header;    //コマンド処理された応答データフレームの先頭ポインタ
    cmd_msg.pStatusSize = pRSPlen; //(int32_t *)&txLen;              //コマンド処理された応答データフレームサイズ

    tdx_queue_send(&cmd_queue, &cmd_msg, 0, 10);    //TX_NO_WAIT);
    tdx_flags_set(&g_command_event_flags, FLG_CMD_EXEC);
    //
    uint32_t actual_events;
    tdx_flags_get(&g_command_event_flags, FLG_CMD_END, /*TX_OR_CLEAR,*/ &actual_events, TX_WAIT_FOREVER);

    Printf("*cmd_msg.pStatusSize = %d\n", *cmd_msg.pStatusSize);
    PrintHex("Response", RSPbuf, *cmd_msg.pStatusSize);

    return rtn;
}


