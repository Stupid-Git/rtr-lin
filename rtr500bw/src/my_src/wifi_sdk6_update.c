
/* Standard includes */
#include <stdlib.h>
#include <stdint.h>


#include <ti/drivers/net/wifi/simplelink.h>     // for "_i32" etc
#include <ti/drivers/net/wifi/device.h>         // for "sl_Task" etc

#include "ssp_common_api.h"

extern void Printf(const char *format, ...);


#define PROGRAMMING_CHUNK_SIZE 4096

static void do_update_UCF(const uint8_t *pUcfFile, const uint32_t UcfFile_len);
static void do_update_UCF(const uint8_t *pUcfFile, const uint32_t UcfFile_len)
{
    int32_t bytePos;
    int32_t bytesRemaining;
    _i32 slRetVal;

    long ErrorNum;
    long ExtendedErrorNum;

    //_u16 chunk_size_remaining;

    bytePos = 0;
    // bytesRead = sizeof(wifi_fwupdate_file_sp_4_5_0_11_3_1_0_5_3_1_0_25);
    bytesRemaining = (int32_t) UcfFile_len;

    Printf("Total Bytes = %d\r\n", bytesRemaining);
    while (bytesRemaining)
    {
        int32_t bytesToWrite;

        bytesToWrite = PROGRAMMING_CHUNK_SIZE;
        if(bytesRemaining < PROGRAMMING_CHUNK_SIZE)
            bytesToWrite = bytesRemaining;

        Printf("Offset[%d] Len = %d: ", bytePos, bytesToWrite);
        //_i32 sl_FsProgram(const _u8*  pData, _u16 DataLen ,const _u8 * pKey ,  _u32 Flags )
        slRetVal = sl_FsProgram (&pUcfFile[bytePos], (_u16)bytesToWrite, NULL, 0);
        Printf("sl_FsProgram retval = 0x%08x, (%d)\r\n", slRetVal, slRetVal);
        if (slRetVal == SL_API_ABORTED)    //timeout
        {
            break;
            //return( slRetVal );
        }
        else if (slRetVal < 0)    //error
        {
            ErrorNum = (long) slRetVal >> 16;
            ExtendedErrorNum = (_u16) (slRetVal & 0xFFFF);
            Printf("sl_FsProgram FAIL\r\n");
            Printf("sl_FsProgram ErrorNum         = 0x%04X (%d)\r\n", ErrorNum, ErrorNum);
            Printf("sl_FsProgram ExtendedErrorNum = 0x%04X (%d)\r\n", ExtendedErrorNum, ExtendedErrorNum);
            break;
        }
        if (slRetVal == 0)    //finished succesfully
        {
            Printf("sl_FsProgram END 1\r\n");
            break;
        }

        bytePos += bytesToWrite;
        bytesRemaining -= bytesToWrite;
    }

}

/*
...
sl_FsProgram retval = 0x00018000, (98304)
sl_FsProgram retval = 0x00019000, (102400)
sl_FsProgram retval = 0xd7fa0c43, (-671478717)
sl_FsProgram FAIL
sl_FsProgram ErrorNum         = 0xFFFFD7FA (-10246)
sl_FsProgram ExtendedErrorNum = 0x0C43 (3139)

#define SL_ERROR_FS_DEVELOPMENT_BOARD_WRONG_MAC                         (-10246L)

returnToFactoryDefault()

*/

/*
#include "boost_cc3135_sp_4.7.h"
static void do_update_4_7(void);
static void do_update_4_7(void)
{
    do_update_UCF(boost_cc3135_sp_4_7_ucf, boost_cc3135_sp_4_7_ucf_len);
}
*/

#include "boost_cc3135_sp_4.13.h"
static void do_update_4_13(void);
static void do_update_4_13(void)
{
    do_update_UCF(boost_cc3135_sp_4_13_ucf, boost_cc3135_sp_4_13_ucf_len);
}



static int16_t cc3135_PrintVersions(void);
static int16_t cc3135_PrintVersions(void)
{
    _i16 retVal = 0;

    Printf(">> Host Driver Version = '%s'\r\n", SL_DRIVER_VERSION); // e.g. "3.0.1.55"

    _u8 pConfigOpt;
    _u16 pConfigLen;
    SlDeviceVersion_t deviceVersion;

    pConfigOpt = SL_DEVICE_GENERAL_VERSION;
    pConfigLen = sizeof(SlDeviceVersion_t);

    /* Acquire device version */
    retVal = sl_DeviceGet(SL_DEVICE_GENERAL, &pConfigOpt, &pConfigLen, (_u8*)(&deviceVersion));
    if(retVal < 0) {
        Printf("sl_DeviceGet. (retVal = %d)\r\n", retVal);
        return retVal;
    }

    /*
     typedef struct
     {
     _u32                ChipId;
     _u8                 FwVersion[4];
     _u8                 PhyVersion[4];
     _u8                 NwpVersion[4];
     _u16                RomVersion;
     _u16                Padding;
     }SlDeviceVersion_t;
     */

    SlDeviceVersion_t *pDV = &deviceVersion;

    Printf(">> Version Info\r\n");

    Printf(">> Chip ID 0x%08X\r\n", pDV->ChipId);

    Printf(">> Version Info - FW %d.%d.%d.%d\r\n", pDV->FwVersion[0], pDV->FwVersion[1], pDV->FwVersion[2], pDV->FwVersion[3]);
    Printf(">> Version Info - PHY %d.%d.%d.%d\r\n", pDV->PhyVersion[0], pDV->PhyVersion[1], pDV->PhyVersion[2], pDV->PhyVersion[3]);
    Printf(">> Version Info - NWP %d.%d.%d.%d\r\n", pDV->NwpVersion[0], pDV->NwpVersion[1], pDV->NwpVersion[2], pDV->NwpVersion[3]);
    Printf(">> Version Info - Rom 0x%04X\r\n", pDV->RomVersion);

    return retVal;

}


