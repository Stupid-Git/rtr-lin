/**
 * @file    Sfmem.h
 * @brief   シリアルフラッシュメモリ用関数ヘッダファイル
 * @date    2019/02/26
 * @author  haya
 */

#ifndef _SFMEM_H_
#define _SFMEM_H_



#include "hal_data.h"
#include "flag_def.h"
#include "MyDefine.h"
#include "Globals.h"
//#include "system_thread.h"
#include "General.h"

//#include "test_thread.h"

#undef EDF

#ifndef _SFMEM_C_
    #define EDF extern
#else
    #define EDF
#endif


// 4M Byte serial Flash
//  0x000000 0x3fffff
//※注  シリアルフラッシュの消去単位 4Kbyte(4096)
//
//  00 0000 -00 0fff    4096    製造時書き込みエリア            <8k block>  ※このエリアは消さない
//
//  00 1000 -00 1fff    4096    gap
//
//  00 2000 -00 3fff    8192    親機動作設定エリア １          <8k block>
//
//  00 4000 -00 5fff    8192    親機動作設定エリア 2           <8k block>   back up
//
//  00 6000 -00 7fff    8192    gap                           <8k block>   root証明書
//
//  00 8000 -00 ffff    32767   gap                           <32k block>
//
//
//  01 0000 -01 ffff    65535   設定ファイル １               <64k block>
//
//  02 0000 -02 ffff    65535   設定ファイル ２               <64k block>
//
//
//  07 0000 -07 ffff  65535    ログエリア1 4096 x 16          <64k block>
//
//  08 0000 -08 ffff  65535    ログエリア2 4096 x 16          <64k block>
//
//
//  09 0000 -09 ffff  65535    TD ログエリア1 4096 x 16        <64k block>
//
//  0A 0000 -0A ffff  65535    TD ログエリア1 4096 x 16        <64k block>
//
//  0B 0000 -0B ffff  65535    TD ログエリア1 4096 x 16        <64k block>
//
//  0C 0000 -0C ffff  65535    TD ログエリア1 4096 x 16        <64k block>
//
//  以降、ROM書き換え用記録領域 (3M Byte)
//  0x0010 0000-0x003e ffff  2effff 3080191 Byte            <64k block>
//


// 4K byte (0x1000)はシリアルフラッシュ消去の最小単位

#define SST26VF032B 0x00bf2642
#define SST26VF064B 0x00bf2643

//#define SERIAL_FLASH_DATA_REFUGE_START  0x00000000          // キャンセルお伺いコマンド発行時、データの退避場所
//#define SERIAL_FLASH_R500_REFUGE_START  0x0000FF00          // キャンセルお伺いコマンド発行時、r500構造体の退避場所
//#define SERIAL_FLASH_REFUGE_END         0x0000FFFF

#define SFM_FACTORY_START           (uint32_t)(0x00000000)          // シリアルフラッシュメモリ番地 製造時設定域 先頭番地
#define SFM_FACTORY_END             (uint32_t)(0x000000ff)          // シリアルフラッシュメモリ番地 製造時設定域 終了番地
#define SFM_FACTORY_START_B         (uint32_t)(0x00001000)          // シリアルフラッシュメモリ番地 製造時設定域 先頭番地
#define SFM_FACTORY_END_B           (uint32_t)(0x000010ff)          // シリアルフラッシュメモリ番地 製造時設定域 終了番地
#define SFM_FACTORY_SIZE            (uint32_t)(0x00000100)          // シリアルフラッシュメモリ番地 製造時設定域 サイズ

#define SFM_CONFIG_START            (uint32_t)(0x00002000)          // シリアルフラッシュメモリ番地 本体設定域 先頭番地    オリジナル
#define SFM_CONFIG_START_B          (uint32_t)(0x00004000)          // シリアルフラッシュメモリ番地 本体設定域 先頭番地    バックアップ
#define SFM_CONFIG_SECT             (uint32_t)(0x00001000)          // シリアルフラッシュメモリ番地 本体設定域 セクタサイズ
#define SFM_CONFIG_SIZE             (uint32_t)(0x00002000)          // シリアルフラッシュメモリ番地 本体設定域 実使用サイズ（２５６バイト区切り） XXXXX 2000-->3000 2020 9/8



