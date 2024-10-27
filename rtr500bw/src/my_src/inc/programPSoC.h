/**
    @brief  PSoCファーム更新処理関数ヘッダファイル
            RTR500BW用
    @date   2020/01/28
    @author t.saito
*/


#ifndef     _PROGRAM_PSOC_H_
#define     _PROGRAM_PSOC_H_



#ifdef  EDF
#undef  EDF
#endif

#ifdef  _PROGRAM_PSOC_C_
#define EDF                         //command.c ソース中のみ extern を削除
#else
#define EDF extern                  // 外部変数
#endif


//管理領域2データフラッシュ (64Byte)
#define FLASH_MAN2_FW_VER       (0x40100000)    ///< ファームウェアバージョン   107 = 1.07
#define FLASH_MAN2_SRC_ADR      (0x40100004)    ///< 書き込み元アドレス（SPIフラッシュ時固定値）
#define FLASH_MAN2_DST_ADR      (0x40100008)    ///< 書き込み先アドレス   通常 PSoC 0x000000～
#define FLASH_MAN2_CP_SIZE      (0x4010000C)    ///< 全書き込みサイズ      通常 PSoC 0x00040000
#define FLASH_MAN2_CRC          (0x40100010)    ///< FW全体のＣＲＣ(未使用）

//管理領域1データフラッシュ (64Byte)
#define FLASH_UPDATE_EXEC       (0x40100040)    ///< 書き込み実行フラグ  メイン0x00000001,PSoC書き込み実行 0x0003, RFM 0x0004


//#define DATA_FLASH_ADR_PSoC_BLOCK          0x40100060  ///< PSoC ファーム キャッシュ済みブロック番号1～64（未使用）
#define DATA_FLASH_ADR_PSoC_WRITE_COLUMN   (0x40100064)  ///< PSoC書き込み済み列番号    1～1024
#define DATA_FLASH_ADR_PSoC_WRITE_MODE     (0x40100068)  ///< PSoC書き込み中フラグ
//#define DATA_FLASH_ADR_PSoC_COMPLETE       (0x4010006C)  ///< PSoC書き込み完了フラグ（未使用）
//#define DATA_FLASH_ADR_RADIO_WRITE_MODE     (0x40100070)  ///< RFM書き込み中フラグ



/**
 * PSoC送信フレーム構造定義
 * PSOC通信（PSoC更新）で使用
 *
 */
typedef struct {
    char soh;               ///< SOH 0x01
    char command;           ///< コマンド
    uint16_t data_length;   ///< データ長
    char data[256];         ///< データ部
    char sum_l;             ///< SUM 下位バイト
    char sum_h;             ///< SUM 上位バイト
    char etb;               ///< ETB 0x17
}__attribute__ ((__packed__)) PSoC_tx_uart_t;

/**
 * PSoC受信フレーム構造定義
 * PSOC通信（PSoC更新）で使用
 *
 */
typedef struct {
    char soh;               ///< SOH 0x01
    char status;            ///< 応答ステータス
    uint16_t data_length;   ///< データ長
    char data[256];         ///< データ部
    char sum_l;             ///<　SUM 下位バイト
    char sum_h;             ///<　SUM 上位バイト
    char etb;               ///< ETB 0x17
}__attribute__ ((__packed__)) PSoC_rx_uart_t;


EDF PSoC_rx_uart_t *pPSoCUartRx;    ///<　PSoCファーム更新用UARTフレームへのポインタ(BLEスレッドでセット)
EDF PSoC_tx_uart_t *pPSoCUartTx;    ///<　PSoCファーム更新用UARTフレームへのポインタ(BLEスレッドでセット)


EDF int fPSoCFwUpExec;             ///< PSoC FW更新実行フラグ
EDF int fPSoCUpdateMode;           ///< PSoC PSoC FW更新モード ﾞ（直接通信モード）


//PSoCファーム更新用（ファーム書き込み）

//PSoC ブートローダ0x38コマンドの応答 8Byte
EDF uint32_t PSoC_Si_ID;            ///<　シリコンID
EDF uint8_t PSoC_Si_Rev;            ///<　シリコンリビジョン
EDF uint32_t PSoC_BtLdrVer;         ///<　ブートローダーバージョン 中身は３Byte
//PSoC ブートローダ0x32コマンドの応答 4Byte
EDF int32_t PSoC_Strat_row;         ///<　ブートローダブルフラッシュ領域最初の列番号(アレイ1）
EDF int32_t PSoC_End_row;           ///<　ブートローダブルフラッシュ領域最終の列番号(アレイ1）
//PSoC ブートローダ0x3Aコマンドの応答 1Byte
EDF uint8_t PSoCCheckSum;           ///<　列のチェックサム



EDF void PSoC_program_FW(void);       //PSoCフラッシュ更新（PSoCブートローダ通信中）
EDF void checkProgramPSoC(void);        //PSoCのFW更新が途中で終わっていないかチェック、更新


#endif

