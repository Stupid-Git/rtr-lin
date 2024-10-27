/**
 * @brief   LED Thread
 * @file    led_thread_entry.c
 *
 * @note    2019.Dec.26 ビルドワーニング潰し完了
 * @note    RTR-500 Ch Busy LED は、30秒後に消す
 * @note	2020.Jul.01 GitHub 630ソース反映済み
 */
#define _LED_THREAD_ENTRY_C_



#include "hal_data.h"
#include "MyDefine.h"
#include "Globals.h"
#include "General.h"
#include "led_thread_entry.h"
#include "led_thread.h"
#include "system_thread.h"

/*
typedef	struct {

	int16_t	Event;
	uint8_t	ActLink;		// LAN側へ切り替えるかどうか
	uint8_t	Over;			// この値までカウントアップ
	uint8_t	Keep;			// 0以外はこの値までで終了
	uint8_t	Err[3], Com[3], Alarm[3], Power[3], Busy[3], Wifi[3], Ble_B[3], Ble_R[3], Ble_G[3];

} LED_COLOR;
*/

static const LED_COLOR LED_TABLE[] = {	// 優先順位が高い順に並べる

//  イベント, Act, Cnt, 1shot { 1=ON / 0=OFF / 2=N/A , count1(絶対時間), count2(絶対時間) }
//                  Over Keep
//							    DIAG            ACTIVE          ALARM           POWER           BUSY            WIFI            BLE-Blue        BLE-Red         BLE-Green

	{ LED_SYSER	, 1,  8, 0,     { 1,  4,  8 },  { 1,  4,  8 },  { 1,  4,  8 },  { 1,  0,  0 },  { 1,  4,  8 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 } },    // 0
																				   
	{ LED_CUR,    1, 10, 0,     { 0,  0,  0 },  { 0,  0,  0 },  { 0,  0,  0 },  { 0,  0,  0 },  { 0,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 } },    // 1
	{ LED_TSTLED, 1, 30, 29,     { 1, 10, 20 },  { 1, 10, 20 },  { 1, 10, 20 },  { 1, 10, 20 },  { 1, 10, 20 },  { 1, 10, 20 },  { 1, 10, 20 },  { 2,  0,  0 },  { 2,  0,  0 } },   // 2

	{ LED_KENSA , 0, 40, 0,     { 1, 20, 40 },  { 0,  0,  0 },  { 1, 20, 40 },  { 1, 20, 40 },  { 1, 20, 40 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 } },    // 3        
																				   
	{ LED_START , 1, 30, 29,    { 1, 20, 30 },  { 1, 20, 30 },  { 1, 20, 30 },  { 1,  0,  0 },  { 1, 20, 30 },  { 1, 20, 30 },  { 1, 20, 30 },  { 2,  0,  0 },  { 2,  0,  0 } },    // 4
	{ LED_READY , 1,  5, 1,     { 0,  0,  0 },  { 1,  0,  0 },  { 0,  0,  0 },  { 2,  0,  0 },  { 0,  0,  0 },  { 0,  0,  0 },  { 0,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 } },    // 5

    { LED_USBCOM, 1,  8, 0,     { 2,  0,  0 },  { 1,  4,  8 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 } },    // 6
    { LED_BLECOM, 1,  8, 0,     { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 1,  4,  8 },  { 2,  0,  0 },  { 2,  0,  0 } },    // 7
    { LED_WIFICOM, 1, 4, 0,     { 2,  0,  0 },  { 1,  2,  4 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 1,  2,  4 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 } },    // 8
    { LED_LANCOM, 1,  8, 0,     { 0,  0,  0 },  { 1,  4,  8 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 } },    // 9
	{ LED_RF_COM, 1, 10, 0,     { 2,  0,  0 },  { 1,  6, 10 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 } },    // 10


	{ LED_USBON , 1,  5, 1,     { 2,  0,  0 },  { 1,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 } },    // 11
	{ LED_BLEON , 1,  5, 1,     { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 1,  0,  0 },  { 0,  0,  0 },  { 2,  0,  0 } },    // 12
	{ LED_WIFION , 1,  5, 1,    { 2,  0,  0 },  { 1,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 1,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 } },    // 13																		   
	{ LED_LANON , 1,  5, 1,     { 2,  0,  0 },  { 1,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 } },    // 14		

	{ LED_BLEOFF , 1,  5, 1,    { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 0,  0,  0 },  { 0,  0,  0 },  { 0,  0,  0 } },   // 15
	{ LED_WIFIOFF , 1,  5, 1,   { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 0,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 } },   // 16		
    { LED_LANOFF  , 1,  5, 1,   { 2,  0,  0 },  { 0,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 0,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 } },   // 17

	{ LED_LANER	, 1, 12, 1,     { 1,  6, 12 },  { 1,  6, 12 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 } },    // 18
																				   
	{ LED_NOTIM	, 1, 12, 1,     { 1,  6, 12 },  { 0,  0,  0 },  { 0,  0,  0 },  { 2,  0,  0 },  { 0,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 } },    // 19
	{ LED_NOREG	, 1,  5, 1,     { 1,  0, 0  },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 } },    // 20
																				   
	{ LED_INIT	, 1, 16, 1,     { 1,  0,  0 },  { 0,  8, 16 },  { 2,  0,  0 },  { 2,  0,  0 },  { 0,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 } },    // 21

																				   
	{ LED_DIAG	, 1,  5, 1,     { 1,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 } },    // 22
	{ LED_DIAGOFF , 1,  5, 1,   { 0,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 },  { 2,  0,  0 } },    // 23
																			   

    //							    DIAG            ACTIVE          ALARM           POWER           BUSY            WIFI            BLE-Blue        BLE-Red         BLE-Green
};



