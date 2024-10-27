/**
 * @file opt_cmd_thread_entry.c
 * @brief  光通信スレッド
 *
 * ・各コマンドの関数の種類はメッセージキューにて引き渡し
 *  ・コマンドの内容についてはバッファ渡し
 *  ・実行の状態は、フラグによる
 *
 *  ・Uart7を使用 2400bps
 *
 * @note    2019.Dec.26 ビルドワーニング潰し完了
 * @note    2020.01.30  v6 0128 ソースマージ済み
 * @note    2020.Jul.03 ソースファイルの整理
 * @note	2020.Aug.07	0807ソース反映済み
 *
 * @note    opt_cmd_thread スタック    1024Byte
 * @note    SCI FIFO 16Byte
 * @note    comms7 受信キュー（バッファ） 4 × 512 Byte = 2048Byte
 * @note    buffer[]    受信バッファ1 1280 Byte   → 廃止
 * @note    OCOM.rxbuf  受信バッファ2 1286 Byte
 *
 * @todo    buffer廃止（受信バッファを一つにする）  → 廃止
 * @todo    commsドライバのキューの直接チェックをやめる  → 廃止
*/
/*
    SSP_SUCCESS                     = 0,

    SSP_ERR_ASSERTION               = 1,        ///< A critical assertion has failed
    SSP_ERR_INVALID_POINTER         = 2,        ///< Pointer points to invalid memory location
    SSP_ERR_INVALID_ARGUMENT        = 3,        ///< Invalid input parameter
    SSP_ERR_INVALID_CHANNEL         = 4,        ///< Selected channel does not exist
    SSP_ERR_INVALID_MODE            = 5,        ///< Unsupported or incorrect mode
    SSP_ERR_UNSUPPORTED             = 6,        ///< Selected mode not supported by this API
    SSP_ERR_NOT_OPEN                = 7,        ///< Requested channel is not configured or API not open
    SSP_ERR_IN_USE                  = 8,        ///< Channel/peripheral is running/busy
    SSP_ERR_OUT_OF_MEMORY           = 9,        ///< Allocate more memory in the driver's cfg.h
    SSP_ERR_HW_LOCKED               = 10,       ///< Hardware is locked
    SSP_ERR_IRQ_BSP_DISABLED        = 11,       ///< IRQ not enabled in BSP
    SSP_ERR_OVERFLOW                = 12,       ///< Hardware overflow
    SSP_ERR_UNDERFLOW               = 13,       ///< Hardware underflow
    SSP_ERR_ALREADY_OPEN            = 14,       ///< Requested channel is already open in a different configuration
    SSP_ERR_APPROXIMATION           = 15,       ///< Could not set value to exact result
    SSP_ERR_CLAMPED                 = 16,       ///< Value had to be limited for some reason
    SSP_ERR_INVALID_RATE            = 17,       ///< Selected rate could not be met
    SSP_ERR_ABORTED                 = 18,       ///< An operation was aborted
    SSP_ERR_NOT_ENABLED             = 19,       ///< Requested operation is not enabled
    SSP_ERR_TIMEOUT                 = 20,       ///< Timeout error
    SSP_ERR_INVALID_BLOCKS          = 21,       ///< Invalid number of blocks supplied
    SSP_ERR_INVALID_ADDRESS         = 22,       ///< Invalid address supplied
    SSP_ERR_INVALID_SIZE            = 23,       ///< Invalid size/length supplied for operation
    SSP_ERR_WRITE_FAILED            = 24,       ///< Write operation failed
    SSP_ERR_ERASE_FAILED            = 25,       ///< Erase operation failed
    SSP_ERR_INVALID_CALL            = 26,       ///< Invalid function call is made
    SSP_ERR_INVALID_HW_CONDITION    = 27,       ///< Detected hardware is in invalid condition
    SSP_ERR_INVALID_FACTORY_FLASH   = 28,       ///< Factory flash is not available on this MCU
    SSP_ERR_INVALID_FMI_DATA        = 29,       ///< Linked FMI data table is not valid
    SSP_ERR_INVALID_STATE           = 30,       ///< API or command not valid in the current state
    SSP_ERR_NOT_ERASED              = 31,       ///< Erase verification failed
    SSP_ERR_SECTOR_RELEASE_FAILED   = 32,       ///< Sector release failed

*/

#define _OPT_CMD_THREAD_ENTRY_C_

#include "opt_cmd_thread.h"
#include "opt_cmd_thread_entry.h"
#include "General.h"
#include "MyDefine.h"
#include "flag_def.h"
#include "Error.h"

#define SCMD_CHECK      1
#define SCMD_NOCHECK    0

//光通信用バッファ
static def_com OCOM;                   ///< 光通信用バッファ  Opti Comm Globals.hから移動

