/*
 * version.h
 *
 *  Created on: Dec 2, 2024
 *      Author: karel
 */

#ifndef VERSION_H_
#define VERSION_H_

#define VERSION_FW  "R.01.00.29"                // 2024 2 24  D.00.03.185のリリース版


/// XML
#define VERSION_XML     "1.26"          //
#define VERSION_XML_W   "1.00"
#define VERSION_PUSH    "1.00"          //
#define VERSION_XML_WDS "1.10"          // 警報XML_DataServer用     // sakaguchi 2021.03.01

/// EEPROMのフォーマット
#define EEPROM_VERSION  1

/// 登録ファイル
#define REGF_VERSION    1

#define MODE_CODE_STRING        "0815"

#define COMPANY_ID 0x0392

#define MAGIC_CODE  ((const char *)"T&D Corporation\0")

//PSoCデフォルト設定
#define CAP_TRIM_MAX    0x7F                    ///<PSoC CapTrim値最大 0x7F
#define CAP_TRIM_MIN    0x40                    ///<PSoC CapTrim値最小 0x40
#define ADV_UPDATE_INTERVAL_MAX 120             ///<アドバタイズデータ更新周期最大 120秒
#define ADV_UPDATE_INTERVAL_MIN 30              ///<アドバタイズデータ更新周期最小 30秒

#define DEFAULT_CAP_TRIM    0x63                ///<PSoC CapTrim値デフォルト
#define DEFAULT_ADV_UPDATE_INTERVAL 60          ///<アドバタイズデータ更新周期デフォルト値


#endif /* VERSION_H_ */
