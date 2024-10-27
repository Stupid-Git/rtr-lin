/**
  ******************************************************************************
    @file   programPSoC.c
    @brief  PSoCファーム更新処理関数
    @date   2020/01/28
    @author t.saito
    @note   2020.01.28  RTR501Bから移植中
    @note   2020.Jun.18 PSoC FW更新可能

    @note   PSoCのブートローダとのUART通信は全二重通信に設定すること
    @todo   PSoCのブートローダ通信処理をBLEの通信スレッドに組み込む

    @note   PSoCのメモリは256Byte列が 512列×2アレイ構成。メモリの最終列にはメタデータがある。
  ******************************************************************************

*/
#ifndef _PROGRAM_PSOC_C_
#define _PROGRAM_PSOC_C_        // ヘッダファイルに対応するソース中に記述


#include <string.h>
#include <stdlib.h>
#include "ble_thread.h"
#include "hal_data.h"
#include "common_data.h"
#include "dataflash.h"
#include "General.h"
#include "Globals.h"
#include "programPSoC.h"        //PSoC FW更新用関数
#include "Sfmem.h"
#include "ble_thread_entry.h"

extern void ble_reset(void);        //ble_thread_entry.h


//PSoCファーム更新用（ファーム書き込み）

//int fPSoCCompleteTx;           ///< PSoC UART送信完了(PSoCファーム書き換え用)
static int fPSoCCompleteRx;           ///< PSoC UART受信完了(PSoCファーム書き換え用)




/// UART通信用  PSoC CRC
static uint16_t PSoC_CRC;

//プロトタイプ
//PSoCブートローダ用 UARTコマンド
static int PSoCCommand_0x31(void);                                          //ベリファイ アプリケーション チェックサム 0x31コマンド
static int PSoCCommand_0x32(uint8_t array);                                 //フラッシュサイズ取得; 0x32コマンド
//未使用
#if 0
static int PSoCCommand_0x34(uint8_t array, uint16_t column);                //列消去 0x34コマンド
static int PSocCommand_0x35(void);                                          //ブートローダ同期（フラッシュバッファ破棄） 0x35コマンド
#endif

static int PSocCommand_0x38(void);                                          //PSoCファーム更新モード移行（ リセット＋0x38コマンド）
static int PSocCommand_0x37(char *pData);                                   //0x37コマンド  PSoCファーム更新 データブロック
static int PSocCommand_0x39(uint8_t array, uint16_t column, char *pData);   //0x39コマンド PSoCファーム更新 データブロック
static int PSocCommand_0x3A(uint8_t array, uint16_t column);                //列のチェックサム取得 0x3A
static int PSocCommand_0x3B(void);                                          //ブートローダモード終了   0x3Bコマンド

static uint8_t PSoC_ldr_SUM( char *buf, int start, int len );               //PSoCブートローダ用SUM演算単純加算 2の補数)
static int PSoC_ldr_TxRx(void);
static uint16_t CyBtldr_ComputeChecksum(char * buf, int size);              //PSoCブートローダ用CRC演算


//******************** PSoC FW更新用関数 ********************
/*
1   0x38            Enter Boot loader
2   0x32            Get Flash size
3   0x37/0x39   Send Data/Program Row
4   0x3A            Verify Row
5   0x31            Verify Application
6   0x3B            End Boot loader



PSoCフラッシュ書き換え用データ受信

PSoCフラッシュ書き換えコマンド

フラッシュに全データが揃っているか確認


１．  ブートローダーモードへ 0x38コマンド

    戻り
    SiId    シリコンID
    SiRev   シリコンリビジョン
    blVer   ブートローダーバージョン

    ２．  フラッシュサイズ取得 0x32コマンド
    引数  アレイID
    戻り
    startRow        開始列
    endRow      終了列


一回に送るデータ数、フレーム数を計算

    do｛
    ３   データ送信1列分になるまで分割送(0x37コマンド)、最終フレームはProgram Row(0x39コマンド)


    ４．列のチェックサム確認 (0x3Aコマンド)
        OKなら次の列をプログラム

    ｝while();

５．全列プログラムが終わったらアプリケーションのチェックサムを確認（0x31コマンド）


６．全てOKならブートローダーモードを抜ける。


*/

