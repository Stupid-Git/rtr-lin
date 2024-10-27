/**
 * @file	RfCmd_func.c
 *
 * @date	2020/02/07
 * @author	tooru.hayashi
 * @note	2020.Jul.01 GitHub 630ソース反映済み
 */


#define _RFCMD_FUNC_C_

#include <stdio.h>
#include <stdarg.h>
#include <limits.h>     //ULONG_MAX
#include <math.h>

#include "MyDefine.h"
#include "Globals.h"
#include "General.h"
#include "Error.h"
#include "Cmd_func.h"
#include "Sfmem.h"
#include "Log.h"

#include "Rfunc.h"
#include "RfCmd_func.h"
#include "rf_thread_entry.h"
#include "rf_thread.h"
#include "new_g.h"                  // 2023.05.31

extern TX_THREAD rf_thread;


static uint8_t Rec_StartTime_set(uint32_t Secs);                   // [EWSTW]にUTCが含まれていた場合、指定時刻で記録開始する

static uint8_t RegistTableEWSTR(void);






/**
 * @brief    ＴコマンドパラメータからＲＦタスク起動
 * @param cmd
 * @return  結果
 * @bug if(){} else if(){} ... で同じ項目があり実行されない箇所がある  →2020.Sep.10 修正済み
 * RF_COMM_PS_DISPLAY_MESSAGE
 * RF_COMM_PS_REMOTE_MEASURE_REQUEST
 * RF_COMM_PS_SET_TIME
 * @note    2020.Sep.10 mutex取得の位置を変更、リファクタリング
 */
