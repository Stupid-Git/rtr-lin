//#include "_r500_config.h"
#include "tx_api.h"
#include "nx_api.h"


#define _R500SUB_C_
#include "R500sub.h"

//#include "r500_defs.h"

//eFDP_t trigger_what_first_download_processing;

bool first_download_flag_crc_NG(char* Banner)
{
    uint32_t _crc = crc32_bfr(&first_download_flag, sizeof(first_download_flag));


    if(  (Banner != NULL)
      && (_crc != first_download_flag_crc)
    ) {
        Printf("FDF CRC Check: %s\r\n", Banner);
    }

    if( _crc != first_download_flag_crc)
    {
        Printf("FDF crc Error: Value = 0x%08x, calculated = 0x%08x\r\n", first_download_flag_crc, _crc);
        return true;
    }
    /* else
    {
        PrintDbg(TD_INFO, "FDF crc OK: Value = 0x%08x, calculated = 0x%08x\r\n", first_download_flag_crc, _crc);
    } */
    return false;
}

/****************************************************************
//
// 電源ON後、Switch-Run後のデータ吸い上げの範囲設定構造体の初期化
//
*****************************************************************/
void first_download_flag_clear(void)
{
    int i;

    for(i=0;i<BM_F_D_F_SIZE;i++){
        first_download_flag.gn[i] = 0xff;
        first_download_flag.kn[i] = 0xff;
        first_download_flag.flag[i] = 0xff;
    }
    first_download_flag_crc = crc32_bfr(&first_download_flag, sizeof(first_download_flag));
}

/****************************************************************
//
// 最初の吸い上げの範囲、Full or 差分データ 判別フラグの読み出し
//
*****************************************************************/
uint8_t first_download_flag_read(uint8_t G_No , uint8_t U_No)
{
    uint8_t flag = 0xFF;
    int i,j;

    for(i=0;i<BM_F_D_F_SIZE;i++){
        if((first_download_flag.gn[i] == G_No) && (first_download_flag.kn[i] == U_No)){
            flag = first_download_flag.flag[i];
            break;
        }
    }

    Printf(" FDF Read  G:%d / U:%d / F:%02X / at i=%d\r\n", G_No, U_No, flag, i);

    return flag;
}

/****************************************************************
//
// 最初の吸い上げの範囲、Full or 差分データ 判別フラグの書き込み
//
// 既にグループNo.子機No.があれば上書き、無ければ0xFFの所に書き込む
//
*****************************************************************/
void first_download_flag_write(uint8_t G_No , uint8_t U_No, eFDF_t Flag)
{
    uint8_t ret = 0xff;
    int i,j;

    for(i=0;i<BM_F_D_F_SIZE;i++){
        if((first_download_flag.gn[i] == G_No) && (first_download_flag.kn[i] == U_No)){
            first_download_flag.flag[i] = Flag;
            ret = 0;
            break;
        }
    }

    if(ret != 0){
        for(i=0;i<BM_F_D_F_SIZE;i++){
            if((first_download_flag.gn[i] == 0xff) && (first_download_flag.kn[i] == 0xff)){
                first_download_flag.gn[i] = G_No;
                first_download_flag.kn[i] = U_No;
                first_download_flag.flag[i] = Flag;
                break;
            }
        }
    }
    first_download_flag_crc = crc32_bfr(&first_download_flag, sizeof(first_download_flag));


    Printf(" FDF Write G:%d / U:%d / F:%02X / at i=%d / ret=%02X\r\n", G_No, U_No, Flag, i, ret);
}