static int Opt_Sum_Crc;        //受信フレームがSUMの場合0 CRC16の場合1



// local function
static void opticom_initialize(void);                              // 光通信 初期化
static void opticom_uninitialize(void);                            // 光通信 未初期化

static void opticom_plane_send(char *pBuf);                  // 光通信 送信
static uint32_t opticom_recv(int check);                     // 光通信 受信

static uint32_t opticom_send_recv(char *pBuf, int check);    // 光通信 送受信

static void opticom_soh_format(uint8_t s, uint8_t sc, uint16_t len, char *pData);         // 光通信 SOH送信フォーマット設定
//static uint32_t opticom_retry_send(char *pBuf, int check);                          // 光通信 光通信送受信（リトライなし）
static void opticom_break(void);                                   // 光通信 ブレーク送出

#if 0
static void opt_start(void);                                        // 光通信 使用開始  opticom_initialize
static void opt_stop(void);                                         // 光通信 使用終了 opticom_uninitialize
#endif

static uint32_t ir_data_block_read(void);                          // 光通信 データブロック要求

//各コマンド処理
static uint32_t func_ir_64byte_write(void);
static uint32_t func_ir_64byte_read(void);
static uint32_t func_ir_header_read(void);
static uint32_t func_ir_data_download_t(void);
//static uint32_t func_ir_type_read(void);           //未使用 コメントアウトされている
static uint32_t func_ir_simple_command(void);
static uint32_t func_ir_simple_command_with_retry(void);
static uint32_t func_ir_soh_direct(void);
static uint32_t func_ir_regist(void);
#if 0
//未使用関数
static void func_ir_data_block_read(void);
#endif


/**
 * Opt Command Thread entry function
 *
 *
 *
 * @note    2020.Jul.03 各コマンド共通処理を関数外に出した
 */
void opt_cmd_thread_entry(void)
{

    uint32_t   actual_events;
    uint32_t optc_msg[4];
    uint32_t Err;

    pOptUartTx = (comSOH_t *)&OCOM.txbuf.header;           //光通信 SOH構造体ポインタ 送信用
    pOptUartRx = (comSOH_t *)&OCOM.rxbuf.header;           //光通信 SOH構造体ポインタ 受信用

    sf_comms_init7();		// OPT Comm Initial

    while(1){

        tx_queue_receive(&optc_queue, optc_msg, TX_WAIT_FOREVER);    //メッセージキュー待ち

        tx_event_flags_get (&optc_event_flags, 0xffffffff,TX_OR_CLEAR, &actual_events,TX_NO_WAIT);      //光通信 全イベントフラグクリア

 //       Printf("OPT Queue %ld \r\n", optc_msg[0]);

//        opt_start();         // 光通信 使用開始（ 光通信 電源ＯＮ）
        opticom_initialize();         // 光通信 使用開始（ 光通信 電源ＯＮ）
        opticom_break();      // ブレーク送出
        Err = ERR(IR, NOERROR);

        switch(optc_msg[0]){
            case ID_IR_64BYTE_READ:
                Printf("    0. ID_IR_64BYTE_READ \r\n");
                Err = func_ir_64byte_read();
                break;
            case ID_IR_64BYTE_WRITE:
                Printf("    1. ID_IR_64BYTE_WRITE \r\n");
                Err = func_ir_64byte_write();
                break;
            case ID_IR_HEADER_READ:
                Printf("    2. ID_IR_HEADER_READ\n");
                Err = func_ir_header_read();
                break;
            case ID_IR_DATA_DOWNLOAD_T:
                Printf("    3. ID_IR_DATA_DOWNLOAD_T\r\n");
                Err = func_ir_data_download_t();
                break;
            //case ID_IR_TYPE_READ:
            //    Err = func_ir_type_read();
            //    break;
            case ID_IR_SIMPLE_COMMAND:
                Printf("    5. IR_SIMPLE_COMMAND \r\n");
                Err = func_ir_simple_command();
                break;
            case ID_IR_SIMPLE_COMMAND_RETRY:
                Printf("    6. ID_IR_SIMPLE_COMMAND_RETRY \r\n");
                Err = func_ir_simple_command_with_retry();        //光コマンド実行 リトライ無し
                break;

            case ID_IR_REGIST:
                Printf("    7. ID_IR_REGIST \r\n");
                Err = func_ir_regist();
                break;
            case ID_IR_SOH_DIRECT:
                Printf("    8. ID_IR_SOH_DIRECT \r\n");
                Err = func_ir_soh_direct();
                break;
            default:
                Printf("    Unknown command \r\n");
                goto exit_pass;
        }

//        opt_stop();             // 光通信 使用終了（光通信 電源OFF）
        opticom_uninitialize();             // 光通信 使用終了（光通信 電源OFF）
        optc.result = Err;

        tx_event_flags_set (&optc_event_flags, FLG_OPTC_END, TX_OR);    //光通信コマンド終了イベントフラグセット
 exit_pass:
        tx_thread_sleep (1);

    }


}


