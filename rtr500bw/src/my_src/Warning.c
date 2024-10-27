/**
 * @file    Warning.c
 *
 * @date    Created on: 2019/09/09
 * @author  tooru.hayashi
 * @note	2020.01.30 改行コードがおかしいので修正
 * @note	2020.01.30	v6ソースマージ済み
 * @note    2020.Jul.01 GitHub 630ソース反映済み
 * @note	2020.Jul.27 GitHub 0722ソース反映済み
 * @note	2020.Aug.07	0807ソース反映済み
 */


#define _WARNING_C_

#include "Version.h"
#include "flag_def.h"
#include "MyDefine.h"
#include "Sfmem.h"
#include "Globals.h"
#include "Config.h"
#include "General.h"
#include "DateTime.h"
#include "Xml.h"
#include "Convert.h"
#include "Lang.h"
#include "Log.h"
#include "Warning.h"
#include "Rfunc.h"
//#include "led_thread_entry.h"
//#include "http_thread.h"
//#include "auto_thread_entry.h"		// sakaguchi 2021.03.01


#define	WR_CH1_UP		BIT(0)		// Ch1が警報状態
#define	WR_CH1_DOWN		BIT(1)		// Ch1が警報状態から復帰

#define	WR_CH2_UP		BIT(2)
#define	WR_CH2_DOWN		BIT(3)

#define	WR_CH3_UP		BIT(4)
#define	WR_CH3_DOWN		BIT(5)

#define	WR_CH4_UP		BIT(6)
#define	WR_CH4_DOWN		BIT(7)

#define	WR_CH5_UP		BIT(8)
#define	WR_CH5_DOWN		BIT(9)

#define	WR_CH6_UP		BIT(10)
#define	WR_CH6_DOWN		BIT(11)

#define	WR_SENSR_UP		BIT(16)
#define	WR_SENSR_DOWN	BIT(17)

#define	WR_RF_UP		BIT(18)
#define	WR_RF_DOWN		BIT(19)

#define	WR_BAT_UP		BIT(20)
#define	WR_BAT_DOWN		BIT(21)

#define	WR_SW_UP		BIT(22)
#define	WR_SW_DOWN		BIT(23)

#define	WR_SENSR2_UP	BIT(24)		// CH3,CH4 センサエラー sakaguchi 2021.04.07
#define	WR_SENSR2_DOWN	BIT(25)		// CH3,CH4 センサエラー sakaguchi 2021.04.07

#define	WR_TEMP_UP		( WR_CH1_UP | WR_CH2_UP | WR_CH3_UP | WR_CH4_UP | WR_CH5_UP | WR_CH6_UP )
#define	WR_TEMP_DOWN	( WR_CH1_DOWN | WR_CH2_DOWN | WR_CH3_DOWN | WR_CH4_DOWN  | WR_CH5_DOWN | WR_CH6_DOWN )

#define	WR_OTHER_UP		( WR_SENSR_UP | WR_RF_UP | WR_BAT_UP )
#define	WR_OTHER_DOWN	( WR_SENSR_DOWN | WR_RF_DOWN | WR_BAT_DOWN )

#define	WR_BASE			( WR_SW_UP | WR_SW_DOWN )
#define	WR_BASE_OTHER	( WR_TEMP_UP | WR_TEMP_DOWN | WR_OTHER_UP | WR_OTHER_DOWN )

#define	WR_END		0xFFFFFFFF

// Warning State
//#define WS_UNKOWN	0
//#define WS_WARNING	1
//#define WS_NORMAL	2
//#define WS_RECOVER	3

// Warning State
#define WS_UNKOWN	0
#define WS_NORMAL	1		// 正常 : (警報なし、復帰)
#define WS_NORMAL2	2		// 正常 : (前回の警報監視から今回の警報監視の間に警報発生したが今は正常)

#define WS_WARN		10		// 警報の種類の区別なしの警報
#define WS_HIGH		11
#define WS_LOW		12
#define WS_SENSER	13
#define WS_OVER		16
#define WS_UNDER	17
#define WS_P_RISE	14
#define WS_P_FALL	15


/**
 * 警報メール構造体
 */
typedef struct {
	union	{
		uint16_t		All;
		struct {
			unsigned int		UpLo:1;			// 上下限値
			unsigned int		Sensor:1;		// 子機のセンサ
			unsigned int		RF:1;			// 子機との無線通信
			unsigned int		Battery:1;		// 子機バッテリ
			unsigned int		Input:1;		// 接点入力ON
			unsigned int		Test:1;			// TESTコマンド時
			unsigned int		gap:10;			// 16bitの余り分
		} Active;
	} SendFlag;

} OBJ_MLFLG;

static uint8_t w_state;
static uint8_t w_type;
//static uint8_t w_flag;                    // sakaguchi 2021.01.13 未使用のため削除




//プロトタイプ
//uint16_t MakeWarningBody( int16_t Cmd, int16_t Adrs, uint32_t GSec, uint32_t Warn );
uint32_t CheckWarning( int16_t Num, uint16_t *Cur, uint16_t *Now, uint16_t *News );



//static void WarningPort(uint16_t warnCur, uint16_t warnNow, uint16_t warnNew);
void WarningPort(uint16_t warnCur, uint16_t warnNow, uint16_t warnNew);



//未使用static uint32_t CheckWarning2( int16_t Num );
//未使用static char *MakeWarningStruct( char *Dst, def_w_config *Cfg, int32_t Channel, int32_t Type );
int GetWarningValue( def_w_config *Cfg, int32_t Channel, int32_t Type );
static void GetWarningTime(void);
static void GetWarningBattLevel(def_w_config *Cfg);					// sakaguchi 2021.01.13 // sakaguchi 2021.01.18
uint16_t CheckWarningMask(OBJ_MLFLG Send, int Adrs);
static void MakeWarningXML( int32_t Test, uint32_t Status, uint32_t pat );
void SendWarningTest(void);

void MakeChannelTag( int ch, int state );
void SendWarningTestNew(uint32_t pat);

void vgroup_read(void);
int get_vgroup_name(uint32_t SerialCode);
static uint16_t get_judgetime(uint8_t jtime);


/**
 *
 * @param Test
 * @return
 */
int SendWarningFile(int Test, uint32_t pat) 
{
	//SSP_PARAMETER_NOT_USED(Test);       //未使用引
	ULONG actual_events;
	UINT status;
	uint32_t warn, flag;
	uint16_t warnNow, warnCur, tmp1, tmp2, tmp3;
	int cnt;
//	uint32_t GSec;
	char fname[300];

    warn = 0;

	//if(Test == 0){
	if((Test == 0) || (Test == 3)){			// sakaguchi 2021.03.09

        warnNow = warnCur = 0;

        for(cnt = 0; cnt < (int)DIMSIZE(auto_control.w_config); cnt++)
        {
            flag = CheckWarning((int16_t)cnt, &tmp1, &tmp2, &tmp3 );
            if(flag != WR_END){
                warn |= flag;
                warnCur |= tmp1;
                warnNow |= tmp2;
				Printf("\r\n## warn = %04X warnCur = %04X warnNow = %04X / flag = %04X (%d)\r\n", warn,warnCur,warnNow,flag, cnt);
            }
            else{
                break;
            }
        }
		WarningPort((uint16_t)warnCur, (uint16_t)warnNow, (uint16_t)warn);
		WarningPort_backup((uint16_t)warnCur, (uint16_t)warnNow, (uint16_t)warn);			// 警報状態のレジスタをバックアップされるSRAMに保存する	2020/09/03 segi

	}

	if(my_config.warning.Route[0] == 3){		// HTTPで警報送信
		sprintf(fname, "*B-*Y*m*d-*H*i*s.xml");
	
		GetFormatString( 0, XMLName, fname, 128, 0 );	// 書式変換

		XML.Create(0);

		if(Test == 3){							// 警報監視１回目	// sakaguchi 2021.03.09
			MakeWarningXML( Test, warnNow ,pat);
		}else{
		MakeWarningXML( Test, warn ,pat);
		}
		Printf(" Warning XML File size = %ld\r\n", strlen(xml_buffer));

		status = tx_event_flags_get (&event_flags_http, (ULONG)0xffffffff,TX_OR_CLEAR, &actual_events,TX_NO_WAIT);
		G_HttpFile[FILE_W].sndflg = SND_ON;									// [FILE]現在値データ送信：ON
		tx_event_flags_set (&event_flags_http, FLG_HTTP_READY, TX_OR);		// HTTPスレッド動作許可:ON

    	status = tx_event_flags_get (&event_flags_http, (ULONG)(FLG_HTTP_SUCCESS | FLG_HTTP_ERROR | FLG_HTTP_CANCEL),TX_OR_CLEAR, &actual_events,TX_WAIT_FOREVER);

		Printf(">>>>>> Warning Send End  %04X \r\n", actual_events);
    	Printf(" status =  %d \n" ,status);     //仮 2020.01.21

		//if(actual_events & FLG_SEND_ERROR){			// sakaguchi 2021.03.10
		if((actual_events & FLG_SEND_ERROR) || (actual_events & FLG_HTTP_CANCEL)){
		    PutLog( LOG_LAN, "$W:$E");
			DebugLog(LOG_DBG, "$W:$E %d", status);
	    	return (-1);
		}
		else{
			PutLog( LOG_LAN, "$W:$S");
			return (0);
		}
	}
	else{								// 警報送信しない
		Printf(">>>>>> Warning No Send \r\n");
		return (0);
	}
}


/**
 * @brief   警報LEDと接点出力の状態を保存	2020/09/03	segi
 * @return  警報  
*/
void WarningPort_backup(uint16_t cur,uint16_t now,uint16_t warn_sts)
{
	auto_control.backup_cur = cur;
	auto_control.backup_now = now;
	auto_control.backup_warn = warn_sts;
	auto_control.backup_sum = auto_control.backup_cur + auto_control.backup_now + auto_control.backup_warn;
}



/**
 * @brief   警報データ チェック &　LED点灯
 * @return  警報  
*/
uint32_t WarningCheck_and_LED(void)
{
	uint32_t warn, flag;
	uint16_t warnNow, warnCur, tmp1, tmp2, tmp3;
	int cnt;

	warn = 0;

	warnNow = warnCur = 0;

    for(cnt = 0; cnt < (int)DIMSIZE(auto_control.w_config); cnt++)
    {
        flag = CheckWarning((int16_t)cnt, &tmp1, &tmp2, &tmp3 );
        if(flag != WR_END){
            warn |= flag;
            warnCur |= tmp1;
             warnNow |= tmp2;
			Printf("\r\n## warn = %04X warnCur = %04X warnNow = %04X / flag = %04X (%d)\r\n", warn,warnCur,warnNow,flag, cnt);
        }
        else{
            break;
        }
    }
	
	WarningPort((uint16_t)warnCur, (uint16_t)warnNow, (uint16_t)warn);

	return(warn);

}


/**
 * @brief   警報データ ch->warnig 作成
 * @param ch
 * @param state	警報状態	警報発生 正常（警報なし、復帰）	正常（過去に警報発生したが今は正常）
 * 			type	0	不明				
					1	上限値警報				
					2	下限値警報				
					3	センサエラー				
					4	測定範囲外(Over)				
					5	測定範囲外(Under)				
					6	パルスイベント(立ち上がり)				
					7	パルスイベント(立ち下がり)				

 * 				
 * 			send	ture or false	
 * 
*/
void MakeChannelTag( int ch, int state )
{
    uint8_t L_limit = 0;            // 警報下限値の出力有無 // sakaguchi 2021.01.13
    uint8_t U_limit = 0;            // 警報上限値の出力有無 // sakaguchi 2021.01.13

	SSP_PARAMETER_NOT_USED(state);

	XML.OpenTag("ch");
		XML.TagOnce("num", "%ld",ch+0);
		XML.TagOnce("name", "" );			// CH名
// sakaguchi 2021.03.01 ↓
		if( my_config.websrv.Mode[0] != 0 ){					// DataServer以外
			XML.TagOnce("state", "%d", state);
		}else{
// 2023.08.04 ↓
			if((FirstAlarmMoni == 1) && (my_config.websrv.Mode[0] == 0)){   // 警報監視（初回）かつ 接続先がDataServerの場合
				WarnVal.warnType = (uint8_t)state;		// 警報種別は警報状態で上書き
			}
// 2023.08.04  ↑

			if( WS_WARN <= state )	WarnVal.warnType = (uint8_t)state;		// 警報発生中なら警報種別は警報状態で上書き
			
			sprintf( XMLTemp, "warnType=\"%d\"", WarnVal.warnType );		// 警報種別
			XML.TagOnceAttrib( "state", XMLTemp, "%d", state );				// 警報状態
		}
// sakaguchi 2021.04.07 ↓
		// 警報が一度も発生していない
		if( memcmp(WarnVal.Val, "No Data", strlen("No Data")) == 0 ){
			if( my_config.websrv.Mode[0] == 0 ){							// 送信先がDataServer
				if(WarnVal.AttribCode){
					XML.TagOnce("type", "%u", WarnVal.AttribCode);			// データ種別
				}else{
					XML.TagOnce("type", "");
				}
			}
			XML.TagOnce("event_unixtime", "");								// 警報発生時刻
			XML.TagOnce("id" 		,"%ld", WarnVal.id);					// 警報ID
			XML.TagOnce("value", "");										// 警報値
		}
		// 立ち上がり・立ち下がり警報発生時
		else if(( memcmp(WarnVal.Val, "Rising Edge", strlen("Rising Edge")) == 0 ) || ( memcmp(WarnVal.Val, "Falling Edge", strlen("Falling Edge")) == 0 )){
			if(WarnVal.utime == 0xFFFF){
				XML.TagOnce("event_unixtime" ,"");							// 警報発生時刻（警報経過秒がMAXの場合は空）
			}else{
				XML.TagOnce("event_unixtime" ,"%ld", WarnVal.utime);		// 警報発生時刻
			}
			XML.TagOnce("id" 		,"%ld", WarnVal.id);					// 警報ID

			if( my_config.websrv.Mode[0] != 0 ){							// 送信先がDataServer以外
				XML.TagOnce("value", "%.16s", WarnVal.Val);
			}else{
				if(WarnVal.AttribCode){
					XML.TagOnce("type", "%u", WarnVal.AttribCode);			// データ種別
				}else{
					XML.TagOnce("type", "");
				}
				XML.TagOnce("value", "%.16s", WarnVal.Val);					// 警報値
			}
		}
		// 警報発生時
		else if(WarnVal.utime){
			if(WarnVal.utime == 0xFFFF){
				XML.TagOnce("event_unixtime" ,"");							// 警報発生時刻（警報経過秒がMAXの場合は空）
			}else{
				XML.TagOnce("event_unixtime" ,"%ld", WarnVal.utime);		// 警報発生時刻
			}
			XML.TagOnce("id" 		,"%ld", WarnVal.id);					// 警報ID

			if( my_config.websrv.Mode[0] != 0 ){							// 送信先がDataServer以外
				XML.TagOnce("value", "%.16s", WarnVal.Val);
			}else{
				if(WarnVal.AttribCode){
					XML.TagOnce("type", "%u", WarnVal.AttribCode);			// データ種別
				}else{
					XML.TagOnce("type", "");
				}
				sprintf( XMLTemp, "raw=\"%ld\"", WarnVal.Val_raw );				// 警報値（素データ）
				XML.TagOnceAttrib( "value", XMLTemp, "%.16s", WarnVal.Val );	// 警報値
			}
		}
		// 警報復帰時
		else{
			if( my_config.websrv.Mode[0] == 0 ){							// 送信先がDataServer
				if(WarnVal.AttribCode){
					XML.TagOnce("type", "%u", WarnVal.AttribCode);			// データ種別
				}else{
					XML.TagOnce("type", "");
				}
			}
			XML.TagOnce("event_unixtime", "");								// 警報発生時刻
			XML.TagOnce("id" 		,"%ld", WarnVal.id);					// 警報ID
			XML.TagOnce("value", "");										// 警報値
		}
// sakaguchi 2021.04.07 ↑
// sakaguchi 2021.03.01 ↑
// sakaguchi 2021.04.07 ↓
		//XML.TagOnce("unit",  "%.64s", WarnVal.Unit);
		//if((WarnVal.AttribCode == 0x004D) || (WarnVal.AttribCode == 0x004E)){	// 立ち上がり、立ち下がり警報
		if(WarnVal.edge){						// 立ち上がり、立ち下がり警報	// sakaguchi 2021.04.16
			XML.TagOnce("unit", "");
			XML.TagOnce("upper_limit", "");
			XML.TagOnce("lower_limit", "");
		}else{
			//XML.Plain("<unit>%.64s</unit>\n", WarnVal.Unit);
			XML.TagOnce2("unit", "%.64s", WarnVal.Unit);				// エスケープ処理追加	// sakaguchi 2021.04.16
	// sakaguchi 2021.01.13 ↓
			//XML.TagOnce("upper_limit", "%.16s", WarnVal.U_limit);
			//XML.TagOnce("lower_limit", "%.16s", WarnVal.L_limit);
			switch(ch){
				case 1:
					L_limit = (WarnVal.limit_flag & 0x01);
					U_limit = (WarnVal.limit_flag & 0x02);
					break;
				case 2:
					L_limit = (WarnVal.limit_flag & 0x04);
					U_limit = (WarnVal.limit_flag & 0x08);
					break;
				case 3:
					L_limit = (WarnVal.limit_flag & 0x10);
					U_limit = (WarnVal.limit_flag & 0x20);
					break;
				case 4:
					L_limit = (WarnVal.limit_flag & 0x40);
					U_limit = (WarnVal.limit_flag & 0x80);
					break;
				case 5:
					U_limit = (WarnVal.limit_flag & 0x01);				// 積算照度
					break;
				case 6:
					U_limit = (WarnVal.limit_flag & 0x02);				// 積算紫外線量
					break;
				default:
					break;
			}
			if(U_limit != 0){
				if( my_config.websrv.Mode[0] != 0 ){					// DataServer以外	// sakaguchi 2021.03.01
					XML.TagOnce("upper_limit", "%.16s", WarnVal.U_limit);
				}else{
					sprintf( XMLTemp, "raw=\"%ld\"", WarnVal.U_limit_raw );
					XML.TagOnceAttrib( "upper_limit", XMLTemp, "%.16s", WarnVal.U_limit );
				}
			}else{
				XML.TagOnce("upper_limit", "");     // 警報判定無しの場合は空要素となる
			}

			if(L_limit != 0){
				if( my_config.websrv.Mode[0] != 0 ){					// DataServer以外	// sakaguchi 2021.03.01
					XML.TagOnce("lower_limit", "%.16s", WarnVal.L_limit);
				}else{
					sprintf( XMLTemp, "raw=\"%ld\"", WarnVal.L_limit_raw );
					XML.TagOnceAttrib( "lower_limit", XMLTemp, "%.16s", WarnVal.L_limit );
				}
			}else{
				XML.TagOnce("lower_limit", "");     // 警報判定無しの場合は空要素となる
			}
		}
// sakaguchi 2021.01.13 ↑
// sakaguchi 2021.04.07 ↑
		XML.TagOnce("judgement_time", "%.16s", WarnVal.Judge_time);
	XML.CloseTag();		
}

/**
 *	警報の接点出力
 * @param warnCur   警報状態 未使用
 * @param warnNow	on_offによって現状を示した物
 * @param warnNew   未使用
 */
//static void WarningPort(uint16_t warnCur, uint16_t warnNow, uint16_t warnNew)
void WarningPort(uint16_t warnCur, uint16_t warnNow, uint16_t warnNew)
{
	uint8_t mask;
	uint32_t now;

	/*
	now = CurrentWarning ;			// SRAMから読む

	Printf("\r\n Warning Port 2 warnCur = %04X warnNow = %04X warnNew = %04X now = %04X\r\n", warnCur,warnNow,warnNew, now);
	if(warnNow != 0xFFFF)			// 初期化以外
		now = warnCur;

	CurrentWarning = now;			// 記憶
	*/


	now = warnNow;
	
	if(now){
		if(warnCur != warnNow){			// 監視間隔内の警報のOn-Offが有った場合		// sakaguchi 2021.04.07
			// LED点滅無し
		}else{
			LED_Set( LED_WARNING, LED_ON );
		}
	}
	else{
		LED_Set( LED_WARNING, LED_OFF );
	}

	

	mask = my_config.warning.Ext[0];
	Printf("Warning Port mask = %02X now = %04X \r\n", mask, now);

	if ( mask & WR_MASK_HILO ){
		now &= ~( BIT(0) | BIT(1) | BIT(8) | BIT(9) | BIT(12) | BIT(13) );
		Printf("\r\n Mask HILO");
	}
	if ( mask & WR_MASK_SENSOR ){
		now &= ~( BIT(4) );
		Printf("\r\n Mask SEN");
	}
	if ( mask & WR_MASK_RF ){
		now &= ~( BIT(5) );
		Printf("\r\n Mask RF");
	}
	if ( mask & WR_MASK_BATT ){
		now &= ~( BIT(6) );
		Printf("\r\n Mask BATT");
	}
	if ( mask & WR_MASK_SENSOR ){		// CH3,CH4センサーエラー sakaguchi 2021.04.07
		now &= ~( BIT(10) );
		Printf("\r\n Mask SEN2");
	}

	if(now){
		EX_WRAN_ON;
	}
	else{
		EX_WRAN_OFF;
	}

	Printf("\r\n Warning Port 2 warnCur = %04X warnNow = %04X warnNew = %04X now = %04X\r\n", warnCur,warnNow,warnNew, now);
	if(warnCur != warnNow){		// 監視間隔内の警報のOn-Offが有った場合
		warn_time_flag = 1;
		EX_WRAN_ON;
		Printf("\r\n ### Warning On-Off\r\n");
	}
}



