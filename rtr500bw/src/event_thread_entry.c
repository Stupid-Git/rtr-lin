/**
 * @file    event_thread_entry.c
 * @note	2020.02.05	v6 0204 日立向けソース反映済み
 * @note    2020.Jun.26 event.cと統合
 * @note    2020.Jun.30 USBの切断処理待ち等を修正
 * @note    2020.Jul.01 GitHub 0630ソース反映済み
 * @note    2020.Jul.01 GitHub 0701ソース反映済み
 */
#define _EVENT_THREAD_ENTRY_C_

#include "MyDefine.h"
#include "flag_def.h"
#include "Globals.h"
#include "General.h"
#include "Config.h"
#include "DateTime.h"
#include "Warning.h"
#include "Log.h"
#include "Rfunc.h"   //check_regist_unit()
#include "system_time.h"
#include "system_thread.h"
#include "led_thread_entry.h"
#include "auto_thread.h"
#include "sntp_thread.h"
#include "usb_thread.h"
#include "auto_thread.h"
//#include "event_thread.h"
#include "event_thread_entry.h"
#include "usb_thread_entry.h"

//extern TX_THREAD event_thread;
extern TX_THREAD sntp_thread;
extern TX_THREAD auto_thread;

//
// entry threadは、terminate後に、resetにて再起動させる？？？？
// この動作はできないみたい！！

//Event.cと統合した
static void EVT_Create(void);
static void TIMER_Create(void);

static void EVENT_SetSuc( uint8_t Num, char *Event );

///イベントオブジェクト
volatile OBJ_EVENT  EVT ={
    EVT_Create,
    0,
    {0},
    {0},
    {0},
    {0}
};
///タイマオブジェクト
volatile OBJ_TIMER  TIMER ={
    TIMER_Create,
    0,
    0,
    0
};



/**
 *  @brief  Event Thread entry function
 *
 *  @note   いろんなスレッドからサスペンドされる。注意（待ち状態の確認無しで）
 *
 *  @note   time_update_callback()でFLG_EVENT_READYがセット
 *  @attention   このスレッドはUSBが非接続の時しか動作しない
 *
 *  @bug   FLG_EVENT_RESTARTをセットしている個所がない
 */
