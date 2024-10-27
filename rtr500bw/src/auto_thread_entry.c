/**
 * 自律動作のスレッド
 * @file    auto_thread_entry.c
 * @date    2019.Dec.26
 * @note    2019.Dec.26 ビルドワーニング潰し完了
 * @note    2020.Jul.07 自律動作関連の関数をrf_thread_entry.cから移動、auto_thread_entry.h追加
 * @note	2020.Jul.27 GitHub 0722ソース反映済み
 * @note	2020.Jul.27 GitHub 0727ソース反映済み
 * @note	2020.Aug.07	0807ソース反映済み 
 */


#define _AUTO_THREAD_ENTRY_C_

#include "flag_def.h"
#include "MyDefine.h"
#include "Log.h"
#include "General.h"
#include "Error.h"
#include "Warning.h"
#include "Monitor.h"
#include "Rfunc.h"
#include "lzss.h"
#include "Suction.h"
#include "Sfmem.h"


#include "rf_thread.h"
#include "rf_thread_entry.h"

#include "event_thread.h"
#include "event_thread_entry.h"

#include "cmd_thread_entry.h"
#include "usb_thread_entry.h"

#include "system_thread.h"

#include "auto_thread.h"
#include "auto_thread_entry.h"
#include "R500sub.h"                // 2023.05.26




//rf_thread_entry.cから移動

//変数定義
static uint8_t Chk574No;                                   ///<  RTR-574 [前半受信待ち]=0  [後半受信待ち]=子機情報の子機番号
static uint8_t DualData;                                   ///<  RTR-574 の吸い上げデータが[照度・紫外線]と[温度・湿度]の2個ペアで(0/1)=(ない/ある)

//プロトタイプ
static uint32_t AT_start(void);
//未使用static void call_PutLog(uint8_t err_no);
//static uint8_t chk_noise(uint8_t ch_no);                 // sakaguchi 2021.01.25 del
static uint8_t check_realscan_data(uint8_t unit_no);

static uint32_t chk_w_config(uint8_t G_No, uint8_t U_No);
static uint32_t set_w_config(uint8_t G_No , uint8_t U_No , uint16_t Dat , uint8_t sw);
static uint16_t w_onoff_read(uint8_t G_No , uint8_t U_No);
static uint8_t w_onoff_write(uint8_t G_No, uint8_t U_No, uint16_t Dat);
//static uint8_t ALM_bak_poi(uint8_t G_No, uint8_t U_No);               // sakaguchi 2021.03.01
static void ALM_bak_f1_clr(uint8_t size);

static uint8_t sensor_check(uint16_t so_data , uint8_t zoku);
static uint8_t ch2_check(uint8_t rec_status);
static uint8_t w_unit_No(uint8_t unit_No ,uint8_t kind ,uint8_t ch1_zoku_0);
static void Warning_Check_After(void);
static void Warning_Info_Clear(void);           // sakaguchi 2020.10.07


static uint8_t download574(void);
static uint8_t  download574_2in1(void);
static uint8_t NIKOICHI_select(void);
static uint8_t NIKOICHI_select_sram(void);


static uint8_t check_download_ng(void);

//static uint8_t WR_chk(uint8_t G_No, uint8_t U_No, uint8_t pogi);      // sakaguchi 2021.03.01
//static uint8_t WR_set(uint8_t G_No ,uint8_t U_No ,uint8_t pogi ,uint8_t setdata);
static uint8_t WR_inc(uint8_t G_No, uint8_t U_No, uint8_t pogi);



//static uint8_t chk_sram_ng(void);     //グローバル
//未使用static uint8_t SRAM_init(void);

static uint16_t Data_diff(uint16_t A_dat , uint16_t B_dat);

static uint8_t auto_download_New(uint8_t PARA);
static uint8_t check_double(uint8_t format_no,uint16_t group_no,uint8_t unit_no);
static uint8_t dual_check_574(uint8_t unit_no);
static uint8_t check_do_flag(void);


static void set_download_retry_do_flag(uint8_t group_cnt);

static void set_keiho_do_flag_New(uint8_t sw);
static void set_sekisan_do_flag_New(uint8_t sw);

static void w_config_data_make(void);
static uint8_t check_do_unit_flag(uint8_t *poi);
static void clr_flag(char *poi , uint8_t No);



static uint16_t chk_download(uint8_t G_No , uint8_t U_No , uint8_t *siri);
static uint8_t set_download(uint8_t G_No , uint8_t U_No , uint8_t *siri , uint16_t Dat_No);
static void ALM_bak_write(void);


extern void WarningPort(uint16_t warnCur, uint16_t warnNow, uint16_t warnNew);


/**
 *  Auto Thread entry function
 */
// run回数が多すぎるので確認すること
void auto_thread_entry(void)
{

    uint32_t actual_events;
    int busy = 0;
    int cnt = 0;

    while (1)
    {
    
        tx_event_flags_get (&g_wmd_event_flags, FLG_EVENT_KEIHO | FLG_EVENT_MONITOR | FLG_EVENT_SUCTION | FLG_EVENT_INITIAL | FLG_EVENT_REGUMONI | FLG_EVENT_FULLMONI , 
                            TX_OR_CLEAR, &actual_events, TX_WAIT_FOREVER);
        Printf("### auto_thread !!!!!!  %04X (%ld)\r\n", actual_events ,cnt);
        cnt++;

        if(actual_events & FLG_EVENT_INITIAL){          // AT Start起動

            if(tx_mutex_get(&g_rf_mutex, WAIT_100MSEC) == TX_SUCCESS)
            {
//                actual_events &= FLG_EVENT_MASK;
                actual_events ^= FLG_EVENT_INITIAL;
                Printf("====> Start AUTO_control %04X\r\n", FLG_EVENT_INITIAL);
                AUTO_control(FLG_EVENT_INITIAL);
                Printf("====> Exit AUTO_control\r\n");

                Printf("    ==== g_rf_mutex put 1 !!\r\n");
                tx_mutex_put(&g_rf_mutex);
            }
            else{
                PutLog(LOG_RF, "RF Thread Busy");
//                tx_event_flags_set (&g_wmd_event_flags, FLG_EVENT_INITIAL, TX_OR);  // AT Startは実行できるまで繰り返す
//                tx_thread_sleep(50);
            }
        }

        if(actual_events){
            if(tx_mutex_get(&mutex_network_init, TX_NO_WAIT) == TX_SUCCESS){
                tx_mutex_put(&mutex_network_init);
                Printf("    Auto network init mutex get!!\r\n");
                Printf("    ==== g_rf_mutex get 1 !!\r\n");
                if(tx_mutex_get(&g_rf_mutex, WAIT_100MSEC) == TX_SUCCESS)
                {
                    actual_events &= FLG_EVENT_MASK;
                    Printf("====> Start AUTO_control %04X\r\n", actual_events);
                    AUTO_control(actual_events);
                    Printf("====> Exit AUTO_control %d\r\n", EWAIT_res);

                    Printf("    ==== g_rf_mutex put 1 !!\r\n");
                    tx_mutex_put(&g_rf_mutex);
                }
                else{
                    PutLog(LOG_RF, "RF Thread Busy");
                }
                busy = 0;
            }
            else{
                busy ++;
                Printf("    ==== Network Init Busy !! (%d)\r\n", busy);  
                PutLog(LOG_SYS, "Network Not Ready");            
            }
        }
        Printf("### auto_thread !!!!!!  End (%ld)(%ld)\r\n", cnt, EWAIT_res);
        tx_thread_sleep (1);
    }
}














// ↓↓↓↓↓ segi ------------------------------------------------------------------------------------
// sakaguchi 2021.01.25 del
///**
// * ノイズのある周波数チャンネルの履歴をチェックする
// * @param ch_no 周波数チャンネル
// * @return  BUSYフラグ(0/0以外)=(BUSYではない/BUSY)
// * @attention   とりあえず、常に0を返す
// * @todo    未完成関数
// */
//static uint8_t chk_noise(uint8_t ch_no)
//{

//    SSP_PARAMETER_NOT_USED(ch_no);       //とりあえず未使用引数

//  uint8_t block;     //とりあえずコメントアウト
//  uint8_t shift;     //とりあえずコメントアウト
//  uint8_t tmp_ch;     //とりあえずコメントアウト
//    uint8_t rtn = 0;

//  block = ch_no / 8;     //とりあえずコメントアウト
//  shift = ch_no % 8;     //とりあえずコメントアウト
//  tmp_ch = (uint8_t)((uint8_t)0x01 << shift);     //とりあえずコメントアウト

//  rtn = NOISE_CH[block] & tmp_ch;     //とりあえずコメントアウト

//    return (rtn);
//}
//********** end of uint8_t chk_noise(uint8_t ch_no) **********


// New New New New New New New New New New New New New New New New New New New New New New
//  自律動作の開始時に現状の状態を把握する
//
//
//  引き数：sw 0:モニタリングと警報監視  1:モニタリングのみ
//  返り値：
//
// New New New New New New New New New New New New New New New New New New New New New New
/**
 * @brief   自律動作の開始時に現状の状態を把握する
 * @return
 * @bug Dbg_Res_Print() 未定義関数   →コメントアウト
 * @attention   Dbg_Res_Print() 未定義関数   →コメントアウト
 * @todo    Dbg_Res_Print() 未定義関数   →コメントアウト
 * @bug PowerResetは常にtrue
 */
static uint32_t AT_start(void)
{
    int i;

    uint8_t group_size,group_cnt;
    uint8_t rx_group_size;
    uint8_t MaxRealscan;
//  uint8_t MaxPolling;
    uint8_t *poi;

    uint8_t G_No;
    uint8_t U_No;

    uint8_t rtn;
    uint8_t retry_cnt;
    uint8_t cnt;
    union{
        uint8_t byte[2];
        uint16_t word;
    }b_w_chg;
    uint8_t chk;            // データが破壊されているかどうかを示すレジスタ
    uint8_t wer_batt;

    uint8_t max_unit_work;      // --------------------------------------- リアルスキャンするMAX子機を保存しておく    2016.03.23追加



//    uint8_t PowerReset = 0;               // 未使用のため削除


//  RF_oya_log_clear();                     // 親番号リストの履歴削除
#if CONFIG_NEW_STATE_CTRL                   // 2023.05.26
    first_download_flag_crc_NG("Pre Dump");
    nsc_debug_dump_auto();
#endif //CONFIG_NEW_STATE_CTRL

// 2023.05.26 ↓
//    chk = 0;
//    if(chk_sram_ng() != 0){
//        // SRAMデータが破壊されている場合、データ番号レジスタ＆警報カウンタ・フラグをクリアする
//        PutLog(LOG_RF,"Condition ALL Clear");
//        RF_power_on_init(0);
//        chk = 1;
//		WarningPort((uint16_t)auto_control.backup_cur, (uint16_t)auto_control.backup_now, (uint16_t)auto_control.backup_warn);		// 2020/09/13 segi
//    }
    chk = chk_sram_ng();
    if(chk != 0){
        // SRAMデータが破壊されている場合、データ番号レジスタ＆警報カウンタ・フラグをクリアする
        if(chk == 6){               // 設定変更時
            PutLog(LOG_RF,"Condition ALL Clear");
            RF_power_on_init(1);                        // 記録データNoはクリアしない
        }else{                      // SRAMデータ破壊時
            PutLog(LOG_RF,"Condition ALL Clear 0");
            RF_power_on_init(0);                        // 記録データNoは全クリア
        }
        chk = 1;
		WarningPort((uint16_t)auto_control.backup_cur, (uint16_t)auto_control.backup_now, (uint16_t)auto_control.backup_warn);		// 2020/09/13 segi
    }
// 2023.05.26 ↑
    else{
        //ここに警報LEDの復帰からの点灯

		//auto_control.backup_cur,auto_control.backup_now,auto_control.backup_warn があり得ない値ならクリアする処理を追加する。
		if(auto_control.backup_sum != (auto_control.backup_cur + auto_control.backup_now + auto_control.backup_warn)){
			auto_control.backup_cur = 0;
			auto_control.backup_now = 0;
			auto_control.backup_warn = 0;
			auto_control.backup_sum = 0;
		}
		WarningPort((uint16_t)auto_control.backup_cur, (uint16_t)auto_control.backup_now, (uint16_t)auto_control.backup_warn);		// 2020/09/03 segi
    }

// 2023.05.26 ↓ 
//    /*
//     * TODO  - If koki registration has changed then FDF info has become invalid, so
//     * TODO    first_do_FDF_Init = true;
//     */
//    if(first_do_FDF_Init == true)
//    {
//        Printf("first_do_FDF_Init == true\r\n");
//    }
    if(first_download_flag_crc_NG("For Init"))
    {
//        first_do_FDF_Init = true; // if crc_NG
//    }
//    if(first_do_FDF_Init == true)
//    {
        first_download_flag_clear();
//        first_do_FDF_Init = false;
    }

#if CONFIG_NEW_STATE_CTRL
    nsc_debug_dump_auto();

    nsc_stage = NSC_STAGE_0;

    first_download_flag_crc_NG("PRE nsc_all");
    nsc_all();
    first_download_flag_crc_NG("POST nsc_all");

    nsc_debug_dump_auto();
#endif //CONFIG_NEW_STATE_CTRL
// 2023.05.26 ↑

	// sakaguchi 2020.10.07
    // 登録ファイルから子機の警報監視設定を参照し、警報監視無しの子機の警報情報をクリアする
    Warning_Info_Clear();

    FirstAlarmMoni = 1;                             // 警報監視１回目ＯＮ	// sakaguchi 2021.03.01
    WR_clr_rfcnt();                                 // 無線エラー回数クリア // sakaguchi 2021.04.07

	group_size = get_regist_group_size();			// 登録ファイルからグループ取得する　あわせて登録情報ヘッダを読み込み

    if(1 == (root_registry.auto_route & 0x01)){     // 自動ルート機能ONの場合
        RF_full_moni();                             // フルモニタリング開始
    }


//  PutLog(LOG_RF,"Auto Control Start");

/*
    chk = 0;

    if(chk_sram_ng() != 0){
        // SRAMデータが破壊されている場合、データ番号レジスタ＆警報カウンタ・フラグをクリアする
        PutLog(LOG_RF,"Condition ALL Clear");
        RF_power_on(0);
        chk = 1;
    }
*/

//    else if(PowerReset == 0){   ///< @bug   常にtrue
//    // 警報カウンタをクリアする
//      PutLog(LOG_RF,"Condition Warning Clear");
//        RF_power_on(1);
//    }

    if(chk == 1){     ///
//      PutLog(LOG_RF,"Condition Init");


        // 前回の自律リアルスキャン取得データはここでクリアする
        rf_ram.auto_format1.data_byte[0] = 0;       // FORMAT1 データバイト数クリア
        rf_ram.auto_format1.data_byte[1] = 0;       // FORMAT1 データバイト数クリア


//				group_size = get_regist_group_size();		// 登録ファイルからグループ取得する関数
        for(group_cnt = 0 ; group_cnt < group_size ; group_cnt++){      // グループ数ループ

            RF_buff.rf_req.current_group = (char)(group_cnt + 1);       // 通信対象グループ番号を０～指定

            // ############################################################################################################
            // ##### 現在値のリアルスキャン処理
            retry_cnt = 2;      // リトライ回数
            //set_do_flag_New(rf_autostart,0);      // ########## 初回時のリアルスキャンフラグ設定 ##########
            get_regist_scan_unit(RF_buff.rf_req.current_group, 0x07);       // データ吸上げ | 警報監視 | モニタリング
            max_unit_work = group_registry.max_unit_no;     // --------------------------------------- リアルスキャンするMAX子機を保存しておく    2014.10.14追加
            do{
                MaxRealscan = (uint8_t)(group_registry.altogether + 1); // 最悪時のリアルスキャン回数(中継機数+1(親機))
                while(MaxRealscan){
                    rtn = auto_realscan_New(2);         // この中で無線通信を行っている取得データはＦＯＲＭＡＴ２
                    if(rtn != AUTO_END){

                        if(RF_buff.rf_res.status == RFM_NORMAL_END){                    // 正常終了したらデータをGSM渡し変数にコピー   RF_command_execute の応答が入っている


//-------------
                            if(r500.node == COMPRESS_ALGORITHM_NOTICE)  // 圧縮データを解凍(huge_buffer先頭は0xe0)
                            {
                                //i = r500.data_size - *(char *)&huge_buffer[2];
                                i = r500.data_size - *(uint16_t *)&huge_buffer[2];          // sakaguchi 2021.02.03

                                memcpy(work_buffer, huge_buffer, r500.data_size);
                                r500.data_size = (uint16_t)LZSS_Decode((uint8_t *)work_buffer, (uint8_t *)huge_buffer);     // 2022.06.09

                                if(i > 0)                               // 非圧縮の中継機データ情報があるとき
                                {
                                    memcpy((char *)&huge_buffer[r500.data_size], (char *)&work_buffer[*(uint16_t *)&work_buffer[2]], (size_t)i);
                                }

                                r500.data_size = (uint16_t)(r500.data_size + i);
                                property.acquire_number = r500.data_size;
                                property.transferred_number = 0;

                                r500.node = 0xff;                       // 解凍が重複しないようにする
                            }
//--------------


                            b_w_chg.byte[0] = huge_buffer[0];                           // 無線通信結果メモリを自律処理用変数にコピーする
                            b_w_chg.byte[1] = huge_buffer[1];                           // 無線通信結果メモリを自律処理用変数にコピーする
                            b_w_chg.word = (uint16_t)(b_w_chg.word + 2);
// sakaguchi 2021.03.10
                            //memcpy(rf_ram.realscan.data_byte,huge_buffer,b_w_chg.word);     // MAX=(30byte×128)+2 = 3842
                            if(RF_buff.rf_req.cmd1 == 0x68){
                                memcpy(rf_ram.realscan.data_byte,huge_buffer,b_w_chg.word);     // MAX=(30byte×128)+2 = 3842
                            }
                            else{
                                memcpy(rf_ram.realscan.data_byte,&huge_buffer[2],b_w_chg.word); // MAX=(30byte×128)+2 = 3842
                            }
// sakaguchi 2021.03.10 ↑
                            copy_realscan(1);
                        }
                        group_registry.max_unit_no = max_unit_work; // copy_realscan(1)でMAXを0にしてしまっていたmax_unit_work 2016.03.23修正
                    }
                    else{
                        break;
                    }
                    MaxRealscan--;
                }

                //set_do_flag_New(rf_autostart,1);  // ########## リトライ時のリアルスキャンフラグ設定 ##########
                get_regist_scan_unit_retry();
                group_registry.max_unit_no = max_unit_work;     // --------------------------------------- リアルスキャンするMAX子機を保存しておく    2014.10.14追加
                //regist.group.max_unit_no = max_unit_work;     // --------------------------------------- リアルスキャンするMAX子機を保存しておく    2016.03.23追加
                tx_thread_sleep (70);       // 2010.08.03 リトライ前のWaitをDelayに変更   xxx03
            }while(retry_cnt--);

            //Dbg_Res_Print();        ///< @attention 未定義関数 @todo  未定義関数 @bug 未定義関数

        }               // 2010.08.04


        // 無線通信で得られた    auto_control.download[] の初期値を設定する
        // 無線通信で得られた    auto_control.warning[]  の初期値を設定する
        poi = rf_ram.auto_format1.data_byte;
        b_w_chg.byte[0] = *poi++;
        b_w_chg.byte[1] = *poi++;
        rx_group_size = (uint8_t)(b_w_chg.word / 64);       // 1子機あたりのデータバイト数は64byteで、子機台数を求める
        for(cnt = 0 ; cnt < rx_group_size ; cnt++){
            G_No = *poi;
            poi += 32;
            U_No = *poi;

//---------------------------------------------------------------------------------------------------------------------
//                      poi += 5;                                   // 2011.06.08削除 警報監視スタート時に
            poi += 4;                                   // 2011.06.08追加 子機警報フラグが立っていたら警報とするように
            wer_batt = *poi++;                          // 2011.06.08追加 警報フラグをwer_battに保存する
//---------------------------------------------------------------------------------------------------------------------

            b_w_chg.byte[0] = *poi++;                   // データカウンタ
            b_w_chg.byte[1] = *poi++;                   // データカウンタ
            b_w_chg.word++;                             // xxx15 2010.03.15追加



            b_w_chg.byte[0] = *poi++;                   // CH1 ALM COUNTER
            b_w_chg.byte[1] = *poi++;                   // CH2 ALM COUNTER
            retry_cnt = *poi++;                         // 1ch機か2ch機か判断する。


            Printf("\r\n[RF ]AT_Start G_No = %d, U_No = %d",G_No,U_No);
            Printf("\r\n[RF ]AT_Start CH1_ALM = %d",b_w_chg.byte[0]);
            Printf("\r\n[RF ]AT_Start CH2_ALM = %d",b_w_chg.byte[1]);

//-------------------- ↓ここから 初回の警報監視で警報とする条件の変更の為に追加した部分 ------------------------------
            if((wer_batt & 0b00010000) != 0){   // CH1警報フラグをチェックする
                if(b_w_chg.byte[0] != 0x00){
                    b_w_chg.byte[0]--;          // 警報フラグが立っていたら警報カウンタを減算して初回は警報にする
                }
                else{
                    b_w_chg.byte[0] = 0xff;     //
                }
            }
            if((wer_batt & 0b00100000) != 0){   // CH2警報フラグをチェックする
                if(b_w_chg.byte[1] != 0x00){
                    b_w_chg.byte[1]--;          // 警報フラグが立っていたら警報カウンタを減算して初回は警報にする
                }
                else{
                    b_w_chg.byte[1] = 0xff;     //
                }
            }
//-------------------- ↑ここまで 初回の警報監視で警報とする条件の変更の為に追加した部分 ------------------------------

            WR_set(G_No,U_No,2,b_w_chg.byte[0]);        // CH1 ALM COUNTER保存
            //if((retry_cnt & 0b00001000) != 0){
            if(ch2_check(retry_cnt) != 0){              // 2ch機の判定           // sakaguchi 2021.04.07
                WR_set(G_No,U_No,3,b_w_chg.byte[1]);    // CH2 ALM COUNTER保存
                b_w_chg.byte[0] = (uint8_t)(b_w_chg.byte[0] + b_w_chg.byte[1]); // 2ch機の場合
            }
            WR_set(G_No,U_No,4,b_w_chg.byte[0]);        // ALM COUNTER保存
            poi += 22;


            b_w_chg.byte[0] = WR_chk(G_No,U_No,2);      // CH1 ALM COUNTER
            Printf("\r\n[RF ]AT_Start CH1Read = %d",b_w_chg.byte[0]);

            b_w_chg.byte[0] = WR_chk(G_No,U_No,3);      // CH2 ALM COUNTER
            Printf("\r\n[RF ]AT_Start CH2Read = %d",b_w_chg.byte[0]);

            b_w_chg.byte[0] = WR_chk(G_No,U_No,4);      // CH1+2 ALM COUNTER
            Printf("\r\n[RF ]AT_Start CH1+CH2 = %d",b_w_chg.byte[0]);


        }

    }
    else{
//      PutLog(LOG_RF,"Condition Non Init");
    }

// 2023.05.26 ↓
    /* TODO ?
     *
     * If warning port backup is all 0, Check current warnings and set Warning Port.
     * This will not catch OnOff types but they will time out anyway.
     *
     */
#if  CONFIG_USE_WARNINGPORT_SET_FROM_W_CONFIG_ON
    {
        bool CheckWarning_ON(uint16_t *Cur, uint16_t *Now);

        uint16_t warnCur = 0;
        uint16_t warnNow = 0;
        uint16_t _warnCur = 0;
        uint16_t _warnNow = 0;
        if(CheckWarning_ON(&_warnCur, &_warnCur)) { // ref CheckWarning(
            warnCur |= _warnCur;
            warnNow |= _warnNow;
        }
        /*
        if(CheckWarning_ON_Base(&_warnCur, &_warnCur)) { // ref CheckWarning_Base(
            warnCur |= _warnCur;
            warnNow |= _warnNow;
        }
        */
        if(warnCur & warnNow)
        {
            uint16_t warn16_X;
            warn16_X = 0x0000;
            WarningPort(warnCur, warnNow, warn16_X);
            WarningPort_backup((uint16_t)warnCur, (uint16_t)warnNow, (uint16_t)warn16_X);
        }
    }
#endif

    first_download_flag_crc_NG("\r\n\r\n AT_start END");
// 2023.05.26 ↑
    return ERR(RF , NOERROR);           // ★★★★★★とりあえず
}
//********** end of AT_start_New **********



/**
 * SRAMが壊れていないか判断する
 * @return  0:壊れていない 1:壊れている
 */
uint8_t chk_sram_ng(void)
{
	uint8_t *in_poi;
	int cnt;
	int zero_cnt;
	uint8_t rtn = 0;
	union{
		uint8_t byte[2];
		uint16_t word;
	}b_w_chg;
    uint32_t crc32;

    Printf("### SRAM Check\r\n");

	zero_cnt = 0;
	in_poi = (uint8_t *)&auto_control.download[0].group_no;	// 自動データ吸上げの前回データ番号メモリが初期値か
	for(cnt = 0 ; cnt < 128 ; cnt++){				// 
		b_w_chg.byte[0] = *in_poi++;
		b_w_chg.byte[1] = *in_poi++;
		in_poi += 6;

		if(b_w_chg.word != 0xFFFF){						// 0xFFFFは初期値が残っている可能性ある
			if(b_w_chg.word == 0){						// 0x0000は親機データが残っている可能性あるが、2個以上あればエラー
				zero_cnt++;
				if(zero_cnt > 1){
					rtn = 1;
					break;
				}
			}
			if((b_w_chg.byte[0] > 128)||(b_w_chg.byte[1] > 128)){	// グループ番号か子機番号が128以上ならエラーと見なす
				rtn = 2;
				break;
			}
		}
	}

	if(rtn < 1){									// 上記チェックではSRAMが壊れているとは判断しなかった場合に実行する
		zero_cnt = 0;
		in_poi = &auto_control.w_config[0].group_no;	// 警報メールフラグメモリが初期値か
		for(cnt = 0 ; cnt < 128 ; cnt++){				// 
			b_w_chg.byte[0] = *in_poi++;
			b_w_chg.byte[1] = *in_poi++;
		//	in_poi += 4;
			in_poi += 6;									// w_onoff対応 2012.10.22

			if(b_w_chg.word != 0xFFFF){					// 0xFFFFは初期値が残っている可能性ある
				if(b_w_chg.word == 0){					// 0x0000は親機データが残っている可能性あるが、2個以上あればエラー
					zero_cnt++;
					if(zero_cnt > 1){
						rtn = 3;
						break;
					}
				}
				if((b_w_chg.byte[0] > 128)||(b_w_chg.byte[1] > 128)){	// グループ番号か子機番号が128以上ならエラーと見なす
					rtn = 4;
					break;
				}
			}
		}
	}

	if(rtn < 1){									// CRCのチェックも追加する
		crc32 = crc32_bfr(&auto_control, 5122);		// ((128*8)*5)+2
		if(auto_control.crc != crc32){
            Printf("### Reboot SRAM CRC Error\r\n");
			rtn = 5;
		}
	}

//-------------------------------------------------------------------	2020/09/13 segi
	if(rtn < 1){									// mate_at_start で復帰したチェック
		if(mate_at_start_reset == 1){
			rtn = 6;
		}
	}
	mate_at_start_reset = 0;						// 他の要因でRESETしても常にクリアしておく
//-------------------------------------------------------------------


    Printf("### SRAM Check %d\r\n", rtn);

	return (rtn);
}
//***** end of chk_sram_ng() *****

#if 0
//未使用
/**
 * SRAMが壊れていないか判断し、壊れていたら無線通信用のレジスタクリア
 * @return  0:壊れていない 1:壊れている
 */
static uint8_t SRAM_init(void)
{
	uint8_t rtn;

	rtn = chk_sram_ng();	// 壊れているかチェック[1:壊れている]
	if(rtn != 0){
		RF_power_on(0);
	}
	return (rtn);
}
#endif



/**
 * 自律動作のモニタリング
 * @param sw        1:モニタリングのみ  0:モニタリングと警報
 * @return          データバイト数
 */
