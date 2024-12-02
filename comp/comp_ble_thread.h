/*
 * comp_ble_thread.h
 *
 *  Created on: Nov 25, 2024
 *      Author: karel
 */

#ifndef COMP_BLE_THREAD_H_
#define COMP_BLE_THREAD_H_

#include "Globals.h"
//extern TX_THREAD ble_thread;

#ifdef EDF
#undef EDF
#endif

#ifdef _BLE_THREAD_ENTRY_C_
#define EDF
#else
#define EDF extern
#endif



//define

//ポートの定義
#define PORT_BLE_RESET       IOPORT_PORT_02_PIN_11               ///< (out) BLE Reset          Low： Reset
#define PORT_BLE_FREEZE      IOPORT_PORT_02_PIN_10               ///< (out) BLE Freeze         Low: Freeze          High: Normal
#define PORT_BLE_ROLE_CHANGE IOPORT_PORT_07_PIN_05               ///< (out) BLE Role Change    Low:クライアント（セントラル）　　High:サーバ（ペリフェラル）  アドバタイズを出すほう
#define PORT_BLE_CMONI        IOPORT_PORT_09_PIN_06               ///< (in ) BLE Connection Monitor


// BLE BleStatus
#define BLE_CONNECT         IOPORT_LEVEL_HIGH
#define BLE_DISCONNECT      IOPORT_LEVEL_LOW

#define ROLE_CLIENT IOPORT_LEVEL_LOW
#define ROLE_SERVER IOPORT_LEVEL_HIGH

// Ｅｘａｍコマンド
#define STXCMD_SEND_BREAK    0x10       ///< ＵＡＲＴ（ロガーＣＰＵ）へブレーク信号送出
#define STXCMD_INFORMATION   0x11       ///<  ファームウェアバージョン、デバイスアドレス、Ｕｎｉｑｕｅ　ＩＤ要求
#define STXCMD_BLE_START     0x12       ///<  ＢＬＥスタック開始
#define STXCMD_BLE_STOP      0x13       ///<  ＢＬＥスタック停止
#define STXCMD_BLE_TX_TEST_START 0x14   ///<  送信テスト
#define STXCMD_BLE_RX_TEST_START 0x15   ///<  受信テスト
#define STXCMD_SET_BLE_TX_POWER  0x16   ///<  送信出力設定
#define STXCMD_TEST_STOP     0x17       ///<  送受信テスト終了
#define STXCMD_DEEPSLEEP     0x18       ///<  ＢＬＥ ＣＰＵ ＤｅｅｐＳｌｅｅｐ
#define STXCMD_SIGNAL_OUT_IN 0x19       ///<   Ｆｒｅｅｚｅ、Ｍｏｎｉｔｏｒ信号試験
#define STXCMD_IMO_ERROR     0x1a       ///<  ＩＭＯ周波数誤差
#define STXCMD_ECO_TRIM      0x1b       ///<  ＥＣＯトリム
#define STXCMD_INHIBIT_RESET 0x1e       ///<  Ｅｘａｍボタン開放したときリセットする／しない
#define STXCMD_BLE_CPU_RESET 0x1f       ///<  ＢＬＥ ＣＰＵリセット

// クライアントコマンド
#define STX_START_SCAN          0x20    ///<  スキャン開始
#define STX_SCAN_DATA           0x21    ///<  スキャンデータフレーム
#define STX_STOP_SCAN           0x22    ///<  スキャン停止
#define STX_CONNECTION           0x23    ///<  コネクション指示
#define STX_DISCONNECTIN        0x24    ///<  ディスコネクション指示


