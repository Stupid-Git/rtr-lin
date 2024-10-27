/**
 * @file Suction.c
 *
 * @date    Created on: 2019/09/03
 * @author  tooru.hayashi
 * @note	2020.Jul.27 GitHub 0727ソース反映済み 
 */

#define _SUCTION_C_


#include "Version.h"
#include "flag_def.h"
#include "MyDefine.h"
#include "Globals.h"
#include "Config.h"
#include "General.h"
#include "Lang.h"
#include "Log.h"
#include "DateTime.h"
#include "Xml.h"
#include "Convert.h"
#include "Suction.h"
#include "Rfunc.h"

//#include "ftp_thread.h"
//#include "http_thread.h"


extern TX_THREAD ftp_thread;
//extern TX_THREAD http_thread;


void SendSuctionTest(void);
void MakeSuctionXML( int32_t Test );
static void MakeSuctionHTTP(int32_t Test);
static void MakeSuctionFTP(int32_Test);
static void SendPushXML( int Kisyu );

//未使用static void MakeSuctionEmail( int32_t );

static uint8_t	PushTemp[64];			// PUSH XML生成用

/**
 * 吸い上げデータ作成・送信
 * @param Test       ０：通常  １：テスト
 * @return
 */
int SendSuctionData( int32_t Test )
{

	UINT status;
	ULONG actual_events;
	
	// FTP or HTTP

	switch (my_config.suction.Route[0])
	{
		case 0:			// Email
			break;
		case 1:			// FTP
			MakeSuctionFTP(Test);

			XML.Create(0);
			MakeSuctionXML( Test );
			Printf(" XML File size = %ld\r\n", strlen(xml_buffer));
			//if(Test == 0){
			    status = tx_event_flags_get (&event_flags_ftp, 0xffffffff,TX_OR_CLEAR, &actual_events,TX_NO_WAIT);
			//}
			tx_thread_resume(&ftp_thread);
			//if(Test == 0){
			    status = tx_event_flags_get (&event_flags_ftp, FLG_FTP_SUCCESS | FLG_FTP_ERROR,TX_OR_CLEAR, &actual_events,TX_WAIT_FOREVER);
			//}

			break;
		case 3:			// HTTP ot HTTPS
			MakeSuctionHTTP(Test);
			XML.Create(0);
    		MakeSuctionXML( Test );
			Printf(" XML File size = %ld\r\n", strlen(xml_buffer));
//sakaguchi ↓
//			HttpFile = FILE_S;
//			if(Test == 0){
//			    status = tx_event_flags_get (&event_flags_http, 0xffffffff,TX_OR_CLEAR, &actual_events,TX_NO_WAIT);
//			}
//			G_HttpFile[FILE_S].sndflg = SND_ON;
//			//tx_thread_resume(&http_thread);
//			if(Test == 0){
//			    status = tx_event_flags_get (&event_flags_http, FLG_HTTP_SUCCESS | FLG_HTTP_ERROR,TX_OR_CLEAR, &actual_events,TX_WAIT_FOREVER);
//			}
			//if(Test == 0){
			    status = tx_event_flags_get (&event_flags_http, 0xffffffff,TX_OR_CLEAR, &actual_events,TX_NO_WAIT);
			//}
			G_HttpFile[FILE_S].sndflg = SND_ON;									// [FILE]記録データ送信:ON
			tx_event_flags_set (&event_flags_http, FLG_HTTP_READY, TX_OR);		// HTTPスレッド動作許可:ON
			//if(Test == 0){
			    //status = tx_event_flags_get (&event_flags_http, FLG_HTTP_SUCCESS | FLG_HTTP_ERROR,TX_OR_CLEAR | FLG_HTTP_CANCEL, &actual_events,TX_WAIT_FOREVER);
				status = tx_event_flags_get (&event_flags_http, (ULONG)(FLG_HTTP_SUCCESS | FLG_HTTP_ERROR | FLG_HTTP_CANCEL),TX_OR_CLEAR, &actual_events,TX_WAIT_FOREVER);
			//}
//sakaguchi ↑
			break;	
		default:
			break;
	}


	Printf(">>>>>> Suction Send End  %04X \r\n", actual_events);
    Printf(" status =  %d \n" ,status);     //仮 2020.01.21

//	if(actual_events & FLG_SEND_ERROR){						// sakaguchi 2021.03.10
	if((actual_events & FLG_SEND_ERROR) || (actual_events & FLG_HTTP_CANCEL)){
		switch(my_config.suction.Route[0]){
			case 3:
				//PutLog( LOG_LAN, "$D:$E HTTP (%08lX)", Download.Attach.serial_no);
				PutLog( LOG_LAN, "$D:$E (%08lX) via HTTP(S)", Download.Attach.serial_no);
				break;
			case 1:
				//PutLog( LOG_LAN, "$D:$E FTP (%08lX)", Download.Attach.serial_no);
				PutLog( LOG_LAN, "$D:$E (%08lX) via FTP", Download.Attach.serial_no);
				break;
			default:
				PutLog( LOG_LAN, "$D:$E (%08lX)", Download.Attach.serial_no);
				break;
		}
		DebugLog(LOG_DBG, "$D:$E  (%08lX) %d", Download.Attach.serial_no, status);
	    return (-1);
	}
	else{
		switch(my_config.suction.Route[0]){
			case 3:
				//PutLog( LOG_LAN, "$D:$S HTTP (%08lX)", Download.Attach.serial_no);
				PutLog( LOG_LAN, "$D:$S (%08lX) via HTTP(S)", Download.Attach.serial_no);
				break;
			case 1:
				//PutLog( LOG_LAN, "$D:$S FTP (%08lX)", Download.Attach.serial_no);
				PutLog( LOG_LAN, "$D:$S (%08lX) via FTP", Download.Attach.serial_no);
				break;
			default:
				PutLog( LOG_LAN, "$D:$S (%08lX)", Download.Attach.serial_no);
				break;
		}
	    return (0);
	}

}