static int16_t led_count;
static int pos_old;
static int LedPos;
static int LedCount;

#define LED_ON_REQ 0x8001
#define LED_1500    30          // 1.5sec    
#define LED_3000    60          // 3.0sec    
#define LED_5000    100          // 5.0sec  

// LED Threadの Message Queueは、1word(4bytes)*5 に深さ
// uint32_t    cmd_id[4];

/** Led Thread entry function */
void led_thread_entry(void)
{
//    UINT status;
    ULONG led_msg[4];
    uint16_t flag,event;
    uint32_t		i;
    uint16_t temp;

    LED_POW_ON;
    
    led_count = 0;
    pos_old = 0;
    LedPos = LED_TABLE_SIZE - 1;
    LedCount = 0;
    
    memset( LED_PAT, 0, sizeof(LED_PAT) );	// 初期化

    Printf(" ------>  led thread start\r\n");
    //led_timer_cbsy = 0;
    //led_timer_alarm = 0;
    for(i=0;i<DIMSIZE(LED_TABLE);i++){
        LED_PAT[i] = 0;
    }
    LED_PAT2[0] = LED_PAT2[1] = 0;

    tx_queue_flush(&led_queue);

    led_timer_com = led_timer_ble = led_timer_wifi = LED_1500 - 1;    // 75*30 = 1500msec
    // 50ms Timer start
    g_timer3.p_api->open(g_timer3.p_ctrl, g_timer3.p_cfg);
    g_timer3.p_api->start(g_timer3.p_ctrl);


    while (1)
    {
        /*status = */tx_queue_receive(&led_queue, led_msg, TX_WAIT_FOREVER);

        event = (uint16_t)led_msg[0];
        flag  = (uint16_t)(led_msg[0] >> 16); 
        //Printf("++++>>>  LED Condition Change !!  %08X %02X %02X %02X\r\n", led_msg[0], event, flag, status);
		for ( i=0; i<DIMSIZE(LED_TABLE); i++ ) {
			if ( LED_TABLE[i].Event == event ) {	// イベント一致？
                //Printf("++++>>>  LED Condition Change !!  %08X %d (%d) <%d> %d %02X\r\n", led_msg[0], event, flag, i, status,led_timer_cbsy);
				LED_PAT[i] = flag | 0x8000;		            // ON/OFF
                switch (event){
                    case LED_USBCOM:
                        led_timer_com = 0;
                        break;
                    case LED_BLECOM:
                        led_timer_ble = 0;
                        break;
                    case LED_WIFICOM:
                        led_timer_wifi = 0;
                        break;
                    case LED_LANCOM:
                        led_timer_lan = 0;
                        break;
                    case LED_BLEON:
                        LED_PAT[15] = 0;    // BLE_OFF Clear
                        break;
                    case LED_WIFION:
                        LED_PAT[16] = 0;    // WIFI_OFF Clear
                        break;
                    case LED_LANON:
                        LED_PAT[17] = 0;    // LAN_OFF Clear
                        break;
                    default:
                        break;
                }
				break;
			}
        }

        //for ( i=0; i<DIMSIZE(LED_TABLE); i++ ) {
        //    Printf("%d-%04X ", i,LED_PAT[i]);
        //}


        switch (event){
            case LED_BUSY:
               	LED_PAT2[0] = flag;		            // ON/OFF
                if(flag == 1){                      // ON ?
                    led_timer_cbsy = 1;
                }
                break;
            case LED_WARNING:
                LED_PAT2[1] = flag;		            // ON/OFF
                if(flag == 1){                       // ON ?
                    led_timer_alarm = 1;
                }
                break;
            case LED_NETER:
                LED_PAT2[2] = flag;		            // ON/OFF
                if(flag == 1){                       // ON ?
                    led_timer_net_err = 2;
                }
                break;
            default:
                break;
        }
        //Printf("++++>>>  LED Condition Change !!  %08X %02X %02X %02X\r\n", led_msg[0], event, flag, status);
        //Printf("++++>>>  LED Event !!  pos=%d act=%d over=%d %d %d %d \r\n", i, LED_TABLE[i] .ActLink, LED_TABLE[i].Over, LED_TABLE[i].Com[0], LED_TABLE[i].Com[1],LED_TABLE[i].Com[2]);

        tx_thread_sleep(2);
    }
}



