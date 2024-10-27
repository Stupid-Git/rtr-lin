/**
 * @file	system_time.c
 *
 * @date	 Created on: 2018/10/19
 * @author	haya
 * @note	2020.01.30	v6ソースマージ済み
 * @note    2020.Jul.01 GitHub 0630ソース反映済み
 * @note	2020.Jul.01 GitHub 0701ソース反映済み

 */

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#define _SYSTEM_TIME_C_

#include "system_time.h"
#include "system_thread.h"
#include "flag_def.h"
#include "MyDefine.h"

#include "Globals.h"
#include "DateTime.h"
#include "General.h"
#include "http_thread.h"
//#include "event_thread.h"
#include "event_thread_entry.h"
#include "ble_thread_entry.h"
#include "led_thread_entry.h"
#include "tls_client.h"            // sakaguchi 2020.09.02
#include "Log.h"
#include "auto_thread_entry.h"

//extern TX_THREAD http_thread;       //sakaguchi
//extern TX_THREAD event_thread;

//extern TX_THREAD event_thread;

//#include "usb_thread_entry.h"

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
#define UNUSED(x) (void)(x)

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Private function prototypes
 **********************************************************************************************************************/



/***********************************************************************************************************************
 * Private functions
 **********************************************************************************************************************/
static int weekday_get(rtc_time_t * p_time);
static void time_validate(int * time, int max);

static const uint8_t month_table[12] = {0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5};
static const uint8_t ly_month_table[12] = {6, 2, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5};
static const uint8_t days_in_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};



/***********************************************************************************************************************
 * Global Variables
 **********************************************************************************************************************/
//volatile bool g_time_updated = false;
//volatile bool test_port = false;
//volatile bool test_port2 = false;

//volatile bool ms1_port = false;

/**
 *********************************************************************************************************************
 * RTC to update the system time.
 * 1sec timer
 * @param p_context
 * @bug FLG_EVENT_READYのイベントはセットしても実行されない
 *********************************************************************************************************************
 */
