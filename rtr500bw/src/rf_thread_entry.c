/**
 * @file    rf_thread_entry.c
 * @brief     無線実行スレッド
 *
    ・各コマンドの関数の種類はメッセージキューにて引き渡し
    ・コマンドの内容についてはバッファ渡し
    ・実行の状態は、フラグによる

 * @attention  b_w_chgの共有体は4Byteと2Byteのものがありパディングを考慮していない
 * @note    2020.01.30  v6 0128 のソースをマージ済み
 * @note    2020.Jun.17 6/17時点のソースマージ済み
 * @note    2020.Jun.18 関数、変数のグローバルとローカルを整理
 * @note    2020.Jun.22 RFMファーム更新機能実装済み
 * @note	2020.Jul.01 GitHub 630ソース反映済み
 * @note	2020.Jul.27 GitHub 0722ソース反映済み
 * @note    2020.Aug.21 RFMフラッシュ書き換え中の電源断等でも次回起動時にファーム更新を再開するように修正
 *
 * @note    rf_cmd_thread スタック    1024Byte
 *
 * @note    SCI FIFO 16Byte
 * @note    comms5 受信キュー（バッファ） 4 × 512 Byte = 2048Byte
 * @note    rf_buffer[]   受信バッファ1 128 Byte rf_buffer に受信し、RCOM.rxbufにコピーしている → 2020.Jul.06 廃止
 * @note    RCOM.rxbuf    受信バッファ2 1286 Byte
 *
 * @todo    rf_buffer廃止（受信バッファを一つにする）  → 2020.Jul.06 廃止
 * @todo    commsドライバのキューの直接チェックをやめる  → 2020.Jul.06 廃止
 * @todo    auto_thread_entry()配下の処理はauto_thread_entry.c移動する    → 2020.Jul.07 とりあえず分離
*/
#define _RF_THREAD_ENTRY_C_

#include "MyDefine.h"
#include "Error.h"
#include "Globals.h"
#include "General.h"

#include "Log.h"
#include "Monitor.h"
#include "Suction.h"
#include "Rfunc.h"
#include "lzss.h"
#include "Warning.h"

#include "system_thread.h"

//extern TX_THREAD rf_thread;
//#include "rf_thread.h"
#include "rf_thread_entry.h"

//#include "cmd_thread.h"
#include "cmd_thread_entry.h"

#include "led_thread_entry.h"
#include "event_thread_entry.h"
#include "auto_thread_entry.h"

#include "programRadio.h"
#include "dataflash.h"  //2020.Sep.15 t.saito

#define PERMANENT_WAIT UINT32_MAX
#define RF_RETRY    1


const uint8_t   RfDelay_Rpt = 9;                                    ///< [中継機宛て]遅延時間計算1.125sec
const uint8_t   RfDelay_Unit = 18;                                  ///< 18 * 0.125 = 2.250[sec]  [子機宛て]遅延時間計算

const uint8_t   TxLoop_Unit = 64;                                   ///< [子機宛て]   ６４回：２．２５秒
const uint8_t   TxLoop_Rpt = 32;                                    ///< [中継機宛て]  ３２回：１．１２秒


//通信バッファ
static def_com RCOM;                        ///< RF通信用バッファ

static int Rf_Sum_Crc;        //受信フレームがSUMの場合0 CRC16の場合1

static struct def_rssi{
    char rpt;
    char level_to_koki;                                        ///<  子機方向へのコマンドのRSSIレベル
    char level_0x34;                                           ///<  0x34応答時のRSSIレベルを保存する
} relay_rssi;


//外部参照 Cmd_func.h
extern void StatusPrintfB(char *, char *, int);                         // 返送パラメータのセット（バイナリ版）



//プロトタイプ
static void rf_delay(uint32_t wt);

static void rfm_initialize(void);                                      // 無線モジュール 初期化
static void rfm_reset(void);                                           // 無線モジュール リセット
static void rfm_send(char *pData);
static void TX_dummy(void);
//未使用static uint8_t RET_set_command(uint8_t in_no);
# if 0
//未使用 未完成 関数
static void rpt_backup(void);
static void rpt_backup_clr(void);
static uint32_t write_ex_rx(void);
#endif

static uint8_t rfm_recv(uint32_t);                                 // 無線モジュール 受信

static void rfm_power_on_initialize(void);                             // 無線モジュール電源ＯＮ時の初期化
static void relay_initialize(uint8_t);                             // 無線モジュール 中継機エラー時の初期化
static uint8_t RF_rpt_batt_set(uint8_t, uint8_t);                        // 中継機の電池残量（５～０）を無線モジュールに書き込む
static uint8_t RF_rxintt(uint8_t);                                     // 間欠受信間隔の設定（Ｒ５００無線モジュールＥＥＰＲＯＭへ書き込み）
static uint8_t RF_rxintt_stop(uint8_t);                                // 間欠受信停止
static uint8_t RF_group_id_set(uint8_t, uint8_t *, uint8_t, uint8_t, uint8_t); // グループＩＤ設定
//未使用static uint8_t RF_cancel(void);                                      // キャンセル
static uint8_t RF_soh_command(uint8_t);                                // Ｒ５００ ＳＯＨコマンド

static void ALM_bak_all_clr(uint8_t size);

static void R500_command_make(uint8_t);                              // R500制御部とのやり取り構造体に入っているデータをR500に送信可能な形に変換する
static uint8_t R500_radio_communication(void);                       // 子機または中継機と無線通信する（リトライ３回）
static uint8_t R500_radio_communication_once(void);                  // 子機または中継機と無線通信する（リトライ無し）
static uint8_t R500_send_recv(uint16_t, uint8_t *);                      // R500にコマンド送信と応答受信
static uint8_t R500_koki_real_scan_reply(void);                      // R500子機からのリアルスキャン回答待ち
static uint8_t R500_koki_whole_scan_reply(void);
static uint8_t R500_koki_present_data_reply(void);                   // R500子機からの現在値回答待ち
static uint8_t R500_koki_setting_data_reply(void);                   // R500子機からの設定値回答待ち
static uint8_t R500_koki_upload_data_reply(void);                    // R500子機からの吸い上げ回答待ち
static uint8_t R500_koki_record_stop_reply(void);                    // R500子機からの記録停止回答待ち
static uint8_t R500_koki_record_start_reply(uint8_t order);                   // R500子機からの記録開始回答待ち
static uint8_t R500_relay_reply(uint8_t);                              // R500中継機からの回答待ち
static uint8_t R500_relay_rf_scan_reply(uint8_t order);            // R500無線スキャン回答待ち
static uint32_t R500_replace_error_code(uint8_t err);              // R500エラーをＴコマンドエラーに置き換える
static uint8_t R500_relay_error_investigate(uint8_t err);          // R500中継機エラー時エラー調査
static uint8_t rxOukagai_M(void);                                  // 親機が親機へのキャンセルお伺いコマンドを受信した時の処理
static uint8_t tx500_com_loop(uint8_t *);
static int retray_size(int);                             // リトライ回数計算

static void Start1000msec(void);                                   // １０００ｍｓタイマスタート
static void Stop1000msec(void);                                    // １０００ｍｓタイマストップ
# if 0
//未使用 未完成 関数
static void Start125msec(void);                                    // １２５ｍｓタイマスタート
static void Stop125msec(void);                                     // １２５ｍｓタイマストップ
#endif
static uint16_t Conv125msec(uint16_t);                             // １２５ｍｓカウンタを秒に変換して返す
static uint8_t rx500_relay_comm_recv(uint8_t *);                   // 追加データと中継情報を受信する
static void calc_rf_timeout(uint16_t OukagaiSize);                 // タイムアウト時間計算



/**
 * @brief   rf処理の delay 分解能 1ms
 * @param wt
 */
void rf_delay(uint32_t wt)
{
    wait.ms_1.rf_wait = wt;
    do{;}while(wait.ms_1.rf_wait!=0);
}



/**
 *  RF Thread entry function
 *  @bug RF_EVENT_EXECUTEコマンドの返り値はREFUSEで良いのか？
 *  @bug inspection_switchをセットしている個所がない
 *  @note   g_rf_mutexの使い方がおかしい
 *  @bug    regf_battery_voltageをセットしている個所が居ない
 *
 */
void rf_thread_entry(void)
{

    uint8_t rtn;

    //変数初期化
    pRfmUartRx = (comSOH_t *)&RCOM.rxbuf.header;    // RFモジュールファーム更新用UARTフレームへのポインタ(バッファへのポインタをセットする)
    pRfmUartTx = (comSOH_t *)&RCOM.txbuf.header;    // RFモジュールファーム更新用UARTフレームへのポインタ(バッファへのポインタをセットする)

    Printf("RF Thread(Uart5) Start\n");

    //ペリフェラル初期化
    sf_comms_init5();									// RF Comm Initial

    checkProgramRadio();        //RFMのFW更新が途中で終わっていないかチェック、更新モードセット

    if(fRadioUpdateMode == 0) {
        rfm_power_on_initialize();                          // 無線モジュール電源ＯＮ時の初期化 CS OFFになる注意
    }
/*
    if(tx_mutex_get(&g_rf_mutex, TX_WAIT_FOREVER) == TX_SUCCESS){
        rfm_power_on_initialize();                          // 無線モジュール電源ＯＮ時の初期化
        tx_mutex_put(&g_rf_mutex);
    }
*/
    //スレッドループ
    for(;;)
    {
 //       working_flag.rfTask = true;   //セットのみ未使用

        if(fRadioUpdateMode == 1)      //RFM FW更新モード
        {
            //ブランクのデータフラッシュは不定値でアクセスのたびに値が変わるので一致してしまうことがある 2020.Sep.15
            if(FLASH_RESULT_NOT_BLANK == CheckBlank_data_flash(FLASH_UPDATE_EXEC,4)){
            if(0x00000004 == *(uint32_t *)FLASH_UPDATE_EXEC)        //無線モジュールファーム更新実行フラグ
            {
                if(tx_mutex_get(&g_rf_mutex, WAIT_100MSEC) == TX_SUCCESS){

                    tx_thread_sleep (10);       //コマンド応答の処理をしている場合がるのでしばらく待つ

                    rfm_program_FW();                      //無線モジュール ファームウェア更新

                    rfm_power_on_initialize();              // 無線モジュール電源ＯＮ時の初期化 CS OFFになる注意

                    tx_mutex_put(&g_rf_mutex);
                }else{
                    __NOP();	//デバッグ用
                }
            }
            else   //ファーム更新実行フラグが4以外の場合、ファーム更新完了する前に別種のファーム更新で上書きされた状態
            {
                    if(FLASH_RESULT_NOT_BLANK == CheckBlank_data_flash(DATA_FLASH_ADR_RADIO_WRITE_MODE,4)){
                if(0x00000001 != *(uint32_t *)DATA_FLASH_ADR_RADIO_WRITE_MODE)    //無線モジュールファーム書き込み中フラグチェック
                {
                    fRadioUpdateMode = 0;                    //書き込み途中ではないので更新実行フラグを消す
                }
            }
                }
            }
            continue;
        }
/* inspection_switchはセットしている個所がない
        if(inspection_switch != 0x00){
            continue;                 // 試験モードなら戻る  ←　ここで戻ると無線系の検査でrf_threadが動作しない
        }
*/
        //        Printf("###rf_command 1 %02X\r\n", rf_command);
//        Printf("###rf_command 2 %02X\r\n", rf_command);
//		Printf("    ==== g_rf_mutex get 3 !!\r\n");

        if(tx_mutex_get(&g_rf_mutex, WAIT_100MSEC) == TX_SUCCESS)
        {

            Printf("###rf_command 3 %02X\r\n", rf_command);

//            in_rf_process = true;                               // ＲＦタスク実行中を知るため    ←フラグ管理不要 OSの機能を使うべき

            timer.int125 = 0;                                   // コマンド開始から子機が応答したときまでの秒数をカウント開始
            timer.latch_enable = false;

            RF_buff.rf_res.time = 0;
            RF_buff.rf_req.cancel = 0;                          // キャンセル要求をクリアする

            RF_CS_ACTIVE();                                     // ＣＳ Ｌｏｗ（無線モジュール アクティブ）
            rf_delay(20);

            ERRORCODE.ACT0 = ERR(RF, NOERROR);

            rtn = RFM_SERIAL_ERR;

            if(rf_command == RF_COMM_GET_MODULE_VERSION)        // 無線モジュールバージョン取得
            {
                if(RF_soh_command(0x72) != RFM_NORMAL_END){
                    memset((char *)RCOM.rxbuf.data, 0x00, 2);
                }
            }
            else if(rf_command == RF_COMM_GET_MODULE_SERIAL)    // 無線モジュールシリアル番号取得
            {
                if(RF_soh_command(0xb3) != RFM_NORMAL_END){
                    memset((char *)RCOM.rxbuf.data, 0x00, 4);
                }
            }
            else
            {
                Printf("###rf_command 4 %02X\r\n", rf_command);

                if((rtn = RF_rxintt_stop(IT_PERMIT)) != RFM_NORMAL_END)    // 間欠受信を許可
                {
                    rf_delay(10);
                    ERRORCODE.ACT0 = ERR(RF, OTHER_ERR);
                }
                else
                {

                    // 通信対象のプロパティ クリア
                    sram.record = 0;
                    property.acquire_number = 0;
                    property.transferred_number = 0;

                    Printf("###rf_command 5 %02X\r\n", rf_command);

                    switch(rf_command)
                    {
                        case RF_COMM_SETTING_WRITE_FORMAT0:     // 無線 子機 設定値書き込み（記録開始情報）
                            rtn = RF_command_execute(0x65);
                            break;

                        case RF_COMM_SETTING_WRITE_FORMAT1:     // 無線 子機 設定値書き込み（警報情報）
                            rtn = RF_command_execute(0x6a);
                            break;

                        case RF_COMM_SETTING_READ:              // 無線 子機 設定値読み込み
                            rtn = RF_command_execute(0x64);
                            Printf("###rf_command 6 %02X\r\n", rf_command);
                            break;

                        case RF_COMM_CURRENT_READINGS:          // 無線 子機 現在値読み込み
                            rtn = RF_command_execute(0x6c);
                            break;

                        case RF_COMM_GATHER_DATA:               // 無線 子機 データ吸い上げ
                            rtn = RF_command_execute((uint8_t)(0x60 + huge_buffer[0]));
                            break;

                        case RF_COMM_SEARCH:                    // 無線 子機検索
                            rtn = RF_command_execute(0x2d);
                            break;

                        case RF_COMM_REPEATER_SEARCH:           // 無線 中継機検索
                            rtn = RF_command_execute(0x40);
                            break;

                        case RF_COMM_REAL_SCAN:                 // 無線 子機 リアルスキャン
                            rtn = RF_command_execute(0x68);
                            break;

						case RF_COMM_WHOLE_SCAN:				// 無線 子機 子機と中継機の全検索
							rtn = RF_command_execute(0x30);
							break;

                        case RF_COMM_SETTING_WRITE_GROUP:       // 無線 グループ一斉記録開始
                            rtn = RF_command_execute(0x66);
                            break;

                        case RF_COMM_RECORD_STOP:               // 無線 子機 記録停止
                            rtn = RF_command_execute(0x27);
                            break;

                        case RF_COMM_PROTECTION:                // 無線 子機 プロテクト設定
                            rtn = RF_command_execute(0x6e);
                            break;

                        case RF_COMM_MONITOR_INTERVAL:          // 無線 子機 モニタリング間隔設定
                            rtn = RF_command_execute(0x6b);
                            break;

//----- BLE設定_WRITE_READ 2019.09.15 segi------------------------------------
                        case RF_COMM_BLE_SETTING_WRITE:			// BLE設定_Write
	//	Writeパラメータ
	//	ENABLE
	//	NAME
	//	SECURITY
	//	PASC
                            rtn = RF_command_execute(0x25);
                            break;
                        case RF_COMM_BLE_SETTING_READ:			// BLE設定_Read
	//=== Writeパラメータ
	//	ENABLE
	//	NAME
	//	SECURITY
	//	PASC(不要)
                            rtn = RF_command_execute(0x2b);
                            break;
//----- スケール変換式_WRITE_READ 2019.09.15 segi-----------------------------
                        case RF_COMM_MEMO_WRITE:				// スケール変換式_Write
                    		rtn = RF_command_execute(0x23);
                            break;
                        case RF_COMM_MEMO_READ:					// スケール変換式_Read
                    		rtn = RF_command_execute(0x22);
                            break;
//----------------------------------------------------------------------------

                        case RF_COMM_REGISTRATION:              // 無線 子機 登録変更
                            rtn = RF_command_execute(0x63);
                            break;
                        case RF_COMM_PS_SETTING_READ:           // ＰＵＳＨ 子機の設定値取得
                            rtn = RF_command_execute(0x22);
                            break;

                        case RF_COMM_PS_SETTING_WRITE:          // ＰＵＳＨ 子機の設定値書き込み
                            rtn = RF_command_execute(0x1c);
                            break;

                        case RF_COMM_PS_GATHER_DATA:            // ＰＵＳＨ データ吸い上げ
                            rtn = RF_command_execute((huge_buffer[0] == 2) ? 0x24 : 0x2a);  // ＰＵＳＨのデータ吸い上げは期間指定のみ、時間指定するとフォーマットエラーになる
                            break;

                        case RF_COMM_PS_DATA_ERASE:             // ＰＵＳＨ データ消去
                            rtn = RF_command_execute(0x1a);
                            break;

                        case RF_COMM_PS_ITEM_LIST_READ:         // ＰＵＳＨ 品目リスト読み込み
                            rtn = RF_command_execute(0x22);
                            break;

                        case RF_COMM_PS_ITEM_LIST_WRITE:        // ＰＵＳＨ 品目リスト書き込み
                            rtn = RF_command_execute(0x23);
                            break;

                        case RF_COMM_PS_WORKER_LIST_READ:       // ＰＵＳＨ 作業者リスト読み込み
                            rtn = RF_command_execute(0x22);
                            break;

                        case RF_COMM_PS_WORKER_LIST_WRITE:      // ＰＵＳＨ 作業者リスト書き込み
                            rtn = RF_command_execute(0x23);
                            break;

                        case RF_COMM_PS_WORK_GROUP_READ:        // ＰＵＳＨ ワークグループ読み込み
                            rtn = RF_command_execute(0x22);
                            break;

                        case RF_COMM_PS_WORK_GROUP_WRITE:       // ＰＵＳＨ ワークグループ書き込み
                            rtn = RF_command_execute(0x23);
                            break;

                        case RF_COMM_PS_DISPLAY_MESSAGE:        // ＰＵＳＨ メッセージ表示
                            rtn = RF_command_execute(0x1f);
                            break;

                        case RF_COMM_PS_REMOTE_MEASURE_REQUEST: // ＰＵＳＨ リモート測定指示
                            rtn = RF_command_execute(0x1e);
                            break;

                        case RF_COMM_PS_SET_TIME:               // ＰＵＳＨ 時計設定
                            rtn = RF_command_execute(0x19);
                            break;

						case RF_EVENT_EXECUTE:
                    		rtn = (uint8_t)AUTO_control(0); ///< @bug RF_EVENT_EXECUTEコマンドの返り値はREFUSEで良いのか？
                    		break;  //2020.Feb.04追加
                        default:
                            rtn = RFM_REFUSE;
                            break;

                    }

                }

                ERRORCODE.ACT0 = R500_replace_error_code(rtn);	// R500エラーをＴコマンドエラーに置き換える

//  このに少し追加する項目あり
//  R500BLEでは、わからない処理が入っている！！


                RF_buff.rf_res.status = END_OK;
            }

 //           RF_CS_INACTIVE();
            RF_RESET_INACTIVE();
			Printf("    ==== g_rf_mutex put 3 !!\r\n");
            tx_mutex_put(&g_rf_mutex);
			Printf("\r\n #####  rf_thread suspend !!!!\r\n\r\n");
            tx_thread_suspend(&rf_thread);
        }
        else{
            Printf("###rf_command mutex busy %02X\r\n", rf_command);
        }
    }//

}