void LED_Set( uint16_t Msg , uint16_t flag)		// LED_**_** のみ指定可能
{
    ULONG   msg[4];
    char temp[32];


    if(TestMode==0){
        // debug 
        switch(Msg){
            case LED_START:
                sprintf(temp, "LED_START #6");
                break;
            case LED_DIAG:
                sprintf(temp, "LED_DIAG #25");
                break;
            case LED_USBON:
                sprintf(temp, "LED_USBON #21");
                break;
            case LED_BLEON:
                sprintf(temp, "LED_BLEON #26");
                break;
            case LED_WIFION:
                sprintf(temp, "LED_WIFION #28");
                break;
            case LED_LANON:
                sprintf(temp, "LED_LANON #30");
                break;
            case LED_USBCOM:
                sprintf(temp, "LED_USBCOM #22");
                break;
            case LED_BLECOM:
                sprintf(temp, "LED_BLECOM #27");
                break;
            case LED_RF_COM:
                sprintf(temp, "LED_RF_COM #24");
                break;
            case LED_LANCOM:
                sprintf(temp, "LED_LAN_COM #23");
                break;
            case LED_WIFICOM:
                sprintf(temp, "LED_WIFICOM #28");
                break;
            case LED_BLEOFF:
                sprintf(temp, "LED_BLEOFF #35");
                break;
            case LED_WIFIOFF:
                sprintf(temp, "LED_WIFIOFF #36");
                break;
            case LED_LANOFF:
                sprintf(temp, "LED_LANOFF #37");
                break;    
            case LED_BUSY:
                sprintf(temp, "LED_BUSY #81");
                break;
            case LED_WARNING:
                sprintf(temp, "LED_WARNING #80");
                break;
            case LED_DIAGOFF:
                sprintf(temp, "LED_DIAGOFF #38");
                break;
            case LED_NETER:
                sprintf(temp, "LED NET Error DIAG ACT ");
                break;
            default:
                sprintf(temp, "???");
                break;
        }
    
    
        Printf(" ===== ------>  led queue send  Msg %d  Flag %02X %s\r\n", Msg, flag, temp);
        msg[0] = (ULONG)Msg + ((ULONG)flag << 16);

        tx_queue_send(&led_queue, msg, WAIT_20MSEC);
    }
    //Printf(" ------>  led queue send  %02X \r\n", status);
}




