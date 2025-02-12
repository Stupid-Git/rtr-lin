#include "_r500_config.h"

#if CONFIG_USE_TD_KON_STUB

#define _TD_KON_STUB_C_
#include "td_kon_stub.h"

/*
#include "SEGGER_RTT.h"

#include "stdarg.h"


//static char PrintfTemp[300];

#undef printf
#define printf(...)     SEGGER_RTT_printf(0, __VA_ARGS__)


void Printf( const char *format, ... )
{
#if 0
    va_list args;
    va_start( args, format );
#ifndef DBG_TERM
    //SEGGER_RTT_printf(0, format);
    vsprintf( PrintfTemp, format ,args);
    printf("%s", PrintfTemp);
#endif
    va_end( args );
#else
    //int r;
    unsigned int BufferIndex = 0;
    va_list ParamList;
    va_start(ParamList, format);
    //r =
    SEGGER_RTT_vprintf(BufferIndex, format, &ParamList);
    va_end(ParamList);
    //return r;
#endif
}

void Printf_Shutdown(void)
{
    SEGGER_RTT_BUFFER_UP *pRing;
    pRing = &_SEGGER_RTT.aUp[0];
    while (pRing->WrOff != pRing->RdOff)
    {
        tx_thread_sleep (1);
    }
}



//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static int _printfType (int TYPE);
static int _printfType (int TYPE)
{
    int done;
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
        default:
            strcpy(_buffer, "\x1b[38;5;15m"); //15 Bright White
            break;
    }
    if(strlen( _buffer) != 0 )
    {
        Printf( _buffer );
    }
    return done;
}

int PrintDbg(int TYPE, const char *format, ...);
int PrintDbg(int TYPE, const char *format, ...)
{
    (void)(TYPE);
#if 0
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


//=============================================================================

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
        Printf("%02X ", ((unsigned char*)data)[i]);
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
                Printf (" ");
                if ((i + 1) % 16 == 0)
                {
                    Printf ("|  %s \r\n", ascii);
                }
                else if (i + 1 == size)
                {
                    ascii[(i + 1) % 16] = '\0';
                    if ((i + 1) % 16 <= 8)
                    {
                        Printf (" ");
                    }
                    for (j = (i + 1) % 16; j < 16; ++j)
                    {
                        Printf ("   ");
                    }
                    Printf ("|  %s \r\n", ascii);
                }
            }
        }
    }
    if(addText == 0)
    {
        Printf("\r\n");
    }
}

void PrintHex( char* banner, uint8_t* pData, int len)
{
    Printf("%s\r\n", banner);
    DumpHex( 1, pData, (size_t)len);
}
*/

void new_uart6_thread_entry(void);
void new_uart6_thread_entry(void)
{
    tx_thread_sleep(100);
}
#endif // CONFIG_USE_TD_KON_STUB

