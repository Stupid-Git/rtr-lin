/**
 * @file	tls_client.c
 *
 * @date	Created on: 2019/09/18
 * @author	tooru.hayashi
 * @note	2020.01.30	v6 0128 ソース反映済み
 * @note	2020.Jul.01 GitHub 630ソース反映済み
 * @note	2020.Jul.01 GitHub 0701ソース反映済み
 * @note	2020.Jul.27 GitHub 0727ソース反映済み 
 * @note	2020.Aug.07	0807ソース反映済み
 */

//#define TRY_WEBSTORAGE 0
#define _TLS_CLIENT_C_

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "tx_api.h"
#include "nx_api.h"

#include "nxd_dns.h"

#include "nx_secure_tls_api.h"

#include "nx_crypto_aes_sce.h"
#include "nx_crypto_sha2_sce.h"
#include "nx_crypto_sha1_sce.h"
#include "nx_crypto_rsa_sce.h"
#include "nx_crypto_hmac_sha1_sce.h"
#include "nx_crypto_hmac_sha2_sce.h"

#include "MyDefine.h"
#include "Config.h"
#include "http_thread.h"
#include "nx_web_http_client.h"

#include "sntp_client.h" //get_time
#include "wifi_thread.h"

#include "Log.h"
#include "General.h"
#include "Globals.h"
#include "Net_func.h"
//#include "TestData.h"
#include "tls_client.h"
#include "Version.h"
#include "Sfmem.h"
#include "Error.h"
#include "Cmd_func.h"
#include "Base.h"
#include "Rfunc.h"
#include "ble_thread_entry.h"
#include "cmd_thread_entry.h"
#include "usb_thread_entry.h"
#include "rf_thread.h"
extern TX_THREAD cmd_thread;



#define UNUSED(X)  (void)(X)
#define ARRAY_SIZE(x)               (sizeof(x) / sizeof(x[0]))  //#include "common_util.h"

// NX_WEB_HTTPS_ENABLE
//#include "http_client_api.h"

UINT my_tls_session_end(void);                                  // sakaguchi 2020.09.02
UINT my_tls_close(NX_WEB_HTTP_CLIENT* client_ptr,int which);

#ifndef UN_USE

// Info for driving nx_web_http_client
typedef struct st_whc_info
{
    char                    *host;      // e.g. "api.webstorage.jp"  "os.mbed.com"
    char                    *resource;  // e.g. "https://api.webstorage.jp/xyz.php"

    NXD_ADDRESS             server_ip;  // ip address of host
    UINT                    server_port;

    char                    *username; // ""
    char                    *password; // ""

//    NX_IP                   *p_ip;          ///< NetX IP struct
//    NX_DNS                  *p_dns;         ///< NetX DNS struct
//    UINT                    thread_priority;///< HTTP thread priority
    NX_WEB_HTTP_CLIENT      *p_secure_client;///< HTTP client structure

//    void                    (*http_init)(void); ///< init function
//    uint8_t                 *p_netif;

} whc_info_t;
#endif // !1

/* Size of buffer for raw certificate data */
//#define CERTIFICATE_BUFFER_SIZE     (2048)
#define CERTIFICATE_BUFFER_SIZE     (4094)

/* Number of intermediate CA's. Increase this value if you get certificate validation errors */
#define NUM_INTERMEDIATE_CA         (4)

// Info for the tls session for the nx_web_http_client
typedef struct st_whc_tls
{
    //uint8_t                   tls_packet_buffer[4000];                                                  ///< TLS packet buffer for reassembly
    uint8_t                   tls_packet_buffer[16000*4];                                                   ///< TLS packet buffer for reassembly

    // Define scratch space for cryptographic methods:
    // 2*sizeof(NX_AES) + sizeof(NX_SHA1_HMAC) + 2*sizeof(NX_CRYPTO_RSA) +
    //     (2 * (sizeof(NX_MD5) + sizeof(NX_SHA1) + sizeof(NX_SHA256)))];
    
//    CHAR                    crypto_metadata[16384]; //8850                                              ///< Crypto metadata
    CHAR                    crypto_metadata[40960]; //8850                                             ///< Crypto metadata sakaguchi cg

    /* Certificate storage */

    NX_SECURE_X509_CERT     remote_intermediate_ca[NUM_INTERMEDIATE_CA];                                ///< Intermediate CA's
    uint8_t                   remote_int_ca_cert_buffer[CERTIFICATE_BUFFER_SIZE * NUM_INTERMEDIATE_CA];   ///< Intermediate CA certificate buffer
    NX_SECURE_X509_CERT     remote_certificate;                                                         ///< Remote server certificate
    uint8_t                   remote_cert_buffer[CERTIFICATE_BUFFER_SIZE];                                ///< Remote server certificate buffer
    uint8_t                 *p_root_ca;                                                                 ///< Root CA cert for this service
    uint16_t                root_ca_len;                                                                ///< Length of Root CA cert
    uint8_t                 *p_device_cert;                                                             ///< Device certificate used for this service
    uint16_t                device_cert_len;                                                            ///< Length of device certificate
    uint8_t                 *p_private_key;                                                             ///< Device's private key for this service
    uint16_t                private_key_len;                                                            ///< Length of private key

    NX_SECURE_TLS_CRYPTO    *p_ciphers;                                                                 ///< Crypto ciphers to be used with TLS

    NX_WEB_HTTP_CLIENT      *p_secure_client;                                                           ///< HTTP client structure

    void                    (*logMsg)(const char *msg);                                                 ///< Message logger
} whc_tls_t;

//whc_info_t  g_whc_info;
srv_info_t  g_whc_info;
whc_tls_t  g_whc_tls;
static  char        S_cSrvApifull[256]; // プロキシサーバ経由のサーバ接続先パス

//extern NX_IP          *g_active_ip;
//extern NX_DNS         *g_active_dns;

static  int         S_iHttpCnt = 0;     // HTTP通信用バッファカウンタ
static  uint32_t    S_ulReqID = 0;      // リクエストID

static  uint8_t     S_ucNextCmd = 0;    // Nextcommand

static  uint8_t     S_ucErrCmd[6];      // エラー応答用コマンド
static  uint8_t     S_ucLogAct = 0;     // ログACT指定
static  uint8_t     S_ucRfWaitCnt = 0;  // 無線通信遅延カウンタ

static  uint8_t     S_ucCmdAns[8192];   // コマンドスレッド応答バッファ     // sakaguchi add 2020.08.27
static int32_t txLen;      //CmdStatusSizeの代わりに  cmd_threadから受け取る 2020,Aug.26    // sakaguchi del 2020.08.27

const char  *HTTP_T2Cmd[] = {
    "NONE",     //0  定義なし
    "WUINF",    //1  機器設定（変更）
    "RUINF",    //2  機器設定（取得）
    "WBLEP",    //3  Bluetooth設定（変更）
    "RBLEP",    //4  Bluetooth設定（取得）
    "WNSRV",    //5  ネットワークサーバ設定（変更）
    "RNSRV",    //6  ネットワークサーバ設定（取得）
    "WNETP",    //7  ネットワーク情報設定（変更）
    "RNETP",    //8  ネットワーク情報設定（取得）
    "WWLAN",    //9  無線LAN情報設定（変更）
    "RWLAN",    //10 無線LAN情報設定（取得）
    "WDTIM",    //11 時刻設定（変更）
    "RDTIM",    //12 時刻設定（取得）
    "WENCD",    //13 エンコード方式（変更）
    "RENCD",    //14 エンコード方式（取得）
    "WWARP",    //15 警報設定（変更）
    "RWARP",    //16 警報設定（取得）
    "WCURP",    //17 モニタリング設定（変更）
    "RCURP",    //18 モニタリング設定（取得）
    "WSUCP",    //19 記録データ送信設定（変更）
    "RSUCP",    //20 記録データ送信設定（取得）
    "WUSRD",    //21 ユーザ定義情報（変更）
    "RUSRD",    //22 ユーザ定義情報（取得）
    "WRTCE",    //23 httpsルート証明書（変更）
    "RRTCE",    //24 httpsルート証明書（取得）
    "WVGRP",    //25 グループ情報（変更）
    "RVGRP",    //26 グループ情報（取得）
    "WPRXY",    //27 プロキシ設定（変更）
    "RPRXY",    //28 プロキシ設定（取得）
    "WSETF",    //29 登録情報設定（変更）
    "RSETF",    //30 登録情報設定（取得）
    "EINIT",    //31 初期化と再起動
    "ETGSM",    //32 テスト（3GおよびGPSモジュール）
    "ETLAN",    //33 テスト
    "EBSTS",    //34 ステータスの取得と設定
    "ELOGS",    //35 ログの取得および消去
    "ETRND",    //36 トレンド情報（モニタリングデータ）取得
    "EMONS",    //37 モニタリングを開始する
    "EDWLS",    //38 記録データ送信開始
    "ERGRP",    //39 指定グループの無線通知設定取得
    "EWLEX",    //40 子機の電波強度を取得する(無線通信)
    "EWLRU",    //41 中継機の電波強度を取得する(無線通信)
    "EWSTR",    //42 子機の設定を取得する(無線通信)
    "EWSTW",    //43 子機の設定を変更する(記録開始)(無線通信)
    "EWCUR",    //44 子機の現在値を取得する(無線通信)
    "EWLAP",    //45 無線LANのアクセスポイントを検索する
    "EWSCE",    //46 子機の検索（無線通信）
    "EWSCR",    //47 中継機の一斉検索（無線通信）
    "EWRSC",    //48 子機のモニタリングデータを取得する（無線通信）
    "EWBSW",    //49 一斉記録開始（無線通信）
    "EWRSP",    //50 子機の記録を停止する（無線通信）
    "EWPRO",    //51 プロテクト設定（無線通信）
    "EWINT",    //52 子機のモニタリング間隔変更（無線通信）
    "EWBLW",    //53 子機のBluetooth設定を設定する(無線通信)
    "EWBLR",    //54 子機のBluetooth設定を取得する(無線通信)
    "EWSLW",    //55 子機のスケール変換式を設定する(無線通信)
    "EWSLR",    //56 子機のスケール変換式を取得する(無線通信)
    "DMY57",    //57 ダミー
    "EAUTH",    //58 パスワード認証
    "EBADV",    //59 アドバタイジングデータの取得
    "EWRSR",    //60 子機と中継機の一斉検索（無線通信）
    "EWREG",    //61 子機登録変更（無線通信）
    "ETWAR",    //62 警報テストを開始する
    "EMONR",    //63 モニタリングで収集した無線通信情報と電池残量を取得する
    "EMONC",    //64 モニタリングで取得した電波強度を削除する
    "WREGI",    //65 登録情報の設定を変更する（子機）
    "EWAIT",    //66 自律動作の開始しない時間を設定する
    "EFIRM",    //67 ファームウェア更新
    "DMY68",    //68 ダミー
};

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

UINT get_web_http_client_response_body(NX_WEB_HTTP_CLIENT* client_ptr );
//UINT find_host_ip_address( whc_info_t* p_info );
static UINT send_file(NX_WEB_HTTP_CLIENT  *client_ptr, NX_PACKET *send_packet, char *buf, size_t len );

static  int     ihttp_snddata_make(int ifile, int *index, char *sndbuf);
static  int     ihttp_postState_make(int icmd, char *sndbuf);
static  int     ihttp_T2cmd_make(int icmd, char *sndbuf);
//static  int     ihttp_logdata_make(char *sndbuf, int size, int act, int area);
static  int     ihttp_logdata_make(char *sndbuf, int act, int area, int file);    // 2023.03.01
static  bool    bhttp_regist_read(char *cbuf, uint32_t offset, uint32_t size);
static  bool    bhttp_vgroup_read(char *cbuf);
static  bool    bhttp_srvcmd_analyze(int ifile, char *cbuf);
static  int     ihttp_T2cmd_index_get(char *cbuf);
static  int32_t lhttp_T2cmd_ParamAdrs_get(char *cbuf, char *Key, char *Adrs);
static  uint32_t ulhttp_T2cmd_analyze(char *cbuf);
static  int     ihttp_act_snd(int icmd, uint16_t act);
static  int     ihttp_cmd_thread_snd(int icmd, char *cbuf);
static  int     iChgChartoBinary(char *value, int isize );
//static  void    vhttp_cmdflg_reset(int ifile, int imode);
//static  void    vhttp_cmdflg_allreset(int imode);
static  int     ihttp_param_get(char *data, uint16_t size, char *para);
static  uint32_t    ulchg_http_status_code(uint32_t status);

extern  const USHORT    nx_crypto_ecc_supported_groups_synergys7[];
extern  const USHORT    nx_crypto_ecc_supported_groups_synergys7_size;
extern  const NX_CRYPTO_METHOD  *nx_crypto_ecc_curves_synergys7[];



static UINT NX_HTTP_tls_setup(NX_WEB_HTTP_CLIENT *client_ptr, NX_SECURE_TLS_SESSION *p_tls_session)
{
    whc_tls_t *p_tls;
    UINT status;
    UINT i;
    static NX_SECURE_X509_CERT    _tls_certificate;
    static NX_SECURE_X509_CERT    _tls_trusted_certificate;
    NX_SECURE_X509_CERT    *p_cert = &_tls_certificate;
    NX_SECURE_X509_CERT    *p_trusted_cert = &_tls_trusted_certificate;


    SSP_PARAMETER_NOT_USED (client_ptr);
    p_tls = &g_whc_tls;

    Printf("  . NX_HTTP_tls_setup\r\n");

    // Allocate space for the certificate coming in from the remote host.
    memset(p_tls->remote_cert_buffer, 0, sizeof(p_tls->remote_cert_buffer));
    memset(&p_tls->remote_certificate, 0, sizeof(p_tls->remote_certificate));
    status = nx_secure_tls_remote_certificate_allocate(p_tls_session,
                                                       &p_tls->remote_certificate,
                                                       p_tls->remote_cert_buffer,
                                                       sizeof(p_tls->remote_cert_buffer));
    if (status != NX_SUCCESS) {
        Printf("  . Unable to allocate memory for remote certificate\r\n");
        Net_LOG_Write( 201, "%04X", status );               // ルート証明書読込エラー
        return( status );
    }

    // Allocate space for the interemediate CA certificate.
    memset(p_tls->remote_int_ca_cert_buffer, 0, sizeof(p_tls->remote_int_ca_cert_buffer));
    memset(p_tls->remote_intermediate_ca, 0, sizeof(p_tls->remote_intermediate_ca));
    for (i = 0; i < ARRAY_SIZE(p_tls->remote_intermediate_ca); i++)
    {
        status = nx_secure_tls_remote_certificate_allocate(p_tls_session,
                                                           &(p_tls->remote_intermediate_ca[i]),
                                                           &(p_tls->remote_int_ca_cert_buffer[i * CERTIFICATE_BUFFER_SIZE]),
                                                           CERTIFICATE_BUFFER_SIZE);
        if (NX_SUCCESS != status) {
            Printf("  . Unable to allocate memory for interemediate CA certificate\r\n");
            Net_LOG_Write( 201, "%04X", status );               // ルート証明書読込エラー
            return( status );
        }
    }

    // Add a CA Certificate to our trusted store for verifying incoming server certificates.
    status = nx_secure_x509_certificate_initialize(p_trusted_cert,
                                                   (uint8_t *)p_tls->p_root_ca, p_tls->root_ca_len,
                                                   NX_NULL, 0,
                                                   NULL, 0,
                                                   NX_SECURE_X509_KEY_TYPE_RSA_PKCS1_DER);
    if (NX_SUCCESS != status) {
        Printf("  . Unable to initialize CA certificate\r\n");
        Net_LOG_Write( 201, "%04X", status );            // ルート証明書読込エラー
        return( status );
    }
    status = nx_secure_tls_trusted_certificate_add(p_tls_session, p_trusted_cert);
    if (NX_SUCCESS != status) {
        Printf("  . Unable to add CA certificate to trusted store\r\n");
        Net_LOG_Write( 202, "%04X", status );            // サーバ証明書認証エラー
        return( status );
    }

    if (p_tls->p_device_cert != NULL && p_tls->device_cert_len != 0)
    {
        /* Add the local certificate for client authentication. */
        status = nx_secure_x509_certificate_initialize (p_cert,
                                               (uint8_t *) p_tls->p_device_cert, p_tls->device_cert_len,
                                               NX_NULL, 0,
                                               p_tls->p_private_key, p_tls->private_key_len,
                                               NX_SECURE_X509_KEY_TYPE_RSA_PKCS1_DER);
        if (NX_SUCCESS != status)
        {
            Printf("  . Unable to initialize device certificate\r\n");
            Net_LOG_Write( 201, "%04X", status );            // ルート証明書読込エラー
            return( status );
        }
        status = nx_secure_tls_local_certificate_add (p_tls_session, p_cert);
        if (NX_SUCCESS != status)
        {
            Printf("  . Unable to add device certificate to trusted store\r\n");
            Net_LOG_Write( 202, "%04X", status );            // サーバ証明書認証エラー
            return( status );
        }
    }

    /* Add a timestamp function for time checking and timestamps in the TLS handshake. */
    nx_secure_tls_session_time_function_set(p_tls_session, get_time);

    Printf("  . NX_HTTP_tls_setup  OK\r\n");

    return (NX_SUCCESS);
}




/*
UINT nx_web_http_client_secure_connect(NX_WEB_HTTP_CLIENT *client_ptr, NXD_ADDRESS *server_ip, UINT server_port,
                                       UINT (*tls_setup)(NX_WEB_HTTP_CLIENT *client_ptr, NX_SECURE_TLS_SESSION *),
                                       ULONG wait_option);
UINT nx_web_http_client_get_secure_start(NX_WEB_HTTP_CLIENT *client_ptr, NXD_ADDRESS *server_ip, UINT server_port, CHAR *resource,
                                         CHAR *host, CHAR *username, CHAR *password,
                                         UINT (*tls_setup)(NX_WEB_HTTP_CLIENT *client_ptr, NX_SECURE_TLS_SESSION *),
                                         ULONG wait_option);
*/


#ifdef NX_CRYPTO_ENGINE_SW
extern const NX_SECURE_TLS_CRYPTO nx_crypto_tls_ciphers;
#else
extern const NX_SECURE_TLS_CRYPTO nx_crypto_tls_ciphers_synergys7;
#endif

#include "my_certs_api_webstorage_jp.h"
//#include "my_certs_1.h"


#include "nx_api.h"
#include "http_thread.h"

//##########################################################################
//
//  HTTPの設定
//      which  0: HTTP
//      which  1: HTTPS
//
//##########################################################################
UINT my_https_setup(int which, int cert_index)                  //sakaguchi cg UT-0036
{
    UINT status = NX_SUCCESS;
    UINT    val = 0;
    int     iCnt = 0;
    int     len = 0;

    whc_tls_t* p_tls;
    srv_info_t* p_info;

    p_tls = &g_whc_tls;
    p_info = &g_whc_info;
    memset (&g_whc_info, 0, sizeof(g_whc_info));
    memset (&g_whc_tls, 0, sizeof(g_whc_tls));

    //-------------------------------------------------------------------------
    //----- HTTPS and HTTP stuff -------------------------------------------------------
    //-------------------------------------------------------------------------

    p_info->p_secure_client = &g_web_http_client0;


//    if(which == CONNECT_HTTPS)
//        p_info->server_port = 443;
//    else
//        p_info->server_port = 80;

    // HTTP通信 かつ プロキシ設定有効の場合は、プロキシ接続用のURL（フルパス）を作成する
    if( (CONNECT_HTTP == which) && (1 == my_config.network.ProxyHttp[0]) ){

        iCnt = 0;
        memset(S_cSrvApifull,0x00,sizeof(S_cSrvApifull));

        // サーバのホスト名
        iCnt += sprintf(&S_cSrvApifull[iCnt],"http://");
        len = (int)strlen(my_config.websrv.Server);
        if(len > (int)sizeof(my_config.websrv.Server))   len = (int)sizeof(my_config.websrv.Server);
        memcpy(&S_cSrvApifull[iCnt], my_config.websrv.Server, (size_t)len);
        iCnt = iCnt + len;

        // サーバのポート番号
        val = my_config.websrv.Port[0];
        val = val + (UINT)my_config.websrv.Port[1]*256;
        iCnt += sprintf(&S_cSrvApifull[iCnt],":%d",val);

        // サーバの接続先パス
        len = (int)strlen(my_config.websrv.Api);
        if(len > (int)sizeof(my_config.websrv.Api))   len = (int)sizeof(my_config.websrv.Api);
        memcpy(&S_cSrvApifull[iCnt], my_config.websrv.Api, (size_t)len);

        // サーバの接続先パス（フル）
        p_info->resource    = S_cSrvApifull;

        // プロキシサーバのポート番号
        val = my_config.network.ProxyHttpPort[0];
        val = val + (UINT)my_config.network.ProxyHttpPort[1]*256;
        p_info->server_port = val;

        // プロキシサーバのホスト名
        p_info->host        = my_config.network.ProxyHttpServ;

    }else{

        // サーバのポート番号
        val = my_config.websrv.Port[0];
        val = val + (UINT)my_config.websrv.Port[1]*256;
        p_info->server_port = val;

        // サーバの接続先パス
        p_info->resource    = my_config.websrv.Api;

        // サーバのホスト名
        p_info->host        = my_config.websrv.Server;

    }

//    val = my_config.websrv.Port[0];
//    val = val + (UINT)my_config.websrv.Port[1]*256;
//    p_info->server_port = val;

//    p_info->resource    = my_config.websrv.Api;
//    p_info->host        = my_config.websrv.Server;

    //p_info->resource = "/api/dev-tr4-1.php";
    //p_info->host = "develop.webstorage.jp";
    //p_info->resource = "index.html";
    //p_info->host = "192.168.10.222";

    p_info->username = 0; //""; // 0 or ""
    p_info->password = 0; //""; // 0 or ""


    //Printf("=====>>>>  mode %d der_len %ld  \r\n", my_config.websrv.Mode[0], my_config.websrv.CertSize1[0] + my_config.websrv.CertSize1[1]*256); 

    //-------------------------------------------------------------------------
    //----- TLS stuff ---------------------------------------------------------
    //-------------------------------------------------------------------------
    
    if(which == CONNECT_HTTPS )
    {
        p_tls->p_secure_client = &g_web_http_client0;

        if(my_config.websrv.Mode[0] == 2){      // webstorage
            ceat_file_read(0,cert_index);                                                    // cert_read  // sakaguchi add UT-0036
            p_tls->p_root_ca = (uint8_t *)      &CertFile_WS[2]; //DST_Root_CA_X3_der;       // CA_cert_der;
//            p_tls->root_ca_len = (uint16_t)CertFile_WS[0] + (uint16_t)CertFile_WS[1] * 256;
            p_tls->root_ca_len = *(uint16_t *)&CertFile_WS[0];
            //p_tls->root_ca_len = (uint16_t)(my_config.websrv.CertSizeW[0]);     //(my_config.websrv.CertSize1[0] + my_config.websrv.CertSize1[1]*256); //DST_Root_CA_X3_der_len;   // rootCA_len;
        }
        else{                                   // user site
            ceat_file_read(1,cert_index);                                                    // cert_read  // sakaguchi add UT-0036
            p_tls->p_root_ca = (uint8_t *)      &CertFile_USER[2];              // User Server
//            p_tls->root_ca_len = (uint16_t)CertFile_USER[0] + (uint16_t)CertFile_USER[1] * 256;
            p_tls->root_ca_len = *(uint16_t *)&CertFile_USER[0];
            //p_tls->root_ca_len = (uint16_t)(my_config.websrv.CertSizeU[0]);     //(my_config.websrv.CertSize2[0] + my_config.websrv.CertSize2[1]*256);    //DST_Root_CA_X3_der_len;   // rootCA_len;
        }
        
        //p_tls->p_root_ca = (uint8_t *)      server_der; //&CertFile_USER[0];          //DST_Root_CA_X3_der;       // CA_cert_der;
        //p_tls->root_ca_len = (uint16_t)     server_der_len ;//CertFileSize_USER;          //DST_Root_CA_X3_der_len;   // rootCA_len;

        p_tls->p_device_cert = (uint8_t *)  0;                          // dev_cert_der;
        p_tls->device_cert_len = (uint16_t) 0;                          // devCert_len;
        p_tls->p_private_key = (uint8_t *)  0;                          // private_key;
        p_tls->private_key_len = (uint16_t) 0;                          // priKey_len;
 
#ifdef NX_CRYPTO_ENGINE_SW
        p_tls->p_ciphers = (NX_SECURE_TLS_CRYPTO *) &nx_crypto_tls_ciphers;
#else
        p_tls->p_ciphers = (NX_SECURE_TLS_CRYPTO *) &nx_crypto_tls_ciphers_synergys7;
#endif
        p_tls->logMsg = (void *)printf; //TODO print_to_console;
    }

    Printf("my_https_setup  %02X\r\n", status);
    return( status );
}






//-----------------------------------------------------------------------------
// my_http_response_cb is our user callback for the nx_web_http_client_XXXX
// suite.
//-----------------------------------------------------------------------------
//VOID (*nx_web_http_client_response_callback)(struct NX_WEB_HTTP_CLIENT_STRUCT *client_ptr,
//                                             CHAR *field_name, UINT field_name_length,
//                                             CHAR *field_value, UINT field_value_length);

//#define RECEIVE_BUFFER_SIZE 2000
//#define RECEIVE_BUFFER_SIZE 1024*12
//#define RECEIVE_BUFFER_SIZE 1024*11
#define RECEIVE_BUFFER_SIZE 1024*5                      // sakaguchi cg
static char g_receive_buffer[RECEIVE_BUFFER_SIZE];
static char g_receive_buffer_back[RECEIVE_BUFFER_SIZE];

void my_http_response_cb( NX_WEB_HTTP_CLIENT *client_ptr,
        CHAR *field_name, UINT field_name_length,
        CHAR *field_value, UINT field_value_length);

void my_http_response_cb(NX_WEB_HTTP_CLIENT *client_ptr,
        CHAR *field_name, UINT field_name_length,
        CHAR *field_value, UINT field_value_length)
{
    UNUSED(client_ptr);

    field_name[field_name_length - 0] = 0;
    field_value[field_value_length - 0] = 0;

    Printf("http_cb name_len = %d, value_len = %d\r\n", field_name_length, field_value_length );
    Printf("http_cb name  = '%s'\r\n", field_name );
    Printf("http_cb value = '%s'\r\n", field_value );
    Printf("------------------------------------------------------\r\n");

    // リクエストID取得
//    if( 0 == memcmp(field_name,(char *)HD_Tandd_Request_ID,strlen(HD_Tandd_Request_ID))){
    if( ( 0 == memcmp(field_name,(char *)HD_Tandd_Request_ID,strlen(HD_Tandd_Request_ID)))
     || ( 0 == memcmp(field_name,(char *)HD_Tandd_Request_ID_lc,strlen(HD_Tandd_Request_ID_lc)))){      // 2022.11.28
        S_ulReqID = (uint32_t)atol(field_value);
    }

    // NextCommand取得
//    if( 0 == memcmp(field_name,(char *)HD_Tandd_Next_Command,strlen(HD_Tandd_Next_Command))){
    if( ( 0 == memcmp(field_name,(char *)HD_Tandd_Next_Command,strlen(HD_Tandd_Next_Command)))
     || ( 0 == memcmp(field_name,(char *)HD_Tandd_Next_Command_lc,strlen(HD_Tandd_Next_Command_lc)))){  // 2022.11.28
        if(USB_CONNECT == isUSBState()){
            // USB接続中はNextCommandは受け付けない
            S_ucNextCmd = 0;
        }else{
            // 無し
            if( 0 == memcmp(field_value,"none",strlen("none"))){
                S_ucNextCmd = 0;
            }
            // 次要求有り
            else if( 0 == memcmp(field_value,(char *)HD_Type_Operation_Request,strlen(HD_Type_Operation_Request))){
                S_ucNextCmd = 1;
            }
            //全設定取得
            else if( 0 == memcmp(field_value,(char *)HD_Type_Setting,strlen(HD_Type_Setting))){
                S_ucNextCmd = 2;
            }
        }
    }
}




//##########################################################################
//
//  TLSの設定
//
//##########################################################################
UINT my_tls_setup(void)
{
    UINT status;
    whc_tls_t* p_tls;

    p_tls = &g_whc_tls;


    //-------------------------------------------------------------------------
    /*
     * This is called during AutoInit
    _err = nx_web_http_client_create (&g_web_http_client0,
                                      "g_web_http_client0 HTTP Client",
                                      &g_ip0,
                                      &g_packet_pool0,
                                      1024);
    */

    // The  nx_web_http_client_create  call does not seem to initialize the
    // tls session, so we do it manually
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //  NX_SUCCESS (0x00) Successful initialization of the TLS session.
    //  NX_PTR_ERROR (0x07) Tried to use an invalid pointer.
    //  NX_INVALID_PARAMETERS (0x4D) The metadata buffer was too small for the given methods.
    //  NX_SECURE_TLS_UNSUPPORTED_CIPHER (0x106) A required cipher method for the enabled version of TLS was not supplied in the cipher table.
    
    status = nx_secure_tls_session_create(&(p_tls->p_secure_client->nx_web_http_client_tls_session),
                                          p_tls->p_ciphers,
                                          p_tls->crypto_metadata,
                                          sizeof(p_tls->crypto_metadata));


    if(status != NX_SUCCESS) {
        Printf("nx_secure_tls_session_create Failed. status = 0x%x\r\n", status);
        Net_LOG_Write( 200, "%04X", status );                      // TLSセッション生成エラー
        DebugLog( LOG_DBG, "nx_secure_tls_session_create (%04X)", status);
        return( status );
    }

// sakaguchi add UT-0036 ↓
    // If ECC Ciphersuite support is enabled, the nx_secure_tls_ecc_initialize API function must be called in the application code after the TLS session is created.
    status = nx_secure_tls_ecc_initialize(&(p_tls->p_secure_client->nx_web_http_client_tls_session),
                                        nx_crypto_ecc_supported_groups_synergys7,
                                        nx_crypto_ecc_supported_groups_synergys7_size,
                                        nx_crypto_ecc_curves_synergys7);
// sakaguchi add UT-0036 ↑

    if(status != NX_SUCCESS) {
        DebugLog( LOG_DBG, "nx_secure_tls_ecc_initialize (%04X)", status);
    }
    //-------------------------------------------------------------------------
    // Allocate space for packet re-assembly.
    status = nx_secure_tls_session_packet_buffer_set(&(p_tls->p_secure_client->nx_web_http_client_tls_session),
                                                     p_tls->tls_packet_buffer,
                                                     sizeof(p_tls->tls_packet_buffer));
    if(status != NX_SUCCESS) {
        Printf("nx_secure_tls_session_packet_buffer_set Failed. status = 0x%x\r\n", status);

        nx_secure_tls_session_delete(&(p_tls->p_secure_client->nx_web_http_client_tls_session));
      //TODO nxd_http_client_delete(p_tls->p_secure_client);
        Net_LOG_Write( 200, "%04X", status );                      // TLSセッション生成エラー
        DebugLog( LOG_DBG, "nx_secure_tls_session_packet_buffer_set (%04X)", status);
        return( status );
    }

    Printf("nx_secure_tls_session_create  status = 0x%x\r\n", status);
    return(status);
}

// NX_NOT_CONNECTED                           0x38(56)


