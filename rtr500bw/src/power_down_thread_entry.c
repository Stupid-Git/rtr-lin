/**
 * @brief   power down thread
 * @file	power_down_thread_entry.c
 * @note	2019.Dec.26 ビルドワーニング潰し完了
 * @note	2020.Aug.07	0807ソース反映済み 
 */
 
#include "flag_def.h"
#include "General.h"
#include "MyDefine.h"
#include "Sfmem.h"
#include "led_thread.h"
#include "event_thread.h"
#include "usb_thread.h"
#include "rf_thread.h"
#include "ble_thread.h"
#include "power_down_thread.h"
#include "led_thread_entry.h"


//extern TX_THREAD rf_thread;
extern TX_THREAD event_thread;
extern TX_THREAD led_thread;
extern TX_THREAD usb_thread;
extern TX_THREAD usb_tx_thread;
extern TX_THREAD ble_thread;




/**
 **  Power Down Thread entry function
 */
#if 0
void power_down_thread_entry(void)
{
    /* TODO: add your own code here */

    UINT status;
    ULONG actual_events;
    ssp_err_t err;
    lvd_status_t lvd1_status;
 //   ioport_level_t level;
    int high_count;

    //g_lpmv2_deep_standby.p_api->lowPowerCfg(g_lpmv2_deep_standby.p_cfg);
    //g_lpmv2_deep_standby.p_api->init();


    while (1)
    {

        status = tx_event_flags_get (&g_power_down_event_flags, FLG_POWER_DOWN,TX_OR_CLEAR, &actual_events,TX_WAIT_FOREVER);
        high_count = 0;
        Printf("=======>>> Power Down Detect Thread !!\r\n");

        //LED_All_Off();
        //LED_POW_OFF;
        LED_ALM_OFF;
        LED_CBSY_OFF;
        LED_DIAG_OFF;
        LED_WIFI_OFF;
        //LED_ACT_OFF;
        //LED_BLE_B_OFF;
        LED_BLE_R_OFF;
        LED_BLE_G_OFF;

        // Frash Memory 制御終了待ち
        tx_mutex_get(&mutex_sfmem, WAIT_100MSEC);

        g_lvd1.p_api->open(g_lvd1.p_ctrl, g_lvd1.p_cfg);


        R_USBFS->SYSCFG_b.DPRPU = 0U;
        R_USBFS->SYSCFG_b.DRPD = 0U;
        R_USBFS->SYSCFG_b.USBE = 0U;
        R_USBFS->SYSCFG_b.SCKE = 0U;
        R_ETHERC0->ECMR = 0U;
        R_BUS->CSRC1[4].CSnCR_b.EXENB = 0;

//        status = g_sf_power_profiles_v2_common.p_api->lowPowerApply(g_sf_power_profiles_v2_common.p_ctrl, &g_sf_power_profiles_v2_low_power);     // sakaguchi del

        Printf("=======>>> Power Down Detect Thread 2 !!\r\n");
        //__BKPT(1);

        tx_thread_suspend(&event_thread);
	    tx_thread_suspend(&ble_thread);
	    tx_thread_suspend(&led_thread);
	    //tx_thread_suspend(&led_cyc_thread);
	    tx_thread_suspend(&usb_thread);
		tx_thread_suspend(&usb_tx_thread);
		
//        g_lpmv2_deep_standby.p_api->lowPowerModeEnter();          // sakaguchi del

        Printf("=======>>> Power Down Detect Thread 3 !!\r\n");

        while(1){
            err = g_lvd1.p_api->statusGet( g_lvd1.p_ctrl, &lvd1_status );
            if(SSP_SUCCESS != err){
                NVIC_SystemReset();          // reboot !!           // ステータスが取得できないため即リブート
            }
            else{
                LED_ACT_OFF;
                if( LVD_THRESHOLD_CROSSING_DETECTED == lvd1_status.crossing_detected )  // VCC < Threshold voltage
                {
                    LED_BLE_B_OFF;
                    high_count = 0;
                    //g_lpmv2_deep_standby.p_api->lowPowerModeEnter();
                    while(1){
                        err = g_lvd1.p_api->statusGet( g_lvd1.p_ctrl, &lvd1_status );
                        if(SSP_SUCCESS != err){
                            NVIC_SystemReset();          // reboot !!           // ステータスが取得できないため即リブート
                        }
                        else{
                            if(lvd1_status.current_state == LVD_CURRENT_STATE_ABOVE_THRESHOLD){     //  VCC >= threshold or monitor is disabled.
                                LED_BLE_R_ON;
                                high_count ++;
                                if(high_count>30){
                                    NVIC_SystemReset();          // reboot !!
                                }
                            }
                            else{
                                high_count = 0;
                            }
                        }
                        tx_thread_sleep(100);   // isec wait
                        /*
                        g_ioport.p_api-> pinRead(IOPORT_PORT_00_PIN_14, &level);
                        
                        if(level == IOPORT_LEVEL_HIGH){
                            g_ioport.p_api->pinWrite(IOPORT_PORT_00_PIN_14, IOPORT_LEVEL_LOW);
                        }
                        else{
                            g_ioport.p_api->pinWrite(IOPORT_PORT_00_PIN_14, IOPORT_LEVEL_HIGH);
                        }
                        */
                    }   
                }
                else{   // VCC > Threshold voltage(2.85V)
                    Printf("=======>>> Power Down Detect Thread 4 !!  %02X %02X %d\r\n", lvd1_status.crossing_detected,lvd1_status.current_state, high_count);
                    // 正常電圧が、10秒継続した場合は、Reboot
                    //if(lvd1_status.current_state == LVD_CURRENT_STATE_ABOVE_THRESHOLD){
                        high_count ++;
                        if(high_count>10){
                            NVIC_SystemReset();          // reboot !!
                        }
                    //}
                    //else{
                    //    high_count = 0;
                    //}
                    LED_POW_OFF;
                    tx_thread_sleep(100);
                    
                }
            }
        }

    }
    if (status != 0)    ///< if文不要
    { 
        status = 0;
    }
}
#endif

