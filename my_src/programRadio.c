/**
 * @brief   無線モジュールファームウェア更新処理
 * @file   programRadio.c
 *
 * @date    2020/01/28
 * @author  t.saito
 *
 * 無線モジュールコマンド
 *  0x70:        検査モード移行
 *  0xD0:       フラッシュ書き換えモードに移行(ブートスワップしてブートローダーで起動するので戻ってこない)
 *  0xD1:       フラッシュ書き換えモード終了
 *  0xD2:       マイコン情報取得
 *  0xD3:       フラッシュステータス読み出し(フラッシュメモリのプロテクト及びセキュリティー状態の確認)    未使用
 *  0xD7:       ブート・フラグ情報の取得/スワップ状態の取得
 *  0xDA:       ブロック書き込み(1024Byte一括 EPV)コマンド
 *  0xDC:       ブロックサムベリファイ
 *  0xE0:       フラッシュ書き換えモードに移行（ブートスワップ、リセット無し）検査用  未実装
 *  0xE1:       ブート・フラグの現在値を反転
 *
 *  @note   2020.Aug.21 RFMフラッシュ書き換え中の電源断等でも次回起動時にファーム更新を再開するように修正
 */
#include "_r500_config.h"

#define _PROGRAM_RADIO_C_
#include "programRadio.h"

//#include "hal_data.h"
#include "Globals.h"
#include "General.h"
#include "Sfmem.h"
#include "dataflash.h"
//#include "rf_thread.h"
//#include "rf_thread_entry.h"

#include "r500_defs.h"

//=======================================================================
typedef uint32_t ssp_err_t;
#define SSP_SUCCESS 0



//変数定義
/**
 * 無線モジュールのフラッシュメモリのセクタサイズ
 * RL78 32KB
 */
static const uint32_t SectorSize[32]={
     1024,  1024,
     1024,  1024,
     1024,  1024,
     1024,  1024,
     1024,  1024,
     1024,  1024,
     1024,  1024,
     1024,  1024,
     1024,  1024,
     1024,  1024,
     1024,  1024,
     1024,  1024,
     1024,  1024,
     1024,  1024,
     1024,  1024,
     1024,  1024,
};



static bool sector_blank[32];       ///<ブロックのブランク情報（書き込み済み情報） //true=未書き込み、false=書き込み済み
static bool sector_verify[32];      ///<ブロックのベリファイ情報 //true=OK、false=NG

static bool boot_curent;            ///< 現在のブート領域  false=0:ブート・クラスタ0 ,true=1:ブート・クラスタ1
static bool boot_reset;             ///< リセット後のブート領域  false=0:ブート・クラスタ0 ,true=1:ブート・クラスタ1

#if 0
//0xD3コマンド用
//未使用
static bool boot_protect;  ///<ブート領域書き換え禁止フラグ（0：禁止，1：許可）
static bool block_protect; ///<ブロック消去禁止フラグ（0：禁止，1：許可）
static bool write_protect; ///<書き込み禁止フラグ（0：禁止，1：許可）
#endif

static uint32_t SectorSum[32];      ///<ブロックのチェックサム(T&D独自)
static int fRfmCompleteRx;      ///< 無線モジュール UART受信完了(無線モジュールファーム書き換え用)


///rfm_cmd_0xD2()で使用するファームウェアリビジョン等の構造定義
typedef struct st_rfm_loader{
    uint16_t RomRevision;       ///< ROMリビジョン
    uint16_t batt_level;        ///< バッテリレベル
    uint16_t LoaderRevision;    ///< ブートローダーリビジョン
    char BuildDate[12];
} /*__packed*/ rfm_loader_t; //TODO gcc packed

static rfm_loader_t loader_ver; ///< rfm_cmd_0xD2()で使用するファームウェアリビジョン等



//プロトタイプ
//void rfm_fw_WriteAll(void);
static void block_writeSwap(uint32_t startBlock, uint32_t endBlock);

static int rfm_cmd_0x70(void);
static int rfm_cmd_0xD2(void);
#if 0
static int rfm_cmd_0xD3(void);        //未使用
#endif
static int rfm_cmd_StartFlashMode(void);
static int rfm_cmd_EndFlashMode(void);
static int rfm_cmd_0xE0(void);
//static int rfm_cmd_SwapBoot(void);

static int rfm_cmd_SumVerify(uint32_t blockNo);
static int rfm_cmd_SumVerifySwap(uint32_t blockNo);
//static int rfm_cmd_GetBootFlag(void);
#if 0
static int rfm_send_break(uint32_t wt);
static int rfm_cmd_0x3F(void);
#endif

static int Rfm_loader_TxRx(void);

static uint32_t calcSectorSum(char *buf, uint32_t length);