static int16_t cc3135_Check_FW_Version(void);
static int16_t cc3135_Check_FW_Version(void)
{
    _i16 retVal = 0;

    //Printf(">> Host Driver Version = '%s'\r\n", SL_DRIVER_VERSION); // e.g. "3.0.1.55"

    _u8 pConfigOpt;
    _u16 pConfigLen;
    SlDeviceVersion_t deviceVersion;

    pConfigOpt = SL_DEVICE_GENERAL_VERSION;
    pConfigLen = sizeof(SlDeviceVersion_t);

    /* Acquire device version */
    retVal = sl_DeviceGet(SL_DEVICE_GENERAL, &pConfigOpt, &pConfigLen, (_u8*)(&deviceVersion));
    if(retVal < 0) {
        Printf("sl_DeviceGet. (retVal = %d)\r\n", retVal);
        retVal = 1;                                              // 2024 01 15 D.00.03.184 バージョン取得失敗時は書換で復帰できる
        return retVal;
    }

    /* The version info for sp_4.13 is as follows
    >> Host Driver Version = '3.0.1.71'
    >> Version Info
    >> Chip ID 0x31100000
    >> Version Info - FW 3.7.0.1
    >> Version Info - PHY 3.1.0.26
    >> Version Info - NWP 4.13.0.2
    >> Version Info - Rom 0x2222
    */


    SlDeviceVersion_t *pDV = &deviceVersion;

    retVal = 1;
    // Check Version Info - NWP 4.13.0.2
    if( (pDV->NwpVersion[0] ==  4) &&
        (pDV->NwpVersion[1] == 13) &&
        (pDV->NwpVersion[2] ==  0) &&
        (pDV->NwpVersion[3] ==  2) )
    {
        retVal = 0;
    }

    return retVal;

}




static int16_t cc3135_GetMacAddr(_u8 *pMacAddress);
static int16_t cc3135_GetMacAddr(_u8 *pMacAddress)
{
    _i16    retVal = 0;
    _u16    pConfigLen = SL_MAC_ADDR_LEN;

    retVal = sl_NetCfgGet(SL_NETCFG_MAC_ADDRESS_GET, NULL, &pConfigLen, pMacAddress);
    if (retVal < 0) {
        return retVal;
    }

    return 0;
}

/*
static int16_t cc3135_SetMacAddr(_u8 *pMacAddress);
static int16_t cc3135_SetMacAddr(_u8 *pMacAddress)
{
       _i16   Status;
       sl_NetCfgSet(SL_NETCFG_MAC_ADDRESS_SET, 1, SL_MAC_ADDR_LEN, (_u8*)pMacAddress);
       Status = sl_Stop(10);
       if (Status)
       {
              return Status;
       }
       Status = sl_Start(NULL, NULL, NULL);
       return Status;
}
*/

static int16_t cc3135_Print_MacAddress(void);
static int16_t cc3135_Print_MacAddress(void)
{
    _i16 retVal = 0;

    uint8_t pMacAddress[6];
    memset(pMacAddress, 0, sizeof(_u8) * SL_WLAN_BSSID_LENGTH);

    retVal = cc3135_GetMacAddr(pMacAddress);
    if (retVal == 0)
    {
        Printf("CC3135 Mac Address = %02X:%02X:%02X:%02X:%02X:%02X \r\n", pMacAddress[0], pMacAddress[1],
                pMacAddress[2], pMacAddress[3], pMacAddress[4], pMacAddress[5]);
    }
    return retVal;
}

