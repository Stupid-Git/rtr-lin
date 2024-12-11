/*
 * debug__xmlsms.c
 *
 *  Created on: Dec 10, 2024
 *      Author: karel
 */


#include "_r500_config.h"
#if 1 //CONFIG_USE_DEBUG_THREAD

#include "r500_defs.h"

//===============================================================================
//#include "debug__headers.h"
#ifndef DEBUG__HEADERS_H_
#define DEBUG__HEADERS_H_

#include "stdint.h"

uint32_t process_debug_str_w(char DbgCmd[]); // SMS
uint32_t process_debug_str_W(char DbgCmd[]); // Warnings
uint32_t process_debug_str_X(char DbgCmd[]); // XML SMS


#endif /* DEBUG__HEADERS_H_ */
//===============================================================================

// Warning State
#define WS_UNKOWN   0
#define WS_NORMAL   1       // 正常 : (警報なし、復帰)
#define WS_NORMAL2  2       // 正常 : (前回の警報監視から今回の警報監視の間に警報発生したが今は正常)

#define WS_WARN     10      // 警報の種類の区別なしの警報
#define WS_HIGH     11
#define WS_LOW      12
#define WS_SENSER   13
#define WS_P_RISE   14
#define WS_P_FALL   15
#define WS_OVER     16
#define WS_UNDER    17

#include "Xml.h"
/*
 *
<device>
<serial>5FA30006</serial>
<model>RTR500BM</model>
<name>RTR500BM_5FA30006</name>
 <battery>
 <state>1</state> //2021-07-02 17:52:11 [電池残量] 正常復帰
 <event_unixtime>1625212331</event_unixtime>
 <level>2</level>
 </battery>
</device>
 *
 */

/*
 *
</header>

<device>
<serial>5FA30006</serial>
<model>RTR500BM</model>
<name>RTR500BM_5FA30006</name>
  <battery>
  <state>10</state> //2021-07-02 17:47:11   [電池残量] 警報発生中
  <event_unixtime>1625212031</event_unixtime>
  <level>0</level>
  </battery>
</device>

</file>
 *
 */
//-----------------------------------------------------------------------------
// Active_Battery .Battery
//-----------------------------------------------------------------------------
void Tag_battery_Koki(int state);
void Tag_battery_Base(int state);
void Tag_ext_ps_Base(int ws_state);

void Tag_battery_Koki(int state) /// (WR_BAT_UP | WR_BAT_DOWN)
{
    XML.OpenTag("battery");
    XML.TagOnce("state", "%d", state);
    XML.TagOnce("event_unixtime", "%ld", WarnVal.utime);
    XML.TagOnce("level", "%d", 1);
#if USE_CONFIG_WARNSMS_ACTIVE_TAG
    XML.TagOnce("tbr_active_tag", "Active_Battery"); // mask.SendFlag.Active.Battery
#endif
    XML.CloseTag();             //</battery>
}

void Tag_battery_Base(int state) /// (WR_B_BAT_UP | WR_B_BAT_DOWN)
{
    XML.OpenTag("battery");
    XML.TagOnce("state", "%d", state);
    XML.TagOnce("event_unixtime", "%ld", WarnVal.utime);
    XML.TagOnce("level", "%d", 1);
#if USE_CONFIG_WARNSMS_ACTIVE_TAG
    XML.TagOnce("tbr_active_tag", "Active_Battery"); // mask.SendFlag.Active.Battery
#endif
    XML.CloseTag();             //</battery>
}

/*
 *
</header>

<device>
<serial>5FA30006</serial>
<model>RTR500BM</model>
<name>RTR500BM_5FA30006</name>
 <ext_ps>
 <state>2</state> // 2021-07-02 17:55:03    [外部電源] 警報発生を検知しましたが、現在は正常復帰しています。
 <event_unixtime>1625212503</event_unixtime>
 </ext_ps>
</device>

</file>
 *
 */

/*
 *
</header>
<device>
<serial>5FA30006</serial>
<model>RTR500BM</model>
<name>RTR500BM_5FA30006</name>
 <ext_ps>
 <state>10</state> // 2021-07-02 18:00:03    [外部電源] 警報発生中
 <event_unixtime>1625212803</event_unixtime>
 </ext_ps>
</device>
</file>
 *
 */

/*
 *
</header>
<device>
<serial>5FA30006</serial>
<model>RTR500BM</model>
<name>RTR500BM_5FA30006</name>
 <ext_ps>
 <state>1</state> // 2021-07-02 18:07:11    [外部電源] 正常復帰
 <event_unixtime>1625213231</event_unixtime>
 </ext_ps>
</device>
</file>

 *
 */
void Tag_ext_ps_Base(int ws_state) /// (WR_EXP_UP | WR_EXP_DOWN)
{
    XML.OpenTag("ext_ps");
    XML.TagOnce("state", "%d", ws_state);
    XML.TagOnce("event_unixtime", "%ld", WarnVal.utime);
#if USE_CONFIG_WARNSMS_ACTIVE_TAG
    XML.TagOnce("tbr_active_tag", "Active_Battery"); // mask.SendFlag.Active.Battery
#endif
    XML.CloseTag();                 //</ext_ps>
}

//-----------------------------------------------------------------------------
// Active_Input .Input
//-----------------------------------------------------------------------------
void Tag_input_Base(int ws_state);
void Tag_input_Base(int ws_state) /// (WR_SW_UP | WR_SW_DOWN)
{
    XML.OpenTag("input");
    XML.TagOnce("state", "%d", ws_state);
    XML.TagOnce("event_unixtime", "%ld", WarnVal.utime);
#if USE_CONFIG_WARNSMS_ACTIVE_TAG
    XML.TagOnce("tbr_active_tag", "Active_Input"); // mask.SendFlag.Active.Input
#endif
    XML.CloseTag();                 //</input>
}



//-----------------------------------------------------------------------------
// Active_UpLo .UpLo
//-----------------------------------------------------------------------------
/*
<device>
<serial>52820397</serial>
<model>RTR502B</model>
<name>Test_502</name>
<group>Group1</group>
<ch>
<num>1</num>
<name></name>
<state>11</state> // 2021-07-02 18:17:13    [ch1] 上限値警報: 35.7 C (上限値: 30.0 C)
<event_unixtime>1625213726</event_unixtime>
<id>6</id>
<value>35.7</value>
<unit>C</unit>
<upper_limit>30.0</upper_limit>
<lower_limit></lower_limit>
<judgement_time>30</judgement_time>
</ch>
</device>
 */

void Tag_ch_temp_Fail_HI(void);
void Tag_ch_temp_Fail_LO(void);
void Tag_ch_temp_Recover_HI(void);
void Tag_ch_temp_Recover_LO(void);
void Tag_ch_temp_Fail_HI_Recover(void);
void Tag_ch_temp_Fail_LO_Recover(void);
void Tag_ch_edge_P_RISE(void);
void Tag_ch_edge_P_FALL(void);
void Tag_ch_edge_NORMAL_NODATA(void);


