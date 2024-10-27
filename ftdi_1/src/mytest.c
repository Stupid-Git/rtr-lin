


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <ftdi.h>




/*
[auto] LED_Set: Msg:10, flag:1005 [BMLED_PWR]
[auto] RF TX >> (err= 0), H=01, C=39, S=00, L=0010 D6 23 C7 52 7D B0 F4 66 00 00 DD 04
[auto] RF RX << (err= 0), H=01, C=39, S=06, L=0004 00 00 00 00 44 00
###RF_command execute 00
R500 command make 30
        R500_radio_communication start
        R500_radio_communication_once start
        R500_send_recv() t=6000
rfm_send com close ssp_err=0
rfm_send com open ssp_err=0
[auto] LED_Set: Msg:10, flag:1005 [BMLED_PWR]
[auto] RF TX >> (err= 0), H=01, C=42, S=00, L=0016 00 00 00 FF 00 00 00 00 00 55 01 01 40 30 01 83 9D 02
        R500_send_recv()  rfm_send  end
[auto] RF RX << (err= 0), H=01, C=34, S=06, L=0004 00 00 00 00 3F 00
^^[auto] RF RX << (err= 0), H=01, C=68, S=15, L=0031 01 00 2A FF 00 00 00 00 00 E1 0C 01 40 68 0B 96 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 FE 03
[auto] <<NG>>  rfm_recv rtn=F4[RFM_R500_NAK] / err=0000 (82) ------------------------------
[auto] RF RX << (err= 0), H=01, C=68, S=15, L=0031 02 00 2F FF 00 00 00 00 00 E1 0C 01 40 68 0B 96 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 04 04
[auto] <<NG>>  rfm_recv rtn=F4[RFM_R500_NAK] / err=0000 (314) ------------------------------
[auto] RF RX << (err= 0), H=01, C=68, S=15, L=0031 03 00 31 FF 00 00 00 00 00 E1 0C 01 40 68 0B 96 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 07 04
[auto] <<NG>>  rfm_recv rtn=F4[RFM_R500_NAK] / err=0000 (313) ------------------------------
R500_radio_communication_once rtn 00
        R500_radio_communication end 0
 *
 */

#define _just_Printf printf

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static void DumpHex(int addText, const void* data, size_t size);
static void DumpHex(int addText, const void* data, size_t size)
{
    char ascii[17];
    size_t i, j;
    ascii[16] = '\0';
    for (i = 0; i < size; ++i) {
        _just_Printf("%02X ", ((unsigned char*)data)[i]);
        if(addText != 0)
        {
            if (((unsigned char *) data)[i] >= ' ' && ((unsigned char *) data)[i] <= '~')
            {
                ascii[i % 16] = ((unsigned char *) data)[i];
            }
            else
            {
                ascii[i % 16] = '.';
            }
            if ((i + 1) % 8 == 0 || i + 1 == size)
            {
                _just_Printf (" ");
                if ((i + 1) % 16 == 0)
                {
                    _just_Printf ("|  %s \r\n", ascii);
                }
                else if (i + 1 == size)
                {
                    ascii[(i + 1) % 16] = '\0';
                    if ((i + 1) % 16 <= 8)
                    {
                        _just_Printf (" ");
                    }
                    for (j = (i + 1) % 16; j < 16; ++j)
                    {
                        _just_Printf ("   ");
                    }
                    _just_Printf ("|  %s \r\n", ascii);
                }
            }
        }
    }
    if(addText == 0)
    {
        _just_Printf("\r\n");
    }
}

void PrintHex( char* banner, uint8_t* pData, int len)
{
    _just_Printf("%s\r\n", banner);
    DumpHex( 1, pData, (size_t)len);
}


//=============================================================================
// https://stackoverflow.com/questions/3756323/how-to-get-the-current-time-in-milliseconds-from-c-in-linux
#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

//karel "round" require to add math library "-lm" in linker

