/**
 * @file    led_thread_entry.h
 *
 * @date    2019/11/08
 * @author  tooru.hayashi
 */

#include "hal_data.h"
//#include "led_thread.h"

#ifndef _LED_THREAD_ENTRY_H_
#define _LED_THREAD_ENTRY_H_



#ifdef EDF
#undef EDF
#endif

#ifdef _LED_THREAD_ENTRY_C_
#define EDF
#else
#define EDF extern
#endif


// LED Port 定義 
#define PORT_LED_POW         IOPORT_PORT_05_PIN_08               ///< (out) LED POW    DS2 DS[1]
#define PORT_LED_WIFI        IOPORT_PORT_08_PIN_02               ///< (out) LED WIFI   DS7 DS[6]
#define PORT_LED_ACT         IOPORT_PORT_08_PIN_03               ///< (out) LED ACT    DS6 DS[5]
#define PORT_LED_DIAG        IOPORT_PORT_05_PIN_07               ///< (out) LED DIAG   DS3 DS[2]

#define PORT_LED_ALM         IOPORT_PORT_05_PIN_12               ///< (out) LED ALM    DS4 DS[3]
#define PORT_LED_CBSY        IOPORT_PORT_05_PIN_03               ///< (out) LED CBSY   DS5 DS[4]

#define PORT_LED_BLE_G       IOPORT_PORT_08_PIN_01               ///< (out) LED BLE 1  DS8  DS[7]  Green
#define PORT_LED_BLE_R       IOPORT_PORT_08_PIN_00               ///< (out) LED BLE 2  DS9  DS[8]  Red
#define PORT_LED_BLE_B       IOPORT_PORT_00_PIN_15               ///< (out) LED BLE 3  DS10 DS[9]  Blue


//#define LED_3C_BLUE     IOPORT_PORT_00_PIN_15               // (out) LED 3 color blue  DS10 DS[9]          


#define LED_OUT(X,Y)    g_ioport.p_api->pinWrite(X, Y)


//#define LED_ON           IOPORT_LEVEL_LOW
//#define LED_OFF          IOPORT_LEVEL_HIGH


#define LED_POW_ON      g_ioport.p_api->pinWrite(PORT_LED_POW, IOPORT_LEVEL_LOW)
#define LED_POW_OFF     g_ioport.p_api->pinWrite(PORT_LED_POW, IOPORT_LEVEL_HIGH)

#define LED_ALM_ON      g_ioport.p_api->pinWrite(PORT_LED_ALM, IOPORT_LEVEL_LOW)
#define LED_ALM_OFF     g_ioport.p_api->pinWrite(PORT_LED_ALM, IOPORT_LEVEL_HIGH)

#define LED_CBSY_ON     g_ioport.p_api->pinWrite(PORT_LED_CBSY, IOPORT_LEVEL_LOW)
#define LED_CBSY_OFF    g_ioport.p_api->pinWrite(PORT_LED_CBSY, IOPORT_LEVEL_HIGH)

#define LED_DIAG_ON     g_ioport.p_api->pinWrite(PORT_LED_DIAG, IOPORT_LEVEL_LOW)
#define LED_DIAG_OFF    g_ioport.p_api->pinWrite(PORT_LED_DIAG, IOPORT_LEVEL_HIGH)

#define LED_ACT_ON      g_ioport.p_api->pinWrite(PORT_LED_ACT, IOPORT_LEVEL_LOW)
#define LED_ACT_OFF     g_ioport.p_api->pinWrite(PORT_LED_ACT, IOPORT_LEVEL_HIGH)

#define LED_WIFI_ON     g_ioport.p_api->pinWrite(PORT_LED_WIFI, IOPORT_LEVEL_LOW)
#define LED_WIFI_OFF    g_ioport.p_api->pinWrite(PORT_LED_WIFI, IOPORT_LEVEL_HIGH)

