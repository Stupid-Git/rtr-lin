/*
 * comp_http_Monitor.c
 *
 *  Created on: Dec 10, 2024
 *      Author: karel
 */

#include "_r500_config.h"

#define _COMP_HTTP_COMMON_C_
#include "comp_http_common.h"

#include "r500_defs.h"



char sH[1024];
char sB[8192];

static const char *HD_Content_Type          = "Content-Type";
//static const char *HD_Content_Type_XML      = "application/xml; charset=UTF-8;";          // sakaguchi 2020.10.19
//static const char *HD_Content_Type_T2       = "application/octet-stream; charset=UTF-8;";
//static const char *HD_Content_Type_LOG      = "application/octet-stream; charset=UTF-8;";
static const char *HD_Content_Type_XML      = "application/xml; charset=UTF-8";
static const char *HD_Content_Type_T2       = "application/octet-stream; charset=UTF-8";
static const char *HD_Content_Type_LOG      = "application/octet-stream; charset=UTF-8";

static const char *HD_User_Agent            = "User-Agent";
// 2023.05.31 ↓
#define HD_User_Agent_D UNIT_BASE_TANDD "/" VERSION_FW
#define HD_User_Agent_H UNIT_BASE_HITACHI "/" VERSION_FW
#define HD_User_Agent_E UNIT_BASE_ESPEC "/" VERSION_FW
// 2023.05.31 ↑

//static const char *HD_Content_Length        = "Content-Length: ";
static const char *HD_Tandd_Api             = "X-Tandd-API-Version";
static const char *HD_Tandd_Api_D           = "1.0";

static const char *HD_Tandd_Serial          = "X-Tandd-BaseUnit-Serial";
static const char *HD_Tandd_Request_Type    = "X-Tandd-Request-Type";
static const char *HD_Tandd_Request_ID      = "X-Tandd-Request-Id";
static const char *HD_Tandd_Next_Command    = "X-Tandd-Next-Command";
static const char *HD_Tandd_Setting_SysCnt  = "X-Tandd-Setting-SysCnt";
static const char *HD_Tandd_Setting_RegCnt  = "X-Tandd-Setting-RegCnt";

static const char *HD_Tandd_Request_ID_lc      = "x-tandd-request-id";      // 2022.11.28
static const char *HD_Tandd_Next_Command_lc    = "x-tandd-next-command";    // 2022.11.28

static const char *HD_Type_RecordData       = "postRecordedData";
static const char *HD_Type_MonitorData      = "postMonitoringData";
static const char *HD_Type_Setting          = "postSettings";
static const char *HD_Type_Log              = "postLogs";
static const char *HD_Type_Warnig           = "postWarnings";
static const char *HD_Type_Operation_Request = "getOperationRequest";
static const char *HD_Type_Operation_Result = "postOperationResult";
static const char *HD_Type_PostState        = "postState";



typedef struct st_srv_info
{
    char                    *host;              // e.g. "api.webstorage.jp"  "os.mbed.com"
    char                    *resource;          // e.g. "https://api.webstorage.jp/xyz.php"

//    NXD_ADDRESS             server_ip;          // ip address of host
//    UINT                    server_port;

    char                    *username;          // ""
    char                    *password;          // ""

//    NX_WEB_HTTP_CLIENT      *p_secure_client;   //< HTTP client structure


} srv_info_t;

srv_info_t* p_info;
srv_info_t g_whc_info;

int my_http_setup()
{
    int rtn = 0;


    g_whc_info.host = "api.webstorage.jp";
    g_whc_info.resource = "/api/rtr500.php";

    //"http://develop.webstorage.jp/api/rtr500/device/"
    //http://develop.webstorage.jp/api/rtr500/device/index.php
    g_whc_info.host = "develop.webstorage.jp";
    g_whc_info.resource = "/api/rtr500/device/index.php";

    p_info = &g_whc_info;
    return rtn;
}

