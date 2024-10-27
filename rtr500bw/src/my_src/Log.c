/**
 * @file Log.c
 *
 * @date	 Created on: 2019/11/06
 * @author	tooru.hayashi
 * @note	2020.01.30	v6ソースマージ済み
 * @note	2020.Jul.01 GitHub 630ソース反映済み
 * @note    2020.Jul.22 spi.rxbuff直読みを廃止
 */

#define _LOG_C_



#include <stdio.h>
#include <stdarg.h>

#include "MyDefine.h"
#include "Version.h"
#include "Globals.h"
#include "General.h"
#include "Config.h"
#include "Sfmem.h"
#include "DateTime.h"
//#include "system_thread.h"

#include "Log.h"

#define  LOG_DATA_SIZE      SFM_LOG_END - SFM_LOG_START
#define  LOG_DATA_SIZE_T    SFM_TD_LOG_END - SFM_TD_LOG_START


static char LogBuf[128];   ///<シリアルフラッシュからの読み込み用バッファ

char	LogTemp[300];
static  char log_sect[32*2];
static  int16_t log_number[32*2];

//static LOG_CTRL	LogCtrl;		///< 制御情報
static LOG_LINE LogLine;		///< 実データ構造体
static LOG_GET  LogGet;         ///< ログ取得用
static LOG_GET  LogGet_Http;    ///< ログ取得用（HTTP専用）	// sakaguchi 2020.12.07
static LOG_INFO LogInfo;        ///< ログ情報
static LOG_INFO LogInfo_T;      ///< ログ情報

int32_t PreReadLog(uint32_t adr, int area);
static void LOG_Write( uint16_t Cat, uint32_t Length, int area  );
static uint8_t LogMatch( uint32_t dst, LOG_LINE *src, int16_t size );

// Log エリアは、64KB block*２   sector size 4KB (4096) ブロック
// 1-log 128byteとして 1024- 32 log
// 1-sector(4096) 32 log 毎に消去される 
//
// Sizeは、128KB 0x0000 0x1FFFF 
//
//  
//#define SFM_LOG_START               0x00070000          // シリアルフラッシュメモリ番地 LOG 先頭番地
//#define SFM_LOG_START2              0x00080000          // シリアルフラッシュメモリ番地 LOG 先頭番地
//#define SFM_LOG_END                 0x0008ffff          // シリアルフラッシュメモリ番地 LOG  終了番地
//#define SFM_LOG_SECT                0x00001000          // セクタサイズ


/**
 * @brief   ネットワークログの書込み
 * @param stat   ネットワーク通信の結果（コード）
 * @param fmt    ネットワーク通信の結果（メッセージ）のフォーマット
 * @param msg    ネットワーク通信の結果（メッセージ）最大30byte
 * @return
 * @note    EBSTSのパラメータで使用する
 */
void Net_LOG_Write( uint32_t stat, const char *fmt, UINT msg)
{
    int mode;

    NetLogInfo.NetUtc = RTC_GetGMTSec();            // 時刻(UTIM)

    mode = ((my_config.websrv.Mode[0] == 0) || (my_config.websrv.Mode[0] == 1)) ? CONNECT_HTTP : CONNECT_HTTPS;
    // HTTP通信でセッションの場合、106に置換する
    if((200 == stat) && (CONNECT_HTTP == mode)){
        stat = 106;
    }

    NetLogInfo.NetStat = stat;                      // ネットワーク通信の結果（コード）

    sprintf(&NetLogInfo.NetMsg[0], fmt, msg);         // ネットワーク通信の結果（メッセージ）
}



/**
 * ログのクリア
 */
void ResetLog(void)
{
    if(tx_mutex_get(&mutex_log, WAIT_100MSEC) == TX_SUCCESS){

        serial_flash_erase(SFM_LOG_START, SFM_LOG_END);
//        tx_mutex_put(&mutex_sfmem);
        tx_mutex_put(&mutex_log);       // 2023.02.20

        LogInfo.Size = 0;
        LogInfo.Count = 0;
        LogInfo.pStart = 0;             // 開始ポインタ(最古データへのポインタ)
        LogInfo.pWrite = 0;             // 次に書き込むポインタ
        LogInfo.NextNumber = 0;
        LogInfo.FirstNumber = 0;

        GetLog( 0, -1, 0, 0 );          // ログ取得用バッファクリア                 // sakaguchi 2020.12.07
//        GetLog_http( 0, -1, 0, 0 );     // ログ取得用(HTTP通信専用)バッファクリア   // sakaguchi 2020.12.07
        GetLog_http( 0, -1, 0, 0, FILE_L );  // ログ取得用(HTTP通信専用)バッファクリア   // 2023.03.01
    }

}

// sakaguchi 2020.12.07 ↓
/**
 * @brief   ログの読み込み（HTTP通信専用）
 * @param Adr       0=読み出し開始   ログを入れるバッファアドレス
 * @param Number    0～9999= 指定されたNumberから残り全て  -1 : 全て
 * @param MaxSize   バッファの最大サイズ(1以上)
 * @param area      0:user  1:debug log
 * @param file      FILE_L:通常送信 / FILE_L_T:テスト送信
 * @return
 * @note    足りなくても１ラインは出すのでMaxSize=0とすると良い
 * @note    Number指定は使用しない
 */