#define SFM_CERT_W_START            (uint32_t)(0x00006000)          // ルート証明書 webstorage 先頭番地  Size 4096
#define SFM_CERT_W_END              (uint32_t)(0x00006FFF)          //
#define SFM_CERT_U_START            (uint32_t)(0x00007000)          // ルート証明書 user定義 先頭番地    Size 4096
#define SFM_CERT_U_END              (uint32_t)(0x00007FFF)          //

#define SFM_GROUP_START             (uint32_t)(0x00008000)          // 表示グループ設定
#define SFM_GROUP_END               (uint32_t)(0x00008FFF)  
#define SFM_GROUP_SIZE              (uint32_t)(0x00001000)          // size 4K  


/*
#define SFM_CERT_W2_START           (uint32_t)(0x00008000)          // ルート証明書 webstorage 先頭番地  Size 4096
#define SFM_CERT_W2_END             (uint32_t)(0x00008FFF)          //
#define SFM_CERT_U2_START           (uint32_t)(0x00009000)          // ルート証明書 user定義 先頭番地    Size 4096
#define SFM_CERT_U2_END             (uint32_t)(0x00009FFF)          //

#define SFM_CERT_W3_START           (uint32_t)(0x0000a000)          // ルート証明書 webstorage 先頭番地  Size 4096
#define SFM_CERT_W3_END             (uint32_t)(0x0000aFFF)          //
#define SFM_CERT_U3_START           (uint32_t)(0x0000b000)          // ルート証明書 user定義 先頭番地    Size 4096
#define SFM_CERT_U3_END             (uint32_t)(0x0000bFFF)          //
*/

#define SFM_REGIST_START            (uint32_t)(0x00010000)          // 子機・中継機 登録ファイル領域  オリジナル  Size 64K
#define SFM_REGIST_END              (uint32_t)(0x0001ffff)
#define SFM_REGIST_SECT             (uint32_t)(0x00001000)          // セクタサイズ
#define SFM_REGIST_SIZE_64           (uint32_t)(0x00010000)          // size 63K
#define SFM_REGIST_SIZE_32           (uint32_t)(0x00008000)          // size 32K
#define SFM_REGIST_SIZE_16          (uint32_t)(0x00004000)          // size 16K
#define SFM_REGIST_SIZE_8           (uint32_t)(0x00004000)          // size 8K
#define SFM_REGIST_SIZE_4           (uint32_t)(0x00002000)          // size 4K

#define SFM_REGIST_START_B          (uint32_t)(0x00020000)          // 子機・中継機 登録ファイル領域 バックアップ
#define SFM_REGIST_END_B            (uint32_t)(0x0002ffff)

#define SFM_TEMP_START              (uint32_t)(0x00030000)          // 子機・中継機 登録ファイル領域 書き込み用 一時ファイル  64K
#define SFM_TEMP_END                (uint32_t)(0x0003ffff)
#define SFM_TEMP_SIZE               (uint32_t)(0x00010000)          // size 64K


#define SFM_CERT_SIZE               (uint32_t)(0x00001000)          // サイズ

#define SFM_CERT_W1_START           (uint32_t)(0x00040000)          // ルート証明書 webstorage 先頭番地  Size 4096
#define SFM_CERT_W1_END             (uint32_t)(0x00040FFF)          //
#define SFM_CERT_W2_START           (uint32_t)(0x00041000)          // ルート証明書 webstorage 先頭番地  Size 4096
#define SFM_CERT_W2_END             (uint32_t)(0x00041FFF)          //
#define SFM_CERT_W3_START           (uint32_t)(0x00042000)          // ルート証明書 webstorage 先頭番地  Size 4096
#define SFM_CERT_W3_END             (uint32_t)(0x00042FFF)          //
#define SFM_CERT_W4_START           (uint32_t)(0x00043000)          // ルート証明書 webstorage 先頭番地  Size 4096
#define SFM_CERT_W4_END             (uint32_t)(0x00043FFF)          //
#define SFM_CERT_W5_START           (uint32_t)(0x00044000)          // ルート証明書 webstorage 先頭番地  Size 4096
#define SFM_CERT_W5_END             (uint32_t)(0x00044FFF)          //
#define SFM_CERT_W6_START           (uint32_t)(0x00045000)          // ルート証明書 webstorage 先頭番地  Size 4096
#define SFM_CERT_W6_END             (uint32_t)(0x00045fFF)          //
#define SFM_CERT_W7_START           (uint32_t)(0x00046000)          // ルート証明書 webstorage 先頭番地  Size 4096
#define SFM_CERT_W7_END             (uint32_t)(0x00046fFF)          //
#define SFM_CERT_W8_START           (uint32_t)(0x00047000)          // ルート証明書 webstorage 先頭番地  Size 4096
#define SFM_CERT_W8_END             (uint32_t)(0x00047fFF)          //


