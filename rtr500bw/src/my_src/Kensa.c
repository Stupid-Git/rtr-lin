/**
 * @file    Kensa.c
 * @brief  検査コマンド
 * @date    Created on: 2019/06/24
 * @author     haya
 * @note	2020.Jul.01 GitHub 630ソース反映済み
 * @attention	検査コマンド P1パラメータ P1=0 が読み込み
 * @note    2020.Aug.07 0807ソース反映済み
 */


#define _KENSA_C_

#include "Version.h"
#include "flag_def.h"

#include "MyDefine.h"
#include "Config.h"
#include "Error.h"

#include "system_thread.h"
#include "ble_thread.h"
#include "event_thread.h"
#include "rf_thread.h"
#include "network_thread.h"
//#include "usb_thread.h"
//#include "led_thread.h"
#include "udp_thread.h"
#include "tcp_thread.h"
#include "cmd_thread_entry.h"
#include "led_thread_entry.h"
#include "usb_thread_entry.h"

#include "Config.h"
#include "Globals.h"
#include "Sfmem.h"
#include "DateTime.h"
#include "Config.h"
#include "Base.h"
#include "Kensa.h"
#include "Cmd_func.h"


//extern TX_THREAD network_thread;
//extern TX_THREAD ble_thread;
//extern TX_THREAD system_thread;
//extern TX_THREAD usb_thread;
extern TX_THREAD led_thread;
//extern TX_THREAD rf_thread;
extern TX_THREAD event_thread;
extern TX_THREAD udp_thread;
extern TX_THREAD tcp_thread;
extern TX_THREAD network_thread;

#define K_LED_BLE_R		0x0001
#define K_LED_BLE_G		0x0002
#define K_LED_BLE_B		0x0004
#define K_LED_POWER		0x0008
#define K_LED_ACTIVE	0x0010
#define K_LED_DIAG		0x0020
#define K_LED_CBSY		0x0040
#define K_LED_WIFI		0x0080
#define K_LED_ALARM		0x0100

#define XRAM_ADDRESS_START  (0x84000000)
#define XRAM_LENGTH         (0x00080000)
#define XRAM_ADDRESS_END    (XRAM_ADDRESS_START+XRAM_LENGTH-1)

//SST26VF032BT  32Mbit
#define SPI_FLASH_ADDRESS_START  (0x00000000)
#define SPI_FLASH_LENGTH         (0x00400000)
#define SPI_FLASH_ADDRESS_END    (SPI_FLASH_ADDRESS_START+SPI_FLASH_LENGTH-1)

//********************************************************
//	変数（static) ＆ プロトタイプ
//********************************************************


///検査コマンドで使用するパラメータ
///@note    2020.Jul.13 構造体にまとめた
static struct{
    uint16_t	cmd;			///< コマンド番号
    char	*pInData;		    ///<  ホストからのデータへのポインタ
    uint16_t	insize;		///<  ホストからのデータサイズ
//    char	**outp;		    ///<  返送するデータへのポインタのポインタ
    char   *pMsg;              ///<  返送するデータへのポインタ
    uint16_t	*outsize;	///<  返送するデータサイズへのポインタ
}KS;



//プロトタイプ
static uint16_t KsInit(void);
static uint16_t KsOutput(void);
static uint16_t KsLed(void);
static uint16_t KsNetInit(void);
static uint16_t KsDhcp(void);
static uint16_t KsTestNumber(void);
static uint16_t KsBleParm(void);
static uint16_t KsVender(void);

static void LED_All_Off(void);

static uint16_t KsSerial(void);
static uint16_t KsFactory(void);
static uint16_t KsSRAM(void);
static uint16_t KsMac(void);
static uint16_t KsBackup(void);
static uint16_t KsSpiFlash(void);

static const char *Magic_Code = "T&D Corporation 2020";


//********************************************************

/**
 * @brief   コマンドテーブル宣言
 * （後方配置によりプロトタイプ宣言をパスさせる）      ←なぜ？
 * @note    2020.01.20  関数の型をuint16t func(void)に修正
 * @note    パディングあり
 */