#if 0
//===========================================================================
// 機 能 : HTTPによるファイルアップロード
// 引 数 : which == CONNECT_HTTPS  or CONNECT_HTTP
// 返 値 : 0: Ok  Other: Error
//===========================================================================
UINT my_https_POST(int which)
{
    UINT status = NX_SUCCESS;

    NX_WEB_HTTP_CLIENT  *client_ptr;

    ULONG                wait_option;
    ULONG                wait_option2;
    UINT                 method;    // eg GET
//    UINT                 input_size;
    UINT                 transfer_encoding_chunked;
    size_t xsize, len;
    char temp[128];


    srv_info_t* p_info;
    p_info = &g_whc_info;

    //-------------------------------------------------------------------------
    client_ptr = p_info->p_secure_client;

    //-------------------------------------------------------------------------
    // Set Callback for HTTP responses
    client_ptr->nx_web_http_client_response_callback = my_http_response_cb;

    //method = NX_WEB_HTTP_METHOD_PUT;        //
 //   input_size = 0;
    transfer_encoding_chunked = 0;          //false
    //wait_option = 500;
    wait_option = 2500;
    wait_option2 = 5000;

    //-------------------------------------------------------------------------
    //----- Find the IP address of the host -----------------------------------
    //-------------------------------------------------------------------------
    Printf(" ========>>>>  find host ip address  [[[ %ld ]]] >>> %ld <<<\r\n", http_count,success_count);
    status = find_ip_address( p_info );
    if( status != NX_SUCCESS)
    {
        PutLog(LOG_LAN, "HTTP DNS Error %d", status);
        return (NX_DNS_QUERY_FAILED);           // 2020 03 03
    }

    //tx_thread_sleep(50);

    if(which == CONNECT_HTTPS){
        //-------------------------------------------------------------------------
        //----- Secure Connect to Host --------------------------------------------
        //-------------------------------------------------------------------------
        Printf("  . nx_web_http_client_secure_connect\r\n");
        status = nx_web_http_client_secure_connect( client_ptr, &p_info->server_ip, p_info->server_port, NX_HTTP_tls_setup, wait_option2);
  
        if(status != NX_SUCCESS) {
            PutLog(LOG_LAN, "HTTP Connect Error %d (%ld)", status ,http_count);
            Printf("    nx_web_http_client_secure_connect Failed. status = %04X  %ld\r\n", status, http_count);
            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
            my_tls_close(client_ptr,which);
            return( status);
        }

        Printf("  . nx_web_http_client_secure_connect %08X\r\n", status);
    }
    else{
        //-------------------------------------------------------------------------
        //----- Connect to Host --------------------------------------------
        //-------------------------------------------------------------------------
        Printf("  . nx_web_http_client_connect\r\n");
        status = nx_web_http_client_connect( client_ptr, &p_info->server_ip, p_info->server_port, wait_option);
  
        if((status != NX_SUCCESS) && (status != NX_ALREADY_BOUND)){
            PutLog(LOG_LAN, "HTTP Connect Error %d", status);
            Printf("    nx_web_http_client_connect Failed. status = %08X\r\n", status);
            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
            return( status);
        }

        Printf("  . nx_web_http_client_connect %02X\r\n", status);        
    }

    // 送信ファイルサイズ
    xsize = strlen(xml_buffer);


    //-------------------------------------------------------------------------
    //----- xxx -----
    //-------------------------------------------------------------------------
    method = NX_WEB_HTTP_METHOD_POST; 
    Printf("  . nx_web_http_client_request_initialize\r\n");
    status = nx_web_http_client_request_initialize(client_ptr, method,
                                                   p_info->resource,
                                                   p_info->host,
                                                   xsize,
                                                   transfer_encoding_chunked,
                                                   p_info->username, p_info->password,
                                                   wait_option);
    if(status != NX_SUCCESS)
    {
        PutLog(LOG_LAN, "HTTP initialize Error %d", status);

        Printf("nx_web_http_client_request_initialize Failed. status = 0x%x\r\n", status);
        //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
        my_tls_close(client_ptr,which);
        return( status);
    }



    
    sprintf( temp, "%04lX", fact_config.SerialNumber);
    len = strlen(temp);

    status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Content_Type,  strlen(HD_Content_Type), (char *)HD_Content_Type_XML,  strlen(HD_Content_Type_XML), wait_option);
 
    status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_User_Agent,  strlen(HD_User_Agent), (char *)HD_User_Agent_D,  strlen(HD_User_Agent_D), wait_option);

    status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Api,  strlen(HD_Tandd_Api), (char *)HD_Tandd_Api_D,  strlen(HD_Tandd_Api_D), wait_option);

    switch (HttpFile)
    {
    case FILE_M:
        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Request_Type,  strlen(HD_Tandd_Request_Type), (char *)HD_Type_MonitorData,  strlen(HD_Type_MonitorData), wait_option);
        break;
    case FILE_S:
        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Request_Type,  strlen(HD_Tandd_Request_Type), (char *)HD_Type_RecordData,  strlen(HD_Type_RecordData), wait_option);
        break;
    default:
        break;
    }

   

    // 親機シリアル番号
    status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Serial,  strlen(HD_Tandd_Serial), temp,  len, wait_option);

    Printf("nx_web_http_client_request_header_add. status = 0x%x  %d\r\n", status);

    //-------------------------------------------------------------------------
    //----- Send the Header -----
    //-------------------------------------------------------------------------
    Printf("  . nx_web_http_client_request_send (Send Header)\r\n");
    status = nx_web_http_client_request_send(client_ptr, wait_option);
    if(status != NX_SUCCESS)
    {
        PutLog(LOG_LAN, "HTTP request send Error %d", status);
        Printf("nx_web_http_client_request_send Failed. status = 0x%x\r\n", status);
        //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
        my_tls_close(client_ptr,which);
        return( status);
    }

    //-------------------------------------------------------------------------
    //----- Send the Body -----
    //-------------------------------------------------------------------------
    NX_PACKET *p_http_post_pkt;
    status =  nx_packet_allocate(&g_packet_pool1, &p_http_post_pkt, NX_TCP_PACKET, NX_WAIT_FOREVER);
    if(status != NX_SUCCESS)
    {
        Printf("nx_packet_allocate Failed. status = 0x%x\r\n", status);
        //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
        my_tls_close(client_ptr,which);
        return( status);
    }

    //tx_thread_sleep(50);
    status = send_file(client_ptr, p_http_post_pkt, xml_buffer, xsize);
    if(status != NX_SUCCESS)
    {
        PutLog(LOG_LAN, "HTTP body send Error %d", status);
        Printf("      send_file  Failed. status = 0x%x\r\n", status);
        //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
        my_tls_close(client_ptr,which);
        return( status);
    }

    nx_packet_release(p_http_post_pkt);

   
    //-------------------------------------------------------------------------
    //----- Get response ------------------------------------------------------
    //-------------------------------------------------------------------------
    //tx_thread_sleep(50);
    status = get_web_http_client_response_body( client_ptr );

    if(status == NX_SUCCESS)
        success_count ++;

    my_tls_close(client_ptr,which);
    return (status);
}
#endif

//===========================================================================
// 機 能 : HTTPによるファイルアップロード
// 引 数 : which == CONNECT_HTTPS  or CONNECT_HTTP
// 返 値 : 0: Ok  Other: Error
//===========================================================================
UINT my_https_POST(int which)
{
    UINT status = NX_SUCCESS;
    UINT status2 = NX_SUCCESS;

    NX_WEB_HTTP_CLIENT  *client_ptr;

    ULONG                wait_option;
    ULONG                wait_option2;
    UINT                 method;
    UINT                 transfer_encoding_chunked;
    size_t xsize, len;
    char temp[128];
    int ifile;
    int icmd;
    srv_info_t* p_info;
    p_info = &g_whc_info;
    NX_PACKET *p_http_post_pkt;
    int iRet;
    uint32_t    actual_events;                              // sakaguchi add 2020.08.28

    rtc_time_t ct;
    // 2021.01.20
    ULONG total_packets;
    ULONG free_packets;
    ULONG empty_pool_requests;
    ULONG empty_pool_suspensions;
    ULONG invalid_packet_releases;

    //g_rtc.p_api->calendarTimeGet(g_rtc.p_ctrl, &ct);
    with_retry_calendarTimeGet("my_https_POST", &ct);       // 2022.09.09

    //-------------------------------------------------------------------------
    client_ptr = p_info->p_secure_client;

    //-------------------------------------------------------------------------
    // Set Callback for HTTP responses
    client_ptr->nx_web_http_client_response_callback = my_http_response_cb;

    transfer_encoding_chunked = 0;          //false
    wait_option = 2500;
    //wait_option2 = 5000;
    //wait_option2 = 10000;
    //wait_option2 = 1000;
    wait_option2 = 2500;                    // 500 -> 2500  // sakaguchi 2021.05.28 HTTPと合わせる

    p_info->server_ip.nxd_ip_address.v4 = target_ip.nxd_ip_address.v4;
    p_info->server_ip.nxd_ip_version = 4;

    //-------------------------------------------------------------------------
    //----- Find the IP address of the host -----------------------------------
    //-------------------------------------------------------------------------
    Printf(" ========>>>>  find host ip address  [[[ %ld ]]] >>> %ld <<< (%ld)\r\n", http_count,success_count, http_error);
    Printf("   dns start: %d/%d/%d %d:%d:%d \r\n", ct.tm_year,ct.tm_mon,ct.tm_mday,ct.tm_hour,ct.tm_min,ct.tm_sec);
    status = find_ip_address( p_info );
    //status = find_ip_address_1_2( p_info );     // 2020.10.23  NG
    //g_rtc.p_api->calendarTimeGet(g_rtc.p_ctrl, &ct);
    with_retry_calendarTimeGet("my_https_POST2", &ct);      // 2022.09.09
    Printf("   dns stop : %d/%d/%d %d:%d:%d \r\n", ct.tm_year,ct.tm_mon,ct.tm_mday,ct.tm_hour,ct.tm_min,ct.tm_sec);

    if( status != NX_SUCCESS)
    {
        PutLog(LOG_LAN, "HTTP DNS Error %d", status);
        vhttp_cmdflg_allreset(E_NG);                    //sakaguchi add 20/04/01
        Net_LOG_Write( 102, "%04X", status );           // DNSエラー
//        my_tls_session_end();                           // sakaguchi 2020.09.02   // sakaguchi 2020.09.03 del
        return (status);
    }

    //tx_thread_sleep(50);

    // プロキシサーバ経由の場合、ホスト名はwebサーバ名に変更する。送信先IPアドレスはプロキシサーバのまま。
    if( (CONNECT_HTTP == which) && (1 == my_config.network.ProxyHttp[0]) ){
        p_info->host = my_config.websrv.Server;
    }

    // HTTP通信発生時はログ送信を行う
    //if( VENDER_HIT != fact_config.Vender){    // Hitachi以外
    if((VENDER_HIT != fact_config.Vender) && (Http_Use == HTTP_SEND)){	// sakaguchi 2020.12.23
        if(SND_ON != G_HttpFile[FILE_L_T].sndflg){  // ログ送信テスト中以外
// 2023.08.01 ↓
            if( LOG_SEND_CNT <= GetLogCnt_http(0)){     // ログが10件以上溜まっている場合
                G_HttpFile[FILE_L].sndflg = SND_ON;         // ログ送信ＯＮ
            }
// 2023.08.01 ↑        
        }
    }


    if(which == CONNECT_HTTPS){
        //-------------------------------------------------------------------------
        //----- Secure Connect to Host --------------------------------------------
        //-------------------------------------------------------------------------
        //  通常は、500ms程度で connectionが完了する
        //  NX_NO_PACKET (0x01) No packet available
        //
        Printf("  . nx_web_http_client_secure_connect\r\n");

        status = nx_web_http_client_secure_connect( client_ptr, &p_info->server_ip, p_info->server_port, NX_HTTP_tls_setup, wait_option2);

        if(status != NX_SUCCESS) {
            PutLog(LOG_LAN, "HTTP Connect Error %d", status);
            Printf("    nx_web_http_client_secure_connect Failed. status = %04X  %ld\r\n", status, http_count);
            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
            //  要らないかも 2020 04 03 my_tls_close(client_ptr,which);
			//vhttp_cmdflg_allreset(E_NG);                    //sakaguchi del UT-0039
// sakaguchi 2021.02.16 ↓
            // 証明書に関するエラーは証明書を切り替えて再送するため、ここでは要求フラグをクリアしない
            switch(status){
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
                    // 処理無し
                    break;
                default:
                    vhttp_cmdflg_allreset(E_NG);                    // HTTP要求を全てクリア
                    break;
            }
// sakaguchi 2021.02.16 ↑
            return( status);
        }

        Printf("  . nx_web_http_client_secure_connect %08X\r\n", status);
    }
    else{
        //-------------------------------------------------------------------------
        //----- Connect to Host --------------------------------------------
        //-------------------------------------------------------------------------
        Printf("  . nx_web_http_client_connect\r\n");
        status = nx_web_http_client_connect( client_ptr, &p_info->server_ip, p_info->server_port, wait_option);

        if((status != NX_SUCCESS) && (status != NX_ALREADY_BOUND)){
            PutLog(LOG_LAN, "HTTP Connect Error %d", status);
            Printf("    nx_web_http_client_connect Failed. status = %08X\r\n", status);
            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
            vhttp_cmdflg_allreset(E_NG);                    //sakaguchi add 20/04/01
            Net_LOG_Write( 200, "%04X", status );           // HTTPセッションエラー
            return( status);
        }

        Printf("  . nx_web_http_client_connect %02X\r\n", status);
    }

    for(;;){

        //-------------------------------------------------------------------------
        //----- 送信ファイルのチェック            -----------------------------------
        //-------------------------------------------------------------------------
        for( ifile=0; ifile<HTTP_FILE_MAX; ifile++ )
        {
            if( SND_ON == G_HttpFile[ifile].sndflg )
            {
                Printf("    Send File Type %ld \r\n", ifile);
                switch( ifile )
                {
                    //-------------------------------------------------------------------------
                    //----- 警報                            -----------------------------------
                    //-------------------------------------------------------------------------
                    case FILE_W:
                        //----- send file size -----
                        xsize = strlen(xml_buffer);

                        //----- client request initialize-----
                        method = NX_WEB_HTTP_METHOD_POST;
                        Printf("  . nx_web_http_client_request_initialize(%d)\r\n",ifile);
                        status = nx_web_http_client_request_initialize(client_ptr, method,
                                                                    p_info->resource,
                                                                    p_info->host,
                                                                    xsize,
                                                                    transfer_encoding_chunked,
                                                                    p_info->username, p_info->password,
                                                                    wait_option);
                        if(status != NX_SUCCESS)
                        {
                            PutLog(LOG_LAN, "HTTP initialize Error %d(%d)", status, ifile);

                            Printf("nx_web_http_client_request_initialize Failed. status = 0x%x (%d)\r\n", status, ifile);
                            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                            my_tls_close(client_ptr,which);
                            vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                            Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー
                            return( status);
                        }
                        sprintf( temp, "%04lX", fact_config.SerialNumber);
                        len = strlen(temp);

                        //----- request header -----
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Content_Type,  strlen(HD_Content_Type), (char *)HD_Content_Type_XML,  strlen(HD_Content_Type_XML), wait_option);
// 2023.05.31 ↓
                        if(fact_config.Vender == VENDER_ESPEC){
                            status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_User_Agent,  strlen(HD_User_Agent), (char *)HD_User_Agent_E,  strlen(HD_User_Agent_E), wait_option);
                        }else{
                            status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_User_Agent,  strlen(HD_User_Agent), (char *)HD_User_Agent_D,  strlen(HD_User_Agent_D), wait_option);
                        }
// 2023.05.31 ↑
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Api,  strlen(HD_Tandd_Api), (char *)HD_Tandd_Api_D,  strlen(HD_Tandd_Api_D), wait_option);
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Request_Type,  strlen(HD_Tandd_Request_Type), (char *)HD_Type_Warnig,  strlen(HD_Type_Warnig), wait_option);
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Serial,  strlen(HD_Tandd_Serial), temp,  len, wait_option);

                        //----- setthing count -----
                        sprintf(temp, "%lu", my_config.device.SysCnt);
                        len = strlen(temp);
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Setting_SysCnt, strlen(HD_Tandd_Setting_SysCnt), temp, len, wait_option);

                        sprintf( temp, "%lu", my_config.device.RegCnt);
                        len = strlen(temp);
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Setting_RegCnt, strlen(HD_Tandd_Setting_RegCnt), temp, len, wait_option);

                        Printf("nx_web_http_client_request_header_add. status = 0x%x  %d\r\n", status);

                        //-------------------------------------------------------------------------
                        //----- Send the Header -----
                        //-------------------------------------------------------------------------
                        Printf("  . nx_web_http_client_request_send (Send Header)\r\n");
                        status = nx_web_http_client_request_send(client_ptr, wait_option);
                        if(status != NX_SUCCESS)
                        {
                            PutLog(LOG_LAN, "HTTP request send Error %d(%d)", status, ifile);
                            Printf("nx_web_http_client_request_send Failed. status = 0x%x\r\n", status);
                            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                            my_tls_close(client_ptr,which);
                            vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                            Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー
                            return(status);
                        }

                        //-------------------------------------------------------------------------
                        //----- Send the Body -----
                        //-------------------------------------------------------------------------
                        status =  nx_packet_allocate(&g_packet_pool1, &p_http_post_pkt, NX_TCP_PACKET, 100 /*NX_WAIT_FOREVER*/);    // 2023.03.01
                        if(status != NX_SUCCESS)
                        {
                            Printf("nx_packet_allocate Failed. status = 0x%x\r\n", status);
                            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                            my_tls_close(client_ptr,which);
                            vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                            Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー
                            return(status);
                        }

                        status = send_file(client_ptr, p_http_post_pkt, xml_buffer, xsize);

                        if(status != NX_SUCCESS)
                        {
                            PutLog(LOG_LAN, "HTTP body send Error %d(%d)", status, ifile);
                            Printf("      send_file  Failed. status = 0x%x\r\n", status);
                            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                            status2 = nx_packet_release(p_http_post_pkt);
                            Printf("nx_packet_release 1 0x%x\r\n", status2);
                            my_tls_close(client_ptr,which);
                            vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                            Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー
                            return(status);
                        }

                        status = nx_packet_release(p_http_post_pkt);
                        Printf("nx_packet_release 2 0x%x\r\n", status);

                        //-------------------------------------------------------------------------
                        //----- Get response ------------------------------------------------------
                        //-------------------------------------------------------------------------
                        //tx_thread_sleep(50);
                        status = get_web_http_client_response_body( client_ptr );

                        if(status == NX_SUCCESS){
                            success_count ++;
                            FirstAlarmMoni = 0;                             // 送信が成功した場合、警報監視１回目フラグはＯＦＦする（失敗した場合、フラグはＯＮのままで再送させる）   // sakaguchi 2021.11.25
                            if( true == bhttp_srvcmd_analyze( ifile, g_receive_buffer ) ){
                                vhttp_cmdflg_reset( ifile, E_OK );          // 正常終了
                            }else{
                                vhttp_cmdflg_reset( ifile, E_NG );          // 異常終了
                            }
                            Net_LOG_Write( 1, "%04X", status );             // WebStorage通信成功

                        }else{
                            vhttp_cmdflg_reset(ifile,E_NG);                 // 異常終了
                            if(status == NX_NO_PACKET){
                                Net_LOG_Write( 105, "%04X", status );       // WebStorage通信失敗（タイムアウト）
                            }else{
                                if(status == NX_WEB_HTTP_STATUS_CODE_NOT_ACCEPTABLE){   // 406 Not Acceptable       // sakaguchi 2020.08.31
                                    Net_LOG_Write( 2, "%04X", NX_SUCCESS );             // WebStorage通信成功（機器登録無し）
                                }
// sakaguchi 2020.12.23 Data Server 412対応 ↓
                                else if(status == NX_WEB_HTTP_STATUS_CODE_PRECONDITION_FAILED){   // 412 Precondition Failed（アクセス拒否）
                                    G_HttpFile[FILE_C].reqid = S_ulReqID;                               // [FILE]リクエストID格納
                                    G_HttpFile[FILE_C].sndflg = SND_ON;                                 // [FILE]機器設定：送信有りをセット
                                    G_HttpCmd[HTTP_CMD_RSETF].sndflg = SND_ON;                          // 登録情報送信
                                    vhttp_sysset_sndon();                                               // 親機設定送信
                                    tx_event_flags_set (&event_flags_http, FLG_HTTP_READY, TX_OR);      // HTTPスレッド動作許可ON
                                    Net_LOG_Write( 1, "%04X", status );                                 // WebStorage通信成功
                                }
// sakaguchi 2020.12.23 ↑
                                else{
                                Net_LOG_Write( 104, "%04X", ulchg_http_status_code(status) );       // WebStorage通信失敗（エラー）
                            }
                            }
                            return(status);
                        }
                        break;



                    //-------------------------------------------------------------------------
                    //----- 記録データ（吸い上げ）            -----------------------------------
                    //-------------------------------------------------------------------------
                    case FILE_S:
                        //----- send file size -----
                        xsize = strlen(xml_buffer);

                        //----- client request initialize-----
                        method = NX_WEB_HTTP_METHOD_POST;
                        Printf("  . nx_web_http_client_request_initialize(%d)\r\n",ifile);
                        status = nx_web_http_client_request_initialize(client_ptr, method,
                                                                    p_info->resource,
                                                                    p_info->host,
                                                                    xsize,
                                                                    transfer_encoding_chunked,
                                                                    p_info->username, p_info->password,
                                                                    wait_option);
                        if(status != NX_SUCCESS)
                        {
                            PutLog(LOG_LAN, "HTTP initialize Error %d(%d)", status, ifile);

                            Printf("nx_web_http_client_request_initialize Failed. status = 0x%x (%d)\r\n", status,ifile);
                            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                            my_tls_close(client_ptr,which);
                            vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                            Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー
                            return( status);
                        }
                        sprintf( temp, "%04lX", fact_config.SerialNumber);
                        len = strlen(temp);

                        //----- request header -----
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Content_Type,  strlen(HD_Content_Type), (char *)HD_Content_Type_XML,  strlen(HD_Content_Type_XML), wait_option);
// 2023.05.31 ↓
                        if(fact_config.Vender == VENDER_ESPEC){
                            status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_User_Agent,  strlen(HD_User_Agent), (char *)HD_User_Agent_E,  strlen(HD_User_Agent_E), wait_option);
                        }else{
                            status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_User_Agent,  strlen(HD_User_Agent), (char *)HD_User_Agent_D,  strlen(HD_User_Agent_D), wait_option);
                        }
// 2023.05.31 ↑
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Api,  strlen(HD_Tandd_Api), (char *)HD_Tandd_Api_D,  strlen(HD_Tandd_Api_D), wait_option);
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Request_Type,  strlen(HD_Tandd_Request_Type), (char *)HD_Type_RecordData,  strlen(HD_Type_RecordData), wait_option);
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Serial,  strlen(HD_Tandd_Serial), temp,  len, wait_option);

                        //----- setthing count -----
                        sprintf(temp, "%lu", my_config.device.SysCnt);
                        len = strlen(temp);
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Setting_SysCnt, strlen(HD_Tandd_Setting_SysCnt), temp, len, wait_option);

                        sprintf( temp, "%lu", my_config.device.RegCnt);
                        len = strlen(temp);
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Setting_RegCnt, strlen(HD_Tandd_Setting_RegCnt), temp, len, wait_option);

                        Printf("nx_web_http_client_request_header_add. status = 0x%x  %d\r\n", status);

                        //-------------------------------------------------------------------------
                        //----- Send the Header -----
                        //-------------------------------------------------------------------------
                        Printf("  . nx_web_http_client_request_send (Send Header)\r\n");
                        status = nx_web_http_client_request_send(client_ptr, wait_option);
                        if(status != NX_SUCCESS)
                        {
                            PutLog(LOG_LAN, "HTTP request send Error %d(%d)", status, ifile);
                            Printf("nx_web_http_client_request_send Failed. status = 0x%x\r\n", status);
                            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                            my_tls_close(client_ptr,which);
                            vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                            Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー
                            return(status);
                        }

                        //-------------------------------------------------------------------------
                        //----- Send the Body -----
                        //-------------------------------------------------------------------------
                        status =  nx_packet_allocate(&g_packet_pool1, &p_http_post_pkt, NX_TCP_PACKET, 100 /*NX_WAIT_FOREVER*/);    // 2023.03.01
                        if(status != NX_SUCCESS)
                        {
                            Printf("nx_packet_allocate Failed. status = 0x%x\r\n", status);
                            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                            my_tls_close(client_ptr,which);
                            vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                            Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー
                            return(status);
                        }

                        status = send_file(client_ptr, p_http_post_pkt, xml_buffer, xsize);

                        if(status != NX_SUCCESS)
                        {
                            PutLog(LOG_LAN, "HTTP body send Error %d(%d)", status, ifile);
                            Printf("      send_file  Failed. status = 0x%x\r\n", status);
                            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                            status2 = nx_packet_release(p_http_post_pkt);
                            Printf("nx_packet_release 3 0x%x\r\n", status2);
                            my_tls_close(client_ptr,which);
                            vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                            Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー
                            return(status);
                        }

                        status = nx_packet_release(p_http_post_pkt);
                        Printf("nx_packet_release 4 0x%x\r\n", status);

                        //-------------------------------------------------------------------------
                        //----- Get response ------------------------------------------------------
                        //-------------------------------------------------------------------------
                        //tx_thread_sleep(50);
                        status = get_web_http_client_response_body( client_ptr );

                        if(status == NX_SUCCESS){
                            success_count ++;

                            if( true == bhttp_srvcmd_analyze( ifile, g_receive_buffer ) ){
                                vhttp_cmdflg_reset( ifile, E_OK );          // 正常終了
                            }else{
                                vhttp_cmdflg_reset( ifile, E_NG );          // 異常終了
                            }
                            Net_LOG_Write( 1, "%04X", status );             // WebStorage通信成功

                        }else{
                            vhttp_cmdflg_reset(ifile,E_NG);                 //異常終了
                            if(status == NX_NO_PACKET){
                                Net_LOG_Write( 105, "%04X", status );       // WebStorage通信失敗（タイムアウト）
                            }else{
                                if(status == NX_WEB_HTTP_STATUS_CODE_NOT_ACCEPTABLE){   // 406 Not Acceptable       // sakaguchi 2020.08.31
                                    Net_LOG_Write( 2, "%04X", NX_SUCCESS );             // WebStorage通信成功（機器登録無し）
                                }
// sakaguchi 2020.12.23 Data Server 412対応 ↓
                                else if(status == NX_WEB_HTTP_STATUS_CODE_PRECONDITION_FAILED){   // 412 Precondition Failed（アクセス拒否）
                                    G_HttpFile[FILE_C].reqid = S_ulReqID;                               // [FILE]リクエストID格納
                                    G_HttpFile[FILE_C].sndflg = SND_ON;                                 // [FILE]機器設定：送信有りをセット
                                    G_HttpCmd[HTTP_CMD_RSETF].sndflg = SND_ON;                          // 登録情報送信
                                    vhttp_sysset_sndon();                                               // 親機設定送信
                                    tx_event_flags_set (&event_flags_http, FLG_HTTP_READY, TX_OR);      // HTTPスレッド動作許可ON
                                    Net_LOG_Write( 1, "%04X", status );                                 // WebStorage通信成功
                                }
// sakaguchi 2020.12.23 ↑
                                else{
                                Net_LOG_Write( 104, "%04X", ulchg_http_status_code(status) );       // WebStorage通信失敗（エラー）
                            }
                            }
                            return(status);
                        }
                        break;



                    //-------------------------------------------------------------------------
                    //----- 現在値データ（モニタ）            -----------------------------------
                    //-------------------------------------------------------------------------
                    case FILE_M:
                        //----- send file size -----
                        xsize = strlen(xml_buffer);

                        //----- client request initialize-----
                        method = NX_WEB_HTTP_METHOD_POST;
                        Printf("  . nx_web_http_client_request_initialize(%d)\r\n",ifile);
                        status = nx_web_http_client_request_initialize(client_ptr, method,
                                                                    p_info->resource,
                                                                    p_info->host,
                                                                    xsize,
                                                                    transfer_encoding_chunked,
                                                                    p_info->username, p_info->password,
                                                                    wait_option);
                        if(status != NX_SUCCESS)
                        {
                            PutLog(LOG_LAN, "HTTP initialize Error %d(%d)", status, ifile);

                            Printf("nx_web_http_client_request_initialize Failed. status = 0x%x (%d)\r\n", status,ifile);
                            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                            my_tls_close(client_ptr,which);
                            vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                            Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー
                            return( status);
                        }
                        sprintf( temp, "%04lX", fact_config.SerialNumber);
                        len = strlen(temp);

                        //----- request header -----
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Content_Type,  strlen(HD_Content_Type), (char *)HD_Content_Type_XML,  strlen(HD_Content_Type_XML), wait_option);
// 2023.05.31 ↓
                        if(fact_config.Vender == VENDER_ESPEC){
                            status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_User_Agent,  strlen(HD_User_Agent), (char *)HD_User_Agent_E,  strlen(HD_User_Agent_E), wait_option);
                        }else{
                            status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_User_Agent,  strlen(HD_User_Agent), (char *)HD_User_Agent_D,  strlen(HD_User_Agent_D), wait_option);
                        }
// 2023.05.31 ↑
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Api,  strlen(HD_Tandd_Api), (char *)HD_Tandd_Api_D,  strlen(HD_Tandd_Api_D), wait_option);
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Request_Type,  strlen(HD_Tandd_Request_Type), (char *)HD_Type_MonitorData,  strlen(HD_Type_MonitorData), wait_option);
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Serial,  strlen(HD_Tandd_Serial), temp,  len, wait_option);             //親機シリアル番号

                        //----- setthing count -----
                        sprintf(temp, "%lu", my_config.device.SysCnt);
                        len = strlen(temp);
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Setting_SysCnt, strlen(HD_Tandd_Setting_SysCnt), temp, len, wait_option);

                        sprintf( temp, "%lu", my_config.device.RegCnt);
                        len = strlen(temp);
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Setting_RegCnt, strlen(HD_Tandd_Setting_RegCnt), temp, len, wait_option);

                        Printf("nx_web_http_client_request_header_add. status = 0x%x  %d\r\n", status);

                        //-------------------------------------------------------------------------
                        //----- Send the Header -----
                        //-------------------------------------------------------------------------
                        Printf("  . nx_web_http_client_request_send (Send Header)\r\n");
                        status = nx_web_http_client_request_send(client_ptr, wait_option);
                        if(status != NX_SUCCESS)
                        {
                            PutLog(LOG_LAN, "HTTP request send Error %d(%d)", status, ifile);
                            Printf("nx_web_http_client_request_send Failed. status = 0x%x\r\n", status);
                            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                            my_tls_close(client_ptr,which);
                            vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                            Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー
                            return(status);
                        }

                        //-------------------------------------------------------------------------
                        //----- Send the Body -----
                        //-------------------------------------------------------------------------
                        status =  nx_packet_allocate(&g_packet_pool1, &p_http_post_pkt, NX_TCP_PACKET, 100 /*NX_WAIT_FOREVER*/);    // 2023.03.01
                        if(status != NX_SUCCESS)
                        {
                            Printf("nx_packet_allocate Failed. status = 0x%x\r\n", status);
                            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                            my_tls_close(client_ptr,which);
                            vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                            Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー
                            return(status);
                        }

                        status = send_file(client_ptr, p_http_post_pkt, xml_buffer, xsize);

                        if(status != NX_SUCCESS)
                        {
                            PutLog(LOG_LAN, "HTTP body send Error %d(%d)", status, ifile);
                            Printf("      send_file  Failed. status = 0x%x\r\n", status);
                            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                            status2 = nx_packet_release(p_http_post_pkt);
                            Printf("nx_packet_release 5 0x%x\r\n", status2);
                            my_tls_close(client_ptr,which);
                            vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                            Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー
                            return(status);
                        }
 #ifdef DBG_TERM                       
                        nx_packet_pool_info_get(&g_packet_pool1,&total_packets, &free_packets, &empty_pool_requests, &empty_pool_suspensions, &invalid_packet_releases);
                        Printf("nx_packet_pool_info_get  %ld %ld %ld %ld %ld \r\n", total_packets, free_packets, empty_pool_requests, empty_pool_suspensions, invalid_packet_releases);
#endif
                        status = nx_packet_release(p_http_post_pkt);
 #ifdef DBG_TERM                       
                        Printf("nx_packet_release 6 0x%x\r\n", status);
                        nx_packet_pool_info_get(&g_packet_pool1,&total_packets, &free_packets, &empty_pool_requests, &empty_pool_suspensions, &invalid_packet_releases);
                        Printf("nx_packet_pool_info_get  %ld %ld %ld %ld %ld \r\n", total_packets, free_packets, empty_pool_requests, empty_pool_suspensions, invalid_packet_releases);