/**
  ******************************************************************************
    @brief  PSoCフラッシュ更新（PSoCブートローダ通信中）
    @retval 結果
    @note   呼び出し元
    @note   この関数を呼び出したらPSoCへの通信等は行なわないこと
    @note   PSoCブートローダと通信するためFWを更新完了するまでBLE通信はできなくなる。
    @note   データフラッシュ（内蔵EEPROM）にPSoC書き込み中フラグを書いているため、
                不慮のリセット等の時はメインプログラムの最初にチェックしてPSoCファームウェア更新を完了させること
    @note   ブートローダがあるので 0x01600(アレイ0 22列)～0x3FFF(1023列(アレイ1 511列))に書き込む
  ******************************************************************************
*/
void PSoC_program_FW(void){

    char Buff[256];         //フラッシュデータ読み出し用バッファ
    int i;
    int PSoCsequence = 0;   //PSoCファーム更新のシーケンス番号
    int retry= 0;                   //リトライカウンタ
    uint8_t arrayNo;            //フラッシュアレイ番号（前半128KB=0、後半128KB=1）
    uint8_t Sum;                //フラッシュ内データ256Byteのチェックサム

    uint32_t FlashAddress = *(uint32_t *)FLASH_MAN2_SRC_ADR; //ファームデータのキャッシュされているアドレス（SPI FLASH） SPIフラッシュ時0x00100000固定値

    uint32_t readBlock = 22; //読み出しブロック番号（256Byte 列） ブートローダがあるので 0x01600～0x3FFFまで書く

    int sts;
    uint32_t eep_buf;       //データフラッシュ書き込み用

    g_sf_comms4.p_api->lock(g_sf_comms4.p_ctrl, SF_COMMS_LOCK_ALL, TX_NO_WAIT);    //comm4ロック
    g_sf_comms4.p_api->close(g_sf_comms4.p_ctrl);       //一旦UART4を閉じる
//    sf_comms_init4();     //UART4初期化 関数内部でopenしている
    g_sf_comms4.p_api->open(g_sf_comms4.p_ctrl, g_sf_comms4.p_cfg);

    do{
       arrayNo =  ((readBlock > 511) ? 1 : 0);

        switch(PSoCsequence){

            case 0:     //PSoC FW更新開始
                Printf(" Start Update PSoC FW \r\n");
                fPSoCFwUpExec = 1;      //PSoCファームウェア更新中（PSoCブートローダ動作）

                eep_buf = 0x0000001;
                flash_hp_open();
                write_data_flash((uint32_t)&eep_buf, DATA_FLASH_ADR_PSoC_WRITE_MODE,  4);   //PSoC書き込み中フラグ セット
                flash_hp_close();

                PSoCsequence++;
                break;

            case 1:
                Printf(" #1: Start Boot loader\r\n");
                if( 0 == PSocCommand_0x38()){   //PSoCファーム更新モード移行（ リセット＋0x38コマンド）
                    PSoCsequence++;
                }else{
                    retry++;
                }
                break;

            case 2:
                Printf(" #2: Get Flash Memory Size \r\n");
                if( 0 == PSoCCommand_0x32(arrayNo)){   //フラッシュサイズ取得; 0x32コマンド
                    PSoCsequence++;
                    Printf(" Arraay 0 Strat row= %d \r\n" ,PSoC_Strat_row);
                    Printf(" Arraay 0 End row= %d \r\n" ,PSoC_End_row);
                }else{
                    retry++;
                }
                break;

            case 3:
            /*
                PSoCのファーム列番号0～1023)はSPIフラッシュ に書き込まれている
                                    フラッシュから256Byteデータを読み出す。
                                    分割して書き込む
                                    最終フレームはアレイ番号＋列番号＋データ
            */
//                Printf(" #3: Send Flash Data (%d) \r\n",readBlock );
                //フラッシュからFWデータを読み出す

                serial_flash_multbyte_read((uint32_t)(char *)(uint32_t)(FlashAddress + (readBlock*256)), 256, Buff);


                //256Byteのチェックサム計算
                Sum = PSoC_ldr_SUM( Buff, 0, 256 );     //PSoCブートローダ用SUM演算単純加算 2の補数)

                //列書き込み(256Byte)は0x37コマンド （データ53Byte×4）＋0x39コマンド（ アレイID＋列番号2Byte+データ44Byte）で1セット
                for(i = 0; i < 4; i++){
                    Retry0:
                    sts = PSocCommand_0x37((char *)&Buff[0 + i *53]);
                    if(sts != 0){
                        Printf(" Error Send Data\r\n");
                        retry++;
                        goto Retry0;
                    }
                }
       Retry1:

                sts = PSocCommand_0x39(arrayNo, (uint16_t)readBlock, (char *)&Buff[212]); //0x39コマンド PSoCファーム更新 データブロック
                if(sts == 0){
                    sts = PSocCommand_0x3A(arrayNo, (uint16_t)readBlock);    //列のチェックサム取得 0x3A
                    if(sts == 0){
                        if(Sum == PSoCCheckSum){    //チェックサムOK

PGM_SKIP:
                            readBlock++;    //1列（256Byte分）進める
                            if(readBlock > 1023){
                                 PSoCsequence++;
                                 break;
                            }
                            if(readBlock % 16 == 0)      //4KBブロックごとにデータフラッシュのブロックCRCをチェック
                            {
                                uint32_t Block4K_No = readBlock / 16;
                                uint32_t data_flash_adr = (uint32_t)(FLASH_DF_CRC_BLOCK2 + Block4K_No * 4);
                                uint16_t Block4K_Crc = *(uint16_t *)(data_flash_adr +2);
                                Printf("  4KiB Block(%d) CRC = %04X \r\n", Block4K_No, Block4K_Crc );
                                if(Block4K_Crc == 0x0000 || Block4K_Crc == 0xE03E)    //全データ0 or 0xFF（ブランク）の場合スキップ
                                {
                                   readBlock +=15;  //データ無しなので4KB 書き込みスキップ
                                   goto PGM_SKIP;
                                }
                            }
                        }else{
                            Printf(" Error Check Sum\r\n");
                            retry++;
                        }
                    }else{
                        Printf(" Error Get Check Sum\r\n");
                        retry++;
                    }
                }else{
                    Printf(" Error Program Flash\r\n");
                    retry++;
                    goto Retry1;
                }
                break;

            case 4:
                Printf(" #4: Verify Application Check Sum \r\n");
                if(0 == PSoCCommand_0x31()){                //ベリファイ アプリケーション チェックサム 0x31コマンド
                    PSoCsequence++;
                }else{
                    Printf(" Error Application Check Sum \r\n");
                    retry++;
                }
                break;

            case 5:
                Printf(" #5: Complete Program Flash!! \r\n");
                if(0 == PSocCommand_0x3B())             //ブートローダモード終了   0x3Bコマンド(このコマンドは応答無し)
                {
                    flash_hp_open();
                    eraseBlock_data_flash(FLASH_UPDATE_EXEC,1);//データフラッシュ64Byteクリア PSoC FW更新実行フラグ、PSoC書き込み中フラグ クリア
                    flash_hp_close();
                    PSoCsequence++;
                }
                break;
            }

        if(PSoCsequence >= 6){
            break;      //更新完了 ループを抜ける
        }
        if(retry > 1000){       //デバッグ用
            flash_hp_open();
            write_data_flash((uint32_t)&readBlock, DATA_FLASH_ADR_PSoC_WRITE_COLUMN, 4);  //PSoC書き込み済み列番号 1～512
            flash_hp_close();
            while(1);        //とりあえず
          break;      //ループを抜ける
        }

    }while(1);

    if(retry != 0){
        __NOP();        //デバッグ用
    }

    fPSoCFwUpExec = 0;
    fPSoCUpdateMode = 0;    //PSoC FW更新モード終了

    //PSoCをリセットする
    Printf(" Reset PSoC! \r\n");
    ble_reset();              //PSoCリセットさせる
    g_external_irq2.p_api->enable(g_external_irq2.p_ctrl);                          //IRQ2イネーブル

}


