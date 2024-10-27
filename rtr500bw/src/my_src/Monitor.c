/**
 * @file    Monitor.c
 *
 *  @date   Created on: 2019/09/05
 *  @author     tooru.hayashi
 * @note	2020.Jul.01 GitHub 630ソース反映済み
 */


#define _MONITOR_C_


#include "Version.h"
#include "flag_def.h"
#include "MyDefine.h"
#include "Lang.h"
#include "Globals.h"
#include "Config.h"
#include "General.h"
#include "DateTime.h"
#include "Xml.h"
#include "Convert.h"
#include "Rfunc.h"
#include "Monitor.h"
#include "Log.h"
#include "ftp_thread.h"
#include "smtp_thread.h"
#include "http_thread.h"


extern TX_THREAD ftp_thread;
extern TX_THREAD smtp_thread;
//extern TX_THREAD http_thread;

static void SendMonitorTest(void);
static int MakeMonitorStruct( int ch, char *frm );

/// 子機1CH分の構造体
/// @note    パディングあり
static struct {

	uint8_t	    GroupNum;				///< グループ番号
	uint8_t	    UnitNum;				///< グループ番号
	uint8_t	    RFErr;
	uint8_t	    Method;					///< MeTableに対応する値 METHOD_***

	uint8_t	    CH;						///< 合計チャンネル数
	uint8_t 	RecordCount;			///< 記録データの数
	uint8_t	    RecordBytes;			///< 記録データのバイト数
	uint8_t	    Battery;				///< ０～５へ変換後の値
	uint8_t	    TempType;				///< oC(0), oF(1)
	uint8_t	    Rssi;					///< 電波強度０～５へ変換後の値

	uint32_t	CurrentUTC;				///< 現在値データの記録された時刻
	uint32_t	RecordUTC;				///< 記録データ値の最古データの時刻
	uint32_t	SerialNumber;			///< シリアル番号
	uint16_t    SpecNumber;				///< 機種コード（独自生成） 2020.01.17 int -> uint16_t

	uint16_t	Interval;				///< 記録間隔（秒）
	uint16_t	DataID;					///< データ識別ID
	uint32_t	CurrentData;			///< 現在値最新データ（最新部分）

	uint16_t	Data[10];				///< どの属性でも

	uint16_t	AttribCode;				///< XMLで使う属性コード

	char	    UnitName[26];			///< 子機名称    8-->26 2019 12 03
	char	    __gap1[2];

	char	    ScaleFlag;				///< スケール変換フラグ（グローバル）
	char	    ScaleMode;				///< スケール変換状態
									// 0:変換文字無し 1:変換文字あり 2:内部(505のV固定)
	char	    *ScaleUnit;				///< スケール変換ONの時の単位へのポインタ

/// 警報用
	int		    Condition;
	int32_t	    Unix_time;
	uint32_t	Value;
	uint16_t	Upper;
	uint16_t	Lower;
	uint8_t	    ALMCount;

/// RSSI末端の中継機番号
	uint8_t	    RepNo;

} MonObj;


/**
 *
 * @param Test
 * @return
 */
int32_t SendMonitorFile(int32_t Test) 
{
	UINT status;
	ULONG actual_events;
	
	// FTP or HTTP or Email

	switch(my_config.monitor.Route[0]){
		case 0:			// Email
			/*
			MakeMonitorMail(0);
			status = tx_event_flags_get (&event_flags_smtp, (ULONG)0xffffffff,TX_OR_CLEAR, &actual_events,TX_NO_WAIT);
			tx_thread_resume(&smtp_thread);

			status = tx_event_flags_get (&event_flags_smtp, (ULONG)(FLG_SMTP_SUCCESS | FLG_SMTP_ERROR),TX_OR_CLEAR, &actual_events,TX_WAIT_FOREVER);

			if( VENDER_HIT != fact_config.Vender){
				G_HttpFile[FILE_I].sndflg = SND_ON;									// [FILE]機器状態送信：ON
				tx_event_flags_set (&event_flags_http, FLG_HTTP_READY, TX_OR);		// HTTPスレッド動作許可:ON
			}
			*/
			break;
		case 1: 		// FTP
			MakeMonitorFTP();
			XML.Create(0);
    		MakeMonitorXML( Test );
			Printf(" XML File size = %ld\r\n", strlen(xml_buffer));
			//if(Test == 0){
			    status = tx_event_flags_get (&event_flags_ftp, (ULONG)0xffffffff,TX_OR_CLEAR, &actual_events,TX_NO_WAIT);
			//}
			tx_thread_resume(&ftp_thread);
			//if(Test == 0){
			    status = tx_event_flags_get (&event_flags_ftp, (ULONG)(FLG_FTP_SUCCESS | FLG_FTP_ERROR),TX_OR_CLEAR, &actual_events,TX_WAIT_FOREVER);
			//}

			if((VENDER_HIT != fact_config.Vender) && (Http_Use == HTTP_SEND)){ 		// 2020.10.16
				if((root_registry.auto_route & 0x01) == 1){		// 自動ルートONの場合 // 2023.07.27
					G_HttpFile[FILE_I].sndflg = SND_ON;									// [FILE]機器状態送信：ON
					tx_event_flags_set (&event_flags_http, FLG_HTTP_READY, TX_OR);		// HTTPスレッド動作許可:ON
				}
			}
			break;
		case 3:			// HTTP
			MakeMonitorHTTP();	 // file name set
			XML.Create(0);
    		MakeMonitorXML( Test );
			Printf(" XML File size = %ld\r\n", strlen(xml_buffer));
//sakaguchi ↓
//			HttpFile = FILE_M;
//			if(Test == 0){
//			    status = tx_event_flags_get (&event_flags_http, (ULONG)0xffffffff,TX_OR_CLEAR, &actual_events,TX_NO_WAIT);
//			}
//			tx_thread_resume(&http_thread);
//			if(Test == 0){
//			    status = tx_event_flags_get (&event_flags_http, (ULONG)(FLG_HTTP_SUCCESS | FLG_HTTP_ERROR),TX_OR_CLEAR, &actual_events,TX_WAIT_FOREVER);
//			}
// hitachi ↓
///*  Test!!
			//if(Test == 0){
			    status = tx_event_flags_get (&event_flags_http, (ULONG)0xffffffff,TX_OR_CLEAR, &actual_events,TX_NO_WAIT);
			//}
			G_HttpFile[FILE_M].sndflg = SND_ON;									// [FILE]現在値データ送信：ON
			//if( VENDER_HIT != fact_config.Vender){
			if((VENDER_HIT != fact_config.Vender) && (Http_Use == HTTP_SEND)){	// sakaguchi 2020.12.23
				if((root_registry.auto_route & 0x01) == 1){		// 自動ルートONの場合 // 2023.07.27
					G_HttpFile[FILE_I].sndflg = SND_ON;								// [FILE]機器状態送信：ON
				}
			}
			tx_event_flags_set (&event_flags_http, FLG_HTTP_READY, TX_OR);		// HTTPスレッド動作許可:ON
			//if(Test == 0){
			    status = tx_event_flags_get (&event_flags_http, (ULONG)(FLG_HTTP_SUCCESS | FLG_HTTP_ERROR | FLG_HTTP_CANCEL),TX_OR_CLEAR, &actual_events,TX_WAIT_FOREVER);
			//}
//*/
// hitachi ↑
//sakaguchi ↑
			break;
		default:
		    return (0);						// 現在値データ送信設定：無しの場合、ログは出力させない	// sakaguchi 2021.10.05
			break;

	}

	Printf(">>>>>> Monitor Send End  %04X \r\n", actual_events);
    Printf(" status =  %d \n" ,status);     //仮 2020.01.21

//	if(actual_events & FLG_SEND_ERROR){		// sakaguchi 2021.03.10
	if((actual_events & FLG_SEND_ERROR) || (actual_events & FLG_HTTP_CANCEL)){
		PutLog( LOG_LAN, "$C:$E");
		DebugLog(LOG_DBG, "$C:$E %d", status);
	    return (-1);
	}
	else{
		PutLog( LOG_LAN, "$C:$S");
	    return (0);
	}
}


