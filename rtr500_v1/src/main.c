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

	int libCurlTest_main(void);
	libCurlTest_main();
	return 0;

	void file_struct_info(void);
	file_struct_info();

    void my_config_file_dummy(void);
    my_config_file_dummy();

    void log_file_dummy(void);
	log_file_dummy();

    void registry_file_dummy(void);
    registry_file_dummy();

    void group_file_dummy(void);
    group_file_dummy();



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