/**
  ******************************************************************************
    @brief  PSoCのFW更新が途中で終わっていないかチェック、更新
    @note   呼び出し元
    @note	2020.Sep.15 データフラッシュのブランクチェックを厳密にした
  ******************************************************************************
*/
void checkProgramPSoC(void)
{
    if(FLASH_RESULT_NOT_BLANK == CheckBlank_data_flash(DATA_FLASH_ADR_PSoC_WRITE_MODE, 4)){
    if(0x0000001 == *(uint32_t *)DATA_FLASH_ADR_PSoC_WRITE_MODE)    //PSoC書き込み中フラグチェック
    {
            if(FLASH_RESULT_NOT_BLANK == CheckBlank_data_flash(FLASH_UPDATE_EXEC, 4)){
        if(0x0000003 == *(uint32_t *)FLASH_UPDATE_EXEC){
            fPSoCUpdateMode = 1;    //PSoC FW更新モード
//            PSoC_command_exec();    //PSoCフラッシュ更新（PSoCブートローダ通信中）
            tx_thread_resume(&ble_thread);     //BLEスレッド起動(他スレッドから呼ぶ場合に必要)
                }
            }
        }
    }
}

/**
  ******************************************************************************
    @brief  PSoCブートローダ用SUM演算（単純加算 2の補数)
    @param  *pBuf    演算対象へのポインタ
    @param  start   開始位置
    @param  len 長さ
    @retval PSoCブートローダ用SUM
    @note   呼び出し元
    @note
  ******************************************************************************
*/
static uint8_t PSoC_ldr_SUM( char *pBuf, int start, int len )
{
    int i;
    int32_t sum = 0;
    uint8_t checksum;

    for (i = 0; i < len; i++)
    {
        sum += (pBuf[i + start]);
    }
    checksum = (uint8_t)((sum & 0xff));

    return ((uint8_t)(1 + ~checksum));


}
 /**
  ******************************************************************************
    @brief  PSoCブートローダ用CRC演算
    @param  pBuf  - The data to compute the checksum on
    @param  size - The number of bytes contained in buf.
    @return The checksum for the provided data.
    @note   PSoCブートローダ用CRC演算(CRCペリフェラル未使用）

                除数  0x8408（0x1021）  生成多項式 X^16＋X^12＋X^5＋１ （CCITT）
                初期値             0xFFFF
                出力XOR       0xFFFF
                右送り
                バイト反転
  ******************************************************************************
*/
static uint16_t CyBtldr_ComputeChecksum(char* pBuf, int size)
{
    uint16_t crc = 0xffff;
    uint16_t tmp;
    int32_t i;

    if (size == 0){
        return ((uint16_t)(~crc));
    }

    do{
        for (i = 0, tmp = 0x00ff & *pBuf++; i < 8; i++, tmp >>= 1)
        {
            if ((crc & 0x0001) ^ (tmp & 0x0001)){
                crc = (crc >> 1) ^ 0x8408;
            }
            else{
                crc >>= 1;
            }
        }
    }
    while (--size);

    crc = (uint16_t)(~crc);
    tmp = crc;
    crc = (uint16_t)((crc << 8) | (tmp >> 8 & 0xFF));

    return (crc);

}