/**
 * 吸い上げデータ作成・送信
 */
void MakeMonitorFTP(void)
{
// ここは修正！！ 済

//	uint16_t  fsize;
	uint32_t fsize = 0;
	uint32_t size  = 0;
	int i;
	char temp[300];


	Read_StrParam( my_config.ftp.Server , FtpHeader.SRV,  sizeof(my_config.ftp.Server));
	Read_StrParam( my_config.ftp.UserID , FtpHeader.ID ,  sizeof(my_config.ftp.UserID));
	Read_StrParam( my_config.ftp.UserPW , FtpHeader.PW ,  sizeof(my_config.ftp.UserPW));
	Read_StrParam( my_config.monitor.Dir , temp ,  sizeof(my_config.monitor.Dir));

	memset(FtpHeader.PATH,0x00,sizeof(FtpHeader.PATH));

	if((strlen(temp) == 1) && (temp[0] = '/')) {
		;	
	}
	else{
		size = GetFormatString( 0, FtpHeader.PATH, temp, 192, 0 );		// 書式変換
		if ( size > 0 ) {
			if ( FtpHeader.PATH[size-1] != '/' ) {	// 最後に'/'が無い場合
				FtpHeader.PATH[size++] = '/';	// '/'を付加
				FtpHeader.PATH[size] = 0;		// NULLを付加
			}
			if(FtpHeader.PATH[0] == '/'){			// 先頭に'/'が有った場合	
				for(i=0;i<size;i++){
					FtpHeader.PATH[i] = FtpHeader.PATH[i+1];
				}	
				size = size - 1; 	
			}
		}
	}


	Read_StrParam( my_config.monitor.Fname, temp ,  sizeof(my_config.monitor.Fname));

	Printf("  %s \r\n", temp );
	fsize = GetFormatString( 0, FtpHeader.PATH+size, temp, 64, 0 );	// 書式変換
	StrCopyN( FtpHeader.FNAME, FtpHeader.PATH+0, (int)fsize );

	fsize = GetFormatString( 0, XMLName, temp, 64, 0 );	// 書式変換
	

	Printf("  %s \r\n", XMLName );						// これでいいの？？ dir名もXMLに書かれる
	Printf("  %s \r\n", FtpHeader.FNAME );

	// PUSH用 ????


	//FTP.DataAdrs = 0;
	//FTP.DataSize = 0;



}

/**
 * 吸い上げデータ作成・送信
 */
void MakeMonitorHTTP(void)
{

	char temp[300];

	Read_StrParam( my_config.monitor.Fname, temp ,  sizeof(my_config.monitor.Fname));

	Printf(" %s (%d)\r\n", temp, strlen(temp) );
	if(strlen(temp) == 0){
		sprintf(temp, "*B-*Y*m*d-*H*i*s.xml");
	}

	GetFormatString( 0, XMLName, temp, 128, 0 );	// 書式変換

	Printf("File Name  %s \r\n", XMLName );						// これでいいの？？ dir名もXMLに書かれる

}


/**
 * 吸い上げデータ作成・送信
 * @param Test
 * @note  未使用関数 メンテナンス未実施
 */
void MakeMonitorMail(bool Test)
{
//	uint8_t	auth;
	char	*mem;
	char 	temp[300];


		// ????? LimitCopy( EmHeader.FRM, ""	);					// From(Description)

		Read_StrParam( my_config.smtp.From , EmHeader.From,  sizeof(my_config.smtp.From));		// From(Address)
		Read_StrParam( my_config.smtp.Sender , EmHeader.Sender,  sizeof(my_config.smtp.Sender));	// From(Description)


		EmHeader.TO[0] = 0;								// To(Address)

		Read_StrParam( my_config.monitor.To1 , EmHeader.TO,  sizeof(my_config.monitor.To1));	// To(Address)

		if(my_config.monitor.Bind != 0){
			Read_StrParam( my_config.monitor.Attach , EmHeader.ATA,  sizeof(my_config.monitor.Attach));	// 添付ファイル名
		
			sprintf(temp,"*Y*m*d-*H*i*s.trz");		// debug
		
			GetFormatString( 0, temp, EmHeader.ATA, 64, Download.Attach.unit_name );	// 書式変換
			StrCopyN( EmHeader.ATA, temp, 64 );
			StrCopyN( XMLName, temp, 64 );

			XML.Create(0);
    		MakeMonitorXML( Test );
		}
		else{
		    EmHeader.ATA[0] = 0;
		}
		

		Read_StrParam( my_config.monitor.Subject , EmHeader.Sbj,  sizeof(my_config.monitor.Subject));	// 


		if ( !Test ){
			mem = MakeMonitorBody();	// Body
		}
		else{
		    mem = StrCopyN( EmHeader.BDY, pLANG(RECORD_OK), 96-3 );	// "Recorded Data Test OK"
		}

		sprintf( mem, CRLF CRLF "%s" CRLF CRLF /*".\0"*/, LANG(END_EMAIL) );		// "End of mail" 最後に改行とピリオドとnullを付加

		if(strlen(EmHeader.BDY) < 128){
		    Printf("Email Body %s \r\n", EmHeader.BDY);
		}


}


/**
 * 現在値データ用 メール本文作成
 * @note   未使用の関数です メンテナンス未実施
 * @return
 */