/**
 * データブロック要求する関数
 * @return  成功:ERR(IR, NOERROR) 失敗:ERR(IR, NORES)
 * @note    ファイル内
 */
static uint32_t ir_data_block_read(void)
{

    int rty_cnt;
    uint32_t Err;

    Err = ERR(IR, NORES);

    opticom_soh_format(0x41, CODE_ACK, 0, (char *)optc.buf);                      // ＮＡＫならブロック再送要求


    for(rty_cnt = 3; rty_cnt > 0; rty_cnt--)
    {
        Err = ERR(IR, NOERROR);

        Err = opticom_send_recv(&OCOM.txbuf.header, SCMD_NOCHECK);        //送受信

        if(Err == ERR(IR, NOERROR)){
            break;
        }

        OCOM.txbuf.subcommand = CODE_NAK;                       // 再送要求

        tx_thread_sleep (5);                              //  wait 50ms

        if(rty_cnt > 1){
            opticom_break();                        // ブレーク送出
        }
    }

    return (Err);

}

#if 0
//cmd_thread_entry.cのローカル関数に移動
/**
 * データ吸い上げ要求する関数
 * @param Cmd       0x48, 0x59, 0x6c command
 * @param DataSize
 * @return          ERR(IR, NOERROR) 失敗:ERR(IR, NORES)
 * @note   T commad
 * @note    2020.Jul.06 6個所あったキュー送信、イベントフラグ取得をopt_command_exec()に共通化
 */
uint32_t IR_header_read(uint8_t Cmd , uint16_t DataSize)
{

//    uint32_t   actual_events;

    optc.Cmd= Cmd;
    optc.DataSize = DataSize;
    optc.cmd_id[0] = ID_IR_HEADER_READ;

    /*
    tx_queue_send(&optc_queue, optc.cmd_id, TX_WAIT_FOREVER);

    tx_event_flags_get (&optc_event_flags, FLG_OPTC_END, TX_OR_CLEAR, &actual_events,TX_WAIT_FOREVER);  //光通信コマンド終了イベント待ち
*/
    opt_command_exec();         //T2からの光通信コマンド実行 (メッセージキューのセット及び「イベントフラグ待ち)

    return (optc.result);
}




/**
 * 光通信 SOH　通常コマンド
 *  データ長 4バイトの普通のコマンド
 * @param   Cmd     コマンド
 * @param   length  データ長
 * @param   pTxData     フレームへセットとするデータへのポインタ
 * @return      成功:     失敗:
 * @note        T command
 *              length=0の場合は、実際の送信データは4byteのNULLを送る    
 * @note    2020.Jul.06 6個所あったキュー送信、イベントフラグ取得をopt_command_exec()に共通化
 * @note    フレームへのデータセットを追加
 */
uint32_t IR_Nomal_command(uint8_t Cmd , uint16_t length, char *pTxData)
{
//    uint32_t   actual_events;

    optc.cm = Cmd;
    optc.cmd_id[0] = ID_IR_SIMPLE_COMMAND;
    optc.len = (length > 64) ? 64 : length;

    if(length == 0){
        memset(OCOM.txbuf.data, 0x00, 4);
    }
    else{
        memcpy(OCOM.txbuf.data, pTxData, length);     //SOHフレームへデータセット
    }
/*
    tx_queue_send(&optc_queue, optc.cmd_id, TX_WAIT_FOREVER);

    tx_event_flags_get (&optc_event_flags, FLG_OPTC_END, TX_OR_CLEAR, &actual_events,TX_WAIT_FOREVER);
*/
    opt_command_exec();         //T2からの光通信コマンド実行 (メッセージキューのセット及びイベントフラグ待ち)

    return (optc.result);
}


/**
 * ６４バイト情報を書き込む関数
 * @param c
 * @param buf
 * @return      成功:     失敗:
 * @note    T command
 * @note    2020.Jul.06 6個所あったキュー送信、イベントフラグ取得をopt_command_exec()に共通化
 */
uint32_t IR_64byte_write(char c, char *buf)
{

    optc.cm = c;
    optc.buf = buf;
    optc.cmd_id[0] = ID_IR_64BYTE_WRITE;

    opt_command_exec();         //T2からの光通信コマンド実行 (メッセージキューのセット及び「イベントフラグ待ち)
    return (optc.result);

}


/**
 * ６４バイト情報を読み込む関数
 * @note    T command
 * @param c
 * @param buf
 * @return  成功:     失敗:
 * @note    2020.Jul.06 6個所あったキュー送信、イベントフラグ取得をopt_command_exec()に共通化
 */
