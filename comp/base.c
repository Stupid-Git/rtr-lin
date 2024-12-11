/*
 * base.c
 *
 *  Created on: Dec 2, 2024
 *      Author: karel
 */

#include "_r500_config.h"
#define _BASE_C_

//#include "hal_data.h"
//#include "flag_def.h"
#include "MyDefine.h"
#include "Globals.h"
//#include "Config.h"
//#include "Sfmem.h"
//#include "General.h"
////#include "cmd_thread.h"
#include "base.h"
#include "version.h"
//#include "rf_thread_entry.h"

#include "r500_defs.h"

//=============================================================================
#include <pthread.h>

#define WAIT_100MSEC 10

pthread_mutex_t mutex_log;
pthread_mutex_t mutex_sfmem;

//=============================================================================



//プロトタイプ
static void read_repair_settings(void);
static void alter_settings_default(uint8_t);
//未使用static void init_factory_system_default(uint32_t);


/**
 * 本体設定をＳＲＡＭへ読み込み
 * @note    ＣＲＣエラーなし、実質使用サイズ２５６バイトのとき処理時間約６ｍｓ
 * @note    ４ｋｂｙｔｅはシリアルフラッシュ消去の最小単位
 * @note    17関数から呼ばれている
 */
void read_my_settings(void)
{
    read_repair_settings();
    my_rpt_number = 0;                  //my_settings.relay.number;

}

// 全エリア - 4byte が、FFの場合のCRC32演算値
// size     256(100h)     0xa7340e2f
//          1024(400h)    0x207eaf8d
//          4096(1000h)   0x6E909C7C
//          8192(2000h)   0xE1174F33

/**
 *  read_my_settings()のサブ関数
 * @note    2020.Jul.22 spi.rxbuf直読みを廃止
 */
static void read_repair_settings(void)
{
    bool blank = false;
    uint8_t result = 0;
//    uint32_t i, j;
    uint32_t crc32;
    uint8_t *dst;


    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS)              // シリアルフラッシュ ミューテックス確保
    {
        // シリアルフラッシュメモリ 0x00002000～0x00003FFF オリジナル域ＣＲＣチェック
 //       crc32_init(&crc32);

        dst = &my_config.device.gap01[0];
        //serial_flash_multbyte_read(SFM_CONFIG_START, SFM_CONFIG_SIZE, dst);// オリジナル域 8KB
        file_load__my_config();
        crc32 = crc32_bfr(dst, SFM_CONFIG_SIZE - 4);
        Printf("CRC %08X\r\n", crc32);
        if(crc32 == 0xE1174F33){
            blank = true;       // 全データ０ｘＦＦだった
            Printf("    All blank!!(1)\r\n");
        }
        if(crc32 != my_config.crc){
            result = 0x01;                           // オリジナル域エラー
            Printf("CRC Error, read_settings(main)\r\n");
        }else{
            result = 0;
            Printf("read_settings success\r\n");
            //goto exit_read_func;
        }

//オリジナル域がエラーなのでバックアップ領域を読み出す

        Printf("read_repair_settings backup area read %d / %08X\r\n", result, crc32);

        // シリアルフラッシュメモリ 0x00004000～0x00005FFF バックアップ域ＣＲＣチェック
//        crc32_init(&crc32);
        dst = &my_config.device.gap01[0];
        //serial_flash_multbyte_read(SFM_CONFIG_START_B, SFM_CONFIG_SIZE, dst);//バックアップ域 8KB
        file_load__my_config_b();

        crc32 = crc32_bfr(dst, SFM_CONFIG_SIZE - 4);
        Printf("CRC %08X\r\n", crc32);
        if(crc32 == 0xE1174F33){
             blank = true;       // 全データ０ｘＦＦだった
             Printf("    All blank!!(2)\r\n");
        }
        if(crc32 != my_config.crc){
            result |= 0x02;                          // バックアップ域エラー
            Printf("CRC Error, read_settings backup area!!\r\n");
        }else{
//          result = 0;
            Printf("read_repair_settings backup area read success %d\r\n", result);
            //goto exit_read_func;
        }


// ここまで400msec 256*32 = 8192

 //ここから設定修復

        if(result == 0x00){
            ; //__NOP();
        }
        else if(result == 0x01)  // オリジナル域を修復（実行時間 約２０ｍｓ）
        {
            // バックアップ域をオリジナル域へコピー
            serial_flash_copy(SFM_CONFIG_START_B, SFM_CONFIG_START, SFM_CONFIG_SIZE);
            dst = &my_config.device.gap01[0];
            //serial_flash_multbyte_read(SFM_CONFIG_START, SFM_CONFIG_SIZE, dst);// オリジナル域 8KB
            file_load__my_config();
            Printf("read_settings orignal area recover \r\n");
        }
        else if(result == 0x02) // バックアップ域を修復（実行時間 約２０ｍｓ）
        {
            // オリジナル域をバックアップ域へコピー
            //serial_flash_copy(SFM_CONFIG_START, SFM_CONFIG_START_B, SFM_CONFIG_SIZE);
            file_store__my_config_b();

            //dst = &my_config.device.gap01[0];
            //serial_flash_multbyte_read(SFM_CONFIG_START, SFM_CONFIG_SIZE, dst);// オリジナル域 8KB
            file_load__my_config();
            Printf("read_settings backup area recover \r\n");
        }
        else if(blank == true)  // デフォルト書き込み（実行時間 約２０ｍｓ）両領域ＣＲＣエラーで全データ０ｘＦＦ
        {

            tx_mutex_put(&mutex_sfmem);

            alter_settings_default(DEFAULT_ALL);
            Printf("all default write!!\r\n\r\n");
            rewrite_settings();     //本体設定書き換え（関数内でmutex_sfmemを取得返却している)
            return;
        }
        else    // 修復不可能なのでそのまま（バックアップアルゴリズムが正確に働けば、両領域が壊れることはないはず）
        {
//TODO            __NOP();
            Printf("SFM 1,2 Error\r\n");
        }

exit_read_func:;

        Printf("read_repair_settings %d / %08X\r\n", result, crc32);
        Printf("config read %02X\r\n", result);

        tx_mutex_put(&mutex_sfmem);
    }
}





