//#include "_r500_config.h"
#include "MyDefine.h"
#if CONFIG_NEW_STATE_CTRL

#define _NEW_G_C_
#include "new_g.h"

//#include "r500_defs.h"
#include "R500sub.h"
#include "Globals.h"
#include "Error.h"
#include "Sfmem.h"
#include "Monitor.h"
#include "rf_thread_entry.h"
#include "Rfunc.h"
//#include "Config.h"
//#include "General.h"

//static def_root_registry _root_registry;
static def_group_registry _group_registry;
/*
 * SFM_REGIST_START         | def_root_registry     | 16 bytes
 * SFM_REGIST_START + 16    | def_group_registry    | 48 bytes
 *
 *
 */

/*
 * # RTR-574 Full download failure
 * # POWER FAILURE low batt / low USB + good batteries / ...
 *
 *
 *
 */

/*
 * STANGE ITEMS
 *  auto_control.crc = 0; <- CFWriteSettingsFile(void) WSETF
 *
 * .rtr501.set_flag
 *
 *  keiho = setinfo & 0x03;       // 警報監視設定 <- CFWriteUnitSettingsFile(void) WREGI
 *  kiroku = setinfo & 0x0C;      // 記録データ送信 <- CFWriteUnitSettingsFile(void) WREGI
 *
 *  first_do_FDF_Init
 *
 *  mate_at_start_reset
 *
 *  mate_at_start
 *
 */

/*
 * # FULL DOWNLOAD ALL
 * 1. Switch S->R Clear FDF flag -> if my_config.suction.Range[0]; //DRNG -> FDF = 0xFF->0x01 else 0xFF->0x00
 *    first_do_FDF_Init = true; -> in AT_start Clear FDF -> OLD will not exist -> create new with value 0xFF
 *
 */

/*
 * # FULL DOWNLOAD KOKI
 * 1. Set a Flag along with Serial Number
 *    in AT_start, after creating kk_all_new match serial number and set FDF = ZENBU
 *
 */

/*
 * # new AT_start(void)
 * chk_sram_ng (1) auto_control.download is corrupt
 * chk_sram_ng (2) auto_control.download is corrupt
 * chk_sram_ng (3) auto_control.w_config is corrupt
 * chk_sram_ng (4) auto_control.w_config is corrupt
 * chk_sram_ng (5) auto_control.crc is corrupt
 * chk_sram_ng (6) if(mate_at_start_reset == 1)
 *
 */

#define NCS_HAS_FDF 1

//void kk_delta_clear_all(void);
//void kk_delta_clear(uint32_t serial_number);
//void kk_delta_set(uint32_t serial_number, uint8_t flagX);
//bool kk_delta_get(uint32_t serial_number, uint8_t* p_flagX);

#define KK_INFO_MAX 128

typedef struct
{
    uint8_t  group_no;
    uint8_t  unit_no;
    uint32_t SerialNo;
    uint16_t interval;      // 子機設定_記録間隔（秒）             // 2023.05.26    
    uint8_t  set_flag;      // 子機設定_記録データ送信（bit2,3）   // 2023.05.26
    uint8_t  w_config_noset;    // w_configにセットしない         // 2023.07.24 
#if NCS_HAS_FDF
    struct
    {
        uint8_t gn;
        uint8_t kn;
        eFDF_t  fdf_flag;
    } fdf;
#endif
    struct
    {
        uint32_t siri;
        uint16_t data_no;
             bool assigned;
    } download;

    struct
    {
        uint8_t  group_no;                                   ///<  グループ番号
        uint8_t  unit_no;                                    ///<  子機番号
        uint16_t Now;
        uint16_t Before;
        uint16_t On_Off;
    } w_config;

    def_a_warning                                                                                                                                                                                                                                       warning;
} kk_info_t;



kk_info_t kk_all_new[KK_INFO_MAX] __attribute__((section(".xram")));
kk_info_t kk_all_old[KK_INFO_MAX] __attribute__((section(".xram")));
uint32_t kk_all_new_crc __attribute__((section(".xram")));

//def_a_download auto_control_download_temp[KK_INFO_MAX] __attribute__((section(".xram"))); // DRNG helper

void nsc_new_to_old(void);
void nsc_new_to_old(void)
{
    int idx_new;
    uint32_t _crc;

    // copy to OLD
    memcpy(kk_all_old, kk_all_new, sizeof(kk_all_new));

    // if new crc is NG the initializse it and then copy to OLD again
    _crc = crc32_bfr(&kk_all_new, sizeof(kk_all_new));
    if(kk_all_new_crc != _crc) {

        Printf("kk_all_new_crc NG kk_all_new_crc = 0x%08x calc_crc = 0x%08x\r\n", kk_all_new_crc, _crc), memset(kk_all_new, 0, sizeof(kk_all_new));
        for(idx_new = 0; idx_new < KK_INFO_MAX ; idx_new++) {
            kk_all_new[idx_new].SerialNo = 0x00000000;
            kk_all_new[idx_new].group_no = 0xFF;
            kk_all_new[idx_new].unit_no = 0xFF;
            kk_all_new[idx_new].interval = 0x00;        // 2023.05.26
            kk_all_new[idx_new].set_flag = 0x00;        // 2023.05.26
            kk_all_new[idx_new].w_config_noset = 0x00;  // 2023.07.24
        }
        kk_all_new_crc = crc32_bfr(&kk_all_new, sizeof(kk_all_new));

        memcpy(kk_all_old, kk_all_new, sizeof(kk_all_new));
    }

}

void nsc_build_new_from_registry(void);
void nsc_build_new_from_registry(void)
{

    int group; // uint8_t 0~4
    int group_max; // uint8_t 0~4

    uint32_t adr, len;

    int pos;
    int idx_new;
    char j;

    // Clear all entries
    memset(kk_all_new, 0, sizeof(kk_all_new));
    for(idx_new = 0; idx_new < KK_INFO_MAX ; idx_new++) {
        kk_all_new[idx_new].SerialNo = 0x00000000;
        kk_all_new[idx_new].group_no = 0xFF;
        kk_all_new[idx_new].unit_no = 0xFF;
        kk_all_new[idx_new].interval = 0x00;            // 2023.05.26
        kk_all_new[idx_new].set_flag = 0x00;            // 2023.05.26
        kk_all_new[idx_new].w_config_noset = 0x00;      // 2023.07.24
    }

    // Set Base Device Group / Unit
    pos = 0;
    kk_all_new[pos].group_no = 0x00;
    kk_all_new[pos].unit_no = 0x00;
    pos++;
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    def_group_registry *read_group_registry = &_group_registry;

    //group_max = get_regist_group_size();                            // 最大グループ番号（グループ数）
    group_max = RF_get_regist_group_size(&adr);                     // 最大グループ番号（グループ数）   // sakaguchi 2020.09.01

#if CONFIG_USE_NEW_SF_SEMAPHORE
    if(get_mutex_sfmem(GMSF_PARAM) == TX_SUCCESS)
#else
    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS) // OLD