void Tag_ch_temp_Fail_HI(void) // WS_HIGH 11
{
    XML.OpenTag("ch");                      //<ch>
    XML.TagOnce("num", "1");                //<num>1</num>
    XML.TagOnce("name", "");                //<name></name>
  XML.TagOnce("state", "%d", WS_HIGH); // "11" //<state>11</state> // 2021-07-02 18:17:13    [ch1] 上限値警報: 35.7 C (上限値: 30.0 C)
  XML.TagOnce("event_unixtime", "1625213726");    //<event_unixtime>1625213726</event_unixtime>
  XML.TagOnce("id", "6");                 //<id>6</id>
  XML.TagOnce("value", "35.7");           //<value>35.7</value>
    XML.TagOnce("unit", "C");               //<unit>C</unit<
    XML.TagOnce("upper_limit", "30.0");     //<upper_limit>30.0</upper_limit>
    XML.TagOnce("lower_limit", "");         //<lower_limit></lower_limit>
    XML.TagOnce("judgement_time", "30");    //<judgement_time>30</judgement_time>
#if USE_CONFIG_WARNSMS_ACTIVE_TAG
    XML.TagOnce("tbr_active_tag", "Active_UpLo"); // mask.SendFlag.Active.UpLo
#endif
    XML.CloseTag();                         //</ch>
}

void Tag_ch_temp_Fail_LO(void) // WS_LOW 12
{
    XML.OpenTag("ch");                      //<ch>
    XML.TagOnce("num", "2");                //<num>2</num>
    XML.TagOnce("name", "");                //<name></name>
    XML.TagOnce("state", "%d", WS_LOW); // "12" //<state>12</state> // 2021-07-02 18:17:13    [ch1] 上限値警報: 35.7 C (上限値: 30.0 C)
  XML.TagOnce("event_unixtime", "1625213726");    //<event_unixtime>1625213726</event_unixtime>
  XML.TagOnce("id", "6");                 //<id>6</id>
  XML.TagOnce("value", "-42.0");           //<value>35.7</value>
    XML.TagOnce("unit", "C");               //<unit>C</unit<
    XML.TagOnce("upper_limit", "");     //<upper_limit>30.0</upper_limit>
    XML.TagOnce("lower_limit", "-30.0");         //<lower_limit></lower_limit>
    XML.TagOnce("judgement_time", "30");    //<judgement_time>30</judgement_time>
#if USE_CONFIG_WARNSMS_ACTIVE_TAG
    XML.TagOnce("tbr_active_tag", "Active_UpLo"); // mask.SendFlag.Active.UpLo
#endif
    XML.CloseTag();                         //</ch>
}

/*
<device>
<serial>52820397</serial>
<model>RTR502B</model>
<name>Test_502</name>
<group>Group1</group>
<ch>
<num>1</num>
<name></name>
<state>1</state> // 2021-07-02 18:22:11 [ch1] 正常復帰
<event_unixtime></event_unixtime>
<id>6</id>
<value></value>
<unit>C</unit>
<upper_limit>30.0</upper_limit>
<lower_limit></lower_limit>
<judgement_time>30</judgement_time>
</ch>
</device>
 */
void Tag_ch_temp_Recover_HI(void)
{
    //XML.TagOnce("serial", "52820397");
    //XML.TagOnce("model", "RTR502B");
    //XML.Plain("<name>%s</name>\n", "Test_502");
    //XML.Plain("<group>%s</group>\n", "Group1");

     XML.OpenTag("ch");                      //<ch>
     XML.TagOnce("num", "1");                //<num>1</num>
     XML.TagOnce("name", "");                //<name></name>
   XML.TagOnce("state", "%d", WS_NORMAL); // "1" //<state>1</state> // 2021-07-02 18:22:11 [ch1] 正常復帰
   XML.TagOnce("event_unixtime", "");   //<event_unixtime></event_unixtime>
   XML.TagOnce("id", "6");              //<id>6</id>
   XML.TagOnce("value", "");            //<value></value>
    XML.TagOnce("unit", "C");               //<unit>C</unit<
    XML.TagOnce("upper_limit", "30.0");     //<upper_limit>30.0</upper_limit>
    XML.TagOnce("lower_limit", "");         //<lower_limit></lower_limit>
    XML.TagOnce("judgement_time", "30");    //<judgement_time>30</judgement_time>
#if USE_CONFIG_WARNSMS_ACTIVE_TAG
    XML.TagOnce("tbr_active_tag", "Active_UpLo"); // mask.SendFlag.Active.UpLo
#endif
    XML.CloseTag();                         //</ch>
}


void Tag_ch_temp_Recover_LO(void)
{
     XML.OpenTag("ch");                      //<ch>
     XML.TagOnce("num", "2");                //<num>1</num>
     XML.TagOnce("name", "");                //<name></name>
   XML.TagOnce("state", "%d", WS_NORMAL); // "1" //<state>1</state> // 2021-07-02 18:22:11 [ch1] 正常復帰
   XML.TagOnce("event_unixtime", "");   //<event_unixtime></event_unixtime>
   XML.TagOnce("id", "6");              //<id>6</id>
   XML.TagOnce("value", "");            //<value></value>
    XML.TagOnce("unit", "C");               //<unit>C</unit<
    XML.TagOnce("upper_limit", "");         //<upper_limit>30.0</upper_limit>
    XML.TagOnce("lower_limit", "-30.0");    //<lower_limit></lower_limit>
    XML.TagOnce("judgement_time", "30");    //<judgement_time>30</judgement_time>
#if USE_CONFIG_WARNSMS_ACTIVE_TAG
    XML.TagOnce("tbr_active_tag", "Active_UpLo"); // mask.SendFlag.Active.UpLo
#endif
    XML.CloseTag();                         //</ch>
}


/*
<device>
<serial>52820397</serial>
<model>RTR502B</model>
<name>Test_502</name>
<group>Group1</group>
<ch>
<num>1</num>
<name></name>
<state>2</state> // 2021-07-02 18:32:13 [ch1] 警報発生を検知しましたが、現在は正常復帰しています。 [警報発生時の測定値: 35.7 C (上限値: 30.0 C) ]
<event_unixtime>1625214456</event_unixtime>
<id>7</id>
<value>35.7</value>
<unit>C</unit<
<upper_limit>30.0</upper_limit>
<lower_limit></lower_limit>
<judgement_time>30</judgement_time>
</ch>
</device>
*/
void Tag_ch_temp_Fail_HI_Recover(void)
{
    //XML.TagOnce("serial", "52820397");
    //XML.TagOnce("model", "RTR502B");
    //XML.Plain("<name>%s</name>\n", "Test_502");
    //XML.Plain("<group>%s</group>\n", "Group1");

    XML.OpenTag("ch");                      //<ch>
    XML.TagOnce("num", "1");                //<num>1</num>
    XML.TagOnce("name", "");                //<name></name>
  XML.TagOnce("state", "%d", WS_NORMAL2); // "2" //<state>2</state> // 2021-07-02 18:32:13 [ch1] 警報発生を検知しましたが、現在は正常復帰しています。 [警報発生時の測定値: 35.7 C (上限値: 30.0 C) ]
  XML.TagOnce("event_unixtime", "1625214456");    //<event_unixtime>1625214456</event_unixtime>
  XML.TagOnce("id", "7");                 //<id>7</id>
  XML.TagOnce("value", "35.7");           //<value>35.7</value>
    XML.TagOnce("unit", "C");               //<unit>C</unit<
    XML.TagOnce("upper_limit", "30.0");     //<upper_limit>30.0</upper_limit>
    XML.TagOnce("lower_limit", "");         //<lower_limit></lower_limit>
    XML.TagOnce("judgement_time", "30");    //<judgement_time>30</judgement_time>
#if USE_CONFIG_WARNSMS_ACTIVE_TAG
    XML.TagOnce("tbr_active_tag", "Active_UpLo"); // mask.SendFlag.Active.UpLo
#endif
    XML.CloseTag();                         //</ch>
}