/**
 * 本体設定書き換え
 * @note    メインエリア、バックアップエリアの２面

         各8K(8192)byte  2sector
          2000 -3fff    8192    親機動作設定エリア １          <8k block>
          4000 -5fff    8192    親機動作設定エリア 2           <8k block>   back up

          処理時間 約600msec
 */
void rewrite_settings(void)
{
//    uint32_t i;

    Printf("rewrite_settings() \r\n");
    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS)              // シリアルフラッシュ ミューテックス確保
    {
        my_config.crc = crc32_bfr((uint8_t *)&my_config, SFM_CONFIG_SIZE - 4);  // ＣＲＣ計算
        Printf("my_config crc %08X\r\n", my_config.crc);

        //serial_flash_block_unlock();                                // グローバルブロックプロテクション解除

        // オリジナル域を書き換えてからバックアップ域を書き換えるという手順が重要（どちらかが必ず正しく保持される）
        //serial_flash_sector_erase(SFM_CONFIG_START);
        //serial_flash_sector_erase(SFM_CONFIG_START + SFM_CONFIG_SECT);                                                                                  // オリジナル域消去

        //serial_flash_multbyte_write(SFM_CONFIG_START, SFM_CONFIG_SIZE, (char *)&my_config);  // デフォルト値書き込み
        file_store__my_config_b();

        #if 0
        for(i = 0; i < SFM_CONFIG_SIZE; i += 256) {
            serial_flash_multbyte_write(SFM_CONFIG_START + i, 256, (char *)&my_config + i);  // デフォルト値書き込み
        }
#endif
        tx_thread_sleep (1);                                                 // オリジナル域の書き込みを確実にしてからバックアップ域消去にいく

        //serial_flash_sector_erase(SFM_CONFIG_START_B);
        //serial_flash_sector_erase(SFM_CONFIG_START_B + SFM_CONFIG_SECT);                                                                                         // バックアップ域消去

        //serial_flash_multbyte_write(SFM_CONFIG_START_B, SFM_CONFIG_SIZE, (char *)&my_config);  // デフォルト値書き込み
        file_store__my_config_b();

        #if 0
        for(i = 0; i < SFM_CONFIG_SIZE; i += 256) {
            serial_flash_multbyte_write(SFM_CONFIG_START_B + i, 256, (char *)&my_config + i);  // デフォルト値書き込み
        }
#endif
        Printf("rewrite_settings() Exec End\r\n");
        tx_mutex_put(&mutex_sfmem);

    }
    else{
        Printf("rewrite_settings() mutex busy !!!! \r\n");
    }
}




/**
 * @brief   Cert File 書き換え
 * @param area  area   0: Webstorage  1:User
 * @param index     インデックス
 * @param size      サイズ
 * @return
 */