/**
 * 吸い上げデータ作成・送信
 */
static void MakeSuctionHTTP(int32_t Test)
{
	char	*mem;
	char temp[300];

	Read_StrParam( my_config.suction.Fname, temp ,  sizeof(my_config.suction.Fname));

	Printf(" %s (%d)\r\n", temp, strlen(temp) );
	if(strlen(temp) == 0){
		sprintf(temp, "*B-*Y*m*d-*H*i*s.trz");
	}

	if(Test){
		memset(XMLName,0x00,sizeof(XMLName));
		GetFormatString( 0, XMLName, temp, 128, "Test Unit" );	// 書式変換
	}
	else{
		get_regist_relay_inf(Download.Attach.group_no, Download.Attach.unit_no, rpt_sequence);

		memset(XMLName,0x00,sizeof(XMLName));
		GetFormatString( 0, XMLName, temp, 128, ru_registry.rtr501.ble.name );	// 書式変換

		// PUSH用 601
		if ( SPEC_601 == GetSpecNumber( Download.Attach.serial_no, Download.Attach.kisyu_code ) )
		{
			mem = strrchr( XMLName, '.' );		// 最後のピリオド
			if ( mem != 0 && Download.Attach.extension[0] != 0 ){
				StrCopyN( mem+1, Download.Attach.extension, 3 );	// 拡張子変換
			}
			else{
				Printf("## Push extension error\r\n");
			}
		}
		// Push 602
		else if ( SPEC_602 == GetSpecNumber( Download.Attach.serial_no, Download.Attach.kisyu_code ) )	
		{
			mem = strrchr( XMLName, '.' );		// 最後のピリオド
			if ( mem != 0 && Download.Attach.extension[0] != 0 ){
				StrCopyN( mem+1, Download.Attach.extension, 3 );	// 拡張子変換
			}
			else{
				Printf("## Push extension error\r\n");
			}
		}
	}

	Printf("File Name  %s \r\n", XMLName );						// これでいいの？？ dir名もXMLに書かれる

	// PUSH用 ????


}

/**
 * 吸い上げデータ作成・送信
 */
static void MakeSuctionFTP(int32_t Test)
{
// ここは修正！！
	char	*mem;
	uint32_t size = 0;
	uint32_t fsize = 0;
	int i;
	char 	temp[300];

	//sprintf(Download.Attach.unit_name, "Unit01" );  // debug

	Read_StrParam( my_config.ftp.Server , FtpHeader.SRV,  sizeof(my_config.ftp.Server));
	Read_StrParam( my_config.ftp.UserID , FtpHeader.ID ,  sizeof(my_config.ftp.UserID));
	Read_StrParam( my_config.ftp.UserPW , FtpHeader.PW ,  sizeof(my_config.ftp.UserPW));
	Read_StrParam( my_config.suction.Dir , temp ,  sizeof(my_config.suction.Dir));
	
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
	Read_StrParam( my_config.suction.Fname , temp ,  sizeof(my_config.suction.Fname));

	Printf("  %s \r\n", temp );

	//fsize = GetFormatString( 0, XMLName, temp, 64, 0 );	// 書式変換
	if(Test){
				GetFormatString( 0, XMLName, temp, 128, "TestUnit" );	// 書式変換
		fsize = GetFormatString( 0, FtpHeader.PATH+size, temp, 128, "TestUnit" );	// 書式変換

	}
	else{
				GetFormatString( 0, XMLName, temp, 128, ru_registry.rtr501.ble.name );	// 書式変換
		fsize = GetFormatString( 0, FtpHeader.PATH+size, temp, 128, ru_registry.rtr501.ble.name );	// 書式変換
		// PUSH用
		if ( SPEC_601 == GetSpecNumber( Download.Attach.serial_no, Download.Attach.kisyu_code ) )
		{
			mem = strrchr( XMLName, '.' );		// 最後のピリオド
			if ( mem != 0 && Download.Attach.extension[0] != 0 )
				StrCopyN( mem+1, Download.Attach.extension, 3 );	// 拡張子変換
			mem = strrchr( FtpHeader.PATH, '.' );		// 最後のピリオド
			if ( mem != 0 && Download.Attach.extension[0] != 0 )
				StrCopyN( mem+1, Download.Attach.extension, 3 );	// 拡張子変換	
		}
		// Push602
		else if ( SPEC_602 == GetSpecNumber( Download.Attach.serial_no, Download.Attach.kisyu_code ) )
		{
			mem = strrchr( XMLName, '.' );		// 最後のピリオド
			if ( mem != 0 && Download.Attach.extension[0] != 0 )
				StrCopyN( mem+1, Download.Attach.extension, 3 );	// 拡張子変換
			mem = strrchr( FtpHeader.PATH, '.' );		// 最後のピリオド
			if ( mem != 0 && Download.Attach.extension[0] != 0 )
				StrCopyN( mem+1, Download.Attach.extension, 3 );	// 拡張子変換	
		}
	}

	StrCopyN( FtpHeader.FNAME, FtpHeader.PATH+0, (int)fsize );
	Printf("  %s \r\n", XMLName );						// これでいいの？？ dir名もXMLに書かれる
	Printf("  %s \r\n", FtpHeader.FNAME );
	Printf("  %s \r\n", FtpHeader.PATH );


	// PUSH用 ????


	//FTP.DataAdrs = 0;
	//FTP.DataSize = 0;



}


