/*
 ============================================================================
 Name        : signals.c
 Author      : Karel Seeuwen
 Version     :
 Copyright   : 
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>

int epoll_eventfd_main(int argc, char *argv[]);
int eventfd_worker_main(int argc, char *argv[]);
int stack_overflow_1_main(int argc, char *argv[]);
int my_event_main(int argc, char *argv[]);


int main(int argc, char *argv[])
{

	//epoll_eventfd_main(argc, argv);
	//eventfd_worker_main(argc, argv);
	//stack_overflow_1_main(argc, argv);

	//my_event_main(argc, argv);

	int my_queue_main();
	//my_queue_main();
	int my_queue_main2();
	//my_queue_main2();

	int my_queue_struct_main();
	//my_queue_struct_main();
	int my_queue_main3();
	my_queue_main3();


	return EXIT_SUCCESS;
}
