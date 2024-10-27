/**
 * @file    Net_func.c
 *
 * @date     Created on: 2019/10/17
 * @author      tooru.hayashi
 * @note	2020.Jul.01 GitHub 630ソース反映済み
 */


#define _NET_FUNC_C_

#include "Net_func.h"
#include "wifi_thread.h"
#include "General.h"
#include "Net_cache.h"                  // sakaguchi 2021.07.20

//"nxd_bsd.h" で定義済み
#if 0
struct in_addr
{
    ULONG           ip_host;             /* Internet address (32 bits).                         */
};
#endif

UINT find_ip_address( srv_info_t* p_info )
{
    UINT status;
	//UINT sta;

    ULONG ip_host;
    struct in_addr addr;
    bool ipval;
	int i;

    ipval = JudgeIPAdrs(p_info->host);
    //Printf(" IP ??? %d \r\n ", ipval);
    if(ipval){                               // host 192.xxx.xxx.xxx 形式
        inet_aton(p_info->host , &addr);
//        p_info->server_ip.nxd_ip_address.v4 = addr.ip_host;
        p_info->server_ip.nxd_ip_address.v4 = addr.s_addr;
        p_info->server_ip.nxd_ip_version = 4;
        Printf(" Host address xxx.xxx.xxx.xxx \r\n");
        status = NX_SUCCESS;
    }
    else{

        // Lookup server name using DNS
        // UINT  nx_dns_host_by_name_get(NX_DNS *dns_ptr, UCHAR *host_name, ULONG *host_address_ptr, ULONG wait_option);
		for(i=0;i<4;i++){
        	Printf ("  . nx_dns_host_by_name_get (%d)\r\n", i);
	        //status = nx_dns_host_by_name_get (&g_dns0, (uint8_t *)p_info->host, &ip_host, 1000);
			//status = nx_dns_host_by_name_get (&g_dns0, (uint8_t *)p_info->host, &ip_host, 2000);				// hayashi 8.27
	
		
			//status = nx_dns_host_by_name_get (&g_dns0, (uint8_t *)p_info->host, &ip_host, 100);				// hayashi 8.27
//			status = nx_dns_host_by_name_get (&g_dns0, (uint8_t *)p_info->host, &ip_host, 5000);				// hayashi 9.1    1000sec ?? 
			status = nx_dns_host_by_name_get (&g_dns0, (uint8_t *)p_info->host, &ip_host, 1000);				// hayashi 10.06   sec ?? 
			//status = nxd_dns_host_by_name_get (&g_dns0, (uint8_t *)p_info->host, &ip_host, 5000, NX_IP_VERSION_V4);
			if (status != NX_SUCCESS)
    	    {
        	    Printf ("  Unable to resolve ");
	            Printf (p_info->host);
    	        Printf ("\r\n");
        	    Printf ("  nx_dns_host_by_name_get Failed. status = 0x%x\r\n", status);
            	// 0xA1
	            // #define NX_DNS_NO_SERVER  0xA1  /* No DNS server was specified          

				switch (status)     // for debug
    	    	{
	        		case 0xa1:
    	        		Printf("NX_DNS_NO_SERVER  /  DNS server was specified\r\n");
	    	        	break;
					case 0xa3:
        	    		Printf("NX_DNS_QUERY_FAILED / No valid DNS response received\r\n");
						break;
					case 0x07:
    		        	Printf("NX_PTR_ERROR\r\n");
						break;
					case 0x11:
            			Printf("NX_CALLER_ERROR\r\n");
						break;
					case 0xa8:
    	    	    	Printf("NX_DNS_PARAM_ERROR\r\n");
						break;
	       		 	default:
						Printf("Other DNS Error\r\n");
        	    		break;
        		}
#if 1 // use cache	// sakaguchi 2021.07.20
				// リンクアップしている場合のみキャッシュから読み込む 2022.04.11
				if(Link_Status == LINK_UP){
					p_info->server_ip.nxd_ip_address.v4 = dns_local_cache_get(p_info->host);;
					p_info->server_ip.nxd_ip_version = 4;
					if(p_info->server_ip.nxd_ip_address.v4 != 0){
						status = NX_SUCCESS;
					}
				}
#endif

            	//return (status);
/*		
				if(i==1){		// 2020 10 07
					sta = nx_dns_delete(&g_dns0);
    				Printf("dns delete %02X\r\n", sta);
    				memset(&g_dns0, 0, sizeof(g_dns0));						

					dns_client_init0();
					nx_dns_server_remove_all(&g_dns0);
					sta = nx_dns_server_add(&g_dns0, net_address.active.dns1);
					Printf("dns server add %02X\r\n", sta);
				}
*/
    	    }
        	else
	        {
				Printf("IP ADDRESS is: %d.%d.%d.%d \n\r", (int)(ip_host>>24), (int)(ip_host>>16)&0xFF, (int)(ip_host>>8)&0xFF, (int)(ip_host)&0xFF );
        	    p_info->server_ip.nxd_ip_address.v4 = ip_host;
            	p_info->server_ip.nxd_ip_version = 4;
				//break;
#if 1 // use cache		// sakaguchi 2021.07.20
        dns_local_cache_put(p_info->host, ip_host);
#endif // use cache
				break;
        	}
		}
    }

    return (status);
}