/**
 * 無線モジュール電源ＯＮ時の初期化
 * ^@note   2020.Jun.22 mutexの処理を関数外に出した
 * @note	2020.Aug.21	delay使用を廃止してthread_sleepに変更
 */
static void rfm_power_on_initialize(void)
{

    uint32_t i;

    Printf("rfm_power_on_initialize()\r\n");

    ERRORCODE.ERR = ERR(ERR, NOERROR);                      // R500初期化
    ERRORCODE.ACT0 = ERR(RF, NOERROR);

    memset(latest, 0x00, 5);                                // 最終実行コマンド５文字をクリア

    if(my_rpt_number == 0)                                  // 親機だった場合、上り（子機へ）方向のコマンド番号だけ使用する
    {
        regf_cmd_id_back = (((regf_cmd_id_back & 0x000000ff) + 50) & 0x000000ff);
    }
    else
    {
        regf_cmd_id_back = 0x00000000;
    }


    // Ｒ５００無線モジュールＥＥＰＲＯＭへ間欠受信間隔の設定を書き込み（３回リトライあり）
    for(i = 0; i < 3; i++)
    {
        rfm_reset();                                        // 無線モジュール リセット
        rfm_initialize();                                   // 無線モジュール アクティブ
/* デバッグ用
        if(RF_soh_command(0xd2) != RFM_NORMAL_END){
            continue;
        }
*/
        // 間欠受信間隔の設定を書き込み（最初の通信がコマンド送信後、応答まで約３００ｍｓかかる）
        if(RF_rxintt(2) != RFM_NORMAL_END){

            if(RCOM.rxbuf.subcommand == 0x0F)       //0x31コマンドで0x0Fが返ってくる場合はブートローダモードの可能性が高い
            {
                //ブートフラグの確認(ブートローダ起動（ブート・スワップ状態）)
                Printf(" Get Boot Flags:");
                tx_thread_sleep (10);        //100ms
                if(SSP_SUCCESS == rfm_cmd_GetBootFlag())        //0xD7コマンドが通る場合ブートローダ
                {
                    Printf(" Success\r\n");
                    Printf("  Current Boot: %d\r\n",RCOM.rxbuf.data[3]);       //現在のブート領域  0:ブート・クラスタ0 ,1:ブート・クラスタ1
                    Printf("  After Reset Boot: %d\r\n",RCOM.rxbuf.data[1]);    //リセット後のブート領域  0:ブート・クラスタ0 ,1:ブート・クラスタ1
                    Printf(" Exec Boot Swap\r\n");

                    //ブートローダで起動しているのでブートスワップする
                    rfm_cmd_SwapBoot();     //0xE1 リセット後ブートスワップされる

					/*
                    if(RCOM.rxbuf.data[3] == 0)
                    {
                        if(RCOM.rxbuf.data[1] == 0){

                            Printf(" Boot Swap\r\n");
                            rfm_cmd_SwapBoot();     //0xE1 リセット後ブートスワップされる
						}else{
                            __NOP();//デバッグ用
                        }
                    }
                    else{
                        __NOP();//デバッグ用
                    }
                    */
                }
            }


            continue;
        }
        tx_thread_sleep(1); //10ms

        // 間欠受信停止
        if(RF_rxintt_stop(IT_STOP) != RFM_NORMAL_END){
            continue;
        }
        tx_thread_sleep(1); //10ms

        // 無線モジュールシリアル番号を読み取る
        if(RF_soh_command(0xb3) == RFM_NORMAL_END)
        {
            regf_rfm_serial_number = *(uint32_t *)&RCOM.rxbuf.data[0];
        }
        else
        {
            regf_rfm_serial_number = 0x00000000;
            continue;
        }
        tx_thread_sleep(1); //10ms
        // 無線モジュールバージョン番号を読み取る
        if(RF_soh_command(0x72) == RFM_NORMAL_END)
        {
            regf_rfm_version_number = (uint32_t)RCOM.rxbuf.data[0] + ((uint32_t)RCOM.rxbuf.data[1] << 8);

        }
        else
        {
            regf_rfm_version_number = 0x00000000;
            continue;
        }
        tx_thread_sleep(1); //10ms
	    if(RF_rpt_batt_set(0xee, 5) != RFM_NORMAL_END)		// RTR500BWは必ず電池残量5としておく
		{
	        continue;
		}

        break;
    }


    while(regf_battery_voltage == 0x0000eeee){      //regf_battery_voltageをセットしている個所が居ない
        rf_delay(5);
    }

    RF_CS_INACTIVE();                                           // ＣＳ Ｌｏｗ（無線モジュール スタンバイ）

    rfm_country = (regf_rfm_serial_number != 0x00000000) ? ((regf_rfm_serial_number & 0xf0000000) >> 24) : ((fact_config.SerialNumber & 0xf0000000) >> 24); // エラーなら本体仕向先をセットする
    // ＤＥＢＵＧ
    if(regf_rfm_serial_number == 0xffffffff){
        rfm_country = DST_JP;
    }
}




/**
 * 無線モジュール 初期化
 * @note ファイル内で使用
 * @note	2020.Aug.21	delay使用を廃止してthread_sleepに変更
 */
