/*
 * td_kon_kon.c
 *
 *  Created on: Nov 25, 2024
 *      Author: karel
 */

#include "_r500_config.h"
#if CONFIG_USE_TD_KON_KON

#include "unistd.h"

#define _TD_KON_KON_C_
#include "td_kon_kon.h"

#include "r500_defs.h"

#include "stdio.h"

#define _just_Printf printf

#define Printf printf


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

#endif // CONFIG_USE_TD_KON_KON