uint32_t start_rftask(uint8_t cmd)
{
    int32_t i, num, band, ch, format, max, group, rumax, rptmax;
    int32_t sz_id, sz_data;

    char *id, *relay, *data, *para;

	uint32_t seri;
	int32_t nth;

	char seri_tmp[2];
	uint8_t cnt;
	char *poi;

    uint32_t Secs = 0;
	int sz_utc;
    RouteArray_t route;
    uint32_t err_code;

    err_code = ERR(CMD, FORMAT);

    Printf("    ==== g_rf_mutex get 2!!\r\n");
    // ＲＦタスク実行
    if(tx_mutex_get(&g_rf_mutex, WAIT_100MSEC) == TX_SUCCESS)   // ＲＦタスク実行中ではないこと
    {

        sz_utc = ParamAdrs("SER",&para);
	if (  sz_utc > 0 ) {	// パラメータにSERがあるか
		seri = 0;
		poi = para;

		for(cnt = 0 ; cnt < sz_utc ; cnt++){
			seri = seri * 16; 
			seri_tmp[0] = *poi;
			seri_tmp[1] = 0x00;
			seri += (uint32_t)strtol(seri_tmp,NULL,16);
			poi++;
		}
		nth = ParamInt("NTH");
		if(nth == INT32_MAX){
			nth = 0;	// NTHが空の場合2個1の前半とみなす
		}
		NTH_2in1 = (uint8_t)nth;
	}
	else{
		NTH_2in1 = 0;
		seri = UINT32_MAX;
	}
	EWSTW_Ser = seri;				// 登録パラメータに反映させる為にシリアルNo.を保存する。


    r500.time_cnt = 0;                                                  // 末端機に届いてＴコマンドになるまでの時間（フォーマットエラー表示のときゼロになるよう）
    RF_buff.rf_res.time = 0;                                            // 自機のコマンド応答のためにコマンド開始から子機が応答したときまでの秒数（フォーマットエラー表示のときゼロになるよう）

    // 登録情報クリア
    memset((char *)&ru_registry, 0x00, sizeof(ru_registry));         // 登録情報クリア
    memset((char *)&group_registry, 0x00, sizeof(group_registry));

    // 登録情報を使うか
    group = ParamInt("GRP");
    band = ParamInt("BAND");                                    // バンド
    ch = ParamInt("CH");                                        // 周波数
        sz_id = ParamAdrs("ID", &id);                               // グループＩＤ

    if(group != INT32_MAX)                                        // ＧＰＲ（グループ）パラメータがあるとき
    {
        if((band != INT32_MAX) || (ch != INT32_MAX) || (id != 0)){
                goto RF_ERROR_END;
        }
            EWSTW_GNo = (uint8_t)group;              // コマンドにより得られたGNoを保存しておく        2020/08/26 segi

        // group_registry.frequency == USE_REG_FRQ なら登録情報を使用する
        band = INT32_MAX;
        ch = INT32_MAX;
        id = 0;
    }

	if(seri != UINT32_MAX){		//### シリアルNo.がある場合 ###
		seri = get_regist_group_adr_ser(seri);			// シリアルNo.からグループ、子機登録を読み出す。
		if(250 >= seri){
			group = (int32_t)seri;				// グループNo.
                EWSTW_GNo = (uint8_t)group;              // コマンドにより得られたGNoを保存しておく        2020/08/26 segi
			band = group_registry.band;			// バンド
			ch = group_registry.frequency;		// 周波数
			id = group_registry.id;				// グループID
			num = ru_registry.rtr501.number;	// 子機番号

	        if((root_registry.auto_route & 0x01) == 1){     // 自動ルート機能ONの場合
				memset( &route, 0x00, sizeof(RouteArray_t) );
				group_registry.altogether = (char)RF_GetRemoteRoute((uint8_t)group, (uint8_t)num, &route);

				memset( rpt_sequence, 0x00, sizeof(rpt_sequence) );
				memcpy( rpt_sequence, route.value, route.count );
			}else{
				group_registry.altogether = get_regist_relay_inf((uint8_t)group, (uint8_t)num, rpt_sequence);   // 中継機情報を組み立て（登録ファイルから指定されたグループ番号のグループ情報をグループ情報構造体にセット）
			}

			num += nth;							// 2個1機種の対応
		}						//### シリアルNo.が存在しない場合 ###
		else{
                err_code = ERR(RF, PARAMISS);
                goto RF_ERROR_END;
		}
	}
	else{
		// バンド
		if(band == INT32_MAX){
		    band = 0;                                                                       // 省略時は0
		}
		else if(((band < 0) || (band > 7)) || ((band > 0) && ((regf_rfm_version_number & 0x0000ffff) < 7))) // 範囲は0～7 無線モジュール版数７未満でのband指定はエラー
		{
                err_code = ERR(RF, NOBAND);
                goto RF_ERROR_END;
		}
		group_registry.band = (char)band;

		// 周波数
		if(ch == INT32_MAX){
		    group_registry.frequency = USE_REG_FRQ;   // ＣＨパラメータなしのとき登録情報を使用する
		}
		else{
		    group_registry.frequency = (char)ch;
		}

		// グループＩＤ
		memset(group_registry.id, 0x00, sizeof(group_registry.id));
		if(sz_id > 8){
		    sz_id = 8;
		}
		if(id == 0)                                                 // ＩＤパラメータなしのとき登録情報グループ１
		{
			group_registry.id[0] = 1;
			id = group_registry.id;
		}
		else{
		    memcpy(group_registry.id, id, (size_t)sz_id);
		}

		// 子機番号（１～２５０） ここではパラメータチェックしない
		num = ParamInt("NUM");
		ru_registry.rtr501.number = (char)num;

		// 中継機番号を中継順に格納（最初は必ず親機番号０）
		group_registry.altogether = (uint8_t)ParamAdrs("RELAY", &relay);
		if(relay == 0)                                              // RELAYパラメータがなければ中継機無し
		{
			rpt_sequence[0] = 0x00;
			group_registry.altogether = 1;
		}
		else{
		    memcpy(rpt_sequence, relay, group_registry.altogether);
		}

		if(group_registry.frequency == USE_REG_FRQ)                 // 子機登録から組み立て（*id はグループ登録番号）
		{
			if(num == INT32_MAX){
			    num = 1;
			    ru_registry.rtr501.number = (char)num;
			}

			// ＩＤをＡＳＣＩＩで指定されてしまったとき（トランストロン対策）→ＡＳＣＩＩ数値をバイナリに読み替える
			if((*id >= 0x30) && (get_regist_group_size() < 0x30)){
			    *id &= 0x0f;
			}

			group_registry.altogether = get_regist_relay_inf(*id, (uint8_t)num, rpt_sequence);   // 中継機情報を組み立て（登録ファイルから指定されたグループ番号のグループ情報をグループ情報構造体にセット）

			if(group_registry.altogether == 0){
//                  err_code = ERR(CMD, FORMAT);    // グループ番号または子機番号が見つからない
                    goto RF_ERROR_END;
			}
		}
	}


    // 中継機ルート設定
    if(ParamInt("ROUTE") == 0){
        RF_buff.rf_req.short_cut = 1;    // ショートカットあり
    }
    else{
        RF_buff.rf_req.short_cut = 0;    // ショートカットなし（パラメータにより固定）
    }

    // その他のパラメータ
    format = ParamInt("FORMAT");

    max = ParamInt("MAX");
/* 不要
    if(max == INT32_MAX){
        max = 250;                               // ０または無指定の場合は最大値
    }
*/
    if(max == 0){
        max = 250;
    }
    if(max > 250){
        max = 250;
    }

    Printf("##start_rftask %d max:%d\r\n", cmd,max);

	// 子機と中継機の一斉検索用
	rumax = ParamInt("RUMAX");
/* 不要
	if(rumax == INT32_MAX){
	    rumax = 100;							// 無指定の場合は最大値
	}
*/
	if(rumax < 0){
	    rumax = 0;
	}
	if(rumax > 100){
	    rumax = 100;
	}


	rptmax = ParamInt("RPTMAX");
/* 不要
	if(rptmax == INT32_MAX){
	    rptmax = 10;							// 無指定の場合は最大値
	}
*/
	if(rptmax < 0){
	    rptmax = 0;
	}
	if(rptmax > 10){
	    rptmax = 10;
	}

        complement_info[0] = 0x00;                              // パラメータ２Ｌ
        complement_info[1] = 0x00;                              // パラメータ２Ｈ
	complement_info[2] = (uint8_t)rptmax;

    // コマンド別パラメータセット
        rf_command = cmd;                                       // コマンド番号セット

        switch(cmd){

            case RF_COMM_GATHER_DATA:       //無線子機　データ吸い上げ 0x08

        i = ParamInt("RANGE");
        if(i == INT32_MAX){
                    goto RF_ERROR_END;
        }
        complement_info[0] = (uint8_t)i;                          // パラメータ２Ｌ（吸い上げ期間）
        complement_info[1] = (uint8_t)(i / 256);                  // パラメータ２Ｈ

        i = ParamInt("MODE");
                /* 不要
        if(i == INT32_MAX){
                    goto RF_ERROR_END;
        }
                */
        if(i > 2){
                    goto RF_ERROR_END;
        }
        huge_buffer[0] = (uint8_t)i;
                break;

            case RF_COMM_SETTING_WRITE:                       //無線子機　設定値書き込み0x06     記録開始ＦＯＲＭＡＴ０ 警報設定ＦＯＲＭＡＴ１

        if((num == INT32_MAX) || (num == 0)){
                    goto RF_ERROR_END;
        }

        if(ParamAdrs("DATA", &data) != 64){
                    goto RF_ERROR_END;
        }
        memcpy(huge_buffer, data, 64);
        memcpy(&EWSTW_Data[0], data, 64);		// 登録パラメータに反映させる為に保存しておく

        ( sz_utc = ParamAdrs("UTC",&para) ) ;
		if ( sz_utc > 0 ) {	// パラメータにUTCがあるか
			Secs = (uint32_t)AtoL( para, sz_utc );
			Rec_StartTime_set(Secs);							// 予約記録時刻を[Secs]にして予約記録開始にする
		}


        if(format == INT32_MAX){
            format = 0;                       // 省略時のデフォルト値
        }
        if(format > 1){
                    goto RF_ERROR_END;
        }

		EWSTW_Format = (uint8_t)format;		// 登録ファイルに反映させる為にFORMATを保存する。

                rf_command = (format == 0) ? RF_COMM_SETTING_WRITE_FORMAT0 : RF_COMM_SETTING_WRITE_FORMAT1; ///< @bug 引数を書き換えてる→修正

                break;

            case RF_COMM_SETTING_READ:                        // 設定値読み込み 0x05

        if((num == INT32_MAX) || (num == 0)){
                    goto RF_ERROR_END;
        }

                break;

            case RF_COMM_SETTING_WRITE_GROUP:                 // グループ一斉記録開始 0x0d

        if(ParamAdrs("DATA", &data) != 64){
                    goto RF_ERROR_END;
        }
        memset(huge_buffer, 0x00, 64);
        memcpy(huge_buffer, data, 64);

        complement_info[0] = (uint8_t)max;
//              complement_info[1] = 0x00;                              // パラメータ２Ｈ
                break;

            case RF_COMM_CURRENT_READINGS:                    // 現在値取得コマンド 0x07

        Printf("num = %d  format = %d \r\n", num, format);
        if((num == INT32_MAX) || (num == 0)){
                    goto RF_ERROR_END;
        }

        if(format == INT32_MAX){
            format = 5;                       // 省略時のデフォルト値
        }

//              complement_info[0] = 0x00;                              // パラメータ２Ｌ
        complement_info[1] = (uint8_t)format;                            // パラメータ２Ｈ（Ｒ５００応答フォーマット）
                break;

            case RF_COMM_REAL_SCAN:                           // リアルスキャンコマンド 0x0B

        if(format == INT32_MAX){
            format = 0;                       // 省略時のデフォルト値
        }

        complement_info[0] = (uint8_t)max;
        complement_info[1] = (uint8_t)format;                            // パラメータ２Ｈ（Ｒ５００応答フォーマット）
                break;

            case RF_COMM_WHOLE_SCAN:                            // 子機と中継機の一斉検索コマンド 0x13

		if(format == INT32_MAX){
		    format = 0;						// 省略時のデフォルト値
		}

		complement_info[0] = (uint8_t)rumax;
		complement_info[1] = (uint8_t)format;							// パラメータ２Ｈ（Ｒ５００応答フォーマット）
                break;

            case RF_COMM_BROADCAST:     //無線子機　中継機検索 0x04       呼び出し元無し
            case RF_COMM_SEARCH:        //無線子機 子機検索コマンド 0x09,
            case RF_COMM_REPEATER_SEARCH:                     // リピータスキャンコマンド 0x0A

        complement_info[0] = (uint8_t)max;                               // パラメータ２Ｌ（最大子機番号）
//              complement_info[1] = 0x00;                              // パラメータ２Ｈ

                break;

            case RF_COMM_MONITOR_INTERVAL:          //無線子機　子機のモニタリング間隔設定 0x02
        complement_info[0] = (uint8_t)ParamInt("TIME");           // パラメータ２Ｌ（インターバル時間）
//              complement_info[1] = 0x00;                              // パラメータ２Ｈ
                break;

            case RF_COMM_PROTECTION:            //無線子機　子機のプロテクト設定 0x03
//              complement_info[0] = 0;                                 // パラメータ２Ｌ（プロテクト設定）
        complement_info[0] |= (uint8_t)((ParamInt("REC") == 1) ? 0x01 : 0);
        complement_info[0] |= (uint8_t)((ParamInt("WARN") == 1) ? 0x02 : 0);
        complement_info[0] |= (uint8_t)((ParamInt("INT") == 1)  ? 0x04 : 0);
//              complement_info[1] = 0x00;                              // パラメータ２Ｈ
                break;

            case RF_COMM_RECORD_STOP:       //無線子機　記録停止 0x0C
        if((num == INT32_MAX) || (num == 0)){
                    goto RF_ERROR_END;
        }

        i = ParamInt("MODE");
                /* 不要
        if(i == INT32_MAX){
                    goto RF_ERROR_END;
        }
                */
        if(i != 1){
                    goto RF_ERROR_END;
        }

                break;

            case RF_COMM_PS_SETTING_READ:       //ＰＵＳＨ　設定値読み込み 0x50

        if((num == INT32_MAX) || (num == 0)){
                    goto RF_ERROR_END;
        }

//              complement_info[0] = 0x00;                              // パラメータ２Ｌ
        complement_info[1] = 0x46;                              // パラメータ２Ｈ
                break;

            case RF_COMM_PS_SETTING_WRITE:      //ＰＵＳＨ　設定値書き込み 0x51
        if((num == INT32_MAX) || (num == 0)){
                    goto RF_ERROR_END;
        }

        if(ParamAdrs("DATA", &data) != 64){
                    goto RF_ERROR_END;
        }
        memcpy(huge_buffer, data, 64);

                break;

            case RF_COMM_PS_GATHER_DATA:        //ＰＵＳＨ　データ吸い上げ 0x52
        i = ParamInt("RANGE");
        if(i == INT32_MAX){
                    goto RF_ERROR_END;
        }
        complement_info[0] = (uint8_t)i;                          // パラメータ２Ｌ（吸い上げ期間）
        complement_info[1] = (uint8_t)(i / 256);                  // パラメータ２Ｈ

        i = ParamInt("MODE");
        if(i == INT32_MAX){
                    goto RF_ERROR_END;
        }
                if(i != 2){                // ＰＵＳＨのデータ吸い上げは期間指定のみ
                    goto RF_ERROR_END;
        }
        huge_buffer[0] = (uint8_t)i;
                break;

            case RF_COMM_PS_DATA_ERASE:     //ＰＵＳＨ　データ消去 0x53

        if((num == INT32_MAX) || (num == 0)){
                    goto RF_ERROR_END;
        }

        i = ParamInt("MODE");
        if(i == INT32_MAX){
                    goto RF_ERROR_END;
        }
        if(i != 1){
                    goto RF_ERROR_END;
        }

                break;

            case RF_COMM_PS_ITEM_LIST_READ:     //ＰＵＳＨ　品目リスト読み込み 0x54
        if((num == INT32_MAX) || (num == 0)){
                    goto RF_ERROR_END;
        }

//              complement_info[0] = 0x00;                              // パラメータ２Ｌ
        complement_info[1] = 0xa1;                              // パラメータ２Ｈ
                break;

            case RF_COMM_PS_ITEM_LIST_WRITE:        //ＰＵＳＨ　品目リスト書き込み 0x55
        if((num == INT32_MAX) || (num == 0)){
                    goto RF_ERROR_END;
        }

//              complement_info[0] = 0x00;                              // パラメータ２Ｌ
        complement_info[1] = 0xa1;                              // パラメータ２Ｈ

        r500.data_size = (uint16_t)ParamAdrs("DATA", &data);
		if(r500.data_size >= 4){
		    r500.data_size = *(uint16_t *)&data[2];
		}
		if(r500.data_size > 1024){
		    r500.data_size = 1024;
		}
        memcpy(huge_buffer, data, r500.data_size);
                break;

            case RF_COMM_PS_WORKER_LIST_READ:       //ＰＵＳＨ　作業者リスト読み込み 0x56

        if((num == INT32_MAX) || (num == 0)){
                    goto RF_ERROR_END;
        }

//              complement_info[0] = 0x00;                              // パラメータ２Ｌ
        complement_info[1] = 0xa0;                              // パラメータ２Ｈ
                break;

            case RF_COMM_PS_WORKER_LIST_WRITE:      //ＰＵＳＨ　作業者リスト書き込み 0x57
        if((num == INT32_MAX) || (num == 0)){
                    goto RF_ERROR_END;
        }

//              complement_info[0] = 0x00;                              // パラメータ２Ｌ
        complement_info[1] = 0xa0;                              // パラメータ２Ｈ

        r500.data_size = (uint16_t)ParamAdrs("DATA", &data);
		if(r500.data_size >= 4){
		    r500.data_size = *(uint16_t *)&data[2];
		}
		if(r500.data_size > 512){
		    r500.data_size = 512;
		}
        memcpy(huge_buffer, data, r500.data_size);
                break;

            case RF_COMM_PS_WORK_GROUP_READ:        //ＰＵＳＨ　ワークグループ読み込み 0x5B
        if((num == INT32_MAX) || (num == 0)){
                    goto RF_ERROR_END;
        }

//              complement_info[0] = 0x00;                              // パラメータ２Ｌ
        complement_info[1] = 0xa2;                              // パラメータ２Ｈ
                break;

            case RF_COMM_PS_WORK_GROUP_WRITE:       //ＰＵＳＨ　ワークグループ書き込み 0x5C

        if((num == INT32_MAX) || (num == 0)){
                    goto RF_ERROR_END;
        }

//              complement_info[0] = 0x00;                              // パラメータ２Ｌ
        complement_info[1] = 0xa2;                              // パラメータ２Ｈ

        r500.data_size = (uint16_t)ParamAdrs("DATA", &data);
		if(r500.data_size >= 4){
		    r500.data_size = *(uint16_t *)&data[2];
		}
		if(r500.data_size > 1008){
		    r500.data_size = 1008;
		}
        memcpy(huge_buffer, data, r500.data_size);
                break;

            case RF_COMM_PS_DISPLAY_MESSAGE:        //ＰＵＳＨ　メッセージ表示 0x59
        if((num == INT32_MAX) || (num == 0)){
                    goto RF_ERROR_END;
        }

        r500.data_size = (uint16_t)ParamAdrs("DATA", &data);
        if(r500.data_size != 128){
                    goto RF_ERROR_END;
        }

        memcpy(huge_buffer, data, r500.data_size);
                break;

            case RF_COMM_PS_REMOTE_MEASURE_REQUEST: //ＰＵＳＨ　リモート測定指示 0x5A
        if((num == INT32_MAX) || (num == 0)){
                    goto RF_ERROR_END;
        }

                sz_data = ParamAdrs("DATA", &data);                     // リモート測定指示データ２バイト
        if(sz_data > 2){
            sz_data = 2;
        }
                memcpy(complement_info, data, (size_t)sz_data);
                break;

            case RF_COMM_PS_SET_TIME:       //ＰＵＳＨ　時刻設定 0x58

        if((num == INT32_MAX) || (num == 0)){
                    goto RF_ERROR_END;
        }

        r500.data_size = (uint16_t)ParamAdrs("DATA", &data);
        if(r500.data_size != 12){
                    goto RF_ERROR_END;
        }

        memcpy(huge_buffer, data, r500.data_size);
                break;

            case RF_COMM_REGISTRATION:      //無線子機　子機登録変更 0x0E
        // num == 0 のときは中継機宛の登録変更
        if(num == INT32_MAX){
                    goto RF_ERROR_END;
        }

        i = ParamInt("MODE");
        /* 不要
        if(i == INT32_MAX){
                    goto RF_ERROR_END;
        }
        */
        if(i > 1){
                    goto RF_ERROR_END;
        }

//              complement_info[0] = 0x00;                              // パラメータ２Ｌ
        if(i == 1){
            complement_info[1] = 0x0e;                   // 中継機自身も同じ登録になる
        }

        // 変更６４バイトデータ作成
        memset(huge_buffer, 0x00, 64);                          // ６４バイト０ｘ００クリア
        r500.data_size = 64;

        // ＩＤパラメータ
                sz_id = ParamAdrs("XID", &id);
        if(sz_id > 8){
            sz_id = 8;
        }
        if(id == 0){
            memcpy(huge_buffer, group_registry.id, 8);  // ＩＤパラメータなしのとき通信先子機のＩＤに置き換える
        }
        else{
            memcpy(huge_buffer, id, (size_t)sz_id);
        }

        // 子機番号
        num = ParamInt("XNUM");
        if(num == INT32_MAX){
            num = ru_registry.rtr501.number;     // ＮＵＭパラメータなしのとき通信先子機のＮＵＭに置き換える
        }
        huge_buffer[16] = (uint8_t)num;

        // 子機周波数
        ch = ParamInt("XCH");
        if(ch == INT32_MAX){
            ch = group_registry.frequency;        // ＣＨパラメータなしのとき通信先子機のＣＨに置き換える
        }
        huge_buffer[17] = (uint8_t)ch;

        // バンド
        band = ParamInt("XBAND");
        if(band == INT32_MAX){
            band = group_registry.band;         // ＣＨパラメータなしのとき通信先子機のＣＨに置き換える
        }
        huge_buffer[18] = (uint8_t)band;

        // 子機名
                sz_data = ParamAdrs("XNAME", &data);
        if(data == 0){
            sprintf((char *)&huge_buffer[8], "RU No%.3d", huge_buffer[16]);
        }
        else{
            memcpy(&huge_buffer[8], data, 8);
        }

        // ＢＬＥ名称
                sz_data = ParamAdrs("XBLEN", &data);    //上のsz_dataは未使用？
        if(data != 0){
            memcpy(&huge_buffer[19], data, (size_t)sz_data);
        }

        huge_buffer[16] = (uint8_t)num;                                  // 子帰終端ＮＵＬＬ書き込みでhuge_buffer[16] が壊れるので再書き込み
                 break;

        //-----------------------------------------------------------------------------------------------
            case RF_COMM_BLE_SETTING_WRITE:             // BLE設定_Write 0x10
		// ENABLE
		num = ParamInt("ENABLE");
        if(num == INT32_MAX){
            huge_buffer[36] = 0;         // ENABLEパラメータなしのとき変更しない
        }
		if(num == 0){			// off
			huge_buffer[36] = 0xff; 
		}
		else if(num == 1){		// on
			huge_buffer[36] = 1; 
		}
        // ＢＬＥ名称
        sz_data = /*(size_t)*/ParamAdrs("NAME", &data);
		if(data != 0){
			memset(&huge_buffer[1], 0, 26);
			memcpy(&huge_buffer[1], data, (size_t)sz_data);
			huge_buffer[0] = 1;
		}
		else{
			huge_buffer[0] = 0;
		}
        /*
		// SECURITY
		num = ParamInt("SECURITY");
        if(num == INT32_MAX) huge_buffer[27] = 0;         // SECURITYパラメータなしのとき変更しない
		if(num == 0){			// off
			huge_buffer[27] = 0xff; 
		}
		else if(num == 1){		// on
			huge_buffer[27] = 1; 
		}
        */
		huge_buffer[27] = 0;
        // NEWPASCパラメータ	(変更後のパスコード)
        sz_id = /*(size_t)*/ParamAdrs("NEWPASC", &id);
        if(sz_id > 4){
            ;//sz_id = 4;
        }
		if(sz_id == 4){
			memcpy(&huge_buffer[32], id, (size_t)sz_id);
			huge_buffer[27] = 0x10;
		}
        // OLDPASCパラメータ	(変更前のパスコード)
        sz_id = /*(size_t)*/ParamAdrs("OLDPASC", &id);
        if(sz_id > 4){
            ;//sz_id = 4;
        }
		if(sz_id == 4){
			memcpy(&huge_buffer[28], id, (size_t)sz_id);
			if(huge_buffer[27] == 0x10){
			    huge_buffer[27] = 1;		// NEWPASCとOLDPASCが両方存在すれば、huge_buffer[27] = 1 にする
			}
		}
		else{
		    huge_buffer[27] = 0;
		}

		memcpy(&EWSTW_Data[0],&huge_buffer[0],64);				// 登録パラメータに反映させる為に保存しておく

                break;


            case RF_COMM_MEMO_WRITE:                        // スケール変換_Write 0x12
		sz_id = /*(size_t)*/ParamAdrs("DATA", &data);

		if(ParamAdrs("DATA", &data) != 64){
                    goto RF_ERROR_END;
		}
		
		r500.data_size = 64;
		
		memcpy(huge_buffer, data, 64);
        memcpy(&EWSTW_Data[0], data, 64);		// 登録パラメータに反映させる為に保存しておく
                break;
        //-----------------------------------------------------------------------------------------------
            case RF_COMM_BLE_SETTING_READ:                  // BLE設定_Read       0x0F

            case RF_COMM_MEMO_READ:                         //無線子機　スケール変換式読み出し 0x11 2020.Sep.10
            case RF_COMM_REPEATER_CLEAR:                    //無線子機　中継機の受信履歴読み込みとクリア 0x01    2020.Sep.10

            case RF_COMM_GET_MODULE_VERSION:                //無線モジュールバージョン取得 0x20   呼び出し元無し 2020.Sep.10
            case RF_COMM_GET_MODULE_SERIAL:                 //無線モジュールシリアル番号取得 0x21  呼び出し元無し     2020.Sep.10
            case RF_COMM_DIRECT_COMMAND:                    //無線モジュールダイレクトコマンド 0x22 呼び出し元無し     2020.Sep.10
            case RF_COMM_SETTING_WRITE_FORMAT0:             //無線子機　設定値書き込み　ＦＯＲＭＡＴ０ 0x30 呼び出し元無し     2020.Sep.10
            case RF_COMM_SETTING_WRITE_FORMAT1:             //無線子機　設定値書き込み　ＦＯＲＭＡＴ１ 0x31 呼び出し元無し     2020.Sep.10
            default:
//		complement_info[0] = 0x00;                              // パラメータ２Ｌ
//		complement_info[1] = 0x00;                              // パラメータ２Ｈ
                break;
        }//switch()

        RF_buff.rf_res.status = BUSY_RF;

		

//      rf_command = cmd;                                       // コマンド番号セット
        tx_mutex_put(&g_rf_mutex);
        Printf("    ==== g_rf_mutex put 2 !!\r\n");

        tx_thread_resume(&rf_thread);       //RFスレッド起床

		Printf("< rf_thread resume >\r\n");
        tx_thread_sleep (1);
        err_code = ERR(RF, NOERROR);        //コマンド処理完了

    }
    else{
		Printf("    ==== g_rf_mutex busy !!!!!!\r\n");
//        return(ERR(RF, BUSY));
        err_code = ERR(RF, BUSY);
    }

    return(err_code);

RF_ERROR_END:      //パラメータ、フォーマットエラー処理
    tx_mutex_put(&g_rf_mutex);
    Printf("    ==== g_rf_mutex put 3 !!\r\n");
    return(err_code);
}