uint8_t RF_monitor_execute(uint8_t sw)
{
    int i;

    uint8_t group_size,group_cnt;   // 登録グループ数,グループ個数ループするカウンタ
    uint8_t rtn = 0;
    uint8_t retry_cnt;
    uint8_t MaxRealscan;
    union{
        uint8_t byte[2];
        uint16_t word;
    }b_w_chg;

    uint8_t max_unit_work;      // --------------------------------------- リアルスキャンするMAX子機を保存しておく    2014.10.14追加

/*
    // 前回の自律リアルスキャン取得データはここでクリアする
    rf_ram.auto_format1.data_byte[0] = 0;       // FORMAT1 データバイト数クリア
    rf_ram.auto_format1.data_byte[1] = 0;       // FORMAT1 データバイト数クリア
    rf_ram.auto_format2.data_byte[0] = 0;       // FORMAT2 データバイト数クリア
    rf_ram.auto_format2.data_byte[1] = 0;       // FORMAT2 データバイト数クリア
    rf_ram.auto_format3.data_byte[0] = 0;       // FORMAT3 データバイト数クリア
    rf_ram.auto_format3.data_byte[1] = 0;       // FORMAT3 データバイト数クリア
    rf_ram.auto_rpt_info.data_byte[0] = 0;      // 中継機  データバイト数クリア
    rf_ram.auto_rpt_info.data_byte[1] = 0;      // 中継機  データバイト数クリア
*/

    group_size = get_regist_group_size();       // 登録ファイルからグループ取得する関数
//  group_size = 1;                             // dbg dbg dbg dbg 登録ファイルからグループ取得する関数

    if((root_registry.auto_route & 0x01) == 1){     // 自動ルート機能ONの場合

        if( true == RF_full_moni_start_check(0) ){
            RF_full_moni();                         // 前回失敗している子機がある場合はフルモニタリングを実施
        }

        RF_regu_Make_Required_ScanList();           // 必須スキャンリストの作成
        RF_regu_moni_init();                        // 子機電波強度の初期化
    }

    // 前回の自律リアルスキャン取得データはここでクリアする
    rf_ram.auto_format1.data_byte[0] = 0;       // FORMAT1 データバイト数クリア
    rf_ram.auto_format1.data_byte[1] = 0;       // FORMAT1 データバイト数クリア
    rf_ram.auto_format2.data_byte[0] = 0;       // FORMAT2 データバイト数クリア
    rf_ram.auto_format2.data_byte[1] = 0;       // FORMAT2 データバイト数クリア
    rf_ram.auto_format3.data_byte[0] = 0;       // FORMAT3 データバイト数クリア
    rf_ram.auto_format3.data_byte[1] = 0;       // FORMAT3 データバイト数クリア
    rf_ram.auto_rpt_info.data_byte[0] = 0;      // 中継機  データバイト数クリア
    rf_ram.auto_rpt_info.data_byte[1] = 0;      // 中継機  データバイト数クリア

    memset(&NOISE_CH[0], 0x00, sizeof(NOISE_CH));       // チャンネルビジーフラグクリア     // sakaguchi 2021.01.25
    for(group_cnt = 0 ; group_cnt < group_size ; group_cnt++){

        moni_err[group_cnt] = 0;                                    // 無線通信エラー子機なければ0

        RF_buff.rf_req.current_group = (char)(group_cnt + 1);       // 通信対象グループ番号を０～指定

        // ############################################################################################################
        // ##### 現在値のリアルスキャン処理

        // ##### 初回時のリアルスキャンフラグ設定 #####
        get_regist_scan_unit(RF_buff.rf_req.current_group, 0x03);       // 警報監視 | モニタリング

        max_unit_work = group_registry.max_unit_no;     // --------------------------------------- リアルスキャンするMAX子機を保存しておく    2014.10.14追加

        if((root_registry.auto_route & 0x01) == 1){     // 自動ルート機能ONの場合

            RF_regu_moni_group(RF_buff.rf_req.current_group, 1);        // レギュラーモニタリング（自動）

        }else{

            RF_regu_moni_group_manual(RF_buff.rf_req.current_group, 1); // レギュラーモニタリング（手動）
#if 0
            retry_cnt = 2;      // リトライ回数
            do{
                MaxRealscan = (uint8_t)(group_registry.altogether + 1); // 最悪時のリアルスキャン回数(中継機数+1(親機))
                while(MaxRealscan){
                    rtn = auto_realscan_New(1);         // この中で無線通信を行っている取得データはＦＯＲＭＡＴ１
                    if(rtn != AUTO_END){

                        if(RF_buff.rf_res.status == RFM_NORMAL_END){                    // 正常終了したらデータをGSM渡し変数にコピー   RF_command_execute の応答が入っている








        //-------------
                                if(r500.node == COMPRESS_ALGORITHM_NOTICE)  // 圧縮データを解凍(huge_buffer先頭は0xe0)
                                {
                                    i = r500.data_size - *(char *)&huge_buffer[2];

                                    memcpy(work_buffer, huge_buffer, r500.data_size);
                                    r500.data_size = (uint16_t)LZSS_Decode(work_buffer, huge_buffer);

                                    if(i > 0)                               // 非圧縮の中継機データ情報があるとき
                                    {
                                        memcpy((char *)&huge_buffer[r500.data_size], (char *)&work_buffer[*(uint16_t *)&work_buffer[2]], (size_t)i);
                                    }

                                    r500.data_size = (uint16_t)(r500.data_size + i);
                                    property.acquire_number = r500.data_size;
                                    property.transferred_number = 0;

                                    r500.node = 0xff;                       // 解凍が重複しないようにする
                                }
        //--------------









                            b_w_chg.byte[0] = huge_buffer[0];                           // 無線通信結果メモリを自律処理用変数にコピーする
                            b_w_chg.byte[1] = huge_buffer[1];                           // 無線通信結果メモリを自律処理用変数にコピーする
                            b_w_chg.word = (uint16_t)(b_w_chg.word + 2);

                        //  memcpy(rf_ram.realscan.data_byte,huge_buffer,b_w_chg.word);     // MAX=(30byte×128)+2 = 3842

                            if(RF_buff.rf_req.cmd1 == 0x68){
                            memcpy(rf_ram.realscan.data_byte,huge_buffer,b_w_chg.word);     // MAX=(30byte×128)+2 = 3842
                            }
                            else{
                                memcpy(rf_ram.realscan.data_byte,&huge_buffer[2],b_w_chg.word); // MAX=(30byte×128)+2 = 3842
                            }
                            copy_realscan(1);

                            group_registry.max_unit_no = max_unit_work; // copy_realscan(1)でMAXを0にしてしまっていたmax_unit_work 2016.03.23修正
                        }
                    }
                    else{
                        break;
                    }
                    MaxRealscan--;
                }

            //  set_do_flag(rf_autostart,1);    // ########## リトライ時のリアルスキャンフラグ設定 ##########
            //  get_regist_scan_unit(RF_buff.rf_req.current_group, 0x03);       // 警報監視 | モニタリング    まだリトライ時のなっていない
                get_regist_scan_unit_retry();
                group_registry.max_unit_no = max_unit_work;     // --------------------------------------- リアルスキャンするMAX子機を保存しておく    2014.10.14追加
                //Delay(700);                   // 2010.08.03 リトライ前のWaitをDelayに変更   xxx03
                tx_thread_sleep (70);       // 2010.08.03 リトライ前のWaitをDelayに変更   xxx03
            }while(retry_cnt--);
#endif

        }





        moni_err[group_cnt] = realscan_err_data_add();      // 受信すべきなのに受信しなかった場合に、無線通信エラー子機のダミーデータを追加

        realscan_data_para(1);              // xxx25 2009.08.25追加 リアルスキャンデータに(自律用)子機設定フラグを追記する

        if(sw == 0){        // sw = 0 の時だけ警報監視する
            // ############################################################################################################
            // ##### 警報のリアルスキャン処理
            retry_cnt = 2;      // リトライ回数
            set_keiho_do_flag_New(0);           // ########## 初回時の警報リアルスキャンフラグ設定 ##########
            group_registry.max_unit_no = max_unit_work;     // --------------------------------------- リアルスキャンするMAX子機を保存しておく    2014.10.14追加
            do{
                MaxRealscan = (uint8_t)(group_registry.altogether + 1); // 最悪時のリアルスキャン回数(中継機数+1(親機))
                while(MaxRealscan){
                    rtn = auto_realscan_New(2);         // この中で無線通信を行っている取得データはＦＯＲＭＡＴ２
                    if(rtn != AUTO_END){

                        if(RF_buff.rf_res.status == RFM_NORMAL_END){                    // 正常終了したらデータをGSM渡し変数にコピー   RF_command_execute の応答が入っている



//-------------
                            if(r500.node == COMPRESS_ALGORITHM_NOTICE)  // 圧縮データを解凍(huge_buffer先頭は0xe0)
                            {
                                //i = r500.data_size - *(char *)&huge_buffer[2];
                                i = r500.data_size - *(uint16_t *)&huge_buffer[2];      // sakaguchi 2021.02.03

                                memcpy(work_buffer, huge_buffer, r500.data_size);
                                r500.data_size = (uint16_t)LZSS_Decode((uint8_t *)work_buffer, (uint8_t *)huge_buffer);     // 2022.06.09

                                if(i > 0)                               // 非圧縮の中継機データ情報があるとき
                                {
                                    memcpy((char *)&huge_buffer[r500.data_size], (char *)&work_buffer[*(uint16_t *)&work_buffer[2]], (size_t)i);
                                }

                                r500.data_size = (uint16_t)(r500.data_size + i);
                                property.acquire_number = r500.data_size;
                                property.transferred_number = 0;

                                r500.node = 0xff;                       // 解凍が重複しないようにする
                            }
//--------------






                            b_w_chg.byte[0] = huge_buffer[0];                           // 無線通信結果メモリを自律処理用変数にコピーする
                            b_w_chg.byte[1] = huge_buffer[1];                           // 無線通信結果メモリを自律処理用変数にコピーする
                            b_w_chg.word = (uint16_t)(b_w_chg.word + 2);


                        //  memcpy(rf_ram.realscan.data_byte,huge_buffer,b_w_chg.word);     // MAX=(30byte×128)+2 = 3842

                            if(RF_buff.rf_req.cmd1 == 0x68){
                                memcpy(rf_ram.realscan.data_byte,huge_buffer,b_w_chg.word);     // MAX=(30byte×128)+2 = 3842
                            }
                            else{
                                memcpy(rf_ram.realscan.data_byte,&huge_buffer[2],b_w_chg.word); // MAX=(30byte×128)+2 = 3842
                            }
                            copy_realscan(2);

                            group_registry.max_unit_no = max_unit_work; // copy_realscan(1)でMAXを0にしてしまっていたmax_unit_work 2016.03.23修正
                        }
                    }
                    else{
                        break;
                    }
                    MaxRealscan--;
                }
                set_keiho_do_flag_New(1);       // ########## リトライ時の警報リアルスキャンフラグ設定 ##########
                group_registry.max_unit_no = max_unit_work;     // --------------------------------------- リアルスキャンするMAX子機を保存しておく    2014.10.14追加
                //Delay(700);                   // 2010.08.03 リトライ前のWaitをDelayに変更   xxx03
                tx_thread_sleep (70);       // 2010.08.03 リトライ前のWaitをDelayに変更   xxx03
            }while(retry_cnt--);

            // 警報監視データに登録ファイルから抜き出した記録属性を付加する
            //keiho_data_ch_zoku();
            realscan_data_para(2);              //


//-------------------- ↓ここから RTR-574対応の為に追加した部分 -------------------------------------------------------------------------------

            // ############################################################################################################
            // ##### 積算警報のリアルスキャン処理
            retry_cnt = 2;      // リトライ回数
            set_sekisan_do_flag_New(0);         // ########## 初回時の積算警報リアルスキャンフラグ設定 ##########
            group_registry.max_unit_no = max_unit_work;     // --------------------------------------- リアルスキャンするMAX子機を保存しておく    2014.10.14追加
            do{
                MaxRealscan = (uint8_t)(group_registry.altogether + 1); // 最悪時のリアルスキャン回数(中継機数+1(親機))
                while(MaxRealscan){
                    rtn = auto_realscan_New(3);         // この中で無線通信を行っている取得データはＦＯＲＭＡＴ３
                    if(rtn != AUTO_END){

                        if(RF_buff.rf_res.status == RFM_NORMAL_END){                    // 正常終了したらデータをGSM渡し変数にコピー   RF_command_execute の応答が入っている

//-------------
                            if(r500.node == COMPRESS_ALGORITHM_NOTICE)  // 圧縮データを解凍(huge_buffer先頭は0xe0)
                            {
                                //i = r500.data_size - *(char *)&huge_buffer[2];
                                i = r500.data_size - *(uint16_t *)&huge_buffer[2];          // sakaguchi 2021.02.03

                                memcpy(work_buffer, huge_buffer, r500.data_size);
                                r500.data_size = (uint16_t)LZSS_Decode((uint8_t *)work_buffer, (uint8_t *)huge_buffer);     // 2022.06.09

                                if(i > 0)                               // 非圧縮の中継機データ情報があるとき
                                {
                                    memcpy((char *)&huge_buffer[r500.data_size], (char *)&work_buffer[*(uint16_t *)&work_buffer[2]], (size_t)i);
                                }

                                r500.data_size = (uint16_t)(r500.data_size + i);
                                property.acquire_number = r500.data_size;
                                property.transferred_number = 0;

                                r500.node = 0xff;                       // 解凍が重複しないようにする
                            }
//--------------

                            b_w_chg.byte[0] = huge_buffer[0];                           // 無線通信結果メモリを自律処理用変数にコピーする
                            b_w_chg.byte[1] = huge_buffer[1];                           // 無線通信結果メモリを自律処理用変数にコピーする
                            b_w_chg.word = (uint16_t)(b_w_chg.word + 2);

                        //  memcpy(rf_ram.realscan.data_byte, huge_buffer,b_w_chg.word);        // MAX=(30byte×128)+2 = 3842

                            if(RF_buff.rf_req.cmd1 == 0x68){
                                memcpy(rf_ram.realscan.data_byte,huge_buffer,b_w_chg.word);     // MAX=(30byte×128)+2 = 3842
                            }
                            else{
                                memcpy(rf_ram.realscan.data_byte,&huge_buffer[2],b_w_chg.word); // MAX=(30byte×128)+2 = 3842
                            }
                            copy_realscan(3);

                            group_registry.max_unit_no = max_unit_work; // copy_realscan(1)でMAXを0にしてしまっていたmax_unit_work 2016.03.23修正
                        }
                    }
                    else{
                        break;
                    }
                    MaxRealscan--;
                }
                set_sekisan_do_flag_New(1);     // ########## リトライ時の積算警報リアルスキャンフラグ設定 ##########
                group_registry.max_unit_no = max_unit_work;     // --------------------------------------- リアルスキャンするMAX子機を保存しておく    2014.10.14追加
                //Delay(700);                   // 2010.08.03 リトライ前のWaitをDelayに変更   xxx03
                tx_thread_sleep (70);       // 2010.08.03 リトライ前のWaitをDelayに変更   xxx03
            }while(retry_cnt--);

            // 警報監視データに登録ファイルから抜き出した記録属性を付加する
            //keiho_data_ch_zoku();
            realscan_data_para(3);              //

//-------------------- ↑ここまで -------------------------------------------------------------------------------------------------------------
        }
        else{
            rf_ram.auto_format2.data_byte[0] = 0x00;        // 警報監視をしない場合(モニタリングのみ)
            rf_ram.auto_format2.data_byte[1] = 0x00;        // 警報監視をしない場合(モニタリングのみ)
//-------------------- ↓ここから RTR-574対応の為に追加した部分 -------------------------------------------------------------------------------
            rf_ram.auto_format3.data_byte[0] = 0x00;        // 警報監視をしない場合(モニタリングのみ)
            rf_ram.auto_format3.data_byte[1] = 0x00;        // 警報監視をしない場合(モニタリングのみ)
//-------------------- ↑ここまで -------------------------------------------------------------------------------------------------------------
        }


    }       // for(group_cnt = 0 ; group_cnt < group_size ; group_cnt++){       // グループ数ループ


    w_config_data_make();           // 警報メールを出す材料、前回と今回の警報状態を保存する

    ALM_bak_write();                // 警報情報をバックアップする

	if(1 == (root_registry.auto_route & 0x01)){		// 自動ルート機能ONの場合
        if((VENDER_HIT != fact_config.Vender) && (Http_Use == HTTP_SEND)){
            G_HttpFile[FILE_I].sndflg = SND_ON;             // [FILE]機器状態送信：ON
        }
    }


    return(rtn);
}



//-------------------------------------
/**
 *
 */
void get_regist_scan_unit_retry(void)
{
    uint8_t cnt;
    uint8_t chk;

    for(cnt = 0; cnt < 16 ; cnt++){
        realscan_buff.do_unit[cnt] = realscan_buff.do_unit[cnt] ^ realscan_buff.over_unit[cnt]; // .do_unit にあったが、over_unit. にない子機Noを .do_unit に設定する。
    }

    memset(realscan_buff.do_rpt,0x00, sizeof(realscan_buff.do_rpt));
    memset(realscan_buff.over_rpt,0x00,sizeof(realscan_buff.over_rpt));

    for(cnt = 1; cnt < 128; cnt++){
        chk = check_flag(realscan_buff.do_unit, cnt);       // 0以外は受信済みの時

        if(chk != 0){
            get_regist_relay_inf(RF_buff.rf_req.current_group, cnt, rpt_sequence);
            set_flag(realscan_buff.do_rpt,ru_registry.rtr501.superior);
        }
    }
}

//--------------------------------------



/**
 * 前回と今回の警報状態を取得する(警報フラグを16bitに変更したバージョン)
 * @param G_No  グループ番号。登録ファイルの先頭からの順番。(0～)
 * @param U_No  子機番号。(1～)
 * @return  L 今回の警報状態
 *          H 前回の警報状態
 * @note    構造体のメンバにon_offを追加した事への対応版
 */
static uint32_t chk_w_config(uint8_t G_No, uint8_t U_No)
{
    int cnt = 0;
    uint8_t *poi;
    union{
        uint8_t byte[4];
        uint16_t word[2];
        uint32_t long_word;
    }b_w_chg;

    poi = &auto_control.w_config[0].group_no;

    b_w_chg.long_word = 0;              // 該当データが無い場合の応答は0x00000000
    do{
        if(*poi == G_No){                   // グループ番号一致を判断
            poi++;
            if(*poi == U_No){               // 子機番号一致を判断
                poi++;
                b_w_chg.byte[0] = *poi++;   // 現状の値を取得
                b_w_chg.byte[1] = *poi++;   //      "
                b_w_chg.byte[2] = *poi++;   // 現状の値を取得
                b_w_chg.byte[3] = *poi++;   //      "

                poi += 2;                           // w_onoff対応 2012.10.22

                break;
            }
            //else poi += 5;
            else{
                poi += 7;                           // w_onoff対応 2012.10.22
            }
        }
        //else poi+= 6;
        else{
            poi+= 8;                                // w_onoff対応 2012.10.22
        }
        cnt++;
    }while(cnt < 128);

    return (b_w_chg.long_word);
}
//***** end of chk_w_config() **********





/**
 * @brief   今回の警報状態を保存する関数。(警報フラグを16bitに変更したバージョン)
 * @param G_No  グループ番号。登録ファイルの先頭からの順番。(0～)
 * @param U_No  子機番号。(1～)
 * @param Dat   今回の警報データデータ0x[1][0]
 * @param sw    0:前回データを更新し、今回データを書き込む  1:前回データに今回データを追記する
 * @return      書き込めた場合
 *               L byte:今回書き込んだデータ
 *               H byte:前回書き込んだデータ
 *               書き込む場所がない場合 0x00000000
 * @note  構造体のメンバにon_offを追加した事への対応版
 */
static uint32_t set_w_config(uint8_t G_No , uint8_t U_No , uint16_t Dat , uint8_t sw)
{
    uint16_t memo_no = ERROR_END;   // ERROR_END = 0xf0
    uint8_t *poi;
    uint8_t *poi_bak;
    int cnt = 0;
    union{
        uint8_t byte[4];
        uint16_t word[2];
        uint32_t long_word;
    }b_w_chg;
    b_w_chg.long_word = 0;

    poi = &auto_control.w_config[0].group_no;
    poi_bak = poi;

    do{
        if(*poi == G_No){                   // グループ番号一致を判断
            poi++;
            if(*poi == U_No){               // 子機番号一致を判断
                poi++;
                b_w_chg.byte[0] = *poi++;   // 現状の値を取得
                b_w_chg.byte[1] = *poi++;   //      "
                b_w_chg.byte[2] = *poi++;   // 現状の値を取得
                b_w_chg.byte[3] = *poi++;   //      "

                poi += 2;                               // w_onoff対応 2012.10.22

                memo_no = (uint16_t)cnt;
                break;
            }
            //else poi += 5;
            else{
                poi += 7;                               // w_onoff対応 2012.10.22
            }
        }
        else if(*poi == 0xff){
            if(memo_no == ERROR_END){
                b_w_chg.long_word = 0;          // 新規に書き込む時の初期値
                memo_no = (uint16_t)cnt;
            }
            poi += 8;                                   // w_onoff対応 2012.10.22
        }
        else{
            poi += 8;                                   // w_onoff対応 2012.10.22
        }
        cnt++;
    }while(cnt < 128);


    if(memo_no != ERROR_END){
        poi = poi_bak;
        poi = poi + (memo_no * 8);                      // w_onoff対応 2012.10.22

        if(sw == 0){
            b_w_chg.word[1] = b_w_chg.word[0];          // 今回の値→前回の値にセットする
            b_w_chg.word[0] = Dat;                      // 引数の値→今回の値にセットする
        }
        else if(sw == 1){
            b_w_chg.word[0] = b_w_chg.word[0] | Dat;    // 今回の値に追記する
        }
        else{
            //b_w_chg.word[0] = b_w_chg.word[0] & 0xfcff; // CH3,CH4の警報のみクリア
            b_w_chg.word[0] = b_w_chg.word[0] & 0xf8ff; // CH3,CH4の警報(センサエラー含む)のみクリア    // sakaguchi 2021.04.13
            b_w_chg.word[0] = b_w_chg.word[0] | Dat;    // 今回の値に追記する
        }


        *poi++ = G_No;
        *poi++ = U_No;
        *poi++ = b_w_chg.byte[0];       // [0]byte は今回のデータ[0]
        *poi++ = b_w_chg.byte[1];       // [1]byte は今回のデータ[1]
        *poi++ = b_w_chg.byte[2];       // [2]byte は前回のデータ[0]
        *poi++ = b_w_chg.byte[3];       // [3]byte は前回のデータ[1]
    }
	auto_control.crc = crc32_bfr(&auto_control, 5122);		// ((128*8)*5)+2	CRC追加	2020/08/22 segi
    return (b_w_chg.long_word);
}
//***** end of set_w_config() **********



/**
 *
 * @param G_No
 * @param U_No
 * @return
 */
static uint16_t w_onoff_read(uint8_t G_No , uint8_t U_No)
{
    int cnt = 0;
    uint8_t *poi;
    union{
        uint8_t byte[2];
        uint16_t word;
    }b_w_chg;

    poi = &auto_control.w_config[0].group_no;

    b_w_chg.word = 0;                   // 該当データが無い場合の応答は0x0000
    do{
        if(*poi == G_No){                   // グループ番号一致を判断
            poi++;
            if(*poi == U_No){               // 子機番号一致を判断
                poi += 5;
                b_w_chg.byte[0] = *poi++;   // 継続/遮断フラグを取得
                b_w_chg.byte[1] = *poi++;   //      "
                break;
            }
            else{
                poi += 7;                           // w_onoff対応 2012.10.22
            }
        }
        else{
            poi+= 8;                                // w_onoff対応 2012.10.22
        }
        cnt++;
    }while(cnt < 128);

    return (b_w_chg.word);

}



/**
 *
 * @param G_No
 * @param U_No
 * @param Dat
 * @return
 */
static uint8_t w_onoff_write(uint8_t G_No, uint8_t U_No, uint16_t Dat)
{
    uint8_t rtn = ERROR_END;    // ERROR_END = 0xf0
    uint8_t *poi;
    int cnt = 0;
    union{
        uint8_t byte[2];
        uint16_t word;
    }b_w_chg;

    poi = &auto_control.w_config[0].group_no;
    do{
        if(*poi == G_No){                   // グループ番号一致を判断
            poi++;
            if(*poi == U_No){               // 子機番号一致を判断
                poi += 7;                               // w_onoff対応 2012.10.22
                rtn = (uint8_t)cnt;
                break;
            }
            else{
                poi += 7;                               // w_onoff対応 2012.10.22
            }
        }
        else if(*poi == 0xff){
            if(rtn == ERROR_END){
                rtn = (uint8_t)cnt;
            }
            poi += 8;                                   // w_onoff対応 2012.10.22
        }
        else{
            poi += 8;                                   // w_onoff対応 2012.10.22
        }
        cnt++;
    }while(cnt < 128);


    if(rtn != ERROR_END){
        poi = &auto_control.w_config[rtn].group_no;

        b_w_chg.word = Dat;                     // 引数の値→今回の値にセットする

        *poi++ = G_No;
        *poi++ = U_No;
        poi += 4;
        *poi++ = b_w_chg.byte[0];       // [2]byte は前回のデータ[0]
        *poi++ = b_w_chg.byte[1];       // [3]byte は前回のデータ[1]
    }
	auto_control.crc = crc32_bfr(&auto_control, 5122);		// ((128*8)*5)+2	CRC追加	2020/08/22 segi
    return (rtn);
}



/**
 * 子機の警報をチェックする
 * 自律動作でとリアルスキャンをして子機から得られたformat1-3のデータから
 * 警報メールを出す為の警報フラグのON/OFFを行う。
 */
