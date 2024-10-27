
/**
 *	@file	tcp_thread_entry.h
 
 * 	@note	2019.Dec.26 ビルドワーニング潰し完了
 */

#define _TCP_THREAD_ENTRY_C_


//#include "nx_api.h"

#include "General.h"
#include "Globals.h"
#include "Log.h"

#include "tcp_thread_entry.h"
//#include "network_thread.h"
//#include "system_thread.h"
#include "cmd_thread_entry.h"

//extern TX_THREAD cmd_thread;


#define SERVER_PORT       (62500)
#define LISTEN_QUEUE_SIZE (1)
#define QUEUE_SIZE      (20)
#define MESSAGE_SIZE    (1)
#define STACK_SIZE        (0x1000)
#define THREAD_PRIORITY   (2)
#define THREAD_TIME_SLICE (1)
#define SERVER_TIMEOUT    (10 * NX_IP_PERIODIC_RATE)
//#define TBUF_SIZE       (1024+128)
#define TBUF_SIZE       (4096+128*4)              // sakaguchi 2021.06.16

static bool          is_open = false;
//static uint8_t       current_clients = 0x00;      //2019.Dec.26 コメントアウト
//static NX_TCP_SOCKET tcp_socket;      //2019.Dec.26 コメントアウト
static NX_TCP_SOCKET socket_table;
static TX_QUEUE      queue_table;
static uint8_t       queue_memory_table[QUEUE_SIZE];
static TX_THREAD     connection_thread_table;
static TX_THREAD     disconnection_thread_table;
static uint8_t       connection_thread_stack_table[STACK_SIZE];
static uint8_t       disconnection_thread_stack_table[STACK_SIZE];
//static void (*p_new_connection_callback)(uint8_t client_bit) = NULL;      //2019.Dec.26 コメントアウト
//static void (*p_disconnection_callback)(uint8_t client_bit)  = NULL;      //2019.Dec.26 コメントアウト

//static def_com SCOM;                   ///<  Socket
static def_com_u SCOM;                   ///<  Socket       // サイズ拡張   // sakaguchi 2021.04.21

static uint32_t  rx_ptr;
static char      tcp_rxb[TBUF_SIZE];
static uint32_t     tcp_receive_length;

static uint8_t  tcp_no_data = 0;        // 受信データ無しカウント（TCP通信切断用）// 2022.10.31

/***********************************************************************************************************************
 * Private function prototypes
 **********************************************************************************************************************/

bool tcp_open(void);
bool tcp_close (void);
bool tcp_send (char const * const p_data, uint32_t size, ULONG wait_option);
int32_t tcp_receive (ULONG wait_option);

static void server_disconnect(NX_TCP_SOCKET * p_socket);
static void tcp_receive_notify(NX_TCP_SOCKET * p_socket);
static void listen_callback(NX_TCP_SOCKET * p_socket, UINT port);
static void connection_thread_entry(void);
static void disconnection_thread_entry(void);
bool tcp_receive2 (char * const p_data, uint32_t size, ULONG wait_option);


uint32_t check_login(void);
int32_t tcp_cmd_recv(void);


#define CMD_LOGIN       "login"
#define CMD_OK          "OK"


/**
 * @brief Tcp Thread entry function
 *
 * @note  login checkは3回実施し、不一致の場合は、コネクションを切断する
 * @note  コマンドシーケンスでは、応答を返送後、３分以内に次のコマンドを受信出来なかった場合は、コネクションを強制的に切断する
 * @note  アプリケーションの最大送信数は、約1024byte
 */