static const struct {

    int16_t     Code;               // コマンドコード
    uint16_t    (* const Func)(void);   // 呼び出し先関数

} KENSA_TABLE[] = {

    {  0, KsInit         },     // 検査に先立ち必ず呼び出す

    //---- 以下へ追加していく
    {  1, KsSerial      },      // シリアル番号書き込み

    {  2, KsFactory     },      // 工場出荷状態設定

    {  3, KsSRAM        },      // SRAMのチェック(Read/Write)
    {  4, KsMac         },      // MAC ADDRESS書き込み ETH
    //{  4, KsInput     },      // 外部入力端子 入力確認
    {  5, KsOutput      },      // 外部入力端子 出力確認
    {  6, KsLed         },      // LED点灯チェック
//  {  7, KsRTC         },      // RTCカウンタの読み出し
//  {  8, KsEEPROM      },      // EEPROM
    {  8, KsSpiFlash      },    // SPI FLASH(Read/Write/Erase)
//  {  9, KsCurrent     },      // 電流測定モード
    { 11, KsTestNumber  },      // 検査番号（製造番号）Read Write
    { 12, KsBleParm     },      // BLEパラメータ書き込み CAP TRIM値、アドバタイズデータ更新周期（0x9Eコマンド周期）
    { 13, KsVender      },      // Vender T&D ESPEC Hitachi

	{ 15, KsBackup      },      // Back up Test
//  SCAP    SRAM Backup check

    // Network
    { 21, KsNetInit     },      // 仮のNetworkを設定値を設定（必ず最初にこれ）
    { 22, KsDhcp        },      // DHCP Test
//  { 23, KsIchipFirm   },      // ICHIP ファーム書き込み
//  { 24, KsIchipVer    },      // ICHIP バージョンチェック
//  { 25, KsIchipReset  },      // ICHIP リセット
//  { 26, KsIchipVer2   },      // ICHIP バージョンチェック 2

    // RF & IR 関係
//  { 31, KsRFTest      },      // RFモジュールテスト
//  { 32, KsRFTestJP    },      // RFモジュールテスト

//  { 35, KsIRTest      },      // IRテスト

    // その他
//  { 41, KsLowPower    },      // 低電圧状態読み込み

    //---- リセット
//  { 91, KsBackup      },      // SRAMとRTCの保持確認
//  { 97, KsMagicCode   },      // マジックコード書き込み
//  { 98, KsReset       },      // リセット

    //---- 検査終了
//  { 99, KsFinalize    },      // 検査終了時呼び出す

    //---- 動作確認用
//  { 100, KsConfirm    },      // ダミー

    //---- ここまで
    { -1, 0 },                  // 終了タグ
};


/**
 * @brief   [KENSA] 検査コマンド
 * 検査コマンド入り口
 * command.c から呼ばれる
 * @param cmd   検査コマンド
 * @param p_in
 * @param sz_in
 * @param p_out
 * @param sz_out
 * @return  コマンド
 */
uint16_t KensaMain( int16_t cmd, char *p_in, uint16_t sz_in, char **p_out, uint16_t *sz_out )
{
    char    **KS_outp;         ///<  返送するデータへのポインタのポインタ

    KS.cmd = (uint16_t)cmd;         // コマンド番号
    KS.pInData = p_in;          // ホストからのデータへのポインタ
    KS.insize = sz_in;      // ホストからのデータサイズ
//    KS.outp = p_out;        // 返送するデータへのポインタのポインタ
    KS_outp = p_out;        // 返送するデータへのポインタのポインタ
    KS.outsize = sz_out;    // 返送するデータサイズへのポインタ

    KS.pMsg = *KS_outp;            // 返送するデータへのポインタ

    int index = 0;
//    sz_in = 0;      // 間借り→　禁止
    while( KENSA_TABLE[index].Code >= 0 ) {
        if ( cmd == (uint16_t)KENSA_TABLE[index].Code ) {
            cmd = (int16_t)KENSA_TABLE[index].Func();       // 関数呼び出し
            *KS.outsize = (uint16_t)(KS.pMsg - *KS_outp);          // 返送サイズ書き込み
            break;
        }
        index++;
    }

    return ((uint16_t)cmd);
}


/**
 * [0] 検査コマンド初期化
 * @return  0   成功
 * @attention   各スレッドの状態に関係なく、強制的にサスペンドしている
 */