/**
 * @brief   RFMファームウェア更新処理
 *          一括書き込み（ブートスワップ対応）
 */
 void rfm_program_FW(void)
{

    int RfmSequence = 0;   //RFMファーム更新のシーケンス番号
    int retry = 0;                   //リトライカウンタ
    uint32_t startblock = RFM_FLASH_START_BLOCK;        //RFM フラッシュの開始ブロック
    uint32_t endblock = RFM_FLASH_END_BLOCK;         //RFM フラッシュの最終ブロック
    uint32_t CurrentBlock;
    ssp_err_t err;
    int err_cnt = 0;
    uint32_t eep_buf;       //データフラッシュ書き込み用

    Printf("\r\n Start Update Radio FW \r\n");

    for (CurrentBlock = startblock; CurrentBlock < endblock + 1; CurrentBlock++)
    {
        sector_blank[CurrentBlock] = true;
        sector_verify[CurrentBlock] = true;
    }
    //ペリフェラル再設定
    /*TODO HW
    g_sf_comms5.p_api->lock(g_sf_comms5.p_ctrl, SF_COMMS_LOCK_ALL, TX_NO_WAIT);    //comm5ロック
    g_sf_comms5.p_api->close(g_sf_comms5.p_ctrl);       //一旦UART5を閉じる
    sf_comms_init5();     //UART5初期化 関数内部でopenしている

    RF_RESET_INACTIVE();
    RF_CS_ACTIVE();
    TODO*/
    tx_thread_sleep (10);        //100ms


    //ファーム書き換え開始
/*TODO
    fRadioFwUpExec = 1;      //RFMファームウェア更新実行中（RFMブートローダ動作）
    if(FLASH_RESULT_BLANK TODO == CheckBlank_data_flash(DATA_FLASH_ADR_RADIO_WRITE_MODE,4))//ブランクでここに来ることは無い
 //        if(0x0000001 != *(uint32_t *)DATA_FLASH_ADR_RADIO_WRITE_MODE)    //RFM書き込み中フラグチェック
    {
        eep_buf = 0x0000001;
        flash_hp_open();
        write_data_flash((uint32_t)&eep_buf, DATA_FLASH_ADR_RADIO_WRITE_MODE,  4);   //RFM書き込み中フラグ セット
        flash_hp_close();
    }
TODO */
    for(;;)
    {
        switch(RfmSequence){

            case 0:
                //ファームウェアバージョン読み込み
                Printf(" #0:Read FW Revision:");
                err = rfm_cmd_0xD2();
                if(err == SSP_SUCCESS){
                    Printf(" Success\r\n");
                    Printf("  ROM Revision: %04X\r\n", loader_ver.RomRevision);
                    Printf("  Battery Level: %d\r\n", loader_ver.batt_level );
                    Printf("  Loader Revision: %04X\r\n",loader_ver.LoaderRevision);
                    Printf("  Build Date: %.12s \r\n", loader_ver.BuildDate);
                    RfmSequence++;
                }
                else{
                    //0xD2が通らない場合 ブートローダ起動？
                    Printf(" Fail! boot loader mode?\r\n");
                    Printf(" Go Program Mode:");
                    err = rfm_cmd_0xE0();
                    if(err == SSP_SUCCESS){
                        Printf(" Success\r\n");
                    }else{
                    Printf(" Fail\r\n");
                    }
                    retry++;
                }
                break;

            case 1:
                //検査モードコマンド (0x70)
                Printf(" #1:Change Test Mode:");
                if(SSP_SUCCESS == rfm_cmd_0x70())
                {
                    Printf(" Success\r\n");
                    tx_thread_sleep (1);        //10ms
                    RfmSequence++;
                }else{
                    Printf(" Fail\r\n");
                    retry++;
                }
                break;

            case 2:
                //フラッシュ書き換えモード移行(0xD0)
                Printf(" #2:Go Program Mode:");
                if(SSP_SUCCESS == rfm_cmd_StartFlashMode())
                {
                    Printf(" Success\r\n");
                    RfmSequence++;
                }
                else{
                    Printf(" Fail\r\n");
                    retry++;
                }

                break;

            case 3:
                //ブートフラグの確認(ブートローダ起動（ブート・スワップ状態）)
                Printf(" #3:Get Boot Flags:");
                tx_thread_sleep (1);        //10ms
                if(SSP_SUCCESS == rfm_cmd_GetBootFlag()){
                    Printf(" Success\r\n");
                    Printf("  Current Boot: %d\r\n",boot_curent);       //現在のブート領域  0:ブート・クラスタ0 ,1:ブート・クラスタ1
                    Printf("  After Reset Boot: %d\r\n",boot_reset);    //リセット後のブート領域  0:ブート・クラスタ0 ,1:ブート・クラスタ1
                    RfmSequence++;
                }
                else
                {
                    Printf(" Fail\r\n");
                    retry++;
                }
                break;

            case 4:
                //フラッシュデータ送信、消去、書き込み、内部ベリファイ
                Printf(" #4:Program RFM Flash:Block (%d - %d)\r\n",startblock, endblock);

                block_writeSwap(startblock, endblock); /* フラッシュメモリ ブロック書き込み（ブロック範囲指定） */



                RfmSequence++;


                break;

            case 5:
                //サムベリファイ（T&D独自）
                Printf(" #5:RFM Sum Verify:Block (%d - %d)\r\n",startblock, endblock);

                for (CurrentBlock = startblock; CurrentBlock < endblock + 1; CurrentBlock++)
                {
Retry_SumVerify:
                    if (boot_curent == false)//現在のブート領域  false:ブート・クラスタ0 ,true:ブート・クラスタ1
                    {
                        err = rfm_cmd_SumVerifySwap(CurrentBlock); // SUMベリファイコマンド(ブートスワップ)
                    }
                    else //ブート・クラスタ0
                    {
                        err = rfm_cmd_SumVerify(CurrentBlock); // SUMベリファイコマンド
                    }
                    if(err != SSP_SUCCESS){
                        //コマンド失敗
                        retry++;
                        goto Retry_SumVerify;
                    }
                }

                //サムベリファイ結果確認
                for (CurrentBlock = startblock; CurrentBlock < endblock + 1; CurrentBlock++)
                {
                   if(false == sector_blank[CurrentBlock]){         //書き込み済みセクタの
                        if(false == sector_verify[CurrentBlock]){   //SUMベリファイ結果
                            err_cnt++;
                        }
                    }
                }
                if(4 >= err_cnt )        //ブートスワップがあるので結果は4以下
                {
                    Printf(" Sum Verify OK\r\n");
                    RfmSequence++;
                }
                else        //SUMベリファイエラーがあった
                {
                    Printf(" Error!! Sum Verify!!\r\n");
                    RfmSequence = 4;        //書き込みからやり直し
                    retry++;
                }
                break;

            case 6:
                //ブートスワップ(0xE1)
                if(boot_reset == true){
                Printf(" #6:Set Boot Swap:");
                if(SSP_SUCCESS == rfm_cmd_SwapBoot())
                {
                    Printf(" Success\r\n");
                    RfmSequence++;
                }
                else{
                    Printf(" Fail\r\n");
                    retry++;
                }
                }else{
                    RfmSequence++;
                }
                break;

            case 7:
                //フラッシュ書き換えモード終了(0xD1)
                Printf(" #7:End RFM Program mode:");
                if(SSP_SUCCESS == rfm_cmd_EndFlashMode()){
                    Printf(" Success\r\n");
//TODO                    flash_hp_open();
//TODO                    eraseBlock_data_flash(FLASH_UPDATE_EXEC,1);//データフラッシュ64Byteクリア RFM FW更新実行フラグ、RFM書き込み中フラグ クリア
//TODO                    flash_hp_close();
                    RfmSequence++;

                }
                else{
                    retry++;
                    Printf(" Fail\r\n");
                }
                break;

        }//switch

        if(RfmSequence > 7){
            break;      //更新完了 ループを抜ける
        }
        if(retry > 1000){       //デバッグ用
            while(1);        //とりあえず
//          break;      //ループを抜ける
        }
        tx_thread_sleep (1);        //10ms
    }//for

    if(RfmSequence > 7){
        Printf("\r\n ### Update RFM FW Complete! ###\r\n");
        fRadioFwUpExec = 0;         //RFM FW更新実行中フラグクリア
        fRadioUpdateMode = 0;       //RFM FW更新モード終了
//        rfmup_mode = false;       //フラッシュ書き換えモードフラグ true = ON, false = OFF

        //RFMをリセットする
        Printf(" Reset Radio module! \r\n\r\n");
/* TODO
        RF_CS_ACTIVE();                                     // 無線モジュール アクティブ

        RF_RESET_ACTIVE();
        tx_thread_sleep (2);        //20ms
        RF_RESET_INACTIVE();
        RF_CS_ACTIVE();
        tx_thread_sleep (1);        //10ms
TODO */
    }
}



 /**
  * フラッシュメモリ ブロック書き込み（ブロック範囲指定）ブートスワップ対応
  * @note  必ず4～書き込む
  * @param startBlock
  * @param endBlock
  */