static void w_config_data_make(void)
{
    uint8_t *out_poi;
    uint8_t G_No;
    uint8_t U_No;
    uint8_t U_No_temp;

    uint8_t reg_kind;           // 子機登録情報
    uint8_t reg_ch1_zoku_0;     // 記録属性
    uint8_t reg_ch2_zoku_0;     // 記録属性(CH2,CH4)           // sakaguchi 2021.04.13

    uint8_t BATT;
    //uint8_t BATT_ref = 3;       // 電池警報を出す基準レベル     // 2012.02.10 電池警報基準変更
    uint8_t BATT_ref = 4;       // 電池警報を出す基準レベル     // 2022.04.11 電池警報基準レベルを3⇒4に変更

	uint8_t kisyu_code = 0;

    uint8_t RFNG;
    uint8_t FLAGS;
    uint8_t ALM;                // 警報フラグ    //  bit7    bit6    bit5    bit4    bit3    bit2    bit1    bit0
                                                //  ---     ---     CH2ALM  CH1ALM  ---     ---     ---     ---

    uint8_t ALM1;               // CH1警報カウンタ
    uint8_t ALM2;               // CH2警報カウンタ

    uint8_t BAT_ALM_cmp = 5;    // 電池警報にする回数
    uint16_t war_Int;
	uint8_t ALMcnt_all_backup = 0;

    uint16_t ALM16bit;
    uint16_t ALM16bit_cmp;

    uint16_t SetFlag;
    uint16_t SetFlag_576 = 0;   // sakaguchi 2021.04.13
//  uint8_t BattChkON = 0;
    union{
        uint8_t byte[2];
        uint16_t word;
    }b_w_chg;

    union{                      // sakaguchi 2021.04.13
        uint8_t byte[2];
        uint16_t word;
    }b_w_chg2;

    uint16_t loop_size;
    int cnt;

    uint16_t w_onof_temp = 0;   // 今回の警報フラグの状態(ALM/BATフラグ)、2in1機種の場合2回設定する為の1時バッファ。
    uint16_t w_onof_back;       // 現状の警報onoffフラグを読み出す。
    uint16_t w_onof_onoff;      // 今回の警報フラグの状態(ALM/BATフラグ)を反転した値(警報解除の時にHi)
    uint16_t w_onof_before;     // 前回の警報フラグ
    uint16_t w_onof_A;          // w_onof_A = w_onof_back & w_onof_before
    uint16_t w_onof_B;          // w_onof_B = /w_onof_before & w_onof_onoff
    uint16_t w_onof_C;          //

    uint32_t Now_GMT;

//	uint8_t sens_err_GNo = 0;
//	uint8_t sens_err_UNo = 0;
//	uint8_t sens_err = 0;       // 未使用のため削除
//	uint32_t sens_chk;          // 未使用のため削除
    uint8_t	limiter_flag = 0;   // 警報監視条件フラグ // sakaguchi 2021.04.07

// 警報種類
//  bit7    bit6    bit5    bit4    bit3    bit2    bit1    bit0
//  input   batt    rf_ng   senc    ex_pow  ----    w_ch2   w_ch1

//  bit15   bit14   bit13   bit12   bit11   bit10   bit9    bit8
//  ----    ----    ch2_mp  ch1_mp  ----    ----    ch4_wr  ch3_wr


    //
    //      子機の警報をチェックする
    //*************************************************************
    //********** FORMAT1データをチェックする
    /*
    b_w_chg.byte[0] = rf_ram.auto_format1.data_byte[0]; // FORMAT1データバイト数
    b_w_chg.byte[1] = rf_ram.auto_format1.data_byte[1]; // FORMAT1データバイト数
    loop_size = b_w_chg.word;
    */
    loop_size = *(uint16_t *)&rf_ram.auto_format1.data_byte[0]; // FORMAT1データバイト数

    out_poi = &rf_ram.auto_format1.kdata[0].group_no;       // データ読み出しポインタ設定
    for(cnt = 0 ; cnt < loop_size ; cnt+=64){
        G_No = *out_poi;                        // グループ番号を取得
        out_poi += 6;
        RFNG = ((*out_poi == 0xcc)||(*out_poi == 0xcb)) ? 1 : 0; // 今回無線エラー/今回無線通信成功
        out_poi++;                                                                              // xxx25 2009.08.25追加
        FLAGS = *out_poi;                       // 自律用フラグを取得する                      // xxx25 2009.08.25追加

        out_poi += 1;
        reg_kind = *out_poi;                    // 子機情報種類を取得する(0xFE,0xFA,0xF8,0xF9など)
        out_poi += 2;
        reg_ch1_zoku_0 = *out_poi;              // 子機記録属性を取得する
        //out_poi += 22;
        out_poi += 1;
        reg_ch2_zoku_0 = *out_poi;              // 子機記録属性を取得する(CH2,CH4)      // sakaguchi 2021.04.13
        out_poi += 21;
        

        U_No = *out_poi;                        // 子機番号を取得
        out_poi += 4;
        BATT = *out_poi;                        // 電池残量
        ALM = BATT;
//Printf("\r\n\r\n### [RF ] ALM org=%02X / G_No=%d  U_No=%d", ALM, G_No,U_No);
        BATT = BATT & 0x0f;                     // 電池残量のLSB 4bitのみ取得
        ALM = ALM & 0xf0;                       // 警報フラグが入っている場所を抜き出す
        ALM = ALM >> 4;                         // ビットの示す情報は [--- --- --- --- MP2 MP1 CH2 CH1] となる

        ALM16bit = (uint16_t)ALM;
//Printf("\r\n### [RF ] ALM16bit=%04X ", ALM16bit);

// sakaguchi 2021.04.13 RTR574,RTR576のCH1とCH3の現在値が取得できていないため修正
        //out_poi += 24;                          // 最新のデータでセンサエラーを判定する
        //b_w_chg.byte[0] = *out_poi++;           // 現在値を取得する(センサエラー把握の為)
        //b_w_chg.byte[1] = *out_poi++;           // 現在値を取得する(センサエラー把握の為)
        //out_poi += 2;
        out_poi += 14;                          // 最新のデータでセンサエラーを判定する
        b_w_chg.byte[0] = *out_poi++;           // CH1,CH3の現在値を取得する(センサエラー把握の為)
        b_w_chg.byte[1] = *out_poi++;           // CH1,CH3の現在値を取得する(センサエラー把握の為)
        out_poi += 8;
        b_w_chg2.byte[0] = *out_poi++;          // CH2,CH4の現在値を取得する(センサエラー把握の為)
        b_w_chg2.byte[1] = *out_poi++;          // CH2,CH4の現在値を取得する(センサエラー把握の為)
        out_poi += 2;
// sakaguchi 2021.04.13 ↑

        U_No_temp = w_unit_No(U_No,reg_kind,reg_ch1_zoku_0);
        if(U_No_temp != U_No){          // [ch3]か[ch4]の警報の場合
            ALM16bit = (uint16_t)(ALM16bit << 8);
            ALM16bit_cmp = 0xfcff;      // [ch3][ch4]の警報フラグ位置をクリア
        }
        else{
            ALM16bit_cmp = 0xfffc;      // [ch1][ch2]の警報フラグ位置をクリア
        }


        w_onof_temp = (uint16_t)(w_onof_temp | (ALM16bit & ~ALM16bit_cmp));



        if((FLAGS & 0x01) != 0){    // 警報監視子機の場合のみ以下を実行する               // xxx25 2009.08.25追加

                SetFlag = (uint16_t)chk_w_config(G_No,U_No_temp);       // 前回の警報状況を読み出す

                SetFlag = (uint16_t)(SetFlag & ~(WR_UNIT_xx));              // ★★★★★ 予備エラーbit OFF


                SetFlag = (uint16_t)(SetFlag & ~(WR_UNIT_RFNG));            // ★★★★★ 無線通信エラーbit OFF
                if(RFNG == 1){
                    if(WR_inc(G_No , U_No , 1) > 2){            // 無線エラーカウンタアップ
                        SetFlag = SetFlag | WR_UNIT_RFNG;       // ★★★★★ 無線通信エラーbit ON
                    }
                }
                else{
                    // 以下は無線通信が成功したときのみ通過する
                    WR_set(G_No , U_No , 1 , 0);                    // 無線エラーカウンタクリア


                    SetFlag = SetFlag & (ALM16bit_cmp | ALM16bit);  // [ch3]か[ch4] 前回上下限警報の場合、フラグ継続中なら警報とする。

//                    SetFlag = (sensor_check(b_w_chg.word , reg_ch1_zoku_0) == 1) ? (SetFlag | WR_UNIT_SENS) : (uint16_t)(SetFlag & ~(WR_UNIT_SENS));    // ★★★★★ センサーエラーbit ON/ センサーエラーbit OFF

                    if((reg_kind == 0xFA)||(reg_kind == 0xF9)){			// RTR-574,RTR-576の場合
//						SetFlag = (sensor_check(b_w_chg.word , reg_ch1_zoku_0) == 1) ? (SetFlag | WR_UNIT_SENS) : (uint16_t)(SetFlag & ~(WR_UNIT_SENS));    // ★★★★★ センサーエラーbit ON/ センサーエラーbit OFF

// sakaguchi 2021.04.13 ↓
                        get_regist_relay_inf(G_No, U_No, rpt_sequence);         // 子機情報取得 regist.unit.size[0]～
                        limiter_flag = ru_registry.rtr574.limiter_flag;         // 警報監視条件フラグ

                        if(U_No_temp == U_No){              // [ch1][ch2]の場合
                            if(reg_kind == 0xFA){           // RTR-574だけチェックする
                                if(sensor_check(b_w_chg.word , reg_ch1_zoku_0) == 1){
                                    if(limiter_flag & 0x03){                // [ch1]の警報監視有りの場合
                                        SetFlag = SetFlag | WR_UNIT_SENS;   // [ch1]センサエラー発生
                                    }
                                }
                                else if(sensor_check(b_w_chg2.word , reg_ch2_zoku_0) == 1){
                                    if(limiter_flag & 0x0C){                // [ch2]の警報監視有りの場合
                                        SetFlag = SetFlag | WR_UNIT_SENS;   // [ch2]センサエラー発生
                                    }
                                }
                                else{
                                    SetFlag = (uint16_t)(SetFlag & ~(WR_UNIT_SENS));    // [ch1][ch2]センサエラー復帰
                                }
                            }
                        }
                        else{                              // [ch3][ch4]の場合
                            if(sensor_check(b_w_chg.word , reg_ch1_zoku_0) == 1){
                                if(limiter_flag & 0x30){                // [ch3]の警報監視有りの場合
                                    SetFlag = SetFlag | WR_UNIT_SENS2;  // [ch3]センサエラー発生
                                }
                            }
                            else if(sensor_check(b_w_chg2.word , reg_ch2_zoku_0) == 1){
                                if(limiter_flag & 0xC0){                // [ch4]の警報監視有りの場合
                                    SetFlag = SetFlag | WR_UNIT_SENS2;  // [ch4]センサエラー発生
                                }
                            }
                            else{
                                SetFlag = (uint16_t)(SetFlag & ~(WR_UNIT_SENS2));    // [ch3][ch4]センサエラー復帰
                            }
                        }
#if 0
						if(sensor_check(b_w_chg.word , reg_ch1_zoku_0) == 1){
// RTR-574,RTR-576のセンサエラー見直し    // sakaguchi 2021.04.07
                            get_regist_relay_inf(G_No, U_No, rpt_sequence);         // 子機情報取得 regist.unit.size[0]～
                            limiter_flag = ru_registry.rtr574.limiter_flag;         // 警報監視条件フラグ
                            if(U_No_temp != U_No){          		    // [ch1][ch2]or[ch3][ch4]の判断
                                if(limiter_flag & 0xF0){                // [ch3][ch4]の警報監視有りの場合
                                    SetFlag = SetFlag | WR_UNIT_SENS2;  // [ch3][ch4]センサエラー発生
                                }
                            }else{
                                if(limiter_flag & 0x0F){                // [ch1][ch2]の警報監視有りの場合
                                    SetFlag = SetFlag | WR_UNIT_SENS;   // [ch1][ch2]センサエラー発生
                                }
                            }
					//		SetFlag = SetFlag | WR_UNIT_SENS;

					//		if(U_No_temp != U_No){          		// [ch1][ch2]or[ch3][ch4]の判断
					//			SetFlag = SetFlag | WR_UNIT_CH3;	// [ch3][ch4]の場合
					//		}
					//		else{									// [ch1][ch2]の場合
					//		}
						}
						else{
                            if(U_No_temp != U_No){          		// [ch1][ch2]or[ch3][ch4]の判断
                                SetFlag = (uint16_t)(SetFlag & ~(WR_UNIT_SENS2));   // [ch3][ch4]センサエラー復帰
                            }else{
                                SetFlag = (uint16_t)(SetFlag & ~(WR_UNIT_SENS));    // [ch1][ch2]センサエラー復帰
                            }

					//		if((SetFlag & WR_UNIT_SENS) != 0){
					//			if(U_No_temp != U_No){          		// [ch1][ch2]or[ch3][ch4]の判断
					//				SetFlag = (uint16_t)(SetFlag & ~(WR_UNIT_CH3));	// 上下限警報状態からセンサエラーになって復帰した場合
					//				SetFlag = (uint16_t)(SetFlag & ~(WR_UNIT_CH4));	// 上下限警報状態からセンサエラーになって復帰した場合
					//			}
					//			else{									// [ch1][ch2]の場合
					//				SetFlag = (uint16_t)(SetFlag & ~(WR_UNIT_CH1));	// 上下限警報状態からセンサエラーになって復帰した場合
					//				SetFlag = (uint16_t)(SetFlag & ~(WR_UNIT_CH2));	// 上下限警報状態からセンサエラーになって復帰した場合
					//			}
					//		}

					//		SetFlag = (uint16_t)(SetFlag & ~(WR_UNIT_SENS));
						}
#endif
// sakaguchi 2021.04.13 ↑

                        BATT_ref = 4;       // xxx03 子機の電池警報レベルが機種ごとに異なる事に対応 2010.08.03
                    }
                                                                                // xxx05 2011.04.05 RTR-576の電池NG基準は5以上(CO2測定限界値)

                    if(BATT > BATT_ref){                        // 子機電池警報の基準は3以上で良いか決定する。


                        war_Int = *(uint16_t *)my_config.warning.Interval;
                        if(war_Int < 5){
                            BAT_ALM_cmp = (uint8_t)(6 - (uint8_t)war_Int);
                            BAT_ALM_cmp = (uint8_t)((BAT_ALM_cmp * BAT_ALM_cmp) + 2);   // 1分*27回=27分、2分*18回=36分、3分*11回=33分、4分*6回=24分
                        }
						BAT_ALM_cmp = BAT_ALM_cmp * 2;				// 電池警報にする回数を倍にする(30分狙いを60分狙いに)	2020/09/10 segi

                    //  SetFlag = (WR_inc(G_No , U_No , 0) > 5) ? (SetFlag | WR_BATT) : (uint16_t)(SetFlag & ~(WR_BATT));        // ★★★★★ 電池警告bit ON/電池警告bit OFF
                        SetFlag = (WR_inc(G_No , U_No , 0) > BAT_ALM_cmp) ? (SetFlag | WR_BATT) : (uint16_t)(SetFlag & ~(WR_BATT));        // ★★★★★ 電池警告bit ON/電池警告bit OFF

                    }

//---------- 2011.09.26 電池警報にヒステリシス追加 ----- ↓ここから ---------------------------------------------------------------
                    else{
                        if((SetFlag & WR_BATT) != 0){           // ######### 前回、電池警報になっていた場合 #########
                            //if(BATT < BATT_ref){                    // RTR-501,502,503,505=2未満、RTR-547,576=4未満か
                            if(BATT < (BATT_ref-1)){                    // RTR-501,502,503,505=1未満、RTR-547,576=3未満か(復帰しにくくした)	2020/09/10 segi
                                SetFlag = (uint16_t)(SetFlag & ~(WR_BATT));     // ★★★★★ 電池警告bit OFF
                                WR_set(G_No , U_No , 0 , 0);        // 電池警告カウンタクリア
                            }
                        }
                        else{
                            SetFlag = (uint16_t)(SetFlag & ~(WR_BATT));         // ★★★★★ 電池警告bit OFF
                            WR_set(G_No , U_No , 0 , 0);            // 電池警告カウンタクリア
                        }
                    }
//---------- 2011.09.26 電池警報にヒステリシス追加 ----- ↑ここまで ---------------------------------------------------------------

                    if((reg_kind == 0xFA)&&(U_No_temp == U_No)){        // ★★★★★ 積算警報フラグ設定(RTR-574で前半子機番号の時のみ)  // xxx20 2010.07.20追加
                        ALM16bit = (uint16_t)ALM;                           //      "
                        ALM16bit = (uint16_t)(ALM16bit << 10);                      //      "
                        SetFlag = SetFlag | (0x3000 & ALM16bit);        //      "
                    }                                                   //      "
                }
        }                                                                                       // xxx25 2009.08.25追加
        else{                                                                                   // xxx25 2009.08.25追加
            SetFlag = 0;                                                                        // xxx25 2009.08.25追加
        }                                                                                       // xxx25 2009.08.25追加

//Printf("\r\n### [RF ] SetFlag=%04X / G=%d U=%d U_T=%d", SetFlag, G_No,U_No,U_No_temp);
        if(U_No == U_No_temp){          // 2in1の前半 or 2in1でない

			if((reg_kind == 0xFA)||(reg_kind == 0xF9)){			// RTR-574,RTR-576の場合
			}
			else{
				if((SetFlag & 0x0003) == 0){							// 警報状態でない場合は、
					SetFlag = (uint16_t)(SetFlag & ~(WR_UNIT_SENS));	// センサーエラーbitもOFFにする
				}
			}

            set_w_config(G_No , U_No , SetFlag , 0);    // 警報状態レジスタを更新する

            w_onof_C = 0x0003;

        }
        else{
            U_No = U_No_temp;           // 2in1の後半

			if((reg_kind == 0xFA)||(reg_kind == 0xF9)){			// RTR-574,RTR-576の場合
			}
			else{
				if((SetFlag & 0x0300) == 0){							// 警報状態でない場合は、
					SetFlag = (uint16_t)(SetFlag & ~(WR_UNIT_SENS));	// センサーエラーbitもOFFにする
				}
			}

            set_w_config(G_No , U_No , SetFlag , 2);    // 警報状態レジスタを追記する

            w_onof_C = 0x0300;

        }


//Printf("\r\n### [RF ] w_onof_onoff=%04X  w_onof_temp=%04X", w_onof_onoff, w_onof_temp);

        w_onof_onoff = (uint16_t)(~w_onof_temp);                    // 今回のリアルスキャンの警報フラグ(ALM/BATT)を反転(今は警報外の場合にHi)
        w_onof_onoff = w_onof_onoff & 0x0303;           //

        w_onof_back = w_onoff_read(G_No , U_No);        // 今回の onoff フラグ読み出し

        Now_GMT = chk_w_config(G_No,U_No_temp);         // 前回の警報フラグ
//Printf("\r\n### [RF ] Now_GMT=%08X(%ld) / G=%d U=%d U_T=%d", Now_GMT , Now_GMT, G_No, U_No, U_No_temp);
        w_onof_before = (uint16_t)(Now_GMT >> 16);          //      "
        w_onof_before = w_onof_before & 0x0303;         //      "

//Printf("\r\n[RF ] w_config_data_make  w_onof_onoff=%04X w_onof_back=%04X w_onof_before=%04x",w_onof_onoff,w_onof_back,w_onof_before);

        w_onof_A = (uint16_t)(w_onof_back & (w_onof_before | ~w_onof_C));
        w_onof_B = (uint16_t)(~w_onof_before & (w_onof_onoff & w_onof_C));


//Printf("\r\n[RF ] w_config_data_make  w_onof_A=%04X w_onof_B=%04X w_onof_before=%04x",w_onof_A,w_onof_B);


        w_onof_A = w_onof_A | w_onof_B;

//Printf("\r\n[RF ] w_config_data_make  chk_w_onoff_write=%04X",w_onof_A);


        w_onoff_write(G_No , U_No , w_onof_A);      // 今回の onoff フラグ書き込み


        w_onof_temp = 0;


//Printf("\r\n[RF ] FORMAT1  G_No=%d U_No=%d flags=%04x",G_No,U_No,SetFlag);

    }


    //*************************************************************
    //********** FORMAT2データをチェックする
    /*
    b_w_chg.byte[0] = rf_ram.auto_format2.data_byte[0]; // FORMAT2データバイト数
    b_w_chg.byte[1] = rf_ram.auto_format2.data_byte[1]; // FORMAT2データバイト数
    loop_size = b_w_chg.word;
    */
    loop_size = *(uint16_t *)&rf_ram.auto_format2.data_byte[0]; // FORMAT2データバイト数

    out_poi = &rf_ram.auto_format2.kdata[0].group_no;       // データ読み出しポインタ設定


    for(cnt = 0 ; cnt < loop_size ; cnt+=64){
        G_No = *out_poi;    // グループ番号を取得

        out_poi += 8;
        reg_kind = *out_poi;                        // 子機情報種類を取得する(0xFE,0xFA,0xF8,0xF9など)
        out_poi += 2;
        reg_ch1_zoku_0 = *out_poi;                  // 子機記録属性を取得する

//		out_poi += 22;
		out_poi += 14;
		kisyu_code = *out_poi;		// パルス記録かどうか判断するために機種コードを読み出す
		out_poi += 8;

        U_No = *out_poi;    // 子機番号を取得

        out_poi += 7;
        ALM1 = *out_poi++;          // CH1警報カウンタ    (format2から取得)
        ALM2 = *out_poi++;          // CH2警報カウンタ    (format2から取得)
        RFNG = *out_poi++;          // データ種類        (format2から取得)
        RFNG = ch2_check(RFNG);     // RFNG(記録状況)からチャンネル数を得る        xxx31 2011.03.31追加

//        out_poi += 22;

		b_w_chg.byte[0] = *out_poi++;		// 警報MAX値
		b_w_chg.byte[1] = *out_poi++;		// 警報MAX値
        out_poi += 20;


        SetFlag = 0;
        U_No_temp = w_unit_No(U_No,reg_kind,reg_ch1_zoku_0);
        if(U_No_temp != U_No){          // [ch3]か[ch4]の警報の場合
Printf("\r\n[RF ] FORMAT2[4] ALM3=%d ALM4=%d",WR_chk(G_No , U_No , 2),WR_chk(G_No , U_No , 3));	
// sakaguchi 2021.04.07 ↓ 警報カウンタが合算されない不具合を修正
            if((ALM1 != WR_chk(G_No , U_No , 2))){
                WR_set(G_No , U_No , 2 , ALM1);
				//ALMcnt_all_backup = ALM1;
                //SetFlag = SetFlag | WR_UNIT_CH3;                          // sakaguchi 2021.04.13 ↓
                if(reg_kind == 0xF9){                                       // RTR-576の場合
                    SetFlag_576 = (uint16_t)chk_w_config(G_No,U_No_temp);   // 前回の警報状況を読み出す
                    if((SetFlag_576 & WR_UNIT_SENS2) == 0 ){                // [CH3][CH4]で既にセンサエラーが検出されている場合は、上下限警報はＯＮしない
                        SetFlag = SetFlag | WR_UNIT_CH3;
                    }
                }
                else{
                    SetFlag = SetFlag | WR_UNIT_CH3;
                }
            }
            if((ALM2 != WR_chk(G_No , U_No , 3)) && (RFNG != 0)){   // 2CH機の場合のみ、ここを実行する        2013.01.22 修正

                WR_set(G_No , U_No , 3 , ALM2);
				//ALMcnt_all_backup += ALM2;
                //SetFlag = SetFlag | WR_UNIT_CH4;                          // sakaguchi 2021.04.13 ↓
                if(reg_kind == 0xF9){                                       // RTR-576の場合
                    SetFlag_576 = (uint16_t)chk_w_config(G_No,U_No_temp);   // 前回の警報状況を読み出す
                    if((SetFlag_576 & WR_UNIT_SENS2) == 0 ){                // [CH3][CH4]で既にセンサエラーが検出されている場合は、上下限警報はＯＮしない
                        SetFlag = SetFlag | WR_UNIT_CH4;
                    }
                }
                else{
                    SetFlag = SetFlag | WR_UNIT_CH4;
                }
            }
            ALMcnt_all_backup = ALM1;
            if(RFNG != 0)   ALMcnt_all_backup += ALM2;              // 2CH機の場合のみ
// sakaguchi 2021.04.07 ↑

            WR_set(G_No , U_No , 4 , ALMcnt_all_backup);

			if((reg_kind == 0xFA)||(reg_kind == 0xF9)){			// RTR-574,RTR-576の場合
			}
			else{
				if(kisyu_code != 0xa5){
			        if(sensor_check(b_w_chg.word , reg_ch1_zoku_0) == 1){	// センサーエラーの場合
				        if(SetFlag & (WR_UNIT_CH3 | WR_UNIT_CH4)){              // sakaguchi 2021.04.07
                            SetFlag = SetFlag | WR_UNIT_SENS;					// センサーエラーbitをONにする
                        }
                    }
			        else{													// センサーエラーでない場合は
				        SetFlag = SetFlag & (~WR_UNIT_SENS);				// センサーエラーbitをOFFにする
			        }
				}
			}


Printf("\r\n[RF ] FORMAT2[5] ALM3=%d ALM4=%d",ALM1,ALM2);
        }
        else{
Printf("\r\n[RF ] FORMAT2[1] ALM1=%d ALM2=%d",WR_chk(G_No , U_No , 2),WR_chk(G_No , U_No , 3));
// sakaguchi 2021.04.07 ↓ 警報カウンタが合算されない不具合を修正
            if((ALM1 != WR_chk(G_No , U_No , 2))){
                WR_set(G_No , U_No , 2 , ALM1);
				//ALMcnt_all_backup = ALM1;
                SetFlag = SetFlag | WR_UNIT_CH1;
            }
            if((ALM2 != WR_chk(G_No , U_No , 3)) && (RFNG != 0)){   // 2CH機の場合のみ、ここを実行する        2013.01.22 修正

                WR_set(G_No , U_No , 3 , ALM2);
				//ALMcnt_all_backup += ALM2;
                SetFlag = SetFlag | WR_UNIT_CH2;
            }
            ALMcnt_all_backup = ALM1;
            if(RFNG != 0)   ALMcnt_all_backup += ALM2;              // 2CH機の場合のみ
// sakaguchi 2021.04.07 ↑
            WR_set(G_No , U_No , 4 , ALMcnt_all_backup);

			if((reg_kind == 0xFA)||(reg_kind == 0xF9)){			// RTR-574,RTR-576の場合
			}
			else{
				if(kisyu_code != 0xa5){
                    if(sensor_check(b_w_chg.word , reg_ch1_zoku_0) == 1){
				        if(SetFlag & (WR_UNIT_CH1 | WR_UNIT_CH2)){              // sakaguchi 2021.04.07
                            SetFlag = SetFlag | WR_UNIT_SENS;
                        }
                    }
                    else{
        				SetFlag = SetFlag & (~WR_UNIT_SENS);
					}
				}
			}


Printf("\r\n[RF ] FORMAT2[2] G_No=%d U_No=%d flags=%04x",G_No,U_No,SetFlag);
Printf("\r\n[RF ] FORMAT2[3] ALM1=%d ALM2=%d",ALM1,ALM2);
        }


        U_No = U_No_temp;
        set_w_config(G_No , U_No , SetFlag , 1);        // 警報状態レジスタに★★★★★(温度等)警報追記(AND)する
//Printf("\r\n[RF ] FORMAT2  G_No=%d U_No=%d flags=%04x",G_No,U_No,SetFlag);

    }




    //
    //      親機の警報をチェックする
        SetFlag = (uint8_t)chk_w_config(0,0);           // 前回の警報状況を読み出して

    //---------------------------------------------------------------------------------------------------------------------

/*
    // *************************************************************
    // ********** 親機電池状態をチェックする
        if(FirstBattStatus > 3000){                     // 最初から電池が無かったかの判断(3.0V以下なら電池は元々無かったと判断)
            if(BattStatus > 5000){                      // 電池電圧が5.0V以下の場合に電池警報を出す。
                auto_control.m_batt = 0;                            // 電池少ないと検出した回数カウンタ
                SetFlag = SetFlag & ~(WR_BATT);                     // ★★★★★ 電池警告bit OFF
            }
            else{
                if(auto_control.m_batt < 250)auto_control.m_batt++; // 電池少ないと検出した回数カウンタ
                if(auto_control.m_batt > 2){
                    SetFlag = SetFlag | WR_BATT;                    // ★★★★★ 電池警告bit ON
                }
                else{
                    SetFlag = SetFlag & ~(WR_BATT);                 // ★★★★★ 電池警告bit OFF
                }
            }
        }
*/

//Printf("\r\n[RF ] SetFlag         = 0x%04x",SetFlag);             //      dbg xxx19

/*
    // *************************************************************
    // ********** 親機外部電源をチェックする
    if(FirstConnectAC == 1){    // 起動時に外部電源が入っていた場合、
        if(ConnectAC == 0){             // 親機の外部電源が抜けているかを示すフラグ(0:抜け 1:差し)
            SetFlag = SetFlag | WR_OYA_AC;      // ★★★★★ 外部電源警報bit ON
        }
        else{
            SetFlag = SetFlag & ~(WR_OYA_AC);   // ★★★★★ 外部電源警報bit OFF
        }
    }
*/

/*
    // *************************************************************
    // ********** 親機接点入力をチェックする
    if(ConnectExt == 1){
        SetFlag = SetFlag | WR_OYA_INPUT;       // ★★★★★ 接点入力bit ON
    }
    else{
        SetFlag = SetFlag & (uint8_t)(~(WR_OYA_INPUT)); // ★★★★★ 接点入力bit OFF
    }
*/

    // GSM通信に成功したら、ConnectExt をクリアする

//---------------------------------------------------------------------------------------------------------------------

    set_w_config(0 , 0 , SetFlag , 0);  // 警報状態レジスタを更新する


//Printf("\r\n[RF ] Oya-Conf G_No=00 U_No=00 flags=%04x",SetFlag);
//Printf("\r\n");
//Dbg_Print(&auto_control.w_config[0].group_no,80);



}

//********** end of void w_config_data_make(void) **********



/**
 * 自律リアルスキャンによって得られた子機番号から警報フラグの子機番号を決定する
 * @param unit_No
 * @param kind
 * @param ch1_zoku_0
 * @return
 */
static uint8_t w_unit_No(uint8_t unit_No ,uint8_t kind ,uint8_t ch1_zoku_0)
{
    uint8_t rtn;

    switch(kind){
        case 0xFA:                      // RTR-574(子機番号が2個割り当てられている。)
            rtn = (ch1_zoku_0 == 0x49) ?  unit_No : (uint8_t)(unit_No - 1);     // [照度・紫外線]データなら子機番号はそのまま/ [温度・湿度]データなら子機番号は一個前にする
            break;

        case 0xF9:                      // RTR-576(子機番号が2個割り当てられている。)
            rtn = (ch1_zoku_0 == 0x42) ? unit_No : (uint8_t)(unit_No - 1);       // [ＣＯ２]データなら子機番号はそのまま/ [温度・湿度]データなら子機番号は一個前にする
            break;

        case 0xFE:                      // RTR-501,502,503(子機番号が1個割り当てられている。)
        case 0xF8:                      // RTR-505(子機番号が1個割り当てられている。)
        default:                        // 未知の情報種類の時はそのままの子機番号を返す。
            rtn = unit_No;
            break;
    }
    return (rtn);
}
//********** end of uchar w_unit_No(uchar unit_No , uchar kind , uchar ch1_zoku[0]) **********



/**
 * 測定データがセンサエラーの値か判定する
 * @param so_data   測定値
 * @param zoku      記録属性
 * @return      (0/1) = (センサOK/センサNG)
 */
static uint8_t sensor_check(uint16_t so_data , uint8_t zoku)
{
    uint8_t tmp_zoku;
    uint8_t rtn;
    uint16_t comp1;
    uint16_t comp2;
    uint16_t comp3;

    switch(zoku){           // 記録属性によりセンサエラー判断値が変わる
        case 0x0d:                      // 温度の場合
        case 0xd0:                      // 湿度の場合
        case 0xd1:                      // 湿度の場合
        case 0x55:                      // 紫外線の場合
        case 0x49:                      // 照度の場合
        case 0x42:                      // CO2の場合
        case 0xff:
            comp1 = 0xeeee;
            comp2 = 0xeeee;
            comp3 = 0xeeee;
            break;

        case 0x4e:                      // 立上りパルスの場合
        case 0x4d:                      // 立下りパルスの場合
            comp1 = 0xf000;
            comp2 = 0xf000;
            comp3 = 0xf000;
            break;

        default:
            tmp_zoku = zoku & 0xf0;
            switch(tmp_zoku){
                case 0x80:              // 4-20mAの場合
                case 0x90:              // 電圧の場合
                    comp1 = 0xf002;
                    comp2 = 0xf001;
                    comp3 = 0xf00f;
                    break;

                default:                // 不明の場合
                    comp1 = 0xeeee;
                    comp2 = 0xeeee;
                    comp3 = 0xeeee;
                    break;
            }
            break;
    }

    rtn = ((so_data == comp1)||(so_data == comp2)||(so_data == comp3)) ? 1 : 0;

    return (rtn);
}
//********** end of uchar sensor_check(uint so_data , uchar zoku) **********




/**
 * リアルスキャンの記録状況から記録チャンネル数を導く
 * @param rec_status        記録状況
 * @return  0:1ch子機   0以外:2ch子機
 */
static uint8_t ch2_check(uint8_t rec_status)
{
    uint8_t rtn = 0;

    rec_status = rec_status & 0b01111100;       // 記録データ種類の部分を抜き出し

    switch(rec_status){
        case 0x10:          // 温度/湿度(RTR-53)
        case 0x08:          // 温度×2
        case 0x18:          // 温度/湿度
        case 0x28:          // 照度/紫外線
        case 0x38:          // 照度/紫外線(積算)
        case 0x1c:          // 温度/高精度湿度
            rtn = 0x08;
            break;

        default:            // 上記以外
            rtn = 0;
            break;
    }
    return (rtn);
}
//********** end of uchar 2ch_check(uchar rec_status) **********


/**
 * 警報バックアップ用データformat1クリア
 * @param size
 */
static void ALM_bak_f1_clr(uint8_t size)
{
    int cnt;
    for(cnt = 0 ; cnt < size ; cnt++){

        *(uint32_t *)ALM_bak[cnt].f1_grobal_time = 0x00000000;// format1クリア
/*      ALM_bak[cnt].f1_grobal_time[0] = 0; // format1クリア
        ALM_bak[cnt].f1_grobal_time[1] = 0;
        ALM_bak[cnt].f1_grobal_time[2] = 0;
        ALM_bak[cnt].f1_grobal_time[3] = 0;
*/
    }
}
//***** end of ALM_bak_f1_clr() *****


/**
 * 警報バックアップ用データ保存位置検索
 * @param G_No  グループ番号
 * @param U_No  子機番号
 * @return
 */
//static uint8_t ALM_bak_poi(uint8_t G_No, uint8_t U_No)
uint8_t ALM_bak_poi(uint8_t G_No, uint8_t U_No)     // sakaguchi 2021.03.01
{
    int cnt = 0;
    while(cnt < 255){
        if((ALM_bak[cnt].group_no == 0)&&(ALM_bak[cnt].unit_no == 0)){
            break;
        }
        if((ALM_bak[cnt].group_no == G_No)&&(ALM_bak[cnt].unit_no == U_No)){
            break;
        }
        cnt++;
    }
    return ((uint8_t)cnt);
}
//***** end of ALM_bak_poi() *****


/**
 * リアルスキャン&警報監視の format1 & format2 & format3 データをバックアップ
 */
static void ALM_bak_write(void)
{
    int cnt;
    uint8_t write_no;
//  uint8_t i;

//  uint8_t *in_poi;
//  uint8_t *out_poi;
/*
    union{
        uint8_t byte[2];
        uint16_t word;
    }b_w_chg;
*/
    uint16_t dataByte;


//  wai_mtx( ALMBAK );              // mutex確保


    ALM_bak_f1_clr(100);        // バッファの容量は100個分
/*
    b_w_chg.byte[0] = rf_ram.auto_format1.data_byte[0]; // format1のデータバイト数把握
    b_w_chg.byte[1] = rf_ram.auto_format1.data_byte[1];
    */
    dataByte = *(uint16_t *)&rf_ram.auto_format1.data_byte[0];  // format1のデータバイト数把握
    if(dataByte != 0){
        dataByte = dataByte / 64;           // データ個数を把握する
        ALM_bak_cnt = (uint8_t)dataByte;            // データ個数をALM_bak_cntに保存する
        for(cnt = 0 ; cnt < ALM_bak_cnt ; cnt++){
            write_no = ALM_bak_poi(rf_ram.auto_format1.kdata[cnt].group_no , rf_ram.auto_format1.kdata[cnt].unit_no);
            if(write_no < 100){
                ALM_bak[write_no].group_no = rf_ram.auto_format1.kdata[cnt].group_no;                                   // グループ番号
                ALM_bak[write_no].unit_no = rf_ram.auto_format1.kdata[cnt].unit_no;                                 // 子機番号
                memcpy(ALM_bak[write_no].f1_grobal_time , rf_ram.auto_format1.kdata[cnt].grobal_time , 60); // format1データ
            }
        }
    }
/*
    b_w_chg.byte[0] = rf_ram.auto_format2.data_byte[0]; // format2のデータバイト数把握
    b_w_chg.byte[1] = rf_ram.auto_format2.data_byte[1];
    */
    dataByte = *(uint16_t *)&rf_ram.auto_format2.data_byte[0];  // format2のデータバイト数把握
    if(dataByte != 0){
        dataByte = dataByte / 64;
        for(cnt = 0 ; cnt < dataByte ; cnt++){
            write_no = ALM_bak_poi(rf_ram.auto_format2.kdata[cnt].group_no , rf_ram.auto_format2.kdata[cnt].unit_no);
            if(write_no < 100){
                memcpy(&ALM_bak[write_no].f2_grobal_time[0] , &rf_ram.auto_format2.kdata[cnt].grobal_time[0],4);        // 無線通信時刻
                ALM_bak[write_no].f2_ALM_count1 = rf_ram.auto_format2.kdata[cnt].ALM_count1;                            // 警報フラグ ch1
                ALM_bak[write_no].f2_ALM_count2 = rf_ram.auto_format2.kdata[cnt].ALM_count2;                            // 警報フラグ ch2
                memcpy(ALM_bak[write_no].f2_data , rf_ram.auto_format2.kdata[cnt].ch1_max_data , 20);           // 警報データ
            }
        }
    }
    /*
    b_w_chg.byte[0] = rf_ram.auto_format3.data_byte[0]; // format3のデータバイト数把握
    b_w_chg.byte[1] = rf_ram.auto_format3.data_byte[1];
    */
    dataByte = *(uint16_t *)&rf_ram.auto_format3.data_byte[0];  // format3のデータバイト数把握
    if(dataByte != 0){
        dataByte = dataByte / 64;
        for(cnt = 0 ; cnt < dataByte ; cnt++){
            write_no = ALM_bak_poi(rf_ram.auto_format3.kdata[cnt].group_no , rf_ram.auto_format3.kdata[cnt].unit_no);
            if(write_no < 100){
                memcpy(ALM_bak[write_no].f3_grobal_time , rf_ram.auto_format3.kdata[cnt].grobal_time,4);        // 無線通信時刻
                ALM_bak[write_no].f3_ALM_count1 = rf_ram.auto_format3.kdata[cnt].ALM_count1;                            // 警報フラグ ch1
                ALM_bak[write_no].f3_ALM_count2 = rf_ram.auto_format3.kdata[cnt].ALM_count2;                            // 警報フラグ ch2
                memcpy(ALM_bak[write_no].f3_data , rf_ram.auto_format3.kdata[cnt].ch1_max_data , 20);           // 警報データ
            }
        }
    }

    ALM_bak_utc = EVT.Trend.StartUTC;

//  rel_mtx( ALMBAK );              // mutex解放


}
//***** end of ALM_bak_write() *****