//uint16_t GetLog_http( char *Adr, int Number, int MaxSize, int area )
uint16_t GetLog_http( char *Adr, int Number, int MaxSize, int area, int file )      // 2023.03.01
{
    uint16_t total = 0;
    uint16_t pre;
    uint32_t adr;
    uint32_t size;
    uint8_t log_cnt = LOG_SEND_CNT;           // ログの蓄積件数   // 2023.03.01 // 2023.08.01 define定義

    if(area==0){
        adr =  SFM_LOG_START;
        size = LOG_DATA_SIZE;
    }
    else
    {
        adr =  SFM_TD_LOG_START;
        size = LOG_DATA_SIZE_T;
    }


    if(!Adr){
        LogGet_Http.pRead =   size + 1;              // リードポインタ初期化
        Number = ( 0 <= Number && Number <= 9999 ) ? Number : -1;
        LogGet_Http.StartNum = Number; 
    }
    else
    {
        if(LogGet_Http.pRead > size){                  // 初期化直後
            if(area==0){
                LogGet_Http.pRead = LogInfo.pStart;     // リードポインタ初期化
			    LogGet_Http.SaveSize = LogInfo.Count;	// 現在保存されているログの合計
            }
            else{
                LogGet_Http.pRead = LogInfo_T.pStart;   // リードポインタ初期化
			    LogGet_Http.SaveSize = LogInfo_T.Count; // 現在保存されているログの合計
            }
        }
        // ログの差分出力 リードポインタは前回の値を引き継ぎ、ログの合計はライトポインタとリードポインタの差分で計算する
        else{
            if(0 == LogGet_Http.SaveSize){
                if(area==0){                                                           // 通常ログ
                    if(LogInfo.pWrite >= LogGet_Http.pRead){
                        LogGet_Http.SaveSize = (LogInfo.pWrite - LogGet_Http.pRead)/128;	                // 差分ログの合計
                    }else{
                        LogGet_Http.SaveSize = (size + 1 - (LogGet_Http.pRead - LogInfo.pWrite))/128;       // 差分ログの合計
    }
                }
                else{                                                                  // デバッグログ
                    if(LogInfo_T.pWrite >= LogGet_Http.pRead){
                        LogGet_Http.SaveSize = (LogInfo_T.pWrite - LogGet_Http.pRead)/128;	            // 差分ログの合計
                    }else{
                        LogGet_Http.SaveSize = (size + 1 - (LogGet_Http.pRead - LogInfo_T.pWrite))/128;   // 差分ログの合計
                    }
                }
            }
        }

        Number = (int16_t)LogGet_Http.StartNum;
        Printf("LogGet_Http.pRead %ld SaveSize %ld \r\n", LogGet_Http.pRead, LogGet_Http.SaveSize);

        if(file == FILE_L_T){       // ログ送信テストは1件でもあれば送信する     // 2023.03.01
            log_cnt = 1;
        }

//        if(LogGet_Http.SaveSize>0){
        if(LogGet_Http.SaveSize >= log_cnt){               // 2023.03.01 指定したログ件数が蓄積されていたらデータを生成
            if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS){

                do{
                    if(area==0){
                        adr = SFM_LOG_START + LogGet_Http.pRead;
                    }
                    else{
                        adr = SFM_TD_LOG_START + LogGet_Http.pRead;
                    }
                    serial_flash_multbyte_read(adr, 128, (char *)&LogLine);

                    if(LogLine.Size>128){
                        Printf("LogLine.Size Over %d\r\n", LogLine.Size);
                        break;
                    }

                    pre = (uint16_t)LogMatch( (uint32_t)Adr, &LogLine, LogLine.Size);
                    total = (uint16_t)(total + pre);
                    Adr = (char *)( (uint32_t)Adr + pre );	// アドレス加算

                    MaxSize = (int16_t)(MaxSize - pre);		// 最大値を減らす

                    LogGet_Http.pRead += 128;
                    adr += 128;
                    if(LogGet_Http.pRead > size)
                        LogGet_Http.pRead = 0;
                    LogGet_Http.SaveSize--;
                    if(LogGet_Http.SaveSize==0)
                        break;
                }while(MaxSize > 0);

                tx_mutex_put(&mutex_sfmem);

            }
        }
        else{                                       // 2023.03.01
            LogGet_Http.SaveSize = 0;
        }

    }

    Printf("LogGet_Http.StartNum %d  size %d  total %d\r\n", LogGet_Http.StartNum, LogGet_Http.SaveSize, total);

    return (total);
}
// sakaguchi 2020.12.07 ↑

// 2023.08.01 ↓
/**
 * @brief   ログ件数読み込み（HTTP通信専用）
 * @param area      0:user  1:debug log
 * @return ログ件数
 */