void tcp_thread_entry(void)
{
    bool flag;
    int32_t ret;
    UINT status;
    uint32_t  actual_events;
 //   uint32_t  actual_events2;
    static CmdQueue_t cmd_msg;      ///< キューでcmd_threadに送るメッセージ
    int log_in = 0;
    static int32_t txLen;      //CmdStatusSizeの代わりに  cmd_threadから受け取る

    tx_event_flags_get (&g_tcp_event_flags, 0xffffffff,TX_OR_CLEAR, &actual_events,TX_NO_WAIT);
    
    flag = tcp_open();
    Printf("start tcp receive !! %d\r\n", flag);
    if(flag == true){
        tx_event_flags_set (&g_tcp_event_flags, FLG_TCP_INIT_SUCCESS, TX_OR);
    }

    while (true)
    {
//ReStart:
ReStart:            // 2022.10.31 復活
        rx_ptr = 0;  
        // 接続待ち
        status = tx_event_flags_get (&g_tcp_event_flags, FLG_TCP_CONNECT,TX_OR_CLEAR, &actual_events,TX_WAIT_FOREVER);
    
        Printf("TCP Connect !!!! \r\n");
        // login send and password check
        if(log_in == 0)
        {
            if(check_login() == 0){
                log_in = 1;
                Printf("login end \r\n");
            }
            else
            {
                PutLog(LOG_LAN, "TCP Login Error");
                DebugLog( LOG_DBG, "TCP Login Error");
                Printf("login error \r\n");
                goto Recv_DC;
            }
        }

// 接続が切れたら再度login

        Printf("[Login OK !!!  Start TCP ]\r\n");
        PutLog(LOG_LAN, "TCP Login Success");    
        DebugLog( LOG_DBG, "TCP Login Success");

        rx_ptr = 0;
        while (1)
        {
Recv_Start:
            ret = tcp_cmd_recv();
            if(ret == 100){
                goto Recv_DC;
            }
            else if ( ret != 0){
                goto Recv_Next;
            }
            //Printf("TCP Command Start!\r\n");
            //コマンドスレッド実行中は待つ 2020.Jul.15
            while((TX_SUSPENDED != cmd_thread.tx_thread_state) /*&& (TX_EVENT_FLAG != cmd_thread.tx_thread_state)*/){
                tx_thread_sleep (1);
            }

            txLen = (int32_t)SCOM.rxbuf.length;
            cmd_msg.CmdRoute = CMD_TCP;            //コマンド キュー  コマンド実行要求元
            cmd_msg.pT2Command = &SCOM.rxbuf.command;    //コマンド処理する受信データフレームの先頭ポインタ
            cmd_msg.pT2Status = &SCOM.txbuf.header;    //コマンド処理された応答データフレームの先頭ポインタ
            cmd_msg.pStatusSize = (int32_t *)&txLen;              //コマンド処理された応答データフレームサイズ

            Printf("tcp length %ld \r\n", txLen);

            tx_queue_send(&cmd_queue, &cmd_msg, TX_WAIT_FOREVER);
            tx_event_flags_set (&g_command_event_flags, FLG_CMD_EXEC,TX_OR);

            //コマンドスレッド起床
            status = tx_thread_resume(&cmd_thread);

            //コマンド処理完了待ち
            status = tx_event_flags_get (&g_command_event_flags, FLG_CMD_END ,TX_OR_CLEAR, &actual_events, TX_WAIT_FOREVER);
 
            Printf("TCP Comannd Send Resp (%ld)\r\n", txLen);


            //status = _ux_device_class_cdc_acm_write(g_cdc, StsArea, CmdStatusSize, &actual_length);
//            tcp_send((char *)StsArea, (uint32_t)CmdStatusSize, TX_WAIT_FOREVER);
//            tcp_send((char *)StsArea, (uint32_t)txLen, TX_WAIT_FOREVER);
            rx_ptr = 0;                 // 今までは、tcp_cmd_recv()で行っていた。
            tcp_send((char *)&SCOM.txbuf.header, (uint32_t)txLen, TX_WAIT_FOREVER);
            //rx_ptr = 0;                 // 今までは、tcp_cmd_recv()で行っていた。

            Reboot_check();        //REBOOT要求のチェック 2020.Jun.10 各タスクに入っていたチェック部を関数にして分離(とりあえずcmd_thread_entry.c)

            status = tx_event_flags_get (&g_tcp_event_flags, FLG_TCP_DISCONNECT, TX_OR, &actual_events, TX_NO_WAIT);
            if(status !=  TX_NO_EVENTS){
                if((actual_events & FLG_TCP_DISCONNECT)){
                    goto Recv_DC;
                }
            }
Recv_Next:
	        tx_thread_sleep (1);
        	goto Recv_Start;
Recv_DC:

	 //       tcp_close();
	 //       goto Restart;
	
	        //nx_tcp_server_socket_unlisten(&g_ip0, SERVER_PORT);
	        tx_event_flags_get (&g_tcp_event_flags, 0xffffffff,TX_OR_CLEAR, &actual_events,TX_NO_WAIT);
	        //tx_thread_sleep (5);
	        //nx_tcp_server_socket_listen(&g_ip0, SERVER_PORT, &(socket_table), LISTEN_QUEUE_SIZE, listen_callback);     
	        // 接続待ち
	        //status = tx_event_flags_get (&g_tcp_event_flags, FLG_TCP_CONNECT,TX_OR_CLEAR, &actual_events,TX_WAIT_FOREVER);
	        //status = nx_tcp_socket_receive_notify(&socket_table, tcp_receive_notify);
	        //if (NX_SUCCESS != status)
	        //{
	        //    Printf("tcp socket receive notify\r\n");
	        //}
	        log_in = 0;
	        Printf("    TCP Reconnect !!\r\n");
// 2022.10.31 ↓
            // Disconnect the client
            nx_tcp_socket_disconnect(&socket_table, SERVER_TIMEOUT);

            // Unaccept this socket 
            nx_tcp_server_socket_unaccept(&socket_table);

            // Setup server socket for listening with this socket again. 
            nx_tcp_server_socket_relisten(&g_ip0, *(uint16_t *)&my_config.network.SocketPort[0], &socket_table);
// 2022.10.31 ↑
	        break;  //goto ReStart;
	
	    }
    }
}