#if 0
/**
 * 吸い上げデータ作成・送信（E-Mail)
 * @param Test
 * @note    未使用
 */
static void MakeSuctionEmail( int32_t Test )
{
//	UCHAR	auth;
	char	*mem;
	char 	temp[300];


		// ????? LimitCopy( EmHeader.FRM, ""	);					// From(Description)

		Read_StrParam( my_config.smtp.From , EmHeader.From,  sizeof(my_config.smtp.From));		// From(Address)
		Read_StrParam( my_config.smtp.Sender , EmHeader.Sender,  sizeof(my_config.smtp.Sender));	// From(Description)


		EmHeader.TO[0] = 0;								// To(Address)
		//EmHeader.TOA1[0] = EmHeader.TOA2[0] = 0;
		//EmHeader.TOA3[0] = EmHeader.TOA4[0] = 0;		// To初期化

		Read_StrParam( my_config.suction.To1 , EmHeader.TO,  sizeof(my_config.suction.To1));	// To(Address)

		Read_StrParam( my_config.suction.Attach , EmHeader.ATA,  sizeof(my_config.suction.Attach));	// 添付ファイル名
		
		sprintf(temp,"*Y*m*d-*H*i*s.trz");		// debug
		
		GetFormatString( 0, temp, EmHeader.ATA, 64, Download.Attach.unit_name );	// 書式変換
		StrCopyN( EmHeader.ATA, temp, 64 );
		StrCopyN( XMLName, temp, 64 );

		Read_StrParam( my_config.suction.Subject , EmHeader.Sbj,  sizeof(my_config.suction.Subject));	// 添付ファイル名


		if ( !Test ){
		    mem = StrCopyN( EmHeader.BDY, EmHeader.ATA, 96-3 );	// mem = StrCopyN( EmHeader.BDY, "Recorded Data", 96-3 );	// Body
		}
		else{
		    mem = StrCopyN( EmHeader.BDY, pLANG(RECORD_OK), 96-3 );	// "Recorded Data Test OK"
		}

		sprintf( mem-1, CRLF CRLF "%s" CRLF CRLF /*".\0"*/, LANG(END_EMAIL) );		// "End of mail" 最後に改行とピリオドとnullを付加

		if(strlen(EmHeader.BDY) < 128)
			Printf("Email Body %s \r\n", EmHeader.BDY);

}


#endif



/**
 * @brief   吸い上げデータ作成・送信
 * @param Test
 */