#endif
                        //-------------------------------------------------------------------------
                        //----- Get response ------------------------------------------------------
                        //-------------------------------------------------------------------------
                        //tx_thread_sleep(50);
                        status = get_web_http_client_response_body( client_ptr );

                        if(status == NX_SUCCESS){
                            success_count ++;

                            if( true == bhttp_srvcmd_analyze( ifile, g_receive_buffer ) ){
                                vhttp_cmdflg_reset( ifile, E_OK );          // 正常終了
                            }else{
                                vhttp_cmdflg_reset( ifile, E_NG );          // 異常終了
                            }
                            Net_LOG_Write( 1, "%04X", status );             // WebStorage通信成功

                        }else{
                            vhttp_cmdflg_reset(ifile,E_NG);                 // 異常終了
                            if(status == NX_NO_PACKET){
                                Net_LOG_Write( 105, "%04X", status );       // WebStorage通信失敗（タイムアウト）
                            }else{
                                if(status == NX_WEB_HTTP_STATUS_CODE_NOT_ACCEPTABLE){   // 406 Not Acceptable       // sakaguchi 2020.08.31
                                    Net_LOG_Write( 2, "%04X", NX_SUCCESS );             // WebStorage通信成功（機器登録無し）
                                }
// sakaguchi 2020.12.23 Data Server 412対応 ↓
                                else if(status == NX_WEB_HTTP_STATUS_CODE_PRECONDITION_FAILED){   // 412 Precondition Failed（アクセス拒否）
                                    G_HttpFile[FILE_C].reqid = S_ulReqID;                               // [FILE]リクエストID格納
                                    G_HttpFile[FILE_C].sndflg = SND_ON;                                 // [FILE]機器設定：送信有りをセット
                                    G_HttpCmd[HTTP_CMD_RSETF].sndflg = SND_ON;                          // 登録情報送信
                                    vhttp_sysset_sndon();                                               // 親機設定送信
                                    tx_event_flags_set (&event_flags_http, FLG_HTTP_READY, TX_OR);      // HTTPスレッド動作許可ON
                                    Net_LOG_Write( 1, "%04X", status );                                 // WebStorage通信成功
                                }
// sakaguchi 2020.12.23 ↑
                                else{
                                Net_LOG_Write( 104, "%04X", ulchg_http_status_code(status) );       // WebStorage通信失敗（エラー）
                            }
                            }
                            return(status);
                        }
                        break;



                    //-------------------------------------------------------------------------
                    //----- 機器設定                         -----------------------------------
                    //-------------------------------------------------------------------------
                    case FILE_C:

                        // WSSからのリクエスト実行中はPostSettingsの送信は停止する  // sakaguchi 2020.09.02
                        if(true == bhttp_request_docheck()){
                            //PutLog(LOG_LAN, "HTTP postSettings stop(%d)", ifile);
                            break;
                        }

                        //----- send file make -----
                        memset(http_buffer, 0x00, sizeof(http_buffer));     // HTTP送信用バッファ：クリア
                        S_iHttpCnt = 0;                                     // バッファカウンタ：クリア
                        S_ulReqID = 0;                                      // リクエストID：クリア
                        S_ucNextCmd = 0;                                    // NextCommand：クリア      //sakaguchi cg UT-0027

                        ihttp_snddata_make( ifile, &icmd, http_buffer );    // HTTPデータ作成

                        //----- send file size -----
                        xsize = (size_t)S_iHttpCnt;

                        //----- client request initialize-----
                        method = NX_WEB_HTTP_METHOD_POST;
                        Printf("  . nx_web_http_client_request_initialize(%d)\r\n",ifile);
                        status = nx_web_http_client_request_initialize(client_ptr, method,
                                                                    p_info->resource,
                                                                    p_info->host,
                                                                    xsize,
                                                                    transfer_encoding_chunked,
                                                                    p_info->username, p_info->password,
                                                                    wait_option);
                        if(status != NX_SUCCESS)
                        {
                            PutLog(LOG_LAN, "HTTP initialize Error %d(%d)", status, ifile);

                            Printf("nx_web_http_client_request_initialize Failed. status = 0x%x (%d)\r\n", status,ifile);
                            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                            my_tls_close(client_ptr,which);
                            vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                            Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー
                            return( status);
                        }
                        sprintf( temp, "%04lX", fact_config.SerialNumber);
                        len = strlen(temp);

                        //----- request header -----
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Content_Type,  strlen(HD_Content_Type), (char *)HD_Content_Type_T2,  strlen(HD_Content_Type_T2), wait_option);
// 2023.05.31 ↓
                        if(fact_config.Vender == VENDER_ESPEC){
                            status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_User_Agent,  strlen(HD_User_Agent), (char *)HD_User_Agent_E,  strlen(HD_User_Agent_E), wait_option);
                        }else{
                            status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_User_Agent,  strlen(HD_User_Agent), (char *)HD_User_Agent_D,  strlen(HD_User_Agent_D), wait_option);
                        }
// 2023.05.31 ↑
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Api,  strlen(HD_Tandd_Api), (char *)HD_Tandd_Api_D,  strlen(HD_Tandd_Api_D), wait_option);
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Request_Type,  strlen(HD_Tandd_Request_Type), (char *)HD_Type_Setting,  strlen(HD_Type_Setting), wait_option);
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Serial,  strlen(HD_Tandd_Serial), temp,  len, wait_option);
                        Printf("nx_web_http_client_request_header_add. status = 0x%x  %d\r\n", status);

                        //----- request id ------
                        if(0 != G_HttpFile[ifile].reqid){                       // [File]リクエストID取得
                            sprintf( temp, "%lu", G_HttpFile[ifile].reqid );
                            len = strlen(temp);
                            status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Request_ID,  strlen(HD_Tandd_Request_ID), temp, len, wait_option);
                        }
                        //----- setthing count -----
                        sprintf(temp, "%lu", my_config.device.SysCnt);
                        len = strlen(temp);
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Setting_SysCnt, strlen(HD_Tandd_Setting_SysCnt), temp, len, wait_option);

                        sprintf( temp, "%lu", my_config.device.RegCnt);
                        len = strlen(temp);
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Setting_RegCnt, strlen(HD_Tandd_Setting_RegCnt), temp, len, wait_option);

                        //-------------------------------------------------------------------------
                        //----- Send the Header -----
                        //-------------------------------------------------------------------------
                        Printf("  . nx_web_http_client_request_send (Send Header)\r\n");
                        status = nx_web_http_client_request_send(client_ptr, wait_option);
                        if(status != NX_SUCCESS)
                        {
                            PutLog(LOG_LAN, "HTTP request send Error %d(%d)", status, ifile);
                            Printf("nx_web_http_client_request_send Failed. status = 0x%x\r\n", status);
                            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                            my_tls_close(client_ptr,which);
                            vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                            Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー
                            return(status);
                        }

                        //-------------------------------------------------------------------------
                        //----- Send the Body -----
                        //-------------------------------------------------------------------------
                        status =  nx_packet_allocate(&g_packet_pool1, &p_http_post_pkt, NX_TCP_PACKET, 100 /*NX_WAIT_FOREVER*/);       // 2023.03.01
                        if(status != NX_SUCCESS)
                        {
                            Printf("nx_packet_allocate Failed. status = 0x%x\r\n", status);
                            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                            my_tls_close(client_ptr,which);
                            vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                            Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー
                            return(status);
                        }

                        status = send_file(client_ptr, p_http_post_pkt, http_buffer, xsize);

                        if(status != NX_SUCCESS)
                        {
                            PutLog(LOG_LAN, "HTTP body send Error %d(%d)", status, ifile);
                            Printf("      send_file  Failed. status = 0x%x\r\n", status);
                            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                            status2 = nx_packet_release(p_http_post_pkt);
                            Printf("nx_packet_release 7 0x%x\r\n", status2);
                            my_tls_close(client_ptr,which);
                            vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                            Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー
                            return(status);
                        }
#ifdef DBG_TERM
                        nx_packet_pool_info_get(&g_packet_pool1,&total_packets, &free_packets, &empty_pool_requests, &empty_pool_suspensions, &invalid_packet_releases);
                        Printf("nx_packet_pool_info_get  %ld %ld %ld %ld %ld \r\n", total_packets, free_packets, empty_pool_requests, empty_pool_suspensions, invalid_packet_releases);
#endif
                        status = nx_packet_release(p_http_post_pkt);
#ifdef DBG_TERM                        
                        Printf("nx_packet_release 8 0x%x\r\n", status);
                        nx_packet_pool_info_get(&g_packet_pool1,&total_packets, &free_packets, &empty_pool_requests, &empty_pool_suspensions, &invalid_packet_releases);
                        Printf("nx_packet_pool_info_get  %ld %ld %ld %ld %ld \r\n", total_packets, free_packets, empty_pool_requests, empty_pool_suspensions, invalid_packet_releases);
#endif
                        //-------------------------------------------------------------------------
                        //----- Get response ------------------------------------------------------
                        //-------------------------------------------------------------------------
                        //tx_thread_sleep(50);
                        status = get_web_http_client_response_body( client_ptr );

                        if(status == NX_SUCCESS){
                            success_count ++;

                            //----- 受信コマンド解析 -----
                            if( true == bhttp_srvcmd_analyze( ifile, g_receive_buffer ) ){
                                vhttp_cmdflg_reset( ifile, E_OK );          // 正常終了
                            }else{
                                vhttp_cmdflg_reset( ifile, E_NG );          // 異常終了
                            }
                            Net_LOG_Write( 1, "%04X", status );             // WebStorage通信成功

                        }else{
                            if( status == NX_WEB_HTTP_STATUS_CODE_BAD_REQUEST ){
                                vhttp_cmdflg_reset(ifile,E_OK);             // BadRequestの場合は要求をクリアする
                            }else{
                            vhttp_cmdflg_reset(ifile,E_NG);                 // 異常終了
                            }

                            if(status == NX_NO_PACKET){
                                Net_LOG_Write( 105, "%04X", status );       // WebStorage通信失敗（タイムアウト）
                            }else{
                                if(status == NX_WEB_HTTP_STATUS_CODE_NOT_ACCEPTABLE){   // 406 Not Acceptable       // sakaguchi 2020.08.31
                                    Net_LOG_Write( 2, "%04X", NX_SUCCESS );             // WebStorage通信成功（機器登録無し）
                                }
// sakaguchi 2020.12.23 Data Server 412対応 ↓
                                else if(status == NX_WEB_HTTP_STATUS_CODE_PRECONDITION_FAILED){   // 412 Precondition Failed（アクセス拒否）
                                    G_HttpFile[FILE_C].reqid = S_ulReqID;                               // [FILE]リクエストID格納
                                    G_HttpFile[FILE_C].sndflg = SND_ON;                                 // [FILE]機器設定：送信有りをセット
                                    G_HttpCmd[HTTP_CMD_RSETF].sndflg = SND_ON;                          // 登録情報送信
                                    vhttp_sysset_sndon();                                               // 親機設定送信
                                    tx_event_flags_set (&event_flags_http, FLG_HTTP_READY, TX_OR);      // HTTPスレッド動作許可ON
                                    Net_LOG_Write( 1, "%04X", status );                                 // WebStorage通信成功
                                }
// sakaguchi 2020.12.23 ↑
                                else{
                                Net_LOG_Write( 104, "%04X", ulchg_http_status_code(status) );       // WebStorage通信失敗（エラー）
                            }
                            }
                            return(status);
                        }
                        break;


                    //-------------------------------------------------------------------------
                    //----- 操作リクエスト                   -----------------------------------
                    //-------------------------------------------------------------------------
                    case FILE_R:

                        //----- send file make -----
                        memset(http_buffer, 0x00, sizeof(http_buffer));     // HTTP送信用バッファ：クリア
                        S_iHttpCnt = 0;                                     // バッファカウンタ：クリア
                        S_ulReqID = 0;                                      // リクエストID：クリア
                        S_ucNextCmd = 0;                                    // NextCommand：クリア      /sakaguchi cg UT-0027

                        //----- send file size -----
                        xsize = strlen(http_buffer);

                        //----- client request initialize-----
                        method = NX_WEB_HTTP_METHOD_POST;
                        Printf("  . nx_web_http_client_request_initialize(%d)\r\n",ifile);
                        status = nx_web_http_client_request_initialize(client_ptr, method,
                                                                    p_info->resource,
                                                                    p_info->host,
                                                                    xsize,
                                                                    transfer_encoding_chunked,
                                                                    p_info->username, p_info->password,
                                                                    wait_option);
                        if(status != NX_SUCCESS)
                        {
// sakaguchi 2021.08.02 ↓
                            if(false == Update_check()){        // アップデート中は出力しない
                                PutLog(LOG_LAN, "HTTP initialize Error %d(%d)", status, ifile);
                            }
// sakaguchi 2021.08.02 ↑
                            Printf("nx_web_http_client_request_initialize Failed. status = 0x%x (%d)\r\n", status,ifile);
                            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                            my_tls_close(client_ptr,which);
                            vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                            Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー
                            return( status);
                        }
                        sprintf( temp, "%04lX", fact_config.SerialNumber);
                        len = strlen(temp);

                        //----- request header -----
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Content_Type,  strlen(HD_Content_Type), (char *)HD_Content_Type_T2,  strlen(HD_Content_Type_T2), wait_option);
// 2023.05.31 ↓
                        if(fact_config.Vender == VENDER_ESPEC){
                            status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_User_Agent,  strlen(HD_User_Agent), (char *)HD_User_Agent_E,  strlen(HD_User_Agent_E), wait_option);
                        }else{
                            status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_User_Agent,  strlen(HD_User_Agent), (char *)HD_User_Agent_D,  strlen(HD_User_Agent_D), wait_option);
                        }
// 2023.05.31 ↑
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Api,  strlen(HD_Tandd_Api), (char *)HD_Tandd_Api_D,  strlen(HD_Tandd_Api_D), wait_option);
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Request_Type,  strlen(HD_Tandd_Request_Type), (char *)HD_Type_Operation_Request,  strlen(HD_Type_Operation_Request), wait_option);
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Serial,  strlen(HD_Tandd_Serial), temp,  len, wait_option);
                        Printf("nx_web_http_client_request_header_add. status = 0x%x  %d\r\n", status);
                        //----- request id ------
                        if(0 != G_HttpFile[ifile].reqid){                       // [File]リクエストID取得
                            sprintf( temp, "%lu", G_HttpFile[ifile].reqid );
                            len = strlen(temp);
                            status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Request_ID,  strlen(HD_Tandd_Request_ID), temp, len, wait_option);
                        }

                        //----- setthing count -----
                        sprintf(temp, "%lu", my_config.device.SysCnt);
                        len = strlen(temp);
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Setting_SysCnt, strlen(HD_Tandd_Setting_SysCnt), temp, len, wait_option);

                        sprintf( temp, "%lu", my_config.device.RegCnt);
                        len = strlen(temp);
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Setting_RegCnt, strlen(HD_Tandd_Setting_RegCnt), temp, len, wait_option);

                        //-------------------------------------------------------------------------
                        //----- Send the Header -----
                        //-------------------------------------------------------------------------
                        Printf("  . nx_web_http_client_request_send (Send Header)\r\n");
                        status = nx_web_http_client_request_send(client_ptr, wait_option);
                        if(status != NX_SUCCESS)
                        {
                            PutLog(LOG_LAN, "HTTP request send Error %d(%d)", status, ifile);
                            Printf("nx_web_http_client_request_send Failed. status = 0x%x\r\n", status);
                            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                            my_tls_close(client_ptr,which);
                            vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                            Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー

                            return(status);
                        }
Printf("Send file size %d\r\n", xsize);
if(xsize!=0){
                        //-------------------------------------------------------------------------
                        //----- Send the Body -----
                        //-------------------------------------------------------------------------
                        status =  nx_packet_allocate(&g_packet_pool1, &p_http_post_pkt, NX_TCP_PACKET, 100 /*NX_WAIT_FOREVER*/);    // 2023.03.01
                        if(status != NX_SUCCESS)
                        {
                            Printf("nx_packet_allocate Failed. status = 0x%x\r\n", status);
                            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                            my_tls_close(client_ptr,which);
                            vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                            Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー
                            return(status);
                        }

                        status = send_file(client_ptr, p_http_post_pkt, http_buffer, xsize);

                        if(status != NX_SUCCESS)
                        {
                            PutLog(LOG_LAN, "HTTP body send Error %d(%d)", status, ifile);
                            Printf("      send_file  Failed. status = 0x%x\r\n", status);
                            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                            status2 = nx_packet_release(p_http_post_pkt);
                            Printf("nx_packet_release 9 0x%x\r\n", status2);
                            my_tls_close(client_ptr,which);
                            vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                            Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー
                            return(status);
                        }

                nx_packet_pool_info_get(&g_packet_pool1,&total_packets, &free_packets, &empty_pool_requests, &empty_pool_suspensions, &invalid_packet_releases);
                Printf("nx_packet_pool_info_get  %ld %ld %ld %ld %ld \r\n", total_packets, free_packets, empty_pool_requests, empty_pool_suspensions, invalid_packet_releases);
                        status = nx_packet_release(p_http_post_pkt);
                        Printf("nx_packet_release 10 0x%x\r\n", status);
}
                nx_packet_pool_info_get(&g_packet_pool1,&total_packets, &free_packets, &empty_pool_requests, &empty_pool_suspensions, &invalid_packet_releases);
                Printf("nx_packet_pool_info_get  %ld %ld %ld %ld %ld \r\n", total_packets, free_packets, empty_pool_requests, empty_pool_suspensions, invalid_packet_releases);
                        //-------------------------------------------------------------------------
                        //----- Get response ------------------------------------------------------
                        //-------------------------------------------------------------------------
                        //tx_thread_sleep(50);
                        status = get_web_http_client_response_body( client_ptr );

                        if(status == NX_SUCCESS){
                            success_count ++;

                            //----- 受信コマンド解析 -----
                            if( true == bhttp_srvcmd_analyze( ifile, g_receive_buffer ) ){
                                vhttp_cmdflg_reset( ifile, E_OK );      // 正常終了
                            }else{
                                vhttp_cmdflg_reset( ifile, E_NG );      // 異常終了
                            }

                            Net_LOG_Write( 1, "%04X", status );         // WebStorage通信成功

                        }else{

                            vhttp_cmdflg_reset( ifile, E_NG );          // 異常終了
                            if(status == NX_NO_PACKET){
                                Net_LOG_Write( 105, "%04X", status );   // WebStorage通信失敗（タイムアウト）
                            }else{
                                if(status == NX_WEB_HTTP_STATUS_CODE_NOT_ACCEPTABLE){   // 406 Not Acceptable       // sakaguchi 2020.08.31
                                    Net_LOG_Write( 2, "%04X", NX_SUCCESS );             // WebStorage通信成功（機器登録無し）
                                }
// sakaguchi 2020.12.23 Data Server 412対応 ↓
                                else if(status == NX_WEB_HTTP_STATUS_CODE_PRECONDITION_FAILED){   // 412 Precondition Failed（アクセス拒否）
                                    G_HttpFile[FILE_C].reqid = S_ulReqID;                               // [FILE]リクエストID格納
                                    G_HttpFile[FILE_C].sndflg = SND_ON;                                 // [FILE]機器設定：送信有りをセット
                                    G_HttpCmd[HTTP_CMD_RSETF].sndflg = SND_ON;                          // 登録情報送信
                                    vhttp_sysset_sndon();                                               // 親機設定送信
                                    tx_event_flags_set (&event_flags_http, FLG_HTTP_READY, TX_OR);      // HTTPスレッド動作許可ON
                                    Net_LOG_Write( 1, "%04X", status );                                 // WebStorage通信成功
                                }
// sakaguchi 2020.12.23 ↑
                                else{
                                Net_LOG_Write( 104, "%04X", ulchg_http_status_code(status) );   // WebStorage通信失敗（エラー）
                            }
                            }
                            return(status);
                        }
                        break;


                    //-------------------------------------------------------------------------
                    //----- 操作リクエスト（結果）            -----------------------------------
                    //-------------------------------------------------------------------------
                    case FILE_A:
                        tx_event_flags_get( &g_command_event_flags, FLG_CMD_END, TX_OR_CLEAR, &actual_events, TX_NO_WAIT ); // sakaguchi add 2020.08.28

                        //----- send file make -----
                        memset(http_buffer,0x00,sizeof(http_buffer));   // HTTP送信用バッファ：クリア
                        S_iHttpCnt = 0;                                 // バッファカウンタ：クリア
                        S_ulReqID = 0;                                  // リクエストID：クリア
                        S_ucNextCmd = 0;                                // NextCommand：クリア      /sakaguchi cg UT-0027

                        ihttp_snddata_make( ifile, &icmd, http_buffer );    //HTTPデータ作成

                        //----- send file size -----
                        xsize = (size_t)S_iHttpCnt;

                        //----- client request initialize-----
                        method = NX_WEB_HTTP_METHOD_POST;
                        Printf("  . nx_web_http_client_request_initialize(%d)\r\n",ifile);
                        status = nx_web_http_client_request_initialize(client_ptr, method,
                                                                    p_info->resource,
                                                                    p_info->host,
                                                                    xsize,
                                                                    transfer_encoding_chunked,
                                                                    p_info->username, p_info->password,
                                                                    wait_option);
                        if(status != NX_SUCCESS)
                        {
// sakaguchi 2021.08.02 ↓
                            if(false == Update_check()){        // アップデート中は出力しない
                                PutLog(LOG_LAN, "HTTP initialize Error %d(%d)", status, ifile);
                            }
// sakaguchi 2021.08.02 ↑
                            Printf("nx_web_http_client_request_initialize Failed. status = 0x%x (%d)\r\n", status, ifile);
                            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                            my_tls_close(client_ptr,which);
                            vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                            Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー
                            return( status);
                        }
                        sprintf( temp, "%04lX", fact_config.SerialNumber);
                        len = strlen(temp);

                        //----- request header -----
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Content_Type,  strlen(HD_Content_Type), (char *)HD_Content_Type_T2,  strlen(HD_Content_Type_T2), wait_option);
// 2023.05.31 ↓
                        if(fact_config.Vender == VENDER_ESPEC){
                            status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_User_Agent,  strlen(HD_User_Agent), (char *)HD_User_Agent_E,  strlen(HD_User_Agent_E), wait_option);
                        }else{
                            status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_User_Agent,  strlen(HD_User_Agent), (char *)HD_User_Agent_D,  strlen(HD_User_Agent_D), wait_option);
                        }
// 2023.05.31 ↑
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Api,  strlen(HD_Tandd_Api), (char *)HD_Tandd_Api_D,  strlen(HD_Tandd_Api_D), wait_option);
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Request_Type,  strlen(HD_Tandd_Request_Type), (char *)HD_Type_Operation_Result,  strlen(HD_Type_Operation_Result), wait_option);
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Serial,  strlen(HD_Tandd_Serial), temp,  len, wait_option);
                        Printf("nx_web_http_client_request_header_add. status = 0x%x  %d\r\n", status);

                        //----- request id ------
                        if(0 != G_HttpCmd[icmd].reqid){                 // [CMD]リクエストID取得
                            sprintf( temp, "%lu", G_HttpCmd[icmd].reqid );
                            len = strlen(temp);
                            status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Request_ID,  strlen(HD_Tandd_Request_ID), temp, len, wait_option);
                        }

                        //----- setthing count -----
                        sprintf( temp, "%lu", my_config.device.SysCnt);
                        len = strlen(temp);
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Setting_SysCnt,  strlen(HD_Tandd_Setting_SysCnt), temp, len, wait_option);

                        sprintf( temp, "%lu", my_config.device.RegCnt);
                        len = strlen(temp);
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Setting_RegCnt,  strlen(HD_Tandd_Setting_RegCnt), temp, len, wait_option);

                        //-------------------------------------------------------------------------
                        //----- Send the Header -----
                        //-------------------------------------------------------------------------
                        Printf("  . nx_web_http_client_request_send (Send Header)\r\n");
                        status = nx_web_http_client_request_send(client_ptr, wait_option);
                        if(status != NX_SUCCESS)
                        {
                            PutLog(LOG_LAN, "HTTP request send Error %d(%d)", status, ifile);
                            Printf("nx_web_http_client_request_send Failed. status = 0x%x\r\n", status);
                            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                            my_tls_close(client_ptr,which);
                            vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                            Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー

                            return(status);
                        }

                        //-------------------------------------------------------------------------
                        //----- Send the Body -----
                        //-------------------------------------------------------------------------
                        status =  nx_packet_allocate(&g_packet_pool1, &p_http_post_pkt, NX_TCP_PACKET, 100 /*NX_WAIT_FOREVER*/);    // 2023.03.01
                        if(status != NX_SUCCESS)
                        {
                            Printf("nx_packet_allocate Failed. status = 0x%x\r\n", status);
                            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                            my_tls_close(client_ptr,which);
                            vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                            Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー
                            return(status);
                        }

                        status = send_file(client_ptr, p_http_post_pkt, http_buffer, xsize);

                        if(status != NX_SUCCESS)
                        {
                            PutLog(LOG_LAN, "HTTP body send Error %d(%d)", status, ifile);
                            Printf("      send_file  Failed. status = 0x%x\r\n", status);
                            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                            status2 = nx_packet_release(p_http_post_pkt);
                            Printf("nx_packet_release 11 0x%x\r\n", status2);
                            my_tls_close(client_ptr,which);
                            vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                            Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー
                            return(status);
                        }

                        status = nx_packet_release(p_http_post_pkt);
                        Printf("nx_packet_release 12 0x%x\r\n", status);

                        //-------------------------------------------------------------------------
                        //----- Get response ------------------------------------------------------
                        //-------------------------------------------------------------------------
                        //tx_thread_sleep(50);
                        status = get_web_http_client_response_body( client_ptr );

                        if(status == NX_SUCCESS){
                            success_count ++;

                            //----- 受信コマンド解析 -----
                            if( true == bhttp_srvcmd_analyze( ifile, g_receive_buffer ) ){
                                vhttp_cmdflg_reset( ifile, E_OK );      // 正常終了
                            }else{
                                vhttp_cmdflg_reset( ifile, E_NG );      // 異常終了
                            }

                            Net_LOG_Write( 1, "%04X", status );         // WebStorage通信成功

                        }else{
                            if( status == NX_WEB_HTTP_STATUS_CODE_BAD_REQUEST ){
                                vhttp_cmdflg_reset(ifile,E_OK);         // BadRequestの場合は要求をクリアする
                            }else{
                                vhttp_cmdflg_reset(ifile,E_NG);         // 異常終了
                        }

                            if(status == NX_NO_PACKET){
                                Net_LOG_Write( 105, "%04X", status );   // WebStorage通信失敗（タイムアウト）
                            }else{
                                if(status == NX_WEB_HTTP_STATUS_CODE_NOT_ACCEPTABLE){   // 406 Not Acceptable       // sakaguchi 2020.08.31
                                    Net_LOG_Write( 2, "%04X", NX_SUCCESS );             // WebStorage通信成功（機器登録無し）
                                }
// sakaguchi 2020.12.23 Data Server 412対応 ↓
                                else if(status == NX_WEB_HTTP_STATUS_CODE_PRECONDITION_FAILED){   // 412 Precondition Failed（アクセス拒否）
                                    G_HttpFile[FILE_C].reqid = S_ulReqID;                               // [FILE]リクエストID格納
                                    G_HttpFile[FILE_C].sndflg = SND_ON;                                 // [FILE]機器設定：送信有りをセット
                                    G_HttpCmd[HTTP_CMD_RSETF].sndflg = SND_ON;                          // 登録情報送信
                                    vhttp_sysset_sndon();                                               // 親機設定送信
                                    tx_event_flags_set (&event_flags_http, FLG_HTTP_READY, TX_OR);      // HTTPスレッド動作許可ON
                                    Net_LOG_Write( 1, "%04X", status );                                 // WebStorage通信成功
                                }
// sakaguchi 2020.12.23 ↑
                                else{
                                Net_LOG_Write( 104, "%04X", ulchg_http_status_code(status) );   // WebStorage通信失敗（エラー）
                            }
                            }
                            return(status);
                        }
                        break;


                    //-------------------------------------------------------------------------
                    //----- 機器状態                        -----------------------------------
                    //-------------------------------------------------------------------------
                    case FILE_I:
                        //----- send file make -----
                        memset(http_buffer,0x00,sizeof(http_buffer));   // HTTP送信用バッファ：クリア
                        S_iHttpCnt = 0;                                 // バッファカウンタ：クリア
                        S_ulReqID = 0;                                  // リクエストID：クリア
                        S_ucNextCmd = 0;                                // NextCommand：クリア

                        // 機器の登録がなく送信データが無い場合はここで終了させる       // sakaguchi 2021.03.01
                        //ihttp_snddata_make( ifile, &icmd, http_buffer );      // HTTPデータ作成
                        iRet = ihttp_snddata_make( ifile, &icmd, http_buffer );    // HTTPデータ作成
                        if(iRet == E_NG){
                            vhttp_cmdflg_reset( ifile, E_OK );      // 正常終了
                            break;
                        }
                        //----- send file size -----
                        xsize = (size_t)S_iHttpCnt;

                        //----- client request initialize-----
                        method = NX_WEB_HTTP_METHOD_POST;
                        Printf("  . nx_web_http_client_request_initialize(%d)\r\n",ifile);
                        status = nx_web_http_client_request_initialize(client_ptr, method,
                                                                    p_info->resource,
                                                                    p_info->host,
                                                                    xsize,
                                                                    transfer_encoding_chunked,
                                                                    p_info->username, p_info->password,
                                                                    wait_option);
                        if(status != NX_SUCCESS)
                        {
                            PutLog(LOG_LAN, "HTTP initialize Error %d(%d)", status, ifile);

                            Printf("nx_web_http_client_request_initialize Failed. status = 0x%x (%d)\r\n", status, ifile);
                            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                            my_tls_close(client_ptr,which);
                            vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                            Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー
                            return(status);
                        }
                        sprintf( temp, "%04lX", fact_config.SerialNumber);
                        len = strlen(temp);

                        //----- request header -----
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Content_Type,  strlen(HD_Content_Type), (char *)HD_Content_Type_T2,  strlen(HD_Content_Type_T2), wait_option);
// 2023.05.31 ↓
                        if(fact_config.Vender == VENDER_ESPEC){
                            status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_User_Agent,  strlen(HD_User_Agent), (char *)HD_User_Agent_E,  strlen(HD_User_Agent_E), wait_option);
                        }else{
                            status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_User_Agent,  strlen(HD_User_Agent), (char *)HD_User_Agent_D,  strlen(HD_User_Agent_D), wait_option);
                        }
// 2023.05.31 ↑
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Api,  strlen(HD_Tandd_Api), (char *)HD_Tandd_Api_D,  strlen(HD_Tandd_Api_D), wait_option);
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Request_Type,  strlen(HD_Tandd_Request_Type), (char *)HD_Type_PostState,  strlen(HD_Type_PostState), wait_option);
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Serial,  strlen(HD_Tandd_Serial), temp,  len, wait_option);
                        Printf("nx_web_http_client_request_header_add. status = 0x%x  %d\r\n", status);


                        //----- request id ------
                        if(0 != G_HttpCmd[icmd].reqid){                 // [CMD]リクエストID取得
                            sprintf( temp, "%lu", G_HttpCmd[icmd].reqid );
                            len = strlen(temp);
                            status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Request_ID,  strlen(HD_Tandd_Request_ID), temp, len, wait_option);
                        }

                        //----- setthing count -----
                        sprintf( temp, "%lu", my_config.device.SysCnt);
                        len = strlen(temp);
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Setting_SysCnt,  strlen(HD_Tandd_Setting_SysCnt), temp, len, wait_option);

                        sprintf( temp, "%lu", my_config.device.RegCnt);
                        len = strlen(temp);
                        status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Setting_RegCnt,  strlen(HD_Tandd_Setting_RegCnt), temp, len, wait_option);


                        //-------------------------------------------------------------------------
                        //----- Send the Header -----
                        //-------------------------------------------------------------------------
                        Printf("  . nx_web_http_client_request_send (Send Header)\r\n");
                        status = nx_web_http_client_request_send(client_ptr, wait_option);
                        if(status != NX_SUCCESS)
                        {
                            PutLog(LOG_LAN, "HTTP request send Error %d(%d)", status, ifile);
                            Printf("nx_web_http_client_request_send Failed. status = 0x%x\r\n", status);
                            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                            my_tls_close(client_ptr,which);
                            vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                            Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー
                            return(status);
                        }

                        //-------------------------------------------------------------------------
                        //----- Send the Body -----
                        //-------------------------------------------------------------------------
                        status =  nx_packet_allocate(&g_packet_pool1, &p_http_post_pkt, NX_TCP_PACKET, 100 /*NX_WAIT_FOREVER*/);    // 2023.03.01
                        if(status != NX_SUCCESS)
                        {
                            Printf("nx_packet_allocate Failed. status = 0x%x\r\n", status);
                            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                            my_tls_close(client_ptr,which);
                            vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                            Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー
                            return(status);
                        }

                        status = send_file(client_ptr, p_http_post_pkt, http_buffer, xsize);

                        if(status != NX_SUCCESS)
                        {
                            PutLog(LOG_LAN, "HTTP body send Error %d(%d)", status, ifile);
                            Printf("      send_file  Failed. status = 0x%x\r\n", status);
                            //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                            status2 = nx_packet_release(p_http_post_pkt);
                            Printf("nx_packet_release 13 0x%x\r\n", status2);
                            my_tls_close(client_ptr,which);
                            vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                            Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー
                            return(status);
                        }

                        status = nx_packet_release(p_http_post_pkt);
                        Printf("nx_packet_release 14 0x%x\r\n", status);

                        //-------------------------------------------------------------------------
                        //----- Get response ------------------------------------------------------
                        //-------------------------------------------------------------------------
                        //tx_thread_sleep(50);
                        status = get_web_http_client_response_body( client_ptr );

                        if(status == NX_SUCCESS){
                            success_count ++;

                            //----- 受信コマンド解析 -----
                            if( true == bhttp_srvcmd_analyze( ifile, g_receive_buffer ) ){
                                vhttp_cmdflg_reset( ifile, E_OK );      // 正常終了
                            }else{
                                vhttp_cmdflg_reset( ifile, E_NG );      // 異常終了
                            }

                            Net_LOG_Write( 1, "%04X", status );         // WebStorage通信成功

                        }else{
                            if( status == NX_WEB_HTTP_STATUS_CODE_BAD_REQUEST ){
                                vhttp_cmdflg_reset( ifile, E_OK );      // BadRequestの場合は要求をクリアする
                            }else{
                                vhttp_cmdflg_reset( ifile, E_NG );      // 異常終了
                            }

                            if(status == NX_NO_PACKET){
                                Net_LOG_Write( 105, "%04X", status );   // WebStorage通信失敗（タイムアウト）
                            }else{
                                if(status == NX_WEB_HTTP_STATUS_CODE_NOT_ACCEPTABLE){   // 406 Not Acceptable       // sakaguchi 2020.08.31
                                    Net_LOG_Write( 2, "%04X", NX_SUCCESS );             // WebStorage通信成功（機器登録無し）
                                }
// sakaguchi 2020.12.23 Data Server 412対応 ↓
                                else if(status == NX_WEB_HTTP_STATUS_CODE_PRECONDITION_FAILED){   // 412 Precondition Failed（アクセス拒否）
                                    G_HttpFile[FILE_C].reqid = S_ulReqID;                               // [FILE]リクエストID格納
                                    G_HttpFile[FILE_C].sndflg = SND_ON;                                 // [FILE]機器設定：送信有りをセット
                                    G_HttpCmd[HTTP_CMD_RSETF].sndflg = SND_ON;                          // 登録情報送信
                                    vhttp_sysset_sndon();                                               // 親機設定送信
                                    tx_event_flags_set (&event_flags_http, FLG_HTTP_READY, TX_OR);      // HTTPスレッド動作許可ON
                                    Net_LOG_Write( 1, "%04X", status );                                 // WebStorage通信成功
                                }
// sakaguchi 2020.12.23 ↑
                                else{
                                Net_LOG_Write( 104, "%04X", ulchg_http_status_code(status) );   // WebStorage通信失敗（エラー）
                            }
                            }
                            return(status);
                        }
                        break;



                    //-------------------------------------------------------------------------
                    //----- ログ                            -----------------------------------
                    //-------------------------------------------------------------------------
                    //-------------------------------------------------------------------------
                    //----- ログ（送信テスト）               -----------------------------------
                    //-------------------------------------------------------------------------
                    case FILE_L:
                    case FILE_L_T:
                        for(;;){
                            //----- send file make -----
                            memset(http_buffer,0x00,sizeof(http_buffer));   // HTTP送信用バッファ：クリア
                            S_iHttpCnt = 0;                                 // バッファカウンタ：クリア
                            S_ulReqID = 0;                                  // リクエストID：クリア
                            S_ucNextCmd = 0;                                // NextCommand：クリア

//                            iRet = ihttp_logdata_make(http_buffer, (int)sizeof(http_buffer), S_ucLogAct, 0);
                            iRet = ihttp_logdata_make(http_buffer, S_ucLogAct, 0, ifile);        // 2023.03.01

                            if(E_NG == iRet){
                                vhttp_cmdflg_reset(ifile,E_OK);             // 正常終了扱いとする
                                break;                                      // ループ終了
                            }else if(E_OVER == iRet){
                                S_ucNextCmd = 1;                            // NextCommand：次要求有り
                            }

                            S_ucLogAct = 1;       // act=1;

                            //----- send file size -----
                            xsize = (size_t)S_iHttpCnt;

                            //----- client request initialize-----
                            method = NX_WEB_HTTP_METHOD_POST;
                            Printf("  . nx_web_http_client_request_initialize(%d)\r\n",ifile);
                            status = nx_web_http_client_request_initialize(client_ptr, method,
                                                                        p_info->resource,
                                                                        p_info->host,
                                                                        xsize,
                                                                        transfer_encoding_chunked,
                                                                        p_info->username, p_info->password,
                                                                        wait_option);
                            if(status != NX_SUCCESS)
                            {
                                PutLog(LOG_LAN, "HTTP initialize Error %d(%d)", status, ifile);

                                Printf("nx_web_http_client_request_initialize Failed. status = 0x%x (%d)\r\n", status,ifile);
                                //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                                my_tls_close(client_ptr,which);
                                vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                                Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー
                                return( status);
                            }
                            sprintf( temp, "%04lX", fact_config.SerialNumber);
                            len = strlen(temp);

                            //----- request header -----
                            status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Content_Type,  strlen(HD_Content_Type), (char *)HD_Content_Type_LOG,  strlen(HD_Content_Type_LOG), wait_option);