char *MakeMonitorBody()
{
	int		grp = -1, size, ch, cnt;
	char	*mem = EmHeader.BDY;			// メール本文格納アドレス
	char	*Form;							// 子機個別データ(64byte)
	uint32_t	current;
	//char *sp, *src;
//	int len;
	char	name[sizeof(my_config.device.Name)+2];				// sakaguchi add 2020.08.27

//	sprintf(XMLTemp,"%s", my_config.device.Name );			// 親機名称		// sakaguchi cg 2020.08.27
	memset(name, 0x00, sizeof(name));
	memcpy(name, my_config.device.Name, sizeof(my_config.device.Name));
	sprintf(XMLTemp,"%s", name );           				// 親機名称

	mem += sprintf( mem, "[%s:%s]" CRLF , LANG(BASE_UNIT), XMLTemp );

	mem += sprintf( mem, "SN:%08lX" CRLF , fact_config.SerialNumber);

	mem += GetTimeString( 0, mem, my_config.device.TimeForm );	// 現在時刻で変換

	mem += sprintf( mem, CRLF CRLF );						// 改行

//	size = SRD_W( rf_ram.auto_format1.data_byte );			// 先頭の2バイト
	size = *(uint16_t *)rf_ram.auto_format1.data_byte;          // 先頭の2バイト      キャストに変更 2020.01.21
	Form = (char *)&rf_ram.auto_format1.kdata[0];			// データの先頭アドレス

	if(size < 64){
	    mem += sprintf( mem, CRLF "No Data" );				// ここは来ないはず
	}
	else
	{
		grp = -1;

		while ( size >= 64 ) {			// 64byte * n個
			MakeMonitorStruct( 0, Form );		// 処理用の構造体を作成（初期化）

			if ( grp != MonObj.GroupNum ) {	// グループ番号

				grp = MonObj.GroupNum;
				mem += sprintf( mem, CRLF "<%.8s>",  group_registry.name );
				Printf("-->%s\r\n",  group_registry.name);

			}

			mem += sprintf( mem,  CRLF "%-26.26s ", MonObj.UnitName );	// 子機名
			Printf("-->%s\r\n",  MonObj.UnitName);

			for ( ch=1; ch<=MonObj.CH; ch++ ) {		// 最大6CH

				cnt = MakeMonitorStruct( ch, Form );	// 処理用の構造体を作成
				
				if ( ch == 3 || ch == 5 ){
				    mem += sprintf( mem,  CRLF "%-8.8s ", " " );	// 空白
				}
				mem += sprintf( mem, "  CH%u ", (uint8_t)ch );
				
				current = MonObj.CurrentData;

				if ( MonObj.RFErr ){				// 無線通信エラー
					mem += sprintf( mem, LANG(COMERR) );	// "ComErr " );
				}
				else if ( ( current == 0xEEEE || current == 0xF00F ) && MonObj.SpecNumber != SPEC_505PS ){
				    mem += sprintf( mem, LANG(SNSERR) );	// "SnsErr " );
				}
				else if ( current == 0xF000 && MonObj.SpecNumber != SPEC_505PS ){
				    mem += sprintf( mem, LANG(NODATA) );	// "NoData " );
				}
				else if ( current == 0xF001 && MonObj.SpecNumber != SPEC_505PS ){
				    mem += sprintf( mem, LANG(UNDERERR) );	// "Under " );
				}
				else if ( current == 0xF002 && MonObj.SpecNumber != SPEC_505PS ){
				    mem += sprintf( mem, LANG(OVERERR) );	// "Over " );
				}
				else
				{
					mem += MeTable[MonObj.Method].Convert( mem, current );
					if ( MonObj.ScaleFlag && MonObj.ScaleMode ){
					    mem += sprintf( mem, " %.64s", MonObj.ScaleUnit );
					}
					else if ( MonObj.ScaleMode == 2 ){
					    mem += sprintf( mem, " %.64s", MonObj.ScaleUnit );
					}
					else
					{
						if ( MonObj.SpecNumber != SPEC_505PS ){
							mem += sprintf( mem, " %s", MeTable[MonObj.Method].TypeChar );
							Printf(" Unit [%s] \r\n", MeTable[MonObj.Method].TypeChar);
						}
						else if ( MonObj.Interval < 60 ){
						    mem += sprintf( mem, " %s/%usec", MeTable[MonObj.Method].TypeChar, MonObj.Interval );
						}
						else{
						    mem += sprintf( mem, " %s/%umin", MeTable[MonObj.Method].TypeChar, MonObj.Interval/60U );
						}
					}
				}

			}

			if ((uint32_t)(mem - EmHeader.BDY) > 1500 ) {	// 1.5Kbyte以内なので
				mem += sprintf( mem, CRLF "Over capacity" CRLF );
				PutLog( LOG_LAN,"Email is over capacity");
				break;
			}


			Form += cnt;
			size -= cnt;

		}
	}


	
	return (mem);
}



/**
 * @brief   現在値データ作成・送信
 * 作成しながら送信しない
 * @param Test
 */
