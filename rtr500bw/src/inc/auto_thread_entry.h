/**
 * @file	auto_thread_entry.h
 *
 * @date	2020/07/07
 * @author	t.saito
 * @note    2020.Jul.07 rf_thread_entry.hから分離
 */

#ifndef INC_AUTO_THREAD_ENTRY_H_
#define INC_AUTO_THREAD_ENTRY_H_




#ifdef EDF
#undef EDF
#endif

#ifndef _AUTO_THREAD_ENTRY_C_
    #define EDF extern
#else
    #define EDF
#endif


//

EDF uint32_t AUTO_control(uint32_t actual_events);
EDF uint32_t RF_event_execute(uint8_t);                            // ＲＦコマンド実行
EDF uint8_t RF_monitor_execute(uint8_t order);        //auto_thread_entry.c

EDF uint8_t realscan_err_data_add(void);

EDF void WR_clr(void);
EDF void WR_clr_rfcnt(void);                                                    // sakaguchi 2021.04.07
EDF void set_flag(char *poi , uint8_t No);
EDF uint8_t check_flag(char *poi , uint8_t No);
EDF void copy_realscan(uint8_t format_no);


EDF uint8_t auto_realscan_New(uint8_t DATA_FORMAT);
EDF void get_regist_scan_unit_retry(void);

EDF uint8_t chk_sram_ng(void);

EDF uint8_t ALM_bak_poi(uint8_t G_No, uint8_t U_No);                            // sakaguchi 2021.03.01
EDF uint8_t WR_chk(uint8_t G_No, uint8_t U_No, uint8_t pogi);                   // sakaguchi 2021.03.01
EDF uint8_t WR_set(uint8_t G_No ,uint8_t U_No ,uint8_t pogi ,uint8_t setdata);  // sakaguchi 2021.03.01


/// 自律動作の自動吸上げをリトライする子機、中継機情報
typedef struct st_def_download_buff_back{                       // 32byte
    char do_rpt[16];                                    ///<  自動データ吸上げに使用する中継機リスト
    char do_unit[16];                               ///<  自動吸上げする子機リスト
}def_download_buff_back;


/// 自律動作の自動吸上げリトライ対象子機、中継機を示す用変数
EDF  struct def_retry_buff{                             // 256byte
    def_download_buff_back download_buff[8];
}retry_buff;


/// 自律動作のリアルスキャン対象子機、中継機を示す変数(1グループの最大子機番号は128とする)
EDF  struct def_realscan_buff{                          // 64byte
    char do_rpt[16];                                    ///<  通信すべき中継機リスト
    char do_unit[16];                               ///<  通信すべき子機リスト
    char over_rpt[16];                              ///<  通信が終わった中継機リスト
    char over_unit[16];                             ///<  通信できた子機リスト
}realscan_buff;


/// 自律動作の自動吸上げ対象子機、中継機を示す用変数(1グループの最大子機番号は128とする)
EDF  struct def_download_buff{                          // 64byte
    char do_rpt[16];                                    ///<  通信すべき　中継機リスト
    char do_unit[16];                               ///<  通信すべき　子機リスト
    char over_rpt[16];                              ///<  通信成功　通信が終わった中継機リスト
    char over_unit[16];                             ///<  通信成功　通信できた子機リスト
    char ng_rpt[16];                                    ///<  通信エラー　通信が終わった中継機リスト
    char ng_unit[16];                               ///<  通信エラー　通信できた子機リスト
}download_buff;






#endif /* INC_AUTO_THREAD_ENTRY_H_ */