/**
  ******************************************************************************
    @brief  PSoCブートローダ用 UART送受信開始
    @note   呼び出し元
    @note
  ******************************************************************************
*/
static int PSoC_ldr_TxRx(void)
{
    ssp_err_t err;



    err = g_sf_comms4.p_api->lock(g_sf_comms4.p_ctrl, SF_COMMS_LOCK_ALL, TX_WAIT_FOREVER);    //comm4ロック

    fPSoCCompleteRx = 0;
    pPSoCUartRx->soh = 0xFF;

    err = g_sf_comms4.p_api->write(g_sf_comms4.p_ctrl, (uint8_t*)&pPSoCUartTx->soh, (uint32_t)(pPSoCUartTx->data_length+ 7), TX_WAIT_FOREVER);
    if(err == SSP_SUCCESS) {
Retry_Rx:
        err = g_sf_comms4.p_api->read(g_sf_comms4.p_ctrl, (uint8_t*)&pPSoCUartRx->soh, 1, 100);     //1Byte読み出し

        if(err == SSP_SUCCESS) {
            if(0x01 != pPSoCUartRx->soh){
                goto Retry_Rx;
            }
            err = g_sf_comms4.p_api->read(g_sf_comms4.p_ctrl, (uint8_t*)&pPSoCUartRx->status, 3, 500);       //Lengthまで3Byte読み出し(受信は完了している)
            if(err == SSP_SUCCESS) {

                err = g_sf_comms4.p_api->read(g_sf_comms4.p_ctrl, (uint8_t*)&pPSoCUartRx->data[0], (uint32_t)(pPSoCUartRx->data_length +3), 500);       //Lengthまで4Byte読み出し(受信は完了している)
                if(err == SSP_SUCCESS) {
                 fPSoCCompleteRx = 1;

                }else{
                    __NOP();//デバッグ用
                }
            }
        }else if(err == SSP_ERR_BREAK_DETECT){
            goto Retry_Rx;      //リセット起動時 ポートがLOなので0xFFが出る
        }
    }
   g_sf_comms4.p_api->unlock(g_sf_comms4.p_ctrl, SF_COMMS_LOCK_ALL);    //comm4ロック解除

    return (err);
}