static void block_writeSwap(uint32_t startBlock, uint32_t endBlock)
{

    uint32_t CurrentBlock;
    ssp_err_t err;

    uint32_t FlashAddress = *(uint32_t *)FLASH_MAN2_SRC_ADR; //ファームデータのキャッシュされているアドレス（SPI FLASH） SPIフラッシュ時0x00100000固定値

    uint32_t WriteBlock;


    for (CurrentBlock = startBlock; CurrentBlock < endBlock+1; CurrentBlock++)
    {

        if (((CurrentBlock > 3) && (CurrentBlock < 8)) || ((CurrentBlock > 28) && (CurrentBlock < 31)))  //4～7,29～30はブートローダエリアなのでスキップ
        {
            ;
        }
        else
        {

            WriteBlock = CurrentBlock;//書き込みブロック

            //通信開始

            pRfmUartTx->header = 0x01;   //SOH
            pRfmUartTx->command = 0xDA;   //コマンド 1024Byteブロック書き込み EPVコマンド
            pRfmUartTx->length = 0x0400;   //LENGTH     1024Byte

            if (CurrentBlock < 4)  //HEXファイルの0～3ブロック
            {
                WriteBlock = (uint32_t)(CurrentBlock + 4);   //サブコマンド Target_sector;   //DATA0 ブロック番号
/*
                //現在のブート領域  false:ブート・クラスタ0 ,true:ブート・クラスタ1
                if (boot_curent == true)//4～7ブロックへ書き込む
                {
                    WriteBlock = (uint32_t)(CurrentBlock + 4);   //サブコマンド Target_sector;   //DATA0 ブロック番号
                }
                else//そのまま0～3ブロックへ書き込む
                {
                    WriteBlock = CurrentBlock;   //サブコマンド Target_sector;   //DATA0 セクタ番号
                }
*/
            }

            Printf("  Block(%d) -> Block(%d)\r\n",CurrentBlock, WriteBlock);
            pRfmUartTx->subcommand = (uint8_t)WriteBlock;    //書き込みブロック指定（ブートスワップ対応）

            //1024Byteフラッシュから読み出し
            serial_flash_multbyte_read((uint32_t)(char *)(uint32_t)(FlashAddress + (CurrentBlock*1024)), SectorSize[CurrentBlock], (char *)&pRfmUartTx->data);

            SectorSum[CurrentBlock] = calcSectorSum((char *)&pRfmUartTx->data,SectorSize[CurrentBlock]);       //セクタのチェックサム計算保存（Verifyで使用する）


            calc_checksum(&pRfmUartTx->header);     //SOHフレームのチェックサム計算、付加

            err = Rfm_loader_TxRx();      //RFMへコマンド送信、受信

            if (err == SSP_SUCCESS)
            {
                if (pRfmUartRx->data[0] == 0x00) //エラーコード取得
                {
                    sector_blank[CurrentBlock] = false;        //書き込み済み
                    sector_verify[CurrentBlock] = false;        //ベリファクィ未チェック
                }
                else
                {
                    switch(pRfmUartRx->data[0]){
                        case 0xE2:
                            Printf("illegal data size\r\n");        //データサイズ違い
                            break;
                        case 0x00:
                            Printf("FSL_OK\r\n"); //正常終了
                            break;
                        case 0x05:
                            Printf("FSL_ERR_PARAMETER\r\n"); //パラメータ・エラー
                            break;
                        case 0x10:
                            Printf("FSL_ERR_PROTECTION\r\n"); //プロテクト・エラー
                            break;
                        case 0x1A:
                            Printf("FSL_ERR_ERASE\r\n"); //消去エラー
                            break;
                        case 0x1B:
                            Printf("FSL_ERR_IVERIFY\r\n"); //ベリファイ（内部ベリファイ）エラー
                            break;
                        case 0x1C:
                            Printf("FSL_ERR_WRITE\r\n"); //書き込みエラー
                            break;
                        case 0x1F:
                            Printf("FSL_ERR_FLOW\r\n"); //フローエラー
                            break;
                    }

                }

            }else{
//TODO               __NOP();		//デバッグ用
            }
        }
    }
}