/*
static void cc3135_test_wifi0(void);
static void cc3135_test_wifi0(void)
{
    ssp_err_t ssp_err;


    Printf("\r\n");
    Printf("\r\n");
    Printf("\r\n");
    Printf("cc3135_test_wifi0 START\r\n");

    ssp_err = g_sf_wifi0.p_api->open(g_sf_wifi0.p_ctrl, g_sf_wifi0.p_cfg);
    if(ssp_err != SSP_SUCCESS) {
        Printf(" g_sf_wifi0.p_api->open. ssp_err = %d, 0x%x\r\n", ssp_err, ssp_err);
    }

    cc3135_Print_MacAddress();

    ssp_err = g_sf_wifi0.p_api->close(g_sf_wifi0.p_ctrl);
    if(ssp_err != SSP_SUCCESS) {
        Printf(" g_sf_wifi0.p_api->close ssp_err = %d, 0x%x\r\n", ssp_err, ssp_err);
    }

    // 1. ->open / ->close
    // RetVal = sl_WlanDisconnect (); returns -2071L #define  SL_ERROR_WLAN_WIFI_ALREADY_DISCONNECTED                        (-2071L)
    // 2. ->open / cc3135_Print_MacAddress / ->close


    Printf("\r\n");
    Printf("cc3135_test_wifi0 END\r\n");
    Printf("\r\n");
    Printf("\r\n");
}
*/


static void cc3135_update_wifi0(void);
static void cc3135_update_wifi0(void)
{
    ssp_err_t ssp_err;
    int32_t RetVal;

    ssp_err = g_sf_wifi0.p_api->open(g_sf_wifi0.p_ctrl, g_sf_wifi0.p_cfg);
    if(ssp_err != SSP_SUCCESS) {
        Printf(" g_sf_wifi0.p_api->open. ssp_err = %d, 0x%x\r\n", ssp_err, ssp_err);
    }

    //cc3135_Print_MacAddress();

    //cc3135_PrintVersions();


    RetVal = cc3135_Check_FW_Version();
    if(RetVal < 0) {
        Printf("CC3135 Firmware Check Failed\r\n");
        //TODO LOG ERROR
        goto L_CloseAndExit;
    }
    if(RetVal == 0) {
        Printf("CC3135 Firmware is up to date\r\n");
        goto L_CloseAndExit;
    }


    //----- Fall through to update Firmware -----
    //void do_update_4_7(void);
    //do_update_4_7(); // -10358L SL_ERROR_FS_CERT_STORE_OR_SP_DOWNGRADE                          (-10358L)

    void do_update_4_13(void);
    do_update_4_13();

    //----- After Update must Restart the CC3135 -----
    ssp_err = g_sf_wifi0.p_api->close(g_sf_wifi0.p_ctrl);
    if(ssp_err != SSP_SUCCESS) {
        Printf(" g_sf_wifi0.p_api->close ssp_err = %d, 0x%x\r\n", ssp_err, ssp_err);
        //=> SF_WIFI_CC31XX_A_Open, ssp_retval = 14
    }

#if 0 //--------------------------------------------
    tx_thread_sleep(10);

#define WIFI_RESET_ACTIVE     g_ioport.p_api->pinWrite(WIFI_RESET_PIN, IOPORT_LEVEL_LOW)
#define WIFI_RESET_INACTIVE   g_ioport.p_api->pinWrite(WIFI_RESET_PIN, IOPORT_LEVEL_HIGH)
    //ssp_err =
    g_ioport.p_api->pinCfg(WIFI_RESET_PIN,      IOPORT_CFG_PORT_DIRECTION_OUTPUT );
    WIFI_RESET_ACTIVE;
    tx_thread_sleep(1);
    WIFI_RESET_INACTIVE;
    tx_thread_sleep(1);
#endif  //--------------------------------------------

    //sl_Stop(10);
    //sl_Start(0, 0, 0);

    ssp_err = g_sf_wifi0.p_api->open(g_sf_wifi0.p_ctrl, g_sf_wifi0.p_cfg);
    if(ssp_err != SSP_SUCCESS) {
        Printf(" g_sf_wifi0.p_api->open. ssp_err = %d, 0x%x\r\n", ssp_err, ssp_err);
        //=> SF_WIFI_CC31XX_A_Open, ssp_retval = 14 // SSP_ERR_ALREADY_OPEN
    }

    cc3135_PrintVersions();

    cc3135_Print_MacAddress();

L_CloseAndExit: ;

    ssp_err = g_sf_wifi0.p_api->close(g_sf_wifi0.p_ctrl);
    if(ssp_err != SSP_SUCCESS) {
        Printf(" g_sf_wifi0.p_api->close ssp_err = %d, 0x%x\r\n", ssp_err, ssp_err);
    }

}



void call_to_check_and_update_CC3135_Firmware(void);
void call_to_check_and_update_CC3135_Firmware(void)
{
    //int i;

    Printf("\r\n");
    Printf("\r\n");
    Printf("================== Stop for CC3135 Update Check ===================\r\n");
    Printf("\r\n");
/*
    Printf("\r\n");
    for(i=0; i< 10; i++ ) {
        Printf("> ");
        tx_thread_sleep(100);
    }
    Printf("\r\n");
*/
     cc3135_update_wifi0();
/*
    void cc3135_test_wifi0(void);
    cc3135_test_wifi0();


    Printf("\r\n");
    for(i=0; i< 10; i++ ) {
        Printf("< ");
        tx_thread_sleep(100);
    }
    Printf("\r\n");
*/
    Printf("\r\n");
    Printf("\r\n");
    Printf("================== Continue with Network ===================\r\n");
    Printf("\r\n");

}