/*
///////////////////////////////////////////////////////////////////////////////////////////////
//  自動警報リアルスキャンする時の為に、通信しなければいけない子機と中継機を表す
//  フラグをセットする
//  既にリアルスキャン(FORMAT1)済みの子機から必要子機のフラグをセットする
//
//  1グループの子機台数は最大128個としてある
//
//  引き数：sw
//              =0 登録ファイルの内容に従った全ての子機が対象
//              =1 既に受信できた(realscan_buff.over_unit)フラグ以外の子機が対象(リトライの場合)
//
///////////////////////////////////////////////////////////////////////////////////////////////
void ___set_keiho_do_flag_New(uint8_t sw)
{
    uint8_t *out_poi;
    uint8_t *in_poi;
    uint8_t *w_flag_poi;
    uint8_t w_flag;

    uint8_t unit_no_tmp;
    uint8_t alm_tmp;

    uint8_t alm_count1;
    uint8_t alm_count2;

    uint8_t set_rpt;
    uint8_t cnt;
    uint8_t RF_NG;
    uint8_t chk;
    union{
        uint8_t byte[2];
        uint16_t word;
    }b_w_chg;
    uint16_t data_cnt;


    in_poi = realscan_buff.do_rpt;
    for(cnt = 0 ; cnt < 48 ; cnt++,in_poi++)*in_poi = 0;    // realscanバッファクリア  do_rpt , do_unit , over_rpt


//----- ここに来る時は既にグループ情報は取得済みのはず -----
//  get_group_info(RF_buff.rf_req.current_group);       // グループ情報取得
//  get_rpt_table(RF_buff.rf_req.current_group);        // 中継テーブル作成


    b_w_chg.byte[0] = rf_ram.auto_format1.data_byte[0]; // リアルスキャンで得られたデータバイト数
    b_w_chg.byte[1] = rf_ram.auto_format1.data_byte[1]; //          "
    if(b_w_chg.word > 8192)b_w_chg.word = 8192;         // 128台分でリミット

    out_poi = &rf_ram.auto_format1.kdata[0].group_no;

    for(data_cnt = 0 ; data_cnt < b_w_chg.word ; data_cnt += 64){
        if(*out_poi == RF_buff.rf_req.current_group){

//-------------------- ↓ここから RTR-574対応の為に追加した部分 -------------------------------------------------------------------------------
            out_poi += 6;
            RF_NG = *out_poi;       // 無線通信エラーマーク(0xCC)を一時保存する
            out_poi += 2;
//-------------------- ↑ここまで -------------------------------------------------------------------------------------------------------------

            w_flag_poi = out_poi;           // 後で警報フラグを書き込むかもしれない場所を記憶しておく
            out_poi += 24;


            unit_no_tmp = *out_poi;         // 子機番号取得
            out_poi += 4;


            if(sw == 0) chk = 0;
            else        chk = check_flag(realscan_buff.over_unit,unit_no_tmp);  // 0以外は受信済みの時
            if(chk == 0){

                out_poi += 3;               // 今回のALMカウント
                alm_count1 = *out_poi;      // 今回のALMカウント取得(CH1 + CH2)
                alm_count2 = WR_chk(RF_buff.rf_req.current_group , unit_no_tmp , 4);    // 前回のALMカウント値を読み出す(CH1 + CH2 は引き数4を指定)

                if((alm_count1 != alm_count2)&&((RF_NG != 0xcc)||(RF_NG != 0xcb))){             // 警報フラグが立っている時の処理  // xxx16 2011.05.16

                    WR_set(RF_buff.rf_req.current_group , unit_no_tmp , 4 , alm_count1);    //今回のALMカウント値を保存

                    // 子機番号をキーワードにして
                    // 登録ファイルから子機情報を探す
                    // その情報から親中継機番号を取得する
                    cnt = 0;
                    //while(cnt < regist.group.unit_size){                      // 登録子機の台数分繰り返す
                    while(cnt < group_registry.remote_unit){                    // 登録子機の台数分繰り返す

                        //get_unit_info(RF_buff.rf_req.current_group,cnt+1,0);  // 子機情報読み出し(中継親番号(regist.unit.net_no)を知る為)
                        get_regist_relay_inf(RF_buff.rf_req.current_group, unit_no_tmp, &rpt_sequence);     // 対象子機の登録情報読み出す(RTR-574/576は子機番号の小さい方へ読み替えた返り値)


//-------------------- ↓ここから RTR-574対応の為に追加した部分 -------------------------------------------------------------------------------
                        if((ru_registry.rtr501.header == 0xFE)||(ru_registry.rtr501.header == 0xF8)){
//-------------------- ↑ここまで -------------------------------------------------------------------------------------------------------------
                                if(ru_registry.rtr574.number == unit_no_tmp){       // リアルスキャンデータ中の子機番号と一致したか
                                    if((ru_registry.rtr574.set_flag & 0x01) == 0x01){   // 登録ファイルは警報監視子機か
                                        if(group_registry.max_unit_no < unit_no_tmp)group_registry.max_unit_no = unit_no_tmp;   // 最大子機番号を把握する
                                        set_flag(realscan_buff.do_unit,unit_no_tmp);                                    // 通信対象子機フラグセット
                                        set_flag(realscan_buff.do_rpt,ru_registry.rtr501.superior);                             // 通信対象中継機フラグセット
                                        break;
                                    }
                                }
//-------------------- ↓ここから RTR-574対応の為に追加した部分 -------------------------------------------------------------------------------
                        }
//                      if(regist.unit.kind == 0xFA){
                        if((ru_registry.rtr501.header == 0xFA)||(ru_registry.rtr501.header == 0xF9)){           // xxx07 2011.03.07 RTR-576に対応した
                                if((ru_registry.rtr574.number == unit_no_tmp)||((ru_registry.rtr574.number + 1) == unit_no_tmp)){                   // リアルスキャンデータ中の子機番号と一致したか
                                    if((ru_registry.rtr574.set_flag & 0x01) == 0x01){   // 登録ファイルは警報監視子機か
                                        if(group_registry.max_unit_no < unit_no_tmp + 1)group_registry.max_unit_no = unit_no_tmp + 1;   // 最大子機番号を把握する
                                        set_flag(realscan_buff.do_unit,unit_no_tmp);                                            // 通信対象子機フラグセット
                                        set_flag(realscan_buff.do_rpt,ru_registry.rtr501.superior);                                     // 通信対象中継機フラグセット
                                        break;
                                    }
                                }
                        }
//-------------------- ↑ここまで -------------------------------------------------------------------------------------------------------------


                        cnt++;
                    }
                }
                out_poi += 25;
            }
            else{
                out_poi += 28;
            }
        }
        else{                   // グループ番号不一致の場合
            out_poi += 64;
        }
    }

    if(sw == 0){
        in_poi = realscan_buff.over_unit;
        for(cnt = 0 ; cnt < 16 ; cnt++,in_poi++)*in_poi = 0;    // realscanバッファクリア  over_unit
    }

}
// ***** end of set_keiho_do_flag(uint8_t sw) *****
*/



/**
 * 自動警報リアルスキャンする時の為に、通信しなければいけない子機と中継機を表すフラグをセットする
 *
 * 既にリアルスキャン(FORMAT1)済みの子機から必要子機のフラグをセットする
 * 1グループの子機台数は最大128個としてある
 * @param sw        =0 登録ファイルの内容に従った全ての子機が対象
 *                  =1 既に受信できた(realscan_buff.over_unit)フラグ以外の子機が対象(リトライの場合)
 */
static void set_keiho_do_flag_New(uint8_t sw)
{
    uint8_t *out_poi;
//    char *in_poi;
//    uint8_t *w_flag_poi;
//  uint8_t w_flag;

    uint8_t unit_no_tmp;
//  uint8_t alm_tmp;

    uint8_t alm_count1;
    uint8_t alm_count2;

//  uint8_t set_rpt;
//  uint8_t cnt;
    uint8_t RF_NG;
    uint8_t chk;
    union{
        uint8_t byte[2];
        uint16_t word;
    }b_w_chg;

    int data_cnt;

    memset(&realscan_buff ,0x00, 48);   // realscanバッファクリア  do_rpt , do_unit , over_rpt

//----- ここに来る時は既にグループ情報は取得済みのはず -----
//  get_group_info(RF_buff.rf_req.current_group);       // グループ情報取得
//  get_rpt_table(RF_buff.rf_req.current_group);        // 中継テーブル作成


    b_w_chg.byte[0] = rf_ram.auto_format1.data_byte[0]; // リアルスキャンで得られたデータバイト数
    b_w_chg.byte[1] = rf_ram.auto_format1.data_byte[1]; //          "
    if(b_w_chg.word > 8192){
        b_w_chg.word = 8192;            // 128台分でリミット
    }

    out_poi = &rf_ram.auto_format1.kdata[0].group_no;

    for(data_cnt = 0 ; data_cnt < b_w_chg.word ; data_cnt += 64){
        if(*out_poi == RF_buff.rf_req.current_group){

//-------------------- ↓ここから RTR-574対応の為に追加した部分 -------------------------------------------------------------------------------
            out_poi += 6;
            RF_NG = *out_poi;       // 無線通信エラーマーク(0xCC)を一時保存する
            out_poi += 2;
//-------------------- ↑ここまで -------------------------------------------------------------------------------------------------------------

//          w_flag_poi = out_poi;           // 後で警報フラグを書き込むかもしれない場所を記憶しておく
            out_poi += 24;


            unit_no_tmp = *out_poi;         // 子機番号取得
            out_poi += 4;

/*
            if(sw == 0){
                chk = 0;
            }
            else{
                chk = check_flag(realscan_buff.over_unit,unit_no_tmp);  // 0以外は受信済みの時
            }
            */
            chk = (sw == 0) ? 0 : check_flag(realscan_buff.over_unit,unit_no_tmp);  // 0以外は受信済みの時

            if(chk == 0){

                out_poi += 3;               // 今回のALMカウント
                alm_count1 = *out_poi;      // 今回のALMカウント取得(CH1 + CH2)
                alm_count2 = WR_chk(RF_buff.rf_req.current_group , unit_no_tmp , 4);    // 前回のALMカウント値を読み出す(CH1 + CH2 は引き数4を指定)

                // sakaguchi 2021.04.07 警報監視（初回）は警報リアルスキャンを実行させるため警報カウントを更新する
	            if((FirstAlarmMoni == 1) && (my_config.websrv.Mode[0] == 0)){           // 警報監視（初回）かつ 接続先がDataServerの場合
                    if(alm_count1 == alm_count2){               // 2023.07.30 警報カウントが同じ場合のみ更新
                        alm_count2 = (uint8_t)(alm_count1+1);                               // sakaguchi 2021.11.25
                    }
                }

                if((alm_count1 != alm_count2)&&((RF_NG != 0xcc)&&(RF_NG != 0xcb))){             // 警報フラグが立っている時の処理  // xxx16 2011.05.16 //2020.01.17 条件式 || を &&に変更

//                  WR_set(RF_buff.rf_req.current_group , unit_no_tmp , 4 , alm_count1);    //今回のALMカウント値を保存

                    get_regist_relay_inf(RF_buff.rf_req.current_group, unit_no_tmp, rpt_sequence);      // 対象子機の登録情報読み出す(RTR-574/576は子機番号の小さい方へ読み替えた返り値)

                    if((ru_registry.rtr501.set_flag & 0x01) == 0x01){   // 登録ファイルは警報監視子機か
                        if(group_registry.max_unit_no < unit_no_tmp + 1){
                            group_registry.max_unit_no = (char)(unit_no_tmp + 1);   // 最大子機番号を把握する
                        }
                        set_flag(realscan_buff.do_unit,unit_no_tmp);                                            // 通信対象子機フラグセット
// sakaguchi 2021.10.05 ↓
                        //set_flag(realscan_buff.do_rpt,ru_registry.rtr501.superior);             // 通信対象中継機フラグセット
                        if( 1 == (root_registry.auto_route & 0x01) ){   // 自動ルート設定
                            // 親番号リストから子機の親番号を取得し、通信対象中継機フラグをセットする
                            set_flag(realscan_buff.do_rpt, (uint8_t)RF_Group[RF_buff.rf_req.current_group-1].remoteUnitParents.value[(int8_t)ru_registry.rtr501.number]);
                        }else{                                          // 手動ルート設定
                            // 登録情報から子機の親番号を取得し、通信対象中継機フラグをセットする
                            set_flag(realscan_buff.do_rpt,ru_registry.rtr501.superior);
                        }
// sakaguchi 2021.10.05 ↑
                    }
                    // 子機番号をキーワードにして
                    // 登録ファイルから子機情報を探す
                    // その情報から親中継機番号を取得する

                }
                out_poi += 25;
            }
            else{
                out_poi += 28;
            }
        }
        else{                   // グループ番号不一致の場合
            out_poi += 64;
        }
    }

    if(sw == 0){
        memset(realscan_buff.over_unit, 0x00, sizeof(realscan_buff.over_unit));  // realscanバッファクリア  over_unit
    }

}
//***** end of set_keiho_do_flag(uint8_t sw) *****




/**
 * 警報か判断する為にバックアップしている前回までの情報を読み出す。
 * @param G_No  グループ番号(0～)
 * @param U_No  子機番号(1～)
 * @param pogi  0:電池Lowを連続検出した回数
 *           1:無線通信エラーの連続回数
 *           2:Ch1警報カウンタ
 *           3:Ch2警報カウンタ
 *           4:合計警報カウンタ(Ch1+Ch2)
 *           5:予備
 * @return
 */
//static uint8_t WR_chk(uint8_t G_No, uint8_t U_No, uint8_t pogi)
uint8_t WR_chk(uint8_t G_No, uint8_t U_No, uint8_t pogi)
{
    uint8_t *in_poi;
    uint8_t *poi_back;
    int cnt = 0;
    int i;
    uint8_t rtn;

    rtn = 0;    // 該当データが無い場合の応答値(0)設定
    in_poi = &auto_control.warning[0].group_no;
    do{
        poi_back = in_poi;

        if(*in_poi == G_No){                                // グループ番号一致を判断
            in_poi++;
            if(*in_poi == U_No){                            // 子機番号一致を判断
                in_poi++;
                for(i = 0 ; i < pogi ; i++ ){
                    in_poi++;       // 指定pogiまでポインタ進める
                }
                rtn = *in_poi++;
                break;
            }
        }
        in_poi = poi_back + 8;
        cnt++;
    }while(cnt < 128);

    return (rtn);
}
//********** end of WR_chk() **********




/**
 * 警報か判断する為のバックアップ情報を書きこむ。
 * @param G_No  グループ番号(0～)
 * @param U_No  子機番号(1～)
 * @param pogi  EDF struct def_a_warning{       // 8byte
 *           UBYTE group_no;             // グループ番号
 *           UBYTE unit_no;              // 子機番号
 *           UBYTE batt;                 // 電池エラー回数  0
 *           UBYTE rfng;                 // 無線エラー回数  1
 *           UBYTE ch1_counter;          // CH1警報カウンタ    2
 *           UBYTE ch2_counter;          // CH2警報カウンタ    3
 *           UBYTE counter;              // 警報カウンタ       4
 *           UBYTE yobi;                 // 予備(未使用)      5
 * @param setdata   書き込み値
 * @return
 */
/*static*/ uint8_t WR_set(uint8_t G_No ,uint8_t U_No ,uint8_t pogi ,uint8_t setdata)
{
    uint16_t memo_no = ERROR_END;
    uint8_t *in_poi;
    uint8_t *poi_back;
    int cnt = 0;
    int i;

    in_poi = &auto_control.warning[0].group_no;
    do{
        poi_back = in_poi;

        if(*in_poi == G_No){                    // グループ番号一致を判断
            in_poi++;
            if(*in_poi == U_No){                // 子機番号一致を判断
                memo_no = (uint16_t)cnt;
                break;
            }
        }
        else if(*in_poi == 0xff){
            if(memo_no == ERROR_END){
                memo_no = (uint16_t)cnt;
            }
        }

        in_poi = poi_back + 8;
        cnt++;
    }while(cnt < 128);

    if(memo_no != ERROR_END){
        in_poi = &auto_control.warning[0].group_no;
        in_poi = in_poi + (memo_no * 8);
        *in_poi++ = G_No;
        *in_poi++ = U_No;
        for(i = 0 ; i < pogi ; i++ ){
            in_poi++;       // 指定pogiまでポインタ進める
        }
        *in_poi = setdata;
    }

	auto_control.crc = crc32_bfr(&auto_control, 5122);		// ((128*8)*5)+2	CRC追加	2020/08/22 segi
    return ((uint8_t)memo_no);
}
//********** end of WR_set() **********





/**
 * 警報か判断する為にバックアップしている情報の値をインクリメントする。
 * (各カウンタは1byteなので250で頭打ちにする)
 * @param G_No  グループ番号(0～)
 * @param U_No  子機番号(1～)
 * @param pogi  0:電池Lowを連続検出した回数
 *              1:無線通信エラーの連続回数
 *              2:Ch1警報カウンタ
 *              3:Ch2警報カウンタ
 *              4:合計警報カウンタ(Ch1+Ch2)
 *              5:予備
 * @return
 */
static uint8_t WR_inc(uint8_t G_No, uint8_t U_No, uint8_t pogi)
{
    uint8_t rtn;

    rtn = WR_chk(G_No , U_No , pogi);
    if(rtn < 250){
        rtn++;                  // 2009/05/30 250カウントで頭打ち
    }
    WR_set(G_No , U_No , pogi , rtn);

    return (rtn);
}
//********** end of WR_inc() **********



/**
 * 警報か判断する為にバックアップしている変数の初期化(0xFFで埋める)
 */
void WR_clr(void)
{
//  uint8_t *in_poi;
    int cnt;
//  uint8_t i;

//  in_poi = &auto_control.warning[0].group_no;
#if CONFIG_NEW_STATE_CTRL       // 2023.05.26
    Printf("\r\n============================\r\n"
                           " auto_control.warning CLEAR\r\n"
                           "============================\r\n");
#endif

    for(cnt = 0 ; cnt < 128 ; cnt++){
        /*
        *in_poi++ = 0xff;
        *in_poi++ = 0xff;
        for(i = 0 ; i < 6 ; i++ ,in_poi++){
            *in_poi = 0;
        }
        */
        *(uint32_t *)&auto_control.warning[cnt].group_no = 0x0000FFFF;
        *(uint32_t *)&auto_control.warning[cnt].ch1_counter =0x00000000;


    }
	auto_control.crc = crc32_bfr(&auto_control, 5122);		// ((128*8)*5)+2	CRC追加	2020/08/22 segi
}
//********** end of WR_clr() **********

// sakaguchi 2021.04.07
/**
 * バックアップしている無線通信エラー回数をクリアする
 */
void WR_clr_rfcnt(void)
{
    int cnt;

    for(cnt = 0 ; cnt < 128 ; cnt++){
        auto_control.warning[cnt].rfng = 0;
    }

    auto_control.crc = crc32_bfr(&auto_control, 5122);		// ((128*8)*5)+2	CRC追加	2020/08/22 segi
}

/**
 * @brief  自律積算警報リアルスキャンする時の為に、通信しなければいけない子機と中継機を表す
 *              フラグをセットする
 *              既にリアルスキャン(FORMAT1)済みの子機から必要子機のフラグをセットする
 *              1グループの子機台数は最大128個としてある
 * @param sw    =0 登録ファイルの内容に従った全ての子機が対象
 *              =1 既に受信できた(realscan_buff.over_unit)フラグ以外の子機が対象(リトライの場合)
 */
static void set_sekisan_do_flag_New(uint8_t sw)
{
    char *out_poi;
//  char *in_poi;
//  char *w_flag_poi;
//  uint8_t w_flag;

    uint8_t unit_no_tmp;
    uint8_t alm_tmp;
//  uint8_t set_rpt;
//  uint8_t cnt;
    uint8_t chk;
/*  union{
        uint8_t byte[2];
        uint16_t word;
    }b_w_chg;
*/
    uint16_t dataByte;

    int data_cnt;
    uint16_t spec;                          // sakaguchi 2021.04.07

    memset(&realscan_buff, 0x00, 48);       // realscanバッファクリア

//  get_group_info(RF_buff.rf_req.current_group);       // グループ情報取得
//  get_rpt_table(RF_buff.rf_req.current_group);        // 中継テーブル作成
/*
    b_w_chg.byte[0] = rf_ram.auto_format1.data_byte[0]; // リアルスキャンで得られたデータバイト数
    b_w_chg.byte[1] = rf_ram.auto_format1.data_byte[1]; //          "
    if(b_w_chg.word > 8192){
        b_w_chg.word = 8192;            // 128台分でリミット
    }
*/
    dataByte = *(uint16_t *)rf_ram.auto_format1.data_byte;
    if(dataByte > 8192){
        dataByte = 8192;            // 128台分でリミット
    }

    out_poi = (char *)&rf_ram.auto_format1.kdata[0].group_no;

    for(data_cnt = 0 ; data_cnt < dataByte ; data_cnt += 64){
        if(*out_poi == RF_buff.rf_req.current_group){

            out_poi += 8;
//          w_flag_poi = out_poi;           // 後で警報フラグを書き込むかもしれない場所を記憶しておく
            out_poi += 24;

            unit_no_tmp = *out_poi;         // 子機番号取得
            out_poi += 4;

            chk = (sw == 0) ? 0 : check_flag(realscan_buff.over_unit,unit_no_tmp);  // 0以外は受信済みの時

            if(chk == 0){
                alm_tmp = *out_poi;     // 警報フラグを取得する
                // sakaguchi 2021.04.07 警報監視（初回）は、RTR574の積算警報リアルスキャンを実行させる
	            if((FirstAlarmMoni == 1) && (my_config.websrv.Mode[0] == 0)){           // 警報監視（初回）かつ 接続先がDataServer
                    get_regist_relay_inf(RF_buff.rf_req.current_group, unit_no_tmp, rpt_sequence);      // 対象子機の登録情報読み出す(RTR-574/576は子機番号の小さい方へ読み替えた返り値)
                    spec = GetSpecNumber( ru_registry.rtr501.serial , ru_registry.rtr505.model );
                    if(spec == SPEC_574){
                        alm_tmp = 0xc0;
                    }
                }

                if((alm_tmp & 0xc0) != 0){
                    //get_unit_no_info(RF_buff.rf_req.current_group,unit_no_tmp);
                    get_regist_relay_inf(RF_buff.rf_req.current_group, unit_no_tmp, rpt_sequence);      // 対象子機の登録情報読み出す(RTR-574/576は子機番号の小さい方へ読み替えた返り値)

                    if((ru_registry.rtr574.set_flag & 0x01) == 0x01){   // 登録ファイルは警報監視子機か
                        if(group_registry.max_unit_no < unit_no_tmp){
                            group_registry.max_unit_no = unit_no_tmp;   // 最大子機番号を把握する
                        }
                        set_flag(realscan_buff.do_unit,unit_no_tmp);                                    // 通信対象子機フラグセット
// sakaguchi 2021.10.05 ↓
                        //set_flag(realscan_buff.do_rpt,ru_registry.rtr574.superior);                             // 通信対象中継機フラグセット
                        if( 1 == (root_registry.auto_route & 0x01) ){   // 自動ルート設定の場合
                            // 親番号リストから子機の親番号を取得し、通信対象中継機フラグをセットする
                            set_flag(realscan_buff.do_rpt, (uint8_t)RF_Group[RF_buff.rf_req.current_group-1].remoteUnitParents.value[(int8_t)ru_registry.rtr574.number]);
                        }else{                                          // 手動ルート設定の場合
                            // 登録情報から子機の親番号を取得し、通信対象中継機フラグをセットする
                            set_flag(realscan_buff.do_rpt, ru_registry.rtr574.superior);
                        }
// sakaguchi 2021.10.05 ↑
                    }

                }
                out_poi += 28;
            }
            else{
                out_poi += 28;
            }
        }
        else{                   // グループ番号不一致の場合
            out_poi += 64;
        }

    }

    if(sw == 0){
        memset(realscan_buff.over_unit, 0, sizeof(realscan_buff.over_unit));// realscanバッファクリア   over_unit
    }

}
//***** end of set_sekisan_do_flag(uchar sw) *****


/**
 * 自律動作時のリアルスキャンデータを別バッファにコピーする
 * realscanをした生データに、GSM処理で使う為に、グループ番号、FORMAT番号、通信時刻を付加する
 * @param format_no
 * @bug b_w_chg 4Byteパディングあり
 */
void copy_realscan(uint8_t format_no)
{
    uint8_t *in_poi,*out_poi;

    int cnt,i;
    uint8_t copy_unit_size; // コピー台数
    uint16_t get_data_size;
    uint16_t all_unit_size;
    union{
        uint8_t byte[4];
        uint16_t word;
        uint32_t chg_long;
    }b_w_chg;
    uint16_t dataByte;

    def_group_registry  read_group_registry;            // グループ情報
    uint16_t adr;                                       // グループ情報のアドレス

	uint8_t regi_intt;                                  // sakaguchi 2021.01.28
    char tmp_route[256];                                // sakaguchi 2021.10.05
    memset(tmp_route, 0x00, sizeof(tmp_route));         // sakaguchi 2021.10.05

/*  b_w_chg.byte[0] = rf_ram.realscan.data_byte[0];     // リアルスキャンで取得したデータバイト数が先頭2byteに入っている。
    b_w_chg.byte[1] = rf_ram.realscan.data_byte[1];     //                      "
*/

    dataByte = *(uint16_t *)rf_ram.realscan.data_byte;

// 2022.01.26 add ↓
    if((uint16_t)(dataByte % 30) != 0){                 // データサイズが30byteで割り切れない場合、無効なデータとしてコピーしない
        PutLog(LOG_RF,"copyscan unit databyte[%d]", dataByte);
        return;
    }
// 2022.01.26 add ↑

    copy_unit_size = (uint8_t)(dataByte / 30);      // 今回無線通信できた子機の台数(リアルスキャン応答は子機あたり30byte)

// 2022.01.26 add ↓
    adr = RF_get_regist_group_adr(RF_buff.rf_req.current_group, &read_group_registry);
    if(0 == adr){                                           // グループ番号が登録ファイルに存在しない
        PutLog(LOG_RF,"copyscan group[%d][%d]", RF_buff.rf_req.current_group, adr);
        return;
    }
// 2022.03.01 del 574,576を含むデータがエラーになってしまう ↓
//    if(read_group_registry.remote_unit < copy_unit_size){   // 子機台数が登録子機台数より大きい場合、無効なデータとしてコピーしない
//        PutLog(LOG_RF,"copyscan unit count[%d][%d]", read_group_registry.remote_unit, copy_unit_size);
//        return;
//    }
// 2022.03.01 del ↑
// 2022.01.26 add ↑

Printf("### copy_realscan %d / %d\r\n", format_no, copy_unit_size);

    switch(format_no){                                  // format_noにより保存場所が異なる
        case 1:
        default:
            in_poi = rf_ram.auto_format1.data_byte;
            break;
        case 2:
            in_poi = rf_ram.auto_format2.data_byte;
            break;
        case 3:
            in_poi = rf_ram.auto_format3.data_byte;
            break;
    }
/*  b_w_chg.byte[0] = *in_poi++;
    b_w_chg.byte[1] = *in_poi++;
    all_unit_size = b_w_chg.word;                       // 現在のformat data 保存エリアのデータバイト数を把握する
*/
    all_unit_size = *(uint16_t *)in_poi;                // 現在のformat data 保存エリアのデータバイト数を把握する

    switch(format_no){
        case 1:
            in_poi = &rf_ram.auto_format1.kdata[0].group_no;
            break;
        case 2:
            in_poi = &rf_ram.auto_format2.kdata[0].group_no;
            break;
        case 3:
            in_poi = &rf_ram.auto_format3.kdata[0].group_no;
            break;
    }



//  in_poi += b_w_chg.word;
    in_poi += all_unit_size;

    get_data_size = 0;
    out_poi = &rf_ram.realscan.data.format1[0].unit_no; // コピー元のポインタ
    for(cnt = 0 ; cnt < copy_unit_size ; cnt++){

        if(check_flag(realscan_buff.do_unit,*out_poi) != 0){

//-------------------- ↓ここから RTR-574対応の為に追加した部分 -------------------------------------------------------------------------------
            if(/*(format_no != 1)||*/(dual_check_574(*out_poi) == NORMAL_END)){
//-------------------- ↑ここまで -------------------------------------------------------------------------------------------------------------

                    if(check_double(format_no,RF_buff.rf_req.current_group,*out_poi) == NORMAL_END){
//#ifndef DBG_TERM
// sakaguchi 2021.01.28 ↓
                        // グループ番号、子機番号の登録ファイルを読み出し
                        //get_regist_relay_inf(RF_buff.rf_req.current_group, *out_poi, rpt_sequence);		// 登録ファイル読み出し
                        get_regist_relay_inf(RF_buff.rf_req.current_group, *out_poi, tmp_route);		// 登録ファイル読み出し         // sakaguchi 2021.10.05

                        // 受信データの子機番号が登録ファイルに存在しているか確認   // sakaguchi 2021.01.28 574,576の大きい子機番号は除外する処理が必要
                        if(ru_registry.rtr501.number != *out_poi){
                            if((ru_registry.rtr501.header == 0xFA)||(ru_registry.rtr501.header == 0xF9)){       // ##### RTR-574,576の場合 #####
                                if(ru_registry.rtr501.number != ((*out_poi)-1)){
                                    PutLog(LOG_RF,"copyscan unitno[%d][%d][%d]", *out_poi, copy_unit_size, dataByte);   // 子機番号異常
                                    out_poi += 30;                  // 次の子機へポインタ加算
                                    continue;                       // 次ループへ
                                }
                            }else{
                                PutLog(LOG_RF,"copyscan unitno[%d][%d][%d]", *out_poi, copy_unit_size, dataByte);   // 子機番号異常
                                out_poi += 30;                  // 次の子機へポインタ加算
                                continue;                       // 次ループへ
                            }
                        }

                        // 受信データのモニタリング間隔が登録ファイルの値と一致しているか比較
                        if(RF_buff.rf_req.data_format == 1){                    // フォーマット１で無線通信した場合のみ
                            if((ru_registry.rtr501.header == 0xFA)||(ru_registry.rtr501.header == 0xF9)){       // ##### RTR-574,576の場合 #####
                                regi_intt = ru_registry.rtr574.data_intt;		// RTR-574,RTR-576はモニタリング間隔の場所が45番目
                            }else{
                                regi_intt = ru_registry.rtr501.data_intt;		// RTR-501,RTR-505はモニタリング間隔の場所が33番目
                            }
                            if(regi_intt != *(out_poi + 8)){ 	// リアルスキャン受信値と登録値の モニタリング間隔を比較

                                if( ((regi_intt == 0x01) && (*(out_poi + 8) == 0xBC))   // 1分(0x01)と60秒(0xBC)の比較      // sakaguchi 2021.04.21
                                 || ((regi_intt == 0xBC) && (*(out_poi + 8) == 0x01))
                                 || ((regi_intt == 0x02) && (*(out_poi + 8) == 0xF8))   // 2分(0x02)と120秒(0xF8)の比較
                                 || ((regi_intt == 0xF8) && (*(out_poi + 8) == 0x02)) )
                                {
                                    // モニタリング間隔は一致と判断し、処理は継続する
                                }
                                else{
                                    PutLog(LOG_RF,"copyscan intt[%d][%d][%d][%d]", *(out_poi + 8), regi_intt, copy_unit_size, dataByte);    // モニタリング間隔異常
                                    out_poi += 30;                  // 次の子機へポインタ加算
                                    continue;                       // 次ループへ
                                }

                            }
                        }
// sakaguchi 2021.01.28 ↑
//#endif
                        set_flag(realscan_buff.over_unit,*out_poi);     // 受信済フラグセット


                        *in_poi++ = RF_buff.rf_req.current_group;           // 現在のグループ番号
                        *in_poi++ = RF_buff.rf_req.data_format;             // FORMAT DATA

                        b_w_chg.chg_long = RF_buff.rf_res.time;             // 無線通信した時刻が入っている
                        *in_poi++ = b_w_chg.byte[0];                        // グローバル時刻
                        *in_poi++ = b_w_chg.byte[1];                        //      "
                        *in_poi++ = b_w_chg.byte[2];                        //      "
                        *in_poi++ = b_w_chg.byte[3];                        //      "

/*
                        for(i = 0 ; i < 26 ; i++ , in_poi++){
                            *in_poi = 0;
                        }
*/

                        for(i = 0 ; i < 24 ; i++ , in_poi++){       // 2017/12/08
                            *in_poi = 0;                            //
                        }                                           //
                        *in_poi++ = End_Rpt_No;                     // 末端の中継機番号
                        *in_poi++ = 0;                              //



                        for(i = 0 ; i < 30 ; i++,in_poi++,out_poi++){
                            *in_poi = *out_poi;
                        }
                        *in_poi++ = 0;                                  // yobi2[0]
                        *in_poi++ = 0;                                  // yobi2[1]
// sakaguchi 2021.01.28-2 重複チェック対応 ↓
                        //get_data_size = (uint16_t)(get_data_size + 64);
                        all_unit_size = (uint16_t)(all_unit_size + 64);
                        b_w_chg.word = (uint16_t)all_unit_size;
                        switch(format_no){
                            case 1:
                                rf_ram.auto_format1.data_byte[0] = b_w_chg.byte[0];
                                rf_ram.auto_format1.data_byte[1] = b_w_chg.byte[1];
                                break;
                            case 2:
                                rf_ram.auto_format2.data_byte[0] = b_w_chg.byte[0];
                                rf_ram.auto_format2.data_byte[1] = b_w_chg.byte[1];
                                break;
                            case 3:
                                rf_ram.auto_format3.data_byte[0] = b_w_chg.byte[0];
                                rf_ram.auto_format3.data_byte[1] = b_w_chg.byte[1];
                                break;
                            default:
                                rf_ram.auto_format1.data_byte[0] = b_w_chg.byte[0];
                                rf_ram.auto_format1.data_byte[1] = b_w_chg.byte[1];
                                break;
                        }
// sakaguchi 2021.01.28-2 ↑
                    }
                    else{
                        //PutLog(LOG_RF,"copyscan double[%d][%d][%d]", *out_poi, copy_unit_size, dataByte);       // sakaguchi 2021.02.01 del
                        out_poi += 30;
                    }

//-------------------- ↓ここから RTR-574対応の為に追加した部分 -------------------------------------------------------------------------------
            }
            else{
                out_poi += 30;
            }
//-------------------- ↑ここまで -------------------------------------------------------------------------------------------------------------

        }
        else{
            out_poi += 30;
        }

    }

// sakaguchi 2021.01.28-2 重複チェック対応のため上に移動 ↓
#if 0
    b_w_chg.word = (uint16_t)(all_unit_size + get_data_size);       // 前回までのバイト長に今回のバイト長を加算

    switch(format_no){
        case 1:
            in_poi = rf_ram.auto_format1.data_byte;
            break;
        case 2:
            in_poi = rf_ram.auto_format2.data_byte;
            break;
        case 3:
            in_poi = rf_ram.auto_format3.data_byte;
            break;
    }
    *in_poi++ = b_w_chg.byte[0];
    *in_poi++ = b_w_chg.byte[1];
#endif
// sakaguchi 2021.01.28-2 ↑


// ##############################################
//      中継機情報を保存する処理
// ##############################################

    if(RF_buff.rf_req.cmd1 == 0x30){

        b_w_chg.byte[0] = *out_poi++;       // リアルスキャンで取得した中継機データバイト数を取得
        b_w_chg.byte[1] = *out_poi++;       // out_poiは中継機情報の先頭バイトになっているハズ

        dataByte = b_w_chg.word;

// 2022.01.26 add ↓
        if((uint16_t)(dataByte % 30) != 0){                // データサイズが30byteで割り切れない場合、無効なデータとしてコピーしない
            PutLog(LOG_RF,"copyscan rpt databyte[%d]", dataByte);
            return;
        }
// 2022.01.26 add ↑

        copy_unit_size = (uint8_t)(dataByte / 30);          // 今回無線通信できた中継機の台数(リアルスキャン応答は子機あたり30byte)

// 2022.01.26 add ↓
        if(read_group_registry.altogether < copy_unit_size){    // 中継機台数が登録中継機台数より大きい場合、無効なデータとしてコピーしない
            PutLog(LOG_RF,"copyscan rpt count[%d][%d]", read_group_registry.altogether, copy_unit_size);
            return;
        }
// 2022.01.26 add ↑

        in_poi = rf_ram.auto_rpt_info.data_byte;
        all_unit_size = *(uint16_t *)in_poi;                // 現在のrpt_info data 保存エリアのデータバイト数を把握する

        in_poi = &rf_ram.auto_rpt_info.kdata[0].group_no;
        in_poi += all_unit_size;

        get_data_size = 0;

        for(cnt = 0 ; cnt < copy_unit_size ; cnt++){

                *in_poi++ = RF_buff.rf_req.current_group;           // 現在のグループ番号
                *in_poi++ = RF_buff.rf_req.data_format;             // FORMAT DATA

                b_w_chg.chg_long = RF_buff.rf_res.time;             // 無線通信した時刻が入っている
                *in_poi++ = b_w_chg.byte[0];                        // グローバル時刻
                *in_poi++ = b_w_chg.byte[1];                        //      "
                *in_poi++ = b_w_chg.byte[2];                        //      "
                *in_poi++ = b_w_chg.byte[3];                        //      "

                for(i = 0 ; i < 24 ; i++ , in_poi++){       // 2017/12/08
                    *in_poi = 0;                            //
                }                                           //
                *in_poi++ = End_Rpt_No;                     // 末端の中継機番号
                *in_poi++ = 0;                              //


                for(i = 0 ; i < 30 ; i++,in_poi++,out_poi++){
                    *in_poi = *out_poi;
                }
                *in_poi++ = 0;                                  // yobi2[0]
                *in_poi++ = 0;                                  // yobi2[1]
                get_data_size = (uint16_t)(get_data_size + 64);

        }

        b_w_chg.word = (uint16_t)(all_unit_size + get_data_size);   // 前回までのバイト長に今回のバイト長を加算
        in_poi = rf_ram.auto_rpt_info.data_byte;
        *in_poi++ = b_w_chg.byte[0];                                // 現在までの中継機情報バイト数を保存
        *in_poi++ = b_w_chg.byte[1];                                // 現在までの中継機情報バイト数を保存



        get_rpt_registry(RF_buff.rf_req.current_group,rf_ram.auto_rpt_info.kdata[0].unit_no);   // 動作確認



    }
}


