#ifndef _FILES_H_
#define _FILES_H_


#define SFM_FACTORY_START           (uint32_t)(0x00000000)          // シリアルフラッシュメモリ番地 製造時設定域 先頭番地
#define SFM_FACTORY_END             (uint32_t)(0x000000ff)          // シリアルフラッシュメモリ番地 製造時設定域 終了番地
#define SFM_FACTORY_START_B         (uint32_t)(0x00001000)          // シリアルフラッシュメモリ番地 製造時設定域 先頭番地
#define SFM_FACTORY_END_B           (uint32_t)(0x000010ff)          // シリアルフラッシュメモリ番地 製造時設定域 終了番地
#define SFM_FACTORY_SIZE            (uint32_t)(0x00000100)          // シリアルフラッシュメモリ番地 製造時設定域 サイズ

#define SFM_CONFIG_START            (uint32_t)(0x00002000)          // シリアルフラッシュメモリ番地 本体設定域 先頭番地    オリジナル
#define SFM_CONFIG_START_B          (uint32_t)(0x00004000)          // シリアルフラッシュメモリ番地 本体設定域 先頭番地    バックアップ
#define SFM_CONFIG_SECT             (uint32_t)(0x00001000)          // シリアルフラッシュメモリ番地 本体設定域 セクタサイズ
#define SFM_CONFIG_SIZE             (uint32_t)(0x00002000)          // シリアルフラッシュメモリ番地 本体設定域 実使用サイズ（２５６バイト区切り） XXXXX 2000-->3000 2020 9/8


/*
#define SFM_CERT_W_START            (uint32_t)(0x00006000)          // ルート証明書 webstorage 先頭番地  Size 4096
#define SFM_CERT_W_END              (uint32_t)(0x00006FFF)          //
#define SFM_CERT_U_START            (uint32_t)(0x00007000)          // ルート証明書 user定義 先頭番地    Size 4096
#define SFM_CERT_U_END              (uint32_t)(0x00007FFF)          //
*/

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
#define SFM_LOG_SIZE                (uint32_t)(0x00020000)          // LOGサイズ //karel

#define SFM_TD_LOG_START            (uint32_t)(0x00090000)          // シリアルフラッシュメモリ番地 T&D LOG 先頭番地      Size 256K
#define SFM_TD_LOG_END              (uint32_t)(0x000cffff)          // シリアルフラッシュメモリ番地 T&D LOG  終了番地
#define SFM_TD_LOG_SIZE             (uint32_t)(0x00040000)          // LOGサイズ //karel


#endif // _FILES_H_