/**
 * UDP後の、login passwordの照合処理
 * @return 0 常に0
 * @bug 返値は常に0(false)
 */
uint32_t check_login(void)
{
    bool ret;
    int32_t status;
    UINT retry;
    ULONG   actual_events;
    uint32_t i;
    uint32_t Size;
    //uint32_t rtn = 0;
    uint32_t rtn = 1;       // Pass NG  // 2022.10.31

    retry = 0;

    while ( retry <=3 )
    {
        // flag clear
        /* status = */
        tx_event_flags_get (&g_tcp_event_flags, FLG_TCP_RECEIVE_END,TX_OR_CLEAR, &actual_events,TX_NO_WAIT);
        Printf(" tcp send [login] \r\n");
        // send 'login'
        ret = tcp_send((char *)CMD_LOGIN, strlen(CMD_LOGIN), TX_WAIT_FOREVER);
        Printf(" tcp send end [login] \r\n");

        //status = tcp_receive(WAIT_1MIN);  6000は、長すぎる
        status = tcp_receive(100);
        Printf("tcp receive status %d\r\n", status);
        if(status == TX_SUCCESS){

            for(i = 0; i < tcp_receive_length; i++){
                Printf("%02X ", tcp_rxb[i]);
            }
            // NetPass照合

            Size = tcp_receive_length - 1;
            if(Size >=6 && Size <=26)
	        {
                if(Memcmp(my_config.network.NetPass, tcp_rxb, (int)strlen(my_config.network.NetPass) ,(int)Size) == 0){
                    Printf("\r\nPassCode OK !!\r\n");
                    // send 'OK'
                    ret = tcp_send((char *)CMD_OK, strlen(CMD_OK), TX_WAIT_FOREVER);
                    rtn = 0;        // Pass OK  // 2022.10.31
                    Printf("ret %d / rtn %d \r\n", ret, rtn);
                    break;
                }
                else
                {
                    ret = tcp_send((char *)CMD_LOGIN, strlen(CMD_LOGIN), TX_WAIT_FOREVER);
                    rtn = 1;        // Pass NG
                    Printf("\r\nPassCode Error !!\r\n");
                }
                
            }

        }
        retry++;
    }

    if(true == ret){    //2019.Dec.26 仮
        return (rtn);
    }
    return (1);
}