void Tag_ch_temp_Fail_LO_Recover(void) // WS_NORMAL2
{
    //XML.TagOnce("serial", "52820397");
    //XML.TagOnce("model", "RTR502B");
    //XML.Plain("<name>%s</name>\n", "Test_502");
    //XML.Plain("<group>%s</group>\n", "Group1");

    XML.OpenTag("ch");                      //<ch>
    XML.TagOnce("num", "2");                //<num>2</num>
    XML.TagOnce("name", "");                //<name></name>
  XML.TagOnce("state", "%d", WS_NORMAL2); // "2" //<state>2</state> // 2021-07-02 18:32:13 [ch1] 警報発生を検知しましたが、現在は正常復帰しています。 [警報発生時の測定値: 35.7 C (上限値: 30.0 C) ]
  XML.TagOnce("event_unixtime", "1625214456");    //<event_unixtime>1625214456</event_unixtime>
  XML.TagOnce("id", "7");                 //<id>7</id>
  XML.TagOnce("value", "-42.0");           //<value>35.7</value>
    XML.TagOnce("unit", "C");               //<unit>C</unit<
    XML.TagOnce("upper_limit", "");     //<upper_limit>30.0</upper_limit>
    XML.TagOnce("lower_limit", "-30.0");         //<lower_limit></lower_limit>
    XML.TagOnce("judgement_time", "30");    //<judgement_time>30</judgement_time>
#if USE_CONFIG_WARNSMS_ACTIVE_TAG
    XML.TagOnce("tbr_active_tag", "Active_UpLo"); // mask.SendFlag.Active.UpLo
#endif
    XML.CloseTag();                         //</ch>
}




//sprintf( WarnVal.Val, "No Data");           // No Data
//w_state = WS_P_RISE; 14
void Tag_ch_edge_P_RISE(void)
{
    /*
    if( ALM & 0x10 ){   // CH1警報が発生中
//      sprintf( WarnVal.Val, " %s%s ", ( edge == 1 ) ? LANG(W_RISE) : LANG(W_FALL), LANG(W_EDGE) );
        sprintf( WarnVal.Val, "%s %s", ( edge == 1 ) ? "Rising" : "Falling", "Edge" );
        w_state = ( edge == 1 ) ? WS_P_RISE : WS_P_FALL;
    }
    else if( on_off & BIT(0) ){     // CH1警報が警報監視間隔内に発生⇒復帰  // sakaguchi 2021.04.14 new
        sprintf( WarnVal.Val, "%s %s", ( edge == 1 ) ? "Rising" : "Falling", "Edge" );
        w_state = ( edge == 1 ) ? WS_P_RISE : WS_P_FALL;        // warnTypeにセットする
    }
    else{
        sprintf( WarnVal.Val, "No Data" );      //"NoData " );  // No Data
        w_state = WS_NORMAL;
    }
*/
    XML.OpenTag("ch");                          //<ch>
    XML.TagOnce("num", "1");                    //<num>1</num>
    XML.TagOnce("name", "");                    //<name></name>
    XML.TagOnce("state", "%d", WS_P_RISE); // "14" //<state>14</state>
    XML.TagOnce("event_unixtime", "1625214456");    //<event_unixtime>1625214456</event_unixtime>
    XML.TagOnce("value", "Rising Edge");            //<value>Sensor Error</value>
#if USE_CONFIG_WARNSMS_ACTIVE_TAG
    XML.TagOnce("tbr_active_tag", "Active_UpLo"); // mask.SendFlag.Active.UpLo
#endif
    XML.CloseTag();                             //</ch>
}

//w_state = WS_P_FALL; 15
void Tag_ch_edge_P_FALL(void)
{
    XML.OpenTag("ch");                          //<ch>
    XML.TagOnce("num", "1");                    //<num>1</num>
    XML.TagOnce("name", "");                    //<name></name>
    XML.TagOnce("state", "%d", WS_P_FALL); // "15" //<state>15</state>
    XML.TagOnce("event_unixtime", "1625214456");    //<event_unixtime>1625214456</event_unixtime>
    XML.TagOnce("value", "Falling Edge");            //<value>Sensor Error</value>
#if USE_CONFIG_WARNSMS_ACTIVE_TAG
    XML.TagOnce("tbr_active_tag", "Active_UpLo"); // mask.SendFlag.Active.UpLo
#endif
    XML.CloseTag();                             //</ch>
}

//w_state = WS_NORMAL; 1
void Tag_ch_edge_NORMAL_NODATA(void)
{
    XML.OpenTag("ch");                          //<ch>
    XML.TagOnce("num", "1");                    //<num>1</num>
    XML.TagOnce("name", "");                    //<name></name>
    XML.TagOnce("state", "%d", WS_NORMAL); // "1" //<state>1</state>
    XML.TagOnce("event_unixtime", "1625214456");    //<event_unixtime>1625214456</event_unixtime>
    XML.TagOnce("value", "No Data");            //<value>Sensor Error</value>
#if USE_CONFIG_WARNSMS_ACTIVE_TAG
    XML.TagOnce("tbr_active_tag", "Active_UpLo"); // mask.SendFlag.Active.UpLo
#endif
    XML.CloseTag();                             //</ch>
}


//-----------------------------------------------------------------------------
// Active_Sensor .Sensor
//-----------------------------------------------------------------------------
//sprintf( WarnVal.Val, "Sensor Error" );     // Senser Error
//w_state = WS_SENSER; //13

void Tag_ch_WS_SENSER(void);
void Tag_ch_WS_UNDER(void);
void Tag_ch_WS_OVER(void);
void Tag_ch_WS_NORMAL_NODATA(void);



void Tag_ch_WS_SENSER(void)
{
    XML.OpenTag("ch");                      //<ch>
    XML.TagOnce("num", "1");                //<num>1</num>
    XML.TagOnce("name", "");                //<name></name>
    XML.TagOnce("state", "%d", WS_SENSER); // "13" //<state>13</state>
    XML.TagOnce("event_unixtime", "1625214456");    //<event_unixtime>1625214456</event_unixtime>
  //XML.TagOnce("id", "7");                 //<id>7</id>
    XML.TagOnce("value", "Sensor Error");           //<value>Sensor Error</value>
  //XML.TagOnce("unit", "C");               //<unit>C</unit<
  //XML.TagOnce("upper_limit", "30.0");     //<upper_limit>30.0</upper_limit>
  //XML.TagOnce("lower_limit", "");         //<lower_limit></lower_limit>
  //XML.TagOnce("judgement_time", "30");    //<judgement_time>30</judgement_time>
#if USE_CONFIG_WARNSMS_ACTIVE_TAG
    XML.TagOnce("tbr_active_tag", "Active_Sensor"); // mask.SendFlag.Active.Sensor
#endif
    XML.CloseTag();                         //</ch>
}


//sprintf( WarnVal.Val, "Underflow" );        // Underflow
//w_state = WS_UNDER; // 17
void Tag_ch_WS_UNDER(void)
{
    XML.OpenTag("ch");                      //<ch>
    XML.TagOnce("num", "1");                //<num>1</num>
    XML.TagOnce("name", "");                //<name></name>
    XML.TagOnce("state", "%d", WS_UNDER); // "17" //<state>17</state>
    XML.TagOnce("event_unixtime", "1625214456");    //<event_unixtime>1625214456</event_unixtime>
    XML.TagOnce("value", "Underflow");           //<value>Sensor Error</value>
#if USE_CONFIG_WARNSMS_ACTIVE_TAG
    XML.TagOnce("tbr_active_tag", "Active_Sensor"); // mask.SendFlag.Active.Sensor
#endif
    XML.CloseTag();                         //</ch>
}