uint32_t IR_64byte_read(char c, char *buf)
{

    optc.cm = c;
    optc.buf = buf;
    optc.cmd_id[0] = ID_IR_64BYTE_READ;

    opt_command_exec();         //T2からの光通信コマンド実行 (メッセージキューのセット及び「イベントフラグ待ち)

    return (optc.result);
}



/**
 * @brief           データ吸い上げ(1024byte毎)
 * @param out_poi   吸い上げデータを入れるポインタ
 * @param sw        0:吸い上げ開始        1:次データ要求
 * @param Cmd       0x48:個数指定        0x49:データ番号指定
 * @param GetTime   0x48:個数[個]指定    0x49:吸い上げデータ番号
 * @return          成功:吸い上げ全データ数、今回転送データ数 失敗:ERR(IR,NORES) 引き数異常:ERR(IR,PARAMISS)
 * @note            T comannd    / 0x47は無い
 * @note    2020.Jul.06 6個所あったキュー送信、イベントフラグ取得をopt_command_exec()に共通化
 */
uint32_t IR_data_download_t(char *out_poi, uint8_t sw, uint8_t Cmd, uint64_t GetTime)
{

    optc.Cmd = Cmd;
    optc.buf = out_poi;
    optc.GetTime = (uint32_t)GetTime;
    optc.sw = sw;
    optc.cmd_id[0] = ID_IR_DATA_DOWNLOAD_T;

    return (optc.result);



}



/**
 * T-Command SOH Direct
 * @param   pTx     転送データへのポインタ
 * @param   length  転送するデータバイト数
 * @return  成功:     失敗:
 * @note       T command
 * @note    2020.Jul.06 6個所あったキュー送信、イベントフラグ取得をopt_command_exec()に共通化
 * @note    2020.Jul.06 バッファ
 */
uint32_t IR_soh_direct(comSOH_t *pTx, uint32_t length)
{

    memcpy(&OCOM.txbuf.header, pTx, length);

    optc.cmd_id[0] = ID_IR_SOH_DIRECT;

    opt_command_exec();         //T2からの光通信コマンド実行 (メッセージキューのセット及び「イベントフラグ待ち)


    return (optc.result);

}



/**
 * 子機登録関数
 * @return      成功:     失敗:
 * @note    T command
 * @note    2020.Jul.06 6個所あったキュー送信、イベントフラグ取得をopt_command_exec()に共通化
 */
uint32_t IR_Regist(void)
{

    optc.cmd_id[0] = ID_IR_REGIST;

    opt_command_exec();         //T2からの光通信コマンド実行 (メッセージキューのセット及び「イベントフラグ待ち)

    return (optc.result);

}


/**
 * @brief   T2からの光通信コマンド実行
 * 各コマンド処理のメッセージキューのセット及び「イベントフラグ待ちを関数化
 * @return  イベントフラグ取得のステータス
 * @note    2020.Jul.06 6個所あったキュー送信、イベントフラグ取得を共通化
 */
static uint32_t opt_command_exec(void){

    uint32_t   actual_events;
    uint32_t status;

    tx_queue_send(&optc_queue, optc.cmd_id, TX_WAIT_FOREVER);

    status = tx_event_flags_get (&optc_event_flags, FLG_OPTC_END, TX_OR_CLEAR, &actual_events,TX_WAIT_FOREVER); //光通信コマンド終了イベント待ち

    return(status);

}
#endif



/**
 * 光通信 初期化
 * @note    ファイル内
 */
static void opticom_initialize(void)
{
    g_ioport.p_api->pinWrite(OPT_PWR, IOPORT_LEVEL_LOW);         //  電源ＯＮ

}


/**
 * 光通信 未初期化
 * @note    ファイル内
 */
static void opticom_uninitialize(void)
{
    g_ioport.p_api->pinWrite(OPT_PWR, IOPORT_LEVEL_HIGH);         // 電源ＯＦＦ
}




/**
 * 光通信 送信
 * @param pBuf  送信データへのポインタ(SOH/STXヘッダへのポインタ)
 * @note    2020.Apr.16   SOHフレーム専用に変更
 * @note    2020.Jul.03 SUM計算、SUM付加を内包
 */