/**
 * @brief   警報データ作成・送信
 * @param Test
 * @param   Status	0:警報無し 1:警報有り
 * @note    2020.Feb.05 わーにぐ潰し済み
 * 			500BWの場合、親機の警報は
 * @bug chに値をセットしていない とりあえず初期値0のみ追加
 */
static void MakeWarningXML( int32_t Test, uint32_t Status, uint32_t pat )
{
	uint8_t	num;
	int16_t		grp, unit, ch = 0, vch;
	int ch_max;
	uint16_t	spec,  now, cur, news, temp_unit=0, type=0;
	uint32_t	warn, utime;
	def_w_config	*cfg;
	
	int temp;
	int len,i;
	int state = 0;												// sakaguchi 2021.03.01
	char	name[sizeof(my_config.device.Name)+2];				// sakaguchi add 2020.08.27
	uint8_t		write_no = 0;									// sakaguchi 2021.03.09
	uint32_t	GSec = 0;										// サマータイム判定用			// DataServer対応 sakaguchi 2021.04.07
    uint8_t		L_limit = 0;									// 警報下限値の出力有無
    uint8_t		U_limit = 0;									// 警報上限値の出力有無
    uint8_t		limiter_flag = 0;								// 警報監視条件フラグ
    uint8_t		sekisan_flag = 0;								// 警報監視条件フラグ（積算）
	int16_t 	method;

	// type 0 : 警報監視開始後の１回目
	//		1 : イベント通
	//		2 : 送信テスト

	//send.SendFlag.All = 0;
	//mask.SendFlag.All = CheckWarningMask( mask, 0 );		// 対象子機のマスクチェック
	Printf("Make Warning XML  \r\n");

	type = 1;

	if((FirstAlarmMoni == 1) && (my_config.websrv.Mode[0] == 0)){	// 接続先がDataServer かつ 警報監視１回目 // sakaguchi 2021.03.01
		type = 0;
	}

	if ( Test == 1) {				// テスト送信？
		SendWarningTest();
	}
	else if( Test == 2){
		SendWarningTestNew(pat);
	}
	else
	{
		vgroup_read();					// 表示グループ情報取得

		temp_unit = (uint16_t)my_config.device.TempUnits[0];	            // oC(0) or oF(1)

		sprintf(XMLTForm,"%s", my_config.device.TimeForm);		    // 日付フォーマット

		if( my_config.websrv.Mode[0] == 0 ){						// DataServer // sakaguchi 2021.03.01
			XML.Plain( XML_FORM_WARN_DATASRV );				        // <file .. name=" まで
		}else{
		XML.Plain( XML_FORM_WARN );				                // <file .. name=" まで
		}
		//XML.Entity( XML.FileName );					                // ファイル名（エンコード付き）
		//XML.Entity( "Warning.xml" );					                // ファイル名（エンコード付き）  debug
		XML.Plain( "%s\"", XML.FileName);
		XML.Plain( " type=\"%d\"", type);     

		memset(name, 0x00, sizeof(name));
		memcpy(name, my_config.device.Name, sizeof(my_config.device.Name));
		sprintf(XMLTemp,"%s", name);
		XML.Plain( " author=\"%s\">\n", XMLTemp);  

		XML.OpenTag( "header" );
		
			utime = RTC_GetGMTSec();                // UTC取得
			XML.TagOnce( "created_unixtime", "%lu", utime );	// 現在値の時刻
			temp = (int)( RTC.Diff / 60 );		        	        // 時差
			XML.TagOnce( "time_diff", "%d", temp );
// sakaguchi 2021.03.01 ↓
			if( my_config.websrv.Mode[0] != 0 ){					// DataServer以外
				XML.TagOnce( "std_bias", "0" );			                // 標準時間のオフセット
				temp = (int)( RTC.AdjustSec / 60 );		                // サマータイム中
				XML.TagOnce( "dst_bias", "%d", temp );					// サマータイムのオフセット
			}else{
				sprintf( XMLTemp, "select=\"%d\"", 1 );					// 標準時間/サマータイム（select属性)

				GSec = RTC_GetGMTSec();
				GSec = GSec + (uint32_t)RTC.Diff;
				if (GetSummerAdjust( GSec ) == 0){						// 標準時間で動作 // sakaguchi 2021.04.07
//				if( RTC.SummerBase == 0 ){								// 標準時間で動作
					XML.TagOnceAttrib( "std_bias", XMLTemp, "%d", 0 );		// 標準時間のオフセット（select属性付）
					temp = (int)( RTC.AdjustSec / 60 );		                // サマータイム中
					XML.TagOnce( "dst_bias", "%d", temp );					// サマータイムのオフセット

				}else{		// 夏時間で動作
					XML.TagOnce( "std_bias", "0" );			                // 標準時間のオフセット
					temp = (int)( RTC.AdjustSec / 60 );		                // サマータイム中
					XML.TagOnceAttrib( "dst_bias", XMLTemp, "%d", temp );	// サマータイムのオフセット（select属性付）
				}
			}
// sakaguchi 2021.03.01 ↑
		
			//XML.Plain( "<time_zone>%s</time_zone>\n", my_config.device.TimeZone);
			//XML.TagOnce( "time_zone", my_config.device.TimeZone );  // タイムゾーン名
		//GetFormatString( 0, XMLName, temp, 64, 0 );	// 書式変換
			XML.OpenTag( "base_unit" );
				sprintf(XMLTemp, "%08lX", fact_config.SerialNumber);
				XML.TagOnce( "serial", XMLTemp );
// 2023.05.31 ↓
				switch(fact_config.Vender){
					case VENDER_TD:
						XML.TagOnce( "model", UNIT_BASE_TANDD /*BaseUnitName*/ );
						break;
					case VENDER_ESPEC:
						XML.TagOnce( "model", UNIT_BASE_ESPEC /*BaseUnitName*/ );
						break;
					case VENDER_HIT:
						XML.TagOnce( "model", UNIT_BASE_HITACHI /*BaseUnitName*/ );
						break;
					default:
						XML.TagOnce( "model", UNIT_BASE_TANDD /*BaseUnitName*/ );
						break;
				}
// 2023.05.31 ↑
				//XML.Plain( "<name>%s</name>\n", my_config.device.Name);	// 親機名称
				XML.Plain( "<name>%s</name>\n", name);	 					// 親機名称		// sakaguchi cg 2020.08.27
				//XML.TagOnce( "name", my_config.device.Name );

				// 警報の有りなしをチェック
				if(Status){
					XML.TagOnce( "warning", "%d", 1 );
				}	
				else{
					XML.TagOnce( "warning", "%d", 0 );	
				}
			XML.CloseTag();		// <base_unit>

		XML.CloseTag();		// <header>

		

		for ( num=0; num<DIMSIZE(auto_control.w_config) ; num++ ) {

// 2023.08.03 ↓ 親機警報はないため除外する
			if((auto_control.w_config[num].group_no == 0) && (auto_control.w_config[num].unit_no == 0))
			{
				continue;
			}
// 2023.08.03 ↑

			warn = CheckWarning( num, &cur, &now, &news ) ;
			Printf("## Check Warning %04X %d / cur:%04X  now:%04X  news:%04X \r\n", warn, num, cur, now, news);	
			//warn = CheckWarning2( num ) ;
			//Printf("## Check Warning2 %04X %d \r\n", warn, num);	
			if ( warn == WR_END ){
				break;	
			}

			cfg = (def_w_config *)&auto_control.w_config[num];

			//if ( flag & WR_TEMP_UP ) {		// CH1 ～ CH6 発生		
			
			if ( grp != (int)auto_control.w_config[num].group_no ) {
				grp = (int)auto_control.w_config[num].group_no;
				get_regist_group_adr( (uint8_t)grp );				// グループ情報取得 regist.group.size[0]～
				Printf("  Group Name <%.8s> \r\n", group_registry.name );
				
			}

			unit = auto_control.w_config[num].unit_no;
			Printf("  Unit Number <%d> \r\n", unit );

			get_regist_relay_inf((uint8_t)grp, (uint8_t)unit, rpt_sequence);	//get_unit_no_info( grp, unit );			// 子機情報取得 regist.unit.size[0]～

			get_vgroup_name(ru_registry.rtr501.serial);
			
			//}

// sakaguchi 2021.03.09 ↓	// DataServer対応 sakaguchi 2021.04.07 ↓
			if( (Test == 3) && (unit != 0) && ((ru_registry.rtr501.set_flag & 0x01) == 1) ){		// 警報監視1回目 かつ　警報監視対象子機

				if( WR_chk( (uint8_t)grp, (uint8_t)unit, 1 ) != 0 ){	// 今回無線通信が失敗
					warn |= WR_RF_UP;									// 無線通信エラー
				}else{
					spec = GetSpecNumber( ru_registry.rtr501.serial , ru_registry.rtr505.model );	// 子機のシリアル番号から機種コード取得
					if(spec == SPEC_574){
						limiter_flag = ru_registry.rtr574.limiter_flag;									// 警報監視条件フラグ
						sekisan_flag = ru_registry.rtr574.sekisan_flag;									// 警報監視条件フラグ
					}
					else if(spec == SPEC_576){
						limiter_flag = ru_registry.rtr576.limiter_flag;									// 警報監視条件フラグ
					}
					else{
						limiter_flag = ru_registry.rtr501.limiter_flag;									// 警報監視条件フラグ
					}

					if( spec == SPEC_576 ){
						if(limiter_flag & 0x03){		// ＣＨ１警報監視有り
							if(now & BIT(0)){				// 現在のＣＨ１警報情報をチェック
								warn |= WR_CH1_UP;			// CH1警報
								warn &= ~(WR_CH1_DOWN);
							}else{
								warn |= WR_CH1_DOWN;		// CH1正常
								warn &= ~(WR_CH1_UP);
							}
						}
						else{
							warn &= ~(WR_CH1_UP | WR_CH1_DOWN);
						}
						if(limiter_flag & 0x30){		// ＣＨ３警報監視有り
							if(now & BIT(8)){				// 現在のＣＨ３警報情報をチェック
								warn |= WR_CH3_UP;			// CH3警報
								warn &= ~(WR_CH3_DOWN);
							}else{
								warn |= WR_CH3_DOWN;		// CH3正常
								warn &= ~(WR_CH3_UP);
							}
						}
						else{
							warn &= ~(WR_CH3_UP | WR_CH3_DOWN);
						}
						if(limiter_flag & 0xC0){		// ＣＨ４警報監視有り
							if(now & BIT(9)){				// 現在のＣＨ４警報情報をチェック
								warn |= WR_CH4_UP;			// CH4警報
								warn &= ~(WR_CH4_DOWN);
							}else{
								warn |= WR_CH4_DOWN;		// CH4正常
								warn &= ~(WR_CH4_UP);
							}
						}
						else{
							warn &= ~(WR_CH4_UP | WR_CH4_DOWN);
						}
					}
					else{
						ch_max = GetChannelNumber(spec);	// 子機の機種コードからチャンネル数を取得
						switch(ch_max){
							case 6:
								if(sekisan_flag & 0x02){		// ＣＨ６警報監視有り
									if(now & BIT(13)){				// 現在のＣｈ６警報情報をチェック
										warn |= WR_CH6_UP;			// CH6警報 ↓ Ch5へ続く
										warn &= ~(WR_CH6_DOWN);
									}else{
										warn |= WR_CH6_DOWN;		// CH6正常 ↓ Ch5へ続く
										warn &= ~(WR_CH6_UP);
									}
								}
								else{
									warn &= ~(WR_CH6_UP | WR_CH6_DOWN);
								}
							case 5:
								if(sekisan_flag & 0x01){		// ＣＨ５警報監視有り
									if(now & BIT(12)){				// 現在のＣＨ５警報情報をチェック
										warn |= WR_CH5_UP;			// CH5警報 ↓ Ch4へ続く
										warn &= ~(WR_CH5_DOWN);
									}else{
										warn |= WR_CH5_DOWN;		// CH5復旧 ↓ Ch4へ続く
										warn &= ~(WR_CH5_UP);
									}
								}
								else{
									warn &= ~(WR_CH5_UP | WR_CH5_DOWN);
								}
							case 4:
								if(limiter_flag & 0xC0){		// ＣＨ４警報監視有り
									if(now & BIT(9)){				// 現在のＣＨ４警報情報をチェック
										warn |= WR_CH4_UP;			// CH4警報 ↓ Ch3へ続く
										warn &= ~(WR_CH4_DOWN);
									}else{
										warn |= WR_CH4_DOWN;		// CH4復旧 ↓ Ch3へ続く
										warn &= ~(WR_CH4_UP);
									}
								}
								else{
									warn &= ~(WR_CH4_UP | WR_CH4_DOWN);
								}
							case 3:
								if(limiter_flag & 0x30){		// ＣＨ３警報監視有り
									if(now & BIT(8)){				// 現在のＣＨ３警報情報をチェック
										warn |= WR_CH3_UP;			// CH3警報 ↓ Ch2へ続く
										warn &= ~(WR_CH3_DOWN);
									}else{
										warn |= WR_CH3_DOWN;		// CH3正常 ↓ Ch2へ続く
										warn &= ~(WR_CH3_UP);
									}
								}
								else{
									warn &= ~(WR_CH3_UP | WR_CH3_DOWN);
								}
							case 2:
								if(limiter_flag & 0x0C){		// ＣＨ２警報監視有り
									if(now & BIT(1)){				// 現在のＣＨ２警報情報をチェック
										warn |= WR_CH2_UP;			// CH2警報 ↓ Ch1へ続く
										warn &= ~(WR_CH2_DOWN);
									}else{
										warn |= WR_CH2_DOWN;		// CH2正常 ↓ Ch1へ続く
										warn &= ~(WR_CH2_UP);
									}
								}
								else{
									warn &= ~(WR_CH2_UP | WR_CH2_DOWN);
								}
							case 1:
								if(limiter_flag & 0x03){		// ＣＨ１警報監視有り
									if(now & BIT(0)){				// 現在のＣＨ１警報情報をチェック
										warn |= WR_CH1_UP;			// CH1警報
										warn &= ~(WR_CH1_DOWN);
									}else{
										warn |= WR_CH1_DOWN;		// CH1正常
										warn &= ~(WR_CH1_UP);
									}
								}
								else{
									warn &= ~(WR_CH1_UP | WR_CH1_DOWN);
								}
								break;
							default:
								break;
						}
					}

					if(( spec == SPEC_574 ) || ( spec == SPEC_576 )){
						// センサエラーの警報変化がある場合、ＣＨ情報は出力しない（ＣＨ警報ビットはＯＦＦ）
						if((warn & (WR_SENSR_UP | WR_SENSR_DOWN)) != 0 ){
							warn &= ~(WR_CH1_UP | WR_CH1_DOWN | WR_CH2_UP | WR_CH2_DOWN );
						}
						if((warn & (WR_SENSR2_UP | WR_SENSR2_DOWN)) != 0 ){
							warn &= ~(WR_CH3_UP | WR_CH3_DOWN | WR_CH4_UP | WR_CH4_DOWN );
						}
						// センサエラーが現在も継続中の場合、ＣＨ情報は出力しない（ＣＨ警報ビットはＯＦＦ）
						if((now & BIT(4)) != 0 ){
							warn &= ~(WR_CH1_UP | WR_CH1_DOWN | WR_CH2_UP | WR_CH2_DOWN );
							warn |= WR_SENSR_UP;
						}
						if((now & BIT(10)) != 0 ){
							warn &= ~(WR_CH3_UP | WR_CH3_DOWN | WR_CH4_UP | WR_CH4_DOWN );
							warn |= WR_SENSR2_UP;
						}
					}
					warn |= WR_RF_DOWN;		// 無線通信正常
				}
			}

			write_no = ALM_bak_poi((uint8_t)grp, (uint8_t)unit);			// 警報監視データのバックアップから読み出し
// sakaguchi 2021.03.09 ↑	// DataServer対応 sakaguchi 2021.04.07 ↑

			if(warn != 0){
				Printf("Waring flag %04X  (%08X)\r\n", warn, ru_registry.rtr501.serial);

				XML.OpenTag( "device" );
				XML.TagOnce( "serial", "%08lX",  ru_registry.rtr501.serial);
				
				spec = GetSpecNumber( ru_registry.rtr501.serial , ru_registry.rtr505.model );	// 子機のシリアル番号から機種コード取得
				ch_max = GetChannelNumber(spec);                //子機の機種コードからチャンネル数を取得
				Printf("  Spec Number <%d>  (%02X)\r\n", spec, ru_registry.rtr505.model );
				Printf("  Ch = <%d> \r\n", ch_max );

				XML.TagOnce( "model", "%.32s", GetModelName( ru_registry.rtr501.serial, spec ) );	// モデル名
				
				//XML.TagOnce( "name", "%.32s", ru_registry.rtr501.ble.name);
				//XML.TagOnce( "group", "%.8s", group_registry.name );
				//XML.Plain("<name>%.32s</name>\n",ru_registry.rtr501.ble.name);
				XML.Plain("<name>%.26s</name>\n",ru_registry.rtr501.ble.name);					// sakaguchi 2021.05.17
				XML.Plain("<group>%.32s</group>\n",group_registry.name);
				if(strlen(VGroup.Name) != 0)
					//XML.Plain("<alias_group>%s</alias_group>\n", VGroup.Name);
					XML.TagOnce2( "alias_group", VGroup.Name );				// エスケープ処理追加	// sakaguchi 2021.04.16

				if(warn & (/*WR_SENSR_UP | WR_SENSR_DOWN |*/ WR_TEMP_UP | WR_TEMP_DOWN)){
					Printf("%04X \r\n",warn & (WR_CH1_UP | WR_CH1_DOWN));
					if((warn & (WR_CH1_UP | WR_CH1_DOWN)) == (WR_CH1_UP | WR_CH1_DOWN)){
						//state = GetWarningValue(cfg, 0, temp_unit);
						WarnVal.warnType = (uint8_t)GetWarningValue(cfg, 0, temp_unit);	// sakaguchi 2021.04.13
						state = WS_NORMAL2;
						MakeChannelTag( 1, state );						// sakaguchi 2021.03.01
						ALM_bak[write_no].warntype[0] = (uint8_t)state;		// 警報種別を保存	// sakaguchi 2021.03.09
					}
					else{ 
						if(warn & WR_CH1_UP){
							state = GetWarningValue(cfg, 0, temp_unit);
							MakeChannelTag( 1, state);
							ALM_bak[write_no].warntype[0] = (uint8_t)state;	// 警報種別を保存	// sakaguchi 2021.03.09
						}
						else if(warn & WR_CH1_DOWN){
							GetWarningValue(cfg, 0, temp_unit);	
							state = WS_NORMAL;
							MakeChannelTag( 1, state );					// sakaguchi 2021.03.01
							ALM_bak[write_no].warntype[0] = (uint8_t)state;	// 警報種別を保存	// sakaguchi 2021.03.09
						}
					}

					if((warn & (WR_CH2_UP | WR_CH2_DOWN)) == (WR_CH2_UP | WR_CH2_DOWN)){
						//state = GetWarningValue(cfg, 1, temp_unit);
						WarnVal.warnType = (uint8_t)GetWarningValue(cfg, 1, temp_unit);	// sakaguchi 2021.04.13
						state = WS_NORMAL2;
						MakeChannelTag( 2, state );						// sakaguchi 2021.03.01
						ALM_bak[write_no].warntype[1] = (uint8_t)state;		// 警報種別を保存	// sakaguchi 2021.03.09
					}
					else{ 
						if(warn & WR_CH2_UP){
							state = GetWarningValue(cfg, 1, temp_unit);
							MakeChannelTag( 2, state);
							ALM_bak[write_no].warntype[1] = (uint8_t)state;	// 警報種別を保存	// sakaguchi 2021.03.09
						}
						else if(warn & WR_CH2_DOWN){
							GetWarningValue(cfg, 1, temp_unit);		
							state = WS_NORMAL;
							MakeChannelTag( 2, state );					// sakaguchi 2021.03.01
							ALM_bak[write_no].warntype[1] = (uint8_t)state;	// 警報種別を保存	// sakaguchi 2021.03.09
						}
					}


					if((warn & (WR_CH3_UP | WR_CH3_DOWN)) == (WR_CH3_UP | WR_CH3_DOWN)){
						//state = GetWarningValue(cfg, 2, temp_unit);
						WarnVal.warnType = (uint8_t)GetWarningValue(cfg, 2, temp_unit);	// sakaguchi 2021.04.13
						if(spec == SPEC_576)
							vch = 2;
						else
							vch = 3;
						state = WS_NORMAL2;
						MakeChannelTag( vch, state );					// sakaguchi 2021.03.01
						ALM_bak[write_no].warntype[2] = (uint8_t)state;		// 警報種別を保存	// sakaguchi 2021.03.09
					}
					else{
						if(spec == SPEC_576)
							vch = 2;
						else
							vch = 3;
						if(warn & WR_CH3_UP){	
							state = GetWarningValue(cfg, 2, temp_unit);
							MakeChannelTag( vch, state);
							ALM_bak[write_no].warntype[2] = (uint8_t)state;	// 警報種別を保存	// sakaguchi 2021.03.09
						}
						else if(warn & WR_CH3_DOWN){
							GetWarningValue(cfg, 2, temp_unit);	
							state = WS_NORMAL;
							MakeChannelTag( vch, state );				// sakaguchi 2021.03.01
							ALM_bak[write_no].warntype[2] = (uint8_t)state;	// 警報種別を保存	// sakaguchi 2021.03.09
						}
					}

					if((warn & (WR_CH4_UP | WR_CH4_DOWN)) == (WR_CH4_UP | WR_CH4_DOWN)){
						if(spec == SPEC_576)
							vch = 3;
						else
							vch = 4;
						//state = GetWarningValue(cfg, 3, temp_unit);
						WarnVal.warnType = (uint8_t)GetWarningValue(cfg, 3, temp_unit);	// sakaguchi 2021.04.13
						state = WS_NORMAL2;
						MakeChannelTag( vch, state );					// sakaguchi 2021.03.01
						ALM_bak[write_no].warntype[3] = (uint8_t)state;		// 警報種別を保存	// sakaguchi 2021.03.09
					}
					else{
						if(spec == SPEC_576)
							vch = 3;
						else
							vch = 4;
						if(warn & WR_CH4_UP){
							state = GetWarningValue(cfg, 3, temp_unit);
							MakeChannelTag( vch, state);
							ALM_bak[write_no].warntype[3] = (uint8_t)state;	// 警報種別を保存	// sakaguchi 2021.03.09
						}
						else if(warn & WR_CH4_DOWN){
							GetWarningValue(cfg, 3, temp_unit);	
							state = WS_NORMAL;
							MakeChannelTag( vch, state );				// sakaguchi 2021.03.01
							ALM_bak[write_no].warntype[3] = (uint8_t)state;	// 警報種別を保存	// sakaguchi 2021.03.09
						}
					}

					if((warn & (WR_CH5_UP | WR_CH5_DOWN)) == (WR_CH5_UP | WR_CH5_DOWN)){
						//state = GetWarningValue(cfg, 0, temp_unit);
						//state = GetWarningValue(cfg, 4, temp_unit);			// CH番号が誤っている // sakaguchi 2021.03.09
						WarnVal.warnType = (uint8_t)GetWarningValue(cfg, 4, temp_unit);	// sakaguchi 2021.04.13
						state = WS_NORMAL2;
						MakeChannelTag( 5, state );						// sakaguchi 2021.03.01
						ALM_bak[write_no].warntype[4] = WS_NORMAL;		// 積算警報に復帰はないため警報種別は正常に戻す // sakaguchi 2021.04.07
					}
					else{
						if(warn & WR_CH5_UP){	
							state = GetWarningValue(cfg, 4, 0);
							MakeChannelTag( 5, state);
							ALM_bak[write_no].warntype[4] = WS_NORMAL;		// 積算警報に復帰はないため警報種別は正常に戻す // sakaguchi 2021.04.07
						}
						else if(warn & WR_CH5_DOWN){
							GetWarningValue(cfg, 4, 0);	
							state = WS_NORMAL;
							MakeChannelTag( 5, state );					// sakaguchi 2021.03.01
							ALM_bak[write_no].warntype[4] = WS_NORMAL;		// 積算警報に復帰はないため警報種別は正常に戻す // sakaguchi 2021.04.07
						}
					}

					if((warn & (WR_CH6_UP | WR_CH6_DOWN)) == (WR_CH6_UP | WR_CH6_DOWN)){
						//state = GetWarningValue(cfg, 5, temp_unit);
						WarnVal.warnType = (uint8_t)GetWarningValue(cfg, 5, temp_unit);	// sakaguchi 2021.04.13
						state = WS_NORMAL2;
						MakeChannelTag( 6, state );						// sakaguchi 2021.03.01
						ALM_bak[write_no].warntype[5] = WS_NORMAL;		// 積算警報に復帰はないため警報種別は正常に戻す // sakaguchi 2021.04.07
					}
					else{
						if(warn & WR_CH6_UP){
							state = GetWarningValue(cfg, 5, 0);
							MakeChannelTag( 6, state);
							ALM_bak[write_no].warntype[5] = WS_NORMAL;		// 積算警報に復帰はないため警報種別は正常に戻す // sakaguchi 2021.04.07
						}
						else if(warn & WR_CH6_DOWN){
							GetWarningValue(cfg, 5, 0);	
							state = WS_NORMAL;
							MakeChannelTag( 6, state );					// sakaguchi 2021.03.01
							ALM_bak[write_no].warntype[5] = WS_NORMAL;		// 積算警報に復帰はないため警報種別は正常に戻す // sakaguchi 2021.04.07
						}
					}
				}

				// 以降の時間を取得する為
				//GetWarningValue(cfg, 0, temp_unit);		
				GetWarningTime();
// sakaguchi 2021.04.07 ↓
				// 574,576のセンサエラー出力
				if((spec == SPEC_574) || (spec == SPEC_576)){

					// CH1,CH2のセンサエラー判定
					if(warn & (WR_SENSR_UP | WR_SENSR_DOWN)){

						memset(WarnVal.Val, 0x00, sizeof(WarnVal.Val));

						limiter_flag = ru_registry.rtr576.limiter_flag;									// 警報監視条件フラグ

						if(warn & WR_SENSR_UP){
							state = WS_SENSER;
							WarnVal.warnType = WS_SENSER;
							sprintf(WarnVal.Val,"Sensor Error");
							if(!WarnVal.Val_raw)	WarnVal.Val_raw = 0xeeee;		// 警報値（素データ）

						}else{
							state = WS_NORMAL;
//							if(WarnVal.warnType == 1)	WarnVal.warnType = WS_UNKOWN;		// 不明	// 2023.08.03 不明は使用しない
							sprintf(WarnVal.Val,"%s","");
						}

						for(ch=1; ch<=2; ch++){

							if((spec == SPEC_576) && (ch==2)){
								break;	// 576はCH1で終了
							}

							if((ch == 1) && ((limiter_flag & 0x03) == 0 )){
								continue;	// CH1が警報監視対象でないためスキップ
							}

							if((ch == 2) && ((limiter_flag & 0x0C) == 0 )){
								continue;	// CH2が警報監視対象でないためスキップ
							}

							if((ch == 1) && (warn & (WR_CH1_UP | WR_CH1_DOWN))){
								continue;	// CH警報が既に出力済みの場合はスキップ
							}
							if((ch == 2) && (warn & (WR_CH2_UP | WR_CH2_DOWN))){
								continue;	// CH警報が既に出力済みの場合はスキップ
							}

							// チャンネル属性、単位を取得
							WarnVal.AttribCode = ru_registry.rtr576.attribute[ch-1];
							method = GetConvertMethod( (uint8_t)WarnVal.AttribCode, (uint8_t)temp_unit, 0 );
							sprintf( WarnVal.Unit, "%s", MeTable[method].TypeChar );

							// 警報判定時間を取得
							sprintf(WarnVal.Judge_time, "%d", get_judgetime(ru_registry.rtr576.judge_time[ch-1]));

							XML.OpenTag("ch");
								XML.TagOnce("num", "%ld", ch);
								XML.TagOnce("name", "" );				// CH名

								if( my_config.websrv.Mode[0] != 0 ){					// DataServer以外
									XML.TagOnce("state" ,"%d", state);
								}else{
									sprintf( XMLTemp, "warnType=\"%d\"", WarnVal.warnType );		// 警報種別
									XML.TagOnceAttrib( "state", XMLTemp, "%d", state );				// 警報状態
									if(WarnVal.AttribCode){
										XML.TagOnce("type", "%u", WarnVal.AttribCode);	// データ種別
									}else{
										XML.TagOnce("type", "");						// データ種別
									}
								}

								if(warn & WR_SENSR_UP){
									XML.TagOnce("event_unixtime" ,"%ld", WarnVal.utime);
									XML.TagOnce("id","");					// id は空
								}else{
									XML.TagOnce("event_unixtime" ,"");
									XML.TagOnce("id","%ld", WarnVal.id);
								}

								if( my_config.websrv.Mode[0] != 0 ){					// DataServer以外
									XML.TagOnce("value", "%s", WarnVal.Val);
								}else{
									if(strlen(WarnVal.Val)){
										sprintf( XMLTemp, "raw=\"%ld\"", WarnVal.Val_raw );				// 警報値（素データ）
										XML.TagOnceAttrib( "value", XMLTemp, "%.16s", WarnVal.Val );	// 警報値
									}else{
										XML.TagOnce("value", "%s", WarnVal.Val);
									}
								}
								//XML.TagOnce("unit",  "");
								//XML.Plain("<unit>%.64s</unit>\n", WarnVal.Unit);
								XML.TagOnce2("unit", "%.64s", WarnVal.Unit);		// エスケープ処理追加	// sakaguchi 2021.04.16

								if(ch == 1){
									L_limit = (limiter_flag & 0x01);
									U_limit = (limiter_flag & 0x02);
									WarnVal.L_limit_raw = (uint32_t)ru_registry.rtr576.limiter1[0];
									WarnVal.U_limit_raw = (uint32_t)ru_registry.rtr576.limiter1[1];
								}else{
									L_limit = (limiter_flag & 0x04);
									U_limit = (limiter_flag & 0x08);
									WarnVal.L_limit_raw = (uint32_t)ru_registry.rtr576.limiter2[0];
									WarnVal.U_limit_raw = (uint32_t)ru_registry.rtr576.limiter2[1];
								}

								MeTable[method].Convert( WarnVal.L_limit, WarnVal.L_limit_raw);
								MeTable[method].Convert( WarnVal.U_limit, WarnVal.U_limit_raw);

								if(U_limit != 0){
									if( my_config.websrv.Mode[0] != 0 ){					// DataServer以外
										XML.TagOnce("upper_limit", "%.16s", WarnVal.U_limit);
									}else{
										sprintf( XMLTemp, "raw=\"%ld\"", WarnVal.U_limit_raw );
										XML.TagOnceAttrib( "upper_limit", XMLTemp, "%.16s", WarnVal.U_limit );
									}
								}else{
									XML.TagOnce("upper_limit", "");     // 警報判定無しの場合は空要素となる
								}

								if(L_limit != 0){
									if( my_config.websrv.Mode[0] != 0 ){					// DataServer以外
										XML.TagOnce("lower_limit", "%.16s", WarnVal.L_limit);
									}else{
										sprintf( XMLTemp, "raw=\"%ld\"", WarnVal.L_limit_raw );
										XML.TagOnceAttrib( "lower_limit", XMLTemp, "%.16s", WarnVal.L_limit );
									}
								}else{
									XML.TagOnce("lower_limit", "");     // 警報判定無しの場合は空要素となる
								}
								XML.TagOnce("judgement_time", "%.16s", WarnVal.Judge_time);

							XML.CloseTag();
							ALM_bak[write_no].warntype[ch-1] = (uint8_t)state;		// 警報種別を保存
						}
					}

					// CH3,CH4のセンサエラー判定
					if(warn & (WR_SENSR2_UP | WR_SENSR2_DOWN)){

						memset(WarnVal.Val, 0x00, sizeof(WarnVal.Val));
						limiter_flag = ru_registry.rtr576.limiter_flag;									// 警報監視条件フラグ

						if(warn & WR_SENSR2_UP){
							state = WS_SENSER;
							WarnVal.warnType = WS_SENSER;
							sprintf(WarnVal.Val,"Sensor Error");
							if(!WarnVal.Val_raw)	WarnVal.Val_raw = 0xeeee;		// 警報値（素データ）
						}else{
							state = WS_NORMAL;
//							if(WarnVal.warnType == 1)	WarnVal.warnType = WS_UNKOWN;		// 不明	// 2023.08.03 不明は使用しない
							sprintf(WarnVal.Val,"%s","");
						}

						for(ch=3; ch<=4; ch++){

							if((ch == 3) && ((limiter_flag & 0x30) == 0 )){
								continue;	// CH3が警報監視対象でないためスキップ
							}

							if((ch == 4) && ((limiter_flag & 0xC0) == 0 )){
								continue;	// CH4が警報監視対象でないためスキップ
							}

							if((ch == 3) && (warn & (WR_CH3_UP | WR_CH3_DOWN))){
								continue;	// CH警報が既に出力済みの場合はスキップ
							}
							if((ch == 4) && (warn & (WR_CH4_UP | WR_CH4_DOWN))){
								continue;	// CH警報が既に出力済みの場合はスキップ
							}

							// チャンネル属性、単位を取得
							WarnVal.AttribCode = ru_registry.rtr576.attribute[ch-1];
							method = GetConvertMethod( (uint8_t)WarnVal.AttribCode, (uint8_t)temp_unit, 0 );
							sprintf( WarnVal.Unit, "%s", MeTable[method].TypeChar );

							// 警報判定時間を取得
							sprintf(WarnVal.Judge_time, "%d", get_judgetime(ru_registry.rtr576.judge_time[ch-1]));

							XML.OpenTag("ch");
								if(spec == SPEC_576){					// 576はCHを補正
									XML.TagOnce("num", "%ld", ch-1);
								}else{
									XML.TagOnce("num", "%ld", ch);
								}
								XML.TagOnce("name", "" );				// CH名

								if( my_config.websrv.Mode[0] != 0 ){					// DataServer以外
									XML.TagOnce("state" ,"%d", state);
								}else{
									sprintf( XMLTemp, "warnType=\"%d\"", WarnVal.warnType );		// 警報種別
									XML.TagOnceAttrib( "state", XMLTemp, "%d", state );				// 警報状態
									if(WarnVal.AttribCode){
										XML.TagOnce("type", "%u", WarnVal.AttribCode);	// データ種別
									}else{
										XML.TagOnce("type", "");						// データ種別
									}
								}

								if(warn & WR_SENSR2_UP){
									XML.TagOnce("event_unixtime" ,"%ld", WarnVal.utime);
									XML.TagOnce("id","");					// id は空
								}else{
									XML.TagOnce("event_unixtime" ,"");
									XML.TagOnce("id","%ld", WarnVal.id);
								}

								if( my_config.websrv.Mode[0] != 0 ){					// DataServer以外
									XML.TagOnce("value", "%s", WarnVal.Val);
								}else{
									if(strlen(WarnVal.Val)){
										sprintf( XMLTemp, "raw=\"%ld\"", WarnVal.Val_raw );				// 警報値（素データ）
										XML.TagOnceAttrib( "value", XMLTemp, "%.16s", WarnVal.Val );	// 警報値
									}else{
										XML.TagOnce("value", "%s", WarnVal.Val);
									}
								}

								//XML.TagOnce("unit",  "");
								//XML.Plain("<unit>%.64s</unit>\n", WarnVal.Unit);
								XML.TagOnce2("unit", "%.64s", WarnVal.Unit);		// エスケープ処理追加	// sakaguchi 2021.04.16

								if(ch == 3){
									L_limit = (limiter_flag & 0x10);
									U_limit = (limiter_flag & 0x20);
									WarnVal.L_limit_raw = (uint32_t)ru_registry.rtr576.limiter3[0];
									WarnVal.U_limit_raw = (uint32_t)ru_registry.rtr576.limiter3[1];
								}else{
									L_limit = (limiter_flag & 0x40);
									U_limit = (limiter_flag & 0x80);
									WarnVal.L_limit_raw = (uint32_t)ru_registry.rtr576.limiter4[0];
									WarnVal.U_limit_raw = (uint32_t)ru_registry.rtr576.limiter4[1];
								}

								MeTable[method].Convert( WarnVal.L_limit, WarnVal.L_limit_raw);
								MeTable[method].Convert( WarnVal.U_limit, WarnVal.U_limit_raw);

								if(U_limit != 0){
									if( my_config.websrv.Mode[0] != 0 ){					// DataServer以外
										XML.TagOnce("upper_limit", "%.16s", WarnVal.U_limit);
									}else{
										sprintf( XMLTemp, "raw=\"%ld\"", WarnVal.U_limit_raw );
										XML.TagOnceAttrib( "upper_limit", XMLTemp, "%.16s", WarnVal.U_limit );
									}
								}else{
									XML.TagOnce("upper_limit", "");     // 警報判定無しの場合は空要素となる
								}

								if(L_limit != 0){
									if( my_config.websrv.Mode[0] != 0 ){					// DataServer以外
										XML.TagOnce("lower_limit", "%.16s", WarnVal.L_limit);
									}else{
										sprintf( XMLTemp, "raw=\"%ld\"", WarnVal.L_limit_raw );
										XML.TagOnceAttrib( "lower_limit", XMLTemp, "%.16s", WarnVal.L_limit );
									}
								}else{
									XML.TagOnce("lower_limit", "");     // 警報判定無しの場合は空要素となる
								}
								XML.TagOnce("judgement_time", "%.16s", WarnVal.Judge_time);

							XML.CloseTag();
							ALM_bak[write_no].warntype[ch-1] = (uint8_t)state;		// 警報種別を保存
						}
					}

				}

				//if((spec == SPEC_574) || (spec == SPEC_576)){
					//if(warn & (WR_SENSR_UP | WR_SENSR_DOWN)){
						//XML.OpenTag("ch");
							//XML.TagOnce("num", "%ld",ch+1);
							//XML.TagOnce("name", "" );			// CH名
							//if(warn & WR_SENSR_UP){
								//XML.TagOnce("state" ,"%d", 13);
								//state = 13;								// sakaguchi 2021.03.01
							//}else{
								//XML.TagOnce("state" ,"%d", 1);
								//state = 1;								// sakaguchi 2021.03.01
							//}
							//XML.TagOnce("event_unixtime" ,"%ld", WarnVal.utime);
							//XML.TagOnce("id" 		,"%ld", WarnVal.id);
							//XML.TagOnce("value", "");
							//XML.TagOnce("unit",  "");
							//XML.TagOnce("upper_limit", "");
							//XML.TagOnce("lower_limit", "");
							////XML.TagOnce("value", "%.16s", WarnVal.Val);
							////XML.TagOnce("unit",  "%.16s", WarnVal.Unit);
							////XML.TagOnce("upper_limit", WarnVal.U_limit);
							////XML.TagOnce("lower_limit", WarnVal.L_limit);
							////XML.TagOnce("judgement_time", WarnVal.Judge_time);
					
						//XML.CloseTag();		
					//}
				//}
// sakaguchi 2021.04.07 ↑

				//WR_set((uint8_t)grp, (uint8_t)unit, 5, (uint8_t)state);		// 警報種別を格納	// sakaguchi 2021.03.09 del

				if(warn & (WR_BAT_UP | WR_BAT_DOWN)){
					GetWarningBattLevel(cfg);				// バッテリーレベル取得 // sakaguchi 2021.01.13 // sakaguchi 2021.01.18
					XML.OpenTag("battery");
					if(warn & WR_BAT_UP)
						XML.TagOnce("state" ,"%d", 10);
					else
						XML.TagOnce("state" ,"%d", 1);
					XML.TagOnce("event_unixtime" ,"%ld", WarnVal.utime);
					XML.TagOnce("level" ,"%d", WarnVal.batt);
					XML.CloseTag();		// battery
				}
				else{	// 警報が無いとき
					/*
					XML.OpenTag("battery");
						XML.TagOnce("state" ,"%d", 1);				
						XML.TagOnce("event_unixtime" ,"%ld", WarnVal.utime);
						XML.TagOnce("level" ,"%d", WarnVal.batt);
					XML.CloseTag();		// battery
					*/
					;
				}
				
				if(warn & (WR_RF_UP | WR_RF_DOWN)){
					XML.OpenTag("comm");
					if(warn & WR_RF_UP)
						XML.TagOnce("state" ,"%d", 10);
					else
						XML.TagOnce("state" ,"%d", 1);
					//GetWarningValue(cfg, 0, temp_unit);							
					XML.TagOnce("event_unixtime" ,"%ld", WarnVal.utime);
					XML.TagOnce("type" ,"%d", 0);
					XML.CloseTag();		// rf_comm
				}
				else{
					/*
					XML.OpenTag("comm");				
						XML.TagOnce("state" ,"%d", 1);				
						XML.TagOnce("event_unixtime" ,"%ld", 0);
						XML.TagOnce("type" ,"%d", 0);
					XML.CloseTag();		// rf comm
					*/
					;
				}



				XML.CloseTag();		// device
			}
		}


		XML.CloseAll();

		XML.Plain( "</file>\n" );

		XML.Output();					// バッファ吐き出

		len = (int)strlen(xml_buffer);
		Printf("====> XML File size = %ld\r\n\r\n", strlen(xml_buffer));
		for(i=0;i<len;i++){
			Printf("%c",xml_buffer[i]);
		}

		Printf("\r\n");

	}

	
}





