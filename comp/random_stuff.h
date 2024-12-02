/*
 * random_stuff.h
 *
 *  Created on: Nov 25, 2024
 *      Author: karel
 */

#ifndef RANDOM_STUFF_H_
#define RANDOM_STUFF_H_

#include "_r500_config.h"
#include "stdint.h"

#include "r500_defs.h"
#undef EDF
#ifndef RANDOM_STUFF_H_
#define EDF extern
#else
#define EDF
#endif

/*tbr
typedef struct {
    char     command;       //T
    char     subcommand;    //2
    uint16_t    length;
    char     data[8192-4];

}__attribute__ ((__packed__)) comT2_t;
*/


EDF int Memcmp(char *dst, char *src, int size_d, int size_s);

EDF int judge_T2checksum(comT2_t *pBuf);                         // チェックサム判定 T2コマンド用

EDF int AtoI( char *src, int size );                            // パラメータ名に対応したデータ位置を検索する
EDF int AtoL( char *src, int size );                           // サイズ付き atol() ※オーバーフローチェック無し


#endif /* RANDOM_STUFF_H_ */
