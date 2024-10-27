/**
	@brief	SNTP�X���b�h
	@file	sntp_thread_entry.c
	 @note	�����R�[�h SJIS->UTF8
*/



#include "flag_def.h"
#include "Globals.h"
#include "General.h"
#include "config.h"
#include "MyDefine.h"
#include "Log.h"

#include "sntp_client.h"
#include "DateTime.h"
#include "sntp_thread.h"

extern TX_THREAD sntp_thread;


/**
 **  SNTP Thread entry function
 */
void sntp_thread_entry(void)
{
    /* TODO: add your own code here */

    int32_t ret = -1;
//    int i,j;
    int i;                      //sakaguchi cg UT-0024
//    ULONG   actual_events;

    while (1)
    {

        //tx_event_flags_get (&event_flags_sntp, 0xffffffff,TX_OR_CLEAR, &actual_events,TX_NO_WAIT);          // flag clear

        //for(i=0;i<10;i++){
        for(i=0;i<2;i++){
            Printf("  . sntp_setup_and_run.\r\n");
            //if (sntp_setup_and_run() == 0){
            if (sntp_setup_and_run2() == 0){    
                ret = 0;
                sntp_stop();
                break;
            }
            else{
                Printf("\r\n  sntp_setup_and_run() Failed.\r\n");
                ret = -1;                                               // 2022.04.11 念のため
                tx_thread_sleep(10);
            }
        }
        
        if(ret==0){
            Printf("----=> SNTP Success \r\n");
            PutLog(LOG_LAN, "SNTP Success");
            tx_event_flags_set (&event_flags_sntp, FLG_SNTP_SUCCESS,TX_OR);
        }
        else{
            Printf("----=> SNTP Error \r\n");
            PutLog(LOG_LAN, "SNTP Error(%d)",i);
            tx_event_flags_set (&event_flags_sntp, FLG_SNTP_ERROR,TX_OR);
        }

        tx_thread_suspend(&sntp_thread);
    }
}