#define LED_BLE_G_ON     g_ioport.p_api->pinWrite(PORT_LED_BLE_G, IOPORT_LEVEL_LOW)
#define LED_BLE_G_OFF    g_ioport.p_api->pinWrite(PORT_LED_BLE_G, IOPORT_LEVEL_HIGH)
#define LED_BLE_R_ON     g_ioport.p_api->pinWrite(PORT_LED_BLE_R, IOPORT_LEVEL_LOW)
#define LED_BLE_R_OFF    g_ioport.p_api->pinWrite(PORT_LED_BLE_R, IOPORT_LEVEL_HIGH)
#define LED_BLE_B_ON     g_ioport.p_api->pinWrite(PORT_LED_BLE_B, IOPORT_LEVEL_LOW)
#define LED_BLE_B_OFF    g_ioport.p_api->pinWrite(PORT_LED_BLE_B, IOPORT_LEVEL_HIGH)




#define	LED_SYSER			 2		// システムエラー
#define	LED_TSTLED			 3		// LED検査
#define	LED_KENSA			 4		// 検査モード
#define	LED_CUR				 5		// 検査モード
#define	LED_START			 6		// 電源投入時
#define	LED_NOTIM			 7		// 時刻設定がされていない
#define	LED_INIT			 8		// 初期化中
#define	LED_USBINI			 9		// USB での初期化中

#define	LED_NOREG			11		// 子機登録無し			0B
#define	LED_LANER			12		// LANエラー			0C
#define	LED_AP				13		// AccessPointエラー	0D

#define	LED_USBON			21		// USB ON			15
#define	LED_USBCOM			22		// USB 通信中		16

#define	LED_LANCOM			23		// LAN通信中		17
#define	LED_RF_COM			24		// 無線通信中		18
#define	LED_DIAG			25		// 自律送信失敗		19

#define	LED_BLEON			26		// BLE ON			1A
#define	LED_BLECOM			27		// BLE 通信中		1B

#define	LED_WIFION			28		// WIFI ON			1C     
#define	LED_WIFICOM			29		// WIFI 通信中		1D
#define LED_LANON			30		// LAN ON			1E

#define	LED_BLEOFF			35		// BLE OFF			
#define	LED_WIFIOFF			36		// WIFI OFF			
#define LED_LANOFF		 	37		// LAN OFF
#define	LED_DIAGOFF			38		// DIAG OFF	

#define	LED_READY			50		// 準備完了（通常時）32	

#define LED_DUMMY			0x7f

#define	LED_WARNING			0x80
#define	LED_BUSY			0x81	// CH BUSY
#define LED_NETER			0x82


#define LED_ON              1
#define LED_OFF             0



EDF void LED_Set( uint16_t Msg , uint16_t flag);	

//EDF void LED_All_On(void);
//EDF void LED_All_Off(void);

typedef	struct {

	int16_t	Event;
	uint8_t	ActLink;		// LAN側へ切り替えるかどうか
	uint8_t	Over;			// この値までカウントアップ
	uint8_t	Keep;			// 0以外はこの値までで終了
	uint8_t	Err[3], Com[3], Alarm[3], Power[3], Busy[3], Wifi[3], Ble_B[3], Ble_R[3], Ble_G[3];

} LED_COLOR;



#define LED_TABLE_SIZE  24		//  LED_COLOR LED_TABLE[]を変えたときはサイズを合わせること


EDF uint16_t LED_PAT[LED_TABLE_SIZE+1];
EDF uint16_t LED_PAT2[3];

EDF uint16_t led_timer_com;       // 50msec 分解能
EDF uint16_t led_timer_ble;
EDF uint16_t led_timer_wifi;
EDF uint16_t led_timer_lan;
EDF uint16_t led_timer_cbsy;
EDF uint16_t led_timer_alarm;
EDF uint16_t led_timer_net_err;

#endif /* _LED_THREAD_ENTRY_H_ */