/**
 * TIMER3 callback 1ms インターバル
 * 50msec
 *  @param p_args
 *  @note  処理時間 max 20usec
 */
void timer3_callback(timer_callback_args_t * p_args)
{
    ioport_level_t level;
    int     i;
    //int Pos = LED_TABLE_SIZE - 1;
    //int Count = 0;




    if(TIMER_EVENT_EXPIRED == p_args->event){

        for(i=0;i<=LED_TABLE_SIZE;i++)
        {
            if(LED_PAT[i]){         // 一番最初に見つかったイベントを処理

                if ( LedPos != i ) {           // ポジション変更？
//Printf("led_cyc_thread_entry  i=%d Pos=%d %04X  Keep %d (%d)\r\n", i,Pos,LED_PAT[i], LED_TABLE[i].Keep, Count);
                    if ( LED_TABLE[i].Err[0] < 2 ){     // DIAG
                        if(LED_TABLE[i].Err[0] == 1){   // On
                            if(LED_PAT[i] == LED_ON_REQ)  
                                g_ioport.p_api->pinWrite(PORT_LED_DIAG, IOPORT_LEVEL_LOW);
                            else
                                g_ioport.p_api->pinWrite(PORT_LED_DIAG, IOPORT_LEVEL_HIGH); 
                        }
                        else{
                            g_ioport.p_api->pinWrite(PORT_LED_DIAG, IOPORT_LEVEL_HIGH); 
                        }
                    }
                    if ( LED_TABLE[i].Com[0] < 2 ){     // ACT
                        if(LED_TABLE[i].Com[0] == 1){
                            if(LED_PAT[i] == LED_ON_REQ)  
                                g_ioport.p_api->pinWrite(PORT_LED_ACT, IOPORT_LEVEL_LOW);
                            else
                                g_ioport.p_api->pinWrite(PORT_LED_ACT, IOPORT_LEVEL_HIGH);
                        }
                        else{
                            g_ioport.p_api->pinWrite(PORT_LED_ACT, IOPORT_LEVEL_HIGH);
                        }
                    }
                    if ( LED_TABLE[i].Alarm[0] < 2 ){   // ALM
                        if(LED_TABLE[i].Alarm[0] == 1){
                            if(LED_PAT[i] == LED_ON_REQ)  
                                g_ioport.p_api->pinWrite(PORT_LED_ALM, IOPORT_LEVEL_LOW);
                            else
                                g_ioport.p_api->pinWrite(PORT_LED_ALM, IOPORT_LEVEL_HIGH);
                        }
                        else{
                            g_ioport.p_api->pinWrite(PORT_LED_ALM, IOPORT_LEVEL_HIGH);
                        }
                    }
                    if ( LED_TABLE[i].Power[0] < 2 ){   // POW
                        if(LED_TABLE[i].Power[0] == 1){
                            if(LED_PAT[i] == LED_ON_REQ) 
                                g_ioport.p_api->pinWrite(PORT_LED_POW, IOPORT_LEVEL_LOW);
                            else
                                g_ioport.p_api->pinWrite(PORT_LED_POW, IOPORT_LEVEL_HIGH);
                        }
                        else{
                            g_ioport.p_api->pinWrite(PORT_LED_POW, IOPORT_LEVEL_HIGH);
                        }
                    }
                    if ( LED_TABLE[i].Busy[0] < 2 ){    // C-BSY
                        if(LED_TABLE[i].Busy[0] == 1){
                            if(LED_PAT[i] == LED_ON_REQ) 
                                g_ioport.p_api->pinWrite(PORT_LED_CBSY, IOPORT_LEVEL_LOW);
                            else
                                g_ioport.p_api->pinWrite(PORT_LED_CBSY, IOPORT_LEVEL_HIGH);
                        }
                        else{
                            g_ioport.p_api->pinWrite(PORT_LED_CBSY, IOPORT_LEVEL_HIGH);
                        }
                    }
                    if ( LED_TABLE[i].Wifi[0] < 2 ){    // WIFI
                        if(LED_TABLE[i].Wifi[0] == 1){
                            if(LED_PAT[i] == LED_ON_REQ) 
                                g_ioport.p_api->pinWrite(PORT_LED_WIFI, IOPORT_LEVEL_LOW);
                            else
                                g_ioport.p_api->pinWrite(PORT_LED_WIFI, IOPORT_LEVEL_HIGH);
                        }
                        else{
                            g_ioport.p_api->pinWrite(PORT_LED_WIFI, IOPORT_LEVEL_HIGH);
                        }
                    }
                    if ( LED_TABLE[i].Ble_B[0] < 2 ){   // BLE
                        if(LED_TABLE[i].Ble_B[0] == 1){
                            if(LED_PAT[i] == LED_ON_REQ)  
                                g_ioport.p_api->pinWrite(PORT_LED_BLE_B, IOPORT_LEVEL_LOW);
                            else
                                g_ioport.p_api->pinWrite(PORT_LED_BLE_B, IOPORT_LEVEL_HIGH);
                        }
                        else{
                            g_ioport.p_api->pinWrite(PORT_LED_BLE_B, IOPORT_LEVEL_HIGH);
                        }
                    }
                    
                    if ( LED_TABLE[i].Ble_G[0] < 2 ){
                        if(LED_TABLE[i].Ble_G[0] == 1){
                            g_ioport.p_api->pinWrite(PORT_LED_BLE_G, IOPORT_LEVEL_LOW);
                        }
                        else{
                            g_ioport.p_api->pinWrite(PORT_LED_BLE_G, IOPORT_LEVEL_HIGH);
                        }
                    }
                    if ( LED_TABLE[i].Ble_R[0] < 2 ){
                        if(LED_TABLE[i].Ble_R[0] == 1){
                            g_ioport.p_api->pinWrite(PORT_LED_BLE_R, IOPORT_LEVEL_LOW);
                        }
                        else{
                            g_ioport.p_api->pinWrite(PORT_LED_BLE_R, IOPORT_LEVEL_HIGH);
                        }
                    }



                    LedPos = i;            // 新しいポジションをセット
                    LedCount = 0;          // 最初から
                }
                break;
            }
  
        }


        LedCount++;
        if ( LedCount == 0 ){
            LedCount = 1;
        }


               // ERR
        if ( LedCount == LED_TABLE[LedPos].Err[1] ) {         // 第１トグル
            if ( LED_TABLE[LedPos].Err[0] < 2 ){
                g_ioport.p_api-> pinRead(PORT_LED_DIAG, &level);
                if(level == IOPORT_LEVEL_HIGH){
                    g_ioport.p_api->pinWrite(IOPORT_PORT_05_PIN_07, IOPORT_LEVEL_LOW); 
                }
                else{
                    g_ioport.p_api->pinWrite(IOPORT_PORT_05_PIN_07, IOPORT_LEVEL_HIGH);
                }
            }
        }
        else if ( LedCount == LED_TABLE[LedPos].Err[2] ) {    // 第２トグル
            if ( LED_TABLE[LedPos].Err[0] < 2 ){
                g_ioport.p_api-> pinRead(PORT_LED_DIAG, &level);
                if(level == IOPORT_LEVEL_HIGH){
                    g_ioport.p_api->pinWrite(IOPORT_PORT_05_PIN_07, IOPORT_LEVEL_LOW);
                }
                else{
                    g_ioport.p_api->pinWrite(IOPORT_PORT_05_PIN_07, IOPORT_LEVEL_HIGH);
                }
            }
        }

        // COM
        if ( LedCount == LED_TABLE[LedPos].Com[1] ) {         // 第１トグル
            if ( LED_TABLE[LedPos].Com[0] < 2 ){
                g_ioport.p_api-> pinRead(PORT_LED_ACT, &level);
                if(level == IOPORT_LEVEL_HIGH){
                    g_ioport.p_api->pinWrite(PORT_LED_ACT, IOPORT_LEVEL_LOW);
                }
                else{
                    g_ioport.p_api->pinWrite(PORT_LED_ACT, IOPORT_LEVEL_HIGH);
                }
            }
        }
        else if ( LedCount == LED_TABLE[LedPos].Com[2] ) {    // 第２トグル
            if ( LED_TABLE[LedPos].Com[0] < 2 ){
                g_ioport.p_api-> pinRead(PORT_LED_ACT, &level);
                if(level == IOPORT_LEVEL_HIGH){
                    g_ioport.p_api->pinWrite(PORT_LED_ACT, IOPORT_LEVEL_LOW);
                }
                else{
                    g_ioport.p_api->pinWrite(PORT_LED_ACT, IOPORT_LEVEL_HIGH);
                }
            }
        }

        // ALARM
        if ( LedCount == LED_TABLE[LedPos].Alarm[1] ) {           // 第１トグル
            if ( LED_TABLE[LedPos].Alarm[0] < 2 ){
                g_ioport.p_api-> pinRead(PORT_LED_ALM, &level);
                if(level == IOPORT_LEVEL_HIGH){
                    g_ioport.p_api->pinWrite(PORT_LED_ALM, IOPORT_LEVEL_LOW);
                }
                else{
                    g_ioport.p_api->pinWrite(PORT_LED_ALM, IOPORT_LEVEL_HIGH);
                }
            }
        }
        else if ( LedCount == LED_TABLE[LedPos].Alarm[2] ) {  // 第２トグル
            if ( LED_TABLE[LedPos].Alarm[0] < 2 ){
                 g_ioport.p_api-> pinRead(PORT_LED_ALM, &level);
                 if(level == IOPORT_LEVEL_HIGH){
                     g_ioport.p_api->pinWrite(PORT_LED_ALM, IOPORT_LEVEL_LOW);
                 }
                else{
                    g_ioport.p_api->pinWrite(PORT_LED_ALM, IOPORT_LEVEL_HIGH);
                }
            }
        }

        // POWER
        if ( LedCount == LED_TABLE[LedPos].Power[1] ) {           // 第１トグル
            if ( LED_TABLE[LedPos].Power[0] < 2 ){
                g_ioport.p_api-> pinRead(PORT_LED_POW, &level);
                if(level == IOPORT_LEVEL_HIGH){
                    g_ioport.p_api->pinWrite(PORT_LED_POW, IOPORT_LEVEL_LOW);
                }
                else{
                    g_ioport.p_api->pinWrite(PORT_LED_POW, IOPORT_LEVEL_HIGH);
                }
            }
        }
        else if ( LedCount == LED_TABLE[LedPos].Power[2] ) {  // 第２トグル
            if ( LED_TABLE[LedPos].Power[0] < 2 ){
                 g_ioport.p_api-> pinRead(PORT_LED_POW, &level);
                 if(level == IOPORT_LEVEL_HIGH){
                     g_ioport.p_api->pinWrite(PORT_LED_POW, IOPORT_LEVEL_LOW);
                 }
                else{
                    g_ioport.p_api->pinWrite(PORT_LED_POW, IOPORT_LEVEL_HIGH);
                }
            }
        }

        // CH BUSY
        if ( LedCount == LED_TABLE[LedPos].Busy[1] ) {            // 第１トグル
            if ( LED_TABLE[LedPos].Busy[0] < 2 ){
                g_ioport.p_api-> pinRead(PORT_LED_CBSY, &level);
                if(level == IOPORT_LEVEL_HIGH){
                    g_ioport.p_api->pinWrite(PORT_LED_CBSY, IOPORT_LEVEL_LOW);
                }
                else{
                    g_ioport.p_api->pinWrite(PORT_LED_CBSY, IOPORT_LEVEL_HIGH);
                }
            }
        }
        else if ( LedCount == LED_TABLE[LedPos].Busy[2] ) {   // 第２トグル
            if ( LED_TABLE[LedPos].Busy[0] < 2 ){
                g_ioport.p_api-> pinRead(PORT_LED_CBSY, &level);
                if(level == IOPORT_LEVEL_HIGH){
                    g_ioport.p_api->pinWrite(PORT_LED_CBSY, IOPORT_LEVEL_LOW);
                }
                else{
                    g_ioport.p_api->pinWrite(PORT_LED_CBSY, IOPORT_LEVEL_HIGH);
                }
            }
        }

        // BLE Access
        if ( LedCount == LED_TABLE[LedPos].Ble_B[1] ) {           // 第１トグル
            if ( LED_TABLE[LedPos].Ble_B[0] < 2 ){
                g_ioport.p_api-> pinRead(PORT_LED_BLE_B, &level);
                if(level == IOPORT_LEVEL_HIGH){
                    g_ioport.p_api->pinWrite(PORT_LED_BLE_B, IOPORT_LEVEL_LOW);
                }
                else{
                    g_ioport.p_api->pinWrite(PORT_LED_BLE_B, IOPORT_LEVEL_HIGH);
                }
            }
        }
        else if ( LedCount == LED_TABLE[LedPos].Ble_B[2] ) {  // 第２トグル
            if ( LED_TABLE[LedPos].Ble_B[0] < 2 ){
                 g_ioport.p_api-> pinRead(PORT_LED_BLE_B, &level);
                 if(level == IOPORT_LEVEL_HIGH){
                     g_ioport.p_api->pinWrite(PORT_LED_BLE_B, IOPORT_LEVEL_LOW);
                 }
                else{
                    g_ioport.p_api->pinWrite(PORT_LED_BLE_B, IOPORT_LEVEL_HIGH);
                }
            }
        }

        // WIFI Access
        if ( LedCount == LED_TABLE[LedPos].Wifi[1] ) {            // 第１トグル
            if ( LED_TABLE[LedPos].Wifi[0] < 2 ){
                g_ioport.p_api-> pinRead(PORT_LED_WIFI, &level);
                if(level == IOPORT_LEVEL_HIGH){
                    g_ioport.p_api->pinWrite(PORT_LED_WIFI, IOPORT_LEVEL_LOW);
                }
                else{
                    g_ioport.p_api->pinWrite(PORT_LED_WIFI, IOPORT_LEVEL_HIGH);
                }
            }
        }
        else if ( LedCount == LED_TABLE[LedPos].Wifi[2] ) {   // 第２トグル
            if ( LED_TABLE[LedPos].Wifi[0] < 2 ){
                 g_ioport.p_api-> pinRead(PORT_LED_BLE_B, &level);
                 if(level == IOPORT_LEVEL_HIGH){
                     g_ioport.p_api->pinWrite(PORT_LED_WIFI, IOPORT_LEVEL_LOW);
                 }
                else{
                    g_ioport.p_api->pinWrite(PORT_LED_WIFI, IOPORT_LEVEL_HIGH);
                }
            }
        }

        if ( LedCount == LED_TABLE[LedPos].Keep ){			// １ショット？
            LED_PAT[LedPos] = 0;						// クリア
            //LedPos = LED_TABLE_SIZE - 1;
            //Printf("===> LED Keep clear(%d)<%d> %04X \r\n", LedPos,Count, LED_PAT[LedPos]);
        }

        if ( LedCount == LED_TABLE[LedPos].Over ){			// カウンタ
		    LedCount = 0;			// 最初から
            //Printf("===> LED Over clear(%d)\r\n",LedPos);
	    }

        // 点滅系のLEDはここで消す
        led_timer_com ++;
        if(led_timer_com == LED_5000){   // 5sec
            LED_PAT[6] = 0;         // USB
        }
        else if ( led_timer_com >  LED_5000)
        {
            led_timer_com =  LED_5000;
        }

        led_timer_wifi ++;
        if(led_timer_wifi ==  LED_5000){   // 1.5sec
            LED_PAT[8] = 0;         // WIFI
        }
        else if ( led_timer_wifi >  LED_5000)
        {
            led_timer_wifi =  LED_5000;
        }

        led_timer_lan ++;
        if(led_timer_lan ==  LED_5000){   // 5sec
            LED_PAT[9] = 0;         // LAN
        }
        else if ( led_timer_lan >  LED_5000)
        {
            led_timer_lan =  LED_5000;
        }



        led_timer_ble ++;
        if(led_timer_ble ==  LED_1500){
            LED_PAT[7] = 0;
        }
        else if ( led_timer_ble >  LED_1500)
        {
            led_timer_ble =  LED_1500;
        }

        //Printf("LED PAT2 %02X (%d)  %02X (%d) \r\n", LED_PAT2[0], led_timer_cbsy, LED_PAT2[1], led_timer_alarm);

        if(led_timer_cbsy !=0 ){
            led_timer_cbsy ++;
            if(led_timer_cbsy >= 600){
                LED_PAT2[0] = 0;
                g_ioport.p_api->pinWrite(PORT_LED_CBSY, IOPORT_LEVEL_HIGH);     //Off
                led_timer_cbsy = 0;
            }
            else 
            {
                if(led_timer_cbsy % 10 == 0){
                    g_ioport.p_api-> pinRead(PORT_LED_CBSY, &level);
                    if(level == IOPORT_LEVEL_HIGH){
                        g_ioport.p_api->pinWrite(PORT_LED_CBSY, IOPORT_LEVEL_LOW);
                    }
                    else{
                        g_ioport.p_api->pinWrite(PORT_LED_CBSY, IOPORT_LEVEL_HIGH);
                    }
                }
            }
        }

        if(led_timer_alarm !=0 ){
            led_timer_alarm ++;
           

            if(LED_PAT2[1] == 0){
                g_ioport.p_api->pinWrite(PORT_LED_ALM, IOPORT_LEVEL_HIGH);
                led_timer_alarm = 0;
            }
            else{
                if(led_timer_alarm % 10 == 0){
                    g_ioport.p_api-> pinRead(PORT_LED_ALM, &level);
                    if(level == IOPORT_LEVEL_HIGH){
                        g_ioport.p_api->pinWrite(PORT_LED_ALM, IOPORT_LEVEL_LOW);
                    }
                    else{
                        g_ioport.p_api->pinWrite(PORT_LED_ALM, IOPORT_LEVEL_HIGH);
                    }
                }
                if(led_timer_alarm == 0){
                    led_timer_alarm ++;
                    Printf("LED Timer Over flow\r\n");

                }
            }
        }

        if(led_timer_net_err !=0 ){
            led_timer_net_err ++;

            if(LED_PAT2[2] == 0){

                g_ioport.p_api->pinWrite(PORT_LED_DIAG, IOPORT_LEVEL_HIGH);
                g_ioport.p_api->pinWrite(PORT_LED_ACT, IOPORT_LEVEL_HIGH);
                led_timer_net_err = 0;
            }
            else{
                if(led_timer_net_err % 2 == 0){
                    g_ioport.p_api-> pinRead(PORT_LED_DIAG, &level);
                    if(level == IOPORT_LEVEL_HIGH){
                        g_ioport.p_api->pinWrite(PORT_LED_DIAG, IOPORT_LEVEL_LOW);
                        g_ioport.p_api->pinWrite(PORT_LED_ACT, IOPORT_LEVEL_LOW);
                    }
                    else{
                        g_ioport.p_api->pinWrite(PORT_LED_DIAG, IOPORT_LEVEL_HIGH);
                        g_ioport.p_api->pinWrite(PORT_LED_ACT, IOPORT_LEVEL_HIGH);
                    }
                }
                if(led_timer_net_err == 0){
                    led_timer_net_err ++;
                }
            }
        }        

    }
}