//BLE g_ble_event_flags
//#define FLG_BLE_RX_COMPLETE       0x00000001
//#define FLG_BLE_TX_COMPLETE       0x00000002
#define FLG_BLE_RX_REQUEST          0x00000004          ///< PSoC受信リクエスト
#define FLG_BLE_TX_REQUEST          0x00000008          ///< USBからPSoCへの送信要求(送信 → 受信)
#define FLG_PSOC_RX_COMPLETE        0x00000010          ///< PSoC 送信完了 PSoCブートローダ
#define FLG_PSOC_UPDATE_REQUEST     0x00000020          ///< PSoC FW更新リクエスト
#define FLG_PSOC_RESET_REQUEST      0x00000040          ///< PSoC リセット要求
#define FLG_PSOC_FREEZ_REQUEST      0x00000080          ///< PSoC FREEZ要求
//#define FLG_PSOC_ERROR            0x00000100          ///< PSoC通信エラー
#define FLG_BLE_ALL_EVENT   (FLG_BLE_RX_REQUEST|FLG_BLE_TX_REQUEST|FLG_PSOC_RX_COMPLETE|FLG_PSOC_UPDATE_REQUEST|FLG_PSOC_RESET_REQUEST|FLG_PSOC_FREEZ_REQUEST)




//型定義

//TR4タイプデバイス名文字列
typedef struct{

    uint8_t length;
    uint8_t AD_Type;
    uint8_t Data[26];       //デバイス名文字列 26バイト固定長 UTF-8
    uint8_t dummy[2];

}__attribute__ ((__packed__)) BLE_Packet_DeviceName_t;     //31Byte

//iOS向け アドバタイズパケット UUID16Byte
typedef struct{

    uint8_t length;
    uint8_t AD_Type;
    uint8_t Data[16];       //UUID
}__attribute__ ((__packed__)) BLE_Packet_UUID16_t;

//TR4タイプ アドバタイズパケット
typedef struct{

    uint8_t length;
    uint8_t AD_Type;
    uint16_t CompanyID;
    uint32_t SerialNo;
    uint8_t code;           //制御コード ｂｉｔ0 BLEセキュリティロック状態 0：解除 1：ロック
    uint8_t Format;     //フォーマット番号 0：RTR-500BLE予約 1：TR-4予約
    uint8_t Data[18];       //フォーマット別18Byte固定長

}__attribute__ ((__packed__)) BLE_Packet_TD_TR4_t;

//RTR-50x/TR-7wb向け アドバタイズパケット UUID4Byte
typedef struct{

    uint8_t length;
    uint8_t AD_Type;
    int32_t Data;       //UUID
}__attribute__ ((__packed__)) BLE_Packet_UUID4_t;

//RTR50x/TR-7wb向け アドバタイズパケット 22Byte
typedef struct{

    uint8_t length;
    uint8_t AD_Type;
    uint16_t CompanyID;     //Company ID
    uint32_t SerialNo;          //機器シリアル番号
    uint8_t code;               //制御コード

    uint8_t SetupWarnCnt;       //各種カウンタ    上位4bit：警報がONになったら+1 (0～15) 下位4bit：設定が変更されたら+1 (0～15)
    uint8_t status1;            //状態コード1    1   下参照
    uint8_t status2;            //状態コード2    1   下参照
    uint16_t data[4];            //測定値   4Ch 3.4Ch用は予備
    uint8_t PassCRC_H;      //パスコードCRC  2Byte ビッグエンディアン
    uint8_t PassCRC_L;
}__attribute__ ((__packed__)) BLE_Packet_TD_t;

//RTR500BW向け アドバタイズパケット 22Byte
typedef struct{

    uint8_t length;
    uint8_t AD_Type;
    uint16_t CompanyID;     //Company ID
    uint32_t SerialNo;      //機器シリアル番号
    uint8_t code;           //制御コード

    uint8_t State;          //状態コード
    uint8_t SysCnt;         // 設定CNT（親機設定）
    uint8_t ScanCnt;        // SCANCNT（対応せず）
    uint8_t WarnCnt;        // 警報CNT
    uint8_t RegCnt;         // 設定CNT（登録情報）
    uint8_t reserve;        // 空き
    uint8_t BattLevel;      //電池レベル BWは0x10固定
    uint16_t data[2];       //測定値  予備
    uint8_t PassCRC_H;      //パスコードCRC  2Byte ビッグエンディアン
    uint8_t PassCRC_L;
}__attribute__ ((__packed__)) BLE_Packet_BW_t;




