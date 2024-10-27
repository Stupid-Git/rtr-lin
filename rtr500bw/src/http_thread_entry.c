/**
 * @note	2019.Dec.26 ビルドワーニング潰し完了
 * @note	2020.01.30	V6 0130ソース反映
 * @note	2020.Jul.01 GitHub 630ソース反映済み
 * @note	2020.Jul.27 GitHub 722ソース反映済み
 * @note	2020.Jul.27 GitHub 0727ソース反映済み 
 */
#include "MyDefine.h"
#include "flag_def.h"
#include "Config.h"
#include "General.h"
#include "Globals.h"
//#include "TestData.h"
#include "tls_client.h"
#include "system_time.h"

#include "Log.h"
#include "led_thread_entry.h"

#include "network_thread.h"
#include "wifi_thread.h"
#include "http_thread.h"
#include "system_thread.h"
#include "wifi_thread_entry.h"          // sakaguchi 2021.11.25


extern TX_THREAD http_thread;           // sakaguchi 2020.12.07

#ifndef UN_USE
#define UNUSED(X)  (void)(X)

// Info for driving nx_web_http_client
typedef struct st_srv_info
{
    char                    *host;      // e.g. "api.webstorage.jp"  "os.mbed.com"
    char                    *resource;  // e.g. "https://api.webstorage.jp/xyz.php"

    NXD_ADDRESS             server_ip;  // ip address of host
    UINT                    server_port;

    char                    *username; // ""
    char                    *password; // ""

    NX_WEB_HTTP_CLIENT      *p_client;///< HTTP client structure


} srv_info_t;

srv_info_t  g_srv_info;

/*
UINT my_http_setup(void);
UINT my_http_PUT(void);
UINT get_host_ip_address( srv_info_t* p_info );
UINT get_web_http_client_response_bodyX(NX_WEB_HTTP_CLIENT* client_ptr );
*/
#endif