void MakeMonitorXML( int32_t Test )
{
    int     grp = -1, size, ch, valid;
    int     temp, count;
	uint32_t	current;
	char	multi[8+2];			// 積算データ
	char	*Form;				// 子機個別データへのポインタ
	char	*ptr;	


	int i,j, len;
	char *pt = (char *)&rf_ram.auto_rpt_info.kdata[0];
	char	name[sizeof(my_config.device.Name)+2];				// sakaguchi add 2020.08.27

	memset(FormatScale, 0x00, sizeof(FormatScale));				// sakaguchi 2021.04.16

	if ( Test ) {				// テスト送信？
		SendMonitorTest();
	}
	else
	{

		vgroup_read();					// 表示グループ情報取得

		MonObj.TempType =  (uint8_t)my_config.device.TempUnits[0];	            // oC(0) or oF(1)
		sprintf(XMLTForm,"%s", my_config.device.TimeForm);		    // 日付フォーマット

		XML.Plain( XML_FORM_CURRENT );				                // <file .. name=" まで
		//XML.Entity( XML.FileName );					                // ファイル名（エンコード付き）
		XML.Plain( "%s", XML.FileName);

		memset(name, 0x00, sizeof(name));
		memcpy(name, my_config.device.Name, sizeof(my_config.device.Name));
		sprintf(XMLTemp,"%s", name );
		XML.Plain( "\" author=\"%s\">\n", XMLTemp);


		XML.OpenTag( "base" );

		sprintf(XMLTemp,"%08lX",fact_config.SerialNumber);
		XML.TagOnce( "serial", XMLTemp /*fact_config.SerialNumber*/); 	        // シリアル番号

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

			//XML.TagOnce( "name", my_config.device.Name );           // 親機名称
			//sprintf(XMLTemp,"%s", my_config.device.Name ); 
			//XML.TagOnce( "name", XMLTemp ); 				          // 親機名称
//			XML.Plain( "<name>%s</name>\n", my_config.device.Name);	 // 親機名称	
			XML.Plain( "<name>%s</name>\n", name);	 				// 親機名称				// sakaguchi cg 2020.08.27

			temp = (int)( RTC.Diff / 60 );		        	        // 時差
			XML.TagOnce( "time_diff", "%d", temp );
			XML.TagOnce( "std_bias", "0" );			                //
			temp = (int)( RTC.AdjustSec / 60 );		                // サマータイム中
			XML.TagOnce( "dst_bias", "%d", temp );
			//XML.Plain( "<time_zone>%s</time_zone>\n", my_config.device.TimeZone);
			memcpy(XMLTemp, my_config.device.TimeZone, sizeof(my_config.device.TimeZone));
			XML.TagOnce2( "time_zone", XMLTemp);	

			XML.OpenTag( "lan" );
				//XML.TagOnce( "output", "%u", ExternalOut(-1) );	    // 出力ピンの状態
			XML.CloseTag();

		XML.CloseTag();		// <base>

        size = *(uint16_t *)rf_ram.auto_format1.data_byte;  // 先頭の2バイト     キャストに変更 2020.01.21
		Form = (char *)&rf_ram.auto_format1.kdata[0];		// データの先頭アドレス
		Printf("monitor data size %d\r\n", size);
// 2022.03.23 del
//#ifdef DBG_TERM
//		if(size != 1664)		// debug 2021.01.27
//			Printf("monitor data size error %d\r\n", size);
//
//		ptr = (char *)&rf_ram.auto_format1.kdata[0];
//		for(i=0;i<size;i+=64){
//			for(j=0;j<64;j++){
//				Printf("%02X ", *ptr++);
//			}
//			Printf("\r\n");
//		}		
//#endif
		while ( size >= 64 ) {			// 64byte * n個

			MakeMonitorStruct( 0, Form );		// 処理用の構造体を作成（初期化）

			if ( grp != MonObj.GroupNum ) {		// グループ番号

				if ( grp != -1 )		// 最初ではない？
				{
				    XML.CloseTag();		// <group>を閉じておく
				}

				grp = MonObj.GroupNum;
				XML.OpenTag( "group" );
				XML.TagOnce( "num", "%d", grp );
				XML.Plain("<name>%s</name>\n", group_registry.name);
				//XML.Plain("<repeater></repeater>\n");

				Printf("%02X %02X\r\n", rf_ram.auto_rpt_info.data_byte[0],rf_ram.auto_rpt_info.data_byte[1]);

				for(i=0;i<64;i++){
					Printf("%02X ", *pt++);
				}
				Printf("\r\n");

			}

			get_vgroup_name( MonObj.SerialNumber);

			XML.OpenTag( "remote" );

			if(strlen(VGroup.Name) != 0)
				//XML.Plain("<alias_group>%s</alias_group>\n", VGroup.Name);
				XML.TagOnce2("alias_group", "%s", VGroup.Name);		// エスケープ処理追加	// sakaguchi 2021.04.16

			XML.TagOnce( "serial", "%08lX", MonObj.SerialNumber );	// HEX8文字

			XML.TagOnce( "model", "%.32s", GetModelName( MonObj.SerialNumber, MonObj.SpecNumber ) );	// モデル名

			XML.TagOnce( "num", "%d", MonObj.UnitNum );				// 子機番号
			//XML.TagOnce( "name", "%.26s", MonObj.UnitName );			// 子機名   8-->26
			XML.Plain("<name>%.26s</name>\n", MonObj.UnitName);

			sprintf( XMLTemp, "repeater=\"%u\"", MonObj.RepNo );		// 属性の作成
			XML.TagOnceAttrib( "rssi", XMLTemp, "%u", MonObj.Rssi );	// 末端の中継機番号 2017-12-12 Ver3.00


			Printf("Serial %08X\r\n",  MonObj.SerialNumber);
			for ( ch=1; ch<=MonObj.CH; ch++ ) {		// 最大6CH

				count = MakeMonitorStruct( ch, Form );	// 処理用の構造体を作成

				XML.OpenTag( "ch" );

					XML.TagOnce( "num", "%d", ch );		// CH番号

					FormatScaleStr( FormatScale, ScaleString );
					//XML.TagOnceEmpty( MonObj.ScaleFlag && MonObj.ScaleMode == 1, "scale_expr", "%.64s", FormatScale );	// スケール変換
					if(MonObj.ScaleFlag && MonObj.ScaleMode == 1){
						//XML.Plain("<scale_expr>%.70s</scale_expr>\n", FormatScale);
						XML.TagOnce2("scale_expr", "%.70s", FormatScale);			// エスケープ処理追加	// sakaguchi 2021.04.16
					}
					else{
						XML.Plain("<scale_expr></scale_expr>\n");
					}
					XML.TagOnce( "name", "" );			// CH名

					if(MonObj.SpecNumber == SPEC_505BLX){
						Printf("KCG Mode \r\n");
					}
					current = MonObj.CurrentData;

					//XML.OpenTag( "current" );
// sakaguchi 2021.01.25 ↓
//					if ( MonObj.RFErr ){				// 無線通信エラー
//						XML.OpenTagAttrib( "current", "state=\"1\"");
//						Printf("******   Communication Error \r\n");
//					}
					if ( MonObj.RFErr == 0xCC ){						// 無線通信エラー
						XML.OpenTagAttrib( "current", "state=\"1\"");
						Printf("******   Communication Error \r\n");
					}
					else if ( MonObj.RFErr == 0xCB ){					// チャンネルビジー
						XML.OpenTagAttrib( "current", "state=\"5\"");	// Channel Busy
						Printf("******   Channel Busy \r\n");
					}
// sakaguchi 2021.01.25 ↑
// sakaguchi 2021.04.07 ↓
					else if ( MonObj.SpecNumber == SPEC_576 && ( 0xF001UL <= current && current < 0xF005UL ) ){	// 576のみ
						XML.OpenTagAttrib( "current", "state=\"2\"");	// Senser Error  576のみ
					}
					else if ( MonObj.SpecNumber == SPEC_576 && ( current == 0xF005UL ) ){	// 576のみ
						XML.OpenTagAttrib( "current", "state=\"4\"");	// Overflow  576のみ
					}
					else if ( ( current == 0xEEEE || current == 0xF00F ) && ( MonObj.SpecNumber != SPEC_505PS && MonObj.SpecNumber != SPEC_505BPS ) ){	// 505PS,505BPS以外
						XML.OpenTagAttrib( "current", "state=\"2\"");	// Senser Error
					}
					else if ( current == 0xF001 ){																	// 全機種共通
						XML.OpenTagAttrib( "current", "state=\"4\"");	// Underflow
					}
					else if ( current == 0xF002 ){																	// 全機種共通
						XML.OpenTagAttrib( "current", "state=\"4\"");	// Overflow
					}
					else if ( current == 0xF000 ){																	// 全機種共通
						XML.OpenTagAttrib( "current", "state=\"3\"");	// No Data
					}
					else{
						XML.OpenTagAttrib( "current", "state=\"0\"");
					}

					//else if ( ( current == 0xEEEE || current == 0xF00F ) && MonObj.SpecNumber != SPEC_505PS ){
					//	XML.OpenTagAttrib( "current", "state=\"2\"");	// Senser Error
					//}
					//else if ( MonObj.SpecNumber == SPEC_576 && ( 0xF001UL <= current && current <= 0xF005UL ) ){
					//	XML.OpenTagAttrib( "current", "state=\"2\"");	// Senser Error  576のみ
					//}
					//else if ( current == 0xF000 && MonObj.SpecNumber != SPEC_505PS ){
					//	XML.OpenTagAttrib( "current", "state=\"3\"");	// No Data
					//}
					//else if ( current == 0xF001 && MonObj.SpecNumber != SPEC_505PS ){
					//	XML.OpenTagAttrib( "current", "state=\"4\"");	// Underflow
					//}
					//else if ( current == 0xF002 && MonObj.SpecNumber != SPEC_505PS ){
					//	XML.OpenTagAttrib( "current", "state=\"4\"");	// Overflow
					//}

					//else if ( ( current == 0xEEEE || current == 0xF00F ) && MonObj.SpecNumber != SPEC_505BPS ){
					//	XML.OpenTagAttrib( "current", "state=\"2\"");	// Senser Error
					//}
					//else if ( current == 0xF000 && MonObj.SpecNumber != SPEC_505BPS ){
					//	XML.OpenTagAttrib( "current", "state=\"3\"");	// No Data
					//}
					//else if ( current == 0xF001 && MonObj.SpecNumber != SPEC_505BPS ){
					//	XML.OpenTagAttrib( "current", "state=\"4\"");	// Underflow
					//}
					//else if ( current == 0xF002 && MonObj.SpecNumber != SPEC_505BPS ){
					//	XML.OpenTagAttrib( "current", "state=\"4\"");	// Overflow
					//}
					//else{
					//	XML.OpenTagAttrib( "current", "state=\"0\"");
					//}
// sakaguchi 2021.04.07 ↑

					XML.TagOnce( "unix_time", "%lu", MonObj.CurrentUTC );	// 現在値の時刻
					GetFormatString( MonObj.CurrentUTC, XMLTemp, XMLTForm, sizeof(XMLTemp)-2, 0 );
					XML.TagOnce( "time_str", "%s", XMLTemp );

					current = MonObj.CurrentData;

					valid = 0;
// sakaguchi 2021.01.25 ↓
					//if ( MonObj.RFErr ){				// 無線通信エラー
					//	XML.TagOnceAttrib( "value", "valid=\"false\"", "Communication Error" );
					//}
					if ( MonObj.RFErr == 0xCC ){							// 無線通信エラー
						XML.TagOnceAttrib( "value", "valid=\"false\"", "Communication Error" );
					}
					else if ( MonObj.RFErr == 0xCB ){						// チャンネルビジー
						XML.TagOnceAttrib( "value", "valid=\"false\"", "Channel Busy" );
					}
// sakaguchi 2021.01.25 ↑
// sakaguchi 2021.04.07 ↓
					else if ( MonObj.SpecNumber == SPEC_576 && ( 0xF001UL <= current && current < 0xF005UL ) ){	// 576のみ
					    XML.TagOnceAttrib( "value", "valid=\"false\"", "Sensor Error" );
					}
					else if ( MonObj.SpecNumber == SPEC_576 && ( current == 0xF005UL ) ){	// 576のみ
					    XML.TagOnceAttrib( "value", "valid=\"false\"", "Overflow" );
					}
					else if ( ( current == 0xEEEE || current == 0xF00F ) && ( MonObj.SpecNumber != SPEC_505PS && MonObj.SpecNumber != SPEC_505BPS ) ){	// 505PS,505BPS以外
					    XML.TagOnceAttrib( "value", "valid=\"false\"", "Sensor Error" );
					}
					else if ( current == 0xF001 ){																	// 全機種共通
						XML.TagOnceAttrib( "value", "valid=\"false\"", "Underflow" );
					}
					else if ( current == 0xF002 ){																	// 全機種共通
						XML.TagOnceAttrib( "value", "valid=\"false\"", "Overflow" );
					}
					else if ( current == 0xF000 ){																	// 全機種共通
						XML.TagOnceAttrib( "value", "valid=\"false\"", "No Data" );
					}

					//else if ( ( current == 0xEEEE || current == 0xF00F ) && MonObj.SpecNumber != SPEC_505PS ){
					//    XML.TagOnceAttrib( "value", "valid=\"false\"", "Sensor Error" );
					//}
					//else if ( MonObj.SpecNumber == SPEC_576 && ( 0xF001UL <= current && current <= 0xF005UL ) ){
					//    XML.TagOnceAttrib( "value", "valid=\"false\"", "Sensor Error" );	// 576のみ
					//}
					//else if ( current == 0xF000 && MonObj.SpecNumber != SPEC_505PS ){
					//    XML.TagOnceAttrib( "value", "valid=\"false\"", "No Data" );
					//}
					//else if ( current == 0xF001 && MonObj.SpecNumber != SPEC_505PS ){
					//    XML.TagOnceAttrib( "value", "valid=\"false\"", "Underflow" );
					//}
					//else if ( current == 0xF002 && MonObj.SpecNumber != SPEC_505PS ){
					//    XML.TagOnceAttrib( "value", "valid=\"false\"", "Overflow" );
					//}
					//else if ( ( current == 0xEEEE || current == 0xF00F ) && MonObj.SpecNumber != SPEC_505BPS ){
					//    XML.TagOnceAttrib( "value", "valid=\"false\"", "Sensor Error" );
					//}
					//else if ( current == 0xF000 && MonObj.SpecNumber != SPEC_505BPS ){
					//    XML.TagOnceAttrib( "value", "valid=\"false\"", "No Data" );
					//}
					//else if ( current == 0xF001 && MonObj.SpecNumber != SPEC_505BPS ){
					//    XML.TagOnceAttrib( "value", "valid=\"false\"", "Underflow" );
					//}
					//else if ( current == 0xF002 && MonObj.SpecNumber != SPEC_505BPS ){
					//    XML.TagOnceAttrib( "value", "valid=\"false\"", "Overflow" );
					//}
// sakaguchi 2021.04.07 ↑
					else
					{
						// Methodに従って変換
						MeTable[MonObj.Method].Convert( ConvertTemp, current );
						XML.TagOnceAttrib( "value", "valid=\"true\"", ConvertTemp );	// 構築した値を出力
						valid = 1;
					}

					if ( valid == 0 )				// valid ="false" ?
					{
						XML.TagOnceEmpty( 0, "unit",  "" );			// 空タグ
					}
					else
					{
						Printf("Scale flag %02X  Scale mode %02X\r\n", MonObj.ScaleFlag,MonObj.ScaleMode);
						if ( MonObj.ScaleFlag && MonObj.ScaleMode ){	// 変換有り
						    //XML.TagOnceEmpty( MonObj.AttribCode, "unit", "%.64s", MonObj.ScaleUnit );
// sakaguchi 2021.01.18 ↓
							//XML.Plain("<unit>%.64s</unit>\n", MonObj.ScaleUnit);
							if ( MonObj.SpecNumber != SPEC_505PS && MonObj.SpecNumber != SPEC_505BPS){
								//XML.TagOnceEmpty( MonObj.AttribCode, "unit", "%.64s", MonObj.ScaleUnit );
								//XML.Plain("<unit>%.64s</unit>\n", MonObj.ScaleUnit);
								XML.TagOnce2( "unit", "%.64s", MonObj.ScaleUnit );									// エスケープ処理追加	// sakaguchi 2021.04.16
							}
							else if ( MonObj.Interval < 60 ){
								if(ch == 1){
									//XML.TagOnceEmpty( MonObj.AttribCode, "unit", "%.64s/%usec", MonObj.ScaleUnit, MonObj.Interval );
									//XML.Plain("<unit>%.64s/%usec</unit>\n", MonObj.ScaleUnit, MonObj.Interval );
									XML.TagOnce2( "unit", "%.64s/%usec",  MonObj.ScaleUnit, MonObj.Interval );		// エスケープ処理追加	// sakaguchi 2021.04.16
								}
								else{
									//XML.TagOnceEmpty( MonObj.AttribCode, "unit", "%.64s", MonObj.ScaleUnit);
									//XML.Plain("<unit>%.64s</unit>\n", MonObj.ScaleUnit);
									XML.TagOnce2( "unit", "%.64s",  MonObj.ScaleUnit );								// エスケープ処理追加	// sakaguchi 2021.04.16
								}
							}
							else{
								if(ch == 1){
									//XML.TagOnceEmpty( MonObj.AttribCode, "unit", "%.64s/%umin", MonObj.ScaleUnit, MonObj.Interval/60U );
									//XML.Plain("<unit>%.64s/%umin</unit>\n", MonObj.ScaleUnit, MonObj.Interval/60U);
									XML.TagOnce2( "unit", "%.64s/%umin",  MonObj.ScaleUnit, MonObj.Interval/60U );	// エスケープ処理追加	// sakaguchi 2021.04.16
								}
								else{
									//XML.TagOnceEmpty( MonObj.AttribCode, "unit", "%.64s", MonObj.ScaleUnit );
									//XML.Plain("<unit>%.64s</unit>\n", MonObj.ScaleUnit );
									XML.TagOnce2( "unit", "%.64s",  MonObj.ScaleUnit );								// エスケープ処理追加	// sakaguchi 2021.04.16
								}
							}
// sakaguchi 2021.01.18 ↑
						}
						else if ( MonObj.ScaleMode == 2 ){		// 内部変換
						    XML.TagOnceEmpty( MonObj.AttribCode, "unit", "%.64s", MonObj.ScaleUnit );
						}
						else
						{
//							if ( MonObj.SpecNumber != SPEC_505PS )
							if ( MonObj.SpecNumber != SPEC_505PS && MonObj.SpecNumber != SPEC_505BPS){
							    XML.TagOnceEmpty( MonObj.AttribCode, "unit", "%s", MeTable[MonObj.Method].TypeChar );
							}
							else if ( MonObj.Interval < 60 ){
							    if(ch == 1){
									XML.TagOnceEmpty( MonObj.AttribCode, "unit", "%s/%usec", MeTable[MonObj.Method].TypeChar, MonObj.Interval );
								}
								else{
									XML.TagOnceEmpty( MonObj.AttribCode, "unit", "%s", MeTable[MonObj.Method].TypeChar);
								}
							}
							else{
								if(ch == 1){
							    	XML.TagOnceEmpty( MonObj.AttribCode, "unit", "%s/%umin", MeTable[MonObj.Method].TypeChar, MonObj.Interval/60U );
								}
								else{
									XML.TagOnceEmpty( MonObj.AttribCode, "unit", "%s", MeTable[MonObj.Method].TypeChar );
								}
							}
						}
					}

					XML.TagOnceEmpty( MonObj.Battery != 0xFF, "batt", "%u", MonObj.Battery );

					XML.CloseTag();		// current

					XML.OpenTag( "record" );
Printf("\r\n --- Monitor XML  type  %u ----- \r\n\r\n ", MonObj.AttribCode);
					XML.TagOnceEmpty( MonObj.RecordCount, "type",      "%u",  MonObj.AttribCode );
					XML.TagOnceEmpty( MonObj.RecordCount, "unix_time", "%lu", MonObj.RecordUTC );

					if ( ch < 5 ) {		// CH1 - CH4 ?

						XML.TagOnceEmpty( MonObj.RecordCount, "data_id",   "%u",  MonObj.DataID );
						XML.TagOnceEmpty( MonObj.RecordCount, "interval",  "%u",  MonObj.Interval );
						XML.TagOnce( "count", "%d", MonObj.RecordCount );
						XML.OpenTag( "data" );
							if ( MonObj.RecordCount ) {
								XML.Base64( true, (char *)( &MonObj.Data[0] ), MonObj.RecordBytes );
								XML.Plain( "\n" );
							}
						XML.CloseTag();	// data
					}
					else {		// CH5, CH6

						XML.TagOnceEmpty( 0, "data_id",  "" );
						XML.TagOnceEmpty( 0, "interval", "" );
						XML.TagOnce( "count", "%d", MonObj.RecordCount );

						memset( &multi[0], 0, sizeof(multi) );		// クリア
						memcpy( &multi[0], &MonObj.CurrentUTC, sizeof(MonObj.CurrentUTC) );	// 吸い上げ時の時刻
						memcpy( &multi[8], &MonObj.Data[0], 2 );	// 素データ１つ

						XML.OpenTag( "data" );
						if ( MonObj.RecordCount ) {
							XML.Base64( true, multi, sizeof(multi) );
							XML.Plain( "\n" );
						}
						XML.CloseTag();	// data
					}
						
					XML.CloseTag();		// record

				XML.CloseTag();		// CH
			}
				
			XML.CloseTag();		// remote


			Form += count;
			size -= count;
			
		}

		XML.CloseAll();

		XML.Plain( "</file>\n" );

		XML.Output();					// バッファ吐き出し

	}
	Printf("====> XML File size = %ld\r\n", strlen(xml_buffer));

	
	len = (int)strlen(xml_buffer);
	Printf("====> XML File size = %ld\r\n\r\n", strlen(xml_buffer));
	for(i=0;i<len;i++){
		Printf("%c",xml_buffer[i]);
	}

	Printf("\r\n");
	

}