#endif
    {
        //Printf("\r\n");
        //Printf("\r\n");

        for(group = 1; group <= group_max ; group++) {

            serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), sizeof(def_group_registry), (char*)read_group_registry);  // グループ情報構造体に読み込む

            adr += read_group_registry->length;                       // グループヘッダ分アドレスＵＰ

            for(j = 0; j < read_group_registry->altogether ; j++)      // 中継機情報分アドレスＵＰ
                    {
                serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), sizeof(rpt_registry), (char*)&rpt_registry); // rpt_registry is global
                //Printf("Repeater length = %d\r\n", rpt_registry.length);
                //Printf("Repeater serial = 0x%08X\r\n", rpt_registry.serial);

                serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), 2, (char*)&len);  // 中継機情報サイズを読み込む
                adr += (len & 0x0000ffff);
            }

            for(j = 0; j < read_group_registry->remote_unit ; j++)     // 子機情報分アドレスＵＰ
                    {

                serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), sizeof(def_ru_registry), (char*)&ru_registry); // ru_registry is global
                //Printf("Unit length = %d\r\n", ru_registry.rtr501.length);
                //Printf("Unit serial = 0x%08X\r\n", ru_registry.rtr501.serial);
                //Printf("Unit number = 0x%08X\r\n", ru_registry.rtr501.number);

                //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                kk_all_new[pos].group_no = (uint8_t)group;
                kk_all_new[pos].unit_no = ru_registry.rtr501.number;
                kk_all_new[pos].SerialNo = ru_registry.rtr501.serial;
                kk_all_new[pos].interval = ru_registry.rtr501.interval;                 // 2023.05.26
                kk_all_new[pos].set_flag = ru_registry.rtr501.set_flag & 0x0C;          // 2023.05.26
                kk_all_new[pos].w_config_noset = 0x00;              // 2023.07.24
                pos++;
                //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

                if((ru_registry.rtr501.header == 0xFA) ||  // rtr-574
                        (ru_registry.rtr501.header == 0xF9)) // rtr-576
                        {
                    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                    kk_all_new[pos].group_no = (uint8_t)group;
                    kk_all_new[pos].unit_no = (uint8_t)(ru_registry.rtr501.number + 1);
                    kk_all_new[pos].SerialNo = ru_registry.rtr501.serial;
                    kk_all_new[pos].interval = ru_registry.rtr501.interval;            // 2023.05.26
                    kk_all_new[pos].set_flag = ru_registry.rtr501.set_flag & 0x0C;     // 2023.05.26
                    kk_all_new[pos].w_config_noset = 0x01;      // 2CH機の後半番号はw_configにセットしない // 2023.07.24
                    pos++;
                    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                }

                serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), 2, (char*)&len);  // 子機情報サイズを読み込む
                adr += (len & 0x0000ffff);
            }
        }

        Printf("\r\n");
        Printf("\r\n");

#if CONFIG_USE_NEW_SF_SEMAPHORE
        put_mutex_sfmem();
#else
        tx_mutex_put(&mutex_sfmem);
#endif

    }

}

void nsc_collect_old_data(void);
void nsc_collect_old_data(void)
{
    int idx_new;
    int idx_old;

    uint8_t G_new;
    uint8_t U_new;
    uint32_t Serial;

    uint8_t G_old;
    uint8_t U_old;

    for(idx_new = 0; idx_new < KK_INFO_MAX ; idx_new++) {
        if(kk_all_new[idx_new].group_no == 0xFF)
            break;
        if(kk_all_new[idx_new].unit_no == 0xFF)
            break;

        if(kk_all_new[idx_new].group_no == 0x00) { // Is Base

            G_old = 0;
            U_old = 0;
            // copy w_config info
            for(idx_old = 0; idx_old < KK_INFO_MAX ; idx_old++) {

                if((auto_control.w_config[idx_old].group_no == G_old) && (auto_control.w_config[idx_old].unit_no == U_old)) {
                    memcpy(&kk_all_new[idx_new].w_config, &auto_control.w_config[idx_old], sizeof(def_w_config));
                    break;
                }
            }
            if(idx_old == KK_INFO_MAX) { // no entry found
                Printf("No Old W_config Found G_old = %d, U_old = %d\r\n", G_old, U_old);
            }
        }

        if(kk_all_new[idx_new].group_no != 0x00) { // Not Base

            G_new = kk_all_new[idx_new].group_no;
            U_new = kk_all_new[idx_new].unit_no;
            Serial = kk_all_new[idx_new].SerialNo;

            kk_all_new[idx_new].download.siri = Serial;
            kk_all_new[idx_new].download.data_no = 0;
            kk_all_new[idx_new].download.assigned = false;

            //kk_all_new[idx_new].warning.* = 0;
            //kk_all_new[idx_new].w_config.* = 0;

            G_old = 0xFF;
            U_old = 0xFF;
            for(idx_old = 0; idx_old < KK_INFO_MAX ; idx_old++) {
                if(kk_all_old[idx_old].SerialNo == Serial) {
                    G_old = kk_all_old[idx_old].group_no;
                    U_old = kk_all_old[idx_old].unit_no;
// 2023.05.26 ↓ 
                    // 記録間隔と記録データ送信有無の設定に変化がある場合、全データ送信とする
                    if((kk_all_old[idx_old].interval != kk_all_new[idx_new].interval)
                    || (kk_all_old[idx_old].set_flag != kk_all_new[idx_new].set_flag))
                    {
                        kk_delta_set(kk_all_old[idx_old].SerialNo, 0x01);     // 変化あり
                    }
// 2023.05.26 ↑
                    // We have devices like the RTR574, RTR576 which have two entries with the
                    // same serial #, and if we don't "invalidate" the old one, then we wiil hit it
                    // twice and et the wrong G_old and U_old
                    kk_all_old[idx_old].SerialNo = 0xFFFFFFFF;

                    break;
                }
            }
            if(idx_old == KK_INFO_MAX) { // no entry found
                Printf("No Old Serial Found\r\n");
#if NCS_HAS_FDF
                kk_all_new[idx_new].fdf.gn = G_new;
                kk_all_new[idx_new].fdf.kn = U_new;
                kk_all_new[idx_new].fdf.fdf_flag = 0xFF; //FDF_ZENBU_1;
                //first_download_flag_write(G_new, U_new, FDF_ZENBU_1);
#endif
                continue;
            }

#if NCS_HAS_FDF
            // copy old fdf info
            for(idx_old = 0; idx_old < KK_INFO_MAX ; idx_old++) {
                if((first_download_flag.gn[idx_old] == G_old) && (first_download_flag.kn[idx_old] == U_old)) {
                    kk_all_new[idx_new].fdf.gn = G_new;
                    kk_all_new[idx_new].fdf.kn = U_new;
                    kk_all_new[idx_new].fdf.fdf_flag = first_download_flag.flag[idx_old];
                    break;
                }
            }
            if((idx_old == KK_INFO_MAX)               // Was a new device
            // | (G_new != G_old) | (U_new != U_old)     // Has been re-registered
            ) {
                kk_all_new[idx_new].fdf.gn = G_new;
                kk_all_new[idx_new].fdf.kn = U_new;
                kk_all_new[idx_new].fdf.fdf_flag = 0xFF; // FDF_ZENBU_1;
                //first_download_flag_write(G_new, U_new, FDF_ZENBU_1);
            }

#if 1
            uint8_t _flag;
            if(kk_delta_get(kk_all_new[idx_new].SerialNo, &_flag))
            {
                if(_flag & 0x01){
                    kk_all_new[idx_new].fdf.fdf_flag = FDF_ZENBU_1;

                    //kk_delta_set(kk_all_new[idx_new].SerialNo, _flag & 0xFE);
                    //kk_delta_set(kk_all_new[idx_new].SerialNo, 0x00);
                    //kk_delta_clear(kk_all_new[idx_new].SerialNo);     // 2CH機があるためここでは消さず最後に全消去する
                }
            }
#endif

#endif
            // copy old download info
            for(idx_old = 0; idx_old < KK_INFO_MAX ; idx_old++) {
                if((auto_control.download[idx_old].group_no == G_old) && (auto_control.download[idx_old].unit_no == U_old)) {
                    memcpy(&kk_all_new[idx_new].download.siri, &Serial, 4); // not needed as Serial already matches
                    memcpy(&kk_all_new[idx_new].download.data_no, &auto_control.download[idx_old].data_no, 2);
                    kk_all_new[idx_new].download.assigned = true;
                    break;
                }
            }
            if(idx_old == KK_INFO_MAX) { // no entry found
                Printf("No Old Download Found G_old = %d, U_old = %d\r\n", G_old, U_old);
            }

            // copy warning info
            for(idx_old = 0; idx_old < KK_INFO_MAX ; idx_old++) {
                if((auto_control.warning[idx_old].group_no == G_old) && (auto_control.warning[idx_old].unit_no == U_old)) {
                    memcpy(&kk_all_new[idx_new].warning, &auto_control.warning[idx_old], sizeof(def_a_warning));
                    break;
                }
            }
            if(idx_old == KK_INFO_MAX) { // no entry found
                Printf("No Old Warning Found G_old = %d, U_old = %d\r\n", G_old, U_old);
            }

            // copy w_config info
            for(idx_old = 0; idx_old < KK_INFO_MAX ; idx_old++) {

                if((auto_control.w_config[idx_old].group_no == G_old) && (auto_control.w_config[idx_old].unit_no == U_old)) {
                    memcpy(&kk_all_new[idx_new].w_config, &auto_control.w_config[idx_old], sizeof(def_w_config));
                    break;
                }
            }
            if(idx_old == KK_INFO_MAX) { // no entry found
                Printf("No Old W_config Found G_old = %d, U_old = %d\r\n", G_old, U_old);
            }
        }
    }

    kk_all_new_crc = crc32_bfr(&kk_all_new, sizeof(kk_all_new));

    kk_delta_clear_all();       // 2023.05.26

}