//sprintf( WarnVal.Val, "Overflow" );         // Overflow
//w_state = WS_OVER; // 16
void Tag_ch_WS_OVER(void)
{
    XML.OpenTag("ch");                      //<ch>
    XML.TagOnce("num", "1");                //<num>1</num>
    XML.TagOnce("name", "");                //<name></name>
    XML.TagOnce("state", "%d", WS_OVER); // "16" //<state>16</state>
    XML.TagOnce("event_unixtime", "1625214456");    //<event_unixtime>1625214456</event_unixtime>
    XML.TagOnce("value", "Overflow");           //<value>Sensor Error</value>
#if USE_CONFIG_WARNSMS_ACTIVE_TAG
    XML.TagOnce("tbr_active_tag", "Active_Sensor"); // mask.SendFlag.Active.Sensor
#endif
    XML.CloseTag();                         //</ch>
}


//sprintf( WarnVal.Val, "No Data");           // No Data
//w_state = WS_NORMAL; 1
void Tag_ch_WS_NORMAL_NODATA(void)
{
    XML.OpenTag("ch");                          //<ch>
    XML.TagOnce("num", "1");                    //<num>1</num>
    XML.TagOnce("name", "");                    //<name></name>
    XML.TagOnce("state", "%d", WS_NORMAL); // "1" //<state>XX</state>
    XML.TagOnce("event_unixtime", "1625214456");    //<event_unixtime>1625214456</event_unixtime>
    XML.TagOnce("value", "No Data");            //<value>Sensor Error</value>
#if USE_CONFIG_WARNSMS_ACTIVE_TAG
    XML.TagOnce("tbr_active_tag", "Active_Sensor"); // mask.SendFlag.Active.Sensor
#endif
    XML.CloseTag();                             //</ch>
}


//-----------------------------------------------------------------------------
// Active_RF .RF
//-----------------------------------------------------------------------------
void Tag_comm_WR_RF_DOWN(void);
void Tag_comm_WR_RF_UP(void);

void Tag_comm_WR_RF_UP(void) // WR_RF_UP -> state 10
{
    XML.OpenTag("comm");                        //<comm>
    XML.TagOnce("state", "%d", WS_WARN); // "10" //<state>10</state>
    XML.TagOnce("event_unixtime", "1625214456");//<event_unixtime>1625214456</event_unixtime>
    XML.TagOnce("type" ,"%d", 0);               //
#if USE_CONFIG_WARNSMS_ACTIVE_TAG
    XML.TagOnce("tbr_active_tag", "Active_RF"); // mask.SendFlag.Active.RF
#endif
    XML.CloseTag();                             //</comm>
}

void Tag_comm_WR_RF_DOWN(void) // WR_RF_DOWN -> state 1
{
    XML.OpenTag("comm");                        //<comm>
    XML.TagOnce("state", "%d", WS_NORMAL); // "1" //<state>1</state>
    XML.TagOnce("event_unixtime", "1625214456");//<event_unixtime>1625214456</event_unixtime>
  //XML.TagOnce("event_unixtime" ,"%ld", WarnVal.utime);
    XML.TagOnce("type" ,"%d", 0);               //
#if USE_CONFIG_WARNSMS_ACTIVE_TAG
    XML.TagOnce("tbr_active_tag", "Active_RF"); // mask.SendFlag.Active.RF
#endif
    XML.CloseTag();                             //</comm>
}




/*
 *
<device>
<serial>5FA30006</serial>
<model>RTR500BM</model>
<name>RTR500BM_5FA30006</name>
 *
 */
void Tag_HEAD_Base_Unit();
void Tag_HEAD_Koki_RTR502B(char* pGroup, char* pName);

void Tag_HEAD_Base_Unit()
{
    //XML.TagOnce("serial", "5FA30006");
    XML.TagOnce("serial", "5FA30005");
    XML.TagOnce("model", "RTR500BM");
    //XML.Plain("<name>%s</name>\n", "RTR500BM_5FA30006");
    XML.Plain("<name>%s</name>\n", "RTR500BM_5FA30005");
}

void Tag_HEAD_Koki_RTR502B(char* pGroup, char* pName)
{
    //XML.TagOnce("serial", "52820397");
    XML.TagOnce("serial", "52820396");
    XML.TagOnce("model", "RTR502B");
    XML.Plain("<name>%s</name>\n", pName); //"Test_502");
    XML.Plain("<group>%s</group>\n", pGroup); //"Group1");
    /*
    XML.TagOnce2("alias_group", "VGroup.Name" );
    */
}


#define BF_TRIG         1
#define BF_RECOVER      2
#define BF_ONOFF        4

#define BF_421 ( BF_ONOFF | BF_RECOVER | BF_TRIG )

void Push_Dev_Koki_Battery(uint32_t _bf);
void Push_Dev_Koki_Battery(uint32_t _bf)
{
    XML.OpenTag("device");  // <device>
    {
        Tag_HEAD_Koki_RTR502B("Group1", "RTR502_BATT");

        if(_bf & BF_TRIG) Tag_battery_Koki(10);
        if(_bf & BF_RECOVER) Tag_battery_Koki(1);
        if(_bf & BF_ONOFF) Tag_battery_Koki(2);
    }
    XML.CloseTag();  // </device>
}

void Push_Dev_Koki_UpLo_HI(uint32_t _bf);
void Push_Dev_Koki_UpLo_HI(uint32_t _bf)
{
    XML.OpenTag("device");  // <device>
    {
        Tag_HEAD_Koki_RTR502B("Group1", "RTR502_HI");

        if(_bf & BF_TRIG) Tag_ch_temp_Fail_HI();
        if(_bf & BF_RECOVER) Tag_ch_temp_Recover_HI();
        if(_bf & BF_ONOFF) Tag_ch_temp_Fail_HI_Recover();
    }
    XML.CloseTag();  // </device>
}

void Push_Dev_Koki_UpLo_LO(uint32_t _bf);
void Push_Dev_Koki_UpLo_LO(uint32_t _bf)
{
    XML.OpenTag("device");  // <device>
    {
        Tag_HEAD_Koki_RTR502B("Group1", "RTR502_LO");

        if(_bf & BF_TRIG) Tag_ch_temp_Fail_LO();
        if(_bf & BF_RECOVER) Tag_ch_temp_Recover_LO();
        if(_bf & BF_ONOFF) Tag_ch_temp_Fail_LO_Recover();
    }
    XML.CloseTag();  // </device>
}

void Push_Dev_Koki_Edge(uint32_t _bf);
void Push_Dev_Koki_Edge(uint32_t _bf)
{
    XML.OpenTag("device");  // <device>
    {
        Tag_HEAD_Koki_RTR502B("Group1", "RTR502_EDGE");

        if(_bf & BF_TRIG) Tag_ch_edge_P_RISE();
        if(_bf & BF_TRIG) Tag_ch_edge_P_FALL();
        if(_bf & BF_RECOVER) Tag_ch_edge_NORMAL_NODATA();

        //if(_bf & BF_TRIG) Tag_ch_temp_Fail_LO();
        //if(_bf & BF_RECOVER) Tag_ch_temp_Recover_LO();
        //if(_bf & BF_ONOFF) Tag_ch_temp_Fail_LO_Recover();
    }
    XML.CloseTag();  // </device>
}



