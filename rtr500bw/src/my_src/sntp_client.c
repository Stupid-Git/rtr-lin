/**
 * @file    sntp_client.c
 * @note    2020.Feb.05 hitaich v6 0204ソース反映済み
 */
#define _SNTP_CLIENT_C_


//#include "tlstest_thread.h"
//#include "nxd_sntp_client.h"
#include "nx_api.h"
#include "sntp_client.h"
#include "wifi_thread.h"
#include "MyDefine.h"
#include "DateTime.h"
#include "General.h"
#include "system_time.h"
#include "Globals.h"                    //sakaguchi UT-0035
#include "Log.h"
#include "Net_func.h"


//extern NX_SNTP_CLIENT *g_active_sntp;
//extern NX_DNS *g_active_dns;

srv_info_t  g_sntp_info;

static ULONG sntp_boot_time;


unsigned long get_time2(void);


/* Leap seconds handler as required by the NetX Duo SNTP client. We
 * don't do anything here.
 */
UINT leap_second_handler(NX_SNTP_CLIENT *client_ptr, UINT leap_indicator)
{
    SSP_PARAMETER_NOT_USED (client_ptr);
    SSP_PARAMETER_NOT_USED (leap_indicator);
    Printf("*** leap_second_handler\r\n");
    return (NX_SUCCESS);
}

/* Kiss of Death handler as required by NetX Duo SNTP client. We don't
 * do anything here.
 */
UINT kiss_of_death_handler(NX_SNTP_CLIENT *client_ptr, UINT KOD_code)
{
    
    SSP_PARAMETER_NOT_USED (client_ptr);
    SSP_PARAMETER_NOT_USED (KOD_code);
    return (NX_SUCCESS);
    
   /*
    UINT remove_server_from_list = NX_FALSE;
    UINT status = NX_SUCCESS;

    Printf("*** SNTP KOD %04X\r\n", KOD_code);
    // Handle kiss of death by code group. 
    switch (KOD_code)
    {
        case NX_SNTP_KOD_RATE:
        case NX_SNTP_KOD_NOT_INIT:
        case NX_SNTP_KOD_STEP:
        // Find another server while this one is temporarily out of service. 
            status = NX_SNTP_KOD_SERVER_NOT_AVAILABLE;
            break;

        case NX_SNTP_KOD_AUTH_FAIL:
        case NX_SNTP_KOD_NO_KEY:
        case NX_SNTP_KOD_CRYP_FAIL:
        
        // These indicate the server will not service client with time updates without successful authentication. 
            remove_server_from_list = NX_TRUE;
            break;
        default:
        // All other codes. Remove server before resuming time updates.
            remove_server_from_list = NX_TRUE;
            break;
    }

    // Removing the server from the active server list?
    if (remove_server_from_list)
    {
    // Let the caller know it has to bail on this server before resuming service.
        status = NX_SNTP_KOD_REMOVE_SERVER;
    }

    return status;
    */

}

// sakaguchi cg UT-0024 ntp Secondary対応 ↓
#if 0
/* Start SNTP client and update local clock */
int sntp_setup_and_run(void)
{
    UINT status;
    ULONG dest;


        /* Lookup server name using DNS */
        Printf("   sntp_setup_and_run (DNS)\r\n");
        //status = nx_dns_host_by_name_get(&g_dns0, (UCHAR *)SNTP_SERVER, &dest, 1000);
        status = nx_dns_host_by_name_get(&g_dns0, (UCHAR *)SNTP_SERVER, &dest, 100);
        if (status != NX_SUCCESS) {
            Printf("  Unable to resolve ");
            Printf("  " SNTP_SERVER);
            Printf("\r\n");
            Printf("  Failed to start SNTP service. status = 0x%x\r\n", status);
            // 0xA1
            // #define NX_DNS_NO_SERVER  0xA1  /* No DNS server was specified                          */

            return (-1);
        }

        Printf("   sntp_setup_and_run (DNS get)\r\n");
        /* Initialize Unicast SNTP client */
        status = nx_sntp_client_initialize_unicast(&g_sntp0, dest);
        if (status != NX_SUCCESS) {
            Printf("  Unable to initialize unicast SNTP client\r\n");
            Printf("  Failed to start SNTP service\r\n");
            
            return (-1);
        }

        /* Set local time to 0 */
        status = nx_sntp_client_set_local_time(&g_sntp0, 0, 0);
        if (status != NX_SUCCESS) {
            Printf("  Unable to set local time for SNTP client\r\n");
            Printf("  Failed to start SNTP service\r\n");
            return (-1);
        }

        /* Run Unicast client */
        status = nx_sntp_client_run_unicast(&g_sntp0);
        if (status != NX_SUCCESS) {
            Printf("  Unable to start unicast SNTP client\r\n");
            return (-1);
        }

 
    Printf("SNTP Start success\r\n");
    //TODO status = ????
    return (0);

}
#endif