/**
 * @brief   検査モード移行コマンド 0x70
 * @return  0=成功    0以外= 失敗
 */
static int rfm_cmd_0x70(void)
{

    ssp_err_t err;

    pRfmUartTx->header = 0x01;   //SOH
    pRfmUartTx->command = 0x70;   //コマンド
    pRfmUartTx->subcommand = 0x00;   //サブコマンド
    pRfmUartTx->length = 0x0004;   //LENGTH 4Byte
    pRfmUartTx->data[0] = 0x00;   //DATA0
    pRfmUartTx->data[1] = 0x06;   //DATA1
    pRfmUartTx->data[2] = 0x00;   //DATA2
    pRfmUartTx->data[3] = 0x00;   //DATA3

    calc_checksum(&pRfmUartTx->header);     //チェックサム計算、付加

    err = Rfm_loader_TxRx();      //RFMへコマンド送信、受信

    //ACK時の処理
    if (err == SSP_SUCCESS) //成功
    {
        if(pRfmUartRx->subcommand != 0x06){
            err = 1;  //失敗
        }
    }

    return (err);
}


/**
 * @brief   マイコン情報取得
 *          0xD2 コマンド
 * @return  0=成功    0以外= 失敗
 */
static int rfm_cmd_0xD2(void){
    ssp_err_t err;

    pRfmUartTx->header = 0x01;   //SOH
    pRfmUartTx->command = 0xD2;   //コマンド
    pRfmUartTx->subcommand = 0x01;   //サブコマンド 1= DATE付き
    pRfmUartTx->length = 0x0004;   //LENGTH 4Byte
    pRfmUartTx->data[0] = 0x00;   //DATA0
    pRfmUartTx->data[1] = 0x00;   //DATA1
    pRfmUartTx->data[2] = 0x00;   //DATA2
    pRfmUartTx->data[3] = 0x00;   //DATA3

    calc_checksum(&pRfmUartTx->header);     //チェックサム計算、付加

    err = Rfm_loader_TxRx();      //RFMへコマンド送信、受信

    if (err == SSP_SUCCESS)
    {
        if(pRfmUartRx->subcommand == 0x06)  //ACK応答
        {
            loader_ver.RomRevision = *(uint16_t *)&pRfmUartRx->data[0];
            loader_ver.batt_level = *(uint16_t *)&pRfmUartRx->data[2];
            loader_ver.LoaderRevision = *(uint16_t *)&pRfmUartRx->data[4];

            if(pRfmUartTx->subcommand == 0x01){
                memcpy(loader_ver.BuildDate, &pRfmUartRx->data[6],12); //11文字＋null
            }
        }
        else{
            err = 1;  //失敗
        }
    }
    return(err);
}