int ceat_file_write(int area, int index, uint16_t size)
{
    uint32_t start_adr;

    Printf("Caet File write %d / %d / %d \r\n", area, index, size);

    if(!(index>=0 && index<=8)){
        Printf("index error \r\n");
        return (-1);
    }

    if(area==0){
        start_adr = SFM_CERT_W1_START + 4096*(uint32_t)index;
    }
    else{
        start_adr = SFM_CERT_U1_START + 4096*(uint32_t)index;
    }



    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS)              // シリアルフラッシュ ミューテックス確保
    {
        serial_flash_block_unlock();                                // グローバルブロックプロテクション解除

        if(area==0){

            *(uint16_t *)&CertFile_WS[0] = size;
//            CertFile_WS[0] = (uint8_t)size;
//            CertFile_WS[1] = (uint8_t)(size / 256);

//            serial_flash_erase(start_adr, start_adr+4096);
            serial_flash_sector_erase(start_adr);           // sector erase 4kbyte  //sakaguchi UT-0036


            serial_flash_multbyte_write(start_adr, (uint32_t)4096, (char *)&CertFile_WS[0]);

        }
        else
        {
            CertFile_USER[0] = (uint8_t)size;
            CertFile_USER[1] = (uint8_t)(size / 256);
            Printf(" %02X %02X \r\n", CertFile_USER[0] ,CertFile_USER[1] );

//            serial_flash_erase(start_adr, start_adr+4096);
            serial_flash_sector_erase(start_adr);           // sector erase 4kbyte  //sakaguchi UT-0036

            serial_flash_multbyte_write(start_adr, 4096, (char *)&CertFile_USER[0]);
        }

        tx_mutex_put(&mutex_sfmem);
        ceat_file_read(area, index);        // 関数内でMutexを取得しているため
        return ( 0 );
    }
    else{
        return ( -1 );
    }
}



/**
 * Cert File 読み込み
 * @param area
 * @param index
 * @note    2020.Jul.22 spi.rxbuf直読みを廃止
 */
void ceat_file_read(int area, int index)
{
    char *dst;
 //   uint32_t i, j;
    uint16_t size;

    uint32_t start_adr;

    Printf("Ceat File read %d / %d \r\n", area, index);
    if(!(index>=0 && index<=8)){
        Printf("index error \r\n");
        return;
    }

#if 0       // sakaguchi 2021.07.20 ミューテックス確保に失敗した場合でも、証明書作業用バッファをクリアしてしまうため、ミューテックス確保後にクリア処理を移動する
    if(area==0){
        start_adr = SFM_CERT_W1_START + 4096*(uint32_t)index;
        dst = &CertFile_WS[0];
        memset(CertFile_WS, 0, sizeof(CertFile_WS));
        //CertFileSize_WS = my_config.websrv.CertSizeW[i];
    }
    else{
        start_adr = SFM_CERT_U1_START + 4096*(uint32_t)index;
        dst = &CertFile_USER[0];
        memset(CertFile_USER, 0, sizeof(CertFile_USER));
        //CertFileSize_USER = my_config.websrv.CertSizeU[i];
    }
#endif

    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS)              // シリアルフラッシュ ミューテックス確保
    {
// sakaguchi 2021.07.20 ↓
        if(area==0){
            start_adr = SFM_CERT_W1_START + 4096*(uint32_t)index;
            dst = &CertFile_WS[0];
            memset(CertFile_WS, 0, sizeof(CertFile_WS));
            //CertFileSize_WS = my_config.websrv.CertSizeW[i];
        }
        else{
            start_adr = SFM_CERT_U1_START + 4096*(uint32_t)index;
            dst = &CertFile_USER[0];
            memset(CertFile_USER, 0, sizeof(CertFile_USER));
            //CertFileSize_USER = my_config.websrv.CertSizeU[i];
        }
// sakaguchi 2021.07.20 ↑
        serial_flash_multbyte_read((uint32_t)start_adr, 4096, dst);
#if 0
        for(i = 0; i < 4096; i += 256)
        {
            serial_flash_multbyte_read((uint32_t)(start_adr + i), 256, NULL);

            for(j = 0; j < 256; j++)
            {
                *dst++ = spi.rxbuf[j+4];
            }
        }
#endif
        if(area==0){
            size = (uint16_t)CertFile_WS[0];
        }
        else{
            size = (uint16_t)CertFile_USER[0];
        }
        Printf("File Size = %d(%04X)\r\n", size, size);

        tx_mutex_put(&mutex_sfmem);
    }

}





/**
 * 工場設定書き換え
 * @note    生産時のみ使用

         各8K(8192)byte  2sector
          0000 -0fff    256

           処理時間 約20msec
 */
void write_system_settings(void)
{

    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS)              // シリアルフラッシュ ミューテックス確保
    {
        fact_config.crc = crc32_bfr((uint8_t *)&fact_config, SFM_FACTORY_SIZE-4);  // ＣＲＣ計算


        serial_flash_block_unlock();                                // グローバルブロックプロテクション解除

        serial_flash_sector_erase(SFM_FACTORY_START);
        serial_flash_multbyte_write(SFM_FACTORY_START, SFM_FACTORY_SIZE, (char *)&fact_config);

        tx_mutex_put(&mutex_sfmem);

    }

    Printf("fact config crc %08X\r\n", fact_config.crc);
}


/**
 * 本体設定デフォルト
 * @param all   全クリアスイッチ
 * @attention   init_factory_system_default()でも初期化している
 */
