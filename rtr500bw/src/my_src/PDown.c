/**
 * @file    PDown.c
 *
 * @date    Created on: 2019/11/07
 * @author  tooru.hayashi
 */

#define _PDOWN_C_

#include "hal_data.h"

#include "MyDefine.h"
#include "General.h"
#include "led_thread_entry.h"

/*
    DPSBYCR.DEEPCUT[1:0] ビットの設定により、ディープソフトウェアスタンバイモード時にスタンバイSRAM へ電源を供給できます。
    DPSBYCR.DEEPCUT[1:0] ビットが00b の場合、スタンバイSRAM のデータをディープソフトウェアスタンバイモードで保持できます。
*/





/* Protect Register key */
#define PRCR_KEY 0xA5U

/* Protect Register access bits */
#define PRCR_ACCESS 0x02U

//プロトタイプ
void deep_standby_check(void);


/*
int pd_count;

void irq6_callback(external_irq_callback_args_t * p_args)
{
    SSP_PARAMETER_NOT_USED(p_args);

    if(pd_count%2)
        LED_All_Off();
    else 
        LED_All_On();
        
    pd_count ++;
    Printf("----> Power Down ditect !! %d\r\n", pd_count);


}
*/



/**
 *
 */
void deep_standby_check(void)
{
    /* Check if the device has just left Deep Software Standby mode. */
    if (1U == R_SYSTEM->RSTSR0_b.DPSRSTF)
    {
        uint16_t current_prcr_value;

        /* Clear the Deep Software Standby Reset flag */
        R_SYSTEM->RSTSR0_b.DPSRSTF = 0U;

        /* Gain access to Deep Software Standby registers to clear
         * the interrupt enable and flag for IRQ11 and RTC Interval.
         * This was the IRQ used to wake the device.
         * The Deep Software Standby registers are write protected.
         */

        /* Read the current value */
        current_prcr_value = R_SYSTEM->PRCR;
        /* Set the password of A5xx and set bit 2 */
        current_prcr_value |= (PRCR_KEY << 8) | PRCR_ACCESS;
        /* Write value back to PRCR to gain access */
        R_SYSTEM->PRCR = current_prcr_value;

        /* Disable IRQ6
         * in the Deep Software Standby Interrupt Enable Register */
        R_SYSTEM->DPSIER0_b.DIRQ6E = 0U;

        /* Disable RTC Interval Interrupt
         * in the Deep Software Standby Interrupt Enable Register */
        R_SYSTEM->DPSIER2_b.DTRTCIIE = 0U;

        /* Clear the IRQ6 interrupt flag
         * This has read/write timing limitations
         * (refer to section 11.2.17 of the Hardware User's Manual).
         * Hence the do - while loop around clearing the flag.
         */
        do
        {
            R_SYSTEM->DPSIFR0_b.DIRQ6F = 0U;
        }
        while (R_SYSTEM->DPSIFR0_b.DIRQ6F != 0U);

        /* Clear the RTC Interval interrupt flag */
        do
        {
            R_SYSTEM->DPSIFR2_b.DTRTCIIF = 0U;
        }
        while (R_SYSTEM->DPSIFR2_b.DTRTCIIF != 0U);

        /* Refer to section 11.9 of the Hardware User's Manual
         * for more information of how I/O keep works.
         *
         * If in the configuration settings for LPM V2 Deep Software Standby Driver,
         *      Maintain or reset the IO port states on exit from deep standby mode = Maintain the IO port states
         * then you have to clear the IOKEEP bit to set the IO port states back to their original value.
         * If
         *      Maintain or reset the IO port states on exit from deep standby mode = Reset the IO port states
         * then this is done automatically after the internal reset.
         *
         * In this example "Maintain the IO port states" is selected
         * so clear the IOKEEP bit. */
        R_SYSTEM->DSPBYCR_b.IOKEEP = 0U;

        /* Turn register protect back on */
        current_prcr_value &= (uint16_t)~PRCR_ACCESS;
        current_prcr_value |= PRCR_KEY << 8;
        R_SYSTEM->PRCR = current_prcr_value;
    }
}