void time_update_callback (rtc_callback_args_t * p_context)
{
        //UNUSED(p_context);      // Eliminate warning from unused arg
    SSP_PARAMETER_NOT_USED(p_context);      //2020.01.20 追加


    CHAR *name;
    UINT state;
    ULONG run_count;
    UINT priority;
    UINT preemption_threshold;
    ULONG time_slice;
    TX_THREAD *next_thread;
    TX_THREAD *suspended_thread;
    uint16_t WebInterval;               // sakaguchi 2021.04.07

        rtc_time_t ct;
        int adjust;

        //g_rtc.p_api->calendarTimeGet(g_rtc.p_ctrl, &ct);
        with_retry_calendarTimeGet("time_update_callback", &ct);    // 2022.09.09

        TIMER.SEC++;
 
        switch (sync_time)
        {
            case 1:             // 0秒同期 次は０秒
                if(ct.tm_sec==59){
                    TIMER.MIN = 59;
                    sync_time = 2;   
                    oneday_sec = (int32_t)(RTC_GetGMTSec() - 1);
                }         
                break;
            case 2:             // 通常動作
                TIMER.MIN++;
                if(TIMER.MIN >= 60)     // 1分で戻す
                    TIMER.MIN = 0;
                break;
            case 0:    
            default:
                TIMER.MIN = 59;
                break;
        }
 
        /* 未使用
        TIMER.HOUR++;
        if(TIMER.HOUR >= 3600){   // １時間で戻す
            TIMER.HOUR = 0;
        }
        */
        /*
        if(TIMER.MIN == 0){
            tx_event_flags_set(&g_rtc_event_flags, FLG_RTC_INT,TX_OR);  // 未使用
        }
        */
 


        // 1000msec counter
        if(timer.int1000 < UINT16_MAX){
            timer.int1000++;
        }

        // 時刻合わせしない限り、ズレないはず！！
        if(TIMER.MIN == 30){
            adjust = 30 - ct.tm_sec;
            Printf("\r\n###########    time adjust !!!! %ld #####\r\n\r\n", adjust);
            // 修正スピードが違う どうする？
            if(adjust < 0){
                TIMER.MIN ++;
            }
            else if (adjust > 0)
            {
                TIMER.MIN --;
            }
        }


        if(TIMER.MIN == 0 && sync_time == 2)          // １分周期で起動
        {
            //Printf("   event thread start: %.2d  \r\n", ct.tm_sec);
            Printf("   event thread start: %d/%d/%d %d:%d:%d (%d)\r\n", ct.tm_year,ct.tm_mon,ct.tm_mday,ct.tm_hour,ct.tm_min,ct.tm_sec,sync_time);
            // event threadのフラグ待ち解除
            tx_event_flags_set (&event_flags_cycle, FLG_EVENT_READY,TX_OR);     //event_threadリスタート

        }



        // socket通信タイムアウト？？
        if(soc_time_flag){
            wait.sec_1.soc_time = 180;  // 180 sec restart;
            soc_time_flag = 0;
        }
        if(wait.sec_1.soc_time > 0){
            wait.sec_1.soc_time--;
        }


        if(mate_time_flag){
//            wait.sec_1.mate_command = 180;  // 300 sec restart;
            wait.sec_1.mate_command = (uint32_t)(mate_time + 1);      // 1秒補正
            mate_time_flag = 0;
        }

        if(wait.sec_1.mate_command > 0){
            wait.sec_1.mate_command --;
            if(wait.sec_1.mate_command==0){
                WaitRequest = WAIT_CANCEL;              // sakaguchi 2020.08.31
                FirstAlarmMoni = 1;                     // 警報監視１回目ＯＮ           // sakaguchi 2021.03.01
                WR_clr_rfcnt();                         // 無線エラー回数クリア         // sakaguchi 2021.04.07

                    if(1 == mate_at_start){
                        tx_event_flags_set (&g_wmd_event_flags, FLG_EVENT_INITIAL, TX_OR);      // AT Start起動（登録情報変更時）
                        mate_at_start = 0;
						mate_at_start_reset = 1;												// AT_start で警報変数クリア
                    }
                tx_thread_info_get(&event_thread, &name, &state, &run_count, &priority, &preemption_threshold,&time_slice,&next_thread,&suspended_thread);
                if(state == TX_SUSPENDED){
//                    WaitRequest = WAIT_CANCEL;        // sakaguchi 2020.08.31
//                    if(1 == mate_at_start){
//                        tx_event_flags_set (&g_wmd_event_flags, FLG_EVENT_INITIAL, TX_OR);      // AT Start起動（登録情報変更時）
//                        mate_at_start = 0;
//                    }
                    tx_thread_resume(&event_thread);  
                    Printf("Event Thread Resume Wait Time Cancel \r\n");
                }
            }
        }

        if(warn_time_flag){
            wait.sec_1.ext_warn = 11;  // 10 sec restart;
            warn_time_flag = 0;
        }

        if(wait.sec_1.ext_warn > 0){
            wait.sec_1.ext_warn --;
            if(wait.sec_1.ext_warn==0){
                EX_WRAN_OFF;
                LED_Set( LED_WARNING, LED_OFF );
            }
        }

		// EWSTWコマンド実行後にEWSTRを続けるタイマ
		if(EWSTR_time > 0){
			EWSTR_time--;
		}

// sakaguchi cg UT-0034 ↓
        //操作リクエスト送信タイマ
        if(ReqOpe_time > 0){
            ReqOpe_time--;
            if(0 == ReqOpe_time){
                if((VENDER_HIT != fact_config.Vender) && (Http_Use == HTTP_SEND)){ 
                //if( VENDER_HIT != fact_config.Vender){    // Hitachi以外
// 2023.08.01 ↓
                    if( LOG_SEND_CNT <= GetLogCnt_http(0)){     // ログが10件以上溜まっている場合
                        G_HttpFile[FILE_L].sndflg = SND_ON;         // ログ
                    }
// 2023.08.01 ↑
                }

                //if(Http_Use == HTTP_SEND){
                WebInterval = *(uint16_t *)my_config.websrv.Interval;   // HTTP通信条件修正 // sakaguchi 2021.04.07
                if(WebInterval){
                    if(true != bhttp_request_docheck()){        // WSSからのリクエスト実行中以外    // sakaguchi 2020.09.02
                        G_HttpFile[FILE_R].sndflg = SND_ON;         // 操作リクエスト
                        tx_event_flags_set(&event_flags_http, FLG_HTTP_READY, TX_OR);
                    }
                }
            }
        }
        //ログ送信タイマ
//        if(PostLog_time > 0){
//            PostLog_time--;
//            if(0 == PostLog_time){
//                G_HttpFile[FILE_L].sndflg = SND_ON;
//                tx_event_flags_set(&event_flags_http, FLG_HTTP_READY, TX_OR);
//            }
//        }
// sakaguchi cg UT-0034 ↑

        RfMonit_time++;                                     // フルモニタリング用タイマー
//g_ioport.p_api->pinWrite(IOPORT_PORT_00_PIN_14, IOPORT_LEVEL_LOW);
/*
    if(RTC.Diff !=  RtcDiff_Backup){
        if(Rtc_err==0){
            PutLog(LOG_SYS,"RTC Diff Error %ld", RTC.Diff);  
            Rtc_err = 1;
        }
        Printf("?????  Time Diff Error %ld\r\n",RTC.Diff);
    }
*/
}