uint16_t GetLogCnt_http(int area)
{
    uint32_t adr;
    uint32_t size;
    uint16_t log_cnt = 0;           // ログの蓄積件数

    if(area==0){
        size = LOG_DATA_SIZE;
    }else{
        size = LOG_DATA_SIZE_T;
    }

    if(LogGet_Http.pRead > size){                  // 初期化直後
        if(area==0){
            log_cnt = LogInfo.Count;
        }else{
            log_cnt = LogInfo_T.Count;
        }
    }
    // ログの差分出力 リードポインタは前回の値を引き継ぎ、ログの合計はライトポインタとリードポインタの差分で計算する
    else{
        if(0 == LogGet_Http.SaveSize){
            if(area==0){                                                           // 通常ログ
                if(LogInfo.pWrite >= LogGet_Http.pRead){
                    log_cnt = (LogInfo.pWrite - LogGet_Http.pRead)/128;	                // 差分ログの合計
                }else{
                    log_cnt = (size + 1 - (LogGet_Http.pRead - LogInfo.pWrite))/128;       // 差分ログの合計
                }
            }
            else{                                                                  // デバッグログ
                if(LogInfo_T.pWrite >= LogGet_Http.pRead){
                    log_cnt = (LogInfo_T.pWrite - LogGet_Http.pRead)/128;	            // 差分ログの合計
                }else{
                    log_cnt = (size + 1 - (LogGet_Http.pRead - LogInfo_T.pWrite))/128;   // 差分ログの合計
                }
            }
        }else{
            log_cnt = LogGet_Http.SaveSize;
        }
    }

    return (log_cnt);
}
// 2023.08.01 ↑



/**
 * @brief   ログの読み込み
 * @param Adr       0=読み出し開始   ログを入れるバッファアドレス
 * @param Number    0～9999= 指定されたNumberから残り全て  -1 : 全て
 * @param MaxSize   バッファの最大サイズ(1以上)
 * @param area      0:user  1:debug log
 * @return
 * @note    足りなくても１ラインは出すのでMaxSize=0とすると良い
 * @note    Number指定は使用しない
 */
uint16_t GetLog( char *Adr, int Number, int MaxSize, int area )
{
    uint16_t total = 0;
    uint16_t pre;
    uint32_t adr;
    uint32_t size;


    if(area==0){
        adr =  SFM_LOG_START;
        size = LOG_DATA_SIZE;
    }
    else
    {
        adr =  SFM_TD_LOG_START;
        size = LOG_DATA_SIZE_T;
    }
    

    if(!Adr){
        LogGet.pRead =   size + 1;              // リードポインタ初期化
        Number = ( 0 <= Number && Number <= 9999 ) ? Number : -1;
        LogGet.StartNum = Number; 
    }
    else
    {
        if(LogGet.pRead > size){                  // 初期化直後
            if(area==0){
                LogGet.pRead = LogInfo.pStart;      // リードポインタ初期化
			    LogGet.SaveSize = LogInfo.Count;	// 現在保存されているログの合計
            }
            else{
                LogGet.pRead = LogInfo_T.pStart;      // リードポインタ初期化
			    LogGet.SaveSize = LogInfo_T.Count;	// 現在保存されているログの合計
            }
        }
// sakaguchi 2020.12.07 ↓
//sakaguchi add UT-0014 ↓
        // ログの差分出力 リードポインタは前回の値を引き継ぎ、ログの合計はライトポインタとリードポインタの差分で計算する
//        else{
//            if(0 == LogGet.SaveSize){
//                if(area==0){                                                           // 通常ログ
//                    if(LogInfo.pWrite >= LogGet.pRead){
//                        LogGet.SaveSize = (LogInfo.pWrite - LogGet.pRead)/128;	                // 差分ログの合計
//                    }else{
//                        LogGet.SaveSize = (size + 1 - (LogGet.pRead - LogInfo.pWrite))/128;     // 差分ログの合計
//                    }
//                }
//                else{                                                                  // デバッグログ
//                    if(LogInfo_T.pWrite >= LogGet.pRead){
//                        LogGet.SaveSize = (LogInfo_T.pWrite - LogGet.pRead)/128;	            // 差分ログの合計
//                    }else{
//                        LogGet.SaveSize = (size + 1 - (LogGet.pRead - LogInfo_T.pWrite))/128;   // 差分ログの合計
//                    }
//                }
//            }
//        }
//sakaguchi add UT-0014 ↑
// sakaguchi 2020.12.07 ↑

        Number = (int16_t)LogGet.StartNum;
        Printf("LogGet.pRead %ld SaveSize %ld \r\n", LogGet.pRead, LogGet.SaveSize);
        if(LogGet.SaveSize>0){
            if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS){
            
                do{            
                    if(area==0){
                        adr = SFM_LOG_START + LogGet.pRead;
                    }
                    else{
                        adr = SFM_TD_LOG_START + LogGet.pRead;
                    }
                    serial_flash_multbyte_read(adr, 128, (char *)&LogLine);
#if 0
                    serial_flash_multbyte_read(adr, 128, LogBuf);
                    memcpy(&LogLine, &LogBuf[0], 128);
#endif
                    if(LogLine.Size>128){
                        Printf("LogLine.Size Over %d\r\n", LogLine.Size);
                        break;
                    }
 
                    pre = (uint16_t)LogMatch( (uint32_t)Adr, &LogLine, LogLine.Size);
                    total = (uint16_t)(total + pre);
                    Adr = (char *)( (uint32_t)Adr + pre );	// アドレス加算

                    MaxSize = (int16_t)(MaxSize - pre);		// 最大値を減らす

                    LogGet.pRead += 128;
                    adr += 128;
                    if(LogGet.pRead > size)
                        LogGet.pRead = 0;
                    LogGet.SaveSize--;
                    if(LogGet.SaveSize==0)
                        break;
                }while(MaxSize > 0);
            
                tx_mutex_put(&mutex_sfmem);
            
            }
        }

    }
    
    Printf("LogGet.StartNum %d  size %d  total %d\r\n", LogGet.StartNum, LogGet.SaveSize, total);

    return (total);
}


 /**
  * ログの書き込み
  * @param Flag
  * @param fmt
  */
