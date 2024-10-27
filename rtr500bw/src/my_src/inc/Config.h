/*
 * Config.h
 *
 *  Created on: 2019/02/18
 *      Author: haya
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#ifdef EDF
#undef EDF
#endif

#include "tx_api.h"

#ifndef CONFIG_C_
    #define EDF extern
#else
    #define EDF
#endif




#pragma pack(1)


/// 本体設定（３２ビット境界であること）
/// size 256 byte
EDF struct def_fact_config {             // 本体設定（３２ビット境界であること）

    uint32_t    SerialNumber;       // シリアル番号
    uint32_t    dummy;
    uint8_t     Vender;             // 0x00: T&D 0x01:ESPEC   0x09:Hitachi
    uint8_t     RfBand;             // 0: JP   1:US    2:EU 

    struct{
        uint8_t     cap_trim;                                       // ＢＬＥ ＣＰＵ ＥＣＯ キャパシタトリム値  default 0x63
        uint8_t     interval_0x9e_command;                          // ＢＬＥ ０ｘ９ｅコマンドインターバル       default  60sec
    } ble;
    // 10 byte

    uint32_t    mac_h;                  // ETH MAC0 High Byte   etc.   0x00002E09               
    uint32_t    mac_l;                  // ETH MAC0 Low  Byte   etc.   0x0A0076C7

    uint32_t    w_mac_h;                // WIFI MAC0 High Byte   etc.   0x00002E09               
    uint32_t    W_mac_l;                // WIFI MAC0 Low  Byte   etc.   0x0A0076C7

    uint32_t    TestNumber;           // 検査番号（製造番号）
    char     gap2[220];           //

    //製造時の通し番号 4byte追加
    //上のパラメータを書くコマンドが無い

    uint32_t    crc;

} fact_config __attribute__((aligned(4)));


// size 8192 byte
EDF struct def_sys_config {             // 本体設定（３２ビット境界であること）

    struct{                             // 256 byte
        char     gap01[4];
        uint32_t    SerialNumber;       // シリアル番号   fact_config.SerialNumberのコピー  
        uint32_t    registration_code;  //                                       ※未使用
        char     Name[32];           // str:親機名称
        char     Description[64];    // str:本機の説明

        char     TempUnits[1];       // 0:摂氏  1:華氏

        char     TimeSync[1];        // 0:時刻同期 OFF  1:3G  2:SNTP
        char     TimeDiff[5];        // str:：時差(+0900)
        char     Summer[26];         // str:サマータイム
        char     TimeForm[32];       // str:日付フォーマット文字
        char     TimeZone[64];       // str:タイムゾーン
// sakaguchi cg UT-0027 ↓
        uint32_t    SysCnt;          // 親機設定変更カウンタ
        uint32_t    RegCnt;          // 登録情報変更カウンタ
        // 245 byte
        char     gap02[11];
// sakaguchi cg UT-0027 ↑
    } device;       // 256 byte

    // ここまで 256 byte
// sakaguchi 2020.11.11 ↓
    //uint8_t gap_001[128];
    struct{
        char        OyaFirm[12];    // 親機ファームウェアバージョン
        uint32_t    RfFirm;         // 無線モジュールファームウェアバージョン
        char        BleFirm[8];     // BLEファームウェアバージョン
    } version;       // 24 byte
    uint8_t gap_001[104];
// sakaguchi 2020.11.11 ↑
    // ここまで 384 byte

    struct {                            // 256 byte
        char     Port[2];            // FTPポート番号 0-65535
        char     Encode[1];          // 0:None  1:UFT-8 2:EUC-JP
        char     Pasv[1];            // 0:FTP Active 1:PASV
        char     Server[64];         // str:FTPサーバ名
        char     UserID[64];         // str:FTPユーザID
        char     UserPW[64];         // str:FTPユーザPW
        // 196 byte
        char     gap2[60];

    } ftp;          // 256 byte

    // ここまで 640 byte
    char gap_002[128];
    // ここまで 768 byte

    struct{                             // 384 byte
        char     DhcpEnable[1];      // 0:Off  1:On
        char     Phy[1];             // 0:有線LAN  1:無線LAN

        char     NetPass[64];        // ネットワークパスワード

        char     SntpServ1[64];      // str:SNTPサーバ名称
        char     SntpServ2[64];      // str:SNTPサーバ名称

        char     IpAddrss[16];       // str:ローカルIPアドレス
        char     SnMask[16];         // str:ローカルサブネットマスク
        char     GateWay[16];        // str:ローカルデフォルトゲートウェイ
        char     Dns1[16];           // str:DNS1
        char     Dns2[16];           // str:DNS2

        char     UdpRxPort[2];       // 0-65535: UDP受信ポート
        char     UdpTxPort[2];       // 0-65535: UDP送信ポート
        char     SocketPort[2];      // 0-65535: TCP受信ポート

        char     ProxyFtp[1];        // 0:Off  1:On
        char     ProxyMail[1];       // 0:Off  1:On
        char     ProxyHttp[1];       // 0:Off  1:On
        char     gap2[1];
        char     ProxyFtpPort[2];    // 0-65535: Proxy受信番号
        char     ProxyMailPort[2];   // 0-65535: Proxy受信番号
        char     ProxyHttpPort[2];    // 0-65535: Proxy受信番号
        char     gap3[2];            //

        char     ProxyFtpIP[16];     // str:PROXYサーバ名称
        char     ProxyMailIP[16];    // str:PROXYサーバ名称
        char     ProxyHttpIP[16];    // str:PROXYサーバ名称
        //  340 byte
        char     ProxyFtpServ[128];   // str:PROXYサーバ名称
        char     ProxyMailServ[128];  // str:PROXYサーバ名称
        char     ProxyHttpServ[128];  // str:PROXYサーバ名称
        // 724

        char     Mtu[2];              // 576-1400: MTU         // MTU追加 sakaguchi 2021.04.07
        char     Interval[1];         // 0-200: 通信間隔[ms]    // sakaguchi 2021.05.28
        // 725
        char     gap4[41];


    } network;      // 768 byte

    // ここまで 1536 byte
    char gap_003[128];
    // ここまで 1664 byte

    struct{                             // 256 byte
        char     Enable[1];          // 0:Off  1:On
        char     BAND[1];            // 0:2.4G 1:5G          未使用
        char     CH2G[1];            // 2.4G Channel         未使用
        char     CH5G[1];            // 5G Channel           未使用
        char     SEC[1];             // セキュリティモード   0:Non  1:使用禁止 2:WEP128  3:WPA-TKIP  4:WPA2-AES

        char     WEP[26];            // WEPキー (13)
        char     SSID[32];           // SSID
        char     PSK[64];            // PSK
        // 125 byte
        char     gap[129];

    } wlan;         // 256 byte

    // ここまで 1920 byte
    char gap_004[128];
    // ここまで 2048 byte

    struct {                            // 128 byte
        char     active;             // ＢＬＥ 停止      0:停止  1:動作
        char     utilization;        // ＢＬＥ パスコード利用 0：無効  1：有効
        uint32_t    passcode;           // ＢＬＥ パスコード  未使用
        // 6 byte

        char     gap[122];
    } ble;          // 128 byte

    // ここまで 2176 byte
    char gap_005[128];
    // ここまで 2304 byte

    struct{                             // 512 byte
        char     Enable[1];          // 0:警報OFF  1:ON
        char     Route[1];           // 0:Email    1:使用禁止 2:送信しない 3:HTTP
        char     gap1[2];
        char     Interval[2];        // 0:警報取得間隔（分）

        char     Fn1[1];             // 0-255:警報Emailのフラグ
        char     Fn2[1];             // 0-255:警報Emailのフラグ
        char     Fn3[1];             // 0-255:警報Emailのフラグ
        char     Fn4[1];             // 0-255:警報Emailのフラグ
        char     Fn5[1];             // 0-255:警報Emailのフラグ
        char     Fn6[1];             // 0-255:警報Emailのフラグ
        char     Fn7[1];             // 0-255:警報Emailのフラグ
        char     Fn8[1];             // 0-255:警報Emailのフラグ
        char     Ext[1];             // 0-255:警報接点のフラグ
        char     gap2[1];

        char     Subject[64];        // str:警報Emailの件名
        char     gap3[64];
        char     Body[64];           // str:警報Emailの本文
        char     gap4[64];
        char     To1[64];            // str:警報Emailの宛先1
        char     To2[64];            // str:警報Emailの宛先2
        char     To3[64];            // str:警報Emailの宛先3
        char     To4[64];            // str:警報Emailの宛先4
        char     To5[64];            // str:警報Emailの宛先5
        char     To6[64];            // str:警報Emailの宛先6
        char     To7[64];            // str:警報Emailの宛先7
        char     To8[64];            // str:警報Emailの宛先8

        // 784 byte
        char     gap5[240];

    } warning;      // 1024 byte

    // ここまで 3328 byte

    struct{                             // 640 byte
        char     Enable[1];          // 0:現在値取得OFF  1:ON
        char     Route[1];           // 0:Email  1:FTP   2:送信しない  3:HTTPS
        char     Bind[1];            // 0:XMLファイル添付有り  1:無し      for Email
        char     gap1[1];
        char     Interval[2];        // 0:現在値取得間隔（分）

        char     Subject[64];        // str:Emailの件名
        char     To1[64];            // str:Emailの宛先1
        char     To2[64];            // str:Emailの宛先2     ※未使用
        char     Attach[64];         // str:Emailの添付ファイル

        char     Dir[192];           // str:FTPフォルダ名
        char     Fname[64];          // str:FTPファイル名
        // 518 byte
        char     gap2[122];
        char     gap3[128];

    } monitor;      // 768 byte

    // ここまで 4096 byte

    struct{                             // 640 byte
        char     Enable[1];          // 0:データ吸い上げ機能OFF  1:ON
        char     Route[1];           // 0:Email  1:FTP   2:使用禁止  3:HTTP
        char     FileType[1];        // 0:ファイル形式 TRX  1:XML  ※未使用
        char     Range[1];           // ※未使用

        char     Event1[6];          // str:吸い上げ日時１
        char     Event2[6];          // str:吸い上げ日時２
        char     Event3[6];          // str:吸い上げ日時３
        char     Event4[6];          // str:吸い上げ日時４
        char     Event5[6];          // str:吸い上げ日時５
        char     Event6[6];          // str:吸い上げ日時６
        char     Event7[6];          // str:吸い上げ日時７
        char     Event8[6];          // str:吸い上げ日時８
        char     Event0[6];          // str:吸い上げ日時0  for Interval吸い上げ

        char     Subject[64];        // str:Emailの件名
        char     To1[64];            // str:Emailの宛先1
        char     Attach[64];         // str:Emailの添付ファイル

        char     Dir[192];           // str:FTPフォルダ名
        char     Fname[64];          // str:FTPファイル名
        //  506 byte        // old 512
        char     gap2[128];
        char     gap3[128];
        char     gap4[6];            // 2020 05 01 
    } suction;      // 768 byte

    // ここまで 4864 byte

    struct {                            // 256 byte
        char     Port[2];            // smtpポート番号 0-65535
        char     AuthMode[1];        // 0:Off   1:On
        char     Encode[1];         // 0:ISO-2022-JP    1:UTF-8 
        char     Server[64];         // str:SMTPサーバ名
        char     UserID[64];         // str:SMTPユーザID
        char     UserPW[64];         // str:SMTPユーザPW
        char     Sender[64];         // str:Email Sender
        char     From[64];           // str:Email From 差出人

        // 324 byte
        uint8_t     gap2[60];

    } smtp;          // 384 byte

    // ここまで 5248 byte

    struct{                             // 320 byte
        char     Server[64];        // str:webstrageサーバ名
        char     gap2[64];
        char     Api[128];
        char     gap3[64];
        char     Port[2];            //
        char     Interval[2];        //
        char     Mode[1];            // 0:http   1:https
        char     gap4[5];
        
        // 330 byte
        char     gap5[182];
    } websrv;         // 512 byte

    // ここまで 5760 byte

    struct{                             // 1024 byte
        uint8_t     gap[1024];
    } gsm;          // 1024 byte

    // ここまで 6784 byte
    uint8_t gap_e001[512];
    // ここまで 7296 byte

    char user_area[64];                                      // ユーザ定義情報

    // ここまで 7360 byte

    char gap_end[824];

    char last[4];
    // ここまで 8188 byte
    uint32_t crc;                   // 4 byte
    // 8192
} my_config __attribute__((aligned(4)));


#pragma pack(4)



EDF uint32_t AdrStr2Long(char *src);
EDF uint32_t AdrStr2Long4(char *src);


#endif /* MY_SRC_INC_CONFIG_H_ */