void Recorded_make_sB_text()
{
    sB[0] = '\0';
#if 1
    strcat(sB, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
//  strcat(sB, "<file format=\"recorded_data\" version=\"1.26\" name=\"RTR500BM_5FA30006_RTR502B_OLD_20241210-170006.trz\" author=\"RTR500BM_5FA30006\">\n");
    strcat(sB, "<file format=\"recorded_data\" version=\"1.26\" name=\"RTR500BM_5FA30005_Unit01_20241210-170006.trz\" author=\"RTR500BM_5FA30005\">\n");
    strcat(sB, "<base>\n");
    strcat(sB, "<serial>5FA30005</serial>\n");
    strcat(sB, "<model>RTR500BM</model>\n");
    strcat(sB, "<name>RTR500BM_5FA30005</name>\n");
    strcat(sB, "</base>\n");
    strcat(sB, "<ch>\n");
    strcat(sB, "<serial>52820396</serial><model>RTR502B</model>\n");
    strcat(sB, "<name>Unit01</name>\n");
    strcat(sB, "<num>1</num>\n");
    strcat(sB, "<time_diff>540</time_diff>\n");
    strcat(sB, "<std_bias>0</std_bias>\n");
    strcat(sB, "<dst_bias>0</dst_bias>\n");
    strcat(sB, "<time_zone>(UTC+09:00) 大阪、札幌、東京</time_zone>\n");
    strcat(sB, "<type>13</type>\n");
    strcat(sB, "<unix_time>1733815802</unix_time>\n");
    strcat(sB, "<data_id>38798</data_id>\n");
    strcat(sB, "<interval>10</interval>\n");
    strcat(sB, "<count>180</count>\n");
    strcat(sB, "<data>\n");
    strcat(sB, "4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOIE4gTiBOEE4QThBOEE\n");
    strcat(sB, "4QThBOEE4QThBOEE4QThBOEE4QThBOEE4QThBOEE4QThBOEE4QThBOEE4QThBOEE4QThBOEE4QThBOEE4QThBOEE4QThBOEE4QThBOEE4QThBOEE4QThBOEE4QThBOEE4QThBOEE4QThBOEE4QThBOEE4QThBOEE\n");
    strcat(sB, "</data>\n");
    strcat(sB, "<upper_limit></upper_limit>\n");
    strcat(sB, "<lower_limit></lower_limit>\n");
    strcat(sB, "<scale_expr></scale_expr>\n");
    strcat(sB, "<record_prop>\n");
    strcat(sB, "CgCsCRZnAAAAVQEBCwAAAAAAAABg8PgqAAAAAAAAaAEAfY6XEQCCXwAADQARAIJfngkWZwAAAAAAAAAAAQBcAQ==\n");
    strcat(sB, "</record_prop>\n");
    strcat(sB, "</ch>\n");
    strcat(sB, "</file>\n");
#endif
}


void Warning_make_sB_text(void)
{
    sB[0] = '\0';

//  strcat(sB, "<file format=\"current_readings\" version=\"1.26\" name=\"RTR500BM_5FA30005_20241209-133811.xml\" author=\"RTR500BM_5FA30005\">\n");
//  strcat(sB, "<file format=\"current_readings\" version=\"1.26\" name=\"RTR500BM_5FA30005_20241210-134811.xml\" author=\"RTR500BM_5FA30005\">\n");
#if 1
    strcat(sB, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    strcat(sB, "<file format=\"warning\" version=\"1.00\" name=\"RTR500BM_5FA30005-20241210-160000.xml\" type=\"1\" author=\"RTR500BM_5FA30005\">\n");

    strcat(sB, "<header>\n");
    strcat(sB, "<created_unixtime>1733815852</created_unixtime>\n");
    strcat(sB, "<time_diff>0</time_diff>\n");
    strcat(sB, "<std_bias>0</std_bias>\n");
    strcat(sB, "<dst_bias>0</dst_bias>\n");
    strcat(sB, "<base_unit>\n");
    strcat(sB, "<serial>5FA30005</serial>\n"); ///
    strcat(sB, "<model>RTR500BM</model>\n");///
    strcat(sB, "<name>RTR500BM_5FA30005</name>\n");/// RTR500BM_5FA30005
    strcat(sB, "<warning>1</warning>\n");
    strcat(sB, "</base_unit>\n");
    strcat(sB, "</header>\n");

#if 1
    strcat(sB, "<device>\n");
    strcat(sB, "<serial>52820396</serial>\n");
    strcat(sB, "<model>RTR502B</model>\n");
    strcat(sB, "<name>Unit01</name>\n");
    strcat(sB, "<group>Group_2</group>\n");

    strcat(sB, "<ch>\n");
    strcat(sB, "<num>1</num>\n");
    strcat(sB, "<name></name>\n");
    strcat(sB, "<state>11</state>\n");
    strcat(sB, "<event_unixtime>1733815852</event_unixtime>\n"); ///
    strcat(sB, "<id>6</id>\n");
    strcat(sB, "<value>35.7</value>\n");
    strcat(sB, "<unit>C</unit>\n");
    strcat(sB, "<upper_limit>30.0</upper_limit>\n");
    strcat(sB, "<lower_limit></lower_limit>\n");
    strcat(sB, "<judgement_time>30</judgement_time>\n");
    strcat(sB, "</ch>\n");
/*
    strcat(sB, "<ch>\n");
    strcat(sB, "<num>1</num>\n");
    strcat(sB, "<name></name>\n");
    strcat(sB, "<state>1</state>\n");
    strcat(sB, "<event_unixtime></event_unixtime>\n");
    strcat(sB, "<id>6</id>\n");
    strcat(sB, "<value></value>\n");
    strcat(sB, "<unit>C</unit>\n");
    strcat(sB, "<upper_limit>30.0</upper_limit>\n");
    strcat(sB, "<lower_limit></lower_limit>\n");
    strcat(sB, "<judgement_time>30</judgement_time>\n");
    strcat(sB, "</ch>\n");

    strcat(sB, "<ch>\n");
    strcat(sB, "<num>1</num>\n");
    strcat(sB, "<name></name>\n");
    strcat(sB, "<state>2</state>\n");
    strcat(sB, "<event_unixtime>1733815852</event_unixtime>\n");
    strcat(sB, "<id>7</id>\n");
    strcat(sB, "<value>35.7</value>\n");
    strcat(sB, "<unit>C</unit>\n");
    strcat(sB, "<upper_limit>30.0</upper_limit>\n");
    strcat(sB, "<lower_limit></lower_limit>\n");
    strcat(sB, "<judgement_time>30</judgement_time>\n");
    strcat(sB, "</ch>\n");
*/
    strcat(sB, "</device>\n");
#endif
    strcat(sB, "</file>\n");

#endif
}
void Monitor_make_sB_text(void)
{
    sB[0] = '\0';
    strcat(sB, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
//  strcat(sB, "<file format=\"current_readings\" version=\"1.26\" name=\"RTR500BM_5FA30005_20241209-133811.xml\" author=\"RTR500BM_5FA30005\">\n");
    strcat(sB, "<file format=\"current_readings\" version=\"1.26\" name=\"RTR500BM_5FA30005_20241210-134811.xml\" author=\"RTR500BM_5FA30005\">\n");
    strcat(sB, "<base>\n");
    strcat(sB, "<serial>5FA30005</serial>\n");
    strcat(sB, "<model>RTR500BM</model>\n");
    strcat(sB, "<name>RTR500BM_5FA30005</name>\n");
    strcat(sB, "<time_diff>540</time_diff>\n");
    strcat(sB, "<std_bias>0</std_bias>\n");
    strcat(sB, "<dst_bias>0</dst_bias>\n");
    strcat(sB, "<time_zone>(UTC+09:00) 大阪、札幌、東京</time_zone>\n");
    strcat(sB, "<gsm>\n");
    strcat(sB, "<ext_ps>1</ext_ps>\n");
    strcat(sB, "<batt>0</batt>\n");
    strcat(sB, "<input>0</input>\n");
    strcat(sB, "<output>1</output>\n");
    strcat(sB, "<gps type=\"dmm\">\n");
    strcat(sB, "<lat></lat>\n");
    strcat(sB, "<lon></lon>\n");
    strcat(sB, "<unix_time></unix_time>\n");
    strcat(sB, "</gps>\n");
    strcat(sB, "<gps type=\"deg\">\n");
    strcat(sB, "<lat></lat>\n");
    strcat(sB, "<lon></lon>\n");
    strcat(sB, "<unix_time></unix_time>\n");
    strcat(sB, "</gps>\n");
    strcat(sB, "</gsm>\n");
    strcat(sB, "</base>\n");
    strcat(sB, "<group>\n");
    strcat(sB, "<num>1</num>\n");
    strcat(sB, "<name>Group_2</name>\n");
    strcat(sB, "<remote>\n");
    strcat(sB, "<serial>52820396</serial>\n");
    strcat(sB, "<model>RTR502B</model>\n");
    strcat(sB, "<num>1</num>\n");
    strcat(sB, "<name>Unit01</name>\n");
    strcat(sB, "<rssi repeater=\"0\">5</rssi>\n");
    strcat(sB, "<ch>\n");
    strcat(sB, "<num>1</num>\n");
    strcat(sB, "<scale_expr></scale_expr>\n");
    strcat(sB, "<name></name>\n");
    strcat(sB, "<current state=\"0\">\n");
//  strcat(sB, "<unix_time>1733719062</unix_time>\n");
    strcat(sB, "<unix_time>1733806061</unix_time>\n");
//  strcat(sB, "<time_str>2024-12-09 13:37:42</time_str>\n");
    strcat(sB, "<time_str>2024-12-10 13:47:41</time_str>\n");
    strcat(sB, "<value valid=\"true\">23.7</value>\n");
    strcat(sB, "<unit>C</unit>\n");
    strcat(sB, "<batt>5</batt>\n");
    strcat(sB, "</current>\n");
    strcat(sB, "<record>\n");
    strcat(sB, "<type>13</type>\n");
//  strcat(sB, "<unix_time>1733718792</unix_time>\n");
    strcat(sB, "<unix_time>1733805791</unix_time>\n");
//  strcat(sB, "<data_id>9649</data_id>\n");
    strcat(sB, "<data_id>12549</data_id>\n");
    strcat(sB, "<interval>30</interval>\n");
    strcat(sB, "<count>10</count>\n");
    strcat(sB, "<data>\n");
//  strcat(sB, "1ATUBNQE1QTVBNUE1QTVBNUE1QQ=\n");
    strcat(sB, "2ATYBNgE2ATYBNgE2ATYBNgE2AQ=\n");
    strcat(sB, "</data>\n");
    strcat(sB, "</record>\n");
    strcat(sB, "</ch>\n");
    strcat(sB, "</remote>\n");
    strcat(sB, "</group>\n");
    strcat(sB, "</file>\n");
}


int Settings_make_sH(void)
{
    int rtn = 0;

    char *pH;
    int len;
    int available;
    available = sizeof(sH);
    pH = sH;
/*
POST /api/rtr500.php HTTP/1.1
Host: 192.168.1.100:80
Content-Type: application/octet-stream; charset=UTF-8
Content-Length: 3680
User-Agent: RTR500BW/R.01.00.06
X-Tandd-API-Version: 1.0
X-Tandd-BaseUnit-Serial: 5EA30001
X-Tandd-Request-Type: postSettings
X-Tandd-Settings-SysCnt: 1
X-Tandd-Settings-RegCnt: 1
（リクエストボディ部）
*/
    char temp[128];
    //sprintf( temp, "%04lX", fact_config.SerialNumber);
    strcpy(temp, "5FA30005");
    //int templen = strlen(temp);

    int xsize;
    xsize = strlen(xml_buffer);

    //POST /api/rtr500.php HTTP/1.1
    len = snprintf(pH, available, "POST %s HTTP/1.1\r\n", p_info->resource);
    pH += len; available -= len; if(available <= 0) return -1;
    //Host: 192.168.1.100:80
    len = snprintf(pH, available, "Host: %s\r\n", p_info->host);
    pH += len; available -= len; if(available <= 0) return -1;

    //Content-Type: application/octet-stream; charset=UTF-8
    len = snprintf(pH, available, "%s: %s\r\n", HD_Content_Type, HD_Content_Type_T2);          //****************
    pH += len; available -= len; if(available <= 0) return -1;
    //Content-Length: 3680
    len = snprintf(pH, available, "Content-Length: %d\r\n", xsize);
    pH += len; available -= len; if(available <= 0) return -1;

    //User-Agent: RTR500BW/R.01.00.06
    len = snprintf(pH, available, "%s: %s\r\n", HD_User_Agent, "RTR500BM/R.01.00.06");          //****************
    pH += len; available -= len; if(available <= 0) return -1;

    /*
    //User-Agent: RTR500BW/R.01.00.06
    if(fact_config.Vender == VENDER_ESPEC){
        len = snprintf(pH, available, "%s: %s\r\n", HD_User_Agent, HD_User_Agent_E);
        pH += len; available -= len; if(available <= 0) return -1;
    }else{
        len = snprintf(pH, available, "%s: %s\r\n", HD_User_Agent, HD_User_Agent_D);
        pH += len; available -= len; if(available <= 0) return -1;
    }
*/
    //X-Tandd-API-Version: 1.0
    len = snprintf(pH, available, "%s: %s\r\n", HD_Tandd_Api, HD_Tandd_Api_D);                  //****************
    pH += len; available -= len; if(available <= 0) return -1;
    //X-Tandd-BaseUnit-Serial: 5EA30001
    len = snprintf(pH, available, "%s: %s\r\n", HD_Tandd_Serial, temp);                         //****************
    pH += len; available -= len; if(available <= 0) return -1;

    //X-Tandd-Request-Type: postSettings
    len = snprintf(pH, available, "%s: %s\r\n", HD_Tandd_Request_Type, HD_Type_Setting);        //****************
    pH += len; available -= len; if(available <= 0) return -1;

    /* TODO
    //----- request id ------
    if(0 != G_HttpFile[ifile].reqid){                       // [File]リクエストID取得
        sprintf( temp, "%lu", G_HttpFile[ifile].reqid );
        len = strlen(temp);
        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Request_ID,  strlen(HD_Tandd_Request_ID), temp, len, wait_option);
    } TODO */

    //----- setthing count -----
    sprintf(temp, "%u", my_config.device.SysCnt);
    //templen = strlen(temp);
    //status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Setting_SysCnt, strlen(HD_Tandd_Setting_SysCnt), temp, len, wait_option);
    len = snprintf(pH, available, "%s: %s\r\n", HD_Tandd_Setting_SysCnt, temp);                 //****************
    pH += len; available -= len; if(available <= 0) return -1;

    sprintf( temp, "%u", my_config.device.RegCnt);
    //templen = strlen(temp);
    //status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Setting_RegCnt, strlen(HD_Tandd_Setting_RegCnt), temp, len, wait_option);
    len = snprintf(pH, available, "%s: %s\r\n", HD_Tandd_Setting_RegCnt, temp);                 //****************
    pH += len; available -= len; if(available <= 0) return -1;


    len = snprintf(pH, available, "\r\n");
    pH += len; available -= len; if(available <= 0) return -1;
    // NOTE:: If we add this extra "\r\n" we get a 400 bad request
    //len = snprintf(pH, available, "\r\n");
    //pH += len; available -= len; if(available <= 0) return -1;

    Printf("Header:\n%s", sH);
/*
[response_headers]
HTTP/1.1 200 OK
Server: nginx
Date: Mon, 09 Dec 2024 07:16:44 GMT
Content-Type: application/octet-stream
Content-Length: 0
Connection: keep-alive
X-Content-Type-Options: nosniff
X-Tandd-Next-Command: postSettings
*/
    return rtn;
}


int Recorded_make_sH(void)
{
    int rtn = 0;

    char *pH;
    int len;
    int available;
    available = sizeof(sH);
    pH = sH;
/*
POST /api/rtr500.php HTTP/1.1
Host: 192.168.1.100:80
Content-Type: application/xml; charset=UTF-8
Content-Length: 3680
User-Agent: RTR500BW/R.01.00.06
X-Tandd-API-Version: 1.0
X-Tandd-BaseUnit-Serial: 5EA30001
X-Tandd-Request-Type: postRecordedData
<?xml version="1.0" encoding="UTF-8"?>
<file format="recorded_data" version="1.26" name="ティアンドデイ社屋_2F室外E_20191203-060947.trz"
…
</file>
*/
    char temp[128];
    //sprintf( temp, "%04lX", fact_config.SerialNumber);
    strcpy(temp, "5FA30005");
    //int templen = strlen(temp);

    int xsize;
    xsize = strlen(xml_buffer);

    //POST /api/rtr500.php HTTP/1.1
    len = snprintf(pH, available, "POST %s HTTP/1.1\r\n", p_info->resource);
    pH += len; available -= len; if(available <= 0) return -1;
    //Host: 192.168.1.100:80
    len = snprintf(pH, available, "Host: %s\r\n", p_info->host);
    pH += len; available -= len; if(available <= 0) return -1;

    //Content-Type: application/xml; charset=UTF-8
    len = snprintf(pH, available, "%s: %s\r\n", HD_Content_Type, HD_Content_Type_XML);          //****************
    pH += len; available -= len; if(available <= 0) return -1;
    //Content-Length: 3680
    len = snprintf(pH, available, "Content-Length: %d\r\n", xsize);
    pH += len; available -= len; if(available <= 0) return -1;

    //User-Agent: RTR500BW/R.01.00.06
    len = snprintf(pH, available, "%s: %s\r\n", HD_User_Agent, "RTR500BM/R.01.00.06");          //****************
    pH += len; available -= len; if(available <= 0) return -1;

    /*
    //User-Agent: RTR500BW/R.01.00.06
    if(fact_config.Vender == VENDER_ESPEC){
        len = snprintf(pH, available, "%s: %s\r\n", HD_User_Agent, HD_User_Agent_E);
        pH += len; available -= len; if(available <= 0) return -1;
    }else{
        len = snprintf(pH, available, "%s: %s\r\n", HD_User_Agent, HD_User_Agent_D);
        pH += len; available -= len; if(available <= 0) return -1;
    }
*/
    //X-Tandd-API-Version: 1.0
    len = snprintf(pH, available, "%s: %s\r\n", HD_Tandd_Api, HD_Tandd_Api_D);                  //****************
    pH += len; available -= len; if(available <= 0) return -1;
    //X-Tandd-BaseUnit-Serial: 5EA30001
    len = snprintf(pH, available, "%s: %s\r\n", HD_Tandd_Serial, temp);                         //****************
    pH += len; available -= len; if(available <= 0) return -1;
    //X-Tandd-Request-Type: postMonitoringData
#if 0 //Monitor
    len = snprintf(pH, available, "%s: %s\r\n", HD_Tandd_Request_Type, HD_Type_MonitorData);
#endif
#if 0 //Warning
    len = snprintf(pH, available, "%s: %s\r\n", HD_Tandd_Request_Type, HD_Type_Warnig);
#endif
#if 1 //Recorded
    len = snprintf(pH, available, "%s: %s\r\n", HD_Tandd_Request_Type, HD_Type_RecordData);     //****************
#endif
    pH += len; available -= len; if(available <= 0) return -1;

#if 0 // Not wirh Recorded
    //----- setthing count -----
    sprintf(temp, "%u", my_config.device.SysCnt);
    //templen = strlen(temp);
    //status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Setting_SysCnt, strlen(HD_Tandd_Setting_SysCnt), temp, len, wait_option);
    len = snprintf(pH, available, "%s: %s\r\n", HD_Tandd_Setting_SysCnt, temp);                 //****************
    pH += len; available -= len; if(available <= 0) return -1;

    sprintf( temp, "%u", my_config.device.RegCnt);
    //templen = strlen(temp);
    //status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Setting_RegCnt, strlen(HD_Tandd_Setting_RegCnt), temp, len, wait_option);
    len = snprintf(pH, available, "%s: %s\r\n", HD_Tandd_Setting_RegCnt, temp);                 //****************
    pH += len; available -= len; if(available <= 0) return -1;
#endif

    len = snprintf(pH, available, "\r\n");
    pH += len; available -= len; if(available <= 0) return -1;
    // NOTE:: If we add this extra "\r\n" we get a 400 bad request
    //len = snprintf(pH, available, "\r\n");
    //pH += len; available -= len; if(available <= 0) return -1;

    Printf("Header:\n%s", sH);
/*
[response_headers]
HTTP/1.1 200 OK
Server: nginx
Date: Mon, 09 Dec 2024 07:16:44 GMT
Content-Type: application/octet-stream
Content-Length: 0
Connection: keep-alive
X-Content-Type-Options: nosniff
X-Tandd-Next-Command: postSettings
*/
    return rtn;
}

int Warning_make_sH(void)
{
    int rtn = 0;

    char *pH;
    int len;
    int available;
    available = sizeof(sH);
    pH = sH;
/*
POST /api/rtr500.php HTTP/1.1
Host: 192.168.1.100:80
Content-Type: application/xml; charset=UTF-8
Content-Length: 3680
User-Agent: RTR500BW/R.01.00.06
X-Tandd-API-Version: 1.0
X-Tandd-BaseUnit-Serial: 5EA30001
X-Tandd-Request-Type: postMonitoringData
<?xml version="1.0" encoding="UTF-8"?>
<file format="current_readings" version="1.26" name="ティアンドデイ社屋_20191206-085636.xml"
author="RTR500BW">
…
</file>

POST /api/rtr500.php HTTP/1.1
Host: 192.168.1.100:80
Content-Type: application/octet-stream; charset=UTF-8 ????? XML!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
Content-Length: 3680
User-Agent: RTR500BW/R.01.00.06
X-Tandd-API-Version: 1.0
X-Tandd-BaseUnit-Serial: 5EA30001
X-Tandd-Request-Type: postWarnings
X-Tandd-Settings-SysCnt: 1
X-Tandd-Settings-RegCnt: 1
<?xml version="1.0" encoding="UTF-8"?>.
<file format="warning" version="1.00" name="RTR500BW_5EA30001-20200630-183953.xml"
type="1" author= "RTR500BW_5EA30001"> ///(省略)///
*/
    char temp[128];
    //sprintf( temp, "%04lX", fact_config.SerialNumber);
    strcpy(temp, "5FA30005");
    //int templen = strlen(temp);

    int xsize;
    xsize = strlen(xml_buffer);

    //POST /api/rtr500.php HTTP/1.1
    len = snprintf(pH, available, "POST %s HTTP/1.1\r\n", p_info->resource);
    pH += len; available -= len; if(available <= 0) return -1;
    //Host: 192.168.1.100:80
    len = snprintf(pH, available, "Host: %s\r\n", p_info->host);
    pH += len; available -= len; if(available <= 0) return -1;

    //Content-Type: application/xml; charset=UTF-8
    len = snprintf(pH, available, "%s: %s\r\n", HD_Content_Type, HD_Content_Type_XML);          //****************
    pH += len; available -= len; if(available <= 0) return -1;
    //Content-Length: 3680
    len = snprintf(pH, available, "Content-Length: %d\r\n", xsize);
    pH += len; available -= len; if(available <= 0) return -1;

    //User-Agent: RTR500BW/R.01.00.06
    len = snprintf(pH, available, "%s: %s\r\n", HD_User_Agent, "RTR500BM/R.01.00.06");          //****************
    pH += len; available -= len; if(available <= 0) return -1;

    /*
    //User-Agent: RTR500BW/R.01.00.06
    if(fact_config.Vender == VENDER_ESPEC){
        len = snprintf(pH, available, "%s: %s\r\n", HD_User_Agent, HD_User_Agent_E);
        pH += len; available -= len; if(available <= 0) return -1;
    }else{
        len = snprintf(pH, available, "%s: %s\r\n", HD_User_Agent, HD_User_Agent_D);
        pH += len; available -= len; if(available <= 0) return -1;
    }
*/
    //X-Tandd-API-Version: 1.0
    len = snprintf(pH, available, "%s: %s\r\n", HD_Tandd_Api, HD_Tandd_Api_D);                  //****************
    pH += len; available -= len; if(available <= 0) return -1;
    //X-Tandd-BaseUnit-Serial: 5EA30001
    len = snprintf(pH, available, "%s: %s\r\n", HD_Tandd_Serial, temp);                         //****************
    pH += len; available -= len; if(available <= 0) return -1;
    //X-Tandd-Request-Type: postMonitoringData
#if 0 //Monitor
    len = snprintf(pH, available, "%s: %s\r\n", HD_Tandd_Request_Type, HD_Type_MonitorData);
#else //Warning
    len = snprintf(pH, available, "%s: %s\r\n", HD_Tandd_Request_Type, HD_Type_Warnig);         //****************
#endif
    pH += len; available -= len; if(available <= 0) return -1;

    //----- setthing count -----
    sprintf(temp, "%u", my_config.device.SysCnt);
    //templen = strlen(temp);
    //status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Setting_SysCnt, strlen(HD_Tandd_Setting_SysCnt), temp, len, wait_option);
    len = snprintf(pH, available, "%s: %s\r\n", HD_Tandd_Setting_SysCnt, temp);                 //****************
    pH += len; available -= len; if(available <= 0) return -1;

    sprintf( temp, "%u", my_config.device.RegCnt);
    //templen = strlen(temp);
    //status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Setting_RegCnt, strlen(HD_Tandd_Setting_RegCnt), temp, len, wait_option);
    len = snprintf(pH, available, "%s: %s\r\n", HD_Tandd_Setting_RegCnt, temp);                 //****************
    pH += len; available -= len; if(available <= 0) return -1;

    len = snprintf(pH, available, "\r\n");
    pH += len; available -= len; if(available <= 0) return -1;
    // NOTE:: If we add this extra "\r\n" we get a 400 bad request
    //len = snprintf(pH, available, "\r\n");
    //pH += len; available -= len; if(available <= 0) return -1;

    Printf("Header:\n%s", sH);
/*
[response_headers]
HTTP/1.1 200 OK
Server: nginx
Date: Mon, 09 Dec 2024 07:16:44 GMT
Content-Type: application/octet-stream
Content-Length: 0
Connection: keep-alive
X-Content-Type-Options: nosniff
X-Tandd-Next-Command: postSettings
*/
    return rtn;
}



int Monitor_make_sH(void)
{
    int rtn = 0;

    char *pH;
    int len;
    int available;
    available = sizeof(sH);
    pH = sH;

    char temp[128];
    //sprintf( temp, "%04lX", fact_config.SerialNumber);
    strcpy(temp, "5FA30005");
    //int templen = strlen(temp);

    int xsize;
    xsize = strlen(xml_buffer);

    //POST /api/rtr500.php HTTP/1.1
    len = snprintf(pH, available, "POST %s HTTP/1.1\r\n", p_info->resource);
    pH += len; available -= len; if(available <= 0) return -1;
    //Host: 192.168.1.100:80
    len = snprintf(pH, available, "Host: %s\r\n", p_info->host);
    pH += len; available -= len; if(available <= 0) return -1;
    //Content-Type: application/xml; charset=UTF-8
    len = snprintf(pH, available, "%s: %s\r\n", HD_Content_Type, HD_Content_Type_XML);
    pH += len; available -= len; if(available <= 0) return -1;
    //Content-Length: 3680
    len = snprintf(pH, available, "Content-Length: %d\r\n", xsize);
    pH += len; available -= len; if(available <= 0) return -1;

    //User-Agent: RTR500BW/R.01.00.06
    len = snprintf(pH, available, "%s: %s\r\n", HD_User_Agent, "RTR500BM/R.01.00.06");
    pH += len; available -= len; if(available <= 0) return -1;

    /*
    //User-Agent: RTR500BW/R.01.00.06
    if(fact_config.Vender == VENDER_ESPEC){
        len = snprintf(pH, available, "%s: %s\r\n", HD_User_Agent, HD_User_Agent_E);
        pH += len; available -= len; if(available <= 0) return -1;
    }else{
        len = snprintf(pH, available, "%s: %s\r\n", HD_User_Agent, HD_User_Agent_D);
        pH += len; available -= len; if(available <= 0) return -1;
    }
*/
    //X-Tandd-API-Version: 1.0
    len = snprintf(pH, available, "%s: %s\r\n", HD_Tandd_Api, HD_Tandd_Api_D);
    pH += len; available -= len; if(available <= 0) return -1;
    //X-Tandd-BaseUnit-Serial: 5EA30001
    len = snprintf(pH, available, "%s: %s\r\n", HD_Tandd_Serial, temp);
    pH += len; available -= len; if(available <= 0) return -1;
    //X-Tandd-Request-Type: postMonitoringData
    len = snprintf(pH, available, "%s: %s\r\n", HD_Tandd_Request_Type, HD_Type_MonitorData);
    pH += len; available -= len; if(available <= 0) return -1;

    //----- setthing count -----
    sprintf(temp, "%u", my_config.device.SysCnt);
    //templen = strlen(temp);
    //status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Setting_SysCnt, strlen(HD_Tandd_Setting_SysCnt), temp, len, wait_option);
    len = snprintf(pH, available, "%s: %s\r\n", HD_Tandd_Setting_SysCnt, temp);
    pH += len; available -= len; if(available <= 0) return -1;

    sprintf( temp, "%u", my_config.device.RegCnt);
    //templen = strlen(temp);
    //status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Setting_RegCnt, strlen(HD_Tandd_Setting_RegCnt), temp, len, wait_option);
    len = snprintf(pH, available, "%s: %s\r\n", HD_Tandd_Setting_RegCnt, temp);
    pH += len; available -= len; if(available <= 0) return -1;

    len = snprintf(pH, available, "\r\n");
    pH += len; available -= len; if(available <= 0) return -1;
    // NOTE:: If we add this extra "\r\n" we get a 400 bad request
    //len = snprintf(pH, available, "\r\n");
    //pH += len; available -= len; if(available <= 0) return -1;

    Printf("Header:\n%s", sH);
/*
[response_headers]
HTTP/1.1 200 OK
Server: nginx
Date: Mon, 09 Dec 2024 07:16:44 GMT
Content-Type: application/octet-stream
Content-Length: 0
Connection: keep-alive
X-Content-Type-Options: nosniff
X-Tandd-Next-Command: postSettings
*/
    return rtn;
}


int Monitor_Make(void)
{
    int rtn = 0;

    my_http_setup();

    Monitor_make_sB_text();
    memcpy(xml_buffer, sB, strlen(sB));

    Monitor_make_sH();

    return rtn;
}

int Warning_Make(void)
{
    int rtn = 0;

    my_http_setup();

    Warning_make_sB_text();
    memcpy(xml_buffer, sB, strlen(sB));

    Warning_make_sH();

    return rtn;
}

int Recorded_Make(void)
{
    int rtn = 0;

    my_http_setup();

    Recorded_make_sB_text();
    memcpy(xml_buffer, sB, strlen(sB));

    Recorded_make_sH();

    return rtn;
}



void do_webStorage_Action(char* pH, char* pB);

uint32_t process_http_test_H(char DbgCmd[])
{
    switch(DbgCmd[1])
    {
        case 'M':
            Monitor_Make();
            do_webStorage_Action(sH, sB);
            break;

        case 'W':
            Warning_Make();
            do_webStorage_Action(sH, sB);
            break;

        case 'R':
            Recorded_Make();
            do_webStorage_Action(sH, sB);
            break;
    }

    return 0;
}
