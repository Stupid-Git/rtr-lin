//#include "_r500_config.h"         // sakaguchi 2021.07.20

//#include "nxd_bsd.h"

#define _NET_CACHE_C_

#include <Net_cache.h>

//#include "r500_defs.h"            // sakaguchi 2021.07.20
#include "MyDefine.h"               // sakaguchi 2021.07.20
#include "Config.h"                 // sakaguchi 2021.07.20



#define USE_NOISY_CACHE  1

#define N_CACHE 4
static char dns_l_cache_names[N_CACHE][128 + 1];
static uint32_t dns_l_cache_ipv4Addrs[N_CACHE];
static uint32_t dns_l_cache_age[N_CACHE];


void dns_local_cache_init(void)
{
    int i;
    for(i = 0; i < N_CACHE ; i++) {
        strcpy(dns_l_cache_names[i], "");
        dns_l_cache_ipv4Addrs[i] = 0;
        dns_l_cache_age[i] = N_CACHE;
    }
}

static void list_delete_index_n(uint32_t n);
static void list_delete_index_n(uint32_t n)
{
    int i;
    int j;

    for(i = 0; i < N_CACHE ; i++) {
        if(dns_l_cache_age[i] == n) // reached the entry
        {
            for(j = i; j < (N_CACHE - 1) ; j++) { // shift older up
                dns_l_cache_age[j] = dns_l_cache_age[j+1];
            }
            dns_l_cache_age[j] = N_CACHE; // last one INVALID;
        }
    }
}

static uint32_t list_shift(void);
static uint32_t list_shift(void)
{
    uint32_t next;
    int i;
    int j;

    // Get the last entry
    next = dns_l_cache_age[N_CACHE - 1];

    // Shift and toss last one
    for(i = (N_CACHE - 1); i > 0 ; i--) {
        dns_l_cache_age[i] = dns_l_cache_age[i - 1];
    }
    dns_l_cache_age[0] = N_CACHE;

    // if last one was not yet used
    if(next == N_CACHE) {

        // find first unused number
        for(next = 0; next < N_CACHE ; next++) {
            for(j = 1; j < N_CACHE ; j++) {
                if(dns_l_cache_age[j] == next)
                    break;
            }
            if(j == N_CACHE) // we did not find next
            {
                break;
            }
        }
    }
    dns_l_cache_age[0] = next;
    return next;
}

void dns_local_cache_put(char *name, uint32_t ipv4Addr)
{
    if(name == 0) {
        return;
    }
    if(ipv4Addr == 0) {
        return;
    }

    uint32_t i;

    // Check and exit if exact match
    for(i = 0; i < N_CACHE ; i++) {
        if(strcmp(dns_l_cache_names[i], name) == 0) {
            if(ipv4Addr == dns_l_cache_ipv4Addrs[i]) { // exit if in Cache
#if USE_NOISY_CACHE
                //Printf("DNS Cache Entry Already Exists\r\n");
#endif
                return;
            }
        }
    }


    uint32_t next;
    // If host name is there but (address is different) update entry
    for(i = 0; i < N_CACHE ; i++) {
        if(strcmp(dns_l_cache_names[i], name) == 0) {
            list_delete_index_n(i); // delete old entry
#if USE_NOISY_CACHE
            Printf("DNS Cache Removed Entry Index = %d\r\n", i);
#endif
            break;
        }
    }

    // Add a new entry
    next = list_shift(); // toss
    strcpy(dns_l_cache_names[next], name);
    dns_l_cache_ipv4Addrs[next] = ipv4Addr;
    dns_l_cache_age[0] = next;
#if USE_NOISY_CACHE
    Printf("DNS Cache Added Entry Index = %d\r\n", next);
#endif

}

uint32_t dns_local_cache_get(char *name)
{
    uint32_t ipv4Addr = 0;
    int i;
    for(i = 0; i < N_CACHE ; i++) {
        if(strcmp(dns_l_cache_names[i], name) == 0) {
            ipv4Addr = dns_l_cache_ipv4Addrs[i];
            break;
        }
    }
    return ipv4Addr;
}


static void dns_local_cache_test_a(uint32_t adrIn, char *urlStr);
static void dns_local_cache_test_a(uint32_t adrIn, char *urlStr)
{
    uint32_t adrOut;
    size_t i;

    adrOut = dns_local_cache_get(urlStr);
    if(adrOut != adrIn) {
        Printf(" dns_local_cache_get() Failed (Not In Cache). Got  0x%08x, expected 0x%08x\r\n", adrOut, adrIn);
    }

    dns_local_cache_put(urlStr, adrIn);
    adrOut = dns_local_cache_get(urlStr);
    if(adrOut != adrIn) {
        Printf(" dns_local_cache_get() Failed. Got  0x%08x, expected 0x%08x\r\n", adrOut, adrIn);
    }

    for(i = 0; i < 100 ; i++) {
        dns_local_cache_put(urlStr, adrIn);

        adrOut = dns_local_cache_get(urlStr);
        if(adrOut != adrIn) {
            Printf(" dns_local_cache_get() Failed. Got  0x%08x, expected 0x%08x\r\n", adrOut, adrIn);
            break;
        }
    }

}


void dns_local_cache_test(void);
void dns_local_cache_test(void)
{
    uint32_t adrIn;
    uint32_t adrOut;
    size_t i;

    dns_local_cache_init();

    for(i = 0; i < 2 ; i++) {
        adrIn = 0x11223344;
        dns_local_cache_test_a(adrIn, "url.1.com");

        adrIn = 0x22334455;
        dns_local_cache_test_a(adrIn, "url.2.com");

        adrIn = 0x33445566;
        dns_local_cache_test_a(adrIn, "url.3.com");

        adrIn = 0x44556677;
        dns_local_cache_test_a(adrIn, "url.4.com");

        adrIn = 0x55667788;
        dns_local_cache_test_a(adrIn, "url.5.com");

        adrOut = dns_local_cache_get("url.1.com");
        Printf(" dns_local_cache_get() Got  0x%08x\r\n", adrOut);
        adrOut = dns_local_cache_get("url.2.com");
        Printf(" dns_local_cache_get() Got  0x%08x\r\n", adrOut);
        adrOut = dns_local_cache_get("url.3.com");
        Printf(" dns_local_cache_get() Got  0x%08x\r\n", adrOut);
        adrOut = dns_local_cache_get("url.4.com");
        Printf(" dns_local_cache_get() Got  0x%08x\r\n", adrOut);

        adrOut = dns_local_cache_get("url.5.com");
        Printf(" dns_local_cache_get() Got  0x%08x\r\n", adrOut);
    }
}