void nsc_recreate_states(void);
void nsc_recreate_states(void)
{
    int idx_new;
    int j;

    // clear current state
    for(j = 0; j < KK_INFO_MAX ; j++) {
        auto_control.download[j].group_no = 0xFF;
        auto_control.download[j].unit_no = 0xFF;
        auto_control.warning[j].group_no = 0xFF;
        auto_control.warning[j].unit_no = 0xFF;
        auto_control.w_config[j].group_no = 0xFF;
        auto_control.w_config[j].unit_no = 0xFF;
    }
#if NCS_HAS_FDF
    for(j = 0; j < KK_INFO_MAX ; j++) {
        first_download_flag.gn[j] = 0xFF;
        first_download_flag.kn[j] = 0xFF;
        first_download_flag.flag[j] = 0xFF;
    }
#endif

    for(idx_new = 0; idx_new < KK_INFO_MAX ; idx_new++) {

        if(kk_all_new[idx_new].group_no == 0xFF)
            break;
        if(kk_all_new[idx_new].unit_no == 0xFF)
            break;

        if(kk_all_new[idx_new].group_no == 0x00) { // Is Base

            // copy w_config info
            for(j = 0; j < KK_INFO_MAX ; j++) {

                if((auto_control.w_config[j].group_no == 0xFF) && (auto_control.w_config[j].unit_no == 0xFF)) {
                    auto_control.w_config[j].group_no = 0;
                    auto_control.w_config[j].unit_no = 0;

                    memcpy(&auto_control.w_config[j].now[0], &kk_all_new[idx_new].w_config.Now, 2);
                    memcpy(&auto_control.w_config[j].before[0], &kk_all_new[idx_new].w_config.Before, 2);
                    memcpy(&auto_control.w_config[j].on_off[0], &kk_all_new[idx_new].w_config.On_Off, 2);

                    break;
                }
            }
            if(j == KK_INFO_MAX) { // no entry found
                Printf("No Room for w_config\r\n");
            }
        }

        if(kk_all_new[idx_new].group_no != 0x00) { // Is Not Base

#if NCS_HAS_FDF
            // copy old fdf info
            for(j = 0; j < KK_INFO_MAX ; j++) {
                if((first_download_flag.gn[j] == 0xFF) && (first_download_flag.kn[j] == 0xFF)) {
                    first_download_flag.gn[j] = kk_all_new[idx_new].fdf.gn;
                    first_download_flag.kn[j] = kk_all_new[idx_new].fdf.kn;
                    first_download_flag.flag[j] = kk_all_new[idx_new].fdf.fdf_flag;
                    break;
                }
            }
#endif
            // copy old download info
            for(j = 0; j < KK_INFO_MAX ; j++) {
                if((auto_control.download[j].group_no == 0xFF) && (auto_control.download[j].unit_no == 0xFF)) {
                    memcpy(&auto_control.download[j].siri, &kk_all_new[idx_new].download.siri, 4);
                    memcpy(&auto_control.download[j].data_no, &kk_all_new[idx_new].download.data_no, 2);
                    auto_control.download[j].group_no = kk_all_new[idx_new].group_no;
                    auto_control.download[j].unit_no = kk_all_new[idx_new].unit_no;
                    break;
                }
            }
            if(j == KK_INFO_MAX) { // no entry found
                Printf("No Room for download\r\n");
            }

            // copy warning info
            for(j = 0; j < KK_INFO_MAX ; j++) {
                if((auto_control.warning[j].group_no == 0xFF) && (auto_control.warning[j].unit_no == 0xFF)) {

                    memcpy(&auto_control.warning[j], &kk_all_new[idx_new].warning, sizeof(def_a_warning));
                    auto_control.warning[j].group_no = kk_all_new[idx_new].group_no;
                    auto_control.warning[j].unit_no = kk_all_new[idx_new].unit_no;
                    break;
                }
            }
            if(j == KK_INFO_MAX) { // no entry found
                Printf("No Room for warning\r\n");
            }

            // copy w_config info
            for(j = 0; j < KK_INFO_MAX ; j++) {
                if((auto_control.w_config[j].group_no == 0xFF) && (auto_control.w_config[j].unit_no == 0xFF)) {

                    if( kk_all_new[idx_new].w_config_noset ){   // 2023.07.24 2CH機の後半番号はw_configにセットしない
                        continue;
                    }

                    memcpy(&auto_control.w_config[j].now[0], &kk_all_new[idx_new].w_config.Now, 2);
                    memcpy(&auto_control.w_config[j].before[0], &kk_all_new[idx_new].w_config.Before, 2);
                    memcpy(&auto_control.w_config[j].on_off[0], &kk_all_new[idx_new].w_config.On_Off, 2);
                    auto_control.w_config[j].group_no = kk_all_new[idx_new].group_no;
                    auto_control.w_config[j].unit_no = kk_all_new[idx_new].unit_no;
                    break;
                }
            }
            if(j == KK_INFO_MAX) { // no entry found
                Printf("No Room for w_config\r\n");
            }

        }

    }
#if 0 // XTRA
    for(idx_new = 0; idx_new < KK_INFO_MAX ; idx_new++) {

        if(kk_all_new[idx_new].group_no == 0xFF)
            break;
        if(kk_all_new[idx_new].unit_no == 0xFF)
            break;

        if((kk_all_new[idx_new].group_no != 0x00) && (kk_all_new[idx_new].unit_no != 0x00))
        {
            uint8_t _flag;
            if(kk_delta_get(kk_all_new[idx_new].SerialNo, &_flag))
            {
                if(_flag == 0)
                    continue;
                if(_flag & 0x01){
                    kk_all_new[idx_new].fdf.fdf_flag = FDF_ZENBU_1;
                }
            }
        }
    }
#endif
    auto_control.crc = crc32_bfr(&auto_control, 5122);      // ((128*8)*5)+2    CRC追加
    first_download_flag_crc = crc32_bfr(&first_download_flag, sizeof(first_download_flag));

}

