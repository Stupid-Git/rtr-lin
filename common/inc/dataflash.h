/**
 * @brief   データフラッシュメモリ処理関数ヘッダファイル
 * @file    dataflash.h
 *
 * @date     Created on: 2019/12/15
 * @author  t.saito
 */

#ifndef COMMON_INC_DATAFLASH_H_
#define COMMON_INC_DATAFLASH_H_

#include <stdint.h>


/* Data Flash Macros */
/** データフラッシュブロックサイズ */
#define FLASH_DF_BLOCK_SIZE             (64)
/** データフラッシュ 先頭ブロックアドレス（管理領域2） */
#define FLASH_DF_64B_BLOCK0             (0x40100000)
/** データフラッシュ ブロック2アドレス(管理領域1) */
#define FLASH_DF_64B_BLOCK1             (0x40100040)
/** データフラッシュ CRC保存ブロック ブロック2~33 */
#define FLASH_DF_CRC_BLOCK2             (0x40100080)

//管理領域1データフラッシュ (64Byte)
#define FLASH_UPDATE_EXEC       (0x40100040)    ///< 書き込み実行フラグ

/* Code Flash Macros */
#define FLASH_CF_BLOCK_SIZE_8KB         (8 * 1024)      ///< コードフラッシュブロックサイズ 8KBブロック
#define FLASH_CF_BLOCK_SIZE_32KB        (32 * 1024)     ///< コードフラッシュブロックサイズ 32KBブロック
#define FLASH_CF_8KB_BLOCK0             (0x00000000)    ///< コードフラッシュ 8KBブロック0 先頭アドレス
#define FLASH_CF_32KB_BLOCK1            (0x00002000)    ///< コードフラッシュ 32KBブロック1 アドレス
#define FLASH_CF_32KB_BLOCK2            (0x00004000)    ///< コードフラッシュ 32KBブロック2 アドレス
#define FLASH_CF_32KB_BLOCK8            (0x00010000)    ///< コードフラッシュ ブロック8 32KBブロック 先頭アドレス
#define FLASH_CF_32KB_BLOCK9            (0x00018000)    ///< コードフラッシュ ブロック9 32KBブロックアドレス
#define FLASH_CF_32KB_BLOCK10           (0x0001FFFF)    ///< コードフラッシュ 32KBブロック10 (バンク1最終アドレス)
#define FLASH_CF_32KB_BLOCK63           (0x001C8000)   ///< コードフラッシュ 32KBブロック63
#define FLASH_CF_32KB_BLOCK64           (0x001D0000)   ///< コードフラッシュ 32KBブロック64
#define FLASH_CF_32KB_BLOCK65           (0x001D8000)   ///< コードフラッシュ 32KBブロック65

extern void flash_hp_open(void);
extern void flash_hp_close(void);
extern void wait_for_blankcheck_flag(void);
extern int CheckBlank_data_flash(uint32_t address, uint32_t size);
extern void eraseBlock_data_flash(uint32_t address, uint32_t num);
extern void write_data_flash(uint32_t src_address, uint32_t dst_address,  uint32_t length);
//TODO extern void BGO_Callback(flash_callback_args_t * p_args);


#endif /* COMMON_INC_DATAFLASH_H_ */