/**
 * @brief   警報チェック
 * Curは警報状態、Nowはon_offによって現状を示した物
 * @param Num
 * @param Cur   警報状態
 * @param Now   on_offによって現状を示した物
 * @param News
 * @return
 */
uint32_t CheckWarning( int16_t Num, uint16_t *Cur, uint16_t *Now, uint16_t *News )
{
	uint32_t	Warn = 0;
	uint16_t	before, now, on_off, change;

	if ( auto_control.w_config[Num].group_no != 0xFF && auto_control.w_config[Num].unit_no != 0xFF ) {


        now    = *(uint16_t *)auto_control.w_config[Num].now;       // 現在      キャストに変更 2020.01.21
        before = *(uint16_t *)auto_control.w_config[Num].before;    // 前回      キャストに変更 2020.01.21
        on_off = *(uint16_t *)auto_control.w_config[Num].on_off;    // 発生+解除フラグ      キャストに変更 2020.01.21

		change = now ^ before;			// 0->1 , 1->0

		Printf("\r\n## now = %04X before = %04X on_off = %04X  change = %04X \r\n", now,before,on_off,change);

		//now &= ( BIT(0) | BIT(1) | BIT(4) | BIT(5) | BIT(6) | BIT(8) | BIT(9) | BIT(12) | BIT(13) );	// 一応マスク
		now &= ( BIT(0) | BIT(1) | BIT(4) | BIT(5) | BIT(6) | BIT(8) | BIT(9) | BIT(10) | BIT(12) | BIT(13) );	// 一応マスク	// BIT(10)=CH3,Ch4のセンサエラー追加 sakaguchi 2021.04.07
		*Cur = now;

		*News = ( change & now );/* | on_off;*/	// ONへ変化したもの ＝ 新規警報あり

		if ( change & BIT(0) ) {			// CH1警報
			if ( now & on_off & BIT(0) ) {
				Warn |= WR_CH1_UP | WR_CH1_DOWN;	// 同時にセット
				now = (uint16_t)(now & ~BIT(0));		// CH1 現状はOFF
			}
			else if ( !( on_off & BIT(0) ) ){
			    Warn |= ( now & BIT(0) ) ? WR_CH1_UP : WR_CH1_DOWN;
			}
		}

		if ( change & BIT(1) ) {			// CH2警報
			if ( now & on_off & BIT(1) ) {
				Warn |= WR_CH2_UP | WR_CH2_DOWN;	// 同時にセット
				now = (uint16_t)(now & ~BIT(1));		// CH2 現状はOFF
			}
			else if ( !( on_off & BIT(1) ) ){
			    Warn |= ( now & BIT(1) ) ? WR_CH2_UP : WR_CH2_DOWN;
			}
		}

		if ( change & BIT(4) ){			// センサー警報
			Warn |= ( now & BIT(4) ) ? WR_SENSR_UP : WR_SENSR_DOWN;
		}
		if ( change & BIT(5) ){			// 無線警報
			Warn |= ( now & BIT(5) ) ? WR_RF_UP : WR_RF_DOWN;
		}
		if ( change & BIT(6) ){			// バッテリ警報
			Warn |= ( now & BIT(6) ) ? WR_BAT_UP : WR_BAT_DOWN;
		}
		if ( change & BIT(8) ) {			// CH3警報
			if ( now & on_off & BIT(8) ) {
				Warn |= WR_CH3_UP | WR_CH3_DOWN;	// 同時にセット
				now = (uint16_t)(now & ~BIT(8));		// CH3 現状はOFF
			}
			else if ( !( on_off & BIT(8) ) ){
			    Warn |= ( now & BIT(8) ) ? WR_CH3_UP : WR_CH3_DOWN;
			}
		}

		if ( change & BIT(9) ) {			// CH4警報
			if ( now & on_off & BIT(9) ) {
				Warn |= WR_CH4_UP | WR_CH4_DOWN;	// 同時にセット
				now = (uint16_t)(now & ~BIT(9));		// CH4 現状はOFF
			}
			else if ( !( on_off & BIT(9) ) ){
			    Warn |= ( now & BIT(9) ) ? WR_CH4_UP : WR_CH4_DOWN;
			}
		}

		if ( change & BIT(10) ){			// センサー警報(CH3,CH4)		// sakaguchi 2021.04.07
			Warn |= ( now & BIT(10) ) ? WR_SENSR2_UP : WR_SENSR2_DOWN;
		}

		if ( change & BIT(12) ){			// CH5警報
			Warn |= ( now & BIT(12) ) ? WR_CH5_UP : WR_CH5_DOWN;
		}
		if ( change & BIT(13) ){			// CH6警報
			Warn |= ( now & BIT(13) ) ? WR_CH6_UP : WR_CH6_DOWN;
		}
		*Now = now;					// 今現在の警報フラグ
	}
	else{
	    Warn = 0xFFFFFFFF;		// これで終了とするので全ビット使うようになった場合は検討しなおし
	}

	return (Warn);				// on_off込み
}