static void opticom_plane_send(char *pBuf)
{


    uint32_t wait1;
    comSOH_t *pCom = (comSOH_t *)pBuf;

    uint32_t num = (uint32_t)(pCom->length + 7);


    if(0 == Opt_Sum_Crc){
        calc_checksum(pBuf);       //SUM計算、SUM付加
    }
    else{
        calc_soh_crc(pBuf);        //CRC計算、CRC付加
    }


    // 1byte 4.2ms  wait:10ms
    wait1 = (uint32_t)((num * 1000) / OPTICOM_BAUDRATE + 1);

    /*err = */
    g_sf_comms7.p_api->write(g_sf_comms7.p_ctrl, (uint8_t *)&pCom->header, num, wait1);


 //   char *pt = pBuf;
    for(uint32_t i = 0; i < num; i++){
         Printf("%02X ", pBuf[i]);
    }
    Printf("\r\n");
}


/**
 * @brief   光通信 受信
 * @param   check   Sub Comannd check  0:no check  1:check
 * @retval  RFM_NORMAL_END
 * @retval  RFM_SERIAL_ERR
 * @retval  RFM_R500_NAK
 * @retval  RFM_R500_BUSY
 * @retval  RFM_R500_CH_BUSY
 * @retval  RFM_SERIAL_TOUT
 * @attention   ドライバのキューを直接チェックしている → 修正
 */
static uint32_t opticom_recv(int check)
{

    int len = 0;
//    int wt = 1;     //タイムアウト用
    int i;
    int err;
    uint32_t rtn = RFM_NORMAL_END;

//    Printf("\r\nopticom_recv\n");
    
//    wait.opt_comm = WAIT_1SEC;

    // SOH 待ち
    for(;;){

        err = g_sf_comms7.p_api->read(g_sf_comms7.p_ctrl, (uint8_t *)&OCOM.rxbuf.header, 1, 200);        //1Byte読み出し タイムアウト 2000ms

        if(err == SSP_SUCCESS){
            if((OCOM.rxbuf.header == CODE_SOH) || (OCOM.rxbuf.header == CODE_STX)){
                Printf("opticom_recv start (%02X)\r\n", OCOM.rxbuf.header);
                break;
            }
            //SOH/STX以外は引き続き受信
        }else if(err == SSP_ERR_BREAK_DETECT){	                                                        // err=201 の場合、受信継続
            Printf("opticom_recv %d\r\n",err);
        
        }else{
            Printf("SOH time out \r\n");
            rtn = RFM_SERIAL_TOUT;
            goto exit_function;
        }
    }

//    wait.opt_comm = WAIT_5SEC;              // 受信タイムアウト（1024byte×10 / 2400bps = 4.267s）   1byte 4.2ms

    err = g_sf_comms7.p_api->read(g_sf_comms7.p_ctrl, (uint8_t *)&OCOM.rxbuf.command, 4, 20);    //commandからLengthまで4Byte受信 タイムアウト200ms

    if(err != SSP_SUCCESS)
    {
        Printf("receve error 1 %d \r\n" ,err);
        rtn = RFM_SERIAL_ERR;
        goto exit_function;
    }
    
    len = OCOM.rxbuf.length;
    Printf("\r\n    opt recive len %ld \r\n", len);

    if(len > (1024+2)){
        len = 1024 + 2;
    }

    //wt = 2 + (len >> 1);						// wt = 1 + len >> 1 --> wt = 2 + len >> 1 に修正segi
//    wt = 100;

    err = g_sf_comms7.p_api->read(g_sf_comms7.p_ctrl, (uint8_t *)&OCOM.rxbuf.data, (uint32_t)(len + 2), 100);  //DATAからSUMまで受信 タイムアウト1000ms
    if(err != SSP_SUCCESS)
    {
        Printf("receve error 2 %d \r\n" ,err);
        rtn = RFM_SERIAL_ERR;
        goto exit_function;
    }

    //デバッグ用の表示
    char *pt = (char *)&OCOM.rxbuf.header;
    for(i = 0; i < len + 7; i++){
          Printf("%02X ", *(pt++));
    }
    Printf("\r\n");

    //チェックサム
    Opt_Sum_Crc = judge_checksum(&OCOM.rxbuf.header);
    if(Opt_Sum_Crc == -1)      //2020.Juｌ.03 CRC/SUM両対応
    {
        Printf("receve error 3 \r\n");
        rtn = RFM_SERIAL_ERR;
        goto exit_function;
    }

    //サブコマンドチェック
    if(check == SCMD_CHECK){
        if(OCOM.rxbuf.subcommand == CODE_NAK) {
            rtn = RFM_R500_NAK;
        }
        else if(OCOM.rxbuf.subcommand == CODE_BUSY) {
            rtn = RFM_R500_BUSY;
        }
        else if(OCOM.rxbuf.subcommand == CODE_CH_BUSY){
            rtn = RFM_R500_CH_BUSY;
        }
        else if(OCOM.rxbuf.subcommand == CODE_REFUSE){      // 拒否 2022.03.23 add
            rtn = RFM_REFUSE;
        }
    }

exit_function:;
    Printf("    opticom_recv exit %02X\r\n", rtn);
    return(rtn);

}