int sntp_setup_and_run(void)
{
    UINT status;
    ULONG dest;
    uint32_t seconds;
    int i,j;                                  //sakaguchi cg UT-0024
    char  *server;
    struct in_addr addr;
    bool ipval;
    //ULONG base_seconds;
    //ULONG base_fraction;

    //base_seconds = 0xd2c96b90; // Jan 24, 2012 UTC
    //base_fraction = 0xa132db1e;
    //base_seconds = 0xe31f71e0; // 2020 09 30  UTC
    //base_fraction = 0x0;
    
    server = my_config.network.SntpServ1;     //Primary


    for(i=0; i<3; i++){ // 2-->3
        //if(1==i) server = my_config.network.SntpServ2;      //Secondary
        if(2==i) server = my_config.network.SntpServ2;      //Secondary

        ipval = JudgeIPAdrs(server);
        if(ipval){
            inet_aton(server , &addr);
            dest = addr.s_addr;
            Printf("IP  ADDRESS is: %d.%d.%d.%d \n\r", (int)(dest>>24), (int)(dest>>16)&0xFF, (int)(dest>>8)&0xFF, (int)(dest)&0xFF );

        }
        else{
            /* Lookup server name using DNS */
            Printf("   sntp_setup_and_run (DNS) %d %s\r\n", i, server);
            //status = nx_dns_host_by_name_get(&g_dns0, (UCHAR *)server, &dest, 5000);            // 2000 ->> 5000(3min) 
            status = nx_dns_host_by_name_get(&g_dns0, (UCHAR *)server, &dest, 1500);            // 1500(1min) 
            //status = nxd_dns_host_by_name_get(&g_dns0, (UCHAR *)server, &dest, 1500, NX_IP_VERSION_V4 ); 

            if (status != NX_SUCCESS) {
                Printf("  Unable to resolve ");
                Printf("  %s",server);
                Printf("\r\n");
                Printf("  Failed to start SNTP service. status = 0x%x\r\n", status);
                // 0xA1
                //  NX_DNS_NO_SERVER        0xA1  No DNS server was specified
                //  NX_DNS_QUERY_FAILED     0xA3  DNS query failed; no DNS server sent an 'answer'   
                DebugLog(LOG_DBG, "SNTP DNS Error %04X(%d)", status,i);
                // 09 15 tx_thread_sleep(500);       // 5sec
                continue;
            }

            Printf("    sntp_setup_and_run (DNS get) %ld\r\n", status);
        }
//        p_info->server_ip.nxd_ip_address.v4 = addr.ip_host;
        Printf("    IP  ADDRESS is: %d.%d.%d.%d \n\r", (int)(dest>>24), (int)(dest>>16)&0xFF, (int)(dest>>8)&0xFF, (int)(dest)&0xFF );

        /// Initialize Unicast SNTP client
        status = nx_sntp_client_initialize_unicast(&g_sntp0, dest);
        if ((status != NX_SUCCESS) && (status != NX_SNTP_CLIENT_ALREADY_STARTED)) {
            Printf("  Unable to initialize unicast SNTP client. status = 0x%x\r\n", status);
            Printf("  Failed to start SNTP service\r\n");
            Printf("  %s\r\n",server);
            DebugLog(LOG_DBG, "nx_sntp_client_initialize_unicast %ld",  status);
            // 09 15 tx_thread_sleep(500);       // 5sec
            continue;
        }

        // Set local time to 0 
        status = nx_sntp_client_set_local_time(&g_sntp0, 0, 0);
//        status = nx_sntp_client_set_local_time(&g_sntp0, base_seconds, base_fraction);
        Printf("    nx_sntp_client_set_local_time %ld\r\n", status);
        if (status != NX_SUCCESS) {
            Printf("  Unable to set local time for SNTP client. status = 0x%x\r\n", status);
            Printf("  Failed to start SNTP service\r\n");
            Printf("  %s\r\n",server);
            DebugLog(LOG_DBG, "nx_sntp_client_set_local_time %ld",  status);
            sntp_stop();                        //SNTP client stop
            continue;
        }

        
        // Run Unicast client
        status = nx_sntp_client_run_unicast(&g_sntp0);
        Printf("    nx_sntp_client_run_unicast %ld\r\n", status);
        if ((status != NX_SUCCESS) && (status != NX_SNTP_CLIENT_ALREADY_STARTED)) {
        //if (status != NX_SUCCESS) {
            Printf("  Unable to start unicast SNTP client. status = 0x%x\r\n", status);
            Printf("  %s\r\n",server);
            DebugLog(LOG_DBG, "nx_sntp_client_run_unicast %ld",  status);
            sntp_stop();                        //SNTP client stop
            continue;
        }

        //for(j=0;j<2;j++)
        {           // 10
            if((seconds = get_adjust_time()) != 0){
                Sntp_Status = SNTP_OK;                             // sakaguchi UT-0035
                sntp_stop();                        //SNTP client stop
                Printf("SNTP Start success[%s]\r\n", server );
                DebugLog(LOG_DBG, "SNTP Adjust Time Success %ld", seconds);
                return (0);
            }
            else{
                tx_thread_sleep(50);    // 10 --> 50
            }
        }

        DebugLog(LOG_DBG, "SNTP Get time Retry %d", i);
        Printf("   sntp_setup_and_run retry (DNS) %d\r\n", i);

        sntp_stop();                        //SNTP client stop
    }

    // 09 04 sntp_stop();                        //SNTP client stop

    DebugLog(LOG_DBG, "SNTP error %ld",  status);
    Sntp_Status = SNTP_ERROR;                             // sakaguchi UT-0035
    Printf("SNTP Start error\r\n");
    return (-1);

}
// sakaguchi cg UT-0024 Secondary対応 ↑