/**
 * @brief   XML用の構造体作成
 * @param ch        ch=0: 初期化,  １～６
 * @param frm
 * @return          戻り値は次データへの加算値
 */
static int MakeMonitorStruct( int ch, char *frm )
{
	int		count, pos, res = sizeof(MON_FORM);
	char	type, attrib, add;
	char *start;
	char *tmp;
	char *pp;
	int ii;
	uint32_t	ltemp;
	MON_FORM	*Form = (MON_FORM *)frm;
//	struct def_ALM_bak	*Alm = &ALM_bak[0];
	char	grp_name[sizeof(group_registry.name)+2];

	if ( ch == 0 ) {			// 初期化

		MonObj.SerialNumber = *(uint32_t *)Form->serial;    // 子機のシリアル番号	2020.01.21 キャストに変更
		MonObj.SpecNumber = GetSpecNumber(MonObj.SerialNumber,Form->kisyu505);
		MonObj.CH = (uint8_t)( MonObj.SpecNumber & 0x00FF );		// 下位4ビットでCH数を表すので

		if(MonObj.CH == 0){
		    MonObj.CH = 1;		// 未知の機種はch個数0としていたのを1に修正	2019/10/23 segi
		}
		
		

		MonObj.UnitNum = Form->unit_no;			// 子機番号
		MonObj.GroupNum = Form->group_no;		// グループ番号
		get_regist_group_adr(MonObj.GroupNum);		//  グループ情報取得 regist.group.size[0]～

		StrCopyN( grp_name, group_registry.name, sizeof(group_registry.name));
		// 子機情報を取得し
		get_regist_relay_inf( MonObj.GroupNum, MonObj.UnitNum, rpt_sequence);	// 505のスケール変換取得のため
		// グループ名を元に戻す
		StrCopyN( group_registry.name, grp_name, sizeof(group_registry.name));

		if(ru_registry.rtr501.header == 0xFE || ru_registry.rtr501.header == 0xF8){		// 501 505
			StrCopyN( MonObj.UnitName, ru_registry.rtr501.ble.name,  sizeof(ru_registry.rtr501.ble.name) );	// 子機名
		}
		else{																			// 574 576
			StrCopyN( MonObj.UnitName, ru_registry.rtr574.name2, sizeof(ru_registry.rtr574.name2) );	// 子機名
		}
		MonObj.ScaleFlag = Form->scale & 0x01;	// スケール変換
		MonObj.ScaleMode = 1;					// 変換文字あり

		// スケール変換せずに505の電圧[mV]の時[V]固定なら内部でスケール変換
		if ( !MonObj.ScaleFlag && MonObj.SpecNumber == SPEC_505V && ( (Form->disp_unit & 0xF0) == 0xA0 || (Form->disp_unit &0x0F) == 0x0A ) ) { /// 505のV固定かどうか
			strcpy( ru_registry.rtr505.scale_setting, SCALE_505V );
			MonObj.ScaleMode = 2;				// 内部処理に設定
		}

		if ( !MonObj.ScaleFlag && MonObj.SpecNumber == SPEC_505BV && ( (Form->disp_unit & 0xF0) == 0xA0 || (Form->disp_unit &0x0F) == 0x0A ) ) { /// 505BのV固定かどうか
			strcpy( ru_registry.rtr505.scale_setting, SCALE_505V );
			MonObj.ScaleMode = 2;				// 内部処理に設定
		}

		// KGCモード ?
		if (MonObj.SpecNumber == SPEC_505BLX){
			if((MonObj.ScaleFlag == 1)&&((ru_registry.rtr505.scale_setting[0] != '\n')||(ru_registry.rtr505.scale_setting[1] != '\n')||(ru_registry.rtr505.scale_setting[2] != '\n')||(ru_registry.rtr505.scale_setting[3] != '\n'))){			// 変化式有りフラグONで有効な値入っていれば登録値を採用
			}
			else{
//			if(MonObj.ScaleFlag){
				strcpy( ru_registry.rtr505.scale_setting, SCALE_505Lx );						// スケール変換式なければ L5 L10 [dB]
			}
		} 
		/*
		KGC_ON = 0;																	// KGCモードなら KGC_ON=1 にする
		if (MonObj.SpecNumber == SPEC_505V || MonObj.SpecNumber == SPEC_505BV){		// KGCモードなら KGC_ON=1 にする
			if(Form->kgc_mode != 0)KGC_ON = ((Form->kgc_mode) & 0x07);				// KGCモードなら KGC_ON=1 にする
		}																			// KGCモードなら KGC_ON=1 にする
		if(KGC_ON != 0){
			if((MonObj.ScaleMode == 0 || MonObj.ScaleMode == 2) && (MonObj.SpecNumber == SPEC_505BV)){	// KGCモード対応
				strcpy( ru_registry.rtr505.scale_setting, SCALE_505Lx );		// L5 L10 [dB]
			}
		}
		*/


		// 最後にnullを入れる処理(64バイト時は何もしない)
//		if ( MonObj.ScaleFlag )
//			CheckScaleStr( regist.unit505.scale_conv, &tmp, &tmp, &(MonObj.ScaleUnit) );

		if ( Form->rf_err == 0xCC || Form->rf_err == 0xCB ) {		// 無線通信エラー状態
			MonObj.Rssi = 0;
			MonObj.RepNo = 0;										// 末端の中継機番号
		} else {
			MonObj.Rssi = rtr500_RSSI_1to5( Form->rssi );
			MonObj.RepNo = Form->rep_no;							// 末端の中継機番号
		}

		return ( 0 );
	}

	if ( MonObj.ScaleFlag || MonObj.ScaleMode == 2 ) {
		GetScaleStr( ru_registry.rtr505.scale_setting, ch - 1 );	// CH毎のスケール変換文字列

		 if ( CheckScaleStr( &tmp, &tmp, &(MonObj.ScaleUnit) ) < 0 ){
			MonObj.ScaleMode = 0;				// 変換文字列無し
		}
		else if ( !MonObj.ScaleMode ){
		    MonObj.ScaleMode = 1;
		}
	}

	type = MonObj.TempType;						// oC or oF
	add = 0;									// 積算以外

	//  1CH 毎に必要な情報を構築
	switch ( MonObj.SpecNumber ) {
	
		case SPEC_501:		// 501/502
		case SPEC_501B:		// 501/502
		case SPEC_502:
		case SPEC_502B:
		case SPEC_502PT:
			count = sizeof(Form->rec.ex501.temp);
			start = Form->rec.ex501.temp;
			break;

		case SPEC_503:
		case SPEC_503B:
		case SPEC_507:
		case SPEC_507B:
			count = sizeof(Form->rec.ex503.temp);
			start = ( ch == 1 ) ? Form->rec.ex503.temp : Form->rec.ex503.hum;
			break;

		case SPEC_574:
			Printf("RTR574 \r\n");
			if ( ch <= 2 ) {			// 0:照度, 1:UV ( 503と同等 )
				count = sizeof(Form->rec.ex574.lx);
				start = ( ch == 1 ) ? Form->rec.ex574.lx : Form->rec.ex574.uv;
//				type = 0;				// 積算ではない
			}
			else if ( ch <= 4 ) {
				Form++;					// この時だけ次のブロックを使う
				count = sizeof(Form->rec.ex503.temp);
				start = ( ch == 3 ) ? Form->rec.ex503.temp : Form->rec.ex503.hum;
			}
			else {
				count = sizeof(Form->rec.ex574.mp_lx);
				start = ( ch == 5 ) ? Form->rec.ex574.mp_lx : Form->rec.ex574.mp_uv;
//				type = 1;				// 積算です
				add = 1;				// 積算です
			}

			res *= 2;			// 2個分

			break;

		case SPEC_576:
			pp = &Form->group_no;
			Printf("RTR576 \r\n");
			for(ii=0;ii<64;ii++){
				Printf("%02X ", *pp++);
			}
			Printf("\r\n");
			if ( ch <= 1 ) {			// CH1:CO2 ( 501と同等 )
				count = sizeof(Form->rec.ex501);
				start = Form->rec.ex501.temp;
			}
			else if ( ch <= 3 ) {		// CH2:温度 CH3:湿度 ( 503と同等 )
				Form++;					// この時だけ次のブロックを使う
				count = sizeof(Form->rec.ex503.temp);
				start = ( ch == 2 ) ? Form->rec.ex503.temp : Form->rec.ex503.hum;
				ch++;		// この後のattribのためだけにこうする
			}

			res *= 2;			// 2個分

			break;

		case SPEC_505PS:		// パルス
		case SPEC_505BPS:		// パルス
			if ( ch == 2 ) {
				Form->attrib[1] = Form->attrib[0];	// 属性コピーしちゃえ
				add |= 0x02;			// 総パルスフラグセット
			}
			/* no break */
		case SPEC_505V:
		case SPEC_505BV:
		case SPEC_505BLX:
		
			add = (uint8_t)(add | ((MonObj.ScaleMode == 2) ? 1 : 0));	// 505電圧 内部変換
			/* no break */
		case SPEC_505A:
		case SPEC_505BA:
			add = (uint8_t)(add | ((MonObj.ScaleFlag) ? 1 : 0));	// 505パルス・電圧・電流のみスケール変換可能
			/* no break */
		case SPEC_505TC:
		case SPEC_505PT:
		case SPEC_505BTC:
		case SPEC_505BPT:
			count = sizeof(Form->rec.ex505.pulse);
			start = Form->rec.ex505.pulse;
			break;

		default:
			count = 0;
			start = Form->rec.ex501.temp;	// デフォルト位置
			attrib = type = add = 0;
			break;
	}

	//MonObj.RFErr = ( Form->rf_err == 0xCC || Form->rf_err == 0xCB ) ? 1 : 0;	// 無線通信エラー状態
	MonObj.RFErr = ( Form->rf_err == 0xCC || Form->rf_err == 0xCB ) ? Form->rf_err : 0;	// 無線通信エラー状態	// sakaguchi 2021.01.25

	attrib = Form->attrib[( ch - 1 ) % 2];				// ブロック0のCH属性
	
	MonObj.Method = (uint8_t)GetConvertMethod( attrib, type, add );	// Method取得
	Printf("MakeMonitorStruct  attrib=%02X type=%02X add=%02X (%d) RF(%02X) Met=%02X Spec=%02X\r\n", attrib, type, add, ch, Form->rf_err, MonObj.Method, MonObj.SpecNumber );
	MonObj.AttribCode = MeTable[MonObj.Method].DataCode;			// XML返送用属性

	//if(MonObj.AttribCode == 0x0092){
	//	if(KGC_ON != 0){
	//		MonObj.AttribCode = 0x009A;		// KGCモードの記録属性
	//	}
	//}
	if( MonObj.SpecNumber == SPEC_505BLX)
	{
		MonObj.AttribCode = 0x009A;		// KGCモードの記録属性
	}

	memcpy( MonObj.Data, start, (uint32_t)count );				// 実体コピー

//	if ( MonObj.SpecNumber == SPEC_505PS )				// 505のパルスだけ特殊
	if ((MonObj.SpecNumber == SPEC_505PS)||(MonObj.SpecNumber == SPEC_505BPS))				// 505のパルスだけ特殊
	{
		if ( ch == 1 )
		{										// 総パルス５個をモニタ間隔毎の４個へ変換
			MonObj.RecordCount = 4;				// 常に4(2byte×4)
			MonObj.RecordBytes = 8;				// 常に8バイト

			for ( count=0; count<4; count++ )
			{
                ltemp  = *(uint32_t *) (start+4);    // 新                        2020.01.21 キャストに変更
				Printf("	Pulse ch1-1 %ld (%d)\r\n", ltemp, count);
                ltemp -= *(uint32_t *) start;        // 古                        2020.01.21 キャストに変更
				Printf("	Pulse ch1-2 %ld (%d)\r\n", ltemp, count);

				start += 4;

				if ( ltemp >= 0xF000 )			// オーバー？
				{
				    ltemp = 0xF002;				// 測定範囲外
				}

				MonObj.Data[ count ] = (uint16_t)ltemp;		// 2byteデータへ
			}
            ltemp  = *(uint32_t *) start ;      // 最新                             2020.01.21 キャストに変更
			Printf("	Pulse ch1-1 %ld\r\n", ltemp);
            ltemp -= *(uint32_t *) (start-4);   // 最新 - (最新-1)   2020.01.21 キャストに変更
			MonObj.CurrentData = ltemp;			// 最終インターバル間データ
			Printf("	Pulse ch1-2 %ld\r\n", ltemp);

		}
		else	// CH=2
		{
			MonObj.RecordCount = 5;						// 常に5(4byte×5)
			MonObj.RecordBytes = (uint8_t)count;			// 常に20バイト
	
			memcpy( MonObj.Data, start, (uint32_t)count );		// 実体コピー

//			ltemp  = SRD_L( (start+16) );		// 最新
			ltemp  = *(uint32_t *)(start+16);       // 最新   2020.01.21 キャストに変更
//			ltemp -= SRD_L( (start+12) );		// 最新 - (最新-1)

			MonObj.CurrentData = ltemp;			// 最終インターバル間データ
			Printf("	Pulse ch2 %ld\r\n", ltemp);
		}
	}
	else
	{
		MonObj.RecordCount = (uint8_t)( count >> 1 );			// 1/2
		MonObj.CurrentData = (uint32_t)( MonObj.Data[ MonObj.RecordCount - 1 ] );	// 最新＝最終データ

		for ( pos=1; pos<=MonObj.RecordCount; pos++ ) {
			if ( MonObj.Data[MonObj.RecordCount - pos] == 0xF000 )	// data無し？
			{
			    break;
			}
		}
		MonObj.RecordCount = (uint8_t)(pos - 1);			// 記録レコード数 ０～１０
		MonObj.RecordBytes = (uint8_t)(MonObj.RecordCount * 2);	// 2バイト×レコード数

		count -= MonObj.RecordBytes;			// 無効データ(0xF000)のバイト数
//		count -= ( MonObj.RecordCount * 2 );	// 無効データ(0xF000)のバイト数

		memcpy( MonObj.Data, start+count, MonObj.RecordBytes );		// 実体コピー
	}

	MonObj.Battery = ExLevelBatt(Form->batt & 0b00001111); // 0-15を0-5へ
    MonObj.DataID = *(uint16_t *)(Form->data_number );     // 識別ID      キャストに変更 2020.01.21
	if ( Form->data_intt < 0x80 ){
	    MonObj.Interval = (Form->data_intt) ? (uint16_t)( ((uint16_t)Form->data_intt) * 60 ) : 30;	// 0は30秒
	}
	else{
	    MonObj.Interval = (uint16_t)(Form->data_intt - 0x80);	// 0x80以上は秒とする
	}
	if ( MonObj.Interval == 0 ){
	    MonObj.Interval = 30;	// 念には念
	}
	Printf("MonObj.Interval  %d (%d)\r\n",MonObj.Interval, Form->data_intt );

// sakaguchi 2021.01.21 ↓
	//ltemp =(uint32_t)( *(uint32_t *)Form->grobal_time -  *(uint16_t *)Form->keika_time );     // 最新レコードの時刻b     キャストに変更 2020.01.21
	if((MonObj.SpecNumber == SPEC_574) && ( ch >= 5 )){
		ltemp =(uint32_t)( *(uint32_t *)Form->grobal_time );		// RTR-574のCH5,CH6は無線通信した時刻をセット
	}else{
    ltemp =(uint32_t)( *(uint32_t *)Form->grobal_time -  *(uint16_t *)Form->keika_time );     // 最新レコードの時刻b     キャストに変更 2020.01.21
	}
// sakaguchi 2021.01.21 ↑

	Printf("  CurrentUTC %ld  %ld  %ld \r\n", ltemp, *(uint32_t *)(Form->grobal_time) , *(uint16_t *)(Form->keika_time) ); 
	MonObj.CurrentUTC = ltemp;
	if ( MonObj.RecordCount > 1 )			// 記録レコード数が2個以上の場合
	{
	    ltemp -= ( (uint32_t)MonObj.RecordCount - 1 ) * MonObj.Interval;
	}
	MonObj.RecordUTC = ltemp;

	if ( MonObj.RFErr ) {		// 無線通信エラー
		MonObj.RecordCount = 0;
		MonObj.Interval = 0;
		MonObj.Battery = 0xFF;
	}

	return (res);			// 処理したバイト数
}