static uint16_t KsInit(void)
{
	uint32_t status;

	status = tx_thread_suspend(&event_thread);
	Printf("[KENSA] event_thread %02X\r\n", status);
	//status = tx_thread_suspend(&ble_thread);
	//Printf("[KENSA] ble_thread %02X\r\n", status);
	status = tx_thread_suspend(&led_thread);
	Printf("[KENSA] led_thread %02X\r\n", status);
	//status = tx_thread_suspend(&network_thread);
	//Printf("[KENSA] network_thread %02X\r\n", status);
	status = tx_thread_suspend(&udp_thread);
	Printf("[KENSA] udp_thread %02X\r\n", status);
	status = tx_thread_suspend(&tcp_thread);
	Printf("[KENSA] tcp_thread %02X\r\n", status);
	
	g_timer3.p_api->stop(g_timer3.p_ctrl);
	TestMode = 1;
	KS.pMsg += sprintf( KS.pMsg, MODE_CODE_STRING );		//機種コード文字列
    return (0);
}




/**
 * [1] シリアル番号書き込み
 * @retval  0   成功
 * @retval  1   失敗
 * @attention    書き込み値はバイナリ値、読み出し値はStringになっている
 */
static uint16_t KsSerial(void)
{
	int	 p1;
	uint16_t flg = 0;
    int i;
    char *dst;


	p1 = ParamInt( "P1" );

    Printf(" #INSPECT  Serial  Write  (%u)  %d %d\r\n", KS.cmd, p1, KS.insize );
	if ( p1 == 0 ) {	// read
		//EEP_RDP( Admin.SerialCode, IchipTemp );
		//Msg += sprintf( Msg, "%.8s", IchipTemp );
	}
	else if ( KS.insize == 4 ) {
        Printf("Write Serial Number ");
        dst = (char *)&fact_config.SerialNumber;
        for(i=0;i<4;i++){
            Printf("%02X ", *KS.pInData);
            *dst++ = *KS.pInData++;
        }
        Printf("\r\n");
        
        my_config.device.SerialNumber = fact_config.SerialNumber;
        Printf(" %08X \r\n", fact_config.SerialNumber);

        write_system_settings();
		//EEP.Write( KS.pInData, EEP_TAGADRS(Admin.SerialCode), EEP_TAGSIZE(Admin.SerialCode) );
		//Msg += sprintf( Msg, "%.8s",KS.pInData );
	}
	else {
	    KS.pMsg += sprintf( KS.pMsg, "Illegal Size" );
		flg = 1;
	}

	// EEP Version
	//EEP_WRVAL( Admin.EepVersion , 1 );	// 同時にEEPROMの識別番号も書いておく

	return (flg);
}



/**
 * [2] 工場出荷状態 Userエリア
 * T&Dの工場出荷状態に戻す
 * @retval  0   成功
 * @retval  1   失敗
 * @note    2020.Jun.30 P1未使用なので修正
 */
static uint16_t KsFactory(void)
{
//	int	 p1;
	uint16_t flg = 0;
 //   int i;
 //   char *dst;


//	p1 = ParamInt( "P1" );

    Printf(" #INSPECT  Factory Default  (%u)  \r\n", KS.cmd );
// 2023.05.31 ↓
//#if 0
//    if(fact_config.Vender == VENDER_HIT){
//        init_factory_default(1);    //日立
//    }
//    else
//#endif
//    {
//        init_factory_default(0, 0);    //T&D
//    }
    init_factory_default(fact_config.Vender, 0);
// 2023.05.31 ↑
	return (flg);
}




/**
 * @brief   [3] 外部SRAMテスト(READ/Write)
 * @note    外部SRAM設定 CS4   0x84000000, LENGTH = 0x0080000  512K byte
 * @retval  0   成功
 * @retval  1   失敗
 * @note    2020.Jul.14 未検証
 * @note    2020.Jul.28 チェック済み
 */