static void debug_print_download(void);
static void debug_print_download(void)
{
    def_a_download *p_dl;
    int i;

    Printf("\r\ndebug_print_download START\r\n");
    for(i = 0; i < 128 ; i++) {
        p_dl = &auto_control.download[i]; // @ debug_print_download
        /**/
        if(p_dl->group_no == 0x00) {
            Printf(" Zero Group at index = %d\r\n", i);
            continue;
        }
        if(p_dl->group_no == 0xff)
            break;
        /**/
        Printf("  group_no = %d\r\n", p_dl->group_no);
        Printf("  unit_no  = %d\r\n", p_dl->unit_no);
        Printf("  serial   = 0x%08x\r\n", *(uint32_t*)&p_dl->siri[0]);
        Printf("  data_no  = %d\r\n", *(uint16_t*)&p_dl->data_no[0]);
        /*
         if(p_dl->group_no == 0x00)
         continue;
         if(p_dl->group_no == 0xff)
         break;
         */
    }
    Printf("\r\ndebug_print_download END\r\n");
}

// 2023.05.26 ↓
#if 0
static void debug_print_download_temp(void);
static void debug_print_download_temp(void)
{
    def_a_download *p_dl;
    int i;

    Printf("\r\ndebug_print_download_temp START\r\n");
    for(i = 0; i < 128 ; i++) {
        p_dl = &auto_control_download_temp[i]; // @ debug_print_download_temp
        /**/
        if(p_dl->group_no == 0x00) {
            Printf(" Zero Group at index = %d\r\n", i);
            continue;
        }
        if(p_dl->group_no == 0xff)
            break;
        /**/
        Printf("  group_no = %d\r\n", p_dl->group_no);
        Printf("  unit_no  = %d\r\n", p_dl->unit_no);
        Printf("  serial   = 0x%08x\r\n", *(uint32_t*)&p_dl->siri[0]);
        Printf("  data_no  = %d\r\n", *(uint16_t*)&p_dl->data_no[0]);
        /*
         if(p_dl->group_no == 0x00)
         continue;
         if(p_dl->group_no == 0xff)
         break;
         */
    }
    Printf("\r\ndebug_print_download_temp END\r\n");
}

static void clr_download_temp(void);
static void clr_download_temp(void)
{
    int i;

    for(i = 0; i < 128 ; i++) {
        memset(&auto_control_download_temp[i].group_no, 0xFF, 2);
        memset(auto_control_download_temp[i].siri, 0x00, 6);
    }
}

static uint8_t set_download_temp(uint8_t G_No, uint8_t U_No, uint8_t *siri, uint16_t *p_Dat_No);
static uint8_t set_download_temp(uint8_t G_No, uint8_t U_No, uint8_t *siri, uint16_t *p_Dat_No)
{
    int cnt = 0;
    uint8_t rtn = MD_ERROR;

    Printf("\r\n");
    if((siri == NULL) || (p_Dat_No == NULL)) {
        if(siri == NULL) {
            Printf("set_download_temp: G_No=%d, U_No=%d, (siri=NULL), Dat_No=%d\r\n", G_No, U_No, *p_Dat_No);
        }
        if(p_Dat_No == NULL) {
            Printf("set_download_temp: G_No=%d, U_No=%d, siri=0x%08x, (Dat_No=NULL)\r\n", G_No, U_No, *(uint32_t*)siri);
        }
    } else {
        Printf("set_download_temp: G_No=%d, U_No=%d, siri=0x%08x, Dat_No=%d\r\n", G_No, U_No, *(uint32_t*)siri, *p_Dat_No);
    }

    def_a_download *pDownload = &auto_control_download_temp[0];

    do {
        if((pDownload->group_no == G_No) && (pDownload->unit_no == U_No)) {
            if(siri != NULL)
                memcpy(pDownload->siri, siri, 4);
            if(p_Dat_No != NULL)
                memcpy(pDownload->data_no, p_Dat_No, 2);
            rtn = MD_OK;
            break;
        }
        //else if((pDownload->group_no == 0xff) && (pDownload->unit_no == 0xff))
        else if((pDownload->group_no == 0xff)) {
            pDownload->group_no = G_No;
            pDownload->unit_no = U_No;
            if(siri != NULL)
                memcpy(pDownload->siri, siri, 4);
            if(p_Dat_No != NULL)
                memcpy(pDownload->data_no, p_Dat_No, 2);
            rtn = MD_OK;
            break;
        } else {
            pDownload++;
        }
        cnt++;
    } while(cnt < KK_INFO_MAX);

    //debug_print_download_temp();

    return (rtn);
}

void copy_download_temp_G_U_SerialNumber(void);
void copy_download_temp_G_U_SerialNumber(void)
{
    int size;
    int count;
    MON_FORM *Form;

    uint8_t __group;
    uint8_t __unit;
    uint32_t __serial;

    size = *(uint16_t*)rf_ram.auto_format1.data_byte;  // 先頭の2バイト     キャストに変更 2020.01.21
    Form = (MON_FORM*)&rf_ram.auto_format1.kdata[0];       // データの先頭アドレス

    count = 64;
    while(size >= 64) {          // 64byte * n個

        __group = Form->group_no;
        __unit = Form->unit_no;
        __serial = *(uint32_t*)Form->serial;

        //Printf("copy_download_temp_G_U_SerialNumber: G_No = %d, __group = %d, __serial = 0x%08X\r\n", __group, __unit, __serial);

        set_download_temp(__group, __unit, (uint8_t*)&__serial, NULL);          //&__dataID); // @ copy_download_temp_G_U_SerialNumber()

        Form++;
        size -= count;

    }

    debug_print_download_temp();

}

void process_realscan_Format0(uint8_t G_No);
void process_realscan_Format0(uint8_t G_No)
{
    uint8_t format_no = 0;
    uint8_t U_No;
    uint16_t Dat_No;

    int cnt;
    uint8_t copy_unit_size; // コピー台数
    uint16_t dataByte;

    dataByte = *(uint16_t*)rf_ram.realscan.data_byte;
    copy_unit_size = (uint8_t)(dataByte / 30);      // 今回無線通信できた子機の台数(リアルスキャン応答は子機あたり30byte)

    Printf("### process_realscan_Format0 G_No = %d / count = %d\r\n", G_No, copy_unit_size);

    if(format_no == 0) {
        for(cnt = 0; cnt < copy_unit_size ; cnt++) {
//            PrintHex("----- RealScan 0 -----", (uint8_t*)&rf_ram.realscan.data.format0[cnt], 30); //RSRS
            Printf("----- RealScan 0 -----\r\n");
            Printf("  Grp  No     = %d\r\n", G_No);
            Printf("  Unit No     = %d\r\n", *(uint8_t*)(&rf_ram.realscan.data.format0[cnt].unit_no));
            Printf("  rec_data_No = %d\r\n", *(uint16_t*)(&rf_ram.realscan.data.format0[cnt].rec_data_No));

            U_No = *(uint8_t*)(&rf_ram.realscan.data.format0[cnt].unit_no);
            Dat_No = *(uint16_t*)(&rf_ram.realscan.data.format0[cnt].rec_data_No);
            set_download_temp(G_No, U_No, NULL, &Dat_No);

        }
        return;
    }

}