/**
  ******************************************************************************
    @brief  PSoCブートローダ用 UARTコマンド
                ベリファイ アプリケーション チェックサム 0x31コマンド
    @retval 0 = 成功 0以外失敗
    @note   呼び出し元
    @note
  ******************************************************************************
*/
static int PSoCCommand_0x31(void){

    pPSoCUartTx->soh = 0x01;        //SOH
    pPSoCUartTx->command = 0x31;
    pPSoCUartTx->data_length = 0x0000;
    pPSoCUartTx->data[0] = 0x3C;        //CRC LSB
    pPSoCUartTx->data[1] = 0x17;        //CRC MSB
    pPSoCUartTx->data[2] = 0x17;            //ETB /0x17

    if(0 ==PSoC_ldr_TxRx())//PSoCブートローダ用 UART送受信開始
    {
        //応答データ1Byte
        if(fPSoCCompleteRx == 1){
            if(pPSoCUartRx->data_length == 1){
                if(0 != pPSoCUartRx->data[0])       //0以外=成功
                {
                    return (0);       //成功
                }
            }
        }
    }

    return (1);          //失敗

}



/**
  ******************************************************************************
    @brief  PSoCブートローダ用 UARTコマンド
                フラッシュサイズ取得; 0x32コマンド
    @retval 0 = 成功 0以外失敗
    @note   呼び出し元
    @note
  ******************************************************************************
*/
static int PSoCCommand_0x32(uint8_t array){

    pPSoCUartTx->soh = 0x01;        //SOH
    pPSoCUartTx->command = 0x32;        //フラッシュサイズ取得;
    pPSoCUartTx->data_length = 0x0001;
    pPSoCUartTx->data[0] = array;       //フラッシュアレイ番号

    PSoC_CRC = CyBtldr_ComputeChecksum(&pPSoCUartTx->soh, pPSoCUartTx->data_length+4);      //CRC計算
    pPSoCUartTx->data[pPSoCUartTx->data_length] = (uint8_t)(PSoC_CRC&0x00FF);                   //CRC16 LSB
    pPSoCUartTx->data[pPSoCUartTx->data_length + 1] = (uint8_t)((PSoC_CRC>>8)&0x00FF);      //CRC16 MSB

    pPSoCUartTx->data[pPSoCUartTx->data_length + 2] = 0x17;         //ETB /0x17

    if(0 == PSoC_ldr_TxRx())    //PSoCブートローダ用 UART送受信開始
    {
        //応答データ4Byte
        if(fPSoCCompleteRx == 1){
            if(pPSoCUartRx->data_length >= 4){
                PSoC_Strat_row  = pPSoCUartRx->data[0] | (uint16_t)pPSoCUartRx->data[1]<<8;         //ブートローダブルフラッシュ領域最初の列番号（アレイ1）
                PSoC_End_row    = pPSoCUartRx->data[2] | (uint16_t)pPSoCUartRx->data[3]<<8;         //ブートローダブルフラッシュ領域最終の列番号(アレイ1）
                return  (0);      //成功
            }
        }
    }
    return (1);               //失敗

}