void Push_Dev_Koki_Sensor(uint32_t _bf);
void Push_Dev_Koki_Sensor(uint32_t _bf)
{
    XML.OpenTag("device");  // <device>
    {
        Tag_HEAD_Koki_RTR502B("Group1", "RTR502_SEN");

        if(_bf & BF_TRIG) Tag_ch_WS_SENSER();
        if(_bf & BF_TRIG) Tag_ch_WS_UNDER();
        if(_bf & BF_TRIG) Tag_ch_WS_OVER();
        if(_bf & BF_RECOVER) Tag_ch_WS_NORMAL_NODATA();
    }
    XML.CloseTag();  // </device>
}

void Push_Dev_Koki_RF(uint32_t _bf);
void Push_Dev_Koki_RF(uint32_t _bf)
{
    XML.OpenTag("device");  // <device>
    {
        Tag_HEAD_Koki_RTR502B("Group1", "RTR502_RF");

        if(_bf & BF_TRIG) Tag_comm_WR_RF_UP();
        if(_bf & BF_RECOVER) Tag_comm_WR_RF_DOWN();
    }
    XML.CloseTag();  // </device>
}


void Push_Dev_Base_Unit_BATTERY_EXT_PS_INPUT(uint32_t _bf);
void Push_Dev_Base_Unit_BATTERY_EXT_PS_INPUT(uint32_t _bf)
{
    XML.OpenTag("device");  // <device>
    {
        Tag_HEAD_Base_Unit();

        if(_bf & BF_TRIG) Tag_battery_Base(WS_WARN); // "10"
        if(_bf & BF_RECOVER) Tag_battery_Base(WS_NORMAL); // "1"
        if(_bf & BF_ONOFF) Tag_battery_Base(WS_NORMAL2); // "2"

        if(_bf & BF_TRIG) Tag_ext_ps_Base(WS_WARN); // "10"
        if(_bf & BF_RECOVER) Tag_ext_ps_Base(WS_NORMAL); // "1"
        if(_bf & BF_ONOFF) Tag_ext_ps_Base(WS_NORMAL2); // "2"

        if(_bf & BF_TRIG) Tag_input_Base(WS_WARN); // "10"
        if(_bf & BF_RECOVER) Tag_input_Base(WS_NORMAL); // "1"
        if(_bf & BF_ONOFF) Tag_input_Base(WS_NORMAL2); // "2"
    }
    XML.CloseTag();  // </device>
/*
 <device>
 <serial>52820397</serial>
 <model>RTR502B</model>
 <name>Test_502</name>
 <group>Group1</group>
 <ch>
 <num>1</num>
 <name></name>
 <state>2</state>
 <event_unixtime>1625120576</event_unixtime>
 <id>5</id>
 <value>35.0</value>
 <unit>C</unit>
 <upper_limit>30.0</upper_limit>
 <lower_limit></lower_limit>
 <judgement_time>30</judgement_time>
 </ch>
 </device>
 */
}


void TEST_MAKE_XML_1(bool param_also_base_errors);
void TEST_MAKE_XML_1(bool param_also_base_errors)
{
    XML.Create(0);

    uint32_t Status = 1; //non zero warnings
    uint32_t GSec;

    uint32_t utime;
    int temp;
    char name[sizeof(my_config.device.Name) + 2];

    WarnVal.utime = RTC_GetLCTSec(0);                 // GMT秒をローカル秒へ変換
    sprintf(XMLTForm, "%s", my_config.device.TimeForm);  // 日付フォーマット

    //if( my_config.websrv.Mode[0] == 0 ){                        // DataServer           // sakaguchi 2021.03.01
    //    XML.Plain( XML_FORM_WARN_DATASRV );                     // <file .. name=" まで
    //}else{
    XML.Plain( XML_FORM_WARN);                             // <file .. name=" まで
    //}
    XML.Plain("%s\"", XML.FileName);
    XML.Plain(" type=\"2\"");
    memset(name, 0x00, sizeof(name));
    memcpy(name, my_config.device.Name, sizeof(my_config.device.Name));
    sprintf(XMLTemp, "%s", name);
    XML.Plain(" author=\"%s\">\n", XMLTemp);
    /*
     <?xml version="1.0" encoding="UTF-8"?>
     <file format="warning" version="1.00" name="RTR500BM_5FA30006-20210701-162713.xml" type="1" author="RTR500BM_5FA30006">
     */
    {   // <file>

        XML.OpenTag("header");  // <header>
        {
            /*
             <header>
             */

            utime = RTC_GetGMTSec();                // UTC取得
            XML.TagOnce("created_unixtime", "%lu", utime);    // 現在値の時刻
            temp = (int)(RTC.Diff / 60);                          // 時差
            XML.TagOnce("time_diff", "%d", temp);
            if(my_config.websrv.Mode[0] != 0) {                    // DataServer以外
                XML.TagOnce("std_bias", "0");                         // 標準時間のオフセット
                temp = (int)(RTC.AdjustSec / 60);                     // サマータイム中
                XML.TagOnce("dst_bias", "%d", temp);                  // サマータイムのオフセット
            } else {
                sprintf(XMLTemp, "select=\"%d\"", 1);                 // 標準時間/サマータイム（select属性)

                GSec = RTC_GetGMTSec();
                GSec = GSec + (uint32_t)RTC.Diff;
                if(GetSummerAdjust(GSec) == 0) {                      // 標準時間で動作 // sakaguchi 2021.04.07
//              if( RTC.SummerBase == 0 ){                              // 標準時間で動作
                    XML.TagOnceAttrib("std_bias", XMLTemp, "%d", 0);      // 標準時間のオフセット（select属性付）
                    temp = (int)(RTC.AdjustSec / 60);                     // サマータイム中
                    XML.TagOnce("dst_bias", "%d", temp);                  // サマータイムのオフセット

                } else {      // 夏時間で動作
                    XML.TagOnce("std_bias", "0");                         // 標準時間のオフセット
                    temp = (int)(RTC.AdjustSec / 60);                     // サマータイム中
                    XML.TagOnceAttrib("dst_bias", XMLTemp, "%d", temp);   // サマータイムのオフセット（select属性付）
                }
            }
            /*
             <created_unixtime>1625120833</created_unixtime>
             <time_diff>600</time_diff>
             <std_bias>0</std_bias>
             <dst_bias>0</dst_bias>
             */

            XML.OpenTag("base_unit");
            {
                sprintf(XMLTemp, "%08lX", fact_config.SerialNumber);
                XML.TagOnce("serial", XMLTemp);
#ifdef BRD_R500BM
                XML.TagOnce("model", "RTR500BM" /*BaseUnitName*/);
#else
                XML.TagOnce( "model", "RTR500BW" /*BaseUnitName*/);
#endif
                //XML.Plain( "<name>%s</name>\n", my_config.device.Name);   // 親機名称
                XML.Plain("<name>%s</name>\n", name);                      // 親機名称     // sakaguchi cg 2020.08.27
                //XML.TagOnce( "name", my_config.device.Name );

                // 警報の有りなしをチェック
                if(Status) {
                    XML.TagOnce("warning", "%d", 1);
                } else {
                    XML.TagOnce("warning", "%d", 0);
                }
            }
            XML.CloseTag();     // <base_unit>
            /*
             <base_unit>
             <serial>5FA30006</serial>
             <model>RTR500BM</model>
             <name>RTR500BM_5FA30006</name>
             <warning>1</warning>
             </base_unit>
             */

            ///TODO
            /*
             <gps type="dmm">
             <lat>3614.014206,N</lat>
             <lon>13756.809141,E</lon>
             <event_unixtime>1625120834</event_unixtime>
             </gps>

             <gps type="deg">
             <lat>+36.233570</lat>
             <lon>+137.946823</lon>
             <event_unixtime>1625120834</event_unixtime>
             </gps>
             */
        }
        XML.CloseTag(); // </header>
        /*
         </header>
         */
        uint32_t _bf;

        _bf = BF_421; // ( BF_ONOFF | BF_RECOVER | BF_TRIG )
        //_bf = BF_TRIG;
        //_bf = BF_RECOVER;
        //_bf = BF_ONOFF;
#if 0
        if(param_also_base_errors) {
            Push_Dev_Base_Unit_BATTERY_EXT_PS_INPUT(_bf);
        }
        Push_Dev_Koki_Battery(_bf);

        Push_Dev_Koki_UpLo_HI(_bf);
        Push_Dev_Koki_UpLo_LO(_bf);

        Push_Dev_Koki_Edge(_bf);

        Push_Dev_Koki_Sensor(_bf);

        Push_Dev_Koki_RF(_bf);
#else //2024
        Push_Dev_Koki_UpLo_HI(_bf);
        //Push_Dev_Koki_UpLo_LO(_bf);

#endif
    }
    XML.Plain( "</file>\n" );       // <file 現在値>
    /*
     </file>
     */

    XML.Output();                   // バッファ吐き出し

}