#if 0
//未使用
/**
 * @brief   フラッシュステータス読み出し(フラッシュメモリのプロテクト及びセキュリティー状態の確認) 0xD3
 * @return  0=成功    0以外= 失敗
 */
static int rfm_cmd_0xD3(void){
    ssp_err_t err;   //受信ステータス

    pRfmUartTx->header = 0x01;   //SOH
    pRfmUartTx->command = 0xD3;   //コマンド
    pRfmUartTx->subcommand = 0x00;   //サブコマンド
    pRfmUartTx->length = 0x0004;   //LENGTH 4Byte
    pRfmUartTx->data[0] = 0x00;   //DATA0
    pRfmUartTx->data[1] = 0x00;   //DATA1
    pRfmUartTx->data[2] = 0x00;   //DATA2
    pRfmUartTx->data[3] = 0x00;   //DATA3

    calc_checksum(&pRfmUartTx->header);     //チェックサム計算、付加

    err = Rfm_loader_TxRx();      //RFMへコマンド送信、受信

    if (err == SSP_SUCCESS)
    {
        //ACK時の処理
        if (pRfmUartRx->subcommand == 0x06){
            if (pRfmUartRx->data[0] == 0x00) //正常コマンド終了
            {
                //ブロックプロテクト状態
                boot_protect = ((pRfmUartRx->data[1] & 0x02) == 0x02) ? true : false ;//bit1 ブート領域書き換え禁止フラグ（0：禁止，1：許可）
                block_protect = ((pRfmUartRx->data[1] & 0x04) == 0x04) ? true : false;//bit2 ブロック消去禁止フラグ（0：禁止，1：許可）
                write_protect = ((pRfmUartRx->data[1] & 0x10) == 0x10) ? true : false;//bit4 書き込み禁止フラグ（0：禁止，1：許可）

            }
            else{
                err = 1;  //失敗
            }
        }
        else{
            err =1;  //失敗
        }
    }
    return (err);
}
#endif

/**
 * フラッシュ書き換えモード開始コマンド 0xD0
 * 再起動、ブートスワップあり
 * @return  0=成功    0以外= 失敗
 */
static int rfm_cmd_StartFlashMode(void)
{

    ssp_err_t err;

    pRfmUartTx->header = 0x01;   //SOH
    pRfmUartTx->command = 0xD0;   //コマンド
    pRfmUartTx->subcommand = 0x00;   //サブコマンド
    pRfmUartTx->length = 0x0004;   //LENGTH 4Byte
    pRfmUartTx->data[0] = 0x00;   //DATA0
    pRfmUartTx->data[1] = 0x04;   //DATA1
    pRfmUartTx->data[2] = 0x03;   //DATA2
    pRfmUartTx->data[3] = 0x09;   //DATA3

    calc_checksum(&pRfmUartTx->header);     //チェックサム計算、付加

    err = Rfm_loader_TxRx();      //RFMへコマンド送信、受信

    if(err == SSP_SUCCESS){
        //ACK時の処理
        if (pRfmUartRx->subcommand == 0x06){
            if (pRfmUartRx->data[0] != 0x00){ //0x00=成功
                err = 1;  //失敗
            }
        }else{
            err = 1;  //失敗
        }
    }
    return (err);

}


/**
 * フラッシュ書き換えモード終了 0xD1
 * @return  0=成功    0以外= 失敗
 */
static int rfm_cmd_EndFlashMode(void)
{

    ssp_err_t err;

    pRfmUartTx->header = 0x01;   //SOH
    pRfmUartTx->command = 0xD1;   //コマンド
    pRfmUartTx->subcommand = 0x00;   //サブコマンド
    pRfmUartTx->length = 0x0004;   //LENGTH 4Byte
    pRfmUartTx->data[0] = 0x00;   //DATA0
    pRfmUartTx->data[1] = 0x00;   //DATA1
    pRfmUartTx->data[2] = 0x00;   //DATA2
    pRfmUartTx->data[3] = 0x00;   //DATA3

    calc_checksum(&pRfmUartTx->header);    //チェックサム計算、付加

    err = Rfm_loader_TxRx();      //RFMへコマンド送信、受信
    if(err == SSP_SUCCESS){
        if(pRfmUartRx->subcommand != 0x06){
            err = 1;  //コマンド失敗
        }
    }
    return (err);
}


