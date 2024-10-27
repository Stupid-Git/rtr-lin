/**
 * @file    Sfmem.c
 *
 * @brief   Serial Flash Memory SST26VF032B
 *
 * @date    Created on: 2019/02/26
 * @author  haya
 * @note    2020.Jul.22     関数の結果を返す様に変更
 * @note    HAL SPIドライバ内部でmutexを利用しているので排他制御はドライバ内でも行われている
 * @note    関数外で使用しているmutex_sfmemはsystem_threadのミューテックス
 *
 * @note    JEDEC-ID
 * 型番, Manufacturer ID (Byte 1) , Device Type (Byte 2), Device ID (Byte 3)
 * SST26VF016         BFH, 26H, 01H
 * SST26VF032         BFH, 26H, 02H
 * ST26VF064B/064BA   BFH, 26H, 43H
 *
 * @note    serial_flash_multbyte_write()関数は関数内部で256Byteずつ分割書き込みしているので関数外で256Byteずつ送る必要はない
 * @note    serial_flash_multbyte_read()関数は関数内部でバッファサイズ（4096Byte）で分割読み込みするので関数外部で分割読み込みする必要はない
 */

#define _SFMEM_C_

#include "Sfmem.h"

// 05   status read     --> 00 ready
// 06   write enable
// 04   write disable
// 03   read
// 02   write
// 20   sector erase     20 <ad1><ad2><ad3>

static volatile bool spi_done = false;

///SPIバッファ
/// @todo   SPIバッファはstaticに変更 or 未使用にしたい    →　static化
/// @note   　read_system_settings()やLog処理系がserial_flash_multbyte_read()で NULL指定でSPIバッファを直接読む → 廃止
static struct {
    char txbuf[SPI_BUFFER_SIZE + 4];
    char rxbuf[SPI_BUFFER_SIZE + 4];
} spi;



static int serial_flash_ready(void);                                    // シリアルＦＬＡＳＨメモリ ＲＥＡＤＹ待ち
static int serial_flash_wren(void);                                   // シリアルＦＬＡＳＨメモリ 書き込み許可
//static int serial_flash_wrdi(void);                                   // シリアルＦＬＡＳＨメモリ 書き込み禁止
static int serial_flash_single_cycle_command(uint8_t);                // シリアルＦＬＡＳＨメモリ シングルサイクルコマンド
uint32_t serial_flash_rdid(void);                               // シリアルＦＬＡＳＨメモリ ＩＤ読み込み
static uint8_t serial_flash_rdsr(void);                                // シリアルＦＬＡＳＨメモリ ステータス読み込み



static int serial_flash_command(uint32_t, uint32_t);    // シリアルＦＬＡＳＨメモリ コマンド
static int serial_flash_block_erase(uint32_t);                        // シリアルＦＬＡＳＨメモリ ブロック消去


/***********************************************************************************************************************
* Function Name: spi_callback
* Description  : A callback function, called by SPI interrupts.
*                This function name must match the name specified in the SSP Configurator setting for "g_spi SPI Driver
*                on r_sci_spi
* Arguments    : p_args -
*                    Pointer to arguments that provide information when this callback is executed.
* Return Value : None
***********************************************************************************************************************/
void spi_callback (spi_callback_args_t * p_args)
{
    SSP_PARAMETER_NOT_USED(p_args);
    spi_done =  true;
}
/***********************************************************************************************************************
End of function spi_callback
***********************************************************************************************************************/



/**
 * @brief   シリアルＦＬＡＳＨメモリ 複数バイト読み込み
 *          spi.rxbufに読み込み、指定されたバッファポインタにコピーする
 * @param adr   先頭アドレス
 * @param len   読み込みバイト数
 * @param buf   読み込みデータ格納アドレス
 * @note    読み込みデータ格納アドレがＮＵＬＬならメモリ移動しない
 *          SPIのバッファサイズは、SPI_BUFFER_SIZE (4096byte)
 * @note    read_system_settings(),read_repair_settings(),ceat_file_read(), Log処理系が NULL指定でSPIバッファを直接読む → 廃止
 */