// 2023.05.31 ↓
                            if(fact_config.Vender == VENDER_ESPEC){
                                status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_User_Agent,  strlen(HD_User_Agent), (char *)HD_User_Agent_E,  strlen(HD_User_Agent_E), wait_option);
                            }else{
                                status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_User_Agent,  strlen(HD_User_Agent), (char *)HD_User_Agent_D,  strlen(HD_User_Agent_D), wait_option);
                            }
// 2023.05.31 ↑
                            status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Api,  strlen(HD_Tandd_Api), (char *)HD_Tandd_Api_D,  strlen(HD_Tandd_Api_D), wait_option);
                            status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Request_Type,  strlen(HD_Tandd_Request_Type), (char *)HD_Type_Log,  strlen(HD_Type_Log), wait_option);
                            status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Serial,  strlen(HD_Tandd_Serial), temp,  len, wait_option);             //親機シリアル番号

                            //----- has next -----
                            if( 1 == S_ucNextCmd ){
                                status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Next_Command,  strlen(HD_Tandd_Next_Command), (char *)HD_Type_Operation_Request,  strlen(HD_Type_Operation_Request), wait_option);
                            }

                            //----- setthing count -----
                            sprintf(temp, "%lu", my_config.device.SysCnt);
                            len = strlen(temp);
                            status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Setting_SysCnt, strlen(HD_Tandd_Setting_SysCnt), temp, len, wait_option);

                            sprintf( temp, "%lu", my_config.device.RegCnt);
                            len = strlen(temp);
                            status = nx_web_http_client_request_header_add(client_ptr, (char *)HD_Tandd_Setting_RegCnt, strlen(HD_Tandd_Setting_RegCnt), temp, len, wait_option);

                            Printf("nx_web_http_client_request_header_add. status = 0x%x  %d\r\n", status);

                            //-------------------------------------------------------------------------
                            //----- Send the Header -----
                            //-------------------------------------------------------------------------
                            Printf("  . nx_web_http_client_request_send (Send Header)\r\n");
                            status = nx_web_http_client_request_send(client_ptr, wait_option);
                            if(status != NX_SUCCESS)
                            {
                                PutLog(LOG_LAN, "HTTP request send Error %d(%d)", status, ifile);
                                Printf("nx_web_http_client_request_send Failed. status = 0x%x\r\n", status);
                                //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                                my_tls_close(client_ptr,which);
                                vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                                Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー
                                return(status);
                            }

                            //-------------------------------------------------------------------------
                            //----- Send the Body -----
                            //-------------------------------------------------------------------------
                            status =  nx_packet_allocate(&g_packet_pool1, &p_http_post_pkt, NX_TCP_PACKET, 100 /*NX_WAIT_FOREVER*/);    // 2023.03.01
                            if(status != NX_SUCCESS)
                            {
                                Printf("nx_packet_allocate Failed. status = 0x%x\r\n", status);
                                //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                                my_tls_close(client_ptr,which);
                                vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                                Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー
                                return(status);
                            }

                            status = send_file(client_ptr, p_http_post_pkt, http_buffer, xsize);

                            if(status != NX_SUCCESS)
                            {
                                PutLog(LOG_LAN, "HTTP body send Error %d(%d)", status, ifile);
                                Printf("      send_file  Failed. status = 0x%x\r\n", status);
                                //_nx_web_http_client_error_exit(p_info->p_secure_client, 500);
                                nx_packet_release(p_http_post_pkt);
                                Printf("nx_packet_release 15 0x%x\r\n", status2);
                                my_tls_close(client_ptr,which);
                                vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                                Net_LOG_Write( 200, "%04X", status );       // HTTPセッションエラー
                                return(status);
                            }

                            status = nx_packet_release(p_http_post_pkt);
                            Printf("nx_packet_release 16 0x%x\r\n", status);

                            //-------------------------------------------------------------------------
                            //----- Get response ------------------------------------------------------
                            //-------------------------------------------------------------------------
                            //tx_thread_sleep(50);
                            status = get_web_http_client_response_body( client_ptr );

                            if(status == NX_SUCCESS){
                                success_count ++;

                                //-------------------------------------------------------------------------
                                //----- 送信完了     ------------------------------------------------------
                                //-------------------------------------------------------------------------
                                if( true == bhttp_srvcmd_analyze( ifile, g_receive_buffer ) ){
                                    vhttp_cmdflg_reset( ifile, E_OK );      // 正常終了
                                }else{
                                    vhttp_cmdflg_reset( ifile, E_NG );      // 異常終了
                                }
                                Net_LOG_Write( 1, "%04X", status );         // WebStorage通信成功

                                //----- send file make -----
                                memset(http_buffer, 0x00, sizeof(http_buffer));
                                S_iHttpCnt = 0;                             // バッファカウンタ：クリア
                                S_ulReqID = 0;                              // リクエストID：クリア
                                S_ucNextCmd = 0;                            // NextCommand：クリア
                                // 全ログデータを送信するためループ継続
// 2023.07.30 DataServer 1リクエスト対策 ↓
                                if( my_config.websrv.Mode[0] == 0 ){
                                    if( LOG_SEND_CNT <= GetLogCnt_http(0)){     // ログが10件以上溜まっている場合 2023.08.01
                                        G_HttpFile[ifile].sndflg = SND_ON;
                                    }
                                    break;
                                }
// 2023.07.30 DataServer 1リクエスト対策 ↑
                            }else{
                                vhttp_cmdflg_reset(ifile,E_NG);             // 異常終了
                                if(status == NX_NO_PACKET){
                                    Net_LOG_Write( 105, "%04X", status );   // WebStorage通信失敗（タイムアウト）
                                }else{
                                    // ログ送信テストの場合は、WSSに親機の登録が無くても正常と返す
//                                    if((ifile == FILE_L_T) && (status == NX_WEB_HTTP_STATUS_CODE_NOT_ACCEPTABLE)){   // 406 Not Acceptable
//                                        Net_LOG_Write( 1, "%04X", NX_SUCCESS );         // WebStorage通信成功
                                    if(status == NX_WEB_HTTP_STATUS_CODE_NOT_ACCEPTABLE){   // 406 Not Acceptable       // sakaguchi 2020.08.31
                                        Net_LOG_Write( 2, "%04X", NX_SUCCESS );             // WebStorage通信成功（機器登録無し）
                                    }
// sakaguchi 2020.12.23 Data Server 412対応 ↓
                                    else if(status == NX_WEB_HTTP_STATUS_CODE_PRECONDITION_FAILED){   // 412 Precondition Failed（アクセス拒否）
                                        G_HttpFile[FILE_C].reqid = S_ulReqID;                               // [FILE]リクエストID格納
                                        G_HttpFile[FILE_C].sndflg = SND_ON;                                 // [FILE]機器設定：送信有りをセット
                                        G_HttpCmd[HTTP_CMD_RSETF].sndflg = SND_ON;                          // 登録情報送信
                                        vhttp_sysset_sndon();                                               // 親機設定送信
                                        tx_event_flags_set (&event_flags_http, FLG_HTTP_READY, TX_OR);      // HTTPスレッド動作許可ON
                                        Net_LOG_Write( 1, "%04X", status );                                 // WebStorage通信成功
                                    }
// sakaguchi 2020.12.23 ↑
                                    else{
                                        Net_LOG_Write( 104, "%04X", ulchg_http_status_code(status) );   // WebStorage通信失敗（エラー）
                                    }
                                }
                                return(status);
                            }
                        }
                        break;

                    default:
                        break;
                }
// 2023.07.30 DataServer 1リクエスト対策 ↓
                if( my_config.websrv.Mode[0] == 0 ){
                    break;
                }
// 2023.07.30 DataServer 1リクエスト対策 ↑
            }        
        }

// 2023.07.30 DataServer 1リクエスト対策 ↓
        if( my_config.websrv.Mode[0] == 0 ){
            break;
        }
// 2023.07.30 DataServer 1リクエスト対策 ↑

        if(SND_OFF == ichttp_filesnd_check()){      // 送信ファイルがある場合はセッションを継続させる
            break;
        }
    }

    my_tls_close(client_ptr,which);
    return (status);
}


//===========================================================================
// 機 能 : ファイル送信
// 引 数 :
// 返 値 :
//===========================================================================

//#define PK_SIZE 1360  // sakaguchi 2021.07.16 del
//#define PK_SIZE 1200
//#define PK_SIZE 1460

static UINT send_file(NX_WEB_HTTP_CLIENT  *client_ptr, NX_PACKET *send_packet, char *buf, size_t len )
{
    UINT       status = NX_SUCCESS;
    UINT       status2 = NX_SUCCESS;
    ULONG total_packets;
    ULONG free_packets;
    ULONG empty_pool_requests;
    ULONG empty_pool_suspensions;
    ULONG invalid_packet_releases;
    ULONG timer_ticks;                  // sakaguchi 2021.05.28

    uint32_t block, amari, end = 0;
    uint32_t zan = len;
// sakaguchi 2021.07.16 ↓
    //int i;
    int mode;
    uint16_t PK_SIZE;

    PK_SIZE = *(uint16_t *)&my_config.network.Mtu[0];   // MTU
    mode = ((my_config.websrv.Mode[0] == 0) || (my_config.websrv.Mode[0] == 1)) ? CONNECT_HTTP : CONNECT_HTTPS;
    if(mode == CONNECT_HTTPS){
        PK_SIZE = (uint16_t)(PK_SIZE - 40 - 40);        // -40byte 暗号化でサイズ増するためさらに40byte減らしておく
    }else{
        PK_SIZE = (uint16_t)(PK_SIZE - 40);             // -40byte
    }
// sakaguchi 2021.07.16 ↑

    amari = len % PK_SIZE;
    block = len / PK_SIZE;

    //if( amari!=0 && block !=0){
    //    block++;
    //}
    //end = block;

    Printf("\n HTTP_BODY 1:   [%d] [%d] [%d] [%d]\r\n", block, amari, end, zan);
    //DebugLog( LOG_DBG, "Send Body %ld/%ld ", len, block);           // 2020.11.25

    int bk = 0;
    while(zan>0)
    {
        // 2020.12.02   
        //for(i=0;i<10;i++){
            status2 = nx_packet_pool_info_get(&g_packet_pool1,&total_packets, &free_packets, &empty_pool_requests, &empty_pool_suspensions, &invalid_packet_releases);
            Printf("nx_packet_pool_info_get status = 0x%x %ld %ld %ld %ld %ld \r\n", status2, total_packets, free_packets, empty_pool_requests, empty_pool_suspensions, invalid_packet_releases);
       
        //    if(free_packets < 4){        // 2020.12.02 
        //        if(i==0)
        //            DebugLog( LOG_DBG, "nx_packet_pool_info_get %ld %ld %ld", free_packets, len, zan);
        //        Printf("Packet full wait \r\n");
        //        tx_thread_sleep (5); 
        //    }
        //    else{
        //        break;
        //    }
            
        //}
        //nx_packet_release(send_packet);

        status =  nx_packet_allocate(&g_packet_pool1, &send_packet, NX_TCP_PACKET, 100 /*NX_WAIT_FOREVER*/);
        if(status != NX_SUCCESS)
        {
            Printf("nx_packet_allocate 2 Failed. status = 0x%x\r\n", status);
            return( status);
        }

        if(zan >= PK_SIZE){
            status = nx_packet_data_append(send_packet, buf+bk*PK_SIZE, PK_SIZE, &g_packet_pool1, 500);
            zan -= PK_SIZE;
            Printf("\n HTTP BODY4 2:   [%d] [%d] [%d] zan[%d]\r\n", block, amari, bk, zan);     
        }
        else{
            status = nx_packet_data_append(send_packet, buf+bk*PK_SIZE, (ULONG)amari, &g_packet_pool1, 500);
            zan -= amari;
            Printf("\n HTTP BODY4 1:   [%d] [%d] [%d] zan[%d]\r\n", block, amari, bk, zan);     
        }

        bk++;


        if (status != NX_SUCCESS)
        {
            Printf("nx_packet_data_append FAILED wd  status = %d 0x%x \r\n", status, status );
            DebugLog( LOG_DBG, "nx_tcp_socket_send FAILED wd 0x%x", status );           // 2020.11.25
            status2 = nx_packet_release(send_packet);                                   // sakaguchi 2021.06.29 releaseに変更
            //status2 = nx_packet_transmit_release(send_packet);
            Printf("nx_packet_release send_file 1 0x%x (%d) \r\n", status2, status2);
            return( status );
        } 
        else 
        {
//          status = nx_web_http_client_put_packet(client_ptr, send_packet, 2500 /*NX_WAIT_FOREVER*/);
            //status = nx_web_http_client_put_packet(client_ptr, send_packet, 250);
            status = nx_web_http_client_put_packet(client_ptr, send_packet, 2500);          // sakaguchi 2021.05.28
            if (status != NX_SUCCESS)
            {
                Printf("nx_tcp_socket_send FAILED wd 3 status = %d 0x%x (%d) \r\n", status, status, block );
                DebugLog( LOG_DBG, "nx_tcp_socket_send FAILED wd 3 0x%x (%d)  %ld", status, block,  free_packets);           // 2020.11.25
                if(status != NX_NOT_CONNECTED){             // 2021.02.08  ifを追加
                    //status2 = nx_packet_release(send_packet);
                    status2 = nx_packet_transmit_release(send_packet);
                    Printf("nx_packet_release send_file 2 0x%x (%d) \r\n", status2, status2);
                }
                else{
                    Printf("NX_NOT_CONNECTED\r\n");
                }
                status2 = nx_packet_release(send_packet);           // sakaguchi 2021.06.29 add
                return( status );
                // NX_TX_QUEUE_DEPTH (0x49)   最大送信キュー深度に達しました。
                // NX_WINDOW_OVERFLOW (0x39)  リクエストが受信側者にアドバタイズされたバイト単位のウィンドウサイズより大きくなっています。
                // NX_NOT_CONNECTED  (0x38)
            }
        }
// sakaguchi 2021.07.16 ↓
        if(zan > 0){
            if(my_config.network.Interval[0]){      // sakaguchi 2021.05.28 モバイルルータ接続時の再送防止
                timer_ticks = (my_config.network.Interval[0] / 10);
                tx_thread_sleep(timer_ticks);
            }
        }
// sakaguchi 2021.07.16 ↑
        //tx_thread_sleep (2);          
        //tx_thread_sleep (5);     // https add
        //nx_packet_release(send_packet);

    }

    tx_thread_sleep (2);
    //nx_packet_release(send_packet);


//Exit:

    return (status);
}


//sakaguchi ↓
//*************************************************************************************/
//* 概要    | HTTP送信データ作成
//* 引数１  | [in] ifile：送信ファイル種別
//* 引数２  | [out]*index：送信コマンドIndex
//* 引数３  | [out]*sndbuf：送信バッファ
//* 戻り値  | 処理結果  成功：E_OK / 失敗:E_NG
//* 詳細    | 機器設定送信時は、複数コマンドを一度に送信するため、送信コマンドIndexは0を返す
//*************************************************************************************/
static int ihttp_snddata_make(int ifile, int *index, char *sndbuf)
{
    int icmd;
    int iRet = E_NG;            // 戻り値

    *index = HTTP_CMD_NONE;     // 送信コマンドIndex

    switch(ifile){

        //--------------------
        //---   機器設定   ---
        //--------------------
        case FILE_C:
            for( icmd=0; icmd<HTTP_CMD_MAX; icmd++ ){
                if( SND_ON == G_HttpCmd[icmd].sndflg ){     // [CMD]機器設定：送信有り？

                    iRet = ihttp_T2cmd_make( icmd, sndbuf );    // T2コマンド作成

                    if( E_OK == iRet ){                         // コマンド作成成功？
                        G_HttpCmd[icmd].sndflg = SND_DO;        // [CMD]機器設定：送信中をセット
                    }
//sakaguchi del UT-0006 ↓
//                    else if( E_OVER == iRet ){                  // 送信バッファオーバー？
//                        break;                                  // 処理終了
//                    }
//sakaguchi del UT-0006 ↑
                }
            }
            //機器設定送信時は、複数コマンドを一度に送信するため、送信コマンドIndexは0を返す
            *index = HTTP_CMD_NONE;
            break;


        //-----------------------------
        //---   操作リクエスト結果   ---
        //-----------------------------
        case FILE_A:
            for( icmd=0; icmd<HTTP_CMD_MAX; icmd++ ){
                if( SND_ON == G_HttpCmd[icmd].ansflg ){     // [CMD]操作リクエスト結果：送信有り？

                    iRet = ihttp_T2cmd_make( icmd, sndbuf );      // T2コマンド作成

                    if( E_OK == iRet ){                         // コマンド作成成功？
                        G_HttpCmd[icmd].ansflg = SND_DO;        // [CMD]操作リクエスト結果：送信中をセット
                        *index = icmd;                          // コマンドIndex格納
                        break;                                  // 処理終了
                    }
                }
            }
            break;


        //-----------------------------
        //---   機器状態            ---
        //-----------------------------
        case FILE_I:
            iRet = ihttp_postState_make( HTTP_CMD_EMONR, sndbuf );    // モニタリングで収集した無線通信情報と電池残量を取得する
            //機器情報送信は、将来的に複数コマンドを一度に送信する場合もあるため、送信コマンドIndexは0を返す
            *index = HTTP_CMD_NONE;
            break;


        default:
            break;
    }
    return(iRet);
}


//******************************************************************************
//* 概要    | 機器状態コマンド作成
//* 引数１  | [in] icmd : 送信コマンドIndex
//* 引数２  | [out]*sndbuf : 送信バッファ
//* 戻り値  | 処理結果 成功：E_OK / 異常：E_NG / サイズオーバー：E_OVER
//* 詳細    | 機器状態コマンドで送信するT2コマンド専用の作成関数
//******************************************************************************/
static int ihttp_postState_make(int icmd, char *sndbuf)
{
    int         i;                                          // ループカウンタ
    int         iCnt;                                       // バッファカウンタ
    LO_HI       iPara;                                      // パラメータサイズ
    uint16_t    usWork = 0x00;                              // 作業用変数
    int         group_max;                                  // 最大グループ番号
    int32_t     group;                                      // グループ番号
    char        outpara[REPEATER_MAX*REMOTE_UNIT_MAX+2];    // モニタリング結果出力用
    int         size;                                       // テーブルサイズ
    LO_HI_LONG  status;                                     // 実行結果
    uint32_t    adr;                                        // アドレス
	uint8_t     max_repeater_no;                            // 最大中継機番号
    int         iRet = E_NG;                                // 戻り値            // sakaguchi 2021.03.01

    //バッファカウンタをセット
    iCnt = S_iHttpCnt;


    switch(icmd){

        case HTTP_CMD_EMONR:            // 無線通信情報と電池残量を取得

            // 手動ルート設定の場合はEMONRは送信しない  // sakaguchi 2021.04.07
            if((root_registry.auto_route & 0x01) == 0){     // 自動ルート機能OFF
                break;
            }

//            group_max = get_regist_group_size();                                // 最大グループ番号（グループ数）
            group_max = RF_get_regist_group_size(&adr);                             // 最大グループ番号（グループ数）   // sakaguchi 2020.09.01
            for(group=1; group<=group_max; group++){

                RF_Group[group-1].max_unit_no = RF_get_max_unitno(group, &max_repeater_no); // 最大子機番号格納         // sakaguchi 2020.09.01
                RF_Group[group-1].max_rpt_no = max_repeater_no;                             // 最大中継機番号格納       // sakaguchi 2020.09.01

                iCnt += sprintf(sndbuf+iCnt,"T2");                                  // T2
                iCnt += sprintf(sndbuf+iCnt,"SZ");                                  // コマンドサイズ領域（ダミー格納）
                iCnt += sprintf(sndbuf+iCnt,"%s:", HTTP_T2Cmd[icmd]);               // コマンド名

                iCnt += sprintf(sndbuf+iCnt,"GRP=");                                // グループ番号
                iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%ld",group);
                sndbuf[iCnt++] = iPara.byte_lo;
                sndbuf[iCnt++] = iPara.byte_hi;
                iCnt += iPara.word;

                iCnt += sprintf(sndbuf+iCnt,"UTC=");                                // モニタリングした時刻
                iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%lu",RF_Group[group-1].moni_time);
                sndbuf[iCnt++] = iPara.byte_lo;
                sndbuf[iCnt++] = iPara.byte_hi;
                iCnt += iPara.word;

                iCnt += sprintf(sndbuf+iCnt,"RUMAX=");                              // 最大子機番号
                iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%d",RF_Group[group-1].max_unit_no);
                sndbuf[iCnt++] = iPara.byte_lo;
                sndbuf[iCnt++] = iPara.byte_hi;
                iCnt += iPara.word;

//                if( 0 != RF_Group[group-1].max_unit_no ){                           // 子機が存在しない場合でも以下のパラメータは省略しない

                memset(outpara,0x00,sizeof(outpara));
                size = RF_ParentTable_Get(outpara, group, 1, 0);                // 子機の親番号（最新）
                iCnt += sprintf(sndbuf+iCnt,"RUPARENT=");
                memcpy(sndbuf+iCnt+2,outpara,(size_t)size);
                iPara.word = (uint16_t)size;
                sndbuf[iCnt++] = iPara.byte_lo;
                sndbuf[iCnt++] = iPara.byte_hi;
                iCnt += iPara.word;


                memset(outpara,0x00,sizeof(outpara));
                size = RF_ParentTable_Get(outpara, group, 1, 1);                // 子機の親番号 (最新より1回前)
                iCnt += sprintf(sndbuf+iCnt,"RUPARENT1=");
                memcpy(sndbuf+iCnt+2,outpara,(size_t)size);
                iPara.word = (uint16_t)size;
                sndbuf[iCnt++] = iPara.byte_lo;
                sndbuf[iCnt++] = iPara.byte_hi;
                iCnt += iPara.word;


                memset(outpara,0x00,sizeof(outpara));
                size = RF_ParentTable_Get(outpara, group, 1, 2);                // 子機の親番号 (最新より2回前)
                iCnt += sprintf(sndbuf+iCnt,"RUPARENT2=");
                memcpy(sndbuf+iCnt+2,outpara,(size_t)size);
                iPara.word = (uint16_t)size;
                sndbuf[iCnt++] = iPara.byte_lo;
                sndbuf[iCnt++] = iPara.byte_hi;
                iCnt += iPara.word;


                memset(outpara,0x00,sizeof(outpara));
                size = RF_ParentTable_Get(outpara, group, 1, 3);                // 子機の親番号 (最新より3回前)
                iCnt += sprintf(sndbuf+iCnt,"RUPARENT3=");
                memcpy(sndbuf+iCnt+2,outpara,(size_t)size);
                iPara.word = (uint16_t)size;
                sndbuf[iCnt++] = iPara.byte_lo;
                sndbuf[iCnt++] = iPara.byte_hi;
                iCnt += iPara.word;


                memset(outpara,0x00,sizeof(outpara));
                size = RF_ParentTable_Get(outpara, group, 1, 5);                // 子機の親番号 (最後に通信できた親番号)
                iCnt += sprintf(sndbuf+iCnt,"RUPARENTLAST=");
                memcpy(sndbuf+iCnt+2,outpara,(size_t)size);
                iPara.word = (uint16_t)size;
                sndbuf[iCnt++] = iPara.byte_lo;
                sndbuf[iCnt++] = iPara.byte_hi;
                iCnt += iPara.word;


                memset(outpara,0x00,sizeof(outpara));
                size = RF_RssiTable_Get(outpara, group, 1);                     // 子機と親番号の機器との電波強度
                iCnt += sprintf(sndbuf+iCnt,"RURSSI=");
                memcpy(sndbuf+iCnt+2,outpara,(size_t)size);
                iPara.word = (uint16_t)size;
                sndbuf[iCnt++] = iPara.byte_lo;
                sndbuf[iCnt++] = iPara.byte_hi;
                iCnt += iPara.word;


                memset(outpara,0x00,sizeof(outpara));
                size = RF_BatteryTable_Get(outpara, group, 1);                  // 子機の電池残量
                iCnt += sprintf(sndbuf+iCnt,"RUBATT=");
                memcpy(sndbuf+iCnt+2,outpara,(size_t)size);
                iPara.word = (uint16_t)size;
                sndbuf[iCnt++] = iPara.byte_lo;
                sndbuf[iCnt++] = iPara.byte_hi;
                iCnt += iPara.word;
//                }


                iCnt += sprintf(sndbuf+iCnt,"RPTMAX=");                             // 最大中継機番号
                iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%d",RF_Group[group-1].max_rpt_no);
                sndbuf[iCnt++] = iPara.byte_lo;
                sndbuf[iCnt++] = iPara.byte_hi;
                iCnt += iPara.word;


//                if(0 != RF_Group[group-1].max_rpt_no){                              // 中継機が存在しない場合でも以下のパラメータは省略しない

                memset(outpara,0x00,sizeof(outpara));
                size = RF_ParentTable_Get(outpara, group, 0, 0);                // 中継機の親番号（最新）
                iCnt += sprintf(sndbuf+iCnt,"RPTPARENT=");
                memcpy(sndbuf+iCnt+2,outpara,(size_t)size);
                iPara.word = (uint16_t)size;
                sndbuf[iCnt++] = iPara.byte_lo;
                sndbuf[iCnt++] = iPara.byte_hi;
                iCnt += iPara.word;


                memset(outpara,0x00,sizeof(outpara));
                size = RF_ParentTable_Get(outpara, group, 0, 1);                // 中継機の親番号 (最新より1回前)
                iCnt += sprintf(sndbuf+iCnt,"RPTPARENT1=");
                memcpy(sndbuf+iCnt+2,outpara,(size_t)size);
                iPara.word = (uint16_t)size;
                sndbuf[iCnt++] = iPara.byte_lo;
                sndbuf[iCnt++] = iPara.byte_hi;
                iCnt += iPara.word;


                memset(outpara,0x00,sizeof(outpara));
                size = RF_ParentTable_Get(outpara, group, 0, 2);                // 中継機の親番号 (最新より2回前)
                iCnt += sprintf(sndbuf+iCnt,"RPTPARENT2=");
                memcpy(sndbuf+iCnt+2,outpara,(size_t)size);
                iPara.word = (uint16_t)size;
                sndbuf[iCnt++] = iPara.byte_lo;
                sndbuf[iCnt++] = iPara.byte_hi;
                iCnt += iPara.word;


                memset(outpara,0x00,sizeof(outpara));
                size = RF_ParentTable_Get(outpara, group, 0, 3);                // 中継機の親番号 (最新より3回前)
                iCnt += sprintf(sndbuf+iCnt,"RPTPARENT3=");
                memcpy(sndbuf+iCnt+2,outpara,(size_t)size);
                iPara.word = (uint16_t)size;
                sndbuf[iCnt++] = iPara.byte_lo;
                sndbuf[iCnt++] = iPara.byte_hi;
                iCnt += iPara.word;


                memset(outpara,0x00,sizeof(outpara));
                size = RF_ParentTable_Get(outpara, group, 0, 5);                // 中継機の親番号 (最後に通信できた親番号)
                iCnt += sprintf(sndbuf+iCnt,"RPTPARENTLAST=");
                memcpy(sndbuf+iCnt+2,outpara,(size_t)size);
                iPara.word = (uint16_t)size;
                sndbuf[iCnt++] = iPara.byte_lo;
                sndbuf[iCnt++] = iPara.byte_hi;
                iCnt += iPara.word;


                memset(outpara,0x00,sizeof(outpara));
                size = RF_RssiTable_Get(outpara, group, 0);                     // 中継機と親番号の機器との電波強度
                iCnt += sprintf(sndbuf+iCnt,"RPTRSSI=");
                memcpy(sndbuf+iCnt+2,outpara,(size_t)size);
                iPara.word = (uint16_t)size;
                sndbuf[iCnt++] = iPara.byte_lo;
                sndbuf[iCnt++] = iPara.byte_hi;
                iCnt += iPara.word;


                memset(outpara,0x00,sizeof(outpara));
                size = RF_BatteryTable_Get(outpara, group, 0);                  // 中継機の電池残量
                iCnt += sprintf(sndbuf+iCnt,"RPTBATT=");
                memcpy(sndbuf+iCnt+2,outpara,(size_t)size);
                iPara.word = (uint16_t)size;
                sndbuf[iCnt++] = iPara.byte_lo;
                sndbuf[iCnt++] = iPara.byte_hi;
                iCnt += iPara.word;

//                }

                //ステータス
                status.longword = ERR( CMD, NOERROR );                              //エラー無しをセット
                iCnt += sprintf(sndbuf+iCnt,"STATUS=");
                iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%04hd-%04hd", (UINT)status.word.hi, (UINT)status.word.lo);
                sndbuf[iCnt++] = iPara.byte_lo;
                sndbuf[iCnt++] = iPara.byte_hi;
                iCnt += iPara.word;

                //コマンドサイズを計算
                iPara.word = (uint16_t)(iCnt-4-S_iHttpCnt);        // 'T2'+size 4byte減算）
                sndbuf[S_iHttpCnt+2] = iPara.byte_lo;
                sndbuf[S_iHttpCnt+3] = iPara.byte_hi;

                //チェックサムを計算
                //コマンド＋パラメータ部の加算
                usWork = 0;
//                for(i=4; i<iCnt; i++){
                for(i=4; i<iCnt-S_iHttpCnt; i++){
                    usWork = (uint16_t) (usWork +sndbuf[S_iHttpCnt+i]);
                }

                iPara.word = usWork;
                sndbuf[iCnt++] = iPara.byte_lo;
                sndbuf[iCnt++] = iPara.byte_hi;

                S_iHttpCnt = iCnt;                                 // バッファカウンタ更新

                iRet = E_OK;                    // 正常             // sakaguchi 2021.03.01
            }
            break;
        default:
            break;
    }



    //return(E_OK);
    return(iRet);           // sakaguchi 2021.03.01

}