UINT find_ip_address2( char *target )
{
    UINT status;

	char  *host;

    ULONG ip_host;
    struct in_addr addr;
    bool ipval;
	//int i;

	host = target;
	Printf("find_ip_address2  %s \r\n", host);
	

	
	ipval = JudgeIPAdrs(host);
    if(ipval){                               // host 192.xxx.xxx.xxx 形式
        inet_aton(host , &addr);
        //p_info->server_ip.nxd_ip_address.v4 = addr.s_addr;
        //p_info->server_ip.nxd_ip_version = 4;
		target_ip.nxd_ip_address.v4 = addr.s_addr;
		target_ip.nxd_ip_version = 4;
        Printf(" Host address xxx.xxx.xxx.xxx \r\n");
        status = NX_SUCCESS;
    }
    else{

        // Lookup server name using DNS
        // UINT  nx_dns_host_by_name_get(NX_DNS *dns_ptr, UCHAR *host_name, ULONG *host_address_ptr, ULONG wait_option);

        Printf ("  . nx_dns_host_by_name_get\r\n");
			
		//status = nx_dns_host_by_name_get (&g_dns0, (uint8_t *)p_info->host, &ip_host, 100);				// hayashi 8.27
		status = nx_dns_host_by_name_get (&g_dns0, (uint8_t *)host, &ip_host, 5000);				// hayashi 9.1    1000sec ?? --> 5min 
		if (status != NX_SUCCESS)
        {
            Printf ("  Unable to resolve ");
            Printf (host);
            Printf ("\r\n");
            Printf ("  nx_dns_host_by_name_get Failed. status = 0x%x\r\n", status);
            // 0xA1
            // #define NX_DNS_NO_SERVER  0xA1  /* No DNS server was specified          

			switch (status)     // for debug
        	{
        	case 0xa1:
            	Printf("NX_DNS_NO_SERVER  /  DNS server was specified\r\n");
            	break;
			case 0xa3:
            	Printf("NX_DNS_QUERY_FAILED / No valid DNS response received\r\n");
				break;
			case 0x07:
            	Printf("NX_PTR_ERROR\r\n");
				break;
			case 0x11:
            	Printf("NX_CALLER_ERROR\r\n");
				break;
			case 0xa8:
            	Printf("NX_DNS_PARAM_ERROR\r\n");
				break;
       	 	default:
				Printf("Other DNS Error\r\n");
            	break;
        	}
            //return (status);
#if 1 // use cache			// sakaguchi 2021.07.20
			// リンクアップしている場合のみキャッシュから読み込む 2022.04.11
			if(Link_Status == LINK_UP){
				target_ip.nxd_ip_address.v4 = dns_local_cache_get(host);
				target_ip.nxd_ip_version = 4;
				if(target_ip.nxd_ip_address.v4 != 0){
					status = NX_SUCCESS;
				}
			}
#endif
        }
        else
        {
			Printf("IP ADDRESS is: %d.%d.%d.%d \n\r", (int)(ip_host>>24), (int)(ip_host>>16)&0xFF, (int)(ip_host>>8)&0xFF, (int)(ip_host)&0xFF );
            target_ip.nxd_ip_address.v4 = ip_host;
            target_ip.nxd_ip_version = 4;

#if 1 // use cache		// sakaguchi 2021.07.20
            dns_local_cache_put(host, ip_host);
#endif // use cache
			//break;
        }
	
    }

    return (status);
}