#define SFM_CERT_U1_START           (uint32_t)(0x00048000)          // ルート証明書 user-server 先頭番地  Size 4096
#define SFM_CERT_U1_END             (uint32_t)(0x00048FFF)          //
#define SFM_CERT_U2_START           (uint32_t)(0x00049000)          // ルート証明書 user-server 先頭番地  Size 4096
#define SFM_CERT_U2_END             (uint32_t)(0x00049FFF)          //
#define SFM_CERT_U3_START           (uint32_t)(0x0004a000)          // ルート証明書 user-server 先頭番地  Size 4096
#define SFM_CERT_U3_END             (uint32_t)(0x0004aFFF)          //
#define SFM_CERT_U4_START           (uint32_t)(0x0004b000)          // ルート証明書 user-server 先頭番地  Size 4096
#define SFM_CERT_U4_END             (uint32_t)(0x0004bFFF)          //
#define SFM_CERT_U5_START           (uint32_t)(0x0004c000)          // ルート証明書 user-server 先頭番地  Size 4096
#define SFM_CERT_U5_END             (uint32_t)(0x0004cFFF)          //
#define SFM_CERT_U6_START           (uint32_t)(0x0004d000)          // ルート証明書 user-server 先頭番地  Size 4096
#define SFM_CERT_U6_END             (uint32_t)(0x0004dFFF)          //
#define SFM_CERT_U7_START           (uint32_t)(0x0004e000)          // ルート証明書 user-server 先頭番地  Size 4096
#define SFM_CERT_U7_END             (uint32_t)(0x0004eFFF)          //
#define SFM_CERT_U8_START           (uint32_t)(0x0004f000)          // ルート証明書 user-server 先頭番地  Size 4096
#define SFM_CERT_U8_END             (uint32_t)(0x0004fFFF)          //





#define SFM_LOG_START               (uint32_t)(0x00070000)          // シリアルフラッシュメモリ番地 LOG 先頭番地      Size 128K
#define SFM_LOG_START2              (uint32_t)(0x00080000)          // シリアルフラッシュメモリ番地 LOG 先頭番地
#define SFM_LOG_END                 (uint32_t)(0x0008ffff)          // シリアルフラッシュメモリ番地 LOG  終了番地
#define SFM_LOG_SECT                (uint32_t)(0x00001000)          // セクタサイズ

#define SFM_TD_LOG_START            (uint32_t)(0x00090000)          // シリアルフラッシュメモリ番地 T&D LOG 先頭番地      Size 256K
#define SFM_TD_LOG_END              (uint32_t)(0x000cffff)          // シリアルフラッシュメモリ番地 T&D LOG  終了番地

// Port 定義
#define SFM_CS          IOPORT_PORT_04_PIN_05               // (out) Serial Flash CS