/**
 *
 * @return  0   :SUCCESS
 *          7   :NO EVENT
 *          10  :Length Over
 *          100 :Dis Connect 
 * 
 * @note    待ち時間は最大3分とする
 */
int32_t tcp_cmd_recv(void)
{
    int32_t ret = -1;
//    uint16_t i;       //2019.Dec.26 コメントアウト
    uint16_t j;
    uint16_t cmd_len = 0;
    UINT status;
    ULONG   actual_events;
    uint32_t len = 0;
    uint32_t t_len = 0;
    uint8_t *poi;

    //rx_ptr = 0;
    Printf("    tcp_cmd_recv start\r\n");
    

    status = tx_event_flags_get (&g_tcp_event_flags, FLG_TCP_RECEIVE_END | FLG_TCP_DISCONNECT ,TX_OR_CLEAR, &actual_events,WAIT_1MIN);
    if(status == TX_NO_EVENTS){
        ret = 7;
// 2023.03.01 タイムアウトによるソケット通信切断処理は一旦削除
//// 2022.10.31 ↓
//        tcp_no_data++;              // 受信データ無しカウント更新
//        if(tcp_no_data >= 3){       // 3回(3分間)受信データ無しでTCP通信を切断
//            tcp_no_data = 0;
//            ret = 100;
//            Printf("    tcp_cmd_recv disconnect no data\r\n");
//            DebugLog( LOG_DBG, "tcp_cmd_recv disconnect no data");
//        }
//// 2022.10.31 ↑
    }
    else if(actual_events & FLG_TCP_RECEIVE_END ){
    
        tcp_no_data = 0;                    // 2022.10.31

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
        
        Printf("TCP recv end  %ld (%ld)\r\n", t_len,cmd_len);

        poi = (uint8_t *)&SCOM.rxbuf.data;
        
        
        if(t_len >= (uint32_t)(cmd_len+6)){
            Printf("TCP recv end 1 %ld (%ld)\r\n", t_len,cmd_len);
            for(j = 0; j < t_len; j++ ){
               *poi++ = tcp_rxb[j+4];
            }
            ret = 0;
        }
        else{
            for(;;){
                /*if(t_len > (uint32_t)(cmd_len+6)){
                    Printf("TCP recv end 2 %ld (%ld)\r\n", t_len,cmd_len);
                    
                    for(j = 0; j < len; j++ ){
                        *poi++ = tcp_rxb[j+4];
                    }
                    ret = 0;
                    break;
                }*/
                //status = tx_event_flags_get (&g_tcp_event_flags, FLG_TCP_RECEIVE_END | FLG_TCP_DISCONNECT ,TX_OR_CLEAR, &actual_events,10);
                status = tx_event_flags_get (&g_tcp_event_flags, FLG_TCP_RECEIVE_END | FLG_TCP_DISCONNECT ,TX_OR_CLEAR, &actual_events,200);    // 2sec wait
                if((status == TX_NO_EVENTS) && ((actual_events & FLG_TCP_RECEIVE_END) == 0 ) ){
                    Printf("S:%04X  l:%ld  e:%04X\r\n", status, tcp_receive_length,actual_events );
                    ret = 7;
                    break;
                }
                else if(actual_events & FLG_TCP_RECEIVE_END ){
                    len = tcp_receive_length;
                    t_len += len;
                    //if(t_len > 1024+128){
                    if(t_len > TBUF_SIZE){          // sakaguchi 2021.06.16
                        Printf("TCP Length over %d\r\n", t_len);
                        ret = 10;
                        break;
                    }
                    else if(t_len > (uint32_t)(cmd_len+6)){
                        for(j = 0; j < t_len; j++ ){
                            *poi++ = tcp_rxb[j+4];
                        }
                        ret = 0;
                        break;    
                    }
                 }
                else if(actual_events & FLG_TCP_DISCONNECT){
                    Printf("    tcp_cmd_recv disconnect %08X\r\n",  actual_events);
                    DebugLog( LOG_DBG, "tcp_cmd_recv disconnect 1 %08X",  actual_events);
                    ret = 100;
                    break;
                }
            }
        }
        
        //ret = 0;
    }
    else if(actual_events & FLG_TCP_DISCONNECT){
        tcp_no_data = 0;                    // 2022.10.31

        Printf("    tcp_cmd_recv disconnect %08X\r\n",  actual_events);
        DebugLog( LOG_DBG, "tcp_cmd_recv disconnect 2 %08X",  actual_events);
        ret = 100;
    }

EXIT:
    Printf("TCP receve end %d\r\n", ret);
    return (ret);
}




