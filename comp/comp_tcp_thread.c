
#include "_r500_config.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include <stdbool.h>

#include "random_stuff.h"
#include "file_structs.h"

#include "r500_defs.h"

//#define PORT 9990
#define PORT 62500 //karel
#define SIZE 1024




//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------


static void print_T2(uint8_t *buf, int buflen);
static void print_T2(uint8_t *buf, int buflen)
{
	uint16_t t2_len;

	PrintHex("RX", buf, buflen);
	// 54 32  06 00  52 55 49 4E 46 3A  BE 01              |  T2[..][RUINF:] [..]

	if(buf[0] != 'T')
		return;
	if(buflen>4)
	{
		t2_len = *(uint16_t*)&buf[2];
		printf("T2 len = %d\n", t2_len);
	}
}



static int creat_socket()
{
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    printf("creat_socket: port = %d\n", PORT); //karel

    int bindResult = bind(server_socket, (struct sockaddr *)&addr, sizeof(addr));
    printf("creat_socket: bindResult = %d\n", bindResult); //karel

    int listenResult = listen(server_socket, 5);
    printf("creat_socket: listenResult = %d\n", listenResult); //karel


    return server_socket;
}

static int wait_client(int server_socket)
{
    struct sockaddr_in cliaddr;
    socklen_t addrlen = sizeof(cliaddr); //karel int -> socklen_t
    printf("waiting connect .. \n");
    int client_socket = accept(server_socket, (struct sockaddr *)&cliaddr, &addrlen);
    printf("accept success %s\n", inet_ntoa(cliaddr.sin_addr));

    return client_socket;
}


#define CMD_LOGIN       "login"
#define CMD_OK          "OK"

#define TBUF_SIZE       (4096+128*4)
static char      tcp_rxb[TBUF_SIZE];
static uint32_t     tcp_receive_length;

/**
 * UDP後の、login passwordの照合処理
 * @return 0 常に0
 * @bug 返値は常に0(false)
 */
int check_login(int client_socket)
{
    //bool ret;
    //int32_t status;
    int retry;
    uint32_t i;
    uint32_t Size;
    //uint32_t rtn = 0;
    uint32_t rtn = 1;       // Pass NG  // 2022.10.31


    Printf(" tcp send [login] \n");
    // send 'login'
    if( write(client_socket, CMD_LOGIN, 5) < 0)
   	{
    	rtn = -1; // failure
    	goto L_Exit;
   	}

    retry = 0;
	while (retry <= 3)
    {
        // NEED to use poll NEED a time out
        //status = tcp_receive(WAIT_1MIN);  6000は、長すぎる
        //status = tcp_receive(100);
        tcp_receive_length = read(client_socket, tcp_rxb, sizeof(tcp_rxb));
        if (tcp_receive_length < 0)
        {
        	printf("Got -1 for bufSize (Disconnected?) - Exiting\n");
        	rtn = -1; // failure
        	goto L_Exit;
            //break;
        }

        if (tcp_receive_length == 0)
        {
        	continue;
            //break;
        }
        printf("check_login: tcp_receive_length: %d\n", tcp_receive_length);

        tcp_rxb[tcp_receive_length] = '\0';
        //printf("From client: %s\n", tcp_rxb);
        for(i = 0; i < tcp_receive_length; i++){
            Printf("%02X ", tcp_rxb[i]);
        }

        if(1)
        {
            // NetPass照合

            Size = tcp_receive_length - 1;
            if(Size >=6 && Size <=26)
	        {
            	strcpy(my_config.network.NetPass, "111111"); //Debug

                if(Memcmp(my_config.network.NetPass, tcp_rxb, (int)strlen(my_config.network.NetPass) ,(int)Size) == 0){
                    Printf("\nPassCode OK !!\n");
                    // send 'OK'
                    if( write(client_socket, CMD_OK, 2) < 0)
                   	{
                    	rtn = -1; // failure
                    	goto L_Exit;
                   	}
                    rtn = 0;        // Pass OK  // 2022.10.31
                    break;
                }
                else
                {
                    Printf("\nPassCode Error !!\n");
                    Printf(" tcp send [login] \n");
                    if( write(client_socket, CMD_LOGIN, 5) < 0)
                   	{
                    	rtn = -1; // failure
                    	goto L_Exit;
                   	}
                    rtn = 1;        // Pass NG
                    break;
                }
            }

        }
        retry++;
    }
L_Exit:;
	printf("check_login: exit rtn = %d\n", rtn);
    return (rtn);
}