//SST26VF032B / SST26VF032BA INSTRUCTIONS
//Configuration
#define SFLASH_RSTEN        0x66        //Reset Enable
#define SFLASH_RST          0x99        //Reset Memory
#define SFLASH_RDSR         0x05        //Read Status Register
#define SFLASH_WRSR         0x01        //Write Status Register
#define SFLASH_RDCR         0x35        // Read Configuration Register
//Read
#define SFLASH_READ         0x03        //Read Memory 03H X 3 0 1 to  40 MHz
#define SFLASH_HS_READ      0x0B        //Read Memory at Higher Speed
#define SFLASH_SB           0xC0        //Set Burst Length
#define SFLASH_RBSPI        0xEC        //SPI Read Burst with Wrap
//Identification
#define SFLASH_JEDEC_ID     0x9F        //JEDEC-ID Read
#define SFLASH_SFDP         0x5A        //Serial Flash Discoverable Parameters
//Write
#define SFLASH_WREN         0x06        //Write Enable 06H
#define SFLASH_WRDI         0x04        //Write Disable
#define SFLASH_SE           0x20        //Erase 4 KBytes of Memory Array
#define SFLASH_BE           0xD8        //Erase 64, 32 or 8 KBytes of Memory Array
#define SFLASH_CE           0xC7        //Erase Full Array
#define SFLASH_PP           0x02        //Page Program
#define SFLASH_WRSU         0xB0        //Suspends Program/Erase
#define SFLASH_WRRE         0x30        //Resumes Program/Erase
//Protection
#define SFLASH_RBPR         0x72        //Read Block-Protection Register
#define SFLASH_WBPR         0x42        // Write Block-Protection Register
#define SFLASH_LBPR         0x8D        // Lock Down Block-Protection Register
#define SFLASH_nVWLDR       0xE8        //non-Volatile Write Lock-Down Register
#define SFLASH_ULBPR        0x98        //Global Block Protection Unlock
#define SFLASH_RSID         0x88        //Read Security ID 88H X 2 1 1 to 2048
#define SFLASH_PSID         0xA5        //Program User Security ID area
#define SFLASH_LSID         0x85        //Lockout Security ID Programming


#define SPI_BUFFER_SIZE  4096

#if 0
///SPIバッファ
/// @todo   SPIバッファはstaticに変更 or 未使用にしたい
/// @note   　read_system_settings()やLog処理系がserial_flash_multbyte_read()で NULL指定でSPIバッファを直接読む
EDF struct {
    char txbuf[SPI_BUFFER_SIZE + 4];
    char rxbuf[SPI_BUFFER_SIZE + 4];
} spi;
#endif

/* 関数宣言 */
EDF uint32_t serial_flash_multbyte_read(uint32_t, uint32_t, char *); // シリアルＦＬＡＳＨメモリ 複数バイト読み込み
EDF int serial_flash_reset(void);
EDF int serial_flash_block_unlock(void);                           // シリアルＦＬＡＳＨメモリ グローバルブロックプロテクション解除
EDF int serial_flash_multbyte_write(uint32_t, uint32_t, char *);// シリアルＦＬＡＳＨメモリ 複数バイト書き込み
EDF int serial_flash_sector_erase(uint32_t);                       // シリアルＦＬＡＳＨメモリ セクタ消去
EDF int serial_flash_erase(uint32_t, uint32_t);                    // シリアルＦＬＡＳＨメモリ 消去
EDF int serial_flash_copy(uint32_t src, uint32_t dst, uint32_t size);   //シリアルフラッシュ内のデータコピー

//static EDF uint32_t serial_flash_rdid(void);                               // シリアルＦＬＡＳＨメモリ ＩＤ読み込み
//static EDF uint8_t serial_flash_rdsr(void);                                // シリアルＦＬＡＳＨメモリ ステータス読み込み
//static EDF int serial_flash_ready(void);                                      // シリアルＦＬＡＳＨメモリ ＲＥＡＤＹ待ち
//static EDF int serial_flash_single_cycle_command(uint8_t);                     // シリアルＦＬＡＳＨメモリ シングルサイクルコマンド
//static EDF int serial_flash_command(uint32_t, uint32_t);    // シリアルＦＬＡＳＨメモリ コマンド
//static EDF int serial_flash_block_erase(uint32_t);                        // シリアルＦＬＡＳＨメモリ ブロック消去
//EDF void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *);             // TxRx Transfer completed callback.
//EDF void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *);                // SPI error callbacks.
//EDF int wai_SFLASH_READY(int16_t timeout);          //シリアルフラッシュ BUSY解除まち

#endif /* SFMEM_H_ */