void event_thread_entry(void)
{

    ULONG actual_events;
    ULONG actual_events2;           // sakaguchi 2020.09.16
    ULONG Flag = 0;
    rtc_time_t Tm;
    int	Week;
    uint8_t Suc = 0;                // Suc:吸い上げは一度やったら60分無視してみる
    uint32_t utime;
    int32_t Min10, DayMin, cyc, retryTM = -1;
	int32_t day_sec;
	uint32_t i;

	//スレッド初期化
    Printf("Event Thread Start\n");

//    tx_event_flags_set (&g_wmd_event_flags, FLG_EVENT_INITIAL, TX_OR);                  // AT Start起動（起動時）
    if(WaitRequest != WAIT_REQUEST){
        tx_event_flags_set (&g_wmd_event_flags, FLG_EVENT_INITIAL, TX_OR);      // AT Start起動（登録情報変更時）
        mate_at_start = 0;
    }else{
        mate_at_start = 1;
    }

RESTART_USB:
        //USB切断待ち（イベントシグナルはsystem_threadの起動時と実際のUSB切断でセットされる）
//そもそもスレッド起動時はUSB切断を待つ必要がない
//		tx_event_flags_get (&event_flags_cycle, FLG_EVENT_DISCONNECT_USB , TX_OR_CLEAR, &actual_events, TX_WAIT_FOREVER);


//        EVT.Create();   //イベント初期化

	for(;;){        // 一番外側

	    if(isUSBState() == USB_CONNECT )      //USBが接続されているとこれ以降何もしない
	    {
//	        tx_thread_sleep(10);    //100ms
            PutLog(LOG_SYS,"USB Connect");          // sakaguchi 2021.01.28 ログ追加
	        tx_event_flags_get (&event_flags_cycle, FLG_EVENT_DISCONNECT_USB , TX_OR_CLEAR, &actual_events, TX_WAIT_FOREVER);
            WarningPort((uint16_t)auto_control.backup_cur, (uint16_t)auto_control.backup_now, (uint16_t)auto_control.backup_warn);		// 2020/09/14 segi
            
            Printf("USB Disconnect(Event)\r\n");
            PutLog(LOG_SYS,"USB Disconnect");       // sakaguchi 2021.01.28 ログ追加
            
            FirstAlarmMoni = 1;                     // 警報監視１回目ＯＮ   // sakaguchi 2021.03.01
            WR_clr_rfcnt();                         // 無線エラー回数クリア // sakaguchi 2021.04.07
	    }

RESTART:
//        while(isUSBState() == USB_DISCONNECT){


            EVT.Create();   //イベント初期化

            if ( CheckEventParam(1) == false ){      // 自律開始可能かどうか？1:Log output 0:Log no output
                PutLog( LOG_SYS, "Event Activation failure" );
                Printf(" Event Activation failure!! \r\n");
                LED_Set( LED_NOREG, LED_ON );
                UnitState = STATE_ERROR;	// エラー停止中

                for(;;)
                {
                    tx_thread_sleep(1000);  //10秒

                    if(CheckEventParam(0) == true){        //EEPROMの設定がされているかどうか   1:Log output 0:Log no output
                        LED_Set( LED_READY, LED_ON );
                        UnitState = STATE_OK;
    					goto RESTART;
//    					break;

                    }
                    else{
                        LED_Set( LED_NOREG, LED_ON );
                        EVT.Create();                       // イベントが無いので、再読み込み　　2020 8/31
                    }

                    if(isUSBState() == USB_CONNECT )      //USBが接続されているとこれ以降何もしない
                    {
                        goto RESTART_USB;
                    }
                }//for
            }
            else{//CheckEventParam(1) == trueの場合
                if(UnitState == STATE_ERROR){             // sakaguchi 2021.01.13 自律開始可能であれば装置ステータスも更新する
                    UnitState = STATE_OK;
                }
            }
//        }

        //CheckEventParam(1) == trueの場合の処理
//        if(isUSBState() == USB_DISCONNECT ) {
     
            if ( EVT.Warn.Enable ) {		// 警報がEnableなら

                // 警報状態をSRAM上のフラグから取得し引きずる
		    /*
		    if ( PowerReset == 1 )
                    ;//WarningPort( 0xFFFF, 0xFFFF, 0 );	// 現状のフラグをセット
                else {
                    ;//SWR_W( CurrentWarning, 0 );		// クリア
                }
			*/
                CurrentWarning = 0;
            }
   

            if ( EVT.Mon.Enable && EVT.Warn.Enable )	// 両方Enableは
            {
                EVT.Trend.Interval = ( EVT.Mon.Interval > EVT.Warn.Interval )
                    ? EVT.Warn.Interval : EVT.Mon.Interval;	// 小さい方

                EVT.Warn.Interval = EVT.Trend.Interval;		// 警報は現在値と同じかそれ以下の周期でなければならない
            }
            else
            {
                if ( EVT.Mon.Enable ){
                    EVT.Trend.Interval = EVT.Mon.Interval;	// Monitor優先
                }
                else if ( EVT.Warn.Enable ){
                    EVT.Trend.Interval = EVT.Warn.Interval;	// 警報優先
                }
                else{
                    EVT.Trend.Interval = 0;
                }
            }

            sync_time = 1;			// １分タイマー起動
            tx_event_flags_get (&event_flags_cycle, (FLG_EVENT_READY | FLG_EVENT_RESTART | FLG_EVENT_CONNECT_USB), TX_OR_CLEAR, &actual_events, TX_NO_WAIT);  // 過去に発生したイベントの読み飛ばし？　
            RfMonit_time = 0;		//  無線フルモニタリング用タイマー
//        }

        // 2020 07 14 hayashi // 2020 07 31 sakaguchi del
//        tx_event_flags_set (&g_wmd_event_flags, FLG_EVENT_INITIAL, TX_OR);                  // AT Start起動

        tx_thread_resume(&auto_thread);
        Printf("auto thread start(resume) \r\n");
        tx_thread_sleep(10);            //  wait 100ms  rfのイニシャル起動待ち

        
		//これが実質のスレッドループ 自律監視
        for(;;){
//        while(isUSBState() == USB_DISCONNECT){

            //イベントシグナル待ち（USB接続イベントを追加）
            tx_event_flags_get (&event_flags_cycle, (FLG_EVENT_READY | FLG_EVENT_RESTART | FLG_EVENT_CONNECT_USB), TX_OR_CLEAR, &actual_events,TX_WAIT_FOREVER);

            if(actual_events & FLG_EVENT_RESTART)       //FLG_EVENT_RESTARTをセットしている個所が居ない
            {
			    tx_event_flags_get (&event_flags_cycle, 0xffffffff,TX_OR_CLEAR, &actual_events,TX_NO_WAIT);          // flag clear
        		tx_event_flags_get (&g_wmd_event_flags, 0xffffffff,TX_OR_CLEAR, &actual_events,TX_NO_WAIT);          // flag clear

				PutLog( LOG_SYS, "Event:Restart");
			    goto RESTART;
			}
			else if(actual_events & FLG_EVENT_CONNECT_USB)  //2020.Jun.26   追加
			{
	            Printf("  ---->>> USB Status  %d\r\n", isUSBState());
	            if( isUSBState()== USB_CONNECT )
	            {
	                goto RESTART_USB;      //USBが接続されているとこれ以降何もしない
//	                break;
	            }
			}
			else if(actual_events & FLG_EVENT_READY)      //イベント処理
			{
	            // 作成中
	            if(WaitRequest == WAIT_REQUEST){
	                Printf("BLE or Socket Command Mode \r\n");
//				mate_time_flag = 1;
	                goto EXIT;                      // WaitRequest のカウントダウンで再開
	            }

// sakaguchi 2020.09.16 ↓
	            if ( UnitState == STATE_COMMAND ){  // 要再起動
	                Printf("  Event Thread UnitState Reboot !!! \r\n");
                    tx_event_flags_set(&event_flags_reboot, FLG_REBOOT_EXEC, TX_OR);
	                goto EXIT;
                }
// sakaguchi 2020.09.16 ↑

	            if ( UnitState != STATE_OK ){       // OK以外なら抜ける
	                Printf("  Event Thread UnitState Error  Exit !!! \r\n");
	                goto EXIT;                      // 誰が再開するの？？？
	            }

				// sakaguchi 2020.10.07
                if((WAIT_CANCEL == WaitRequest) && (1 == mate_at_start)){   // USB接続中のAT STARTキャンセルの復帰処理
                    mate_at_start = 0;
                    tx_event_flags_set (&g_wmd_event_flags, FLG_EVENT_INITIAL, TX_OR);      // AT Start起動（登録情報変更時）
                }

	            utime = RTC_GetGMTSec();                // UTC取得
	            RTC_GetLCT( &Tm );                      // LTC
	            Printf("    ### Event Task LTC %d/%.2d/%.2d %.2d:%.2d:%.2d (%d)\r\n", Tm.tm_year+2000,Tm.tm_mon,Tm.tm_mday,Tm.tm_hour,Tm.tm_min,Tm.tm_sec, Tm.tm_wday);

	            Week = Tm.tm_wday + 1;

	            // 10分周期で
	            Min10 = ( EVT.MinCount % 10 == 0 ) ? 1 : 0;

                // 警報
	            if ( EVT.Warn.Enable ) {
	                if ( EVT.MinCount % EVT.Warn.Interval == 0 ) {
	                    Flag |= FLG_EVENT_KEIHO;                    // 警報のイベントフラグ
	                    EVT.Trend.StartUTC = utime;                 // 開始時刻
	                    Printf("===========>  Warning Timing !!! \r\n");
	                    //PutLog( LOG_SYS, "Event:Warning");
	                }
	            }

	            // 現在値
	            if ( EVT.Mon.Enable ) {

	                if ( EVT.MinCount % EVT.Mon.Interval == 0 ) {
	                    Flag |= FLG_EVENT_MONITOR;                  // モニタのイベントフラグ
	                    CurrRead_ReTry = 0;                         // リトライクリア

	                    EVT.Trend.StartUTC = utime;                 // 開始時刻
	                    //PutLog( LOG_SYS, "Event:Monitor");
	                    Printf("===========>  Monitor Timing !!! (%d) (%d) \r\n",EVT.MinCount,EVT.Mon.Interval);
	                }
	                else if ( Min10 && CurrRead_ReTry ){
	                    Flag |= FLG_EVENT_MONITOR;                  // リトライ
	                }
	            }


	            // 吸い上げ
	            if ( retryTM > 0 ){                                 // リトライ専用タイマー
	                retryTM--;
	            }
	            if ( EVT.Suc.Enable && !Suc ) {

	                if ( retryTM == 0 && Download_ReTry ) {
	                    Flag |= FLG_EVENT_SUCTION;                  // 吸い上げイベントフラグ
	                    retryTM = -1;                               // リトライは1回だけ
	                }

	                if(utime > RUN_BASE_SEC){                        // 明らかに過去でない   2019 01 01 00:00:00

	                    for ( i=0; i<DIMSIZE(EVT.Suc.EnFlag); i++ )     // 吸い上げイベント全チェック
	                    {
	                        if ( EVT.Suc.EnFlag[i] && ( EVT.Suc.Week[i] == 0 || EVT.Suc.Week[i] == Week ) ) {       // Enable & 毎日 or 曜日一致
	                            DayMin = (int32_t)Tm.tm_hour * 60 + Tm.tm_min;  // 00:00からの分を算出

	                            cyc = 60;                                       // デフォルト60分周期

	                            if ( EVT.Suc.EnFlag[i] > 1 ){                   // EnFlagに意味あり？
	                                DayMin %= EVT.Suc.EnFlag[i];                // 余りがイベント時間
	                            }
	                            if ( EVT.Suc.DayMin[i] < 0 ) {                  // 毎時の分周期？
	                                DayMin %= ( -EVT.Suc.DayMin[i] );           // 余りがイベント時間
	                                cyc = ( -EVT.Suc.DayMin[i] );               // 周期
	                            }
	                            else{
	                                DayMin = DayMin - EVT.Suc.DayMin[i];
	                            }

	                            if ( 0 <= DayMin && DayMin < 3 ) {              // 3分の幅を持たせてチェック(時刻補正があるため)
	                                Flag |= FLG_EVENT_SUCTION;                  // 吸い上げイベントフラグ
	                                Download_ReTry = 0;                         // リトライクリア
	                                Suc = 5-1;                                  // 5分無視してみる

	                                //PutLog( LOG_SYS, "Event:Suction");
	                                Printf("===========>  Suction Timing !!!  \r\n");


	                                retryTM = ( cyc >= 60 ) ? 30 : -1;      //1時間以上の周期はリトライ30分

	                                break;
	                            }

	                        }
	                    }
	                }
	                else{
	                    Printf(" RTC is incorrect\r\n");
	                }
	            }
	            else{
	                Suc --;
	            }

	            if(Flag != 0){          // 有効なイベント有り！！
            
	                Printf("Event Flag %04X \r\n", Flag); //
	                switch (Flag)
	                {
                    case FLG_EVENT_KEIHO | FLG_EVENT_MONITOR | FLG_EVENT_SUCTION:
					    //PutLog( LOG_SYS, "Event:Download/Warning/Monitor");
					    PutLog( LOG_SYS, "Event started (Warning Monitoring / Current Readings and Rec Data Transmission)");
	                    break;
	                case FLG_EVENT_KEIHO | FLG_EVENT_MONITOR:
					    //PutLog( LOG_SYS, "Event:Warning/Monitor");
					    PutLog( LOG_SYS, "Event started (Warning Monitoring / Current Readings Transmission)");
	                    break;
	                case FLG_EVENT_MONITOR | FLG_EVENT_SUCTION:
					    //PutLog( LOG_SYS, "Event:Download/Monitor");	
					    PutLog( LOG_SYS, "Event started (Current Readings and Rec Data Transmission)");	
	                    break;
	                case FLG_EVENT_KEIHO | FLG_EVENT_SUCTION:
					    //PutLog( LOG_SYS, "Event:Download/Warning");
					    PutLog( LOG_SYS, "Event started (Rec Data Transmission / Warning Monitoring)");
	                    break;
	                case FLG_EVENT_KEIHO :
					    //PutLog( LOG_SYS, "Event:Warning");
					    PutLog( LOG_SYS, "Event started (Warning Monitoring)");
	                    break;
	                case FLG_EVENT_MONITOR:
					    //PutLog( LOG_SYS, "Event:Monitor");
					    PutLog( LOG_SYS, "Event started (Current Readings Transmission)");
	                    break;
	                case FLG_EVENT_SUCTION:
					    //PutLog( LOG_SYS, "Event:Download");
					    PutLog( LOG_SYS, "Event started (Rec Data Transmission)");
	                    break;
	                }
                    //DebugLog(LOG_DBG, "Event %04X", Flag);
	                tx_event_flags_set (&g_wmd_event_flags, Flag, TX_OR);       // 自律動作用イベントフラグ
	                Flag = 0;
	            }

	            EVT.MinCount++;
                
	            if(EVT.MinCount == 480){
                    if(my_config.device.TimeSync[0] != 0){
	                    Printf("SNTP Thread resume \r\n");
                        DebugLog(LOG_DBG, "SNTP Thread resume");
	                    tx_thread_resume(&sntp_thread);
                    }
	            }
                
               /*
                if(EVT.MinCount % 10 == 0){
	                Printf("SNTP Thread resume \r\n");
                    DebugLog(LOG_DBG, "SNTP Thread resume");
	                tx_thread_resume(&sntp_thread);
	            }
                */
	            if ( EVT.MinCount >= 1440 ){
	                day_sec = (int32_t)RTC_GetGMTSec();
	                Printf("XXXXXXXXX    day seconds %ld\r\n", day_sec - oneday_sec);
                    DebugLog(LOG_DBG, "day seconds diff %ld", day_sec - oneday_sec);
	                oneday_sec = day_sec;
	                EVT.MinCount = 0;                   // １日分で０へ戻す
	            }

			}//FLG_EVENT_READY
			else//これ以外のイベント
			{
			    ;
			}
        }//for  自律監視の終わり


EXIT:
        Printf("--->###  Restart or End Event Thread !!\r\n");
        tx_thread_suspend(&event_thread);           //time_update_callback()で復帰する


    }   // 一番外側
}


