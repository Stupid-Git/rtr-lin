/*
 * comp_rfm_hw.c
 *
 *  Created on: Dec 3, 2024
 *      Author: karel
 */


#include "_r500_config.h"

#define _COMP_RFM_HW_C_
#include "comp_rfm_hw.h"



#include "r500_defs.h"

/*static*/ extern def_com RCOM;                        ///< RF通信用バッファ
/*static*/ extern int Rf_Sum_Crc;        //受信フレームがSUMの場合0 CRC16の場合1

void hw_RF_CS_ACTIVE(void)
{
    //TODO Drive CS Pin Active (LOW)
}
void hw_RF_CS_INACTIVE(void)
{
    //TODO Drive CS Pin Inactive (HIGH)
}
void hw_RF_RESET_ACTIVE(void)
{
    //TODO
}
void hw_RF_RESET_INACTIVE(void)
{
    //TODO
}


/**
 * 無線モジュール リセット
 * @note    ファイル内で使用
 * @note    2020.Aug.21 delay使用を廃止してthread_sleepに変更
 */
void hw_rfm_reset(void)
{
    hw_RF_CS_ACTIVE();                                     // ＣＳ Ｌｏｗ（無線モジュール アクティブ）

    hw_RF_RESET_ACTIVE();

    tx_thread_sleep(1); //10ms

    /*
    wait.ms_1.rf_wait = 2;
    do{;}while(wait.ms_1.rf_wait!=0);
*/
    hw_RF_RESET_INACTIVE();
    /*
    wait.ms_1.rf_wait = 2;
    do{;}while(wait.ms_1.rf_wait!=0);
*/
    tx_thread_sleep(1); //10ms
}


/**
 * 無線モジュール 初期化
 * @note ファイル内で使用
 * @note    2020.Aug.21 delay使用を廃止してthread_sleepに変更
 */
void hw_rfm_initialize(void)
{
    hw_RF_CS_ACTIVE();                                                  // ＣＳ Ｌｏｗ（無線モジュール アクティブ）
    tx_thread_sleep(7); //70ms
    //osDelay(20);                                                // ＪＰ：６ｍｓ必要 ＵＳ：２～３ｍｓ必要
/*    wait.ms_1.rf_wait = 20;
    do{;}while(wait.ms_1.rf_wait!=0);

    // 新ＲＦモジュールに必要
    //osDelay(50);
    wait.ms_1.rf_wait = 50;
    do{;}while(wait.ms_1.rf_wait!=0);
*/
}


/**
 * 無線モジュール 送信
 * @param pData  送信データへのポインタ（SOH/STXヘッダへのポインタ）
 * @note    2020.Jul.03 SUM/CRC付加を内包
 * @note    2020.Jul.03  引数numを廃止
 */
void hw_rfm_send(char *pData )
{

    uint16_t err;
    uint32_t wt;
    uint32_t i;
    uint16_t num;
/**/
    // 途中でRFMとの通信を切断した時の、ゴミ処理
    /*TODO
    err = g_sf_comms5.p_api->close(g_sf_comms5.p_ctrl);
    Printf("hw_rfm_send com close %d \r\n", err);
    err = g_sf_comms5.p_api->open(g_sf_comms5.p_ctrl, g_sf_comms5.p_cfg);
    Printf("hw_rfm_send com open %d \r\n", err);
    TODO*/
/**/
// 2022.08.08 ↓ 無線通信はSUM計算固定
    //if(0 == Rf_Sum_Crc){
    //    num = calc_checksum((char *)pData);       //SUM計算、SUM付加
    //}
    //else{
    //    num = calc_soh_crc((char *)pData);        //CRC計算、CRC付加
    //}
    Rf_Sum_Crc = 0;                         // SUM計算
    num = calc_checksum((char *)pData);     //SUM計算、SUM付加
// 2022.08.08 ↑

    memset((char *)&RCOM.rxbuf.header, 0x00, 3);                        // 先頭の３バイトをクリア
    RCOM.rxbuf.length = sizeof(RCOM.rxbuf.data) - 2;

    Printf("hw_rfm_send start\r\n");

    for(i=0;i<num;i++){
        Printf("%02X ", pData[i]);
    }
    Printf("\r\n");


    // 1byte 520us  wait:10ms   19200bps
    wt = (uint32_t)(((num * 1000) / 19200) + 1 + 1);
    Printf("hw_rfm_send wt %ld\r\n",wt);
/*TODO
    err = g_sf_comms5.p_api->lock(g_sf_comms5.p_ctrl, SF_COMMS_LOCK_TX, TX_WAIT_FOREVER);    //comm5ロック
    err = g_sf_comms5.p_api->write(g_sf_comms5.p_ctrl, (uint8_t *)pData, num, wt);
    g_sf_comms5.p_api->unlock(g_sf_comms5.p_ctrl, SF_COMMS_LOCK_TX);    //comm5ロック解除
TODO*/
    Printf("hw_rfm_send %d(%d) wt:%d\r\n", err, num ,wt);
}