void MakeSuctionXML( int32_t Test )
{
	int16_t		ch_num, ch, ch576;
	int count;
	int32_t		itemp, size;
	uint32_t	utemp, kisyu;
	uint16_t	spec_number;
	uint8_t		sc, mmap; /*, lf;*/
	bool    upflg, loflg;
	char		*data_adrs;
	char		multi[8+4];			// 積算出力用
	//int16_t		db_size, db_x;
	//char		*db_adr;
	char	name[sizeof(my_config.device.Name)+2];				// sakaguchi add 2020.08.27

	if ( Test ) {				// テスト送信？
		SendSuctionTest();
	}
	else
	{
	

		kisyu = GetSpecNumber( Download.Attach.serial_no, Download.Attach.kisyu_code );
		if (( kisyu == SPEC_601 ) || ( kisyu == SPEC_602 ))
		{
			Printf("Push Down Load\r\n");
			SendPushXML( kisyu );
			return;
		}

		XML.Plain( XML_FORM_RECORD );			// <file .. name=" まで
		//XML.Entity( XML.FileName );				// ファイル名（エンコード付き）
		//XML.Plain( "\">\n" );
		XML.Plain( "%s", XML.FileName);

		memset(name, 0x00, sizeof(name));
		memcpy(name, my_config.device.Name, sizeof(my_config.device.Name));
		sprintf(XMLTemp,"%s", name);
		XML.Plain( "\" author=\"%s\">\n", XMLTemp );

		XML.OpenTag( "base" );
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

		//XML.TagOnce( "name", my_config.device.Name );
//		XML.Plain( "<name>%s</name>\n", my_config.device.Name);	// 親機名称
		XML.Plain( "<name>%s</name>\n", name);	 				// 親機名称		// sakaguchi cg 2020.08.27

		XML.CloseTag();				// <base>


		ch_num = kisyu & 0x00FF;	// CH数
		count = Download.Attach.data_byte[0] >> 1;				// データカウント
		Printf("Down Load count(%d) ch(%d) \r\n", count,ch_num);
//		mmap = ( kisyu != SPEC_505PS && ch_num != 1 ) ? 1 : 0;	// 2CHのデータ並び
		mmap = ( ((kisyu != SPEC_505PS)&&(kisyu != SPEC_505BPS)) && ch_num != 1 ) ? 1 : 0;	// 2CHのデータ並び

		if ( kisyu == SPEC_576 ){
		    mmap = 0;						// 追加
		}
		if ( mmap ){
		    count >>= 1;					// 2CH機はさらに2分の1にする
		}

		sc = Download.Attach.sett_flags & 0x01;		// 変換する/しないフラグ


		for ( ch=0; ch<ch_num; ch++ ) {

			XML.OpenTag( "ch" );

			XML.TagOnce( "serial", "%08lX",Download.Attach.serial_no );
//Download.Attach.kisyu_code = 0xa6;
			spec_number = GetSpecNumber( Download.Attach.serial_no, Download.Attach.kisyu_code );
			XML.TagOnce( "model", "%.32s", GetModelName( Download.Attach.serial_no, spec_number ) );	// モデル名
Printf("Spec code %04X / %02X\r\n", spec_number, sc);

//			XML.TagOnce( "name", "%.26s", Download.Attach.unit_name );				// 子機名	この子機名(Max8byte)は500B対応で未使用にする
//----↓↓↓↓↓------------------------------------------------------2019.12.11追加
			get_regist_relay_inf(Download.Attach.group_no, Download.Attach.unit_no, rpt_sequence);
			//XML.TagOnce( "name", "%.26s", ru_registry.rtr501.ble.name );			// 子機名(登録パラメータファイルからRead)
			XML.Plain("<name>%.26s</name>\n", ru_registry.rtr501.ble.name);
//----↑↑↑↑↑------------------------------------------------------2019.12.11追加


			XML.TagOnce( "num", "%u", ch+1 );										// チャンネル番号

			itemp = (int)( RTC.Diff / 60 );											// 時差
			XML.TagOnce( "time_diff", "%d", itemp );
			XML.TagOnce( "std_bias", "0" );											//
			itemp = (int)( RTC.AdjustSec / 60 );									// サマータイム中
			XML.TagOnce( "dst_bias", "%d", itemp );
			//XML.Plain( "<time_zone>%s</time_zone>\n", my_config.device.TimeZone);
			memcpy(XMLTemp, my_config.device.TimeZone, sizeof(my_config.device.TimeZone));
			XML.TagOnce2( "time_zone", XMLTemp);	

//			if ( kisyu == SPEC_505PS && ch == 1 ) {		// パルスの2ch目はこちら
			if ( (kisyu == SPEC_505PS || kisyu == SPEC_505BPS) && ch == 1 ) {		// パルスの2ch目はこちら

				itemp = GetConvertMethod( Download.Attach.zoku[0], 0, 0x04 );		// 時刻付き総パルス
				XML.TagOnceEmpty( count, "type", "%u", MeTable[itemp].DataCode );	// CH属性値
// sakaguchi 2021.01.18 ↓
				//XML.TagOnce( "unix_time", "%lu", Download.Attach.GMT );				// 現在値の時刻
				//XML.TagOnceEmpty( 0, "data_id", "" );								// 識別番号
				//XML.TagOnceEmpty( 0, "interval", "" );								// 記録インターバル
				utemp = Download.Attach.GMT - Download.Attach.wait_time;
				XML.TagOnce( "unix_time", "%lu", utemp );							// 現在値の時刻
				XML.TagOnceEmpty( count, "data_id", "%u", Download.Attach.end_no );	// 識別番号
				XML.TagOnceEmpty( count, "interval", "%u", Download.Attach.intt );	// 記録インターバル
// sakaguchi 2021.01.18 ↑
				XML.TagOnce( "count", "%u", count ? 1 : 0 );						// 記録データ数

				XML.OpenTag( "data" );

				if ( count ) {
					memset( &multi[0], 0, sizeof(multi) );		// クリア
					//memcpy( &multi[0], &Download.Attach.GMT, sizeof(Download.Attach.GMT) );	// 吸い上げ時のGMT
					memcpy( &multi[0], &utemp, sizeof(utemp) );									// 吸い上げ時のGMT		// sakaguchi 2021.01.18
					memcpy( &multi[8], &Download.Attach.total_pulse[0], 4 );	// 素データ１つ
					XML.Base64( 1, multi, 12 /*sizeof(multi)*/ );
					XML.Plain( "\n" );
				}

				XML.CloseTag();		// data

				XML.TagOnceEmpty( 0, "upper_limit", "" );			// 上限値
				XML.TagOnceEmpty( 0, "lower_limit", "" );			// 下限値

				if ( sc ){
					GetScaleStr( Download.Attach.scale_conv, 1 );	// CH2のスケール変換文字列

					FormatScaleStr( FormatScale, ScaleString );
					//XML.TagOnceEmpty( sc, "scale_expr", "%.64s", FormatScale );	// スケール変換文字列
					//XML.Plain("<scale_expr>%.70s</scale_expr>\n", FormatScale);
					XML.TagOnce2("scale_expr", "%.70s", FormatScale);				// エスケープ処理追加	// sakaguchi 2021.04.16
				}
				else{
					XML.Plain("<scale_expr></scale_expr>\n");
				}	
				XML.TagOnceEmpty( 0, "record_prop", "" );
			}
		
			else if ( ch < 4 ) {			// 積算値以外？

				if ( kisyu == SPEC_576 )
				{
					ch576 = ch;				// オリジナル保存
					if ( ch != 0 ) {
						mmap = 1;
						ch++;				// 0, 2, 3 とする
					}
				}

				itemp = GetConvertMethod( Download.Attach.zoku[ch%4], 0, 0 );			// 積算以外,通常パルス数
				Printf("GetConvertMethod itemp %ld\r\n", itemp);
// 2023.07.31 ↓
				if(kisyu == SPEC_505BLX){
					XML.TagOnceEmpty( count, "type", "%u", 0x009A );		// KGCモードの記録属性
				}else{
					XML.TagOnceEmpty( count, "type", "%u", MeTable[itemp].DataCode );		// CH属性値
				}
// 2023.07.31 ↑
				utemp = Download.Attach.GMT - Download.Attach.wait_time;
				utemp -= (uint32_t)( count - 1 ) * Download.Attach.intt;					// 最古データの時刻計算

				XML.TagOnce( "unix_time", "%lu", utemp );								// 現在値の時刻
				XML.TagOnceEmpty( count, "data_id", "%u", Download.Attach.end_no );		// 識別番号
				XML.TagOnceEmpty( count, "interval", "%u", Download.Attach.intt );		// 記録インターバル
				XML.TagOnce( "count", "%u", count );									// 記録データ数

				XML.OpenTag( "data" );

				data_adrs = Download.Attach.data_poi[(ch%4)/2];							// データ開始アドレス
				if ( ch % 2 ){
				    data_adrs += 2;
				}

				size = count * 2;														// 全データ数 2倍

				while ( size > 0 ) {
					itemp = ( size > 240 ) ? 240 : size;								// 1回の転送は4の倍数である事
//					XML.Base64( ch_num == 1, data_adrs, itemp );
					XML.Base64( mmap == 0, data_adrs, itemp );
					XML.Plain( "\n" );
					size -= itemp;
//					data_adrs += ( ch_num == 1 ) ? itemp : itemp*2;
					data_adrs += ( mmap == 0 ) ? itemp : itemp*2;
				}

				XML.CloseTag();		// data

				upflg = loflg = 0;
				switch ( ch ) {
					case 0:
						upflg |= Download.Attach.ud_flags.up_1;
						loflg |= Download.Attach.ud_flags.lo_1;
						break;
					case 1:
						upflg |= Download.Attach.ud_flags.up_2;
						loflg |= Download.Attach.ud_flags.lo_2;
						break;
					case 2:
						upflg |= Download.Attach.ud_flags.up_3;
						loflg |= Download.Attach.ud_flags.lo_3;
						break;
					case 3:
						upflg |= Download.Attach.ud_flags.up_4;
						loflg |= Download.Attach.ud_flags.lo_4;
						break;
				}

				utemp = Download.Attach.upper_limit[ ch ];					// 上限値
				XML.TagOnceEmpty( upflg, "upper_limit", "%u", utemp );		// 上限値

				utemp = Download.Attach.lower_limit[ ch ];					// 上限値
				XML.TagOnceEmpty( loflg, "lower_limit", "%u", utemp );		// 下限値

				if ( sc ){
					if(spec_number == SPEC_505BLX){
						if((Download.Attach.scale_conv[0] != '\n')||(Download.Attach.scale_conv[1] != '\n')||(Download.Attach.scale_conv[2] != '\n')||(Download.Attach.scale_conv[3] != '\n')){			// 変化式有りフラグONで有効な値入っていれば登録値を採用
						}
						else{
							strcpy( Download.Attach.scale_conv, SCALE_505Lx );		// L5 L10 [dB]
						}
					}
				    GetScaleStr( Download.Attach.scale_conv, ch );			// CH毎のスケール変換文字列

					FormatScaleStr( FormatScale, ScaleString );
					//XML.Plain("<scale_expr>%.70s</scale_expr>\n", FormatScale);
					XML.TagOnce2("scale_expr", "%.70s", FormatScale);		// エスケープ処理追加	// sakaguchi 2021.04.16
				}
				else{
					XML.Plain("<scale_expr></scale_expr>\n");
				}
				XML.OpenTag( "record_prop" );
				// ヘッダ情報
				size = (int)Download.Attach.header_size[(ch%4)/2];
				data_adrs = Download.Attach.data_poi[(ch%4)/2];				// データ開始アドレス
				data_adrs -= size;											// ヘッダの先頭アドレス
				while ( size > 0 ) {
					itemp = ( size > 256 ) ? 256 : size;					// 1回の転送は4の倍数である事
					XML.Base64( true, data_adrs, itemp );
					XML.Plain( "\n" );
					size -= itemp;
					data_adrs += itemp;
				}
				XML.CloseTag();		// record_prop

				if ( kisyu == SPEC_576 ){
				    ch = ch576;						// オリジナルへ戻す
				}


			}
			else // 積算値はこちら
			{

				itemp = GetConvertMethod( Download.Attach.zoku[ch%4], 0, 1 );			// 積算
// 2023.07.31 ↓
				if(kisyu == SPEC_505BLX){
					XML.TagOnceEmpty( count, "type", "%u", 0x009A );		// KGCモードの記録属性
				}else{
					XML.TagOnceEmpty( count, "type", "%u", MeTable[itemp].DataCode );		// CH属性値
				}
// 2023.07.31 ↑
				XML.TagOnce( "unix_time", "%lu", Download.Attach.GMT );					// 現在値の時刻
				XML.TagOnceEmpty( 0, "data_id", "" );									// 識別番号
				XML.TagOnceEmpty( 0, "interval", "" );									// 記録インターバル
				XML.TagOnce( "count", "%u", count ? 1 : 0 );							// 記録データ数

				XML.OpenTag( "data" );

				if ( count ) {
					memset( &multi[0], 0, sizeof(multi) );										// クリア
					memcpy( &multi[0], &Download.Attach.GMT, sizeof(Download.Attach.GMT) );		// 吸い上げ時のGMT
					memcpy( &multi[8], &Download.Attach.multiply[ch % 2], 2 );					// 素データ１つ
					XML.Base64( 1, multi, 10 /*sizeof(multi)*/ );
					XML.Plain( "\n" );
				}

				XML.CloseTag();		// data

				utemp = Download.Attach.mp_limit[ ch % 2 ];						// 上限値

				//if ( ch == 4 && Download.Attach.ud_flags.up_1 ){
				if ( ch == 4 && (Download.Attach.mp_flags & 0x01) ){				// 積算上下限値フラグ // sakaguchi 2021.01.21
				    upflg = 1;
				}
				//else if ( ch == 5 && Download.Attach.ud_flags.up_2 ){
				else if ( ch == 5 && ((Download.Attach.mp_flags >> 1) & 0x01) ){	// 積算上下限値フラグ // sakaguchi 2021.01.21
				    upflg = 1;
				}
				else{
				    upflg = 0;
				}

				XML.TagOnceEmpty( upflg, "upper_limit", "%u", utemp );			// 上限値
				XML.TagOnceEmpty( 0, "lower_limit", "" );						// 下限値

				// 今は出来ない
				XML.TagOnceEmpty( 0, "scale_expr", "" );						// スケール変換文字列
				XML.TagOnceEmpty( 0, "record_prop", "" );
			}

			XML.CloseTag();		// ch tag

		}			// end of for

		XML.CloseAll();		// 念には念

		XML.Plain( "</file>\n" );

		XML.Output();					// バッファ吐き出し


		int len = (int)strlen(xml_buffer);
		Printf("====> Download XML File size = %ld\r\n\r\n", strlen(xml_buffer));
		for(int i=0;i<len;i++){
			Printf("%c",xml_buffer[i]);
		}

		Printf("\r\n");

	}		// テストではない

}