/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/

/**
 * TCP Open
 * @return
 */
bool tcp_open(void)
{
    bool  ret = true;
    UINT status;
    ULONG ip_interface_status;
    
    // Check if initialization is not done
    if (is_open)
    {
        ret = false;
        goto EXIT;
    }

    // Wait for IP Link to be enabled 
    status = nx_ip_interface_status_check(&g_ip0, 0, NX_IP_LINK_ENABLED, &ip_interface_status, NX_WAIT_FOREVER);
    if (NX_SUCCESS != status)
    {
        Printf("tcp IP Link error\r\n");
        ret = false;
        goto EXIT;
    }

    // Create queue for receiving
    status = tx_queue_create(&(queue_table), "Receive Queue", MESSAGE_SIZE, &(queue_memory_table), QUEUE_SIZE);
    if (TX_SUCCESS != status)
    {
        Printf("tcp queue create error\r\n");
        ret = false;
        goto EXIT;
    }

    // Create thread for connection
    tx_thread_create(&(connection_thread_table), "Connection Thread", connection_thread_entry, 0, &(connection_thread_stack_table),
                    STACK_SIZE, THREAD_PRIORITY, THREAD_PRIORITY, THREAD_TIME_SLICE, TX_DONT_START);
    if (TX_SUCCESS != status)
    {
        Printf("tcp connect thread create error\r\n");
        ret = false;
        goto EXIT;
    }

    // Create thread for disconnection
    tx_thread_create(&(disconnection_thread_table), "Disconnection Thread", disconnection_thread_entry, 0, &(disconnection_thread_stack_table),
                    STACK_SIZE, THREAD_PRIORITY, THREAD_PRIORITY, THREAD_TIME_SLICE, TX_DONT_START);
    if (TX_SUCCESS != status)
    {
        Printf("tcp disconnect thread create error\r\n");
        ret = false;
        goto EXIT;
    }

    // Create server socket
    status = nx_tcp_socket_create(&g_ip0, &(socket_table), "Multi-Client Server Socket", NX_IP_NORMAL, NX_FRAGMENT_OKAY, 0x80, 200, NX_NULL, server_disconnect);
    if (NX_SUCCESS != status)
    {
        Printf("tcp socket create error\r\n");
        ret = false;
        goto EXIT;
    }

    // Register callback function which is called when one or more packets are received
    status = nx_tcp_socket_receive_notify(&(socket_table), tcp_receive_notify);
    if (NX_SUCCESS != status)
    {
        Printf("tcp socket receive notify\r\n");
        ret = false;
        goto EXIT;
    }

    // Start to listen
    //status = nx_tcp_server_socket_listen(&g_ip0, SERVER_PORT, &(socket_table), LISTEN_QUEUE_SIZE, listen_callback);
    status = nx_tcp_server_socket_listen(&g_ip0, *(uint16_t *)&my_config.network.SocketPort[0], &(socket_table), LISTEN_QUEUE_SIZE, listen_callback);       // sakaguchi 2020.09.24
    if (NX_SUCCESS != status)
    {
        Printf("tcp socket listen\r\n");
        ret = false;
        goto EXIT;
    }

    is_open = true;
EXIT:
    return (ret);

}

/**
 * TCP Close
 * @return
 */