uint32_t do_realscan_Format0(void);
uint32_t do_realscan_Format0(void)
{
    int i;

    uint8_t group_size, group_cnt;
    uint8_t MaxRealscan;
    //uint8_t MaxPolling;

    uint8_t G_No;
    //uint8_t U_No;

    uint8_t rtn;
    uint8_t retry_cnt;
    union
    {
        uint8_t  byte[2];
        uint16_t word;
    } b_w_chg;

    uint8_t max_unit_work;      // --------------------------------------- リアルスキャンするMAX子機を保存しておく    2016.03.23追加

    Printf("\r\n");
    Printf("do_realscan_Format0 - Start\r\n");
    Printf("\r\n");

    group_size = get_regist_group_size();           // 登録ファイルからグループ取得する　あわせて登録情報ヘッダを読み込み

    //if(1 == (root_registry.auto_route & 0x01)) {     // 自動ルート機能ONの場合
    //    RF_full_moni();                             // フルモニタリング開始
    //}

    for(group_cnt = 0; group_cnt < group_size ; group_cnt++) {      // グループ数ループ

        RF_buff.rf_req.current_group = (char)(group_cnt + 1);       // 通信対象グループ番号を０～指定

        //retry_cnt = 2;      // リトライ回数
        retry_cnt = 0;      // リトライ回数

        get_regist_scan_unit(RF_buff.rf_req.current_group, 0x07);       // データ吸上げ | 警報監視 | モニタリング
        max_unit_work = group_registry.max_unit_no;

        //Printf("===== do_realscan_Format0 group_cnt = %d, group_size = %d\r\n", group_cnt, group_size);

        do {
            MaxRealscan = (uint8_t)(group_registry.altogether + 1); // 最悪時のリアルスキャン回数(中継機数+1(親機))

            while(MaxRealscan) {
                //Printf("===== do_realscan_Format0 MaxRealscan= %d\r\n", MaxRealscan);
                rtn = auto_realscan_New(0);         // この中で無線通信を行っている取得データはＦＯＲＭＡＴ0

                if(rtn != AUTO_END) {
                    if(RF_buff.rf_res.status == RFM_NORMAL_END) {                    // 正常終了したらデータをGSM渡し変数にコピー   RF_command_execute の応答が入っている

                        if(r500.node == COMPRESS_ALGORITHM_NOTICE)  // 圧縮データを解凍(huge_buffer先頭は0xe0)
                        {
                            i = r500.data_size - *(uint16_t*)&huge_buffer[2];          // sakaguchi 2021.02.03

                            memcpy(work_buffer, huge_buffer, r500.data_size);
                            r500.data_size = (uint16_t)LZSS_Decode((uint8_t*)work_buffer, (uint8_t*)huge_buffer);

                            if(i > 0)                               // 非圧縮の中継機データ情報があるとき
                            {
                                memcpy((char*)&huge_buffer[r500.data_size], (char*)&work_buffer[*(uint16_t*)&work_buffer[2]], (size_t)i);
                            }

                            r500.data_size = (uint16_t)(r500.data_size + i);
                            property.acquire_number = r500.data_size;
                            property.transferred_number = 0;

                            r500.node = 0xff;                       // 解凍が重複しないようにする
                        }

                        b_w_chg.byte[0] = huge_buffer[0];                           // 無線通信結果メモリを自律処理用変数にコピーする
                        b_w_chg.byte[1] = huge_buffer[1];                           // 無線通信結果メモリを自律処理用変数にコピーする
                        b_w_chg.word = (uint16_t)(b_w_chg.word + 2);

                        if(RF_buff.rf_req.cmd1 == 0x68) {
                            memcpy(rf_ram.realscan.data_byte, huge_buffer, b_w_chg.word);     // MAX=(30byte×128)+2 = 3842
                        } else {
                            memcpy(rf_ram.realscan.data_byte, &huge_buffer[2], b_w_chg.word); // MAX=(30byte×128)+2 = 3842
                        }

                        G_No = RF_buff.rf_req.current_group;
                        process_realscan_Format0(G_No); //copy_realscan(0);
                    }
                    group_registry.max_unit_no = max_unit_work; // copy_realscan(1)でMAXを0にしてしまっていたmax_unit_work 2016.03.23修正
                } else {
                    break;
                }
                MaxRealscan--;
            }

            get_regist_scan_unit_retry();
            group_registry.max_unit_no = max_unit_work;     // --------------------------------------- リアルスキャンするMAX子機を保存しておく    2014.10.14追加

            tx_thread_sleep(70);       // 2010.08.03 リトライ前のWaitをDelayに変更   xxx03
        } while(retry_cnt--);

    }

    Printf("\r\n");
    Printf("do_realscan_Format0 - End\r\n");
    Printf("\r\n");

    return ERR(RF, NOERROR);
}
#endif
// 2023.05.26 ↑

#if  0
void chk_download_temp_for_new_tags(void);
void chk_download_temp_for_new_tags(void)
{
    def_a_download *p_dl_temp;
    def_a_download *p_dl_O;
    int i;
    int j;

    Printf("\r\n chk_download_temp_for_new_tags START\r\n");

    for(i = 0; i < 128; i++) {
        p_dl_temp = &auto_control_download_temp[i]; // @ cpy_download_from_temp
        if(p_dl_temp->group_no == 0x00)
        continue;
        if(p_dl_temp->group_no == 0xff)
        break;
        for(j = 0; j < 128; j++) {
            p_dl_O = &auto_control.download[j];
            if(p_dl_O->group_no == 0x00)
            continue;
            if(p_dl_O->group_no == 0xff)
            break;
            if(memcmp(p_dl_temp->siri, p_dl_O->siri, 4) == 0) {
                Printf("exists:  serial   = 0x%08x\r\n", *(uint32_t*)&p_dl_temp->siri[0]);
                break;
            }
        }
        if((p_dl_O->group_no == 0xff) || (j == 128)) // was not in old so set start data to 0
        {
            Printf("new   :  serial   = 0x%08x\r\n", *(uint32_t*)&p_dl_temp->siri[0]);
            //p_dl_temp->data_no = 0;
            //OR?
            first_download_flag_write(p_dl_temp->group_no, p_dl_temp->unit_no, FDF_ZENBU_1);
        }

    }

    Printf("\r\n chk_download_temp_for_new_tags END\r\n");
}