//event.cから移動
/**
 *
 */
static void TIMER_Create(void)
{

    //TIMER.MIN = 0;
    TIMER.SEC = 0;
    //TIMER.HOUR = 0; 未使用
}

/**
 *  @brief イベント作成
 *  @note   LO_HIマクロ削除
 */
static void EVT_Create(void)
{
    int16_t flag;
    uint32_t i;

    EVT.MinCount = 0;

    EVT.Warn.Enable = my_config.warning.Enable[0];
    EVT.Mon.Enable  = my_config.monitor.Enable[0];
    EVT.Suc.Enable  = my_config.suction.Enable[0];

    EVT.Warn.Interval = *(int16_t *)&my_config.warning.Interval[0];

    EVT.Mon.Interval = *(int16_t *)&my_config.monitor.Interval[0];


    Printf("    Warning Interval %d \r\n", EVT.Warn.Interval);
    Printf("    Monitor Interval %d \r\n", EVT.Mon.Interval);

    EVENT_SetSuc( 0, &my_config.suction.Event0[0]);
    EVENT_SetSuc( 1, &my_config.suction.Event1[0]);
    EVENT_SetSuc( 2, &my_config.suction.Event2[0]);
    EVENT_SetSuc( 3, &my_config.suction.Event3[0]);
    EVENT_SetSuc( 4, &my_config.suction.Event4[0]);
    EVENT_SetSuc( 5, &my_config.suction.Event5[0]);
    EVENT_SetSuc( 6, &my_config.suction.Event6[0]);
    EVENT_SetSuc( 7, &my_config.suction.Event7[0]);
    EVENT_SetSuc( 8, &my_config.suction.Event8[0]);


    flag = (int16_t)EVT.Suc.EnFlag[0];
    EVT.Suc.EventCnt = flag ? 1 : 0;        // イベント数

    for ( i = 1; i < DIMSIZE(EVT.Suc.EnFlag); i++ ) {   // 吸い上げイベント全チェック
        if( flag ){
            EVT.Suc.EnFlag[i] = 0;      // 強制 Disable
        }
        else if ( EVT.Suc.EnFlag[i] ) {
            EVT.Suc.EventCnt++;         // 全イベント数;
        }
        Printf("\r\n EnFlag=%d / EventCnt=%d", EVT.Suc.EnFlag[i], EVT.Suc.EventCnt);
    }

    EVT.Warn.Route = my_config.warning.Route[0];
    EVT.Mon.Route  = my_config.monitor.Route[0];
    EVT.Suc.Route  = my_config.suction.Route[0];

    EVT.Trend.Interval = 0;                 // 両方Disable
    EVT.Trend.StartUTC = 0;


}