//******************************************************************************
//* 概要    | T2コマンド作成（SAVE専用）
//* 引数１  | [in] icmd : 送信コマンドIndex
//* 引数２  | [in] step : 送信段階（step1/step2/step3）
//* 引数３  | [out]*sndbuf : 送信バッファ
//* 戻り値  | 処理結果 成功：E_OK / 異常：E_NG
//* 詳細    | 指定した送信コマンドIndexでT2コマンドを作成する。
//*         | 保護解除⇒保護開始(パラメータ：SAVE)が必要なT2コマンドの作成に使用する。
//******************************************************************************/
static int ihttp_T2cmd_make_save(int icmd, int step, char *sndbuf)
{
    int         iRet = E_NG;                    // 戻り値
    int         i,iCnt = 0;                     // カウンタ
    uint32_t    ulWork = 0;                     // 作業用変数
    char        cArea[6];                       // エリア
    int32_t     lsize;                          // パラメータサイズ
    LO_HI       iPara;                          // パラメータサイズ
    char	    *data = NULL;                          // 書込データ

    memset(cArea,0x00,sizeof(cArea));

    // step2：書込データ送信（バックアップデータをそのまま使用する）
    if(SND_STEP2 == step){
        // 書込データのサイズを取得
        lsize = lhttp_T2cmd_ParamAdrs_get(g_receive_buffer_back, "DATA=", data);
        if(INT32_MAX == lsize){
            return(iRet);           // 書込データ無しのためエラー終了
        }
        G_HttpCmd[icmd].offset = (uint16_t)(G_HttpCmd[icmd].offset + lsize);    // オフセットを進める

        iPara.byte_lo = g_receive_buffer_back[2];
        iPara.byte_hi = g_receive_buffer_back[3];
        memcpy(sndbuf, g_receive_buffer_back, (size_t)iPara.word+6);
        return(E_OK);
    }

    iCnt += sprintf(sndbuf+iCnt,"T2");
    iCnt += sprintf(sndbuf+iCnt,"SZ");                      // コマンドサイズ（ダミーを格納）
    iCnt += sprintf(sndbuf+iCnt,"%s:", HTTP_T2Cmd[icmd]);   // コマンド

    switch(icmd){

        case HTTP_CMD_WRTCE:            // ルート証明書の変更

            // エリア情報を取得
            lsize = lhttp_T2cmd_ParamAdrs_get(g_receive_buffer_back, "AREA=", &cArea[0]);
            if(INT32_MAX == lsize){
                return(iRet);           // エリア情報無しのためエラー終了
            }

            // step1：保護解除
            if(SND_STEP1 == step){
                // エリア
                iCnt += sprintf(sndbuf+iCnt,"AREA=");
                iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%s",cArea);
                sndbuf[iCnt++] = iPara.byte_lo;
                sndbuf[iCnt++] = iPara.byte_hi;
                iCnt += iPara.word;

                // 保護解除
                ulWork = 0;
                iCnt += sprintf(sndbuf+iCnt,"SAVE=");
                iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%lu",ulWork);
                sndbuf[iCnt++] = iPara.byte_lo;
                sndbuf[iCnt++] = iPara.byte_hi;
                iCnt += iPara.word;

                // 総バイト数
                ulWork = G_HttpCmd[icmd].size;
                iCnt += sprintf(sndbuf+iCnt,"SIZE=");
                iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%lu",ulWork);
                sndbuf[iCnt++] = iPara.byte_lo;
                sndbuf[iCnt++] = iPara.byte_hi;
                iCnt += iPara.word;

                iRet = E_OK;            // 戻り値：正常
            }

            // step3：保護開始
            else if(SND_STEP3 == step){
                // エリア
                iCnt += sprintf(sndbuf+iCnt,"AREA=");
                iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%s",cArea);
                sndbuf[iCnt++] = iPara.byte_lo;
                sndbuf[iCnt++] = iPara.byte_hi;
                iCnt += iPara.word;

                // 保護開始
                ulWork = G_HttpCmd[icmd].size;          // 書込データサイズを格納
                iCnt += sprintf(sndbuf+iCnt,"SAVE=");
                iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%lu",ulWork);
                sndbuf[iCnt++] = iPara.byte_lo;
                sndbuf[iCnt++] = iPara.byte_hi;
                iCnt += iPara.word;

                iRet = E_OK;            // 戻り値：正常
            }
            break;


        case HTTP_CMD_WSETF:                    // 登録情報の変更

            // step1：保護解除
            if(SND_STEP1 == step){

                // 保護解除
                ulWork = 0;
                iCnt += sprintf(sndbuf+iCnt,"SAVE=");
                iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%lu",ulWork);
                sndbuf[iCnt++] = iPara.byte_lo;
                sndbuf[iCnt++] = iPara.byte_hi;
                iCnt += iPara.word;

                iRet = E_OK;
            }

            // step3：保護開始
            else if(SND_STEP3 == step){

                // 保護
                ulWork = G_HttpCmd[icmd].size;          // 書込データサイズを格納
                iCnt += sprintf(sndbuf+iCnt,"SAVE=");
                iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%lu",ulWork);
                sndbuf[iCnt++] = iPara.byte_lo;
                sndbuf[iCnt++] = iPara.byte_hi;
                iCnt += iPara.word;

                iRet = E_OK;
            }
            break;

        default:
            break;
    }

    if(E_OK == iRet){

        // コマンドサイズを計算
        iPara.word = (uint16_t)(iCnt-4);        // 'T2'+size分を減算
        sndbuf[2] = iPara.byte_lo;
        sndbuf[3] = iPara.byte_hi;

        // チェックサムを計算
        ulWork = 0;
        for(i=4; i<iCnt; i++){
            ulWork = (ulWork + sndbuf[i]);
        }
        iPara.word = (uint16_t)ulWork;
        sndbuf[iCnt++] = iPara.byte_lo;
        sndbuf[iCnt++] = iPara.byte_hi;
    }

    return(iRet);
}

//******************************************************************************
//* 概要    | T2コマンド作成
//* 引数１  | [in] icmd : 送信コマンドIndex
//* 引数２  | [out]*sndbuf : 送信バッファ
//* 戻り値  | 処理結果 成功：E_OK / 異常：E_NG / サイズオーバー：E_OVER
//* 詳細    | 指定した送信コマンドIndexでT2コマンドを作成する
//******************************************************************************/
static int ihttp_T2cmd_make(int icmd, char *sndbuf)
{
    int         iRet = E_NG;                    //戻り値
    int         i;                              //ループカウンタ
    int         isector;                        //ループカウンタ
    int         imax;
    int         iCnt;
    LO_HI       iPara;                          //パラメータサイズ
    uint16_t    usWork = 0x00;                  //作業用変数
    char        cRejist[SFM_REGIST_SECT];       //作業用バッファ
    char        cVGroup[SFM_GROUP_SIZE];        //作業用バッファ
    uint32_t    ultime;                         //ローカルタイム取得
    char        ctm[20];                        //ローカルタイム取得
    rtc_time_t  tm;                             //ローカルタイム取得
    LO_HI_LONG  status;                         //実行結果
    uint32_t    ulsize;
    ioport_level_t level;                       //sakaguchi add UT-0016
    uint16_t    size;                           //sakaguchi UT-0040
//    static int32_t txLen;      //CmdStatusSizeの代わりに  cmd_threadから受け取る 2020,Aug.26      // sakaguchi del 2020.08.27

    memset(cRejist,0x00,sizeof(cRejist));
    memset(cVGroup,0x00,sizeof(cVGroup));
    memset(ctm,0x00,sizeof(ctm));

    // バッファカウンタをセット
    iCnt = S_iHttpCnt;

    // Bodyを空で応答する場合、ここで処理終了（例.サーバからの全設定取得時のレスポンス）
    if( ( HTTP_CMD_NONE == icmd ) && ( 1 == G_HttpCmd[HTTP_CMD_NONE].save ) ){
        G_HttpCmd[HTTP_CMD_NONE].save = 0;
//        return(true);
        return(E_OK);
    }


    // cmd threadの応答データを使用するコマンド
    switch(icmd)
    {
        case HTTP_CMD_EMONS:        // モニタリングを開始する
        case HTTP_CMD_EMONR:        // モニタリングで収集した無線通信情報と電池残量を取得する
        case HTTP_CMD_EMONC:        // モニタリングで取得した電波強度を削除する
        case HTTP_CMD_ERGRP:        // 指定グループの無線通知設定取得
        case HTTP_CMD_ETGSM:        // テスト（3GおよびGPSモジュール）
        case HTTP_CMD_EWLEX:        // 子機の電波強度を取得する(無線通信)
        case HTTP_CMD_EWLRU:        // 中継機の電波強度を取得する(無線通信)
        case HTTP_CMD_EWSTR:        // 子機の設定を取得する(無線通信)
        case HTTP_CMD_EWSTW:        // 子機の設定を変更する(記録開始)(無線通信)
        case HTTP_CMD_EWCUR:        // 子機の現在値を取得する(無線通信)
//        case HTTP_CMD_EWSUC:        // 子機の記録データを取得する(無線通信)       // 対応せず削除
        case HTTP_CMD_EWSCE:        // 子機の検索（無線通信）
        case HTTP_CMD_EWSCR:        // 中継機の一斉検索（無線通信）
        case HTTP_CMD_EWRSC:        // 子機のモニタリングデータを取得する（無線通信）
        case HTTP_CMD_EWBSW:        // 一斉記録開始（無線通信）
        case HTTP_CMD_EWRSP:        // 子機の記録を停止する（無線通信）
        case HTTP_CMD_EWPRO:        // プロテクト設定（無線通信）
        case HTTP_CMD_EWINT:        // 子機のモニタリング間隔変更（無線通信）
        case HTTP_CMD_EWBLW:        // 子機のBluetooth設定を設定する(無線通信)
        case HTTP_CMD_EWBLR:        // 子機のBluetooth設定を取得する(無線通信)
        case HTTP_CMD_EWSLW:        // 子機のスケール変換式を設定する(無線通信)
        case HTTP_CMD_EWSLR:        // 子機のスケール変換式を取得する(無線通信)
        case HTTP_CMD_EBADV:        // アドバタイジングデータの取得
        case HTTP_CMD_EWRSR:        // 子機と中継機の一斉検索（無線通信）
        case HTTP_CMD_EWREG:        // 子機登録変更（無線通信）
        case HTTP_CMD_ETWAR:        // 警報テストを開始する
        case HTTP_CMD_WREGI:        // 登録情報の設定を変更する（子機）
        case HTTP_CMD_ETLAN:        // テスト
        case HTTP_CMD_EWAIT:        // 自律動作を開始しない時間を設定する
        case HTTP_CMD_EWLAP:        // 無線LANのアクセスポイントを検索する
            //データ
//            memcpy(sndbuf+iCnt, StsArea, (size_t)txLen);
            memcpy(sndbuf+iCnt, S_ucCmdAns, (size_t)txLen);                // sakaguchi cg 2020.08.27
            iCnt += txLen;
            S_iHttpCnt = iCnt;          // バッファカウンタ更新
            iRet = E_OK;
            return(iRet);               // 処理終了
            break;

        case HTTP_CMD_EFIRM:        // ファームウェア更新
        case HTTP_CMD_RRTCE:        // httpsルート証明書（取得）
            if(ERR( CMD, NOERROR ) == G_HttpCmd[icmd].status){
                //データ
//            memcpy(sndbuf+iCnt, StsArea, (size_t)txLen);
            memcpy(sndbuf+iCnt, S_ucCmdAns, (size_t)txLen);                // sakaguchi cg 2020.08.27
            iCnt += txLen;
                S_iHttpCnt = iCnt;          // バッファカウンタ更新
                iRet = E_OK;
                return(iRet);               // 処理終了
            }else{
                // パスワード認証エラーの場合はcmd_threadの応答データは使用できないため、この関数内でコマンドを生成する
            }
            break;

        case HTTP_CMD_ELOGS:            // ログの取得および消去
            if( 0 != G_HttpCmd[icmd].save ){
//                memcpy(sndbuf+iCnt, StsArea, (size_t)txLen);
                memcpy(sndbuf+iCnt, S_ucCmdAns, (size_t)txLen);            // sakaguchi cg 2020.08.27
                iCnt += txLen;
                S_iHttpCnt = iCnt;          // バッファカウンタ更新
                iRet = E_OK;
                return(iRet);               // 処理終了
            }else{
                //ステータス
                G_HttpCmd[icmd].status = ERR( CMD, NOERROR );       // エラー無しをセット
                G_HttpFile[FILE_L].sndflg = SND_ON;                 // ログ送信フラグをONする
                S_ucLogAct = 0;                                     // 操作リクエスト時は全てのログを送信(ACT=0)
            }
            break;
        default:
            break;
    }


    iCnt += sprintf(sndbuf+iCnt,"T2");
    iCnt += sprintf(sndbuf+iCnt,"SZ");           // コマンドサイズ（ダミーを格納）
    // 未定義のコマンドの場合
    if(HTTP_CMD_NONE == icmd){
        iCnt += sprintf(sndbuf+iCnt,"%s:", S_ucErrCmd);         // エラー応答用コマンドを格納
    }else{
        iCnt += sprintf(sndbuf+iCnt,"%s:", HTTP_T2Cmd[icmd]);
    }


    switch(icmd)
    {
        case HTTP_CMD_NONE:         // エラーコマンド
        case HTTP_CMD_WUINF:        // 機器設定（変更）
        case HTTP_CMD_WBLEP:        // Bluetooth設定（変更）
        case HTTP_CMD_WDTIM:        // 時刻設定（変更）
        case HTTP_CMD_WWARP:        // 警報設定（変更）
        case HTTP_CMD_WCURP:        // モニタリング設定（変更）
        case HTTP_CMD_WSUCP:        // 記録データ送信設定（変更）
        case HTTP_CMD_WVGRP:        // グループ情報（変更）
        case HTTP_CMD_WPRXY:        // プロキシ設定（変更）
        case HTTP_CMD_EINIT:        // 初期化と再起動
        case HTTP_CMD_EAUTH:        // パスワード認証
        case HTTP_CMD_ELOGS:        // ログの取得および消去
        case HTTP_CMD_RRTCE:        // httpsルート証明書（取得）
        case HTTP_CMD_WNETP:        // ネットワーク情報を変更する
            // ステータスのみ
            status.longword = G_HttpCmd[icmd].status;
            iCnt += sprintf(sndbuf+iCnt,"STATUS=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%04hd-%04hd", (UINT)status.word.hi, (UINT)status.word.lo);
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;
            break;

        case HTTP_CMD_WRTCE:        // httpsルート証明書（変更）
        case HTTP_CMD_WSETF:        // 登録情報設定（変更）
            // 全データ受信完了していない場合は、パラメータにオフセットを付ける
            if(G_HttpCmd[icmd].offset < G_HttpCmd[icmd].size){
                iCnt += sprintf(sndbuf+iCnt,"OFFSET=");
                iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%u",G_HttpCmd[icmd].offset);
                sndbuf[iCnt++] = iPara.byte_lo;
                sndbuf[iCnt++] = iPara.byte_hi;
                iCnt += iPara.word;
            }
            // ステータス
            status.longword = G_HttpCmd[icmd].status;
            iCnt += sprintf(sndbuf+iCnt,"STATUS=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%04hd-%04hd", (UINT)status.word.hi, (UINT)status.word.lo);
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;
            break;

        case HTTP_CMD_EBSTS:        // ステータスの取得と設定
            // 動作状態
            iCnt += sprintf(sndbuf+iCnt,"STATE=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%u",UnitState);
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // USB接続の有無
            iCnt += sprintf(sndbuf+iCnt,"USB=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%u",isUSBState());
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // 接点出⼒の状態
            g_ioport.p_api-> pinRead(EX_WRAN, &level);
            if(level == IOPORT_LEVEL_HIGH)	usWork = 0;
            else							usWork = 1;
            iCnt += sprintf(sndbuf+iCnt,"SWOUT=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%u",usWork);
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            //起動回数、前回起動時のUNIX秒は不要のため削除

            // 外部電源の有無（500BWは有り固定）
            iCnt += sprintf(sndbuf+iCnt,"PWR=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%d",1);
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // 最後にネットワーク通信した時刻（UTIM）
            iCnt += sprintf(sndbuf+iCnt,"NETUTC=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%lu",NetLogInfo.NetUtc);
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // 最後のネットワーク通信の結果（コード）
            iCnt += sprintf(sndbuf+iCnt,"NETSTAT=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%lu",NetLogInfo.NetStat);
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // 最後にネットワーク通信した時刻（メッセージ）
            iCnt += sprintf(sndbuf+iCnt,"NETMSG=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%s",NetLogInfo.NetMsg);
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // ステータス
            G_HttpCmd[icmd].status = ERR( CMD, NOERROR );
            status.longword = G_HttpCmd[icmd].status;
            iCnt += sprintf(sndbuf+iCnt,"STATUS=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%04hd-%04hd", (UINT)status.word.hi, (UINT)status.word.lo);
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;
            break;


        case HTTP_CMD_RUINF:        // 機器設定（取得）
            // 親機名称
            iCnt += sprintf(sndbuf+iCnt,"NAME=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%s",my_config.device.Name);
            if(iPara.word > sizeof(my_config.device.Name))  iPara.word = sizeof(my_config.device.Name);     //sakaguchi add UT-0018
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // 温度単位
            iCnt += sprintf(sndbuf+iCnt,"UNITS=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%d",iChgChartoBinary(my_config.device.TempUnits,sizeof(my_config.device.TempUnits)));
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // 親機説明
            iCnt += sprintf(sndbuf+iCnt,"DESC=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%s",my_config.device.Description);
            if(iPara.word > sizeof(my_config.device.Description))  iPara.word = sizeof(my_config.device.Description);     //sakaguchi add UT-0018
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // シリアル番号
            iCnt += sprintf(sndbuf+iCnt,"SER=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%.8lX", fact_config.SerialNumber);
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            //本機のファームウェアバージョン
            iCnt += sprintf(sndbuf+iCnt,"FWV=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%s", VERSION_FW);
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            //RFモジュール部のファームウェアバージョン
            iCnt += sprintf(sndbuf+iCnt,"RFV=");
            //iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%.4X", regf_rfm_version_number & 0x0000ffff);
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%.4lX", regf_rfm_version_number & 0x0000ffff);    // 2023.03.01 warning潰し
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            //RFモジュール部のシリアル番号
            iCnt += sprintf(sndbuf+iCnt,"RFS=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%.8lX", regf_rfm_serial_number);
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            //BLEファームウェアバージョン
            iCnt += sprintf(sndbuf+iCnt,"BLV=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%.4s", psoc.revision);
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            //無線基準認証制度
            iCnt += sprintf(sndbuf+iCnt,"WCER=");
            switch(fact_config.SerialNumber & 0xf0000000){
                case 0x30000000:    // US
                    usWork = 1;
                    break;
                case 0x40000000:    // EU
                    usWork = 2;
                    break;
                case 0x50000000:    // 日本
                case 0xE0000000:    // 日本(ESPEC)
                default:            //
                    usWork = 0;
                    break;
            }
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%d", usWork);
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // ステータス
            G_HttpCmd[icmd].status = ERR( CMD, NOERROR );
            status.longword = G_HttpCmd[icmd].status;
            iCnt += sprintf(sndbuf+iCnt,"STATUS=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%04hd-%04hd", (UINT)status.word.hi, (UINT)status.word.lo);
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;
            break;


        case HTTP_CMD_RBLEP:        // Bluetooth設定（取得）
            // Bluetooth機能
            iCnt += sprintf(sndbuf+iCnt,"ENABLE=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%d",my_config.ble.active);
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // ステータス
            G_HttpCmd[icmd].status = ERR( CMD, NOERROR );
            status.longword = G_HttpCmd[icmd].status;
            iCnt += sprintf(sndbuf+iCnt,"STATUS=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%04hd-%04hd", (UINT)status.word.hi, (UINT)status.word.lo);
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;
            break;


        case HTTP_CMD_RDTIM:        // 時刻設定（取得）
            // 時刻取得
            ultime = RTC_GetGMTSec();           // UNIXタイム取得
            RTC_GSec2LCT( ultime, &tm );        // ローカルタイム取得
            RTC_GetFormStr( &tm, ctm , "*Y*m*d*H*i*s" );

            // ローカルタイムでの時刻
            iCnt += sprintf(sndbuf+iCnt,"DTIME=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%s",ctm);
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // UNIXタイムでの時刻
            iCnt += sprintf(sndbuf+iCnt,"UTC=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%lu",ultime);
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // サマータイム情報
            iCnt += sprintf(sndbuf+iCnt,"DST=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%s",my_config.device.Summer);
            if(iPara.word > sizeof(my_config.device.Summer))  iPara.word = sizeof(my_config.device.Summer);     //sakaguchi add UT-0018
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // 日付書式
            iCnt += sprintf(sndbuf+iCnt,"FORM=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%s",my_config.device.TimeForm);
            if(iPara.word > sizeof(my_config.device.TimeForm))  iPara.word = sizeof(my_config.device.TimeForm);     //sakaguchi add UT-0018
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // 時差情報
            iCnt += sprintf(sndbuf+iCnt,"DIFF=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%s",my_config.device.TimeDiff);
            if(iPara.word > sizeof(my_config.device.TimeDiff))  iPara.word = sizeof(my_config.device.TimeDiff);     //sakaguchi add UT-0018
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // タイムゾーン名
            iCnt += sprintf(sndbuf+iCnt,"ZONE=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%s",my_config.device.TimeZone);
            if(iPara.word > sizeof(my_config.device.TimeZone))  iPara.word = sizeof(my_config.device.TimeZone);     //sakaguchi add UT-0018
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // サマータイム補正値
            usWork = (uint16_t)GetSummerAdjust( ultime ) / 60;
            iCnt += sprintf(sndbuf+iCnt,"BIAS=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%+.02d",usWork);
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // 時刻同期
             // 警報機能
            iCnt += sprintf(sndbuf+iCnt,"SYNC=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%d",iChgChartoBinary(my_config.device.TimeSync,sizeof(my_config.device.TimeSync)));
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // ステータス
            G_HttpCmd[icmd].status = ERR( CMD, NOERROR );
            status.longword = G_HttpCmd[icmd].status;
            iCnt += sprintf(sndbuf+iCnt,"STATUS=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%04hd-%04hd", (UINT)status.word.hi, (UINT)status.word.lo);
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;
            break;


        case HTTP_CMD_RWARP:        // 警報設定（取得）
            // 警報機能
            iCnt += sprintf(sndbuf+iCnt,"ENABLE=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%d",iChgChartoBinary(my_config.warning.Enable,sizeof(my_config.warning.Enable)));
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // 警報監視間隔
            iCnt += sprintf(sndbuf+iCnt,"INT=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%d",iChgChartoBinary(my_config.warning.Interval,sizeof(my_config.warning.Interval)));
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // 警報の発信方法
            iCnt += sprintf(sndbuf+iCnt,"ROUTE=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%d",iChgChartoBinary(my_config.warning.Route,sizeof(my_config.warning.Route)));
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // 接点出力に反映する条件
            iCnt += sprintf(sndbuf+iCnt,"EXT=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%d",iChgChartoBinary(my_config.warning.Ext,sizeof(my_config.warning.Ext)));
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // ステータス
            G_HttpCmd[icmd].status = ERR( CMD, NOERROR );
            status.longword = G_HttpCmd[icmd].status;
            iCnt += sprintf(sndbuf+iCnt,"STATUS=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%04hd-%04hd", (UINT)status.word.hi, (UINT)status.word.lo);
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;
            break;


        case HTTP_CMD_RCURP:        // モニタリング設定（取得）
            // モニタリング機能
            iCnt += sprintf(sndbuf+iCnt,"ENABLE=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%d",iChgChartoBinary(my_config.monitor.Enable,sizeof(my_config.monitor.Enable)));
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // モニタリングデータの送信方法
            iCnt += sprintf(sndbuf+iCnt,"ROUTE=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%d",iChgChartoBinary(my_config.monitor.Route,sizeof(my_config.monitor.Route)));
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // 送信間隔
            iCnt += sprintf(sndbuf+iCnt,"INT=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%d",iChgChartoBinary(my_config.monitor.Interval,sizeof(my_config.monitor.Interval)));
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // FTPの送信先フォルダ
            iCnt += sprintf(sndbuf+iCnt,"DIR=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%s",my_config.monitor.Dir);
            if(iPara.word > sizeof(my_config.monitor.Dir))  iPara.word = sizeof(my_config.monitor.Dir);     //sakaguchi add UT-0018
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // FTPで送信するファイル名
            iCnt += sprintf(sndbuf+iCnt,"FNAME=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%s",my_config.monitor.Fname);
            if(iPara.word > sizeof(my_config.monitor.Fname))  iPara.word = sizeof(my_config.monitor.Fname);     //sakaguchi add UT-0018
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // ステータス
            G_HttpCmd[icmd].status = ERR( CMD, NOERROR );
            status.longword = G_HttpCmd[icmd].status;
            iCnt += sprintf(sndbuf+iCnt,"STATUS=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%04hd-%04hd", (UINT)status.word.hi, (UINT)status.word.lo);
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;
            break;


        case HTTP_CMD_RSUCP:        // 記録データ送信設定（取得）
            // 記録データ送信機能
            iCnt += sprintf(sndbuf+iCnt,"ENABLE=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%d",iChgChartoBinary(my_config.suction.Enable,sizeof(my_config.suction.Enable)));
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // 記録データ送信方法
            iCnt += sprintf(sndbuf+iCnt,"ROUTE=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%d",iChgChartoBinary(my_config.suction.Route,sizeof(my_config.suction.Route)));
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // 送信周期
            iCnt += sprintf(sndbuf+iCnt,"EVT0=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%s",my_config.suction.Event0);
            if(iPara.word > sizeof(my_config.suction.Event0))  iPara.word = sizeof(my_config.suction.Event0);     //sakaguchi add UT-0018
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // 送信日時1
            iCnt += sprintf(sndbuf+iCnt,"EVT1=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%s",my_config.suction.Event1);
            if(iPara.word > sizeof(my_config.suction.Event1))  iPara.word = sizeof(my_config.suction.Event1);     //sakaguchi add UT-0018
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // 送信日時2
            iCnt += sprintf(sndbuf+iCnt,"EVT2=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%s",my_config.suction.Event2);
            if(iPara.word > sizeof(my_config.suction.Event2))  iPara.word = sizeof(my_config.suction.Event2);     //sakaguchi add UT-0018
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // 送信日時3
            iCnt += sprintf(sndbuf+iCnt,"EVT3=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%s",my_config.suction.Event3);
            if(iPara.word > sizeof(my_config.suction.Event3))  iPara.word = sizeof(my_config.suction.Event3);     //sakaguchi add UT-0018
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // 送信日時4
            iCnt += sprintf(sndbuf+iCnt,"EVT4=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%s",my_config.suction.Event4);
            if(iPara.word > sizeof(my_config.suction.Event4))  iPara.word = sizeof(my_config.suction.Event4);     //sakaguchi add UT-0018
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // 送信日時5
            iCnt += sprintf(sndbuf+iCnt,"EVT5=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%s",my_config.suction.Event5);
            if(iPara.word > sizeof(my_config.suction.Event5))  iPara.word = sizeof(my_config.suction.Event5);     //sakaguchi add UT-0018
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // 送信日時6
            iCnt += sprintf(sndbuf+iCnt,"EVT6=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%s",my_config.suction.Event6);
            if(iPara.word > sizeof(my_config.suction.Event6))  iPara.word = sizeof(my_config.suction.Event6);     //sakaguchi add UT-0018
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // 送信日時7
            iCnt += sprintf(sndbuf+iCnt,"EVT7=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%s",my_config.suction.Event7);
            if(iPara.word > sizeof(my_config.suction.Event7))  iPara.word = sizeof(my_config.suction.Event7);     //sakaguchi add UT-0018
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // 送信日時8
            iCnt += sprintf(sndbuf+iCnt,"EVT8=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%s",my_config.suction.Event8);
            if(iPara.word > sizeof(my_config.suction.Event8))  iPara.word = sizeof(my_config.suction.Event8);     //sakaguchi add UT-0018
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // FTPの送信先フォルダ
            iCnt += sprintf(sndbuf+iCnt,"DIR=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%s",my_config.suction.Dir);
            if(iPara.word > sizeof(my_config.suction.Dir))  iPara.word = sizeof(my_config.suction.Dir);     //sakaguchi add UT-0018
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // FTP送信時のファイル名
            iCnt += sprintf(sndbuf+iCnt,"FNAME=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%s",my_config.suction.Fname);
            if(iPara.word > sizeof(my_config.suction.Fname))  iPara.word = sizeof(my_config.suction.Fname);     //sakaguchi add UT-0018
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // ステータス
            G_HttpCmd[icmd].status = ERR( CMD, NOERROR );
            status.longword = G_HttpCmd[icmd].status;
            iCnt += sprintf(sndbuf+iCnt,"STATUS=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%04hd-%04hd", (UINT)status.word.hi, (UINT)status.word.lo);
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;
            break;


        case HTTP_CMD_RSETF:        // 登録情報設定（取得）

            //登録情報が更新された際は全データを通知する。
            ulsize = SFM_REGIST_SECT;                   // セクタサイズに補正
            imax = (int)(1024*32 / SFM_REGIST_SECT);    // 分割数（32KB/セクタサイズ）

            //登録情報コマンド作成
            for( isector=0; isector<imax; isector++ ){

                if( isector>=1 ){                 // 連続して生成する場合
                    // コマンドサイズを計算
                    iPara.word = (uint16_t)(iCnt-4-S_iHttpCnt);     //コマンド+パラメーラ数（T2(2byte)+コマンドサイズ領域(2byte)を減算）
                    sndbuf[S_iHttpCnt+2] = iPara.byte_lo;
                    sndbuf[S_iHttpCnt+3] = iPara.byte_hi;

                    // チェックサムを計算
                    usWork = 0;
//                    for(i=4; i<iCnt; i++){
                    for(i=4; i<iCnt-S_iHttpCnt; i++){
                        usWork = (uint16_t) (usWork +sndbuf[S_iHttpCnt+i]);
                    }
                    iPara.word = usWork;
                    sndbuf[iCnt++] = iPara.byte_lo;
                    sndbuf[iCnt++] = iPara.byte_hi;

                    S_iHttpCnt = iCnt;                      // 開始カウンタを更新

                    iCnt += sprintf(sndbuf+iCnt,"T2");
                    iCnt += sprintf(sndbuf+iCnt,"SZ");      // コマンドサイズ領域（ダミー格納）
                    iCnt += sprintf(sndbuf+iCnt,"%s:", HTTP_T2Cmd[icmd]);
                }

                // 登録情報：取得失敗
                if( false == bhttp_regist_read( cRejist, ulsize*(uint32_t)isector, ulsize ) ){
                    // ステータス
                    G_HttpCmd[icmd].status = ERR( CMD, RUNNING );       // コマンド処理中をセット
                    status.longword = G_HttpCmd[icmd].status;
                    iCnt += sprintf(sndbuf+iCnt,"STATUS=");
                    iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%04hd-%04hd", (UINT)status.word.hi, (UINT)status.word.lo);
                    sndbuf[iCnt++] = iPara.byte_lo;
                    sndbuf[iCnt++] = iPara.byte_hi;
                    iCnt += iPara.word;
                    break;                      // 処理終了
                }
                // 登録情報：取得成功
                else{
                    // 登録情報のOFFSET0
                    iCnt += sprintf(sndbuf+iCnt,"OFFSET=");
                    iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%lu", ulsize*(uint32_t)isector);
                    sndbuf[iCnt++] = iPara.byte_lo;
                    sndbuf[iCnt++] = iPara.byte_hi;
                    iCnt += iPara.word;

                    // 登録情報のバイト列
                    iCnt += sprintf(sndbuf+iCnt,"DATA=");
                    memcpy(sndbuf+iCnt+2, &cRejist[0], (size_t)ulsize);
                    iPara.word = (uint16_t)ulsize;
                    sndbuf[iCnt++] = iPara.byte_lo;
                    sndbuf[iCnt++] = iPara.byte_hi;
                    iCnt += iPara.word;

                    // ステータス
                    G_HttpCmd[icmd].status = ERR( CMD, NOERROR );
                    status.longword = G_HttpCmd[icmd].status;
                    iCnt += sprintf(sndbuf+iCnt,"STATUS=");
                    iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%04hd-%04hd", (UINT)status.word.hi, (UINT)status.word.lo);
                    sndbuf[iCnt++] = iPara.byte_lo;
                    sndbuf[iCnt++] = iPara.byte_hi;
                    iCnt += iPara.word;
                }
            }
            break;


        case HTTP_CMD_RVGRP:        // グループ情報（取得）
            // 取得失敗？
            if( false == bhttp_vgroup_read( cVGroup )){
                //ステータス
                G_HttpCmd[icmd].status = ERR( CMD, RUNNING );       // コマンド処理中をセット
            }
            // 取得成功
            else{
                // 登録情報のバイト列
                size = (uint16_t)(cVGroup[0] + cVGroup[1]*256);
                if(size == 0xFFFF)    size = 0;                     // sakaguchi UT-0041
                iCnt += sprintf(sndbuf+iCnt,"DATA=");
                memcpy(sndbuf+iCnt+2, &cVGroup[2], size);
                iPara.word = size;
                sndbuf[iCnt++] = iPara.byte_lo;
                sndbuf[iCnt++] = iPara.byte_hi;
                iCnt += iPara.word;

                // ステータス
                G_HttpCmd[icmd].status = ERR( CMD, NOERROR );
            }
            // ステータス
            status.longword = G_HttpCmd[icmd].status;
            iCnt += sprintf(sndbuf+iCnt,"STATUS=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%04hd-%04hd", (UINT)status.word.hi, (UINT)status.word.lo);
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;
            break;


        case HTTP_CMD_RPRXY:         // プロキシ設定（取得）
            // HTTPにてプロキシサーバを経由する
            iCnt += sprintf(sndbuf+iCnt,"HTTPEN=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%d",iChgChartoBinary(my_config.network.ProxyHttp,sizeof(my_config.network.ProxyHttp)));
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // HTTPプロキシサーバのホスト名
            iCnt += sprintf(sndbuf+iCnt,"HTTPSV=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%s",my_config.network.ProxyHttpServ);
            if(iPara.word > sizeof(my_config.network.ProxyHttpServ))  iPara.word = sizeof(my_config.network.ProxyHttpServ);     //sakaguchi add UT-0018
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // HTTPプロキシサーバのポート番号
            iCnt += sprintf(sndbuf+iCnt,"HTTPPT=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%d",iChgChartoBinary(my_config.network.ProxyHttpPort,sizeof(my_config.network.ProxyHttpPort)));
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // ステータス
            G_HttpCmd[icmd].status = ERR( CMD, NOERROR );
            status.longword = G_HttpCmd[icmd].status;
            iCnt += sprintf(sndbuf+iCnt,"STATUS=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%04hd-%04hd", (UINT)status.word.hi, (UINT)status.word.lo);
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;
            break;

        case HTTP_CMD_RNETP:        // ネットワーク情報設定（取得）
            // ネットワークパスワード
            iCnt += sprintf(sndbuf+iCnt,"PW=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%s",my_config.network.NetPass);
            if(iPara.word > sizeof(my_config.network.NetPass))  iPara.word = sizeof(my_config.network.NetPass);     //sakaguchi add UT-0018
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;

            // ステータス
            G_HttpCmd[icmd].status = ERR( CMD, NOERROR );
            status.longword = G_HttpCmd[icmd].status;
            iCnt += sprintf(sndbuf+iCnt,"STATUS=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%04hd-%04hd", (UINT)status.word.hi, (UINT)status.word.lo);
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;
            break;

        default:
            // ステータス
            G_HttpCmd[icmd].status = ERR( CMD, FORMAT );        // コマンドフォーマットエラー
            status.longword = G_HttpCmd[icmd].status;
            iCnt += sprintf(sndbuf+iCnt,"STATUS=");
            iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%04hd-%04hd", (UINT)status.word.hi, (UINT)status.word.lo);
            sndbuf[iCnt++] = iPara.byte_lo;
            sndbuf[iCnt++] = iPara.byte_hi;
            iCnt += iPara.word;
            break;
    }

    //コマンドサイズを計算
    iPara.word = (uint16_t)(iCnt-4-S_iHttpCnt);        // 'T2'+size 4byte減算）
    sndbuf[S_iHttpCnt+2] = iPara.byte_lo;
    sndbuf[S_iHttpCnt+3] = iPara.byte_hi;

    //チェックサムを計算
    usWork = 0;
//    for(i=4; i<iCnt; i++){
    for(i=4; i<iCnt-S_iHttpCnt; i++){
        usWork = (uint16_t) (usWork +sndbuf[S_iHttpCnt+i]);
    }

    iPara.word = usWork;
    sndbuf[iCnt++] = iPara.byte_lo;
    sndbuf[iCnt++] = iPara.byte_hi;

    S_iHttpCnt = iCnt;                      // バッファカウンタ更新

    iRet = E_OK;

    return(iRet);
}