/**
 * @brief       光通信 送受信
 *              チェックサム計算、チェックサム付加、送信、受信
 * @param  pBuf
 * @param  check
 * @retval  MD_OK
 * @retval  MD_ERROR
 * @note    USB
 * @note    2020.Jul.03 修正
 */
static uint32_t opticom_send_recv(char *pBuf, int check)
{
    uint32_t ret,Err;


    opticom_plane_send(pBuf);  // 送信（１６バイトＦＩＦＯがあるので送信残り最大１６バイトあり）

    ret = opticom_recv(check);  //受信

    switch (ret)
    {
        case MD_OK:
            if(OCOM.rxbuf.command != OCOM.txbuf.command){
                Err = ERR(IR, RESERR);
            }
            else{
                Err = ERR(IR, NOERROR);
            }
            break;
        case RFM_SERIAL_TOUT:
            Err = ERR(IR, NORES);
            break;
        case RFM_REFUSE:            // 拒否 2022.03.23 add
            Err = ERR(IR, REFUSE);
            break;
        case RFM_SERIAL_ERR:
        default:
            Err = ERR(IR, RESERR);
            break;
    }

    return(Err);

}



/**
 * @brief   光通信 送信フォーマット設定
 *          OCOM.txbufに送信フレームをセット
 * @param c         コマンド
 * @param sc        サブコマンド
 * @param len       データ長
 * @param   pData   データへのポインタ
 * @note    データ長0のときは、4バイトのデータを送る
 * @note    USB tcommand
 * @note    2020.01.31  引数lenをuint8_t -> uint16_tに変更
 * @note    2020.Jul.04 SOHフレームへのデータセットまで追加
 */
static void opticom_soh_format(uint8_t c, uint8_t sc, uint16_t len, char *pData)
{

    OCOM.txbuf.header = CODE_SOH;
    OCOM.txbuf.command = c;
    OCOM.txbuf.subcommand = sc;
    OCOM.txbuf.length = len;

//    if(len == 0){
    if(len < 4){
        OCOM.txbuf.length = 4;
        memset(OCOM.txbuf.data, 0x00, 4);
        if(len != 0){
            memcpy(OCOM.txbuf.data, pData, len);
        }
    }else{
        memcpy(OCOM.txbuf.data, pData, len);
    }
}



/**
 * 光通信 ブレーク送出
 * @note    T Command
 * @note    2020.Jul.03  修正
 */
static void opticom_break(void)
{

    uint8_t bk = 0x00;

    g_sf_comms7.p_api->write(g_sf_comms7.p_ctrl, &bk, 1, 5);

    tx_thread_sleep (5);                              //  wait 50ms に変更 10ms ????
    Printf("OPT Break send\r\n");
}


/**
 *
 * @note    0x48：個数指定  0x49：データ番号指定   
 * @return  実行結果 エラーコード
 * @note    2020.Jul.03 各コマンド共通処理を関数外に出した
 * @see ru_header   子機 ヘッダ 64Byte
 */