//未使用
#if 0
/**
  ******************************************************************************
    @brief  PSoCブートローダ用 UARTコマンド
                列消去 0x34コマンド
    @param  array       フラッシュアレイ番号
    @param  column  データ列番号
    @retval 0 = 成功 0以外失敗
    @note   呼び出し元
    @note
  ******************************************************************************
*/
static int PSoCCommand_0x34(uint8_t array, uint16_t column){

    pPSoCUartTx->soh = 0x01;        //SOH
    pPSoCUartTx->command = 0x34;                //列消去
    pPSoCUartTx->data_length = 0x0003;

    pPSoCUartTx->data[0] = array;       //フラッシュアレイ番号
    pPSoCUartTx->data[1] = (uint8_t)(column&0xFF);          //データ列番号下位
    pPSoCUartTx->data[2] = (uint8_t)((column>>8)&0xFF);     //データ列番号上位

    PSoC_CRC = CyBtldr_ComputeChecksum(&pPSoCUartTx->soh, pPSoCUartTx->data_length+4);      //CRC計算
    pPSoCUartTx->data[pPSoCUartTx->data_length] = (uint8_t)(PSoC_CRC&0x00FF);               //CRC LSB
    pPSoCUartTx->data[pPSoCUartTx->data_length+1] = (uint8_t)((PSoC_CRC>>8)&0x00FF);        //CRC MSB

    pPSoCUartTx->data[pPSoCUartTx->data_length+2] = 0x17;           //ETB /0x17

    if(0 == PSoC_ldr_TxRx())    //PSoCブートローダ用 UART送受信開始
    {
        //応答データ0Byte
        if(fPSoCCompleteRx == 1){
            return (0);           //成功
        }
    }
    return (1);           //失敗
}

/**
  ******************************************************************************
    @brief  PSoCブートローダ用 UARTコマンド
                ブートローダ同期（フラッシュバッファ破棄） 0x35コマンド
    @param  none
    @retval 0 = 成功 0以外失敗
    @note   呼び出し元
    @note
  ******************************************************************************
*/
static int PSocCommand_0x35(void){


    pPSoCUartTx->soh = 0x01;        //SOH
    pPSoCUartTx->command = 0x35;                //ブートローダ同期
    pPSoCUartTx->data_length = 0x0000;
    pPSoCUartTx->data[0] = 0x5F;        //CRC LSB
    pPSoCUartTx->data[1] = 0x76;            //CRC MSB
    pPSoCUartTx->data[2] = 0x17;            //ETB /0x17

    if(0 == PSoC_ldr_TxRx())    //PSoCブートローダ用 UART送受信開始
    {
        //応答データ0Byte
        if(fPSoCCompleteRx == 1){
            return (0);               //成功
        }
    }
    return (1);                   //失敗

}
#endif