/**
 * フラッシュ書き換えモード開始(再起動無し)
 * @return  0=成功    0以外= 失敗
 */
static int rfm_cmd_0xE0(void)
{
    ssp_err_t err;

    pRfmUartTx->header = 0x01;   //SOH
    pRfmUartTx->command = 0xE0;   //コマンド
    pRfmUartTx->subcommand = 0x00;   //サブコマンド
    pRfmUartTx->length = 0x0004;   //LENGTH 4Byte
    pRfmUartTx->data[0] = 0x00;   //DATA0
    pRfmUartTx->data[1] = 0x00;   //DATA1
    pRfmUartTx->data[2] = 0x00;   //DATA2
    pRfmUartTx->data[3] = 0x00;   //DATA3

    calc_checksum(&pRfmUartTx->header);    //チェックサム計算、付加

    err = Rfm_loader_TxRx();      //RFMへコマンド送信、受信

    if(err == SSP_SUCCESS){
        if(pRfmUartRx->subcommand != 0x06){
            err = 1;  //コマンド失敗
        }
    }
    return (err);
}


/**
 * @brief ブートスワップコマンド(0xE1)
 * 次回リセット起動時にブートスワップされる
 * @return  0=成功    0以外= 失敗
 */
int rfm_cmd_SwapBoot(void)
{
    ssp_err_t err;

    pRfmUartTx->header = 0x01;   //SOH
    pRfmUartTx->command = 0xE1;   //コマンド
    pRfmUartTx->subcommand = 0x00;   //サブコマンド
    pRfmUartTx->length = 0x0004;   //LENGTH 4Byte
    pRfmUartTx->data[0] = 0x00;   //DATA0
    pRfmUartTx->data[1] = 0x00;   //DATA1
    pRfmUartTx->data[2] = 0x00;   //DATA2
    pRfmUartTx->data[3] = 0x00;   //DATA3

    calc_checksum(&pRfmUartTx->header);    //チェックサム計算、付加

    err = Rfm_loader_TxRx();      //RFMへコマンド送信、受信

    if(err == SSP_SUCCESS){
        if(pRfmUartRx->subcommand != 0x06){
            err = 1;  //コマンド失敗
        }
    }
    return (err);
}


/**
 * SUMベリファイコマンド（T&D独自）
 * 0xDCコマンド
 * 結果がsector_verify[]に書かれる
 * @param blockNo
 * @return  0=成功    0以外= 失敗
 * @note    SectorSumは予め計算しておくこと
 * @note    このコマンド応答は ACK = ベリファイOK, NAK = ベリファイNG
 *
 */
static int rfm_cmd_SumVerify(uint32_t blockNo)
{

    ssp_err_t err;
    uint32_t Send_SUM_Data = SectorSum[blockNo];        //セクタのSUM読み出し

    pRfmUartTx->header = 0x01;   //SOH
    pRfmUartTx->command = 0xDC;   //コマンド
    pRfmUartTx->subcommand = (uint8_t)blockNo;   //サブコマンド SUMチェックセクタ番号
    pRfmUartTx->length = 0x0004;   //LENGTH 4Byte

    *(uint32_t *)&pRfmUartTx->data[0] = Send_SUM_Data;       //SUM(Add 4Byte データ)
 /*
    pRfmUartTx->data[0] = (uint8_t)(Send_SUM_Data & 0xFF);   //DATA1 SUM(Add 4Byte データ)
    pRfmUartTx->data[1] = (uint8_t)((Send_SUM_Data >> 8) & 0xFF);   //DATA2
    pRfmUartTx->data[2] = (uint8_t)((Send_SUM_Data >> 16) & 0xFF);   //DATA3
    pRfmUartTx->data[3] = (uint8_t)((Send_SUM_Data >> 24) & 0xFF);   //DATA4
*/
    calc_checksum(&pRfmUartTx->header);    //チェックサム計算、付加

    err = Rfm_loader_TxRx();      //RFMへコマンド送信、受信

    //
    if (err == SSP_SUCCESS)
    {
        //応答は ACK = ベリファイOK, NAK = ベリファイNG
        if(pRfmUartRx->subcommand == 0x06){
            sector_verify[blockNo] = true;
        }
        else{
            sector_verify[blockNo] = false;
        }
    }
    return (err);
 }


/**
 * SUMベリファイコマンド（T&D独自）ブートスワップ時用
 * 0xDCコマンド
 * 結果がsector_verify[]に書かれる
 * @param blockNo
 * @return  0=成功    0以外= 失敗
 * @note    このコマンド応答は ACK = ベリファイOK, NAK = ベリファイNG
 */
