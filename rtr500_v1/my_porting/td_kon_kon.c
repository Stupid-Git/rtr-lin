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

static char PrintfTemp[300];


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
                    _just_Printf ("|  %s \n", ascii);
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
                    _just_Printf ("|  %s \n", ascii);
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
    _just_Printf("%s\n", banner);
    DumpHex( 1, pData, (size_t)len);
}




//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static int _printfType (int TYPE);
static int _printfType (int TYPE)
{
    // https://tools.paco.bg/14/
    int done = 0;
    char _buffer[20];
    switch(TYPE)
    {
        case TD_EVENT:
            strcpy(_buffer, "\x1b[38;5;10m"); //10 Bright Green
            break;
        case TD_INFO:
            strcpy(_buffer, "\x1b[38;5;14m"); //14 Bright Light Blue (Cyan)
            break;
        case TD_DEBUG:
            strcpy(_buffer, "\x1b[38;5;12m"); //12 Purple
            break;
        case TD_WARN:
            strcpy(_buffer, "\x1b[38;5;11m"); //11 Bright Yellow
            break;
        case TD_ERROR:
            strcpy(_buffer, "\x1b[38;5;9m"); //9 Bright Red
            break;
        case TD_PINK:
            strcpy(_buffer, "\x1b[38;5;13m"); //13 Bright Pink
            break;
        default:
            strcpy(_buffer, "\x1b[38;5;15m"); //15 Bright White
            break;
    }
    if(strlen( _buffer) != 0 )
    {
        _just_Printf( _buffer );
        //Printf( _buffer );
    }
    return done;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int PrintDbg(int TYPE, const char *format, ...)
{
    (void)(TYPE);
    int r = 0;
//#ifndef DBG_TERM

//TODO    _printfType(TYPE);

//TODO _just_Printf("[%s] ", tn_getMyName());

    unsigned int BufferIndex = 0;
    va_list ParamList;
    va_start(ParamList, format);
#if 0
    r = SEGGER_RTT_vprintf(BufferIndex, format, &ParamList);
#else
//    r = UART_RTT_vprintf(BufferIndex, format, &ParamList);
    vsprintf( PrintfTemp, format ,ParamList);
    printf("%s", PrintfTemp);

#endif
    va_end(ParamList);
//TODO    _printfType(TD_NORM);
//#endif
    return r;
}

/* REF
int PrintDbg(int TYPE, const char *format, ...);
int PrintDbg(int TYPE, const char *format, ...)
{
    (void)(TYPE);
#if 1
    int done = 0;
#ifndef DBG_TERM
    va_list args;
    va_start (args, format);
    //SEGGER_RTT_printf(0, format);
    //done = td_debug_vprintf( TYPE, format, arg);
    vsprintf( PrintfTemp, format ,args);
    printf("%s", PrintfTemp);
    va_end( args );
#endif
    return done;
#else
    int r = 0;
#ifndef DBG_TERM
    _printfType(TYPE);
    unsigned int BufferIndex = 0;
    va_list ParamList;
    va_start(ParamList, format);
    r = SEGGER_RTT_vprintf(BufferIndex, format, &ParamList);
    va_end(ParamList);
    _printfType(TD_NORM);
#endif
    return r;
#endif
}
REF */


#endif // CONFIG_USE_TD_KON_KON

