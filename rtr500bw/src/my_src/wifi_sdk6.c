
//2023.11.16

#include "wifi_sdk6.h"

uint32_t sdk6_connect_secValue_from_scan_secValue(uint8_t scan_secValue, uint8_t* p_connect_secValue)
{
    uint32_t status = 0;
    uint8_t connect_secValue;

    connect_secValue = 0xFF;

    if(scan_secValue == SL_WLAN_SECURITY_TYPE_BITMAP_OPEN)          //  (0)
        connect_secValue = SL_WLAN_SEC_TYPE_OPEN;                   //  (0)

    if(scan_secValue == SL_WLAN_SECURITY_TYPE_BITMAP_WEP)           //  (1)
        connect_secValue = SL_WLAN_SEC_TYPE_WEP;                    //  (1)

    if(scan_secValue == SL_WLAN_SECURITY_TYPE_BITMAP_WPA)           //  (2)
        connect_secValue = SL_WLAN_SEC_TYPE_WPA;                    //  (2)

    if(scan_secValue == SL_WLAN_SECURITY_TYPE_BITMAP_WPA2)          //  (4)
        connect_secValue = SL_WLAN_SEC_TYPE_WPA_WPA2;               //  (2)

#if 1 //CONFIG_USE_NEW_SECURITY
    if(scan_secValue == SL_WLAN_SECURITY_TYPE_BITMAP_MIX_WPA_WPA2)  //  (6)
        connect_secValue = SL_WLAN_SEC_TYPE_WPA2_PLUS;              // (11)

    if(scan_secValue == SL_WLAN_SECURITY_TYPE_BITMAP_WPA3)          //  (5)
        connect_secValue = SL_WLAN_SEC_TYPE_WPA3;                   // (12)
#endif

    if(connect_secValue == 0xFF) {
        status = 1; // NG
    } else {
        *p_connect_secValue = connect_secValue;
        status = 0; // OK
    }

    return status;
}