/**
 *******************************************************************************************************************
 * @brief   Corrects the time value if it has wrapped above the maximum value or below 0.
 *
 * @param[in]   p_time  Time value
 * @param[in]   max     Maximum value for this time
 *******************************************************************************************************************
*/
static void time_validate(int * p_time, int max)
{
    if (*p_time < 0)
    {
        *p_time = max - 1;
    }
    if (*p_time >= max)
    {
        *p_time = 0;
    }
}

/**
 * *****************************************************************************************************************//**
 * @brief   Modifies the current time variable by the amount of time requested.  Wrap around events will not affect the
 *
 * @param   p_time
 * years   number of years to adjust by (signed value)
 * months  number of months to adjust by (signed value)
 * days    number of days to adjust by (signed value)
 * hours   number of hours to adjust by (signed value)
 * mins    number of minutes to adjust by (signed value)
 * secs    number of seconds to adjust by (signed value)
 * 
 * RTCへ時間の設定を行う SMTP等からコールされる
 * 
***********************************************************************************************************************/
void adjust_time(rtc_time_t * p_time)
{
    ssp_err_t error;            //sakaguchi add UT-0021
    rtc_time_t t;

//    g_rtc.p_api->calendarTimeGet(g_rtc.p_ctrl, &t);
    with_retry_calendarTimeGet("adjust_time", &t);      // 2022.09.09
 
    t.tm_sec    = p_time->tm_sec;
    t.tm_min    = p_time->tm_min;
    t.tm_hour   = p_time->tm_hour;
    t.tm_mday   = p_time->tm_mday;
    t.tm_mon    = p_time->tm_mon;
    t.tm_year   = p_time->tm_year;
    t.tm_wday   = p_time->tm_wday;

    bool leap = false;
    if (0 == t.tm_year % 4)
    {
        leap = true;
        if (0 == t.tm_year % 100)
        {
            leap = false;
            if (0 != t.tm_year % 400)
            {
                leap = true;
            }
        }
    }

    /** Ensure that all time values are valid. */
    time_validate(&t.tm_sec, 60);
    time_validate(&t.tm_min, 60);
    time_validate(&t.tm_hour, 24);
    time_validate(&t.tm_mon, 12);

    /** Calculate the maximum days in the current month. */
    int32_t temp_month = (t.tm_mon + 12) % 12;
    uint8_t days_in_current_month = days_in_month[temp_month];
    if (leap && (1 == temp_month))
    {
        days_in_current_month++;
    }
    //This fixes for 1.2.0
    time_validate(&t.tm_mday, days_in_current_month+1);
    t.tm_wday = weekday_get(&t);

//sakaguchi add UT-0021 ↓
    error = g_rtc.p_api->calendarTimeSet(g_rtc.p_ctrl, &t, true);
    Printf("   adjust Time: %d/%d/%d %d:%d:%d  (%d)[%d]\n", t.tm_year,t.tm_mon,t.tm_mday,t.tm_hour,t.tm_min,t.tm_sec,t.tm_wday,error);

    error = g_rtc.p_api->calendarCounterStart(g_rtc.p_ctrl);
    Printf("   counter start: [%d]\n",error);
//sakaguchi add UT-0021 ↑

    /** Post message to system. */
    //time_message_post();
}