/**
 * @brief   [EWSTW]にUTCが含まれていた場合、指定時刻で記録開始する
 * @param Secs
 * @return
 */
static uint8_t Rec_StartTime_set(uint32_t Secs)
{
	rtc_time_t Tm;

	union{
		uint8_t byte[4];
		uint32_t l_word;
	}b_lw_chg;

	uint32_t diff;
	uint32_t GMT_temp;

	b_lw_chg.l_word =  RTC_GetGMTSec();			// 現在のGMTを取得する
	RTC_GSec2Date(b_lw_chg.l_word, &Tm);

	if(Secs > b_lw_chg.l_word + 20){
		b_lw_chg.l_word = Secs - b_lw_chg.l_word;
		huge_buffer[12] = b_lw_chg.byte[0];		// 記録開始までの秒数
		huge_buffer[13] = b_lw_chg.byte[1];		// 記録開始までの秒数
		huge_buffer[14] = b_lw_chg.byte[2];		// 記録開始までの秒数
		huge_buffer[15] = b_lw_chg.byte[3];		// 記録開始までの秒数
		huge_buffer[10] = 0x01;
		b_lw_chg.l_word = Secs;
		huge_buffer[2] = b_lw_chg.byte[0];		// 記録開始日時
		huge_buffer[3] = b_lw_chg.byte[1];		// 記録開始日時
		huge_buffer[4] = b_lw_chg.byte[2];		// 記録開始日時
		huge_buffer[5] = b_lw_chg.byte[3];		// 記録開始日時
		EWSTW_utc = b_lw_chg.l_word;
		return(0);
	}
	else{

		diff = (uint32_t)(60 - Tm.tm_sec);
		GMT_temp = b_lw_chg.l_word;				// 現在のGMTをGMT_tempにコピーする
		b_lw_chg.l_word = 60 + diff;			// 少し余裕を見て60秒後とする	(指定のUTCが過去なら即時記録)
		huge_buffer[12] = b_lw_chg.byte[0];		// 記録開始までの秒数			(指定のUTCが過去なら即時記録)
		huge_buffer[13] = b_lw_chg.byte[1];		// 記録開始までの秒数			(指定のUTCが過去なら即時記録)
		huge_buffer[14] = b_lw_chg.byte[2];		// 記録開始までの秒数			(指定のUTCが過去なら即時記録)
		huge_buffer[15] = b_lw_chg.byte[3];		// 記録開始までの秒数			(指定のUTCが過去なら即時記録)
		huge_buffer[10] = 0x01;					// 予約記録開始					(指定のUTCが過去なら即時記録)
		
		//b_lw_chg.l_word = Secs + 60 + diff;
		b_lw_chg.l_word = GMT_temp + 60 + diff;	// 現在のGMTに60秒と毎分0秒になる為の補正を足したのが記録開始日時

		huge_buffer[2] = b_lw_chg.byte[0];		// 記録開始日時
		huge_buffer[3] = b_lw_chg.byte[1];		// 記録開始日時
		huge_buffer[4] = b_lw_chg.byte[2];		// 記録開始日時
		huge_buffer[5] = b_lw_chg.byte[3];		// 記録開始日時
		EWSTW_utc = b_lw_chg.l_word;
		DebugLog( LOG_DBG, "EWSTW %ld (%ld)", b_lw_chg.l_word, diff);
		
		/*
	//	b_lw_chg.l_word = Secs;
		huge_buffer[2] = 0;						// (無効にする)記録開始日時		(指定のUTCが過去なら即時記録)
		huge_buffer[3] = 0;						// (無効にする)記録開始日時		(指定のUTCが過去なら即時記録)
		huge_buffer[4] = 0;						// (無効にする)記録開始日時		(指定のUTCが過去なら即時記録)
		huge_buffer[5] = 0;						// (無効にする)記録開始日時		(指定のUTCが過去なら即時記録)
		*/
		return(1);
	}

}



