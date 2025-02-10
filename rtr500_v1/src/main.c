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
#include <stdint.h>
#include <string.h>

#include <unistd.h> // sleep()

int main(void)
{
	puts("!!!Hello World!!!"); /* prints !!!Hello World!!! */

	int acm0_main(void);
	acm0_main();
    return 0;

    void files_load_and_or_init(void);
    files_load_and_or_init();
    return 0;

    int libCurlTest_main(void);
	libCurlTest_main();
	return 0;
/*
!!!Hello World!!!
fact_config     size = 256
my_config       size = 8192
group_data      size = 4096
regist_data     size = 32768
temp_data       size = 65536
CertFile_WS_1   size = 4096
CertFile_USER_1 size = 4096
log_data        size = 131072
td_log_data     size = 262144
SN: 5(05)

BaseName = RTR500BW_5F580123

rewrite_settings()

my_config crc 969429AC

rewrite_settings() Exec End
*/


	/*
	void comp_log_init(void);
	void comp_log_test(void);
	comp_log_init();
	comp_log_test();
    */

	int cmd_thread_init(void);
    int cmd_thread_start(void);
    cmd_thread_init();
    cmd_thread_start();

    int cmd_thread_fakeT2(void);
    cmd_thread_fakeT2();

    int tcpudp_start(void);
    //tcpudp_start();

    void MakeMonitorXML(int32_t Test);
    //MakeMonitorXML(1);
    //MakeMonitorXML(0); // make a real monitor file //TODO requires binary input data to be made

    char DbgCmd[128];

    uint32_t process_debug_str_X(char DbgCmd[]); // Xml - SMS
    strcpy(DbgCmd, "Xj");
    //process_debug_str_X(DbgCmd);

    /**/
    uint32_t process_http_test_H(char DbgCmd[]);
    strcpy(DbgCmd, "HM1");
    //process_http_test_H(DbgCmd);
    strcpy(DbgCmd, "HW1");
    //process_http_test_H(DbgCmd);
    strcpy(DbgCmd, "HR1");
    process_http_test_H(DbgCmd);
    /**/

//printf("main exiting soon 2\n");
//    sleep(30);
//printf("main exiting soon 1\n");
//    sleep(30);
printf("main exiting soon 0\n");
    sleep(30);
	puts("!!!Goodbye World!!!"); /* prints !!!Hello World!!! */

	return EXIT_SUCCESS;
}