void print_current_time_with_ms (char* banner)
{
    long            ms; // Milliseconds
    time_t          s;  // Seconds
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);

    s  = spec.tv_sec;
    ms = round(spec.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds
    if (ms > 999) {
        s++;
        ms = 0;
    }

    //printf("Current time: %"PRIdMAX".%03ld seconds since the Epoch\n", (intmax_t)s, ms);
    printf("%s : %"PRIdMAX".%03ld seconds\n", banner, (intmax_t)s, ms);

}


//=============================================================================
//=============================================================================


struct ftdi_context *ftdi;
#include "string.h" // memcpy


int readPacket(int size)
{
	int f;
	uint8_t buf[128];
    int len;
    int wait_count = 0;

	//usleep(1 * 10000);

    //print_current_time_with_ms("RX a");

    struct timespec spec_a;
    struct timespec spec_b;
    clock_gettime(CLOCK_REALTIME, &spec_a);

    do
    {
    	f = ftdi_read_data(ftdi, &buf[0], 1);
    	if((f==1) && buf[0] == 0x01)
    	{
    		//fprintf(stderr, "read %d bytes\n", f);
    		//fflush(stderr);
    		break;
    	}
    	wait_count++;

    	clock_gettime(CLOCK_REALTIME, &spec_b);
        if((spec_b.tv_sec - spec_a.tv_sec) > 10 )
        {
        	printf("Waited for more than 10 seconds\n");
        	fflush(stdout);
        	return -1;
        }
    } while (f!=1);
	//fprintf(stderr, "read %d bytes\n", f);
	//fflush(stderr);
    //print_current_time_with_ms("RX a1");

	f = ftdi_read_data(ftdi, &buf[1], 4);
	//if(f==4)
	//	fprintf(stderr, "read %d bytes\n", f);

	len = buf[4]*256 + buf[3];
if(len>120) len=120;

	f = ftdi_read_data(ftdi, &buf[5], len + 2);
	//if(f==(len + 2))
	//	fprintf(stderr, "read %d bytes\n", f);

	//fflush(stderr);

    //print_current_time_with_ms("RX b");
	PrintHex("RX", buf, 5 + len + 2);
	//printf("wait_count = %d\n", wait_count);
	fflush(stdout);
	return 0;
}