int sntp_setup_and_run2(void)
{
    UINT status;
    ULONG dest;
    uint32_t seconds;
    int i,j;                                  //sakaguchi cg UT-0024
    char  *server;
    //struct in_addr addr;
    //bool ipval;
    //ULONG base_seconds;
    //ULONG base_fraction;

    srv_info_t* p_info;
    p_info = &g_sntp_info;
    memset (&g_sntp_info, 0, sizeof(g_sntp_info));

    //base_seconds = 0xd2c96b90; // Jan 24, 2012 UTC
    //base_fraction = 0xa132db1e;
    //base_seconds = 0xe31f71e0; // 2020 09 30  UTC
    //base_fraction = 0x0;
    
    p_info->host = (char *)my_config.network.SntpServ1;     //Primary

    for(i=0; i<3; i++){ // 2-->3
        // リンクダウンを検出したらエラー終了 2022.04.11
        if(LINK_DOWN == Link_Status){
            Sntp_Status = SNTP_ERROR;
            Printf("SNTP Start error LinkDown\r\n");
            return (-1);
        }

        if(2==i) p_info->host = (char *)my_config.network.SntpServ2;      //Secondary

        status = find_ip_address( p_info );
        //status = find_ip_address_1_2( p_info );  　良くない
        if( status != NX_SUCCESS)
        {
            DebugLog(LOG_DBG, "SNTP DNS Error %04X(%d)", status, i);
            continue;
        }
        /*
        ipval = JudgeIPAdrs(server);
        if(ipval){
            inet_aton(server , &addr);
            dest = addr.s_addr;
            Printf("IP  ADDRESS is: %d.%d.%d.%d \n\r", (int)(dest>>24), (int)(dest>>16)&0xFF, (int)(dest>>8)&0xFF, (int)(dest)&0xFF );

        }
        else{
            // Lookup server name using DNS
            Printf("   sntp_setup_and_run (DNS) %d %s\r\n", i, server);
            //status = nx_dns_host_by_name_get(&g_dns0, (UCHAR *)server, &dest, 5000);            // 2000 ->> 5000(3min) 
            status = nx_dns_host_by_name_get(&g_dns0, (UCHAR *)server, &dest, 1500);            // 1500(1min) 
            //status = nxd_dns_host_by_name_get(&g_dns0, (UCHAR *)server, &dest, 1500, NX_IP_VERSION_V4 ); 

            if (status != NX_SUCCESS) {
                Printf("  Unable to resolve ");
                Printf("  %s",server);
                Printf("\r\n");
                Printf("  Failed to start SNTP service. status = 0x%x\r\n", status);
                // 0xA1
                //  NX_DNS_NO_SERVER        0xA1  No DNS server was specified
                //  NX_DNS_QUERY_FAILED     0xA3  DNS query failed; no DNS server sent an 'answer'   
                DebugLog(LOG_DBG, "SNTP DNS Error %04X(%d)", status,i);
                // 09 15 tx_thread_sleep(500);       // 5sec
                continue;
            }

            Printf("    sntp_setup_and_run (DNS get) %ld\r\n", status);
        }
        */

//        p_info->server_ip.nxd_ip_address.v4 = addr.ip_host;
        //Printf("    IP  ADDRESS is: %d.%d.%d.%d \n\r", (int)(dest>>24), (int)(dest>>16)&0xFF, (int)(dest>>8)&0xFF, (int)(dest)&0xFF );

        /// Initialize Unicast SNTP client
        dest = p_info->server_ip.nxd_ip_address.v4;
        status = nx_sntp_client_initialize_unicast(&g_sntp0, dest);
        if ((status != NX_SUCCESS) && (status != NX_SNTP_CLIENT_ALREADY_STARTED)) {
            Printf("  Unable to initialize unicast SNTP client. status = 0x%x\r\n", status);
            Printf("  Failed to start SNTP service\r\n");
            Printf("  %s\r\n",server);
            DebugLog(LOG_DBG, "nx_sntp_client_initialize_unicast %ld",  status);
            // 09 15 tx_thread_sleep(500);       // 5sec
            continue;
        }

        // Set local time to 0 
        status = nx_sntp_client_set_local_time(&g_sntp0, 0, 0);
//        status = nx_sntp_client_set_local_time(&g_sntp0, base_seconds, base_fraction);
        Printf("    nx_sntp_client_set_local_time %ld\r\n", status);
        if (status != NX_SUCCESS) {
            Printf("  Unable to set local time for SNTP client. status = 0x%x\r\n", status);
            Printf("  Failed to start SNTP service\r\n");
            Printf("  %s\r\n",server);
            DebugLog(LOG_DBG, "nx_sntp_client_set_local_time %ld",  status);
            sntp_stop();                        //SNTP client stop
            continue;
        }

        
        // Run Unicast client
        status = nx_sntp_client_run_unicast(&g_sntp0);
        Printf("    nx_sntp_client_run_unicast %ld\r\n", status);
        if ((status != NX_SUCCESS) && (status != NX_SNTP_CLIENT_ALREADY_STARTED)) {
        //if (status != NX_SUCCESS) {
            Printf("  Unable to start unicast SNTP client. status = 0x%x\r\n", status);
            Printf("  %s\r\n",server);
            DebugLog(LOG_DBG, "nx_sntp_client_run_unicast %ld",  status);
            sntp_stop();                        //SNTP client stop
            continue;
        }

        //for(j=0;j<2;j++)
        {           // 10
            if((seconds = get_adjust_time()) != 0){
                Sntp_Status = SNTP_OK;                             // sakaguchi UT-0035
                sntp_stop();                        //SNTP client stop
                Printf("SNTP Start success[%s]\r\n", server );
                DebugLog(LOG_DBG, "SNTP Adjust Time Success %ld", seconds);
                return (0);
            }
            else{
                tx_thread_sleep(50);    // 10 --> 50
            }
        }

        DebugLog(LOG_DBG, "SNTP Get time Retry %d", i);
        Printf("   sntp_setup_and_run retry (DNS) %d\r\n", i);

        sntp_stop();                        //SNTP client stop
    }

    // 09 04 sntp_stop();                        //SNTP client stop

    DebugLog(LOG_DBG, "SNTP error %ld",  status);
    Sntp_Status = SNTP_ERROR;                             // sakaguchi UT-0035
    Printf("SNTP Start error\r\n");
    return (-1);

}