static uint16_t KsSRAM(void)
{
    uint32_t address;
    uint32_t size;
    int  p1;
    uint16_t ret = 1;

    p1 = ParamInt( "P1" );


    if ( KS.insize >= 8 ) {
        address = *(uint32_t *)&KS.pInData[0];
        size = *(uint32_t *)&KS.pInData[4];

        //チェック(とりあえず一回4KBまで)
        if((address < XRAM_ADDRESS_START) || ((address + size) > XRAM_ADDRESS_END) || (size > UX_TX_BUFFER_SIZE)){
            KS.pMsg += sprintf( KS.pMsg, "Illegal Size" );
            ret = 1;
        }
        else if ( p1 == 0 ) {    // read
            memcpy(KS.pMsg, (char *)address, size);
            KS.pMsg += size;
            ret = 0;
        }
        else if ( p1 == 1 ){       //write
            memcpy((char *)address, &KS.pInData[8], size);
            ret = 0;
        }else{
            ;
        }
    }
    else {
        KS.pMsg += sprintf( KS.pMsg, "Illegal Parameter Size" );
        ret = 1;
    }
    return(ret);

#if 0
	/*
	uint64_t adrs, count, src, i;
	uint8_t	rd,wd;

	Printf("  RAM Test Start \r\n");
	adrs = xml_buffer;  //0x84000000;
	wd = 0x3A;
	for(i=0;i< 0x007ffff;i++)
	{
		*( (uint8_t *)adrs ) = wd;
		 adrs ++;
	}

	adrs = 0x84000000;
	for(i=0;i< 0x007ffff;i++)
	{
		rd = *( (uint8_t *)adrs );
		adrs ++;
		Printf("[%02X] ", rd);

		//if(rd !=0x00)
		//{
		//	Printf("RAM Error 1 %ld \r\n");
		//	break;
		//}
	}

	adrs = 0x84000000;
	wd = 0xff;
	for(i=0;i< 0x007ffff;i++)
	{
		*( (uint8_t *)adrs ) = wd;
		 adrs ++;
	}

	adrs = 0x84000000;
	for(i=0;i< 0x007ffff;i++)
	{
		rd = *( (uint8_t *)adrs );
		adrs ++;
		if(rd !=0xff)
		{
			Printf("RAM Error 2 %ld \r\n");
			break;
		}
	}
*/

	char *dst;
	char rd,wd;
	int i ,j;

	dst = xml_buffer;

	wd = 0x00;
	for(i = 0; i < 1024; i++){
		*dst = wd;
		dst++;
		wd++;
	}

	dst = xml_buffer;
	j = 0;
	for(i = 0; i < 1024; i++){
		rd = *dst;
		dst++;
		Printf("[%02X] ", rd);
			
		j++;
		if(j==16){
			j=0;
			Printf("\r\n");
		}
	}


	return (0);
#endif

}


/**
 * @brief   [4] MACアドレス書き込み
 * @retval  0   成功
 * @retval  1   失敗
 * @attention    書き込み値はバイナリ値、読み出し値はStringになっている
 */
static uint16_t KsMac(void)
{
	int	 p1;
	uint16_t flg = 0;
    int i;
    char *dst;


	p1 = ParamInt( "P1" );

    Printf(" #INSPECT  MAC Address  Write  (%u)  %d %d\r\n", KS.cmd, p1, KS.insize );
	if ( p1 == 0 ) {	// read
		Printf	("MAC Read %04X-%08lX \r\n", (uint16_t)fact_config.mac_h, fact_config.mac_l);
		KS.pMsg += sprintf( KS.pMsg, "%04X-%08lX", (uint16_t)fact_config.mac_h, fact_config.mac_l);		//機種コード
	}
	else if ( KS.insize == 8 ) {
        Printf("Write MAC Address ");
        dst = (char *)&fact_config.mac_h;
        for(i=0;i<8;i++){
            Printf("%02X ", *KS.pInData);
            *dst++ = *KS.pInData++;
        }
        Printf("\r\n");
        
        Printf(" %04X-%04X \r\n", fact_config.mac_h, fact_config.mac_l);

        write_system_settings();
	}
	else {
	    KS.pMsg += sprintf( KS.pMsg, "Illegal Size" );
		flg = 1;
	}

	return (flg);
}

/**
 * @brief   [5] 外部出力
 * @return
 */
static uint16_t KsOutput(void)
{
//	int	 p1;
	uint16_t flg = 0;
	uint8_t port;

	//p1 = ParamInt( "P1" );
	port = *(uint8_t *)KS.pInData;

    Printf(" #INSPECT  Extern Port Output (%u)  %d %d\r\n", KS.cmd, port, KS.insize );
	if ( port == 1 ) {	//外部出力ON
		EX_WRAN_OFF;		
	}
	else {
		EX_WRAN_ON;
	}

	// EEP Version
	//EEP_WRVAL( Admin.EepVersion , 1 );	// 同時にEEPROMの識別番号も書いておく

	return (flg);
}

