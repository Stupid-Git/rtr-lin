/*
 * Config.c
 *
 *  Created on: 2019/02/18
 *      Author: haya
 *      2019.Dec.26 ビルドワーニング潰し完了
 */


#define CONFIG_C_
#include <stdio.h>
#include <stdlib.h>
#include "Config.h"
#include "tx_api.h"
#include "General.h"

//#undef CONFIG_C_


uint8_t     Dir[192];
int xxx;
int x2[4];

/**
 *
 * @param src
 * @return
 */
uint32_t AdrStr2Long(char *src)
{
    int x1;
    uint32_t ip = 0;
    int i,j,k;
    char dst[16];
    char c;
    char adr[4];
    uint32_t temp[4];
    char buf[16];

    memset(dst, 0, sizeof(dst));

    j = k = 0;

    for(i=0;i<16;i++){
        dst[i] = *src++;
        c = dst[i];
        if((c == 0x2e) || (c == 0x00))
        {
            adr[j] = 0x00;
            sprintf(buf,"%s",adr);
            x1 = atoi(buf);
            temp[k++] = (uint32_t)x1;
            j = 0;
            if(k>4){
                break;
            }
        }
        else{
            adr[j] = c;
            j++;
        }

        if(c == 0){
            break;
        }
    }


    ip = (temp[0] << 24) + (temp[1] << 16) + (temp[2] << 8) + temp[3];

    Printf("\nip: %08X\n", ip);

    return ( ip );
}

/**
 *
 * @param src
 * @return
 */
uint32_t AdrStr2Long4(char *src)
{
    uint32_t temp[4];
    uint32_t ip = 0;

    temp[3] = (uint32_t)(*src++);
    temp[2] = (uint32_t)(*src++);
    temp[1] = (uint32_t)(*src++);
    temp[0] = (uint32_t)(*src++);

    ip = (temp[0] << 24) + (temp[1] << 16) + (temp[2] << 8) + temp[3];

    Printf("\nip2: %ld\n", ip);

    return ( ip );
}