//========================================================
//　吸い上げデータ作成・送信(PUSH用）
//========================================================
void SendPushXML( int Kisyu )
{
	int		itemp;
	uint16_t utemp, size;
	uint8_t	*adrs;
	uint8_t temp[32];
	char	name[sizeof(my_config.device.Name)+2];				// sakaguchi add 2020.08.27

	XML.Plain( XML_FORM_PUSH );					// <file .. name=" まで
	XML.Plain( "%s", XML.FileName);

	memset(name, 0x00, sizeof(name));
	memcpy(name, my_config.device.Name, sizeof(my_config.device.Name));
	sprintf(XMLTemp,"%s", name);
	XML.Plain( "\" author=\"%.33s\">\n", XMLTemp );

	XML.OpenTag( "base" );
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

//		XML.Plain( "<name>%s</name>\n", my_config.device.Name);	// 親機名称
		XML.Plain( "<name>%.34s</name>\n", name);	 				// 親機名称		// sakaguchi cg 2020.08.27

	XML.CloseTag();				// <base>


	XML.OpenTag( "record" );

		XML.TagOnce( "serial", "%08lX",Download.Attach.serial_no );
		XML.TagOnce( "model", "%.32s", GetModelName( Download.Attach.serial_no, Kisyu ) );	// モデル名
		XML.TagOnce( "name", "%.8s", Download.Attach.unit_name );	// 子機名

		itemp = (int)( Download.Attach.time_diff / 60L );		// 時差(秒から分へ）
		XML.TagOnce( "time_diff", "%d", itemp );
		
		XML.TagOnce( "data_id", "%u", Download.Attach.end_no );	// 最終データ番号

		///// ヘッダ情報
		size = Download.Attach.header_size[0];
		adrs = Download.Attach.data_poi[0];			// データ開始アドレス
		adrs -= size;			

		Printf("Push Header size %ld\r\n", size);
		XML.OpenTag( "header" );
		while ( size > 0 ) {
			itemp = ( size > 240 ) ? 240 : size;	// 1回の転送は4の倍数である事
			XML.Base64( 1, adrs, itemp );
			XML.Plain( "\n" );
			size -= itemp;
			adrs += itemp;
		}
		XML.CloseTag();		// header
	
		size = Download.Attach.data_byte[0];			// 全データ数
		adrs = Download.Attach.data_poi[0];				// データの先頭

		Printf("Push Data size %ld %04X\r\n", size,adrs);

		XML.TagOnce( "count", "%u", size / 16 );		// 16バイトの個数

		XML.OpenTag( "data" );
		while ( size > 0 )
		{
			itemp = ( size > 240 ) ? 240 : size;		// 1回の転送は4の倍数である事
			XML.Base64( 1, adrs, itemp );
			XML.Plain( "\n" );
			size -= itemp;
			adrs += itemp;
		}
		XML.CloseTag();		// data

		///// 品目テーブル以下のサイズ
		size = RF_buff.rf_res.data_size - Download.Attach.data_byte[0] - Download.Attach.header_size[0];
		size /= 16;			// 16バイトの個数

		Printf("Push item list size %ld\r\n", size);

		XML.OpenTag( "item_list" );

		while ( size && *adrs == 'M' )
		{
			XML.OpenTag( "item" );

			itemp = AtoI( adrs + 2, 2 );			// 品目番号
			XML.TagOnce( "num", "%u", itemp );

			StrCopyN( temp, adrs + 4, 12 );		// 最大12文字としておく
			//Sjis2UTF8( PushTemp, temp, 12, 0 );
			Sjis2UTF8( PushTemp, temp, 18, 0 );		// UTF-8変換後の最大バイト数（全角6文字×3byte）※UTF-8 4byte文字はSJISには存在しない //sakaguchi 2020.11.20 
			//XML.Plain( "<name>%s</name>\n", PushTemp);	// 品目名	
			XML.TagOnce2("name", "%s", PushTemp);	// エスケープ処理追加	// sakaguchi 2021.04.16
			XML.CloseTag();		// item

			adrs += 16;
			size--;
		}

		XML.CloseTag();			// item_list

		XML.OpenTag( "user_list" );

		while ( size && *adrs == 'U' )
		{
			XML.OpenTag( "user" );

			itemp = AtoI( adrs + 2, 2 );			// 作業者番号
			XML.TagOnce( "num", "%u", itemp );

			StrCopyN( temp, adrs + 4, 12 );		// 最大12文字としておく
			//Sjis2UTF8( PushTemp, temp, 12, 0 );
			Sjis2UTF8( PushTemp, temp, 18, 0 );		// UTF-8変換後の最大バイト数（全角6文字×3byte）※UTF-8 4byte文字はSJISには存在しない //sakaguchi 2020.11.20 
			//XML.Plain( "<name>%s</name>\n", PushTemp);
			XML.TagOnce2("name", "%s", PushTemp);	// エスケープ処理追加	// sakaguchi 2021.04.16
			XML.CloseTag();		// user

			adrs += 16;
			size--;
		}

		XML.CloseTag();		// user_list

		XML.OpenTag( "group_list" );

		while ( size && *adrs == 'W' )
		{
			XML.OpenTag( "group" );

			itemp = AtoI( adrs + 2, 2 );			// 作業者番号
			XML.TagOnce( "num", "%u", itemp );

			StrCopyN( temp, adrs + 4, 12 );		// 最大12文字としておく
			//Sjis2UTF8( PushTemp, temp, 12, 0 );
			Sjis2UTF8( PushTemp, temp, 18, 0 );		// UTF-8変換後の最大バイト数（全角6文字×3byte）※UTF-8 4byte文字はSJISには存在しない //sakaguchi 2020.11.20 
			//XML.Plain( "<name>%s</name>\n", PushTemp);
			XML.TagOnce2("name", "%s", PushTemp);	// エスケープ処理追加	// sakaguchi 2021.04.16
			XML.CloseTag();		// group

			adrs += 16;
			size--;
		}

		XML.CloseTag();		// user_list


	XML.CloseTag();		// record

	XML.CloseAll();		// 念には念



	XML.Plain( "</file>\n" );

	XML.Output();					// バッファ吐き出し


	int len = (int)strlen(xml_buffer);
	Printf("====> Download XML File size = %ld\r\n\r\n", strlen(xml_buffer));
	for(int i=0;i<len;i++){
		Printf("%c",xml_buffer[i]);
	}

	Printf("\r\n");

}

