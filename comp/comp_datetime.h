/*
 * comp_datetime..h
 *
 *  Created on: Dec 2, 2024
 *      Author: karel
 */

#ifndef COMP_DATETIME_H_
#define COMP_DATETIME_H_


#include "_r500_config.h"

/***********************************************************************************************************************
Includes
***********************************************************************************************************************/
//#include "bsp_api.h"
//#include "tx_api.h"
//#include "hal_data.h"
//#include "r_rtc_api.h"

typedef struct tm rtc_time_t;

#include "r500_defs.h"




#ifdef EDF
#undef EDF
#endif
#ifdef _COMP_DATETIME_C_
#define EDF
#else
#define EDF extern
#endif




/**********************************************************************************************************************
Macro definitions
***********************************************************************************************************************/




#define UNIX_BASE_SEC   946684800       ///< 2000/01/01 00:00:00 GMT の秒
//#define RUN_BASE_SEC  1262304000      // 2010/01/01 00:00:00 GMT の秒
#define RUN_BASE_SEC    1546300800      ///<  2019/01/01 00:00:00 GMT の秒


/***********************************************************************************************************************
Function prototypes
**********************************************************************************************************************/


//EDF void RTC_Create(rtc_time_t *tm);
EDF void RTC_Create(void);
//EDF rtc_time_t RTC_GetGMT( void );
EDF void RTC_GetGMT( rtc_time_t *Tm );
//EDF rtc_time_t RTC_GetGMT( rtc_time_t *tm );
EDF uint32_t RTC_GetGMTSec( void );
EDF uint32_t RTC_Date2GSec( rtc_time_t *Tm );
EDF uint32_t RTC_Date2GDay( rtc_time_t *Tm );

//EDF long RTC_GetGMTSec( void );
EDF bool RTC_GSec2Date( uint32_t Secs, rtc_time_t *Tm );
EDF bool RTC_GSec2LCT( uint32_t GSec, rtc_time_t *Val );
EDF bool RTC_GetLCT( rtc_time_t *Val );

EDF int RTC_GetFormStr( rtc_time_t *Tm, char *Dst, char *TForm );

EDF uint32_t GetSummerAdjust( uint32_t GSecs );

EDF bool SetDST( int Year );
EDF uint32_t RTC_GetLCTSec( uint32_t GSec );

EDF /*ssp_err_t*/uint32_t with_retry_calendarTimeGet(char* caption, rtc_time_t *p_rtc_t);   // 2022.09.09


/*　　使わない
typedef struct {
    uint8_t Sec;
    uint8_t Min;
    uint8_t Hour;
    uint8_t Week;
    uint8_t Day;
    uint8_t Month;
    uint8_t Year;
} RTC_Struct;
*/

// rtr_struct_tの構造体
/*
struct tm
{
  int   tm_sec;
  int   tm_min;
  int   tm_hour;
  int   tm_mday;
  int   tm_mon;
  int   tm_year;
  int   tm_wday;
  int   tm_yday;
  int   tm_isdst;
#ifdef __TM_GMTOFF
  long  __TM_GMTOFF;
#endif
#ifdef __TM_ZONE
  const char *__TM_ZONE;
#endif
};
*/

/**
 * @note    パディングあり
 */
typedef struct obj_rtc_st
{


    int32_t Diff;
    int32_t AdjustSec;

    char    SummerBase;     ///<  サマータイムの基準
                            ///<  0:無し  1:ローカル  2:GMT

      rtc_time_t    SummerIN;   ///<  サマータイム(IN) (GMT基準に変換してセット）
      rtc_time_t    SummerOUT;  ///<  サマータイム(OUT) (GMT基準に変換してセット）
                            // ※INとOUTの年は同じである事。
} OBJ_RTC;

EDF OBJ_RTC RTC;



#endif /* COMP_DATETIME_H_ */
