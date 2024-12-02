
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>



static char* banner = "\n\
  _              _      _\n\
 | |___  __ _ __| |  __| |__ _ ___ _ __  ___ _ _\n\
 | / _ \\/ _` / _` | / _` / _` / -_) '  \\/ _ \\ ' \\\n\
 |_\\___/\\__,_\\__,_| \\__,_\\__,_\\___|_|_|_\\___/_||_|\n\
             ___ ___ _ ___ _____ _ _\n\
            (_-</ -_) '_\\ V / -_) '_|\n\
            /__/\\___|_|  \\_/\\___|_|     @crackcat\n\n\
";


/// UDP 応答の構造
struct sUDP{
    //0
    char    Code[2];
    char    Command[1];
    char    Status[1];
    char    Reserved1[1];
    char    DHCP[1];
    char    MAC[6];
    char    IP[4];
    char    MASK[4];
    char    GW[4];
    char    DNS1[4];
    char    DNS2[4];
    //32
    char    ComPort[2];
    char    Serial[8];
    char    BaseName[32];
    char    Description[64];
    //138
    char    Reserved2[54];
    //192
    char    PW[64];
 } UDP;
 uint8_t udp_rxb[256];



 /**
  * @brief   UDP 応答のセット
  * @bug UDP.Serialにセットする値は8Byteを超える
  */
 void uint32to8(uint32_t val, char *RET)
 {
     char	dmy[4];

     dmy[3] = (uint8_t)(val & 0x000000ff);
     dmy[2] = (uint8_t)((val >> 8 ) & 0x000000ff);
     dmy[1] = (uint8_t)((val >> 16 ) & 0x000000ff);
     dmy[0] = (uint8_t)((val >> 24 ) & 0x000000ff);

     for(int i = 0; i < 4; i++){
         RET[i] = dmy[i];
     }
 }

 void Udp_Resp_Set(void)
 {
  //   uint8_t *dst;     //2019.Dec.26 コメントアウト
    // uint32_t val;

     memset(&UDP, 0x00, sizeof(UDP));
     UDP.Code[0]     = 0x50;
     UDP.Code[1]     = 0x01;
     UDP.Command[0]  = 0x01;
     UDP.Status[0]   = 0x00;
     UDP.DHCP[0] = 1;//my_config.network.DhcpEnable[0];
     UDP.IP[0] = 10; UDP.IP[1] = 2; UDP.IP[2] = 254; UDP.IP[3] = 65;
     UDP.MASK[0] = 255; UDP.MASK[1] = 255; UDP.MASK[2] = 0; UDP.MASK[3] = 0;
     UDP.GW[0] = 10; UDP.GW[1] = 2; UDP.GW[2] = 0; UDP.GW[3] = 1;
     UDP.DNS1[0] = 10; UDP.DNS1[1] = 10; UDP.DNS1[2] = 10; UDP.DNS1[3] = 10;
     UDP.DNS2[0] = 10; UDP.DNS2[1] = 10; UDP.DNS2[2] = 10; UDP.DNS2[3] = 10;

     //UDP.ComPort[0] = 0x24;
     //UDP.ComPort[1] = 0xf4;
     uint16_t word_data = 62500;
     *(uint16_t *)&UDP.ComPort[0] = word_data;
     //UDP.ComPort[0] = my_config.network.SocketPort[0];
     //UDP.ComPort[1] = my_config.network.SocketPort[1];

     //fact_config.SerialNumber = 0x5f58ffff;// 0x5f9cffff; //debug
     //sprintf( (char *)UDP.Serial , "%.8lX", fact_config.SerialNumber);       ///< @note 8Byte超える @bug 8Byte超える
     sprintf( (char *)UDP.Serial , "58581234");
     sprintf( (char *)UDP.BaseName , "RTR500BW_Fake");
     sprintf( (char *)UDP.Description , "Expllll");
     //memcpy(UDP.BaseName,my_config.device.Name, 32);
     //memcpy(UDP.Description,my_config.device.Description,64);
     // PW は送信しない
     // MAC test data

     /*
     val = net_address.eth.mac1;
     UDP.MAC[1] = (UCHAR)(val & 0x000000ff);
     UDP.MAC[0] = (UCHAR)((val >> 8 ) & 0x000000ff);

     val = net_address.eth.mac2;
     UDP.MAC[5] = (UCHAR)(val & 0x000000ff);
     UDP.MAC[4] = (UCHAR)((val >> 8 ) & 0x000000ff);
     UDP.MAC[3] = (UCHAR)((val >> 16 ) & 0x000000ff);
     UDP.MAC[2] = (UCHAR)((val >> 24 ) & 0x000000ff);
     */
     /**/
     UDP.MAC[0] = 0x2e;
     UDP.MAC[1] = 0x09;
     UDP.MAC[2] = 0x0a;
     UDP.MAC[3] = 0x00;
     UDP.MAC[4] = 0x76;
     UDP.MAC[5] = 0x42; //0xc7;
     /**/

 }