// 2023.05.26 ↓
#if  CONFIG_USE_WARNINGPORT_SET_FROM_W_CONFIG_ON
bool CheckWarning_ON(uint16_t *Cur, uint16_t *Now);
bool CheckWarning_ON(uint16_t *Cur, uint16_t *Now)
{
    int i;

    bool warning_is_ON;
    uint16_t now;
    uint16_t before;
    uint16_t on_off;
    uint16_t _warnCur;
    uint16_t _warnNow;

    _warnCur = 0;
    _warnNow = 0;
    warning_is_ON = false;

    for(i = 0; i < 128/*KK_INFO_MAX*/; i++) {
        if(auto_control.w_config[i].group_no != 0xFF && auto_control.w_config[i].unit_no != 0xFF) {

            now = *(uint16_t*)auto_control.w_config[i].now;       // 現在      キャストに変更 2020.01.21
            before = *(uint16_t*)auto_control.w_config[i].before;       // 前回      キャストに変更 2020.01.21
            on_off = *(uint16_t *)auto_control.w_config[i].on_off;// 発生+解除フラグ      キャストに変更 2020.01.21

//#ifdef BRD_R500BM
//            if(auto_control.w_config[i].group_no == 0x00 && auto_control.w_config[i].unit_no == 0x00) {
//                uint16_t passMask_Oya = ( WR_OYA_AC | WR_OYA_BATT | WR_OYA_INPUT); // (BIT(3) |  BIT(2) | BIT(7))
//                now &= passMask_Oya;
//                before &= passMask_Oya;
//                on_off &= passMask_Oya;
//           }
//#endif

            if(auto_control.w_config[i].group_no != 0x00 && auto_control.w_config[i].unit_no != 0x00) {
                uint16_t passMask_KoKi = ( BIT(0) | BIT(1) |          BIT(4) | BIT(5) | BIT(6) | BIT(8) | BIT(9) | BIT(10) | BIT(12) | BIT(13) );
                now    &= passMask_KoKi;
                before &= passMask_KoKi;
                on_off &= passMask_KoKi;
            }

            uint16_t _onBits;
            _onBits = (uint16_t)now & ((uint16_t)~before) & ((uint16_t)~on_off);     // now Is Set, before Not Set and on_off Not Set -> NEW Warning
            if(_onBits) {
                _warnCur |= _onBits;
                _warnNow |= _onBits;
            }

            _onBits = now & before; // both set -> warning is On
            if(_onBits) {
                _warnCur |= _onBits;
                _warnNow |= _onBits;
            }

        }
    }

    *Cur = _warnCur;
    *Now = _warnNow;

    warning_is_ON = (_warnCur != 0) | (_warnNow != 0);
    return (warning_is_ON);
}
#endif // CONFIG_USE_WARNINGPORT_SET_FROM_W_CONFIG_ON
// 2023.05.26 ↑

// DataServer対応 sakaguchi 2021.04.07 ↓
#if 0
/***********************************************************************************************
 * 警報データの構築
 * @param Dst
 * @param Cfg
 * @param Channel
 * @param Type
 * @return 
 * @see w_state
 *
 **********************************************************************************************/