//******************************************************************************
//* 概要   | ACT送信
//* 引数１ | [in]icmd : 送信コマンドIndex
//* 引数２ | [in]act  : ACT指定(0/1)
//* 戻り値 | 処理結果 正常：E_OK
//* 詳細   | 指定したコマンドでステータス取得(ACT)をcmd threadへ送信する
//******************************************************************************/
static int ihttp_act_snd(int icmd, uint16_t act)
{
    int         iRet = E_NG;                    // 戻り値
    int         i;
//    static uint32_t    cmd_msg[4];
    static CmdQueue_t cmd_msg;      ///< キューでcmd_threadに送るメッセージ
    uint32_t    actual_events;
    int         iCnt = 0x00;                    // コマンドバイト数
    LO_HI       iPara;                          // パラメータサイズ
    uint16_t    usWork = 0x00;                  // 作業用変数
//    static int32_t txLen;      //CmdStatusSizeの代わりに  cmd_threadから受け取る 2020,Aug.26      // sakaguchi del 2020.08.27

    char        cWork[1024];                    // 作業用バッファ
    memset(cWork,0x00,sizeof(cWork));

    // コマンド生成
    iCnt += sprintf(cWork+iCnt,"%s:", HTTP_T2Cmd[icmd]);
    iCnt += sprintf(cWork+iCnt,"ACT=");                     // コマンド実行の種類
    usWork = act;                                           // ACT指定
    iPara.word = (uint16_t)sprintf(cWork+iCnt+2,"%d",usWork);
    cWork[iCnt++] = iPara.byte_lo;
    cWork[iCnt++] = iPara.byte_hi;
    iCnt += iPara.word;

    // チェックサム生成
    usWork = 0;
    for(i=0; i<iCnt; i++){
        usWork = (uint16_t) (usWork + cWork[i]);
    }
    iPara.word = usWork;
    cWork[iCnt++] = iPara.byte_lo;
    cWork[iCnt++] = iPara.byte_hi;

    //コマンドスレッド実行中は待つ 2020.Jul.15
    while((TX_SUSPENDED != cmd_thread.tx_thread_state) /*&& (TX_EVENT_FLAG != cmd_thread.tx_thread_state)*/){
        tx_thread_sleep (1);
    }
    // cmd threadへの引き渡し
    HTTP.rxbuf.command = (char)icmd;                // コマンドIndex格納
    HTTP.rxbuf.length = (uint16_t)(iCnt-2);         // コマンド長格納(sum-2byte)
    memcpy( HTTP.rxbuf.data, cWork, (size_t)iCnt );

    // cmd threadへ送信
//    cmd_msg[0] = CMD_HTTP;
//    cmd_msg[1] = (uint32_t)&HTTP.rxbuf.header;      // コマンド処理する受信データフレームの先頭ポインタ
//    cmd_msg[2] = (uint32_t)&StsArea;
    cmd_msg.CmdRoute = CMD_HTTP;                //コマンド キュー  コマンド実行要求元
//    cmd_msg.pT2Command = (char *)&HTTP.rxbuf.header;      // コマンド処理する受信データフレームの先頭ポインタ
//    cmd_msg.pT2Status = StsArea;
    cmd_msg.pStatusSize = (int32_t *)&txLen;              //コマンド処理された応答データフレームサイズ
    cmd_msg.pT2Command = &HTTP.rxbuf.command;    //コマンド処理する受信���ータフレームの先頭ポインタ
//    cmd_msg.pT2Status = &HTTP.rxbuf.header;    //コマンド処理された応答データフレームの先頭ポインタ（2020.Jun.15現在未対応）
    cmd_msg.pT2Status = &S_ucCmdAns[0];          //コマンド処理された応答データフレームの先頭ポインタ               // sakaguchi cg 2020.08.28

//    tx_queue_send(&cmd_queue, cmd_msg, TX_WAIT_FOREVER);
    tx_queue_send(&cmd_queue, &cmd_msg, TX_WAIT_FOREVER);
    tx_event_flags_get( &g_command_event_flags, FLG_CMD_END, TX_OR_CLEAR, &actual_events, TX_NO_WAIT ); // sakaguchi add 2020.08.28
    tx_event_flags_set (&g_command_event_flags, FLG_CMD_EXEC,TX_OR);
    tx_thread_resume(&cmd_thread);
//    tx_event_flags_get (&g_command_event_flags, FLG_CMD_END ,TX_OR_CLEAR, &actual_events,TX_WAIT_FOREVER);    // sakaguchi del 2020.08.27

    iRet = E_OK;
    return(iRet);
}


//******************************************************************************
//* 概要   | cmdthreadへの送信処理
//* 引数１ | [in]icmd : 送信コマンドIndex
//* 引数２ | [in]cbuf : 送信データ
//* 戻り値 | 処理結果 正常：E_OK
//* 詳細   | 登録情報の変更(WSETF)、ルート証明書変更(WRTCE)で使用する
//******************************************************************************/
static int ihttp_cmd_thread_snd(int icmd, char *cbuf)
{
    int         iRet = E_NG;                    //戻り値
//    int         i;
//    uint32_t    cmd_msg[4];
    static CmdQueue_t cmd_msg;      ///< キューでcmd_threadに送るメッセージ
    uint32_t    actual_events;
//    static int32_t txLen;      //CmdStatusSizeの代わりに  cmd_threadから受け取る 2020,Aug.26      // sakaguchi del 2020.08.27

    HTTP.rxbuf.command = (char)icmd;            // 受信コマンドIndex格納

    HTTP.rxbuf.length = *(uint16_t *)&cbuf[2];
    memcpy(HTTP.rxbuf.data, cbuf+4, (size_t)HTTP.rxbuf.length+2 );  // 受信コマンド格納

    //コマンドスレッド実行中は待つ 2020.Jul.15
    while((TX_SUSPENDED != cmd_thread.tx_thread_state) /*&& (TX_EVENT_FLAG != cmd_thread.tx_thread_state)*/){
        tx_thread_sleep (1);
    }

    cmd_msg.CmdRoute = CMD_HTTP;                //コマンド キュー  コマンド実行要求元

//    cmd_msg.pT2Command = (char *)&HTTP.rxbuf.header;  // コマンド処理する受信データフレームの先頭ポインタ
//    cmd_msg.pT2Status = StsArea;
    cmd_msg.pStatusSize = (int32_t *)&txLen;              //コマンド処理された応答データフレームサイズ

    cmd_msg.pT2Command = &HTTP.rxbuf.command;    //コマンド処理する受信データフレームの先頭ポインタ
//    cmd_msg.pT2Status = &HTTP.rxbuf.header;    //コマンド処理された応答データフレームの先頭ポインタ（2020.Jun.15現在未対応）
    cmd_msg.pT2Status = &S_ucCmdAns[0];          //コマンド処理された応答データフレームの先頭ポインタ                   // sakaguchi cg 2020.08.27

//    tx_queue_send( &cmd_queue, cmd_msg, TX_WAIT_FOREVER );
    tx_queue_send( &cmd_queue, &cmd_msg, TX_WAIT_FOREVER );
    tx_event_flags_get( &g_command_event_flags, FLG_CMD_END, TX_OR_CLEAR, &actual_events, TX_NO_WAIT ); // sakaguchi add 2020.08.28
    tx_event_flags_set( &g_command_event_flags, FLG_CMD_EXEC, TX_OR );
    tx_thread_resume( &cmd_thread );
//    tx_event_flags_get( &g_command_event_flags, FLG_CMD_END, TX_OR_CLEAR, &actual_events, TX_WAIT_FOREVER );      // sakaguchi del 2020.08.27

    iRet = E_OK;
    return(iRet);
}



//******************************************************************************
//* 概要    | ログデータ作成
//* 引数１  | [out]*sndbuf : 生成したログデータ
//* 引数２  | [in]act：ログ取得方法（全ログ：0 / 差分：1）
//* 引数３  | [in]area：ログ種類（通常ログ：0 / デバッグログ：1）
//* 引数４  | [in]file：ファイル種別（通常：FILE_L / テスト：FILE_L_T）
//* 戻り値  | 処理結果 成功：E_OK / 失敗:E_NG
//* 詳細    | 
//******************************************************************************/
//static int ihttp_logdata_make(char *sndbuf, int buffsize, int act, int area )
static int ihttp_logdata_make(char *sndbuf, int act, int area, int file)      // 2023.03.01
{
    int         iRet = E_NG;                    // 戻り値
    int         i;                              // ループカウンタ
    int         iCnt = 0x00;                    // コマンドバイト数
    LO_HI       iPara;                          // パラメータサイズ
    uint16_t    usWork = 0x00;                  // 作業用変数
    LO_HI_LONG  status;                         // 実行結果
    int len = 0;
    int size;

    GetLogInfo( area );                         // ログの使用状況を取得         // sakaguchi 2020.12.07

    if ( act == 0 ) {
        //GetLogInfo( area );
        //GetLog( 0, -1, 0, area );			    // ポインタセット
        //GetLog_http( 0, -1, 0, area );			    // ポインタセット           // sakaguchi 2020.12.07
        GetLog_http( 0, -1, 0, area, file );			// ポインタセット           // 2023.03.01
    }

    // バッファカウンタセット
    iCnt = S_iHttpCnt;

    // コマンド名
    iCnt += sprintf(sndbuf+iCnt,"T2");
    iCnt += sprintf(sndbuf+iCnt,"SZ");           // コマンドサイズ（ダミー格納）
    iCnt += sprintf(sndbuf+iCnt,"ELOGS:");

    // ログデータ
    iCnt += sprintf(sndbuf+iCnt,"DATA=");

// sakaguchi 2020.12.07 ↓
#if 0
    for(;;){
        size = GetLog( sndbuf+iCnt+2+len, 0, 1024*30, area );	// GetLogで指定できるバッファサイズは32767byteのため、30KBとする
        if(size == 0){
            break;
        }else{
            iRet = E_OK;            // 読込OK
        }
        len += size;
        //連続読込判断
        if(buffsize <= (len + 1024*30 + 100)){
            if(1024*30 == size){
                iRet = E_OVER;      // バッファオーバー（次データ有り）
            }
            break;
        }
    }
#endif
    //size = GetLog_http( sndbuf+iCnt+2+len, 0, 1024*30, area );	// GetLogで指定できるバッファサイズは32767byteのため、30KBとする
    size = GetLog_http( sndbuf+iCnt+2+len, 0, 1024*30, area, file );	// GetLogで指定できるバッファサイズは32767byteのため、30KBとする    // 2023.03.01
    if(size == 0){
        return(iRet);       // ここで終了
    }
    iRet = E_OK;            // 読込OK
    len += size;
// sakaguchi 2020.12.07 ↑

    iPara.word = (uint16_t)len;
    sndbuf[iCnt++] = iPara.byte_lo;
    sndbuf[iCnt++] = iPara.byte_hi;
    iCnt += iPara.word;

    // ステータス
    status.longword = ERR( CMD, NOERROR );
    iCnt += sprintf(sndbuf+iCnt,"STATUS=");
    iPara.word = (uint16_t)sprintf(sndbuf+iCnt+2,"%04hd-%04hd", (UINT)status.word.hi, (UINT)status.word.lo);
    sndbuf[iCnt++] = iPara.byte_lo;
    sndbuf[iCnt++] = iPara.byte_hi;
    iCnt += iPara.word;

    // コマンドサイズを計算
    iPara.word = (uint16_t)(iCnt-4-S_iHttpCnt);
    sndbuf[S_iHttpCnt+2] = iPara.byte_lo;
    sndbuf[S_iHttpCnt+3] = iPara.byte_hi;

    // チェックサムを計算
    usWork = 0;
//    for(i = 4; i < iCnt; i++){
    for(i = 4; i < iCnt-S_iHttpCnt; i++){
        usWork = (uint16_t) (usWork +sndbuf[S_iHttpCnt+i]);
    }
    iPara.word = usWork;
    sndbuf[iCnt++] = iPara.byte_lo;
    sndbuf[iCnt++] = iPara.byte_hi;

    S_iHttpCnt = iCnt;                  // バッファカウンタ更新

    return(iRet);
}


//******************************************************************************
//* 概要   | サーバ受信コマンド解析
//* 引数１ | [in] ifile : サーバへの送信ファイル
//* 引数２ | [in] *cbuf : サーバからの受信コマンド
//* 戻り値 | 解析結果   true：正常 / false：異常
//* 詳細   | 引数２の受信コマンドは"T2"から含めること
//******************************************************************************/
static bool bhttp_srvcmd_analyze(int ifile, char *cbuf)
{
    bool bRet = false;                  // 戻り値
    int icmd;                           // コマンドIndex
    LO_HI len;                          // コマンドサイズ
//    uint32_t cmd_msg[4];
    static CmdQueue_t cmd_msg;      ///< キューでcmd_threadに送るメッセージ
    uint32_t actual_events;
    uint32_t status;
    uint8_t  ucStep;
//    static int32_t txLen;      //CmdStatusSizeの代わりに  cmd_threadから受け取る 2020,Aug.26      // sakaguchi del 2020.08.27

    memset( S_ucErrCmd, 0x00, sizeof(S_ucErrCmd) );


    switch(ifile)
    {
        //------------------------------------
        //---   操作リクエスト送信時の応答   ---
        //------------------------------------
        case FILE_R:

            if( 2 == S_ucNextCmd ){             // NextCommand：全設定取得？
//                G_HttpCmd[FILE_C].reqid = S_ulReqID;                    // [FILE]リクエストID格納
                G_HttpFile[FILE_C].reqid = S_ulReqID;                   // [FILE]リクエストID格納              // sakaguchi 2020.09.02
                G_HttpFile[FILE_C].sndflg = SND_ON;                     // [FILE]機器設定：送信有りをセット     // sakaguchi 2020.09.02

                G_HttpCmd[HTTP_CMD_RSETF].sndflg = SND_ON;              // 登録情報送信     //sakaguchi UT-0029
                vhttp_sysset_sndon();                                   // 親機設定送信
                tx_event_flags_set (&event_flags_http, FLG_HTTP_READY, TX_OR);      // HTTPスレッド動作許可ON

                G_HttpCmd[HTTP_CMD_NONE].reqid = S_ulReqID;             // [CMD]リクエストID格納
                G_HttpCmd[HTTP_CMD_NONE].ansflg = SND_ON;               // [CMD]操作リクエスト結果：送信有りをセット
                G_HttpCmd[HTTP_CMD_NONE].status = ERR(CMD,NOERROR);     // [CMD]ステータス：エラー無しをセット
                G_HttpCmd[HTTP_CMD_NONE].save = 1;                      // [CMD]一時格納：操作リクエスト結果でBody部は送信しない
                G_HttpFile[FILE_A].sndflg = SND_ON;                     // [FILE]操作リクエスト結果：送信有りをセット
                return(true);                                           // 処理終了
            }

            //受信コマンドのIndex取得
            icmd = ihttp_T2cmd_index_get( cbuf+4 );

            if(HTTP_CMD_NONE == icmd){                  // 未定義のコマンド
                if('\0' != cbuf[4]){
                    memcpy( S_ucErrCmd, cbuf+4, 5 );                        // エラー応答用にコマンドを格納
                    G_HttpCmd[HTTP_CMD_NONE].reqid = S_ulReqID;             // [CMD]リクエストID格納
                    G_HttpCmd[HTTP_CMD_NONE].ansflg = SND_ON;               // [CMD]操作リクエスト結果：送信有りをセット
                    G_HttpCmd[HTTP_CMD_NONE].status = ERR(CMD,FORMAT);      // [CMD]ステータス：コマンドフォーマットエラー
                    G_HttpFile[FILE_A].sndflg = SND_ON;                     // [FILE]操作リクエスト結果：送信有りをセット
                }
                return(false);                  // 処理終了
            }

            // リクエストIDが同じコマンドが処理中の場合は、要求を破棄する
            if(G_HttpCmd[icmd].reqid == S_ulReqID){
                Printf("========= same reqid end =========== \r\n");
                return(false);                  // 処理終了
            }


            // ルート証明書の書換
            if((HTTP_CMD_WRTCE == icmd) || (HTTP_CMD_WSETF == icmd)){

                // 受信データの解析処理とデータ格納
                status = ulhttp_T2cmd_analyze(cbuf);

                // 解析エラーの場合
                if(status != ERR(CMD,NOERROR)){
                    G_HttpCmd[icmd].reqid = S_ulReqID;                      // [CMD]リクエストID格納
                    G_HttpCmd[icmd].ansflg = SND_ON;                        // [CMD]操作リクエスト結果：送信有りをセット
                    G_HttpCmd[icmd].status = status;                        // [CMD]ステータス
                    G_HttpFile[FILE_A].sndflg = SND_ON;                     // [FILE]操作リクエスト結果：送信有りをセット
                    return(true);                   // 処理終了
                }

// sakaguchi 2021.07.20 ↓
                if(WAIT_REQUEST != WaitRequest){            // 設定変更コマンド受信時は自律動作を一時停止させる
                    WaitRequest = WAIT_REQUEST;             // 一時停止要求
                    mate_time_flag = 1;                     // 自律動作の一時停止フラグON
                    mate_time = 60;                         // 自律動作の一時停止時間
                }
// sakaguchi 2021.07.20 ↑

                // T2コマンドを生成
                memset(http_buffer, 0x00, sizeof(http_buffer));

                // OFFSETが0以外の場合、STEP1は実行済みのためSTEP2へ移行
                ucStep = (0 != G_HttpCmd[icmd].offset) ? SND_STEP2 : SND_STEP1;

                if(E_OK == ihttp_T2cmd_make_save(icmd, ucStep, http_buffer)){
                    // http_threadからcmd_threadに対し、保護解除(step1)⇒データ送信(step2)⇒保護開始(step3)を送信する
                    G_HttpCmd[icmd].reqid = S_ulReqID;                       // [CMD]リクエストID格納
                    G_HttpCmd[icmd].ansflg = ucStep;                         // [CMD]操作リクエスト結果：step1送信をセット
                    ihttp_cmd_thread_snd(icmd, http_buffer);                 // cmd_threadへ送信
                    bRet = true;
                }
                return(bRet);                   // 処理終了
            }

            // ルート証明書の取得 // ファームウェア更新
            if((HTTP_CMD_RRTCE == icmd) || (HTTP_CMD_EFIRM == icmd)){

                // 受信データの解析処理とデータ格納
                status = ulhttp_T2cmd_analyze(cbuf);

                // 解析エラーの場合
                if(status != ERR(CMD,NOERROR)){
                    G_HttpCmd[icmd].reqid = S_ulReqID;                      // [CMD]リクエストID格納
                    G_HttpCmd[icmd].ansflg = SND_ON;                        // [CMD]操作リクエスト結果：送信有りをセット
                    G_HttpCmd[icmd].status = status;                        // [CMD]ステータス
                    G_HttpFile[FILE_A].sndflg = SND_ON;                     // [FILE]操作リクエスト結果：送信有りをセット
                    return(true);                   // 処理終了
                }
            }

            // 設定変更コマンド // ルート証明書の取得 // ファームウェア更新
            if(('W' == cbuf[4]) || (HTTP_CMD_RRTCE == icmd) || (HTTP_CMD_EFIRM == icmd)){
// sakaguchi 2021.08.02 ↓
                //if(WAIT_REQUEST != WaitRequest){            // 設定変更コマンド受信時は自律動作を一時停止させる // sakaguchi 2021.01.25
                    WaitRequest = WAIT_REQUEST;             // 一時停止要求
                    mate_time_flag = 1;                     // 自律動作の一時停止フラグON
                    mate_time = 60;                         // 自律動作の一時停止時間
                //}
// sakaguchi 2021.08.02 ↑
                HTTP.rxbuf.command = (char)icmd;            // 受信コマンドIndex格納
                len.byte_lo = cbuf[2];                      // 受信コマンドサイズ格納
                len.byte_hi = cbuf[3];
                HTTP.rxbuf.length = (uint16_t)len.word;
                memcpy(HTTP.rxbuf.data, cbuf+4, (size_t)HTTP.rxbuf.length+2 );  // 受信コマンド格納

                G_HttpCmd[icmd].reqid = S_ulReqID;          // [CMD]リクエストID格納
                G_HttpCmd[icmd].ansflg = SND_OFF;           // [CMD]操作リクエスト結果：送信無しをセット
                //コマンドスレッド実行中は待つ 2020.Jul.15
                while((TX_SUSPENDED != cmd_thread.tx_thread_state) /*&& (TX_EVENT_FLAG != cmd_thread.tx_thread_state)*/){
                    tx_thread_sleep (1);
                }

                cmd_msg.CmdRoute = CMD_HTTP;                           // cmd_threadへ要求

//                cmd_msg.pT2Command = (char *)&HTTP.rxbuf.header;    //コマンド処理する受信データフレームの先頭ポインタ
//                cmd_msg.pT2Status = StsArea;
                cmd_msg.pStatusSize = (int32_t *)&txLen;              //コマンド処理された応答データフレームサイズ
                cmd_msg.pT2Command = &HTTP.rxbuf.command;    //コマンド処理する受信データフレームの先頭ポインタ
//                cmd_msg.pT2Status = &HTTP.rxbuf.header;    //コマンド処理された応答データフレームの先頭ポインタ（2020.Jun.15現在未対応）
                cmd_msg.pT2Status = &S_ucCmdAns[0];          //コマンド処理された応答データフレームの先頭ポインタ               // sakaguchi cg 2020.08.27

                tx_queue_send( &cmd_queue, &cmd_msg, TX_WAIT_FOREVER );
                tx_event_flags_get( &g_command_event_flags, FLG_CMD_END, TX_OR_CLEAR, &actual_events, TX_NO_WAIT ); // sakaguchi add 2020.08.28
                tx_event_flags_set( &g_command_event_flags, FLG_CMD_EXEC, TX_OR );
                tx_thread_resume( &cmd_thread );
//                tx_event_flags_get( &g_command_event_flags, FLG_CMD_END, TX_OR_CLEAR, &actual_events, TX_WAIT_FOREVER );  // sakaguchi del 2020.08.27
                bRet = true;
            }
            // 設定取得コマンド
            else if('R' == cbuf[4]){
                G_HttpCmd[icmd].reqid = S_ulReqID;           // [CMD]リクエストID格納
                G_HttpCmd[icmd].ansflg = SND_ON;             // [CMD]操作リクエスト結果：送信有りをセット
                G_HttpFile[FILE_A].sndflg = SND_ON;          // [FILE]操作リクエスト結果：送信有りをセット
                bRet = true;
            }
            // その他コマンド
            else if('E' == cbuf[4]){
                switch(icmd)
                {
                    case HTTP_CMD_EWLEX:            //40 子機の電波強度を取得する(無線通信)
                    case HTTP_CMD_EWLRU:            //41 中継機の電波強度を取得する(無線通信)
                    case HTTP_CMD_EWSTR:            //42 子機の設定を取得する(無線通信)
                    case HTTP_CMD_EWSTW:            //43 子機の設定を変更する(記録開始)(無線通信)
                    case HTTP_CMD_EWCUR:            //44 子機の現在値を取得する(無線通信)
//                    case HTTP_CMD_EWSUC:            //45 子機の記録データを取得する(無線通信)     // 対応せず削除
                    case HTTP_CMD_EWSCE:            //46 子機の検索（無線通信）
                    case HTTP_CMD_EWSCR:            //47 中継機の一斉検索（無線通信）
                    case HTTP_CMD_EWRSC:            //48 子機のモニタリングデータを取得する（無線通信）
                    case HTTP_CMD_EWBSW:            //49 一斉記録開始（無線通信）
                    case HTTP_CMD_EWRSP:            //50 子機の記録を停止する（無線通信）
                    case HTTP_CMD_EWPRO:            //51 プロテクト設定（無線通信）
                    case HTTP_CMD_EWINT:            //52 子機のモニタリング間隔変更（無線通信）
                    case HTTP_CMD_EWBLW:            //53 子機のBluetooth設定を設定する(無線通信)
                    case HTTP_CMD_EWBLR:            //54 子機のBluetooth設定を取得する(無線通信)
                    case HTTP_CMD_EWSLW:            //55 子機のスケール変換式を設定する(無線通信)
                    case HTTP_CMD_EWSLR:            //56 子機のスケール変換式を取得する(無線通信)
                    case HTTP_CMD_EWRSR:            //60 子機と中継機の一斉検索（無線通信）
                    case HTTP_CMD_EWREG:            //61 子機登録変更（無線通信）

                        // 無線通信遅延中
                        if(60 >= S_ucRfWaitCnt){

                            if(tx_mutex_get(&g_rf_mutex, WAIT_100MSEC) == TX_SUCCESS){   // ＲＦタスク実行中ではないこと
                                // 無線通信可のため処理継続
                                tx_mutex_put(&g_rf_mutex);
                            }
                            else{
                                // 無線通信中は自律一時停止と、WSSへの操作リクエスト送信タイマをセットし処理終了
// sakaguchi del 一旦削除 2020.08.28 ↓ // sakaguchi 2020.08.31 復活
                                if(WAIT_REQUEST != WaitRequest){
                                    WaitRequest = WAIT_REQUEST;             // 一時停止要求
                                    mate_time_flag = 1;                     // 自律動作の一時停止フラグON
                                    mate_time = 30;                         // 自律動作の一時停止時間
                                }
// sakaguchi del 一旦削除 2020.08.28 ↑
                                ReqOpe_time = 12;
                                S_ucRfWaitCnt++;                            // 無線通信遅延カウンタクリア
                                return(true);                               // 処理終了
                            }
                        }
                        S_ucRfWaitCnt = 0;                              // 無線通信遅延カウンタクリア
                        break;
                    default:
                        break;
                }

                HTTP.rxbuf.command = (char)icmd;             // 受信コマンドIndex格納
//                len.byte_lo = cbuf[2];                          // 受信コマンドサイズ格納
//                len.byte_hi = cbuf[3];
//                HTTP.rxbuf.length = (uint16_t)len.word;
                HTTP.rxbuf.length = *(uint16_t *)&cbuf[2];
                memcpy( HTTP.rxbuf.data, cbuf+4, (size_t)HTTP.rxbuf.length+2 );  // 受信コマンド格納

                switch(icmd)
                {
                    case HTTP_CMD_EINIT:            //初期化と再起動
                    case HTTP_CMD_EBSTS:            //ステータスの取得と設定
                    case HTTP_CMD_EMONS:            //モニタリングを開始する
                    case HTTP_CMD_EMONR:            //モニタリングで収集した無線通信情報と電池残量を取得する
                    case HTTP_CMD_EMONC:            //モニタリングで取得した電波強度を削除する
                    case HTTP_CMD_ERGRP:            //指定グループの無線通知設定取得
                    case HTTP_CMD_EAUTH:            //パスワード認証
                    case HTTP_CMD_EBADV:            //アドバタイジングデータの取得
                    case HTTP_CMD_ETWAR:            //警報テストを開始する
                    case HTTP_CMD_EWAIT:            //自律動作を開始しない時間を設定する
                    case HTTP_CMD_EWLAP:            //無線LANのアクセスポイントを検索する                         // sakaguchi 2020.09.02
                        G_HttpCmd[icmd].reqid = S_ulReqID;      // [CMD]リクエストID格納
                        G_HttpCmd[icmd].ansflg = SND_OFF;       // [CMD]操作リクエスト結果：送信無しをセット
                        break;

                    case HTTP_CMD_ELOGS:            //ログの取得および消去
//                        if( 0 == ihttp_param_get(cbuf+4, len.word, "MODE") ){   //MODE=0(ログ取得)?
                        if( 0 == ihttp_param_get(cbuf+4, HTTP.rxbuf.length, "MODE") ){   //MODE=0(ログ取得)?    // sakaguchi 2020.09.02
                            G_HttpCmd[icmd].reqid = S_ulReqID;          // [CMD]リクエストID格納
                            G_HttpCmd[icmd].save = 0;                   // [CMD]一時格納用：mode=0
                            G_HttpCmd[icmd].ansflg = SND_ON;            // [CMD]操作リクエスト結果：送信有りをセット
                            G_HttpFile[FILE_A].sndflg = SND_ON;         // [FILE]操作リクエスト結果：送信有りをセット
                            return(true);                               // 処理終了（cmdthreadへは送信せず、httpthread内でログを取得する）
                        }else{
                            G_HttpCmd[icmd].reqid = S_ulReqID;          // [CMD]リクエストID格納
                            G_HttpCmd[icmd].save = 1;                   // [CMD]一時格納用：mode=1
                            G_HttpCmd[icmd].ansflg = SND_OFF;           // [CMD]操作リクエスト結果：送信無しをセット
                        }
                        break;

                    case HTTP_CMD_ETGSM:            //テスト（3GおよびGPSモジュール）
                    case HTTP_CMD_ETLAN:            //テスト
//                        if( 0 == ihttp_param_get(cbuf+4, len.word, "ACT") ){   //ACT0?
                        if( 0 == ihttp_param_get(cbuf+4, HTTP.rxbuf.length, "ACT") ){   //ACT0?                 // sakaguchi 2020.09.02
                            G_HttpCmd[icmd].reqid = S_ulReqID;      // [CMD]リクエストID格納
                            G_HttpCmd[icmd].ansflg = SND_ACT0;      // [CMD]操作リクエスト結果：ACT0送信中をセット
                        }else{
                            G_HttpCmd[icmd].reqid = S_ulReqID;      // [CMD]リクエストID格納
                            G_HttpCmd[icmd].ansflg = SND_OFF;       // [CMD]操作リクエスト結果：送信無しをセット
                        }
                        break;

                    //無線通信関連
                    case HTTP_CMD_EWLEX:            //40 子機の電波強度を取得する(無線通信)
                    case HTTP_CMD_EWLRU:            //41 中継機の電波強度を取得する(無線通信)
                    case HTTP_CMD_EWSTR:            //42 子機の設定を取得する(無線通信)
                    case HTTP_CMD_EWSTW:            //43 子機の設定を変更する(記録開始)(無線通信)
                    case HTTP_CMD_EWCUR:            //44 子機の現在値を取得する(無線通信)
//                    case HTTP_CMD_EWSUC:            //45 子機の記録データを取得する(無線通信)     // 対応せず削除
                    case HTTP_CMD_EWSCE:            //46 子機の検索（無線通信）
                    case HTTP_CMD_EWSCR:            //47 中継機の一斉検索（無線通信）
                    case HTTP_CMD_EWRSC:            //48 子機のモニタリングデータを取得する（無線通信）
                    case HTTP_CMD_EWBSW:            //49 一斉記録開始（無線通信）
                    case HTTP_CMD_EWRSP:            //50 子機の記録を停止する（無線通信）
                    case HTTP_CMD_EWPRO:            //51 プロテクト設定（無線通信）
                    case HTTP_CMD_EWINT:            //52 子機のモニタリング間隔変更（無線通信）
                    case HTTP_CMD_EWBLW:            //53 子機のBluetooth設定を設定する(無線通信)
                    case HTTP_CMD_EWBLR:            //54 子機のBluetooth設定を取得する(無線通信)
                    case HTTP_CMD_EWSLW:            //55 子機のスケール変換式を設定する(無線通信)
                    case HTTP_CMD_EWSLR:            //56 子機のスケール変換式を取得する(無線通信)
                    case HTTP_CMD_EWRSR:            //60 子機と中継機の一斉検索（無線通信）
                    case HTTP_CMD_EWREG:            //61 子機登録変更（無線通信）
                        G_HttpCmd[icmd].reqid = S_ulReqID;      // [CMD]リクエストID格納
                        G_HttpCmd[icmd].ansflg = SND_ACT0;      // [CMD]操作リクエスト結果：ACT0送信中をセット
                        break;
                    //対象外のコマンド
                    default:
                        G_HttpCmd[icmd].reqid = S_ulReqID;           // [CMD]リクエストID格納
                        G_HttpCmd[icmd].ansflg = SND_ON;             // [CMD]操作リクエスト結果：送信有りをセット
                        G_HttpCmd[icmd].status = ERR(CMD,FORMAT);    // [CMD]ステータス：コマンドフォーマットエラー
                        G_HttpFile[FILE_A].sndflg = SND_ON;          // [FILE]操作リクエスト結果：送信有りをセット
                        return(bRet);                                // 処理終了
                }
                //コマンドスレッド実行中は待つ 2020.Jul.15
                while((TX_SUSPENDED != cmd_thread.tx_thread_state) /*&& (TX_EVENT_FLAG != cmd_thread.tx_thread_state)*/){
                    tx_thread_sleep (1);
                }
                cmd_msg.CmdRoute = CMD_HTTP;                //コマンド キュー  コマンド実行要求元
//                cmd_msg.pT2Command = (char *)&HTTP.rxbuf.header;    //コマンド処理する受信データフレームの先頭ポインタ
//                cmd_msg.pT2Status = StsArea;
                cmd_msg.pStatusSize = (int32_t *)&txLen;              //コマンド処理された応答データフレームサイズ

                cmd_msg.pT2Command = &HTTP.rxbuf.command;    //コマンド処理する受信データフレームの先頭ポインタ
//                cmd_msg.pT2Status = &HTTP.rxbuf.header;    //コマンド処理された応答データフレームの先頭ポインタ（2020.Jun.15現在未対応）
                cmd_msg.pT2Status = &S_ucCmdAns[0];          //コマンド処理された応答データフレームの先頭ポインタ               // sakaguchi cg 2020.08.27


                tx_queue_send( &cmd_queue, &cmd_msg, TX_WAIT_FOREVER );
                tx_event_flags_get( &g_command_event_flags, FLG_CMD_END, TX_OR_CLEAR, &actual_events, TX_NO_WAIT ); // sakaguchi add 2020.08.28
                tx_event_flags_set( &g_command_event_flags, FLG_CMD_EXEC, TX_OR );
                tx_thread_resume( &cmd_thread );
//                tx_event_flags_get( &g_command_event_flags, FLG_CMD_END, TX_OR_CLEAR, &actual_events, TX_WAIT_FOREVER );  // sakaguchi del 2020.08.27
                bRet = true;
            }
            break;

        //----------------------------------
        //---   機器設定送信時            ---
        //----------------------------------
        //----------------------------------
        //---   警報送信時              ---
        //----------------------------------
        //----------------------------------
        //---   記録データ送信時      ---
        //----------------------------------
        //----------------------------------
        //---   現在値データ送信時     ---
        //----------------------------------
        //----------------------------------
        //---   ログ送信時              ---
        //----------------------------------
        //----------------------------------
        //---   ログ送信テスト時         ---
        //----------------------------------
        case FILE_C:
        case FILE_W:
        case FILE_S:
        case FILE_M:
        case FILE_L:
        case FILE_L_T:
            if( 1 == S_ucNextCmd ){                             // NextCommand：次要求有り？
                if(true != bhttp_request_docheck()){            // WSSからのリクエスト実行中以外    // sakaguchi 2020.09.02
                G_HttpFile[FILE_R].sndflg = SND_ON;                                 // [FILE]操作リクエスト：送信有りをセット
                G_HttpFile[FILE_R].reqid = S_ulReqID;                               // [FILE]操作リクエスト：リクエストIDを格納
                tx_event_flags_set (&event_flags_http, FLG_HTTP_READY, TX_OR);      // HTTPスレッド動作許可ON
                }
            }else if( 2 == S_ucNextCmd ){                       // NextCommand：全設定取得？
//                G_HttpCmd[FILE_C].reqid = S_ulReqID;                                // [FILE]機器設定：リクエストID格納
                G_HttpFile[FILE_C].reqid = S_ulReqID;                               // [FILE]リクエストID格納              // sakaguchi 2020.09.02
                G_HttpFile[FILE_C].sndflg = SND_ON;                                 // [FILE]機器設定：送信有りをセット     // sakaguchi 2020.09.02
                G_HttpCmd[HTTP_CMD_RSETF].sndflg = SND_ON;                          // 登録情報送信     //sakaguchi UT-0029
                vhttp_sysset_sndon();                                               // 親機設定送信
                tx_event_flags_set (&event_flags_http, FLG_HTTP_READY, TX_OR);      // HTTPスレッド動作許可ON
            }
            bRet = true;
            break;

        //----------------------------------
        //---   操作リクエスト結果送信時   ---
        //----------------------------------
        case FILE_A:

            if( 1 == S_ucNextCmd ){                             // NextCommand：次要求有り？
                G_HttpFile[FILE_R].sndflg = SND_ON;         // [FILE]操作リクエスト：送信有りをセット
                G_HttpFile[FILE_R].reqid = S_ulReqID;                               // [FILE]操作リクエスト：リクエストIDを格納
                // 操作リクエストを連続で送信させたいため、HTTPスレッド動作許可フラグをONする
                tx_event_flags_set (&event_flags_http, FLG_HTTP_READY, TX_OR);      // HTTPスレッド動作許可ON
            }else if( 2 == S_ucNextCmd ){                       // NextCommand：全設定取得？
//                G_HttpCmd[FILE_C].reqid = S_ulReqID;                                // [FILE]機器設定：リクエストID格納
                G_HttpFile[FILE_C].reqid = S_ulReqID;                               // [FILE]リクエストID格納              // sakaguchi 2020.09.02
                G_HttpFile[FILE_C].sndflg = SND_ON;                                 // [FILE]機器設定：送信有りをセット     // sakaguchi 2020.09.02
                G_HttpCmd[HTTP_CMD_RSETF].sndflg = SND_ON;                          // 登録情報送信     //sakaguchi UT-0029
                vhttp_sysset_sndon();                                               // 親機設定送信
                tx_event_flags_set (&event_flags_http, FLG_HTTP_READY, TX_OR);      // HTTPスレッド動作許可ON
            }
            bRet = true;
            break;

        //------------------
        //---   その他   ---
        //------------------
        default:
            break;
    }

    return(bRet);

}