/*extern TODO*/ char g_smsTextStr[4096];

static void msTextStr_dump(void);
static void msTextStr_dump(void)
{
    tx_thread_sleep(100);
    PrintDbg(TD_INFO, "========================================\r\n");
    char *p = g_smsTextStr;
    while(*p != 0) {
        Printf("%c", *p);
        p++;
    }
    PrintDbg(TD_INFO, "========================================\r\n");
}


void TEST_XML_parse(void);
void TEST_XML_parse(void)
{


    //----- Parse -----
    {
        //size_t i;
        //size_t len;
        uint8_t mask;
        //int rtn;

        //len = strlen(xml_buffer);
#if 0
        Printf("====> XML File size = %ld\r\n\r\n", strlen(xml_buffer));
        for(i = 0; i < len ; i++) {
            Printf("%c", xml_buffer[i]);
        }
        Printf("\r\n");
#endif


#if 0
        int parse_sxml(uint8_t mask, uint8_t *xml_buf, size_t xml_len);
        mask = 0xFE;
        //parse_sxml(mask, (uint8_t*)xml_buffer, len);

        mask = 0xFD;
        //parse_sxml(mask, (uint8_t*)xml_buffer, len);
#endif

        int tdxml_convertXmlToSms(uint8_t mask, uint8_t *xml_buf, size_t xml_len, char* p_smsText, size_t maxStrlen);


        mask = 0xFF;
        //rtn =
// TODO        tdxml_convertXmlToSms(mask, (uint8_t*)xml_buffer, strlen(xml_buffer), g_smsTextStr, sizeof(g_smsTextStr) - 1);
        msTextStr_dump();
/*
        mask = 0xFE; //WR_MASK_HILO
        rtn = tdxml_convertXmlToSms(mask, (uint8_t*)xml_buffer, strlen(xml_buffer), g_smsTextStr, sizeof(g_smsTextStr) - 1);
        msTextStr_dump();

        mask = 0xFD; //
        rtn = tdxml_convertXmlToSms(mask, (uint8_t*)xml_buffer, strlen(xml_buffer), g_smsTextStr, sizeof(g_smsTextStr) - 1);
        msTextStr_dump();

        mask = 0xFB; //
        rtn = tdxml_convertXmlToSms(mask, (uint8_t*)xml_buffer, strlen(xml_buffer), g_smsTextStr, sizeof(g_smsTextStr) - 1);
        msTextStr_dump();

        mask = 0xF7; //
        rtn = tdxml_convertXmlToSms(mask, (uint8_t*)xml_buffer, strlen(xml_buffer), g_smsTextStr, sizeof(g_smsTextStr) - 1);
        msTextStr_dump();

        mask = 0xEF; //
        rtn = tdxml_convertXmlToSms(mask, (uint8_t*)xml_buffer, strlen(xml_buffer), g_smsTextStr, sizeof(g_smsTextStr) - 1);
        msTextStr_dump();
*/
        mask = 0x00;
      //rtn =
// TODO       tdxml_convertXmlToSms(mask, (uint8_t*)xml_buffer, strlen(xml_buffer), g_smsTextStr, sizeof(g_smsTextStr) - 1);
        msTextStr_dump();
    }
}

void TEST_XML_fleece(void);
void TEST_XML_fleece(void)
{

    //----- Fleece -----
    {
        size_t i;
        size_t len;

        len = strlen(xml_buffer);

        Printf("====> XML File size = %ld  (Before Fleece)\r\n", len);

        int fleece_XML(uint8_t *xml_buf, size_t xml_len);
//TODO        len = (size_t)fleece_XML((uint8_t*)xml_buffer, len);

        Printf("====> XML File size = %ld  (After  Fleece)\r\n", len);

        len = strlen(xml_buffer);
        Printf("====> XML File size = %ld\r\n\r\n", strlen(xml_buffer));
        for(i = 0; i < len ; i++) {
            Printf("%c", xml_buffer[i]);
        }

        Printf("\r\n");
    }


}

#if USE_CONFIG_WARNSMS_BUILD_ONCE
void smsPoke(char *text);
void smsPoke(char *text)
{
    PrintDbg(TD_INFO, "smsPoke -> '%s'\r\n", text);
}
#endif


uint32_t process_debug_str_X(char DbgCmd[]) // Xml - SMS
{
    uint32_t status;

    bool param_also_base_errors = true;
    bool output_lang_EN = true;
    bool output_lang_JP = true;


    switch (DbgCmd[1])
    {
        case '0':
            output_lang_EN = true;
            output_lang_JP = true;
            param_also_base_errors = false;
            break;
        case '1':
            output_lang_EN = true;
            output_lang_JP = true;
            param_also_base_errors = true;
            break;
        case 'j':
            output_lang_EN = false;
            output_lang_JP = true;
            param_also_base_errors = false;
            break;
        case 'J':
            output_lang_EN = false;
            output_lang_JP = true;
            param_also_base_errors = true;
            break;
        case 'e':
            output_lang_EN = true;
            output_lang_JP = false;
            param_also_base_errors = false;
            break;
        case 'E':
            output_lang_EN = true;
            output_lang_JP = false;
            param_also_base_errors = true;
            break;
    }


    status = 0;
    switch (DbgCmd[1])
    {
        case '0':
        case '1':
        case 'e':
        case 'E':
        case 'j':
        case 'J':
        {
            // 1. Generate XML Warning Data
            TEST_MAKE_XML_1(param_also_base_errors);

            int LangFlag_Backup;
            LangFlag_Backup = LangFlag;

            // 2a. Parse XML -> SMS Text (LANG=JP)
            if(output_lang_JP){
            LangFlag = LANG_TABLE_JP;
            TEST_XML_parse();
            }

            // 2a. Parse XML -> SMS Text (LANG=EN)
            if(output_lang_EN) {
            LangFlag = LANG_TABLE_US;
            TEST_XML_parse();
            }

            LangFlag = LangFlag_Backup;

            // 3. Remove Extra Tag(s) from XML -> OK for WS
            TEST_XML_fleece();
        }
            break;


        case 'p': // print out
            msTextStr_dump(); // g_smsTextStr
            break;

        case 's': // send sms
            {
#define STR_PHONE_RED_IPHONE        "+817028078916"
#define STR_PHONE_KAREL_I           "+818061432492"
#define STR_PHONE_KAREL_L           "08061432492"

                /* TODO
                char *phoneNumberStr = STR_PHONE_RED_IPHONE;
                uint32_t smsTextLength = (uint32_t)strlen(g_smsTextStr);
                T2ERR_t T2Err;
                T2Err = SMS.Send(phoneNumberStr, g_smsTextStr, smsTextLength);
                if(T2Err == ERR( GSM, NOERROR))
                {

                }
                TODO */
            }
            break;

    }

    return status;
}