static void *tcp_thread(void *socket_desc);
static void *tcp_thread(void *socket_desc)
{
	int rtn = 0;
    int client_socket = *(int *)socket_desc;
    //char buf[SIZE];

    int log_in = 0;

    Printf("--------------------------------------\n");
    Printf("TCP Connect !!!! \n");
    Printf("client_socket = %d\n", client_socket);
    // login send and password check
    if(log_in == 0)
    {
        rtn = check_login(client_socket);
        if(rtn == 0){
            log_in = 1;
            Printf("login end \n");
        }
        else
        {
            //PutLog(LOG_LAN, "TCP Login Error");
            //DebugLog( LOG_DBG, "TCP Login Error");
            Printf("login error \n");
            goto L_Exit_Thread;
        }
    }

    while (1)
    {
    	//ret = tcp_cmd_recv();
        tcp_receive_length = read(client_socket, tcp_rxb, sizeof(tcp_rxb));
        if (tcp_receive_length == 0)
        {
        	continue;
            //break;
        }
        if (tcp_receive_length == -1)
        {
        	printf("Got -1 for tcp_receive_length (Disconnected?) - Exiting\n");
            break;
        }

        printf("tcp_thread: tcp_receive_length: %d\n", tcp_receive_length);

        print_T2((uint8_t*)tcp_rxb, tcp_receive_length);

        //=================================
        //=================================
        {
        	static def_com_u SCOM;
            uint32_t len;
            uint32_t t_len;
            uint16_t cmd_len;
            uint8_t *poi;
            uint32_t j;

        	len = tcp_receive_length;
        	t_len = len;
        	if(tcp_rxb[0] != 'T'){
        		goto EXIT;
        	}
        	if(len < 4){
        		goto EXIT;
        	}

        	SCOM.rxbuf.header = SCOM.rxbuf.command = tcp_rxb[0];
        	SCOM.rxbuf.subcommand = tcp_rxb[1];
        	//poi = (uint8_t *)&SCOM.rxbuf.command;
        	// データ部以外が6バイト
        	cmd_len = (uint16_t)((uint16_t)tcp_rxb[2] + (uint16_t)tcp_rxb[3] * 256);
        	SCOM.rxbuf.length = cmd_len;

        	Printf("TCP recv end  %d (%d)\n", t_len,cmd_len);

        	poi = (uint8_t *)&SCOM.rxbuf.data;

        	if(t_len >= (uint32_t)(cmd_len+6))
        	{
        		Printf("TCP recv end 1 %d (%d)\n", t_len,cmd_len);
        		for(j = 0; j < t_len; j++ ){
        			*poi++ = tcp_rxb[j+4];
        		}
        		//ret = 0;

        		int DoCmd(uint8_t *REQbuf, uint8_t *RSPbuf, uint32_t REQlen,  uint32_t *pRSPlen);
        	    DoCmd(&SCOM.rxbuf.command, &SCOM.txbuf.header, t_len,  &t_len);
                Printf("TCP recv end 2 %d (%d)\n", t_len,cmd_len);

                ssize_t wsize = write(client_socket, &SCOM.txbuf.header, t_len);
                Printf("TCP write wsize = %ld\n", wsize);
        	}

        	EXIT:;
        }
        //=================================
        //=================================

    }

    L_Exit_Thread: ;
    close(client_socket);

	printf("tcp_thread: Exited\n");
    return 0;
}


static void *tcp_connection_thread(void *socket_desc);
static void *tcp_connection_thread(void *socket_desc)
{
    int server_socket = creat_socket();

    while (1)
    {
        int client_socket = wait_client(server_socket);
        pthread_t id;
        pthread_create(&id, NULL, (void *)tcp_thread, (void *)&client_socket);

        pthread_detach(id);
    }

    return 0;
}


int tcp_thread_start(void);
int tcp_thread_start(void)
{
    pthread_t id;
    pthread_create(&id, NULL, (void *)tcp_connection_thread, (void *)NULL);

    //pthread_detach(id); //?

    return 0;
}