/**
 *
 * @param p_time
 * @return
 */
int weekday_get(rtc_time_t * p_time)
{
    int32_t day = p_time->tm_mday;
    bool leap = false;
    if (0 == p_time->tm_year % 4)
    {
        leap = true;
        if (0 == p_time->tm_year % 100)
        {
            leap = false;
            if (0 != p_time->tm_year % 400)
            {
                leap = true;
            }
        }
    }
    int32_t month_offset;
    /*
    if (leap)
    {
        month_offset = ly_month_table[p_time->tm_mon];
    }
    else
    {
        month_offset = month_table[p_time->tm_mon];
    }
    */
    month_offset = (leap) ? ly_month_table[p_time->tm_mon] : month_table[p_time->tm_mon];

    int32_t year_offset = p_time->tm_year % 100;
    int32_t century_offset = (3 - ((p_time->tm_year / 100) % 4)) * 2;
    int32_t offset = day + month_offset + year_offset + year_offset / 4 + century_offset;
    int32_t index = offset % 7;

    return (index);
}



/**
 * RTCの初期化
 * system_thread_entryから呼ばれる
 */
void system_time_init(void)
{
    ssp_err_t error;
//    rtc_time_t t = {0};

    error = g_rtc.p_api->open(g_rtc.p_ctrl, g_rtc.p_cfg);
    if (SSP_SUCCESS == error)
    {
        sync_time = 0;
        TIMER.MIN = 60;

        g_rtc.p_api->calendarCounterStart(g_rtc.p_ctrl);

        rtc_time_t current_time;
        //g_rtc.p_api->calendarTimeGet(g_rtc.p_ctrl, &current_time);
        with_retry_calendarTimeGet("system_time_init", &current_time);      // 2022.09.09
        Printf("power on Time: %d/%d/%d %d:%d:%d  (%d)\n", current_time.tm_year,current_time.tm_mon,current_time.tm_mday,current_time.tm_hour,current_time.tm_min,current_time.tm_sec,current_time.tm_wday);
/*
        struct  tm      {
                int     tm_sec;         // 秒 (0から59)
                int     tm_min;         // 分 (0から59)
                int     tm_hour;        // 時 (0から23)
                int     tm_mday;        // 日 (1から31)
                int     tm_mon;         // 月 (0から11)
                int     tm_year;        // 年 (年 - 1900)
                int     tm_wday;        // 週 (0から6 : 0が日曜日
                int     tm_yday;        // 年内の経過日数 (0から365)
                int     tm_isdst;       // 夏時間 : 0以外の値
        };
*/
        rtc_time_t st = {0};

        if(current_time.tm_year >= 120 &&  current_time.tm_mon >= 8){
            ;
        }
        else{
        	st.tm_year = 2020-1900;
        	st.tm_mon = 1-1;
        	st.tm_mday = 1;
        	st.tm_hour = 11;
        	st.tm_min = 11;
        	st.tm_sec = 11;

        	error = g_rtc.p_api->calendarTimeSet(g_rtc.p_ctrl, &st, true);
        }
        //adjust_time(&t);

        //g_rtc.p_api->calendarTimeGet(g_rtc.p_ctrl, &current_time);  //
        with_retry_calendarTimeGet("system_time_init2", &current_time);     // 2022.09.09
//        t.tm_wday = weekday_get(&current_time);
        st.tm_wday = weekday_get(&current_time);

        g_rtc.p_api->periodicIrqRateSet(g_rtc.p_ctrl, RTC_PERIODIC_IRQ_SELECT_1_SECOND);
        error = g_rtc.p_api->irqEnable(g_rtc.p_ctrl, RTC_EVENT_PERIODIC_IRQ);
        if (error != SSP_SUCCESS)
        {
            Printf("RTC Initial Error \r\n");
            while(1);
            /** TODO_THERMO: Post error message. */
        }
       
        wait.sec_1.mate_command = 0;
       
    } 
    else 
    {
        while(1);
    }
}