bool tcp_close (void)
{
    bool ret = true;

    // Check if initialization is done
    if (!is_open)
    {
        ret = false;
        goto EXIT;
    }

    {
        // Disconnect the client
        nx_tcp_socket_disconnect(&socket_table, SERVER_TIMEOUT);

        // Unaccept this socket 
        nx_tcp_server_socket_unaccept(&socket_table);

        // Delete this socket
        nx_tcp_socket_delete(&socket_table);

        // Delete ThreadX resources
        tx_thread_terminate(&(connection_thread_table));
        tx_thread_delete(&(connection_thread_table));
        tx_thread_terminate(&(disconnection_thread_table));
        tx_thread_delete(&(disconnection_thread_table));
        tx_queue_delete(&(queue_table));
    }

    // Unlisten on this socket
    //nx_tcp_server_socket_unlisten(&g_ip0, SERVER_PORT);
    nx_tcp_server_socket_unlisten(&g_ip0, *(uint16_t *)&my_config.network.SocketPort[0]);           // sakaguchi 2020.09.24

    /* Reset client bits */

    is_open = false;

EXIT:
    return (ret);
}



/**
 *
 * @param p_data
 * @param size
 * @param wait_option
 * @return
 * @todo    sizeが大きすぎて抜けない時が有った
 */
bool tcp_send (char const * const p_data, uint32_t size, ULONG wait_option)
{
    bool        _ret = true;
    NX_PACKET * p_packet;
    uint32_t        status;
   // volatile int32_t    i;        //2019.Dec.26 コメントアウト
    int32_t sz;
    int32_t ss;        //data size
    volatile uint32_t bk = 0;

 
    sz = (int32_t)size;
    //if(sz > 2048)
    //    sz = 2048;

    Printf("\r\n    tcp send start (%ld, %ld)\r\n", size, sz);
    /* Check if initialization is done */
    if (!is_open)
    {
        DebugLog( LOG_DBG, "TCP Open Error");
        _ret = false;
        goto EXIT;
    }

    while(sz > 0){
        // Allocate packet from packet pool/
        status = nx_packet_allocate(&g_packet_pool0, &p_packet, NX_TCP_PACKET, wait_option);
        if (NX_SUCCESS != status)
        {
            DebugLog( LOG_DBG, "TCP Allocate packet Error %ld", status);
            _ret = false;
            goto EXIT;
        }

        if(sz > 2048){
            ss = 2048;
        }
        else{
            ss = sz;
        }
        // Set data to send
        status = nx_packet_data_append(p_packet, (uint8_t *)(p_data + bk*2048) , (uint32_t)ss, &g_packet_pool0, wait_option);
        if (NX_SUCCESS != status)
        {
            nx_packet_release(p_packet);    // 2020.12.08
            _ret = false;
            goto EXIT;
        }

        // Send packet to the client
        status = nx_tcp_socket_send(&socket_table, p_packet, wait_option);
        if (NX_SUCCESS != status)
        {
            nx_packet_release(p_packet);
        }
        sz -= 2048;
        bk++;
        Printf("tcp send bk (%d)\r\n", bk);
    }
    Printf("tcp send end (%d)\r\n", status);
EXIT:
    return (_ret);
}



/**
 *
 * @param wait_option
 * @return  結果 TX_SUCCESS＝成功、 －1 ＝ 失敗
 */
int32_t tcp_receive (ULONG wait_option)
{
//    UINT      status;     //2019.Dec.26 コメントアウト
    ULONG   actual_events;
//    uint32_t i;     //2019.Dec.26 コメントアウト
    int32_t ret = -1;

    // Check if initialization is done
    if (!is_open)
    {
        goto EXIT;
    }

    /*status = */tx_event_flags_get (&g_tcp_event_flags, FLG_TCP_RECEIVE_END,TX_OR_CLEAR, &actual_events,wait_option);
    if(actual_events & FLG_TCP_RECEIVE_END ){
        ret = TX_SUCCESS;
    }

    //Printf("tcp receive %d \r\n", ret);
EXIT:
    Printf("tcp receive %d \r\n", ret);
    return (ret);
}