/**
 * 自律リアルスキャンを行ったが、無線通信できなかった子機の通信エラーデータを追加する
 *
 * RF_buff.rf_req.current_groupが現在グループ(番号)とする
 * RF_buff.rf_req.data_formatが現在のフォーマット(番号)とする
 *
 * realscan_buff.do_unit - realscan_buff.over_unit が通信出来なかった子機番号となる
 */
uint8_t realscan_err_data_add(void)
{
    uint8_t *size_poi;
    uint8_t *data_poi;
    uint8_t Oct_cnt;
    uint8_t Svn_cnt;
    uint8_t cnt;
    uint8_t ok_buff;
    uint8_t ng_buff;
    uint8_t cmp_data;
    uint8_t unit_no;
    union{
        uint8_t byte[2];
        uint16_t word;
    }b_w_chg;
    union{
        uint8_t byte[4];
        uint32_t w_word;
    }b_l_chg;

    uint8_t rtn = 0;

    size_poi = rf_ram.auto_format1.data_byte;   // 追記先がFORMAT1かFORMAT2か判断
    if(RF_buff.rf_req.data_format == 1){
        size_poi = rf_ram.auto_format1.data_byte;   // 追記先がFORMAT1かFORMAT2か判断
    }
    else if(RF_buff.rf_req.data_format == 2){
        size_poi = rf_ram.auto_format2.data_byte;
    }
    b_w_chg.byte[0] = *size_poi++;
    b_w_chg.byte[1] = *size_poi++;
    data_poi = size_poi + b_w_chg.word;     // 通信できなかった子機情報を追記する現状の末尾アドレスを取得
    size_poi -= 2;                          // 最後にデータbyte数を書き込む先(先頭アドレス)を保存しておく


    for(Oct_cnt = 0 ; Oct_cnt < 16 ; Oct_cnt++){

        ok_buff = realscan_buff.do_unit[Oct_cnt] & realscan_buff.over_unit[Oct_cnt];    // 無線通信できた子機を示すフラグ
        ng_buff = realscan_buff.do_unit[Oct_cnt] ^ ok_buff;                             // 無線通信できない子機を示すフラグ

        cmp_data = 1;
        for(Svn_cnt = 0 ; Svn_cnt < 8 ; Svn_cnt++){             // 2013.01.22 子機番号 7,15,23,31...子機のエラーデータ抜ける

            if((cmp_data & ng_buff) != 0){
                // エラーデータを追加する処理
                unit_no = (uint8_t)((Oct_cnt * 8) + Svn_cnt);       // 追加する子機番号

                *data_poi++ = RF_buff.rf_req.current_group;                     // 01 グループ番号
                *data_poi++ = RF_buff.rf_req.data_format;                       // 01 データフォーマット番号

//              for(cnt = 0 ; cnt < 4 ; cnt++,data_poi++)*data_poi = 0;         // 04 グローバル時間はALL 0
                b_l_chg.w_word = RF_buff.rf_res.time;       // 無線通信したUTC                                                        // xxx28 2009.09.28 追加
                for(cnt = 0 ; cnt < 4 ; cnt++,data_poi++){                                                                          // xxx28 2009.09.28 追加
                    *data_poi = b_l_chg.byte[cnt];                              // 04 無線通信した時刻が入っているグローバル時間 // xxx28 2009.09.28 追加
                }                                                                                                                   // xxx28 2009.09.28 追加


//              for(cnt = 0 ; cnt < 26 ; cnt++,data_poi++)*data_poi = 0xcc;     // 26 無効データマーク0xcc
                /*
                if(chk_noise(group_registry.frequency) == 0){                           // 通信エラー要因がチャンネルBUSYか判断する
                    for(cnt = 0 ; cnt < 26 ; cnt++,data_poi++){
                        *data_poi = 0xcc;   // 26 無効データマーク0xcc
                    }
                }
                else{
                    for(cnt = 0 ; cnt < 26 ; cnt++,data_poi++){
                        *data_poi = 0xcb;   // 26 無効データマーク0xcb  xxx16 2011.05.16
                    }
                }
                */

Printf("##realscan_err_data_add()  unit_no  %d   (%02X) %d \r\n", unit_no, realscan_buff.over_unit[Oct_cnt], Oct_cnt);
                for(cnt = 0 ; cnt < 26 ; cnt++,data_poi++){
                    //*data_poi = (chk_noise(group_registry.frequency) == 0) ? 0xcc : 0xcb;   // 26 無効データマーク0xcc/ 26 無効データマーク0xcb  xxx16 2011.05.16
                    *data_poi = (NOISE_CH[RF_buff.rf_req.current_group-1] == 0) ? 0xcc : 0xcb;   // 0xcc:無線通信エラー/ 0xcb:チャンネルビジー  // sakaguchi 2021.01.25
                }

                *data_poi++ = unit_no;                                          // 01 子機番号
                *data_poi++ = 0;                                                // 01 RSSI(未受信は0)
                for(cnt = 0 ; cnt < 2 ; cnt++,data_poi++){
                    *data_poi = 0x00;       // 02 最終記録からの経過秒数
                }
                *data_poi++ = 0;                                                // 01 BATT/ALM
                for(cnt = 0 ; cnt < 27 ; cnt++,data_poi++){
                    *data_poi = 0x00;       // 27 無効データマーク0x00
                }

                b_w_chg.word = (uint16_t)(b_w_chg.word + 64);
                rtn++;
            }
            cmp_data = (uint8_t)(cmp_data << 1);
        }
    }

    *size_poi++ = b_w_chg.byte[0];
    *size_poi++ = b_w_chg.byte[1];
    return(rtn);
}
//***** end of realscan_err_data_add(void)



/**
 *  リアルスキャンデータ(FORMAT1)に子機の(自律動作用)子機設定フラグを追記する
 * @param format_no
 */
void realscan_data_para(uint8_t format_no)
{
    uint8_t *poi;
    uint8_t unit_size;
    int cnt;
    uint16_t G_No;
    uint16_t U_No;
    union{
        uint8_t byte[2];
        uint16_t word;
    }b_w_chg;
    union{
        uint8_t byte[4];
        uint32_t w_word;
    }b_l_chg;



//  b_w_chg.byte[0] = sram.auto_format1.data_byte[0];
//  b_w_chg.byte[1] = sram.auto_format1.data_byte[1];
    switch(format_no){
        default:
        case 1:
            memcpy(b_w_chg.byte , rf_ram.auto_format1.data_byte , 2);
            poi = &rf_ram.auto_format1.kdata[0].group_no;
            break;
        case 2:
            memcpy(b_w_chg.byte , rf_ram.auto_format2.data_byte , 2);
            poi = &rf_ram.auto_format2.kdata[0].group_no;
            break;
        case 3:
            memcpy(b_w_chg.byte , rf_ram.auto_format3.data_byte , 2);
            poi = &rf_ram.auto_format3.kdata[0].group_no;
            break;
    }

    unit_size = (uint8_t)(b_w_chg.word / 64);               // 1子機あたり64byteなので子機数を把握する


    for(cnt = 0  ; cnt < unit_size ; cnt++){
        G_No = *poi;
        poi += 32;
        U_No = *poi;
        //get_unit_no_info(G_No,U_No);
        get_regist_relay_inf((uint8_t)G_No, (uint8_t)U_No, rpt_sequence);       // 対象子機の登録情報読み出す(RTR-574/576は子機番号の小さい方へ読み替えた返り値)

        poi -= 25;
        //*poi = regist.unit.flags;                     // [07]
        *poi = ru_registry.rtr501.set_flag;             // [07]
        poi++;
        //*poi = regist.unit.kind;                      // [08] データ種類も入れるようにした  xxx20 2010.07.20
        *poi = ru_registry.rtr501.header;               // [08] データ種類も入れるようにした

//-----ここから-----RTR-505対応で追加--------------------------------------------------------------------------
        poi++;
        /*
        if(ru_registry.rtr501.header == 0xF8){          // ##### RTR-505の場合 #####
            *poi = ru_registry.rtr505.header;           // [09]
        }
        else{
            *poi = 0x00;                                // [09]
        }
        */
        *poi = (ru_registry.rtr501.header == 0xF8) ? (ru_registry.rtr505.set_flag >> 5) : 0x00;	// ##### RTR-505の場合 #####/ [09]  2020 07 22
        poi++;
//-----ここまで------------------------------------------------------------------------------------------------

        if((ru_registry.rtr501.header == 0xFA)||(ru_registry.rtr501.header == 0xF9)){       // ##### RTR-574,576の場合 #####
            if(ru_registry.rtr501.number == U_No){
                //*poi = regist.unit574.ch_zoku[0];     // [10]
                *poi = ru_registry.rtr574.attribute[0]; // [10]
                poi++;
                *poi = ru_registry.rtr574.attribute[1]; // [11]
            }
            else{                                       // ##### RTR-501,502,503の場合 #####
                *poi = ru_registry.rtr574.attribute[2]; // [10]
                poi++;
                *poi = ru_registry.rtr574.attribute[3]; // [11]
            }
        }
        else{
                *poi = ru_registry.rtr501.attribute[0]; // [10]
                poi++;
                *poi = ru_registry.rtr501.attribute[1]; // [11]
        }
        poi++;

        b_l_chg.w_word = ru_registry.rtr501.serial;

        *poi++ = b_l_chg.byte[0];                   // [12]
        *poi++ = b_l_chg.byte[1];                   // [13]
        *poi++ = b_l_chg.byte[2];                   // [14]
        *poi++ = b_l_chg.byte[3];                   // [15]

        if((ru_registry.rtr501.header == 0xFA)||(ru_registry.rtr501.header == 0xF9)){       // ##### RTR-574,576の場合 #####
                *poi++ = ru_registry.rtr574.name[0];    // [16] 子機名
                *poi++ = ru_registry.rtr574.name[1];    // [17] "
                *poi++ = ru_registry.rtr574.name[2];    // [18] "
                *poi++ = ru_registry.rtr574.name[3];    // [19] "
                *poi++ = ru_registry.rtr574.name[4];    // [20] "
                *poi++ = ru_registry.rtr574.name[5];    // [21] "
                *poi++ = ru_registry.rtr574.name[6];    // [22] "
                *poi++ = ru_registry.rtr574.name[7];    // [23] "
        }
        else{                                           // ##### RTR-501,502,503の場合 #####
                *poi++ = ru_registry.rtr501.name[0];        // [16] 子機名
                *poi++ = ru_registry.rtr501.name[1];        // [17] "
                *poi++ = ru_registry.rtr501.name[2];        // [18] "
                *poi++ = ru_registry.rtr501.name[3];        // [19] "
                *poi++ = ru_registry.rtr501.name[4];        // [20] "
                *poi++ = ru_registry.rtr501.name[5];        // [21] "
                *poi++ = ru_registry.rtr501.name[6];        // [22] "
                *poi++ = ru_registry.rtr501.name[7];        // [23] "
        }

        if(ru_registry.rtr501.header == 0xF8){              // ##### RTR-505の場合 #####
            //*poi++ = regist.unit505.kisyu_code;
            *poi++ = ru_registry.rtr505.model;
            //*poi++ = regist.unit505.disp_unit;
            *poi++ = ru_registry.rtr505.display_unit;
            poi += 38;
        }
        else{
            poi += 40;
        }


    }
}
//***** end of realscan_data_para(void) *****


/**
 * リアルスキャンデータを copy_realscan する時、既に取得済みデータがあるかチェックする
 * @param format_no
 * @param group_no
 * @param unit_no
 * @return      ダブっていない時、   NORMAL_END ダブっている時、     ERROR_END
 */
static uint8_t check_double(uint8_t format_no,uint16_t group_no,uint8_t unit_no)
{
    uint8_t *out_poi;
    uint8_t unit_size;
    uint8_t rtn = NORMAL_END;
    int cnt;

    union{
        uint8_t byte[4];
        uint16_t word;
    }b_w_chg;

//  if(format_no == 1)      out_poi = &sram.auto_format1.data_byte[0];  // DATA FORMATにより保存エリアが異なる
//  else if(format_no == 2) out_poi = &sram.auto_format2.data_byte[0];  // DATA FORMATにより保存エリアが異なる
//  else                    return NORMAL_END;
    switch(format_no){
        case 1:
            out_poi = rf_ram.auto_format1.data_byte;    // DATA FORMATにより保存エリアが異なる
            break;
        case 2:
            out_poi = rf_ram.auto_format2.data_byte;    // DATA FORMATにより保存エリアが異なる
            break;
        case 3:
            out_poi = rf_ram.auto_format3.data_byte;    // DATA FORMATにより保存エリアが異なる
            break;
        default:
            return (NORMAL_END);
            break;
    }


    b_w_chg.byte[0] = *out_poi++;               // 既に保存されている子機データ数を把握する
    b_w_chg.byte[1] = *out_poi++;               //              "
    unit_size = (uint8_t)(b_w_chg.word / 64);       //              "

    for(cnt = 0 ; cnt < unit_size ; cnt++){     // 子機台数分ループ
        if(*out_poi == group_no){
            out_poi += 32;
            if(*out_poi == unit_no){
                rtn = ERROR_END;
                break;
            }
            else out_poi += 32;
        }
        else{
            out_poi += 64;
        }
    }
    return (rtn);
}


/**
 * リアルスキャンの生データ(30byte)の中に引数の子機番号データが存在するチェックする
 * @param unit_no   子機番号(1～)
 * @return  存在する=NORMAL_END   存在せず=ERROR_END
 */
static uint8_t check_realscan_data(uint8_t unit_no)
{
    uint8_t unit_size;
    int cnt;
    uint8_t rtn = ERROR_END;

    uint16_t dataByte;

    dataByte = *(uint16_t *)rf_ram.realscan.data_byte;  // 既に保存されている子機データ数を把握する

    unit_size = (uint8_t)(dataByte / 30);

    for(cnt = 0 ; cnt < unit_size ; cnt++){
        if(rf_ram.realscan.data.format1[cnt].unit_no == unit_no){
            rtn = NORMAL_END;
            break;
        }
    }
    return (rtn);
}
//***** end of check_realscan_data(uchar unit_no) *****

/**
 * リアルスキャンの生データ(30byte)の中に引数の子機番号データが存在するチェックする
 * @param unit_no   子機番号(1～)
 * @return      存在する=NORMAL_END   存在せず=ERROR_END
 */
static uint8_t dual_check_574(uint8_t unit_no)
{
uint8_t check_No;
    uint8_t rtn;
    //get_unit_no_info(RF_buff.rf_req.current_group , unit_no);     // 子機登録情報を読み出す
// sakaguchi 2021.10.05 ↓
    //get_regist_relay_inf(RF_buff.rf_req.current_group, unit_no, rpt_sequence);      // 対象子機の登録情報読み出す(RTR-574/576は子機番号の小さい方へ読み替えた返り値)
    char tmp_route[256];
    memset(tmp_route, 0x00, sizeof(tmp_route));
    get_regist_relay_inf(RF_buff.rf_req.current_group, unit_no, tmp_route);      // 対象子機の登録情報読み出す(RTR-574/576は子機番号の小さい方へ読み替えた返り値)
// sakaguchi 2021.10.05 ↑

    if((ru_registry.rtr501.header == 0xFA)||(ru_registry.rtr501.header == 0xF9)){       // RTR-576を追加 2012.02.13
// sakaguchi 2021.01.21 ↓
#if 0
//  	check_No = (ru_registry.rtr574.number == unit_no) ? (uint8_t)(ru_registry.rtr574.number + 1) : ru_registry.rtr574.number;    // 登録情報通りの子機番号なら後半子機番号があるかチェック
//	    rtn = check_realscan_data(check_No);                            // 相方がいない場合は ERROR_END とする
		check_No = ru_registry.rtr574.number;					// 2020/08/28 segi
		if(ru_registry.rtr574.number == unit_no){
			rtn = check_realscan_data(check_No);				// 前半がrf_ram.realscan.にいない場合は ERROR_END とする
			if(rtn == NORMAL_END){
				rtn = check_realscan_data(check_No + 1);		// 後半がrf_ram.realscan.にいない場合は ERROR_END とする
			}
		}
		else{
//			rtn = NORMAL_END;
			rtn = check_realscan_data(check_No);				// 前半がrf_ram.realscan.にいない場合は ERROR_END とする
		}
#endif
		check_No = ru_registry.rtr574.number;               // 小さい子機番号を取得
		rtn = check_realscan_data(check_No);				// 前半がrf_ram.realscan.にいない場合は ERROR_END とする
		if(rtn == NORMAL_END){
			rtn = check_realscan_data((uint8_t)(check_No + 1));		// 後半がrf_ram.realscan.にいない場合は ERROR_END とする
		}
// sakaguchi 2021.01.21 ↑
    }
    else{
        rtn = NORMAL_END;
    } 


	if(rtn != NORMAL_END){
		//clr_flag(realscan_buff.over_unit,unit_no);     // 前半後半揃わない場合は受信済フラグクリア
		clr_flag(realscan_buff.over_unit,check_No);     // 前半後半揃わない場合は受信済フラグクリア     // sakaguchi 2021.01.21
		clr_flag(realscan_buff.over_unit,(uint8_t)(check_No+1));   // 前半後半揃わない場合は受信済フラグクリア     // sakaguchi 2021.01.21
	}


    return (rtn);
}
//***** end of dual_check_574(uchar unit_no) *****


/**
 * ##### 自律動作の全ての入り口 #####
 * 警報監視
 * モニタリング
 * (外部電源、電池残量少、外部接点入力)
 * 自動吸上げ
 * @param actual_events
 * @return
 */
uint32_t AUTO_control(uint32_t actual_events)
{
    uint8_t exec_WMD_W = 0;     // テスト用自律フラグ
    uint8_t exec_WMD_M = 0;     // テスト用自律フラグ
    uint8_t exec_WMD_D = 0;     // テスト用自律フラグ
    uint8_t exec_WMD_I = 0;     // テスト用自律フラグ
    uint8_t exec_WMD_R = 0;     // レギュラーモニタリング用フラグ
    uint8_t exec_WMD_F = 0;     // フルモニタリング用フラグ

    int32_t Err = 0;        //とりあえず 2019.Dec.26
    volatile uint32_t rtn;
    uint16_t WarCnt;

/*
    union{
        uint8_t byte[2];
        uint16_t word;
    }b_w_chg;
*/
    //uint32_t actual_events;



//#define FLG_EVENT_KEIHO         BIT_0
//#define FLG_EVENT_MONITOR       BIT_1
//#define FLG_EVENT_SUCTION       BIT_2


    //tx_event_flags_get (&g_wmd_event_flags, FLG_EVENT_KEIHO | FLG_EVENT_MONITOR | FLG_EVENT_SUCTION, TX_OR_CLEAR, &actual_events, TX_NO_WAIT);

    if(actual_events & FLG_EVENT_KEIHO){
        exec_WMD_W = 1;
		EWAIT_res = 1;		// モニタリング中
    }
    if(actual_events & FLG_EVENT_MONITOR){
        exec_WMD_M = 1;
		EWAIT_res = 1;		// モニタリング中
    }
    if(actual_events & FLG_EVENT_SUCTION){
        exec_WMD_D = 1;
		EWAIT_res = 2;		// 吸い上げ中
    }
    if(actual_events & FLG_EVENT_INITIAL){
	    if(isUSBState() == USB_CONNECT ){			// USBが接続されているとAT_start()もさせない 2020/09/11 segi
//	    	PutLog(LOG_RF, "AT_start() cancel");	// debug
            mate_at_start = 1;                      // 再セット// sakaguchi 2020.10.07
		}											// USBが接続されているとAT_start()もさせない 2020/09/11 segi
		else{										// USBが接続されているとAT_start()もさせない 2020/09/11 segi
        	exec_WMD_I = 1;
			EWAIT_res = 1;		// モニタリング中
		}											// USBが接続されているとAT_start()もさせない 2020/09/11 segi
    }
    if(actual_events & FLG_EVENT_REGUMONI){
        exec_WMD_R = 1;
		EWAIT_res = 1;		// モニタリング中
    }
    if(actual_events & FLG_EVENT_FULLMONI){
        exec_WMD_F = 1;
		EWAIT_res = 1;		// モニタリング中
    }

//  exec_WMD_W = 1;
//  exec_WMD_M = 1;
//  exec_WMD_D = 1;


    NTH_2in1 = 0;

    // 自律動作の開始
    if(exec_WMD_I == 1){
        rtn = AT_start();
    }

    // レギュラーモニタリング開始
    if(exec_WMD_R == 1){
        RF_regu_moni();
        // モニタデータ送信
        if( *(uint16_t *)&rf_ram.auto_format1.data_byte[0] ){       // データが0以外(有る)場合送信
            if(Err != 0){
                tx_thread_sleep(WAIT_5SEC);     /*Delay(5000);*/    // 連続してサーバーにアクセスすると、拒否される
            }
            Err = SendMonitorFile(0);
            //PutLog(LOG_RF, "Monitor %ld \r\n", Err);      // debug
            if(Err == 0){

                CurrRead_ReTry = 0;                             // GSM通信正常終了なのでリトライしない
            }
            else{
                if(CurrRead_ReTry < 2){
                    CurrRead_ReTry++;       // CurrRead_ReTry < [リトライ回数] かチェック
                }
                else{
                    CurrRead_ReTry = 0;     // リトライ終了
                }
            }
        }
    }

    // フルモニタリング開始
    if(exec_WMD_F == 1){
        if(1 == (root_registry.auto_route & 0x01)){     // 自動ルート設定の場合
            RF_full_moni();         // フルモニタリングを実行
        }else{
            RF_regu_moni();         // 手動ルートのレギュラーモニタリングを実行
        }
        // モニタデータ送信
        if( *(uint16_t *)&rf_ram.auto_format1.data_byte[0] ){       // データが0以外(有る)場合送信
            if(Err != 0){
                tx_thread_sleep(WAIT_5SEC);     /*Delay(5000);*/    // 連続してサーバーにアクセスすると、拒否される
            }
            Err = SendMonitorFile(0);
            //PutLog(LOG_RF, "Monitor %ld \r\n", Err);      // debug
            if(Err == 0){

                CurrRead_ReTry = 0;                             // GSM通信正常終了なのでリトライしない
            }
            else{
                if(CurrRead_ReTry < 2){
                    CurrRead_ReTry++;       // CurrRead_ReTry < [リトライ回数] かチェック
                }
                else{
                    CurrRead_ReTry = 0;     // リトライ終了
                }
            }
        }
    }

    //警報監視かモニタリングをやる
    if((exec_WMD_W == 1)||(exec_WMD_M == 1)){
        if(exec_WMD_W == 1){

            memcpy(&auto_control.warning_back[0].group_no, &auto_control.warning[0].group_no, 1024);        // 警報判定の為の前回情報をバックアップ
            memcpy(&auto_control.w_config_back[0].group_no, &auto_control.w_config[0].group_no, 1024);      // 警報判定の為の前回情報をバックアップ(警報フラグ16bit対応) w_onoff対応 2012.10.22
            auto_control.crc = crc32_bfr(&auto_control, 5122);		// ((128*8)*5)+2	CRC追加	2020/08/22 segi
            RF_monitor_execute(0);
// sakaguchi UT-0035 ↓
Printf("## Warning_Check 1\r\n");
            WarCnt = Warning_Check();
            if (WarCnt > 0){
                    auto_control.warcnt = (UCHAR)(auto_control.warcnt + WarCnt);
                    auto_control.crc = crc32_bfr(&auto_control, 5122);		// ((128*8)*5)+2	CRC追加	2020/08/22 segi
// sakaguchi UT-0035 ↑
//ここに Warning送信を追加
                    if((FirstAlarmMoni == 1) && (my_config.websrv.Mode[0] == 0)){       // sakaguchi 2021.03.01
                        Err = SendWarningFile(3, 0);
                        //FirstAlarmMoni = 0;                                           // sakaguchi 2021.11.25 del
                    }else{
                        Err = SendWarningFile(0, 0);
                        //FirstAlarmMoni = 0;                                           // sakaguchi 2021.11.25 del
                    }
//                  Err = Gsm_Send_Warning( (UBYTE *)&sram.auto_format2.kdata[0], 0 );

                    if(Err == 0){           // ########## ネットワーク送信が成功した場合 ##########
                        Warning_Check_After();
                    }
                    else{                   // ########## ネットワーク送信が失敗した場合 ##########
                        if(Err == -1){
                            memcpy(&auto_control.warning[0].group_no, &auto_control.warning_back[0].group_no, 1024);        // 警報判定の為の前回情報をバックアップを復活
                            memcpy(&auto_control.w_config[0].group_no, &auto_control.w_config_back[0].group_no, 1024);  // 警報判定の為の前回情報をバックアップを復活(警報フラグ16bit対応)
                            auto_control.crc = crc32_bfr(&auto_control, 5122);		// ((128*8)*5)+2	CRC追加	2020/08/22 segi
							Warning_Check();        // 警報
Printf("## Warning_Check 2\r\n");
                        }
                    }
            }
            else{
                if((FirstAlarmMoni == 1) && (my_config.websrv.Mode[0] == 0)){      // sakaguchi 2021.03.01
                    Err = SendWarningFile(3, 0);
                    //FirstAlarmMoni = 0;                                           // sakaguchi 2021.11.25 del
                }
            }

        }
        else if(exec_WMD_M == 1){
            rtn = RF_monitor_execute(1);
            //PutLog(LOG_RF, "Monitor %ld %02X", rtn, rtn);     // debug

        }


        if(exec_WMD_M == 1){
            // モニタのデータ送信
//          b_w_chg.byte[0] = rf_ram.auto_format1.data_byte[0];
//          b_w_chg.byte[1] = rf_ram.auto_format1.data_byte[1];
            if( *(uint16_t *)&rf_ram.auto_format1.data_byte[0] ){       // データが0以外(有る)場合送信
                if(Err != 0){
                    tx_thread_sleep(WAIT_5SEC);     /*Delay(5000);*/    // 連続してサーバーにアクセスすると、拒否される
                }
                Err = SendMonitorFile(0);
                //PutLog(LOG_RF, "Monitor %ld \r\n", Err);      // debug
                if(Err == 0){

                    CurrRead_ReTry = 0;                             // GSM通信正常終了なのでリトライしない
                }
                else{
                    if(CurrRead_ReTry < 2){
                        CurrRead_ReTry++;       // CurrRead_ReTry < [リトライ回数] かチェック
                    }
                    else{
                        CurrRead_ReTry = 0;     // リトライ終了
                    }
                }
            }
        }
    }

    // データ吸い上げをやる
    if(exec_WMD_D == 1){

        rtn = (Download_ReTry == 0) ? RF_event_execute(0) : RF_event_execute(1);        // 初回の自動吸上げ/ リトライの自動吸上げ

        if(rtn == ERR(RF, NOTFINISH)){
            rtn = RF_event_execute(1);      // リトライの自動吸上げ   (NOERROR) or (NOTFINISH)
        }

        if(rtn == ERR(RF, NOERROR)){
            Download_ReTry = 0;                         // 正常終了の場合はリトライしない
        }

		else if(rtn == ERR(RF , BREAK_END)){			// 2020/08/17 segi
            Download_ReTry = 0;							// 2020/08/17 segi	緊急停止の場合は、WAIT明けはリトライで新規吸い上げ
		}												// 2020/08/17 segi

        else if(rtn == ERR(RF, NOTFINISH)){             // 異常終了の子機が有る場合
            if(Download_ReTry < 1){
                Download_ReTry++;       // Download_ReTry < [リトライ回数] かチェック
            }
            else{
                Download_ReTry = 0;     // リトライ終了
            }
        }

    }

	EWAIT_res = 0;		// 何もしていない(全部終わった)
    return (rtn);


}
//***** end of AUTO_control(void) *****

/**
 * 警報の有無のチェック
 *  戻り値：    警報数
 */
static void Warning_Check_After(void)
{
    int i,j;
//  uint16_t kw_cnt = 0;        //2019.Dec.26 コメントアウト
//  uint8_t alm = 0;        //2019.Dec.26 コメントアウト

    uint16_t now ,on_off, before;
//  uint8_t Warn;

    uint16_t W_T;
    uint16_t mask;
    uint16_t spec;              // sakaguchi 2021.04.07

    Printf("\r\nWarning_Check_After\r\n");
    for(i=0;i<128;i++){
        if((auto_control.w_config[i].group_no==0xff) && (auto_control.w_config[i].unit_no==0xff)){
            break;
        }

//      now = SRD_W( auto_control.w_config[i].now );
//      before = SRD_W( auto_control.w_config[i].before );
//      on_off = SRD_W( auto_control.w_config[i].on_off );

        now = *(uint16_t *)auto_control.w_config[i].now;            //キャストに変更 2020.01.21
        before = *(uint16_t *)auto_control.w_config[i].before;            //キャストに変更 2020.01.21
        on_off = *(uint16_t *)auto_control.w_config[i].on_off;              //キャストに変更 2020.01.21

        //  before=0  now=1  on_off=1 の場合、now = 0 にする
        mask = 0x0001;
        W_T = now;

//      for(j=0;j<8;j++){
        for(j=0;j<16;j++){		
//          if((before & mask)==0 && (now & mask)==1 && (on_off & mask)==1 ){
            if((before & mask) == 0 && (now & mask) != 0 && (on_off & mask) != 0 ){		// 2020/09/12 segi
 
                W_T = (uint16_t)(W_T &(~mask));
            }
            mask = (uint16_t)(mask<<1);
        }

// sakaguchi 2021.04.07 ↓
        get_regist_relay_inf((uint8_t)auto_control.w_config[i].group_no, (uint8_t)auto_control.w_config[i].unit_no, rpt_sequence);
        spec = GetSpecNumber( ru_registry.rtr501.serial , ru_registry.rtr505.model );

        // CH警報が全てOFFならセンサ警報もOFFする（但しRTR-574,RTR-576は除く）
        if((spec != SPEC_574) && (spec != SPEC_576)){
            if((W_T & 0x0303) == 0){
                mask = 0x0010;
                W_T = (uint16_t)(W_T &(~mask));
            }
        }
// sakaguchi 2021.04.07 ↑

        SWR_W( auto_control.w_config[i].now , W_T);

        //Printf("[Warning (%d) group_no:%d unit_no:%d]", i,auto_control.w_config[i].group_no,auto_control.w_config[i].unit_no);
        //Printf("    [before:%04X now:%04X on_off:%04X W_T:%04X]\r\n", before, now , on_off, W_T );

    }
    auto_control.crc = crc32_bfr(&auto_control, 5122);		// ((128*8)*5)+2	CRC追加	2020/08/22 segi
    Printf("\r\n");
}