uint32_t serial_flash_multbyte_read(uint32_t adr, uint32_t len, char *buf)
{

    uint32_t zan, offset,size;
    uint32_t read_adr;
    uint32_t i,block;
    int ret = -1;
    zan = len;
    offset = 0;
    block = len/SPI_BUFFER_SIZE;
    if((len % SPI_BUFFER_SIZE )!= 0){
        block = block + 1;  
    }

    for(i = 0; i < block; i++){
        
        ret = serial_flash_ready();     // Ready待ち(SPI FLASH ステータス読み込みコマンド)
        if(ret != 0){
            break;
        }
        //        memset(spi.txbuf,0x00, sizeof(spi.txbuf));
//       memset(spi.rxbuf,0x00, sizeof(spi.rxbuf));
        memset(spi.txbuf,0x00, 4);      //4Byteだけクリア
        memset(spi.rxbuf,0x00, 4);      //4Byteだけクリア

        read_adr = adr + SPI_BUFFER_SIZE * i;
        if(zan > SPI_BUFFER_SIZE){
            size = SPI_BUFFER_SIZE;
            zan = zan - size;
        }
        else{          
            size = zan;
            zan = 0;
        }

        //Printf("Read SFM len:%d zan:%d\r\n", len, zan );
        spi.txbuf[0] = SFLASH_READ;
        spi.txbuf[1] = (char)(read_adr >> 16);    //MSB
        spi.txbuf[2] = (char)(read_adr >> 8);
        spi.txbuf[3] = (char)(read_adr >> 0);     //LSB

        ret = serial_flash_command(4, size);
        if(ret != 0){
            break;
        }
        if(buf != NULL){
            memcpy(buf + offset * SPI_BUFFER_SIZE, (char *)&spi.rxbuf[4], size);
            offset++; 
        }
        else{
            break;
        }
    }
    if(ret != 0){
        size = 0;
    }
    return(size);
}

/**
 * @brief   シリアルフラッシュリセットコマンド
 * @retval  0    成功
 * @retval  0以外 失敗
 * @note    2020.Jul.22 動作結果を返値するように変更
 * @note    データも設定も初期化される
 * @note    未使用
 */
int serial_flash_reset(void)
{
    int ret = -1;
    ret = serial_flash_single_cycle_command(SFLASH_RSTEN);    //リセットイネーブル
    if (0 == ret){
        ret = serial_flash_single_cycle_command(SFLASH_RST);      //リセット実行
        if (0 == ret){
            ret = serial_flash_ready();     // Ready待ち(SPI FLASH ステータス読み込みコマンド)
        }
    }
    return(ret);
}



/**
 * シリアルＦＬＡＳＨメモリ 書き込み許可
 * @retval  0    成功
 * @retval  0以外 失敗
 * @attention   ブロック保護解除後に発行すること
 * @note    2020.Jul.22 返値の仕様を変更
 */
static int serial_flash_wren(void)
{
    return (serial_flash_single_cycle_command(SFLASH_WREN));
}



#if 0
/**
 * シリアルＦＬＡＳＨメモリ 書き込み禁止
 */
static int serial_flash_wrdi(void)
{
    volatile int rtn = 0;   // 最適化を阻止
    serial_flash_single_cycle_command(SFLASH_WRDI);
    rtn++;
    return rtn;
}
#endif


/**
 * シリアルＦＬＡＳＨメモリ グローバルブロックプロテクション解除
 * @retval  0    成功
 * @retval  0以外 失敗
 * @note    2020.Jul.22 動作結果を返値するように変更
 */
int serial_flash_block_unlock(void)
{
    int ret = -1;

    ret = serial_flash_ready();     // Ready待ち(SPI FLASH ステータス読み込みコマンド) 2020.Jul.22 追加
    if(ret == 0){
        ret = serial_flash_wren();                                      // 書き込み許可
        if(ret == 0){
            ret = serial_flash_single_cycle_command(SFLASH_ULBPR);
        }
    }
    return(ret);
}



/**
 * @brief   シリアルＦＬＡＳＨメモリ ＩＤ読み込み
 * @return  ＪＥＤＥＣ ＩＤ
 * @note    実行時間１２８μｓ＠８ＭＨｚ
 * 型番, Manufacturer ID (Byte 1) , Device Type (Byte 2), Device ID (Byte 3)
 * SST26VF016         BFH, 26H, 01H
 * SST26VF032         BFH, 26H, 02H
 * ST26VF064B/064BA   BFH, 26H, 43H
 */