/**
 * @brief   設定ファイルの子機情報の書き換え
 * 情報は、GID、子機番号のみで子機を特定する
 *
 * @note    機種により、テーブルアドレスに違いがある。
 */
void RegistTableWrite(void)
{

    //SSP_PARAMETER_NOT_USED(G_No);       //未使用引数
    //SSP_PARAMETER_NOT_USED(S_No);       //未使用引数
    //SSP_PARAMETER_NOT_USED(format);       //未使用引数
/*
	uint8_t EWSTW_Data[64];		// コマンド入力されたDATA			グローバルに移動
	uint8_t EWSTW_Format;		// コマンド入力されたFORMAT			グローバルに移動
//	uint8_t EWSTW_GNo;			// コマンドから導いたGNo			グローバルに移動
//	uint8_t EWSTW_UNo;			// コマンドから導いたUNo			グローバルに移動
	uint32_t EWSTW_Ser;			// コマンドから導いたシリアルNo		グローバルに移動
	uint32_t EWSTW_Adr;			// FLASHに書き込むアドレス
	uint8_t Regist_Data[96+64];	// FLASHに書き込むデータ			グローバルに移動
*/

	union{
		uint8_t byte[2];
		uint16_t word;
	}b_w_chg;

	int16_t offset;
	double hPa;

	uint8_t data_tmp,data_tmp2;
//	uint32_t Size;
//	uint32_t adr_cnt = 0;

// 2023.05.26 ↓
	// EWSTWによる子機設定変更時に記録開始設定が変更されているか確認し、変更されている場合は対象子機は次回全データ吸上げとする
	uint8_t unit_no = 0;
	uint8_t group_no = 0;
	char tmp_route[256];
	memset(tmp_route, 0x00, sizeof(tmp_route));

	group_no = RF_get_regist_group_no(group_registry.id);	// 無線通信IDからグループ番号を取得
	if(group_no != 0){
		unit_no = ru_registry.rtr501.number;				// EWSTWが実行された子機番号
		get_regist_relay_inf(group_no, unit_no, tmp_route);	// 登録情報から子機情報(ru_registry)を更新

		switch(ru_registry.rtr501.header){
			case 0xFE:			// RTR501/502/503/507
			case 0xFA:			// RTR574
			case 0xF9:			// RTR576
				if(EWSTW_Format == 0){
					if((EWSTW_Data[17] & 0x01) == 0){				//##### 記録開始設定(マスクチェック) #####
						kk_delta_set(ru_registry.rtr501.serial, 0x01);     // 変化あり
					}
				}
				break;
			case 0xF8:			// RTR505
				if(EWSTW_Format == 0){
					if((((EWSTW_Data[17] & 0x01) == 0) && ((EWSTW_Data[30] & 0x01) != 0))	// 設定変更フラグ（記録開始）
					 || ((EWSTW_Data[30] & 0x10) != 0)){									// 設定変更フラグ（表示設定）
						kk_delta_set(ru_registry.rtr505.serial, 0x01);     // 変化あり
					}
				}
				break;
			case 0xF7:			// RTR601
			default:
				break;
		}
	}
// 2023.05.26 ↑

	if(EWSTW_Ser == UINT32_MAX)return;		// T2コマンドにSERが含まれていない場合、以下は実行しない(start_rftask()でEWSTW_Serにシリアルがセットされている)

    uint16_t lowerlimit = 0;
    uint16_t upperlimit = 0;

	switch(Regist_Data[2]){
		case 0xFE:			// RTR501/502/503/507
			if(EWSTW_Format == 0){
				if((EWSTW_Data[17] & 0x01) == 0){				//##### 記録開始設定(マスクチェック) #####
Printf( "OKA: RecInterval %d -> %d\r\n", *(uint16_t*)&Regist_Data[10], *(uint16_t*)&EWSTW_Data[0] );
					memcpy(&Regist_Data[10],&EWSTW_Data[0],2);		// 記録間隔
					memcpy(&Regist_Data[34],&EWSTW_Data[11],1);		// 記録モード
				}
			}
			if((EWSTW_Data[17] & 0x04) == 0){					//##### モニタリング間隔(マスクチェック) #####
				memcpy(&Regist_Data[33],&EWSTW_Data[18],1);		// モニタリング間隔
			}
			if((EWSTW_Data[17] & 0x02) == 0){					//##### 警報条件の設定(マスクチェック) #####
				data_tmp = 0;			// 警報のON/OFFを入れる
				b_w_chg.byte[0] = EWSTW_Data[20];	// ch1下限値
				b_w_chg.byte[1] = EWSTW_Data[21];	// ch1下限値
                lowerlimit = b_w_chg.word;
                b_w_chg.byte[0] = EWSTW_Data[22];	// ch1上限値
				b_w_chg.byte[1] = EWSTW_Data[23];	// ch1上限値
                upperlimit = b_w_chg.word;

				if((lowerlimit != 0xF060) && (lowerlimit != upperlimit)){
					data_tmp |= 0x01;
					memcpy(&Regist_Data[14],&EWSTW_Data[19],1);		// ch1警報判定時間
					memcpy(&Regist_Data[24],&EWSTW_Data[20],2);		// ch1警報下限値
				}
				if((upperlimit != 0x2AF8) && (lowerlimit != upperlimit)){
					data_tmp |= 0x02;
					memcpy(&Regist_Data[14],&EWSTW_Data[19],1);		// ch1警報判定時間
					memcpy(&Regist_Data[26],&EWSTW_Data[22],2);		// ch1警報上限値
				}

				b_w_chg.byte[0] = EWSTW_Data[25];	// ch2下限値
				b_w_chg.byte[1] = EWSTW_Data[26];	// ch2下限値
                lowerlimit = b_w_chg.word;
				b_w_chg.byte[0] = EWSTW_Data[27];	// ch2上限値
				b_w_chg.byte[1] = EWSTW_Data[28];	// ch2上限値
                upperlimit = b_w_chg.word;

				if((lowerlimit != 0xF060) && (lowerlimit != upperlimit)){
						data_tmp |= 0x04;
						memcpy(&Regist_Data[15],&EWSTW_Data[24],1);		// ch2警報判定時間
						memcpy(&Regist_Data[28],&EWSTW_Data[25],2);		// ch2警報下限値
					}
				if((upperlimit != 0x2AF8) && (lowerlimit != upperlimit)){
						data_tmp |= 0x08;
						memcpy(&Regist_Data[15],&EWSTW_Data[24],1);		// ch2警報判定時間
						memcpy(&Regist_Data[30],&EWSTW_Data[27],2);		// ch2警報上限値
					}
                if(data_tmp != 0){
				    memcpy(&Regist_Data[32],&data_tmp,1);			// 警報ON/OFFフラグ
				}
			}
			break;

		case 0xFA:			// RTR574
			if(EWSTW_Format == 0){
				if((EWSTW_Data[17] & 0x01) == 0){				//##### 記録開始設定(マスクチェック) #####
					memcpy(&Regist_Data[10],&EWSTW_Data[0],2);		// 記録間隔
					memcpy(&Regist_Data[46],&EWSTW_Data[11],1);		// 記録モード
				}
			}
			if((EWSTW_Data[17] & 0x04) == 0){					//##### モニタリング間隔(マスクチェック) #####
				memcpy(&Regist_Data[45],&EWSTW_Data[18],1);		// モニタリング間隔
			}
			if((EWSTW_Data[17] & 0x02) == 0){					//##### 警報条件の設定(マスクチェック) #####
				//警報条件の設定
				data_tmp = 0;			// 警報のON/OFFを入れる

				b_w_chg.byte[0] = EWSTW_Data[20];	// 照度下限値
				b_w_chg.byte[1] = EWSTW_Data[21];	// 照度下限値
                lowerlimit = b_w_chg.word;
				b_w_chg.byte[0] = EWSTW_Data[22];	// 照度上限値
				b_w_chg.byte[1] = EWSTW_Data[23];	// 照度上限値
                upperlimit = b_w_chg.word;

				if((lowerlimit != 0xEEEE) && (lowerlimit != upperlimit)){
					data_tmp |= 0x01;
					memcpy(&Regist_Data[24],&EWSTW_Data[19],1);		// 照度警報判定時間
					memcpy(&Regist_Data[28],&EWSTW_Data[20],2);		// 照度警報下限値
				}
				if((upperlimit != 0xEEEE) && (lowerlimit != upperlimit)){
					data_tmp |= 0x02;
					memcpy(&Regist_Data[24],&EWSTW_Data[19],1);		// 照度警報判定時間
					memcpy(&Regist_Data[30],&EWSTW_Data[22],2);		// 照度警報上限値
				}

				b_w_chg.byte[0] = EWSTW_Data[25];	// 紫外線下限値
				b_w_chg.byte[1] = EWSTW_Data[26];	// 紫外線下限値
                lowerlimit = b_w_chg.word;
				b_w_chg.byte[0] = EWSTW_Data[27];	// 紫外線上限値
				b_w_chg.byte[1] = EWSTW_Data[28];	// 紫外線上限値
                upperlimit = b_w_chg.word;

				if((lowerlimit != 0xEEEE) && (lowerlimit != upperlimit)){
					data_tmp |= 0x04;
					memcpy(&Regist_Data[25],&EWSTW_Data[24],1);		// 紫外線警報判定時間
					memcpy(&Regist_Data[32],&EWSTW_Data[25],2);		// 紫外線警報下限値
				}
				if((upperlimit != 0xEEEE) && (lowerlimit != upperlimit)){
					data_tmp |= 0x08;
					memcpy(&Regist_Data[25],&EWSTW_Data[24],1);		// 紫外線警報判定時間
					memcpy(&Regist_Data[34],&EWSTW_Data[27],2);		// 紫外線警報上限値
				}

				b_w_chg.byte[0] = EWSTW_Data[30];	// 温度下限値
				b_w_chg.byte[1] = EWSTW_Data[31];	// 温度下限値
                lowerlimit = b_w_chg.word;
				b_w_chg.byte[0] = EWSTW_Data[32];	// 温度上限値
				b_w_chg.byte[1] = EWSTW_Data[33];	// 温度上限値
                upperlimit = b_w_chg.word;

				if((lowerlimit != 0xF060) && (lowerlimit != upperlimit)){
					data_tmp |= 0x10;
					memcpy(&Regist_Data[26],&EWSTW_Data[29],1);		// 温度警報判定時間
					memcpy(&Regist_Data[36],&EWSTW_Data[30],2);		// 温度警報下限値
				}
				if((upperlimit != 0x2AF8) && (lowerlimit != upperlimit)){
					data_tmp |= 0x20;
					memcpy(&Regist_Data[26],&EWSTW_Data[29],1);		// 温度警報判定時間
					memcpy(&Regist_Data[38],&EWSTW_Data[32],2);		// 温度警報上限値
				}

				b_w_chg.byte[0] = EWSTW_Data[35];	// 湿度下限値
				b_w_chg.byte[1] = EWSTW_Data[36];	// 湿度下限値
                lowerlimit = b_w_chg.word;
				b_w_chg.byte[0] = EWSTW_Data[37];	// 湿度上限値
				b_w_chg.byte[1] = EWSTW_Data[38];	// 湿度上限値
                upperlimit = b_w_chg.word;
				
                if((lowerlimit != 0xF060) && (lowerlimit != upperlimit)){
					data_tmp |= 0x40;
					memcpy(&Regist_Data[27],&EWSTW_Data[34],1);		// 湿度警報判定時間
					memcpy(&Regist_Data[40],&EWSTW_Data[35],2);		// 湿度警報下限値
				}
				if((upperlimit != 0x2AF8) && (lowerlimit != upperlimit)){
					data_tmp |= 0x80;
					memcpy(&Regist_Data[27],&EWSTW_Data[34],1);		// 湿度警報判定時間
					memcpy(&Regist_Data[42],&EWSTW_Data[37],2);		// 湿度警報上限値
				}


				data_tmp2 = 0;			// 警報のON/OFFを入れる
				b_w_chg.byte[0] = EWSTW_Data[39];	// 積算照度上限値
				b_w_chg.byte[1] = EWSTW_Data[40];	// 積算照度上限値
				if(b_w_chg.word != 0){
					data_tmp2 |= 0x01;
					memcpy(&Regist_Data[48],&EWSTW_Data[39],2);		// 積算照度上限値
				}
				b_w_chg.byte[0] = EWSTW_Data[41];	// 積算紫外線上限値
				b_w_chg.byte[1] = EWSTW_Data[42];	// 積算紫外線上限値
				if(b_w_chg.word != 0){
					data_tmp2 |= 0x02;
					memcpy(&Regist_Data[50],&EWSTW_Data[41],2);		// 積算紫外線上限値
				}

                if((data_tmp != 0) || (data_tmp2 != 0)){
				    memcpy(&Regist_Data[44],&data_tmp,1);			// 警報ON/OFFフラグ(警報条件)
				    memcpy(&Regist_Data[47],&data_tmp2,1);			// 警報ON/OFFフラグ(警報条件2)
                }
			}

			if((EWSTW_Data[17] & 0x10) == 0){					//##### 操作スイッチロック(マスクチェック) #####
				memcpy(&Regist_Data[52],&EWSTW_Data[45],1);		// 操作スイッチロック
			}
			if((EWSTW_Data[17] & 0x08) == 0){					//##### 液晶表示項目(マスクチェック) #####
				memcpy(&Regist_Data[53],&EWSTW_Data[44],1);		// 液晶表示設定
			}
			break;

		case 0xF8:			// RTR505
			if(RegistTableEWSTR() == RFM_NORMAL_END){

				if(EWSTW_Format == 0){
					if(((EWSTW_Data[17] & 0x01) == 0)&&((EWSTW_Data[30] & 0x01) != 0)){	//##### 設定項目フラッグ(記録関連)が1なら変更
						memcpy(&Regist_Data[10],&EWSTW_Data[0],2);		// 記録間隔
						memcpy(&Regist_Data[34],&EWSTW_Data[11],1);		// 記録モード
						memcpy(&Regist_Data[37],&EWSTW_Data[41],1);		// 測定・記録形式

						if(Regist_Data[36] == 0xa4){	// ##### 電圧モードの場合 #####
							memcpy(&Regist_Data[39],&EWSTW_Data[24],1);		// プレヒート
							memcpy(&Regist_Data[40],&EWSTW_Data[25],2);		// プレヒート時間
						}
						if(Regist_Data[36] == 0xa5){	// ##### パルスモードの場合 #####
//							if((EWSTW_Data[30] & 0x02) != 0){					// 設定項目フラッグ(警報監視関連)が1なら変更
//								memcpy(&Regist_Data[42],&EWSTW_Data[24],1);		// パルス警報条件
//							}

							memcpy(&Regist_Data[12],&EWSTW_Data[42],1);		// チャンネル1属性
						}
						memcpy(&Regist_Data[43],&EWSTW_Data[27],1);		// KGCモードの設定1
						memcpy(&Regist_Data[44],&EWSTW_Data[28],1);		// KGCモードの設定2
						Regist_Data[44] = Regist_Data[44] & 0x0F;		// KGCモードの設定2 演算モードの上位は0クリア
					}
				}
				if(((EWSTW_Data[17] & 0x04) == 0)&&((EWSTW_Data[30] & 0x04) != 0)){		//##### 設定項目フラッグ(モニタリング間隔)が1なら変更
					memcpy(&Regist_Data[33],&EWSTW_Data[18],1);		// モニタリング間隔
				}
				if(((EWSTW_Data[17] & 0x02) == 0)&&((EWSTW_Data[30] & 0x02) != 0)){		//##### 設定項目フラッグ(警報監視関連)が1なら変更
					memcpy(&Regist_Data[14],&EWSTW_Data[19],1);		// ch1警報判定時間
					memcpy(&Regist_Data[24],&EWSTW_Data[20],2);		// ch1警報下限値
					memcpy(&Regist_Data[26],&EWSTW_Data[22],2);		// ch1警報上限値
					memcpy(&Regist_Data[32],&EWSTW_Data[29],1);		// 警報ON/OFFフラグ(警報条件)
					//memcpy(&Regist_Data[42],&EWSTW_Data[24],1);		// パルス警報条件
					if(Regist_Data[36] == 0xa5){					// ##### 電圧モードの場合 #####	// sakaguchi 2021.04.21
						memcpy(&Regist_Data[42],&EWSTW_Data[24],1);		// パルス警報条件
					}else{
						Regist_Data[42] = 0x00;
					}
				}
				if((EWSTW_Data[30] & 0x10) != 0){					//##### 設定項目フラッグ(表示単位)が1なら変更
					memcpy(&Regist_Data[38],&EWSTW_Data[16],1);		// 表示設定
				}
				else{
					if((Regist_Data[36] == 0xa1)||(Regist_Data[36] == 0xa2)){	// TC Pt の場合のみ表示単位からC/Fのみ取得して[38]bit0に設定
						if((EWSTW_Data[16] & 0x01) == 0){
						    Regist_Data[38] &= 0xFE;	// bit0=0(C)
						}
						else{
						    Regist_Data[38] |= 0x01;	// bit0=1(F)
						}
					}
				}


			}

			break;

		case 0xF9:			// RTR576
			if(EWSTW_Format == 0){
				if((EWSTW_Data[17] & 0x01) == 0){				//##### 記録開始設定(マスクチェック) #####
					memcpy(&Regist_Data[10],&EWSTW_Data[0],2);		// 記録間隔
					memcpy(&Regist_Data[46],&EWSTW_Data[11],1);		// 記録モード
				}
			}
			if((EWSTW_Data[17] & 0x04) == 0){					//##### モニタリング間隔(マスクチェック) #####
				memcpy(&Regist_Data[45],&EWSTW_Data[18],1);		// モニタリング間隔
			}
			if((EWSTW_Data[17] & 0x02) == 0){					//##### 警報条件の設定(マスクチェック) #####
				//警報条件の設定
				data_tmp = 0;			// 警報のON/OFFを入れる
				
				b_w_chg.byte[0] = EWSTW_Data[20];	// ＣＯ２下限値
				b_w_chg.byte[1] = EWSTW_Data[21];	// ＣＯ２下限値
                lowerlimit = b_w_chg.word;
				b_w_chg.byte[0] = EWSTW_Data[22];	// ＣＯ２上限値
				b_w_chg.byte[1] = EWSTW_Data[23];	// ＣＯ２上限値
                upperlimit = b_w_chg.word;
				
                if((lowerlimit != 0xF000) && (lowerlimit != upperlimit)){
					data_tmp |= 0x01;
					memcpy(&Regist_Data[24],&EWSTW_Data[19],1);		// ＣＯ２警報判定時間
					memcpy(&Regist_Data[28],&EWSTW_Data[20],2);		// ＣＯ２警報下限値
				}
				if((upperlimit != 0xF000) && (lowerlimit != upperlimit)){
					data_tmp |= 0x02;
					memcpy(&Regist_Data[24],&EWSTW_Data[19],1);		// ＣＯ２警報判定時間
					memcpy(&Regist_Data[30],&EWSTW_Data[22],2);		// ＣＯ２警報上限値
				}

				b_w_chg.byte[0] = EWSTW_Data[30];	// 温度下限値
				b_w_chg.byte[1] = EWSTW_Data[31];	// 温度下限値
                lowerlimit = b_w_chg.word;
				b_w_chg.byte[0] = EWSTW_Data[32];	// 温度上限値
				b_w_chg.byte[1] = EWSTW_Data[33];	// 温度上限値
                upperlimit = b_w_chg.word;
				
                if((lowerlimit != 0xEEEE) && (lowerlimit != upperlimit)){
					data_tmp |= 0x10;
					memcpy(&Regist_Data[26],&EWSTW_Data[29],1);		// 温度警報判定時間
					memcpy(&Regist_Data[36],&EWSTW_Data[30],2);		// 温度警報下限値
				}
				if((upperlimit != 0xEEEE) && (lowerlimit != upperlimit)){
					data_tmp |= 0x20;
					memcpy(&Regist_Data[26],&EWSTW_Data[29],1);		// 温度警報判定時間
					memcpy(&Regist_Data[38],&EWSTW_Data[32],2);		// 温度警報上限値
				}

				b_w_chg.byte[0] = EWSTW_Data[35];	// 湿度下限値
				b_w_chg.byte[1] = EWSTW_Data[36];	// 湿度下限値
                lowerlimit = b_w_chg.word;
				b_w_chg.byte[0] = EWSTW_Data[37];	// 湿度上限値
				b_w_chg.byte[1] = EWSTW_Data[38];	// 湿度上限値
                upperlimit = b_w_chg.word;

				if((lowerlimit != 0xEEEE) && (lowerlimit != upperlimit)){
					data_tmp |= 0x40;
					memcpy(&Regist_Data[27],&EWSTW_Data[34],1);		// 湿度警報判定時間
					memcpy(&Regist_Data[40],&EWSTW_Data[35],2);		// 湿度警報下限値
				}
				if((upperlimit != 0xEEEE) && (lowerlimit != upperlimit)){
					data_tmp |= 0x80;
					memcpy(&Regist_Data[27],&EWSTW_Data[34],1);		// 湿度警報判定時間
					memcpy(&Regist_Data[42],&EWSTW_Data[37],2);		// 湿度警報上限値
				}

                if(data_tmp != 0){
				memcpy(&Regist_Data[44],&data_tmp,1);			// 警報ON/OFFフラグ(警報条件)
			}
			}

			if((EWSTW_Data[17] & 0x40) == 0){					//##### 気圧補正値(マスクチェック) #####
			//	memcpy(&Regist_Data[48],&EWSTW_Data[46],1);		// 気圧補正値

				memcpy(&offset,&EWSTW_Data[46],2);		// 気圧補正値
            	hPa = (101.325 - (offset / 390.0 / 10.0 / 0.016)) * 10.0;
            	double d = round(hPa);
            	offset = (int16_t)d;
//            	offset = (int16_t)((int)round(hPa));
				memcpy(&Regist_Data[48],&offset,2);	// 気圧hPa
			}
			if((EWSTW_Data[17] & 0x10) == 0){					//##### 操作スイッチロック(マスクチェック) #####
				memcpy(&Regist_Data[50],&EWSTW_Data[45],1);		// 操作スイッチロック
			}
			if((EWSTW_Data[17] & 0x08) == 0){					//##### 液晶表示項目(マスクチェック) #####
				memcpy(&Regist_Data[51],&EWSTW_Data[44],1);		// 液晶表示設定
			}
			break;

		case 0xF7:			// RTR601
		default:
			return;

	}

	b_w_chg.byte[0] = Regist_Data[0];
	b_w_chg.byte[1] = Regist_Data[1];


// FLASHの登録パラメータを(書き込む)更新する処理を入れる
// 書き込む情報は下記の通り
//
//	EWSTW_Adr			:FLASHのWriteアドレス
//	b_w_chg.word		:FLASHのWriteバイト数
//	Regist_Data[96+64]	:FLASHのWriteデータ

//int i;
Printf("OKA: EWSTW_Adr=%d serial=%08X, RecInterval=%d\r\n", EWSTW_Adr, *(uint32_t*)&Regist_Data[6], *(uint16_t*)&Regist_Data[10] );

	if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS){

		Printf("RegistTableWrite() FLASH Read Write\r\n");

		serial_flash_multbyte_read(SFM_REGIST_START, SFM_REGIST_SIZE_32, work_buffer); // 読み込み  work_buffer size 32K

		// データ変更
		memcpy(&work_buffer[EWSTW_Adr], &Regist_Data[0], b_w_chg.word);

		// 書き込み
		serial_flash_block_unlock();                        // グローバルブロックプロテクション解除
		serial_flash_erase(SFM_REGIST_START, SFM_REGIST_END);

		serial_flash_multbyte_write(SFM_REGIST_START, SFM_REGIST_SIZE_32, work_buffer);				// 32kbyte書き込み

		tx_thread_sleep (2);    

		serial_flash_erase(SFM_REGIST_START_B, SFM_REGIST_END_B);

		serial_flash_multbyte_write(SFM_REGIST_START_B, SFM_REGIST_SIZE_32, work_buffer);				// 32kbyte書き込み

        tx_mutex_put(&mutex_sfmem);
	}

	if((VENDER_HIT != fact_config.Vender) && (Http_Use == HTTP_SEND)){      // Hitachi以外
		G_HttpFile[FILE_C].sndflg = SND_ON;                     			// [FILE]機器設定：送信有りをセット
		G_HttpCmd[HTTP_CMD_RSETF].sndflg = SND_ON;              			// [CMD] 機器設定(RSETF)：送信有りをセット
	}
	my_config.device.RegCnt++;