void power_down_thread_entry(void)
{
    /* TODO: add your own code here */

    UINT status;
    ULONG actual_events;
    ssp_err_t err;
    lvd_status_t lvd1_status;
 //   ioport_level_t level;
    int high_count;

    //g_lpmv2_deep_standby.p_api->lowPowerCfg(g_lpmv2_deep_standby.p_cfg);
    //g_lpmv2_deep_standby.p_api->init();


    while (1)
    {

        status = tx_event_flags_get (&g_power_down_event_flags, FLG_POWER_DOWN,TX_OR_CLEAR, &actual_events,TX_WAIT_FOREVER);
        high_count = 0;
        Printf("=======>>> Power Down Detect Thread !!\r\n");

        //LED_All_Off();
        //LED_POW_OFF;
        LED_ALM_OFF;
        LED_CBSY_OFF;
        LED_DIAG_OFF;
        LED_WIFI_OFF;
        //LED_ACT_OFF;
        //LED_BLE_B_OFF;
        LED_BLE_R_OFF;
        LED_BLE_G_OFF;

        // Frash Memory 制御終了待ち
        tx_mutex_get(&mutex_sfmem, WAIT_100MSEC);


        tx_thread_suspend(&event_thread);
	    tx_thread_suspend(&ble_thread);
	    tx_thread_suspend(&led_thread);
	    //tx_thread_suspend(&led_cyc_thread);
	    tx_thread_suspend(&usb_thread);
		tx_thread_suspend(&usb_tx_thread);
		
        R_USBFS->SYSCFG_b.DPRPU = 0U;
        R_USBFS->SYSCFG_b.DRPD = 0U;
        R_USBFS->SYSCFG_b.USBE = 0U;
        R_USBFS->SYSCFG_b.SCKE = 0U;
        R_ETHERC0->ECMR = 0U;
        R_BUS->CSRC1[4].CSnCR_b.EXENB = 0;          // SRAM Disable

        R_SYSTEM->SBYCR_b.OPE = 0;                 // SRAM Disable

        R_SYSTEM->DSPBYCR_b.DEEPCUT = 1;            // BSRAM USB Power Off



        R_BUS->CSRC1[4].CSnCR_b.EXENB = 0;

        R_PFS->P905PFS_b.PDR = 1;       // CS
        R_PFS->P905PFS_b.PODR = 1;
        R_PFS->P905PFS_b.PMR = 0;

        R_PFS->P600PFS_b.PDR = 1;       // RD
        R_PFS->P600PFS_b.PODR = 1;
        R_PFS->P600PFS_b.PMR = 0;

        R_PFS->P601PFS_b.PDR = 1;       // WR
        R_PFS->P601PFS_b.PODR = 1;
        R_PFS->P601PFS_b.PMR = 0;

        R_PFS->P100PFS_b.PMR = 0;       // D0
        R_PFS->P100PFS_b.PDR = 1;
        R_PFS->P100PFS_b.PODR = 0;

        R_PFS->P101PFS_b.PMR = 0;       // D1
        R_PFS->P101PFS_b.PDR = 1;
        R_PFS->P101PFS_b.PODR = 0;
        
        R_PFS->P102PFS_b.PMR = 0;       // D2
        R_PFS->P102PFS_b.PDR = 1;
        R_PFS->P102PFS_b.PODR = 0;
        
        R_PFS->P103PFS_b.PMR = 0;       // D3
        R_PFS->P103PFS_b.PDR = 1;
        R_PFS->P103PFS_b.PODR = 0;
        
        R_PFS->P104PFS_b.PMR = 0;       // D4
        R_PFS->P104PFS_b.PDR = 1;
        R_PFS->P104PFS_b.PODR = 0;
                
        R_PFS->P105PFS_b.PMR = 0;       // D5
        R_PFS->P105PFS_b.PDR = 1;
        R_PFS->P105PFS_b.PODR = 0;               
        
        R_PFS->P106PFS_b.PMR = 0;       // D6
        R_PFS->P106PFS_b.PDR = 1;
        R_PFS->P106PFS_b.PODR = 0;

        R_PFS->P107PFS_b.PMR = 0;       // D7
        R_PFS->P107PFS_b.PDR = 1;
        R_PFS->P107PFS_b.PODR = 0;

        R_PFS->P608PFS_b.PMR = 0;       // A0
        R_PFS->P608PFS_b.PDR = 1;
        R_PFS->P608PFS_b.PODR = 0;

        R_PFS->P115PFS_b.PMR = 0;       // A1
        R_PFS->P115PFS_b.PDR = 1;
        R_PFS->P115PFS_b.PODR = 0;

        R_PFS->P114PFS_b.PMR = 0;       // A2
        R_PFS->P114PFS_b.PDR = 1;
        R_PFS->P114PFS_b.PODR = 0;

        R_PFS->P113PFS_b.PMR = 0;       // A3
        R_PFS->P113PFS_b.PDR = 1;
        R_PFS->P113PFS_b.PODR = 0;

        R_PFS->P112PFS_b.PMR = 0;       // A4
        R_PFS->P112PFS_b.PDR = 1;
        R_PFS->P112PFS_b.PODR = 0;

        R_PFS->P111PFS_b.PMR = 0;       // A5
        R_PFS->P111PFS_b.PDR = 1;
        R_PFS->P111PFS_b.PODR = 0;

        R_PFS->P301PFS_b.PMR = 0;       // A6
        R_PFS->P301PFS_b.PDR = 1;
        R_PFS->P301PFS_b.PODR = 0;

        R_PFS->P302PFS_b.PMR = 0;       // A7
        R_PFS->P302PFS_b.PDR = 1;
        R_PFS->P302PFS_b.PODR = 0;

        R_PFS->P303PFS_b.PMR = 0;       // A8
        R_PFS->P303PFS_b.PDR = 1;
        R_PFS->P303PFS_b.PODR = 0;

        R_PFS->P304PFS_b.PMR = 0;       // A9
        R_PFS->P304PFS_b.PDR = 1;
        R_PFS->P304PFS_b.PODR = 0;

        R_PFS->P305PFS_b.PMR = 0;       // A10
        R_PFS->P305PFS_b.PDR = 1;
        R_PFS->P305PFS_b.PODR = 0;

        R_PFS->P306PFS_b.PMR = 0;       // A11
        R_PFS->P306PFS_b.PDR = 1;
        R_PFS->P306PFS_b.PODR = 0;

        R_PFS->P307PFS_b.PMR = 0;       // A12
        R_PFS->P307PFS_b.PDR = 1;
        R_PFS->P307PFS_b.PODR = 0;

        R_PFS->P308PFS_b.PMR = 0;       // A13
        R_PFS->P308PFS_b.PDR = 1;
        R_PFS->P308PFS_b.PODR = 0;

        R_PFS->P309PFS_b.PMR = 0;       // A14
        R_PFS->P309PFS_b.PDR = 1;
        R_PFS->P309PFS_b.PODR = 0;

        R_PFS->P310PFS_b.PMR = 0;       // A15
        R_PFS->P310PFS_b.PDR = 1;
        R_PFS->P310PFS_b.PODR = 0;

        R_PFS->P205PFS_b.PMR = 0;       // A16
        R_PFS->P205PFS_b.PDR = 1;
        R_PFS->P205PFS_b.PODR = 0;

        R_PFS->P207PFS_b.PMR = 0;       // A17
        R_PFS->P207PFS_b.PDR = 1;
        R_PFS->P207PFS_b.PODR = 0;
     
#if 0
        g_lvd1.p_api->open(g_lvd1.p_ctrl, g_lvd1.p_cfg);

/*
        R_USBFS->SYSCFG_b.DPRPU = 0U;
        R_USBFS->SYSCFG_b.DRPD = 0U;
        R_USBFS->SYSCFG_b.USBE = 0U;
        R_USBFS->SYSCFG_b.SCKE = 0U;
        R_ETHERC0->ECMR = 0U;
        R_BUS->CSRC1[4].CSnCR_b.EXENB = 0;
*/
        status = g_sf_power_profiles_v2_common.p_api->lowPowerApply(g_sf_power_profiles_v2_common.p_ctrl, &g_sf_power_profiles_v2_low_power);     // sakaguchi del

        Printf("=======>>> Power Down Detect Thread 2 !!\r\n");
        //__BKPT(1);

        tx_thread_suspend(&event_thread);
	    tx_thread_suspend(&ble_thread);
	    tx_thread_suspend(&led_thread);
	    //tx_thread_suspend(&led_cyc_thread);
	    tx_thread_suspend(&usb_thread);
		tx_thread_suspend(&usb_tx_thread);
		
        R_USBFS->SYSCFG_b.DPRPU = 0U;
        R_USBFS->SYSCFG_b.DRPD = 0U;
        R_USBFS->SYSCFG_b.USBE = 0U;
        R_USBFS->SYSCFG_b.SCKE = 0U;
        R_ETHERC0->ECMR = 0U;
        R_BUS->CSRC1[4].CSnCR_b.EXENB = 0;          // SRAM Disable

        R_SYSTEM->SBYCR_b.OPE = 0;                 // SRAM Disable

        R_SYSTEM->DSPBYCR_b.DEEPCUT = 1;            // BSRAM USB Power Off
//====
        R_SYSTEM->VBTICTLR = 0;
        R_SYSTEM->SBYCR_b.SSBY = 1; 
        R_SYSTEM->DSPBYCR_b.DPSBY = 1;
        //__WFI();

        //while(1);
//=====

        g_lpmv2_deep_standby.p_api->lowPowerModeEnter(); 
#endif

//  R_SYSTEM->DSPBYCR_b.DEEPCUT = 1;            // BSRAM USB Power Off

//        while(1);
//        g_lpmv2_deep_standby.p_api->lowPowerModeEnter();          // sakaguchi del

        //Printf("=======>>> Power Down Detect Thread 3 !!\r\n");

        while(1){
            err = g_lvd1.p_api->statusGet( g_lvd1.p_ctrl, &lvd1_status );
            if(SSP_SUCCESS != err){
                NVIC_SystemReset();          // reboot !!           // ステータスが取得できないため即リブート
            }
            else{
                if( LVD_THRESHOLD_CROSSING_DETECTED == lvd1_status.crossing_detected )  // VCC < Threshold voltage
                {
                    high_count = 0;
                    //g_lpmv2_deep_standby.p_api->lowPowerModeEnter();
                    while(1){
                        err = g_lvd1.p_api->statusGet( g_lvd1.p_ctrl, &lvd1_status );
                        if(SSP_SUCCESS != err){
                            NVIC_SystemReset();          // reboot !!           // ステータスが取得できないため即リブート
                        }
                        else{
                            if(lvd1_status.current_state == LVD_CURRENT_STATE_ABOVE_THRESHOLD){     //  VCC >= threshold or monitor is disabled.
                                high_count ++;
                                if(high_count>30){
                                    NVIC_SystemReset();          // reboot !!
                                }
                            }
                            else{
                                high_count = 0;
                            }
                        }
                        tx_thread_sleep(100);   // isec wait
                        /*
                        g_ioport.p_api-> pinRead(IOPORT_PORT_00_PIN_14, &level);
                        
                        if(level == IOPORT_LEVEL_HIGH){
                            g_ioport.p_api->pinWrite(IOPORT_PORT_00_PIN_14, IOPORT_LEVEL_LOW);
                        }
                        else{
                            g_ioport.p_api->pinWrite(IOPORT_PORT_00_PIN_14, IOPORT_LEVEL_HIGH);
                        }
                        */
                    }   
                }
                else{   // VCC > Threshold voltage(2.85V)
                    //Printf("=======>>> Power Down Detect Thread 4 !!  %02X %02X %d\r\n", lvd1_status.crossing_detected,lvd1_status.current_state, high_count);
                    // 正常電圧が、10秒継続した場合は、Reboot
                    //if(lvd1_status.current_state == LVD_CURRENT_STATE_ABOVE_THRESHOLD){
                        high_count ++;
                        if(high_count>10){
                            NVIC_SystemReset();          // reboot !!
                        }
                    //}
                    //else{
                    //    high_count = 0;
                    //}
                    tx_thread_sleep(100);
                    
                }
            }
        }

    }
    if (status != 0)    ///< if文不要
    { 
        status = 0;
    }
}