static void alter_settings_default(uint8_t all)
{
    char str[32];

    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS)              // シリアルフラッシュ ミューテックス確保
    {
        memset((uint8_t *)&my_config, 0xff, sizeof(my_config));    //

        //my_settings.relay.compress = 0;
        //my_settings.relay.number = 0;
        //my_settings.relay.band = 0;
        //my_settings.relay.channel = 0;
        //memcpy(my_settings.relay.id.indicate, "   ", 4);
        //my_settings.relay.id.low = 0x00000000;
        //my_settings.relay.id.high = 0x00000000;

        //my_settings.led_blink = 1;                              // 通信中ＬＥＤ点滅         0：しない   1：する
        my_config.ble.active = 1;                             // ＢＬＥ 停止               0:停止        1:動作
        my_config.ble.utilization = 0;                        // ＢＬＥ パスコード利用  0：開放        1：閉鎖
        my_config.ble.passcode = 0;                           // ＢＬＥ パスコード

// 2023.05.31 ↓
        if(fact_config.Vender == VENDER_ESPEC){
            sprintf(str, "%s_%.8lX", UNIT_BASE_ESPEC, (unsigned long)fact_config.SerialNumber);
        }else{
            sprintf(str, "%s_%.8lX", UNIT_BASE_TANDD, (unsigned long)fact_config.SerialNumber);
        }
// 2023.05.31 ↑
        strcpy((char *)my_config.device.Name, str);            // 親機名称

        strcpy((char *)my_config.device.Description, "Explanation");   // 親機説明

        memset(my_config.user_area, 0x00, sizeof(my_config.user_area)); // ユーザ定義情報

        if(all == DEFAULT_ALL)
        {
            fact_config.ble.cap_trim = DEFAULT_CAP_TRIM;             // ＢＬＥ ＣＰＵ ＥＣＯ キャパシタトリム値
            fact_config.ble.interval_0x9e_command = DEFAULT_ADV_UPDATE_INTERVAL;
            fact_config.SerialNumber = 0x5f560000;
            my_config.device.registration_code = 0x00000000;

            //my_settings.adc_slope = 10000;
            //my_settings.debug_direct = 0x00000000;
        }

        tx_mutex_put(&mutex_sfmem);
    }
}


/**
 * @brief   SFMに工場出荷状態の初期値を書き込みを行う
 * @param type  VENDER_TD(0):T&D  VENDER_HIT(9):Hitach(BLE Off) VENDER_ESPEC(1):ESPEC
 * @param mode  0:検査 1:運用時
 * @todo    初期値をコードに埋め込んでいるので量産前に修正必須
 * @note    JP,US,EUは設定値が違う
 * @note    LO_HIマクロ削除
 */