void xx()
{
	int f = 0;

	uint8_t buf[128];
	uint8_t buf0[] =  {0x01, 0x39, 0x00,  0x0a,0x00,  0xD6, 0x23, 0xC7, 0x52,  0x7D, 0xB0, 0xF4, 0x66,  0x00, 0x00,   0xDD, 0x04};
	//uint8_t buf0r[] = {0x01, 0x39, 0x06,  0x04,0x00,  0x00, 0x00, 0x00, 0x00,                                       0x44, 0x00};

	//RF TX >> (err= 0), H=01, C=42, S=00, L=0016 00 00 00 FF 00 00 00 00 00 55 01 01 40 30 01 83 9D 02
	uint8_t buf1[] =  {0x01, 0x42, 0x00,  0x10,0x00,  0x00, 0x00, 0x00, 0xFF,  0x00, 0x00, 0x00, 0x00,  0x00, 0x55, 0x01, 0x01,  0x40, 0x30, 0x01, 0x83,   0x9D, 0x02};


	//================================================================

	memcpy(buf, buf0, sizeof(buf0));
    print_current_time_with_ms("TX a");
    f = ftdi_write_data(ftdi, buf, sizeof(buf0));
    print_current_time_with_ms("TX b");
    PrintHex("TX", buf, sizeof(buf0));
	fflush(stdout);

	readPacket(11);

    //================================================================
	memcpy(buf, buf1, sizeof(buf1));
	print_current_time_with_ms("TX a");
	f = ftdi_write_data(ftdi, buf, sizeof(buf1));
	print_current_time_with_ms("TX b");
    PrintHex("TX", buf, sizeof(buf1));
	fflush(stdout);

	readPacket(11);

	readPacket(38);
	readPacket(38);
	readPacket(38);
	//readPacket(38);


    /*
    RF TX H=01, C=39, S=00, L=0010 D6 23 C7 52 7D B0 F4 66 00 00 DD 04
    RF RX H=01, C=39, S=06, L=0004 00 00 00 00 44 00
    RF TX H=01, C=42, S=00, L=0016 00 00 00 FF 00 00 00 00 00 55 01 01 40 30 01 83 9D 02
    RF RX H=01, C=34, S=06, L=0004 00 00 00 00 3F 00
    RF RX H=01, C=68, S=15, L=0031 01 00 2A FF 00 00 00 00 00 E1 0C 01 40 68 0B 96 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 FE 03
    RF RX H=01, C=68, S=15, L=0031 02 00 2F FF 00 00 00 00 00 E1 0C 01 40 68 0B 96 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 04 04
    RF RX H=01, C=68, S=15, L=0031 03 00 31 FF 00 00 00 00 00 E1 0C 01 40 68 0B 96 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 07 04
     */
/*
karel@lindev:~/Projects/gh/Stupid-Git/rtr-lin/ftdi_1/Debug$ sudo ./ftdi_1
TX  01 39 00  0A 00  D6 23 C7 52 7D B0 F4 66 00 00 DD 04
RX  01 39 06  04 00  00 00 00 00 44 00
TX  01 42 00  10 00  00 00 00 FF 00 00 00 00 00 55 01 01 40 30 01 83 9D 02
RX  01 34 06  04 00  00 00 00 00 3F 00
RX  01 68 15  1F 00  01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 00 00 00 00 9E 00
RX  01 68 15  1F 00  02 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 00 00 00 00 9F 00
RX  01 68 15  1F 00  03 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 00 00 00 00 A0 00
*/

}

void FUNC2(void)
{
/*
==============================================================================================================
==============================================================================================================
==============================================================================================================
[auto] RF TX >> (err= 0), H=01, C=39, S=00, L=0010 11 00 82 5F 9E 09 16 67 00 00 5A 02
[auto] RF RX << (err= 0), H=01, C=39, S=06, L=0004 00 00 00 00 44 00
###RF_command execute 00
R500 command make 30
        R500_radio_communication start
        R500_radio_communication_once start
        R500_send_recv() t=6000
rfm_send com close ssp_err=0
rfm_send com open ssp_err=0
[auto] LED_Set: Msg:10, flag:1005 [BMLED_PWR]
[auto] RF TX >> (err= 0), H=01, C=42, S=00, L=0016 00 00 00 FF 00 00 00 00 00 A6 01 00 40 30 01 83 ED 02
        R500_send_recv()  rfm_send  end
[auto] RF RX << (err= 0), H=01, C=34, S=06, L=0004 00 00 00 00 3F 00
$BOGUS$ sync_time = 1^$BOGUS$ sync_time = 1^=====>> LOG[RF :[auto] Channel Busy](19)
[auto] RF RX << (err= 0), H=01, C=34, S=12, L=0004 00 00 00 00 4B 00
==============================================================================================================
==============================================================================================================
==============================================================================================================
*/

	//TX >> (err= 0), H=01, C=39, S=00, L=0010   11 00 82 5F 9E 09 16 67 00 00 5A 02
	//RX << (err= 0), H=01, C=39, S=06, L=0004 00 00 00 00 44 00
	//TX >> (err= 0), H=01, C=42, S=00, L=0016   00 00 00 FF  00 00 00 00  00 A6 01 00  40 30 01 83  ED 02
	//RX << (err= 0), H=01, C=34, S=06, L=0004 00 00 00 00 3F 00
    //RX << (err= 0), H=01, C=34, S=12, L=0004 00 00 00 00 4B 00

	uint8_t buft1[] = {0x01, 0x39, 0x00, 0x0a, 0x00, 0x11, 0x00, 0x82, 0x5F,  0x9E, 0x09, 0x16, 0x67,  0x00, 0x00, 0x5A, 0x02};
	uint8_t buft2[] = {0x01, 0x42, 0x00, 0x10,0x00,  0x00, 0x00, 0x00, 0xFF,  0x00, 0x00, 0x00, 0x00,  0x00, 0xA6, 0x01, 0x00,  0x40, 0x30, 0x01, 0x83,  0xED, 0x02};
//	uint8_t buft2[] = {0x01, 0x42, 0x00, 0x10,0x00,  0x00, 0x00, 0x00, 0xFF,  0x00, 0x00, 0x00, 0x00,  0x00, 0xA7, 0x01, 0x01,  0x40, 0x30, 0x01, 0x83,  0xEF, 0x02};

	int f = 0;
	uint8_t buf[128];

	//================================================================

	memcpy(buf, buft1, sizeof(buft1));
    print_current_time_with_ms("TX a");
    f = ftdi_write_data(ftdi, buf, sizeof(buft1));
    print_current_time_with_ms("TX b");
    PrintHex("TX", buf, sizeof(buft1));
	fflush(stdout);

	readPacket(11);

    //================================================================
	memcpy(buf, buft2, sizeof(buft2));
	print_current_time_with_ms("TX a");
	f = ftdi_write_data(ftdi, buf, sizeof(buft2));
	print_current_time_with_ms("TX b");
    PrintHex("TX", buf, sizeof(buft2));
	fflush(stdout);

	readPacket(11);

	readPacket(38);
	readPacket(38);
	readPacket(38);

	//readPacket(38);
}