/**
 * 吸い上げイベントをセット
 * @param Num
 * @param Event
 */
static void EVENT_SetSuc( uint8_t Num, char *Event )
{
    int     Min = -10000;

    EVT.Suc.EnFlag[Num] = ( Event[0] == '1' ) ? 1 : 0 ;     // ON/OFF
    EVT.Suc.Week[Num]   = (uint8_t)(AtoI( Event+1, 1 ));        // 曜日

    if ( EVT.Suc.EnFlag[Num] )
    {
        if ( Event[2] != '#' )                  // hour 数字？
        {                                       // 00:00からの分数を計算
            Min = AtoI( Event+2, 2 ) * 60;      // 時を分へ
            Min = Min + AtoI( Event+4, 2 );     // 分を加算
        }
        else if ( Event[3] >= '1' && Event[3] <= '4' )
        {
            Min = AtoI( Event+4, 2 );           // 分そのまま
            EVT.Suc.EnFlag[Num] = (uint8_t)(( Event[3] - '0' ) * 60);   // EnFlagに意味を持たせる
        }
        else if ( Event[3] == '0' )
        {
            Min = AtoI( Event+4, 2 );           // 分そのまま
            if ( Min < 10 || Min > 30 ){
                EVT.Suc.EnFlag[Num] = 0;        // 強制Disable
            }
            else
            {
                EVT.Suc.EnFlag[Num] = 60;       // 毎時：EnFlagに意味を持たせる
                Min = -Min;                     // マイナス値は周期とする
            }
        }
        else{
            EVT.Suc.EnFlag[Num] = 0;            // 強制Disable
        }

        EVT.Suc.DayMin[Num] = (int16_t)Min;
    }
}