/**
  ******************************************************************************
    @brief  PSoCブートローダ用 UARTコマンド
                PSoCファーム更新モード移行（ リセット＋0x38コマンド）
    @retval 0 = 成功 0以外失敗
    @note   呼び出し元
    @note
  ******************************************************************************
*/
static int PSocCommand_0x38(void){

    //BLEリセット
    g_external_irq2.p_api->disable(g_external_irq2.p_ctrl);                          //IRQ2ディセーブル

    ble_reset();

    tx_thread_sleep (60);     //600msディレイ   500ms NG、600ms以上OK

    //ブートローダーモードへのコマンドをSynergyが送る

    //ブートローダーモードへのコマンド

    pPSoCUartTx->soh = 0x01;        //SOH
    pPSoCUartTx->command = 0x38;
    pPSoCUartTx->data_length = 0x0000;

    PSoC_CRC = CyBtldr_ComputeChecksum(&pPSoCUartTx->soh, pPSoCUartTx->data_length+4);      //CRC計算

    pPSoCUartTx->data[0] = 0xA0;        //CRC16
    pPSoCUartTx->data[1] = 0x09;        //CRC16 0xA009
    pPSoCUartTx->data[2] = 0x17;        //ETB /0x17

    if(0 == PSoC_ldr_TxRx())    //PSoCブートローダ用 UART送受信開始
    {
        //応答データ8Byte
        if(fPSoCCompleteRx == 1){
            if((pPSoCUartRx->status == 0x00) && (pPSoCUartRx->data_length == 8))//#define CYRET_SUCCESS           0x00
            {
                memcpy((uint8_t *)&PSoC_Si_ID, &pPSoCUartRx->data[0],4);            //シリコンID
                PSoC_Si_Rev = pPSoCUartRx->data[4];                                     //シリコンリビジョン
                memcpy((uint8_t *)&PSoC_BtLdrVer, &pPSoCUartRx->data[5], 3);        //ブートローダーバージョン 中身は３Byte

                return (0);           //成功
            }
        }
    }
    return (1);               //失敗
}

/**
  ******************************************************************************
    @brief  PSoCブートローダ用 UARTコマンド
                0x37コマンド  PSoCファーム更新 データブロック
    @param  pData       データ列へのポインタ
    @retval 0 = 成功 0以外失敗
    @note   呼び出し元
    @note
    @note   列書き込み(256Byte)は0x37コマンド （データ53Byte×4）＋0x39コマンド（ アレイID＋列番号2Byte+データ44Byte）で1セット
  ******************************************************************************
*/
static int PSocCommand_0x37(char *pData){

    pPSoCUartTx->soh = 0x01;        //SOH

    pPSoCUartTx->command = 0x37;                                    //データのみ
    pPSoCUartTx->data_length = 53;      //データ長
    memcpy(&pPSoCUartTx->data[0] ,pData, pPSoCUartTx->data_length);     //データコピー data2から連続してデータが入っている

    PSoC_CRC = CyBtldr_ComputeChecksum(&pPSoCUartTx->soh, pPSoCUartTx->data_length+4);      //CRC計算
    pPSoCUartTx->data[pPSoCUartTx->data_length] = (uint8_t)(PSoC_CRC&0x00FF);                   //CRC16 LSB
    pPSoCUartTx->data[pPSoCUartTx->data_length + 1] = (uint8_t)((PSoC_CRC>>8)&0x00FF);      //CRC16 MSB
    pPSoCUartTx->data[pPSoCUartTx->data_length + 2] = 0x17;         //ETB /0x17

    if(0 == PSoC_ldr_TxRx())    //PSoCブートローダ用 UART送受信開始
    {
        //応答データ0Byte
        if(fPSoCCompleteRx == 1){
            return (0);           //成功
        }
    }
    return (1);               //失敗

}

/**
  ******************************************************************************
    @brief  PSoCブートローダ用 UARTコマンド
                0x39コマンド PSoCファーム更新 データブロック
    @param  array       フラッシュアレイ番号
    @param  column  データ列番号
    @param  pData       データ列へのポインタ
    @retval 0 = 成功 0以外失敗
    @note   呼び出し元
    @note
    @note   列書き込み(256Byte)は0x37コマンド （データ53Byte×4）＋0x39コマンド（ アレイID＋列番号2Byte+データ44Byte）で1セット
  ******************************************************************************
*/
static int PSocCommand_0x39(uint8_t array, uint16_t column, char *pData)
{

    uint16_t  col = (column > 511) ? (uint16_t)(column - 512) : column;

    pPSoCUartTx->soh = 0x01;        //SOH

    pPSoCUartTx->command = 0x39;                                                //データ＋Program Row
    pPSoCUartTx->data_length = 3+44;                //データ長
    pPSoCUartTx->data[0] = array;       //フラッシュアレイ番号
    pPSoCUartTx->data[1] = (uint8_t)(col&0xFF);          //データ列番号下位
    pPSoCUartTx->data[2] = (uint8_t)((col>>8)&0xFF);     //データ列番号上位
    memcpy(&pPSoCUartTx->data[3] ,pData, 44);               //データコピー

    PSoC_CRC = CyBtldr_ComputeChecksum(&pPSoCUartTx->soh, pPSoCUartTx->data_length+4);      //CRC計算
    pPSoCUartTx->data[pPSoCUartTx->data_length] = (uint8_t)(PSoC_CRC&0x00FF);                   //CRC16 LSB
    pPSoCUartTx->data[pPSoCUartTx->data_length + 1] = (uint8_t)((PSoC_CRC>>8)&0x00FF);      //CRC16 MSB
    pPSoCUartTx->data[pPSoCUartTx->data_length + 2] = 0x17;         //ETB /0x17

    if (0 == PSoC_ldr_TxRx())    //PSoCブートローダ用 UART送受信開始
    {
        //応答データ0Byte
        if(fPSoCCompleteRx == 1){
            return (0);               //成功
        }
    }
    return (1);                   //失敗

}