UINT find_ip_address_1_2( srv_info_t* p_info )
{
    UINT status;

    ULONG ip_host;
    struct in_addr addr;
    bool ipval;
    //int i;

    ipval = JudgeIPAdrs(p_info->host);
    //Printf(" IP ??? %d \r\n ", ipval);
    if(ipval){                               // host 192.xxx.xxx.xxx 形式
        inet_aton(p_info->host , &addr);
//        p_info->server_ip.nxd_ip_address.v4 = addr.ip_host;
        p_info->server_ip.nxd_ip_address.v4 = addr.s_addr;
        p_info->server_ip.nxd_ip_version = 4;
        Printf(" Host address xxx.xxx.xxx.xxx \r\n");
        status = NX_SUCCESS;
    }
    else{

        // Lookup server name using DNS
        // UINT  nx_dns_host_by_name_get(NX_DNS *dns_ptr, UCHAR *host_name, ULONG *host_address_ptr, ULONG wait_option);

        Printf ("  . nx_dns_host_by_name_get\r\n");


        status = nx_dns_host_by_name_get (&g_dns0, (uint8_t *)p_info->host, &ip_host, 100);
        if (status == NX_SUCCESS)
        {
            goto L_OK;
        }
        Printf("  . find_ip_address_1_2: fail 1: status = %d\r\n", status);
        status = nx_dns_host_by_name_get (&g_dns0, (uint8_t *)p_info->host, &ip_host, 100);
        if (status == NX_SUCCESS)
        {
            goto L_OK;
        }
        Printf("  . find_ip_address_1_2: fail 2: status = %d\r\n", status);

        status = nx_dns_host_by_name_get (&g_dns0, (uint8_t *)p_info->host, &ip_host, 5000);
        if (status != NX_SUCCESS)
        {
            Printf ("  Unable to resolve ");
            Printf (p_info->host);
            Printf ("\r\n");
            Printf ("  nx_dns_host_by_name_get Failed. status = 0x%x\r\n", status);
            // 0xA1
            // #define NX_DNS_NO_SERVER  0xA1  /* No DNS server was specified

            switch (status)     // for debug
            {
            case 0xa1:
                Printf("NX_DNS_NO_SERVER  /  DNS server was specified\r\n");
                break;
            case 0xa3:
                Printf("NX_DNS_QUERY_FAILED / No valid DNS response received\r\n");
                break;
            case 0x07:
                Printf("NX_PTR_ERROR\r\n");
                break;
            case 0x11:
                Printf("NX_CALLER_ERROR\r\n");
                break;
            case 0xa8:
                Printf("NX_DNS_PARAM_ERROR\r\n");
                break;
            default:
                Printf("Other DNS Error\r\n");
                break;
            }
#if 1 // use cache			// sakaguchi 2021.07.20
			// リンクアップしている場合のみキャッシュから読み込む 2022.04.11
			if(Link_Status == LINK_UP){
				p_info->server_ip.nxd_ip_address.v4 = dns_local_cache_get(p_info->host);;
				p_info->server_ip.nxd_ip_version = 4;
				if(p_info->server_ip.nxd_ip_address.v4 != 0){
					status = NX_SUCCESS;
				}
			}
#endif
            //return (status);
            goto L_EXIT;
        }
        else
        {
            //Printf("IP ADDRESS is: %d.%d.%d.%d \n\r", (int)(ip_host>>24), (int)(ip_host>>16)&0xFF, (int)(ip_host>>8)&0xFF, (int)(ip_host)&0xFF );
            //p_info->server_ip.nxd_ip_address.v4 = ip_host;
            //p_info->server_ip.nxd_ip_version = 4;
            //break;
            goto L_OK;
        }
    L_OK: ;
        Printf("IP ADDRESS is: %d.%d.%d.%d \n\r", (int)(ip_host>>24), (int)(ip_host>>16)&0xFF, (int)(ip_host>>8)&0xFF, (int)(ip_host)&0xFF );
        p_info->server_ip.nxd_ip_address.v4 = ip_host;
        p_info->server_ip.nxd_ip_version = 4;
#if 1 // use cache			// sakaguchi 2021.07.20
        dns_local_cache_put(p_info->host, ip_host);
#endif // use cache

    }
    L_EXIT: ;
    return (status);
}



/**
 * IPアドレス判定
 * @param src
 * @return
 */
bool JudgeIPAdrs( char *src )
{

	size_t	size = strlen( src );
	char 	*wd;
	uint16_t	i, len;
	char	Period, dmy[4];
	bool	err = false;

	if ( size >= 7 && size <= 15 ) {		// *.*.*.*

		Period = 0;

		for ( i=0; i<3; i++ ) {				// ピリオド3つ
		    wd = memchr( src, '.', size );
			if ( wd ) {
				len = (uint16_t)(wd - src);				// 文字数
				if ( len ) {
//					Term[Period++] = Str2U( (char *)src, 4 );
					Period++;
					if ( Str2U( src, 4 ) < 0 ){
					    goto IPerr;
					}
					src = wd + 1;
					size = (size_t)(size - (uint16_t)( len + 1 ));
				}
				else{
				    goto IPerr;
				}
			}
			else{
			    goto IPerr;
			}
		}

		if ( Period == 3 && size <= 3 ) {
			memcpy( dmy, src, size );
			dmy[size] = '\0';
//			Term[3] = Str2U( (char *)dmy, 4 );	// 最後の１セグメント
			if ( Str2U( dmy, 4 ) < 0 ){
			    goto IPerr;
			}
		}
		else{
		    goto IPerr;
		}
/*
		for ( i=0; i<4; i++ ) {
			if ( Term[i] < 0 || Term[i] > 255 )
				goto IPerr;
		}
*/
	}
	else{
	    goto IPerr;
	}

	err = true;

IPerr:

	return (err);

}



