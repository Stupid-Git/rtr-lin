#ifndef WIFI_SDK6_H_
#define WIFI_SDK6_H_

//2023.11.16
#include <ti/drivers/net/wifi/wlan.h>
extern uint8_t CC3135_New_Security;

uint32_t sdk6_connect_secValue_from_scan_secValue(uint8_t scan_secValue, uint8_t* p_connect_secValue);

#endif /* WIFI_SDK6_H_ */