/**
 *
 * @param p_data
 * @param size
 * @param wait_option
 * @return
 */
bool tcp_receive2 (char * const p_data, uint32_t size, ULONG wait_option)
{
    bool      _ret = true;
    UINT      _status;
    uint8_t * p_data_internal = (uint8_t *)p_data;
    uint8_t   ch;

    // Check if initialization is done
    if (!is_open)
    {
        _ret = false;
        goto EXIT;
    }

    // Repeat until specified bytes are acquired
    while (size)
    {
        // Retrieve one byte from queue
        _status = tx_queue_receive(&queue_table, &ch, wait_option);
        if (TX_SUCCESS != _status)
        {
            _ret = false;
            goto EXIT;
        }

        *p_data_internal++ = ch;
        size--;
    }

EXIT:

    return (_ret);
}



/*******************************************************************************************************************//**
 * @brief       This is called when a disconnect is issued by the client
 *
 * @param[in]   p_socket    Pointer to server socket
 * @retval      none
 **********************************************************************************************************************/
static void server_disconnect (NX_TCP_SOCKET * p_socket)
{
    //int  index;

    /* Get the index of the client */
    //index = 0;  //search_socket_table_index(p_socket);

    SSP_PARAMETER_NOT_USED(p_socket);   //2019.Dec.26 仮でコメントアウト

    Printf("tcp server_disconnect \r\n");

    /* Start a thread corresponding to socket for accept */
    if (TX_COMPLETED == disconnection_thread_table.tx_thread_state)
    {
        tx_thread_reset(&(disconnection_thread_table));
    }
    tx_thread_resume(&(disconnection_thread_table));

    
    return;
}


/*******************************************************************************************************************//**
 * @brief       This is called when one or more packets are received
 *
 * @param[in]   p_socket    Pointer to server socket
 * @retval      none
 * 
 * この中で、TCPのパケットが受信される
 **********************************************************************************************************************/
static void tcp_receive_notify (NX_TCP_SOCKET * p_socket)
{
    UINT        status;
    NX_PACKET * p_packet;
    ULONG       len;

    Printf("tcp_receive_notify (%d)\r\n", rx_ptr);
    tcp_receive_length = 0;

    // Receive a packet
    status = nx_tcp_socket_receive(p_socket, &p_packet, NX_NO_WAIT);
    if (NX_SUCCESS == status)
    {
        len = p_packet->nx_packet_length;
        Printf("tcp_receive_notify %d \r\n", len);
        if(len > TBUF_SIZE)
            len = TBUF_SIZE;
        tcp_receive_length = len;
        memcpy(&tcp_rxb[rx_ptr], p_packet->nx_packet_prepend_ptr, len);
        rx_ptr += tcp_receive_length;
        // Release the packet
        nx_packet_release(p_packet);
        // tcp  receive end flag set
        tx_event_flags_set (&g_tcp_event_flags, FLG_TCP_RECEIVE_END, TX_OR);
    }
}

/*******************************************************************************************************************//**
 * @brief       This is called when connection request is arrived
 *
 * @param[in]   p_socket    Pointer to server socket
 * @param[in]   port        Port number
 * @retval      none
 **********************************************************************************************************************/
static void listen_callback (NX_TCP_SOCKET * p_socket, UINT port)
{
    //int  index;

    SSP_PARAMETER_NOT_USED(p_socket);   //2019.Dec.26 仮でコメントアウト
    SSP_PARAMETER_NOT_USED(port);

    /* Get the index of the client */
    //index = 0;  //search_socket_table_index(p_socket);

    Printf("tcp listen callback  port:%d\r\n", port);

    /* Start a thread corresponding to socket for accept */
    if (TX_COMPLETED == connection_thread_table.tx_thread_state)
    {
        tx_thread_reset(&(connection_thread_table));
    }
    tx_thread_resume(&(connection_thread_table));
}