static uint32_t func_ir_data_download_t(void)
{

    uint32_t rtn;
    uint32_t t = 0;
    uint16_t intt;//記録インターバル

    Printf("----> func_ir_data_download_t   optc.sw  %02X \r\n", optc.sw );
    Printf("---->                           optc.Cmd %02X \r\n", optc.Cmd );

    if(optc.sw > 0)
    {
        rtn = ir_data_block_read();                             // データブロック転送要求

        if(rtn == ERR(IR, NOERROR))
        {
            memcpy( optc.buf, (char *)OCOM.rxbuf.data, OCOM.rxbuf.length);  // cmdstatusize;
            rtn = OCOM.rxbuf.length;
        }
    }
    else
    {
        Printf("---->                           optc.Cmd        %02X \r\n", optc.Cmd );
        Printf("---->                           optc.GetTime    %ld  \r\n", optc.GetTime );
        switch(optc.Cmd)
        {
            case 0x47:                                          // 
            case 0x48:                                          // 個数指定の場合

                if(optc.GetTime == 0){
                    t = 0;                         // GetTimeが0の場合は全データ吸い上げ
                }
                else
                {
                    opticom_soh_format(0x48, 0x00, 2, (char *)&optc.GetTime);              // 光通信 SOH送信フォーマット設定

                    rtn = opticom_send_recv(&OCOM.txbuf.header, SCMD_CHECK);                                 //送受信（リトライ無し）

                    if(rtn == ERR(IR, NOERROR))
                    {
                        if(optc.Cmd == 0x48)
                        {
                            // 吸上げ期間と記録インターバルからデータ吸上げ個数を決定する

                            intt = *(uint16_t *)&ru_header.intt[0];                // 記録インターバル[秒]
                            t = (optc.GetTime * 60) / intt;                  // 吸い上げ期間を秒に変換してから吸い上げデータ数に変換する

                            if(((optc.GetTime * 60) % intt) != 0){
                                t++;       // 余りがあるならもう１個分追加
                            }

                            t *= 2;                                             // データバイト数は記録データ数の2倍

                            Printf("\r\n  intt = %ld   t = %ld \r\n", intt, t);
                        }
                        else{  // 0x47
                            // データ個数指定からデータバイト数を算出する

                            t = optc.GetTime * 2;                                    // データバイト数は記録データ数の2倍

                            optc.Cmd = 0x48;
                        }

                        if(ru_header.ch_zoku[1] != 0x00){
                            t *= 2;                // ２ＣＨ記録機種の場合、２倍にする
                        }

                        if(t >= 0x00010000){
                            t = 0x00000000;                     // ２バイト制限
                        }
                    }
                    else{
                        goto Exit_Func;
                    }
                }
                break;

            case 0x49:                                          // データ番号指定の場合

                t = optc.GetTime;
                break;

            default:
                rtn = ERR(IR, PARAMISS);
                goto Exit_Func;
        }

        Printf("\r\n  --> exec 1 %02X %ld\r\n", optc.Cmd, t);

        opticom_soh_format(optc.Cmd, 0x00, 2, (char *)&t);              // 光通信 SOH送信フォーマット設定

        rtn = opticom_send_recv(&OCOM.txbuf.header, SCMD_CHECK);                                 //送受信（リトライ無し）
        
        Printf("\r\n  --> exec 2 %04X \r\n", rtn);
        if(rtn == ERR(IR, NOERROR))
        {
            memcpy((char *)ru_header.intt,  OCOM.rxbuf.data, 64);
            memcpy(optc.buf, (char *)ru_header.intt, sizeof(ru_header));        //ru_header 64byte
            rtn = (uint32_t)(ru_header.data_byte[0] + (uint16_t)ru_header.data_byte[1] * 256);
            Printf("\r\n  --> exec 3 %04X  %02X %02X %02X %02X\r\n", rtn, ru_header.data_byte[0],ru_header.data_byte[1],ru_header.intt[0],ru_header.intt[1]);
        }
    }

   
Exit_Func:;
    Printf("  --> exec end %04X \r\n", rtn);

    return (rtn);

}



#if 0

/**
 * 光通信ポート 使用開始
 * @note  T Command USB
 */
static void opt_start(void)
{
    opticom_initialize();
}


/**
 * 光通信ポート 使用終了
 * @note    T Command USB
 */
static void opt_stop(void)
{
    opticom_uninitialize();
}

#endif


/**
 * データ吸い上げ要求する関数
 * ID_IR_HEADER_READ
 * @return  実行結果 エラーコード
 * @note    2020.Jul.03 各コマンド共通処理を関数外に出した
 */
static uint32_t func_ir_regist(void)
{
    uint32_t Err;
                                           // ブレーク送出
    char buf[20];

    //一旦バッファにコピー（パディングがあるかも）
    memcpy((char *)&buf[0], optc.gid, sizeof(optc.gid));
    memcpy((char *)&buf[8], optc.name, sizeof(optc.name));
    buf[16] = optc.num;
    buf[17] = optc.ch;
    buf[18] = optc.band;


    opticom_soh_format(0x60, 0x00, 19, buf);              // 光通信 SOH送信フォーマット設定

    Err = opticom_send_recv(&OCOM.txbuf.header, SCMD_CHECK);                                 //送受信（リトライ無し）

    return (Err);
}



/**
 * 光コマンド実行 リトライ無し
 * CM+SCM+Length(4)+ Data[4]
 * @return  実行結果 エラーコード
 * @note    2020.Jul.03 各コマンド共通処理を関数外に出した
 */
static uint32_t func_ir_simple_command(void)
{

    uint32_t Err;

//    opticom_soh_format(optc.cm, CODE_ACK, optc.len, (char *)optc.buf);              // 光通信 SOH送信フォーマット設定
    opticom_soh_format(optc.cm, optc.scm, optc.len, (char *)optc.buf);              // 光通信 SOH送信フォーマット設定

    Err = opticom_send_recv((char *)&OCOM.txbuf.header, SCMD_NOCHECK);        //送受信

    return (Err);

}



/**
 * 光コマンド実行 リトライ無し
 * CM+SCM+Length(4)+ Data[4]
 * @return  実行結果 エラーコード
 * @note    2020.Jul.03 各コマンド共通処理を関数外に出した
 */