uint32_t serial_flash_rdid(void)
{

    serial_flash_ready();         // Ready待ち(SPI FLASH ステータス読み込みコマンド)

    spi.txbuf[0] = SFLASH_JEDEC_ID;
    spi.rxbuf[0] = 0x00;
    spi.rxbuf[1] = 0x00;
    spi.rxbuf[2] = 0x00;
    spi.rxbuf[3] = 0x00;

    serial_flash_command(1, 3);     //書き込み1Byte 読み込み3Byte
    //Printf("%02X %02X %02X %02X (%02X)\r\n", spi.rxbuf[0],spi.rxbuf[1],spi.rxbuf[2],spi.rxbuf[3],spi.txbuf[0]);

    return(((uint32_t)spi.rxbuf[1] << 16) + ((uint32_t)spi.rxbuf[2] << 8) + spi.rxbuf[3]);

}


/**
 * @brief   シリアルＦＬＡＳＨメモリ ステータス読み込み
 * @return  ステータス
 */
uint8_t serial_flash_rdsr(void)
{
    uint8_t rtn;
    int ret = -1;

    spi.txbuf[0] = SFLASH_RDSR;
    spi.rxbuf[0] = 0x00;

    ret = serial_flash_command(1, 1);     //書き込み1Byte 読み込み1Byte
    if(ret == 0){
        rtn = spi.rxbuf[1];
    }
    else{
        rtn = 0xFF; //2020.Jul.22追加
    }
    
    return (rtn);     //(spi.rxbuf[1]);

}


/**
 * シリアルＦＬＡＳＨメモリ ＲＥＡＤＹ待ち
 * SPI FLASH ステータス読み込みコマンド
 * @note   ５０ｍｓタイムアウト
 * @retval  0    成功
 * @retval  0以外 失敗
 * @note    2020.Jul.22 動作結果を返値するように変更
 */
int serial_flash_ready(void)
{
//    volatile bool test_sfm = false;
    int rtn = -1;

    wait.ms_1.sfm_wait = 50; //WAIT_50MSEC;

    for(;;)
    {
        rtn = serial_flash_rdsr() & 0x01;       //シリアルＦＬＡＳＨメモリ ステータス読み込み
        if(rtn == 0){
            //Printf("sfm ready %d(%ld)\r\n", rtn, wait.ms_1.sfm_wait);
            break;
        }
        if(wait.ms_1.sfm_wait == 0){
            Printf("sfm ready timeout !!!!!!!!\r\n");
            rtn = -1;
            break;                                // 無限ループ回避
        }
//        test_sfm = !test_sfm;
    }
    //Printf("SF ready Status %02X\r\n", rtn);
    return(rtn);
 }


/**
 * @brief   シリアルＦＬＡＳＨメモリ シングルサイクルコマンド
 * @param cmd   コマンド
 * @retval  0    成功
 * @retval  0以外 失敗
 * @note    2020.Jul.22 動作結果を返値するように変更
 */
int serial_flash_single_cycle_command(uint8_t cmd)
{
    volatile int rtn = 0;
    spi.txbuf[0] = cmd;
    spi.rxbuf[0] = 0x00;

    rtn = serial_flash_command(1, 0);     //書き込み1Byte 読み込み0Byte
    return (rtn);
}


/**
 * シリアルＦＬＡＳＨメモリ コマンド
 * @param snum  送信バイト数
 * @param rnum  受信バイト数
 * @note    バッファアドレスは固定
 * @note    ５０ｍｓタイムアウト
 * @retval  0    成功
 * @retval  0以外 失敗
 * @note    2020.Jul.22 動作結果を返値するように変更
 */
static int serial_flash_command(uint32_t snum, uint32_t rnum)
{

    ssp_err_t      err;
    int ret = 0;

    //Printf("serial flash command %02X \r\n", spi.txbuf[0]);

    g_ioport.p_api->pinWrite(SFM_CS, IOPORT_LEVEL_LOW);         // CS Low

    spi_done = false;       //SPIコールバックでtrueになる

    err = g_spi.p_api->writeRead(g_spi.p_ctrl, spi.txbuf, spi.rxbuf, snum + rnum, SPI_BIT_WIDTH_8_BITS);

    if (SSP_SUCCESS != err)
    {
        Printf("spi flash command error %d\n", (int)err);
        ret = -1;
//        serial_flash_reset();       //シリアルフラッシュリセット
    }
    else{
        // Wait until SPI operation completes  50msec のタイマーを追加のこと
         wait.sfm = WAIT_200MSEC;   //4KB読むときは 100ms程度は必要なので
         while (false == spi_done)
         {
             if(wait.sfm == 0){
                Printf("spi flash command error 2\n"); 
                ret = -1;
                 break;
             }
         }
    }


    g_ioport.p_api->pinWrite(SFM_CS, IOPORT_LEVEL_HIGH);        // CS High

    return(ret);
 
}