int GetWarningValue( def_w_config *Cfg, int32_t Channel, int32_t Type )
{
	//int16_t state = WS_UNKOWN;			// 警報状態
	int8_t		edge = 0;
	uint8_t 	scale,attrib, grp, unit, add = 0;
	uint16_t	spec, data, data2, time, u_limit,l_limit;
//	char limiter_flag;
	int16_t 	method;
	uint32_t	GSec;
	char		*tmp, *scale_unit = 0;
	uint16_t 	*warn, *past;
	int32_t ch = Channel;

	WARN_FORM	*Form;
    uint8_t     tmp_limit_flag = 0;                                 // sakaguchi 2021.01.13
	uint8_t		write_no = 0;										// sakaguchi 2021.03.01
	WARN_FORM	warn_temp;
	
	/*
	memset(WarnVal.Ch_Name, 0x00, strlen(WarnVal.Ch_Name));
	memset(WarnVal.Val,		0x00, strlen(WarnVal.Val));
	memset(WarnVal.Unit,	0x00, strlen(WarnVal.Unit));
	memset(WarnVal.L_limit, 0x00, strlen(WarnVal.L_limit));
	memset(WarnVal.U_limit, 0x00, strlen(WarnVal.U_limit));
	2021.03.18  初期化不具合　strlenの処理では、終わりが見つからない場合がある
	*/
	memset(WarnVal.Ch_Name, 0x00, sizeof(WarnVal.Ch_Name));
	memset(WarnVal.Val,		0x00, sizeof(WarnVal.Val));
	memset(WarnVal.Unit,	0x00, sizeof(WarnVal.Unit));
	memset(WarnVal.L_limit, 0x00, sizeof(WarnVal.L_limit));
	memset(WarnVal.U_limit, 0x00, sizeof(WarnVal.U_limit));
	memset(WarnVal.Judge_time, 0, sizeof(WarnVal.Judge_time));		// 2020.11.18  クリアしていないために、Senserエラー時に文字化けしていた
	//memset(&Form, 0x00, sizeof(WARN_FORM));							// sakaguchi 2021.03.01
	memset(&warn_temp, 0x00, sizeof(WARN_FORM));					// sakaguchi 2021.03.01

	WarnVal.limit_flag = 0;                                         // sakaguchi 2021.01.13
	WarnVal.utime = 0;
	WarnVal.id = 0;
	WarnVal.Val_raw = 0;											// sakaguchi 2021.03.01
	WarnVal.L_limit_raw = 0;										// sakaguchi 2021.03.01
	WarnVal.U_limit_raw = 0;										// sakaguchi 2021.03.01
	WarnVal.AttribCode = 0;											// sakaguchi 2021.03.01
	WarnVal.warnType = 0;											// sakaguchi 2021.03.01

	w_state = 0;

	grp  = Cfg->group_no;
	unit = Cfg->unit_no;

	if ( Channel >= 4 ) {
		add = 1;								// 積算とする
		Form = (WARN_FORM *)rf_ram.auto_format3.kdata;
	}
	else {
		Form = (WARN_FORM *)rf_ram.auto_format2.kdata;
		if ( Channel >= 2 )			// CH3,CH4は子機番号+1
			unit++;
	}

	for ( data=0; data<DIMSIZE(rf_ram.auto_format2.kdata); data++ ) {
		if ( grp == Form->group_no && unit == Form->unit_no ){
		    break;
		}
		else{
		    Form++;
		}
	}
	
// sakaguchi 2021.03.01 ↓
	// 警報監視１回目
	if((FirstAlarmMoni == 1) && (my_config.websrv.Mode[0] == 0)){

		Form = &warn_temp;									// Formを上書き

		write_no = ALM_bak_poi(grp, unit);					// 警報監視データのバックアップから読み出し

		Form->group_no = grp;								// グループ番号

		Form->kind = ALM_bak[write_no].f1_unit_kind;		// 設定ファイルの子機種類(0xFE, 0xFA)
		Form->scale = ALM_bak[write_no].f1_scale_flags;		// スケール変換フラグ
		*(uint16_t *)Form->attrib = *(uint16_t *)&ALM_bak[write_no].f1_ch_zoku[0];			// CH属性
		*(uint32_t *)Form->serial = *(uint32_t *)&ALM_bak[write_no].f1_unit_sirial[0];		// シリアル番号

		Form->kisyu505 = ru_registry.rtr505.model;			// 505機種コード
		Form->disp_unit = ru_registry.rtr505.display_unit;	// 表示単位(505)

		Form->unit_no = unit;								// 子機番号
		Form->rssi = ALM_bak[write_no].f1_rssi;				// RSSIレベル

		switch(Channel){
			case 0:		// CH1
			case 2:		// CH3
				Form->alarm_cnt = ALM_bak[write_no].f2_ALM_count1;		// 警報カウント

				*(uint32_t *)Form->grobal_time = *(uint32_t *)&ALM_bak[write_no].f2_grobal_time[0];			// グローバル時間
				*(uint16_t *)Form->keika_time = *(uint16_t *)&ALM_bak[write_no].f2_data[2];					// 経過秒

				*(uint16_t *)Form->rec.ex501.warn_temp = *(uint16_t *)&ALM_bak[write_no].f2_data[0];		// 警報値
				*(uint16_t *)Form->rec.ex501.past_temp = *(uint16_t *)&Form->keika_time[0];					// 経過秒
				break;
			case 1:		// CH2
			case 3:		// CH4
				Form->alarm_cnt = ALM_bak[write_no].f2_ALM_count2;		// 警報カウント

				*(uint32_t *)Form->grobal_time = *(uint32_t *)&ALM_bak[write_no].f2_grobal_time[0];			// グローバル時間
				*(uint16_t *)Form->keika_time = *(uint16_t *)&ALM_bak[write_no].f2_data[12];				// 経過秒

				*(uint16_t *)Form->rec.ex503.warn_hum = *(uint16_t *)&ALM_bak[write_no].f2_data[10];		// 警報値
				*(uint16_t *)Form->rec.ex503.past_hum = *(uint16_t *)&Form->keika_time[0];					// 経過秒
				break;
			case 4:		// CH5
				Form->alarm_cnt = ALM_bak[write_no].f3_ALM_count1;		// 警報カウント

				*(uint32_t *)Form->grobal_time = *(uint32_t *)&ALM_bak[write_no].f3_grobal_time[0];			// グローバル時間
				*(uint16_t *)Form->keika_time = *(uint16_t *)&ALM_bak[write_no].f3_data[2];					// 経過秒

				*(uint16_t *)Form->rec.ex501.warn_temp = *(uint16_t *)&ALM_bak[write_no].f3_data[0];		// 警報値
				*(uint16_t *)Form->rec.ex501.past_temp = *(uint16_t *)&Form->keika_time[0];					// 経過秒
				break;
			case 5:		// CH6
				Form->alarm_cnt = ALM_bak[write_no].f3_ALM_count2;		// 警報カウント

				*(uint32_t *)Form->grobal_time = *(uint32_t *)&ALM_bak[write_no].f3_grobal_time[0];			// グローバル時間
				*(uint16_t *)Form->keika_time = *(uint16_t *)&ALM_bak[write_no].f3_data[12];				// 経過秒

				*(uint16_t *)Form->rec.ex503.warn_hum = *(uint16_t *)&ALM_bak[write_no].f3_data[10];		// 警報値
				*(uint16_t *)Form->rec.ex503.past_hum = *(uint16_t *)&Form->keika_time[0];					// 経過秒
				break;
			default:
				Form->alarm_cnt = ALM_bak[write_no].f2_ALM_count1;		// 警報カウント

				*(uint32_t *)Form->grobal_time = *(uint32_t *)&ALM_bak[write_no].f2_grobal_time[0];			// グローバル時間
				*(uint16_t *)Form->keika_time = *(uint16_t *)&ALM_bak[write_no].f2_data[2];					// 経過秒

				*(uint16_t *)Form->rec.ex501.warn_temp = *(uint16_t *)&ALM_bak[write_no].f1_data[0];		// 警報値
				*(uint16_t *)Form->rec.ex501.past_temp = *(uint16_t *)&Form->keika_time[0];					// 経過秒
				break;
		}
	}
// sakaguchi 2021.03.01 ↑


	Printf("	group  number %d \r\n", Form->group_no);
	Printf("	format number %d \r\n", Form->format_no);
	Printf("	unit   number %d \r\n", Form->unit_no);
	Printf("	Channel       %d \r\n", Channel);
	Printf("	alram  count  %d \r\n", Form->alarm_cnt);
	Printf("	serial number %08X \r\n",  *(uint32_t *)Form->serial);

	WarnVal.id = (uint32_t)Form->alarm_cnt;


	spec = GetSpecNumber( *(uint32_t *)Form->serial, Form->kisyu505 );  // 子機のシリアル番号 

	Printf("	GetWarningValue() spec %04X\r\n", spec);

	//  1CH 毎に必要な情報を構築
	switch ( spec ) {
	
		case SPEC_505PS:		// 505のパルスだけ特殊
		case SPEC_505BPS:		// 505のパルスだけ特殊

			if ( ( Form->rec_mode & 0b00001100 ) == 0b00000100 ) { // パルスの警報？
				edge = ( Form->rec_mode >> 4 ) & 0b00000111;
				if ( edge == 3 || edge == 7 ){
					edge = 1;					// Rise
				}
				else if ( edge == 2 || edge == 6 ){
					edge = 2;					// Fall
				}
				else{
					edge = 0;
				}
			}
			/* no break */

		case SPEC_501:			// 501/502
		case SPEC_502:
		case SPEC_501B:			// 501B/502B
		case SPEC_502B:

		case SPEC_502PT:

		case SPEC_505PT:		// 505も同じなので
		case SPEC_505TC:
		case SPEC_505V:
		case SPEC_505A:

		case SPEC_505BPT:		// 505Bも同じなので
		case SPEC_505BTC:
		case SPEC_505BV:
		case SPEC_505BA:
		case SPEC_505BLX:

			warn = (uint16_t *)Form->rec.ex501.warn_temp;		// 警報値
			past = (uint16_t *)Form->rec.ex501.past_temp;		// 経過秒数

			break;

		case SPEC_503:
		case SPEC_503B:
		case SPEC_574:///// たまたま一緒なので使っちゃう
		case SPEC_576:///// たまたま一緒なので使っちゃう
		case SPEC_507:///// たまたま一緒なので使っちゃう
		case SPEC_507B:

			Printf("limiter flag %02X\r\n", ru_registry.rtr576.limiter_flag);

			if ( ( Channel % 2 ) == 0 ) {		// CH1, CH3
				warn = (uint16_t *)Form->rec.ex503.warn_temp;
				past = (uint16_t *)Form->rec.ex503.past_temp;
			}
			else {
				warn = (uint16_t *)Form->rec.ex503.warn_hum;
				past = (uint16_t *)Form->rec.ex503.past_hum;
			}
			break;

		default:
			warn = (uint16_t *)Form->rec.ex501.warn_temp;
			past = (uint16_t *)Form->rec.ex501.past_temp;
			break;
	}

	attrib = Form->attrib[ Channel % 2 ];		// CH属性

	time = *past;                               // 経過秒 (10秒で1カウント)
	if ( time == 0xFFFF ){
	    GSec = 0;
	}
	else{
		GSec = (uint32_t)(*(uint32_t *)Form->grobal_time);	//  debug
		Printf("	Warning past time %ld\r\n", GSec);
		if(GSec > 0){										// sakaguchi 2021.03.01
        GSec = (uint32_t)(*(uint32_t *)Form->grobal_time  - (uint32_t)( time * 10UL ));  // Global - 経過秒        2020.01.21 キャストに変更
	}
	}

	Printf("	Warning past time %04X %ld\r\n", time, GSec);

	WarnVal.utime = GSec;

	data = *warn;       // 警報 max値

	// スケール変換処理
	//  get_unit_no_info( grp, unit );			// 505のスケール変換取得のため
	get_regist_relay_inf( grp, unit, rpt_sequence);	// 505のスケール変換取得のため  2019 12 03
	scale = Form->scale & 0x01;	// スケール変換
	if(spec == SPEC_505V || spec == SPEC_505BV || spec == SPEC_505A || spec == SPEC_505BA || spec == SPEC_505PS || spec == SPEC_505BPS){
		if(memcmp(ru_registry.rtr505.scale_setting, "\n\n\n\n", 4) == 0){
			scale = 0;
		}
	}


	/*
	// スケール変換せずに505の電圧[mV]の時[V]固定なら内部でスケール変換
	if ( !scale && spec == SPEC_505V		/// 505のV固定かどうか
		&& ( (Form->disp_unit & 0xF0) == 0xA0 || (Form->disp_unit &0x0F) == 0x0A ) ) {
		strcpy( (char *)ru_registry.rtr505.scale_setting, SCALE_505V );
		scale = 1;
	}
	if ( !scale && spec == SPEC_505BV		/// 505のV固定かどうか
		&& ( (Form->disp_unit & 0xF0) == 0xA0 || (Form->disp_unit &0x0F) == 0x0A ) ) {
		strcpy( (char *)ru_registry.rtr505.scale_setting, SCALE_505V );
		scale = 1;
	}
	*/

	if (spec == SPEC_505BLX){
		if(scale){
			strcpy( ru_registry.rtr505.scale_setting, SCALE_505Lx );		// L5 L10 [dB]
		}
	} 

	// 最後にnullを入れる処理(64バイト時は何もしない)

	if ( scale == 1 ) {
		GetScaleStr( (char *)ru_registry.rtr505.scale_setting, 0 );	// CH毎のスケール変換文字列
		CheckScaleStr( &tmp, &tmp, &scale_unit );
	}

	Printf("<<<  %04X  / %04X / %d / %d / %d>>>\r\n", data, spec, edge, scale, add);
	if ( edge )
	{
		sprintf( WarnVal.Val, " %s%s ", ( edge == 1 ) ? LANG(W_RISE) : LANG(W_FALL), LANG(W_EDGE) );

		w_state = ( edge == 1 ) ? WS_P_RISE : WS_P_FALL;

	}
	else if ( spec == SPEC_505PS || spec == SPEC_505V || spec == SPEC_505A || spec == SPEC_505BPS || spec == SPEC_505BV || spec == SPEC_505BA || spec == SPEC_505BLX)
	{
		Printf("<<<1  %ld  >>>\r\n", data);

		if ( data == 0xF00F ){			// センサエラー
			sprintf( WarnVal.Val, LANG(W_SENSOR) );	//"N/A " );
			//w_state = WS_WARNING; 
			w_state = WS_SENSER;
		}
		else if ( data == 0xF002 ){		// オーバーフロー
			sprintf( WarnVal.Val, LANG(W_OVER) );	//"Over " );
			//w_state = WS_WARNING; 
			w_state = WS_OVER;
		}
		else if ( data == 0xF001 ){		// アンダーフロー
			sprintf( WarnVal.Val, LANG(W_UNDER) );	//"Under " );
			//w_state = WS_WARNING; 
			w_state = WS_UNDER;
		}
		else{
			method = GetConvertMethod( attrib, (uint8_t)Type, scale );	// method
			MeTable[method].Convert( WarnVal.Val, (uint32_t)data );		// 変換
				
			Printf("scale %d (%ld) %s\r\n", scale ,data, WarnVal.Val );
			if ( !scale )
			{
				data2 = (uint16_t)(( Form->interval < 0x80 ) ? Form->interval : Form->interval - 0x80);
				if ( data2 == 0 )	data2 = 30;		// 0は30秒

				if ( spec != SPEC_505PS && spec != SPEC_505BPS){
				    sprintf( WarnVal.Unit, "%s", MeTable[method].TypeChar );	// 単位
				}
				else if ( (Form->interval < 0x80) && Form->interval ){
				    sprintf( WarnVal.Unit, "%s/%umin", MeTable[method].TypeChar, data2 ); // 単位
				}
				else{
				    sprintf( WarnVal.Unit, "%s/%usec", MeTable[method].TypeChar, data2 ); // 単位
				}
				Printf("<<<  uint-1 %s >>>\r\n", WarnVal.Unit);
			}
			else{
// sakaguchi 2021.01.18 ↓
			    //sprintf( WarnVal.Unit, "%.64s", scale_unit );	// 単位
				//Printf("<<<  uint-2 %s >>>\r\n", WarnVal.Unit);
				data2 = (uint16_t)(( Form->interval < 0x80 ) ? Form->interval : Form->interval - 0x80);
				if ( data2 == 0 )	data2 = 30;		// 0は30秒

				if ( spec != SPEC_505PS && spec != SPEC_505BPS){
			    sprintf( WarnVal.Unit, "%.64s", scale_unit );	// 単位
				}
				else if ( (Form->interval < 0x80) && Form->interval ){
					sprintf( WarnVal.Unit, "%.64s/%umin", scale_unit, data2 ); // 単位
				}
				else{
					sprintf( WarnVal.Unit, "%.64s/%usec", scale_unit, data2 ); // 単位
				}
				Printf("<<<  uint-2 %s >>>\r\n", WarnVal.Unit);
// sakaguchi 2021.01.18 ↑
			}
		}
	}
	else if ( spec == SPEC_576 && ( 0xF001UL <= data && data <= 0xF005UL ) )
	{
		sprintf( WarnVal.Val, LANG(W_SENSOR) );			// センサエラー
		//w_state = WS_WARNING;
		w_state = WS_SENSER;
	}
	else if ( data == 0xEEEE || data == 0xF000 ) {		// Up/Lo で 0xEEEE の時
		sprintf( WarnVal.Val, "N/A " );
		//w_state = WS_NORMAL;
		w_state = WS_SENSER;
	}
	else
	{	// 正常値なら値を確認

		method = GetConvertMethod( attrib, (uint8_t)Type, add );		// method
		MeTable[method].Convert( WarnVal.Val, (uint32_t)data );			// 変換
		sprintf( WarnVal.Unit, "%s", MeTable[method].TypeChar );	// 単位
		Printf("<<<2 %ld  >>>\r\n", data);
	}

	Printf("<<<  data %04X  / spec %04X / edge %d / scale %d / add %d / method %04X / attrib %02X >>>\r\n", data, spec, edge, scale, add, method, attrib);
	Printf("<<<  uint-3 %s >>>\r\n", WarnVal.Unit);

	if ( !edge && w_state == 0 ){

		memset(WarnVal.L_limit, 0, sizeof(WarnVal.L_limit));
    	memset(WarnVal.U_limit, 0, sizeof(WarnVal.U_limit));
    	memset(WarnVal.Judge_time, 0, sizeof(WarnVal.Judge_time));

		//if(w_state != 0){
			w_state = WS_NORMAL2;
		//}


		switch ( spec ) {
	
		case SPEC_501:			// 501/502
		case SPEC_502:
		case SPEC_501B:			// 501B/502B
		case SPEC_502B:
		case SPEC_502PT:
		case SPEC_505PT:		// 505も同じなので
		case SPEC_505TC:
		case SPEC_505BPT:		// 505Bも同じなので
		case SPEC_505BTC:

			method = GetConvertMethod( attrib, (uint8_t)Type, add );		// method
			Printf("limiter flag %02X\r\n", ru_registry.rtr501.limiter_flag);
            //w_flag = ru_registry.rtr501.limiter_flag;
            WarnVal.limit_flag = ru_registry.rtr501.limiter_flag;           // sakaguchi 2021.01.13 XML空要素を判定するため上下限判定フラグを保存
            l_limit = ru_registry.rtr501.limiter1[0];
            u_limit = ru_registry.rtr501.limiter1[1];

            if(ru_registry.rtr501.limiter_flag & 0x03){
                MeTable[method].Convert( WarnVal.L_limit, (uint32_t)ru_registry.rtr501.limiter1[0]);			// 変換
                MeTable[method].Convert( WarnVal.U_limit, (uint32_t)ru_registry.rtr501.limiter1[1]);			// 変換
				
				Printf("judge time %d\r\n", ru_registry.rtr501.judge_time[0]);
                sprintf(WarnVal.Judge_time, "%d", get_judgetime(ru_registry.rtr501.judge_time[0]));

                if(ru_registry.rtr501.limiter_flag & 0x01){
                    if(l_limit >= data){
                        w_state = WS_LOW;
                    }
                }
                if(ru_registry.rtr501.limiter_flag & 0x02){
                    if(data >= u_limit){
                        w_state = WS_HIGH;
                    }
                }
			}

			break;


		case SPEC_505PS:		// 505のパルスだけ特殊
		case SPEC_505BPS:		// 505のパルスだけ特殊
		case SPEC_505V:
		case SPEC_505A:
		case SPEC_505BV:
		case SPEC_505BA:
		case SPEC_505BLX:
			method = GetConvertMethod( attrib, (uint8_t)Type, scale );	// method
			Printf("limiter flag %02X\r\n", ru_registry.rtr505.limiter_flag);
            //w_flag = ru_registry.rtr505.limiter_flag;
            WarnVal.limit_flag = ru_registry.rtr505.limiter_flag;       // sakaguchi 2021.01.13 XML空要素を判定するため上下限判定フラグを保存
            l_limit = ru_registry.rtr505.limiter1[0];
            u_limit = ru_registry.rtr505.limiter1[1];

            if(ru_registry.rtr501.limiter_flag & 0x03){
                MeTable[method].Convert( WarnVal.L_limit, (uint32_t)ru_registry.rtr505.limiter1[0]);			// 変換
                MeTable[method].Convert( WarnVal.U_limit, (uint32_t)ru_registry.rtr505.limiter1[1]);			// 変換

				Printf("judge time %d\r\n", ru_registry.rtr501.judge_time[0]);	
				sprintf(WarnVal.Judge_time, "%d", get_judgetime(ru_registry.rtr505.judge_time[0]));

				if(ru_registry.rtr505.limiter_flag & 0x01){
                    if(l_limit >= data){
                        w_state = WS_LOW;
                    }
			    }
			    if(ru_registry.rtr505.limiter_flag & 0x02){
                    if(data >= u_limit){
                        w_state = WS_HIGH;
                    }
			    }
			}
			
			break;

		case SPEC_503:
		case SPEC_503B:
		case SPEC_507:
		case SPEC_507B:

			method = GetConvertMethod( attrib, (uint8_t)Type, add );		// method
			Printf("limiter flag %02X\r\n", ru_registry.rtr501.limiter_flag);
			WarnVal.limit_flag = ru_registry.rtr501.limiter_flag;       // sakaguchi 2021.01.13 XML空要素を判定するため上下限判定フラグを保存
			
			if (Channel == 0 ) {		// CH1
                l_limit = ru_registry.rtr501.limiter1[0];
                u_limit = ru_registry.rtr501.limiter1[1];
			
                if(ru_registry.rtr501.limiter_flag & 0x03){
                    MeTable[method].Convert( WarnVal.L_limit, (uint32_t)ru_registry.rtr501.limiter1[0]);			// 変換
                    MeTable[method].Convert( WarnVal.U_limit, (uint32_t)ru_registry.rtr501.limiter1[1]);			// 変換
					sprintf(WarnVal.Judge_time, "%d", get_judgetime(ru_registry.rtr501.judge_time[0]));

                    if(ru_registry.rtr501.limiter_flag & 0x01){
                        if(l_limit >= data){
                            w_state = WS_LOW;
                        }
                    }
                    if(ru_registry.rtr501.limiter_flag & 0x02){
                        if(data >= u_limit){
                            w_state = WS_HIGH;
                        }
                    }
				}
			}
			else {
                l_limit = ru_registry.rtr501.limiter2[0];
                u_limit = ru_registry.rtr501.limiter2[1];

                if(ru_registry.rtr501.limiter_flag & 0x0c){
                    MeTable[method].Convert( WarnVal.L_limit, (uint32_t)ru_registry.rtr501.limiter2[0]);			// 変換
                    MeTable[method].Convert( WarnVal.U_limit, (uint32_t)ru_registry.rtr501.limiter2[1]);			// 変換
					sprintf(WarnVal.Judge_time, "%d", get_judgetime(ru_registry.rtr501.judge_time[1]));
                    if(ru_registry.rtr501.limiter_flag & 0x04){
                        if(l_limit >= data){
                            w_state = WS_LOW;
                        }
                    }
                    if(ru_registry.rtr501.limiter_flag & 0x08){
                        if(data >= u_limit){
                            w_state = WS_HIGH;
                        }
                    }
                }
			}
			break;
		
		case SPEC_574:
		case SPEC_576:

			method = GetConvertMethod( attrib, (uint8_t)Type, add );		// method
			Printf("limiter flag %02X\r\n", ru_registry.rtr574.limiter_flag);
			WarnVal.limit_flag = ru_registry.rtr574.limiter_flag;           // sakaguchi 2021.01.13 XML空要素を判定するため上下限判定フラグを保存
			//if((spec == SPEC_576) && (Channel != 0)){
			if(spec == SPEC_576){                   // sakaguchi 2021.01.13 RTR576は上下限値判定を補正（bit2,3をCh2判定、bit4,5をCh3判定にする）
				tmp_limit_flag = WarnVal.limit_flag;
                WarnVal.limit_flag = WarnVal.limit_flag >> 2;
                WarnVal.limit_flag = (uint8_t)(WarnVal.limit_flag | (tmp_limit_flag & 0x03));
			}

			switch (Channel)
			{
			    case 1:
					l_limit = ru_registry.rtr574.limiter2[0];
					u_limit = ru_registry.rtr574.limiter2[1];

					if(ru_registry.rtr574.limiter_flag & 0x0c){
						MeTable[method].Convert( WarnVal.L_limit, (uint32_t)ru_registry.rtr574.limiter2[0]);			// 変換
						MeTable[method].Convert( WarnVal.U_limit, (uint32_t)ru_registry.rtr574.limiter2[1]);			// 変換
						sprintf(WarnVal.Judge_time, "%d", get_judgetime(ru_registry.rtr574.judge_time[1]));

                        if(ru_registry.rtr574.limiter_flag & 0x04){
                            if(l_limit >= data){
                                w_state = WS_LOW;
                            }
                        }
                        if(ru_registry.rtr574.limiter_flag & 0x08){
                            if(data >= u_limit){
                                w_state = WS_HIGH;
                            }
                        }
					}

					break;
                case 2:
                    l_limit = ru_registry.rtr574.limiter3[0];
                    u_limit = ru_registry.rtr574.limiter3[1];

                    if(ru_registry.rtr574.limiter_flag & 0x30){
                        MeTable[method].Convert( WarnVal.L_limit, (uint32_t)ru_registry.rtr574.limiter3[0]);			// 変換
                        MeTable[method].Convert( WarnVal.U_limit, (uint32_t)ru_registry.rtr574.limiter3[1]);			// 変換
						sprintf(WarnVal.Judge_time, "%d", get_judgetime(ru_registry.rtr574.judge_time[2]));

                        if(ru_registry.rtr574.limiter_flag & 0x10){
                            if(l_limit >= data){
                                w_state = WS_LOW;
                            }
                        }
                        if(ru_registry.rtr574.limiter_flag & 0x20){
                            if(data >= u_limit){
                                w_state = WS_HIGH;
                            }
                        }
                    }

                    break;
                case 3:
                    l_limit = ru_registry.rtr574.limiter4[0];
                    u_limit = ru_registry.rtr574.limiter4[1];

                    if(ru_registry.rtr574.limiter_flag & 0xc0){
                        MeTable[method].Convert( WarnVal.L_limit, (uint32_t)ru_registry.rtr574.limiter4[0]);			// 変換
                        MeTable[method].Convert( WarnVal.U_limit, (uint32_t)ru_registry.rtr574.limiter4[1]);			// 変換
						sprintf(WarnVal.Judge_time, "%d", get_judgetime(ru_registry.rtr574.judge_time[3]));

                        if(ru_registry.rtr574.limiter_flag & 0x40){
                            if(l_limit >= data){
                                w_state = WS_LOW;
                            }
                        }
                        if(ru_registry.rtr574.limiter_flag & 0x80){
                            if(data >= u_limit){
                                w_state = WS_HIGH;
                            }
                        }
                    }

                    break;

                case 0:
                default:
                    l_limit = ru_registry.rtr574.limiter1[0];
                    u_limit = ru_registry.rtr574.limiter1[1];

                    if(ru_registry.rtr574.limiter_flag & 0x03){
                        MeTable[method].Convert( WarnVal.L_limit, (uint32_t)ru_registry.rtr574.limiter1[0]);			// 変換
                        MeTable[method].Convert( WarnVal.U_limit, (uint32_t)ru_registry.rtr574.limiter1[1]);			// 変換
						sprintf(WarnVal.Judge_time, "%d", get_judgetime(ru_registry.rtr574.judge_time[0]));

                        if(ru_registry.rtr574.limiter_flag & 0x01){
                            if(l_limit >= data){
                                w_state = WS_LOW;
                            }
                        }
                        if(ru_registry.rtr574.limiter_flag & 0x02){
                            if(data >= u_limit){
                                w_state = WS_HIGH;
                            }
                        }
                    }

                    break;
            }// switch(Channel)の閉じ括弧

            break;

		default:
			method = GetConvertMethod( attrib, (uint8_t)Type, add );		// method
			WarnVal.limit_flag = ru_registry.rtr501.limiter_flag;           // sakaguchi 2021.01.13 XML空要素を判定するため上下限判定フラグを保存
			MeTable[method].Convert( WarnVal.L_limit, (uint32_t)ru_registry.rtr501.limiter1[0]);			// 変換
			MeTable[method].Convert( WarnVal.U_limit, (uint32_t)ru_registry.rtr501.limiter1[1]);			// 変換
			//sprintf(WarnVal.Judge_time, "%d", ru_registry.rtr501.judge_time[0]);
			sprintf(WarnVal.Judge_time, "%d", get_judgetime(ru_registry.rtr501.judge_time[0]));
            l_limit = ru_registry.rtr501.limiter1[0];
            u_limit = ru_registry.rtr501.limiter1[1];
			break;
		}//switch ( spec ) の閉じ括弧


		Printf("<<<  data %04X  / l_limit %04X / u_limit %04X / state %d / ch %d >>>\r\n", data, l_limit, u_limit, w_state, Channel);
	}

	WarnVal.Val_raw = data;						// 警報値（素データ）		// sakaguchi 2021.03.01
	WarnVal.L_limit_raw = l_limit;				// 下限値（素データ）
	WarnVal.U_limit_raw = u_limit;				// 上限値（素データ）
	WarnVal.AttribCode = attrib;				// データ種別
	
	write_no = ALM_bak_poi(grp, unit);			// 前回の警報種別をバックアップから読み出し	// sakaguchi 2021.03.09
	WarnVal.warnType = ALM_bak[write_no].warntype[Channel];
	if( (WS_UNKOWN <= WarnVal.warnType) && (WarnVal.warnType <= WS_NORMAL2) ){
		WarnVal.warnType = WS_NORMAL;			// 警報状態を正常(1)に補正
	}

	Printf("<<<  w_state %d / w_type %d >>>\r\n", w_state, w_type);

	Printf("	judge time %s (%d)\r\n", WarnVal.Judge_time, ch);

	WarnVal.batt = ExLevelBatt(Form->batt & 0b00001111);
	Printf("	battery       %d \r\n", Form->batt, WarnVal.batt);

	return (w_state);
}
#endif

/***********************************************************************************************
　　 * 警報データの構築
 * @param Dst
 * @param Cfg
 * @param Channel
 * @param Type
 * @return 
 * @see w_state
 *
 **********************************************************************************************/