#endif // CONFIG_USE_DEBUG_THREAD

/*
 *
XML:Start
====> XML File size = 1031

<?xml version="1.0" encoding="UTF-8"?>
<file format="warning" version="1.00" name="" type="2" author="RTR500BM_5FA30006">
<header>
<created_unixtime>1625183160</created_unixtime>
<time_diff>600</time_diff>
<std_bias>0</std_bias>
<dst_bias>0</dst_bias>
<base_unit>
<serial>5FA30006</serial>
<model>RTR500BM</model>
<name>RTR500BM_5FA30006</name>
<warning>1</warning>
</base_unit>
</header>
<device>
<serial>52820397</serial>
<model>RTR502B</model>
<name>Test_502</name>
<group>Group1</group>
<battery>
<state>1</state>
<event_unixtime>1625219160</event_unixtime>
<level>1</level>
</battery>
</device>
<device>
<serial>52820397</serial>
<model>RTR502B</model>
<name>Test_502</name>
<group>Group1</group>
<battery>
<state>2</state>
<event_unixtime>1625219160</event_unixtime>
<level>1</level>
</battery>
</device>
<device>
<serial>52820397</serial>
<model>RTR502B</model>
<name>Test_502</name>
<group>Group1</group>
<battemy>
<state>10</state>
<event_unixtime>1625219160</event_unixtime>
<level>1</level>
</battery>
</device>
</file>
 *
 */