/**
 * @brief   [6] LED
 * @retval  0   成功
 * @retval  1   失敗
 * @attention   検査コマンド仕様と違う → 検査コマンドに合わせた2020.Jun.30
 * @bug       検査コマンド仕様と違う → 検査コマンドに合わせた2020.Jun.30
 * @note    2020.Jun.30  P1未使用なので修正
 */
static uint16_t KsLed(void)
{
//	int	 p1;
	uint16_t flg = 0;
 //   int i;
 //   char *dst;
	uint16_t led;

//	p1 = ParamInt( "P1" );
	if(KS.insize == 2){    //ホストからのデータサイズ
    //	led = (uint16_t)((uint16_t)*KS.pInData + (uint16_t)((*(KS.pInData+1)) << 8));
        led = *(uint16_t *)KS.pInData;

        Printf("[LED Test]  %04X\r\n", led);
        LED_All_Off();
        if(led & K_LED_BLE_R)
            LED_BLE_R_ON;
        if(led & K_LED_BLE_G)
            LED_BLE_G_ON;
        if(led & K_LED_BLE_B)
            LED_BLE_B_ON;
        if(led & K_LED_POWER)
            LED_POW_ON;
        if(led & K_LED_ACTIVE)
            LED_ACT_ON;
        if(led & K_LED_DIAG)
            LED_DIAG_ON;
        if(led & K_LED_CBSY)
            LED_CBSY_ON;
        if(led & K_LED_WIFI)
            LED_WIFI_ON;
        if(led & K_LED_ALARM)
            LED_ALM_ON;
	}else{
	    KS.pMsg += sprintf( KS.pMsg, "Illegal Size" );
        flg = 1;
	}
 	return (flg);
}


/**
 * @brief   [21] LAN DHCPテスト起動（Network 仮のParmeter設定）
 * @retval  0   成功
 * @retval  1   失敗
 * @attention   最終的に修正すること
 * @todo      最終的に修正すること
 */
static uint16_t KsNetInit(void)
{
	int	 p1;
	p1 = ParamInt( "P1" );

    Printf(" #INSPECT 21 %ld\r\n", p1);

	if ( p1 == 0 ){
		Printf("Ethrenet DHCP Test\r\n");
		my_config.network.Phy[0] = 0;	// Eth
	}
	else{
		Printf("WIFI DHCP Test\r\n");
		my_config.network.Phy[0] = 1;	// Wifi
	}
	my_config.network.DhcpEnable[0] = 1;
	
	strcpy((char *)my_config.wlan.SSID, "Buffalo-G-D920"); 
	strcpy((char *)my_config.wlan.PSK,  "ucw4w6iesm6f6"); 
	
	//my_config.wlan.SEC[0]    = 4;
	my_config.wlan.SEC[0]    = 3;

	fact_config.mac_h = 0x0000000D;
    //fact_config.mac_l = 0x8b0b0009;
	fact_config.mac_l = *(uint32_t *)KS.pInData;
	Printf(" %08X \r\n", fact_config.mac_l);

	NetStatus = NETWORK_DOWN;                       // sakaguchi 2020.09.17
	NetTest = 2;
	NetTestCount = 0;
	tx_thread_resume(&network_thread);

	tx_event_flags_set (&event_flags_link, FLG_LINK_REBOOT, TX_OR);
	
	return (0);
}


/**
 * @brief   [22] LAN DHCP結果取得
 * @retval  0   成功
 */
static uint16_t KsDhcp(void)
{
	Printf(" #INSPECT  22 %ld\r\n", NetTest);
    KS.pMsg += sprintf( KS.pMsg, "%d", NetTest );
	/*
	if(NetStatus == NETWORK_UP)
		pMsg += sprintf( pMsg, "0");
	else
		pMsg += sprintf( pMsg, "1");
	*/

	return (0);
}


/**
 * @brief   [11] 検査番号（製造番号）Read Write
 * @retval  0   成功
 * @retval  1   失敗
 * @attention    書き込み値はバイナリ値、読み出し値はStringになっている
 */
