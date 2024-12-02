#ifndef TD_KON_STUB_H_
#define TD_KON_STUB_H_

#include "_r500_config.h"

#if CONFIG_USE_TD_KON_STUB

#include "stdint.h"
#include "stdbool.h"

#undef EDF

#ifdef _TD_KON_STUB_C_
#define EDF
#else
#define EDF extern
#endif

inline void Printf(const char *fmt, ...) __attribute__((always_inline));
inline void Printf(const char *fmt, ...)
{
    (void) (fmt);
    //return 0;
}

inline void Printf_StartUp(void) __attribute__((always_inline));
inline void Printf_StartUp(void)
{
}

inline void Printf_Shutdown (void) __attribute__((always_inline));
inline void Printf_Shutdown (void)
{
}


/*EDITING*/
#define TD_NORM  0
#define TD_EVENT 1
#define TD_INFO  2
#define TD_DEBUG 3
#define TD_WARN  4
#define TD_ERROR 5
#define TD_PINK  6

inline int PrintDbg(int TYPE, const char *format, ...) __attribute__((always_inline));
inline int PrintDbg(int TYPE, const char *format, ...)
{
    (void)(TYPE);
    (void)(format);
    return 0;
}
#define td_Dbg PrintDbg
/*
inline int td_Dbg( int i, char* format, ...)
{
    (void)(i);
    (void)(format);
    return 0;
}
*/
#define td_printf Printf
/*
inline int td_printf(char* format, ...)
{
    (void)(format);
    return 0;
}
*/
inline int td_printfRaw(char* format, ...) __attribute__((always_inline));
inline int td_printfRaw(char* format, ...)
{
    (void)(format);
    return 0;
}
inline int UART_PRINT(char* format, ...) __attribute__((always_inline));
inline int UART_PRINT(char* format, ...)
{
    (void)(format);
    return 0;
}

inline int td_printHex(char* format, ...) __attribute__((always_inline));
inline int td_printHex(char* format, ...)
{
    (void)(format);
    return 0;
}

inline void PrintHex( char* banner, uint8_t* pData, int len) __attribute__((always_inline));
inline void PrintHex( char* banner, uint8_t* pData, int len)
{
    (void)(banner);
    (void)(pData);
    (void)(len);
}



#endif // CONFIG_USE_TD_KON_STUB

#endif /* TD_KON_STUB_H_ */