void FUNC3(void)
{
/*
==============================================================================================================
==============================================================================================================
==============================================================================================================
[auto] RF TX >> (err= 0), H=01, C=39, S=00, L=0010 11 00 82 5F 9E 09 16 67 00 00 5A 02
[auto] RF RX << (err= 0), H=01, C=39, S=06, L=0004 00 00 00 00 44 00
###RF_command execute 00
R500 command make 30
        R500_radio_communication start
        R500_radio_communication_once start
        R500_send_recv() t=6000
rfm_send com close ssp_err=0
rfm_send com open ssp_err=0
[auto] LED_Set: Msg:10, flag:1005 [BMLED_PWR]
[auto] RF TX >> (err= 0), H=01, C=42, S=00, L=0016 00 00 00 FF 00 00 00 00 00 A7 01 01 40 30 01 83 EF 02
        R500_send_recv()  rfm_send  end
[auto] RF RX << (err= 0), H=01, C=34, S=06, L=0004 00 00 00 00 3F 00
$BOGUS$ sync_time = 1^$BOGUS$ sync_time = 1^[auto] RF RX << (err= 0), H=01, C=68, S=06, L=0031 01 01 B4 0D 00 00 02 00 00 00 01 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 E1 04 E2 04 9F 0A
[auto] RF RX << (err= 0), H=01, C=68, S=06, L=0031 02 01 B5 0D 00 00 02 00 00 00 01 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 E1 04 E2 04 A1 0A
[auto] RF RX << (err= 0), H=01, C=68, S=15, L=0031 03 01 14 0D 00 00 02 00 00 00 01 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 E1 04 E2 04 10 0A
==============================================================================================================
==============================================================================================================
==============================================================================================================
*/

	/*
	karel@lindev:~/Projects/gh/Stupid-Git/rtr-lin/ftdi_1/Debug$ sudo ./ftdi_1
	[sudo] password for karel:
	W f = 128
	R f = 0
	TX a : 1729498653.359 seconds
	TX b : 1729498653.379 seconds
	TX
	01 39 00 0A 00 11 00 82  5F 9E 09 16 67 00 00 5A  |  .9......_...g..Z
	02                                                |  .
	RX a : 1729498653.389 seconds
	RX b : 1729498653.439 seconds
	RX
	01 39 06 04 00 00 00 00  00 44 00                 |  .9.......D.
	TX a : 1729498653.439 seconds
	TX b : 1729498653.439 seconds
	TX
	01 42 00 10 00 00 00 00  FF 00 00 00 00 00 A7 01  |  .B..............
	01 40 30 01 83 EF 02                              |  .@0....
	RX a : 1729498653.449 seconds
	RX b : 1729498653.471 seconds
	RX
	01 34 06 04 00 00 00 00  00 3F 00                 |  .4.......?.
	RX a : 1729498653.481 seconds	RX b : 1729498655.852 seconds
	RX a : 1729498655.862 seconds	RX b : 1729498655.916 seconds
	RX a : 1729498655.926 seconds	RX b : 1729498655.980 seconds

	RX  01 68 06  1F 00   01 01 7B 1D 00 00 26 00 00 00 01 E2 04 E3 04 E2 04 E2 04 E2 04 E2 04 E3 04 E3 04 E3 04 E3 04  50 0A
	RX	01 68 06  1F 00   02 01 7A 1D 00 00 26 00 00 00 01 E2 04 E3 04 E2 04 E2 04 E2 04 E2 04 E3 04 E3 04 E3 04 E3 04  50 0A
	RX 	01 68 06  1F 00   03 00 78 B5 00 00 01 01 B3 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  73 02
	*/
	//==============================================================================================================
	//==============================================================================================================
	//==============================================================================================================
	uint8_t buft1[] = {0x01, 0x39, 0x00, 0x0a, 0x00, 0x11, 0x00, 0x82, 0x5F, 0x9E, 0x09, 0x16, 0x67, 0x00, 0x00, 0x5A, 0x02};
	uint8_t bufr1[] = {0x01, 0x39, 0x06, 0x04,0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x00};
	//###RF_command execute 00
	//R500 command make 30
	//        R500_radio_communication start
	//        R500_radio_communication_once start
	//        R500_send_recv() t=6000
	//rfm_send com close ssp_err=0
	//rfm_send com open ssp_err=0
	//[auto] LED_Set: Msg:10, flag:1005 [BMLED_PWR]
	uint8_t buft2[] = {0x01, 0x42, 0x00, 0x10,0x00,  0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA7, 0x01, 0x01, 0x40, 0x30, 0x01, 0x83, 0xEF, 0x02};
	//        R500_send_recv()  rfm_send  end
//	uint8_t bufr2[] = {0x01, C=34, S=06, L=0004 00 00 00 00 3F 00};
	//$BOGUS$ sync_time = 1^$BOGUS$ sync_time = 1^
//	uint8_t bufr3[] = {0x01, 0x68, 0x06, 31.0,  0x01 01 B4 0D 00 00 02 00 00 00 01 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 E1 04 E2 04 9F 0A};
//	uint8_t bufr4[] = {0x01, 0x68, 0x06, 31.0,  0x02 01 B5 0D 00 00 02 00 00 00 01 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 E1 04 E2 04 A1 0A};
	uint8_t bufr5[] = {0x01, 0x68, 0x15, 31.0,  0x03, 0x01, 0x14, 0x0D, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0xE1, 0x04, 0xE2, 0x04, 0x10, 0x0A};
	//==============================================================================================================
	//==============================================================================================================
	//==============================================================================================================


	int f = 0;
	uint8_t buf[128];

	//================================================================

	memcpy(buf, buft1, sizeof(buft1));
    print_current_time_with_ms("TX a");
    f = ftdi_write_data(ftdi, buf, sizeof(buft1));
    print_current_time_with_ms("TX b");
    PrintHex("TX", buf, sizeof(buft1));
	fflush(stdout);

	readPacket(11);

    //================================================================
	memcpy(buf, buft2, sizeof(buft2));
	print_current_time_with_ms("TX a");
	f = ftdi_write_data(ftdi, buf, sizeof(buft2));
	print_current_time_with_ms("TX b");
    PrintHex("TX", buf, sizeof(buft2));
	fflush(stdout);

	readPacket(11);

	readPacket(38);
	readPacket(38);
	readPacket(38);
	//readPacket(38);

}
int mytest_main(int argc, char **argv);
int mytest_main(int argc, char **argv)
{
	unsigned char buf[1024];
	int f = 0, i;
	int vid = 0x403;
	int pid = 0;
	int baudrate = 115200;
	int interface = INTERFACE_ANY;
	//int do_write = 0;
	//unsigned int pattern = 0xffff;
	int retval = EXIT_FAILURE;


	//=====================================
	vid = 0x0403;
	pid = 0x6001;
	interface = INTERFACE_ANY;
	baudrate = 19200;
	//baudrate = 1200;

    // Init
    if ((ftdi = ftdi_new()) == 0)
    {
        fprintf(stderr, "ftdi_new failed\n");
        return EXIT_FAILURE;
    }


    // Select interface
    ftdi_set_interface(ftdi, interface);
    // Open device
    f = ftdi_usb_open(ftdi, vid, pid);
    if (f < 0)
    {
    	fprintf(stderr, "unable to open ftdi device: %d (%s)\n", f, ftdi_get_error_string(ftdi));
    	exit(-1);
    }

    // Set baudrate
    f = ftdi_set_baudrate(ftdi, baudrate);
    if (f < 0)
    {
    	fprintf(stderr, "unable to set baudrate: %d (%s)\n", f, ftdi_get_error_string(ftdi));
    	exit(-1);
    }


    /* Set line parameters
     *
     * TODO: Make these parameters settable from the command line
     *
     * Parameters are chosen that sending a continuous stream of 0x55
     * should give a square wave
     *
     */
    f = ftdi_set_line_property(ftdi, 8, STOP_BIT_1, NONE);
    if (f < 0)
    {
        fprintf(stderr, "unable to set line parameters: %d (%s)\n", f, ftdi_get_error_string(ftdi));
        exit(-1);
    }

#if 1
    for(i=0; i<1024; i++)
    {
    	//buf[i] = (uint8_t)(i % 256); //pattern;
    	buf[i] = (uint8_t)(0x00);
    }
    //signal(SIGINT, sigintHandler);
    //while (!exitRequested)

    {
        f = ftdi_write_data(ftdi, buf, 10);//128);
        fprintf(stderr, "W f = %d\n", f);
    }
#if 0
    {
        f = ftdi_read_data(ftdi, buf, sizeof(buf));
        fprintf(stderr, "R f = %d\n", f);

        if (f<0)
             usleep(1 * 1000000);
         else if(f> 0 /*&& !do_write*/)
         {
             fprintf(stderr, "read %d bytes\n", f);
             fwrite(buf, f, 1, stdout);
             fflush(stderr);
             fflush(stdout);
         }
     }
#endif
#endif

    //usleep(1 * 1000000);
    //xx();
    FUNC2();
    //usleep(2 * 1000000);
    printf("------------------------------------------------------------------\n");
    {
           f = ftdi_write_data(ftdi, buf, 10);//128);
           fprintf(stderr, "W f = %d\n", f);
       }
    FUNC3();
    //usleep(1 * 1000000);
    //xx();

    signal(SIGINT, SIG_DFL);
    retval =  EXIT_SUCCESS;

    ftdi_usb_close(ftdi);
//do_deinit:
	ftdi_free(ftdi);

    return retval;

}