static uint16_t KsTestNumber(void)
{

	int	 p1;
	uint16_t flg = 0;
    int i;
    char *dst;


	p1 = ParamInt( "P1" );

//    Printf(" #INSPECT  Number Read  Write  (%u)  %d %d\r\n", KS.cmd, p1, KS.insize );
	if ( p1 == 0 ) {	// read
	    Printf(" #INSPECT  Read Manufacturing serial number \r\n");
	    KS.pMsg += sprintf( KS.pMsg, "%08lX", fact_config.TestNumber);
	}
	else if ( KS.insize == 4 ) {
	    Printf(" #INSPECT  Write Manufacturing serial number \r\n");
        dst = (char *)&fact_config.TestNumber;      // 検査番号（製造番号）
        for(i=0;i<4;i++){
            Printf("%02X ", *KS.pInData);
            *dst++ = *KS.pInData++;
        }
        Printf("\r\n");
        
        Printf(" %08X \r\n", fact_config.TestNumber);

        write_system_settings();
	}
	else {
	    KS.pMsg += sprintf( KS.pMsg, "Illegal Size" );
		flg = 1;
	}

	return (flg);


}

/**
 * @brief   [12] BLE Parameter Read Write
 *          BLEパラメータ書き込み CAP TRIM値、アドバタイズデータ更新周期（0x9Eコマンド周期）
 * @note   2020.Jun.29 仕様書では p1 =0がreadになっている
 * @attention   書き込み値はバイナリ値、読み出し値はStringになっている
 * @retval  0   成功
 * @retval  1   失敗
 * @note    グローバル変数 KS.insize 使用
 */
static uint16_t KsBleParm(void)
{

	int	 p1;
	uint16_t flg = 0;
    int i;
    char *dst;


	p1 = ParamInt( "P1" );

    Printf(" #INSPECT  BLE Read  Write  (%u)  %d %d\r\n", KS.cmd, p1, KS.insize );
	if ( p1 == 0 ) {	// read 
	    KS.pMsg += sprintf( KS.pMsg, "%02X%02X", fact_config.ble.cap_trim,fact_config.ble.interval_0x9e_command);
	}
	else if ( KS.insize == 2 ) {
        Printf("Write PSoC Cap Trim.");
        dst = (char *)&fact_config.ble.cap_trim;
        for(i=0;i<2;i++){
            Printf("%02X ", *KS.pInData);
            *dst++ = *KS.pInData++;
        }
        Printf("\r\n");
        
        Printf(" %02X %02X\r\n", fact_config.ble.cap_trim,fact_config.ble.interval_0x9e_command);

        write_system_settings();
	}
	else {
	    KS.pMsg += sprintf( KS.pMsg, "Illegal Size" );
		flg = 1;
	}

	return (flg);


}


/**
 * @brief   [13] Vender & Area Read Write
 * @retval  0   成功
 * @retval  1   失敗
 * @attention   書き込み値はバイナリ値、読み出し値はStringになっている
 */
static uint16_t KsVender(void)
{

	int	 p1;
	uint16_t flg = 0;
    int i;
    char *dst;

	p1 = ParamInt( "P1" );

    Printf(" #INSPECT  Vender Read  Write  (%u)  %d %d\r\n", KS.cmd, p1, KS.insize );
	if ( p1 == 0 ) {	// read 
	    KS.pMsg += sprintf( KS.pMsg, "V:%02XR:%02X", fact_config.Vender,fact_config.RfBand);
	}
	else if ( KS.insize == 2 ) {
        Printf("Verder  ");
        dst = (char *)&fact_config.Vender;
		for(i=0;i<2;i++){
            Printf("%02X ", *KS.pInData);
            *dst++ = *KS.pInData++;
        }
        Printf("\r\n");
        
        Printf(" %02X %02X\r\n", fact_config.Vender,fact_config.RfBand);

        write_system_settings();
	}
	else {
	    KS.pMsg += sprintf( KS.pMsg, "Illegal Size" );
		flg = 1;
	}

	return (flg);

}

/**
 * @brief   [15] Back up Test
 * @retval  0   成功
 * @retval  1   失敗
 * @attention   書き込み値はバイナリ値、読み出し値はStringになっている
 */
static uint16_t KsBackup(void)
{

	int	 p1;
//    int i;
//    char *dst;

	p1 = ParamInt( "P1" );

    Printf(" #INSPECT  Back Up Test (%u)  %d %d\r\n",  KS.cmd, p1 );
	if ( p1 == 0 ) {	// read 
		if(memcmp(BackUpTest,(char *) Magic_Code, strlen(Magic_Code)) == 0){
			KS.pMsg += sprintf( KS.pMsg, "0" );
		}
		else{
			KS.pMsg += sprintf( KS.pMsg, "1" );
		}
	}
	else {
		memset(BackUpTest, 0x00, sizeof(BackUpTest));
		sprintf(BackUpTest,"%s", Magic_Code);
	}

	return (0);

}