/* HTTP Thread entry function */
void http_thread_entry(void)
{
    /* TODO: add your own code here */

    UINT status, status_s;
    UINT status_dns;
    rtc_time_t ct;

    int mode;
    int cert_index = 0;                 //sakaguchi add UT-0036
    int cert_max;                       //sakaguchi add UT-0036
    
    uint32_t actual_events;
    uint32_t actual_events2;            //sakaguchi add UT-0008


    http_count = 0;
    success_count = 0;
    http_error = 0;

    status = nx_web_http_client_delete(&g_web_http_client0);
    Printf("## HTTP Thread Start / http client delete  %02X\r\n", status);

// sakaguchi 2020.12.23 ↓
    if(HTTP_SEND == Http_Use){
    ReqOpe_time = 5;                //操作リクエスト送信タイマ（秒） 初回起動時は5秒後に送信する
    vhttp_sysset_sndon();           //初回起動時は親機設定をWSSに送信する sakaguchi 2020.09.04
    G_HttpCmd[HTTP_CMD_RSETF].sndflg = SND_ON;  // 初回起動時は登録情報をWSSに送信する sakaguchi 2020.09.07
    }
// sakaguchi 2020.12.23 ↑

    for(;;){

        //イベントフラグ（動作許可）を待つ
        tx_event_flags_get( &event_flags_http, FLG_HTTP_READY, TX_OR_CLEAR, &actual_events, TX_WAIT_FOREVER );
        Printf("    HTTP Wait network mutex");
        tx_mutex_get(&mutex_network_init, TX_WAIT_FOREVER);
        Printf("    HTTP network init mutex get!!\r\n");
        tx_mutex_put(&mutex_network_init);   
        Printf("    HTTP network init mutex put!!\r\n");

        for(;;){

            if( SND_OFF == ichttp_filesnd_check() ){    // 送信ファイル無し？
                Printf("Http non Send File\r\n");
                tx_event_flags_set (&event_flags_http, FLG_HTTP_CANCEL, TX_OR);     // add 2020 06 09
                break;                                  // 処理終了
            }

            http_count ++;
           
            if(my_config.network.Phy[0] == WIFI){
                LED_Set( LED_WIFICOM, LED_ON);
                LED_Set( LED_WIFION, LED_ON);
            }
            else{
                LED_Set( LED_LANCOM, LED_ON);
                LED_Set( LED_LANON, LED_ON);
            }
            mode = ((my_config.websrv.Mode[0] == 0) || (my_config.websrv.Mode[0] == 1)) ? CONNECT_HTTP : CONNECT_HTTPS;

            NetCmdStatus = ETLAN_BUSY;
            status_s = 1;
            //g_rtc.p_api->calendarTimeGet(g_rtc.p_ctrl, &ct);
            RTC_GetLCT( &ct );	
            Printf(" HTTP  start %d/%d/%d %d:%d:%d \r\n", ct.tm_year/*+1900*/,ct.tm_mon+0,ct.tm_mday,ct.tm_hour,ct.tm_min,ct.tm_sec);

            //g_rtc.p_api->calendarTimeGet(g_rtc.p_ctrl, &ct);
            //Printf("===>   dns start : %d/%d/%d %d:%d:%d \r\n", ct.tm_year,ct.tm_mon,ct.tm_mday,ct.tm_hour,ct.tm_min,ct.tm_sec);

            status_dns = NX_SUCCESS; 
            
            //g_rtc.p_api->calendarTimeGet(g_rtc.p_ctrl, &ct);
            //Printf("===>   dns stop : %d/%d/%d %d:%d:%d \r\n", ct.tm_year,ct.tm_mon,ct.tm_mday,ct.tm_hour,ct.tm_min,ct.tm_sec);
        
            if(status_dns == NX_SUCCESS){       
                if(mode == CONNECT_HTTPS){          // Connect HTTPS
                    web_http_client_init0();

// sakaguchi cg UT-0036 ↓
ChangeCert:
                    status = my_https_setup(CONNECT_HTTPS, cert_index);

                    if(status == NX_SUCCESS){
                        status   = my_tls_setup();
                        if(status == NX_SUCCESS){
                            tls_error = 0;                              // tls errorが続いた場合は、復帰しないかも        
                            status_s = my_https_POST(CONNECT_HTTPS);

                            switch (status_s)
                            {
                                case NX_SUCCESS:
                                    http_error = 0;
                                    Printf("--->>> HTTPS Success %02X\r\n", status_s);
                                    break;
                                case NX_WEB_HTTP_STATUS_CODE_BAD_REQUEST:
                                    Printf("---XXX HTTPS Error 400 Bad Request\r\n");
                                    PutLog(LOG_LAN,"HTTPS 400 Bad Request");
                                    DebugLog( LOG_DBG, "HTTPS 400 Bad Request (%04X)", status_s);
                                    break;
                                case NX_WEB_HTTP_STATUS_CODE_INTERNAL_ERROR:
                                    Printf("---XXX HTTPS Error 500 Internal Server Error\r\n");
                                    PutLog(LOG_LAN,"HTTPS 500 Server Error");
                                    DebugLog( LOG_DBG, "HTTPS 500 Server Error (%04X)", status_s);
                                    break;
                                case NX_WEB_HTTP_STATUS_CODE_FORBIDDEN:
                                    Printf("---XXX HTTPS Error 403 Forbidden\r\n");
                                    PutLog(LOG_LAN,"HTTPS 403 Forbidden");
                                    DebugLog( LOG_DBG, "HTTPS 403 Forbidden (%04X)", status_s);
                                    break;
                                case NX_WEB_HTTP_STATUS_CODE_NOT_FOUND:
                                    Printf("---XXX HTTPS Error 404 Not Found\r\n");
                                    PutLog(LOG_LAN,"HTTPS 404 Not Found");
                                    DebugLog( LOG_DBG, "HTTPS 404 Not Found (%04X)", status_s);
                                    break;
                                case NX_WEB_HTTP_STATUS_CODE_NOT_ACCEPTABLE:
                                    Printf("---XXX HTTPS Error 406 Not Acceptable\r\n");
                                    PutLog(LOG_LAN,"HTTPS 406 Not Acceptable");
                                    DebugLog( LOG_DBG, "HTTPS 406 Not Acceptable (%04X)", status_s);
                                    break;
                                case NX_WEB_HTTP_STATUS_CODE_SERVICE_UNAVAILABLE:
                                    Printf("---XXX HTTPS status code 503 Service Unavailable\r\n");
                                    PutLog(LOG_LAN,"HTTPS 503 Service Unavailable");
                                    DebugLog( LOG_DBG, "HTTPS 503 Service Unavailable (%04X)", status_s);
                                    break;
                                case NX_WEB_HTTP_INCOMPLETE_PUT_ERROR:
                                    Printf("---XXX HTTPS Improper HTTP header format\r\n");
                                    PutLog(LOG_LAN,"HTTPS Improper HTTP header format");
                                    DebugLog( LOG_DBG, "HTTPS Improper HTTP header format (%04X)", status_s);
                                    break;
// sakaguchi 2020.12.23 Data Server 412対応 ↓
                                case NX_WEB_HTTP_STATUS_CODE_PRECONDITION_FAILED:
                                    Printf("---XXX HTTPS Error 412 Precondition Failed \r\n");
                                    PutLog(LOG_LAN,"HTTPS Error 412 Precondition Failed");
                                    DebugLog( LOG_DBG, "HTTPS Error 412 Precondition Failed (%04X)", status_s);
                                    break;
// sakaguchi 2020.12.23 ↑

//sakaguchi UT-0041 ↓
                                case NX_SECURE_TLS_INVALID_SERVER_CERT:             // (0x10C) An incoming server certificate did not parse correctly.
                                case NX_SECURE_TLS_UNSUPPORTED_PUBLIC_CIPHER:       // (0x10D) A certificate provided by a server specified a public-key operation we do not support.
                                case NX_SECURE_TLS_INVALID_CERTIFICATE:             // (0x112) An X509 certificate did not parse correctly.
                                case NX_SECURE_TLS_UNKNOWN_CERT_SIG_ALGORITHM:      // (0x116) A certificate during verification had an unsupported signature algorithm.
                                case NX_SECURE_TLS_CERTIFICATE_SIG_CHECK_FAILED:    // (0x117) A certificate signature verification check failed - certificate data did not match signature.
                                case NX_SECURE_TLS_CERTIFICATE_NOT_FOUND:           // (0x119) In an operation on a certificate list, no matching certificate was found.
                                case NX_SECURE_TLS_INVALID_SELF_SIGNED_CERT:        // (0x11A) The remote host sent a self-signed certificate and NX_SECURE_ALLOW_SELF_SIGNED_CERTIFICATES is not defined.  //sakaguchi add UT-0039
                                case NX_SECURE_TLS_ISSUER_CERTIFICATE_NOT_FOUND:    // (0x11B) A remote certificate was received with an issuer not in the local trusted store.
                                case NX_SECURE_X509_MULTIBYTE_TAG_UNSUPPORTED:      //（0x181）We encountered a multi-byte ASN.1 tag - not currently supported.
                                case NX_SECURE_X509_ASN1_LENGTH_TOO_LONG:           //（0x182）Encountered a length value longer than we can handle. 
                                case NX_SECURE_X509_FOUND_NON_ZERO_PADDING:         //（0x183）Expected a padding value of 0 - got something different.
                                case NX_SECURE_X509_MISSING_PUBLIC_KEY:             //（0x184）X509 expected a public key but didn't find one. 
                                case NX_SECURE_X509_INVALID_PUBLIC_KEY:             //（0x185）Found a public key, but it is invalid or has an incorrect format. 
                                case NX_SECURE_X509_INVALID_CERTIFICATE_SEQUENCE:   //（0x186）The top-level ASN.1 block is not a sequence - invalid X509 certificate. 
                                case NX_SECURE_X509_MISSING_SIGNATURE_ALGORITHM:    //（0x187）Expecting a signature algorithm identifier, did not find it. 
                                case NX_SECURE_X509_INVALID_CERTIFICATE_DATA:       //（0x188）Certificate identity data is in an invalid format.
                                case NX_SECURE_X509_UNEXPECTED_ASN1_TAG:            //（0x189）We were expecting a specific ASN.1 tag for X509 format but we got something else.
                                case NX_SECURE_PKCS1_INVALID_PRIVATE_KEY:           //（0x18A）A PKCS#1 private key file was passed in, but the formatting was incorrect.
                                case NX_SECURE_X509_CHAIN_TOO_SHORT:                //（0x18B）An X509 certificate chain was too short to hold the entire chain during chain building.
                                case NX_SECURE_X509_CHAIN_VERIFY_FAILURE:           //（0x18C）An X509 certificate chain was unable to be verified (catch-all error).
                                case NX_SECURE_X509_PKCS7_PARSING_FAILED:           //（0x18D）Parsing an X.509 PKCS#7-encoded signature failed.
                                case NX_SECURE_X509_CERTIFICATE_NOT_FOUND:          //（0x18E）In looking up a certificate, no matching entry was found.
                                case NX_SECURE_X509_INVALID_VERSION:                //（0x18F）A certificate included a field that isn't compatible with the given version.
                                case NX_SECURE_X509_INVALID_TAG_CLASS:              //（0x190）A certificate included an ASN.1 tag with an invalid tag class value.
                                case NX_SECURE_X509_INVALID_EXTENSIONS:             //（0x191）A certificate included an extensions TLV but that did not contain a sequence. 
                                case NX_SECURE_X509_INVALID_EXTENSION_SEQUENCE:     //（0x192）A certificate included an extension sequence that was invalid X.509.
                                case NX_SECURE_X509_CERTIFICATE_EXPIRED:            //（0x193）A certificate had a "not after" field that was less than the current time.
                                case NX_SECURE_X509_CERTIFICATE_NOT_YET_VALID:      //（0x194）A certificate had a "not before" field that was greater than the current time.
                                case NX_SECURE_X509_CERTIFICATE_DNS_MISMATCH:       //（0x195）A certificate Common Name or Subject Alt Name did not match a given DNS TLD.
                                case NX_SECURE_X509_INVALID_DATE_FORMAT:            //（0x196）A certificate contained a date field that is not in a recognized format.
                                case NX_SECURE_X509_CRL_ISSUER_MISMATCH:            //（0x197）A provided CRL and certificate were not issued by the same Certificate Authority.
                                case NX_SECURE_X509_CRL_SIGNATURE_CHECK_FAILED:     //（0x198）A CRL signature check failed against its issuer.
                                case NX_SECURE_X509_CRL_CERTIFICATE_REVOKED:        //（0x199）A certificate was found in a valid CRL and has therefore been revoked.
                                case NX_SECURE_X509_WRONG_SIGNATURE_METHOD:         //（0x19A）In attempting to validate a signature the signature method did not match the expected method.
                                case NX_SECURE_X509_EXTENSION_NOT_FOUND:            //（0x19B）In looking for an extension, no extension with a matching ID was found.
                                case NX_SECURE_X509_ALT_NAME_NOT_FOUND:             //（0x19C）A name was searched for in a subjectAltName extension but was not found.
                                case NX_SECURE_X509_INVALID_PRIVATE_KEY_TYPE:       //（0x19D）Private key type given was unknown or invalid.
                                case NX_SECURE_X509_NAME_STRING_TOO_LONG:           //（0x19E）A name passed as a parameter was too long for an internal fixed-size buffer.
                                case NX_SECURE_X509_EXT_KEY_USAGE_NOT_FOUND:        //（0x19F）In parsing an ExtendedKeyUsage extension, the specified usage was not found.
                                case NX_SECURE_X509_KEY_USAGE_ERROR:                //（0x1A0）For use with key usage extensions - return this to indicate an error at the application level with key usage.
                                    Printf("---XXX HTTPS CERTIFICATE Error %02X\r\n", status_s);
//                                    PutLog(LOG_LAN,"HTTPS Staus %04X", status_s);
                                    PutLog(LOG_LAN,"HTTPS Status %04X", status_s);      // 2023.11.22
                                    DebugLog( LOG_DBG, "HTTPS 1(%04X)", status_s);
                                    // root certificate
                                    if( my_config.websrv.Mode[0] == 2 ){    // webstorage
                                        cert_max = 6;                           // 6面
                                    }else{                                  // user site
                                        cert_max = 2;                           // 2面
                                    }

                                    cert_index++;

                                    if( cert_index < cert_max ){
                                        status = my_tls_DOWN();             // sakaguchi add UT-0039
                                        Printf("--- ChangeCert index[%d] max[%d]\r\n", cert_index, cert_max );          // sakaguchi 2021.06.17
                                        goto ChangeCert;                    // 証明書切替
                                    }else{
                                        cert_index = 0;                     // 初期値に戻す
                                        vhttp_cmdflg_allreset(E_NG);        // HTTP要求を全てクリア // sakaguchi 2021.02.16
                                        Printf("--- ChangeCert END index[%d] max[%d]\r\n", cert_index, cert_max );      // sakaguchi 2021.06.17
                                    }
                                    break;
//sakaguchi UT-0041 ↑
                                default:
                                    http_error++;
                                    DebugLog( LOG_DBG, "HTTPS 2(%04X)", status_s);
                                    Printf("---XXX HTTPS Error %02X (%d)\r\n", status_s, http_error);
                                    break;
                            }

                            //if(status_s != NX_SUCCESS){
                            //    PutLog(LOG_LAN,"HTTPS Staus %04X", status_s);
                            //    DebugLog( LOG_DBG, "HTTPS(%04X)", status_s);
                            //}

                            /* DNS error 復旧で不要かも
                            if(status_s != NX_DNS_QUERY_FAILED){
                                //http_error++;
                                DebugLog( LOG_DBG, "HTTPS(%04X)", status_s);
                                status = my_tls_DOWN();
                            }*/
                    
                            // sakaguchi 2020.09.03
                            my_tls_session_delete();
                        }
                        else{
                            tls_error++;
                            http_error++;
                            DebugLog( LOG_DBG, "HTTPS TLS Setup Error (%04X)(%ld)(%ld)", status,http_error,tls_error);
                            Printf("    TLS Setup Error %02X\r\n", status);
                        }
                    }
                    else{
                        http_error++;
                    }
// sakaguchi cg UT-0036 ↑
                    if((status != NX_SUCCESS) || (status_s != NX_SUCCESS)){
                        my_client_error_exit();         // sakaguchi 2020.09.09
                    }
                    status = nx_web_http_client_delete(&g_web_http_client0);
                    Printf("#########      http client delete  %02X\r\n", status);

//                    status = nx_web_http_client_create (&g_web_http_client0, "g_web_http_client0 HTTP Client", &g_ip0, &g_packet_pool1, 4096);
//                    Printf("\r\n#########      http client create  %02X\r\n\r\n", status);

                }

                else{                               // Connect HTTP
                    web_http_client_init0();
                    status   = my_https_setup(CONNECT_HTTP, 0);         // sakaguchi cg UT-0036
                    status_s = my_https_POST(CONNECT_HTTP);

                    switch (status_s)
                    {
                        case NX_SUCCESS:
                            http_error = 0;
                            Printf("--->>> HTTP Success %02X\r\n", status_s);
                           break;
                        case NX_WEB_HTTP_STATUS_CODE_BAD_REQUEST:
                            Printf("---XXX HTTP Error 400 Bad Request\r\n");
                            PutLog(LOG_LAN,"HTTP 400 Bad Request");
                            DebugLog(LOG_DBG, "HTTP 400 Bad Request (%04X)", status_s);
                            break;
                        case NX_WEB_HTTP_STATUS_CODE_INTERNAL_ERROR:
                            Printf("---XXX HTTP Error 500 Internal Server Error\r\n");
                            PutLog(LOG_LAN,"HTTP 500 Server Error");
                            DebugLog(LOG_DBG, "HTTP 500 Server Error (%04X)", status_s);
                            break;
                        case NX_WEB_HTTP_STATUS_CODE_FORBIDDEN:
                            Printf("---XXX HTTP Error 403 Forbidden\r\n");
                            PutLog(LOG_LAN,"HTTP 403 Forbidden");
                            DebugLog(LOG_DBG, "HTTP 403 Forbidden (%04X)", status_s);
                            break;
                        case NX_WEB_HTTP_STATUS_CODE_NOT_FOUND:
                            Printf("---XXX HTTP Error 404 Not Found\r\n");
                            PutLog(LOG_LAN,"HTTP 404 Not Found");
                            DebugLog( LOG_DBG, "HTTP 404 Not Found (%04X)", status_s);
                            break;
                        case NX_WEB_HTTP_STATUS_CODE_NOT_ACCEPTABLE:
                            Printf("---XXX HTTP Error 406 Not Acceptable\r\n");
                            PutLog(LOG_LAN,"HTTP 406 Not Acceptable");
                            DebugLog( LOG_DBG, "HTTP 406 Not Acceptable (%04X)", status_s);
                            break;
                        case NX_WEB_HTTP_STATUS_CODE_SERVICE_UNAVAILABLE:
                            Printf("---XXX HTTP status code 503 Service Unavailable\r\n");
                            PutLog(LOG_LAN,"HTTP 503 Service Unavailable");
                            DebugLog(LOG_DBG, "HTTP 503 Service Unavailable (%04X)", status_s);
                            break;
                        case NX_WEB_HTTP_INCOMPLETE_PUT_ERROR:
                            Printf("---XXX HTTP Improper HTTP header format\r\n");
                            PutLog(LOG_LAN,"HTTP Improper HTTP header format");
                            DebugLog(LOG_DBG, "HTTP Improper HTTP header format (%04X)", status_s);
                            break;
// sakaguchi 2020.12.23 Data Server 412対応 ↓
                        case NX_WEB_HTTP_STATUS_CODE_PRECONDITION_FAILED:
                            Printf("---XXX HTTPS Error 412 Precondition Failed \r\n");
                            PutLog(LOG_LAN,"HTTPS Error 412 Precondition Failed");
                            DebugLog( LOG_DBG, "HTTPS Error 412 Precondition Failed (%04X)", status_s);
                            break;
// sakaguchi 2020.12.23 ↑
                        default:
                            http_error++;                       // 2021.02.08
                            Printf("---XXX HTTP Error\r\n");
                            PutLog(LOG_LAN,"HTTP Error");
                            DebugLog(LOG_DBG, "HTTP Error (%04X)", status_s);
                            break;
                    }
                    Printf("https get %02X\r\n", status_s);

                    if((status != NX_SUCCESS) || (status_s != NX_SUCCESS)){
                        my_client_error_exit();         // sakaguchi 2020.09.09
                    }
                    status = nx_web_http_client_delete(&g_web_http_client0);
                    Printf("    http client delete  %02X\r\n", status);
                }   // if end Connect HTTP 
            }
            else{
                status_s = status_dns;

            }
            //g_rtc.p_api->calendarTimeGet(g_rtc.p_ctrl, &ct);
            RTC_GetLCT( &ct );	
            Printf(" HTTP  end   %d/%d/%d %d:%d:%d \r\n", ct.tm_year/*+1900*/,ct.tm_mon+0,ct.tm_mday,ct.tm_hour,ct.tm_min,ct.tm_sec);

// 2023.07.27 DataServer 1リクエスト対策 ↓
#if 0
            if(status_s == NX_SUCCESS){
                NetCmdStatus = ETLAN_SUCCESS;
                tx_event_flags_set (&event_flags_http, FLG_HTTP_SUCCESS, TX_OR);
            }
            else{
                NetCmdStatus = ETLAN_ERROR;
                tx_event_flags_set (&event_flags_http, FLG_HTTP_ERROR, TX_OR);
            }
#endif
            if(( my_config.websrv.Mode[0] == 0 ) && (SND_ON == ichttp_filesnd_check())){    // DataServerでまだ送信ファイルがある場合
                // event_flags_httpはまだセットしない
            }else{
                if(status_s == NX_SUCCESS){
                    NetCmdStatus = ETLAN_SUCCESS;
                    tx_event_flags_set (&event_flags_http, FLG_HTTP_SUCCESS, TX_OR);
                }
                else{
                    NetCmdStatus = ETLAN_ERROR;
                    tx_event_flags_set (&event_flags_http, FLG_HTTP_ERROR, TX_OR);
                }
            }
// 2023.07.27 DataServer 1リクエスト対策 ↑

            //if(http_error>3){
            if(http_error>1){    
                NetCmdStatus = ETLAN_ERROR;
                tx_event_flags_set (&event_flags_http, FLG_HTTP_ERROR, TX_OR);      // 2023.07.27

                http_error = 0;
                NetReboot = 1;
                Printf("\r\n########  Network  Reboot !!!!! ######################  \r\n");
                tx_event_flags_set (&event_flags_link, FLG_LINK_REBOOT, TX_OR);
                tx_thread_suspend(&http_thread);        // sakaguchi 2020.12.07
            }

//sakaguchi add UT-0008 ↓
            status = tx_event_flags_get (&event_flags_reboot, FLG_REBOOT_REQUEST ,TX_OR_CLEAR, &actual_events2,TX_NO_WAIT);
            if((status == TX_SUCCESS) && (actual_events2 & FLG_REBOOT_REQUEST)){
                NetCmdStatus = ETLAN_ERROR;
                tx_event_flags_set (&event_flags_http, FLG_HTTP_ERROR, TX_OR);      // 2023.07.27

                Printf("\r\n   Reboot Request (%04X) !!!!!\r\n\r\n", actual_events2);
                tx_thread_sleep (50);
                tx_event_flags_set(&event_flags_reboot, FLG_REBOOT_EXEC, TX_OR);
                tx_thread_suspend(&http_thread);         // sakaguchi 2020.12.07
            }
//sakaguchi add UT-0008 ↑

            // 途中でWIFIがDown debug中
            status = tx_event_flags_get (&event_flags_wifi, FLG_WIFI_DOWN ,TX_OR_CLEAR, &actual_events2,TX_NO_WAIT);
            if((status == TX_SUCCESS) && (actual_events2 & FLG_WIFI_DOWN)){
                NetCmdStatus = ETLAN_ERROR;
                tx_event_flags_set (&event_flags_http, FLG_HTTP_ERROR, TX_OR);      // 2023.07.27

                Printf("\r\n   WIFI Link Down (%04X) !!!!!\r\n\r\n", actual_events2);
                tx_event_flags_set (&event_flags_link, 0x00000000, TX_AND);
                tx_event_flags_set (&event_flags_link, FLG_LINK_DOWN, TX_OR);
                tx_thread_suspend(&http_thread);        // sakaguchi 2020.12.07
            }
            //break;            // 送信ファイルがある場合は処理を継続させる // sakaguchi 20.08.21 del

            // LinkDownを検出したらサスペンド       // sakaguchi 2021.11.25
//            if(LINK_DOWN == Link_Status){
            if(NETWORK_DOWN == NetStatus){        // ネットワーク未接続を検出したらサスペンド 2022.03.23 cg
                NetCmdStatus = ETLAN_ERROR;
                tx_event_flags_set (&event_flags_http, FLG_HTTP_ERROR, TX_OR);      // 2023.07.27

                Printf("\r\n   Network Down!!!!!\r\n\r\n");
                tx_thread_suspend(&http_thread);
            }

        }
        tx_thread_sleep (1);
    }
}