/**
  ******************************************************************************
    @brief  PSoCブートローダ用 UARTコマンド
                列のチェックサム取得 0x3A
    @param  array       フラッシュアレイ番号
    @param  column  データ列番号
    @retval 0 = 成功 0以外失敗
    @note   グローバル変数使用   @see    PSoCCheckSum
    @note   呼び出し元
    @note
  ******************************************************************************
*/
static int PSocCommand_0x3A(uint8_t array, uint16_t column)
{

    uint16_t  col = (column > 511) ? (uint16_t)(column - 512) : column;

    pPSoCUartTx->soh = 0x01;        //SOH
    pPSoCUartTx->command = 0x3A;
    pPSoCUartTx->data_length = 0x0003;
    pPSoCUartTx->data[0] = array;       //フラッシュアレイ番号
    pPSoCUartTx->data[1] = (uint8_t)(col&0xFF);          //データ列番号下位
    pPSoCUartTx->data[2] = (uint8_t)((col>>8)&0xFF);     //データ列番号上位
    PSoC_CRC = CyBtldr_ComputeChecksum(&pPSoCUartTx->soh, pPSoCUartTx->data_length+4);      //CRC計算
    pPSoCUartTx->data[pPSoCUartTx->data_length] = (uint8_t)(PSoC_CRC&0x00FF);               //CRC LSB
    pPSoCUartTx->data[pPSoCUartTx->data_length + 1] = (uint8_t)((PSoC_CRC>>8)&0x00FF);      //CRC MSB
    pPSoCUartTx->data[pPSoCUartTx->data_length + 2] = 0x17;         //ETB /0x17

    if( 0 == PSoC_ldr_TxRx())    //PSoCブートローダ用 UART送受信開始
    {
        //応答データ1Byte
        if(fPSoCCompleteRx >= 1){
            if(pPSoCUartRx->data_length == 1){
                PSoCCheckSum = pPSoCUartRx->data[0];    //列のチェックサム保存
                return (0);       //成功
            }
        }
    }
    return (1);               //失敗

}

/**
  ******************************************************************************
    @brief  PSoCブートローダ用 UARTコマンド
                ブートローダモード終了 0x3Bコマンド
    @retval 0 = 成功 0以外失敗
    @note   呼び出し元
    @note
  ******************************************************************************
*/
static int PSocCommand_0x3B(void){

    pPSoCUartTx->soh = 0x01;        //SOH
    pPSoCUartTx->command = 0x3B;        //ブートローダモード終了
    pPSoCUartTx->data_length = 0x0000;
    pPSoCUartTx->data[0] = 0x4F;        //CRC LSB
    pPSoCUartTx->data[1] = 0x6D;        //CRC MSB
    pPSoCUartTx->data[2] = 0x17;        //ETB /0x17

    if(0== PSoC_ldr_TxRx())    //PSoCブートローダ用 UART送受信開始
    {
        //0x3Bコマンドは応答無し
        return (0);       //成功
    }
    return(0);
}


#endif

/***** end of file "programPSoC.c" *****/