void PutLog( uint8_t Flag, const char *fmt, ... )
{
	va_list args;

	int	size;

    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS){     // 排他制御（LOG_Write関数内から移動）2022.03.23 mv

        va_start( args, fmt );

        size = vsprintf( (char *)LogTemp, fmt, args );	// SRAMのテンポラリへいったん作成
        if(size > LOG_LINE_SIZE){
            size = LOG_LINE_SIZE;   
        }

        Printf( "=====>> LOG[%s:%s](%d) \r\n", CatMsg[Flag], LogTemp, size );
        //Printf( "### Size=%08lX Count=%04X Number=%04X \r\n ", LogCtrl.Size, LogCtrl.Count,LogCtrl.Number) ;	

        LOG_Write(Flag,(uint32_t)size, 0);

        va_end( args );

        tx_mutex_put(&mutex_sfmem);             // 排他制御（LOG_Write関数内から移動）2022.03.23 mv
    }
}

/**
  * ログの書き込み
  * @param Flag
  * @param fmt
  */
void DebugLog( uint8_t Flag, const char *fmt, ... )
{
	va_list args;

	int	size;

    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS){     // 排他制御（LOG_Write関数内から移動）2022.03.23 mv

        va_start( args, fmt );

        size = vsprintf( (char *)LogTemp, fmt, args );	// SRAMのテンポラリへいったん作成
        if(size > LOG_LINE_SIZE)
            size = LOG_LINE_SIZE;   

        Printf( "=====>> Debug LOG[%s:%s](%d) \r\n", CatMsg[Flag], LogTemp, size );
        //Printf( "### Size=%08lX Count=%04X Number=%04X \r\n ", LogCtrl.Size, LogCtrl.Count,LogCtrl.Number) ;	

        LOG_Write(Flag,(uint32_t)size, 1);

        va_end( args );

        tx_mutex_put(&mutex_sfmem);             // 排他制御（LOG_Write関数内から移動）2022.03.23 mv
    }
}



/**
 * １ログの書き込み
 * @param Cat
 * @param Length
 * @param area
 */
static void LOG_Write( uint16_t Cat, uint32_t Length, int area )
{
    uint32_t adr;
    uint32_t Size;
//    int i;

    memset((uint8_t *)&LogLine, 0x00, sizeof(LogLine));

    if ( Length >= sizeof( LogLine.Data ) ){
		Length = sizeof( LogLine.Data );		// 文字数超えたらカット
		LogTemp[Length++] = '\0';					// データ部の最後はnullに
	}
	else{
		LogTemp[Length++] = '\0';					// データ部の最後はnullに
	}

    memcpy(&LogLine.Data, &LogTemp, Length);    

    Size = (Length + sizeof( LogLine ) - sizeof( LogLine.Data ) - sizeof( LogLine.gap));	// データ部以外を加算



    LogLine.Size = (uint8_t)Size;
	LogLine.Category = (uint8_t)Cat;
    if(area==0){
	LogLine.Number = LogInfo.NextNumber;
    }
    else
    {
    	LogLine.Number = LogInfo_T.NextNumber;
    }
    
	LogLine.GSec = RTC_GetGMTSec();					// GMT秒セット
    LogLine.LSec = RTC_GetLCTSec(LogLine.GSec);		// LCT秒へ変換後セット
/*
    Printf( "LogCtrl.pWrite   %04X\r\n", LogCtrl.pWrite);

    Printf( "LOG_Write Size   %02X\r\n", Size) ;	
    Printf( "LOG_Write Cat    %02X\r\n", LogLine.Category) ;	
    Printf( "LOG_Write Number %02X\r\n", LogLine.Number) ;	
    Printf( "LOG_Write GSec   %dl\r\n", LogLine.GSec) ;	
    Printf( "LOG_Write LTC    %dl\r\n", LogLine.LSec) ;	
    Printf( "LOG_Write mgs    %s\r\n", LogLine.Data) ;	
    Printf( "Log Address      %ld\r\n", LogInfo.pWrite);
*/
 
    if ( Size == 0 ){
        Printf( "LOG_Write Size Zero %02X\r\n", Size) ;
    }
	
    //if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS){   // LOG_Write関数外へ移動 2022.03.23 del

        serial_flash_block_unlock();                                // グローバルブロックプロテクション解除

        if(area==0){
            adr = LogInfo.pWrite;
// sakaguchi 2020.12.07 ↓
            adr += 128;                                                  // 次ブロックを算出
            if(adr >= LOG_DATA_SIZE){
                adr = 0;
            }
// sakaguchi 2020.12.07 ↑
            if((adr % 4096) == 0){                                       // ブロックの先頭では、消去を行う    
                //Printf("Next block erase %ld\r\n", adr);
                serial_flash_sector_erase(SFM_LOG_START + adr );                // sector erase 4kbyte
            }
            serial_flash_multbyte_write(SFM_LOG_START + LogInfo.pWrite, 128, (char *)&LogLine);

            LogInfo.pWrite += 128;
            if(LogInfo.pWrite >= LOG_DATA_SIZE ){
                LogInfo.pWrite = 0;
            }
            LogInfo.NextNumber ++;
            if(LogInfo.NextNumber >= 10000){
                LogInfo.NextNumber = 0;
            }
        }
        else{
            adr = LogInfo_T.pWrite;
// sakaguchi 2020.12.07 ↓
            adr += 128;                                                  // 次ブロックを算出
            if(adr >= LOG_DATA_SIZE_T){
                adr = 0;
            }
// sakaguchi 2020.12.07 ↑
            if((adr % 4096) == 0){                                       // ブロックの先頭では、消去を行う    
                Printf("Next block erase %ld\r\n", adr);
                serial_flash_sector_erase(SFM_TD_LOG_START + adr );                // sector erase 4kbyte
            }
            serial_flash_multbyte_write(SFM_TD_LOG_START + LogInfo_T.pWrite, 128, (char *)&LogLine);

            LogInfo_T.pWrite += 128;
            if(LogInfo_T.pWrite >= LOG_DATA_SIZE_T ){
                LogInfo_T.pWrite = 0;
            }
            LogInfo_T.NextNumber ++;
            if(LogInfo_T.NextNumber >= 10000){
                LogInfo_T.NextNumber = 0;
            }

        }

        //tx_mutex_put(&mutex_sfmem);       // LOG_Write関数外へ移動 2022.03.23 del
    //}


}