static int rfm_cmd_SumVerifySwap(uint32_t blockNo)
{
    uint32_t block_swap_no;

    int err;
    switch(blockNo)
    {
        case 0:
        case 1:
        case 2:
        case 3:
            block_swap_no = (uint32_t)(blockNo + 4);
            break;
        case 4:
        case 5:
        case 6:
        case 7:
            block_swap_no = (uint32_t)(blockNo - 4);
            break;
        default:
            block_swap_no = blockNo;
            break;
    }

    uint32_t Send_SUM_Data = SectorSum[block_swap_no];  //セクタのSUM読み出し

    pRfmUartTx->header = 0x01;   //SOH
    pRfmUartTx->command = 0xDC;   //コマンド
    pRfmUartTx->subcommand = (uint8_t)blockNo;   //サブコマンド SUMチェックセクタ番号
    pRfmUartTx->length = 0x0004;   //LENGTH 4Byte

    *(uint32_t *)&pRfmUartTx->data[0] = Send_SUM_Data;       //SUM(Add 4Byte データ)

    calc_checksum(&pRfmUartTx->header);     //チェックサム計算、付加

    err = Rfm_loader_TxRx();      //RFMへコマンド送信、受信

    if (err == SSP_SUCCESS)
    {
        //応答は ACK = ベリファイOK, NAK = ベリファイNG
        if(pRfmUartRx->subcommand == 0x06){
            sector_verify[blockNo] = true;
        }
        else{
            sector_verify[blockNo] = false;
        }
    }
    return (err);
 }


/**
 * @brief   ブートフラグの確認
 * 0xD7コマンド
 *  ブートフラグがboot_curentにセットされる
 */
int rfm_cmd_GetBootFlag(void)
{
    ssp_err_t err;   //受信ステータス

    pRfmUartTx->header = 0x01;   //SOH
    pRfmUartTx->command = 0xD7;   //コマンド
    pRfmUartTx->subcommand = 0x00;   //サブコマンド
    pRfmUartTx->length = 0x0004;   //LENGTH 4Byte
    pRfmUartTx->data[0] = 0x00;   //DATA0
    pRfmUartTx->data[1] = 0x00;   //DATA1
    pRfmUartTx->data[2] = 0x00;   //DATA2
    pRfmUartTx->data[3] = 0x00;   //DATA3

    calc_checksum(&pRfmUartTx->header);     //チェックサム計算、付加

    err = Rfm_loader_TxRx();      //RFMへコマンド送信、受信

    if (err == SSP_SUCCESS)
    {
        //ACK時の処理
        if(pRfmUartRx->subcommand == 0x06){
            if (pRfmUartRx->data[0] == 0x00) //正常コマンド終了
            {
                //リセット後のブート領域 （FSL_GetBootFlagの結果）

     //           リセット後のブート領域：　0=ブートクラスタ0、 1=ブートクラスタ1
                boot_reset = (pRfmUartRx->data[1] == 0x00) ?  false : true;
            }
            else{
                boot_curent = false;
            }

            if (pRfmUartRx->data[2] == 0x00) //正常コマンド終了
            {
                //スワップ状態：現在のブート領域（FSL_GetSwapStateの結果）：0=ブートクラスタ0、 1=ブートクラスタ1
                boot_curent = (pRfmUartRx->data[3] == 0x00) ? false : true;
            }
            else{
                boot_reset = false;
            }

        }else{
            err = 1;    //失敗
        }
    }
    return(err);

}


#if 0
/**
 * @brief 無線モジュールへブレーク送信
 * @param wt    待ち時間（OS Ｔｉｃｋ単位）
 * @return  SSP_SUCCESS=成功
 */
static int rfm_send_break(uint32_t wt){

   uint8_t  data = 0x00;
   ssp_err_t err;

   err = g_sf_comms5.p_api->lock(g_sf_comms5.p_ctrl, SF_COMMS_LOCK_ALL, wt);    //comm5ロック
    if(err == SSP_SUCCESS){
       err = g_sf_comms5.p_api->write(g_sf_comms5.p_ctrl, (uint8_t *)&data, 1, wt);

       g_sf_comms5.p_api->unlock(g_sf_comms5.p_ctrl, SF_COMMS_LOCK_ALL);    //comm5ロック解除
    }
   return(err);
}


/**
 * @brief   間欠受信停止
 * @return  0=成功    0以外= 失敗
 */