void cpy_download_from_temp(void)
{
    def_a_download *p_dl;
    int i;

    Printf("\r\ncpy_download_from_temp START\r\n");
    for(i = 0; i < 128; i++) {
        p_dl = &auto_control_download_temp[i]; // @ cpy_download_from_temp
        if(p_dl->group_no == 0x00)
        continue;
        if(p_dl->group_no == 0xff)
        break;
        Printf("  group_no = %d\r\n", p_dl->group_no);
        Printf("  unit_no  = %d\r\n", p_dl->unit_no);
        Printf("  serial   = 0x%08x\r\n", *(uint32_t*)&p_dl->siri[0]);
        Printf("  data_no  = %d\r\n", *(uint16_t*)&p_dl->data_no[0]);
        set_download(p_dl->group_no, p_dl->unit_no, &p_dl->siri[0], *(uint16_t*)&p_dl->data_no[0]);
    }
    auto_control.crc = crc32_bfr(&auto_control, 5122);      // ((128*8)*5)+2    CRC追加   // karel 2021.06.28

    Printf("\r\ncpy_download_from_temp END\r\n");
}
#endif // 0

void nsc_realscan_data_no_update(void);
void nsc_realscan_data_no_update(void)
{
    int idx_new;
    int j;

    bool need_to_update = false;

// 2023.05.26 ↓
//    //DRNG
//    char suction_Range = 0;
//    suction_Range = my_config.suction.Range[0]; //DRNG
//    if(suction_Range != 0) {
//        // Set all first_download_flag to ZENBU or all entries to 0xFF,0xFF
//        // do crc
//        return;
//    }
// 2023.05.26 ↑

    for(idx_new = 0; idx_new < KK_INFO_MAX ; idx_new++) {

        if(kk_all_new[idx_new].group_no == 0xFF)
            break;
        if(kk_all_new[idx_new].unit_no == 0xFF)
            break;
        if(kk_all_new[idx_new].group_no != 0x00) {
            if(kk_all_new[idx_new].download.assigned == false) {
                need_to_update = true;
                break;
            }
        }
    }

    if(need_to_update == false)
        return;

    // Do realscan to collect data_no

    /*
     if(0) {
     //--- Delete all temp records
     clr_download_temp();    // delete all temp entries

     //--- Monitor to get G_No, U_No, serial
     RF_regu_moni();         // 手動ルートのレギュラーモニタリングを実行
     //BOGUS_EXTRACT_GNSD();
     copy_download_temp_G_U_SerialNumber();

     //--- RealScan Format 0 to get (G_No, U_No), data_..
     uint32_t do_realscan_Format0(void);
     do_realscan_Format0();

     //debug_print_download_temp();
     //debug_print_download();

     #if 1//R500BM_RECHECK_DOWNLOAD
     // Create Real download state holder with new entries set to start at 0
     //if(chk == 0) { // only if sram was OK do we check for new devices
     chk_download_temp_for_new_tags();
     //}
     #endif

     // Delete all download records
     clr_download();
     // Create Real download records
     cpy_download_from_temp();

     debug_print_download_temp();
     debug_print_download();
     }
     */
// 2023.05.26 ↓ 500BWでは使用しない
//    //----- Clear download_temp -----
//    clr_download_temp();    // delete all temp entries
//
//    //----- Scan to Get Group, Unit, Serial_number -----
//    if(1 == (root_registry.auto_route & 0x01)) {     // 自動ルート設定の場合
//        RF_full_moni();         // フルモニタリングを実行
//    } else {
//        RF_regu_moni();         // 手動ルートのレギュラーモニタリングを実行
//    }
//
//    //----- Copy Group, Unit, Serial_number -----
//    copy_download_temp_G_U_SerialNumber();
//
//    //----- Scan to Get Group, Unit, Data_No -----
//    //----- and copy to download_temp        -----
//    do_realscan_Format0();
//
//    debug_print_download_temp();
//    debug_print_download();
// 2023.05.26 ↑

    uint8_t G_new;
    uint8_t U_new;
    uint32_t Serial;

// 2023.05.26 ↓ 500BWでは使用しない
#if 0
    //----- Update data_no if not assigned -----
    for(idx_new = 0; idx_new < KK_INFO_MAX ; idx_new++) {

        if(kk_all_new[idx_new].group_no == 0xFF)
            break;
        if(kk_all_new[idx_new].unit_no == 0xFF)
            break;
        if(kk_all_new[idx_new].group_no != 0x00) {

            G_new = kk_all_new[idx_new].group_no;
            U_new = kk_all_new[idx_new].unit_no;
            Serial = kk_all_new[idx_new].SerialNo;

            if(kk_all_new[idx_new].download.assigned == false) {

                for(j = 0; j < KK_INFO_MAX ; j++) {
                    if((auto_control_download_temp[j].group_no == G_new) && (auto_control_download_temp[j].unit_no == U_new)) {
                        memcpy(&kk_all_new[idx_new].download.siri, &Serial, 4); // not needed as Serial already matches
                        memcpy(&kk_all_new[idx_new].download.data_no, &auto_control_download_temp[j].data_no, 2);
                        kk_all_new[idx_new].download.assigned = true;
                        break;
                    }
                }

                int k;
                for(k = 0; k < KK_INFO_MAX ; k++) {
                    if((first_download_flag.gn[k] == G_new) && (first_download_flag.kn[k] == U_new)) {
                        /*
                         * This is correct: If we could not find the data_no through real scan then
                         * we want to download all the data, because otherwise the data_no will be 0
                         * and 0 could mean the start of data or could be some other invalid value
                         * which could put a hole in the downloads.
                         */
                        if(j == KK_INFO_MAX) {
                            first_download_flag.flag[k] = FDF_ZENBU_1; // If we could Not find a data number to set
                        } else {
                            first_download_flag.flag[k] = FDF_SABUN_0; // If we could set the last data_no
                        }
                    }
                }
            }
        }
    }
#endif
// 2023.05.26 ↑

    //----- Reassign auto_control.download -----
    for(idx_new = 0; idx_new < KK_INFO_MAX ; idx_new++) {

        if(kk_all_new[idx_new].group_no == 0xFF)
            break;
        if(kk_all_new[idx_new].unit_no == 0xFF)
            break;

        if(kk_all_new[idx_new].group_no != 0x00) {

            G_new = kk_all_new[idx_new].group_no;
            U_new = kk_all_new[idx_new].unit_no;
            Serial = kk_all_new[idx_new].SerialNo;

            for(j = 0; j < KK_INFO_MAX ; j++) {

                if((auto_control.download[j].group_no == G_new) && (auto_control.download[j].unit_no == U_new)) {
                    memcpy(&auto_control.download[j].siri, &kk_all_new[idx_new].download.siri, 4);
                    memcpy(&auto_control.download[j].data_no, &kk_all_new[idx_new].download.data_no, 2);
                    auto_control.download[j].group_no = kk_all_new[idx_new].group_no;
                    auto_control.download[j].unit_no = kk_all_new[idx_new].unit_no;

                    if(kk_all_new[idx_new].download.assigned == false) {
                        Printf("\r\n\r\n\r\n================================ \r\n download.assigned == false \r\n");
                        *(uint16_t*)&auto_control.download[j].data_no[0] = 0;
                    }
                    break;
                }

            }
        }
    }

    kk_all_new_crc = crc32_bfr(&kk_all_new, sizeof(kk_all_new));
    auto_control.crc = crc32_bfr(&auto_control, 5122);      // ((128*8)*5)+2    CRC追加
    first_download_flag_crc = crc32_bfr(&first_download_flag, sizeof(first_download_flag));

}

