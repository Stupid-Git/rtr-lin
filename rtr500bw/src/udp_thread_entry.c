/**
 * @file    udp_thread_entry.c
 * @brief   UDPスレッド
 *
 * 2019.Dec.26 ビルドワーニング潰し完了
 */

#define _UDP_THREAD_ENTRY_C_

#include "nx_api.h"
#include "udp_thread.h"
#include "udp_thread_entry.h"

#include "network_thread.h"
//#include "dhcp_thread.h"
#include "Globals.h"
#include "General.h"


/*
 *
 * 2019.Dec.26 ビルドワーニング潰し完了
 */

static NX_UDP_SOCKET udp_socket;
//static NX_UDP_SOCKET udp_socket_t;
uint8_t udp_rxb[256];

/// UDP 応答の構造
struct{ 
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




 /**
  * Udp Thread entry function
  */
void udp_thread_entry(void)
{
    UINT  status;
    volatile uint32_t err = 0;
    UINT port, r_port;
    //uint32_t addr;
//    UINT  size;      //2019.Dec.26 コメントアウト
    ULONG len;

    NX_PACKET* packet;
    NX_PACKET* packet2;
//    NXD_ADDRESS address;      //2019.Dec.26 コメントアウト
    ULONG addr = IP_ADDRESS(255,255,255,255);


    r_port = (UINT)my_config.network.UdpRxPort;
    Printf("UDP start %d\r\n", r_port);

    // Linkしたら実行する

    status = nx_udp_enable(&g_ip0);
    if(status){
        err |= 0x01;
    }
 
    for(;;){

        // Create UDP socket.

        status = nx_udp_socket_create(&g_ip0, &udp_socket,"UDP Socket 0", NX_IP_NORMAL, NX_FRAGMENT_OKAY,0x80,30);
        Printf("### UDP nx_udp_socket_create  0x%x\r\n", status);
        if(status){
            Printf("### UDP nx_udp_socket_create  Error\r\n");
            goto EXIT_END;
        }

        //status = nx_udp_socket_bind(&udp_socket, 62501, 300);
        status = nx_udp_socket_bind(&udp_socket, *(uint16_t *)&my_config.network.UdpRxPort[0], 300);        // sakaguchi 2020.09.24
        if(status){
            Printf("### UDP socket bind error 0x%x\r\n", status);
            nx_udp_socket_delete(&udp_socket);
            goto EXIT_END;
        }

        while (1)       // 受信
        {
            //Printf("    Network Mutex get wait\r\n");
            if(tx_mutex_get(&mutex_network_init,  WAIT_100MSEC) == TX_SUCCESS)
            { 
                tx_mutex_put(&mutex_network_init);
                //  0x00    NX_SUCCESS
                //  0x01    NX_NO_PACKET
                //  0x24    NX_NOT_BOUND
                status =  nx_udp_socket_receive(&udp_socket, &packet, 500/*  NX_WAIT_FOREVER*/);
                //Printf("UDP receive  0x%x\r\n", status);
            if(status == NX_SUCCESS){
                nx_udp_source_extract( packet, &addr, &port );
                len = packet->nx_packet_length;
                if(len > 256){
                    len = 256;
                }
                memcpy(udp_rxb, packet->nx_packet_prepend_ptr, len);
                    //for(int i=0;i<len;i++)
                    //    Printf("%02X ", udp_rxb[i]);
                    //Printf("\r\n");

                nx_packet_release( packet );
                if((udp_rxb[0] == 0x50) && (udp_rxb[1] == 0x01) && (udp_rxb[2] == 0x01)){

                    Udp_Resp_Set();

                    status = nx_packet_allocate( &g_packet_pool0, &packet2, NX_UDP_PACKET, NX_WAIT_FOREVER );
                    //Printf("\r\nUDP send 1 %d\r\n", status);
                    status = nx_packet_data_append( packet2, &UDP, 256, (NX_PACKET_POOL *)&packet2, NX_WAIT_FOREVER );
                    //Printf("\r\nUDP send 2 %d\r\n", status);
                    addr = IP_ADDRESS(255,255,255,255);
                    //if(net_cfg.interface_index != 1 )
                    //    status = nx_udp_socket_send(&udp_socket, packet2, addr, 62502);
                    //else
                            //status = nx_udp_socket_interface_send(&udp_socket, packet2, addr, 62502, net_cfg.interface_index);
                        status = nx_udp_socket_interface_send(&udp_socket, packet2, addr, *(uint16_t *)&my_config.network.UdpTxPort[0], net_cfg.interface_index);           // sakaguchi 2020.09.24

                    Printf("UDP send end %d\r\n", status);
                    nx_packet_transmit_release( packet2 );
                }
            }
            }
            //Printf("\r\nudp recive1 %d %ld port=%ld adr=%ld  size=%d \r\n", status, len, port,addr,size);
            //tx_thread_sleep (500);   // sleep 5s
            tx_thread_sleep (200);   // sleep 2s
        }
// ここには、来ない
        nx_udp_socket_unbind(&udp_socket);
EXIT_END:

        tx_thread_sleep (1000);     // sleep 10s
    }

}


/**
 * @brief   UDP 応答のセット
 * @bug UDP.Serialにセットする値は8Byteを超える
 */
void Udp_Resp_Set(void)
{
 //   uint8_t *dst;     //2019.Dec.26 コメントアウト
    uint32_t val;

    memset(&UDP, 0x00, sizeof(UDP));
    UDP.Code[0]     = 0x50;
    UDP.Code[1]     = 0x01;
    UDP.Command[0]  = 0x01;
    UDP.Status[0]   = 0x00;
    UDP.DHCP[0] = my_config.network.DhcpEnable[0];
    uint32to8(net_address.active.address, UDP.IP);
    uint32to8(net_address.active.mask, UDP.MASK);
    uint32to8(net_address.active.gateway, UDP.GW);
    uint32to8(net_address.active.dns1, UDP.DNS1);
    uint32to8(net_address.active.dns2, UDP.DNS2);
    
    //UDP.ComPort[0] = 0x24;
    //UDP.ComPort[1] = 0xf4;
    UDP.ComPort[0] = my_config.network.SocketPort[0];
    UDP.ComPort[1] = my_config.network.SocketPort[1];

    //fact_config.SerialNumber = 0x5f58ffff;// 0x5f9cffff; //debug
    sprintf( (char *)UDP.Serial , "%.8lX", fact_config.SerialNumber);       ///< @note 8Byte超える @bug 8Byte超える
    memcpy(UDP.BaseName,my_config.device.Name, 32);
    memcpy(UDP.Description,my_config.device.Description,64);
    // PW は送信しない
    // MAC test data

    val = net_address.eth.mac1;
    UDP.MAC[1] = (UCHAR)(val & 0x000000ff);      
    UDP.MAC[0] = (UCHAR)((val >> 8 ) & 0x000000ff); 
    
    val = net_address.eth.mac2; 
    UDP.MAC[5] = (UCHAR)(val & 0x000000ff);      
    UDP.MAC[4] = (UCHAR)((val >> 8 ) & 0x000000ff);
    UDP.MAC[3] = (UCHAR)((val >> 16 ) & 0x000000ff);
    UDP.MAC[2] = (UCHAR)((val >> 24 ) & 0x000000ff);
    /*
    UDP.MAC[0] = 0x2e;
    UDP.MAC[1] = 0x09;
    UDP.MAC[2] = 0x0a;
    UDP.MAC[3] = 0x00;
    UDP.MAC[4] = 0x76;
    UDP.MAC[5] = 0xc7;
    */

}


