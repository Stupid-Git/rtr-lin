/*
 ============================================================================
 Name        : tcp_server.c
 Author      : Karel Seeuwen
 Version     :
 Copyright   : 
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>

//int poll_server(void);
int thread_server(void);
int udp_server_main(void);


int main(void)
{

	//poll_server();

	thread_server();
	//udp_server_main();

	return EXIT_SUCCESS;
}

//https://github.com/weboutin/simple-socket-server/