/**
 *****************************************************************************************************************//**
 * @brief       Establish a connection, and relisten
 *
 *********************************************************************************************************************
*/
static void connection_thread_entry (void)
{
    UINT status;
    ULONG   actual_events;
    //int  index = 0;//(int)entry_input;
//    bool error = false;       //2019.Dec.26 コメントアウト

    Printf("tcp connect thread entry 1\r\n");
    rx_ptr = 0;

    // Accept the connection
    status = nx_tcp_server_socket_accept(&socket_table, SERVER_TIMEOUT);
    Printf("tcp connect thread entry 2-%d\r\n", status);

    if (NX_SUCCESS != status)
    {
        nx_tcp_server_socket_unaccept(&(socket_table));
 //       error = true;     //2019.Dec.26 コメントアウト
    }
    
    // Check if the socket is closed
    if (NX_TCP_CLOSED == socket_table.nx_tcp_socket_state)
    {
        // Relisten on this socket
        //status = nx_tcp_server_socket_relisten(&g_ip0, SERVER_PORT, &(socket_table));
        status = nx_tcp_server_socket_relisten(&g_ip0, *(uint16_t *)&my_config.network.SocketPort[0], &(socket_table));       // sakaguchi 2020.09.24
        if ((NX_SUCCESS != status) && (NX_CONNECTION_PENDING != status)){
            Printf("tcp connect thread entry 3-%d\r\n", status);
        }
        else{
            Printf("tcp connect thread entry 4\r\n");
        }
    }


    tx_event_flags_get (&g_tcp_event_flags, FLG_TCP_DISCONNECT,TX_OR_CLEAR, &actual_events, TX_NO_WAIT);
    tx_event_flags_set (&g_tcp_event_flags, FLG_TCP_CONNECT,TX_OR);
    Printf("tcp connect thread entry end %08X\r\n" , actual_events);
    /*
    if (error)
    {
        goto EXIT;
    }

    // Call the callback function if not NULL
    if (p_new_connection_callback)
    {
        Printf("tcp connect thread entry 5\r\n");
        //p_new_connection_callback((uint8_t)(1 << index));
    }
*/
//EXIT:     //2019.Dec.26 コメントアウト
    return;
}



/*******************************************************************************************************************//**
 * @brief       Disconnect client, and relisten
 *
 **********************************************************************************************************************/
static void disconnection_thread_entry (void)
{
    UINT status;
    //int  index = 0; //(int)entry_input;

    Printf("disconnet thread entry \r\n");

    // Check if this socket entered a disconnect state
    if (NX_TCP_ESTABLISHED < socket_table.nx_tcp_socket_state)
    {
        // Disconnect the client 
        Printf("disconnet thread entry 1\r\n");
        nx_tcp_socket_disconnect(&(socket_table), SERVER_TIMEOUT);
    }


    // Unaccept this socket
    nx_tcp_server_socket_unaccept(&(socket_table));

    // Flush queue 
    tx_queue_flush(&(queue_table));

    // Check if a socket is already in listen state
    if (NX_TCP_LISTEN_STATE == socket_table.nx_tcp_socket_state)
    {
        // Found. Exit this function without relisten
        Printf("disconnet thread entry 2\r\n"); 
        goto EXIT;
    }

    // Search a socket to relisten
    // Check if the socket is closed
    if (NX_TCP_CLOSED == socket_table.nx_tcp_socket_state)
    {
        // Relisten on this socket
        //status = nx_tcp_server_socket_relisten(&g_ip0, SERVER_PORT, &(socket_table));
        status = nx_tcp_server_socket_relisten(&g_ip0, *(uint16_t *)&my_config.network.SocketPort[0], &(socket_table));          // sakaguchi 2020.09.24
         Printf("disconnet thread entry 3 %d\r\n", status);
        //if ((NX_SUCCESS != status) && (NX_CONNECTION_PENDING != status))
    }

    tx_event_flags_set (&g_tcp_event_flags, FLG_TCP_DISCONNECT,TX_OR);

EXIT:
    // Call the callback function if not NULL
    //if (p_disconnection_callback)
    //{
    //    p_disconnection_callback((uint8_t)(1 << index));
    //}

    return;
}