void nsc_debug_dump_auto(void);
void nsc_debug_dump_auto(void)
{
    Printf("\r\n");
    Printf("----- nsc_debug_dump_auto -----\r\n");

    bool one_zero;
    int i;

    one_zero = true;
    for(i = 0; i < KK_INFO_MAX ; i++) {
        if(first_download_flag.gn[i] == 0xFF)
            break;
        if(first_download_flag.kn[i] == 0xFF)
            break;
        if((first_download_flag.gn[i] != 0x00) || ((first_download_flag.gn[i] == 0x00) && one_zero)) {
            if(first_download_flag.gn[i] == 0x00)
                one_zero = false;
            Printf("FDF[%d] G=%d, U=%d, Flag = 0x%X\r\n", i, first_download_flag.gn[i], first_download_flag.kn[i], first_download_flag.flag[i]);
        }
    }
    one_zero = true;
    for(i = 0; i < KK_INFO_MAX ; i++) {
        if(auto_control.download[i].group_no == 0xFF)
            break;
        if(auto_control.download[i].unit_no == 0xFF)
            break;
        if((auto_control.download[i].group_no != 0x00) || ((auto_control.download[i].group_no == 0x00) && one_zero)) {
            if(auto_control.download[i].group_no == 0x00)
                one_zero = false;
            Printf("Download[%d] G=%d, U=%d, Serial=%08X, Data_NO = %d\r\n", i, auto_control.download[i].group_no, auto_control.download[i].unit_no, *(uint32_t*)&auto_control.download[i].siri[0],
                   *(uint16_t*)&auto_control.download[i].data_no[0]);
        }
    }
    one_zero = true;
    for(i = 0; i < KK_INFO_MAX ; i++) {
        if(auto_control.warning[i].group_no == 0xFF)
            break;
        if(auto_control.warning[i].unit_no == 0xFF)
            break;
        if((auto_control.warning[i].group_no != 0x00) || ((auto_control.warning[i].group_no == 0x00) && one_zero)) {
            if(auto_control.warning[i].group_no == 0x00)
                one_zero = false;
            Printf("Warning[%d] G=%d, U=%d\r\n", i, auto_control.warning[i].group_no, auto_control.warning[i].unit_no);
        }
    }
    one_zero = true;
    for(i = 0; i < KK_INFO_MAX ; i++) {
        if(auto_control.w_config[i].group_no == 0xFF)
            break;
        if(auto_control.w_config[i].unit_no == 0xFF)
            break;
        if((auto_control.w_config[i].group_no != 0x00) || ((auto_control.w_config[i].group_no == 0x00) && one_zero)) {
            if(auto_control.w_config[i].group_no == 0x00)
                one_zero = false;
            Printf("W_config[%d] G=%d, U=%d, Now=%04X, B4=%04X, OO=%04X, \r\n", i, auto_control.w_config[i].group_no, auto_control.w_config[i].unit_no, *(uint16_t*)&auto_control.w_config[i].now[0],
                   *(uint16_t*)&auto_control.w_config[i].before[0], *(uint16_t*)&auto_control.w_config[i].on_off[0]);
        }
    }

    Printf("----- nsc_debug_dump_auto END -----\r\n");
}

void nsc_all(void)
{
    Printf("\r\n");
    Printf("\r\n----- nsc_all -----\r\n");

    nsc_new_to_old();
    first_download_flag_crc_NG("XXX 1");
    nsc_build_new_from_registry();
    first_download_flag_crc_NG("XXX 2");
    nsc_collect_old_data();
    first_download_flag_crc_NG("XXX 3");
    nsc_recreate_states();
    first_download_flag_crc_NG("XXX 4");
    nsc_realscan_data_no_update();

    Printf("----- nsc_all -----\r\n");
}

#if  0
void BOGUS_EXTRACT_GNSD(void);
void BOGUS_EXTRACT_GNSD(void)
{
    int grp = -1, size, ch; //, valid;
    int count;//, temp;
    //uint32_t    current;
    //char    multi[8+2];         // 積算データ
    char *Form;// 子機個別データへのポインタ
    //char    *ptr;

    int i;
    char *pt = (char *)&rf_ram.auto_rpt_info.kdata[0];

    uint8_t __group;
    uint8_t __unit;
    uint32_t __serial;
    uint16_t __dataID;

    //Printf("\r\n");
    //Printf("\r\n");
    //Printf("---------- BOGUS_EXTRACT_GNSD ----------\r\n");
    //Printf("\r\n");

    //----------------------------------------------------------

    size = *(uint16_t *)rf_ram.auto_format1.data_byte;// 先頭の2バイト     キャストに変更 2020.01.21
    Form = (char *)&rf_ram.auto_format1.kdata[0];// データの先頭アドレス
    Printf("monitor data size %d\r\n", size);
#ifdef DBG_TERM
    if(size != 1664)        // debug 2021.01.27
    Printf("monitor data size error %d\r\n", size);

    ptr = (char *)&rf_ram.auto_format1.kdata[0];
    for(i=0;i<size;i+=64) {
        for(j=0;j<64;j++) {
            Printf("%02X ", *ptr++);
        }
        Printf("\r\n");
    }
#endif

    while(size >= 64) {          // 64byte * n個

        MakeMonitorStruct(0, Form);// 処理用の構造体を作成（初期化）

        if(grp != MonObj.GroupNum) {     // グループ番号
            grp = MonObj.GroupNum;
            __group = (uint8_t)grp;
            Printf("%02X %02X\r\n", rf_ram.auto_rpt_info.data_byte[0], rf_ram.auto_rpt_info.data_byte[1]);
            for(i = 0; i < 64; i++) {
                Printf("%02X ", *pt++);
            }
            Printf("\r\n");
        }

        __serial = MonObj.SerialNumber;
        __unit = MonObj.UnitNum;
        Printf("__group   = %d\r\n", __group);
        Printf("__unit    = %d\r\n", __unit);
        Printf("__serial  = %08X\r\n", __serial);

        for(ch = 1; ch <= MonObj.CH; ch++) {     // 最大6CH
            count = MakeMonitorStruct(ch, Form);// 処理用の構造体を作成
            Printf("__ch      = %d\r\n", ch);
            if(ch < 5) {     // CH1 - CH4 ?
                __dataID = MonObj.DataID;
                Printf("__dataID  = %08X\r\n", __dataID);
                //XML.TagOnceEmpty( MonObj.RecordCount, "data_id",   "%u",  MonObj.DataID );
            } else {      // CH5, CH6
                __dataID = MonObj.DataID;
                //XML.TagOnceEmpty( 0, "data_id",  "" );
            }

            if(__dataID != 0)
            break;
        }

        set_download_temp(__group, __unit, (uint8_t*)&__serial, NULL);      //&__dataID); // @ BOGUS_EXTRACT_GNSD()

        Form += count;
        size -= count;

    }

    debug_print_download_temp();

}
#endif // 0






typedef struct kk_delta_s
{
    uint32_t serial_number;
    uint8_t flagX;
} kk_delta_t;
kk_delta_t kk_delta[KK_INFO_MAX] __attribute__((section(".xram")));

//void kk_delta_clear_all(void);
//void kk_delta_clear(uint32_t serial_number);
//void kk_delta_set(uint32_t serial_number, uint8_t flagX);
//bool kk_delta_get(uint32_t serial_number, uint8_t* p_flagX);
//void kk_delta_set_all(uint8_t flagX);                           // 2023.05.26

void kk_delta_clear_all(void)
{
    int i;

    for(i=0; i< KK_INFO_MAX; i++)
    {
        kk_delta[i].serial_number = 0x00000000;
        kk_delta[i].flagX = 0x00;
    }
}

