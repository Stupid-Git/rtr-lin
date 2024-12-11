#ifndef R500SUB_H_
#define R500SUB_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef  EDF
#undef  EDF
#endif

#ifdef  _R500SUB_C_
#define EDF
#else
#define EDF extern
#endif

typedef enum {
    FDP_TRIG_NONE_0 = 0,
    FDP_TRIG_STBYtoRUN_1 = 1,
    FDP_TRIG_DO_ALL_2 = 2,
    FDP_TRIG_FORCE_ALL_3 = 3,
} eFDP_t;

typedef enum {
    FDF_SABUN_0 = 0,
    FDF_ZENBU_1 = 1,
} eFDF_t;

#define BM_F_D_F_SIZE 128

EDF struct def_first_download_flag{                             //  2016 05 18
    uint8_t gn[BM_F_D_F_SIZE];
    uint8_t kn[BM_F_D_F_SIZE];
    eFDF_t  flag[BM_F_D_F_SIZE];
} first_download_flag                   __attribute__((section(".xram")));
EDF uint32_t first_download_flag_crc    __attribute__((section(".xram")));


EDF eFDP_t trigger_what_first_download_processing;
//extern eFDP_t trigger_what_first_download_processing;

EDF bool first_download_flag_crc_NG(char* Banner);
EDF void first_download_flag_clear(void);
EDF uint8_t first_download_flag_read(uint8_t G_No , uint8_t U_No);
EDF void first_download_flag_write(uint8_t G_No , uint8_t U_No, eFDF_t Flag);


//EDF bool first_do_FDF_Init;           // 2023.05.26

#endif /* R500SUB_H_ */
