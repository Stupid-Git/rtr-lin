/*
 ============================================================================
 Name        : rtr500_v1.c
 Author      : Karel Seeuwen
 Version     :
 Copyright   : 
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // sleep()
int main(void)
{
	puts("!!!Hello World!!!"); /* prints !!!Hello World!!! */

	void file_struct_info(void);
	file_struct_info();

	void log_file_dummy(void);
	log_file_dummy();

	void my_config_file_dummy(void);
	my_config_file_dummy();

	/*
	void comp_log_init(void);
	void comp_log_test(void);
	comp_log_init();
	comp_log_test();
    */

	int tcpudp_start(void);
	//tcpudp_start();

	int cmd_thread_start(void);
	cmd_thread_start();

	sleep(10);
	puts("!!!Goodbye World!!!"); /* prints !!!Hello World!!! */

	return EXIT_SUCCESS;
}


/*
 * 15:16:25 **** Incremental Build of configuration Debug for project rtr500_v1 ****
make all
Building file: /home/karel/Projects/gh/Stupid-Git/rtr-lin/common/Sfmem.c
Invoking: GCC C Compiler
gcc -I"/home/karel/Projects/gh/Stupid-Git/rtr-lin/rtr500_v1/src" -I"/home/karel/Projects/gh/Stupid-Git/rtr-lin/rtr500_v1/my_porting" -I"/home/karel/Projects/gh/Stupid-Git/rtr-lin/my_src" -I"/home/karel/Projects/gh/Stupid-Git/rtr-lin/my_src/inc" -I"/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp" -I"/home/karel/Projects/gh/Stupid-Git/rtr-lin/common/inc" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"common/Sfmem.d" -MT"common/Sfmem.o" -o "common/Sfmem.o" "/home/karel/Projects/gh/Stupid-Git/rtr-lin/common/Sfmem.c"
Finished building: /home/karel/Projects/gh/Stupid-Git/rtr-lin/common/Sfmem.c

Building target: rtr500_v1
Invoking: GCC C Linker
gcc  -o "rtr500_v1"  ./src/main.o  ./my_src/Cmd_func.o ./my_src/Error.o ./my_src/General.o ./my_src/Globals.o ./my_src/glb_1.o  ./my_porting/td_kon_kon.o ./my_porting/td_kon_stub.o  ./comp/base.o ./comp/comp_auto_thread.o ./comp/comp_ble_thread.o ./comp/comp_cmd.o ./comp/comp_cmd_sub.o ./comp/comp_datetime.o ./comp/comp_gos.o ./comp/comp_led_thread.o ./comp/comp_log.o ./comp/comp_opto_thread.o ./comp/comp_tcp_thread.o ./comp/comp_tcpudp.o ./comp/comp_udp_thread.o ./comp/files.o ./comp/random_stuff.o ./comp/tdx.o  ./common/Convert.o ./common/Lang.o ./common/Sfmem.o ./common/Unicode.o ./common/Xml.o ./common/lzss.o    -lpthread
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `StatusPrintfB':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:9006: multiple definition of `StatusPrintfB'; ./comp/comp_cmd.o:/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd.c:716: first defined here
/usr/bin/ld: ./comp/comp_log.o:(.data.rel.local+0x0): multiple definition of `CatMsg'; ./my_src/General.o:/home/karel/Projects/gh/Stupid-Git/rtr-lin/my_src/inc/General.h:187: first defined here
/usr/bin/ld: ./comp/comp_udp_thread.o: in function `uint32to8':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_udp_thread.c:58: multiple definition of `uint32to8'; ./my_src/General.o:/home/karel/Projects/gh/Stupid-Git/rtr-lin/my_src/General.c:660: first defined here
/usr/bin/ld: ./comp/random_stuff.o: in function `Memcmp':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/random_stuff.c:17: multiple definition of `Memcmp'; ./my_src/General.o:/home/karel/Projects/gh/Stupid-Git/rtr-lin/my_src/General.c:959: first defined here
/usr/bin/ld: ./comp/random_stuff.o: in function `judge_T2checksum':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/random_stuff.c:42: multiple definition of `judge_T2checksum'; ./my_src/General.o:/home/karel/Projects/gh/Stupid-Git/rtr-lin/my_src/General.c:373: first defined here
/usr/bin/ld: ./comp/random_stuff.o: in function `AtoI':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/random_stuff.c:83: multiple definition of `AtoI'; ./my_src/General.o:/home/karel/Projects/gh/Stupid-Git/rtr-lin/my_src/General.c:1865: first defined here
/usr/bin/ld: ./comp/random_stuff.o: in function `AtoL':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/random_stuff.c:144: multiple definition of `AtoL'; ./my_src/General.o:/home/karel/Projects/gh/Stupid-Git/rtr-lin/my_src/General.c:1926: first defined here
/usr/bin/ld: ./my_src/Cmd_func.o: in function `ParamInt':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/my_src/Cmd_func.c:192: undefined reference to `PLIST'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/my_src/Cmd_func.c:194: undefined reference to `PLIST'
/usr/bin/ld: ./my_src/Cmd_func.o: in function `ParamHex':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/my_src/Cmd_func.c:215: undefined reference to `PLIST'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/my_src/Cmd_func.c:217: undefined reference to `PLIST'
/usr/bin/ld: ./my_src/Cmd_func.o: in function `ParamInt32':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/my_src/Cmd_func.c:239: undefined reference to `PLIST'
/usr/bin/ld: ./my_src/Cmd_func.o:/home/karel/Projects/gh/Stupid-Git/rtr-lin/my_src/Cmd_func.c:241: more undefined references to `PLIST' follow
/usr/bin/ld: ./my_src/General.o: in function `GetFormatString':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/my_src/General.c:1264: undefined reference to `RTC_GSec2LCT'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/my_src/General.c:1267: undefined reference to `RTC_GetLCT'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/my_src/General.c:1330: undefined reference to `RTC'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/my_src/General.c:1330: undefined reference to `RTC'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/my_src/General.c:1334: undefined reference to `RTC'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/my_src/General.c:1334: undefined reference to `RTC'
/usr/bin/ld: ./my_src/General.o: in function `GetTimeString':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/my_src/General.c:1401: undefined reference to `RTC_GSec2Date'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/my_src/General.c:1404: undefined reference to `RTC_GetLCT'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/my_src/General.c:1459: undefined reference to `RTC'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/my_src/General.c:1459: undefined reference to `RTC'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/my_src/General.c:1462: undefined reference to `RTC'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/my_src/General.c:1462: undefined reference to `RTC'
/usr/bin/ld: ./my_src/General.o: in function `GetTimeFormat':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/my_src/General.c:1495: undefined reference to `RTC_GSec2Date'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/my_src/General.c:1497: undefined reference to `RTC_GetLCT'
/usr/bin/ld: ./comp/base.o: in function `read_my_settings':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/base.c:52: undefined reference to `my_rpt_number'
/usr/bin/ld: ./comp/base.o: in function `read_repair_settings':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/base.c:159: undefined reference to `__NOP'
/usr/bin/ld: ./comp/comp_cmd.o: in function `AnalyzeCmd':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd.c:441: undefined reference to `PLIST'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd.c:441: undefined reference to `PLIST'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd.c:444: undefined reference to `PLIST'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd.c:444: undefined reference to `PLIST'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd.c:444: undefined reference to `PLIST'
/usr/bin/ld: ./comp/comp_cmd.o:/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd.c:445: more undefined references to `PLIST' follow
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFInitilize':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:477: undefined reference to `RF_power_on_init'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFWriteInfo':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:730: undefined reference to `vhttp_sysset_sndon'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFWriteSettingsFile':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:951: undefined reference to `RF_full_moni_init'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFWriteUnitSettingsFile':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:1043: undefined reference to `get_regist_group_adr_ser'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:1093: undefined reference to `kk_delta_set'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFWriteVGroup':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:1662: undefined reference to `vhttp_sysset_sndon'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFBSStatus':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:1773: undefined reference to `isUSBState'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFWriteBleParam':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:2002: undefined reference to `vhttp_sysset_sndon'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:2011: undefined reference to `LED_Set'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:2021: undefined reference to `LED_Set'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFWriteNetwork':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:2524: undefined reference to `Net_LOG_Write'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFWriteNetParam':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:2746: undefined reference to `vhttp_sysset_sndon'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:2822: undefined reference to `Net_LOG_Write'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFReadWalnAp':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:2880: undefined reference to `wifi_module_scan'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFWriteWlanParam':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:2999: undefined reference to `Net_LOG_Write'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFWriteDtime':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:3062: undefined reference to `RTC_GetLCT'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:3160: undefined reference to `vhttp_sysset_sndon'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:3161: undefined reference to `RTC_Create'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:3172: undefined reference to `RTC_GSec2LCT'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:3198: undefined reference to `RTC'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:3206: undefined reference to `SetDST'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:3209: undefined reference to `RTC_GSec2Date'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:3215: undefined reference to `adjust_time'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:3225: undefined reference to `RTC_Date2GSec'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:3229: undefined reference to `SetDST'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:3230: undefined reference to `RTC_GSec2Date'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:3232: undefined reference to `GetSummerAdjust'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:3233: undefined reference to `RTC'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:3233: undefined reference to `RTC_GSec2Date'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:3240: undefined reference to `adjust_time'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFReadDtime':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:3277: undefined reference to `RTC_GetGMTSec'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:3279: undefined reference to `RTC_GSec2LCT'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:3281: undefined reference to `RTC_GetFormStr'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:3285: undefined reference to `GetSummerAdjust'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFWProxy':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:3490: undefined reference to `Net_LOG_Write'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFWriteWarning':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:3753: undefined reference to `LED_Set'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:3765: undefined reference to `vhttp_sysset_sndon'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFWriteMonitor':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:3991: undefined reference to `vhttp_sysset_sndon'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFWriteSuction':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:4310: undefined reference to `kk_delta_set_all'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:4321: undefined reference to `vhttp_sysset_sndon'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFExtentionReg':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:4402: undefined reference to `optc'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:4403: undefined reference to `optc'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:4404: undefined reference to `optc'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:4406: undefined reference to `optc'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:4407: undefined reference to `optc'
/usr/bin/ld: ./comp/comp_cmd_sub.o:/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:4408: more undefined references to `optc' follow
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFExtentionSerial':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:4439: undefined reference to `pOptUartRx'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:4439: undefined reference to `pOptUartRx'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:4439: undefined reference to `pOptUartRx'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:4439: undefined reference to `pOptUartRx'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFExtSettings':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:4493: undefined reference to `kk_delta_set'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:4509: undefined reference to `kk_delta_set'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFExtBlp':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:4886: undefined reference to `pOptUartRx'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:4894: undefined reference to `pOptUartRx'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:4897: undefined reference to `pOptUartRx'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:4905: undefined reference to `pOptUartRx'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFExtSOH':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:4975: undefined reference to `pOptUartRx'
/usr/bin/ld: ./comp/comp_cmd_sub.o:/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:4975: more undefined references to `pOptUartRx' follow
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFExWirelessLevel':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:5013: undefined reference to `start_rftask'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:5018: undefined reference to `PLIST'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFRelayWirelessLevel':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:5073: undefined reference to `start_rftask'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:5078: undefined reference to `PLIST'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFWirelessSettingsRead':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:5129: undefined reference to `start_rftask'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFWirelessSettingsWrite':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:5180: undefined reference to `start_rftask'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:5191: undefined reference to `WR_set'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:5192: undefined reference to `WR_set'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:5193: undefined reference to `WR_set'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:5194: undefined reference to `RegistTableWrite'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFWirelessMonitor':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:5232: undefined reference to `start_rftask'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFWirelessSuction':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:5297: undefined reference to `start_rftask'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:5336: undefined reference to `PLIST'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFSearchKoki':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:5432: undefined reference to `start_rftask'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:5468: undefined reference to `PLIST'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFSearchRelay':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:5551: undefined reference to `start_rftask'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:5587: undefined reference to `PLIST'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFExRealscan':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:5668: undefined reference to `start_rftask'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:5723: undefined reference to `PLIST'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFThroughoutSearch':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:5833: undefined reference to `start_rftask'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:5886: undefined reference to `PLIST'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:5942: undefined reference to `get_regist_group_size'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:5944: undefined reference to `get_regist_group_adr'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:5946: undefined reference to `RF_get_regist_RegisterRemoteUnit'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:5953: undefined reference to `RF_table_make'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:5958: undefined reference to `RF_WirelessGroup_RefreshParentTable'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:5959: undefined reference to `RF_oya_log_last'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFRecStartGroup':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:6004: undefined reference to `start_rftask'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:6041: undefined reference to `PLIST'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFWirelessRecStop':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:6117: undefined reference to `start_rftask'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFSetProtection':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:6155: undefined reference to `start_rftask'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFExInterval':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:6193: undefined reference to `start_rftask'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFWirelessRegistration':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:6231: undefined reference to `start_rftask'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFBleSettingsRead':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:6273: undefined reference to `start_rftask'
/usr/bin/ld: ./comp/comp_cmd_sub.o:/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:6348: more undefined references to `start_rftask' follow
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFBleSettingsWrite':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:6359: undefined reference to `RegistTableBleWrite'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFScaleSettingsRead':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:6394: undefined reference to `start_rftask'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFScaleSettingsWrite':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:6445: undefined reference to `start_rftask'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:6456: undefined reference to `RegistTableSlWrite'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFPushSettingsRead':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:6491: undefined reference to `start_rftask'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:6495: undefined reference to `PLIST'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFPushSettingsWrite':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:6536: undefined reference to `start_rftask'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFPushSuction':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:6581: undefined reference to `start_rftask'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:6619: undefined reference to `PLIST'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFPushRecErase':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:6724: undefined reference to `start_rftask'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFPushItemListRead':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:6761: undefined reference to `start_rftask'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:6766: undefined reference to `PLIST'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFPushItemListWrite':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:6807: undefined reference to `start_rftask'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFPushPersonListRead':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:6844: undefined reference to `start_rftask'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:6849: undefined reference to `PLIST'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFPushPersonListWrite':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:6888: undefined reference to `start_rftask'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFPushWorkgrpRead':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:6925: undefined reference to `start_rftask'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:6930: undefined reference to `PLIST'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFPushWorkgrpWrite':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:6970: undefined reference to `start_rftask'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFPushMessage':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7010: undefined reference to `start_rftask'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFPushRemoteMeasure':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7048: undefined reference to `start_rftask'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFPushClock':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7087: undefined reference to `start_rftask'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFTestLAN':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7175: undefined reference to `SendMonitorFile'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7178: undefined reference to `SendWarningFile'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7181: undefined reference to `SendSuctionData'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7184: undefined reference to `SendLogData'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFTestWarning':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7292: undefined reference to `SendWarningFile'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFReadSettingsGroup':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7391: undefined reference to `get_regist_group_adr_ser'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7395: undefined reference to `get_regist_relay_inf'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7402: undefined reference to `GetRoot_from_ResultTable'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7414: undefined reference to `get_regist_group_adr_rpt_ser'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7418: undefined reference to `get_regist_relay_inf_relay'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7425: undefined reference to `GetRoot_from_ResultTable'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7431: undefined reference to `get_regist_scan_unit'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7452: undefined reference to `get_regist_group_adr'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7458: undefined reference to `get_regist_scan_unit'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFReadMonitorResult':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7569: undefined reference to `RF_get_regist_group_adr'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7623: undefined reference to `RF_ParentTable_Get'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7627: undefined reference to `RF_ParentTable_Get'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7631: undefined reference to `RF_ParentTable_Get'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7635: undefined reference to `RF_ParentTable_Get'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7639: undefined reference to `RF_ParentTable_Get'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7643: undefined reference to `RF_RssiTable_Get'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7647: undefined reference to `RF_BatteryTable_Get'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7658: undefined reference to `RF_ParentTable_Get'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7662: undefined reference to `RF_ParentTable_Get'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7666: undefined reference to `RF_ParentTable_Get'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7670: undefined reference to `RF_ParentTable_Get'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7674: undefined reference to `RF_ParentTable_Get'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7678: undefined reference to `RF_RssiTable_Get'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7682: undefined reference to `RF_BatteryTable_Get'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFClearMonitor':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7713: undefined reference to `realscan_buff'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7714: undefined reference to `realscan_buff'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7718: undefined reference to `RF_clear_rssi'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7726: undefined reference to `RTC_GetGMTSec'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7745: undefined reference to `RF_clear_rssi'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7760: undefined reference to `RF_clear_rssi'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7800: undefined reference to `RTC_GetGMTSec'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFKensa':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7835: undefined reference to `rf_command'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7846: undefined reference to `RF_monitor_execute'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7848: undefined reference to `MakeMonitorXML'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7852: undefined reference to `RF_monitor_execute'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7853: undefined reference to `MakeMonitorFTP'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7855: undefined reference to `MakeMonitorXML'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7869: undefined reference to `get_regist_group_adr_ser'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7873: undefined reference to `RF_monitor_execute'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7874: undefined reference to `SendWarningFile'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7879: undefined reference to `RF_event_execute'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7885: undefined reference to `RF_event_execute'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7900: undefined reference to `AUTO_control'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFInspect':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:7984: undefined reference to `KensaMain'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `CFR500Special':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:8162: undefined reference to `R500C_Direct'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `IR_header_read':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:8705: undefined reference to `optc'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:8706: undefined reference to `optc'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:8707: undefined reference to `optc'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:8711: undefined reference to `optc'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `IR_Nomal_command':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:8732: undefined reference to `optc'
/usr/bin/ld: ./comp/comp_cmd_sub.o:/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:8733: more undefined references to `optc' follow
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `IR_soh_direct':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:8828: undefined reference to `pOptUartTx'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:8830: undefined reference to `optc'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:8835: undefined reference to `optc'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `IR_Regist':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:8850: undefined reference to `optc'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:8854: undefined reference to `optc'
/usr/bin/ld: ./comp/comp_cmd_sub.o: in function `opt_command_exec':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/comp/comp_cmd_sub.c:8870: undefined reference to `optc'
/usr/bin/ld: ./common/Convert.o: in function `Mul10Add':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/common/Convert.c:1406: undefined reference to `pow'
/usr/bin/ld: /home/karel/Projects/gh/Stupid-Git/rtr-lin/common/Convert.c:1447: undefined reference to `pow'
/usr/bin/ld: ./common/Unicode.o: in function `Sjis2EUC':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/common/Unicode.c:139: undefined reference to `SSP_PARAMETER_NOT_USED'
/usr/bin/ld: ./common/Xml.o: in function `XML_Create':
/home/karel/Projects/gh/Stupid-Git/rtr-lin/common/Xml.c:104: undefined reference to `Printf'
collect2: error: ld returned 1 exit status
make: *** [makefile:47: rtr500_v1] Error 1
"make all" terminated with exit code 2. Build might be incomplete.

15:16:26 Build Failed. 217 errors, 0 warnings. (took 374ms)

 *
 *
 *
 */
