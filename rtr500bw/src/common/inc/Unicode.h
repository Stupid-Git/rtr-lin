/**
 * @brief   Unicode文字列操作変換ヘッダファイル
 * @file    Unicode.h
 *
 *  Created on: 2019/09/03
 *      Author: tooru.hayashi
 */

#ifndef _UNICODE_H_
#define _UNICODE_H_



#include <stdio.h>
#include "Version.h"
#include "flag_def.h"
#include "MyDefine.h"
#include "Globals.h"
#include "Xml.h"
#include "General.h"
#undef EDF

#ifndef _UNICODE_C_
    #define EDF extern
#else
    #define EDF
#endif

//EDF char	EncodeTemp[200];		// Encode用

EDF int Sjis2UTF8( char *Dst, char *Src, int Max, char **Next );
EDF int Sjis2EUC( char *Dst, char *Src, int Max, char **Next );
EDF int ChangeUTF8( char *Dst, uint16_t Code );
EDF int Sjis2Jis( char *Dst, char *Src, int Max, char ** );
EDF int Sjis2JisHeader( char *Dst, char *Src, int Max );
EDF int Sjis2UTF8Header( char *Dst, char *Src, int Max );
EDF uint16_t ToMIME( char *Dst, char *Src, int Size );

EDF int WinStr2UTF8(char *Dst, char *Src, int Size );
EDF int Win2UTF8(char *Dst, char Data);

#endif /* _UNICODE_H_ */
