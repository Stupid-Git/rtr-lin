/*
 * base.h
 *
 *  Created on: Dec 2, 2024
 *      Author: karel
 */

#ifndef BASE_H_
#define BASE_H_

#include "_r500_config.h"

#ifdef EDF
#undef EDF
#endif
#ifdef _BASE_C_
#define EDF
#else
#define EDF extern
#endif

EDF void read_my_settings(void);
//EDF void read_repair_settings(void);  //->static
EDF void rewrite_settings(void);
//EDF void alter_settings_default(uint8_t);  //->static
EDF void init_factory_default(uint8_t, uint8_t);
EDF void init_regist_file_default(void);                // 子機登録ファイル初期化
EDF void init_group_file_default(void);                 // グループファイル初期化（ヘッダ追加）
EDF void write_system_settings(void);
//EDF void init_factory_system_default(uint32_t);     //->static
//EDF void read_system_settings(void);        //-> system_thread_entry.c

EDF int ceat_file_write(int area, int index, uint16_t size);
EDF void ceat_file_read(int area, int index);


#endif /* BASE_H_ */
