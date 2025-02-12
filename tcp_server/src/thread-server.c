

//
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

//#define PORT 9990
#define PORT 62500 //karel
#define SIZE 1024



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



#if 0
static uint32_t CFReadInfo(void)
{
    char str[5];


    // 親機名称
    StatusPrintf_S(sizeof(my_config.device.Name),  "NAME", "%s", my_config.device.Name);
    // 本体シリアル番号
    //fact_config.SerialNumber = 0x5f58ffff;
    StatusPrintf("SER", "%.8lX", fact_config.SerialNumber);
     // 本機の説明
    StatusPrintf_S(sizeof(my_config.device.Description),  "DESC", "%s", my_config.device.Description);
    // 本体ファームウェアバージョン
    //StatusPrintf( "FWV", "%s %s", VERSION_FW, __DATE__ );  //debug
    StatusPrintf( "FWV", "%s", VERSION_FW);     // release
    // 無線モジュール ファームウェアバージョン
    StatusPrintf( "RFV", "%.4X", regf_rfm_version_number & 0x0000ffff);

    // debug
    //StatusPrintf( "NETV", "%s", VERSION_FW);

    // 温度単位
    StatusPrintf_v2s( "UNITS", my_config.device.TempUnits, sizeof(my_config.device.TempUnits), "%lu");

    // 販売者
//    StatusPrintf("VENDER", "%d", fact_config.Vender);
    /*
    memcpy((char *)&(StsArea[CmdStatusSize]), "NAME=00", 7);
    sp = CmdStatusSize + 5;                                     // データ数書き込み位置ラッチ
    CmdStatusSize += 7;



    memcpy(&StsArea[CmdStatusSize], my_config.device.Name, 26);
    StsArea[CmdStatusSize + 26] = 0x00;                         // 最大２６バイトとするため、２６バイト目をＮＵＬＬにする

    word_data_a = strlen((char *)&StsArea[CmdStatusSize]);           // ＮＵＬＬコードまでの長さ
    if(word_data_a > 26) word_data_a = 26;
    StsArea[sp] = (uint8_t)a.byte_lo;
    StsArea[sp + 1] = a.byte_hi;
    CmdStatusSize += word_data_a;

    // 本機の説明
    memcpy(&StsArea[CmdStatusSize], "DESC=00", 7);
    sp = CmdStatusSize + 5;                                     // データ数書き込み位置ラッチ
    CmdStatusSize += 7;

    memcpy(&StsArea[CmdStatusSize], my_config.device.Description, 64);
    StsArea[CmdStatusSize + 64] = 0x00;                         // 最大６４バイトとするため、６４バイト目をＮＵＬＬにする

    word_data_a = strlen((char *)&StsArea[CmdStatusSize]);           // ＮＵＬＬコードまでの長さ
    if(word_data_a > 64) word_data_a = 64;
    StsArea[sp] = a.byte_lo;
    StsArea[sp + 1] = a.byte_hi;
    CmdStatusSize += word_data_a;
    */

    // シリアル通信速度
    //StatusPrintf("BAUD", "%u", my_settings.scom_baudrate);

    // パスコード
    //StatusPrintfB("PASC", (char *)&my_config.ble.passcode, 4);

    // ＢＬＥデバイスアドレス
    //StatusPrintfB("DADR", (char *)&psoc.device_address, 8);

    // 登録コード
    //StatusPrintfB("REGC", (char *)&my_config.device.registration_code, 4);



    switch(fact_config.SerialNumber & 0xf0000000){
        case 0x30000000:    // US
            StatusPrintf( "WCER", "%s", "1");
            break;
        case 0x40000000:    // EU
            StatusPrintf( "WCER", "%s", "2");
            break;
        case 0x50000000:    // 日本
            StatusPrintf( "WCER", "%s", "0");
            break;
        case 0xE0000000:    // 日本(ESPEC)
            StatusPrintf( "WCER", "%s", "0");
            break;
        default:            //
            StatusPrintf( "WCER", "%s", "0");
            break;

    }



    // 無線モジュール シリアル番号
    StatusPrintf("RFS", "%.8lX", regf_rfm_serial_number);



    // ＢＬＥ ファームウェアバージョン
    //taskENTER_CRITICAL();
    memcpy(str, psoc.revision, 4);
    str[4] = 0x00;
    //taskEXIT_CRITICAL();
    StatusPrintf( "BLV", "%s", &str);
    //rfm_reset();


    return(ERR(CMD, NOERROR));
}
#endif // 0

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

static void *socket_handler(void *socket_desc)
{

    int client_socket = *(int *)socket_desc;

    char buf[SIZE];

    write(client_socket, CMD_LOGIN, 5);

    while (1)
    {
        int bufSize = read(client_socket, buf, SIZE - 1);
        printf("bufsize: %d\n", bufSize);
        if (bufSize == 0)
        {
        	continue;
            //break;
        }
        if (bufSize == -1)
        {
        	printf("Got -1 for bufSize (Disconnected?) - Exiting\n");
            break;
        }
        buf[bufSize] = '\0';
        printf("From client: %s\n", buf);
        print_T2((uint8_t*)buf, bufSize);

        write(client_socket, "OK", 2);
        //write(client_socket, buf, bufSize);

        if (strncmp(buf, "end", 3) == 0)
        {
        	printf("Got End - Exiting\n");
            break;
        }
    }
    close(client_socket);

	printf("Exited\n");
    return 0;
}

int thread_server(void);
int thread_server(void)
{
    int server_socket = creat_socket();

    while (1)
    {
        int client_socket = wait_client(server_socket);
        pthread_t id;
        pthread_create(&id, NULL, (void *)socket_handler, (void *)&client_socket);

        pthread_detach(id);
    }

    return 0;
}