//グローバル変数

EDF int16_t BlePass;            ///<  0:PassOff 1:PassOK  CFBleAuth（）でセット

EDF volatile bool during_scan;

//EDF char    AdvPacket[64];          ///<    アドバタイズパケットデータ ->構造体に変更


///PSoC設定値
EDF struct def_psoc{
    uint64_t    device_address;
    uint64_t    chip_id;
    uint8_t     revision[8];
}__attribute__ ((__packed__))  psoc;

#if 0
///PSoC 設定テーブル(TR-7wb)
EDF struct{
    uint8_t enabe;          //BLE有効/無効
    uint8_t PSoC_CapTrim;       //PSOC ECO CAPTRIM値 1Byte デフォルト 0x55        //2018.Mar.30
    uint8_t cmd_Interval;       //  0x9Eコマンド周期 [s] デフォルト = 60秒
    uint8_t security;               //BLEセキュリティ パスコード設定時1 未設定時0
    union {            //BLEファームウェアVer 4Byte
        uint8_t byte[4];
        uint32_t  word;
    }__attribute__ ((__packed__)) FwVer;
    uint32_t PassCode;              //BLEパスコード
    union  //BLEデバイスアドレス6Byte(8Byte) (例 0xFC195141751B)
    {
        uint8_t byte[8];
        uint64_t  dword;
    }__attribute__ ((__packed__)) DEVAddress;
}__attribute__ ((__packed__)) BLE;
#endif

///チップ固有ＩＤ 8Byte  (LOT0, LOT1, LOT2,Wafer,DiyX,DieY,DieSort,DieMinor)
EDF union {
    uint8_t byte[8];
    uint64_t  dword;
}__attribute__ ((__packed__)) BLE_ChipID;


///アドバタイジングパケット (パッシブスキャン応答)
EDF struct def_ble_adv_packet_t{
        uint8_t reserve;        //予備/ bit0=0のときDeepSleep保持、リセットで解除 bit0=1のとき通常動作
        uint8_t length;
        uint8_t AD_Type;
        uint8_t AD_Data;
        union{
           BLE_Packet_TD_TR4_t TR4Adv;     //BLEアドバタイズパケットTR-4タイプ  カンパニーID、シリアル等    28Byte

            struct{                          //BLEアドバタイズパケットRTR501,TR-7wbタイプ
                BLE_Packet_UUID4_t Uuid;     //4Byte UUID           6Byte
                BLE_Packet_TD_t Cid;         //カンパニーID、シリアル等         22Byte
            }__attribute__ ((__packed__)) RTR5_TR7;

            struct{                          //BLEアドバタイズパケットRTR500BWタイプ （仮）
                BLE_Packet_UUID4_t Uuid;     //4Byte UUID           6Byte
                BLE_Packet_BW_t Cid;         //カンパニーID、シリアル等                 22Byte
            }__attribute__ ((__packed__)) RTR500BW;

        }__attribute__ ((__packed__)) Value;

        BLE_Packet_DeviceName_t DevName;     //機器名称
}__attribute__ ((__packed__)) BLE_ADV_Packet;



//プロトタイプ
//EDF void ble_advertise_packet(void);      ->static
EDF int ble_connect_check(void);
EDF void ble_reset(void);
EDF int isBleRole(void);
EDF int isBleConnect(void);


///USB,PSoC間転送用メッセージキューの定義
typedef struct{
    comSOH_t *pUSB2PSoC;   //USBからPSoCへ転送する受信データフレームの先頭ポインタ(SOH/STX)
    comSOH_t *pPSoC2USB;   //PSoCからの応答をUSBへ転送する応答フレームの先頭ポインタ(SOH/STX)
}usb_psoc_t;


#endif /* COMP_BLE_THREAD_H_ */