/**
 * EEPROMの設定がされているかどうか
 * （イベントチェック用、先頭バイトのみ）
 * @param log  1:Log output 0:Log no output
 * @return
 */
bool CheckEventParam(int log)
{
    bool    err = 0;
    uint8_t smtp, ftp, sender, hsrv;
    uint8_t rtn;
//  uint8_t hsrv;
    uint8_t WarnTo1,WarnTo2,WarnTo3,WarnTo4;
    uint8_t MonTo1,MonTo2;
    uint8_t SucTo1;
    const char  *msg;
    uint16_t WebInterval;                    // sakaguchi 2020.12.23

    Http_Use = HTTP_NOSEND;                           // HTTPによる送信無し

    // イベントのチェック
    smtp = my_config.smtp.Server[0];        // SMTP server
    sender = my_config.smtp.Sender[0];      // SMTP Sender
    ftp =  my_config.ftp.Server[0];         // FTP server
    hsrv = my_config.websrv.Server[0];      // Web server

    WarnTo1 = my_config.warning.To1[0];
    WarnTo2 = my_config.warning.To2[0];
    WarnTo3 = my_config.warning.To3[0];
    WarnTo4 = my_config.warning.To4[0];

    MonTo1 = my_config.monitor.To1[0];
    MonTo2 = my_config.monitor.To2[0];
    SucTo1 = my_config.suction.To1[0];

    WebInterval = *(uint16_t *)my_config.websrv.Interval;       // sakaguchi 2020.12.23

    //if(WebInterval) Http_Use = HTTP_SEND;                       // sakaguchi 2020.12.23
    if( ( WebInterval )                                           // HTTP通信間隔設定有り   // HTTP通信条件修正 sakaguchi 2021.04.07
     || (( EVT.Warn.Enable ) && ( EVT.Warn.Route == 3 ))          // 警報HTTP通信
     || (( EVT.Mon.Enable  ) && ( EVT.Mon.Route == 3  ))          // モニタリングHTTP通信
     || (( EVT.Suc.Enable  ) && ( EVT.Suc.Route == 3  )) ){       // 吸い上げHTTP通信
        Http_Use = HTTP_SEND; 
    }

    rtn = check_regist_unit();
    if (  rtn == 0 ) {  // 子機登録無し
        if(log){
        PutLog( LOG_SYS, "Remote Unit is not registered" );
        }
        err = 1;
    }
    else if ( !EVT.Warn.Enable && !EVT.Mon.Enable && !EVT.Suc.Enable )
    {   // １つもEnableになっていない場合はこちら
        if(log){
        PutLog( LOG_SYS, "All Events are disabled" );
        }
        err = 1;
    }
    else
    {

        if ( EVT.Warn.Enable ) {                // 警報           if ( EVT.Warn.Route == 0 ) {        // E-Mail
            /*  E-Mailは無し
            if ( EVT.Warn.Route == 0 ) {        // E-Mail
                if ( !smtp || !sender ||        // SMTP無し or Sender無し
                     ( !WarnTo1 && !WarnTo2
                     && !WarnTo3 && !WarnTo4 ) ) {  // 宛先全て無し
                        if(log){
                        PutLog( LOG_SYS, "$WReport: Settings are incomplete" );
                        }
                        err = 1;
                }
            }
            */
           //Http_Use = HTTP_SEND;          // sakaguchi 2020.12.23
        }

        msg = 0;
        if ( EVT.Mon.Enable ) {             // 現在値
            /*if ( EVT.Mon.Route == 0 ) {     // E-Mail
                if ( !smtp || !sender ||    // SMTP無し or Sender無し
                        ( !MonTo1 && !MonTo2 ) ){   // 宛先全て無し
                    msg = "E-mail";
                }
            }
            else*/ 
            if ( EVT.Mon.Route == 1 ) {
                if ( !ftp ){                    // FTP無し
                    msg = "FTP";
                }
            }
            else if ( EVT.Mon.Route == 3 ) {
                //Http_Use = HTTP_SEND;     // sakaguchi 2020.12.23
                if ( !hsrv ){               // webserver無し
                    msg = "HTTP";
                }
            }

            if ( msg ) {
                if(log){
                    PutLog( LOG_SYS, "$C: Settings are incomplete(%s)", msg );
                }
                err = 1;
            }

        }
        else{
            if(log){
                PutLog( LOG_SYS, "$C: Disable" );
            }
        }

        msg = 0;
        if ( EVT.Suc.Enable ) {             // 吸い上げ
            if ( EVT.Suc.EventCnt == 0 ) {  // イベントが全てDisable
                if(log){
                    PutLog( LOG_SYS, "No Auto-Sending schedule" );
                }
                err = 1;
            }
            else
            {
                /*
                if ( EVT.Suc.Route == 0 ) {     // E-Mail
                    if ( !smtp || !sender || !SucTo1 ){ // SMTP無し or Sender無し or 宛先全て無し
                        msg = "E-mail";
                    }
                }
                else
                */ 
                if ( EVT.Suc.Route == 1 ) {
                    if ( !ftp ){                    // FTP無し
                        msg = "FTP";
                    }
                }
                else if ( EVT.Suc.Route == 3 ) {
                    //Http_Use = HTTP_SEND;         // sakaguchi 2020.12.23
                    if ( !hsrv ){
                        msg = "HTTP";
                    }
                }
                if ( msg ) {
                    if(log){
                        PutLog( LOG_SYS, "$A: Settings are incomplete(%s)", msg );
                    }
                    err = 1;
                }

            }
        }
    }

    Printf( "CheckEventParam() %d  \r\n", err);

    return ( err ? false : true );
}