// sakaguchi 2020.10.07 ↓
/**
 * @brief   警報監視OFFの子機の警報情報をクリアする
 * @return  無し
*/
static void Warning_Info_Clear(void)
{
	int i,j;
    uint16_t adr,len;
	uint8_t group_no;
	uint8_t unit_no;
    def_ru_registry     read_ru_regist;                 // 子機情報
    def_group_registry  read_group_registry;            // グループ情報

    Printf("Warning_Info_Clear() \r\n");

	for(i = 0; i < 128; i++){
		if((0xff == auto_control.w_config[i].group_no) && (0xff == auto_control.w_config[i].unit_no)){
		    break;
		}
		group_no = auto_control.w_config[i].group_no;
		unit_no	= auto_control.w_config[i].unit_no;

		adr = RF_get_regist_group_adr((uint8_t)group_no, &read_group_registry);		// グループ情報読み出し
		if(0 == adr){
			continue;			// グループ情報無しのため次ループへ
		}

		adr = (uint16_t)(adr + read_group_registry.length);					// グループヘッダ分アドレスＵＰ

		if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS)			// シリアルフラッシュ ミューテックス確保 スレッド確定
		{
			for(j = 0; j < read_group_registry.altogether; j++)				// 中継機情報
			{
				serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), 2, (char *)&len);	// 中継機情報サイズを取得
				adr = (uint16_t)(adr + len);														// 中継機情報分アドレスＵＰ
			}
			for(j = 0; j < read_group_registry.remote_unit; j++)			// 子機情報
			{
				serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), sizeof(read_ru_regist), (char *)&read_ru_regist);	// 子機情報取得

				if(unit_no == read_ru_regist.rtr501.number){				// 子機番号が警報監視対象でない
					if(0x01 != (read_ru_regist.rtr501.set_flag & 0x01)){
						// 警報情報クリア
						auto_control.w_config[i].now[0] = 0x00;
						auto_control.w_config[i].now[1] = 0x00;
						auto_control.w_config[i].before[0] = 0x00;
						auto_control.w_config[i].before[1] = 0x00;
						auto_control.w_config[i].on_off[0] = 0x00;
						auto_control.w_config[i].on_off[1] = 0x00;
						// 警報情報（バックアップ）クリア
						auto_control.w_config_back[i].now[0] = 0x00;
						auto_control.w_config_back[i].now[1] = 0x00;
						auto_control.w_config_back[i].before[0] = 0x00;
						auto_control.w_config_back[i].before[1] = 0x00;
						auto_control.w_config_back[i].on_off[0] = 0x00;
						auto_control.w_config_back[i].on_off[1] = 0x00;
					}
				}
                adr = (uint16_t)(adr + read_ru_regist.rtr501.length);       // 子機情報分アドレスＵＰ
			}
			tx_mutex_put(&mutex_sfmem);
		}
	}
}
// sakaguchi 2020.10.07 ↑


/**
 * 自律動作の自動データ吸上げ
 * @param order order 0:初回の通信   1:リトライ
 * @return  order 0:初回の通信   1:リトライ
 */
uint32_t RF_event_execute(uint8_t order)
{
    uint8_t rtn = 0;        ///< @bug セットのみ未使用
    int rty_cnt;
    int group_size,group_cnt;   // 登録グループ数,グループ個数ループするカウンタ
    int MaxDownload;            // 1グループ内の最大子機台数

    uint8_t exec_WMD_W = 0;     // テスト用自律フラグ
    uint8_t exec_WMD_M = 0;     // テスト用自律フラグ
    int32_t Err = 0;        //とりあえず 2019.Dec.26
    uint16_t WarCnt;


/*
    union{
        uint8_t byte[2];
        uint16_t word;
    }b_w_chg;
*/
    int32_t l_rtn;

    volatile uint32_t f_rtn;


    f_rtn = ERR(RF , NOERROR);


    group_size = get_regist_group_size();       // 登録ファイルからグループ取得する関数
//  group_size = 1;                             // dbg dbg dbg dbg 登録ファイルからグループ取得する関数

    Printf("    .RF_event_execute group size %d \r\n", group_size);

    if((root_registry.auto_route & 0x01) == 1){     // 自動ルート機能ONの場合
        if( true == RF_full_moni_start_check(1) ){
            RF_full_moni(); // 前回失敗している子機がある場合はフルモニタリングを実施
        }
    }

    for(group_cnt = 0 ; group_cnt < group_size ; group_cnt++){
        download_err[group_cnt] = 0;                            // 無線通信エラー子機なければ0


        RF_buff.rf_req.current_group = (char)(group_cnt + 1);       // 通信対象グループ番号を０～指定

        if(order == 0){
            // 初回通信(リトライでない)時の通信対象子機のフラグをONにする。
            group_registry.max_unit_no = get_regist_gather_unit(RF_buff.rf_req.current_group);

//          download_buff.do_unit[0]...[15] = 0b00000110;   // ←←←←←←←←←←←←[関数に置き換える]
        }
        else{
            // リトライ時の通信対象子機のフラグをONにする。
            //memcpy(&download_buff.do_unit,&download_buff.ng_unit,16);         // 初回通信でngだった子機をリトライする
            set_download_retry_do_flag((uint8_t)group_cnt);
        }

    //  group_registry.max_unit_no = 1;     // 1個のグループに含まれる子機番号の最大値 ←←←←←←←[関数に置き換える]
        MaxDownload = (uint8_t)(group_registry.max_unit_no + 1) ;           // 吸上げ回数リミッタ

        Printf("    MaxDownload (%ld)  %d \r\n", MaxDownload, group_cnt);

        while(MaxDownload){
            RF_buff.rf_req.current_group = (char)(group_cnt + 1);       // 通信対象グループ番号を０～指定
            //RF_command_execute(0x61);             // 1台分のデータ吸上げ
            Printf("    .RF_event_execute Start download (%ld)\r\n", MaxDownload);

            rtn = (dbg_cmd != 6) ? auto_download_New(0x61) : auto_download_New(0x62);// デバッグ用に全データ吸上げする設定

            Printf("    .RF_event_execute end download (%ld)\r\n", rtn);

            if(rtn != AUTO_END){

//              uncmpressData( 1 );                                 // 圧縮データの回答処理（データ＋Propertyセット） 2020.01.29 サブ関数化

                if(r500.node == COMPRESS_ALGORITHM_NOTICE)      // 圧縮データを解凍
                {
                    memcpy(work_buffer, huge_buffer, r500.data_size);
                    r500.data_size = (uint16_t)LZSS_Decode((uint8_t *)work_buffer, (uint8_t *)huge_buffer);     // 2022.06.09

                    property.acquire_number = r500.data_size;
                    property.transferred_number = 0;
                    r500.node = 0xff;                       // 解凍が重複しないようにする

                }






                memcpy(rf_ram.adr, huge_buffer,r500.data_size);     // 無線通信で吸上げたデータをxml作成用受け渡しバッファにコピー

                RF_buff.rf_res.data_size = r500.data_size;                  // 無線通信の結果バイト数を応答レジスタにコピー。

//-------------------- ↓ここから RTR-574対応の為に追加した部分 -------------------------------------------------------------------------------
                if(download574() == NORMAL_END){        // RTR-574 or RTR-576の吸い上げデータか調べる
                                                        // <<NORMAL_ENDじゃない条件>>
                                                        // ①[照度・紫外線][温度・湿度]両方が吸い上げ対象で、[照度・紫外線]だけ吸い上がった時
                                                        // ②吸い上げ対象がRTR-574で、無線通信エラーの場合
//-------------------- ↑ここまで -------------------------------------------------------------------------------------------------------------

                            if(RF_buff.rf_res.status == RFM_NORMAL_END){    // 正常終了したらデータをGSM渡し変数にコピー
                                if(RF_buff.rf_res.data_size > 63){
//-------------------- ↓ここから RTR-574対応の為に追加した部分 -------------------------------------------------------------------------------
                                    if(Chk574No == 0){          // 501,502,503,505は吸い上げ終わった。574,576は2個いち回(前半か後半片方指定の場合でも終了した)吸い上げた。
                                        download574_2in1();     // 吸上げデータの情報を、Download.Attach. にセットする。
//-------------------- ↑ここまで -------------------------------------------------------------------------------------------------------------
                                        rty_cnt = 1;        // 通信回数(1+リトライ)
                                        while(rty_cnt){
                                            //****************************************
                                            //*****      GSMでデータ送る関数     *****
                                            tx_thread_sleep (200);      // 2秒wait
        // とりあえずコメントアウト         l_rtn = Gsm_Send_Data(&rf_ram.header.data[0] , (RF_buff.rf_res.data_size - 64));
                                            //****************************************
        // とりあえずコメントアウト         if(l_rtn == ERR( GSM, NOERROR)) break;

                                            l_rtn = SendSuctionData(0);

                                            Printf("\r\nSendSuction Status %ld\r\n ", l_rtn);
                                            //if(l_rtn == ERR( LAN, NOERROR))   break;
                                            if(l_rtn == 0){
                                                break;
                                            }

                                            //l_rtn = 0;    break;          // <--------------------------------------------------- とりあえず実行させる

                                            rty_cnt--;
                                            if(rty_cnt){
                                                tx_thread_sleep (500);      // 5秒wait
                                            }
                                        }

        // とりあえずコメントアウト         if(l_rtn == ERR( GSM, NOERROR)){        // GSM通信が正常終了だった場合

                                        if(l_rtn == 0){             // <--------------------------------------------------- とりあえず実行させる




                                            if(DualData == 0){      //  シングルデータの場合
                                                    set_flag(download_buff.over_unit,rtn);                      // 通信成功したした子機のフラグセット
                                    //              b_w_chg.word = (uint16_t)(Download.Attach.end_no + 1);              // xxx05 2010.08.05 +1 を追記  次回吸い上げるデータ番号を決定する
                                                    set_download(RF_buff.rf_req.current_group , Download.Attach.unit_no , rf_ram.header.serial_no , (uint16_t)(Download.Attach.end_no + 1));    // ★★★GSM通信が成功した場合のみ★★★今回の最終データ番号を保存しておく
// 2023.05.26 ↓
                                                    first_download_flag_write(RF_buff.rf_req.current_group, Download.Attach.unit_no, FDF_SABUN_0);
                                                    first_download_flag_read(RF_buff.rf_req.current_group, Download.Attach.unit_no);  //debug
// 2023.05.26 ↑
                                            }
                                            else if(DualData ==1){  //  [照度・紫外線][温度・湿度]のデュアルデータの場合
                                                    set_flag(download_buff.over_unit,rtn);                      // 通信成功したした子機のフラグセット[温度・湿度]
                                                    set_flag(download_buff.over_unit,(uint8_t)(rtn-1));                 // 通信成功したした子機のフラグセット[照度・紫外線]
                //                                  b_w_chg.word = (uint16_t)(Download.Attach.end_no + 1);              // xxx05 2010.08.05 +1 を追記  次回吸い上げるデータ番号を決定する
                                                    set_download(RF_buff.rf_req.current_group , Download.Attach.unit_no , rf_ram.header.serial_no , (uint16_t)(Download.Attach.end_no + 1));    // ★★★GSM通信が成功した場合のみ★★★今回の最終データ番号を保存しておく
                                                    set_download(RF_buff.rf_req.current_group , (uint8_t)(Download.Attach.unit_no+1) , rf_ram.header.serial_no , (uint16_t)(Download.Attach.end_no + 1));   // ★★★GSM通信が成功した場合のみ★★★今回の最終データ番号を保存しておく
// 2023.05.26 ↓
                                                    first_download_flag_write(RF_buff.rf_req.current_group, Download.Attach.unit_no, FDF_SABUN_0);
                                                    first_download_flag_read(RF_buff.rf_req.current_group, Download.Attach.unit_no);  //debug
                                                    first_download_flag_write(RF_buff.rf_req.current_group, (uint8_t)(Download.Attach.unit_no+1), FDF_SABUN_0);
                                                    first_download_flag_read(RF_buff.rf_req.current_group, (uint8_t)(Download.Attach.unit_no+1));  //debug
// 2023.05.26 ↑

                                            }
                                        }
                                        else{
                                            if(DualData == 0){      //  シングルデータの場合
                                                set_flag(download_buff.ng_unit,rtn);        // GSM通信がエラーだったらNG子機フラグをセットする
                                            }
                                            else if(DualData ==1){  //  [照度・紫外線][温度・湿度]のデュアルデータの場合
                                                set_flag(download_buff.ng_unit,rtn);        // GSM通信がエラーだったらNG子機フラグをセットする
                                                set_flag(download_buff.ng_unit,(uint8_t)(rtn-1));       // GSM通信がエラーだったらNG子機フラグをセットする
                                            }

                                        }

                                    //  Printf("\r\n[RF ] Download  Last Data No=%d",b_w_chg.word);         // dbg
                                    //  b_w_chg.byte[0] = rf_ram.header.data_byte[0];                       // dbg
                                    //  b_w_chg.byte[1] = rf_ram.header.data_byte[1];                       // dbg
                                    //  Printf("    Download  Get DataByte=%d",b_w_chg.word);               // dbg
//-------------------- ↓ここから RTR-574対応の為に追加した部分 -------------------------------------------------------------------------------
                                    }
//-------------------- ↑ここまで -------------------------------------------------------------------------------------------------------------
                                }
                            }
                            else{
                                set_flag(download_buff.ng_unit,rtn);        // 無線通信がエラーだった
                                download_err[group_cnt]++;                  // 無線通信エラー子機あった(無線通信エラー子機なければ0)

                                if((ru_registry.rtr501.header == 0xFA)||(ru_registry.rtr501.header == 0xF9)){               // 登録情報が RTR-574 or RTR-576 の場合
                                    //PutLog(LOG_RF,"$D $E  %.8s",&ru_registry.rtr574.name[0]);
                                    PutLog(LOG_RF,"$D $E (%08lX)", ru_registry.rtr574.serial);
                                }
                                else{
                                    //PutLog(LOG_RF,"$D $E %.8s",&ru_registry.rtr501.name[0]);
                                    PutLog(LOG_RF,"$D $E (%08lX)", ru_registry.rtr501.serial);
                                }
                            }




//-------------------- ↓RTR-574対応の為に追加した部分 ----------------------------------------------------------------------------------------
                }
//-------------------- ↑ここまで -------------------------------------------------------------------------------------------------------------


				if(WaitRequest == WAIT_REQUEST){		// USB差されたなどで自律動作を速やかに停止させる	2020/08/17 segi
					f_rtn = ERR(RF , BREAK_END);
                    DebugLog( LOG_DBG, "AutoDownload Break End(Wait Request)");
					goto BREAK_END;
				}


// ↓↓↓ここから モニタリング、警報監視のイベント確認&実行 ----------
                uint32_t actual_events = 0;
                tx_event_flags_get (&g_wmd_event_flags, FLG_EVENT_KEIHO | FLG_EVENT_MONITOR  , TX_OR_CLEAR, &actual_events, TX_NO_WAIT);
                Printf("### Download after Event Check !!!!!!  %04X\r\n", actual_events & (FLG_EVENT_KEIHO | FLG_EVENT_MONITOR));

                if(actual_events & FLG_EVENT_KEIHO){
                    exec_WMD_W = 1;
                }
                if(actual_events & FLG_EVENT_MONITOR){
                    exec_WMD_M = 1;
                }

                //警報監視かモニタリングをやる
                if((exec_WMD_W == 1)||(exec_WMD_M == 1)){
                    if(exec_WMD_W == 1){

                        memcpy(&auto_control.warning_back[0].group_no, &auto_control.warning[0].group_no, 1024);        // 警報判定の為の前回情報をバックアップ
                        memcpy(&auto_control.w_config_back[0].group_no, &auto_control.w_config[0].group_no, 1024);      // 警報判定の為の前回情報をバックアップ(警報フラグ16bit対応) w_onoff対応 2012.10.22
                        auto_control.crc = crc32_bfr(&auto_control, 5122);		// ((128*8)*5)+2	CRC追加	2020/08/22 segi
                        RF_monitor_execute(0);
            // sakaguchi UT-0035 ↓
                        WarCnt = Warning_Check();
                        if (WarCnt > 0){
                                auto_control.warcnt = (UCHAR)(auto_control.warcnt + WarCnt);
		                        auto_control.crc = crc32_bfr(&auto_control, 5122);		// ((128*8)*5)+2	CRC追加	2020/08/22 segi
            // sakaguchi UT-0035 ↑
            //ここに Warning送信を追加
                                if((FirstAlarmMoni == 1) && (my_config.websrv.Mode[0] == 0)){       // sakaguchi 2021.03.01
                                    Err = SendWarningFile(3, 0);
                                    //FirstAlarmMoni = 0;                                           // sakaguchi 2021.11.25 del
                                }else{
                                Err = SendWarningFile(0, 0);
                                    //FirstAlarmMoni = 0;                                           // sakaguchi 2021.11.25 del
                                }
            //                  Err = Gsm_Send_Warning( (UBYTE *)&sram.auto_format2.kdata[0], 0 );

                                if(Err == 0){           // ########## ネットワーク送信が成功した場合 ##########
                                    Warning_Check_After();
                                }
                                else{                   // ########## ネットワーク送信が失敗した場合 ##########
                                    if(Err == -1){
                                        memcpy(&auto_control.warning[0].group_no, &auto_control.warning_back[0].group_no, 1024);        // 警報判定の為の前回情報をバックアップを復活
                                        memcpy(&auto_control.w_config[0].group_no, &auto_control.w_config_back[0].group_no, 1024);  // 警報判定の為の前回情報をバックアップを復活(警報フラグ16bit対応)
                                       	auto_control.crc = crc32_bfr(&auto_control, 5122);		// ((128*8)*5)+2	CRC追加	2020/08/22 segi
										Warning_Check();        // 警報
Printf("## Warning_Check 2\r\n");
                                    }
                                }
                        }
                        else{       // Warningが無い場合も警報監視１回目は送信する
                            if((FirstAlarmMoni == 1) && (my_config.websrv.Mode[0] == 0)){       // sakaguchi 2021.03.01
                                Err = SendWarningFile(3, 0);
                                //FirstAlarmMoni = 0;                                               // sakaguchi 2021.11.25 del
                            }
                        }

                    }
                    else if(exec_WMD_M == 1){
                        rtn = RF_monitor_execute(1);
                        Printf("Download --> Monitor %04X\r\n");
                        //PutLog(LOG_RF, "Monitor %ld %02X", rtn, rtn);     // debug

                    }


                    if(exec_WMD_M == 1){
                        // モニタのデータ送信
                        if( *(uint16_t *)&rf_ram.auto_format1.data_byte[0] ){       // データが0以外(有る)場合送信
                            if(Err != 0){
                                tx_thread_sleep(WAIT_5SEC);     /*Delay(5000);*/    // 連続してサーバーにアクセスすると、拒否される
                            }
                            Err = SendMonitorFile(0);
                            //PutLog(LOG_RF, "Monitor %ld \r\n", Err);      // debug
                            if(Err == 0){

                                CurrRead_ReTry = 0;                             // GSM通信正常終了なのでリトライしない
                            }
                            else{
                                if(CurrRead_ReTry < 2){
                                    CurrRead_ReTry++;       // CurrRead_ReTry < [リトライ回数] かチェック
                                }
                                else{
                                    CurrRead_ReTry = 0;     // リトライ終了
                                }
                            }
                        }
                    }
                    exec_WMD_W = 0;     // テスト用自律フラグ
                    exec_WMD_M = 0;     // テスト用自律フラグ
                }

                //if(actual_events & FLG_EVENT_KEIHO){
                //  ;
                //}
/*
                if(E_OK == pol_flg( ID_FLG_EVENT, EF_EVENT_KEIHO | EF_EVENT_MONITOR | EF_EVENT_BATT, TWF_ORW, &evFlag )){   // イベントフラグ取得 2009/12/04 親機のみの警報チェックに対応



                }
*/

//  ↑↑↑ここまで モニタリング、警報監視のイベント確認&実行 ----------
            }
            else{
                break;          // rtn = AUTO_END なら抜ける
            }
            MaxDownload--;
        }       // while(MaxDownload){




        if(check_download_ng() == ERROR_END){
            f_rtn = ERR(RF , NOTFINISH);
        }
//      memcpy(&download_buff.do_rpt[0] , &retry_buff.download_buff[group_cnt].do_rpt[0] , 16);     // リトライする時の中継機情報を保存
//      memcpy(download_buff.ng_unit , &retry_buff.download_buff[group_cnt].do_unit[0] , 16);   // リトライする時の子機情報を保存

        memcpy(&retry_buff.download_buff[group_cnt].do_rpt[0], &download_buff.do_rpt[0], 16);       // リトライする時の中継機情報を保存
        memcpy(&retry_buff.download_buff[group_cnt].do_unit[0], download_buff.ng_unit, 16);     // リトライする時の子機情報を保存



    }       // for(group_cnt = 0 ; group_cnt < group_size ; group_cnt++){       // グループ数ループ


BREAK_END:					// // USB差されたなどで自律動作を速やかに停止させる	2020/08/17 segi

    return (f_rtn);
}



/**
 *
 * @return
 */
static uint8_t check_download_ng(void)
{
    int cnt;
    char *poi;
    uint8_t rtn = NORMAL_END;;

    poi = download_buff.ng_unit;
    for(cnt = 0 ; cnt < 16 ; cnt++ , poi++){
        if(*poi != 0){
            rtn = ERROR_END;
            break;
        }
    }
    return (rtn);
}
//********** end of check_download_ng() **********



/**
 * @brief   自動データ吸上げのリトライで吸上げる子機(do_unitフラグ)を設定する
 *          RF_buff.rf_req.current_group にグループ番号が入っている。
 * @param group_cnt
 */
static void set_download_retry_do_flag(uint8_t group_cnt)
{
//  uint8_t cnt;
//  uint8_t group_no;
//  char *in_poi;
//  char *out_poi;
    uint8_t G_No;

    G_No = group_cnt;

    get_regist_group_adr((uint8_t)(G_No+1));
//  get_rpt_table(G_No);            // 中継テーブル作成

//  memcpy(download_buff.do_rpt , &retry_buff.download_buff[G_No].do_rpt[0] ,16);       // 中継機
//  memcpy(download_buff.do_unit, &retry_buff.download_buff[G_No].do_unit[0], 16);  // 子機情報
//  memset(&download_buff.over_rpt[0], 0x00, 32);       // 自動吸い上げOKフラグをクリア
//  memset(&download_buff.ng_rpt[0], 0x00, 32);             // 自動吸い上げNGフラグをクリア
    memcpy(download_buff.do_rpt , &retry_buff.download_buff[G_No].do_rpt[0] ,32);       // 中継機/ 子機情報
    memset(&download_buff.over_rpt[0], 0x00, 64);       // 自動吸い上げOKフラグ, 自動吸い上げNGフラグをクリア
    group_registry.max_unit_no = (char)(group_registry.remote_unit * 2);    // xxx11 2010.08.11


}
//***** end of set_download_retry_do_flag() *****



/**
 * @brief   自動データ吸い上げした時、RTR-574やRTR-576の吸い上げデータか調べて、[照度・紫外線]と[温度・湿度]を連結するか処理を決める
 *
 *       無線通信が成功でも失敗でもコールする
 *       子機がRTR-574以外の場合は何もしない
 *       auto_download(0x61)の返り値が AUTO_END 以外の場合に実行する
 * @retval  NORMAL_END: ネットワークにデータ送信準備が整いました
 * @retval  ERROR_END   : ネットワークにデータ送信してはならない
 *
 */
static uint8_t download574(void)
{
    uint8_t rtn;

    rtn = NORMAL_END;;

    if(RF_buff.rf_res.status == RFM_NORMAL_END){    // 無線通信正常終了したらデータをGSM渡し変数にコピー
    //  無線通信が成功した時の処理
            // 吸い上げデータがRTR-574やRTR-576か見抜く
            switch(NIKOICHI_select_sram()){
                case 0:
                    //##### 今回の吸い上げデータは２個いちの前半データ #####
                    if(check_flag(download_buff.do_unit , (uint8_t)(rf_ram.auto_download.unit_no + 1)) != 0){   // 後半データも吸い上げ対象か見抜く
                        // #### 後半も吸上げる設定になっている
                        set_flag(download_buff.over_unit, rf_ram.auto_download.unit_no);                // 前半吸い上げったフラグを立てる
                        Chk574No = rf_ram.auto_download.unit_no;        // 現子機番号を保存する
                        // 前半データ(照度・紫外線/ＣＯ２)を[sram2]にコピーする
                        memcpy(rf_ram2.adr, rf_ram.adr,32388);                                  // sram2 ← sram 吸い上げデータをコピー
                        tx_thread_sleep (70);       // 2010.08.03 リトライ前のWaitをDelayに変更   xxx03
                        rtn = ERROR_END;            // ２個いちデータの前半が吸上げ終了なのでまだネットワークに送出しない
                    }
                    else{
                        // #### 後半は吸上げない設定になっている
                        Chk574No = 0;       // 吸い上げ対象は前半データのみなので次関数でネットワークデータ送信する
                        DualData = 0;
                    }
                    break;

                case 1:
                    //##### 今回の吸い上げデータは２個いちの後半データ #####
                    DualData = (Chk574No == 0) ? 0 : 1;// 後半データのみの場合/ 前半と後半の両方

                    Chk574No = 0;
                    break;          // NORMAL_ENDとなる

                case 2:
                    //##### 今回の吸い上げデータは２個いちではない #####
                    Chk574No = 0;
                    DualData = 0;
                    break;          // NORMAL_ENDとなる
            }
    }
    else{
    //      無線通信が失敗した時の処理
    //      RTR-574以外は何もしない
    //      RTR-574の場合、[照度・紫外線][温度・湿度]とも通信エラーフラグをセットする
            get_regist_relay_inf(RF_buff.rf_req.current_group,rf_ram.auto_download.unit_no, rpt_sequence);      // 対象子機の登録情報読み出す(RTR-574/576は子機番号の小さい方へ読み替えた返り値)

            if((ru_registry.rtr501.header == 0xFA)||(ru_registry.rtr501.header == 0xF9)){               // 登録情報が RTR-574 or RTR-576 の場合
                Chk574No = 0;
                DualData = 0;
                if(ru_registry.rtr574.set_flag & 0x04){     // [照度・紫外線]or[CO2](前半)が吸い上げ対象の場合
                    clr_flag(download_buff.over_unit , ru_registry.rtr574.number);          // [照度・紫外線]か[温度・紫外線]どちらか通信エラーなら両方エラーとする
                    set_flag(download_buff.ng_unit , ru_registry.rtr574.number);            // [照度・紫外線]か[温度・紫外線]どちらか通信エラーなら両方エラーとする
                }
                if(ru_registry.rtr574.set_flag & 0x08){     // [温度・湿度](後半)が吸い上げ対象の場合
                    clr_flag(download_buff.over_unit , (uint8_t)(ru_registry.rtr574.number+1));     // [照度・紫外線]か[温度・紫外線]どちらか通信エラーなら両方エラーとする
                    set_flag(download_buff.ng_unit , (uint8_t)(ru_registry.rtr574.number+1));           // [照度・紫外線]か[温度・紫外線]どちらか通信エラーなら両方エラーとする
                }
                //rtn = ERROR_END;
            }
    }
    return (rtn);
}
//***** end of download574() *****




/**
 * @brief   自動吸い上げしたRTR-574のダウンロードデータを1つにまとめた構造体にセットする
 * この関数を呼ぶ前には、必ず auto_download(0x61) が実行されていて、その中で get_unit_no_info() が実行されている
  * @return
 */
