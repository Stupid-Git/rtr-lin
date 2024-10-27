/*
 * main.c
 *
 *  Created on: Oct 21, 2024
 *      Author: karel
 */

#include "ftdi.h"

int eeprom_main(int argc, char **argv);
int serial_test_main(int argc, char **argv);


int mytest_main(int argc, char **argv);

int main(int argc, char **argv)
{
	int rtn;

	rtn = 0;

	//rtn = eeprom_main(argc, argv);
	//rtn = serial_test_main(argc, argv);
	rtn = mytest_main(argc, argv);


	return rtn;
}