/**
 * @brief   シリアルＦＬＡＳＨメモリ 複数バイト書き込み
 *
 * 256Byteページずつ書き込む
 * @param adr   アドレス
 * @param len   書き込みバイト数
 * @param buf   書き込みデータ格納ポインタ
 * @retval  0    成功
 * @retval  0以外 失敗
 * @note    2020.Jul.22 動作結果を返値するように変更
 */
int serial_flash_multbyte_write(uint32_t adr, uint32_t len, char *buf)
{
    ssp_err_t      err;
    uint32_t write_size, j, k;
    int ret = 0;

    //Printf(" serial_flash_multbyte_write  adr %08X len = %d \r\n", adr, len);

    j = 0;
//    memset(spi.txbuf,0x00, sizeof(spi.txbuf));
//    memset(spi.rxbuf,0x00, sizeof(spi.rxbuf));
    memset(spi.txbuf,0x00, 4);      //4Byteだけクリア
    memset(spi.rxbuf,0x00, 4);      //4Byteだけクリア
    while(len > 0)
    {
        write_size = (adr / 256 + 1) * 256 - adr;                        // 今回の書き込みバイト数（初回、２５６未満の時だけ２５６未満）
        if(write_size > len){
            write_size = len;
        }

        //Printf("mulit byte write %d/%ld\r\n\r\r", write_size,len);
        // ページ区切り書き込み
        ret = serial_flash_ready();     // Ready待ち(SPI FLASH ステータス読み込みコマンド)
        if(ret == -1){
            break;
        }
        ret = serial_flash_wren();                                    // 書き込み許可
        if(ret == -1){
            break;
        }
        g_ioport.p_api->pinWrite(SFM_CS, IOPORT_LEVEL_LOW);         // CS Low

        spi_done = false;       //SPIコールバックでtrueになる


        spi.txbuf[0] = SFLASH_PP;                                    // ０ｘ００００００～、０ｘ３Ｆ００００～ はブロック幅が違うので注意！
        spi.txbuf[1] = (uint8_t)(adr >> 16);
        spi.txbuf[2] = (uint8_t)(adr >> 8);
        spi.txbuf[3] = (uint8_t)(adr >> 0);

        err = g_spi.p_api->writeRead(g_spi.p_ctrl, spi.txbuf, spi.rxbuf, 4, SPI_BIT_WIDTH_8_BITS);
        if (SSP_SUCCESS != err)
        {
            Printf("spi writeread error 1 %d\r\n", err);
            ret = -1;
            break;
        }
        // Wait until SPI operation completes  50msec のタイマーを追加のこと
        wait.sfm = WAIT_50MSEC;
        while (false == spi_done){
            if(wait.sfm == 0){ 
                Printf("spi writeread error time out 1\r\n");
                ret = -1;
                break;
            }
        }
        if(ret == -1){
            break;
        }
        for(k = 0; k < write_size; k++){
            spi.txbuf[k] = buf[j++];
        }

        spi_done = false;       //SPIコールバックでtrueになる

        err = g_spi.p_api->writeRead(g_spi.p_ctrl, spi.txbuf, spi.rxbuf, write_size, SPI_BIT_WIDTH_8_BITS);
        if(err != SSP_SUCCESS){
            Printf("spi writeread error 2 %d\r\n", err);
            ret = -1;
            break;
        }
        if(ret == -1){
            break;
        }
        wait.sfm = WAIT_50MSEC;
        while (false == spi_done)
        {
            if(wait.sfm == 0){
                Printf("spi writeread error time out 2\r\n");
                ret = -1;
                break;
            }
        }

        g_ioport.p_api->pinWrite(SFM_CS, IOPORT_LEVEL_HIGH);        // CS High

        if(len >= write_size){
            len -= write_size;
        }
        else {
            len = 0;
        }

        adr += write_size;
    }//while

    g_ioport.p_api->pinWrite(SFM_CS, IOPORT_LEVEL_HIGH);        // CS High
    if(ret != 0){
        Printf("## Error! Write Serial Flash \r\n");
    }
    return(ret);

}