uint8_t  download574_2in1(void)
{
    uint8_t rtn;

    union{
        uint8_t byte[4];
        uint16_t word[2];
        uint32_t chg_long;
    }b_w_end_no;            // 最終データ番号

    union{
        uint8_t byte[4];
        uint16_t word[2];
        uint32_t chg_long;
    }b_w_data_byte;         // 記録データバイト数

    union{
        uint8_t byte[4];
        uint16_t word[2];
        uint32_t chg_long;
    }b_w_SD;                // 先頭データ番号

//  uint16_t SSD12SD34;
//  uint16_t SED12SD34;
//  uint16_t TIME_INC;
    int SSD12SD34;
    int SED12SD34;
    uint32_t TIME_INC;


    uint16_t ch_cnt[2];

    ch_cnt[0] = 0;      // チャンネル数を保存する変数追加  xxx07 2011.03.08
    ch_cnt[1] = 0;      // チャンネル数を保存する変数追加  xxx07 2011.03.08

    rtn = 0;

    if(DualData != 0){      // ２個いちデータ
        //#################### ２個いちデータを吸い上げた場合 ###########################################
        //#################### RTR-574で、[照度・紫外線][温度・湿度]を吸い上げた場合 ####################
        //#################### RTR-576で、[ＣＯ２][温度・湿度]を吸い上げた場合 ##########################
        Download.Attach.group_no = rf_ram2.auto_download.group_no;                      // グループ番号(0～)
        Download.Attach.unit_no = rf_ram2.auto_download.unit_no;                            // 子機番号(1～)
        Download.Attach.kisyu_code = rf_ram2.header574.kisyu_code;                      // 機種コード(RTR-574は0x57)
        memcpy(&Download.Attach.intt , &rf_ram2.header574.intt[0] , 2);                 // 記録間隔[秒]
        memcpy(&Download.Attach.start_time[0] , &rf_ram2.header574.start_time[0] , 8);  // (子機に書かれている)記録開始日時
        memcpy(&Download.Attach.serial_no , &rf_ram2.header574.serial_no[0] , 4);           // 子機シリアル番号
        Download.Attach.zoku[0] = rf_ram2.header574.ch_zoku[0];                         // ch1記録属性[照度]
        Download.Attach.zoku[1] = rf_ram2.header574.ch_zoku[1];                         // ch2記録属性[紫外線]
        if(Download.Attach.zoku[0] != 0){
            ch_cnt[0] = 1;                                          // 記録属性によりチャンネル数算出  xxx07 2011.03.07
        }
        if(Download.Attach.zoku[1] != 0){
            ch_cnt[0]++;                                            // 記録属性によりチャンネル数算出  xxx07 2011.03.07
        }

        Download.Attach.zoku[2] = rf_ram.header574.ch_zoku[0];                          // ch3記録属性[温度]
        Download.Attach.zoku[3] = rf_ram.header574.ch_zoku[1];                          // ch4記録属性[湿度]

        if(Download.Attach.zoku[2] != 0){
            ch_cnt[1] = 1;                                          // 記録属性によりチャンネル数算出  xxx07 2011.03.07
        }
        if(Download.Attach.zoku[3] != 0){
            ch_cnt[1]++;                                            // 記録属性によりチャンネル数算出  xxx07 2011.03.07
        }
        memcpy(&Download.Attach.group_id[0] , &rf_ram2.header574.group_id[0] , 8);      // グループID
        memcpy(&Download.Attach.unit_name[0] , &rf_ram2.header574.unit_name[0] , 8);        // 子機名
        memcpy(&Download.Attach.multiply[0] , &rf_ram2.header574.ch1_mp[0] , 2);            // ch1積算値[積算照度]
        memcpy(&Download.Attach.multiply[1] , &rf_ram2.header574.ch2_mp[0] , 2);            // ch2積算値[積算紫外線]
        Download.Attach.multiply[2] = 0;                                                // ch3積算値
        Download.Attach.multiply[3] = 0;                                                // ch4積算値
        memcpy(&Download.Attach.wait_time , &rf_ram2.header574.wait_time , 4);          // 最終記録からの経過秒数(常に初回に吸い上げる[照度・紫外線](前半)のを採用する)    // コピー数を2byte→4byteに修正 2014.10.21
        memcpy(&Download.Attach.end_no , &rf_ram2.header574.end_no[0] , 2);             // 最終データ番号(常に初回に吸い上げる[照度・紫外線](前半)のを採用する)

        memcpy(&Download.Attach.lower_limit[0] , &rf_ram2.header574.ch1_lower[0] , 2);  // ch1下限値
        memcpy(&Download.Attach.lower_limit[1] , &rf_ram2.header574.ch2_lower[0] , 2);  // ch2下限値
        memcpy(&Download.Attach.lower_limit[2] , &rf_ram.header574.ch1_lower[0] , 2);       // ch3下限値
        memcpy(&Download.Attach.lower_limit[3] , &rf_ram.header574.ch2_lower[0] , 2);       // ch4下限値

        memcpy(&Download.Attach.upper_limit[0] , &rf_ram2.header574.ch1_upper[0] , 2);  // ch1上限値
        memcpy(&Download.Attach.upper_limit[1] , &rf_ram2.header574.ch2_upper[0] , 2);  // ch2上限値
        memcpy(&Download.Attach.upper_limit[2] , &rf_ram.header574.ch1_upper[0] , 2);       // ch3上限値
        memcpy(&Download.Attach.upper_limit[3] , &rf_ram.header574.ch2_upper[0] , 2);       // ch4上限値

        memcpy(&Download.Attach.mp_limit[0] , &rf_ram2.header574.ch1_mp_upper[0] , 2);  // ch1積算上限値
        memcpy(&Download.Attach.mp_limit[1] , &rf_ram2.header574.ch2_mp_upper[0] , 2);  // ch2積算上限値
        memcpy(&Download.Attach.mp_limit[2] , &rf_ram.header574.ch1_mp_upper[0] , 2);       // ch3積算上限値
        memcpy(&Download.Attach.mp_limit[3] , &rf_ram.header574.ch2_mp_upper[0] , 2);       // ch4積算上限値

        memcpy(&Download.Attach.ud_flags , &ru_registry.rtr574.limiter_flag , 1);               // 上下限値の有効/無効フラグ
        Download.Attach.mp_flags = ru_registry.rtr574.sekisan_flag;                             // 積算上限値の有効/無効フラグ


        b_w_end_no.byte[0] = rf_ram2.header574.end_no[0];           // [照度・紫外線](前半)の最終データ番号
        b_w_end_no.byte[1] = rf_ram2.header574.end_no[1];           // [照度・紫外線](前半)の最終データ番号
        b_w_data_byte.byte[0] = rf_ram2.header574.data_byte[0]; // [照度・紫外線](前半)の記録データバイト
        b_w_data_byte.byte[1] = rf_ram2.header574.data_byte[1]; // [照度・紫外線](前半)の記録データバイト
//      if(b_w_data_byte.word[0] != 0)b_w_data_byte.word[0] = b_w_data_byte.word[0] / 4;    // データバイト数をデータ個数に変換する
        if(b_w_data_byte.word[0] != 0){
            b_w_data_byte.word[0] = (uint16_t)(b_w_data_byte.word[0] / (ch_cnt[0] * 2));    // データバイト数をデータ個数に変換する       xxx07 2011.03.07
        }
        b_w_SD.word[0] = (uint16_t)Data_diff(b_w_end_no.word[0] , (uint16_t)(b_w_data_byte.word[0] - 1));       // (前半の)先頭データ番号を求める

        b_w_end_no.byte[2] = rf_ram.header574.end_no[0];            // [温度・湿度](後半)の最終データ番号
        b_w_end_no.byte[3] = rf_ram.header574.end_no[1];            // [温度・湿度](後半)の最終データ番号
        b_w_data_byte.byte[2] = rf_ram.header574.data_byte[0];  // [温度・湿度](後半)の記録データバイト
        b_w_data_byte.byte[3] = rf_ram.header574.data_byte[1];  // [温度・湿度](後半)の記録データバイト
//      if(b_w_data_byte.word[1] != 0)b_w_data_byte.word[1] = b_w_data_byte.word[1] / 4;    // データバイト数をデータ個数に変換する
        if(b_w_data_byte.word[1] != 0){
            b_w_data_byte.word[1] = (uint16_t)(b_w_data_byte.word[1] / (ch_cnt[1] * 2));    // データバイト数をデータ個数に変換する       xxx07 2011.03.07
        }
        b_w_SD.word[1] = (uint16_t)Data_diff(b_w_end_no.word[1] , (uint16_t)(b_w_data_byte.word[1] - 1));       // (後半の)先頭データ番号を求める

        Download.Attach.sett_flags = 0;         // 予めクリア 2014 06 05 haya スケール変換式のずれの問題修正



        if(b_w_SD.word[0] == b_w_SD.word[1]){
        //########## [照度・紫外線]と[温度・湿度]データの先頭データ番号も、データ個数も一致している場合 ##########
            memcpy(&Download.Attach.GMT , &rf_ram2.auto_download.start_time[0] , 4);        // 子機との無線通信開始時刻(GMT)前半の吸い上げ時刻を採用する

            Download.Attach.data_poi[0] = (char *)&rf_ram2.header574.data[0];                       // [照度・紫外線](前半)のデータ本体の先頭アドレス
            Download.Attach.data_poi[1] = (char *)&rf_ram.header574.data[0];                        // [温度・湿度](後半)のデータ本体の先頭アドレス

            Download.Attach.header_size[0] = &rf_ram2.header574.data[0] - &rf_ram2.header574.intt[0];       // ヘッダサイズ
            Download.Attach.header_size[1] = &rf_ram.header574.data[0] - &rf_ram.header574.intt[0];     // ヘッダサイズ


//          memcpy(&Download.Attach.data_byte[0],&rf_ram2.header.data_byte[0],2);           // 1ch or/and 2ch データの合計バイト数(先に吸い上げる[照度・紫外線]に合わせる)
//          memcpy(&Download.Attach.data_byte[1],&rf_ram2.header.data_byte[0],2);           // 3ch or/and 4ch データの合計バイト数(先に吸い上げる[照度・紫外線]に合わせる)

            b_w_data_byte.word[0] = (uint16_t)((b_w_data_byte.word[0] * (ch_cnt[0] * 2)));          // 前半データの記録データ数 × 前半チャンネル数 × ２
            Download.Attach.data_byte[0] = b_w_data_byte.word[0];

            b_w_data_byte.word[1] = (uint16_t)((b_w_data_byte.word[0] * (ch_cnt[1] * 2)));          // 前半データの記録データ数 × 後半チャンネル数 × ２
            Download.Attach.data_byte[1] = b_w_data_byte.word[1];

        }
        else{
        //########## [照度・紫外線]と[温度・湿度]データの先頭データ番号や、データ個数が不一致の場合 ##########
            memcpy(&Download.Attach.GMT , &rf_ram.auto_download.start_time[0] , 4);     // 子機との無線通信開始時刻(GMT)後半の吸い上げ時刻を採用する

            SSD12SD34 = Data_diff(b_w_SD.word[1] , b_w_SD.word[0]);     // [温度・湿度]と[照度・紫外線]の先頭データ番号の差を求める
            SED12SD34 = Data_diff(b_w_end_no.word[0] , b_w_SD.word[1]); // [照度・紫外線]の最終データ番号と[温度・湿度]の先頭データ番号の差を求める

        //  Download.Attach.data_poi[0] = &rf_ram2.header574.data[0] + (SSD12SD34 * 4); // [照度・紫外線]のデータ本体の先頭アドレス
            /*
            if(Download.Attach.zoku[1] != 0){
                Download.Attach.data_poi[0] = (char *)&rf_ram2.header574.data[0] + (SSD12SD34 * 4); // [照度・紫外線]のデータ本体の先頭アドレス
            }
            else{
                Download.Attach.data_poi[0] = (char *)&rf_ram2.header574.data[0] + (SSD12SD34 * 2); // [CO2]のデータ本体の先頭アドレス
            }
            */
            Download.Attach.data_poi[0] = (Download.Attach.zoku[1] != 0) ? (char *)&rf_ram2.header574.data[0] + (SSD12SD34 * 4) : (char *)&rf_ram2.header574.data[0] + (SSD12SD34 * 2); // [照度・紫外線]のデータ本体の先頭アドレス/ [CO2]のデータ本体の先頭アドレス


            Download.Attach.data_poi[1] = (char *)&rf_ram.header574.data[0];                        // [温度・湿度]のデータ本体の先頭アドレス

            Download.Attach.header_size[0] = &rf_ram2.header574.data[0] - &rf_ram2.header574.intt[0];       // ヘッダサイズ
            Download.Attach.header_size[1] = &rf_ram.header574.data[0] - &rf_ram.header574.intt[0];     // ヘッダサイズ


            TIME_INC = (uint32_t)(SSD12SD34 * Download.Attach.intt);                                // 前半吸い上げの時間(秒数)算出
            Download.Attach.wait_time = (uint32_t)(Download.Attach.wait_time + TIME_INC);           // 最終記録からの経過秒数 に前半吸い上げの時間を加算

//          Download.Attach.data_byte[0] = (SED12SD34 + 1) * 4;                         // 1ch or/and 2ch データの合計バイト数
//          Download.Attach.data_byte[1] = (SED12SD34 + 1) * 4;                         // 3ch or/and 4ch データの合計バイト数

            Download.Attach.data_byte[0] = (uint16_t)((SED12SD34 + 1) * (ch_cnt[0] * 2));           // 1ch or/and 2ch データの合計バイト数
            Download.Attach.data_byte[1] = (uint16_t)((SED12SD34 + 1) * (ch_cnt[1] * 2));           // 3ch or/and 4ch データの合計バイト数


        }

    }
    else{
        //#################### ２個いちデータ以外を吸い上げた場合 #######################################
        Download.Attach.group_no = rf_ram.auto_download.group_no;                           // グループ番号(0～)
        Download.Attach.unit_no = rf_ram.auto_download.unit_no;                         // 子機番号(1～)
        memcpy(&Download.Attach.GMT,&rf_ram.auto_download.start_time[0],4);             // 子機との無線通信開始時刻(GMT)

        Download.Attach.kisyu_code = rf_ram.header.kisyu_code;                          // 機種コード(RTR-574は0x57) 64byteと128byteでアドレス共通

//------------------------------------------------------------ RTR505B-Lx騒音振動の場合は、登録ファイルのみmodelが0xA6となる
		if(ru_registry.rtr505.model == 0xa6){
			Download.Attach.kisyu_code = ru_registry.rtr505.model;
		}
//------------------------------------------------------------                                                                                        // (RTR-505の場合はセンサユニットにより変化する)
        memcpy(&Download.Attach.intt,&rf_ram.header.intt[0],2);                         // 記録間隔[秒]
        memcpy(&Download.Attach.start_time[0],&rf_ram.header.start_time[0],8);          // (子機に書かれている)記録開始日時
        memcpy(&Download.Attach.serial_no,rf_ram.header.serial_no,4);                   // 子機シリアル番号

        memcpy(&Download.Attach.group_id[0],&rf_ram.header.group_id[0],8);              // グループID
        memcpy(&Download.Attach.unit_name[0],&rf_ram.header.unit_name[0],8);                // 子機名

        memcpy(&Download.Attach.wait_time,&rf_ram.header.wait_time,4);                  // 最終記録からの経過秒数      // コピー数を2byte→4byteに修正 2014.10.21
        memcpy(&Download.Attach.end_no,&rf_ram.header.end_no[0],2);                     // 最終データ番号

        b_w_SD.byte[0] = (0xf0 & rf_ram.header.ul_protect);                             // 記録ヘッダー(64byte)からの追加数(64byte単位)を求める
        b_w_SD.byte[0] = b_w_SD.byte[0] >> 4;                                           //                  "
        b_w_SD.byte[1] = 0;                                                             //                  "
        b_w_SD.word[0] = (uint16_t)(b_w_SD.word[0] * 64);                                           //                  "
        Download.Attach.data_poi[0] = (char *)((uint8_t *)&rf_ram.header.data[0] + b_w_SD.word[0]);         // ロギングデータ本体の先頭アドレス
        Download.Attach.data_poi[1] = Download.Attach.data_poi[0];                      //

        Download.Attach.header_size[0] = (uint8_t)( ((uint8_t *)&rf_ram.header.data - (uint8_t *)&rf_ram.header.intt[0]) + b_w_SD.word[0]); // ヘッダサイズ
        Download.Attach.header_size[1] = 0;                                                                 // ヘッダサイズ


        Download.Attach.total_pulse[0] = 0;     // 予めクリア
        Download.Attach.total_pulse[1] = 0;     // 予めクリア
        Download.Attach.rec_sett = 0;           // 予めクリア
        Download.Attach.sett_flags = 0;         // 予めクリア


        //#################### RTR-501,502,503の場合か、RTR-574で[照度・紫外線]か[温度・湿度]の片方を吸い上げた場合 ##########
        if(ru_registry.rtr501.header == 0xFE){                                  // 情報種類を判断(RTR-501,502,503)
                memcpy(&Download.Attach.data_byte[0],&rf_ram.header.data_byte[0],2);                // 1ch or 2ch データの合計バイト数
                Download.Attach.data_byte[1] = 0;                                               // 1ch or 2ch データの合計バイト数

                memcpy(&Download.Attach.ud_flags , &ru_registry.rtr501.limiter_flag, 1);                    // 上下限値の有効/無効フラグ
                Download.Attach.mp_flags = 0;                                                   // 積算上限値の有効/無効フラグ

                Download.Attach.zoku[0] = rf_ram.header.ch_zoku[0];                             // ch1記録属性[温度]
                Download.Attach.zoku[1] = rf_ram.header.ch_zoku[1];                             // ch2記録属性[湿度]
                Download.Attach.zoku[2] = 0;                                                    // ch3記録属性
                Download.Attach.zoku[3] = 0;                                                    // ch4記録属性

                Download.Attach.multiply[0] = 0;                                                // ch1積算値
                Download.Attach.multiply[1] = 0;                                                // ch2積算値
                Download.Attach.multiply[2] = 0;                                                // ch3積算値
                Download.Attach.multiply[3] = 0;                                                // ch4積算値

                memcpy(&Download.Attach.lower_limit[0] , &rf_ram.header.ch1_lower[0] , 2);      // ch1下限値
                memcpy(&Download.Attach.lower_limit[1] , &rf_ram.header.ch2_lower[0] , 2);      // ch2下限値
                Download.Attach.lower_limit[2] = 0;                                             // ch3下限値
                Download.Attach.lower_limit[3] = 0;                                             // ch4下限値

                memcpy(&Download.Attach.upper_limit[0] , &rf_ram.header.ch1_upper[0] , 2);      // ch1上限値
                memcpy(&Download.Attach.upper_limit[1] , &rf_ram.header.ch2_upper[0] , 2);      // ch2上限値
                Download.Attach.upper_limit[2] = 0;                                             // ch3上限値
                Download.Attach.upper_limit[3] = 0;                                             // ch4上限値

                Download.Attach.mp_limit[0] = 0;                                                // ch1積算上限値
                Download.Attach.mp_limit[1] = 0;                                                // ch2積算上限値
                Download.Attach.mp_limit[2] = 0;                                                // ch3積算上限値
                Download.Attach.mp_limit[3] = 0;                                                // ch4積算上限値
        }

        //#################### RTR-505を吸い上げた場合 ##########
        else if(ru_registry.rtr501.header == 0xF8){                                     // 情報種類を判断
                memcpy(&Download.Attach.data_byte[0],&rf_ram.header.data_byte[0],2);                // 1ch or 2ch データの合計バイト数
                Download.Attach.data_byte[1] = 0;                                               // 1ch or 2ch データの合計バイト数

                memcpy(&Download.Attach.ud_flags , &ru_registry.rtr505.limiter_flag, 1);                    // 上下限値の有効/無効フラグ
                Download.Attach.mp_flags = 0;                                                   // 積算上限値の有効/無効フラグ

                Download.Attach.zoku[0] = rf_ram.header.ch_zoku[0];                             // ch1記録属性[温度,電圧,パルスなど]
                Download.Attach.zoku[1] = rf_ram.header.ch_zoku[1];                             // ch2記録属性[湿度]
                Download.Attach.zoku[2] = 0;                                                    // ch3記録属性
                Download.Attach.zoku[3] = 0;                                                    // ch4記録属性

//-----↓↓↓↓↓ RTR-505で追加した部分↓↓↓↓↓↓↓↓↓----------------------
                Download.Attach.multiply[0] = 0;                                                // ch1積算値

                if(rf_ram.header.kisyu_code == 0xa5){                                               // RTR-505パルス記録か判断する
                    //memcpy(&Download.Attach.multiply[0],&rf_ram.header.ch2_lower[0],4);           // ch1積算パルス(ch2下限値,上限値のアドレスに入る)
                    memcpy(&Download.Attach.total_pulse[0],&rf_ram.header.ch2_lower[0],4);      // ch1積算パルス(ch2下限値,上限値のアドレスに入る)
                    Download.Attach.zoku[1] = Download.Attach.zoku[0];                          // RTR-505パルス記録の場合は、ch2にもパルス属性を設定
                }

//-----↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑----------------------

                Download.Attach.multiply[1] = 0;                                                // ch2積算値
                Download.Attach.multiply[2] = 0;                                                // ch3積算値
                Download.Attach.multiply[3] = 0;                                                // ch4積算値

                memcpy(&Download.Attach.lower_limit[0] , &rf_ram.header.ch1_lower[0] , 2);      // ch1下限値
//-----↓↓↓↓↓ RTR-505で追加した部分↓↓↓↓↓↓↓↓↓----------------------
                if((rf_ram.header.kisyu_code != 0xa4)&&(rf_ram.header.kisyu_code != 0xa5)){ // RTR-505電圧・パルス記録以外か判断する
                    memcpy(&Download.Attach.lower_limit[1] , &rf_ram.header.ch2_lower[0] , 2);  // ch2下限値
                }
                else{
                    Download.Attach.lower_limit[1] = 0;                                         // ch2下限値
                }
//-----↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑----------------------
                Download.Attach.lower_limit[2] = 0;                                             // ch3下限値
                Download.Attach.lower_limit[3] = 0;                                             // ch4下限値


                memcpy(&Download.Attach.upper_limit[0] , &rf_ram.header.ch1_upper[0] , 2);      // ch1上限値
//-----↓↓↓↓↓ RTR-505で追加した部分↓↓↓↓↓↓↓↓↓----------------------
                if((rf_ram.header.kisyu_code != 0xa4)&&(rf_ram.header.kisyu_code != 0xa5)){ // RTR-505電圧・パルス記録以外か判断する
                    memcpy(&Download.Attach.upper_limit[1] , &rf_ram.header.ch2_upper[0] , 2);  // ch2上限値
                }
                else{
                    Download.Attach.upper_limit[1] = 0;                                         // ch2上限値
                }
//-----↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑----------------------
                Download.Attach.upper_limit[2] = 0;                                             // ch3上限値
                Download.Attach.upper_limit[3] = 0;                                             // ch4上限値

                Download.Attach.mp_limit[0] = 0;                                                // ch1積算上限値
                Download.Attach.mp_limit[1] = 0;                                                // ch2積算上限値
                Download.Attach.mp_limit[2] = 0;                                                // ch3積算上限値
                Download.Attach.mp_limit[3] = 0;                                                // ch4積算上限値

//-----↓↓↓↓↓ RTR-505で追加した部分↓↓↓↓↓↓↓↓↓----------------------
                Download.Attach.rec_sett = rf_ram.header.aki2;                                  //  測定・記録(瞬時/即時)形式
                //Download.Attach.sett_flags = ru_registry.rtr505.scale_flag;                   //  bit0:スケール・単位変換(しない/する)=(0/1)
                Download.Attach.sett_flags = (ru_registry.rtr505.set_flag >> 5) & 0x01 ;        //  bit0:スケール・単位変換(しない/する)=(0/1)
                if((Download.Attach.sett_flags & 0x01) == 0x01){                                //  スケール変換する時だけ登録ファイルから変換データをコピーする
                    memcpy(&Download.Attach.scale_conv[0] , &ru_registry.rtr505.scale_setting[0], 64);//    スケール・単位変換データ
                }
//-----↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑----------------------
        }

        //#################### RTR-601を吸い上げた場合 ##########
        else if(ru_registry.rtr501.header == 0xF7){                                     // 情報種類を判断  RTR-601追加
                memcpy(&Download.Attach.data_byte[0],&rf_ram.header.data_byte[0],2);                // 1ch or 2ch データの合計バイト数
                Download.Attach.data_byte[1] = 0;                                               // 1ch or 2ch データの合計バイト数

        }

        else{
                //#################### RTR-574[照度・紫外線]を吸い上げた場合 ##########

                memcpy(&Download.Attach.ud_flags , &ru_registry.rtr574.limiter_flag, 1);        // 上下限値の有効/無効フラグ
                Download.Attach.mp_flags = ru_registry.rtr574.sekisan_flag;                             // 積算上限値の有効/無効フラグ

                if(NIKOICHI_select() == 0){     // ２個いち子機の場合、前半データか後半データかを判断する

                    //#################### RTR-574[照度・紫外線]を吸い上げた場合 ##########
                    //#################### RTR-576[ＣＯ２]を吸い上げた場合 ##########
                        memcpy(&Download.Attach.data_byte[0],&rf_ram.header.data_byte[0],2);                // 1ch or 2ch データの合計バイト数
                        Download.Attach.data_byte[1] = 0;                                               // 3ch or 4ch データの合計バイト数

                        Download.Attach.zoku[0] = rf_ram.header574.ch_zoku[0];                          // ch1記録属性[照度]
                        Download.Attach.zoku[1] = rf_ram.header574.ch_zoku[1];                          // ch2記録属性[紫外線]
                        Download.Attach.zoku[2] = 0;                                                    // ch3記録属性[温度]
                        Download.Attach.zoku[3] = 0;                                                    // ch4記録属性[湿度]

                        memcpy(&Download.Attach.multiply[0] , &rf_ram.header574.ch1_mp[0] , 2);         // ch1積算値[積算照度]
                        memcpy(&Download.Attach.multiply[1] , &rf_ram.header574.ch2_mp[0] , 2);         // ch2積算値[積算紫外線]
                        Download.Attach.multiply[2] = 0;                                                // ch3積算値
                        Download.Attach.multiply[3] = 0;                                                // ch4積算値

                        memcpy(&Download.Attach.lower_limit[0] , &rf_ram.header574.ch1_lower[0] , 2);       // ch1下限値
                        memcpy(&Download.Attach.lower_limit[1] , &rf_ram.header574.ch2_lower[0] , 2);       // ch2下限値
                        Download.Attach.lower_limit[2] = 0;                                             // ch3下限値
                        Download.Attach.lower_limit[3] = 0;                                             // ch4下限値

                        memcpy(&Download.Attach.upper_limit[0] , &rf_ram.header574.ch1_upper[0] , 2);       // ch1上限値
                        memcpy(&Download.Attach.upper_limit[1] , &rf_ram.header574.ch2_upper[0] , 2);       // ch2上限値
                        Download.Attach.upper_limit[2] = 0;                                             // ch3上限値
                        Download.Attach.upper_limit[3] = 0;                                             // ch4上限値

                        memcpy(&Download.Attach.mp_limit[0] , &rf_ram.header574.ch1_mp_upper[0] , 2);       // ch1積算上限値
                        memcpy(&Download.Attach.mp_limit[1] , &rf_ram.header574.ch2_mp_upper[0] , 2);       // ch2積算上限値
                        Download.Attach.mp_limit[2] = 0;                                                // ch3積算上限値
                        Download.Attach.mp_limit[3] = 0;                                                // ch4積算上限値
                }
                else{
                    //#################### RTR-574,576[温度・湿度]を吸い上げた場合 ##########
                        Download.Attach.unit_no = (uint8_t)(Download.Attach.unit_no - 1);                           // (ヘッダは後半の子機番号なので)1引く

                        Download.Attach.data_byte[0] = 0;                                               // 1ch or 2ch データの合計バイト数
                        memcpy(&Download.Attach.data_byte[1],&rf_ram.header.data_byte[0],2);                // 3ch or 4ch データの合計バイト数

                        Download.Attach.zoku[0] = 0;                                                    // ch1記録属性[温度]
                        Download.Attach.zoku[1] = 0;                                                    // ch2記録属性[湿度]
                        Download.Attach.zoku[2] = rf_ram.header574.ch_zoku[0];                          // ch3記録属性[照度]
                        Download.Attach.zoku[3] = rf_ram.header574.ch_zoku[1];                          // ch4記録属性[紫外線]

                        Download.Attach.multiply[0] = 0;                                                // ch1積算値
                        Download.Attach.multiply[1] = 0;                                                // ch2積算値
                        memcpy(&Download.Attach.multiply[2] , &rf_ram.header574.ch1_mp[0] , 2);         // ch3積算値[積算温度](現状は未使用)
                        memcpy(&Download.Attach.multiply[3] , &rf_ram.header574.ch2_mp[0] , 2);         // ch4積算値[積算湿度](現状は未使用)

                        Download.Attach.lower_limit[0] = 0;                                             // ch1下限値
                        Download.Attach.lower_limit[1] = 0;                                             // ch2下限値
                        memcpy(&Download.Attach.lower_limit[2] , &rf_ram.header574.ch1_lower[0] , 2);       // ch3下限値
                        memcpy(&Download.Attach.lower_limit[3] , &rf_ram.header574.ch2_lower[0] , 2);       // ch4下限値

                        Download.Attach.upper_limit[0] = 0;                                             // ch1上限値
                        Download.Attach.upper_limit[1] = 0;                                             // ch2上限値
                        memcpy(&Download.Attach.upper_limit[2] , &rf_ram.header574.ch1_upper[0] , 2);       // ch3上限値
                        memcpy(&Download.Attach.upper_limit[3] , &rf_ram.header574.ch2_upper[0] , 2);       // ch4上限値

                        Download.Attach.mp_limit[0] = 0;                                                // ch1積算上限値
                        Download.Attach.mp_limit[1] = 0;                                                // ch2積算上限値
                        memcpy(&Download.Attach.mp_limit[2] , &rf_ram.header574.ch1_mp_upper[0] , 2);       // ch3積算上限値(現状は未使用)
                        memcpy(&Download.Attach.mp_limit[3] , &rf_ram.header574.ch2_mp_upper[0] , 2);       // ch4積算上限値(現状は未使用)
                }



        }

    }
    return (rtn);
}

/**
 * @brief   ２個いちデータ機種(RTR-574,RTR-576など)の片方データを吸い上げた場合、前半か後半のどちらのデータかを判断する
 *
 * 引き数： regist.unit.kind            = 登録ファイルの情報種類
 *               rf_ram.header574.ch_zoku[0] = Ch1 or Ch3 の記録属性
 *               rf_ram.header574.ch_zoku[1] = Ch2 or Ch4 の記録属性
 * @return  0:前半データ 1:後半データ
 */
static uint8_t NIKOICHI_select(void)
{
    uint8_t rtn;

    switch(ru_registry.rtr501.header){
        case 0xFA:      //### RTR-574
            rtn = ((rf_ram.header574.ch_zoku[0] == 0x49)&&(rf_ram.header574.ch_zoku[1] == 0x55)) ? 0 : 1;    // 前半/ 後半
            break;

        case 0xF9:      //### RTR-576
            rtn = ((rf_ram.header574.ch_zoku[0] == 0x42)&&(rf_ram.header574.ch_zoku[1] == 0x00)) ? 0 : 1;  // 前半/ 後半
            break;

        case 0xFE:      //### RTR-501,502,503
        case 0xF8:      //### RTR-505
        case 0xF7:      //### RTR-601       // RTR-601追加
        default:
            rtn = 0;    // ２個いちでない機種は、前半として処理する(後半を待たずにネットワークにデータ送出)
            break;
    }

    return (rtn);
}
//***** end of NIKOICHI_select() *****



/**
 * @brief   ２個いちデータ機種(RTR-574,RTR-576など)の片方データを吸い上げた場合、前半か後半のどちらのデータかを判断する
 *
 * 引き数：    rf_ram.header.kisyu_code        = 1バイト機種コード
 *               rf_ram.header574.ch_zoku[0] = Ch1 or Ch3 の記録属性
 *               rf_ram.header574.ch_zoku[1] = Ch2 or Ch4 の記録属性
 * @return  0:２個いち前半データ 1:２個いち後半データ 2:２個いちデータではない
 * @note    ※２個いち=RTR-574 or RTR-576
 */
static uint8_t NIKOICHI_select_sram(void)
{
    uint8_t rtn;

    switch(rf_ram.header.kisyu_code){
        case 0x57:      //##### RTR-574
            rtn = ((rf_ram.header574.ch_zoku[0] == 0x49)&&(rf_ram.header574.ch_zoku[1] == 0x55)) ? 0 : 1;   // 記録属性が前半か判断する
            break;

        case 0x56:      //##### RTR-576
            rtn = ((rf_ram.header574.ch_zoku[0] == 0x42)&&(rf_ram.header574.ch_zoku[1] == 0x00)) ? 0 : 1;   // 記録属性が前半か判断する
            break;

        default:        //##### ２こいちでない機種
            rtn = 2;        // ２こいちでない機種は、前半として処理する
            break;
    }

    return (rtn);
}
//***** end of NIKOICHI_select_sram() *****




/**
 * @brief   2個のカウンタの差を求める
 * カウンタは0～0xffffをエンドレスでグルグル回る
 * @param A_dat 引かれる数
 * @param B_dat 引く数
 * @return  A_dat - B_dat の結果
 */
static uint16_t Data_diff(uint16_t A_dat , uint16_t B_dat)
{
    uint16_t rtn;

    rtn = (A_dat < B_dat) ? (uint16_t)((0 - (int)B_dat) + A_dat) : (uint16_t)(A_dat - B_dat);

    return (rtn);
}
//***** end of uint Data_diff(uint A_dat , uint B_dat) *****



/**
 * @brief   自律動作のデータ吸上げを行う。
 *
 * この関数を実行する前の手順
 * - uint get_group_size(void)でグループ数を把握。この数×子機台数分、この関数を実行する。
 * - void set_do_flag(rf_download) 通信すべき子機、中継機をflagに設定する。
 * - uchar auto_download(void)本関数を実行する。
 * @param PARA  0x60=個数指定 0x61=番号指定 0x62=時間指定
 * @return
 */