static int rfm_cmd_0x3F(void)
{

    ssp_err_t err;

    pRfmUartTx->header = 0x01;   //SOH
    pRfmUartTx->command = 0x3F;   //コマンド
    pRfmUartTx->subcommand = 0x00;   //サブコマンド
    pRfmUartTx->length = 0x0004;   //LENGTH 4Byte
    pRfmUartTx->data[0] = 0x01;   //DATA0
    pRfmUartTx->data[1] = 0x00;   //DATA1
    pRfmUartTx->data[2] = 0x00;   //DATA2
    pRfmUartTx->data[3] = 0x00;   //DATA3

    calc_checksum(&pRfmUartTx->header);     //チェックサム計算、付加

    err = Rfm_loader_TxRx();      //RFMへコマンド送信、受信

    //ACK時の処理
    if (err == SSP_SUCCESS) //成功
    {
        if(pRfmUartRx->subcommand != 0x06){
            err = 1;  //失敗
        }
    }

    return (err);
}
#endif


/**
  ******************************************************************************
    @brief  無線モジュール ブートローダ用 UART送受信開始(ブレークあり)
    @note   呼び出し元
    @note   無線モジュールは基本的にブレーク無しでも通信できる
  ******************************************************************************
*/
static int Rfm_loader_TxRx(void)
{
    ssp_err_t err;
    uint8_t  data = 0x00;

/*TODO HW
    err = g_sf_comms5.p_api->lock(g_sf_comms5.p_ctrl, SF_COMMS_LOCK_ALL, TX_WAIT_FOREVER);    //comm5ロック

    err = g_sf_comms5.p_api->write(g_sf_comms5.p_ctrl, (uint8_t *)&data, 1, TX_WAIT_FOREVER);    //NULLブレーク送信
*/
    tx_thread_sleep (2);        //20ms

    fRfmCompleteRx = 0;
    pRfmUartRx->header = 0xFF;

    // 1byte 520us  wait:10ms   19200bps

/*TODO HW
    err = g_sf_comms5.p_api->write(g_sf_comms5.p_ctrl, (uint8_t*)&pRfmUartTx->header, (uint32_t)(pRfmUartTx->length+ 7), TX_WAIT_FOREVER);
    if(err == SSP_SUCCESS) {
Retry_Rx:
        tx_thread_sleep (1);
        err = g_sf_comms5.p_api->read(g_sf_comms5.p_ctrl, (uint8_t*)&pRfmUartRx->header, 1, 300);     //1Byte読み出し

        if(err == SSP_SUCCESS) {
            if(0x01 != pRfmUartRx->header)      //SOH?
            {
                goto Retry_Rx;
            }
            err = g_sf_comms5.p_api->read(g_sf_comms5.p_ctrl, (uint8_t*)&pRfmUartRx->command, 4, 100);       //Lengthまで4Byte読み出し(受信は完了している)
            if(err == SSP_SUCCESS) {

                err = g_sf_comms5.p_api->read(g_sf_comms5.p_ctrl, (uint8_t*)&pRfmUartRx->data[0], (uint32_t)(pRfmUartRx->length +2), 500);       //残りデータ+SUM読み出し(受信は完了している)
                if(err == SSP_SUCCESS) {
                 fRfmCompleteRx = 1;

                }else{
                    __NOP();//デバッグ用
                }
            }
        }else if(err == SSP_ERR_BREAK_DETECT){
            goto Retry_Rx;      //リセット起動時 ポートがLOなので0xFFが出る
        }else{
            __NOP();//デバッグ用
        }
    }else{
        __NOP();//デバッグ用
    }
    g_sf_comms5.p_api->unlock(g_sf_comms5.p_ctrl, SF_COMMS_LOCK_ALL);    //comm5ロック解除
*/
    return (err);
}


/**
  ******************************************************************************
    @brief  RFMのFW更新が途中で終わっていないかチェック、更新モードセット
    @note   呼び出し元
    @note	2020.Sep.15 データフラッシュのブランクチェックを厳密にした
  ******************************************************************************
*/
#define FLASH_RESULT_NOT_BLANK 42 //TODO
void checkProgramRadio(void)
{
/* TODO
    if(FLASH_RESULT_NOT_BLANK == CheckBlank_data_flash(DATA_FLASH_ADR_RADIO_WRITE_MODE,4)){
    if(0x0000001 == *(uint32_t *)DATA_FLASH_ADR_RADIO_WRITE_MODE)    //PSoC書き込み中フラグチェック
    {
            if(FLASH_RESULT_NOT_BLANK == CheckBlank_data_flash(FLASH_UPDATE_EXEC,4)){
        if(0x0000004 == *(uint32_t *)FLASH_UPDATE_EXEC){
           fRadioUpdateMode = 1;    //RFM FW更新モード
//           tx_thread_resume(&rf_thread);     //RFスレッド起動（他スレッド化r他呼ぶ時に必要）
                }
            }
        }
    }
TODO */
}

/**
 * @brief   セクタのチェックサムを計算する（T&D独自）
 * @param buf       計算するバッファへのポインタ
 * @param length    計算するバイト長（通常1024）
 * @return  計算結果
 * @note    SUMは4Byte長
 */
static uint32_t calcSectorSum(char *buf, uint32_t length){
    uint32_t i;
    uint32_t sum = 0;

    for(i = 0; i < length; i++){
      sum += (uint8_t)*buf++;
    }
    return (sum);
}