/**
 * @brief   シリアルＦＬＡＳＨメモリ セクタ消去(4KB)
 * @param adr   アドレス
 * @note    処理時間 約40msec
 * @note    4KBセクタ
 * @note    Sector/Block Erase: 18 ms (typ), 25 ms (max)
 * @note    A31-A12以下のアドレス指定は不要
 * @retval  0    成功
 * @retval  0以外 失敗
 * @note    2020.Jul.22 動作結果を返値するように変更
 */
int serial_flash_sector_erase(uint32_t adr)
{
    int ret = -1;
    //Printf("serial_flash_sector_erase \r\n");
    for(;;){
        ret = serial_flash_ready();         // Ready待ち(SPI FLASH ステータス読み込みコマンド)
        if(ret != 0){
            break;
        }
        ret = serial_flash_wren();                                        // 書き込み許可
        if(ret != 0){
            break;
        }
        spi.txbuf[0] = SFLASH_SE;                                        // ４ｋｂ消去
        spi.txbuf[1] = (uint8_t)(adr >> 16);
        spi.txbuf[2] = (uint8_t)(adr >> 8);
        spi.txbuf[3] = (uint8_t)(adr >> 0);

        // in out 100us
        ret = serial_flash_command(4, 0);     //書き込み4Byte 読み込み0Byte
        if(ret != 0){
            break;
        }
            // readyになるまで msec
        ret = serial_flash_ready();         // Ready待ち(SPI FLASH ステータス読み込みコマンド)
        break;
    }
    if(ret != 0){
        Printf("## Error! Erase Sector Serial Flash \r\n");
    }
    //Printf("serial_flash_sector_erase end %ld\r\n", adr);
    return(ret);

}


/**
 * シリアルＦＬＡＳＨメモリ ブロック消去 (8KB/32KB/64KB)
 * @param adr   アドレス
 * @retval  0    成功
 * @retval  0以外 失敗
 * @note    2020.Jul.22 動作結果を返値するように変更
 */
int serial_flash_block_erase(uint32_t adr)
{
    int ret = -1;

    for(;;){
        ret = serial_flash_ready();      // Ready待ち(SPI FLASH ステータス読み込みコマンド)
        if(ret != 0){
            break;
        }                                // ＲＥＡＤＹ待ち
        ret = serial_flash_wren();                                       // 書き込み許可
        if(ret != 0){
            break;
        }                                        // 書き込み許可

        spi.txbuf[0] = SFLASH_BE;                                        // ０ｘ００００００～、０ｘ３Ｆ００００～ はブロック幅が違うので注意！
        spi.txbuf[1] = (uint8_t)(adr >> 16);
        spi.txbuf[2] = (uint8_t)(adr >> 8);
        spi.txbuf[3] = (uint8_t)(adr >> 0);

        ret = serial_flash_command(4, 0);     //書き込み4Byte 読み込み0Byte
        if(ret != 0){
            break;
        }
        ret = serial_flash_ready();         // Ready待ち(SPI FLASH ステータス読み込みコマンド)
        break;
    }
    if(ret != 0){
        Printf("## Error! Erase Block Serial Flash \r\n");
    }
    return(ret);
}


/**
 * @brief   シリアルＦＬＡＳＨメモリ 消去
 * @param sta   開始アドレス
 * @param end   最終アドレス
 * @retval  0    成功
 * @retval  0以外 失敗
 * @note    2020.Jul.22 動作結果を返値するように変更
 */
