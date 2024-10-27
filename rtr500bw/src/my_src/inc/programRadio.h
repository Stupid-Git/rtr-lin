/**
 * @file	programRadio.h
 *
 * @date	2020/06/18
 * @author	t.saito
 */

#ifndef _PROGRAM_RADIO_H_
#define _PROGRAM_RADIO_H_

#ifdef  EDF
#undef  EDF
#endif

#ifdef  _PROGRAM_RADIO_C_
#define EDF                         //ソース中のみ extern を削除
#else
#define EDF extern                  // 外部変数
#endif


//無線モジュールのフラッシュメモリブロック
#define RFM_FLASH_START_BLOCK  0        ///< RFM フラッシュの開始ブロック
#define RFM_FLASH_END_BLOCK  28        ///< RFM フラッシュの最終ブロック

//S5D9 管理領域2データフラッシュ (64Byte)
#define FLASH_MAN2_FW_VER       (0x40100000)    ///< ファームウェアバージョン   107 = 1.07
#define FLASH_MAN2_SRC_ADR      (0x40100004)    ///< 書き込み元アドレス（SPIフラッシュ時固定値）
#define FLASH_MAN2_DST_ADR      (0x40100008)    ///< 書き込み先アドレス   通常 PSoC 0x000000～
#define FLASH_MAN2_CP_SIZE      (0x4010000C)    ///< 全書き込みサイズ      通常 PSoC 0x00040000
#define FLASH_MAN2_CRC          (0x40100010)    ///< FW全体のＣＲＣ(未使用）

//S5D9 管理領域1データフラッシュ (64Byte)
#define FLASH_UPDATE_EXEC       (0x40100040)    ///< 書き込み実行フラグ  メイン0x00000001,PSoC書き込み実行 0x0003, RFM 0x0004

//#define DATA_FLASH_ADR_PSoC_BLOCK          0x40100060  ///< PSoC ファーム キャッシュ済みブロック番号1～64（未使用）
//#define DATA_FLASH_ADR_PSoC_WRITE_COLUMN   (0x40100064)  ///< PSoC書き込み済み列番号    1～1024
//#define DATA_FLASH_ADR_PSoC_WRITE_MODE     (0x40100068)  ///< PSoC書き込み中フラグ
//#define DATA_FLASH_ADR_PSoC_COMPLETE       (0x4010006C)  ///< PSoC書き込み完了フラグ（未使用）
#define DATA_FLASH_ADR_RADIO_WRITE_MODE     (0x40100070)  ///< RFM書き込み中フラグ


EDF comSOH_t *pRfmUartRx;    ///<　RFモジュールファーム更新用UARTフレームへのポインタ(RF_threadでバッファへのポインタをセットする)
EDF comSOH_t *pRfmUartTx;    ///<　RFモジュールファーム更新用UARTフレームへのポインタ(RF_threadでバッファへのポインタをセットする)

EDF int fRadioFwUpExec;             ///< RFM FW更新実行中フラグ
EDF int fRadioUpdateMode;           ///< RFM アップデートﾓｰﾄﾞ（直接通信ﾓｰﾄﾞ）

//グローバル関数
EDF  void rfm_program_FW(void);
EDF void checkProgramRadio(void);
EDF int rfm_cmd_GetBootFlag(void);
EDF int rfm_cmd_SwapBoot(void);

#endif /* _PROGRAM_RADIO_H_ */