/*
 *
[auto] RF_full_moni() - Start

##########################[Group1] RF_full_moni start
###RF_command execute 30
od:30 id:536860864 rpt:00 band:00 freq:00 01
rfm_send com close ssp_err=0
rfm_send com open ssp_err=0
[auto] LED_Set: Msg:10, flag:1005 [BMLED_PWR]
==============================================================================================================
==============================================================================================================
[auto] RF TX >> (err= 0), H=01, C=39, S=00, L=0010 11 00 82 5F 9E 09 16 67 00 00 5A 02
[auto] RF RX << (err= 0), H=01, C=39, S=06, L=0004 00 00 00 00 44 00
###RF_command execute 00
R500 command make 30
        R500_radio_communication start
        R500_radio_communication_once start
        R500_send_recv() t=6000
rfm_send com close ssp_err=0
rfm_send com open ssp_err=0
[auto] LED_Set: Msg:10, flag:1005 [BMLED_PWR]
[auto] RF TX >> (err= 0), H=01, C=42, S=00, L=0016 00 00 00 FF 00 00 00 00 00 A5 01 01 40 30 01 83 ED 02
        R500_send_recv()  rfm_send  end
[auto] RF RX << (err= 0), H=01, C=34, S=06, L=0004 00 00 00 00 3F 00
[event] LED_Set: Msg:10, flag:1001 [BMLED_PWR]
[event] SYS_MODE_RUN_WITH_AUTO
[event] ---> @ TICK: g_seconds_until_GSM_try8 = 30
[event] Event Thread Stop (main)
[event] ------------------------------

[event] sleep flags = 0x000FDFAC [    |  a |    | U L|  kb]
$BOGUS$ sync_time = 1^$BOGUS$ sync_time = 1^[auto] RF RX << (err= 0), H=01, C=68, S=06, L=0031 01 01 B8 06 00 00 02 00 00 00 01 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 E1 04 E2 04 9C 0A
[auto] RF RX << (err= 0), H=01, C=68, S=06, L=0031 02 01 B9 06 00 00 02 00 00 00 01 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 E1 04 E2 04 9E 0A
[auto] RF RX << (err= 0), H=01, C=68, S=15, L=0031 03 01 30 06 00 00 02 00 00 00 01 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 E1 04 E2 04 25 0A
==============================================================================================================
==============================================================================================================
==============================================================================================================
[auto] <<NG>>  rfm_recv rtn=F4[RFM_R500_NAK] / err=0000 (313) ------------------------------
R500_radio_communicationxonce rtn 00
        R500_radio_communication end 0
###RF_command execute end 00
### copy_realscan 1 / 1
$BOGUS$ sync_time = 1^##########################[Group1] RF_full_moni end status[0]

[auto] RF_full_moni() - End


[auto] set_download_temp: G_No=1, U_No=1, siri=0x5f820011, (Dat_No=NULL)

debug_print_download_temp START
  group_no = 1
  unit_no  = 1
  serial   = 0x5f820011
  data_no  = 0

debug_print_download_temp END

[auto] di_realscan_Format0 - Start

--->get_regist_scan_unit  adr=16
auto_realscan_New -->  RF_command_execute(30)
###RF_command execute 30
od:30 id:536860864 rpt:00 band:00 freq:00 01
rfm_send com close ssp_err=0
rfm_send com open ssp_e
r=0
[auto] LED_Set: Msg:10, flag:1005 [BMLED_PWR]
==============================================================================================================
==============================================================================================================
==============================================================================================================
[auto] RF TX >> (err= 0), H=01, C=39, S=00, L=0010 11 00 82 5F 9E 09 16 67 00 00 5A 02
[auto] RF RX << (err= 0), H=01, C=39, S=06, L=0004 00 00 00 00 44 00
###RF_command execute 00
R500 command make 30
        R500_radio_communication start
        R500_radio_communication_once start
        R500_send_recv() t=6000
rfm_send com close ssp_err=0
rfm_send com open ssp_err=0
[auto] LED_Set: Msg:10, flag:1005 [BMLED_PWR]
[auto] RF TX >> (err= 0), H=01, C=42, S=00, L=0016 00 00 00 FF 00 00 00 00 00 A6 01 00 40 30 01 83 ED 02
        R500_send_recv()  rfm_send  end
[auto] RF RX << (err= 0), H=01, C=34, S=06, L=0004 00 00 00 00 3F 00
$BOGUS$ sync_time = 1^$BOGUS$ sync_time = 1^=====>> LOG[RF :[auto] Channel Busy](19)
[auto] RF RX << (err= 0), H=01, C=34, S=12, L=0004 00 00 00 00 4B 00
==============================================================================================================
==============================================================================================================
==============================================================================================================
[auto] <<NG>>  rfm_recv rtn=F5[RFM_R500_CH_BUSY] / err=0000 (185) ------------------------------
R500_radio_communication_once rtn F5
$BOGUS$ sync_time = 1F$BOGUS$ sync_time = 1^    R500_radio_communication end 245
###RF_command execute end F5
    DL_start_utc  1729497551
    Attach.GMT    1727913602
$BOGUS$ sync_time = 1^
[auto] do_realscan_Format0 - End


debug_print_download_temp START
  group_no = 1
  unit_no  = 1
  serial   = 0x5f820011
  data_no  = 0

debug_print_download_temp END

debug_print_download START
  group_no = 1
  unit_no  = 1
  serial   = 0x5f820011
  data_no  = 0

debug_print_download END
[auto] ----- nsc_all -----

[auto] ----- nsc_debug_dump_auto -----
FDF[0] G=1, U=1, Flag = 0x0
Download[0] G=1, U=1, Serial=5F820011, Data_NO = 0
Warning[0] G=1, U=1
W_config[0] G=0, U=0, Now=0000, B4=0000, OO=0000,
W_config[1] G=1, U=1, Now=0000, B4=0000, OO=0000,
[auto] ----- nsc_debug_dump_auto END -----
Warning_Info_Clear()

[auto] RF_full_moni() - Start

##########################[Group1] RF_full_moni start
###RF_command execute 30
od:30 id:536860864 rpt:00 band:00 freq:00 01
rfm_send com close ssp_err=0
rfm_send com open ssp_err=0
[auto] LED_Set: Msg:10, flag:1005 [BMLED_PWR]
==============================================================================================================
==============================================================================================================
==============================================================================================================
[auto] RF TX >> (err= 0), H=01, C=39, S=00, L=0010 11 00 82 5F 9E 09 16 67 00 00 5A 02
[auto] RF RX << (err= 0), H=01, C=39, S=06, L=0004 00 00 00 00 44 00
###RF_command execute 00
R500 command make 30
        R500_radio_communication start
        R500_radio_communication_once start
        R500_send_recv() t=6000
rfm_send com close ssp_err=0
rfm_send com open ssp_err=0
[auto] LED_Set: Msg:10, flag:1005 [BMLED_PWR]
[auto] RF TX >> (err= 0), H=01, C=42, S=00, L=0016 00 00 00 FF 00 00 00 00 00 A7 01 01 40 30 01 83 EF 02
        R500_send_recv()  rfm_send  end
[auto] RF RX << (err= 0), H=01, C=34, S=06, L=0004 00 00 00 00 3F 00
$BOGUS$ sync_time = 1^$BOGUS$ sync_time = 1^[auto] RF RX << (err= 0), H=01, C=68, S=06, L=0031 01 01 B4 0D 00 00 02 00 00 00 01 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 E1 04 E2 04 9F 0A
[auto] RF RX << (err= 0), H=01, C=68, S=06, L=0031 02 01 B5 0D 00 00 02 00 00 00 01 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 E1 04 E2 04 A1 0A
[auto] RF RX << (err= 0), H=01, C=68, S=15, L=0031 03 01 14 0D 00 00 02 00 00 00 01 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 00 F0 E1 04 E2 04 10 0A
==============================================================================================================
==============================================================================================================
==============================================================================================================
[auto] <<NG>>  rfm_recv rtn=F4[RFM_R500_NAK] / err=0000 (313) ------------------------------
R500_radio_communication_once rtn 00
        R500_radio_communication end 0
###RF_command execute end 00
### copy_realscan 1 / 1
$BOGUS$ sync_time = 1^##########################[Group1] RF_full_moni end status[0]

[auto] RF_full_moni() - End


 *
 */