/**
 * ブロック先頭での ブランクのチェック
 * @param adr       アドレス
 * @param area      エリア 0:User log  1:T&D log
 * @return          0:ブランク -1:使用済み
 */
int32_t PreReadLog(uint32_t adr, int area)
{
    int32_t sts = 0;
    int32_t j;

    if(area == 0){
    adr +=  SFM_LOG_START; 
    }
    else{
        adr +=  SFM_TD_LOG_START; 
    }

    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS){

        serial_flash_multbyte_read(adr, 128, LogBuf);

        for(j = 0; j < 128; j++)
        {
            if(LogBuf[j] != 0xff){
                sts = -1;
            }
        }

        tx_mutex_put(&mutex_sfmem);

    }

    return (sts);
}


//===================================================
// 機 能 : 最初、最後のログの検出
// 引 数 : アドレス
// 返 値 : 空きの先頭アドレス
//
//  Log Area   64K block + 64K block
//  4k sector が、32個
//
//	uint32_t	Size;
//	uint32_t	pStart;			// 開始ポインタ(最古データへのポインタ)
//	uint32_t	pWrite;			// 次に書き込むポインタ
//	uint16_t	Count;			// ログ数
//	uint16_t	NextNumber;		// シリアル番号（次に書かれる番号）
//	uint16_t	FirstNumber;	// 最も古いログ番号
//
//  log_sect    0:空き  1:フル  2:使いかけ
//===================================================
#define MAX_SECTOR      32L     // 全セクター数
#define BLOCK           32L     // 1 sectorのブロック数

/**
 * ログの使用状況を取得
 * @param area
 * @return
 */