void init_factory_default(uint8_t type, uint8_t mode)
{

    //(void)(mode);       //未使用引数
    uint16_t word_data;
    char str[32];
    uint32_t sn = fact_config.SerialNumber;
    uint32_t SysCnt = my_config.device.SysCnt ;          // 親機設定変更カウンタ
    uint32_t RegCnt = my_config.device.RegCnt;          // 登録情報変更カウンタ
    uint8_t country;                                    // 仕向け先
    char OyaFirm[12];                                   // 親機ファームウェアバージョン
    uint32_t RfFirm;                                    // 無線モジュールファームウェアバージョン
    char BleFirm[8];                                    // BLEファームウェアバージョン

    sn = sn >> 28;                                      // 5:Jp 3:Us 4:Eu E:Espec C:Hitachi
    country = (uint8_t)sn;

    Printf("SN: %ld(%02X)\r\n", sn,country);

    // ファームウェアバージョンを退避           // sakaguchi 2020.11.11
    memcpy(OyaFirm, &my_config.version.OyaFirm[0], sizeof(OyaFirm));
    RfFirm = my_config.version.RfFirm;
    memcpy(BleFirm, &my_config.version.BleFirm[0], sizeof(BleFirm));

    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS)              // シリアルフラッシュ ミューテックス確保
    {
        memset((uint8_t *)&my_config, 0x00, sizeof(my_config));    //

        // 運用時はクリアしない
        if(mode!=0){
            my_config.device.SysCnt = SysCnt++;         // 運用時はクリアせず＋１ // 2023.05.09
            my_config.device.RegCnt = RegCnt++;
            // ファームウェアバージョンを再セット           // sakaguchi 2020.11.11
            memcpy(&my_config.version.OyaFirm[0], OyaFirm, sizeof(my_config.version.OyaFirm));
            my_config.version.RfFirm = RfFirm;
            memcpy(&my_config.version.BleFirm[0], BleFirm, sizeof(my_config.version.BleFirm));
        }
        // 2023.08.21 工場出荷時は0クリア -> 0はDataServerで異常と判断されるため初期値は1にしておく
        else{
            my_config.device.SysCnt = 1;
            my_config.device.RegCnt = 1;
        }

        // device
        my_config.device.SerialNumber = fact_config.SerialNumber;
// 2023.05.31 ↓
        switch(type){
            case VENDER_TD:     // for T&D
                sprintf(str, "%s_%.8lX", UNIT_BASE_TANDD, (unsigned long)fact_config.SerialNumber);
                break;
            case VENDER_ESPEC:  // for Espec
                sprintf(str, "%s_%.8lX", UNIT_BASE_ESPEC, (unsigned long)fact_config.SerialNumber);
                break;
            case VENDER_HIT:    // for Hitachi
                sprintf(str, "%s_%.8lX", UNIT_BASE_HITACHI, (unsigned long)fact_config.SerialNumber);
                break;
            default:
                sprintf(str, "%s_%.8lX", UNIT_BASE_TANDD, (unsigned long)fact_config.SerialNumber);
                break;
        }
// 2023.05.31 ↑
        Printf("BaseName = %s\r\n",str);
        WriteText(my_config.device.Name, str, strlen(str), sizeof(my_config.device.Name));     // 親機名称

        WriteText(my_config.device.Description, "Explanation", strlen("Explanation"), sizeof(my_config.device.Description));     // 親機説明

        memset(my_config.user_area, 0x00, sizeof(my_config.user_area)); // ユーザ定義情報


        WriteText(my_config.network.SntpServ1, "time.google.com", strlen("time.google.com"), sizeof(my_config.network.SntpServ1));

        my_config.device.TimeSync[0] = 1;                                           // SNTP

        switch (country)
        {
        case 0x03:      // US
            my_config.device.TempUnits[0] = 1;                                          // F
            WriteText(my_config.device.TimeDiff, "-0800", strlen("-0800"), sizeof(my_config.device.TimeDiff));                      // 時差 -0800
            WriteText(my_config.device.TimeForm, "*M-*d-*Y *H:*i:*s", strlen("*M-*d-*Y *H:*i:*s"), sizeof(my_config.device.TimeForm));
            WriteText(my_config.device.TimeZone, "(GMT-08:00) Pacific Time (US & Canada)", strlen("(GMT-08:00) Pacific Time (US & Canada)"), sizeof(my_config.device.TimeZone));
            WriteText(my_config.device.Summer, "10300+0802001100+010200+60", strlen("10300+0802001100+010200+60"), sizeof(my_config.device.Summer));  // 3月第2日曜日午前2時にLocal+60分

            WriteText(my_config.network.SntpServ2, "time.nist.gov", strlen("time.nist.gov"), sizeof(my_config.network.SntpServ2)); // ng

            WriteText(my_config.websrv.Server, "api.webstorage-service.com", strlen("api.webstorage-service.com"), sizeof(my_config.websrv.Server));
            WriteText(my_config.websrv.Api, "/rtr500/device/", strlen("/rtr500/device/"), sizeof(my_config.websrv.Api));

            break;
        case 0x04:      // EU
            my_config.device.TempUnits[0] = 0;                                          // C
            WriteText(my_config.device.TimeDiff, "+0100", strlen("+0100"), sizeof(my_config.device.TimeDiff));                      // 時差 +0100
            WriteText(my_config.device.TimeForm, "*d-*m-*Y *H:*i:*s", strlen("*d-*m-*Y *H:*i:*s"), sizeof(my_config.device.TimeForm));
            WriteText(my_config.device.TimeZone, "(UTC+01:00) Central European Time", strlen("(UTC+01:00) Central European Time"), sizeof(my_config.device.TimeZone));
            WriteText(my_config.device.Summer, "20300-0101001000-010100+60", strlen("20300-0101001000-010100+60"), sizeof(my_config.device.Summer));  // 3月第2日曜日午前2時にLocal+60分

            WriteText(my_config.network.SntpServ2, "europe.pool.ntp.org", strlen("europe.pool.ntp.org"), sizeof(my_config.network.SntpServ2));

            WriteText(my_config.websrv.Server, "api.webstorage-service.com", strlen("api.webstorage-service.com"), sizeof(my_config.websrv.Server));
            WriteText(my_config.websrv.Api, "/rtr500/device/", strlen("/rtr500/device/"), sizeof(my_config.websrv.Api));

            break;
        default:        // JP  or ESPEC or Hitachi
            my_config.device.TempUnits[0] = 0;                                          // C
/*DEBUG
            WriteText(my_config.device.TimeDiff, "+0100", strlen("+0100"), sizeof(my_config.device.TimeDiff));                      // 時差 +0100
            WriteText(my_config.device.TimeForm, "*d-*m-*Y *H:*i:*s", strlen("*d-*m-*Y *H:*i:*s"), sizeof(my_config.device.TimeForm));
            WriteText(my_config.device.TimeZone, "(UTC+01:00) Central European Time", strlen("(UTC+01:00) Central European Time"), sizeof(my_config.device.TimeZone));
            WriteText(my_config.device.Summer, "20300-0101001000-010100+60", strlen("20300-0101001000-010100+60"), sizeof(my_config.device.Summer));  // 3月第2日曜日午前2時にLocal+60分
*/ /*DEBUG*/
            WriteText(my_config.device.TimeDiff, "+0900", strlen("+0900"), sizeof(my_config.device.TimeDiff));                      // 時差 +0900
            WriteText(my_config.device.TimeForm, "*Y-*m-*d *H:*i:*s", strlen("*Y-*m-*d *H:*i:*s"), sizeof(my_config.device.TimeForm));
//      NG    WriteText(my_config.device.TimeZone, "(UTC+09:00) 大阪、札幌、東京", strlen("(UTC+09:00) 大阪、札幌、東京"), sizeof(my_config.device.TimeZone));
//      NG      WriteText(my_config.device.TimeZone, "(UTC+09:00) ヤクーツク", strlen("(UTC+09:00) ヤクーツク"), sizeof(my_config.device.TimeZone));
            WriteText(my_config.device.Summer, "00101+0000000101+000000+00", strlen("00101+0000000101+000000+00"), sizeof(my_config.device.Summer));  // Summer time 無し
/*DEBUG*/
/*OK*/memset(my_config.device.TimeZone, 0, sizeof(my_config.device.TimeZone));
uint8_t jjj[] = {0x28, 0x55, 0x54, 0x43, 0x2B, 0x30, 0x39, 0x3A, 0x30, 0x30, 0x29, 0x20, 0x91, 0xE5,
                 0x8D, 0xE3, 0x81, 0x41, 0x8E, 0x44, 0x96, 0x79, 0x81, 0x41, 0x93, 0x8C, 0x8B, 0x9E};
//memcpy(my_config.device.TimeZone, jjj, sizeof(jjj));

uint8_t uuu[] = {0x28, 0x55, 0x54, 0x43, 0x2b, 0x30, 0x39, 0x3a, 0x30, 0x30, 0x29, 0x20, 0xe5, 0xa4,
                 0xa7, 0xe9, 0x98, 0xaa, 0xe3, 0x80, 0x81, 0xe6, 0x9c, 0xad, 0xe5, 0xb9, 0x8c, 0xe3,
                 0x80, 0x81, 0xe6, 0x9d, 0xb1, 0xe4, 0xba, 0xac};
//memcpy(my_config.device.TimeZone, uuu, sizeof(uuu));
/*
Resonse
54 32 BF 00 52 44 54 49  4D 3A 44 54 49 4D 45 3D  |  T2..RDTIM:DTIME=
0E 00 32 30 32 34 31 32  31 30 30 33 30 32 31 37  |  ..20241210030217
55 54 43 3D 0A 00 31 37  33 33 37 39 39 37 33 37  |  UTC=..1733799737
42 49 41 53 3D 03 00 2B  30 30 44 53 54 3D 1A 00  |  BIAS=..+00DST=..
30 30 31 30 31 2B 30 30  30 30 30 30 30 31 30 31  |  00101+0000000101
2B 30 30 30 30 30 30 2B  30 30 46 4F 52 4D 3D 11  |  +000000+00FORM=.
00 2A 59 2D 2A 6D 2D 2A  64 20 2A 48 3A 2A 69 3A  |  .*Y-*m-*d *H:*i:
2A 73 44 49 46 46 3D 05  00 2B 30 39 30 30 5A 4F  |  *sDIFF=..+0900ZO
4E 45 3D 24 00 28 55 54  43 2B 30 39 3A 30 30 29  |  NE=$.(UTC+09:00)
20 E5 A4 A7 E9 98 AA E3  80 81 E6 9C AD E5 B9 8C  |   ...............
E3 80 81 E6 9D B1 E4 BA  AC 53 59 4E 43 3D 01 00  |  .........SYNC=..
31 53 54 41 54 55 53 3D  09 00 30 30 30 32 2D 30  |  1STATUS=..0002-0
30 30 30 2D 1C                                    |  000-.
Resonse
54 32 BF 00 52 44 54 49  4D 3A 44 54 49 4D 45 3D  |  T2..RDTIM:DTIME=
0E 00 32 30 32 34 31 32  31 30 30 32 35 30 30 39  |  ..20241210025009
55 54 43 3D 0A 00 31 37  33 33 37 39 39 30 30 39  |  UTC=..1733799009
42 49 41 53 3D 03 00 2B  30 30 44 53 54 3D 1A 00  |  BIAS=..+00DST=..
30 30 31 30 31 2B 30 30  30 30 30 30 30 31 30 31  |  00101+0000000101
2B 30 30 30 30 30 30 2B  30 30 46 4F 52 4D 3D 11  |  +000000+00FORM=.
00 2A 59 2D 2A 6D 2D 2A  64 20 2A 48 3A 2A 69 3A  |  .*Y-*m-*d *H:*i:
2A 73 44 49 46 46 3D 05  00 2B 30 39 30 30 5A 4F  |  *sDIFF=..+0900ZO
4E 45 3D 24 00 28 55 54  43 2B 30 39 3A 30 30 29  |  NE=$.(UTC+09:00)
20 E5 A4 A7 E9 98 AA E3  80 81 E6 9C AD E5 B9 8C  |   ...............
E3 80 81 E6 9D B1 E4 BA  AC 53 59 4E 43 3D 01 00  |  .........SYNC=..
31 53 54 41 54 55 53 3D  09 00 30 30 30 32 2D 30  |  1STATUS=..0002-0
30 30 30 28 1C                                    |  000(.
*/


            WriteText(my_config.network.SntpServ2, "ntp.nict.jp", strlen("ntp.nict.jp"), sizeof(my_config.network.SntpServ2));

            WriteText(my_config.websrv.Server, "api.webstorage.jp", strlen("api.webstorage.jp"), sizeof(my_config.websrv.Server));
            WriteText(my_config.websrv.Api, "/rtr500/device/", strlen("/rtr500/device/"), sizeof(my_config.websrv.Api));

// 2023.05.31 ↓
            if(type == VENDER_HIT){     // for Hitachi
                WriteText(my_config.websrv.Server, "/", strlen("/"), sizeof(my_config.websrv.Server));
                WriteText(my_config.websrv.Api, "/", strlen("/"), sizeof(my_config.websrv.Api));
            }else{                      // for T&D,Espec
                WriteText(my_config.websrv.Server, "api.webstorage.jp", strlen("api.webstorage.jp"), sizeof(my_config.websrv.Server));
                WriteText(my_config.websrv.Api, "/rtr500/device/", strlen("/rtr500/device/"), sizeof(my_config.websrv.Api));
            }
// 2023.05.31 ↑
            break;
        }

        // ftp
        my_config.ftp.Port[0] = 21;
        my_config.ftp.Pasv[0] = 1;          // PASV

        // network
        my_config.network.DhcpEnable[0] = 1;      // DHCP On

        WriteText(my_config.network.IpAddrss, "192.168.5.100", strlen("192.168.5.100"), sizeof(my_config.network.IpAddrss));
        WriteText(my_config.network.SnMask,   "255.255.0.0", strlen("255.255.0.0"), sizeof(my_config.network.SnMask));
        WriteText(my_config.network.GateWay,  "0.0.0.0", strlen("0.0.0.0"), sizeof(my_config.network.GateWay));
        WriteText(my_config.network.Dns1,  "0.0.0.0", strlen("0.0.0.0"), sizeof(my_config.network.Dns1));
        WriteText(my_config.network.Dns2,  "0.0.0.0", strlen("0.0.0.0"), sizeof(my_config.network.Dns2));

        WriteText(my_config.network.NetPass,  "password", strlen("password"), sizeof(my_config.network.NetPass));

        word_data = 62500;
        *(uint16_t *)&my_config.network.SocketPort[0] = word_data;
        word_data = 62501;
        *(uint16_t *)&my_config.network.UdpRxPort[0] = word_data;
        word_data = 62502;
        *(uint16_t *)&my_config.network.UdpTxPort[0] = word_data;

        my_config.network.Phy[0] = 0;

        // mtu                                  // MTU追加 sakaguchi 2021.04.07
        word_data = 1400;                       // 1400
        *(uint16_t *)&my_config.network.Mtu[0] = word_data;

        // 通信間隔                              // sakaguchi 2021.05.28
        my_config.network.Interval[0] = 0;

        // wlan
        my_config.wlan.Enable[0] = 0;
        my_config.wlan.SEC[0]    = 4;
        //my_config.wlan.SEC[0]    = 0;     // なし

        // ble
// 2023.05.31 ↓
        if(type == VENDER_HIT){         // for Hitachi
            my_config.ble.active = 0;           // BLE OFF
        }else{                          // for T&D,Espec
            my_config.ble.active = 1;           // BLE ON
        }
// 2023.05.31 ↑
        my_config.ble.utilization = 0;

        // smtp
        my_config.smtp.Server[0] = 0;
        my_config.smtp.UserID[0] = 0;
        my_config.smtp.UserPW[0] = 0;
        my_config.smtp.Sender[0] = 0;
        my_config.smtp.From[0] = 0;
        my_config.smtp.Port[0] = 25;
        my_config.smtp.AuthMode[0] = 0;

        word_data = 443;
        *(uint16_t *)&my_config.websrv.Port[0] = word_data;
        //x.word = 1440;                                // 1 day
        word_data = 10;                                    // 10Min
        *(uint16_t *)&my_config.websrv.Interval[0] = word_data;

// 2023.05.31 ↓
        if(type == VENDER_HIT){             // for Hitachi
            my_config.websrv.Mode[0] = 3;       // https User
        }else{                              // for T&D,Espec
            my_config.websrv.Mode[0] = 2;       // https WebStorage
        }
// 2023.05.31 ↑

        // warning
        my_config.warning.Enable[0] = 0;
        my_config.warning.Interval[0] = 5;
        my_config.warning.Route[0] = 3;
        my_config.warning.Ext[0] = 0xf0;

        // monitor
        my_config.monitor.Enable[0] = 1;
        my_config.monitor.Interval[0] = 10;
        my_config.monitor.Route[0] = 3;                                             // http
        // suction
        //my_config.suction.Enable[0] = 1;
        my_config.suction.Enable[0] = 0;                                            // 2022.05.24 cg
        my_config.suction.Route[0] = 3;                                             // http
        my_config.suction.FileType[0] = 1;                                          // XML プログラムではここは見ない  ※未使用

        //WriteText(my_config.suction.Event0, "000000", strlen("000000"), sizeof(my_config.suction.Event0));
        WriteText(my_config.suction.Event0, "00#400", strlen("000000"), sizeof(my_config.suction.Event0));
        //WriteText(my_config.suction.Event1, "100600", strlen("000000"), sizeof(my_config.suction.Event1));
        WriteText(my_config.suction.Event1, "000000", strlen("000000"), sizeof(my_config.suction.Event1));      // 2022.05.24 cg
        WriteText(my_config.suction.Event2, "000000", strlen("000000"), sizeof(my_config.suction.Event2));
        WriteText(my_config.suction.Event3, "000000", strlen("000000"), sizeof(my_config.suction.Event3));
        WriteText(my_config.suction.Event4, "000000", strlen("000000"), sizeof(my_config.suction.Event4));
        WriteText(my_config.suction.Event5, "000000", strlen("000000"), sizeof(my_config.suction.Event5));
        WriteText(my_config.suction.Event6, "000000", strlen("000000"), sizeof(my_config.suction.Event6));
        WriteText(my_config.suction.Event7, "000000", strlen("000000"), sizeof(my_config.suction.Event7));
        WriteText(my_config.suction.Event8, "000000", strlen("000000"), sizeof(my_config.suction.Event8));
        WriteText(my_config.suction.Fname, "*B_*C_*Y*m*d*-*H*i*s.trz", strlen("*B_*C_*Y*m*d*-*H*i*s.trz"), sizeof(my_config.suction.Fname));


        WriteText(my_config.last, "1234", strlen("1234"), sizeof(my_config.last));
        //for(int i=0;i<18;i++){
        for(int i=0;i<11;i++){
            my_config.device.gap02[i] = (uint8_t)(0x30 + i);
        }

        tx_mutex_put(&mutex_sfmem);

        rewrite_settings();     //本体設定書き換え（関数内でmutex_sfmemを取得返却している)

   }
}