/* Stop SNTP client */
void sntp_stop(void)
{
 //   if (&g_sntp0)
        nx_sntp_client_stop(&g_sntp0);
}


/**
 *
 */
unsigned long get_adjust_time(void)
{
    uint32_t seconds, milliseconds;
    UINT status;
    rtc_time_t Tm;
    rtc_time_t ct;
  
    int dest;
    UINT sntp_status;

    //Printf("----->  get-adjust_time \r\n");

    for (dest = 0; dest < 20; dest++)      //  10
    {
        seconds = 0;
        status = nx_sntp_client_receiving_updates(&g_sntp0, &sntp_status);
        Printf("=====>  get-adjust_time %04X  sntp_status[%04X] dest[%d] \r\n", status, sntp_status, dest );   // sakaguchi add UT-0024
        //DebugLog(LOG_DBG, "nx_sntp_client_receiving_updates %ld/%ld",  status, sntp_status);
//sakaguchi add UT-0024 ↓
        if(sntp_status != NX_TRUE){
            tx_thread_sleep(25);         // 2020 09 04 test
            continue;
        }
//sakaguchi add UT-0024 ↑

        if (status != NX_SUCCESS){
            ;
        }
        else{

            /* Get local time as set by SNTP client */
            status = nx_sntp_client_get_local_time(&g_sntp0, &seconds, &milliseconds, NX_NULL);
            Printf("=====>  nx_sntp_client_get_local_time %04X  seconds[%d]\r\n", status, seconds );
            if (status != NX_SUCCESS)
                return (0);
            if(seconds!=0){
                //g_rtc.p_api->calendarTimeGet(g_rtc.p_ctrl, &ct);
                with_retry_calendarTimeGet("get_adjust_time", &ct);     // 2022.09.09
                Printf("   #################   Current Time(%d): %d/%d %d:%d:%d         ################# \n", ct.tm_year,ct.tm_mon,ct.tm_mday,ct.tm_hour,ct.tm_min,ct.tm_sec);
                /* Convert to Unix epoch */
              
                seconds -= UNIX_TO_NTP_EPOCH_SECS;

                RTC_GSec2Date(seconds, &Tm);
                SetDST( Tm.tm_year );					// サマータイム再構築           // DataServer対応 サマータイム動作中か判定するために再構成が必要 sakaguchi 2021.04.07
                //Tm.tm_mday = 4;
                //Tm.tm_hour = 20;
                //Tm.tm_min = 30;
                //Tm.tm_sec = 0;


                Printf("   #################   SNTP    Time(%d): %d/%d %d:%d:%d (%d)    ################# \n", Tm.tm_year,Tm.tm_mon,Tm.tm_mday,Tm.tm_hour,Tm.tm_min,Tm.tm_sec, dest);

                Tm.tm_year += (2000-1900);
                Tm.tm_mon -= 1;
                adjust_time(&Tm);
                return (seconds);
            }
            tx_thread_sleep(50);    // test
            //Printf("=====>  get-adjust_time %02X, %ld\r\n", status, seconds);
        }
    }
    Printf("\r\n");
    return (seconds);

}


/* Return number of seconds since Unix Epoch (1/1/1970 00:00:00) */
unsigned long get_time(void)
{

    ULONG seconds;
    
    seconds = (ULONG)RTC_GetGMTSec();

    return (seconds);

}

unsigned long get_time2(void)
{
    uint32_t seconds, milliseconds;
    UINT status;
    rtc_time_t Tm;

    /* Get local time as set by SNTP client */
    status = nx_sntp_client_get_local_time(&g_sntp0, &seconds, &milliseconds, NX_NULL);
    if (status != NX_SUCCESS)
        return (0);

    /* Convert to Unix epoch */
    seconds -= UNIX_TO_NTP_EPOCH_SECS;

    RTC_GSec2Date(seconds, &Tm);
    Printf("   ################# get_time()  SNTP Time: %d/%d/%d %d:%d:%d    ################# \n", Tm.tm_year,Tm.tm_mon,Tm.tm_mday,Tm.tm_hour,Tm.tm_min,Tm.tm_sec);


    return (seconds);

}

/* Return sntp_boot_time in seconds */

/**
 *  Return SNTP boot time as received from server in seconds
 * @return SNTP boot time
 */
unsigned long get_sntp_boot_time(void)
{
    return (sntp_boot_time);
}