int32_t GetLogInfo(int area)
{
    uint32_t adr;
    int32_t end_adr;

    int i,j; //start, end, end_s;
    int max_sector;

    int32_t no_brank_count = 0;             // no brank count
    int32_t bk_e,bk_s;                      // end block   start block  user 32(4*32=128k) tand 64(4*64=256) block
    uint16_t FirstNumber;

    for(i=0;i<64;i++){
        log_sect[i] = 0;
        log_number[i] = -1;
    }

    if(area==0){
    adr =  SFM_LOG_START;
        max_sector = MAX_SECTOR;
    }
    else
    {
        adr =  SFM_TD_LOG_START;
        max_sector = MAX_SECTOR * 2;
    }


    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS){

        //セクタの状態取得
        for(i=0;i<max_sector;i++){
            serial_flash_multbyte_read((uint32_t)(adr + (uint32_t)i*4096), 128, LogBuf);

            if(LogBuf[0] == 0xff){        // logの種別
                log_sect[i] = 0;            // brank
// sakaguchi 2020.12.07 ↓
                if(i==0){                 // 先頭セクタが空で、次セクタにログがある場合も一周したと判断し、最古ログ番号を格納する
                    serial_flash_multbyte_read((uint32_t)(adr + (uint32_t)(i+1)*4096), 128, LogBuf);
                    if(LogBuf[0] != 0xff){
                        if(area==0){
                            FirstNumber = LogInfo.FirstNumber = (uint16_t)(LogBuf[2] + LogBuf[3] * 256);
                        }
                        else{
                            FirstNumber = LogInfo_T.FirstNumber = (uint16_t)(LogBuf[2] + LogBuf[3] * 256);
                        }
                    }
                }
// sakaguchi 2020.12.07 ↑
            }
            else{
                if(i==0){
                    if(area==0){
                        FirstNumber = LogInfo.FirstNumber = (uint16_t)(LogBuf[2] + LogBuf[3] * 256);
                    }
                    else{
                        FirstNumber = LogInfo_T.FirstNumber = (uint16_t)(LogBuf[2] + LogBuf[3] * 256);
                    }
                }    

                log_sect[i] = 1;            // full use
                no_brank_count ++;
                serial_flash_multbyte_read(adr + (uint32_t)i*4096 +128*31, 128, LogBuf);     // セクタの最終ブロック
                if(LogBuf[0] == 0xff){
                    log_sect[i] = 2;        // use and brank
                }
            }
            //Printf("%d(%d) ", i,log_sect[i]);
        }

        //Printf("\r\nLog brank secter %d\r\n", 32- no_brank_count);
        if(no_brank_count==0){                         // 全部ブランク
            if(area==0){
            LogInfo.Size = 0;   
            LogInfo.Count = 0;   
            LogInfo.pStart = 0;             // 開始ポインタ(最古データへのポインタ)
            LogInfo.pWrite = 0;             // 次に書き込むポインタ    
            LogInfo.NextNumber = 0;   
            LogInfo.FirstNumber = 0;
                FirstNumber = 0;
            }
            else
            {
                LogInfo_T.Size = 0;   
                LogInfo_T.Count = 0;   
                LogInfo_T.pStart = 0;             // 開始ポインタ(最古データへのポインタ)
                LogInfo_T.pWrite = 0;             // 次に書き込むポインタ    
                LogInfo_T.NextNumber = 0;   
                LogInfo_T.FirstNumber = 0;
                FirstNumber = 0;
            }
            
        }
        else if(no_brank_count==1){                 // 最初のブロックのみ使用
            if(area==0){
            LogInfo.pStart = 0;
            LogInfo.FirstNumber = 0;
            end_adr = -1;
                for( j=31;j>=0;j--){        // block内(4k)のスキャン
                serial_flash_multbyte_read(adr + (uint32_t)j*128, 128, LogBuf);
                if(LogBuf[0] != 0xff){
                    end_adr = (j+1)*128;    
                    LogInfo.NextNumber = (uint16_t)(LogBuf[2] + (uint16_t)LogBuf[3] * 256 + 1);
                    break;
                }
            }
            LogInfo.pWrite = (uint32_t)end_adr;
            LogInfo.Size = LogInfo.NextNumber;
        }
            else{
                LogInfo_T.pStart = 0;
                LogInfo_T.FirstNumber = 0;
                end_adr = -1;
                for( j=31;j>=0;j--){        // block内(4k)のスキャン
                    serial_flash_multbyte_read(adr + (uint32_t)j*128, 128, LogBuf);
                    if(LogBuf[0] != 0xff){
                        end_adr = (j+1)*128;    
                        LogInfo_T.NextNumber = (uint16_t)(LogBuf[2] + (uint16_t)LogBuf[3] * 256 + 1);
                        break;
                    }
                }
                LogInfo_T.pWrite = (uint32_t)end_adr;
                LogInfo_T.Size = LogInfo_T.NextNumber;
            }
        }
        //else if(LogInfo.FirstNumber==0){    // １周していない
        else if(FirstNumber==0){    // １周していない
            //Printf("\r\nLog tpye 2\r\n");
            if(area==0){
            LogInfo.pStart = 0;   
            LogInfo.FirstNumber = 0;  
            }
            else{
                LogInfo_T.pStart = 0;   
                LogInfo_T.FirstNumber = 0;
            }

            for(i=0;i<max_sector;i++){              // sector
                if(log_sect[i]==2){         // 使いかけ
                    //Printf("\r\nLog tpye 3(%d)\r\n",i);

                    bk_e = i;
                    if(area==0){
                    bk_s = (i==31) ? 0 : i+1;

                    for( j=BLOCK-1;j>=0;j--){  // block 
                        serial_flash_multbyte_read(adr + (uint32_t)bk_e*4096 + (uint32_t)j*128, 128, LogBuf);
                        if(LogBuf[0] != 0xff){
                            end_adr = (j+1)*128 + bk_e*4096;    
                            LogInfo.NextNumber = (uint16_t)(LogBuf[2] + (uint16_t)LogBuf[3] * 256 + 1);
                            break;  
                        }
                    }

                    LogInfo.pWrite = (uint32_t)end_adr;
                    }
                    else{
                        bk_s = (i==63) ? 0 : i+1;

                        for( j=BLOCK-1;j>=0;j--){  // block 
                            serial_flash_multbyte_read(adr + (uint32_t)bk_e*4096 + (uint32_t)j*128, 128, LogBuf);
                            if(LogBuf[0] != 0xff){
                                end_adr = (j+1)*128 + bk_e*4096;    
                                LogInfo_T.NextNumber = (uint16_t)(LogBuf[2] + (uint16_t)LogBuf[3] * 256 + 1);
                                break;  
                            }
                        }

                        LogInfo_T.pWrite = (uint32_t)end_adr;
                    }
                    break;
                }
                if(log_sect[i]==0){     // このセクタは空き 最新のログは前のセクタ、最古は最初のセクタ
                    
                    bk_e = i;

                    serial_flash_multbyte_read(adr + (uint32_t)bk_e*4096 - 128, 128, LogBuf);
                    if(area==0){
                    LogInfo.NextNumber = (uint16_t)(LogBuf[2] + (uint16_t)LogBuf[3] * 256 + 1);
                    LogInfo.pWrite = (uint32_t)(bk_e*4096);
                    }
                    else
                    {
                        LogInfo_T.NextNumber = (uint16_t)(LogBuf[2] + (uint16_t)LogBuf[3] * 256 + 1);
                        LogInfo_T.pWrite = (uint32_t)(bk_e*4096);
                    }
                    
                    break;
                }
            }

        }
        else{   
            // 空きブロック、使いかけブロックを検索
            // 見つかったブロックが、最終ブロックの場合
            //Printf("\r\nLog tpye 5\r\n");
            end_adr = -1;
            for(i=0;i<max_sector;i++){   // sector
                if(log_sect[i]==2){     // 使いかけ
                    //Printf("\r\nLog tpye 6\r\n");

                    bk_e = i;
                    

                    if(area==0)
                    {
                    bk_s = (i==31) ? 0 : i+1;

                    LogInfo.pStart = (uint32_t)(bk_s * 4096);
                    for( j=BLOCK-1;j>=0;j--){  // block
                        serial_flash_multbyte_read((uint32_t)(adr + (uint32_t)bk_e*4096 + (uint32_t)j*128), 128, LogBuf);
                        if(LogBuf[0] != 0xff){
                            end_adr = (j+1)*128 + bk_e*4096;      
                            LogInfo.NextNumber = (uint16_t)(LogBuf[2] + LogBuf[3] * 256 + 1);
                            break;  
                        }
                    }
                    serial_flash_multbyte_read((uint32_t)(adr + (uint32_t)bk_s*4096), 128, LogBuf);
                    LogInfo.FirstNumber = (uint16_t)(LogBuf[2] + (uint16_t)LogBuf[3] * 256);
                    LogInfo.pWrite = (uint32_t)end_adr;
                    }
                    else
                    {
                        bk_s = (i==63) ? 0 : i+1;

                        LogInfo_T.pStart = (uint32_t)(bk_s * 4096);
                        for( j=BLOCK-1;j>=0;j--){  // block
                            serial_flash_multbyte_read((uint32_t)(adr + (uint32_t)bk_e*4096 + (uint32_t)j*128), 128, LogBuf);
                            if(LogBuf[0] != 0xff){
                                end_adr = (j+1)*128 + bk_e*4096;      
                                LogInfo_T.NextNumber = (uint16_t)(LogBuf[2] + LogBuf[3] * 256 + 1);
                                break;  
                            }
                        }
                        serial_flash_multbyte_read((uint32_t)(adr + (uint32_t)bk_s*4096), 128, LogBuf);
                        LogInfo_T.FirstNumber = (uint16_t)(LogBuf[2] + (uint16_t)LogBuf[3] * 256);
                        LogInfo_T.pWrite = (uint32_t)end_adr;
                    }
                    
                    break;
                    
                }
                if(log_sect[i]==0){     // 全部空き 
                    //Printf("\r\nLog tpye 7\r\n");

                    if(area==0){
                    bk_e = (i==0) ? 31 : i-1;
                    bk_s = (i==31) ? 0 : i+1;

                    LogInfo.pStart = (uint32_t)(bk_s * 4096);

                        //serial_flash_multbyte_read((uint32_t)(adr + (uint32_t)bk_e*4096 - 128), 128, LogBuf);
                        serial_flash_multbyte_read((uint32_t)(adr + (uint32_t)bk_e*4096 + (4096-128)), 128, LogBuf);    // 空きセクタの前の最終ブロック     // sakaguchi 2020.12.07
                    LogInfo.NextNumber = (uint16_t)(LogBuf[2] + (uint16_t)LogBuf[3] * 256 + 1);
                        serial_flash_multbyte_read((uint32_t)(adr + (uint32_t)bk_s*4096), 128, LogBuf);                 // 空きセクタの次の先頭ブロック
                    LogInfo.FirstNumber = (uint16_t)(LogBuf[2] + (uint16_t)LogBuf[3] * 256);
                        //LogInfo.pWrite = (uint32_t)(bk_e*4096);
                        LogInfo.pWrite = (uint32_t)(i*4096);                                                            // 空きセクタの先頭アドレス         // sakaguchi 2020.12.07
                    }
                    else{
                        bk_e = (i==0) ? 63 : i-1;
                        bk_s = (i==63) ? 0 : i+1;

                        //LogInfo.pStart = (uint32_t)(bk_s * 4096);
                        LogInfo_T.pStart = (uint32_t)(bk_s * 4096);                                                     // sakaguchi 2020.12.07

                        //serial_flash_multbyte_read((uint32_t)(adr + (uint32_t)bk_e*4096 - 128), 128, LogBuf);
                        serial_flash_multbyte_read((uint32_t)(adr + (uint32_t)bk_e*4096 + (4096-128)), 128, LogBuf);    // sakaguchi 2020.12.07
                        LogInfo_T.NextNumber = (uint16_t)(LogBuf[2] + (uint16_t)LogBuf[3] * 256 + 1);
                        serial_flash_multbyte_read((uint32_t)(adr + (uint32_t)bk_s*4096), 128, LogBuf);
                        LogInfo_T.FirstNumber = (uint16_t)(LogBuf[2] + (uint16_t)LogBuf[3] * 256);
                        //LogInfo_T.pWrite = (uint32_t)(bk_e*4096); 
                        LogInfo_T.pWrite = (uint32_t)(i*4096);                                                          // sakaguchi 2020.12.07
                    }

                    break;

                }
            }
        }

//sakaguchi cg UT-0014 ↓
        if(area==0){
        if(LogInfo.NextNumber >= LogInfo.FirstNumber){
//                LogInfo.Count = (uint16_t)(LogInfo.NextNumber - LogInfo.FirstNumber + 1);
                LogInfo.Count = (uint16_t)(LogInfo.NextNumber - LogInfo.FirstNumber);
        }
        else{
//                LogInfo.Count =  (uint16_t)(10000 - (LogInfo.NextNumber - LogInfo.FirstNumber + 1));
                LogInfo.Count =  (uint16_t)(10000 - (LogInfo.FirstNumber - LogInfo.NextNumber));
      	  }
        }
        else
        {
            if(LogInfo_T.NextNumber >= LogInfo_T.FirstNumber){
//                LogInfo_T.Count = (uint16_t)(LogInfo_T.NextNumber - LogInfo_T.FirstNumber + 1);
                LogInfo_T.Count = (uint16_t)(LogInfo_T.NextNumber - LogInfo_T.FirstNumber);
            }
            else{
//                LogInfo_T.Count =  (uint16_t)(10000 - (LogInfo_T.NextNumber - LogInfo_T.FirstNumber + 1));
                LogInfo_T.Count =  (uint16_t)(10000 - (LogInfo_T.FirstNumber - LogInfo_T.NextNumber));
            }
        }
//sakaguchi cg UT-0014 ↑
/*
        Printf("Log pStart    %ld\r\n", LogInfo.pStart);
        Printf("Log start num %ld\r\n", LogInfo.FirstNumber);    
        Printf("Log pWrite    %ld\r\n", LogInfo.pWrite);
        Printf("Log next  num %ld\r\n", LogInfo.NextNumber);       
        Printf("Log Size      %ld\r\n", LogInfo.Size);       
        Printf("Log Count     %ld\r\n", LogInfo.Count);
*/
        tx_mutex_put(&mutex_sfmem);

    }

    return ( 0 );
}


