//	tx_event_flags_set (&g_wmd_event_flags, FLG_EVENT_INITIAL, TX_OR);      // AT Start起動（登録情報変更時）
	if(WaitRequest != WAIT_REQUEST){
		tx_event_flags_set (&g_wmd_event_flags, FLG_EVENT_INITIAL, TX_OR);      // AT Start起動（登録情報変更時）
		mate_at_start = 0;
	}else{
		mate_at_start = 1;
	}
}


/**
 *
 * @return
 */
static uint8_t RegistTableEWSTR(void)
{
	uint8_t rtn = RFM_REFUSE;

//	return(RFM_NORMAL_END);		//とりあえず成功で応答


	if((EWSTW_Data[16] != Regist_Data[38])&&((Regist_Data[36] == 0xa1)||(Regist_Data[36] == 0xa2))){	// TC Pt の場合、センサ種の切り替え成功したか受信する

		if((EWSTW_Data[30] & 0x10) != 0){		// 設定変更フラグの表示設定（＝センサ種）が変更された場合のみ実行する // sakaguchi 2020.08.31

		EWSTR_time = 10;
		while(EWSTR_time > 0){		// 最大10秒繰り返す
			rtn = (uint8_t)RF_command_execute(0x64);

			if(rtn == RFM_NORMAL_END){
				if(huge_buffer[16] == EWSTW_Data[16]){
					break;
				}
				else{
					rtn = RFM_REFUSE;						// 受信値のセンサ切り替えできていない
				}
			}
			}
		}
		else{
			rtn = RFM_NORMAL_END;
		}
	}
	else{
		rtn = RFM_NORMAL_END;
	}

	return(rtn);

}