/**
 * テスト用現在値データ作成・送信
 */
static void SendMonitorTest(void)
{
	int     temp;
	//uint32_t 	utime = RTC_GetLCTSec( 0 );
	uint32_t 	utime = RTC_GetGMTSec();
//	int		tm;
	char	name[sizeof(my_config.device.Name)+2];				// sakaguchi add 2020.08.27

	MonObj.TempType =  (uint8_t)my_config.device.TempUnits[0];	            // oC(0) or oF(1)
	sprintf(XMLTForm,"%s", my_config.device.TimeForm);		    // 日付フォーマット

//	XML.Plain( XML_FORM_CURRENT );				                // <file .. name=" まで
	XML.Plain( XML_FORM_CURRENT_TEST );				                // <file .. name=" まで
	XML.Plain( "%s", XML.FileName);

	memset(name, 0x00, sizeof(name));
	memcpy(name, my_config.device.Name, sizeof(my_config.device.Name));
	sprintf(XMLTemp,"%s", name);
	XML.Plain( "\" author=\"%.33s\">\n", XMLTemp);

	XML.OpenTag( "base" );

	sprintf(XMLTemp,"%08lX",fact_config.SerialNumber);
	XML.TagOnce( "serial", XMLTemp /*fact_config.SerialNumber*/); 	        // シリアル番号

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

		//sprintf(XMLTemp,"%s", my_config.device.Name ); 
		//XML.TagOnce( "name", XMLTemp ); 				          // 親機名称
		//XML.Plain( "<name>%s</name>\n", my_config.device.Name);		// 親機名称
	XML.Plain( "<name>%s</name>\n", name);	 					// 親機名称			// sakaguchi cg 2020.08.27


	temp = (int)( RTC.Diff / 60 );		        	        // 時差
	XML.TagOnce( "time_diff", "%d", temp );
	XML.TagOnce( "std_bias", "0" );			//
	temp = (int)( RTC.AdjustSec / 60 );		                // サマータイム中
	XML.TagOnce( "dst_bias", "%d", temp );
	//XML.Plain( "<time_zone>%s</time_zone>\n", my_config.device.TimeZone);
	
	memcpy(XMLTemp, my_config.device.TimeZone, sizeof(my_config.device.TimeZone));
	XML.TagOnce2( "time_zone", XMLTemp);	

	XML.OpenTag( "lan" );
		//XML.TagOnce( "output", "%u", ExternalOut(-1) );	// 出力ピンの状態
	XML.CloseTag();

	XML.CloseTag();		// <base>

// 2022.10.31 子機情報を削除 ↓
#if 0
	XML.OpenTag( "group" );
		XML.TagOnce( "num", "%d", 1 );
		XML.Plain("<name>%s</name>\n", "Test");

		XML.OpenTag( "remote" );

			XML.TagOnce( "serial", "5F829999");	// HEX8文字

			XML.TagOnce( "model", "RTR501B" );	// モデル名

			XML.TagOnce( "num", "%d", 1 );				// 子機番号
			XML.Plain("<name>%s</name>\n", "Test01");

			sprintf( XMLTemp, "repeater=\"%u\"", 0 );		// 属性の作成
			XML.TagOnceAttrib( "rssi", XMLTemp, "%u", 5 );	// 末端の中継機番号 2017-12-12 Ver3.00

			XML.OpenTag( "ch" );

				XML.TagOnce( "num", "%d", 1 );		// CH番号

				XML.TagOnce( "scale_expr", "" );	// スケール変換
				XML.TagOnce( "name", "" );			// CH名

				XML.OpenTagAttrib( "current", "state=\"0\"");

					XML.TagOnceAttrib( "value", "valid=\"true\"", "99.9" );
					XML.TagOnce( "unit", "%.64s", "C" );
					XML.TagOnce( "batt", "%d", 5 );

				XML.CloseTag();		// <current>

				//XML.OpenTag( "recode" );
				XML.OpenTag( "record" );					// sakaguchi 2021.05.17
					XML.TagOnce( "type", "%d", 13 );
					XML.TagOnce( "unix_time", "%ld", utime );
					XML.TagOnce( "data_id", "%d", 111 );
					XML.TagOnce( "interval", "%d", 10 );
					XML.TagOnce( "count", "%d", 10 );
					XML.TagOnce( "data", "%s", "6gTpBOoE6wTrBOsE6gTqBOoE6wQ=" );	

				XML.CloseTag();		// <recode>
			XML.CloseTag();		// <ch>


		XML.CloseTag();		// <remote>


	XML.CloseTag();		// <group>
#endif
// 2022.10.31 子機情報を削除 ↑

	XML.Plain( "</file>\n" );		// <file 現在値>

	XML.Output();					// バッファ吐き出し


}