/**
 * @brief   拡張 memcpy() + キーワード置き換え
 * $C:Current Readings
 * $W:Warning
 * $D:Download
 * $A:Auto Download
 * @param dst
 * @param src
 * @param size
 * @return
 */
static uint8_t LogMatch( uint32_t dst, LOG_LINE *src, int16_t size )
{
	LOG_LINE	*targ = (LOG_LINE *)dst;
	char	data, *pos = (char *)src;
	uint32_t	i, len;
	size_t	log_size = 0;

	///@note    パディングあり
	static const struct {
		const char	code;
		const char	len;
		const char	*text;
	} ChgTbl[] = {
				//  1234567890123456
		{ 'S',  7, "Success"			},
        { 'E',  5, "Error"	      		},
		{ 'A', 12, "AutoDownload"		},
		{ 'C', 15, "CurrentReadings"	},
//		{ 'D',  8, "Download"			},
		{ 'D', 21, "Rec Data Transmission"  },
		{ 'W',  7, "Warning"			},
	};

	// 実体以外をまずコピー
	//for ( i=0; i<sizeof(LogLine)-sizeof(LogLine.Data) -4; i++ ) {
    for ( i=0; i<sizeof(LogLine)-sizeof(LogLine.Data) - 1; i++ ) {    
		*( (char *)dst ) = *pos;
		dst++;
		pos++;
		size--;
		log_size++;
	}

	while( size > 0 ) {

		data = *pos++;
		size--;

		if ( data == '$' && size > 1 ) {
			for ( i=len=0; i<DIMSIZE(ChgTbl); i++ ) {
				if ( *pos == ChgTbl[i].code ) {
					len = ChgTbl[i].len;
					if ( len + log_size < sizeof(LOG_LINE) ) {
						ExMemcpy( dst, (uint32_t)ChgTbl[i].text, len ); //拡張memcpy() →標準ライブラリで良いのでは？
						size--, pos++;
						dst += len;
						log_size += len;
						break;
					}
				}
			}
		}
		else{
		    len = 0;
		}

		if ( len == 0 ) {
			*( (char *)dst ) = data;		// そのまま
			dst++, log_size++;
		}

		if ( log_size >= sizeof(LogLine.Data) )
			break;
	}

	targ->Size = (uint8_t)log_size;			// サイズ合わせ

	return ((uint8_t)log_size);
}