int serial_flash_erase(uint32_t sta, uint32_t end)
{

    uint32_t i, j;
    int ret = -1;

    for(i = sta; i <= end; i += (1024 * 64))
    {
        if(i == 0x00000000)
        {
            for(j = 0x00000000; j <= 0x00007fff; j += (1024 * 8)){       // 32KB
                ret = serial_flash_block_erase(j);  // ８ｋｂｙｔｅづつ消去
            }
            // ０ｘ００８０００～０ｘ００ｆｆｆｆ消去（３２ｋｂｙｔｅ消去）
            ret = serial_flash_block_erase(0x00008000);                       // 32KB
        }
        else if(i == 0x003f0000)
        {
            // ０ｘ３ｆ００００～０ｘ３ｆ７ｆｆｆ消去（３２ｋｂｙｔｅ消去）
            ret = serial_flash_block_erase(i);                                // 32KB

            for(j = 0x003f8000; j <= 0x003fffff; j += (1024 * 8)){       // 32KB
                ret = serial_flash_block_erase(j);  // ８ｋｂｙｔｅづつ消去
            }
        }
        else {
            ret = serial_flash_block_erase(i);                       // ６４ｋｂｙｔｅづつ消去
        }
        if(ret != 0){
            Printf("## Error! Erase Serial Flash \r\n");
            break;
        }
    }

    return(ret);


}

/**
 * シリアルフラッシュ内のデータコピー
 * 設定領域を読みだしてから書き込む
 * @attention   消去はセクタ単位（4KB）
 * @param src   シリアルフラッシュの読み出し元アドレス
 * @param dst   シリアルフラッシュの書き込み先アドレス
 * @param size  コピーサイズ
 * @retval  0       成功
 * @retval  0以外     失敗
 * @note    2020.Jul.22
 *
 */
int serial_flash_copy(uint32_t src, uint32_t dst, uint32_t size)
{
    uint32_t len;
    int ret = -1;

    // バックアップ域を消去
    serial_flash_block_unlock();                        // グローバルブロックプロテクション解除
    serial_flash_sector_erase(dst);                      //4KB消去

    if(size > SFM_CONFIG_SECT){
        serial_flash_sector_erase(dst + SFM_CONFIG_SECT);    //次の4KB消去
    }
// 2023.03.15 ↓ 4KB単位で2回に分けてコピーを実施
//    len = serial_flash_multbyte_read(src , size, NULL);
//    if(len == size){
//        ret = serial_flash_multbyte_write(dst , size, (char *)&spi.rxbuf[4]);
//    }
    // 最初の4KB
    len = serial_flash_multbyte_read(src , SFM_CONFIG_SECT, NULL);
    if(len == SFM_CONFIG_SECT){
        ret = serial_flash_multbyte_write(dst , SFM_CONFIG_SECT, (char *)&spi.rxbuf[4]);
    }
    // 次の4KB
    len = serial_flash_multbyte_read(src + SFM_CONFIG_SECT , SFM_CONFIG_SECT, NULL);
    if(len == SFM_CONFIG_SECT){
        ret = serial_flash_multbyte_write(dst + SFM_CONFIG_SECT , SFM_CONFIG_SECT, (char *)&spi.rxbuf[4]);
    }
// 2023.03.15 ↑ 4KB単位で2回に分けてコピーを実施
    return(ret);
}

#if 0
/**
 * シリアルフラッシュ BUSY解除まち
 * @param timeout
 * @return  -1 = BUSY  -1以外 = 準備完了（待った時間）
 * @note 未使用
 */
int wai_SFLASH_READY(int16_t timeout){
    uint8_t sts;
    int rtn = -1;
    int16_t cnt = timeout;

    do{
        tx_thread_sleep (1);
        cnt--;
        sts = serial_flash_rdsr();

        if(0 == (sts&0x81)){      //bit0, bit7がBUSYフラグ
            rtn = (int16_t)(timeout - cnt);
            break;
        }

     }while(cnt >= 0);
     return(rtn);
 }


/**
 * SPIフラッシュ複数読み込み（バッファ直接）
 * @param adr
 * @param len
 * @param buf
 * @note 未使用
 */
void serial_flash_multbyte_readBuf(uint32_t adr, uint32_t len, uint8_t *buf)
{

    serial_flash_ready();         // Ready待ち(SPI FLASH ステータス読み込みコマンド)

    spi.txbuf[0] = SFLASH_READ;        //コマンド
    spi.txbuf[1] = (uint8_t)(adr >> 16);    //アドレス
    spi.txbuf[2] = (uint8_t)(adr >> 8);
    spi.txbuf[3] = (uint8_t)(adr >> 0);

//    serial_flash_command(spi.txbuf, 4, buf, len);
    serial_flash_command(4, len);
}
#endif