int GetWarningValue( def_w_config *Cfg, int32_t Channel, int32_t Type )
{
	int8_t		edge = 0;
	uint8_t 	scale,attrib, grp, unit, add = 0;
	uint16_t	spec, data, data2, time, u_limit, l_limit, on_off;	// sakaguchi 2021.04.14 new
	//int16_t		data_s;												// sakaguchi 2021.04.14 new	// sakaguchi 2021.06.15
	int16_t 	method = 0;											// sakaguchi 2021.04.07-2
	uint32_t	GSec;
	char		*tmp, *scale_unit = 0;
	uint16_t 	*warn, *past;
	int32_t ch = Channel;

	WARN_FORM	*Form;
    uint8_t     tmp_limit_flag = 0;
	uint8_t		write_no = 0;
    uint16_t 	i,loop_size;
	uint8_t		search_ok = 0;
	uint8_t 	ALM;

// sakaguchi 2021.11.11 ↓
	int16_t 	method_noscale = 0;	
	char		warn_noscale[16];
	char		warn_u_limit_noscale[16];
	char		warn_l_limit_noscale[16];
	
	memset(warn_noscale, 0x00, sizeof(warn_noscale));
	memset(warn_u_limit_noscale, 0x00, sizeof(warn_u_limit_noscale));
	memset(warn_l_limit_noscale, 0x00, sizeof(warn_l_limit_noscale));
// sakaguchi 2021.11.11 ↑

	memset(WarnVal.Ch_Name, 0x00, sizeof(WarnVal.Ch_Name));
	memset(WarnVal.Val,		0x00, sizeof(WarnVal.Val));
	memset(WarnVal.Unit,	0x00, sizeof(WarnVal.Unit));
	memset(WarnVal.L_limit, 0x00, sizeof(WarnVal.L_limit));
	memset(WarnVal.U_limit, 0x00, sizeof(WarnVal.U_limit));
	memset(WarnVal.Judge_time, 0, sizeof(WarnVal.Judge_time));		// 2020.11.18  クリアしていないために、Senserエラー時に文字化けしていた

	WarnVal.limit_flag = 0;
	WarnVal.utime = 0;
	WarnVal.id = 0;
	WarnVal.Val_raw = 0;
	WarnVal.L_limit_raw = 0;
	WarnVal.U_limit_raw = 0;
	WarnVal.AttribCode = 0;
	WarnVal.warnType = 0;
	WarnVal.edge = 0;							// sakaguchi 2021.04.16

	w_state = 0;
	data = 0;
	//data_s = 0;								// sakaguchi 2021.06.15

	grp  = Cfg->group_no;
	unit = Cfg->unit_no;
	on_off = *(uint16_t *)Cfg->on_off;			// sakaguchi 2021.04.14 new

	if ( Channel >= 4 ) {
		add = 1;								// 積算とする
		Form = (WARN_FORM *)rf_ram.auto_format3.kdata;
		loop_size = *(uint16_t *)&rf_ram.auto_format3.data_byte[0]; // FORMAT3データバイト数
	}
	else {
		Form = (WARN_FORM *)rf_ram.auto_format2.kdata;
		loop_size = *(uint16_t *)&rf_ram.auto_format2.data_byte[0]; // FORMAT2データバイト数
		if ( Channel >= 2 )			// CH3,CH4は子機番号+1
			unit++;
	}

	// 子機の警報情報が存在しているか確認
	//for ( data=0; data<DIMSIZE(rf_ram.auto_format2.kdata); data++ ) {
	for ( i=0; i<loop_size; i++ ) {
		if ( grp == Form->group_no && unit == Form->unit_no ){
		    search_ok = 1;
			break;
		}
		else{
		    Form++;
		}
	}

	// 子機登録情報の読込
	get_regist_relay_inf( grp, unit, rpt_sequence);
	spec = GetSpecNumber( ru_registry.rtr501.serial , ru_registry.rtr505.model );

	//////////////////////////////////////////////////////////////////////////////////////
	// 子機の警報情報が存在していない場合、登録情報から警報XMLに必要な情報を生成する
	//////////////////////////////////////////////////////////////////////////////////////
	if(search_ok == 0){

		// 警報ＩＤの取得
		if ( Channel >= 4 ) {
			WarnVal.id = 0;				// 積算警報の初期値（記録やり直し時）は0
		}else{
			if ( ( Channel % 2 ) == 0 ) {		// CH1, CH3
				WarnVal.id = (uint32_t)WR_chk(grp , unit , 2);
			}else{								// CH2, CH4
				WarnVal.id = (uint32_t)WR_chk(grp , unit , 3);
			}
		}

		// 立ち上がり、立ち下がり警報の有無
		if((spec == SPEC_505PS) || (spec == SPEC_505BPS)){

			if ( ( ru_registry.rtr505.rec_mode & 0b00001100 ) == 0b00000100 ) {
				edge = ( ru_registry.rtr505.rec_mode >> 4 ) & 0b00000111;
				if ( edge == 3 || edge == 7 ){
					edge = 1;					// Rise
				}
				else if ( edge == 2 || edge == 6 ){
					edge = 2;					// Fall
				}
				else{
					edge = 0;
				}
			}
		}

		// スケール変換取得
		if(ru_registry.rtr505.set_flag & 0x20){			// スケール変換チェック
			scale = 1;
		}
		if(spec == SPEC_505V || spec == SPEC_505BV || spec == SPEC_505A || spec == SPEC_505BA || spec == SPEC_505PS || spec == SPEC_505BPS){
			if(memcmp(ru_registry.rtr505.scale_setting, "\n\n\n\n", 4) == 0){
				scale = 0;
			}
		}
		if(spec == SPEC_505BLX){
			if(scale){
				strcpy( ru_registry.rtr505.scale_setting, SCALE_505Lx );		// L5 L10 [dB]
			}
		}

		if(scale == 1){
			GetScaleStr( (char *)ru_registry.rtr505.scale_setting, 0 );			// CH毎のスケール変換文字列
			CheckScaleStr( &tmp, &tmp, &scale_unit );
		}

		// チャンネル属性取得
		if((ru_registry.rtr501.header == 0xFA)||(ru_registry.rtr501.header == 0xF9)){       // ##### RTR-574,576の場合 #####
			if(ru_registry.rtr501.number == unit){
				if((Channel % 2) == 0){
					attrib = ru_registry.rtr574.attribute[0]; 	// CH1
				}else{
					attrib = ru_registry.rtr574.attribute[1]; 	// CH2
				}
			}
			else{
				if((Channel % 2) == 0){
					attrib = ru_registry.rtr574.attribute[2]; 	// CH3
				}else{
					attrib = ru_registry.rtr574.attribute[3]; 	// CH4
				}
			}
			if(ru_registry.rtr501.header == 0xFA){	// RTR-574
				if(Channel == 4){
					attrib = 0x49;								// CH5（積算照度）
				}else if(Channel == 5){
					attrib = 0x55;								// CH6（積算紫外線）
				}
			}
		}
		else{																				// ##### RTR-501,502,503の場合 #####
			if((Channel % 2) == 0){					// CH1
				attrib = ru_registry.rtr501.attribute[0]; // [10]
			}else{
				attrib = ru_registry.rtr501.attribute[1]; // [11]
			}
		}

		w_state = WS_NORMAL;
	}
	//////////////////////////////////////////////////////////////////////////////////////
	// 警報情報が存在している場合、子機から受信したデータから警報XMLに必要な情報を生成する
	//////////////////////////////////////////////////////////////////////////////////////
	else{

		// 警報ＩＤの取得
		if ( Channel >= 4 ) {					// CH5, CH6
			WarnVal.id = 1;						// 積算警報の警報IDは1固定
		}else{
			if ( ( Channel % 2 ) == 0 ) {		// CH1, CH3
				WarnVal.id = (uint32_t)Form->alarm_cnt;
			}else{								// CH2, CH4
				WarnVal.id = (uint32_t)Form->interval;		// FORMAT2のCH2,CH4の警報カウンタはintervalに格納されている
			}
		}

		Printf("	group  number %d \r\n", Form->group_no);
		Printf("	format number %d \r\n", Form->format_no);
		Printf("	unit   number %d \r\n", Form->unit_no);
		Printf("	Channel       %d \r\n", Channel);
		Printf("	alram  count  %d \r\n", Form->alarm_cnt);
		Printf("	serial number %08X \r\n",  *(uint32_t *)Form->serial);

		spec = GetSpecNumber( *(uint32_t *)Form->serial, Form->kisyu505 );  // 子機のシリアル番号

		Printf("	GetWarningValue() spec %04X\r\n", spec);

		//  1CH 毎に必要な情報を構築
		switch ( spec ) {

			case SPEC_505PS:		// 505のパルスだけ特殊
			case SPEC_505BPS:		// 505のパルスだけ特殊

				if ( ( Form->rec_mode & 0b00001100 ) == 0b00000100 ) { // パルスの警報？
					edge = ( Form->rec_mode >> 4 ) & 0b00000111;
					if ( edge == 3 || edge == 7 ){
						edge = 1;					// Rise
					}
					else if ( edge == 2 || edge == 6 ){
						edge = 2;					// Fall
					}
					else{
						edge = 0;
					}
				}
				/* no break */

			case SPEC_501:			// 501/502
			case SPEC_502:
			case SPEC_501B:			// 501B/502B
			case SPEC_502B:

			case SPEC_502PT:

			case SPEC_505PT:		// 505も同じなので
			case SPEC_505TC:
			case SPEC_505V:
			case SPEC_505A:

			case SPEC_505BPT:		// 505Bも同じなので
			case SPEC_505BTC:
			case SPEC_505BV:
			case SPEC_505BA:
			case SPEC_505BLX:

				warn = (uint16_t *)Form->rec.ex501.warn_temp;		// 警報値
				past = (uint16_t *)Form->rec.ex501.past_temp;		// 経過秒数

				break;

			case SPEC_503:
			case SPEC_503B:
			case SPEC_574:///// たまたま一緒なので使っちゃう
			case SPEC_576:///// たまたま一緒なので使っちゃう
			case SPEC_507:///// たまたま一緒なので使っちゃう
			case SPEC_507B:

				Printf("limiter flag %02X\r\n", ru_registry.rtr576.limiter_flag);

				if ( ( Channel % 2 ) == 0 ) {		// CH1, CH3
					warn = (uint16_t *)Form->rec.ex503.warn_temp;
					past = (uint16_t *)Form->rec.ex503.past_temp;
				}
				else {
					warn = (uint16_t *)Form->rec.ex503.warn_hum;
					past = (uint16_t *)Form->rec.ex503.past_hum;
				}
				break;

			default:
				warn = (uint16_t *)Form->rec.ex501.warn_temp;
				past = (uint16_t *)Form->rec.ex501.past_temp;
				break;
		}

		attrib = Form->attrib[ Channel % 2 ];		// CH属性

		time = *past;                               // 経過秒 (10秒で1カウント)
		if ( time == 0xFFFF ){
	//	    GSec = 0;
			GSec = 0xFFFF;							// 経過秒がMAXの場合
		}
		else{
			GSec = (uint32_t)(*(uint32_t *)Form->grobal_time);	//  debug
			Printf("	Warning past time %ld\r\n", GSec);
			if(GSec > 0){										// sakaguchi 2021.03.01
				GSec = (uint32_t)(*(uint32_t *)Form->grobal_time  - (uint32_t)( time * 10UL ));  // Global - 経過秒        2020.01.21 キャストに変更
			}
		}

		Printf("	Warning past time %04X %ld\r\n", time, GSec);

		WarnVal.utime = GSec;

		data = *warn;       // 警報 max値	
		//data_s = (int16_t)data;		// sakaguchi 2021.04.14 new	// sakaguchi 2021.06.15 del

		// スケール変換処理
		//  get_unit_no_info( grp, unit );			// 505のスケール変換取得のため
		get_regist_relay_inf( grp, unit, rpt_sequence);	// 505のスケール変換取得のため  2019 12 03
		scale = Form->scale & 0x01;	// スケール変換
		if(spec == SPEC_505V || spec == SPEC_505BV || spec == SPEC_505A || spec == SPEC_505BA || spec == SPEC_505PS || spec == SPEC_505BPS){
			if(memcmp(ru_registry.rtr505.scale_setting, "\n\n\n\n", 4) == 0){
				scale = 0;
			}
		}


		/*
		// スケール変換せずに505の電圧[mV]の時[V]固定なら内部でスケール変換
		if ( !scale && spec == SPEC_505V		/// 505のV固定かどうか
			&& ( (Form->disp_unit & 0xF0) == 0xA0 || (Form->disp_unit &0x0F) == 0x0A ) ) {
			strcpy( (char *)ru_registry.rtr505.scale_setting, SCALE_505V );
			scale = 1;
		}
		if ( !scale && spec == SPEC_505BV		/// 505のV固定かどうか
			&& ( (Form->disp_unit & 0xF0) == 0xA0 || (Form->disp_unit &0x0F) == 0x0A ) ) {
			strcpy( (char *)ru_registry.rtr505.scale_setting, SCALE_505V );
			scale = 1;
		}
		*/

		if (spec == SPEC_505BLX){
			if(scale){
				strcpy( ru_registry.rtr505.scale_setting, SCALE_505Lx );		// L5 L10 [dB]
			}
		} 

		// 最後にnullを入れる処理(64バイト時は何もしない)

		if ( scale == 1 ) {
			GetScaleStr( (char *)ru_registry.rtr505.scale_setting, 0 );	// CH毎のスケール変換文字列
			CheckScaleStr( &tmp, &tmp, &scale_unit );
		}

		Printf("<<<  %04X  / %04X / %d / %d / %d>>>\r\n", data, spec, edge, scale, add);
		if ( edge )
		{
			ALM = Form->batt;
			if( ALM & 0x10 ){	// CH1警報が発生中
		//		sprintf( WarnVal.Val, " %s%s ", ( edge == 1 ) ? LANG(W_RISE) : LANG(W_FALL), LANG(W_EDGE) );
				sprintf( WarnVal.Val, "%s %s", ( edge == 1 ) ? "Rising" : "Falling", "Edge" );
				w_state = ( edge == 1 ) ? WS_P_RISE : WS_P_FALL;
			}
			else if( on_off & BIT(0) ){		// CH1警報が警報監視間隔内に発生⇒復帰	// sakaguchi 2021.04.14 new
				sprintf( WarnVal.Val, "%s %s", ( edge == 1 ) ? "Rising" : "Falling", "Edge" );
				w_state = ( edge == 1 ) ? WS_P_RISE : WS_P_FALL;		// warnTypeにセットする
			}
			else{
				sprintf( WarnVal.Val, "No Data" );		//"NoData " );	// No Data
				w_state = WS_NORMAL;
			}
		}
		else if ( spec == SPEC_505PS || spec == SPEC_505V || spec == SPEC_505A || spec == SPEC_505BPS || spec == SPEC_505BV || spec == SPEC_505BA || spec == SPEC_505BLX)
		{
			Printf("<<<1  %ld  >>>\r\n", data);

			if ( ( data == 0xEEEE || data == 0xF00F ) && ( spec != SPEC_505PS && spec != SPEC_505BPS ) ){	// 505PS,505BPS以外
				sprintf( WarnVal.Val, "Sensor Error" );	//"N/A " );		// Senser Error
				w_state = WS_SENSER;
			}
			else if ( data == 0xF001 ){			// 505全機種共通
				sprintf( WarnVal.Val, "Underflow" );	//"Under " );	// Underflow
				w_state = WS_UNDER;
			}
			else if ( data == 0xF002 ){			// 505全機種共通
				sprintf( WarnVal.Val, "Overflow" );		//"Over " );	// Overflow
				w_state = WS_OVER;
			}
			else if ( data == 0xF000 ){			// 505全機種共通
				sprintf( WarnVal.Val, "No Data" );		//"NoData " );	// No Data
				w_state = WS_NORMAL;
			}
			//if ( data == 0xF00F ){			// センサエラー
				//sprintf( WarnVal.Val, LANG(W_SENSOR) );	//"N/A " );
				////w_state = WS_WARNING; 
				//w_state = WS_SENSER;
			//}
			//else if ( data == 0xF002 ){		// オーバーフロー
				//sprintf( WarnVal.Val, LANG(W_OVER) );	//"Over " );
				////w_state = WS_WARNING; 
				//w_state = WS_OVER;
			//}
			//else if ( data == 0xF001 ){		// アンダーフロー
				//sprintf( WarnVal.Val, LANG(W_UNDER) );	//"Under " );
				////w_state = WS_WARNING; 
				//w_state = WS_UNDER;
			//}
			else{
				method = GetConvertMethod( attrib, (uint8_t)Type, scale );	// method
				MeTable[method].Convert( WarnVal.Val, (uint32_t)data );		// 変換

// sakaguchi 2021.11.11 ↓
				// 上下限警報の判定はスケール変換前の測定値を使用する
				method_noscale = GetConvertMethod( attrib, (uint8_t)Type, 0 );		// method（スケール変換なし）
				MeTable[method_noscale].Convert( warn_noscale, (uint32_t)data );	// 変換（スケール変換なし）
// sakaguchi 2021.11.11 ↑


				if(spec == SPEC_505BLX){	// 505BLXは0.1db精度のため、警報値を小数点以下１位に補正する
					for(i=0; i<sizeof(WarnVal.Val); i++){
						if(WarnVal.Val[i] == '.'){
							WarnVal.Val[i+2] = 0;		// 小数点以下１位まで有効にしNULLをセット
							break;
						}
					}
				}

				Printf("scale %d (%ld) %s\r\n", scale ,data, WarnVal.Val );
			}
		}
		else if ( spec == SPEC_576 && ( 0xF001UL <= data && data < 0xF005UL ) ){	// 576 CO2センサエラー
			sprintf( WarnVal.Val, "Sensor Error" );		// センサエラー
			w_state = WS_SENSER;
		}
		else if ( spec == SPEC_576 && data == 0xF005UL ){	// 576 測定範囲外
			sprintf( WarnVal.Val, "Overflow" );			// Overflow
			w_state = WS_OVER;
		}
		else if ( spec == SPEC_576 && data == 0xEEEE ){		// 576 記録無し or センサエラー	// sakaguchi 2021.04.13
			if( ((Channel == 2) || (Channel == 3)) && (WarnVal.id) ){	// CH3 or CH4で警報発生中の場合はセンサエラー
				sprintf( WarnVal.Val, "Sensor Error" );	// Senser Error
				w_state = WS_SENSER;
			}else{
				sprintf( WarnVal.Val, "No Data");		// No Data
				w_state = WS_NORMAL;
			}
		}
		else if ( data == 0xEEEE || data == 0xF00F ){	// 505,576以外共通
			sprintf( WarnVal.Val, "Sensor Error" );		// Senser Error
			w_state = WS_SENSER;
		}
		else if ( data == 0xF001 ){						// 505,576以外共通
			sprintf( WarnVal.Val, "Underflow" );		// Underflow
			w_state = WS_UNDER;
		}
		else if ( data == 0xF002 ){						// 505,576以外共通
			sprintf( WarnVal.Val, "Overflow" );			// Overflow
			w_state = WS_OVER;
		}
		else if ( data == 0xF000 ){						// 505以外共通、576 CO2データ無し含む
			sprintf( WarnVal.Val, "No Data");			// No Data
			w_state = WS_NORMAL;
		}
		else
		{	// 正常値なら値を確認

			method = GetConvertMethod( attrib, (uint8_t)Type, add );		// method
			MeTable[method].Convert( WarnVal.Val, (uint32_t)data );			// 変換
			Printf("<<<2 %ld  >>>\r\n", data);
		}

		Printf("<<<  data %04X  / spec %04X / edge %d / scale %d / add %d / method %04X / attrib %02X >>>\r\n", data, spec, edge, scale, add, method, attrib);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ここから警報発生復旧の共通処理
	//////////////////////////////////////////////////////////////////////////////////////
	memset(WarnVal.L_limit, 0, sizeof(WarnVal.L_limit));
	memset(WarnVal.U_limit, 0, sizeof(WarnVal.U_limit));
	memset(WarnVal.Judge_time, 0, sizeof(WarnVal.Judge_time));

	// 現在値の単位取得 WarnVal.Unit
	if ( spec == SPEC_505PS || spec == SPEC_505V || spec == SPEC_505A || spec == SPEC_505BPS || spec == SPEC_505BV || spec == SPEC_505BA || spec == SPEC_505BLX)
	{
		method = GetConvertMethod( attrib, (uint8_t)Type, scale );	// method		// sakaguchi 2021.04.07-2

		if ( !scale )	// スケール変換なし
		{
			//data2 = (uint16_t)(( ru_registry.rtr505.interval < 0x80 ) ? ru_registry.rtr505.interval * 60 : ru_registry.rtr505.interval - 0x80);
			//if ( data2 == 0 )	data2 = 30;		// 0は30秒
			data2 = ru_registry.rtr505.interval;				// 登録ファイルから記録間隔を取得	// sakaguchi 2021.04.16

			if ( spec != SPEC_505PS && spec != SPEC_505BPS){
				sprintf( WarnVal.Unit, "%s", MeTable[method].TypeChar );	// 単位
			}
			else if ( data2 >= 60 ){
				sprintf( WarnVal.Unit, "%s/%umin", MeTable[method].TypeChar, data2/60 ); // 単位
			}
			else{
				sprintf( WarnVal.Unit, "%s/%usec", MeTable[method].TypeChar, data2 ); // 単位
			}
			Printf("<<<  uint-1 %s >>>\r\n", WarnVal.Unit);
		}
		else{
			//data2 = (uint16_t)(( ru_registry.rtr505.interval < 0x80 ) ? ru_registry.rtr505.interval * 60 : ru_registry.rtr505.interval - 0x80);
			//if ( data2 == 0 )	data2 = 30;		// 0は30秒
			data2 = ru_registry.rtr505.interval;				// 登録ファイルから記録間隔を取得	// sakaguchi 2021.04.16

			if ( spec != SPEC_505PS && spec != SPEC_505BPS){
				sprintf( WarnVal.Unit, "%.64s", scale_unit );	// 単位
			}
			else if ( data2 >= 60 ){
				sprintf( WarnVal.Unit, "%.64s/%umin", scale_unit, data2/60 ); // 単位
			}
			else{
				sprintf( WarnVal.Unit, "%.64s/%usec", scale_unit, data2 ); // 単位
			}
			Printf("<<<  uint-2 %s >>>\r\n", WarnVal.Unit);
		}

	}else{
		method = GetConvertMethod( attrib, (uint8_t)Type, add );	// method
		sprintf( WarnVal.Unit, "%s", MeTable[method].TypeChar );	// 単位
	}

	switch ( spec ) {

		case SPEC_501:			// 501/502
		case SPEC_502:
		case SPEC_501B:			// 501B/502B
		case SPEC_502B:
		case SPEC_502PT:
		case SPEC_505PT:		// 505も同じなので
		case SPEC_505TC:
		case SPEC_505BPT:		// 505Bも同じなので
		case SPEC_505BTC:

			method = GetConvertMethod( attrib, (uint8_t)Type, add );		// method
			Printf("limiter flag %02X\r\n", ru_registry.rtr501.limiter_flag);
			//w_flag = ru_registry.rtr501.limiter_flag;
			WarnVal.limit_flag = ru_registry.rtr501.limiter_flag;           // sakaguchi 2021.01.13 XML空要素を判定するため上下限判定フラグを保存
			l_limit = ru_registry.rtr501.limiter1[0];
			u_limit = ru_registry.rtr501.limiter1[1];

			if(ru_registry.rtr501.limiter_flag & 0x03){
				MeTable[method].Convert( WarnVal.L_limit, (uint32_t)ru_registry.rtr501.limiter1[0]);			// 変換
				MeTable[method].Convert( WarnVal.U_limit, (uint32_t)ru_registry.rtr501.limiter1[1]);			// 変換
				
				Printf("judge time %d\r\n", ru_registry.rtr501.judge_time[0]);
				sprintf(WarnVal.Judge_time, "%d", get_judgetime(ru_registry.rtr501.judge_time[0]));

				if(w_state == 0){
					if(ru_registry.rtr501.limiter_flag & 0x01){
						//if(l_limit >= data){
						if( atof(WarnVal.L_limit) >= atof(WarnVal.Val) ){		// sakaguchi 2021.06.15
							//Printf("L_Limit[%f] Val[%f]\r\n",atof(WarnVal.L_limit),atof(WarnVal.Val));
							w_state = WS_LOW;
						}
					}
					if(ru_registry.rtr501.limiter_flag & 0x02){
						//if(data >= u_limit){
						if( atof(WarnVal.U_limit) <= atof(WarnVal.Val) ){		// sakaguchi 2021.06.15
							//Printf("U_Limit[%f] Val[%f]\r\n",atof(WarnVal.U_limit),atof(WarnVal.Val));
							w_state = WS_HIGH;
						}
					}
				}
			}

			break;


		case SPEC_505PS:		// 505のパルスだけ特殊
		case SPEC_505BPS:		// 505のパルスだけ特殊
		case SPEC_505V:
		case SPEC_505A:
		case SPEC_505BV:
		case SPEC_505BA:
		case SPEC_505BLX:
			method = GetConvertMethod( attrib, (uint8_t)Type, scale );	// method
			method_noscale = GetConvertMethod( attrib, (uint8_t)Type, 0 );	// method（スケール変換なし）	// sakaguchi 2021.11.11

			Printf("limiter flag %02X\r\n", ru_registry.rtr505.limiter_flag);
			//w_flag = ru_registry.rtr505.limiter_flag;
			WarnVal.limit_flag = ru_registry.rtr505.limiter_flag;       // sakaguchi 2021.01.13 XML空要素を判定するため上下限判定フラグを保存
			l_limit = ru_registry.rtr505.limiter1[0];
			u_limit = ru_registry.rtr505.limiter1[1];

			if(ru_registry.rtr501.limiter_flag & 0x03){
				MeTable[method].Convert( WarnVal.L_limit, (uint32_t)ru_registry.rtr505.limiter1[0]);			// 変換
				MeTable[method].Convert( WarnVal.U_limit, (uint32_t)ru_registry.rtr505.limiter1[1]);			// 変換

				MeTable[method_noscale].Convert( warn_l_limit_noscale, (uint32_t)ru_registry.rtr505.limiter1[0]);			// 変換（スケール変換なし）	// sakaguchi 2021.11.11
				MeTable[method_noscale].Convert( warn_u_limit_noscale, (uint32_t)ru_registry.rtr505.limiter1[1]);			// 変換（スケール変換なし）	// sakaguchi 2021.11.11

				Printf("judge time %d\r\n", ru_registry.rtr501.judge_time[0]);	
				sprintf(WarnVal.Judge_time, "%d", get_judgetime(ru_registry.rtr505.judge_time[0]));

				if(w_state == 0){
// sakaguchi 2021.11.11 ↓
					// 上下限警報の判定はスケール変換前の測定値を使用する
					if(ru_registry.rtr505.limiter_flag & 0x01){
						if( atof(warn_l_limit_noscale) >= atof(warn_noscale) ){
							Printf("L_Limit[%f] Val[%f]\r\n",atof(warn_l_limit_noscale),atof(warn_noscale));
							w_state = WS_LOW;
						}
					}
					if(ru_registry.rtr505.limiter_flag & 0x02){
						if( atof(warn_u_limit_noscale) <= atof(warn_noscale) ){
							Printf("U_Limit[%f] Val[%f]\r\n",atof(warn_u_limit_noscale),atof(warn_noscale));
							w_state = WS_HIGH;
						}
					}
//					if(ru_registry.rtr505.limiter_flag & 0x01){
//// sakaguchi 2021.06.15 ↓
//						if( atof(WarnVal.L_limit) >= atof(WarnVal.Val) ){
//							Printf("L_Limit[%f] Val[%f]\r\n",atof(WarnVal.L_limit),atof(WarnVal.Val));
//							w_state = WS_LOW;
//						}
//						//if((spec == SPEC_505PS) || (spec == SPEC_505BPS)){		// sakaguchi 2021.04.14 new
//						//	if(l_limit >= data){
//						//		w_state = WS_LOW;
//						//	}
//						//}
//						//else{
//						//	if(l_limit >= data_s){		// 負の値を考慮
//						//		w_state = WS_LOW;
//						//	}
//						//}
//// sakaguchi 2021.06.15 ↑
//					}
//					if(ru_registry.rtr505.limiter_flag & 0x02){
//// sakaguchi 2021.06.15 ↓
//						if( atof(WarnVal.U_limit) <= atof(WarnVal.Val) ){
//							Printf("U_Limit[%f] Val[%f]\r\n",atof(WarnVal.U_limit),atof(WarnVal.Val));
//							w_state = WS_HIGH;
//						}
//						//if((spec == SPEC_505PS) || (spec == SPEC_505BPS)){		// sakaguchi 2021.04.14 new
//						//	if(data >= u_limit){
//						//		w_state = WS_HIGH;
//						//	}
//						//}
//						//else{
//						//	if(data_s >= u_limit){		// 負の値を考慮
//						//		w_state = WS_HIGH;
//						//	}
//						//}
//// sakaguchi 2021.06.15 ↑
//					}
// sakaguchi 2021.11.11 ↑
				}
			}

			if(spec == SPEC_505BLX){	// 505BLXは0.1db精度のため、警報値を小数点以下１位に補正する
				for(i=0; i<sizeof(WarnVal.U_limit); i++){
					if(WarnVal.U_limit[i] == '.'){
						WarnVal.U_limit[i+2] = 0;		// 小数点以下１位まで有効にしNULLをセット
						break;
					}
				}
				for(i=0; i<sizeof(WarnVal.L_limit); i++){
					if(WarnVal.L_limit[i] == '.'){
						WarnVal.L_limit[i+2] = 0;		// 小数点以下１位まで有効にしNULLをセット
						break;
					}
				}
			}
			break;

		case SPEC_503:
		case SPEC_503B:
		case SPEC_507:
		case SPEC_507B:

			method = GetConvertMethod( attrib, (uint8_t)Type, add );		// method
			Printf("limiter flag %02X\r\n", ru_registry.rtr501.limiter_flag);
			WarnVal.limit_flag = ru_registry.rtr501.limiter_flag;       // sakaguchi 2021.01.13 XML空要素を判定するため上下限判定フラグを保存
			
			if (Channel == 0 ) {		// CH1
				l_limit = ru_registry.rtr501.limiter1[0];
				u_limit = ru_registry.rtr501.limiter1[1];
			
				if(ru_registry.rtr501.limiter_flag & 0x03){
					MeTable[method].Convert( WarnVal.L_limit, (uint32_t)ru_registry.rtr501.limiter1[0]);			// 変換
					MeTable[method].Convert( WarnVal.U_limit, (uint32_t)ru_registry.rtr501.limiter1[1]);			// 変換
					sprintf(WarnVal.Judge_time, "%d", get_judgetime(ru_registry.rtr501.judge_time[0]));

					if(w_state == 0){
						if(ru_registry.rtr501.limiter_flag & 0x01){
							//if(l_limit >= data){
							if( atof(WarnVal.L_limit) >= atof(WarnVal.Val) ){		// sakaguchi 2021.06.15
								w_state = WS_LOW;
							}
						}
						if(ru_registry.rtr501.limiter_flag & 0x02){
							//if(data >= u_limit){
							if( atof(WarnVal.U_limit) <= atof(WarnVal.Val) ){		// sakaguchi 2021.06.15
								w_state = WS_HIGH;
							}
						}
					}
				}
			}
			else {
				l_limit = ru_registry.rtr501.limiter2[0];
				u_limit = ru_registry.rtr501.limiter2[1];

				if(ru_registry.rtr501.limiter_flag & 0x0c){
					MeTable[method].Convert( WarnVal.L_limit, (uint32_t)ru_registry.rtr501.limiter2[0]);			// 変換
					MeTable[method].Convert( WarnVal.U_limit, (uint32_t)ru_registry.rtr501.limiter2[1]);			// 変換
					sprintf(WarnVal.Judge_time, "%d", get_judgetime(ru_registry.rtr501.judge_time[1]));

					if(w_state == 0){
						if(ru_registry.rtr501.limiter_flag & 0x04){
							//if(l_limit >= data){
							if( atof(WarnVal.L_limit) >= atof(WarnVal.Val) ){		// sakaguchi 2021.06.15
								w_state = WS_LOW;
							}
						}
						if(ru_registry.rtr501.limiter_flag & 0x08){
							//if(data >= u_limit){
							if( atof(WarnVal.U_limit) <= atof(WarnVal.Val) ){		// sakaguchi 2021.06.15
								w_state = WS_HIGH;
							}
						}
					}
				}
			}
			break;
	
		case SPEC_574:
		case SPEC_576:

			method = GetConvertMethod( attrib, (uint8_t)Type, add );		// method
			Printf("limiter flag %02X\r\n", ru_registry.rtr574.limiter_flag);

			if(Channel >= 4){
				WarnVal.limit_flag = ru_registry.rtr574.sekisan_flag;
			}else{
				WarnVal.limit_flag = ru_registry.rtr574.limiter_flag;           // sakaguchi 2021.01.13 XML空要素を判定するため上下限判定フラグを保存
				if(spec == SPEC_576){                   						// RTR576は上下限値判定を補正（bit2,3をCh2判定、bit4,5をCh3判定にする）
					tmp_limit_flag = WarnVal.limit_flag;
					WarnVal.limit_flag = WarnVal.limit_flag >> 2;
					WarnVal.limit_flag = (uint8_t)(WarnVal.limit_flag | (tmp_limit_flag & 0x03));
				}
			}

			switch (Channel)
			{
				case 1:
					l_limit = ru_registry.rtr574.limiter2[0];
					u_limit = ru_registry.rtr574.limiter2[1];

					if(ru_registry.rtr574.limiter_flag & 0x0c){
						MeTable[method].Convert( WarnVal.L_limit, (uint32_t)ru_registry.rtr574.limiter2[0]);			// 変換
						MeTable[method].Convert( WarnVal.U_limit, (uint32_t)ru_registry.rtr574.limiter2[1]);			// 変換
						sprintf(WarnVal.Judge_time, "%d", get_judgetime(ru_registry.rtr574.judge_time[1]));

						if(w_state == 0){
							if(ru_registry.rtr574.limiter_flag & 0x04){
								//if(l_limit >= data){
								if( atof(WarnVal.L_limit) >= atof(WarnVal.Val) ){		// sakaguchi 2021.06.15
									w_state = WS_LOW;
								}
							}
							if(ru_registry.rtr574.limiter_flag & 0x08){
								//if(data >= u_limit){
								if( atof(WarnVal.U_limit) <= atof(WarnVal.Val) ){		// sakaguchi 2021.06.15
									w_state = WS_HIGH;
								}
							}
						}
					}
					break;

				case 2:
					l_limit = ru_registry.rtr574.limiter3[0];
					u_limit = ru_registry.rtr574.limiter3[1];

					if(ru_registry.rtr574.limiter_flag & 0x30){
						MeTable[method].Convert( WarnVal.L_limit, (uint32_t)ru_registry.rtr574.limiter3[0]);			// 変換
						MeTable[method].Convert( WarnVal.U_limit, (uint32_t)ru_registry.rtr574.limiter3[1]);			// 変換
						sprintf(WarnVal.Judge_time, "%d", get_judgetime(ru_registry.rtr574.judge_time[2]));

						if(w_state == 0){
							if(ru_registry.rtr574.limiter_flag & 0x10){
								//if(l_limit >= data){
								if( atof(WarnVal.L_limit) >= atof(WarnVal.Val) ){		// sakaguchi 2021.06.15
									w_state = WS_LOW;
								}
							}
							if(ru_registry.rtr574.limiter_flag & 0x20){
								//if(data >= u_limit){
								if( atof(WarnVal.U_limit) <= atof(WarnVal.Val) ){		// sakaguchi 2021.06.15
									w_state = WS_HIGH;
								}
							}
						}
					}
					break;

				case 3:
					l_limit = ru_registry.rtr574.limiter4[0];
					u_limit = ru_registry.rtr574.limiter4[1];

					if(ru_registry.rtr574.limiter_flag & 0xc0){
						MeTable[method].Convert( WarnVal.L_limit, (uint32_t)ru_registry.rtr574.limiter4[0]);			// 変換
						MeTable[method].Convert( WarnVal.U_limit, (uint32_t)ru_registry.rtr574.limiter4[1]);			// 変換
						sprintf(WarnVal.Judge_time, "%d", get_judgetime(ru_registry.rtr574.judge_time[3]));

						if(w_state == 0){
							if(ru_registry.rtr574.limiter_flag & 0x40){
								//if(l_limit >= data){
								if( atof(WarnVal.L_limit) >= atof(WarnVal.Val) ){		// sakaguchi 2021.06.15
									w_state = WS_LOW;
								}
							}
							if(ru_registry.rtr574.limiter_flag & 0x80){
								//if(data >= u_limit){
								if( atof(WarnVal.U_limit) <= atof(WarnVal.Val) ){		// sakaguchi 2021.06.15
									w_state = WS_HIGH;
								}
							}
						}
					}
					break;

				case 4:			// 積算照度
					u_limit = ru_registry.rtr574.sekisan_lu;
					l_limit = 0;

					if(ru_registry.rtr574.sekisan_flag & 0x01){
						MeTable[method].Convert( WarnVal.U_limit, ru_registry.rtr574.sekisan_lu);			// 変換
						sprintf(WarnVal.Judge_time, "%d", get_judgetime(ru_registry.rtr574.judge_time[0]));

						if(w_state == 0){
							//if(data >= u_limit){
							if( atof(WarnVal.U_limit) <= atof(WarnVal.Val) ){		// sakaguchi 2021.06.15
								w_state = WS_HIGH;
							}
						}
					}
					break;

				case 5:			// 積算紫外線量
					u_limit = ru_registry.rtr574.sekisan_uv;
					l_limit = 0;

					if(ru_registry.rtr574.sekisan_flag & 0x02){
						MeTable[method].Convert( WarnVal.U_limit, ru_registry.rtr574.sekisan_uv);			// 変換
						sprintf(WarnVal.Judge_time, "%d", get_judgetime(ru_registry.rtr574.judge_time[0]));

						if(w_state == 0){
							//if(data >= u_limit){
							if( atof(WarnVal.U_limit) <= atof(WarnVal.Val) ){		// sakaguchi 2021.06.15
								w_state = WS_HIGH;
							}
						}
					}
					break;

				case 0:
				default:
					l_limit = ru_registry.rtr574.limiter1[0];
					u_limit = ru_registry.rtr574.limiter1[1];

					if(ru_registry.rtr574.limiter_flag & 0x03){
						MeTable[method].Convert( WarnVal.L_limit, (uint32_t)ru_registry.rtr574.limiter1[0]);			// 変換
						MeTable[method].Convert( WarnVal.U_limit, (uint32_t)ru_registry.rtr574.limiter1[1]);			// 変換
						sprintf(WarnVal.Judge_time, "%d", get_judgetime(ru_registry.rtr574.judge_time[0]));

						if(w_state == 0){
							if(ru_registry.rtr574.limiter_flag & 0x01){
// sakaguchi 2021.06.15 ↓
								if( atof(WarnVal.L_limit) >= atof(WarnVal.Val) ){
									//Printf("U_Limit[%f] Val[%f]\r\n",atof(WarnVal.L_limit),atof(WarnVal.Val));
									w_state = WS_LOW;
								}
								//if(spec == SPEC_574){				// RTR-574の場合	// sakaguchi 2021.04.14 new
								//	if(l_limit >= data){
								//		w_state = WS_LOW;
								//	}
								//}
								//else{
								//	if(l_limit >= data_s){			// RTR-576 CO2の場合（負の値を考慮）
								//		w_state = WS_LOW;
								//	}
								//}
// sakaguchi 2021.06.15 ↑
							}
							if(ru_registry.rtr574.limiter_flag & 0x02){
// sakaguchi 2021.06.15 ↓
								if( atof(WarnVal.U_limit) <= atof(WarnVal.Val) ){
									//Printf("U_Limit[%f] Val[%f]\r\n",atof(WarnVal.U_limit),atof(WarnVal.Val));
									w_state = WS_HIGH;
								}
								//if(spec == SPEC_574){				// RTR-574の場合	// sakaguchi 2021.04.14 new
								//	if(data >= u_limit){
								//		w_state = WS_HIGH;
								//	}
								//}
								//else{
								//	if(data_s >= u_limit){			// RTR-576 CO2の場合（負の値を考慮）
								//		w_state = WS_HIGH;
								//	}
								//}
// sakaguchi 2021.06.15 ↑
							}
						}
					}
					break;
			}// switch(Channel)の閉じ括弧

			break;

		default:
			method = GetConvertMethod( attrib, (uint8_t)Type, add );		// method
			WarnVal.limit_flag = ru_registry.rtr501.limiter_flag;           // sakaguchi 2021.01.13 XML空要素を判定するため上下限判定フラグを保存
			MeTable[method].Convert( WarnVal.L_limit, (uint32_t)ru_registry.rtr501.limiter1[0]);			// 変換
			MeTable[method].Convert( WarnVal.U_limit, (uint32_t)ru_registry.rtr501.limiter1[1]);			// 変換
			//sprintf(WarnVal.Judge_time, "%d", ru_registry.rtr501.judge_time[0]);
			sprintf(WarnVal.Judge_time, "%d", get_judgetime(ru_registry.rtr501.judge_time[0]));
			l_limit = ru_registry.rtr501.limiter1[0];
			u_limit = ru_registry.rtr501.limiter1[1];
			break;
		
	}//switch ( spec ) の閉じ括弧

	Printf("<<<  data %04X  / l_limit %04X / u_limit %04X / state %d / ch %d >>>\r\n", data, l_limit, u_limit, w_state, Channel);

	if(w_state == 0){
		//w_state = WS_NORMAL2;
		w_state = WS_NORMAL;
	}

	if(!edge){		// 立ち上がり、立ち下がり警報は更新しない
		WarnVal.Val_raw = data;							// 警報値（素データ）
		WarnVal.L_limit_raw = l_limit;					// 下限値（素データ）
		WarnVal.U_limit_raw = u_limit;					// 上限値（素データ）
	}else{			// 立ち上がり、立ち下がり警報 不要な情報は念のためクリア
		memset(WarnVal.L_limit, 0, sizeof(WarnVal.L_limit));
		memset(WarnVal.U_limit, 0, sizeof(WarnVal.U_limit));
		WarnVal.Val_raw = 0;							// 警報値（素データ）
		WarnVal.L_limit_raw = 0;						// 下限値（素データ）
		WarnVal.U_limit_raw = 0;						// 上限値（素データ）
	}

	WarnVal.AttribCode = MeTable[method].DataCode;	// データ種別
	if(spec == SPEC_505BLX){
		WarnVal.AttribCode = 0x009A;				// KGCモードの記録属性
	}

	// 警報種別（warntype）を警報バックアップから取得する
	write_no = ALM_bak_poi(grp, Cfg->unit_no);
	if((ALM_bak[write_no].group_no == grp) && (ALM_bak[write_no].unit_no == Cfg->unit_no)){
		WarnVal.warnType = ALM_bak[write_no].warntype[Channel];

		if(WarnVal.warnType == WS_NORMAL2){
			WarnVal.warnType = WS_NORMAL;			// 警報状態を正常(1)に補正
		}
// 2023.08.03 ↓
		if(WarnVal.warnType == WS_UNKOWN){		// 初期化後はWS_UNKOWN(0)になる
			WarnVal.warnType = WS_NORMAL;			// 警報状態を正常(1)に補正
		}
// 2023.08.03 ↑
	}
	else{
		//WarnVal.warnType = WS_UNKOWN;				// 警報バックアップに警報種別がないため不明をセット	// 2023.08.03 不明は使用しない
	}

	Printf("<<<  w_state %d / w_type %d >>>\r\n", w_state, w_type);

	Printf("	judge time %s (%d)\r\n", WarnVal.Judge_time, ch);

	WarnVal.batt = ExLevelBatt(Form->batt & 0b00001111);
	//Printf("	battery       %d \r\n", Form->batt, WarnVal.batt);
	Printf("	battery       %d %d \r\n", Form->batt, WarnVal.batt);

	WarnVal.edge = edge;							// 立上り・立下り	// sakaguchi 2021.04.16

	return (w_state);
}
// sakaguchi 2021.04.07 ↑


/***********************************************************************************************
 * 警報データの構築
 * センサ、電池、通信警報の時刻の取得
 * @return 
 *
 **********************************************************************************************/
static void GetWarningTime(void)
{
	WARN_FORM	*Form;
	Form = (WARN_FORM *)rf_ram.auto_format1.kdata;
	WarnVal.utime = (uint32_t)(*(uint32_t *)Form->grobal_time);  // Global  経過秒数は無し
	Printf("	Warning time(Batt) %ld\r\n", WarnVal.utime);
}

// sakaguchi 2021.01.13 // sakaguchi 2021.01.18
/***********************************************************************************************
 * 警報データの構築
 * バッテリレベルの取得
 * @return 
 *
 **********************************************************************************************/
static void GetWarningBattLevel(def_w_config *Cfg)
{
	WARN_FORM	*Form;
	uint8_t 	grp,unit;
	uint16_t	data;
	Form = (WARN_FORM *)rf_ram.auto_format1.kdata;

	grp  = Cfg->group_no;
	unit = Cfg->unit_no;

	for ( data=0; data<DIMSIZE(rf_ram.auto_format1.kdata); data++ ) {
		if ( grp == Form->group_no && unit == Form->unit_no ){
			break;
		}
		else{
			Form++;
		}
	}

	WarnVal.batt = ExLevelBatt(Form->batt & 0b00001111);
	Printf("	Warning Batt %d\r\n", WarnVal.batt);
}


/**
 *
 * @param Send
 * @param Adrs
 * @return
 */
uint16_t CheckWarningMask(OBJ_MLFLG Send, int Adrs)
{

	uint8_t mask = 0;

	Printf("CheckWarningMask 1 (%ld) \r\n", Adrs);

	Send.SendFlag.All = 0xffff;

	switch (Adrs)
	{
		case 0:
			mask = my_config.warning.Fn1[0];
			break;
		case 1:
			mask = my_config.warning.Fn2[0];
			break;
		case 2:
			mask = my_config.warning.Fn3[0];
			break;
		case 3:
			mask = my_config.warning.Fn4[0];
			break;
		case 4:
			mask = my_config.warning.Ext[0];
			break;

	}

	Printf("CheckWarningMask 2 %02X  %04X (%ld)\r\n", mask, Send.SendFlag.All, Adrs);

	if ( mask & WR_MASK_HILO ){
		Send.SendFlag.Active.UpLo = (int16_t)0;				// 上下限値マスク
		Printf("\r\n Mask HILO");
	}
	if ( mask & WR_MASK_SENSOR ){
		Send.SendFlag.Active.Sensor = (int16_t)0;			// センサマスク
		Printf("\r\n Mask SEN");
	}
	if ( mask & WR_MASK_RF ){
		Send.SendFlag.Active.RF = (int16_t)0;				// 無線通信マスク
		Printf("\r\n Mask RF");
	}
	if ( mask & WR_MASK_BATT ){
		Send.SendFlag.Active.Battery = (int16_t)0;			// バッテリマスク
		Printf("\r\n Mask BATT");
	}
	if ( mask & WR_MASK_INPUT ){
		Send.SendFlag.Active.Input = (int16_t)0;				// 接点入力マスク
		Printf("\r\n Mask INPUT");
	}

	Printf("\r\n CheckWarningMask 3 %02X %04X  \r\n", mask , Send.SendFlag.All );

	return (Send.SendFlag.All);
}


/**
 * テスト用警報データ作成・送信
 */
void SendWarningTest(void)
{
	uint32_t	utime;
	int     temp;
//	int		tm;
	char	name[sizeof(my_config.device.Name)+2];				// sakaguchi add 2020.08.27

	WarnVal.utime = RTC_GetLCTSec( 0 );         // GMT秒をローカル秒へ変換


	sprintf(XMLTForm,"%s", my_config.device.TimeForm);		    // 日付フォーマット

	if( my_config.websrv.Mode[0] == 0 ){						// DataServer 			// sakaguchi 2021.03.01
		XML.Plain( XML_FORM_WARN_DATASRV );				        // <file .. name=" まで
	}else{
	XML.Plain( XML_FORM_WARN );				                // <file .. name=" まで
	}
	XML.Plain( "%s\"", XML.FileName);
	XML.Plain( " type=\"2\"");     

	memset(name, 0x00, sizeof(name));
	memcpy(name, my_config.device.Name, sizeof(my_config.device.Name));
	sprintf(XMLTemp,"%s", name);
	XML.Plain( " author=\"%s\">\n", XMLTemp);  

	XML.OpenTag( "header" );
		
		utime = RTC_GetGMTSec();                // UTC取得
		XML.TagOnce( "created_unixtime", "%lu", utime );	// 現在値の時刻
		temp = (int)( RTC.Diff / 60 );		        	        // 時差
		XML.TagOnce( "time_diff", "%d", temp );
		XML.TagOnce( "std_bias", "0" );			                //
		temp = (int)( RTC.AdjustSec / 60 );		                // サマータイム中
		XML.TagOnce( "dst_bias", "%d", temp );
		
		XML.OpenTag( "base_unit" );
			sprintf(XMLTemp, "%08lX", fact_config.SerialNumber);
			XML.TagOnce( "serial", XMLTemp );
// 2023.05.31 ↓
			switch(fact_config.Vender){
				case VENDER_TD:
					XML.TagOnce( "model", UNIT_BASE_TANDD /*BaseUnitName*/ );
					break;
				case VENDER_ESPEC:
					XML.TagOnce( "model", UNIT_BASE_ESPEC /*BaseUnitName*/ );
					break;
				case VENDER_HIT:
					XML.TagOnce( "model", UNIT_BASE_HITACHI /*BaseUnitName*/ );
					break;
				default:
					XML.TagOnce( "model", UNIT_BASE_TANDD /*BaseUnitName*/ );
					break;
			}
// 2023.05.31 ↑
//			XML.Plain( "<name>%s</name>\n", my_config.device.Name);	 // 親機名称	// sakaguchi cg 2020.08.27
			XML.Plain( "<name>%s</name>\n", name);	 				 // 親機名称

			XML.TagOnce( "warning", "%d", 1 );
		XML.CloseTag();		// <base_unit>

	XML.CloseTag();		// <header>



	XML.Plain( "</file>\n" );		// <file 現在値>

	XML.Output();					// バッファ吐き出し

	int len = (int)strlen(xml_buffer);
	Printf("====> Download XML File size = %ld\r\n\r\n", strlen(xml_buffer));
	for(int i=0;i<len;i++){
		Printf("%c",xml_buffer[i]);
	}
	Printf("\r\n");
	
}


/**
 * テスト用警報データ作成・送信
 * @param   pat
 * @bug int と unsigned long で論理演算している → 引数をuint32_t変更
 */
void SendWarningTestNew(uint32_t pat)
{
	uint32_t	utime;
	int     temp;
//	int		tm;
	int len,i;
	char	name[sizeof(my_config.device.Name)+2];				// sakaguchi add 2020.08.27

	WarnVal.id = 999;
	sprintf(WarnVal.Val,"12.3");
	sprintf(WarnVal.Unit, "C");
	sprintf(WarnVal.U_limit, "123");
	sprintf(WarnVal.L_limit, "123");
	sprintf(WarnVal.Judge_time, "0");
//	sprintf(WarnVal.batt,"5");
	WarnVal.batt = '5';

	WarnVal.utime = RTC_GetLCTSec( 0 );         // GMT秒をローカル秒へ変換


	sprintf(XMLTForm,"%s", my_config.device.TimeForm);		    // 日付フォーマット

	if( my_config.websrv.Mode[0] == 0 ){						// DataServer 			// sakaguchi 2021.03.01
		XML.Plain( XML_FORM_WARN_DATASRV );				        // <file .. name=" まで
	}else{
	XML.Plain( XML_FORM_WARN );				                // <file .. name=" まで
	}
	XML.Plain( "%s\"", XML.FileName);
	XML.Plain( " type=\"2\"");     

	memset(name, 0x00, sizeof(name));
	memcpy(name, my_config.device.Name, sizeof(my_config.device.Name));
	sprintf(XMLTemp,"%s", name);
	XML.Plain( " author=\"%s\">\n", XMLTemp);  

	XML.OpenTag( "header" );
		
		utime = RTC_GetGMTSec();                // UTC取得
		XML.TagOnce( "created_unixtime", "%lu", utime );	// 現在値の時刻
		temp = (int)( RTC.Diff / 60 );		        	        // 時差
		XML.TagOnce( "time_diff", "%d", temp );
		XML.TagOnce( "std_bias", "0" );			                //
		temp = (int)( RTC.AdjustSec / 60 );		                // サマータイム中
		XML.TagOnce( "dst_bias", "%d", temp );
		
		XML.OpenTag( "base_unit" );
			sprintf(XMLTemp, "%08lX", fact_config.SerialNumber);
			XML.TagOnce( "serial", XMLTemp );
// 2023.05.31 ↓
			switch(fact_config.Vender){
				case VENDER_TD:
					XML.TagOnce( "model", UNIT_BASE_TANDD /*BaseUnitName*/ );
					break;
				case VENDER_ESPEC:
					XML.TagOnce( "model", UNIT_BASE_ESPEC /*BaseUnitName*/ );
					break;
				case VENDER_HIT:
					XML.TagOnce( "model", UNIT_BASE_HITACHI /*BaseUnitName*/ );
					break;
				default:
					XML.TagOnce( "model", UNIT_BASE_TANDD /*BaseUnitName*/ );
					break;
			}
// 2023.05.31 ↑
//			XML.Plain( "<name>%s</name>\n", my_config.device.Name);	// 親機名称	// sakaguchi cg 2020.08.27
			XML.Plain( "<name>%s</name>\n", name);					// 親機名称

			XML.TagOnce( "warning", "%d", 1 );
		XML.CloseTag();		// <base_unit>

	XML.CloseTag();		// <header>


	if(pat & WTEST_UPPER){
		XML.OpenTag( "device" );
		XML.TagOnce( "serial", "11111111");				// sakaguchi 2020.09.14
// 2023.05.31 ↓
		if(fact_config.Vender == VENDER_ESPEC){
		    XML.TagOnce( "model", TEST_MODEL_ESPEC );
		}else{
		    XML.TagOnce( "model", TEST_MODEL_TANDD );
		}
// 2023.05.31 ↑
    	XML.Plain("<name>%s</name>\n","Test1");
		XML.Plain("<group>%s</group>\n","Group1");

	    sprintf(WarnVal.Val,"154.9");
    	sprintf(WarnVal.Unit, "C");
	    sprintf(WarnVal.U_limit, "77.7");
    	sprintf(WarnVal.L_limit, "12.3");
//			sprintf(WarnVal.batt,"5");
	    WarnVal.batt ='5';
		w_state = WS_HIGH;
		//w_type = 1;
    	MakeChannelTag( 1, WS_HIGH);
/*
		XML.OpenTag("battery");
		XML.TagOnce("state" ,"%d", 1);
	    XML.TagOnce("event_unixtime" ,"%ld", WarnVal.utime);
    	XML.TagOnce("level" ,"%d", WarnVal.batt);
	    XML.CloseTag();		// battery


		XML.OpenTag("comm");
		XML.TagOnce("state" ,"%d", 1);				
    	XML.TagOnce("event_unixtime" ,"%ld", 0);
   		XML.TagOnce("type" ,"%d", 0);
		XML.CloseTag();		
*/
		XML.CloseTag();		// device
	}

	if(pat & WTEST_LOWER){
		XML.OpenTag( "device" );
		XML.TagOnce( "serial", "22222222");				// sakaguchi 2020.09.14
// 2023.05.31 ↓
		if(fact_config.Vender == VENDER_ESPEC){
		    XML.TagOnce( "model", TEST_MODEL_ESPEC );
		}else{
		    XML.TagOnce( "model", TEST_MODEL_TANDD );
		}
// 2023.05.31 ↑
		XML.Plain("<name>%s</name>\n","Test2");
		XML.Plain("<group>%s</group>\n","Group1");

		sprintf(WarnVal.Val,"-59.9");
		sprintf(WarnVal.Unit, "C");
		sprintf(WarnVal.U_limit, "77.7");
		sprintf(WarnVal.L_limit, "-12.3");
		sprintf(WarnVal.Judge_time, "30");
//		sprintf(WarnVal.batt,"5");
		WarnVal.batt = '5';
		w_state = WS_LOW;
			
		MakeChannelTag( 1, WS_LOW);
/*
		XML.OpenTag("battery");
		XML.TagOnce("state" ,"%d", 1);
		XML.TagOnce("event_unixtime" ,"%ld", WarnVal.utime);
		XML.TagOnce("level" ,"%d", WarnVal.batt);
		XML.CloseTag();		// battery


		XML.OpenTag("comm");
		XML.TagOnce("state" ,"%d", 1);				
		XML.TagOnce("event_unixtime" ,"%ld", 0);
		XML.TagOnce("type" ,"%d", 0);
		XML.CloseTag();		
*/
		XML.CloseTag();		// device
	}

	if(pat & WTEST_SENSOR){
		XML.OpenTag( "device" );
		XML.TagOnce( "serial", "33333333");				// sakaguchi 2020.09.14
// 2023.05.31 ↓
		if(fact_config.Vender == VENDER_ESPEC){
		    XML.TagOnce( "model", TEST_MODEL_ESPEC );
		}else{
		    XML.TagOnce( "model", TEST_MODEL_TANDD );
		}
// 2023.05.31 ↑
		XML.Plain("<name>%s</name>\n","Test3");
		XML.Plain("<group>%s</group>\n","Group1");

		//memset(WarnVal.Val, 0, sizeof(WarnVal.Val));
		//sprintf(WarnVal.Val,"N/A");
		sprintf(WarnVal.Val,"Sensor Error");			// sakaguchi 2021.04.07
		sprintf(WarnVal.Unit, "C");
		sprintf(WarnVal.U_limit, "77.7");
		sprintf(WarnVal.L_limit, "-12.3");
		sprintf(WarnVal.Judge_time, "30");
		//sprintf(WarnVal.id, "30");

		XML.OpenTag( "ch" );
		XML.TagOnce("num", "1");
		XML.TagOnce("name", "" );			// CH名
		XML.TagOnce("state" ,"%d", 13);
		XML.TagOnce("event_unixtime" ,"%ld", WarnVal.utime);
		XML.TagOnce("id" 		,"%ld", WarnVal.id);
		XML.TagOnce("value", "%.16s", WarnVal.Val);
		XML.TagOnce("unit","");
		XML.TagOnce("upper_limit","");
		XML.TagOnce("lower_limit","");
		XML.TagOnce("judgement_time", WarnVal.Judge_time);
					
		XML.CloseTag();	//ch		
		XML.CloseTag();		

/*
		XML.OpenTag("battery");
		XML.TagOnce("state" ,"%d", 1);
		XML.TagOnce("event_unixtime" ,"%ld", WarnVal.utime);
		XML.TagOnce("level" ,"%d", WarnVal.batt);
		XML.CloseTag();		// battery


		XML.OpenTag("comm");
		XML.TagOnce("state" ,"%d", 1);				
		XML.TagOnce("event_unixtime" ,"%ld", 0);
		XML.TagOnce("type" ,"%d", 0);
		XML.CloseTag();		
*/
		XML.CloseTag();		// device
	}

	if(pat & WTEST_COMM){
		XML.OpenTag( "device" );
		XML.TagOnce( "serial", "44444444");				// sakaguchi 2020.09.14
// 2023.05.31 ↓
		if(fact_config.Vender == VENDER_ESPEC){
		    XML.TagOnce( "model", TEST_MODEL_ESPEC );
		}else{
		    XML.TagOnce( "model", TEST_MODEL_TANDD );
		}
// 2023.05.31 ↑
		XML.Plain("<name>%s</name>\n","Test4");
		XML.Plain("<group>%s</group>\n","Group1");

		XML.OpenTag("comm");
		XML.TagOnce("state" ,"%d", 10);				
		XML.TagOnce("event_unixtime" ,"%ld", WarnVal.utime);
		XML.TagOnce("type" ,"%d", 0);
		XML.CloseTag();		

		XML.CloseTag();		// device
	}

	if(pat & WTEST_BATT){
		XML.OpenTag( "device" );
		XML.TagOnce( "serial", "55555555");				// sakaguchi 2020.09.14
// 2023.05.31 ↓
		if(fact_config.Vender == VENDER_ESPEC){
		    XML.TagOnce( "model", TEST_MODEL_ESPEC );
		}else{
		    XML.TagOnce( "model", TEST_MODEL_TANDD );
		}
// 2023.05.31 ↑
		XML.Plain("<name>%s</name>\n","Test5");
		XML.Plain("<group>%s</group>\n","Group1");

		XML.OpenTag("battery");
		XML.TagOnce("state" ,"%d", 10);
		XML.TagOnce("event_unixtime" ,"%ld", WarnVal.utime);
		XML.TagOnce("level" ,"%d", 1);
		XML.CloseTag();		// battery

/*
		XML.OpenTag("comm");
		XML.TagOnce("state" ,"%d", 1);				
		XML.TagOnce("event_unixtime" ,"%ld", 0);
		XML.TagOnce("type" ,"%d", 0);
		XML.CloseTag();		
*/
		XML.CloseTag();		// device
	}


	XML.Plain( "</file>\n" );		// <file 現在値>

	XML.Output();					// バッファ吐き出し
	len = (int)strlen(xml_buffer);
	Printf("====> XML File size = %ld\r\n\r\n", strlen(xml_buffer));
	for(i=0;i<len;i++){
		Printf("%c",xml_buffer[i]);
	}

	Printf("\r\n");
	
}


/**
* 	表示グループファイルから、表示グループ情報構造にデータをロードする
*/
void vgroup_read(void)
{
	uint16_t size;
	//int size = sizeof(VGroup);
	//Printf("VGroup Struct %d\r\n", size);
	//memset(VGroup.code, 0, sizeof(VGroup));
	memset(&VGroup, 0, sizeof(VGroup));									// sakaguchi 2020.12.24


	if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS)              // シリアルフラッシュ ミューテックス確保 スレッド確定
    {
		serial_flash_multbyte_read(SFM_GROUP_START, (size_t)SFM_GROUP_SIZE, work_buffer); // 読み込み 4k to work_buffer size 32K
		
//      size = work_buffer[0] + work_buffer[1]*256;
        size = *(uint16_t *)&work_buffer[0];
		if(size > 1000){
            size = 1000;
        }
		memcpy((char *)&VGroup.code[0], &work_buffer[2], size);

		tx_mutex_put(&mutex_sfmem);
	}
}