void kk_delta_clear(uint32_t serial_number)
{
    int i;

    for(i=0; i< KK_INFO_MAX; i++)
    {
        if(kk_delta[i].serial_number == serial_number) {
            kk_delta[i].serial_number = 0x00000000;
            kk_delta[i].flagX = 0x00;
            break;
        }
    }
    if(i==KK_INFO_MAX)
    {
        // Not there
        return;
    }

    // Shift Up
    int j;
    for(j=i; j < (KK_INFO_MAX - 1); j++)
    {
        kk_delta[j].serial_number = kk_delta[j+1].serial_number;
        kk_delta[j].flagX = kk_delta[j+1].flagX;
    }
    kk_delta[j].serial_number = 0x00000000;
    kk_delta[j].flagX = 0x00;
}

void kk_delta_set(uint32_t serial_number, uint8_t flagX)
{
    int i;

    for(i=0; i< KK_INFO_MAX; i++)
    {
        if(kk_delta[i].serial_number == serial_number) {
            kk_delta[i].flagX = flagX;
            return;
        }
        if(kk_delta[i].serial_number == 0x00000000) {
            break;
        }
    }
    if(i==KK_INFO_MAX)
    {
        // FULL error
        return;
    }
    kk_delta[i].serial_number = serial_number;
    kk_delta[i].flagX = flagX;
}


bool kk_delta_get(uint32_t serial_number, uint8_t* p_flagX)
{
    int i;

    for(i=0; i< KK_INFO_MAX; i++)
    {
        if(kk_delta[i].serial_number == serial_number) {
            *p_flagX = kk_delta[i].flagX;
            return true;
        }
    }
    *p_flagX = 0x00;
    return false;
}

// 2023.05.26 ↓
void kk_delta_set_all(uint8_t flagX)
{
    int i;

    for(i=0; i<KK_INFO_MAX; i++){
        if(auto_control.download[i].group_no == 0xFF)
            break;
        if(auto_control.download[i].unit_no == 0xFF)
            break;
        if(auto_control.download[i].group_no != 0x00){
            kk_delta_set(*(uint32_t*)&auto_control.download[i].siri[0], flagX);
        }
    }
}
// 2023.05.26 ↑

#endif // CONFIG_NEW_STATE_CTRL



//void npc_init(void)
//{
//
//}


/*
 * | Vusb    | Vac      | Vbatt     |                                                  |
 * | V < 4.1 |  0       | V < X ==0 | Disconnect USB? All USB returns Low Power error  |
 * | V < 4.1 |  0       | X < V < Y | NO LTE  |
 * | V < 4.1 |  0       | Y < V     | [B]ALL OK  |

 * | V > 4.1 |  0       | V < X ==0 | All OK but not LTE  |
 * | V > 4.1 |  0       | X < V < Y | ALL OK  |
 * | V > 4.1 |  0       | Y < V     | [B]ALL OK  |

 * | 0       | V < 4.1  | V < X ==0 | DS  |
 * | 0       | V < 4.1  | X < V < Y | ALL OK |
 * | 0       | V < 4.1  | Y < V     | [B]ALL OK  |

 * | 0       | V > 4.1  | V < X ==0 | [A]All OK (on power fail glitch processing)  |
 * | 0       | V > 4.1  | X < V < Y | [A]ALL OK |
 * | 0       | V > 4.1  | Y < V     | [B]ALL OK |

 */
/*
 * When Bat goes too low <5.2V? then we have to wait for bat > 5.8 Volts to come alive again
 * g_GotBadBattStatus
 *
 */

/* Boot (Power On or DS wake)
 *  - Batt only < X volts -> DS
 *  - Batt only X < V < Y volts /->/ Turn on LTE, if V < X -> DS /else/ Normal Run
 *  - Batt only Y < V volts Normal Run
 */

/* No Sim
 *  - (Batt) CPU standby to allow BLE
 *  - (AC) CPU standby but LEDS On
 */

/* No Pin - same as No Sim
 */

/* No Registry - same as No Sim
 */

/* No LTE Service - same as ???
 *
 */
/*
 * [event] ---------------------------------------------------------------------
 * powwa_stop_B: 'Release AT'
 * [pswitch] ---------------------------------------------------------------------
 * [pswitch] Powwa: USB = OFF AC = ON, V Int = 4.748, BATT = 5.634
 * [pswitch] Powwa: Batt Min = 3.615, Batt Max = 5.675,  Vint Min = 3.173, Vint Max = 5.389
 * [pswitch] Powwa: Volts_Sample_No = 3518
 * [pswitch] ---------------------------------------------------------------------
 *
 * [http] ---------------------------------------------------------------------
 * powwa_stop_B: 'Release IP'
 * [pswitch] ---------------------------------------------------------------------
 * [pswitch] Powwa: USB = OFF AC = ON, V Int = 4.727, BATT = 5.640
 * [pswitch] Powwa: Batt Min = 3.601, Batt Max = 5.651,  Vint Min = 3.155, Vint Max = 5.415
 * [pswitch] Powwa: Volts_Sample_No = 1741
 * [pswitch] ---------------------------------------------------------------------
 */


/*
 *
[auto] @@@@@@@@@@ auto_thread @@@@@@@@@@ FLG_AUTO_SUCTION

    ==== g_rf_mutex get 1 !!
====> Start AUTO_control 0004
    .RF_event_execute group size 2
    MaxDownload (4)  0
    .RF_event_execute Start download (4)
    auto_download_new Start (61)

######################################################################################################
######################################################################################################
 #### SucRange 1
 FDF Read  G:1 / U:1 / F:00 / at i=0
 FDF Read  G:1 / U:1 / F:00 / at i=0

[auto] DRNG range = 1, first_flag = 0, cmdIn = 0x61, cmdActual = 0x61


[auto] chk_download: G_No=1, U_No=1, siri=0x52c01292, dataID = 84
    .group_registry.frequency 8
auto_download_New -->  RF_command_execute(61)
###RF_command execute 61
od:61 id:536860844 rpt:00 band:00 freq:08 54
rfm_send com close 0
rfm_send com open 0
[auto] LED_Set: Msg:10, flag:1005 [BMLED_PWR]
###RF_command execute 00
R500 command make 61
        R500_radio_communication start
        R500_radio_communication_once start
        R500_send_recv() t=6000
rfm_send com close 0
rfm_send com open 0
[auto] LED_Set: Msg:10, flag:1005 [BMLED_PWR]
        R500_send_recv()  rfm_send  end
^^
extract_property model 57
^^^^^^
                                                           [bled] BLE SOH r = 0, t = 63, [2(n=585, 585, 71)]
[auto] <<NG>>  rfm_recv rtn=F0 / 0014 (1) ------------------------------
R500_radio_communication_once rtn F0
^^^
     R500_radio_communication end 240
###RF_command execute end F0                                RFM_SERIAL_TOUT
=====>> LOG[RF :[auto] RFCMD EXE[f0][61]](24)

    auto_download_new End  (unit_no 1)
    .RF_event_execute end download (1)
=====>> LOG[RF :[auto] $D $E (52C01292)](23)
### Download after Event Check !!!!!!  0000
    .RF_event_execute Start download (3)
    auto_download_new Start (61)

######################################################################################################
######################################################################################################
 #### SucRange 1
 FDF Read  G:1 / U:3 / F:00 / at i=2
 FDF Read  G:1 / U:3 / F:00 / at i=2
 *
 */