//#define TCP_PORT 62500
#define R_PORT 62501
#define T_PORT 62502

static void *udp_thread(void *params);
static void *udp_thread(void *params)
{
	uint16_t r_port = R_PORT;   //r_port = (UINT)my_config.network.UdpRxPort;
	uint16_t t_port = T_PORT;   //t_port = (UINT)my_config.network.UdpTxPort;

    //Printf("UDP start %d\r\n", r_port);

    for(;;)
    {
    	printf(banner);

    	/* Variables for socket communication */
    	struct sockaddr_in servaddr; // server address structure
    	struct sockaddr_in clientaddr; // client address structure
    	unsigned int clientAddrLen;
    	int recvMsgSize;

    	/* Create a UDP socket */
    	int sockfd;
    	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) {
    		perror("Socket creation failed");
            goto EXIT_END;
    	}

    	/* Zero out and prepare the address structures */
    	memset(&servaddr, 0, sizeof(servaddr));
    	memset(&clientaddr, 0, sizeof(clientaddr));

    	servaddr.sin_family = AF_INET;
    	servaddr.sin_addr.s_addr = INADDR_ANY; // no need for htonl becuz its 0 anyway (0.0.0.0)
    	servaddr.sin_port = htons(r_port);
    	clientAddrLen = sizeof(clientaddr);

    	/* Bind the socket to 0.0.0.0 (all interfaces) */
    	if ( bind(sockfd, (const struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ) {
    		perror("Binding to socket failed");
            goto EXIT_END;
    	}

        while (1)       // 受信
        {
            //Printf("    Network Mutex get wait\r\n");
            if(1) //if(tx_mutex_get(&mutex_network_init,  WAIT_100MSEC) == TX_SUCCESS)
            {
                //tx_mutex_put(&mutex_network_init);


        		/* Receive incoming values into a load stats structure */
        		/* For this app it isn't even necessary, but I like it */
        		//if ( (recvMsgSize = recvfrom(sockfd, pStats, sizeof(*pStats), 0, (struct sockaddr *) &clientaddr, &clientAddrLen)) < 0) {
        		if ( (recvMsgSize = recvfrom(sockfd, udp_rxb, sizeof(udp_rxb), 0, (struct sockaddr *) &clientaddr, &clientAddrLen)) < 0) {
        			perror("rcvfrom() failed");
        			goto EXIT_END; //exit(EXIT_FAILURE);
        		}

        		/* Some pretty debug information */
        		printf("[+] Handling incoming request (%s)\n", inet_ntoa(clientaddr.sin_addr));
        		printf("\tReceived bytes: %d\n", recvMsgSize);

        		/*
        		uint32_t MYADDR;
        		MYADDR = 10;
        		MYADDR = (MYADDR << 8) + 2;
        		MYADDR = (MYADDR << 8) + 73;
        		MYADDR = (MYADDR << 8) + 31;
        		struct in_addr myin_addr;
        		myin_addr.s_addr = htonl(MYADDR);

        		printf("  IP = %s", inet_ntoa(clientaddr.sin_addr));
        		printf("MYIP = %s", inet_ntoa(myin_addr));
        	//printf("MYIP = %s", inet_ntoa(MYADDR));

        		//if(clientaddr.sin_addr.s_addr != myin_addr.s_addr)
        		//{
        		//	continue;
        		//}
        		*/


                if((udp_rxb[0] == 0x50) && (udp_rxb[1] == 0x01) && (udp_rxb[2] == 0x01)){
                    printf("GOT REQ\n");

                    Udp_Resp_Set();

                    clientaddr.sin_port = htons(t_port); //(uint16_t *)&my_config.network.UdpTxPort[0]

            		/* Send back the updated structure */
            		int sent;
            		//if ( (sent = sendto(sockfd, (struct rload_stat *)pStats, sizeof(*pStats), 0, (struct sockaddr*)&clientaddr, sizeof(clientaddr))) < 0) {
            	    //}
            		if ( (sent = sendto(sockfd, (struct sUDP*)&UDP, 256, 0, (struct sockaddr*)&clientaddr, sizeof(clientaddr))) < 0) {
            			perror("sento() failed");
            			goto EXIT_END; //exit(EXIT_FAILURE);
            		}

            		/* Even more output */
            		printf("\tSent bytes: %d\n", sent);
                }
            }
            //Printf("\r\nudp recive1 %d %ld port=%ld adr=%ld  size=%d \r\n", status, len, port,addr,size);
            //tx_thread_sleep (500);   // sleep 5s

            sleep(2);   // sleep 2s
        }
// ここには、来ない
        //nx_udp_socket_unbind(&udp_socket);
EXIT_END:

        sleep(10);     // sleep 10s
    }

	return NULL;
}

int udp_thread_start(void);
int udp_thread_start(void)
{
    pthread_t id;
    pthread_create(&id, NULL, (void *)udp_thread, (void *)NULL);

    //pthread_detach(id); //?

    return 0;
}