/*
//========================================================
//　吸い上げデータ作成・送信(PUSH用）
//========================================================
void SendPushXML( int Kisyu )
{
	int		itemp;
	UWORD	utemp, size;
	UBYTE	*adrs;

//
//	adrs = strrchr( XML.FileName, '.' );	// 最後のピリオド
//	if ( adrs != 0 && Download.Attach.extension[0] != 0 )
//		StrCopyN( adrs+1, Download.Attach.extension, 3 );	// 拡張子変換
//

	XML.Plain( XML_FORM_PUSH );				// <file .. name=" まで
	XML.Plain( "%s", XML.FileName);
	sprintf(XMLTemp,"%s", my_config.device.Name );     
	XML.Plain( "\" author=\"%.33s\">\n", XMLTemp );
	//XML.Plain( "\" author=\"%s Ver.%s", BaseUnitName, VERSION_FW );	// author

	XML.OpenTag( "base" );

	sprintf(XMLTemp, "%08lX", fact_config.SerialNumber);
	XML.TagOnce( "serial", XMLTemp );
	XML.TagOnce( "model", "RTR-500BW");
	XML.Plain( "<name>%s</name>\n", my_config.device.Name);	 // 親機名称

	XML.CloseTag();		// <base>

	XML.OpenTag( "record" );

	XML.TagOnce( "serial", "%08lX",Download.Attach.serial_no );

	XML.TagOnce( "model", "%.32s", GetModelName( Download.Attach.serial_no, Kisyu ) );	// モデル名

	XML.TagOnce( "name", "%.8s", Download.Attach.unit_name );	// 子機名


	XML.CloseTag();		// record

	XML.CloseAll();		// 念には念

	XML.Plain( "</file>\n" );

	XML.Output();					// バッファ吐き出し


/////////////////////////

	XML.OpenTag( "record" );


	XML.TagOnce( "serial", "%08lX",Download.Attach.serial_no );

	XML.TagOnce( "model", "%.32s", GetModelName( Download.Attach.serial_no, Kisyu ) );		// モデル名

	XML.TagOnce( "name", "%.8s", Download.Attach.unit_name );	// 子機名

	itemp = (int)( Download.Attach.time_diff / 60L );		// 時差(秒から分へ）
	XML.TagOnce( "time_diff", "%d", itemp );

	XML.TagOnce( "data_id", "%u", Download.Attach.end_no );	// 最終データ番号

	///// ヘッダ情報
	size = Download.Attach.header_size[0];
	adrs = Download.Attach.data_poi[0];			// データ開始アドレス
	adrs -= size;								// ヘッダの先頭アドレス

	XML.OpenTag( "header" );
	while ( size > 0 ) {
		itemp = ( size > 240 ) ? 240 : size;	// 1回の転送は4の倍数である事
		XML.Base64( TRUE, adrs, itemp );
		XML.Plain( "\n" );
		size -= itemp;
		adrs += itemp;
	}
	XML.CloseTag();		// header

	size = Download.Attach.data_byte[0];			// 全データ数
	adrs = Download.Attach.data_poi[0];				// データの先頭

	XML.TagOnce( "count", "%u", size / 16 );		// 16バイトの個数
	
	XML.OpenTag( "data" );
	while ( size > 0 )
	{
		itemp = ( size > 240 ) ? 240 : size;		// 1回の転送は4の倍数である事
		XML.Base64( TRUE, adrs, itemp );
		XML.Plain( "\n" );
		size -= itemp;
		adrs += itemp;
	}
	XML.CloseTag();		// data

	///// 品目テーブル以下のサイズ
	size = RF_buff.rf_res.data_size - Download.Attach.data_byte[0] - Download.Attach.header_size[0];
	size /= 16;			// 16バイトの個数

	XML.OpenTag( "item_list" );

	while ( size && *adrs == 'M' )
	{
		XML.OpenTag( "item" );

		itemp = AtoI( adrs + 2, 2 );			// 品目番号
		XML.TagOnce( "num", "%u", itemp );

		StrCopyN( PushTemp, adrs + 4, 12 );		// 最大12文字としておく
		XML.TagOnce( "name", PushTemp );		// 品目名

		XML.CloseTag();		// item

		adrs += 16;
		size--;
	}

	XML.CloseTag();			// item_list

	XML.OpenTag( "user_list" );

	while ( size && *adrs == 'U' )
	{
		XML.OpenTag( "user" );

		itemp = AtoI( adrs + 2, 2 );			// 作業者番号
		XML.TagOnce( "num", "%u", itemp );

		StrCopyN( PushTemp, adrs + 4, 12 );		// 最大12文字としておく
		XML.TagOnce( "name", PushTemp );		// 作業者名

		XML.CloseTag();		// user

		adrs += 16;
		size--;
	}

	XML.CloseTag();		// user_list

	XML.OpenTag( "group_list" );

	while ( size && *adrs == 'W' )
	{
		XML.OpenTag( "group" );

		itemp = AtoI( adrs + 2, 2 );			// 作業者番号
		XML.TagOnce( "num", "%u", itemp );

		StrCopyN( PushTemp, adrs + 4, 12 );		// 最大12文字としておく
		XML.TagOnce( "name", PushTemp );		// 作業者名

		XML.CloseTag();		// group

		adrs += 16;
		size--;
	}

	XML.CloseTag();		// user_list

	XML.CloseTag();		// record

	XML.CloseAll();		// 念には念

	XML.Plain( "</file>\n" );

	XML.Output();					// バッファ吐き出し

}
*/






/**
 * テスト用吸い上げデータ作成・送信
 */
void SendSuctionTest(void)
{
	char	name[sizeof(my_config.device.Name)+2];				// sakaguchi add 2020.08.27

	XML.Plain( XML_FORM_RECORD_TEST );		// <file .. name=" まで
	XML.Plain( "%s", XML.FileName);

	memset(name, 0x00, sizeof(name));
	memcpy(name, my_config.device.Name, sizeof(my_config.device.Name));
	sprintf(XMLTemp,"%s", name );
	XML.Plain( "\" author=\"%.33s\">\n", XMLTemp );

	XML.OpenTag( "base" );

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

//	XML.TagOnce( "name", my_config.device.Name );				// sakaguchi cg 2020.08.27
//	XML.TagOnce( "name", name );
	XML.Plain( "<name>%s</name>\n", name);	 

	XML.CloseTag();		// <base>

	XML.Plain( "</file>\n" );		// <file 現在値>

	XML.Output();					// バッファ吐き出し

	int len = (int)strlen(xml_buffer);
	Printf("====> Download XML File size = %ld\r\n\r\n", strlen(xml_buffer));
	for(int i=0;i<len;i++){
		Printf("%c",xml_buffer[i]);
	}

	Printf("\r\n");

}