//******************************************************************************
//* 概要   | T2コマンドIndex取得
//* 引数   | [out]*cbuf : 受信データ
//* 戻り値 | コマンドIndex
//* 詳細   | 入力した受信データから受信したコマンドindexを出力する
//******************************************************************************/
static int ihttp_T2cmd_index_get(char *cbuf)
{
    int iRet = HTTP_CMD_NONE;               // 戻り値：コマンドIndex
    int icmd;

    for( icmd=0; icmd<HTTP_CMD_MAX; icmd++ ){
        if( 0 == memcmp( HTTP_T2Cmd[icmd], cbuf, 5 ) ){         // 先頭5byte比較
            iRet = icmd;
            break;
        }
    }
    return(iRet);
}


//******************************************************************************
//* 概要   | T2コマンドパラメータ値（10進数）取得
//* 引数１ | [in]*cbuf : 受信データ
//* 引数２ | [in]*Key : パラメータ名
//* 戻り値 | パラメータ値（10進数）
//* 詳細   | 受信データから指定したパラメータ名の値（10進数）を出力する
//******************************************************************************/
static int32_t lhttp_T2cmd_ParamInt_get(char *cbuf, char *Key)
{
    int32_t lRet = -1;
    uint16_t i,pt;
    uint16_t total,len;

    total = cbuf[2];
    total = (uint16_t)(total+cbuf[3]*256);      // 受信コマンドの長さ

    pt = 4;
    for(i=0; i<total; i++){
        if( 0 == memcmp(cbuf+pt+i, Key, strlen(Key))){      // パラメータ名検索
            pt = (uint16_t)(pt+i+(int)strlen(Key));
            len = cbuf[pt++];
            len = (uint16_t)(len+cbuf[pt++]*256);
            lRet = AtoL(&cbuf[pt],len);
            break;
        }
    }
    return(lRet);
}

//******************************************************************************
//* 概要   | T2コマンドパラメータ値（文字列）取得
//* 引数１ | [in]*cbuf : 受信データ
//* 引数２ | [in]*Key : パラメータ名
//* 引数３ | [put]*Adrs : パラメータ値（文字列）
//* 戻り値 | パラメータ値（文字列）のサイズ
//* 詳細   | 受信データから指定したパラメータ名の値（文字列）を出力する
//******************************************************************************/
static int32_t lhttp_T2cmd_ParamAdrs_get(char *cbuf, char *Key, char *Adrs)
{
    int32_t lRet = -1;
    uint16_t i,pt;
    uint16_t total,len;

    total = cbuf[2];
    total = (uint16_t)(total+cbuf[3]*256);      // 受信コマンドの長さ

    pt = 4;
    for(i=0; i<total; i++){
        if( 0 == memcmp(cbuf+pt+i, Key, strlen(Key))){      // パラメータ名検索
            pt = (uint16_t)(pt+i+(int)strlen(Key));
            len = cbuf[pt++];
            len = (uint16_t)(len+cbuf[pt++]*256);
            // ファームアップデート時のパスワード長のチェック
            if(0 == memcmp(Key, "PW=", strlen("PW="))){
                if((len < 6) || (64 < len)){
                    pt = 4;                                 // サイズエラーの場合はポインタを元に戻す
                    continue;
                }
            }
            memcpy(Adrs, cbuf+pt, len);
            lRet = len;
            break;
        }
    }
    return(lRet);
}


//******************************************************************************
//* 概要   | T2コマンド解析
//* 引数   | [out]*cbuf : 受信データ
//* 戻り値 | コマンドIndex
//* 詳細   | 入力した受信データから受信したコマンドindexを出力する
//******************************************************************************/
static uint32_t ulhttp_T2cmd_analyze(char *cbuf)
{
    uint32_t ulRet = ERR(CMD, NOERROR);       // 戻り値：ステータス
    int icmd;
    int32_t loffset,lsize,ltotal;
    char   cPass[65];
    memset(cPass,0x00,sizeof(cPass));


    // コマンドIndex取得
    icmd = ihttp_T2cmd_index_get(cbuf+4);          // コマンドIndex取得

    switch(icmd){

        case HTTP_CMD_WRTCE:    // ルート証明書変更
            if((0 == my_config.websrv.Mode[0]) || (1 == my_config.websrv.Mode[0])){     // HTTPの場合はエラー
                ulRet = ERR( CMD, FORMAT );
                break;
            }else{
                lsize = lhttp_T2cmd_ParamAdrs_get(cbuf, "PW=", cPass);                 // パスワード不一致でエラー
                if((6 <= lsize) && (lsize <= (int)sizeof(my_config.network.NetPass))){
                    if(0 != memcmp(cPass, my_config.network.NetPass, (size_t)lsize)){
                        ulRet = ERR( CMD, FORMAT );
                        break;
                    }
                }else{
                    ulRet = ERR( CMD, FORMAT );
                    break;
                }
            }

            loffset = lhttp_T2cmd_ParamInt_get(cbuf, "OFFSET=");            // オフセット取得
            if(INT32_MAX == loffset){
                ulRet = ERR(CMD,FORMAT);
                break;
            }

            ltotal = lhttp_T2cmd_ParamInt_get(cbuf, "SIZE=");                // 総バイト数取得
            if(INT32_MAX == ltotal ){
                ulRet = ERR(CMD,FORMAT);
                break;
            }

            lsize = lhttp_T2cmd_ParamAdrs_get(cbuf, "DATA=", &g_receive_buffer_back[0]);      // 受信バイト数取得
            if(INT32_MAX == lsize ){
                ulRet = ERR(CMD,FORMAT);
                break;
            }

            memcpy(g_receive_buffer_back,g_receive_buffer,RECEIVE_BUFFER_SIZE);     // 受信データバックアップ
            G_HttpCmd[icmd].offset = (uint16_t)loffset;                             // オフセット
            G_HttpCmd[icmd].size = (uint16_t)ltotal;                                // 総バイト数
            break;


        case HTTP_CMD_WSETF:    // 登録情報変更
            loffset = lhttp_T2cmd_ParamInt_get(cbuf, "OFFSET=");            // オフセット取得
            if(INT32_MAX == loffset){
                ulRet = ERR(CMD,FORMAT);
                break;
            }

            ltotal = lhttp_T2cmd_ParamInt_get(cbuf, "SIZE=");                // 総バイト数取得
            if(INT32_MAX == ltotal ){
                ulRet = ERR(CMD,FORMAT);
                break;
            }

            lsize = lhttp_T2cmd_ParamAdrs_get(cbuf, "DATA=", &g_receive_buffer_back[0]);      // 受信バイト数取得
            if(INT32_MAX == lsize ){
                ulRet = ERR(CMD,FORMAT);
                break;
            }

            memcpy(g_receive_buffer_back,g_receive_buffer,RECEIVE_BUFFER_SIZE);     // 受信データバックアップ
            G_HttpCmd[icmd].offset = (uint16_t)loffset;                             // オフセット
            G_HttpCmd[icmd].size = (uint16_t)ltotal;                                // 総バイト数
            break;


        case HTTP_CMD_RRTCE:                    // ルート証明書取得
            if((0 == my_config.websrv.Mode[0]) || (1 == my_config.websrv.Mode[0])){     // HTTPの場合はエラー
                ulRet = ERR(CMD,FORMAT);
            }else{
                lsize = lhttp_T2cmd_ParamAdrs_get(cbuf, "PW=", cPass);                 // パスワード不一致でエラー
                if((6 <= lsize) && (lsize <= (int)sizeof(my_config.network.NetPass))){
                    if(0 != memcmp(cPass, my_config.network.NetPass, (size_t)lsize)){
                        ulRet = ERR( CMD, FORMAT );
                    }
                }else{
                    ulRet = ERR( CMD, FORMAT );
                }
            }
            break;


        case HTTP_CMD_EFIRM:                    // ファームウェア更新
            lsize = lhttp_T2cmd_ParamAdrs_get(cbuf, "PW=", cPass);                 // パスワード不一致でエラー
            if((6 <= lsize) && (lsize <= (int)sizeof(my_config.network.NetPass))){
                if(0 != memcmp(cPass, my_config.network.NetPass, (size_t)lsize)){
                    ulRet = ERR( CMD, FORMAT );
                }else{
                    // ファームウェア更新中は自律動作を一時停止
                    WaitRequest = WAIT_REQUEST;             // 一時停止要求
                    mate_time_flag = 1;                     // 自律動作の一時停止フラグON
                    mate_time = 60;                         // 自律動作の一時停止時間
                }
            }else{
                ulRet = ERR( CMD, FORMAT );
            }
            break;


        case HTTP_CMD_NONE:             // 未定義のコマンド
            ulRet = ERR(CMD, FORMAT);
            break;


        default:
            ulRet = ERR(CMD, NOERROR);        // エラー無しで何もしない
            break;
    }

    return(ulRet);
}



//******************************************************************************
//* 概要   | 登録情報データ読込
//* 引数１ | [out]*cbuf：読み込んだ登録情報
//* 引数２ | [in] offset：オフセット
//* 引数３ | [in] size：読込サイズ
//* 戻り値 | 取得結果 成功：true / 失敗：false
//* 詳細   | 指定したオフセットから指定したサイズ分読み込む
//******************************************************************************/
static bool bhttp_regist_read(char *cbuf, uint32_t offset, uint32_t size)
{
    bool        bRet = false;               // 戻り値
    uint32_t    adr;                        // 読込開始アドレス

    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS){

        if(size > SFM_REGIST_SECT){
            size = SFM_REGIST_SECT;         //セクタサイズ以上であれば補正
        }

        adr = (SFM_REGIST_START + offset);
        if(adr > SFM_REGIST_END){
            return(bRet);                   // 範囲外のためエラー
        }

        serial_flash_multbyte_read(adr, size, cbuf); // 読み込み

        tx_mutex_put(&mutex_sfmem);

        bRet = true;
    }
    return(bRet);
}

//******************************************************************************
//* 概要   | グループ情報データ読込
//* 引数１ | [out]*cbuf：グループ情報
//* 戻り値 | 取得結果 成功：true / 失敗：false
//* 詳細   |
//******************************************************************************/
static bool bhttp_vgroup_read(char *cbuf)
{
    bool bRet = false;              //戻り値

    Printf("CMD:RVGRP\r\n");

    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS){

        serial_flash_multbyte_read(SFM_GROUP_START, (size_t)SFM_GROUP_SIZE, cbuf);      // 読み込み 4k

        tx_mutex_put(&mutex_sfmem);
        bRet = true;
    }
    return(bRet);
}


//sakaguchi  ↓
//*************************************************************************************
//* 概要   | HTTP通信用フラグリセット
//* 引数１ | [in]ifile：ファイル指定
//* 引数２ | [in]imode：状態（E_OK：正常終了時/E_NG：異常終了時）
//* 戻り値 | 無し
//* 詳細   | 指定された状態（正常終了時/異常終了時）によってHTTP通信用フラグを更新する
//************************************************************************************/
//static void vhttp_cmdflg_reset(int ifile, int imode)
void vhttp_cmdflg_reset(int ifile, int imode)                  // sakaguchi 2021.02.16
{
    int i;
    int iSndOnCnt = 0;

    //-------------------------
    //--- 状態（正常終了時） ---
    //-------------------------
    if(E_OK == imode){

        switch( ifile ){

            //-------------------------
            //--- 機器設定          ---
            //-------------------------
            case FILE_C:
                for( i=0; i<HTTP_CMD_MAX; i++ ){
                    if( SND_DO == G_HttpCmd[i].sndflg ){    // [CMD]機器設定送信中？
                        G_HttpCmd[i].sndflg = SND_OFF;          // [CMD]機器設定：送信無し
                        G_HttpCmd[i].reqid = 0x00;              // [CMD]リクエストID：クリア
                        G_HttpCmd[i].save = 0;                  // [CMD]一時格納用：クリア
                        G_HttpCmd[i].offset = 0;                // [CMD]オフセット：クリア
                        G_HttpCmd[i].size = 0;                  // [CMD]サイズ：クリア
                        G_HttpCmd[i].status = 0;                // [CMD]実行結果：クリア
                    }
                    if( SND_ON == G_HttpCmd[i].sndflg ){    // [CMD]機器設定送信有り？
                        iSndOnCnt++;                            // 送信有カウンタ更新
                    }
                }
                if( 0 == iSndOnCnt ){                       //送信が必要なコマンド無し？
                    G_HttpFile[ifile].sndflg = SND_OFF;         // [FILE]機器設定：送信無し
                    G_HttpFile[ifile].reqid = 0x00;             // [FILE]リクエストID：クリア
                }
                break;

            //-------------------------
            //--- 操作リクエスト     ---
            //-------------------------
            //-------------------------
            //--- 機器状態          ---
            //-------------------------
            case FILE_R:
            case FILE_I:
                G_HttpFile[ifile].sndflg = SND_OFF;         // [FILE]操作リクエスト：送信無し
                G_HttpFile[ifile].reqid = 0x00;             // [FILE]リクエストID：クリア
                break;

            //--------------------------
            //--- 操作リクエスト結果  ---
            //--------------------------
            case FILE_A:
                for( i=0; i<HTTP_CMD_MAX; i++ ){
                    //if( SND_DO == G_HttpCmd[i].ansflg ){    // [CMD]操作リクエスト結果送信中？        // sakaguchi 2020.09.04
                        G_HttpCmd[i].ansflg = SND_OFF;          // [CMD]操作リクエスト結果：送信無し
                        G_HttpCmd[i].reqid = 0x00;              // [CMD]リクエストID：クリア
                        G_HttpCmd[i].save = 0;                  // [CMD]一時格納用：クリア
                        G_HttpCmd[i].offset = 0;                // [CMD]オフセット：クリア
                        G_HttpCmd[i].size = 0;                  // [CMD]サイズ：クリア
                        G_HttpCmd[i].status = 0;                // [CMD]実行結果：クリア
                    }
                //if( 0 == iSndOnCnt ){                       //送信が必要なコマンド無し？
                    G_HttpFile[ifile].sndflg = SND_OFF;         // [FILE]操作リクエスト結果：送信無し
                    G_HttpFile[ifile].reqid = 0x00;             // [FILE]リクエストID：クリア
                //}
                break;

            //-------------------------
            //--- 警報              ---
            //-------------------------
            //-------------------------
            //--- 吸い上げ          ---
            //-------------------------
            //-------------------------
            //--- モニター          ---
            //-------------------------
            //-------------------------
            //--- ログ              ---
            //-------------------------
            //-------------------------
            //--- ログ送信テスト     ---
            //-------------------------
            case FILE_W:
            case FILE_S:
            case FILE_M:
                // 警報、モニタリング、記録データの送信要求を消す（同時にＯＮされることはない ）// sakaguchi 2021.03.09
                G_HttpFile[FILE_W].sndflg = SND_OFF;
                G_HttpFile[FILE_W].reqid = 0x00;
                G_HttpFile[FILE_S].sndflg = SND_OFF;
                G_HttpFile[FILE_S].reqid = 0x00;
                G_HttpFile[FILE_M].sndflg = SND_OFF;
                G_HttpFile[FILE_M].reqid = 0x00;
                break;
            case FILE_L:
            case FILE_L_T:
                G_HttpFile[ifile].sndflg = SND_OFF;         // [FILE]：送信無し
                G_HttpFile[ifile].reqid = 0x00;             // [FILE]リクエストID：クリア
                break;
            default:
                break;
        }
    }
    //-------------------------
    //--- 状態（異常終了時） ---
    //-------------------------
    else{

        switch( ifile ){

            //-------------------------
            //--- 機器設定          ---
            //-------------------------
            case FILE_C:
                for( i=0; i<HTTP_CMD_MAX; i++ ){
// sakaguchi 2021.06.17 ↓
                    //if( SND_DO == G_HttpCmd[i].sndflg ){    // [CMD]機器設定：送信中？
//                        G_HttpCmd[i].sndflg = SND_ON;           // [CMD]機器設定：送信有りをセット（再送）
                        // サーバ通信異常の場合は再送せず。設定値カウンタによる整合処理で再送するため問題なし。
                        G_HttpCmd[i].sndflg = SND_OFF;          // [CMD]機器設定：送信無し
                        G_HttpCmd[i].reqid = 0x00;              // [CMD]リクエストID：クリア
                        G_HttpCmd[i].save = 0;                  // [CMD]一時格納用：クリア
                        G_HttpCmd[i].offset = 0;                // [CMD]オフセット：クリア
                        G_HttpCmd[i].size = 0;                  // [CMD]サイズ：クリア
                        G_HttpCmd[i].status = 0;                // [CMD]実行結果：クリア
                    //}
                    //if( SND_ON == G_HttpCmd[i].sndflg ){    // [CMD]機器設定：送信有り？
                    //    iSndOnCnt++;                            // 送信有りカウント更新
                    //}
                }

                //if( 0 == iSndOnCnt ){                       // 送信が必要なコマンド無し？
                    G_HttpFile[ifile].sndflg = SND_OFF;         // [FILE]機器設定：送信無し
                    G_HttpFile[ifile].reqid = 0x00;             // [FILE]リクエストID：クリア
                //}
// sakaguchi 2021.06.17 ↑
                break;

// sakaguchi 2021.05.28 ↓
            //-------------------------
            //--- 操作リクエスト     ---
            //-------------------------
            case FILE_R:
                if(G_HttpCmd[HTTP_CMD_EFIRM].reqid != 0x00){    // アップデート途中の異常時は操作リクエストを再送する
                    Printf("EFIRM Recovery FILE_R ReqID[%d]\r\n, G_HttpCmd[HTTP_CMD_EFIRM].reqid");
                    G_HttpFile[ifile].sndflg = SND_ON;          // [FILE]操作リクエスト：送信有り
                    G_HttpFile[ifile].reqid = 0x00;             // [FILE]リクエストID：クリア
                }else{
                    G_HttpFile[ifile].sndflg = SND_OFF;         // [FILE]操作リクエスト：送信無し
                    G_HttpFile[ifile].reqid = 0x00;             // [FILE]リクエストID：クリア
                }
                break;
// sakaguchi 2021.05.28 ↑

            //-------------------------
            //--- 機器状態          ---
            //-------------------------
            case FILE_I:
                G_HttpFile[ifile].sndflg = SND_OFF;         // [FILE]操作リクエスト：送信無し
                G_HttpFile[ifile].reqid = 0x00;             // [FILE]リクエストID：クリア
                break;

// sakaguchi 2021.05.28 ↓
            //-------------------------
            //--- 操作リクエスト結果  ---
            //-------------------------
            case FILE_A:
                if(G_HttpCmd[HTTP_CMD_EFIRM].reqid != 0x00){    // アップデート途中の異常時は操作リクエストを再送する
                    Printf("EFIRM Recovery FILE_A ReqID[%d]\r\n, G_HttpCmd[HTTP_CMD_EFIRM].reqid");
                    G_HttpFile[ifile].sndflg = SND_ON;          // [FILE]操作リクエスト結果：送信有り
                    G_HttpFile[ifile].reqid = 0x00;             // [FILE]リクエストID：クリア
                }else{
                    for( i=0; i<HTTP_CMD_MAX; i++ ){
                        G_HttpCmd[i].ansflg = SND_OFF;          // [CMD]操作リクエスト結果：送信無し
                        G_HttpCmd[i].reqid = 0x00;              // [CMD]リクエストID：クリア
                        G_HttpCmd[i].save = 0;                  // [CMD]一時格納用：クリア
                        G_HttpCmd[i].offset = 0;                // [CMD]オフセット：クリア
                        G_HttpCmd[i].size = 0;                  // [CMD]サイズ：クリア
                        G_HttpCmd[i].status = 0;                // [CMD]実行結果：クリア
                    }
                    G_HttpFile[ifile].sndflg = SND_OFF;         // [FILE]操作リクエスト結果：送信無し
                    G_HttpFile[ifile].reqid = 0x00;             // [FILE]リクエストID：クリア
                }
                break;
// sakaguchi 2021.05.28 ↑

            //-------------------------
            //--- 警報              ---
            //-------------------------
            //-------------------------
            //--- 吸い上げ          ---
            //-------------------------
            //-------------------------
            //--- モニター          ---
            //-------------------------
            case FILE_W:
            case FILE_S:
            case FILE_M:
                // 警報、モニタリング、記録データの送信要求を消す（同時にＯＮされることはない ）// sakaguchi 2021.03.09
                G_HttpFile[FILE_W].sndflg = SND_OFF;
                G_HttpFile[FILE_W].reqid = 0x00;
                G_HttpFile[FILE_S].sndflg = SND_OFF;
                G_HttpFile[FILE_S].reqid = 0x00;
                G_HttpFile[FILE_M].sndflg = SND_OFF;
                G_HttpFile[FILE_M].reqid = 0x00;
                //G_HttpFile[ifile].sndflg = SND_OFF;         // [FILE]：送信無し
                //G_HttpFile[ifile].reqid = 0x00;             // [FILE]リクエストID：クリア
                break;

            //-------------------------
            //--- ログ              ---
            //-------------------------
            //-------------------------
            //--- ログ送信テスト     ---
            //-------------------------
            case FILE_L:                                    // 異常時は次回タイミングで再送
            case FILE_L_T:
                G_HttpFile[ifile].sndflg = SND_OFF;         // [FILE]ログ：送信無し
                G_HttpFile[ifile].reqid = 0x00;             // [FILE]リクエストID：クリア
                S_ucLogAct = 0;                             // 異常後は全てのログを送信する（リードポインタが進んでいるため）
                break;

            default:
                break;
        }
    }
}
//sakaguchi ↑

//sakaguchi ↓
//*************************************************************************************
//* 概要   | HTTP通信用フラグALLリセット
//* 引数１ | [in]imode：状態（E_OK：正常終了時/E_NG：異常終了時）
//* 戻り値 | 無し
//* 詳細   | 指定された状態（正常終了時/異常終了時）によって全てのHTTP通信用フラグを更新する
//************************************************************************************/
//static void vhttp_cmdflg_allreset(int imode)
void vhttp_cmdflg_allreset(int imode)               // sakaguchi 2021.02.16
{
    int i;

    for( i=0; i<HTTP_FILE_MAX; i++ ){
        vhttp_cmdflg_reset(i,imode);
    }
}
//sakaguchi ↑