/**
 * 無線モジュール 受信

 * @param t タイムアウト時間［ｍｓ］
 * @return
 *         1byte 520us   19200bps  packet size max 7+64 = 71
 *         71byte 37 m
 *
 * @note    rf_buffer に受信し、RCOM.rxbufにコピーしている→廃止
 * @note    2020.Jul.06  rf_buffer廃止（受信バッファを一つにした）
 * @note    2020.Jul.06  commsドライバのキューの直接チェックを廃止
 * @note    2020.Aug.21 受信サイズ66Byte制限を外した
 * @note    2020.Aug.21 REFUS応答時、RFM_NORMAL_ENDで返っていた
 */
uint8_t hw_rfm_recv(uint32_t t)
{
    uint32_t err;
    uint16_t len;
    uint8_t rtn = RFM_NORMAL_END;

    uint32_t RfRxtimeout;


    wait.rfm_comm = RfRxtimeout = (t == UINT32_MAX) ? 0 : (t / 10);    // 受信タイムアウトセット  10ms分解能

//rfm_recv_init:
    //err = g_sf_comms5.p_api->lock(g_sf_comms5.p_ctrl, SF_COMMS_LOCK_RX, TX_WAIT_FOREVER);    //comm5ロック

    memset((char *)&RCOM.rxbuf.header, 0x00, 3);                    // 先頭の３バイトをクリア
    RCOM.rxbuf.length = sizeof(RCOM.rxbuf.data) - 2;

    Printf("[hw_rfm_recv Start] (%ld)(%ld)\r\n", wait.rfm_comm,t);

    // SOH 受信待ちループ
    for(;;){

        RCOM.rxbuf.header = 0x00;

        if(RfRxtimeout == 0)    RfRxtimeout = 1;                // sakaguchi 2021.06.29 タイムアウトなし設定の場合、readタイムアウトは1(10ms)にする
        /*TODO
        err = g_sf_comms5.p_api->read(g_sf_comms5.p_ctrl, (uint8_t *)&RCOM.rxbuf.header, 1, RfRxtimeout); //1Byte受信 タイムアウト 10ms->ゆざー指定に変更

        if(err == SSP_SUCCESS){
            if(RCOM.rxbuf.header == CODE_SOH){
                Printf("## hw_rfm_recv SOH (%ld)\r\n", wait.rfm_comm);
                break;
            }
        }else if(err == SSP_ERR_TIMEOUT)
        {
            rtn = RFM_SERIAL_TOUT;
            goto exit_function;
        }
        TODO*/

        if(t == UINT32_MAX){
           if(timer.int1000 > RF_buff.rf_req.timeout_time)     // タイムアウトなし設定なら、無線コマンド全体のタイムアウトで戻る
           {
                rtn = RFM_DEADLINE;
                goto exit_function;
           }
        }
        else if(wait.rfm_comm == 0){
            /*TODO
            g_sf_comms5.p_api->close(g_sf_comms5.p_ctrl);
            g_sf_comms5.p_api->open(g_sf_comms5.p_ctrl, g_sf_comms5.p_cfg);
            TODO*/
            Printf("RF Com Reboot \r\n");
            rtn = RFM_SERIAL_TOUT;
            goto exit_function;
        }

        if(RF_buff.rf_req.cancel == 1)                          // キャンセルフラグ
        {
            rtn = RFM_CANCEL;
            goto exit_function;
        }

        if(wait.rfm_comm == 0){
            rtn = RFM_SERIAL_TOUT;
            goto exit_function;
        }

        tx_thread_sleep(5);

    }//受信待ちループ

    //SOH以降の残りバイト受信
    Printf("\r\n## hw_rfm_recv SOH OK (%ld)\r\n", wait.rfm_comm);

    /*TODO
    err = g_sf_comms5.p_api->read(g_sf_comms5.p_ctrl, (uint8_t *)&RCOM.rxbuf.command, 4, 3);    //commandからLengthまで4Byte受信 タイムアウト30ms→200ms→30ms
    if(err != SSP_SUCCESS){
        rtn = RFM_SERIAL_ERR2;
        goto exit_function;
    }
    TODO*/

    if(RCOM.rxbuf.length == 0){                                 // 2023.03.01 受信データ長が0の場合、エラーで終了
        rtn = RFM_SERIAL_ERR2;
        goto exit_function;
    }

    /*TODO
    //66Byteの受信制限を外した 2020.Aug.01
    err = g_sf_comms5.p_api->read(g_sf_comms5.p_ctrl, (uint8_t *)&RCOM.rxbuf.data, (uint32_t)(RCOM.rxbuf.length+2), 5);       //DATAからSUMまで受信 タイムアウト 50ms→200ms→50ms
    if(err != SSP_SUCCESS){
        rtn = RFM_SERIAL_ERR2;                                  // 2023.02.20 異常時の戻り値追加
        goto exit_function;
    }
    TODO*/

    len = (RCOM.rxbuf.length > 66) ? 66 :RCOM.rxbuf.length;     //デバッグ表示用の制限（コマンド本体は制限しない」）
//    Printf("\r\n RF RX(%d) \r\n" , len );
    Printf("RF RX\r\n%02X %02X %02X %04X ", RCOM.rxbuf.header, RCOM.rxbuf.command, RCOM.rxbuf.subcommand, RCOM.rxbuf.length);
    for(int i=0;i<len + 2;i++){
        Printf("%02X ", RCOM.rxbuf.data[i]);
    }
    Printf("\r\n");

// 20200529_下記のコメントアウトを外した
    if(timer.latch_enable == true)
    {
        timer.latch_enable = false;

        timer.arrival = timer.int125;                           // 子機が応答した時間をラッチ
        //timer.int125 = 0;                                     // ここでクリアするとリトライ時のTIMEパラメータ用のカウントに影響する
    }
// ここまで



    RfCh_Status = CH_OK;                                            // sakaguchi UT-0035
    Rf_Sum_Crc = judge_checksum(&RCOM.rxbuf.header);     //CRC受信はCRCで送信、SUM受信はSUMで送信するため保存しておく
    if(Rf_Sum_Crc == -1)     //2020.Juｌ.03 CRC/SUM両対応
    {
        rtn = RFM_SERIAL_ERR;
    }
    else if(RCOM.rxbuf.subcommand == CODE_NAK){
        rtn = RFM_R500_NAK;
    }
    else if(RCOM.rxbuf.subcommand == CODE_BUSY){
        rtn = RFM_R500_BUSY;
    }
    else if(RCOM.rxbuf.subcommand == CODE_CH_BUSY){             // 無線モジュールからCH_BUSYを受信した    segi
        LED_Set(LED_BUSY, LED_ON);      // ここでいいの？？？？
        rtn = RFM_R500_CH_BUSY;
        RfCh_Status = CH_BUSY;                                  // sakaguchi UT-0035
        PutLog(LOG_RF, "Channel Busy");
    }
    else if(RCOM.rxbuf.subcommand == CODE_REFUSE){
        rtn = RFM_REFUSE;      //仮      NOMAL_ENDが返っていた
    }
// 2024 01 15 D.00.03.184 ↓
    else if(RCOM.rxbuf.subcommand == CODE_CRC){
        rtn = RFM_R500_NAK;
        PutLog(LOG_RF, "CRC Error");
    }
// 2024 01 15 D.00.03.184 ↑
    else{
        __NOP();        //ACK応答
    }

exit_function:;

    //g_sf_comms5.p_api->unlock(g_sf_comms5.p_ctrl, SF_COMMS_LOCK_RX);    //comm5ロック解除

    Printf("\r\n  hw_rfm_recv rtn=%02X / %04X (%ld)\r\n", rtn, err,wait.rfm_comm );
    return(rtn);


}



/**
 * @brief   無線モジュールのビジー状態をチェックする
 * 無線モジュールが通信中かどうかチェックする
 * @return  1 = BUSY
 * @note    未使用
 */
int hw_Chreck_RFM_Busy(void)
{
    return 0;
    /*TODO
    ioport_level_t level;
    g_ioport.p_api->pinRead(RFM_BUSY_PORT, &level);
    if(level == IOPORT_LEVEL_LOW){
        return (1);
    }
    else {
        return(0);
    }
    TODO*/
 }

/**
 * @brief   無線モジュールのステータスをチェックする
 * @return  1 = module受信動作中
 * @note   通常ポートは  OD出力ポート設定
 * @note    未使用
 */
int hw_Chreck_RFM_Status(void)
{
    return 0;
    /*TODO
    ioport_level_t level;
    g_ioport.p_api->pinRead(RFM_RESET_PORT, &level);
    return (level);
    TODO*/
}