static uint32_t func_ir_simple_command_with_retry(void)
{

    uint32_t Err;

    opticom_soh_format(optc.cm, CODE_ACK, 0, (char *)optc.buf);              // 光通信 SOH送信フォーマット設定

    Err = opticom_send_recv(&OCOM.txbuf.header, SCMD_CHECK);                                 //送受信（リトライ無し）

    return (Err);

}



/**
 * データ吸い上げ要求する関数
 * ID_IR_HEADER_READ
 * @return  実行結果 エラーコード
 * @note    2020.Jul.03 各コマンド共通処理を関数外に出した
 */
static uint32_t func_ir_header_read(void)
{

    uint32_t Err;

    opticom_soh_format(optc.Cmd, 0x00, 2, (char *)&optc.DataSize);              // 光通信 SOH送信フォーマット設定

    Err = opticom_send_recv(&OCOM.txbuf.header, SCMD_CHECK);                                 //送受信（リトライ無し）

    if(Err == ERR(IR, NOERROR)){
        memcpy((char *)&ru_header, (char *)OCOM.rxbuf.data, sizeof(ru_header));    // ヘッダファイルコピー 64Byte
    }

    return (Err);

}



#if 0
//未使用関数
/**
 * データ吸い上げ要求する関数
 * ID_IR_HEADER_READ
 * @return  実行結果 エラーコード
 * @note    2020.Jul.03 各コマンド共通処理を関数外に出した
 */
static uint32_t func_ir_data_block_read(void)
{

    uint32_t Err;


 //   opt_start();         // 光通信 使用開始（ 光通信 電源ＯＮ）

 //   opticom_break();                                            // ブレーク送出

 //   opticom_soh_format(0x41, CODE_ACK, 0, (char *)&optc.DataSize);              // 光通信 SOH送信フォーマット設定
    opticom_soh_format(0x41, CODE_ACK, 2, (char *)&optc.DataSize);              // 光通信 SOH送信フォーマット設定
 /*
    OCOM.txbuf.data[0] = (uint8_t)optc.DataSize;                  // 吸い上げバイト数 or データ番号;
    OCOM.txbuf.data[1] = (uint8_t)(optc.DataSize >> 8);
*/
    Err = opticom_retry_send();                                 //送受信（リトライ無し）

 //   opt_stop();             // 光通信 使用終了（光通信 電源OFF）

    if(Err == ERR(IR, NOERROR)){
        memcpy((char *)&ru_header, (char *)OCOM.rxbuf.data, sizeof(ru_header));    // ヘッダファイルコピー 64Byte
    }

//    optc.result = Err;
    return (Err);

}

#endif




/**
 *  ６４バイト情報を書き込む関数
 *  ID  ID_IR_64BYTE_WRITE
 *  @return  実行結果 エラーコード
 *  @note   T command
 * @note    2020.Jul.03 各コマンド共通処理を関数外に出した
 */
static uint32_t func_ir_64byte_write(void)
{

    uint32_t Err;   //2020.Jan.16

    opticom_soh_format(optc.cm, 0x00, 64, (char *)optc.buf);              // 光通信 SOH送信フォーマット設定

    Err = opticom_send_recv(&OCOM.txbuf.header, SCMD_CHECK);                                 //送受信（リトライ無し）

    return (Err);

}




/**
 * ６４バイト情報を読み込む関数
 * ID  ID_IR_64BYTE_READ
 * @return  実行結果 エラーコード
 * @note    2020.Jul.03 各コマンド共通処理を関数外に出した
 */
static uint32_t func_ir_64byte_read(void)
{
     uint32_t Err;      //2020.Jan.16

     opticom_soh_format(optc.cm, 0x00, 0, (char *)optc.buf);              // 光通信 SOH送信フォーマット設定

     Err = opticom_send_recv(&OCOM.txbuf.header, SCMD_CHECK);                                 //送受信（リトライ無し）

     if(Err == ERR( IR, NOERROR )){
         memcpy((char *)optc.buf, (char *)OCOM.rxbuf.data, 64);   // 読み込みデータコピー
     }

     return (Err);

}


/**
 * 光コマンド実行 SOH direct
 * Length & Sum(CRC)も含めてコマンドより取得
 * 送信データは、OCOM.txbufにコピー済み
 * @return  実行結果 エラーコード
 * @note    2020.Jul.03 各コマンド共通処理を関数外に出した
 */
static uint32_t func_ir_soh_direct(void)
{

    uint32_t Err;
    uint16_t len;

    Err = ERR(IR, NOERROR);

    len = OCOM.txbuf.length;
    if(len > 1024){
        OCOM.txbuf.length = 1024;
    }

    Err = opticom_send_recv((char *)&OCOM.txbuf.header, SCMD_CHECK);        //送受信

//    Printf("func_ir_soh_direct    %02X\r\n",Err);

    return (Err);

}

