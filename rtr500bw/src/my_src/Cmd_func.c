/**
 * @file    Cmd_func.c
 *
 * @date    2020/02/07
 * @author  tooru.hayashi
 */

#define _CMD_FUNC_C_

#include <stdio.h>
#include <stdarg.h>
#include <limits.h>     //ULONG_MAX
#include <math.h>

#include "MyDefine.h"
#include "Globals.h"
#include "General.h"
#include "Cmd_func.h"
#include "cmd_thread_entry.h"

#if 0
//cmd_thread_entry.c/hに移動

/**
 * @brief   返送パラメータのセット
 * @param Name
 * @param fmt
 * @note    グローバル変数使用
 *          StsArea[]       返送パラメータエリア
 *          CmdStatusSize   返送パラメータのサイズ（ポインタ）
 */
void StatusPrintf( char *Name, const char *fmt, ... )
{
    va_list args;

    int len;
    char    *sz, *ps = &(StsArea[CmdStatusSize]);

    va_start( args, fmt );

    len = (int)strlen( Name );
    strcpy( ps, Name );
    ps += len;
    *ps++ = '=';
    sz = ps;
    ps += 2;
    CmdStatusSize += len + 1 + 2;       // '=' + size area

    len = vsprintf( ps, fmt, args );
    CmdStatusSize += len;
    *sz++ = (uint8_t)len;
    *sz   = (uint8_t)(len>>8);

    va_end( args );
}


/**
 * @brief   返送パラメータのセット 最大サイズ指定
 * @param max
 * @param Name
 * @param fmt
 * @note    グローバル変数使用
 *          StsArea[]       返送パラメータエリア
 *          CmdStatusSize   返送パラメータのサイズ（ポインタ）
 */
void StatusPrintf_S(uint16_t max,  char *Name, const char *fmt,  ... )
{
    va_list args;

    int     len;
    char   *sz, *ps = &(StsArea[CmdStatusSize]);

    va_start( args, fmt );

    len = (int)strlen( Name );
    strcpy( ps, Name );
    ps += len;
    *ps++ = '=';
    sz = ps;
    ps += 2;
    CmdStatusSize += len + 1 + 2;       // '=' + size area

    len = vsprintf( ps, fmt, args );
    if(len > max)
        len = max;
    CmdStatusSize += len;
    *sz++ = (uint8_t)len;
    *sz   = (uint8_t)(len>>8);

    va_end( args );
}


/**
 * @brief   返送パラメータのセット 最大サイズ指定
 * @param Name
 * @param Data
 * @param size
 * @param fmt
 * @note    グローバル変数使用
 *          StsArea[]       返送パラメータエリア
 *          CmdStatusSize   返送パラメータのサイズ（ポインタ）
 */
void StatusPrintf_v2s(char *Name, char *Data, int size, const char *fmt)
{

    char   *Csz;
    int    len,sz;
    int i;
    char tmp[8];
    uint32_t val = 0;


    Csz = (char *)&(StsArea[CmdStatusSize]);

    len = (int16_t)strlen( Name );
    memcpy( Csz, Name, (size_t)len );
    Csz += len;

    *Csz++ = '=';

    memcpy( tmp, Data, (size_t)size );
    for(i= size -1; i>=0;i--){
        val = (val <<8 ) | (uint8_t)tmp[i];   
    }

    sz = sprintf(tmp,fmt, val);

    *Csz++ = (uint8_t)( sz % 256 );         // Size L
    *Csz++ = (uint8_t)( sz / 256 );         // Size H

    memcpy( Csz, tmp, (size_t)sz );

    CmdStatusSize += len + 1 + 2 + sz;

}



/**
 * 返送パラメータのセット（バイナリ版）
 * @param Name
 * @param Data
 * @param Size
 * @note    グローバル変数使用
 *          StsArea[]       返送パラメータエリア
 *          CmdStatusSize   返送パラメータのサイズ（ポインタ）
 */
void StatusPrintfB( char *Name, char *Data, int Size )
{
    char   *Csz;
    int    len;

    Csz = (char *)&(StsArea[CmdStatusSize]);

    len = (int)strlen( Name );
    memcpy( Csz, Name, (size_t)len );
    Csz += len;

    *Csz++ = '=';

    *Csz++ = (uint8_t)( Size % 256 );         // Size L
    *Csz++ = (uint8_t)( Size / 256 );         // Size H

    if ( Size ){
        memcpy( Csz, Data, (size_t)Size );
    }

    CmdStatusSize += len + 1 + 2 + Size;
}