/**
* 	表示グループファイルから、指定した子機の表示グループを抽出する
*	@param SerialCode
*	
*/
int get_vgroup_name(uint32_t SerialCode)
{
	int ret = -1;
	uint32_t sn;
	int i,k, koki_num;
	int name_len;					// グループ名サイズ
	int g_num = (int)VGroup.g_num;	// 総グループ数
	char *src;
	char gtemp[64];

	//Printf("Unit Serial %08X\r\n", SerialCode);
	memset(VGroup.Name, 0, sizeof(VGroup.Name));

	src = VGroup.data;

	if(!strncmp( "VGRP", VGroup.code, 4)){
		for(i=0;i<g_num;i++){

			name_len = *src++;
			//Printf("VGroup name length %d\r\n", name_len);
			if(name_len > 64)
				name_len = 64;
			memset(gtemp ,0, sizeof(gtemp));
			memcpy(gtemp, src, (uint32_t)name_len);
			//Printf("%s\r\n", gtemp);
			src += name_len;
			koki_num = *src++;
			src += 3;
			for(k=0;k<koki_num;k++){
				sn = *(uint32_t *)src;
				src += 4;
				//Printf("sn = %08X\r\n", sn);
				if(sn == SerialCode){
					memcpy(VGroup.Name, gtemp, (uint32_t)name_len);
					ret = 0;
					goto EXIT;
				}
			}

		}
	}
EXIT:
	return ret;
}


/**
* 	警報判定時間を秒に変換する
*	@param jtime 分
*		0の場合は、30秒
*		最上位Bitが立っている場合は、秒
*		それ以外は　x60秒	　　
*/
static uint16_t get_judgetime(uint8_t jtime)
{
	uint16_t jret;

	//if(jtime != (uint8_t)0){
	//	jret = (uint16_t)(jtime * 60);	
	//}

	if(jtime == 0){
		jret = 30;
	}
	else if((jtime & 0x80) == 0x80){
		jret = (uint16_t)(jtime & 0x7f);
	}
	else{
		jret = (uint16_t)(jtime * 60);	
	}

	Printf("Judgetime %ld (%d) \r\n", jret, jtime);

	return(jret);
}