/**
 * @brief   設定ファイルの子機情報の書き換え
 *
 * 情報は、GID、子機番号のみで子機を特定する
 * @note    機種により、テーブルアドレスに違いがある。
 * @note    b_w_chg共有体廃止
 */
void RegistTableBleWrite(void)
{

/*
	uint8_t EWSTW_Data[64];		// コマンド入力されたDATA			グローバルに移動
	uint8_t EWSTW_Format;		// コマンド入力されたFORMAT			グローバルに移動
//	uint8_t EWSTW_GNo;			// コマンドから導いたGNo			グローバルに移動
//	uint8_t EWSTW_UNo;			// コマンドから導いたUNo			グローバルに移動
	uint32_t EWSTW_Ser;			// コマンドから導いたシリアルNo		グローバルに移動
	uint32_t EWSTW_Adr;			// FLASHに書き込むアドレス
	uint8_t Regist_Data[96+64];	// FLASHに書き込むデータ			グローバルに移動
*/
#if 0
	union{
		uint8_t byte[2];
		uint16_t word;
	}b_w_chg;
#endif
//	uint32_t Size;
//	uint32_t adr_cnt = 0;
	

	if(EWSTW_Ser == UINT32_MAX){
	    return;		// T2コマンドにSERが含まれていない場合、以下は実行しない
	}

	switch(Regist_Data[2]){
		case 0xFE:			// RTR501/502/503/507
		case 0xF8:			// RTR505
			if(EWSTW_Data[0] == 0x01){
				memcpy(&Regist_Data[58],&EWSTW_Data[1],26);
			}
			if(EWSTW_Data[27] == 0x01){
				memcpy(&Regist_Data[84],&EWSTW_Data[32],4);
			}
			if(EWSTW_Data[36] == 0x01){			// ON
				Regist_Data[5] = Regist_Data[5] | 0x10;
			}
			else if(EWSTW_Data[36] == 0xFF){	// OFF
				Regist_Data[5] = Regist_Data[5] & 0xef;
			}
			break;

		case 0xFA:			// RTR574
		case 0xF9:			// RTR576
			if(EWSTW_Data[0] == 0x01){
				memcpy(&Regist_Data[58],&EWSTW_Data[1],26);
			}
			break;

		case 0xF7:			// RTR601
		default:
			return;

	}

#if 0
	b_w_chg.byte[0] = Regist_Data[0];
	b_w_chg.byte[1] = Regist_Data[1];
#endif

// FLASHの登録パラメータを(書き込む)更新する処理を入れる
// 書き込む情報は下記の通り
//
//	EWSTW_Adr			:FLASHのWriteアドレス
//	b_w_chg.word		:FLASHのWriteバイト数
//	Regist_Data[96+64]	:FLASHのWriteデータ

	if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS){

		//Size = 0x0FFF;		// 4k

//		serial_flash_multbyte_read(SFM_TEMP_START, SFM_REGIST_SIZE_32, work_buffer); // 読み込み  work_buffer size 32K
		serial_flash_multbyte_read(SFM_REGIST_START, SFM_REGIST_SIZE_32, work_buffer); // 読み込み  work_buffer size 32K

		Printf("FLASH_Read\r\n");

		// データ変更
//		memcpy(&work_buffer[EWSTW_Adr], &Regist_Data[0], b_w_chg.word);
        memcpy(&work_buffer[EWSTW_Adr], &Regist_Data[0], *(uint16_t *)&Regist_Data[0]);
		// 書き込み
		serial_flash_block_unlock();                        // グローバルブロックプロテクション解除
		serial_flash_erase(SFM_REGIST_START, SFM_REGIST_END);

		serial_flash_multbyte_write(SFM_REGIST_START, SFM_REGIST_SIZE_32, work_buffer);				// 32kbyte書き込み


//		serial_flash_block_unlock();                        // グローバルブロックプロテクション解除
		serial_flash_erase(SFM_REGIST_START_B, SFM_REGIST_END_B);

		serial_flash_multbyte_write(SFM_REGIST_START_B, SFM_REGIST_SIZE_32, work_buffer);				// 32kbyte書き込み

        tx_mutex_put(&mutex_sfmem);
	}

	if((VENDER_HIT != fact_config.Vender) && (Http_Use == HTTP_SEND)){		// Hitachi以外
		G_HttpFile[FILE_C].sndflg = SND_ON;                     			// [FILE]機器設定：送信有りをセット
		G_HttpCmd[HTTP_CMD_RSETF].sndflg = SND_ON;              			// [CMD] 機器設定(RSETF)：送信有りをセット
	}
	my_config.device.RegCnt++;

}