#endif


/**
 * @brief   パラメータ名に対応したデータ(10進数)を取得する
 * @param Key
 * @return
 */
int32_t ParamInt( char *Key )
{
    int32_t     Value = INT32_MAX;
    int32_t     Size;
    uint8_t       hash;

    hash = Hash( Key );
    if ( hash ) {               // 定義されている
        Size = PLIST.Arg[hash].Size;
        if ( Size ){
            Value = AtoI( PLIST.Arg[hash].Data, Size );
        }
    }

    return (Value);
}


/**
 * @brief   パラメータ名に対応したデータ(10進数)を取得する
 * @param Key
 * @return
 */
int32_t ParamHex( char *Key )
{
    int32_t Value = INT32_MAX;
    int32_t Size;
    uint8_t  hash;

    hash = Hash( Key );
    if ( hash ) {               // 定義されている
        Size = PLIST.Arg[hash].Size;
        if ( Size )
            Value = strtol(PLIST.Arg[hash].Data, 0x00, 16);
    }

    return (Value);
}




/**
 * @brief   パラメータ名に対応したデータ(10進数)を取得する
 * @param Key
 * @return
 */
int32_t ParamInt32( char *Key )
{
    int32_t    Value = INT32_MAX;
    int    Size;
    uint8_t    hash;

    hash = Hash( Key );
    if ( hash ) {               // 定義されている
        Size = PLIST.Arg[hash].Size;
        if ( Size ){
            Value = AtoL( PLIST.Arg[hash].Data, Size );
        }
    }

    return (Value);
}



/**
 * @brief   パラメータ名に対応したデータ(文字列)を取得する
 * @param Key
 * @param Adrs
 * @return
 * @see PLIST
 */
int32_t ParamAdrs( char *Key, char **Adrs )
{
    int    sz = -1;
    uint8_t   hash;


    //if(memcmp("RELAY", Key, 5) == 0) sz = 0;

    hash = Hash( Key );
    if ( hash ) {                       // 定義されている
        sz = PLIST.Arg[hash].Size;
        *Adrs = PLIST.Arg[hash].Data;
    }
    else{
        *Adrs = 0;                      // ポインタも0にしちゃう
    }

    return (sz);
}




/**
 * パラメータ名に対応したデータ位置を検索する
 * （将来的にはハッシュにしたいな～）
 * @param Key
 * @return
 * @note    見つからない場合は０を返す
 *  PLIST.Arg[0]は全て０がセットされているので、取得した場合全てが０（ヌルポインタ）となる。
 */
uint8_t Hash( char *Key )
{
    int   Count = PLIST.Count;            // パラメータ数
    uint8_t    i;

    for ( i=1; i<=Count; i++ ) {
        if ( strcmp( Key, PLIST.Arg[i].Name ) == 0 ) {
            Key = 0;        // find
            break;
        }
    }

    if ( Key != 0 ) {       // 型もへったくれもないな、しかし
        i = 0;
    }
    return (i);

// 見つからない場合は０を返す
// PLIST.Arg[0]は全て０がセットされているので、取得した場合
// 全てが０（ヌルポインタ）となる。

}


/// @brief  指定した文字列から不要な文字を削除する
/// @param[in] *str    入力文字列
/// @param[in] *delim  削除文字
/// @param[out] *outlist[]  削除文字を除いた文字列
/// @return 出力される文字列数
int split( char *str, const char *delim, char *outlist[] ) {
    char    *tk;
    int     cnt = 0;

    tk = strtok( str, delim );
    while( tk != NULL ) {
        outlist[cnt++] = tk;
        tk = strtok( NULL, delim );
    }
    return cnt;
}

/// @brief    指定した文字列をカンマ区切りにし、全体を[]で囲う
/// @param      *str  入力/出力文字列
/// @param      size  入力文字列数
/// @return     出力される文字列数
int comma_separate_array_make( char *str, int size ) {

    int i;
    int cnt = 0;
    char temp[REMOTE_UNIT_MAX+1];
    uint8_t atai;

    memset(temp,0x00,sizeof(temp));
    temp[cnt++] = '[';

    for(i=0; i<size; i++){
        atai = str[i];
        cnt += sprintf(&temp[cnt],"%d",atai);

        if(i == size-1){
            temp[cnt++] = ']';
        }else{
            temp[cnt++] = ',';
        }
    }
    memcpy(str, temp, (size_t)cnt);
    return cnt;
}