/*
 uint32_t    SerialNumber;       // シリアル番号
    uint32_t    dummy;
    UCHAR     Vender;             // 0: T&D 1:ESPEC
    UCHAR     RfBand;             // 0: JP   1:US    2:EU 

    struct{
        UCHAR     cap_trim;                                       // ＢＬＥ ＣＰＵ ＥＣＯ キャパシタトリム値
        UCHAR     interval_0x9e_command;                          // ＢＬＥ ０ｘ９ｅコマンドインターバル
    } ble;
    // 10 byte

    uint32_t    mac_h;                  // ETH MAC0 High Byte   etc.   0x00002E09               
    uint32_t    mac_l;                  // ETH MAC0 Low  Byte   etc.   0x0A0076C7

    uint32_t    w_mac_h;                // WIFI MAC0 High Byte   etc.   0x00002E09               
    uint32_t    W_mac_l;                // WIFI MAC0 Low  Byte   etc.   0x0A0076C7

    uint32_t    Kensa_Number;           // 検査番号

	"%.8lX", fact_config.SerialNumber);
*/



/**
 *  LED 全OFF
 */
static void LED_All_Off(void)
{
    LED_POW_OFF;
    LED_ALM_OFF;
    LED_CBSY_OFF;
    LED_DIAG_OFF;
    LED_WIFI_OFF;
    LED_ACT_OFF;
    LED_BLE_B_OFF;
    LED_BLE_R_OFF;
    LED_BLE_G_OFF;
}

/**
 * @brief    外部SPI フラッシュメモリ テスト(READ/Write/Erase)
 * SST26VF032BT  32Mbit
 * @retval  0   成功
 * @retval  1   失敗
 * @note    SPIフラッシュはメモリ空間にマッピングされていない
 * @note    2020.Jul.14 未検証
 * @note    2020.Jul.28 Read4KB セクタ消去、書き込み(16Byte)チェック済み
 */
static uint16_t KsSpiFlash(void)
{
    uint32_t address;
    uint32_t size;
    int  p1;
    uint16_t ret = 1;

    p1 = ParamInt( "P1" );


    if ( KS.insize >= 8 ) {
        address = *(uint32_t *)&KS.pInData[0];
        size = *(uint32_t *)&KS.pInData[4];

        //チェック(とりあえず一回4KBまで)
        if( ((address + size) > SPI_FLASH_ADDRESS_END) || (size > UX_TX_BUFFER_SIZE)){
            KS.pMsg += sprintf( KS.pMsg, "Illegal Size" );
            ret = 1;
        }
        else if ( p1 == 0 ) {    // read    とりあえず一回4KBまで
            if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS){
                serial_flash_multbyte_read(address, size, KS.pMsg);
                KS.pMsg += size;
                ret = 0;
                tx_mutex_put(&mutex_sfmem);
            }
            else{
                KS.pMsg += sprintf( KS.pMsg, "Busy Flash Memory" );
                ret = 1;
            }
        }
        else if ( p1 == 1 ){       //write  256Byteページ単位
            if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS){
                serial_flash_block_unlock();             // グローバルブロックプロテクション解除
                serial_flash_multbyte_write(address, size, &KS.pInData[8]);
                ret = 0;
                tx_mutex_put(&mutex_sfmem);
            }
            else{
                KS.pMsg += sprintf( KS.pMsg, "Busy Flash Memory" );
                ret = 1;
            }
        }
        else if ( p1 == 2 ){       //erase 4KBセクタ単位
            if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS){
                serial_flash_block_unlock();             // グローバルブロックプロテクション解除
                serial_flash_sector_erase(address);     //消去は4KB単位
                ret = 0;
                tx_mutex_put(&mutex_sfmem);
            }
            else{
                KS.pMsg += sprintf( KS.pMsg, "Busy Flash Memory" );
                ret = 1;
            }
        }
        else{
            ;
        }
    }
    else {
        KS.pMsg += sprintf( KS.pMsg, "Illegal Parameter Size" );
        ret = 1;
    }
    return(ret);
}