static uint8_t auto_download_New(uint8_t PARA)
{
//  uint8_t cnt;        //2019.Dec.26 コメントアウト
    uint8_t unit_no;
//  rtc_time_t tm;
/*  union{
        uint8_t byte[2];
        uint16_t word;
    }b_w_chg;
    */
/*
    union{
        uint8_t byte[4];
        uint32_t l_word;
    }b_lw_chg;
*/
	uint64_t GMT_cal;
	uint64_t LAST_cal;

    uint32_t rtn;
    RouteArray_t route;


    Printf("    auto_download_new Start (%02X)\r\n", PARA);

    unit_no = check_do_unit_flag((uint8_t *)&download_buff.do_unit[0]);         // 未通信の子機が有る場合は子機番号が返る(登録順番ではない)

/* dbg
    download_buff.do_unit[0] = 0b11111110;          // ←←←←←←←←←←←←[関数に置き換える]
    download_buff.do_unit[1] = 0b11111111;          // ←←←←←←←←←←←←[関数に置き換える]
    download_buff.do_unit[2] = 0b11111111;          // ←←←←←←←←←←←←[関数に置き換える]
    download_buff.do_unit[3] = 0b11111111;          // ←←←←←←←←←←←←[関数に置き換える]
    download_buff.do_unit[4] = 0b11111111;          // ←←←←←←←←←←←←[関数に置き換える]
    download_buff.do_unit[5] = 0b11111111;          // ←←←←←←←←←←←←[関数に置き換える]
    download_buff.do_unit[6] = 0b11111111;          // ←←←←←←←←←←←←[関数に置き換える]
    download_buff.do_unit[7] = 0b11111111;          // ←←←←←←←←←←←←[関数に置き換える]
    download_buff.do_unit[8] = 0b11111111;          // ←←←←←←←←←←←←[関数に置き換える]
    download_buff.do_unit[9] = 0b11111111;          // ←←←←←←←←←←←←[関数に置き換える]
    download_buff.do_unit[10] = 0b00111111;         // ←←←←←←←←←←←←[関数に置き換える]
    download_buff.do_unit[11] = 0b00000000;         // ←←←←←←←←←←←←[関数に置き換える]
    download_buff.do_unit[12] = 0b00000000;         // ←←←←←←←←←←←←[関数に置き換える]
    download_buff.do_unit[13] = 0b00000000;         // ←←←←←←←←←←←←[関数に置き換える]
    download_buff.do_unit[14] = 0b00000000;         // ←←←←←←←←←←←←[関数に置き換える]
    download_buff.do_unit[15] = 0b00000000;         // ←←←←←←←←←←←←[関数に置き換える]
    while(unit_no != 0){
        set_flag(download_buff.over_unit,unit_no);
        unit_no = check_do_unit_flag(&download_buff.do_unit[0]);            // 未通信の子機が有る場合は子機番号が返る(登録順番ではない)
    }
dbg */

    if(unit_no != NORMAL_END){
// 2023.05.26 ↓
        /*
         * Note: 普通PARAは0x61になっています。
         * RTR500BWの構造では「debug」のとき　(dbg_cmd == 6)はPARA=0x62になっています
         */
        uint8_t DataRange;
//        char suction_Range = 0;
        eFDF_t first_flag;

//        suction_Range = my_config.suction.Range[0]; //DRNG
//        Printf("\r\n #### SucRange %d\r\n", suction_Range);
//
//        // First add entries if not yet defined. i.e. has been cleared for some reason
//        first_flag = first_download_flag_read(RF_buff.rf_req.current_group, unit_no);
//        if(first_flag == 0xFF) {
//            if(suction_Range != 0) {
//                first_download_flag_write(RF_buff.rf_req.current_group, unit_no, FDF_ZENBU_1);
//            } else {
//                first_download_flag_write(RF_buff.rf_req.current_group, unit_no, FDF_SABUN_0);
//            }
//        }

        first_flag = first_download_flag_read(RF_buff.rf_req.current_group, unit_no);
        if(first_flag == FDF_SABUN_0) {
            RF_buff.rf_req.cmd1 = PARA;     // 0x61->差分吸上げコマンド [PARAは通常0x61][debug->0x62->全吸上げコマンド]]
            PARA = RF_buff.rf_req.cmd1;
            //RF_buff.rf_req.cmd1 = 0x61;     // 0x61->差分吸上げコマンド
            DataRange = 0;
        } else if(first_flag == FDF_ZENBU_1) {
            RF_buff.rf_req.cmd1 = 0x62;     // 0x62->全吸上げコマンド
            PARA = RF_buff.rf_req.cmd1;
            DataRange = 1;
        }


        //----- Noisy -----
        Printf("\r\n");
        Printf("first_flag = %x, cmdIn = 0x%02x, cmdActual = 0x%02x\r\n", first_flag, PARA, RF_buff.rf_req.cmd1);
        //----- Noisy -----
//        RF_buff.rf_req.cmd1 = PARA;                     // データ吸上げコマンド
// 2023.05.26 ↑

        RF_buff.rf_req.cmd2 = 0x00;

        RF_buff.rf_req.data_size = 2;                       // 追加データは中継情報のみ

//      get_unit_no_info(RF_buff.rf_req.current_group,unit_no);             // 子機番号をキーワードに登録情報を読み出す         // ←←←←←←←←←←←←[関数に置き換える]


//      RF_buff.rf_req.rpt_cnt = get_rpt_table_New(RF_buff.rf_req.current_group,regist.unit.net_no);                // 2012.06.27 中継機テーブルを作成し、中継機数を返す


//      RTC.GetGMT(&tm);
//      RF_buff.rf_req.time = RTC.Date2GSec(&tm);               // コマンドを発行する直前のGMTを保存する

//      RF_buff.rf_req.data_poi = &rf_ram.adr[0];

//      get_unit_no_info(RF_buff.rf_req.current_group,unit_no);             // 子機番号をキーワードに登録情報を読み出す

//      RF_buff.rf_req.rpt_cnt = get_rpt_cnt(regist.unit.net_no);       // 中継情報バイト数(子機の親中継機番号から中継機台数を取得)
//      RF_buff.rf_req.rpt_poi = &rf_ram.rpt_buff.adr[0];

        if((root_registry.auto_route & 0x01) == 1){     // 自動ルート機能ONの場合

	        get_regist_relay_inf(RF_buff.rf_req.current_group, unit_no, huge_buffer);		// 子機登録を読み出す(中継機テーブルは読み捨てる)   // sakaguchi 2021.10.05 下から移動

            //  電波強度から子機のルート情報を取得
            memset( &route, 0x00, sizeof(RouteArray_t) );
            //group_registry.altogether = (char)RF_GetRemoteRoute(RF_buff.rf_req.current_group, unit_no, &route);
            group_registry.altogether = (char)RF_GetRemoteRoute(RF_buff.rf_req.current_group, ru_registry.rtr501.number, &route);           // sakaguchi 2021.10.05

            memset( rpt_sequence, 0x00, sizeof(rpt_sequence) );
            memcpy( rpt_sequence, route.value, route.count );

	        //group_registry.altogether = get_regist_relay_inf(RF_buff.rf_req.current_group, unit_no, huge_buffer);		// 子機登録を読み出す(中継機テーブルは読み捨てる)	2020/08/21 segi // sakaguchi 2021.10.05 del
																														// chk_download()で子機シリアルが必要な為			2020/08/21 segi

        }else{
            group_registry.altogether = get_regist_relay_inf(RF_buff.rf_req.current_group, unit_no, rpt_sequence);      // 中継機テーブルを取得する関数
        }
//-----↓↓↓↓↓----- RTR-601で追加した部分 -----↓↓↓↓↓-----                                // RTR-601追加
        if(ru_registry.rtr501.header == 0xF7){                                                          // RTR-601追加
            RF_buff.rf_req.cmd1 = 0x2A;                     // RTR-601用データ吸い上げ(番号指定)コマンド
            PARA = RF_buff.rf_req.cmd1;                     // データ吸上げコマンド
            memcpy(&Download.Attach.extension[0] , &ru_registry.rtr501.remain[20] , 3);         // RTR-601追加 ファイル拡張子
            Download.Attach.extension[3] = 0;                                                   // RTR-601追加
            memcpy(&Download.Attach.time_diff, &ru_registry.rtr501.remain[11] , 4);             // RTR-601追加 UTCとの時差
        }                                                                                       // RTR-601追加
        else{                                                                                   // RTR-601追加
            ///@attention バグ？ @bug バグ？ memcpy? memset? @todo バグ？
//          memcpy(&Download.Attach.extension[0] , 0 , 4);                                      // RTR-601追加 RTR-601以外はクリア とりあえずmemset()に変更 2020.01.23
            memset(Download.Attach.extension , 0 , 4);                                      // RTR-601追加 RTR-601以外はクリア とりあえずmemset()に変更 2020.01.23
        }                                                                                       // RTR-601追加
//-----↑↑↑↑↑----- RTR-601で追加した部分 -----↑↑↑↑↑-----                                // RTR-601追加


//        if(dbg_cmd != 6){
        if((dbg_cmd != 6) && (DataRange == 0/*OFF*/)) {     // 2023.05.26
//          b_w_chg.word = (uint16_t)chk_download(RF_buff.rf_req.current_group , unit_no , (uint8_t *)&ru_registry.rtr501.serial);  // 前回のデータ番号を参照する
//          complement_info[0] = b_w_chg.byte[0];           // データ番号を指定する(前回取得データを元に)
//          complement_info[1] = b_w_chg.byte[1];           // データ番号を指定する(前回取得データを元に)
            *(uint16_t *)&complement_info[0] = (uint16_t)chk_download(RF_buff.rf_req.current_group , unit_no , (uint8_t *)&ru_registry.rtr501.serial);   // 前回のデータ番号を参照する

			//----- 子機の設定値が初期化された場合の最初のデータ吸い上げは全データとする 2020/02/08 segi -----
			if((chk_download_null == 1)&&(*(uint16_t *)&complement_info[0] == 0)&&(RF_buff.rf_req.cmd1 == 0x61)){
				chk_download_null = 0;
				RF_buff.rf_req.cmd1 = 0x62;
                PARA = RF_buff.rf_req.cmd1;                     // データ吸上げコマンド  2021.04.01
 #ifdef DBG_TERM
                Printf("check down load null set");             // 2021.03.30 test debug
#endif                 
			}
			//------------------------------------------------------------------------------------------------

        }
        else{       // デバッグ用に全データ吸上げする設定
//          complement_info[0] = 0;                         // データ番号を指定する(前回取得データを元に)
//          complement_info[1] = 0;                         // データ番号を指定する(前回取得データを元に)
            *(uint16_t *)&complement_info[0] = 0;       // データ番号を指定する(前回取得データを元に)
        }


    //  get_regist_group_adr(RF_buff.rf_req.current_group);
        //グループの登録情報取得                       // ←←←←←←←←←←←←[関数に置き換える]
        /*group_registry.id[0] = 'R';
        group_registry.id[1] = 'T';
        group_registry.id[2] = 'R';
        group_registry.id[3] = '5';
        group_registry.id[4] = '0';
        group_registry.id[5] = '0';
        group_registry.id[6] = '_';
        group_registry.id[7] = 't';
        */
        my_rpt_number = 0;
        //group_registry.band = 0;
        //group_registry.frequency = 3;
        Printf("    .group_registry.frequency %d \r\n", group_registry.frequency);
        ru_registry.rtr501.number = unit_no;



//      RF_buff.rf_req.node = unit_no;
        rf_ram.auto_download.group_no = RF_buff.rf_req.current_group;   // 対象のグループ番号

        rf_ram.auto_download.unit_no = unit_no;                     // 対象の子機番号

        RF_buff.rf_res.time = 0;
        timer.int125 = 0;                                   // コマンド開始から子機が応答したときまでの秒数をカウント開始
        DL_start_utc = RTC_GetGMTSec();                         // コマンドを発行する直前のGMTを保存する

        Printf("auto_download_New -->  RF_command_execute(%02X)\r\n", PARA);
        rtn = RF_command_execute(PARA);
        RF_buff.rf_res.status = (char)rtn;

// ↓↓↓↓↓------------------------------------------------------------------------------------

//		RF_buff.rf_res.time = RF_buff.rf_res.time + DL_start_utc;

// 時間計算をLASTを参照するように修正 2020.07.07
		GMT_cal = RTC_GetGMTSec();					// 無線通信終了後のGMT取得
		GMT_cal = (GMT_cal * 1000) + 500;			// 500msec足して誤差を四捨五入	2020/08/28 segi
		LAST_cal = timer.int125 * 125uL;
		GMT_cal = GMT_cal - LAST_cal;

		if((GMT_cal % 1000) < 500){
			GMT_cal = GMT_cal / 1000;
		}
		else{
			GMT_cal = GMT_cal / 1000;
			GMT_cal++;
		}
		RF_buff.rf_res.time = (uint32_t)GMT_cal;	// コマンドを発行する直前のGMTを保存する
// ↑↑↑↑↑↑-----------------------------------------------------------------------------------

        memcpy(rf_ram.auto_download.start_time, (uint8_t *)&RF_buff.rf_res.time, 4);// 子機とRF通信開始したGMTを保存
        memset(&rf_ram.auto_download.start_time[4], 0x00, 4);

        //memcpy(&b_lw_chg.l_word,&Download.Attach.GMT,4);


//      Printf( "\r\n[rtn:0x%x][r500.cmd1:0x%x][r500.cmd2:0x%x][r500.data_size:%d][r500.para2:0x%x][r500.rf_cmd2:0x%x]",rtn,r500.cmd1,r500.cmd2,r500.data_size,r500.para2,r500.rf_cmd2);


    }
    else{
        unit_no = AUTO_END;     // 自動吸上げ対象子機が終了した
//      Printf("\r\n One Group Sequence End GroupNo=%d",RF_buff.rf_req.current_group);
//      Printf("\r\n");
    }

    Printf("    auto_download_new End  (unit_no %d)\r\n", unit_no);
    return (unit_no);
}



/**
 * @brief   自律動作のリアルスキャンを行う。
 *
 * この関数を実行する前の手順
 * - uint get_group_size(void)でグループ数を把握。この数×中継機の回数分、この関数を実行する。
 * - void set_do_flag(rf_realscan) 通信すべき子機、中継機をflagに設定する。
 * - uchar auto_realscan(void)本関数を実行する。
 * @param DATA_FORMAT   通常は0x02を指定する。
 * @return
 */
uint8_t auto_realscan_New(uint8_t DATA_FORMAT)
{

    uint8_t scn_o_n = 1;    // 0:0x68  1:0x30



//  uint8_t cnt;        //2019.Dec.26 コメントアウト
    uint8_t rtn;
//  uint32_t l_rtn;     //2019.Dec.26 コメントアウト
	uint64_t GMT_cal;
	uint64_t LAST_cal;
/*
    union{
        uint8_t byte[4];
        uint32_t l_word;
    }b_lw_chg;
*/

    RouteArray_t route;                             // 中継ルート
    uint8_t unit_no;                                // スキャン対象の子機番号   // sakaguchi 2021.10.05

    if((root_registry.auto_route & 0x01) == 0){     // 自動ルート機能OFFか
        scn_o_n = 0;
    }



    rtn = check_do_flag();              // 未通信の中継機が有る場合は中継機番号
    if(rtn != NOT_UNIT){


        if(scn_o_n == 0){
            RF_buff.rf_req.cmd1 = 0x68;                             // リアルスキャンコマンド(子機だけ)
        }
        else{
            RF_buff.rf_req.cmd1 = 0x30;                             // リアルスキャンコマンド(子機と中継機)
//          r500.route.rpt_max = 2;                     // お試し
            //complement_info[2] = group_registry.altogether;
            complement_info[2] = RF_Group[RF_buff.rf_req.current_group-1].max_rpt_no;   // 最大中継機番号をセット   // sakaguchi 2021.10.05
        }



        RF_buff.rf_req.cmd2 = 0x00;

        RF_buff.rf_req.data_size = 2;                           // 追加データは中継情報のみ

        //complement_info[0] = (char)(group_registry.max_unit_no + 1);    // 1個のグループに含まれる最大子機番号
        complement_info[0] = (char)(group_registry.max_unit_no);    // 1個のグループに含まれる最大子機番号          // sakaguchi 2021.02.03
        complement_info[1] = DATA_FORMAT;                       // データ番号を指定する(前回取得データを元に)

        RF_buff.rf_req.data_format = DATA_FORMAT;           // 2019.09.07追加 500WではR500_command_make()でやっていた

        get_regist_group_adr(RF_buff.rf_req.current_group);     // グループの登録情報取得

        if((root_registry.auto_route & 0x01) == 1){     // 自動ルート機能ONの場合

// sakaguchi 2021.10.05 ↓
//            //  電波強度から中継機のルート情報を取得
//            memset( &route, 0x00, sizeof(RouteArray_t) );
//            group_registry.altogether = (char)RF_GetRepeaterRoute(RF_buff.rf_req.current_group, rtn, &route);

            // スキャン対象子機を取得する
            unit_no = check_do_unit_flag((uint8_t *)&realscan_buff.do_unit[0]);

            // RTR574,576の子機番号補正が必要なため、子機の登録情報を読み出す
            get_regist_relay_inf(RF_buff.rf_req.current_group, unit_no, rpt_sequence);

            memset( &route, 0x00, sizeof(RouteArray_t) );
            memset( rpt_sequence, 0x00, sizeof(rpt_sequence) );

            // 登録情報から読み出した子機番号でルート情報を取得する
            group_registry.altogether = (char)RF_GetRemoteRoute(RF_buff.rf_req.current_group, ru_registry.rtr501.number, &route);
            memcpy( rpt_sequence, route.value, route.count );
// sakaguchi 2021.10.05 ↑

        }else{
            group_registry.altogether = get_regist_relay_inf_relay(RF_buff.rf_req.current_group, rtn, rpt_sequence);        // 中継機テーブルを取得する関数
        }

        End_Rpt_No = rtn;                                           // リアルスキャンの末端中継機Noを記憶

        if(group_registry.altogether > 1){
            set_flag(&realscan_buff.over_rpt[0], rpt_sequence[group_registry.altogether - 1]);  // 中継機があれば末端中継機と通信済とする
        }
        else{
            set_flag(&realscan_buff.over_rpt[0],0);                                             // 中継機なければ親機と通信済とする
        }

        RF_buff.rf_res.time = 0;
        timer.int125 = 0;                                   // コマンド開始から子機が応答したときまでの秒数をカウント開始
        DL_start_utc = RTC_GetGMTSec();                         // コマンドを発行する直前のGMTを保存する

        Printf("auto_realscan_New -->  RF_command_execute(%02X)\r\n", RF_buff.rf_req.cmd1);
        rtn = RF_command_execute(RF_buff.rf_req.cmd1);              // 無線通信をする。
        RF_buff.rf_res.status = rtn;

        RF_buff.rf_res.time = RF_buff.rf_res.time + DL_start_utc;
//      b_lw_chg.l_word = RF_buff.rf_res.time;                  // 子機とRF通信開始したGMTを保存

//      memcpy(&b_lw_chg.l_word,&Download.Attach.GMT,4);
        Printf("    DL_start_utc  %ld \r\n", DL_start_utc) ;
        Printf("    Attach.GMT    %ld \r\n", Download.Attach.GMT) ;
        //memcpy(&RF_buff.rf_res.time, &Download.Attach.GMT,4);        // 子機とRF通信開始したGMTを保存
//		memcpy(&RF_buff.rf_res.time, &DL_start_utc,4);        // 子機とRF通信開始したGMTを保存

// ↓↓↓↓↓------------------------------------------------------------------------------------
// 時間計算をLASTを参照するように修正 2020.07.07
		GMT_cal = RTC_GetGMTSec();					// 無線通信終了後のGMT取得
		GMT_cal = (GMT_cal * 1000) + 500;			// 500msec足して誤差を四捨五入	2020/08/28 segi
		LAST_cal = timer.int125 * 125uL;
		GMT_cal = GMT_cal - LAST_cal;

		if((GMT_cal % 1000) < 500){
			GMT_cal = GMT_cal / 1000;
		}
		else{
			GMT_cal = GMT_cal / 1000;
			GMT_cal++;
		}
		RF_buff.rf_res.time = (uint32_t)GMT_cal;	// コマンドを発行する直前のGMTを保存する



		//PutLog(LOG_RF,"GMT_cal=(%ld) LAST_cal=(%ld)", GMT_cal,LAST_cal);



// ↑↑↑↑↑↑-----------------------------------------------------------------------------------


    }
    else{
        rtn = AUTO_END;     // リアルスキャンする対象が終了した。
    }
    return (rtn);
}





/**
 * @brief   リアルスキャンフラグをチェックし、まだ無線通信するべきか判断する
 * @return  NOT_UNIT(0xe2)=もう終了した   それ以外= 無線通信パラメタの中継情報バイト数
 */
static uint8_t check_do_flag(void)
{
    uint8_t *in_poi,*out_poi;
    int cnt;
    uint8_t rpt_chk = NOT_UNIT;
    uint8_t unit_chk = NOT_UNIT;

    uint8_t select_over;
    uint8_t ref_flag;
    uint8_t cmp_flag;
    int cmp_cnt;

    in_poi = (uint8_t *)realscan_buff.do_rpt;
    out_poi = (uint8_t *)&realscan_buff.over_rpt[0];
    for(cnt = 0 ; cnt < 16 ; cnt++,in_poi++,out_poi++){
        if(*in_poi != *out_poi){
            cmp_flag = *in_poi ^ *out_poi;
            ref_flag = 1;
            for(cmp_cnt = 0 ; cmp_cnt < 8 ; cmp_cnt++){
                if((ref_flag & cmp_flag) != 0){
                    rpt_chk = (uint8_t)(cmp_cnt + (cnt * 8));
                    break;
                }
                ref_flag = (uint8_t)(ref_flag << 1);
            }
            break;
        }
    }

    in_poi = (uint8_t *)realscan_buff.do_unit;
    out_poi = (uint8_t *)realscan_buff.over_unit;
    for(cnt = 0 ; cnt < 16 ; cnt++,in_poi++,out_poi++){
        select_over = (*in_poi & *out_poi);     // 今回受信フラグの立っている子機のデータだけを選別
        if(*in_poi != select_over){
            unit_chk = ERROR_END;
            break;
        }
    }

    if(unit_chk == NOT_UNIT){
        rpt_chk = NOT_UNIT;     // 子機が全部通信終了していたら終了とする
    }
    return(rpt_chk);                // 中継情報バイト数(無線通信のパラメータ設定値)
}
//***** end of check_do_flag(void) *****


/**
 * @brief   自動吸上げフラグをチェックし、まだ自動吸上げ無線通信するべきか判断する
 * @param poi
 * @return      NORMAL_END(0x00)=もう終了した      それ以外= 通信するべき子機番号
 */
static uint8_t check_do_unit_flag(uint8_t *poi)
{
    uint8_t *in_poi,*out_poi,*out_ng;
    int cnt;
    uint8_t unit_chk = NORMAL_END;

    uint8_t ref_flag;
    uint8_t cmp_flag;
    int cmp_cnt;

    in_poi = poi;               // do_unit      の位置
    out_poi = in_poi + 32;      // over_unit    の位置
    out_ng = out_poi + 32;      // ng_unit      の位置

    for(cnt = 0 ; cnt < 16 ; cnt++,in_poi++,out_poi++,out_ng++){

        cmp_flag = *out_poi | *out_ng;                  // OR (over_unit,ng_unit どちらかbitがONなら通信済み)

        if(*in_poi != cmp_flag){    // まだ残り子機ある
            cmp_flag = *in_poi ^ cmp_flag;              // XOR (do_unitだけに有るbitを抽出)
            ref_flag = 1;
            for(cmp_cnt = 0 ; cmp_cnt < 8 ; cmp_cnt++){ // 未通信の子機を小さい番号から探す
                if((ref_flag & cmp_flag) != 0){         // AND
                    unit_chk =  (uint8_t)(cmp_cnt + (cnt * 8));
                    break;
                }
                ref_flag = (uint8_t)(ref_flag << 1);
            }
            break;
        }
    }
    return (unit_chk);
}
//***** end of check_do__unit_flag(void) *****


/**
 * 指定ポインタからのフラグをNoに従いSETする
 * @param poi   flagが並んでいる先頭アドレス
 *              do_rpt[0]   over_rpt[0]
 *              do_unit[0]  over_unit[0]
 * @param No    Hiにするflagの位置 0～
 */
void set_flag(char *poi , uint8_t No)
{
    uint8_t shift;
    uint8_t set_data;

    poi = poi + (No / 8);
    shift = No % 8;         // ローテート回数

    No = 1;
    set_data = (uint8_t)(No << shift);

    No = *poi;              // 変化させる前のデータをNoに読み込む
    *poi = No | set_data;
}
//***** end of set_flag() *****



/**
 * 指定ポインタからのフラグをNoに従いCLRする
 * @param poi       flagが並んでいる先頭アドレス
 *                  do_rpt[0]    over_rpt[0]
 *                  do_unit[0]  over_unit[0]
 * @param No        Loにするflagの位置 0～
 */
static void clr_flag(char *poi , uint8_t No)
{
    uint8_t shift;
    uint8_t set_data;

    poi = poi + (No / 8);
    shift = No % 8;         // ローテート回数

    No = 1;
    set_data = (uint8_t)(No << shift);
    set_data = (uint8_t)(~set_data);

    No = *poi;              // 変化させる前のデータをNoに読み込む
    *poi = No & set_data;
}
//***** end of clr_flag() *****



/**
 * 指定子機番号のフラグがON/OFFかを取得する
 * @param poi       flagが並んでいる先頭アドレス
 *                  do_rpt[0]   over_rpt[0]
 *                  do_unit[0]  over_unit[0]
 * @param No        チェックするflagの位置 0～
 * @return
 */
uint8_t check_flag(char *poi , uint8_t No)
{
    uint8_t shift;
    uint8_t set_data;

    poi = poi + (No / 8);
    shift = No % 8;         // ローテート回数

    No = 1;
    set_data = (uint8_t)(No << shift);

    return(*poi & set_data);
}
//***** end of check_flag() *****



/**
 * 自動データ吸上げ時に前回のデータ番号を参照する関数。初回の場合は0(全データ指示)が返る
 * @param G_No  グループ番号。登録ファイルの先頭からの順番。(0～)
 * @param U_No  子機番号。(1～)
 * @param siri
 * @return      指定子機の前回のデータ番号。まだない場合(初回)は0x00を応答する
 */
static uint16_t chk_download(uint8_t G_No , uint8_t U_No , uint8_t *siri)
{
    uint8_t *poi;
    int cnt = 0;
    union{
        uint8_t byte[2];
        uint16_t word;
    }b_w_chg;

//  uint8_t dbg;

	chk_download_null = 0;		// 子機番号が空だった場合、全データ吸い上げのマークをクリア。	2021/02/08 segi

    b_w_chg.word = 0;
    poi = (uint8_t *)&auto_control.download[0].group_no;
    do{
        if(*poi == G_No){                   // グループ番号一致を判断
            poi++;
            if(*poi == U_No){               // 子機番号一致を判断
                        poi++;
                        /* dbg = memcmp(poi,siri,4);*/ //とりあえずコメントアウト 2020.01.22
                        if(*poi == *siri){
                            poi++;
                            siri++;
                            if(*poi == *siri){
                                poi++;
                                siri++;
                                if(*poi == *siri){
                                    poi++;
                                    siri++;
                                    if(*poi == *siri){
                                        poi++;
                                        siri++;
                                        b_w_chg.byte[0] = *poi++;       // 前回データ番号を読み出す
                                        b_w_chg.byte[1] = *poi++;       // 前回データ番号を読み出す
                                        break;
                                    }
                                    else{           // グループ番号,子機番号一致するも、シリアル番号不一致の場合
                                        break;
                                    }
                                }
                                else{           // グループ番号,子機番号一致するも、シリアル番号不一致の場合
                                    break;
                                }
                            }
                            else{           // グループ番号,子機番号一致するも、シリアル番号不一致の場合
                                break;
                            }
                        }
                        else{           // グループ番号,子機番号一致するも、シリアル番号不一致の場合
                            break;
                        }
            }
            else{
                poi += 7;
            }
        }
        else if(*poi == 0xff){          // 先頭から書き込むので、0xff(初期値)が出たら最終と判断
			chk_download_null = 1;		// 子機番号が空だった場合、全データ吸い上げのマークをつける。	2021/02/08 segi
#ifdef DBG_TERM
        PutLog(LOG_RF, "check down load null");     // 2021.03.30 test debug
#endif 
            
            break;
        }
        else{
            poi+= 8;
        }
        cnt++;
    }while(cnt < 128);

    return (b_w_chg.word);
}
//********** end of chk_download() **********



/**
 * 自動データ吸上げ時に今回のデータ番号を書き込む関数
 * @param G_No  グループ番号。登録ファイルの先頭からの順番。(0～)
 * @param U_No  子機番号。(1～)
 * @param siri
 * @param Dat_No    今回のデータ吸上げデータ番号
 * @return      書き込めた場合         MD_OK
 *                 書き込む場所がない場合  MD_ERROR
 */
static uint8_t set_download(uint8_t G_No , uint8_t U_No , uint8_t *siri , uint16_t Dat_No)
{
    uint8_t *poi;
    int cnt = 0;
    union{
        uint8_t byte[2];
        uint16_t word;
    }b_w_chg;
    uint8_t rtn = MD_ERROR;

    b_w_chg.word = Dat_No;
    poi = (uint8_t *)&auto_control.download[0].group_no;
    do{
        if(*poi == G_No){                   // グループ番号一致を判断
            poi++;
            if(*poi == U_No){               // 子機番号一致を判断
                poi++;
                *poi++ = *siri++;                   // シリアル番号を書きこむ
                *poi++ = *siri++;                   // シリアル番号を書きこむ
                *poi++ = *siri++;                   // シリアル番号を書きこむ
                *poi++ = *siri++;                   // シリアル番号を書きこむ
                *poi++ = b_w_chg.byte[0];   // データ番号を書き込む
                *poi++ = b_w_chg.byte[1];   // データ番号を書き込む
                rtn = MD_OK;
                break;
            }
            else{
                poi += 7;
            }
        }
        else if(*poi == 0xff){          // 先頭から書き込むので、0xff(初期値)が出たら最終と判断
            *poi++ = G_No;                  // グループ番号を書き込む
            *poi++ = U_No;                  // 子機番号を書き込む
            *poi++ = *siri++;                       // シリアル番号を書きこむ
            *poi++ = *siri++;                       // シリアル番号を書きこむ
            *poi++ = *siri++;                       // シリアル番号を書きこむ
            *poi++ = *siri++;                       // シリアル番号を書きこむ
            *poi++ = b_w_chg.byte[0];       // データ番号を書き込む
            *poi++ = b_w_chg.byte[1];       // データ番号を書き込む
            rtn = MD_OK;
            break;
        }
        else{
            poi+= 8;
        }
        cnt++;
    }while(cnt < 128);

    return (rtn);
}
//********** end of set_download() **********




#if 0
//未使用

/**
 * call Put Log
 * @param err_no
 * @note    未使用
 */
static void call_PutLog(uint8_t err_no)
{
    switch (err_no){
        case RFM_NORMAL_END:        // 0x00                               // 正常終了
                PutLog(LOG_RF, "RFM_NORMAL_END");   // debug
                break;
        case RFM_LOW_BATT:          // 0xe0                               // 電池電圧が低い
                PutLog(LOG_RF, "RFM_LOW_BATT");     // debug
                break;
        case RFM_SERIAL_TOUT:       // 0xf0                               // シリアル通信のタイムアウト
                PutLog(LOG_RF, "RFM_SERIAL_TOUT");  // debug
                break;
        case RFM_DEADLINE:          // 0xf1                               // コマンド終了の締め切り時間切れ
                PutLog(LOG_RF, "RFM_DEADLINE");     // debug
                break;
        case RFM_CANCEL:            // 0xf2                               // キャンセル
                PutLog(LOG_RF, "RFM_CANCEL");       // debug
                break;
        case RFM_R500_BUSY:         // 0xf3                               // R500無線モジュールがBUSY応答の場合
                PutLog(LOG_RF, "RFM_R500_BUSY");    // debug
                break;
        case RFM_R500_NAK:          // 0xf4                               // R500無線モジュールがNAK応答の場合
                PutLog(LOG_RF, "RFM_R500_NAK"); // debug
                break;
        case RFM_R500_CH_BUSY:      // 0xf5                               // R500無線モジュールがCH_BUSY応答の場合
                PutLog(LOG_RF, "RFM_R500_CH_BUSY"); // debug
                break;
        case RFM_PRTCOL_ERR:        // 0xf8                               // R500無線モジュール通信で不正プロトコルであった
                PutLog(LOG_RF, "RFM_PRTCOL_ERR");   // debug
                break;
        case RFM_SERIAL_ERR:        // 0xf9                               // R500無線モジュールとのシリアル通信エラー
                PutLog(LOG_RF, "RFM_SERIAL_ERR");   // debug
                break;
        case RFM_RU_PROTECT:        // 0xfa                               // 子機にプロテクトかかっていて記録開始できない
                PutLog(LOG_RF, "RFM_RU_PROTECT");   // debug
                break;
        case RFM_RT_SHORTAGE:       // 0xfb                               // 記録開始までの秒数足りない
                PutLog(LOG_RF, "RFM_RT_SHORTAGE");  // debug
                break;
        case RFM_REFUSE:            // 0xfc                               // コマンド処理が拒否された
                PutLog(LOG_RF, "RFM_REFUSE");   // debug
                break;
        case RFM_KOKI_ERROR:        // 0xfd                               // 子機通信のエラー
                PutLog(LOG_RF, "RFM_KOKI_ERROR");   // debug
                break;
        case RFM_RPT_ERROR:         // 0xfe                               // 中継機通信のエラー
                PutLog(LOG_RF, "RFM_RPT_ERROR");    // debug
                break;
        case RFM_SRAM_OVER:         // 0xff                               // ＳＲＡＭオーバー
                PutLog(LOG_RF, "RFM_SRAM_OVER");    // debug
                break;
        case RFM_INUSE:             // 0xf6                               // チャンネル空き無し(国内版で追加したステータス)
                PutLog(LOG_RF, "RFM_INUSE");    // debug
                break;
    }
}

#endif





// ↑↑↑↑↑ segi ------------------------------------------------------------------------------------