/**
 * 子機登録ファイル初期化
 */
void init_regist_file_default(void)
{
    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS)          // シリアルフラッシュ ミューテックス確保 スレッド確定
    {
        //serial_flash_multbyte_read(SFM_REGIST_START, sizeof(root_registry), (char *)&root_registry);  // ルート情報構造体に読み込む
        void file_load__registry(void);
        file_load__registry();

        memset((uint8_t *)&root_registry,0x00,sizeof(root_registry));
        root_registry.length = 16;
        root_registry.header = 0xfb;
        root_registry.group = 0;
        root_registry.auto_route = 0x01;
        root_registry.rssi_limit1 = 80;
        root_registry.rssi_limit2 = 60;
        root_registry.version = 0x02;

        //serial_flash_block_unlock();                        // グローバルブロックプロテクション解除
        //serial_flash_erase(SFM_REGIST_START, SFM_REGIST_END);
        //serial_flash_multbyte_write(SFM_REGIST_START, sizeof(root_registry), (char *)&root_registry);
        memset(&regist_data[0], 0xff, sizeof(regist_data)); // equivalent to erase flash (All 0xFF)
        memcpy(&regist_data[0], &root_registry, sizeof(root_registry)); //SFM_REGIST_START
        void file_store__registry(void);
        file_store__registry();

        tx_mutex_put(&mutex_sfmem);
    }

}

/**
 * グループファイル初期化
 * ヘッダ追加
 */
void init_group_file_default(void)
{
    static const uint8_t header[18] = {0x10,0x00,0x56,0x47,0x52,0x50,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS){

    	//serial_flash_block_unlock();                                // グローバルブロックプロテクション解除
        //serial_flash_erase(SFM_GROUP_START, SFM_GROUP_END);
        //serial_flash_multbyte_write(SFM_GROUP_START, (uint32_t)18, (char *)&header[0]);

        void file_load__group(void);
        file_load__group();
        memset(&group_data[0], 0xff, sizeof(group_data));
        memcpy(&group_data[0], &header[0], 18); //SFM_GROUP_START
        void file_store__group(void);
        file_store__group();

        tx_mutex_put(&mutex_sfmem);
    }
}