/* "X0" result
 *
 *
 *
 * [S][S][S][S][S][S][S][S]Got T2



XML:Start
------------------------------------------------------------
{1}RTR500BM_5FA30006
{2}[親機:RTR500BM_5FA30006]

RTR500BM_5FA30006
電池少 2022-01-17 16:36
電池少　復帰
電池少　復帰
停電 2022-01-17 16:36
停電　復帰
停電　復帰
接点入力 ON 2022-01-17 16:36
接点入力 OFF
接点入力 OFF

Group1 RTR502_BATT
電池少r2022-01-17 16:36
電池少　復帰
電池少　復帰

Group1 RTR502_HI
Over 35.7 C 2021-07-02 08:15
正常復帰
正常復帰

Group1 RTR502_LO
Under -42.0 C 2021-07-02 08:15
正常復帰
正常復帰

Group1 RTR502_EDGE
Rising Ed後 2021-07-02 08:27
Falling Edge 2021-07-02 08:27
正常復帰

Group1 RTR502_SEN
Sensor Error 2021-07-02 08:27
Underflow Underflow 2021-07-02 08:27
Overflow Overflow 2021-07-02 08:27
正常復帰

Group1 RTR502_RF
無線失敗 2021-07-02 08:27
無線失敗　復帰
------------------------------------------------------------------------


------------------------------------------------------------------------
{1}RTR500BM_5FA30006
{2}[Base Unit:RTR500BM_5FA30006]

RTR500BM_5FA30006
Low Batt 2022-01-17 16:36
Low Batt: Recovered
Low Batt: Recovered
Ext Pwr Lost 2022-01-17 16:36
Ext Pwr Lost: Recovered
Ext Pwr Lost: Recovered
Input ON 2022-01-17 16:36
Input OFF
Input OFF

Group1 RTR502_BATT
Low Batt 2022-01-17 16:36
Low Batt: Recovered
Low Batt: Recovered

Group1 RTR502_HI
Above Limit  35.7 C 2021-07-02 08:15
Recovered
Recovered

Group1 RTR502_LO
Below Limit  -42.0 C 2021-07-02 08:15
Recovered
Recovered

Group1 RTR502_EDGE
Rising Edge 2021-07-02 08:27
Falling Edge 2021-07-02 08:27
Recovered

Group1 RTR502_SEN
Sensor Error 2021-07-02 08:27
Underflow Underflow 2021-07-02 08:27
Overflow Overflow 2021-07-02 08:27
Recovered

Group1 RTR502_RF
Comm Err 2021-07-02 08:27
Comm Err: Recovered

------------------------------------------------------------------------

====> XML File size = 6022  (Before Fleece)
Remove '<tbr_active_tag>Active_Battery</tbr_active_tag>
'
moveCount = 5370
removeCount = 48
Remove '<tbr_active_tag>Active_Battery</tbr_active_tag>
'
moveCount = <223
removeCount = 48
Remove '<tbr_active_tag>Active_Battery</tbr_active_tag>
'
moveCount = 5076
removeCount = 48
Remove '<tbr_active_tag>Active_Battery</tbr_active_tag>
'
moveCount = 4946
removeCount = 48
Remove '<tbr_active_tag>Active_Battery</tbr_active_tag>
'
moveCount = 4818
removeCount = 48
Remove '<tbr_active_tag>Active_Battery</tbr_active_tag>
'
moveCount = 4690
removeCount = 48
Remove '<tbr_active_tag>Active_Input</tbr_active_tag>
'
moveCount = 4564
removeCount = 46
Remove '<tbr_active_tag>Active_Input</tbr_active_tag>
'
moveCount = 4440
removeCount = 46
Remove '<tbr_active_tag>Active_Input</tbr_active_tag>
'
moveCount = 4316
removeCount = 46
Remove '<tbr_active_tag>Active_Battery</tbr_active_tag>
'
moveCount = 4055
removeCount = 48
Remove '<tbr_active_tag>Active_Battery</tbr_active_tag>
'
moveCount = 3908
removeCount = 48
Remove '<tbr_active_tag>Active_Battery</tbr_active_tag>
'
moveCount = 3761
removeCount = 48
Remove '<tbr_active_tag>Active_UpLo</tbr_active_tag>
'
moveCount = 3356
removeCount = 45
Remove '<tbr_active_tag>Active_UpLo</tbr_active_tag>
'
moveCount = 3084
removeCount = 45
Remove '<tbr_active_tag>Active_UpLo</tbr_active_tag>
'
moveCount = 2798
removeCount = 45
Remove '<tbr_active_tag>Active_UpLo</tbr_active_tag>
'
moveCount = 2396
removeCount = 45
Remove '<tbr_active_tag>Active_UpLo</tbr_active_tag>
'
moveCount = 2123
removeCount = 45
Remove '<tbr_active_tag>Active_UpLo</tbr_active_tag>
'
moveCount = 1835
removeCount = 45
Remove '<tbr_active_tag>Active_UpLo</tbr_active_tag>
'
moveCount = 1548
removeCount = 45
Remove '<tbr_active_tag>Active_UpLo</tbr_active_tag>
'
moveCount = 1375
removeCount = 45
Remove '<tbr_active_tag>Active_UpLo</tbr_active_tag>
'
moveCount = 1208
removeCount = 45
Remove '<tbr_active_tag>Active_Sensor</tbr_active_tag>
'
moveCount = 919
removeCount = 47
Remove '<tbr_active_tag>Active_Sensor</tbr_active_tag>
'
moveCount = 747
removeCount = 47
Remove '<tbr_active_tag>Active_Sensor</tbr_active_tag>
'
moveConnt = 576
removeCount = 47
Remove '<tbr_active_tag>Active_Sensor</tbr_active_tag>
'
moveCount = 407
removeCount = 47
Remove '<tbr_active_tag>Active_RF</tbr_active_tag>
'
moveCount = 161
removeCount = 43
Remove '<tbr_active_tag>Active_RF</tbr_active_tag>
'
moveCount = 27
removeCount = 43
tagCount = 27
====> XML File size = 4773  (After  Fleece)
====> XML File size = 4773

<?xml version="1.0" encoding="UTF-8"?>
<file format="warning" version="1.00" name="RTR500BM_5FA30006_20220117-161007.xml" type="2" author="RTR500BM_5FA30006">
<header>
<created_unixtime>1642404964</created_unixtime>
<time_diff>540<etime_diff>
<std_bias>0</std_bias>
<dst_bias>0</dst_bias>
<base_unit>
<serial>5FA30006</serial>
<model>RTR500BM</model>
<name>RTR500BM_5FA30006</name>
<warning>1</warning>
</base_unit>
</header>
<device>
<serial>5FA30006</serial>
<model>RTR500BM</model>
<name>RTR500BM_5FA30006</name>
<battery>
<state>10</state>
<event_unixtime>1642437364</event_unixtime>
<level>1</level>
</battery>
<battery>
<state>1</state>
<event_unixtime>1642437364</event_unixtime>
<level>1</level>
</battery>
<battery>
<state>2</state>
<event_unixtime>1642437364</event_unixtime>
<lexel>1</level>
</battery>
<ext_ps>
<state>10</state>
<event_unixtime>1642437364</event_unixtime>
</ext_ps>
<ext_ps>
<state>1</state>
<event_unixtime>1642437364</event_unixtime>
</ext_ps>
<ext_ps>
<state>2</state>
<event_unixtime>1642437364</event_unixtime>
</ext_ps>
<input>
<state>10</state>
<event_unixtime>1642437364</event_unixtime>
</input>
<input>
<state>1</state>
<event_unixtime>1642437364</event_unixtime>
</input>
<input>
<state>2</state>
<event_unixtime>1642437364</event_unixtime>
</in2ut>
</device>
<device>
<serial>52820397</serial>
<model>RTR502B</model>
<name>RTR502_BATT</name>
<group>Group1</group>
<battery>
<state>10</state>
<event_unixtime>1642437364</event_unixtime>
<level>1</level>
</battery>
<battery>
<state>1</state>
<event_unixtime>1642437364</event_unixtime>
<level>1</level>
</battery>
<battery>
<state>2</state>
<event_unixtime>1642437364</event_unixtime>
<level>1</level>
</battery>
</device>
<device>
<serial>52820397</serial>
<model>RTR502B</model>
<name>RTR502_HI</name>
<group>Group1</group>
<ch>
<num>1</num>
<name></name>
<state>11</state>
<event_unixtime>1625213726</event_unixtime>
<id>6</id>
<value>35.7</value>
<unit>C</unit>
<upper_limit>30.0</upper_limit>
<lower_limit></lower_limit>
<judgement_tise>30</judgement_time>
</ch>
<ch>
<num>1</num>
<name></name>
<state>1</state>
<event_unixtime></event_unixtime>
<id>6</id>
<value></value>
<unit>C</unit>
<upper_limit>30.0</upper_limit>
>lower_limit></lower_limit>
<judgement_time>30</judgement_time>
</ch>
<ch>
<num>1</num>
<name></name>
<state>2</state>
<event_unixtime>1625214456</event_unixtime>
<id>7</id>
<value>35.7</value>
<unit>C</unit>
<upper_limit>30.0</upper_limit>
<lower_limit></lower_limit>
<judgement_time>30</judgement_time>
</ch>
</device>
<device>
<serial>52820397</serial>
<model>RTR502B</model>
<name>RTR502_LO</name>
<group>Group1</group>
<ch>
<num>2</num>
<name></name>
<state>12</state>
<event_unixtime>1625213726</event_unixtime>
<id>6</id>
<value>-42.0</value>
<unit>C</unit>
<upper_limit></upper_limit>
<lower_limit>-30.0</lower_limit>
<judgement_time>30</judgement_time>
</ch>
<ch>
<num>2</num>
<name></name>
<state>1</state>
<event_unixtime></event_unixtime>
<id>6</id>
<value></value>
<unit>C</unit>
<upper_limit></upper_limit>
<lower_limit>-30.0</lower_limit>
<judgement_time>30</judgement_time>
</ch>
<ch>
<num>2</num>
<name></name>
<state>2</state>
<event_unixtime>1625214456</event_unixtime>
<id>7</id>
<value>-42.0</value>
<unit>C</unit>
<upper_limit></upper_limit>
<lower_limit>-30.0</lower_limit>
<judgement_time>30</judgement_time>
</ch>
</device>
<device>
<serial>52820397</serial>
<model>RTR502B</model>
<name>RTR502_EDGE</name>
<group>Group1</group>
<ch>
<num>1</num>
<name></name>
<state>14</state>
<event_unixtime>1625214456</event_unixtime>
<value>Rising Edge</value>
</ch>
<ch>
<num>1</num>
<name></name>
<state>15</state>
<event_unixtime>1625214456</event_unixtime>
<value>Falling Edge</value>
</ch>
<ch>
<num>1</num>
<name></name>
<state>1</state>
<event_unixtime>1625214456</event_unixtime>
<value>No >ata</value>
</ch>
</device>
<device>
<serial>52820397</serial>
<model>RTR502B</model>
<name>RTR502_SEN</name>
<group>Group1</group>
<ch>
<num>1</num>
<name></name>
<state>13</state>
<event_unixtime>1625214456</event_unixtime>
<value>Sensor Error</value>
</ch>
<ch>
<num>1</num>
<name></name>
<state>17</state>
<event_unixtime>1625214456</event_unixtime>
<value>Underflow</value>
</ch>
<ch>
<num>1</num>
<name></name>
<state>16</state>
<event_unixtime>1625214456</event_unixtime>
<value>Overflow</value>
</ch>
<ch>
<num>1</num>
<name></name>
<state>1</state>
<event_unixtime>1625214456</event_unixtime>
<value>No Data</value>
</ch>
</device>
<device>
<serial>52820397</serial>
<model>RTR502B</model>
<name>RTR502_RF</name>
<group>Group1</group>
<comm>
<state>10</state>
<event_unixtime>1625214456</event_unixtime>
<type>0</type>
</comm>
<comm>
<state>1</state>
<event_unixtime>1625214456</event_unixtime>
<type>0</type>
</comm>
</device>
</file>

*** debugTask: STOP

[debug] sleep flags = 0x000FFF88 [    |    |    | UBL| pkb]
[S][S][S][S][S][S][S][S][S][S][S][S][S][S][S][S][S][S][S][S][S][S][S][S][S][S][S]
 *
 *
 */