/**
 * @brief   設定ファイルの子機情報の書き換え
 *
 * 情報は、GID、子機番号のみで子機を特定する
 * @note    機種により、テーブルアドレスに違いがある。
 * @note    b_w_chg共有体廃止
 * @note    AdrSFTがセットのみ未使用なのでコメントアウト
 */
void RegistTableSlWrite(void)
{

/*
	uint8_t EWSTW_Data[64];		// コマンド入力されたDATA			グローバルに移動
	uint8_t EWSTW_Format;		// コマンド入力されたFORMAT			グローバルに移動
//	uint8_t EWSTW_GNo;			// コマンドから導いたGNo			グローバルに移動
//	uint8_t EWSTW_UNo;			// コマンドから導いたUNo			グローバルに移動
	uint32_t EWSTW_Ser;			// コマンドから導いたシリアルNo		グローバルに移動
	uint32_t EWSTW_Adr;			// FLASHに書き込むアドレス
	uint8_t Regist_Data[96+64];	// FLASHに書き込むデータ			グローバルに移動
*/
#if 0
	union{
		uint8_t byte[2];
		uint16_t word;
	}b_w_chg;
#endif
	uint16_t word_data;
//	uint32_t Size;
//	uint8_t AdrSFT = 0;     //セットのみ未使用
//	uint32_t adr_cnt = 0;

	if(EWSTW_Ser == UINT32_MAX)return;		// T2コマンドにSERが含まれていない場合、以下は実行しない

	switch(Regist_Data[2]){
		case 0xF8:			// RTR505
			if((Regist_Data[5] & 0x20) != 0){
			    word_data = *(uint16_t *)&Regist_Data[0];
                if(word_data == 96){
                    word_data = 160;
                    *(uint16_t *)&Regist_Data[0] = word_data;
 //                   AdrSFT = 1;//セットのみ未使用
                }
#if 0
				b_w_chg.byte[0] = Regist_Data[0];
				b_w_chg.byte[1] = Regist_Data[1];
				if(b_w_chg.word == 96){
					b_w_chg.word = 160;
					Regist_Data[0] = b_w_chg.byte[0];
					Regist_Data[1] = b_w_chg.byte[1];
					AdrSFT = 1;
				}
#endif
				memcpy(&Regist_Data[96],EWSTW_Data, 64);
			}
			break;
		default:
			return;
	}

	word_data = *(uint16_t *)&Regist_Data[0];
#if 0
	b_w_chg.byte[0] = Regist_Data[0];
	b_w_chg.byte[1] = Regist_Data[1];
#endif

// FLASHの登録パラメータを(書き込む)更新する処理を入れる
// 書き込む情報は下記の通り
//
//	EWSTW_Adr			:FLASHのWriteアドレス
//	b_w_chg.word		:FLASHのWriteバイト数
//	Regist_Data[96+64]	:FLASHのWriteデータ

	if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS){

		//Size = 0x0FFF;		// 4k

		// 読み出し
//		serial_flash_multbyte_read(SFM_TEMP_START, SFM_REGIST_SIZE_32, work_buffer); // 読み込み  work_buffer size 32K
		serial_flash_multbyte_read(SFM_REGIST_START, SFM_REGIST_SIZE_32, work_buffer); // 読み込み  work_buffer size 32K

		Printf("FLASH_Read\r\n");

		// データ変更
//		memcpy(&work_buffer[EWSTW_Adr], &Regist_Data[0], b_w_chg.word);
        memcpy(&work_buffer[EWSTW_Adr], &Regist_Data[0], word_data);
		// 書き込み
		serial_flash_block_unlock();                        // グローバルブロックプロテクション解除
		serial_flash_erase(SFM_REGIST_START, SFM_REGIST_END);

		serial_flash_multbyte_write(SFM_REGIST_START, SFM_REGIST_SIZE_32, work_buffer);				// 32kbyte書き込み


//		serial_flash_block_unlock();                        // グローバルブロックプロテクション解除
		serial_flash_erase(SFM_REGIST_START_B, SFM_REGIST_END_B);

		serial_flash_multbyte_write(SFM_REGIST_START_B, SFM_REGIST_SIZE_32, work_buffer);				// 32kbyte書き込み

        tx_mutex_put(&mutex_sfmem);
	}

	if((VENDER_HIT != fact_config.Vender) && (Http_Use == HTTP_SEND)){		// Hitachi以外
		G_HttpFile[FILE_C].sndflg = SND_ON;                     			// [FILE]機器設定：送信有りをセット
		G_HttpCmd[HTTP_CMD_RSETF].sndflg = SND_ON;              			// [CMD] 機器設定(RSETF)：送信有りをセット
	}
	my_config.device.RegCnt++;

}