//sakaguchi ↓
//******************************************************************************
//* 概要   | HTTP送信ファイルの有無チェック
//* 引数   | 無し
//* 戻り値 | HTTP送信ファイルの有無（有り：SND_ON / 無し：SND_OFF）
//* 詳細   | 操作リクエストによる無線通信のステータス確認も実施する
//******************************************************************************/
int ichttp_filesnd_check(void)
{
    int iRet = SND_OFF;                         // 戻り値
    int i;
    int icmd;
    LO_HI a;
    uint32_t    actual_events;                              // sakaguchi add 2020.08.28

//sakaguchi add UT-0030 ↓
    //------------------------------------------
    //---  操作リクエスト送信タイマリスタート   ---
    //------------------------------------------
    if( 0 == ReqOpe_time ){
        //----- timer set -----
        a.byte_lo = my_config.websrv.Interval[0];
        a.byte_hi = my_config.websrv.Interval[1];
        ReqOpe_time = a.word*60;                            //操作リクエスト送信タイマ（秒）
    }

    // sakaguchi add 2020.08.28
    // コマンドスレッドから結果応答でイベントフラグクリア
    if( SND_ON ==  G_HttpFile[FILE_A].sndflg ){
        tx_event_flags_get( &g_command_event_flags, FLG_CMD_END, TX_OR_CLEAR, &actual_events, TX_NO_WAIT );
    }

//sakaguchi del UT-0030 ↓
    //------------------------------------------
    //---  ログ送信タイマリスタート            ---
    //------------------------------------------
//    if( 0 == PostLog_time ){
//        //----- timer set -----
//        PostLog_time = LOG_INTERVAL;                        //ログ送信タイマセット(24時間)
//    }
//sakaguchi del UT-0030 ↑

    //------------------------------------------
    //---  USB接続チェック                    ---
    //------------------------------------------
    if((USB_CONNECT == isUSBState() ) && (COMMAND_LOCAL != CmdMode)){   // Test 送信実行の為
        // 警報、モニタリング、記録データの送信要求を消す           // sakaguchi 2021.03.09
        G_HttpFile[FILE_W].sndflg = SND_OFF;
        G_HttpFile[FILE_W].reqid = 0x00;
        G_HttpFile[FILE_S].sndflg = SND_OFF;
        G_HttpFile[FILE_S].reqid = 0x00;
        G_HttpFile[FILE_M].sndflg = SND_OFF;
        G_HttpFile[FILE_M].reqid = 0x00;
        return(iRet);
    }
//sakaguchi add UT-0030 ↑

    //---------------------------------
    //---  ACT通信のステータス確認  ---
    //---------------------------------
    for( icmd=0; icmd<HTTP_CMD_MAX; icmd++ ){
//sakaguchi cg UT-0010 ↓
        if( RCV_ACT0 == G_HttpCmd[icmd].ansflg ){    // ACT0 受信済み？

            if( ( ERR(CMD,NOERROR) == G_HttpCmd[icmd].status )      // [CMD]実行結果：コマンド処理 異常無し？
             || ( ERR(LAN,NOERROR) == G_HttpCmd[icmd].status )      // [CMD]実行結果：LAN通信 異常無し？
             || ( ERR(RF,NOERROR)  == G_HttpCmd[icmd].status ) ){   // [CMD]実行結果：無線通信 異常無し？

                G_HttpCmd[icmd].ansflg = SND_ACT1;                  // [CMD]ACT1送信指示
                ihttp_act_snd(icmd,1);                              // cmd_threadへACT1送信
                break;

            }else{                                              // [CMD]実行結果：異常

                G_HttpCmd[icmd].ansflg = SND_ON;                    // [CMD]操作リクエスト結果：送信
                G_HttpFile[FILE_A].sndflg = SND_ON;                 // [FILE]操作リクエスト結果：送信
                break;
            }
        }

        if( RCV_ACT1 == G_HttpCmd[icmd].ansflg ){    // ACT1 受信済み？

            if( ( ERR(CMD,RUNNING) == G_HttpCmd[icmd].status )      // [CMD]実行結果：コマンド処理中？
             || ( ERR(RF,BUSY)     == G_HttpCmd[icmd].status ) ){   // [CMD]実行結果：無線通信中？
                                                                    // [CMD]実行結果：LAN通信中(BUSY)は再送せず終了
                G_HttpCmd[icmd].ansflg = SND_ACT1;                  // [CMD]ACT1送信
                tx_thread_sleep(100);                               // wait 1s（ポーリング用遅延）
                ihttp_act_snd(icmd,1);                              // cmd_threadへACT1送信（再送）
                break;

            }else{                                                  //実行結果：コマンド処理中、無線通信中以外

                G_HttpCmd[icmd].ansflg = SND_ON;                    // [CMD]操作リクエスト結果：送信
                G_HttpFile[FILE_A].sndflg = SND_ON;                 // [FILE]操作リクエスト結果：送信
                break;
            }
        }
//sakaguchi cg UT-0010 ↑

        if( RCV_STEP1 == G_HttpCmd[icmd].ansflg ){    // STEP1 受信済み？

            if( ERR( CMD, NOERROR ) == G_HttpCmd[icmd].status ){    // [CMD]実行結果：正常

                // STEP2のコマンド作成
                memset(http_buffer, 0x00, sizeof(http_buffer));
                if(E_OK == ihttp_T2cmd_make_save(icmd, SND_STEP2, http_buffer))
                {
                    G_HttpCmd[icmd].ansflg = SND_STEP2;             // [CMD]操作リクエスト結果：step2送信をセット
                    ihttp_cmd_thread_snd(icmd, http_buffer);        // cmd_threadへ送信
                }
                break;

            }else{                                                  // [CMD]実行結果：異常
                G_HttpCmd[icmd].ansflg = SND_ON;                    // [CMD]操作リクエスト結果：送信
                G_HttpFile[FILE_A].sndflg = SND_ON;                 // [FILE]操作リクエスト結果：送信
                break;
            }
        }

        if( RCV_STEP2 == G_HttpCmd[icmd].ansflg ){    // STEP2 受信済み？

            if( ERR( CMD, NOERROR ) == G_HttpCmd[icmd].status ){    // [CMD]実行結果：正常

                // 全データ受信完了していない場合は、続きのデータをサーバに要求
                if(G_HttpCmd[icmd].offset < G_HttpCmd[icmd].size){
                    G_HttpCmd[icmd].ansflg = SND_ON;                    // [CMD]操作リクエスト結果：送信
                    G_HttpFile[FILE_A].sndflg = SND_ON;                 // [FILE]操作リクエスト結果：送信
                    break;                                              // 処理終了
                }

                // STEP2のコマンド作成
                memset(http_buffer, 0x00, sizeof(http_buffer));
                if(E_OK == ihttp_T2cmd_make_save(icmd, SND_STEP3, http_buffer))
                {
                    G_HttpCmd[icmd].ansflg = SND_STEP3;             // [CMD]操作リクエスト結果：step3送信をセット
                    ihttp_cmd_thread_snd(icmd, http_buffer);        // cmd_threadへ送信
                }
                break;

            }else{                                                  // [CMD]実行結果：異常
                G_HttpCmd[icmd].ansflg = SND_ON;                    // [CMD]操作リクエスト結果：送信
                G_HttpFile[FILE_A].sndflg = SND_ON;                 // [FILE]操作リクエスト結果：送信
                break;
            }
        }

        if( RCV_STEP3 == G_HttpCmd[icmd].ansflg ){    // STEP3 受信済み？
            G_HttpCmd[icmd].ansflg = SND_ON;                    // [CMD]操作リクエスト結果：送信
            G_HttpFile[FILE_A].sndflg = SND_ON;                 // [FILE]操作リクエスト結果：送信
            break;
        }
    }

    //---------------------------------
    //---  送信ファイルの有無確認    ---
    //---------------------------------
    for( i=0; i<HTTP_FILE_MAX; i++ ){

        if( SND_ON == G_HttpFile[i].sndflg ){      // [FILE]送信有り？
            iRet = SND_ON;
            break;
        }
    }

// sakaguchi 2021.08.02 ↓
    // ファームアップデート中はWSSに対しリクエストを送信し続ける
    if((SND_OFF == G_HttpFile[FILE_R].sndflg) && (true == Update_check())){
        G_HttpFile[FILE_R].sndflg = SND_ON;
        iRet = SND_ON;
    }
// sakaguchi 2021.08.02 ↑

    return(iRet);
}

static int iChgChartoBinary(char *value, int isize)
{
    int ibin=0;
    int i;

    for( i=isize-1; i>=0; i-- ){
        ibin = (ibin << 8 ) | value[i];
    }

    return(ibin);
}
//sakaguchi ↑

//sakaguchi add UT-0010 ↓
//******************************************************************************
//* 概要   | パラメータ値取得
//* 引数１ | [in]*data：データ
//* 引数２ | [in]size ：データ長
//* 引数３ | [in]*para：検索するパラメータ名
//* 戻り値 | パラメータ値（int型）
//* 詳細   | 入力されたデータから指定したパラメータ名の値を出力する
//* 詳細   | 引数１のデータはT2コマンドのフォーマットとする
//******************************************************************************/
static int ihttp_param_get(char *data, uint16_t size, char *para)
{
    int i;
    LO_HI psize;                        // パラメータサイズ
    uint8_t uclen;                        // パラメータ長
    char  cWork[10];

    memset(cWork,0x00,sizeof(cWork));
    psize.word = 0;                                   // sakaguchi 2020.09.02

    uclen = (uint8_t)strlen(para);                    // パラメータ長格納

    for( i=0; i<size; i++){
        if( 0 == memcmp( data+i, para, uclen ) ){   // パラメータ名検索
            psize.byte_lo = data[i+uclen+1];        // パラメータサイズ取得
            psize.byte_hi = data[i+uclen+2];
            break;
        }
    }
    memcpy(cWork, data+i+uclen+3, psize.word);      // パラメータ値格納

    return(atoi(cWork));
}
//sakaguchi add UT-0010 ↑

//sakaguchi add UT-0026 ↓
//******************************************************************************
//* 概要   | 親機設定送信指示
//* 引数  | 無し
//* 戻り値 | 無し
//* 詳細   | HTTP通信の送信対象となる親機設定の送信用フラグを全てONする
//******************************************************************************/
void vhttp_sysset_sndon(void)
{
    if(VENDER_HIT != fact_config.Vender){                   // sakaguchi 2020.12.24
    //if((VENDER_HIT != fact_config.Vender) && (Http_Use == HTTP_SEND)){ 
    //if( 0x09 != fact_config.Vender){                    // Hitach以外
    G_HttpFile[FILE_C].sndflg = SND_ON;                     // [FILE]機器設定：送信有りをセット
    G_HttpCmd[HTTP_CMD_RUINF].sndflg = SND_ON;              // [CMD]機器設定(RUINF)：送信有りをセット
    G_HttpCmd[HTTP_CMD_RBLEP].sndflg = SND_ON;              // [CMD]機器設定(RBLEP)：送信有りをセット
    G_HttpCmd[HTTP_CMD_RDTIM].sndflg = SND_ON;              // [CMD]機器設定(RDTIM)：送信有りをセット
    G_HttpCmd[HTTP_CMD_RWARP].sndflg = SND_ON;              // [CMD]機器設定(RWARP)：送信有りをセット
    G_HttpCmd[HTTP_CMD_RCURP].sndflg = SND_ON;              // [CMD]機器設定(RCURP)：送信有りをセット
    G_HttpCmd[HTTP_CMD_RSUCP].sndflg = SND_ON;              // [CMD]機器設定(RSUCP)：送信有りをセット
    G_HttpCmd[HTTP_CMD_RVGRP].sndflg = SND_ON;              // [CMD]機器設定(RVGRP)：送信有りをセット
    G_HttpCmd[HTTP_CMD_RNETP].sndflg = SND_ON;              // [CMD]機器設定(RNETP)：送信有りをセット sakaguchi add UT-0033
    }
}
//sakaguchi add UT-0026 ↑

// sakaguchi 2020.09.02 ↓
//******************************************************************************
//* 概要   | WSSからのリクエスト実行中判定
//* 引数   | 無し
//* 戻り値 | true:リクエスト実行中 false：リクエスト無し
//* 詳細   | 実行中のリクエストがあればtrueを返す
//******************************************************************************/
bool bhttp_request_docheck(void)
{
    static int ilog = 0;                    // sakaguchi 2020.09.04
    int i;
    for( i=1; i<HTTP_CMD_MAX; i++ ){        // 0(cmd_noneは除外)
        if(0 != G_HttpCmd[i].reqid){        // リクエストIDが格納させている
            if(ilog == 0){
                Printf("  . HTTP Post stop[%d]\n", i);
                PutLog(LOG_LAN, "HTTP Post stop(%d)", i);
            }
            ilog++;
            return(true);                   // リクエスト実行中
        }
    }
    if(ilog > 0){
        Printf("  . HTTP Post start\n");
        PutLog(LOG_LAN, "HTTP Post start");
    }
    ilog = 0;
    return(false);
}
// sakaguchi 2020.09.02 ↑


/**
 * ログ送信指示
 */
void SendLogData(void)
{
    PutLog(LOG_LAN, "ETLAN http(s) send test");

    if(USB_CONNECT == isUSBState()){    // USB接続時でもHTTP送信テストを行う
        CmdMode = COMMAND_LOCAL;
        G_HttpFile[FILE_R].sndflg = SND_OFF;        // リクエストは停止させる
    }
    G_HttpFile[FILE_L].sndflg = SND_OFF;            // 通常のログ送信は停止させる
    G_HttpFile[FILE_L_T].sndflg = SND_ON;           // ログ送信テスト
    tx_event_flags_set(&event_flags_http, FLG_HTTP_READY, TX_OR);
    tx_thread_sleep(5);
    CmdMode = COMMAND_NON;
}


// error 11A 11Bは、nx_secure_tls.h をみる



// 0x3000C      NX_WEB_HTTP_GET_DONE
// 0x3002C      NX_WEB_HTTP_STATUS_CODE_FORBIDDEN           403 Forbidden
// 0x3003B      NX_WEB_HTTP_STATUS_CODE_INTERNAL_ERROR      HTTP status code 500 Internal   Server Error
// 0x3000D      NX_WEB_HTTP_BAD_PACKET_LENGTH               400 bad request
// 0x3000F      NX_WEB_HTTP_INCOMPLETE_PUT_ERROR            Server responds before PUT is complete
// 0x30029      NX_WEB_HTTP_STATUS_CODE_BAD_REQUEST      
// 0x3003E      NX_WEB_HTTP_STATUS_CODE_SERVICE_UNAVAILABLE  HTTP status code 503 Service Unavailable  
// 0x3002D      NX_WEB_HTTP_STATUS_CODE_NOT_FOUND           HTTP status code 404 Not Found
// 0x01         NX_NO_PACKET
// 0x07         NX_PTR_ERROR 
// 0x38         NX_NOT_CONNECTED
// 0x39         NX_WINDOW_OVERFLOW
// 0x49         NX_TX_QUEUE_DEPTH
// 0x22         NX_ALREADY_BOUND
// 0x24         NX_NOT_BOUND 
// 0x114        NX_SECURE_TLS_ALERT_RECEIVED                The remote host sent an alert indicating an error and ending the TLS session.  sever 無し？
// 0x104        NX_SECURE_TLS_INVALID_PACKET

/**
 *
 * @param client_ptr
 * @return
 */
UINT get_web_http_client_response_body(NX_WEB_HTTP_CLIENT* client_ptr )
{
    ULONG       https_client_wait_option = 1500;
    NX_PACKET  *receive_packet;
    ULONG       bytes_copied;
    ULONG       windex = 0;                                                //sakaguchi UT-0036
    UINT        status,status2;

    // 2021.01.20
    ULONG total_packets;
    ULONG free_packets;
    ULONG empty_pool_requests;
    ULONG empty_pool_suspensions;
    ULONG invalid_packet_releases;

#ifdef DBG_TERM
    Printf("get_web_http_client_response_body() %ld 0x%x  \r\n", body_get_count, body_status);
    Printf("  . nx_web_http_client_response_body_get\r\n");
    nx_packet_pool_info_get(&g_packet_pool1,&total_packets, &free_packets, &empty_pool_requests, &empty_pool_suspensions, &invalid_packet_releases);
    Printf("nx_packet_pool_info_get  %ld %ld %ld %ld %ld \r\n", total_packets, free_packets, empty_pool_requests, empty_pool_suspensions, invalid_packet_releases);
#endif

    memset(g_receive_buffer, 0x00, sizeof(g_receive_buffer));

    while (1)
    {
        // Get a response packet.
        //status = nx_web_http_client_response_body_get (client_ptr, &receive_packet, https_client_wait_option);
        status = nx_web_http_client_response_body_get (client_ptr, &receive_packet, https_client_wait_option*2);        // 2020.12.24
        Printf("    nx_web_http_client_response_body_get status New = 0x%x\r\n", status);

        // All response packets have already been read, stop receiving.
        //             0x24                        0x30029 (400 bad request)                         0x3003B   (500 internal server error) 
        if ((status == NX_NOT_BOUND) || (status == NX_WEB_HTTP_STATUS_CODE_BAD_REQUEST) || (status == NX_WEB_HTTP_STATUS_CODE_INTERNAL_ERROR))
        {
            nx_packet_release (receive_packet);
            Printf(" get_web_http_client_response_body 1\r\n");
            break;
        }

        if ((status == NX_WEB_HTTP_STATUS_CODE_FORBIDDEN))
        {
            nx_packet_release (receive_packet);
            Printf(" get_web_http_client_response_body 1\r\n");
            break;
        }
#ifdef DBG_TERM
        if(status == NX_SUCCESS)
            Printf("    NX_SUCCESS\r\n"); 
        else if(status == NX_WEB_HTTP_GET_DONE)
            Printf("    NX_WEB_HTTP_GET_DONE\r\n");
        else if(status == NX_NOT_CONNECTED)
            Printf("    NX_NOT_CONNECTED\r\n");
        else if(status == NX_NO_PACKET)
            Printf("    NX_NO_PACKET\r\n");
        else if(status == NX_WEB_HTTP_STATUS_CODE_SERVICE_UNAVAILABLE)
            Printf("    NX_WEB_HTTP_STATUS_CODE_SERVICE_UNAVAILABLE\r\n");
        else if(status == NX_WEB_HTTP_STATUS_CODE_NOT_FOUND)
            Printf("    NX_WEB_HTTP_STATUS_CODE_NOT_FOUND\r\n");
        else
            Printf("    OTHER Status 0x%x\r\n", status); 
        
#endif 
        // Check to see if we have a packet.
        if ((status == NX_SUCCESS) || (status == NX_WEB_HTTP_GET_DONE) )
        {

            //bytes_copied = 0;
            // Retrieve the data from the packet.
//            status2 = nx_packet_data_retrieve (receive_packet, g_receive_buffer, &bytes_copied);
            status2 = nx_packet_data_retrieve (receive_packet, &g_receive_buffer[windex], &bytes_copied);   //sakaguchi UT-0036
            if (status2 != NX_SUCCESS)
            {
                Printf ("    nx_packet_data_retrieve Failed. status = 0x%x\r\n", status2);
            }

            Printf("    bytes_copied %ld 0x%x\r\n", bytes_copied, status2);
            // Release the packet.
            status2 = nx_packet_release (receive_packet);
            if (status2 != NX_SUCCESS)
            {
                Printf ("    nx_packet_release Failed. status = 0x%x\r\n", status2);
            }

            
            // Print a part of the response.
            if( bytes_copied > 0 ) {
                windex += bytes_copied;                                                         //sakaguchi UT-0036
                //g_receive_buffer[bytes_copied] = 0; // null terminate for Printf              //sakaguchi UT-0036
                Printf ("packet [len]: %d\r\n", (int) bytes_copied);
                //Printf ("packet [body]:\r\n%s\r\n", g_receive_buffer);
            }
        }

        if (status == NX_WEB_HTTP_GET_DONE) 
        {
            Printf("    NX_WEB_HTTP_GET_DONE 2\r\n");
            break;
        }
        if (status != NX_SUCCESS)
        {
            // 2021.01.20 status2 = nx_packet_release (receive_packet);       // add 2020 03 27
            Printf ("    nx_packet_data_retrieve Failed. status = 0x%x   0x%x  Exit no NX SUCCESS \r\n", status,status2);

            break;
        }
    }

#if 0
    nx_packet_release (receive_packet);
    Printf("    nx_web_http_client_response_body_get status Exit = 0x%x\r\n", status);
    if(status == NX_WEB_HTTP_GET_DONE)      // HTTP client get is complete 
        status = NX_SUCCESS;
#endif 
 


    body_status = status;
#ifdef DBG_TERM
    nx_packet_pool_info_get(&g_packet_pool1,&total_packets, &free_packets, &empty_pool_requests, &empty_pool_suspensions, &invalid_packet_releases);
    Printf("nx_packet_pool_info_get x3 %ld %ld %ld %ld %ld \r\n", total_packets, free_packets, empty_pool_requests, empty_pool_suspensions, invalid_packet_releases);
    // 2021.01.21 
    //status2 = nx_packet_release (receive_packet); //invalid_packet_releasesが増えるのは、ここが原因の一つ
    Printf("    nx_web_http_client_response_body_get status Exit = 0x%x 0x%x \r\n", status,status2);
#endif
    // packet poolが枯渇する対策　　2021.02.11
    switch(status){
        case NX_WEB_HTTP_GET_DONE:  // HTTP client get is complete
            status = NX_SUCCESS;
            break;
        case NX_NOT_CONNECTED:
        case NX_NO_PACKET:
        case NX_WEB_HTTP_STATUS_CODE_SERVICE_UNAVAILABLE:
        case NX_WEB_HTTP_STATUS_CODE_NOT_FOUND:
            break;
        default:
            nx_packet_release (receive_packet);
            break;
    }
/*
    if(status == NX_WEB_HTTP_GET_DONE)      // HTTP client get is complete 
        status = NX_SUCCESS;
    else if(status == NX_NOT_CONNECTED){    // 2021.02.08
        ;
    }
    else if(status == NX_NO_PACKET){    // 2021.02.09
        ;
    }
    else if(status == NX_WEB_HTTP_STATUS_CODE_SERVICE_UNAVAILABLE){    // 2021.02.09
        ;
    }
    else{
        nx_packet_release (receive_packet);
    }
  */  
    nx_packet_pool_info_get(&g_packet_pool1,&total_packets, &free_packets, &empty_pool_requests, &empty_pool_suspensions, &invalid_packet_releases);
    Printf("nx_packet_pool_info_get x4 %ld %ld %ld %ld %ld \r\n", total_packets, free_packets, empty_pool_requests, empty_pool_suspensions, invalid_packet_releases);
    
    Printf("get_web_http_client_response_body() Exit 0x%x  %ld\r\n", status, body_get_count);
    body_get_count++;
    
    return(status);
}





/*
 *
NX 1.  Create a TCP socket by calling the nx_tcp_socket_create() API.                                      //
NX 2.  Set up a buffer for TLS packet assembly using the nx_secure_tls_session_packet_buffer_set() API.    //
NX 3.  Bind to a local port by calling the nx_tcp_client_socket_bind() API.                                //
NX 4.  Allocate space for processing received server certificate data by calling the
       nx_secure_tls_remote_certificate_allocate() API, for the remote server certificate and for the      //
       remote server certificate issuer certificate.
NX 5.  Initialize the local identity certificate by calling the nx_secure_x509_certificate_initialize() API.    //
       Do the same for the trusted CA (Certificate Authority) certificate.
NX 6.  Add the local and trusted certificates by calling the nx_secure_tls_local_certificate_add() API and      //
       nx_secure_tls_trusted_certificate_add() API respectively                                                 //

                                               nxd_tcp_client_socket_connect
NX 7.  Connect to the TLS server by calling the nx_tcp_client_socket_connect API.                           //???
NX 8.  Start the TLS session by calling the nx_secure_tls_session_start API.                                //

NX 9.  Allocate a packet for sending data. Because this packet must have the TLS header data, use the
    nx_secure_tls_packet_allocate API.                                                                      //
NX 10. Add data to the packet by calling the nx_packet_data_append API.                                     //
NX 11. Send the packet by calling the nx_secure_tls_session_send API.                                       //

NX 12. Receive the TLS server response by calling the nx_secure_tls_session_receive API.                    //

NX 13. End the TLS session by calling the nx_secure_tls_session_end API.                                    //
NX 14. The TLS client socket is still connected to the Server socket at this point, so terminate
       the TCP socket connection by calling the nx_tcp_socket_disconnect API.                               //
NX 15. Release the local port by calling the nx_tcp_client_socket_unbind API.                               //
NX 16. Delete the TCP socket by calling the nx_tcp_socket_delete API.                                       //
NX 17. Delete the TLS session by calling the nx_secure_tls_session_delete API. See comments in 7.1.1. about //
       ending a TLS session below.
*/

VOID _nx_web_http_client_error_exit(NX_WEB_HTTP_CLIENT *, UINT);

// sakaguchi 2020.09.02 ↓
UINT my_tls_session_end(void)
{
    whc_tls_t* p_tls;
    UINT status1;

    p_tls = &g_whc_tls;

    status1 = nx_secure_tls_session_end(&(p_tls->p_secure_client->nx_web_http_client_tls_session), 1000 /*NX_WAIT_FOREVER*/);      // 2023.03.01
    Printf("      nx_secure_tls_session_end  %04X \r\n", status1);

    return (SSP_SUCCESS);
}
// sakaguchi 2020.09.02 ↑

// sakaguchi 2020.09.03 ↓
UINT my_tls_session_delete(void)
{
    whc_tls_t* p_tls;
    UINT status1;

    p_tls = &g_whc_tls;

    status1 = nx_secure_tls_session_delete(&(p_tls->p_secure_client->nx_web_http_client_tls_session));
    Printf("      nx_secure_tls_session_delete  %04X \r\n", status1);

    return (SSP_SUCCESS);
}
// sakaguchi 2020.09.03 ↑


//UINT my_tls_close();
UINT my_tls_close(NX_WEB_HTTP_CLIENT* client_ptr, int which)
{

    SSP_PARAMETER_NOT_USED(client_ptr);
    whc_tls_t* p_tls;

    p_tls = &g_whc_tls;
//    NX_PACKET  *p_packet;

//    UINT status;
    UINT status1,status2;

//    status = NX_SUCCESS;

    //whc_tls_t  *p_tls = &g_whc_tls;
 //   srv_info_t  *p_info = &g_whc_info;


    

    if(which == CONNECT_HTTPS){

#if 0
        status = nx_tcp_socket_receive(&(p_tls->p_secure_client->nx_web_http_client_socket), &p_packet, 10);
        Printf(" tls close %02X \r\n", status);
    
        status1 = nx_secure_tls_session_end(&(p_tls->p_secure_client->nx_web_http_client_tls_session), NX_WAIT_FOREVER);

        // Free up certificate buffers for next connection 
        status2 = nx_secure_tls_remote_certificate_free_all(&(p_tls->p_secure_client->nx_web_http_client_tls_session));

        status3 = nx_secure_tls_session_delete(&(p_tls->p_secure_client->nx_web_http_client_tls_session));  //sakaguchi free_allと処理順入替

        Printf("      nx_secure_tls_remote_certificate_free_all  %04X / %04X / %04X  \r\n", status1,status2,status3);
        if(status2) {

            return (SSP_ERR_ABORTED);
        }
#endif
        status1 = nx_secure_tls_session_end(&(p_tls->p_secure_client->nx_web_http_client_tls_session), 1000 /*NX_WAIT_FOREVER*/);    // 2023.03.01

        // Free up certificate buffers for next connection 
        status2 = nx_secure_tls_remote_certificate_free_all(&(p_tls->p_secure_client->nx_web_http_client_tls_session));
        Printf("      nx_secure_tls_remote_certificate_free_all  %04X / %04X  \r\n", status1,status2);
        if(status2) {

            return (SSP_ERR_ABORTED);
        }

    }
    return (SSP_SUCCESS);
}

// sakaguchi 2020.09.09 ↓
UINT my_client_error_exit(void)
{
    srv_info_t  *p_info = &g_whc_info;

    _nx_web_http_client_error_exit(p_info->p_secure_client, 500);
    Printf("  . _nx_web_http_client_error_exit\r\n");
    return (SSP_SUCCESS);
}
// sakaguchi 2020.09.09 ↑


//UINT my_tls_DOWN(void);
UINT my_tls_DOWN(void)
{
    UINT status;

    status = NX_SUCCESS;

    //whc_tls_t  *p_tls = &g_whc_tls;
    srv_info_t  *p_info = &g_whc_info;

    //NX 13. End the TLS session by calling the nx_secure_tls_session_end API.
    // End the TLS session. This is required to properly reset down the TLS connection
    // and required if the TLS session is to be used for another connection attempt.
    /*
    status = nx_secure_tls_session_end(&p_tls->p_secure_client->nx_web_http_client_tls_session, 500);
    if (status != NX_SUCCESS)
    {
        Printf("nx_secure_tls_session_end FAILED  status = %d, 0x%x\r\n", status, status);
    }
    */


   
    _nx_web_http_client_error_exit(p_info->p_secure_client, 500);
    Printf("  . _nx_web_http_client_error_exit  %ld \r\n", status);
    //if (status != NX_SUCCESS)
    //{
    //    Printf("_nx_web_http_client_error_exit FAILED  status = %d, 0x%x\r\n", status, status);
    //}
/*SSP 1.6.3 : this fails
    status = nx_secure_tls_session_delete(&p_tls->p_secure_client->nx_web_http_client_tls_session);
    if (status != NX_SUCCESS)
    {
        Printf("nx_secure_tls_session_delete FAILED  status = %d, 0x%x\r\n", status, status);
    }
SSP 1.6.3 : this fails */

/* DON'T CALL nx_web_http_client_delete because we would have to create it again
    Printf("  . nx_web_http_client_delete\r\n");
    status = nx_web_http_client_delete(p_info->p_secure_client);
    if (status != NX_SUCCESS)
    {
        Printf("nx_web_http_client_delete FAILED  status = %d, 0x%x\r\n", status, status);
    }
*/

    status = NX_SUCCESS;
    return( status);
}

//******************************************************************************
//* 概要   | HTTPステータスコード変換
//* 引数   | [in]status : SSPステータスコード
//* 戻り値 | HTTPステータスコード
//* 詳細   | SSPステータスコードをHTTPステータスコードに変換する
//******************************************************************************/
static uint32_t ulchg_http_status_code(uint32_t status)
{
    uint32_t ulhttp_code;

    ulhttp_code = status;

    switch(ulhttp_code){
        case NX_WEB_HTTP_STATUS_CODE_CONTINUE:              // 0x3001A
            ulhttp_code = 0x100;
            break;
        case NX_WEB_HTTP_STATUS_CODE_SWITCHING_PROTOCOLS:   // 0x3001B
            ulhttp_code = 0x101;
            break;
        case NX_WEB_HTTP_STATUS_CODE_CREATED:               // 0x3001C
            ulhttp_code = 0x201;
            break;
        case NX_WEB_HTTP_STATUS_CODE_ACCEPTED:              // 0x3001D
            ulhttp_code = 0x202;
            break;
        case NX_WEB_HTTP_STATUS_CODE_NON_AUTH_INFO:         // 0x3001E
            ulhttp_code = 0x203;
            break;
        case NX_WEB_HTTP_STATUS_CODE_NO_CONTENT:            // 0x3001F
            ulhttp_code = 0x204;
            break;
        case NX_WEB_HTTP_STATUS_CODE_RESET_CONTENT:         // 0x30020
            ulhttp_code = 0x205;
            break;
        case NX_WEB_HTTP_STATUS_CODE_PARTIAL_CONTENT:       // 0x30021
            ulhttp_code = 0x206;
            break;
        case NX_WEB_HTTP_STATUS_CODE_MULTIPLE_CHOICES:      // 0x30022
            ulhttp_code = 0x300;
            break;
        case NX_WEB_HTTP_STATUS_CODE_MOVED_PERMANETLY:      // 0x30023
            ulhttp_code = 0x301;
            break;
        case NX_WEB_HTTP_STATUS_CODE_FOUND:                 // 0x30024
            ulhttp_code = 0x302;
            break;
        case NX_WEB_HTTP_STATUS_CODE_SEE_OTHER:             // 0x30025
            ulhttp_code = 0x303;
            break;
        case NX_WEB_HTTP_STATUS_CODE_NOT_MODIFIED:          // 0x30026
            ulhttp_code = 0x304;
            break;
        case NX_WEB_HTTP_STATUS_CODE_USE_PROXY:             // 0x30027
            ulhttp_code = 0x305;
            break;
        case NX_WEB_HTTP_STATUS_CODE_TEMPORARY_REDIRECT:    // 0x30028
            ulhttp_code = 0x307;
            break;
        case NX_WEB_HTTP_STATUS_CODE_BAD_REQUEST:           // 0x30029
            ulhttp_code = 0x400;
            break;
        case NX_WEB_HTTP_STATUS_CODE_UNAUTHORIZED:          // 0x3002A
            ulhttp_code = 0x401;
            break;
        case NX_WEB_HTTP_STATUS_CODE_PAYMENT_REQUIRED:      // 0x3002B
            ulhttp_code = 0x402;
            break;
        case NX_WEB_HTTP_STATUS_CODE_FORBIDDEN:             // 0x3002C
            ulhttp_code = 0x403;
            break;
        case NX_WEB_HTTP_STATUS_CODE_NOT_FOUND:             // 0x3002D
            ulhttp_code = 0x404;
            break;
        case NX_WEB_HTTP_STATUS_CODE_METHOD_NOT_ALLOWED:    // 0x3002E
            ulhttp_code = 0x405;
            break;
        case NX_WEB_HTTP_STATUS_CODE_NOT_ACCEPTABLE:        // 0x3002F
            ulhttp_code = 0x406;
            break;
        case NX_WEB_HTTP_STATUS_CODE_PROXY_AUTH_REQUIRED:   // 0x30030
            ulhttp_code = 0x407;
            break;
        case NX_WEB_HTTP_STATUS_CODE_REQUEST_TIMEOUT:       // 0x30031
            ulhttp_code = 0x408;
            break;
        case NX_WEB_HTTP_STATUS_CODE_CONFLICT:              // 0x30032
            ulhttp_code = 0x409;
            break;
        case NX_WEB_HTTP_STATUS_CODE_GONE:                  // 0x30033
            ulhttp_code = 0x410;
            break;
        case NX_WEB_HTTP_STATUS_CODE_LENGTH_REQUIRED:       // 0x30034
            ulhttp_code = 0x411;
            break;
        case NX_WEB_HTTP_STATUS_CODE_PRECONDITION_FAILED:   // 0x30035
            ulhttp_code = 0x412;
            break;
        case NX_WEB_HTTP_STATUS_CODE_ENTITY_TOO_LARGE:      // 0x30036
            ulhttp_code = 0x413;
            break;
        case NX_WEB_HTTP_STATUS_CODE_URL_TOO_LARGE:         // 0x30037
            ulhttp_code = 0x414;
            break;
        case NX_WEB_HTTP_STATUS_CODE_UNSUPPORTED_MEDIA:     // 0x30038
            ulhttp_code = 0x415;
            break;
        case NX_WEB_HTTP_STATUS_CODE_RANGE_NOT_SATISFY:     // 0x30039
            ulhttp_code = 0x416;
            break;
        case NX_WEB_HTTP_STATUS_CODE_EXPECTATION_FAILED:    // 0x3003A
            ulhttp_code = 0x417;
            break;
        case NX_WEB_HTTP_STATUS_CODE_INTERNAL_ERROR:        // 0x3003B
            ulhttp_code = 0x500;
            break;
        case NX_WEB_HTTP_STATUS_CODE_NOT_IMPLEMENTED:       // 0x3003C
            ulhttp_code = 0x501;
            break;
        case NX_WEB_HTTP_STATUS_CODE_BAD_GATEWAY:           // 0x3003D
            ulhttp_code = 0x502;
            break;
        case NX_WEB_HTTP_STATUS_CODE_SERVICE_UNAVAILABLE:   // 0x3003E
            ulhttp_code = 0x503;
            break;
        case NX_WEB_HTTP_STATUS_CODE_GATEWAY_TIMEOUT:       // 0x3003F
            ulhttp_code = 0x504;
            break;
        case NX_WEB_HTTP_STATUS_CODE_VERSION_ERROR:         // 0x30040
            ulhttp_code = 0x505;
            break;
        default:
            break;
    }

    return(ulhttp_code);
}