static void rfm_initialize(void)
{
    RF_CS_ACTIVE();                                                  // ＣＳ Ｌｏｗ（無線モジュール アクティブ）
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
 * 無線モジュール リセット
 * @note    ファイル内で使用
 * @note	2020.Aug.21	delay使用を廃止してthread_sleepに変更
 */
static void rfm_reset(void)
{
    RF_CS_ACTIVE();                                     // ＣＳ Ｌｏｗ（無線モジュール アクティブ）

    RF_RESET_ACTIVE();

    tx_thread_sleep(1); //10ms

    /*
    wait.ms_1.rf_wait = 2;
    do{;}while(wait.ms_1.rf_wait!=0);
*/
    RF_RESET_INACTIVE();
    /*
    wait.ms_1.rf_wait = 2;
    do{;}while(wait.ms_1.rf_wait!=0);
*/
    tx_thread_sleep(1); //10ms
}


/**
 * 無線モジュール 送信
 * @param pData  送信データへのポインタ（SOH/STXヘッダへのポインタ）
 * @note    2020.Jul.03 SUM/CRC付加を内包
 * @note    2020.Jul.03  引数numを廃止
 */
static void rfm_send(char *pData )
{

    uint16_t err;
    uint32_t wt;
    uint32_t i;
    uint16_t num;
/**/
	// 途中でRFMとの通信を切断した時の、ゴミ処理	
	err = g_sf_comms5.p_api->close(g_sf_comms5.p_ctrl);
	Printf("rfm_send com close %d \r\n", err);
	err = g_sf_comms5.p_api->open(g_sf_comms5.p_ctrl, g_sf_comms5.p_cfg);
	Printf("rfm_send com open %d \r\n", err);
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

    Printf("rfm_send start\r\n");

    for(i=0;i<num;i++){
        Printf("%02X ", pData[i]);
    }
    Printf("\r\n");


    // 1byte 520us  wait:10ms   19200bps
    wt = (uint32_t)(((num * 1000) / 19200) + 1 + 1);
	Printf("rfm_send wt %ld\r\n",wt);
	err = g_sf_comms5.p_api->lock(g_sf_comms5.p_ctrl, SF_COMMS_LOCK_TX, TX_WAIT_FOREVER);    //comm5ロック
    err = g_sf_comms5.p_api->write(g_sf_comms5.p_ctrl, (uint8_t *)pData, num, wt);
    g_sf_comms5.p_api->unlock(g_sf_comms5.p_ctrl, SF_COMMS_LOCK_TX);    //comm5ロック解除
    Printf("rfm_send %d(%d) wt:%d\r\n", err, num ,wt);
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
static uint8_t rfm_recv(uint32_t t)
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

	Printf("[rfm_recv Start] (%ld)(%ld)\r\n", wait.rfm_comm,t);  

	// SOH 受信待ちループ
	for(;;){

        RCOM.rxbuf.header = 0x00;

        if(RfRxtimeout == 0)    RfRxtimeout = 1;                // sakaguchi 2021.06.29 タイムアウトなし設定の場合、readタイムアウトは1(10ms)にする
        err = g_sf_comms5.p_api->read(g_sf_comms5.p_ctrl, (uint8_t *)&RCOM.rxbuf.header, 1, RfRxtimeout); //1Byte受信 タイムアウト 10ms->ゆざー指定に変更

        if(err == SSP_SUCCESS){
            if(RCOM.rxbuf.header == CODE_SOH){
                Printf("## rfm_recv SOH (%ld)\r\n", wait.rfm_comm);
                break;
            }
        }else if(err == SSP_ERR_TIMEOUT)
        {
            rtn = RFM_SERIAL_TOUT;
            goto exit_function;
        }

        if(t == UINT32_MAX){
           if(timer.int1000 > RF_buff.rf_req.timeout_time)     // タイムアウトなし設定なら、無線コマンド全体のタイムアウトで戻る
           {
                rtn = RFM_DEADLINE;
                goto exit_function;
           }
        }
        else if(wait.rfm_comm == 0){
            g_sf_comms5.p_api->close(g_sf_comms5.p_ctrl);
            g_sf_comms5.p_api->open(g_sf_comms5.p_ctrl, g_sf_comms5.p_cfg);
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
	Printf("\r\n## rfm_recv SOH OK (%ld)\r\n", wait.rfm_comm);  

    err = g_sf_comms5.p_api->read(g_sf_comms5.p_ctrl, (uint8_t *)&RCOM.rxbuf.command, 4, 3);    //commandからLengthまで4Byte受信 タイムアウト30ms→200ms→30ms
    if(err != SSP_SUCCESS){
		rtn = RFM_SERIAL_ERR2;
        goto exit_function;
    }

    if(RCOM.rxbuf.length == 0){                                 // 2023.03.01 受信データ長が0の場合、エラーで終了
        rtn = RFM_SERIAL_ERR2;
        goto exit_function;
    }

    //66Byteの受信制限を外した 2020.Aug.01
    err = g_sf_comms5.p_api->read(g_sf_comms5.p_ctrl, (uint8_t *)&RCOM.rxbuf.data, (uint32_t)(RCOM.rxbuf.length+2), 5);       //DATAからSUMまで受信 タイムアウト 50ms→200ms→50ms
    if(err != SSP_SUCCESS){
        rtn = RFM_SERIAL_ERR2;                                  // 2023.02.20 異常時の戻り値追加
        goto exit_function;
    }

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
        //timer.int125 = 0;										// ここでクリアするとリトライ時のTIMEパラメータ用のカウントに影響する
    }
// ここまで



	RfCh_Status = CH_OK;											// sakaguchi UT-0035
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
    else if(RCOM.rxbuf.subcommand == CODE_CH_BUSY){				// 無線モジュールからCH_BUSYを受信した	segi
		LED_Set(LED_BUSY, LED_ON);		// ここでいいの？？？？
        rtn = RFM_R500_CH_BUSY;
		RfCh_Status = CH_BUSY;									// sakaguchi UT-0035
        PutLog(LOG_RF, "Channel Busy");
    }
    else if(RCOM.rxbuf.subcommand == CODE_REFUSE){
        rtn = RFM_REFUSE;      //仮		NOMAL_ENDが返っていた
    }
// 2024 01 15 D.00.03.184 ↓
    else if(RCOM.rxbuf.subcommand == CODE_CRC){
        rtn = RFM_R500_NAK;
        PutLog(LOG_RF, "CRC Error");
    }
// 2024 01 15 D.00.03.184 ↑
    else{
        __NOP();		//ACK応答
    }

exit_function:;
  
    //g_sf_comms5.p_api->unlock(g_sf_comms5.p_ctrl, SF_COMMS_LOCK_RX);    //comm5ロック解除

    Printf("\r\n  rfm_recv rtn=%02X / %04X (%ld)\r\n", rtn, err,wait.rfm_comm );
    return(rtn);


}


/**
 * 無線モジュール エラー時の初期化
 * @param num   自局の番号
 */
static void relay_initialize(uint8_t num)
{
    SSP_PARAMETER_NOT_USED(num);       //未使用引数
    /*
    uint32_t i;

    for(i = 0; i < 3; i++)
    {
        rfm_reset();
        rfm_initialize();

        // 間欠受信を許可
        if(RF_rxintt_stop(IT_PERMIT) != RFM_NORMAL_END) continue;
        rf_delay(10);

        // グループＩＤ設定
        if(num == 0)
        {
            if(RF_group_id_set(PSTV_OFF, group_registry.id, my_rpt_number, group_registry.band, group_registry.frequency) != RFM_NORMAL_END) continue;
        }
        else
        {
            if(tx_mutex_get(&g_sfmem_mutex, WAIT_100MSEC) == TX_SUCCESS)         // シリアルフラッシュ ミューテックス確保 スレッド確定
            {
                if(RF_group_id_set(PSTV_ON, (uint8_t *)&my_settings.relay.id.low, my_settings.relay.number, my_settings.relay.band, my_settings.relay.channel) != RFM_NORMAL_END) continue;
                tx_mutex_put(&g_sfmem_mutex);
            }
        }

        rf_delay(10);

        // 中継機の電池残量（５～０）を無線モジュールに書き込む（必ず書き込む）
        if(RF_rpt_batt_set(0xee, BAT_level_0to5(regf_battery_voltage)) != RFM_NORMAL_END) continue;
        rf_delay(10);

        break;
    }
    */
}



/**
 * 中継機の電池残量（５～０）を無線モジュールに書き込む
 * @param past  セット済み電池残量（５～０）
 * @param curr  今回電池残量（５～０）
 * @return      結果
 * @note    前回セット値と同じなら送出しない regf_rfm_version_numberの上位にセット済み値を記憶 通信前のディレー付き
 *          無線モジュール電源ＯＦＦすると電池残量が消去される。再セットまで０になる。
 * @note    不要 中継機用
 */
static uint8_t RF_rpt_batt_set(uint8_t past, uint8_t curr)
{

    if(past == curr){
        return(RFM_NORMAL_END);
    }

    rf_delay(10);

    // ＲＦモジュールにセットした電池レベルをストア、前回値と異なるフラグもクリア
    regf_rfm_version_number = ((regf_rfm_version_number & 0x0000ffff) | (((uint32_t)curr << 16) & 0x00ff0000));


    RCOM.txbuf.header = CODE_SOH;
    RCOM.txbuf.command = 0x35;
    RCOM.txbuf.subcommand = 0x00;
    RCOM.txbuf.length = 4;
    RCOM.txbuf.data[0] = curr;
    RCOM.txbuf.data[1] = 0;
    RCOM.txbuf.data[2] = 0;
    RCOM.txbuf.data[3] = 0;

    rfm_send(&RCOM.txbuf.header);        //無線モジュール SOH/STXフレーム送信（SUM/CRC自動付加）


    return(rfm_recv(500));
}



/**
 * 間欠受信間隔の設定（Ｒ５００無線モジュールＥＥＰＲＯＭへ書き込み）
 * @param is    × 0.5[秒] = 間欠受信間隔[秒]
 * @return  結果
 * @note    不要かも
 */
static uint8_t RF_rxintt(uint8_t is)
{

    RCOM.txbuf.header = CODE_SOH;
    RCOM.txbuf.command = 0x31;
    RCOM.txbuf.subcommand = 0x00;
    RCOM.txbuf.length = 4;
    RCOM.txbuf.data[0] = is;
    RCOM.txbuf.data[1] = 0;
    RCOM.txbuf.data[2] = 0;
    RCOM.txbuf.data[3] = 0;

    rfm_send(&RCOM.txbuf.header);        //無線モジュール SOH/STXフレーム送信（SUM/CRC自動付加）

    return(rfm_recv(500));      //無線モジュール受信
}



/**
 * 間欠受信停止
 * @param is    １：停止 ０：解除
 * @return      結果
 * @note        不要かも
 */
static uint8_t RF_rxintt_stop(uint8_t is)
{
    if(is > 1){
        is = 1;
    }


    RCOM.txbuf.header = CODE_SOH;
    RCOM.txbuf.command = 0x3f;
    RCOM.txbuf.subcommand = 0x00;
    RCOM.txbuf.length = 4;
    RCOM.txbuf.data[0] = is;
    RCOM.txbuf.data[1] = 0;
    RCOM.txbuf.data[2] = 0;
    RCOM.txbuf.data[3] = 0;

    rfm_send(&RCOM.txbuf.header);        //無線モジュール SOH/STXフレーム送信（SUM/CRC自動付加）

    return(rfm_recv(500));      //無線モジュール受信
}


/**
 * グループＩＤ設定
 * @param sw    保存スイッチ
 * @param id    グループIDが入っているポインタ
 * @param numb  中継機番号(親機は０)
 * @param band  バンド
 * @param freq  周波数チャンネル
 * @return  結果
 */
static uint8_t RF_group_id_set(uint8_t sw, uint8_t *id, uint8_t numb, uint8_t band, uint8_t freq)
{
    RCOM.txbuf.header = CODE_SOH;
    RCOM.txbuf.command = (sw == PSTV_ON) ? 0x3a : 0x39;
    RCOM.txbuf.subcommand = 0x00;
    RCOM.txbuf.length = 10;

    memcpy((uint8_t *)RCOM.txbuf.data, id, 8);

    RCOM.txbuf.data[8] = numb;
    RCOM.txbuf.data[9] = (uint8_t)(freq + ((band & 0x07) << 5));           // bandは２＾０～２＾２使用

    rfm_send(&RCOM.txbuf.header);        //無線モジュール SOH/STXフレーム送信（SUM/CRC自動付加）

    return(rfm_recv(500));      //無線モジュール受信
}


#if 0
/**
 * キャンセル
 * @return  結果
 * @note    未使用
 */
static uint8_t RF_cancel(void)
{
    return(RF_soh_command(0x2f));
}
#endif

/**
 * Ｒ５００ ＳＯＨコマンド
 * @param cmd   コマンド
 * @return  結果
 */
static uint8_t RF_soh_command(uint8_t cmd)
{
    RCOM.txbuf.header = CODE_SOH;
    RCOM.txbuf.command = cmd;
    RCOM.txbuf.subcommand = 0x00;
    RCOM.txbuf.length = 4;

    RCOM.txbuf.data[0] = 0;
    RCOM.txbuf.data[1] = 0;
    RCOM.txbuf.data[2] = 0;
    RCOM.txbuf.data[3] = 0;

    rfm_send(&RCOM.txbuf.header);        //無線モジュール SOH/STXフレーム送信（SUM/CRC自動付加）

    return(rfm_recv(500));      //無線モジュール受信
}


/**
 * ＲＦコマンド実行
 * @param order ＲＦコマンド
 * @return
 */
uint8_t RF_command_execute(uint8_t order)
{
    uint8_t rtn;
//    uint8_t *pt;


//----- 無線通信の失敗が連続する場合、念の為無線モジュールをリブートする 20200718 segi------------
	if(RF_Err_cnt > 5){
        rfm_reset();
        rfm_initialize();
		RF_Err_cnt = 0;
        PutLog(LOG_RF, "RFM RESET");                // sakaguchi 2021.06.29
	}
//------------------------------------------------------------------------------------------------


    Printf("###RF_command execute %02X\r\n", order);
    Printf("od:%02X id:%ld rpt:%02X band:%02X freq:%02X %02X\r\n", order,group_registry.id, my_rpt_number, group_registry.band, group_registry.frequency, complement_info[0]);
    rtn = RF_group_id_set(PSTV_OFF, (uint8_t *)group_registry.id, my_rpt_number, group_registry.band, group_registry.frequency);   // グループＩＤ設定
    Printf("###RF_command execute %02X\r\n", rtn);
// sakaguchi 2021.06.29 ↓
    if(rtn != RFM_NORMAL_END){
        RF_Err_cnt++;                               // 無線通信エラーカウンタを更新
        PutLog(LOG_RF, "GROUP ID SET[%x][%d][%d][%x]", rtn, RF_Err_cnt, RF_buff.rf_req.timeout_time,order);
        return(rtn);
    }else{
        RF_Err_cnt = 0;
    }
// sakaguchi 2021.06.29 ↑
    R500_command_make(order);                                   // Ｒ５００用コマンド変換

    //pt = &r500.soh;
    //for(int i=0;i<22;i++)
    //    Printf("%02X ", *pt++);
    //Printf("\r\n");
    
    rtn = R500_radio_communication();                           // コマンド実行
    Printf("###RF_command execute end %02X\r\n", rtn);
    
    if(rtn == RFM_R500_CH_BUSY){                                // sakaguchi 2021.01.25
        NOISE_CH[RF_buff.rf_req.current_group-1] = 1;           // チャンネルビジー
    }else{
        NOISE_CH[RF_buff.rf_req.current_group-1] = 0;
    }

    if((rtn != RFM_NORMAL_END) && (rtn != RFM_R500_CH_BUSY)){   // CH BUSYは除外
        PutLog(LOG_RF, "RFCMD EXE[%x][%x]", rtn, order);        // sakaguchi 2021.07.21 ログ見直し
    }

    return(rtn);
}



/**
 * 制御部とのやり取り構造体に入っているデータをR500に送信可能な形に変換する
 * @param cmd1  無線コマンド１
 * @note    huge_buffer[0]～ 追加転送データ＋中継機情報
 */
static void R500_command_make(uint8_t cmd1)
{
    r500.soh = CODE_SOH;
    r500.cmd1 = 0x42;
    r500.cmd2 = 0x00;
    r500.len = 16;

    Printf("R500 command make %02X \r\n", cmd1);

    if((group_registry.altogether > 1) && (RF_buff.rf_req.short_cut == 0)){
        cmd1 |= 0x80;    // ショートカット動作しない（子機宛てコマンドならビットセットしない）
    }

    switch(cmd1 & 0x7f)
    {
        case 0x66:      // ***** 一斉記録開始 *****
            r500.para2[0] = complement_info[0];                 // 最大子機番号
            r500.para2[1] = 0x00;
            r500.data_size = 64;
            break;

        case 0x65:      // ***** 記録開始（設定値書き込み） *****
        case 0x6a:      // ***** 警報条件設定 *****
        case 0x1c:      // ***** [PUSH]設定値書き込み *****
//---------------------------------------------------- segi
        case 0x25:      // ***** 子機のBluetooth設定Write *****
//---------------------------------------------------- segi
    		r500.para2[0] = 0x00;
            r500.para2[1] = 0x00;
            r500.data_size = 64;
            break;

        case 0x63:      // 子機登録変更
//        case 0x69:      // ***** 上下限値書き換え *****
            r500.para2[0] = complement_info[0];
            r500.para2[1] = complement_info[1];
            r500.data_size = 64;
            break;


        case 0x69:      // ***** BLE通信OFF/ON *****					// 2019.09.15追加 segi

		case 0x30:		// ＥＷＲＳＲ（子機と中継機の一斉検索）

        case 0x28:      // リアルスキャン（旧互換）
        case 0x68:      // ***** リアルスキャン *****

        case 0x2c:      // 現在値取得（旧互換）
        case 0x2d:      // ***** 子機検索 *****
        case 0x27:      // ***** 記録停止 *****
        case 0x60:      // ***** 記録データ吸上げ(データ個数) *****
        case 0x61:      // ***** 記録データ吸上げ(データ番号) *****
        case 0x62:      // ***** 記録データ吸上げ(データ時間) *****
        case 0x64:      // ***** 記録設定テーブル取得 *****

        case 0x6c:      // ***** 現在値取得 *****
        case 0x6d:      // ***** 子機検索(ブロードキャスト) *****
        case 0x6e:      // ***** 記録設定変更プロテクト *****
        case 0x6b:      // ***** モニタリング間隔変更 *****
        case 0x40:      // ***** 中継機検索 *****
        case 0x0e:      // ***** 受信履歴取得 *****
        case 0x0f:      // ***** 受信履歴消去 *****

        case 0x1a:      // ***** [PUSH]記録データ消去 *****
        case 0x22:      // ***** [PUSH]設定値、品目、作業者、ワークテーブル読み込み *****
        case 0x1e:      // ***** [PUSH]測定指示 *****
        case 0x2a:      // ***** [PUSH]データ吸い上げ(番号指定)*****
        case 0x24:      // ***** [PUSH]データ吸い上げ(時間指定) *****

            r500.para2[0] = complement_info[0];
            r500.para2[1] = complement_info[1];
            r500.data_size = 0;
            break;

        case 0x19:      // ***** [PUSH]時刻設定 *****
        case 0x1f:      // ***** [PUSH]メッセージ表示 *****
        case 0x23:      // ***** [PUSH]設定値、品目、作業者、ワークテーブル書き込み *****

            r500.para2[0] = complement_info[0];
            r500.para2[1] = complement_info[1];

            // データサイズはstart_rftask()で設定
            //r500.data_size = 1024;
            break;

        default:        // ***** その他の無線コマンド *****
            r500.data_size = 0;
            break;
    }

    if(group_registry.altogether == 0){
        group_registry.altogether = 1;   // 中継機情報が１の時、中継機の数は０（０は異常値なので１以上にしておく）
    }

    memcpy(&huge_buffer[r500.data_size], rpt_sequence, group_registry.altogether); // 中継情報を保存する

    r500.end_rpt = rpt_sequence[group_registry.altogether - 1];

    r500.para1.rpt_cnt = (uint8_t)group_registry.altogether;

    if(group_registry.altogether <= 1)                          // ########## 中継情報がない場合（中継機情報が１の時、中継機の数は０） ##########
    {
        r500.next_rpt = 0xff;                                   // 子機と無線通信する（自局は必ず親なので中継処理は不要）
        r500.end_rpt = my_rpt_number;
        r500.para1.rpt_cnt = 0;
    }
    else                                                        // ########## 中継情報が複数バイトある場合 ##########
    {
        huge_buffer[r500.data_size] = my_rpt_number;
        r500.next_rpt = huge_buffer[r500.data_size + 1];        // 次の中継機
    }

    r500.data_size = (uint16_t)(r500.data_size + r500.para1.rpt_cnt);       // 追加データバイト長に中継情報を加算する

//    r500.start_rpt = my_rpt_number;                             // my_rpt_numberには中継機番号(親機は0)が入っている
	r500.route.start_rpt = my_rpt_number;								// my_rpt_numberには中継機番号(親機は0)が入っている
	if((cmd1 & 0x7f) == 0x30){
	    r500.route.rpt_max = complement_info[2];	// ＥＷＲＳＲコマンドの場合、検索最大中継機番号を入れる
	    //r500.route.rpt_max = group_registry.altogether;				// ＥＷＲＳＲコマンドの場合、検索最大中継機番号を入れる
	}

    r500.my_rpt = my_rpt_number;                                // my_rpt_numberには中継機番号(親機は0)が入っている

    regf_cmd_id_back = ((regf_cmd_id_back & 0x000000ff) + 1) & 0x000000ff;
    r500.para1.cmd_id = regf_cmd_id_back & 0x000000ff;          // コマンドIDを設定する

    r500.rf_cmd1 = (r500.next_rpt == 0xff) ? TxLoop_Unit : TxLoop_Rpt;  //子機通信の巡回回数  ６４回：２秒（日本向けの巡回回数はモジュール自身が変更する） /中継機通信の巡回回数 ３２回：１秒

    r500.rf_cmd2 = cmd1;                                        // 無線コマンド
    r500.node = ru_registry.rtr501.number;                      // 子機番号
	r500.node = (char)(r500.node + NTH_2in1);

    r500.time_cnt = 0;                                          // 子機への無線コマンド送信までの時間を初期化する
}


/**
 * 子機または中継機と無線通信する（子機宛てならリトライ３回）
 * @return  結果
 * @note    子機宛てだけリトライ 親機－中継機間はＢＵＳＹエラーだけリトライ
 */
static uint8_t R500_radio_communication(void)
{
    uint8_t i, rtn, cc[sizeof(r500)];

	Printf("	R500_radio_communication start\r\n");

    calc_rf_timeout(0);                                         // タイムアウト時間計算
    Start1000msec();                                            // １０００ｍｓタイマースタート

    memcpy(cc, &r500.dummy, sizeof(r500));                                // コマンド退避

    for(i = 0; i < RF_RETRY; i++)                               // リトライ３回  デバックでは、1回
    {
        repeater_is_engaged = true;                             // 自局宛の無線通信中
        wait.inproc_rfm = WAIT_1SEC + WAIT_200MSEC;             // 最低、１秒間の無線ＬＥＤ点灯を確保する

        // RTR-500Cは、ここでi=0のときだけショートカット動作許可する

        sram.record = 0;

        rtn = R500_radio_communication_once();

        if(rtn == RFM_NORMAL_END){
            break;
        }
        if(rtn == RFM_LOW_BATT){
            break;
        }

        // エラー応答があったとき
        repeater_is_engaged = true;                             // 自局宛の無線通信中
        wait.inproc_rfm = WAIT_1SEC + WAIT_200MSEC;             // 最低、１秒間の無線ＬＥＤ点灯を確保する

        rf_delay(1000);                                          // 無線モジュールが中継機にＡＣＫ返す時間を確保すること

        repeater_is_engaged = false;

        if(RF_buff.rf_req.cancel == 1U)                         // 途中キャンセル要求があった場合
        {
            rtn = RFM_CANCEL;
            break;
        }

        memcpy(&r500.dummy, cc, sizeof(r500));                            // コマンド復帰

        if(r500.next_rpt != 0xff)                               // 中継機宛てだった場合
        {
            //if((rtn != RFM_R500_BUSY) && (rtn != RFM_RPT_ERROR) && (rtn != RFM_R500_CH_BUSY)) break;  // ＢＵＳＹエラー以外はリトライしない
            if((rtn == RFM_NORMAL_END) || (rtn == RFM_RU_PROTECT)){
                break;   // 無線プロテクト以外はリトライする
            }

            relay_initialize(my_rpt_number);                    // 無線モジュールを初期化する

            regf_cmd_id_back = ((regf_cmd_id_back & 0x000000ff) + 1) & 0x000000ff;
            r500.para1.cmd_id = regf_cmd_id_back & 0x000000ff;      // コマンドIDを設定する
        }

        if(rtn == RFM_R500_CH_BUSY){
            wait.channel_busy = WAIT_2SEC;
        }

        rf_delay(100);
        if(rfm_country == DST_JP){
            rf_delay(1200);                // ２秒休止の確保
        }

        timer.int1000 = 0;                                      // タイムアウトクリア
    }

    repeater_is_engaged = false;

    Stop1000msec();                                             // １０００ｍｓタイマーストップ

	Printf("	R500_radio_communication end %d\r\n", rtn);
    return(rtn);
}


/**
 * 子機または中継機と無線通信する（リトライ無し）
 * @return  結果
 * @note	LO_HI_LONGマクロ廃止
 */
static uint8_t R500_radio_communication_once(void)
{
    uint8_t rtn, order;

    uint32_t dword_data;

	Printf("	R500_radio_communication_once start\r\n");

    // 末端中継機の場合、rx500_relay_proceed()でRF_buff.rf_res.timeにr500.time_cntを保存してある。親機の場合、RF_buff.rf_res.timeはゼロ
    r500.time_cnt = (uint16_t)(RF_buff.rf_res.time + (uint32_t)timer.int125);         // Ｔコマンド処理開始してからここまでの１２５ｍｓタイマカウント数

    order = r500.rf_cmd2 & 0x7f;

    //Printf("R500_radio_communication_once order %02X \r\n", order);

    switch(order)
    {
        case 0x66:
        case 0x65:

			if((order == 0x65) && (huge_buffer[17] & 0x01) != 0x00){ // RF_COMM_SETTING_WRITE_FORMAT0 のとき無線マスクバイトの記録開始ビットが１（無効）ならエラーにしない
				;//break;
			}
			else if((Regist_Data[2] == 0xF8) && ((EWSTW_Data[30] & 0x01) == 0)){	// RTR505の時、設定項目フラッグbit0=0(記録関連変更しない)ならエラーにしない
				;//break;
			}
	    	else{
			
           		dword_data =*(uint32_t *)&huge_buffer[12];                        // 予約待ち時間
	            if(dword_data <= (uint32_t)(Conv125msec((uint16_t)(r500.time_cnt + RfDelay_Unit)))){
	                return(RFM_RT_SHORTAGE);   // 予約待ち時間が(ここまでの)無線通信時間より短い場合、記録開始までの秒数足りないエラー
	            }
	    	}
            break;

        default:
            break;
    }

    // Ｒ５００に無線コマンド送信と第１応答を受信

    //if((rtn = R500_send_recv(300, huge_buffer)) != RFM_NORMAL_END) goto R500_rc_end;  // R500にコマンド送信と応答受信（コマンド送信終わりから応答開始まで約１０ｍｓ）
    rtn = R500_send_recv(6000, (uint8_t *)huge_buffer);
    if(rtn != RFM_NORMAL_END)
    {
        if(r500.next_rpt != 0xff)                               // 中継機へ無線コマンドを送った場合
        {
            if(rfm_recv(1400) == RFM_NORMAL_END)                // 次の中継機コマンドまたはダミー送信が受信できた場合は、中継機通信成功と見なす
            {
                if(RCOM.rxbuf.command == 0x42)
                {
					RF_Err_cnt = 0;				// 中継機宛の無線通信は成功した
                    rtn = RFM_NORMAL_END;
                    goto bailout;
                }
            }
			else{
				RF_Err_cnt = (uint8_t)(RF_Err_cnt + 1);	// 中継機宛の無線通信は失敗した
			}
        }

        goto R500_rc_end;   // R500にコマンド送信と応答受信（コマンド送信終わりから応答開始まで約１０ｍｓ）
    }
    else{
        ;//Printf("R500_radio_communication_once 2 rtn %02X %02X \r\n", rtn, r500.next_rpt);
    }

    bailout:;

    if(r500.next_rpt == 0xff)                                   // 子機へ無線コマンドを送った場合 第２応答以降を受信（第１応答終わりから第２応答開始まで約１２ｍｓ）
    {
        // 末端中継機の場合、rx500_relay_proceed()でRF_buff.rf_res.timeにr500.time_cntを保存してある。親機の場合、RF_buff.rf_res.timeはゼロ
        // ここでは通信リトライがあるかもしれないのでRF_buff.rf_res.timeを変更してはいけない

        timer.arrival = 0;                                      // 子機が応答したときまでの秒数ラッチを１回だけ許可する rfm_recv()でラッチ
        timer.latch_enable = true;

        //Printf("R500_radio_communication_once 3 order %02X \r\n", order);

		r500.data_size = 0;

        switch(order)
        {
            case 0x66:  // 一斉記録開始
            case 0x40:  // 中継機検索
            case 0x6d:  // 子機検索(ブロードキャスト)
            case 0x2d:  // 子機検索

                if((rtn = R500_relay_rf_scan_reply(order)) != RFM_NORMAL_END){
                    goto R500_rc_end;
                }
                break;

            case 0x27:  // 記録停止
            case 0x6e:  // プロテクト
            case 0x6b:  // モニタリング間隔変更

            // ＰＵＳＨコマンド
            case 0x1e:  // リモート測定指示
            case 0x1a:  // データ消去

                if((rtn = R500_koki_record_stop_reply()) != RFM_NORMAL_END){
                    goto R500_rc_end;
                }
                break;

            case 0x63:  // 子機登録変更
                if((rtn = R500_koki_record_start_reply(order)) != RFM_NORMAL_END){
                    rtn = RFM_KOKI_ERROR;
                    goto R500_rc_end;
                }
                break;

            case 0x65:  // 記録開始(個別)
            case 0x69:  // 上下限値書き換え
            case 0x6a:  // 警報条件設定

        	case 0x25:	// BLE設定Write
        	
            // ＰＵＳＨコマンド
            case 0x19:  // 時刻設定
            case 0x1c:  // 設定値書き込み
            case 0x1f:  // メッセージ表示
            case 0x23:  // 品目、作業者、ワークテーブル書き込み

                if((rtn = R500_koki_record_start_reply(order)) != RFM_NORMAL_END){
                    goto R500_rc_end;
                }
                break;

            case 0x64:  // 設定値受信

        	case 0x2b:	// BLE設定Read
        	
                        // ＰＵＳＨコマンド
            case 0x22:  // 設定値、品目、作業者、ワークテーブル取得

                if((rtn = R500_koki_setting_data_reply()) != RFM_NORMAL_END){
                    goto R500_rc_end;
                }
                break;

            case 0x28:  // リアルスキャン（旧互換）
            case 0x68:  // リアルスキャン

                if((rtn = R500_koki_real_scan_reply()) != RFM_NORMAL_END){
                    goto R500_rc_end;
                }
                break;

			case 0x30:	// ＥＷＲＳＲ（子機と中継機の一斉検索）

				if((rtn = R500_koki_whole_scan_reply()) != RFM_NORMAL_END){
				    goto R500_rc_end;
				}
				break;

            case 0x2c:  // 現在値取得（旧互換）
            case 0x6c:  // 現在値取得

                if((rtn = R500_koki_present_data_reply()) != RFM_NORMAL_END){
                    goto R500_rc_end;
                }
                break;

            case 0x60:  // データ吸上げ
            case 0x61:  // データ吸上げ
            case 0x62:  // データ吸上げ

            // ＰＵＳＨコマンド
            case 0x2a:  // データ吸い上げ(番号指定)
            case 0x24:  // データ吸い上げ(時間指定)

                if((rtn = R500_koki_setting_data_reply()) != RFM_NORMAL_END){ // ＳＲＡＭに設定値を記録する
                    goto R500_rc_end;
                }
                if((rtn = R500_koki_upload_data_reply()) != RFM_NORMAL_END){
                    goto R500_rc_end;
                }
                break;

            case 0x0e:  // 中継機の受信履歴読み込み
            case 0x0f:  // 中継機の受信履歴クリア
            default:
                rtn = RFM_REFUSE;
                goto R500_rc_end;
        }

/*
		if(timer.latch_enable == true)
		{
			timer.latch_enable = false;
	
			timer.arrival = timer.int125;						// 子機が応答した時間をラッチ
			//timer.int125 = 0;										// ここでクリアするとリトライ時のTIMEパラメータ用のカウントに影響する
		}
*/
		// 20200529_上記を下記に修正
    	//__disable_irq();		この関数は何ですか？？？？ 知らない使い方はしない！！
		if(timer.int125 >= timer.arrival){
		    timer.int125 = (uint16_t)(timer.int125 - timer.arrival);	// 子機が応答した時点をカウント開始にする
		}
		///__enable_irq();
        

        *(uint16_t *)&r500.para2[0] = Conv125msec((uint16_t)(RF_buff.rf_res.time + timer.arrival));   // コマンド開始から子機が応答したときまでの秒数rfm_recv()でラッチ
//この部分が RTR500BCと違っている
		// RTR500BC r500.time_cnt = timer.int125;							// 末端機に届いてＴコマンドになるまでの時間をカウント開始
		// RTR500BC if(my_rpt_number != 0) timer.int125 = 0;				// 自局が親機の場合はクリアしない（直接LASTパラメータになる）、末端中継機はr500.time_cntへ現在時を一旦格納、手前中継機への転送時間をカウントするためクリア


//        r500.time_cnt = 0;                                      // 末端機に届いてＴコマンドになるまでの時間をカウント開始
// 20200529_上記を下記に変更
		r500.time_cnt = timer.int125;							// 末端機に届いてＴコマンドになるまでの時間をカウント開始

        RF_buff.rf_res.time = *(uint16_t *)&r500.para2[0];                        // 自機のコマンド応答のためにコマンド開始から子機が応答したときまでの秒数を確定する
        property.acquire_number = r500.data_size;
    }
    else    // 中継機へ無線コマンドを送った場合
    {
        timer.latch_enable = false;

        switch(order)
        {
			case 0x30:	// ＥＷＲＳＲ（子機と中継機の一斉検索）

            case 0x0e:  // 中継機の受信履歴読み込み
            case 0x0f:  // 中継機の受信履歴クリア

            case 0x6e:  // プロテクト
            case 0x6b:  // モニタリング間隔変更

            case 0x63:  // 子機登録変更
            case 0x65:  // 記録開始(個別)
            case 0x69:  // 上下限値書き換え		// BLE ON/OFFに変更 2019.09.15
            case 0x6a:  // 警報条件設定

            case 0x27:  // 記録停止
            case 0x28:  // リアルスキャン（旧互換）
            case 0x2c:  // 現在値取得（旧互換）

            case 0x66:  // 一斉記録開始g
            case 0x40:  // 中継機検索
            case 0x6d:  // 子機検索(ブロードキャスト)
            case 0x2d:  // 子機検索

            case 0x60:  // データ吸上げ
            case 0x61:  // データ吸上げ
            case 0x62:  // データ吸上げ

            case 0x68:  // リアルスキャン
            case 0x64:  // 設定値受信
            case 0x6c:  // 現在値取得

            // ＰＵＳＨコマンド
            case 0x22:  // 設定値読み込み
            case 0x1c:  // 設定値書き込み
            case 0x2a:  // データ吸い上げ(番号指定)
            case 0x24:  // データ吸い上げ(時間指定)

            case 0x1a:  // データ消去
            case 0x19:  // 時刻設定
            case 0x1f:  // メッセージ表示
            case 0x23:  // 品目、作業者、ワークテーブル書き込み

            case 0x1e:  // リモート測定指示

			case 0x25:	// 子機ＢＬＥ設定の書き込み
			case 0x2B:	// 子機ＢＬＥ設定の読み込み

                if((rtn = R500_relay_reply(order)) != RFM_NORMAL_END){
                    goto R500_rc_end;
                }
                break;

            default:
                rtn = RFM_REFUSE;
                goto R500_rc_end;
        }

        if(r500.para1.cmd_id != (regf_cmd_id_back & 0x000000ff)){
            rtn = RFM_RPT_ERROR;   // コマンド応答の中のコマンドID一致しない場合は中継機間通信エラーと見なす
        }

        // 末端中継機がr500.time_cntをクリアする前にr500.para2へr500.time_cntを移す
        RF_buff.rf_res.time = *(uint16_t *)&r500.para2[0];                        // コマンド開始から子機が応答したときまでの秒数セット（中継機経由の場合は中継機の r500.para2 に格納されている）
    }

    R500_rc_end:;

    Printf("R500_radio_communication_once rtn %02X\r\n", rtn);
    return(RF_buff.rf_res.status = rtn);
}




/**
 * Power SW ON時に呼ぶ
 * RTR-500W対応で、auto_control.download[0]のクリアはやめる
 * @param sw    0: download データ番号もクリアする
 * 1: download データ番号は何もしない
 */
void RF_power_on_init(uint8_t sw)
{
	int cnt;

    Printf("RF_power_on_init() \r\n");

	if(sw == 0){
//		// 自動データ吸上げの前回データ番号クリア
        for(cnt = 0 ; cnt < 128 ; cnt++){						//
            memset(&auto_control.download[cnt].group_no, 0xFF, 2);
            memset(auto_control.download[cnt].siri, 0x00, 6);
        }
//		RF_full_moni_init();						        // モニタリング用テーブルの初期化       // 2020/08/24 sakaguchi del
	}

	WR_clr();												// 警報条件記憶レジスタクリア

    // 警報メールフラグクリア(16bit対応済み)
	for(cnt = 0 ; cnt < 128 ; cnt++){						// 
        memset(&auto_control.w_config[cnt].group_no, 0xFF, 2);
        memset(auto_control.w_config[cnt].now, 0x00, 6);        //now,before,on_off 6Byte
        memset(&auto_control.w_config_back[cnt].group_no, 0xFF, 2);     // sakaguchi 2021.03.09
        memset(auto_control.w_config_back[cnt].now, 0x00, 6);           // sakaguchi 2021.03.09
	}

	memset(retry_buff.download_buff[0].do_rpt, 0x00, sizeof(retry_buff));

	ALM_bak_all_clr(100);

	auto_control.backup_cur = 0;
	auto_control.backup_now = 0;
	auto_control.backup_warn = 0;
	auto_control.backup_sum = 0;

	auto_control.crc = crc32_bfr(&auto_control, 5122);		// ((128*8)*5)+2	CRC追加	2020/08/22 segi

	//chk_sram_ng();


	//RF_full_moni();
	
	
}
//********** end of RF_power_on() **********

/**
 * 警報バックアップ用データオールクリア
 * @param size
 */
static void ALM_bak_all_clr(uint8_t size)
{
    SSP_PARAMETER_NOT_USED(size);                   // 未使用引数   // sakaguchi 2021.03.01
    memset(&ALM_bak[0], 0x00, sizeof(ALM_bak));     // 全消去

//	int cnt;
//	for(cnt = 0 ; cnt < size ; cnt++){
//        memset(&ALM_bak[cnt].group_no, 0x00, 6);  // group_no,unit_no,format1クリア
//        memset(&ALM_bak[cnt].f1_grobal_time[0], 0x00, 4);  // format1クリア
//        memset(&ALM_bak[cnt].f2_grobal_time[0], 0x00, 4);   // format2クリア
//		memset(&ALM_bak[cnt].f3_grobal_time[0], 0x00, 4);   // format3クリア
//	}

	ALM_bak_cnt = 0;
	ALM_bak_utc = 0;
}
//***** end of ALM_bak_all_clr() *****





/**
 * 警報の有無のチェック
 * @attention   ble_thread, rf_threadから呼ばれる
 * @return      警報数
 */
uint16_t Warning_Check(void)
{
	int i;
	uint16_t kw_cnt = 0;
	uint8_t alm = 0;
	
	uint16_t now ,on_off, before;
//	uint16_t Warn;

//    Printf("\r\nWarning_Check\r\n");
	for(i=0;i<128;i++){
		if((auto_control.w_config[i].group_no==0xff) && (auto_control.w_config[i].unit_no==0xff)){
		    break;
		}
//		now = SRD_W( auto_control.w_config[i].now );			// .now[0] と .now[1] を連結して now に読み出す。
//		before = SRD_W( auto_control.w_config[i].before );		// .before[0] と .before[1] を連結して before に読み出す。
//		on_off = SRD_W( auto_control.w_config[i].on_off );		// .no_off[0] と .on_off[1] を連結して on_off に読み出す。

        now = *(uint16_t *)auto_control.w_config[i].now;            // .now[0] と .now[1] を連結して now に読み出す。     キャストに変更 2020.01.21
        before = *(uint16_t *)auto_control.w_config[i].before;      // .before[0] と .before[1] を連結して before に読み出す。     キャストに変更 2020.01.21
        on_off = *(uint16_t *)auto_control.w_config[i].on_off;      // .no_off[0] と .on_off[1] を連結して on_off に読み出す。     キャストに変更 2020.01.21


		if( now != before){		// 前回と今回が不一致なら警報発生
//			Warn = 0;
			if((now & 0xFCFC) != (before & 0xFCFC)){		
				kw_cnt++;
			}
			else{
				if( !(before & BIT(0)) && (now & BIT(0)) && !(on_off & BIT(0))){
				    kw_cnt++;
				}
				if( (before & BIT(0)) && !(now & BIT(0)) && !(on_off & BIT(0))){
				    kw_cnt++;
				}
				if( !(before & BIT(0)) && (now & BIT(0)) && (on_off & BIT(0))){
				    kw_cnt++;
				}

				if( !(before & BIT(1)) && (now & BIT(1)) && !(on_off & BIT(1))){
				    kw_cnt++;
				}
				if( (before & BIT(1)) && !(now & BIT(1)) && !(on_off & BIT(1))){
				    kw_cnt++;
				}
				if( !(before & BIT(1)) && (now & BIT(1)) && (on_off & BIT(1))){
				    kw_cnt++;
				}

				if( !(before & BIT(8)) && (now & BIT(8)) && !(on_off & BIT(8))){
				    kw_cnt++;
				}
				if( (before & BIT(8)) && !(now & BIT(8)) && !(on_off & BIT(8))){
				    kw_cnt++;
				}
				if( !(before & BIT(8)) && (now & BIT(8)) && (on_off & BIT(8))){
				    kw_cnt++;
				}

				if( !(before & BIT(9)) && (now & BIT(9)) && !(on_off & BIT(9))){
				    kw_cnt++;
				}
				if( (before & BIT(9)) && !(now & BIT(9)) && !(on_off & BIT(9))){
				    kw_cnt++;
				}
				if( !(before & BIT(9)) && (now & BIT(9)) && (on_off & BIT(9))){
				    kw_cnt++;
				}


			}
		}
		
		if(now != 0){
		    alm++;
		}


		//Printf("[Warning (%d) group_no:%d unit_no:%d](%d)  [ before:%04X now:%04X on_off:%04X]\r\n", i, auto_control.w_config[i].group_no,auto_control.w_config[i].unit_no,kw_cnt , before, now , on_off );

	}

	Printf("\r\n[Warning count %d]\r\n", kw_cnt);


#ifdef	_RTR500_GSM_
	if(now){
		rtr500gsm_output_pin(1);		//	警報出力オン
		LED.Set(LED_WARNING);			//  LED 点灯
	}
	else{
		rtr500gsm_output_pin(0);		//	警報出力オフ
		LED.Reset(LED_WARNING);			//  LED 消灯
	}
#endif //_RTR500_GSM_


	return (kw_cnt);
}




/**
 * R500にコマンド送信と応答受信
 * @param t     タイムアウト時間ｍｓ
 * @param buf   追加送信データアドレス
 * @return  結果
 * @note    追加データ（６４バイト越えるデータ）未対応
 */
static uint8_t R500_send_recv(uint16_t t, uint8_t *buf)
{
    uint8_t rtn, req[2];

    //ushort bs, i, error_max, error_cnt, busy;                 // Prototype3 2018_6_29
    uint16_t i, busy;

	Printf("	R500_send_recv() t=%d \r\n", t);
	/* rfm_send() に内包 2020.Jul.03
	calc_checksum_data(&r500.soh);
*/
    rfm_send(&r500.soh);        //無線モジュール SOH/STXフレーム送信（SUM/CRC自動付加）

	Printf("	R500_send_recv()  rfm_send  end \r\n");
    busy = 0;
    relay_rssi.level_0x34 = 0;

    while(1U)
    {
        rtn = rfm_recv(t);
        if(RCOM.rxbuf.command != 0x34){
            break;                   // 正常受付応答か？
        }

        //if(RCOM.rxbuf.data[2] == CODE_BUSY)                     // Prototype3 2018_6_29 Ｒ５００モジュールがＢＵＳＹ応答だったら再受信（従来ここでＢＵＳＹ応答だったらエラー処理）
        if(RCOM.rxbuf.subcommand == CODE_BUSY)                     // Prototype3 2018_6_29 Ｒ５００モジュールがＢＵＳＹ応答だったら再受信（従来ここでＢＵＳＹ応答だったらエラー処理）
        {
            if(busy++ > 20){
                return(RFM_R500_BUSY);              // Ｒ５００は２秒ごと最大約１０回、ｂｕｓｙ出す
            }
            continue;
        }

        if((RCOM.rxbuf.data[2] == CODE_NAK) && (RCOM.rxbuf.data[3] == 0xff)){
            return(RFM_RU_PROTECT);    // 無線プロテクトされている場合
        }

        if(RCOM.rxbuf.length > 4){
            relay_rssi.level_0x34 = RCOM.rxbuf.data[4];   // RSSIレベルを保存する
        }

        if(rtn != RFM_NORMAL_END)
        {
            return(rtn);                                        // 最初の０ｘ３４がＢＵＳＹならエラーリターン
        }

        if(r500.data_size == 0){
            return(RFM_NORMAL_END);         // 追加転送データ無ければここで終わり
        }

        // 更に送信するデータがある場合（Ｒ５００から送信リクエストがある場合）

        if(RCOM.rxbuf.data[1] == 0xff){
            break;                                       // Ｒ５００からの要求ブロック数が０ｘｆｆ００以上ならエラー
        }
        if((RCOM.rxbuf.data[0] == 0x00) && (RCOM.rxbuf.data[1] == 0x00)){
            break;     // Ｒ５００からの要求ブロック数が０ｘ００００ならエラー
        }

        // Prototype3 2018_6_29
        //bs = (r500.data_size - 1) / 64 + 1;                   // 追加転送するブロック数
        //error_max = retray_size(bs);                          // ＢＵＳＹリトライ回数
        //i = error_cnt = 0;
        i = 0;

        req[0] = RCOM.rxbuf.data[0];                            // 要求されたデータ番号ラッチ
        req[1] = RCOM.rxbuf.data[1];

        //rf_delay(5);   これは要らない！！　2020 08 27 
        busy = 0;

        while(1U)
        {
            if(RF_buff.rf_req.cancel == 1U){
                return(RFM_CANCEL); // 途中キャンセル要求があった場合
            }

            //if(watch_battery() == ON) return(RFM_LOW_BATT);   // 電池電圧監視

            RCOM.txbuf.header = CODE_SOH;
            RCOM.txbuf.command = 0x33;
            RCOM.txbuf.subcommand = 0x00;
            RCOM.txbuf.length = 66;
            RCOM.txbuf.data[0] = req[0];
            RCOM.txbuf.data[1] = req[1];

            i = (uint16_t)(req[0] + (uint16_t)req[1] * 256 - 1);

            //if((r500.next_rpt != 0xff) && (bs < (i + (320 / 64)))) repeater_is_engaged = false;   // 中継機宛で転送時間が残り約１．２５秒未満なら無線ＬＥＤ点滅期間を終了する

            memcpy((char *)&RCOM.txbuf.data[2], (char *)&buf[64 * i], 64);      // 追加ブロック内容６４バイトをコピー

            /* rfm_send() に内包 2020.Jul.03
            if(0 == Rf_Sum_Crc){
                calc_checksum(&RCOM);       //SUM計算、SUM付加
            }
            else{
                calc_soh_crc(&RCOM);        //CRC計算、CRC付加
            }
            */
            rfm_send(&RCOM.txbuf.header);        //無線モジュール SOH/STXフレーム送信（SUM/CRC自動付加）

            if(((r500.rf_cmd2 & 0x7f) == 0x66) && (r500.next_rpt == 0xff)){
                return(RFM_NORMAL_END);  // グループ一斉記録開始コマンドで中継機無しの場合、応答を待たないでリターン
            }

            recv_0x33response:;

            // ＤＥＢＵＧ
            //if(rfm_recv(3200) == RFM_SERIAL_TOUT) return(RFM_SERIAL_TOUT);
            if(rfm_recv(6000) == RFM_SERIAL_TOUT){
                return(RFM_SERIAL_TOUT);
            }

            if((RCOM.rxbuf.command == 0x34) && (RCOM.rxbuf.length > 4)){
                relay_rssi.level_0x34 = RCOM.rxbuf.data[4]; // RSSIレベルを保存する
            }

            if(RCOM.rxbuf.subcommand == CODE_BUSY)
            {
                if(busy++ > 20){
                    return(RFM_R500_BUSY);          // Ｒ５００は２秒ごと最大約１０回、ｂｕｓｙ出す
                }
                if(busy++ >= 10)
                {
                    __NOP();
                }

                //if((error_cnt++) >= error_max) return(RFM_R500_BUSY);

                if(my_rpt_number > 0){
                    repeater_is_engaged = false;
                }
                goto recv_0x33response;                         // Ｂｕｓｙ応答、０ｘ３３コマンド再送しないこと
            }

            if((RCOM.rxbuf.subcommand == CODE_NAK) && (RCOM.rxbuf.command == 0x34) && (RCOM.rxbuf.data[0] == 0xff) && (RCOM.rxbuf.data[1] == 0xff)){

				if(r500.next_rpt == 0xff) return(RFM_KOKI_ERROR);	// 子機へ無線コマンドを送った場合（子機がRTR601の場合、子機への追加データ転送エラーは RFM_RPT_ERROR で返ってくる、その他の子機の場合は、子機宛てコマンドが RFM_RPT_ERROR で返ってくることはない）// 2022.06.09

				return(RFM_RPT_ERROR);  // 親機～中継機間エラー
			}

            if(RCOM.rxbuf.subcommand == CODE_CH_BUSY){
                return(RFM_R500_CH_BUSY);
            }
            if(RCOM.rxbuf.subcommand != CODE_ACK){
                return(RFM_R500_NAK);
            }

            if((RCOM.rxbuf.command == 0x34) && (RCOM.rxbuf.data[0] == 0xff) && (RCOM.rxbuf.data[1] == 0xff)){
                break; // Ｒ５００からの要求ブロック数が０ｘｆｆｆｆなら終わり
            }

            req[0] = RCOM.rxbuf.data[0];                        // 要求されたデータ番号ラッチ
            req[1] = RCOM.rxbuf.data[1];
        }

        return(RFM_NORMAL_END);
    }

    return(RFM_SERIAL_ERR);
}


/**
 * R500子機からのリアルスキャン回答待ち
 * ＢＵＳＹでもＮＡＫでも再受信する
 * @return  結果
 */
static uint8_t R500_koki_real_scan_reply(void)
{
    uint8_t rtn;

    int i, j;


    r500.data_size = 0;

    huge_buffer[0] = 0;                                         // 子機データ総バイト数
    huge_buffer[1] = 0;

    for(j = 2, i = 0; i < (r500.para2[0] + 1); i++)
    {
        if(timer.int1000 > RF_buff.rf_req.timeout_time){
            return(RFM_DEADLINE);   //  無線コマンド終了の締め切り時間切れ
        }

        if(RF_buff.rf_req.cancel == 1U){
            return(RFM_CANCEL);     // 途中キャンセル要求があった場合
        }

        rtn = rfm_recv(3200);                                   // R500の応答受信（コマンド０ｘ３３、ＡＣＫ／ＢＵＳＹ、６６バイト）

        if((RCOM.rxbuf.command == 0x34) && (rtn == RFM_R500_CH_BUSY)){
            return(rtn);  // チャンネルビジーならリターン
        }

        if(rtn == RFM_SERIAL_TOUT){                             // sakaguchi 2021.02.02
            return(rtn);             // タイムアウト以外を読む
        }
        if((RCOM.rxbuf.command == 0x28) || (RCOM.rxbuf.command == 0x68))
        {
            //if(i == 0) continue;                              // 初回は同時応答なので読み捨て
            if(RCOM.rxbuf.data[0] == 1)                         // 初回は同時応答なので読み捨て（i == 0のとき不正データ受信する場合がある、ブロック番号１のとき初回ブロックと判定する）
            {
                i = 0;
                j = 2;
                continue;
            }

            //if(rtn == RFM_SERIAL_TOUT){  // sakaguchi 2021.02.02 del
                //return(rtn);             // タイムアウト以外を読む
            //}

            if(j > (int)(sizeof(huge_buffer) - 30)){
                break;
            }

            memset(&huge_buffer[j], 0x00, 30);                  // クリア３０バイト

            if(RCOM.rxbuf.subcommand == CODE_ACK)               // 応答しない子機は飛ばす 先頭２バイトは子機データ総バイト数（中継機情報を除く）
            {
                memcpy((char*)&huge_buffer[j], (char*)&RCOM.rxbuf.data[1], 30);   // 個別子機データ格納

                // ＤＥＢＵＧ用
                //if(j >= 90) huge_buffer[j] = j / 30 + 1;

                j += 30;
            }

            //property.acquire_number = j;                      // 受信したデータバイト数（総バイト数エリア２バイト含む）
        }
    }

    // ここで先頭バイトセットするので、中継機なしの場合は、受信済みデータの順次転送できない
    huge_buffer[0] = (uint8_t)(j - 2);                            // 子機データ総バイト数
    huge_buffer[1] = (uint8_t)((j - 2) / 256);

    // 注意：ここでは r500.para1.rpt_cnt = 0 となっているので、本機が親機の場合は r500.para1.rpt_cnt = 1 として処理する
    i = (my_rpt_number == 0) ? 1 : 0;

    memset((char*)&huge_buffer[j], 0x00, (size_t)(i * 2));                     // 中継機の電池残量、電波強度の領域をクリア

    j += (i * 2);                                                           // 中継機の電池残量、電波強度の領域確保
    if(i > 0){
        huge_buffer[j - 1] = (uint16_t)BAT_level_0to5((uint16_t)regf_battery_voltage);    // 領域確保された場合だけ電池残レベル格納
    }

    r500.data_size = (uint16_t)j;                                         // 受信したデータバイト数（総バイト数エリア２バイト含む）
    property.acquire_number = (uint16_t)j;

    return(RFM_NORMAL_END);
}




/**
 * @brief   R500子機と中継機からの一斉検索回答待ち
 * @return  結果
 * @note    ＢＵＳＹでもＮＡＫでも再受信する
 * @note    2020.Feb.03  修正
 */
static uint8_t R500_koki_whole_scan_reply(void)
{
	uint8_t rtn = RFM_NORMAL_END;
	int i, j, k, n;


	r500.data_size = 0;

	huge_buffer[0] = 0;											// 子機・中継機データ総バイト数
	huge_buffer[1] = 0;
	
	huge_buffer[2] = 0;											// 子機データ総バイト数
	huge_buffer[3] = 0;

	for(n = 0, k = 0, j = 4, i = 0; i < (r500.para2[0] + 1 + r500.route.rpt_max + 1); i++)	// 初回は同時応答なので＋１、親機も応答するので更に＋１
	{
		if(timer.int1000 > RF_buff.rf_req.timeout_time){
//		    rtn = RFM_DEADLINE; //  無線コマンド終了の締め切り時間切れ
//		    break;
		    return(RFM_DEADLINE);	// 	無線コマンド終了の締め切り時間切れ
		}

		if(RF_buff.rf_req.cancel == 1U){
//		    rtn = RFM_CANCEL;      // 途中キャンセル要求があった場合
//		    break;
		    return(RFM_CANCEL);		// 途中キャンセル要求があった場合
		}

		rtn = rfm_recv(3200);									// R500の応答受信（コマンド０ｘ３３、ＡＣＫ／ＢＵＳＹ、６６バイト）
		
		if((RCOM.rxbuf.command == 0x34) && (rtn == RFM_R500_CH_BUSY))
//		    break;
		    return(rtn);	// チャンネルビジーならリターン
		
		if(rtn == RFM_SERIAL_TOUT){
//		    break;
		    return(rtn);					// タイムアウト以外を読む
		}

		if(RCOM.rxbuf.command == 0x68)
		{
			if((r500.data_size == 0) && (RCOM.rxbuf.data[0] == 1)){
			    continue;				// 初回は同時応答なので読み捨て（i == 0のとき不正データ受信する場合がある、ブロック番号１のとき初回ブロックと判定する）
			}
			
			if(j > (int)(sizeof(huge_buffer) - 30)){
			    break;
			}

			memset(&huge_buffer[j], 0x00, 30);					// クリア３０バイト

			if(RCOM.rxbuf.subcommand == CODE_ACK)				// 応答しない子機は飛ばす 先頭２バイトは子機データ総バイト数（中継機情報を除く）
			{
				memcpy((char *)&huge_buffer[j], (char *)&RCOM.rxbuf.data[1], 30);	// 個別子機データ格納

				j += 30;
			}

//			if(i == r500.para2[0])								// 指定した子機最大数までループした
			if((r500.data_size == 0) && (i >= r500.para2[0]))	// 指定した子機最大数までループした
			{
				r500.data_size = 4;
				
				// ここで先頭バイトセットする
				huge_buffer[2] = (uint8_t)(j - 4);				// 子機データ総バイト数
				huge_buffer[3] = (uint8_t)((j - 4) / 256);
			
				// これ以降は中継機データである。初回、親機データが入るがＮＡＫ応答の場合、内容は子機最終と同じ（子機のデータに見えるので注意） 
				k = j;
				huge_buffer[k + 0] = 0;							// 中継機データ総バイト数
				huge_buffer[k + 1] = 0;

				j += 2;
				n = j;
			}
		}
	}

	huge_buffer[0] = (uint8_t)(j - 2);							// 子機・中継機データ総バイト数
	huge_buffer[1] = (uint8_t)((j - 2) / 256);

        huge_buffer[k + 0] = (uint8_t)(j - n);						// 中継機データ総バイト数
	huge_buffer[k + 1] = (uint8_t)((j - n) / 256);

        r500.data_size = (uint16_t)j;											// 受信したデータバイト数（総バイト数エリア２バイト含む）
        property.acquire_number = (uint16_t)j;

	return(RFM_NORMAL_END);
}



/**
 * R500子機からの現在値回答待ち
 * @return  結果
 * @note    ＢＵＳＹなら再受信する
 * @note    2020.Feb.03  修正
 */
static uint8_t R500_koki_present_data_reply(void)
{
    uint8_t rtn = RFM_NORMAL_END;


    while(1U)
    {
        if(timer.int1000 > RF_buff.rf_req.timeout_time){
            rtn = RFM_DEADLINE;   //  無線コマンド終了の締め切り時間切れ
            break;
//            return(RFM_DEADLINE);   //  無線コマンド終了の締め切り時間切れ
        }

        if(RF_buff.rf_req.cancel == 1U){
            rtn = RFM_CANCEL;     // 途中キャンセル要求があった場合
            break;
//            return(RFM_CANCEL);     // 途中キャンセル要求があった場合
        }

        rtn = rfm_recv(3200);                                   // R500の応答受信（コマンド０ｘ３３、ＡＣＫ／ＢＵＳＹ、６６バイト）

        //Printf("R500_koki_present_data_reply 1 %d \r\n" ,rtn);

        //if(RCOM.rxbuf.subcommand == RFM_R500_BUSY){
		if(rtn == RFM_R500_BUSY){				// 2020/07/10 修正 segi
            continue;
        }
        //if(RCOM.rxbuf.subcommand == RFM_LOW_BATT){
		if(rtn == RFM_LOW_BATT){				// 2020/07/10 修正 segi
            break;
 //           return(rtn);
        }

        if((RCOM.rxbuf.command == 0x2c) && (rtn == RFM_R500_NAK)){
            rtn = RFM_KOKI_ERROR;
            break;
//            return(RFM_KOKI_ERROR);
        }
        if((RCOM.rxbuf.command == 0x6c) && (rtn == RFM_R500_NAK)){
            rtn = RFM_KOKI_ERROR;
            break;
//            return(RFM_KOKI_ERROR);
        }

        if(rtn != RFM_NORMAL_END){
            break;
//            return(rtn);
        }

        //Printf("R500_koki_present_data_reply 2 %d \r\n" ,rtn);

        if((RCOM.rxbuf.command == 0x2c) || (RCOM.rxbuf.command == 0x6c))
        {
            r500.data_size = (uint16_t)(RCOM.rxbuf.length - 1);
            memset(&huge_buffer[10], 0xee, 20);       ///< ? データ部を無効データで埋めておく @bug 0xeeee -> 0xee
            memcpy((char *)&huge_buffer[2], (char *)&RCOM.rxbuf.data[1], r500.data_size);

            huge_buffer[0] = (uint8_t)r500.data_size;           // 先頭２バイトに子機情報のバイト数を記述
            huge_buffer[1] = (uint8_t)(r500.data_size >> 8);
            r500.data_size = (uint16_t) (r500.data_size + r500.data_size + 2);

            //Printf("R500_koki_present_data_reply 3 %d \r\n" ,rtn);
            rtn = RFM_NORMAL_END;
            break;
        }
    }

    //Printf("R500_koki_present_data_reply end %d \r\n" ,rtn);

    return(rtn);
}



/**
 *  R500子機からの設定値回答待ち
 * ＳＲＡＭに記録する／しない
 * @return  結果
 * @note    ＢＵＳＹなら再受信する
 * @note    emporary[]には吸い上げデータ格納しない
 */
static uint8_t R500_koki_setting_data_reply(void)
{
    uint8_t i, at, rtn;
    uint8_t order;

    while(1U)                                                   // コマンド０ｘ４２、０ｘ００、１６バイト待ち
    {
    	if(timer.int1000 > RF_buff.rf_req.timeout_time){
    		return(RFM_DEADLINE);   //  無線コマンド終了の締め切り時間切れ
    	}
    		
        if(RF_buff.rf_req.cancel == 1U){
            return(RFM_CANCEL);     // 途中キャンセル要求があった場合
        }
        rtn = rfm_recv(3200);                                   // R500の応答受信
        //Printf("R500_koki_setting_data_reply 1 %d %02X %02X\r\n" ,rtn, RCOM.rxbuf.command, RCOM.rxbuf.subcommand);

        //if(RCOM.rxbuf.subcommand == RFM_R500_BUSY){
		if(rtn == RFM_R500_BUSY){				// 2020/07/10 修正 segi
            continue;
        }
        //if(RCOM.rxbuf.subcommand == RFM_LOW_BATT){
		if(rtn == RFM_LOW_BATT){				// 2020/07/10 修正 segi
            return(rtn);
        }

        memcpy((char *)&r500.soh, (char *)&RCOM.rxbuf.header, sizeof(r500) - 1);    // ０ｘ４２応答受信をｒ５００構造体にコピー（転送データバイト数ラッチ）

        if(RCOM.rxbuf.command == 0x42)
        {
            if(RCOM.rxbuf.subcommand == CODE_NAK){
                return(RFM_KOKI_ERROR);
            }

			if(rtn != RFM_NORMAL_END) return(rtn);      // 2023.02.20 異常時の判定追加

            if((r500.next_rpt == my_rpt_number) && (r500.my_rpt == 0xff)){
                break;
            }
        }

        if(rtn != RFM_NORMAL_END){
            return(rtn);
        }
    }

    //Printf("R500_koki_setting_data_reply 2 %d / size %d / timeout %d (%ld)\r\n" ,rtn, r500.data_size, RF_buff.rf_req.timeout_time, timer.int1000);

    i = 0;
    at = (uint8_t)(r500.data_size / 64);

    while(at > i)                                               // コマンド０ｘ３３、ＡＣＫ、６６バイト待ち
    {
        if(timer.int1000 > RF_buff.rf_req.timeout_time){
            Printf("R500_koki_setting_data_reply timeout\r\n");
            return(RFM_DEADLINE);   //  無線コマンド終了の締め切り時間切れ
        }

        if(RF_buff.rf_req.cancel == 1U){
            return(RFM_CANCEL);     // 途中キャンセル要求があった場合
        }

        rtn = rfm_recv(4000);                                   // R500の応答受信

        //Printf("R500_koki_setting_data_reply 3 %d at=%d i=%d\r\n" ,rtn, at, i);

        if(rtn == RFM_R500_BUSY){
            continue;
        }
        if(rtn != RFM_NORMAL_END){
            return(rtn);
        }

        //Printf("R500_koki_setting_data_reply 4 %d at=%d i=%d\r\n" ,rtn, at, i);
        if(RCOM.rxbuf.command == 0x33)
        {

        	order = r500.rf_cmd2 & 0x7f;
        	if((order == 0x22)||(order == 0x2b)){
        	}
        	else{
	        	if(i == 0){
	                memcpy((char *)ru_header.intt, (char *)&RCOM.rxbuf.data[2], 64);    // ヘッダファイルコピー 応答フレームを格納（２バイト減）
	                extract_property();                                 // 子機ヘッダからプロパティ情報を読み取る

	                at = (uint8_t)((ru_header.ul_protect >> 4) + 1);               // 追加ヘッダブロック
	            }
        	}

            memcpy((char *)&huge_buffer[sram.record], (char *)&RCOM.rxbuf.data[2], 64); // ヘッダファイルコピー 応答フレームを格納（２バイト減）
            sram.record = (uint16_t)(sram.record + 64);

            i++;
        }

        //Printf("R500_koki_setting_data_reply 5 %d at=%d i=%d\r\n" ,rtn, at, i);
    }

    //Printf("## R500_koki_setting_data_reply end %d \r\n" ,rtn);

    return(RFM_NORMAL_END);
}



/**
 *  @brief  R500子機からの吸い上げ回答待ち
 *
 * ＢＵＳＹなら再受信する r500.data_size には吸い上げするバイト数が入る
 *   huge_bufferに吸い上げデータ格納する
 * @return  結果
 *
 */
static uint8_t R500_koki_upload_data_reply(void)
{
    uint8_t rtn = RFM_NORMAL_END;

    int i, clen, remain;


    i = (uint16_t)(64 * ((ru_header.ul_protect >> 4) + 1));         // ここまでの受信数（設定テーブル６４ｂｙｔｅ、追加分も受信済み）

    // RTR-601では、設定テーブル64byte＋吸い上げデータバイト数＋テーブル情報16byte×3
    //remain = ru_header.data_byte[0] + (ushort)ru_header.data_byte[1] * 256;   // 吸い上げデータバイト数＋ヘッダ６４バイト（有効バイト数をセット）
    remain = (uint16_t)(r500.data_size - i);


	while(i < r500.data_size)
	{
		if(timer.int1000 > RF_buff.rf_req.timeout_time){
		    rtn = RFM_DEADLINE;   //  無線コマンド終了の締め切り時間切れ
		    break;
//		    return(RFM_DEADLINE);   //  無線コマンド終了の締め切り時間切れ
		}

        if(RF_buff.rf_req.cancel == 1U){
            rtn = RFM_CANCEL;     // 途中キャンセル要求があった場合
            break;
//            return(RFM_CANCEL);     // 途中キャンセル要求があった場合
        }

        rtn = rfm_recv(4000);                                   // R500の応答受信（コマンド０ｘ３３、ＡＣＫ／ＢＵＳＹ、６４バイト）

//		if(rtn == 0xf0){
//			rtn = 0;
//		}

        if(rtn == RFM_R500_BUSY){
            continue;
        }

        if(rtn == RFM_R500_NAK){                                // NAK応答は子機通信エラーとする    // sakaguchi 2021.06.15
            rtn = RFM_KOKI_ERROR;
        }

        if(rtn != RFM_NORMAL_END){
            break;
 //           return(rtn);
        }

        if(RCOM.rxbuf.command == 0x33)                          // 送信間隔２３０ｍｓくらい
        {
            clen = (remain > 64) ? 64 : remain;

            memcpy((char *)&huge_buffer[sram.record], (char *)&RCOM.rxbuf.data[2], (size_t)clen);   // 応答フレーム（２バイト減）ＳＲＡＭストア

            sram.record = (uint16_t)(sram.record + clen);
            remain -= clen;

			i += clen;

            property.acquire_number = (uint16_t)i;                        // 受信したデータバイト数（ヘッダ含む）
        }
    }


	return(rtn);
}


/**
 * @brief    R500子機からの記録停止回答待ち
 * @return  結果
 * @note    ＢＵＳＹなら再受信する
 */
static uint8_t R500_koki_record_stop_reply(void)
{
    uint8_t rtn;


    while(1U)
    {
        if(timer.int1000 > RF_buff.rf_req.timeout_time){
            return(RFM_DEADLINE);   //  無線コマンド終了の締め切り時間切れ
        }

        if(RF_buff.rf_req.cancel == 1U){
            return(RFM_CANCEL);     // 途中キャンセル要求があった場合
        }

        rtn = rfm_recv(4000);                                   // R500の応答受信（コマンド０ｘ３４、ＡＣＫ／ＢＵＳＹ）

        if(rtn == RFM_R500_BUSY){
            continue;
        }
        if(rtn != RFM_NORMAL_END){
            return(rtn);
        }

        if(RCOM.rxbuf.command == 0x34)                          // 送信間隔２３０ｍｓくらい
        {
            if((RCOM.rxbuf.data[2] == CODE_NAK) && (RCOM.rxbuf.data[3] == 0xff)){
                return(RFM_RU_PROTECT);    // 記録開始コマンドでプロテクトエラーの時
            }
            if((RCOM.rxbuf.data[2] == CODE_ACK) && (RCOM.rxbuf.data[3] == 0xff)){
                return(RFM_NORMAL_END);    // 正常終了
            }
        }
    }
}


/**
 * R500子機からの記録開始回答待ち
 * @param order コマンド
 * @return  結果
 * @note    ＢＵＳＹなら再受信する
 */
static uint8_t R500_koki_record_start_reply(uint8_t order)
{
    if(RCOM.rxbuf.command == 0x34)                              // 送信間隔２３０ｍｓくらい
    {
        if((RCOM.rxbuf.data[2] == CODE_NAK) && (RCOM.rxbuf.data[3] == 0xff)){

			if(0x25 == order){				// BLE設定Writeの場合
				return(RFM_PASS_ERR);    	// パスコード不正エラー
			}else{
	            return(RFM_RU_PROTECT);    // 記録開始コマンドでプロテクトエラーの時
	        }
        }
        if((RCOM.rxbuf.data[2] == CODE_ACK) && (RCOM.rxbuf.data[3] == 0xff)){
            return(RFM_NORMAL_END);    // 正常終了
        }
    }

    return(RFM_PRTCOL_ERR);
}



/**
 * R500中継機からの回答待ち
 * @param order コマンド
 * @return  結果
 */
static uint8_t R500_relay_reply(uint8_t order)
{
    SSP_PARAMETER_NOT_USED(order);       //未使用引数

    uint8_t rtn, com, btl, *hbuf;

    int i, ds, remain, clen;


    // 中継機からの応答受信（ＢＵＳＹ信号ＯＮ直後の受信）

    while(1U)
    {
        repeater_is_engaged = false;

        if(timer.int1000 > RF_buff.rf_req.timeout_time){
            return(RFM_DEADLINE);   //  無線コマンド終了の締め切り時間切れ
        }

        if(RF_buff.rf_req.cancel == 1U){
            return(RFM_CANCEL);     // 途中キャンセル要求があった場合
        }

        rtn = rfm_recv(PERMANENT_WAIT);                         // シリアル受信のタイムアウトは無し

        if(rtn == RFM_DEADLINE){
            return(RFM_DEADLINE);           // 無線コマンド終了の締め切り時間切れならエラー終了
        }
        if(rtn != RFM_NORMAL_END){
			tx_thread_sleep(20);	// 200ms delay 2020 05 07  @ToDo debug
            continue;
        }

        memcpy((char *)&r500.soh, (char *)&RCOM.rxbuf.header, sizeof(r500) - 1);    // 応答受信をｒ５００構造体にコピー
        timer.int125 = (uint16_t)(r500.time_cnt + RfDelay_Rpt);                 // 子機到達から親機コマンドが終了するまでのカウント開始

        if((r500.cmd1 == 0x42) && (r500.cmd2 == 0x00) && (r500.next_rpt == my_rpt_number))
        {
            if((r500.rf_cmd2 & 0x7f) == cmd2_OUKA1)             // 親機がキャンセルお伺いコマンドを受信した時の処理
            {
                rxOukagai_M();                                  // 中継機情報を受信、キャンセルお伺いコマンドに対してＯＫ応答を送信
                continue;
            }

            break;                                              // 自局宛、無線応答コマンドを検出
        }
    }

    if(r500.data_size == 0){
        return(RFM_PRTCOL_ERR);
    }

    if((r500.para2[0] != 0x0e) && (r500.para2[0] != 0x0f))      // 中継機 受信履歴読み込みとクリアコマンド以外
    {
        if((r500.rf_cmd2 & 0x7f) == cmd2_END_VARIOUS)
        {
            rtn = R500_relay_error_investigate(r500.para2[0]);  // 中継機エラー、エラー調査

            if(r500.data_size < 1024)                           // 中継機エラーなので中継情報のみ想定
            {
                rx500_relay_comm_recv((uint8_t *)&RCOM.txbuf.command);     // 追加データも全部受信する（huge_bufferを壊さないこと）
                //osDelay(110);                                 // 電波を出して中継のＡＣＫ通信を阻害する 110msec delay
                tx_thread_sleep (11);                           // 電波を出して中継のＡＣＫ通信を阻害する 110msec delay
                TX_dummy();
            }
            return(rtn);
        }
    }

//-----cmd2の応答がEND_OTHER_ERR(0x0Cだった場合の処理を追加した)-----------------------------
    if((r500.rf_cmd2 & 0x7f) == END_OTHER_ERR)
    {
        Printf("## END_OTHER_ERR\r\n");
//#ifndef DBG_TERM


        rtn = RFM_RPT_ERROR;  								// 中継機エラー
        if(r500.data_size < 1024)                           // 中継機エラーなので中継情報のみ想定
        {
            rx500_relay_comm_recv((uint8_t *)&RCOM.txbuf.command);     // 追加データも全部受信する（huge_bufferを壊さないこと）
            //osDelay(110);                                 // 電波を出して中継のＡＣＫ通信を阻害する 110msec delay
            tx_thread_sleep (11);                           // 電波を出して中継のＡＣＫ通信を阻害する 110msec delay
            TX_dummy();
        }
        return(rtn);
    
//memset((char *)&RCOM.rxbuf.data[2], 0x05, 64);     	// データを適当な値にしてしまう debug
//#endif

    }
    
//--------------------------------------------------------------------------------------------

    // 中継機から回答データを取得

    repeater_is_engaged = true;                                 // 自局宛の無線通信中
    wait.inproc_rfm = WAIT_1SEC + WAIT_200MSEC;                 // 最低、１秒間の無線ＬＥＤ点灯を確保する

    timer.int1000 = 0;                                          // 中継機から送信があったのでタイムアウトクリア
    RF_buff.rf_req.timeout_time = 180;                          // 最近中継機からの受信は１８０秒以内に終わること

    i = 0;

    ds = remain = r500.data_size;
    r500.data_size = (uint16_t)(r500.data_size - r500.para1.rpt_cnt);                       // 付加されているリピータ情報バイト分を差し引く

    com = (r500.rf_cmd2 & 0x7f);                                // リアルスキャンコマンドを判別する
    btl = (uint16_t)BAT_level_0to5((uint16_t)regf_battery_voltage);                 // 電池残レベルラッチ
    hbuf = (uint8_t *)&huge_buffer[r500.data_size - 1];                    // 電池残レベル格納番地

    while(i < ds)   // r500.data_sizeはブロック長を除いた、中継機情報（１台１バイト）を含めた転送要求バイト数
    {
        if(timer.int1000 > RF_buff.rf_req.timeout_time){
            return(RFM_DEADLINE);   //  無線コマンド終了の締め切り時間切れ
        }

        if(RF_buff.rf_req.cancel == 1U){
            return(RFM_CANCEL);     // 途中キャンセル要求があった場合
        }

        // ＤＥＢＵＧ １．０５
        //rtn = rfm_recv(3200);                                 // R500の応答受信（コマンド０ｘ３３、ＡＣＫ／ＢＵＳＹ、６６バイト）
        rtn = rfm_recv(6000);                                   // R500の応答受信（コマンド０ｘ３３、ＡＣＫ／ＢＵＳＹ、６６バイト）

        if(rtn == RFM_R500_BUSY){
            continue;
        }
        if(rtn != RFM_NORMAL_END){
            return(rtn);
        }

        if(RCOM.rxbuf.command == 0x33)                          // ０ｘ３３以外のコマンドは読み捨てる
        {
            clen = (remain > 64) ? 64 : remain;

            if(clen > 0)
            {
                if(sram.record > (sizeof(huge_buffer) - 64)){
                    break; // 受信数制限
                }

                memcpy((char *)&huge_buffer[sram.record], (char *)&RCOM.rxbuf.data[2], (size_t)clen);   // ブロック番号２バイトを除いて付け加えコピー

                if((com == END_OK_rpt) && (r500.node == COMPRESS_ALGORITHM_NOTICE) && (sram.record == 0))   // リアルスキャンの圧縮データのとき、RTR-500Cが中継できるように加工した圧縮データヘッダを元に直す
                {
                    RCOM.rxbuf.length = (uint16_t)(huge_buffer[0] + ((uint32_t)huge_buffer[1] << 8) + 2);   // 圧縮データバイト数を圧縮ヘッダ含むバイト数に修復
                    huge_buffer[0] = huge_buffer[2];                                            // 圧縮情報を先頭２バイトに移動する
                    huge_buffer[1] = huge_buffer[3];
                    huge_buffer[2] = (uint8_t)(RCOM.rxbuf.length >> 0); // データバイト数を先頭に移動する
                    huge_buffer[3] = (uint8_t)(RCOM.rxbuf.length >> 8);
                }

                sram.record = (uint16_t)(sram.record + clen);
                remain -= clen;
            }

            if(com == END_OK_rpt){
                *hbuf = btl;                  // リアルスキャンコマンドなら本機の電池残セット
            }

            if(sram.record > r500.data_size){
                property.acquire_number = r500.data_size;
            }
            else{
                property.acquire_number = sram.record;
            }

            i += clen;
        }
    }

    property.acquire_number = r500.data_size;                   // 有効バイト数をセット

    rf_delay(110);                                               // 電波を出して中継のＡＣＫ通信を阻害する
    TX_dummy();

    return(RFM_NORMAL_END);
}



/**
 * R500中継機からのリピータスキャン回答待ち
 * @param order コマンド
 * @return      結果
 * @note    ＢＵＳＹでもＮＡＫでも再受信する
 */
static uint8_t R500_relay_rf_scan_reply(uint8_t order)
{
    uint8_t rtn;

    int i, j;


    r500.data_size = 0;
    huge_buffer[0] = 0xfb;

    for(j = 0, i = 0; i < (r500.para2[0] + 1); i++)
    {
        if(timer.int1000 > RF_buff.rf_req.timeout_time){
            return(RFM_DEADLINE);   //  無線コマンド終了の締め切り時間切れ
        }

        if(RF_buff.rf_req.cancel == 1U){
            return(RFM_CANCEL);     // 途中キャンセル要求があった場合
        }

        rtn = rfm_recv(3200);                                   // R500の応答受信（コマンド０ｘ３３、ＡＣＫ／ＢＵＳＹ、６６バイト）
        if(rtn == RFM_LOW_BATT){
            return(rtn);
        }

        if((RCOM.rxbuf.command == 0x34) && (rtn == RFM_R500_CH_BUSY)){
            return(rtn);  // チャンネルビジーならリターン
        }

        if(((order == 0x2d) || (order == 0x66)) && (i == 0)){
            continue;  // 子機検索、一斉記録開始コマンドは同時応答データ破棄（最初の応答を破棄）
        }

        if((RCOM.rxbuf.command == order) && (RCOM.rxbuf.subcommand == CODE_ACK))
        {
            if(rtn == RFM_SERIAL_TOUT){
                return(rtn);             // タイムアウト以外を読む
            }

            if(huge_buffer[0] == RCOM.rxbuf.data[1]){
                continue;  // ２回目以降の受信が初回の子機情報と相違すること（huge_buffer[0]には本関数始まりで存在しない子機番号０を設定済み）
            }

            if(j > (int)(sizeof(huge_buffer) - 19)){
                break;
            }

            memcpy((char *)&huge_buffer[j], (char *)&RCOM.rxbuf.data[1], 19);   // 個別中継機データ格納

            j += 19;
            property.acquire_number = (uint16_t)j;                        // 受信したデータバイト数
        }
    }

    r500.data_size = (uint16_t)j;
    property.acquire_number = (uint16_t)j;

    return(RFM_NORMAL_END);
}



/**
 * R500エラーをＴコマンドエラーに置き換える
 * @param err   R500エラーコード
 * @return      置き換えＴコマンドエラーコード
 */
static uint32_t R500_replace_error_code(uint8_t err)
{
    uint32_t rtn;


    switch(err)
    {
        case RFM_NORMAL_END:                                    // 正常終了
            rtn = ERR(RF, NOERROR);
            break;

        case RFM_SERIAL_TOUT:                                   // シリアル通信のタイムアウト
            rtn = ERR(CMD, TIMEOUT);
            break;

        case RFM_DEADLINE:                                      // コマンド終了の締め切り時間切れ
            rtn = ERR(RF, ToTERR);
            break;

        case RFM_CANCEL:                                        // キャンセル
            rtn = ERR(RF, CANCEL);
            break;

        case RFM_R500_BUSY:                                     // R500無線モジュールがBUSY応答の場合
            //rtn = ERR(RF, BUSY);
			rtn = ERR(RF, OTHER_ERR);		// 2020/07/10 修正 segi
            break;

        case RFM_R500_NAK:                                      // R500無線モジュールがNAK応答の場合
            rtn = ERR(RF, OTHER_ERR);
            break;

        case RFM_R500_CH_BUSY:                                  // R500無線モジュールがチャンネルビジー応答の場合
            rtn = ERR(RF, INUSE);
            break;

        case RFM_PRTCOL_ERR:                                    // R500無線モジュール通信で不正プロトコルであった
            rtn = ERR(RF, OTHER_ERR);
            break;

        case RFM_SERIAL_ERR:                                    // R500無線モジュールとのシリアル通信エラー
            rtn = ERR(RF, OTHER_ERR);
            break;

        case RFM_RU_PROTECT:                                    // 子機にプロテクトかかっていて記録開始できない
            rtn = ERR(RF, PROTECT);
            break;

        case RFM_RT_SHORTAGE:                                   // 記録開始までの秒数足りない
            rtn = ERR(RF, RECTIME);
            break;

        case RFM_REFUSE:                                        // コマンド処理が拒否された
            rtn = ERR(CMD, FORMAT);
            break;

        case RFM_KOKI_ERROR:                                    // 子機通信のエラー
            rtn = ERR(RF, UNIT_ERR);
            break;

        case RFM_RPT_ERROR:                                     // 中継機通信のエラー
            rtn = ERR(RF, RPT_ERR);
            break;

        case RFM_SRAM_OVER:                                     // ＳＲＡＭオーバー
            rtn = ERR(SYS, SRAM);
            break;

        case RFM_LOW_BATT:                                      // 電池電圧が低い
            rtn = ERR(SYS, LOWBATTERY);
            break;

		case RFM_PASS_ERR:										// パスコード不正エラー
			rtn = ERR(CMD, PASS);
			break;

        default:
            rtn = ERR(RF, OTHER_ERR);
            break;
    }

    return (rtn);
}



/**
 * R500中継機エラー時エラー調査
 * @param err   エラー情報
 * @return      調査結果
 */
static uint8_t R500_relay_error_investigate(uint8_t err)
{
    if(err == para2_LOG_GET)        return(RFM_RPT_ERROR);
    else if(err == para2_LOG_CLR)   return(RFM_RPT_ERROR);
    else if(err == para2_REC_T_ERR) return(RFM_RT_SHORTAGE);
    else if(err == para2_PRO)       return(RFM_RU_PROTECT);
    else if(err == para2_CANCEL)    return(RFM_CANCEL);
    else if(err == para2_RPT_ERR_H) return(RFM_RPT_ERROR);
    else if(err == para2_RPT_ERR_L) return(RFM_RPT_ERROR);
    else if(err == para2_KOKI_ERR)  return(RFM_KOKI_ERROR);
    else if(err == para2_BUSY)      return(RFM_R500_BUSY);      // Prototype3 2018_6_29
    else                            return(RFM_RPT_ERROR);
}


/**
 * @brief       親機がキャンセルお伺いコマンドを受信した時の処理
 *
 * 長く時間がかかる処理実行中、中継機が自発的に続行の許可を求める処理
 * @return
 */
static uint8_t rxOukagai_M(void)
{
    uint8_t rtn = RFM_CANCEL;

    int i = 0;

//    LO_HI a;


    while(i < r500.data_size)                                   // 中継機からの親機へのキャンセルお伺いコマンドの追加情報をすべて受信
    {
        if(timer.int1000 > RF_buff.rf_req.timeout_time){
            return(RFM_DEADLINE);   //  無線コマンド終了の締め切り時間切れ
        }

        if(RF_buff.rf_req.cancel == 1U){
            return(RFM_CANCEL);     // 途中キャンセル要求があった場合
        }

        rtn = rfm_recv(3200);                                   // R500の応答受信（コマンド０ｘ３３、ＡＣＫ／ＢＵＳＹ、６６バイト）

        if(rtn == RFM_R500_BUSY){
            continue;
        }
        if(rtn != RFM_NORMAL_END){
            return(rtn);
        }

        if(RCOM.rxbuf.command == 0x33)                          // ０ｘ３３以外のコマンドは読み捨てる
        {
            memcpy((char *)&rpt_sequence[i], (char *)&RCOM.rxbuf.data[2], 64);  // ブロック番号２バイトを除く
            i += 64;
            if(i > 255){
                i = 255;
            }
        }
    }

    //osDelay(110);
    tx_thread_sleep (11);
    TX_dummy();

    if(RF_buff.rf_req.cancel == 0U)                             // キャンセル指示有り？
    {
//        a.byte_lo = r500.para2[0];                              // お伺いサイズ
//        a.byte_hi = r500.para2[1];
//        calc_rf_timeout(a.word);                                // 無線タイムアウト時間を再設定する RF_buff.rf_req.timeout_timeにタイムアウト値が入る
        calc_rf_timeout(*(uint16_t *)&r500.para2[0] );         // 無線タイムアウト時間を再設定する RF_buff.rf_req.timeout_timeにタイムアウト値が入る
        // 親機へのキャンセルお伺いコマンドのr500.para2には転送データバイト数が入っている

        r500.end_rpt = rpt_sequence[0];                         // 最終宛先
        r500.route.start_rpt = my_rpt_number;                         // 自局番号がスタート
        r500.my_rpt = my_rpt_number;                            // 自局番号を設定

        rpt_data_reverse();                                     // 中継情報を反転する

        r500.next_rpt = rpt_sequence[1];
        r500.data_size = r500.para1.rpt_cnt;

        r500.rf_cmd1 = TxLoop_Rpt;                              // 中継機と通信する時の巡回数
        r500.rf_cmd2 = (char)((r500.rf_cmd2 & 0x80) | cmd2_OUKA2);      // 親機へのキャンセルお伺いコマンドの応答

        r500.node = 0;                                          // 中継機が受け取り先の場合は0

        memcpy(huge_buffer, rpt_sequence, r500.para1.rpt_cnt);

        rtn = tx500_com_loop((uint8_t *)huge_buffer);
    }

    return(rtn);
}


/**
 * @brief   リトライ回数計算
 *
 * R500から中継機又は親機宛てに無線コマンドを送信する場合に、追加データのブロック長を元に全体のリトライ回数を決定する。
 * 20ブロック以下はリトライ5回まで、20ブロック以上は+25%(2009.11.03修正)をリトライ回数とする
 * @param data_cnt  データブロック数
 * @return  リトライを含めたブロック回数制限値
 * @note    2020.Jan.16 引数、返り値をintに修正
 */
static int retray_size(int data_cnt)
{
    return ((data_cnt < 21) ? (data_cnt + 5) : (data_cnt + data_cnt / 4));

}



/**
 * @brief   １０００ｍｓタイマースタート
 */
static void Start1000msec(void)
{
    timer.int1000 = 0;
}



/**
 * @brief   １０００ｍｓタイマーストップ
 */
static void Stop1000msec(void)
{
    __NOP();
}


# if 0
//未使用 未完成 関数
/**
 * @brief   １２５ｍｓタイマースタート
 */
static void Start125msec(void)
{
    timer.int125 = 0;
    timer.latch_enable = false;
}


/**
 * @brief   １２５ｍｓタイマーストップ
 */
static void Stop125msec(void)
{
    __NOP();
}
#endif


/**
 * @brief   １２５ｍｓカウンタを秒に変換して返す
 * @param count カウンタ値
 * @return      秒
 */
static uint16_t Conv125msec(uint16_t count)
{
    return((uint16_t)((count + 1) / 8));
}





/**
 * @brief       R500からのコマンドフレーム(0x42)と追加データ、中継情報を受信する
 *
 * コマンドフレームの先頭をポインタで渡される。
 * 保存場所は、
 * コマンドフレーム        r500.soh～r500.sum
 * 追加データ、中継情報  sram.adr[32314]
 * @param buf       コマンドフレームの先頭ポインタ
 * @retval  0x00:   正常終了。
 * @retval  0x01:   0x42以外のコマンドだった。
 * @retval  0x02:   自局宛以外のコマンドだった。
 * @retval  0x03:   R500からのシリアル通信が途絶えた。
 * @retval  0x04:
 * @retval  0x05:
 * @retval  0x06:
 * @retval  0x07:
 * @retval  0xf0:   無線通信が失敗した。
 */
static uint8_t rx500_relay_comm_recv(uint8_t *buf)
{
    uint8_t rtn = 0;

    int remain, clen;
    int i = 0;


    remain = r500.data_size;

    while(i < r500.data_size)
    {
        if(RF_buff.rf_req.cancel == 1){
            rtn = RFM_CANCEL;        // 途中キャンセル要求があった場合
        }

        // ＤＥＢＵＧ １．０５
        //rtn = rfm_recv(4000);
        rtn = rfm_recv(6000);

        if(rtn == RFM_R500_BUSY){
            continue;
        }

        if(rtn != RFM_NORMAL_END)
        {
            r500.cmd2 = CODE_NAK;
            r500.data_size = 0;
            rtn = RFM_RPT_ERROR;
            break;
        }

        if(RCOM.rxbuf.command == 0x33)                          // ０ｘ３３以外のコマンドは読み捨てる
        {
            rtn = RFM_NORMAL_END;

            clen = (remain > 64) ? 64 : remain;

            if(clen > 0)
            {
                memcpy((char *)&buf[i], (char *)&RCOM.rxbuf.data[2], (size_t)clen);     // ブロック番号２バイトを除いて付け加えコピー
                remain -= clen;
            }

            i += clen;
        }
    }

    return (rtn);
}

/**
 * ダミー送信
 * @attention    r500構造体を使用しないこと（一時的に使用もＮＧ）
 */
static void TX_dummy(void)
{
    repeater_is_engaged = true;                                 // 電波出力中
    wait.inproc_rfm = WAIT_1SEC + WAIT_200MSEC;                 // 最低、１秒間の無線ＬＥＤ点灯を確保する

    memset(&r500_backup.soh, 0x00, sizeof(r500_backup) - 1);

    r500_backup.soh = CODE_SOH;
    r500_backup.cmd1 = 0x42;
    r500_backup.cmd2 = 0x00;
    r500_backup.len = 0x10;
    r500_backup.end_rpt = my_rpt_number;                        // 最終中継機番号
	r500_backup.route.start_rpt = my_rpt_number;				// 発信元番号    r500_backup.my_rpt = my_rpt_number;                         // 送信局番号
    r500_backup.next_rpt = my_rpt_number;                       // 次中継機番号
    r500_backup.rf_cmd1 = TxLoop_Rpt;                           // コマンド巡回数
    r500_backup.rf_cmd2 = 0x01;                                 // 無線コマンドは親機へのキャンセルお伺いコマンドにしておく
    r500_backup.node = 0xFF;                                    // 子機番号
    r500_backup.rssi = 0x00;
    /* rfm_send() に内包 2020.Jul.03
    calc_checksum_data(&r500_backup.soh);
    */
    rfm_send(&r500_backup.soh);        //無線モジュール SOH/STXフレーム送信（SUM/CRC自動付加）

    rfm_recv(200);
    rfm_recv(2000);
}



#if 0
//未使用
/**
 * 子機や中継機との無線通信結果により親機まで戻す無線コマンドを設定する
 * @param in_no 無線通信関数からの返り値
 * @return      r500.rf_cmd2に設定するコマンド(ショートカットフラグが無い状態)
 * @note    未使用
 */
static uint8_t RET_set_command(uint8_t in_no)
{
    uint8_t w_cmd2 = 0x00;
    uint8_t w_para2 = (r500.rf_cmd2 & 0x7f);


    switch(w_para2)
    {
        case 0x0e:
        case 0x0f:
            w_cmd2 = cmd2_END_VARIOUS;
            break;

        default:
            switch(in_no)
            {
                case RFM_RU_PROTECT:                            // 子機にプロテクトかかっていて記録開始できない状態
                    w_cmd2 = cmd2_END_VARIOUS;
                    w_para2 = para2_PRO;
                    break;

                case RFM_RT_SHORTAGE:                           // 記録開始までの秒数足りず
                    w_cmd2 = cmd2_END_VARIOUS;
                    w_para2 = para2_REC_T_ERR;
                    break;

                case RFM_RPT_ERROR:                             // 中継機と無線通信できなかった
                    w_cmd2 = cmd2_END_VARIOUS;
                    w_para2 = para2_RPT_ERR_H;
                    break;

                case RFM_KOKI_ERROR:                            // 子機と無線通信できなかった
                    w_cmd2 = cmd2_END_VARIOUS;
                    w_para2 = para2_KOKI_ERR;
                    break;

                case RFM_CANCEL:                                // 無線通信キャンセル
                    w_cmd2 = cmd2_END_VARIOUS;
                    w_para2 = para2_CANCEL;
                    break;

                case RFM_INUSE:                                 // チャンネル空き無し
				case RFM_R500_CH_BUSY:
                    w_cmd2 = cmd2_END_VARIOUS;
                    w_para2 = para2_INUSE;
                    break;

                case RFM_R500_BUSY:                             // 無線通信中 Prototype3 2018_6_29
                    w_cmd2 = cmd2_END_VARIOUS;
                    w_para2 = para2_BUSY;
                    break;

				/* エラー処理は RFM_INUSE
                case RFM_R500_CH_BUSY:
                    RF_buff.rf_res.status = BUSY_RF;
                    break;
*/
                case RFM_NORMAL_END:                            // 正常終了
					w_cmd2 = ((w_para2 == 0x30) || (w_para2 == 0x68))  ? END_OK_rpt : END_OK;       // リアルスキャンだけEND_OK_rptとして、復路で中継機の電池残量、電波強度を入れていく
					break;

                default:
                    w_cmd2 = END_OTHER_ERR;
                    break;
            }
    }

    if(w_cmd2 == cmd2_END_VARIOUS){
        memcpy(r500.para2, &w_para2, 2);
    }
    return (w_cmd2);
}
#endif





/**
 *
 * @param buf
 * @return
 * @attention   この関数でrpt_sequence引数としないように
 */
static uint8_t tx500_com_loop(uint8_t *buf)
{
    int i;
    uint8_t rtn, rpt = r500.next_rpt;
    uint32_t base_time = r500.time_cnt;

    for(i = 0; i < 3; i++)
    {
        repeater_is_engaged = true;                             // 無線通信中
        wait.inproc_rfm = WAIT_1SEC + WAIT_200MSEC;             // 最低、１秒間の無線ＬＥＤ点灯を確保する

        // 自機が中継機の場合
        if(my_rpt_number != 0)
        {
            RF_rpt_batt_set((uint8_t)(regf_rfm_version_number >> 16), BAT_level_0to5((uint16_t)regf_battery_voltage));  // 中継機の電池残量（５～０）を無線モジュールに書き込む（前回値と同じなら書き込みしない）
            rf_delay(10);
        }

        // 次の中継機を復元
        r500.next_rpt = rpt;

        // 中継機情報をrpt_sequenceへ再展開（注意！！ この関数でrpt_sequence引数としないように）
        if(r500.data_size >= r500.para1.rpt_cnt){
            memcpy(rpt_sequence, &buf[r500.data_size - r500.para1.rpt_cnt], r500.para1.rpt_cnt);
        }
        else
        {
            rtn = RFM_RPT_ERROR;                                // フォーマットエラー
            break;
        }

        // 初回の通信の場合のみショートカット有効にしている
        if((i == 0) && ((r500.rf_cmd2 & 0x80) == 0x00)){
            r500.next_rpt = (char)shortcut_info_redesignation();  // ショートカットで次の中継機を指定する(上位bit=0:ショートカット)
        }

        r500.time_cnt = (uint16_t)(base_time + timer.int125);

        //if((my_rpt_number != 0) && (r500.data_size < 320)) repeater_is_engaged = false;       // 転送時間が残り約１．２５秒未満なら無線ＬＥＤ点滅期間を終了する

        rtn = R500_send_recv(6000, buf);

        if((rtn == RFM_NORMAL_END) || (rtn == RFM_RU_PROTECT)){
            break;   // 無線プロテクト以外はリトライする
        }

        //if((rtn == RFM_NORMAL_END) || (rtn == RFM_RU_PROTECT) || (rtn == RFM_SERIAL_TOUT)) break; // 無線プロテクトかシリアルタイムアウト以外はリトライする

        //if((rtn == RFM_NORMAL_END) || ((rtn != RFM_R500_NAK) && (rtn != RFM_RPT_ERROR) && (rtn != RFM_R500_CH_BUSY))) break;  // リトライする条件 最初に応答無かった時・CH Busyの時

        /*
        if(((r500.rf_cmd2 & 0x7f) == cmd2_OUKA1) || ((r500.rf_cmd2 & 0x7f) == cmd2_OUKA2))  // キャンセルお伺いコマンドなら完全なリトライする（キャンセルお伺いコマンドはコマンドＩＤ不問）
        {
            if(rtn == RFM_NORMAL_END) break;
        }
        else if((r500.rf_cmd2 & 0x7f) > 0x0d)                   // 子機宛ての上り通信の場合
        {
            if(rtn == RFM_NORMAL_END) break;
        }
        else if((rtn == RFM_NORMAL_END) || ((rtn != RFM_R500_NAK) && (rtn != RFM_RPT_ERROR) && (rtn != RFM_R500_CH_BUSY))) break;   // リトライする条件 最初に応答無かった時・CH Busyの時
        */

        if(my_rpt_number > 0) repeater_is_engaged = false;

        if(rtn != RFM_SERIAL_TOUT)
        {
            if(rfm_recv(1400) == RFM_NORMAL_END)                // 次の中継機コマンドまたはダミー送信が受信できた場合は、中継機通信成功と見なす
            {
                if(RCOM.rxbuf.command == 0x42)
                {
                    rtn = RFM_NORMAL_END;
                    break;
                }
            }
        }

        // 中継機に送信するとき
        // 転送途中であれば受け側の無線モジュールがエラーにする。リトライ送信を止める意味がある。最終ＡＣＫを受けなくても成功にできる。
        // 無線モジュール仕様書では、終了応答でしかＲＳＳＩセットしないので、多分、ここの意味無い？
        /*
        if(relay_rssi.level_0x34 > 100)
        {
            rtn = RFM_NORMAL_END;                               // 0x34受信時のRSSIレベルがかなり高い場合、中継機通信成功と見なす
            break;
        }
        */

        if(rtn == RFM_R500_CH_BUSY){
            wait.channel_busy = WAIT_2SEC;
        }

        tx_thread_sleep (100);  // 1 sec
        if(i >= 2){
            break;
        }

        if(rtn == RFM_R500_BUSY)                                // 無線モジュールを初期化する（無線モジュールがＢｕｓｙ応答中だと、初期化してＣＳ Ｌｏｗにしないと電波が出ない）
        {
            relay_initialize(my_rpt_number);
            RF_CS_ACTIVE();
        }

        if(rfm_country == DST_JP){
            tx_thread_sleep (210);                // ２秒休止の確保
        }
    }

    if(rtn == RFM_SERIAL_TOUT){
        rtn = RFM_RPT_ERROR;             // 次の中継機が応答しなかった
    }

    return(rtn);
}



/**
 * @brief   タイムアウト時間計算
 * r500レジスタの値から、無線コマンドを発行する場合のタイムアウト時間を計算する。
 * 引数：なし
 * 返り値：タイムアウト時間[秒]
 * RF_buff.rf_req.timeout_time に返り値と同じ値が入る
 * @param OukagaiSize
 */
static void calc_rf_timeout(uint16_t OukagaiSize)
{
    //uint OukagaiSize = 0;

    uint16_t rtn;
//    uint16_t rpt_com_time;
//    uint16_t scan_time;
//    uint16_t unit_com_time;
//    uint16_t block_size;
    int rpt_com_time;       //2020.Jan.16 intに変更
    int scan_time;
    int unit_com_time;
    int block_size;


    const uint16_t calling_Rpt_time = RfDelay_Rpt;      // 09 * 0.125 = 1.125[sec]  [中継機宛]連続送信時間
    const uint16_t calling_Unit_time = RfDelay_Unit;    // 18 * 0.125 = 2.250[sec]  [子機宛]連続送信時間
//  const uint16_t block_time = 3;                      // 03 * 0.125 = 0.375[sec]  64byte1ブロックを転送する時間
    const uint16_t block_time = 2;                      // 03 * 0.125 = 0.250[sec]  64byte1ブロックを転送する時間
    const uint16_t search20_time = 8;                   // 08 * 0.125 = 1.000[sec]  子機検索,一斉記録開始等の応答20個分の時間
    const uint16_t realscan20_time = 8;                 // 08 * 0.125 = 1.000[sec]  リアルスキャン等の応答20個分の時間

    const uint16_t ex_Rpt_Size = 10;
    const uint16_t ex_Unit_Size = 32;

    uint16_t r500_para_tmp;
    //r500_para_tmp = (uint)(r_r500_para1());
    r500_para_tmp = r500.para1.rpt_cnt;

    //if(r_r500_rf_cmd2() != 0x01){ // 0x01は親機へのキャンセルお伺いコマンド(親機へのキャンセルお伺いコマンドの場合はデータ数が分かる)
    if((r500.rf_cmd2 & 0x7f) != 0x01){
        // 中継機の往復にかかる時間を算出する
        // このswitch文では中継機の往復で転送するブロック数を算出(リトライも考慮した数)
        //switch(r500.rf_cmd2){
        //switch(r_r500_rf_cmd2()){
        switch(r500.rf_cmd2 & 0x7f){
            case 0x60:  // データ吸上げ(個数)   親機へのキャンセルお伺いコマンドが返るまでの時間
            case 0x61:  // データ吸上げ(番号)   親機へのキャンセルお伺いコマンドが返るまでの時間
            case 0x62:  // データ吸上げ(時間)   親機へのキャンセルお伺いコマンドが返るまでの時間
            case 0x24:      // ********** PUSHデータ吸い上げ(時間)
            case 0x2a:      // ********** PUSHデータ吸い上げ(番号)
                //block_size = retray_size(501);    // ２０１１／０１／０５ 瀬木さんに確認して削除

            case 0x63:  // 子機登録変更
            case 0x64:  // 設定テーブル取得
            case 0x65:  // 記録開始
            case 0x69:  // 上下限変更
            case 0x6A:  // 警報条件設定
            case 0x6C:  // 現在値取得
            case 0x1c:      // ********** PUSH設定値WRITE
            case 0x19:      // ********** PUSH時計
            case 0x1f:      // ********** PUSHメッセージ
            case 0x1e:      // ********** PUSHリモート
			case 0x25:	// 子機ＢＬＥ設定の書き込み
			case 0x2B:	// 子機ＢＬＥ設定の読み込み
                block_size = retray_size(2);
                break;

            case 0x22:      // ********** PUSH設定値READ
            case 0x23:      // ********** PUSHテーブルWRITE
                block_size = retray_size(16);
                break;

            case 0x66:  // 一斉記録開始
            case 0x6D:  // 子機検索(ブロードキャスト)
            case 0x2D:  // 子機検索
            case 0x40:  // 中継機検索
                //scan_time = (uchar)r500.para2;    // 最大子機(中継機)番号
                scan_time = (uint16_t)r500.para2[0];   // 最大子機(中継機)番号
                scan_time = (uint16_t)(scan_time * 19);
                block_size = scan_time / 64;
                if((scan_time % 64) != 0){
                    block_size++;
                }
                block_size = retray_size(block_size);
                block_size = block_size + retray_size(1);
                break;

//          case 0x67:

            case 0x68:  // リアルスキャン
                //scan_time = (uchar)r500.para2;    // 最大子機番号
                scan_time = (uint8_t)r500.para2[0];   // 最大子機番号
                scan_time = scan_time * 30;
                block_size = scan_time / 64;
                if((scan_time % 64) != 0){
                    block_size++;
                }
                block_size = retray_size(block_size);
                break;

			case 0x30:	// 子機と中継機の一斉検索
				scan_time = (uint8_t)r500.para2[0];	// 最大子機番号
				scan_time += r500.route.rpt_max;	// 最大中継機番号

				scan_time = scan_time * 30;
				block_size = scan_time / 64;
				if((scan_time % 64) != 0){
				    block_size++;
				}
				block_size = retray_size(block_size);
				break;

            case 0x6E:  // プロテクトON/OFF
            case 0x6B:  // モニタリング間隔変更
            case 0x27:  // ロギング停止
            case 0x0f:  // 中継機受信履歴消去
            case 0x1a:      // ********** PUSHデータ消去
                block_size = retray_size(1);
                break;

            case 0x0e:  // 中継機受信履歴取得
                block_size = ex_Rpt_Size + ex_Unit_Size;
                block_size = block_size * 2;
                block_size = (block_size / 64) + 1;
                block_size = block_size * ((r500_para_tmp + 1) / 2);
                block_size = retray_size(block_size);
                break;

//          case 0x6F:

            default:
                block_size = 0;
                break;
        }
        //rpt_com_time = r500.para1 * ((calling_Rpt_time * 2) + (block_size * block_time)); // 親機-末端中継機の往復通信時間
        //rpt_com_time = r500_para_tmp * ((calling_Rpt_time * 2) + (block_size * block_time));  // 親機-末端中継機の往復通信時間
        rpt_com_time = r500_para_tmp * ((calling_Rpt_time * 2 * 5) + (block_size * block_time));    // 親機-末端中継機の往復通信時間(リトライ時間も考慮)


        // 子機との通信時間を算出する
        // このswitch文では中継機の往復で転送するブロック数を算出(リトライも考慮した数)

        unit_com_time = (calling_Unit_time * 3);

        //switch(r500.rf_cmd2){
        //switch(r_r500_rf_cmd2()){
        switch(r500.rf_cmd2 & 0x7f){
            case 0x60:  // データ吸上げ(個数)   親機へのキャンセルお伺いコマンドが返るまでの時間
            case 0x61:  // データ吸上げ(番号)   親機へのキャンセルお伺いコマンドが返るまでの時間
            case 0x62:  // データ吸上げ(時間)   親機へのキャンセルお伺いコマンドが返るまでの時間
            case 0x24:      // ********** PUSHデータ吸い上げ(時間)
            case 0x2a:      // ********** PUSHデータ吸い上げ(番号)
                unit_com_time += (block_time * retray_size(501));
                break;

            case 0x64:  // 設定テーブル取得
            case 0x6C:  // 現在値取得
			case 0x25:	// 子機ＢＬＥ設定の書き込み
			case 0x2B:	// 子機ＢＬＥ設定の読み込み
                unit_com_time = (uint16_t)(unit_com_time + block_time * retray_size(1));
                break;

            case 0x63:  // 子機登録変更
            case 0x65:  // 記録開始
            case 0x69:  // 上下限変更
            case 0x6A:  // 警報条件設定
            case 0x6E:  // プロテクトON/OFF
            case 0x6B:  // モニタリング間隔変更
            case 0x27:  // ロギング停止
            case 0x1c:      // ********** PUSH設定値WRITE
            case 0x19:      // ********** PUSH時計
            case 0x1f:      // ********** PUSHメッセージ
            case 0x1e:      // ********** PUSHリモート
            case 0x1a:      // ********** PUSH消去
                unit_com_time = (uint16_t)(unit_com_time + block_time * retray_size(0));
                break;

            case 0x22:      // ********** PUSH設定値READ
            case 0x23:      // ********** PUSHテーブルWRITE
                unit_com_time = (uint16_t)(unit_com_time + block_time * retray_size(16));
                break;

            case 0x66:  // 一斉記録開始
            case 0x6D:  // 子機検索(ブロードキャスト)
            case 0x2D:  // 子機検索
            case 0x40:  // 中継機検索
                scan_time = (uint8_t)r500.para2[0];
                block_size = scan_time / 20;
                if((scan_time % 20) != 0){
                    block_size++;
                }
                unit_com_time = (uint16_t)(unit_com_time + search20_time * block_size);
                break;

//          case 0x67:

            case 0x68:  // リアルスキャン
                //scan_time = (uchar)r500.para2;
                scan_time = (uint8_t)r500.para2[0];
                block_size = scan_time / 20;
                if((scan_time % 20) != 0){
                    block_size++;
                }
                unit_com_time += (realscan20_time * block_size);
                break;

			case 0x30:	// 子機と中継機の一斉検索
				scan_time = (uint8_t)r500.para2[0];
				scan_time += r500.route.rpt_max;				// 最大中継機番号
			
				block_size = scan_time / 20;
				if((scan_time % 20) != 0) block_size++;
				unit_com_time += (realscan20_time * block_size);
				break;

//          case 0x6F:

            case 0x0e:  // 受信履歴取得
            case 0x0f:  // 受信履歴消去
//              unit_com_time = 0;
                break;

            default:
                unit_com_time += (block_time * retray_size(0));
                break;
        }
    }
    else{   // 親機へのキャンセルお伺いコマンドを受信した場合のタイムアウト設定
        //rpt_com_time = (r500.para1 * ((calling_Rpt_time * 2))) - calling_Rpt_time;
        /*
        rpt_com_time = (r500_para_tmp * ((calling_Rpt_time * 2))) - calling_Rpt_time;
        unit_com_time = retray_size(OukagaiSize / 64);                      // リトライ時間を
        unit_com_time = calling_Rpt_time + (unit_com_time * block_time);    // 連続送信とデータ転送を含めた通信時間(リトライ考慮済み）
        */

        // 上式ではお伺いコマンドの上り下り到達時間が実際の半分程度、かつ、TX_dummy()受信２．２秒、かつ、お伺いコマンドのリトライ（６秒／回）が考慮されていない
        // ４中継毎に１回リトライ、最低でも１８秒余裕を想定する
        rpt_com_time = (uint16_t)((r500_para_tmp * (calling_Rpt_time * 4)) - ((r500_para_tmp > 0) ? calling_Rpt_time : 0));
        unit_com_time = (uint16_t)(calling_Rpt_time + (retray_size(OukagaiSize / 64) * block_time) + (18 + (uint16_t)r500_para_tmp / 4 * 6) * 8);
    }

    rtn = (uint16_t)(((rpt_com_time + 7) / 8) + ((unit_com_time + 7) / 8)); // 切り上げるために7足してから8で割る。

    if(((r500.rf_cmd2 & 0x7f) == 0x01) && (rtn < 180)){
        rtn = 180;   // お伺いコマンドの場合は最低180秒にする
    }

    timer.int1000 = 0;
    RF_buff.rf_req.timeout_time = rtn;                          // 計算したタイムアウト時間をレジスタにセット

    //return (rtn);
}




/**
 * @brief   自局の中継機番号が中継機テーブルの先頭から何番目にいるか返す
 * @param Max_Size
 * @return
 */
uint8_t rpt_cnt_search(uint8_t Max_Size)
{
    uint8_t rtn = 0xfe;
    int cnt;
    char *poi;

    poi = rpt_sequence;
    for(cnt = 0 ; cnt < Max_Size ; cnt++,poi++){
        if(my_rpt_number == *poi){
            rtn = (uint8_t)(cnt + 1);
            break;
        }
    }
    return (rtn);
}

# if 0
//未使用 未完成 関数

/**
 * 時局宛て以外の中継器番号を保存する
 * m57.rpt_MyNo m57.rpt_RF_Level
 */
static void rpt_backup(void)
{
}


/**
 * 時局宛て以外の中継機番号、子機番号を保存をクリア
 */
static void rpt_backup_clr(void)
{
}


/**

 * @return  変換後の秒数
 */
static uint32_t write_ex_rx(void)
{

    return(0);
}

#endif

/**
 * @brief   R500C ダイレクトコマンドの実行部
 *          CFR500Special()で使用
 * @param mode
 * @param time
 * @param pData  データ部へのポインタ
 * @return
 * @note    cmd_thread_entry.c CFR500Special() より分離、関数化
 */
uint32_t R500C_Direct(int mode, int time, char *pData)
{
    SSP_PARAMETER_NOT_USED(mode);       //現在未使用

    uint32_t    Err;

    RF_CS_ACTIVE();                                       // ＣＳ Ｌｏｗ（無線モジュール アクティブ）
    rf_delay(20);

    rfm_send(pData);        //無線モジュール SOH/STXフレーム送信（SUM/CRC自動付加）


    if(rfm_recv((uint32_t)time) == RFM_NORMAL_END)
    {
        StatusPrintfB("DATA", (char *)&RCOM.rxbuf.header, RCOM.rxbuf.length + 5);   // チェックサム部は送らない
        Err = ERR(RF,NOERROR);
    }
    else{
      Err = ERR(RF, OTHER_ERR);
    }

    //RF_CS_INACTIVE;

    return (Err);
}

/**
 * @brief   無線モジュールのビジー状態をチェックする
 * 無線モジュールが通信中かどうかチェックする
 * @return  1 = BUSY
 * @note    未使用
 */
int Chreck_RFM_Busy(void){
    ioport_level_t level;
    g_ioport.p_api->pinRead(RFM_BUSY_PORT, &level);
    if(level == IOPORT_LEVEL_LOW){
        return (1);
    }
    else {
        return(0);
    }
 }

/**
 * @brief   無線モジュールのステータスをチェックする
 * @return  1 = module受信動作中
 * @note   通常ポートは  OD出力ポート設定
 * @note    未使用
 */
int Chreck_RFM_Status(void){
    ioport_level_t level;
    g_ioport.p_api->pinRead(RFM_RESET_PORT, &level);
    return(level);

 }