/**
 *
 * @param p_args
 * @note  Power Down Detect IRQ6 
 */
void irq6_callback(external_irq_callback_args_t * p_args)
{
    SSP_PARAMETER_NOT_USED(p_args);

    Printf("------->>> Power Down Detect IRQ6  !!\r\n");

    tx_event_flags_set (&g_power_down_event_flags, FLG_POWER_DOWN,TX_OR);

}

/**
 *
 * @param p_args
 * @note  VCC < threshold
 */
void lvd1_callback(lvd_callback_args_t *p_args)
{
    SSP_PARAMETER_NOT_USED(p_args);
    if( LVD_CURRENT_STATE_BELOW_THRESHOLD == p_args->status.current_state )
    {
        //g_ioport.p_api->pinWrite(IOPORT_PORT_00_PIN_14, IOPORT_LEVEL_LOW);   
        //tx_event_flags_set (&g_power_down_event_flags, FLG_POWER_DOWN,TX_OR);   // 9.25  
        ;//while(1);
    }
}



/*
void callback_lvd1(lvd_callback_args_t * p_args)
{
    SSP_PARAMETER_NOT_USED( p_args );
    if( LVD_CURRENT_STATE_BELOW_THRESHOLD == p_args->status.current_state )
    {
        LED_POW_OFF;
    }
}

void lvd1_callback(lvd_callback_args_t *p_args)
{
    SSP_PARAMETER_NOT_USED(p_args);
    if( LVD_CURRENT_STATE_BELOW_THRESHOLD == p_args->status.current_state )
    {
        LED_POW_OFF;

        R_MSTP->MSTPCRB_b.MSTPB11 = 1U;     // USB FS
        R_MSTP->MSTPCRB_b.MSTPB15 = 1U;     // ETH
        R_MSTP->MSTPCRB_b.MSTPB18 = 1U;     // 
        R_MSTP->MSTPCRB_b.MSTPB23 = 1U;     // 
        R_MSTP->MSTPCRB_b.MSTPB24 = 1U;     // 
        R_MSTP->MSTPCRB_b.MSTPB26 = 1U;     // 
        R_MSTP->MSTPCRB_b.MSTPB27 = 1U;     // 

        R_MSTP->MSTPCRC_b.MSTPC14 = 1U;     // 
        R_MSTP->MSTPCRC_b.MSTPC31 = 1U;     // 

        R_MSTP->MSTPCRD_b.MSTPD5 =  1U;     // 

        //while(1);

    }
}
*/