/**
 * TIMER0 callback 125ms インターバル
 * 8191sec までカウント
 * @param p_args
 */
void timer0_callback(timer_callback_args_t * p_args)
{
    if(TIMER_EVENT_EXPIRED == p_args->event){
        if(timer.int125 < UINT16_MAX){
            timer.int125++;
        }
    }
}


/**
 * TIMER1 callback 10ms インターバル
 * 8191sec までカウント
 * @param p_args
 */
void timer1_callback(timer_callback_args_t * p_args)
{

    if(TIMER_EVENT_EXPIRED == p_args->event){
       
        //test_port2 = !test_port2;

//        if(wait.stop_mode.ble > 0) wait.stop_mode.ble--;     //未使用なのでコメントアウト
//        if(wait.stop_mode.scom > 0) wait.stop_mode.scom--;
//        if(wait.stop_mode.pow > 0) wait.stop_mode.pow--;
//        if(wait.stop_mode.psoc_restert > 0) wait.stop_mode.psoc_restert--;
        if(wait.sfm > 0) wait.sfm--;
//        if(wait.usb.comm > 0) wait.usb.comm--;
//        if(wait.usb.tx > 0) wait.usb.tx--;
//        if(wait.ble_comm > 0) wait.ble_comm--;     //未使用なのでコメントアウト
//        if(wait.scom_comm > 0) wait.scom_comm--;
//        if(wait.opt_comm > 0) wait.opt_comm--;
        if(wait.rfm_comm > 0) wait.rfm_comm--;
//        if(wait.r500_comm > 0) wait.r500_comm--;
        if(wait.channel_busy > 0) wait.channel_busy--;
        if(wait.radio_icon_indicate > 0) wait.radio_icon_indicate--;
        if(wait.inproc_rfm > 0) wait.inproc_rfm--;
        if(wait.sfm > 0)wait.sfm--;


        /* 新基板で定義のこと
        if(BLE_CONNECTION == DISCONNECT) wait.stop_mode.ble = 0;    // コネクション中でなければ解除
        else connection_postponement_stop_mode = true;              // コネクション中か、コネクション終了後０ｘ９Ｅコマンドを受付前

        if(wait.stop_mode.pow < WAIT_30MSEC)                    // ＳＴＯＰモードに入らないようにするタイマー
        {
            if(BLE_ROLE == ROLE_CLIENT) wait.stop_mode.pow = WAIT_30MSEC;   // ＢＬＥがクライアント（セントラル）の場合
            if(USB_VBUS == VBUS_HIGH) wait.stop_mode.pow = WAIT_30MSEC;     // ＵＳＢバスパワーがある場合
            if(EXT_POW == POW_HIGH) wait.stop_mode.pow = WAIT_30MSEC;       // 外部電源がある場合
            if(OPT_POWSW == POWSW_ON) wait.stop_mode.pow = WAIT_30MSEC;     // 光ポート電源がＯＮの場合
        }
         */

    }

 
}


/**
 * TIMER2 callback 1ms インターバル
 * ???sec までカウント
 * @param p_args
 */
void timer2_callback(timer_callback_args_t * p_args)
{
    if(TIMER_EVENT_EXPIRED == p_args->event){
        //ms1_port = !ms1_port;
        if(wait.ms_1.ble_reset_tm > 0){
            g_ioport.p_api->pinWrite(PORT_BLE_RESET, IOPORT_LEVEL_LOW);
            wait.ms_1.ble_reset_tm--;
        }
        else{
            g_ioport.p_api->pinWrite(PORT_BLE_RESET, IOPORT_LEVEL_HIGH);
        }

        if(wait.ms_1.rf_wait > 0){
            wait.ms_1.rf_wait--;
        }
        if(wait.ms_1.sfm_wait > 0){
            wait.ms_1.sfm_wait--;
        }
        else{
            sfm_flag = 1;
        }
            
    }

}
