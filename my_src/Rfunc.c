/**
 * @file	Rfunc.c
 *
 * @date	 Created on: 2019/07/29
 * @author	tooru.hayashi
 * @note	2020.01.30	v6 0130ソース反映済み
 * @note	2020.06.30 GitHub 630ソース反映済み
 * @note	2020.Jul.27 GitHub 0727ソース反映済み 
 * @note	2020.Aug.07	0807ソース反映済み
 */


#include "_r500_config.h"

#define _RFUNC_C_
#include "Rfunc.h"

//#include "hal_data.h"
#include "flag_def.h"
#include "MyDefine.h"
#include "Globals.h"
#include "General.h"
#include "Sfmem.h"
//#include "system_thread.h"
//#include "rf_thread_entry.h"
#include "lzss.h"
#include "Cmd_func.h"
//#include "auto_thread_entry.h"

#include "r500_defs.h"

extern pthread_mutex_t mutex_sfmem;


extern uint8_t realscan_err_data_add(void);

//プロトタイプ

//static  uint32_t searchscan_time = 0;                      // 前回のサーチスキャン実行時のUTIM
static  uint32_t searchscan_time[GROUP_MAX];               // 前回のサーチスキャン実行時のUTIM  // sakaguchi 2021.01.21
static  RemoteUnitRssiTable_t   S_tUnitRssiTbl;            // 子機電波強度テーブル
static  RouteArray_t scan_route_list[SCANLIST_MAX+1];      // スキャンルートリスト
static  RouteArray_t scan_route_list_all[4][SCANLIST_MAX+1];      // スキャンルートリスト
#if 0
//未使用
static void shortcut_info_clear(void);                                  // ショートカット情報　クリア
static void shortcut_info_update(void);                                 // ショートカット情報　更新
#endif
static uint8_t shortcut_info_number_read(uint8_t);                         // ショートカット情報　中継機番号　読み込み
//未使用static void shortcut_info_number_write(uint8_t, uint8_t);               // ショートカット情報　中継機番号　書き込み
static uint8_t shortcut_info_rssi_read(uint8_t);                           // ショートカット情報　ＲＳＳＩ　読み込み
//未使用static void shortcut_info_rssi_write(uint8_t, uint8_t);                 // ショートカット情報　ＲＳＳＩ　書き込み
static bool shortcut_info_search(uint8_t);                              // ショートカット情報　指定された中継器番号を探す

//uint8_t RF_get_regist_RegisterRemoteUnit(uint8_t gnum);

//  RouteArray
static void RouteArray_Clear(RouteArray_t *route);
//  WirelessGroup
static void WirelessGroup_Clear(WirelessGroup_t *grp);
static void WirelessGroup_ClearRssiTable(WirelessGroup_t *grp);
static int WirelessGroup_UpdateRepeaterRssi(WirelessGroup_t *grp, int parentNum, int repeaterNum, uint8_t rssi);
static int WirelessGroup_UpdateRemoteUnitRssi(WirelessGroup_t *grp, int parentNum, int remoteUnitNum, uint8_t rssi);
//void RF_WirelessGroup_RefreshParentTable(WirelessGroup_t *grp, uint8_t rssiLimit);
static int WirelessGroup_GetRemoteUnitRssi(const WirelessGroup_t *grp, int parentNum, int remoteUnitNum, uint8_t *rssi);
static int WirelessGroup_GetRepeaterRssi(const WirelessGroup_t *grp, int parentNum, int repeaterNum, uint8_t *rssi);
static int WirelessGroup_GetRepeaterRoute(const WirelessGroup_t *grp, int repeaterNum, RouteArray_t *route);
static int WirelessGroup_GetRemoteRoute(const WirelessGroup_t *grp, int remoteUnitNum, RouteArray_t *route);
static int WirelessGroup_RegisterRemoteUnit(WirelessGroup_t *grp, int remoteUnitNum);
static int WirelessGroup_UnregisterRemoteUnit(WirelessGroup_t *grp, int remoteUnitNum);
//  RepeaterParent
static void RepeaterParent_Clear(RepeaterParentArray_t *tbl);
static void RepeaterParent_Refresh(WirelessGroup_t *grp, uint8_t rssiLimit);
static void RepeaterParent_DoUpdate(WirelessGroup_t *grp, uint8_t rssiLimit, ParentArray_t *parents);
static int RepeaterParent_GetRoute(const RepeaterParentArray_t *tbl, int repeaterNum, RouteArray_t *route);
//  RepeaterRSSI
static void RepeaterRssi_Clear(RepeaterRssiTable_t *tbl);
static int RepeaterRssi_Index(int numL, int numR);
static uint8_t RepeaterRssi_Get(const RepeaterRssiTable_t *tbl, int numL, int numR);
static int RepeaterRssi_Update(RepeaterRssiTable_t *tbl, int numL, int numR, uint8_t rssi, bool force);
//RemoteUnit
static void RemoteUnitRssiArray_Clear(RemoteUnitRssiArray_t *rssiArray);
static void RemoteUnitRssiTable_Clear(RemoteUnitRssiTable_t *tbl);
static void RemoteUnitParent_Clear(RemoteUnitParentArray_t *parents);
static void RemoteUnitParent_Refresh(WirelessGroup_t *grp, uint8_t rssiLimit);
static int RemoteUnitRssi_Update(WirelessGroup_t *grp, int parentNum, int remoteUnitNum, uint8_t rssi, bool force);
static void Make_Required_ScanList(uint8_t grp_no, RouteArray_t *scan_route);
static void Make_Performance_ScanList(uint8_t grp_no, RouteArray_t *scan_route);
static void Make_Search_ScanList(uint8_t grp_no, RouteArray_t *scan_route);
//static void GetRoot_from_ResultTable(uint8_t grp_no, int flg, int no, RouteArray_t* route);
static bool Search_Scan_Start_Check(uint8_t grp_no);
static void scan_route_adjust(RouteArray_t *scan_route);
static void scan_route_add(RouteArray_t *scan_route, RouteArray_t add_route);
static void scan_route_delete(RouteArray_t *scan_route, RouteArray_t del_route);
static bool check_scan_notyet(int grp_no, int flg, uint8_t check);

//プロトタイプ追加
//uint32_t get_regist_group_adr_rpt_ser(uint32_t serial);   //→グローバル
//static uint8_t RF_get_rpt_registry_full(uint8_t gnum);      //→グローバル

//rf_thread_entry.cから移動
static char regu_moni_scan(uint8_t data_format);


/**
 * 中継機が受信したコマンドIDが既中継か判断する
 * @return  未中継     NORMAL_END  既中継     ERROR_END
 */
uint32_t Same_ID_Check(void)
{
    uint8_t rf_cmd, rf_cmd_id;
    uint32_t rtn = RFM_RPT_ERROR;


    rf_cmd = (r500.rf_cmd2 & 0x7f);                             // 無線コマンド番号を取得する

    switch(rf_cmd)                                              // 無線コマンド番号により上りか下りか判断する
    {
        case 0x00:
        case 0x01:
        case 0x02:
            rtn = RFM_NORMAL_END;                               // コマンド中継する(単純に止めてはいけないコマンド)
            break;

        default:
            rf_cmd_id = r500.para1.cmd_id;

            if(rf_cmd > 0x0d)                                   // 子機宛ての上り通信の場合
            {
                if((uint8_t)((regf_cmd_id_back >> 0) & 0x000000ff) != rf_cmd_id)    // 上り（子機へ）：下位を使用
                {
                    regf_cmd_id_back = (regf_cmd_id_back & 0xffffff00) | (((uint32_t)(rf_cmd_id) << 0) & 0x000000ff);
                    rtn = RFM_NORMAL_END;
                }
            }
            else                                                // 親機宛ての下り通信の場合
            {
                if((uint8_t)((regf_cmd_id_back >> 16) & 0x000000ff) != rf_cmd_id)   // 下り（子機から）：上位を使用
                {
                    regf_cmd_id_back = (regf_cmd_id_back & 0xff00ffff) | (((uint32_t)(rf_cmd_id) << 16) & 0x00ff0000);
                    rtn = RFM_NORMAL_END;
                }
            }

            break;
    }

    return (rtn);
}

/**
 * 中継情報の格納エリアに保存されている中継情報を反転する
 */
void rpt_data_reverse(void)
{
    char *top_poi,*last_poi;
    uint8_t tmp_buff;
    uint8_t harf_cnt;
    uint8_t cnt;
    uint8_t r500_para_tmp;
    r500_para_tmp = r500.para1.rpt_cnt;

    if(r500_para_tmp > 1)
    {
        harf_cnt = r500_para_tmp / 2;                           // 結果は切捨てでOK

        top_poi = &rpt_sequence[0];
        last_poi = &rpt_sequence[r500_para_tmp - 1];

        for(cnt = 0 ; cnt < harf_cnt ; cnt++, top_poi++, last_poi--)
        {
            tmp_buff = *top_poi;                                //前寄りデータと後寄りデータを入れ替えていく。
            *top_poi = *last_poi;
            *last_poi = tmp_buff;
        }
    }
}



/**
 * 子機ヘッダからプロパティ情報を読み取る
 */
void extract_property(void)
{
    property.protocol = TR5_NEW;
    property.interval = (uint16_t)((ru_header.intt[0] & 0x00ff) + (ru_header.intt[1] & 0x00ff)* 256);

    property.attribute[0] = ru_header.ch_zoku[0];
    property.attribute[1] = ru_header.ch_zoku[1];

    if(ru_header.kisyu_code == 0x5c)
    {
        //property.model = RTR501;                              // 共通ヘッダ１ブロックだけの場合の機種番号
        property.model = RTR501VA;                              // 共通ヘッダ１ブロックの他に専用ヘッダ１ブロックが記録されている場合の機種番号
        property.channel = 1;
    }
    else if(ru_header.kisyu_code == 0x5d)
    {
        //property.model = RTR502;
        property.model = RTR502VA;
        property.channel = 1;
    }
    else if(ru_header.kisyu_code == 0x5e)
    {
        //property.model = RTR503;
        property.model = RTR503VA;
        property.channel = 2;
    }
    else if(ru_header.kisyu_code == 0x55)
    {
        //property.model = RTR507;
        property.model = RTR507VA;
        property.channel = 2;
    }
    else if(ru_header.kisyu_code == 0x57)
    {
        property.model = RTR574;
        property.channel = 2;                                   // ＲＴＲ－５７４無線吸い上げでは２ＣＨ
    }
    else if(ru_header.kisyu_code == 0x56)
    {
        property.model = RTR576;

//        if(ru_header.ch_zoku[1] == 0) property.channel = 1;     // ＲＴＲ－５７６無線吸い上げ ＣＨ２属性が入っていなければ１ＣＨ
//        else                          property.channel = 2;
        property.channel = (ru_header.ch_zoku[1] == 0) ? 1 : 2;     // ＲＴＲ－５７６無線吸い上げ ＣＨ２属性が入っていなければ１ＣＨ
    }
    else if(ru_header.kisyu_code == 0xa1)
    {
        property.model = RTR505TC;
        property.channel = 1;
    }
    else if(ru_header.kisyu_code == 0xa2)
    {
        property.model = RTR505Pt;
        property.channel = 1;
    }
    else if(ru_header.kisyu_code == 0xa3)
    {
        property.model = RTR505mA;
        property.channel = 1;
    }
    else if(ru_header.kisyu_code == 0xa4)
    {
        property.model = RTR505V;
        property.channel = 1;
    }
    else if(ru_header.kisyu_code == 0xa5)
    {
        property.model = RTR505P;
        property.channel = 1;
    }
    else if(ru_header.kisyu_code == 0xa6)
    {
        property.model = RTR505IT;
        property.channel = 1;
    }
    else if(ru_header.kisyu_code == 0xaf)
    {
        property.model = RTR505TH;
        property.channel = 2;
    }
    else
    {
        property.model = 0x0000;
        property.channel = 1;
    }

    property.sendon_number = (uint16_t)((ru_header.data_byte[0] + (uint16_t)ru_header.data_byte[1] * 256) / 2 / property.channel);            // 吸い上げデータ数
    property.record_number = (uint16_t)((ru_header.all_data_byte[0] + (uint16_t)ru_header.all_data_byte[1] * 256) / 2 / property.channel);    // 総記録データ数

    Printf("extract_property model %02X\r\n", property.model);
}






/**
 * 子機登録ファイルの中に、自律動作(警報,現在値,吸上げ)対象の子機のある/なしを検出
 * @return  0:登録子機なし        1:登録子機あり
 */
uint8_t check_regist_unit(void)
{
    uint8_t group_size,group_cnt;
    uint8_t rtn = 0;

    group_size = get_regist_group_size();

    group_cnt = 0;

    while(group_cnt < group_size){									// グループ数ループ

        if(get_regist_scan_unit((uint8_t)(group_cnt+1), 0x07) != 0)
        {
            rtn = 1;
            break;
        }    
        group_cnt++;
    }

    Printf("  check regist unit %d (gs %d) \r\n", rtn, group_size);
    return (rtn);
}







/**
 * 登録ファイルからグループ数を得る
 * @return      グループ番号
 * @note    ルート情報構造体にルート情報格納済み
 */
uint8_t get_regist_group_size(void)
{
    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS)          // シリアルフラッシュ ミューテックス確保 スレッド確定
    {
        serial_flash_multbyte_read(SFM_REGIST_START, sizeof(root_registry), (char *)&root_registry);  // ルート情報構造体に読み込む
        tx_mutex_put(&mutex_sfmem);
    }

    if(root_registry.length != 0x10){
        root_registry.group = 0;   // 情報サイズが違っていたら０
    }
    if(root_registry.header != 0xfb){
        root_registry.group = 0;   // 情報種類コードが違っていたら０
    }

    //Printf("  get_regist_group_size  %d\r\n", root_registry.group);
    return(root_registry.group);
}



/**
 * 登録ファイルからグループ番号指定してグループ情報アドレスを取得する
 * @param gnum  グループ番号
 * @return      グループ情報の格納されているアドレス（SFM_REGIST_STARTからのオフセット）
 * @note        グループ情報構造体にグループ情報格納済み
 */
uint16_t get_regist_group_adr(uint8_t gnum)
{
   int i, j, max;

    uint32_t adr, len;

    i = ~gnum;
    max = get_regist_group_size();                              // 最大グループ番号（グループ数）
    adr = root_registry.length;                                 // 最初のグループヘッダアドレス

    if(max == 0){
        return(0);
    }

    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS)              // シリアルフラッシュ ミューテックス確保 スレッド確定
    {
        for(i = 1; i <= max; i++)
        {
            serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), sizeof(group_registry), (char *)&group_registry);  // グループ情報構造体に読み込む

            if(i == gnum){
                break;                                // 指定グループなら抜ける
            }

            adr += group_registry.length;                       // グループヘッダ分アドレスＵＰ

            for(j = 0; j < group_registry.altogether; j++)      // 中継機情報分アドレスＵＰ
            {
                serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), 2, (char *)&len);  // 中継機情報サイズを読み込む
                adr += (len & 0x0000ffff);
            }

            for(j = 0; j < group_registry.remote_unit; j++)     // 子機情報分アドレスＵＰ
            {
                serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), 2, (char *)&len);  // 子機情報サイズを読み込む
                adr += (len & 0x0000ffff);
            }
        }

        tx_mutex_put(&mutex_sfmem);
    }

    if(i != gnum){
        adr = 0;                                      // 見つからなかった場合
    }

    return((uint16_t)adr);
}





//----------------------------------------------------------------------------------------------------
// 作成中
/**
 * 登録ファイルからシリアルNoを指定して子機取得
 *
 * @param serial
 * @return
 */
uint32_t get_regist_group_adr_ser(uint32_t serial)
{

	uint8_t i, j, max;
	uint16_t adr, len;
	uint8_t rtn = 0xfe;			// 0xfeは処理中を示すステータス

	max = get_regist_group_size();                              // 最大グループ番号（グループ数）
	adr = root_registry.length;                                 // 最初のグループヘッダアドレス

    if(max == 0){
        return(0xff);		// 見つからなかった
    }

    for(j = 1; j <= max; j++)
	{
		if(rtn == 0xfe){
			adr = get_regist_group_adr(j);										// グループ情報構造体に読み込む
			if(adr == 0){
			    rtn = 0xff;
			}

			adr = (uint16_t)(adr + group_registry.length);								        // グループヘッダ分アドレスＵＰ

			if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS)			// シリアルフラッシュ ミューテックス確保 スレッド確定
			{
		        // 子機番号から子機の親番号を探す
				for(i = 0; i < group_registry.altogether; i++)					// 中継機情報分アドレスＵＰ
				{
					serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), 2, (char *)&len);	// 中継機情報サイズを読み込む
					adr = (uint16_t)(adr + len);
				}

		        for(i = 0; i < group_registry.remote_unit; i++)
				{
					serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), sizeof(ru_registry), (char *)&ru_registry);	// 子機情報構造体に読み込む

					if(ru_registry.rtr501.serial == serial){
						EWSTW_Adr = (uint32_t)(adr);													// 登録パラメータを更新する為にFLASHのアドレスを保存しておく
						memcpy(&Regist_Data[0],&ru_registry.rtr501.length,ru_registry.rtr501.length);	// 登録パラメータを更新する為に現状登録パラメータを保存しておく
						rtn = j;
						break;
					}
					adr = (uint16_t)(adr + ru_registry.rtr501.length);
				}
				tx_mutex_put(&mutex_sfmem);
			}
		}
		else{
			break;
		}
	}
	if(rtn == 0xfe){
	    rtn = 0xff;		// 見つからなかった
	}

	return(rtn);
}


//----------------------------------------------------------------------------------------------------
// 作成中
/**
 * 登録ファイルからシリアルNoを指定して中継機取得
 *
 * @param serial
 * @return
 */
uint32_t get_regist_group_adr_rpt_ser(uint32_t serial)
{

	uint8_t i, j, max;
	uint16_t adr;
	uint8_t rtn = 0xfe;			// 0xfeは処理中を示すステータス

	max = get_regist_group_size();                              // 最大グループ番号（グループ数）
	adr = root_registry.length;                                 // 最初のグループヘッダアドレス

    if(max == 0){
        return(0xff);		// 見つからなかった
    }

    for(j = 1; j <= max; j++)
	{
		if(rtn == 0xfe){
			adr = get_regist_group_adr(j);										// グループ情報構造体に読み込む
			if(adr == 0){
			    rtn = 0xff;
			}

			adr = (uint16_t)(adr + group_registry.length);								        // グループヘッダ分アドレスＵＰ

			if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS)			// シリアルフラッシュ ミューテックス確保 スレッド確定
			{
		        // 子機番号から子機の親番号を探す
				for(i = 0; i < group_registry.altogether; i++)					// 中継機情報分アドレスＵＰ
				{
//					serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), 2, (char *)&len);	// 中継機情報サイズを読み込む
					serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), sizeof(rpt_registry), (char *)&rpt_registry);	// 中継機情報サイズを読み込む

					if(rpt_registry.serial == serial){
//						EWSTW_Adr = (uint32_t)(adr);													// 登録パラメータを更新する為にFLASHのアドレスを保存しておく
//						memcpy(&Regist_Data[0],&ru_registry.rtr501.length,ru_registry.rtr501.length);	// 登録パラメータを更新する為に現状登録パラメータを保存しておく
						rtn = j;
						break;
					}

//					adr = (uint16_t)(adr + len);
					adr = (uint16_t)(adr + rpt_registry.length);
				}

				tx_mutex_put(&mutex_sfmem);
			}
		}
		else{
			break;
		}
	}
	if(rtn == 0xfe){
	    rtn = 0xff;		// 見つからなかった
	}

	return(rtn);
}





























/**
 * 登録ファイルからグループ番号を指定して自動吸上げ対象子機リストを作成する
 * @param gnum  グループ番号、子機リスト情報アドレス
 * @return      対象子機台数  0 グループ番号または子機番号が見つからない
 */
uint8_t get_regist_gather_unit(uint8_t gnum)
{
    uint8_t rtn = 0;
    uint8_t i;
    uint16_t  adr, len;

    adr = get_regist_group_adr(gnum);									// グループ情報構造体に読み込む
    if(adr == 0){
        return(0);
    }

	memset(&download_buff,0x00,sizeof(download_buff));

    adr = (uint16_t)(adr + group_registry.length);								// グループヘッダ分アドレスＵＰ

    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS)			// シリアルフラッシュ ミューテックス確保 スレッド確定
    {
        // 子機番号から子機の親番号を探す
		for(i = 0; i < group_registry.altogether; i++)					// 中継機情報分アドレスＵＰ
		{
			serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), 2, (char *)&len);	// 中継機情報サイズを読み込む
			adr = (uint16_t)(adr + len);
		}

        for(i = 0; i < group_registry.remote_unit; i++)
		{
			serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), sizeof(ru_registry), (char *)&ru_registry);	// 子機情報構造体に読み込む

			if((ru_registry.rtr501.set_flag & 0x04) != 0){
				set_flag(download_buff.do_unit, (uint8_t)ru_registry.rtr501.number);
				rtn ++;
			}
            if((ru_registry.rtr501.header == 0xfa) || (ru_registry.rtr501.header == 0xf9)){		// ＲＴＲ－５７４、５７６の場合、3,4chのデータ吸上げするか否か
				if((ru_registry.rtr501.set_flag & 0x08) != 0){
					set_flag(download_buff.do_unit, (uint8_t)(ru_registry.rtr501.number + 1));
					rtn ++;
				}
			}

			adr = (uint16_t)( adr + ru_registry.rtr501.length);
		}
		tx_mutex_put(&mutex_sfmem);
	}
	return(rtn);
}




/**
 * 登録ファイルからグループ番号を指定して自動自動ルート作成変数に登録する
 * @param gnum  グループ番号、子機リスト情報アドレス
 * @return      対象子機台数  0 グループ番号または子機番号が見つからない
 */
uint8_t RF_get_regist_RegisterRemoteUnit(uint8_t gnum)
{
//    uint8_t rtn = 0;
    uint8_t i,max_unit_no;
    uint16_t  adr, len;
	
    adr = get_regist_group_adr(gnum);									// グループ情報構造体に読み込む
    if(adr == 0){
        return(0);
    }

	//memset(&download_buff,0x00,sizeof(download_buff));

    adr = (uint16_t)(adr + group_registry.length);								// グループヘッダ分アドレスＵＰ
	max_unit_no = 0;					// 登録されている最大子機番号把握する為、最初は0としておく
	
    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS)			// シリアルフラッシュ ミューテックス確保 スレッド確定
    {
        // 子機番号から子機の親番号を探す
		for(i = 0; i < group_registry.altogether; i++)					// 中継機情報分アドレスＵＰ
		{
			serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), 2, (char *)&len);	// 中継機情報サイズを読み込む
			adr = (uint16_t)(adr + len);
		}

        for(i = 0; i < group_registry.remote_unit; i++)
		{
			serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), sizeof(ru_registry), (char *)&ru_registry);	// 子機情報構造体に読み込む

			WirelessGroup_RegisterRemoteUnit(&RF_Group[gnum-1], ru_registry.rtr501.number);

			if(max_unit_no < ru_registry.rtr501.number){
				max_unit_no = ru_registry.rtr501.number;
			}

			if((ru_registry.rtr501.header == 0xfa) || (ru_registry.rtr501.header == 0xf9)){		// ＲＴＲ－５７４、５７６の場合、3,4chのデータ吸上げするか否か
				max_unit_no++;	// 574,576は後半の子機番号も最大対象
			}

			adr = (uint16_t)( adr + ru_registry.rtr501.length);
		}
		tx_mutex_put(&mutex_sfmem);
	}

	group_registry.max_unit_no = max_unit_no;			// 最大子機番号

	return(max_unit_no);                                // 最大子機番号を返す
}






















/**
 * 登録ファイルからグループ番号を指定してリアルスキャン対象子機リストを作成する
 * @param gnum  グループ番号
 * @param PARA  子機リスト情報アドレス
 * @return      対象子機台数      0 グループ番号または子機番号が見つからない
 * @note        PARAは、警報・モニタ・吸上げのマスク
 * @note        確認済み
 */
uint8_t get_regist_scan_unit(uint8_t gnum, uint8_t PARA)
{
    uint8_t rtn = 0;
    uint8_t i;
    uint16_t  adr, len;

	uint8_t max_unit_no = 0;
//	uint8_t max_rpt_no;			// グローバル変数にする
	uint8_t read_rpt_no;


    adr = get_regist_group_adr(gnum);									// グループ情報構造体に読み込む
    Printf("--->get_regist_scan_unit  adr=%d \r\n",adr);
    if(adr == 0){
        return(0);
    }

	memset(&realscan_buff, 0x00, sizeof(realscan_buff));

    adr = (uint16_t)(adr + group_registry.length);								// グループヘッダ分アドレスＵＰ

    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS)			// シリアルフラッシュ ミューテックス確保 スレッド確定
    {
        // 子機番号から子機の親番号を探す
		max_rpt_no = 0;
		for(i = 0; i < group_registry.altogether; i++)					// 中継機情報分アドレスＵＰ
		{
			serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), 2, (char *)&len);	// 中継機情報サイズを読み込む

			serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr + 3), 1, (char *)&read_rpt_no);	// 最大中継機番号を把握
			if(max_rpt_no < read_rpt_no){
			    max_rpt_no = read_rpt_no;
			}

			adr = (uint16_t)(adr + len);
		}

        for(i = 0; i < group_registry.remote_unit; i++)
		{
			serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), sizeof(ru_registry), (char *)&ru_registry);	// 子機情報構造体に読み込む

//			if(PARA != 0){

				if((ru_registry.rtr501.header == 0xfa)||(ru_registry.rtr501.header == 0xf9)){
					if((ru_registry.rtr501.set_flag & PARA) != 0){											// モニタリング、警報監視、自動吸上げ対象子機か判断
						if(max_unit_no < ru_registry.rtr501.number){
						    max_unit_no = ru_registry.rtr501.number;	// 最大子機番号を把握する
						}
						set_flag(realscan_buff.do_unit,ru_registry.rtr501.number);						// 通信対象子機フラグセット
						set_flag(realscan_buff.do_unit,(uint8_t)(ru_registry.rtr501.number+1));					// 通信対象子機フラグセット (2個1機種は連番もリアルスキャン対象)
						set_flag(realscan_buff.do_rpt,ru_registry.rtr501.superior);						// 通信対象中継機フラグセット
						rtn ++;
					}
					else if(PARA == 0){		// PARA=0なら登録情報を確認するだけ
						if(max_unit_no < ru_registry.rtr501.number){
						    max_unit_no = ru_registry.rtr501.number;	// 最大子機番号を把握する
						}
						rtn ++;
					}
                    max_unit_no = (uint8_t)(max_unit_no + 1);		// 574,576なら子機番号を2個使うので 2020/07/21 segi
				}
				else{
					if((ru_registry.rtr501.set_flag & PARA) != 0){											// モニタリング、警報監視、自動吸上げ対象子機か判断
						if(max_unit_no < ru_registry.rtr501.number){
						    max_unit_no = ru_registry.rtr501.number;	// 最大子機番号を把握する
						}
						set_flag(&realscan_buff.do_unit[0],(uint8_t)ru_registry.rtr501.number);						// 通信対象子機フラグセット
						set_flag(&realscan_buff.do_rpt[0],(uint8_t)ru_registry.rtr501.superior);						// 通信対象中継機フラグセット
						rtn ++;
					}
					else if(PARA == 0){		// PARA=0なら登録情報を確認するだけ
						if(max_unit_no < ru_registry.rtr501.number){
						    max_unit_no = ru_registry.rtr501.number;	// 最大子機番号を把握する
						}
						rtn ++;
					}

				}

//			}

			adr = (uint16_t)(adr + ru_registry.rtr501.length);
		}

		group_registry.max_unit_no = max_unit_no;
		tx_mutex_put(&mutex_sfmem);
	}
	return(rtn);														// 

}




/**
 * @brief   登録ファイルからグループ番号と子機番号を指定して中継機情報を作成する
 * @param gnum  グループ番号
 * @param knum  子機番号
 * @param relay 中継機情報アドレス
 * @return  中継機数（中継機無しの時１）  0 グループ番号または子機番号が見つからない
 */
uint8_t get_regist_relay_inf(uint8_t gnum, uint8_t knum, char *relay)
{
    uint8_t link, rtn = 1;
    uint8_t i, j;
    uint16_t  rpt, adr, len;


    adr = get_regist_group_adr(gnum);                           // グループ情報構造体に読み込む
    if(adr == 0){
        return(0);
    }

    adr = (uint16_t)(adr + group_registry.length);                               // グループヘッダ分アドレスＵＰ

    relay[0] = 0x00;                                            // 中継機情報の先頭バイトは必ず０ｘ００



//    if(group_registry.altogether == 0) return(1);							// 中継機0個でも子機情報読み込む為にここでreturnさせない




    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS)              // シリアルフラッシュ ミューテックス確保 スレッド確定
    {
        // 子機番号から子機の親番号を探す

        rpt = adr;                                              // 中継機情報アドレスラッチ

        for(i = 0; i < group_registry.altogether; i++)          // 中継機情報分アドレスＵＰ
        {
            serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), 2, (char *)&len);  // 中継機情報サイズを読み込む
            adr = (uint16_t)(adr + len);
        }

        for(i = 0; i < group_registry.remote_unit; i++)
        {
            serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), sizeof(ru_registry), (char *)&ru_registry);    // 子機情報構造体に読み込む

            if(ru_registry.rtr501.number == knum){
                break;
            }

            if((ru_registry.rtr501.header == 0xfa) || (ru_registry.rtr501.header == 0xf9))  // ＲＴＲ－５７４、５７６の場合、子機番号の小さい方へ読み替える
            {
                if((ru_registry.rtr501.number + 1) == knum)
                {
                    knum = ru_registry.rtr501.number;
                    break;
                }
            }

            adr = (uint16_t)(adr + ru_registry.rtr501.length);
        }

        if(ru_registry.rtr501.number != knum)                   // 指定されたグループに子機番号が見つからない
        {
            rtn = 0;
            goto end_relay_inf;
        }

        if(ru_registry.rtr501.superior == 0x00){
            goto end_relay_inf; // 指定された子機の親番号が親機（０ｘ００）だったとき
        }

        // 繋がっていく中継機をたどる

        rpt_registry.superior = ru_registry.rtr501.superior;    // 子機の上位中継機番号

        for(j = 0; j < group_registry.altogether; j++)
        {
            link = rpt_registry.superior;

            adr = rpt;                                          // 中継機情報アドレスを指定

            for(i = 0; i < group_registry.altogether; i++)
            {
                serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), sizeof(rpt_registry), (char *)&rpt_registry);  // 中継機情報構造体に読み込む
                if(link == rpt_registry.number){
                    break;
                }
                adr = (uint16_t)(adr + rpt_registry.length);
            }

            if(i >= group_registry.altogether)                  // 全スキャンしても見つからなかった
            {
                rtn = 0;
                goto end_relay_inf;
            }

            relay[j + 1] = rpt_registry.number;

            if(rpt_registry.superior == 0x00){
                break;            // 親番号が０
            }
        }

        link = (uint8_t)(j + 1);                                           // 中継数を確定

        for(j = 0; j < (link / 2); j++)                         // 中継機部分を反転
        {
            rpt = relay[link - j];
            relay[link - j] = relay[j + 1];
            relay[j + 1] = (uint8_t)rpt;       //2020.01.20 キャスト追加 uint16_t -> uint8_t
        }

        rtn = (uint8_t)(link + 1);

        end_relay_inf:;

        tx_mutex_put(&mutex_sfmem);
    }

    return(rtn);                                                // 子機と繋がる中継機が見つからなかった
}





/**
 * 登録ファイルからグループ番号と中継機番号を指定して中継機情報を作成する
 * @param gnum      グループ番号
 * @param knum      中継機番号
 * @param relay     中継機情報アドレス
 * @return          中継機数（中継機無しの時１）  0 グループ番号または子機番号が見つからない
 */
uint8_t get_regist_relay_inf_relay(uint8_t gnum, uint8_t knum, char *relay)
{
    uint8_t link, rtn = 1;
    uint8_t i, j;
    uint16_t  rpt, adr;


    adr = get_regist_group_adr(gnum);                           // グループ情報構造体に読み込む
    if(adr == 0){
        return(0);
    }

    adr = (uint16_t)(adr + group_registry.length);                               // グループヘッダ分アドレスＵＰ

    relay[0] = 0x00;                                            // 中継機情報の先頭バイトは必ず０ｘ００

    if(knum == 0){
        return(1);							// 中継機0個でも子機情報読み込む為にここでreturnさせない
    }


    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS)              // シリアルフラッシュ ミューテックス確保 スレッド確定
    {
        // 子機番号から子機の親番号を探す

		rpt = adr;                                              // 中継機情報アドレスラッチ

		for(i = 0; i < group_registry.altogether; i++)          // 中継機情報分アドレスＵＰ
		{
			//serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), 2, (char *)&len);  // 中継機情報サイズを読み込む
			serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), sizeof(rpt_registry), (char *)&rpt_registry);    // 中継機情報構造体に読み込む
			//adr += len;
			if(rpt_registry.number == knum){
			    break;
			}
			adr = (uint16_t)(adr + rpt_registry.length);
		}

        if(rpt_registry.number != knum)                   // 指定されたグループに子機番号が見つからない
        {
            rtn = 0;
            goto end_relay_inf;
        }


/*
        for(i = 0; i < group_registry.remote_unit; i++)
        {
            serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), sizeof(ru_registry), (char *)&ru_registry);    // 子機情報構造体に読み込む

            if(ru_registry.rtr501.number == knum) break;

            if((ru_registry.rtr501.header == 0xfa) || (ru_registry.rtr501.header == 0xf9))  // ＲＴＲ－５７４、５７６の場合、子機番号の小さい方へ読み替える
            {
                if((ru_registry.rtr501.number + 1) == knum)
                {
                    knum = ru_registry.rtr501.number;
                    break;
                }
            }

            adr += ru_registry.rtr501.length;
        }

        if(ru_registry.rtr501.number != knum)                   // 指定されたグループに子機番号が見つからない
        {
            rtn = 0;
            goto end_relay_inf;
        }

        if(ru_registry.rtr501.superior == 0x00) goto end_relay_inf; // 指定された子機の親番号が親機（０ｘ００）だったとき
*/


        // 繋がっていく中継機をたどる

//        rpt_registry.superior = ru_registry.rtr501.superior;    // 子機の上位中継機番号
		rpt_registry.superior = rpt_registry.number;

        for(j = 0; j < group_registry.altogether; j++)
        {
            link = rpt_registry.superior;

            adr = rpt;                                          // 中継機情報アドレスを指定

            for(i = 0; i < group_registry.altogether; i++)
            {
                serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), sizeof(rpt_registry), (char *)&rpt_registry);  // 中継機情報構造体に読み込む
                if(link == rpt_registry.number){
                    break;
                }
                adr = (uint16_t)(adr + rpt_registry.length);
            }

            if(i >= group_registry.altogether)                  // 全スキャンしても見つからなかった
            {
                rtn = 0;
                goto end_relay_inf;
            }

            relay[j + 1] = rpt_registry.number;

            if(rpt_registry.superior == 0x00){
                break;            // 親番号が０
            }
        }

        link = (uint8_t)(j + 1);                                           // 中継数を確定

        for(j = 0; j < (link / 2); j++)                         // 中継機部分を反転
        {
            rpt = relay[link - j];
            relay[link - j] = relay[j + 1];
            relay[j + 1] = (uint8_t)rpt;        //2020.01.20 キャスト追加 uint16_t -> uint8_t
        }

        rtn = (uint8_t)(link + 1);

        end_relay_inf:;

        tx_mutex_put(&mutex_sfmem);
    }

    return(rtn);                                                // 子機と繋がる中継機が見つからなかった
}




/**
 * 登録ファイルからグループ番号と中継機番号を指定して中継機情報を作成する
 * @param gnum      グループ番号
 * @param knum      中継機番号
 * @return          1:指定された中継機あった  0:グループ番号または子機番号が見つからない
 * @note    2020.04.24  rpt変数にadrをセットしているのにadrの方を弄っていたのを修正
 */
uint8_t get_rpt_registry(uint8_t gnum, uint8_t knum)
{
    uint8_t rtn = 1;
    uint8_t i;
    uint16_t  rpt, adr;


    adr = get_regist_group_adr(gnum);                           // グループ情報構造体に読み込む
    if(adr == 0){
        return(0);
    }

    adr = (uint16_t)(adr + group_registry.length);                               // グループヘッダ分アドレスＵＰ


    if(knum == 0){
        return(1);							// 中継機0個でも子機情報読み込む為にここでreturnさせない
    }


    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS)              // シリアルフラッシュ ミューテックス確保 スレッド確定
    {
        // 子機番号から子機の親番号を探す

		rpt = adr;                                              // 中継機情報アドレスラッチ

		for(i = 0; i < group_registry.altogether; i++)          // 中継機情報分アドレスＵＰ
		{
			//serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), 2, (char *)&len);  // 中継機情報サイズを読み込む
			serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + rpt), sizeof(rpt_registry), (char *)&rpt_registry);    // 中継機情報構造体に読み込む
			//adr += len;
			if(rpt_registry.number == knum){
			    break;
			}
			rpt = (uint16_t)(rpt + rpt_registry.length);
		}

        if(rpt_registry.number != knum)                   // 指定されたグループに中継機番号が見つからない
        {
            rtn = 0;
            goto end_relay_inf;
        }

        end_relay_inf:;

        tx_mutex_put(&mutex_sfmem);
    }

    return(rtn);                                                // 子機と繋がる中継機が見つからなかった
}







/**
 * 登録ファイルからグループ番号を指定して中継機リストを作成する
 * @param gnum      グループ番号
 * @return          1:指定された中継機あった  0:グループ番号または子機番号が見つからない
 * @note    2020.04.24  rpt変数にadrをセットしているのにadrの方を弄っていたのを修正
 */
uint8_t RF_get_rpt_registry_full(uint8_t gnum)
{
    uint8_t i;
    uint16_t  rpt, adr;


	memset(realscan_buff.do_rpt,0x00, sizeof(realscan_buff.do_rpt));
	memset(realscan_buff.over_rpt,0x00,sizeof(realscan_buff.over_rpt));


    adr = get_regist_group_adr(gnum);                           // グループ情報構造体に読み込む
    if(adr == 0){
        return(0);
    }

    adr = (uint16_t)(adr + group_registry.length);                               // グループヘッダ分アドレスＵＰ


    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS)              // シリアルフラッシュ ミューテックス確保 スレッド確定
    {
        // 子機番号から子機の親番号を探す
		max_rpt_no = 0;		// 最大中継機番号も把握する

		rpt = adr;                                              // 中継機情報アドレスラッチ

		for(i = 0; i < group_registry.altogether; i++)          // 中継機情報分アドレスＵＰ
		{
			serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + rpt), sizeof(rpt_registry), (char *)&rpt_registry);    // 中継機情報構造体に読み込む

			set_flag(&realscan_buff.do_rpt[0],(uint8_t)rpt_registry.number);						// 通信対象中継機フラグセット

			if(max_rpt_no < rpt_registry.number){
			    max_rpt_no = rpt_registry.number;
			}

			rpt = (uint16_t)(rpt + rpt_registry.length);
		}

        tx_mutex_put(&mutex_sfmem);
    }

//    return(group_registry.altogether);                    // 中継機台数つからなかった
    return(max_rpt_no);                 // 最大中継機番号を返す
}



































#if 0
//未使用





/**
 * ショートカット情報をクリア
 * @note    未使用
 */
static void shortcut_info_clear(void)
{
    regf_shortcut_info0 = 0x00000000;
    regf_shortcut_info1 = 0x00000000;
    regf_shortcut_info2 = 0x00000000;
    regf_shortcut_info3 = 0x00000000;
}



/**
 * ショートカット情報を更新
 * @note 未使用
 */
static void shortcut_info_update(void)
{
    uint8_t i, max, diff, place = 0;


    // 受信実績のある中継機が見つかった場合、ＲＳＳＩ更新
    for(i = 0; i < 8; i++)
    {
        if(shortcut_info_number_read(i) == r500.my_rpt)
        {
            shortcut_info_rssi_write(i, r500.rssi);
            return;
        }
    }

    // 空白エリアを見つけて保存
    for(i = 0; i < 8; i++)
    {
        if((shortcut_info_number_read(i) == 0x00) && (shortcut_info_rssi_read(i) == 0x00))
        {
            shortcut_info_number_write(i, r500.my_rpt);
            shortcut_info_rssi_write(i, r500.rssi);
            return;
        }
    }

    // 空白エリアが無い場合、中継機番号が一番離れたエリアに上書きする
    for(max = 0, diff = 0, i = 0; i < 8; i++)
    {
        diff = shortcut_info_number_read(i);

//        if(diff > my_rpt_number) diff = (uint8_t)(diff - my_rpt_number);
//        else                     diff = (uint8_t)(my_rpt_number - diff);
        diff = (diff > my_rpt_number) ? (uint8_t)(diff - my_rpt_number) : (uint8_t)(my_rpt_number - diff);

        if(max < diff)
        {
            max = diff;
            place = i;
        }
    }

    shortcut_info_number_write(place, r500.my_rpt);
    shortcut_info_rssi_write(place, r500.rssi);
    return;
}
#endif

/**
 * ショートカット情報 中継機番号 読み込み
 * @param n     記憶位置０～７
 * @return      中継機番号
 * @note    未使用
 */
static uint8_t shortcut_info_number_read(uint8_t n)
{
    switch(n)
    {
        case 0:
            return((uint8_t)(regf_shortcut_info0 & 0x000000ff));

        case 1:
            return((uint8_t)((regf_shortcut_info0 & 0x0000ff00) >> 8));

        case 2:
            return((uint8_t)((regf_shortcut_info0 & 0x00ff0000) >> 16));

        case 3:
            return((uint8_t)((regf_shortcut_info0 & 0xff000000) >> 24));

        case 4:
            return((uint8_t)(regf_shortcut_info1 & 0x000000ff));

        case 5:
            return((uint8_t)((regf_shortcut_info1 & 0x0000ff00) >> 8));

        case 6:
            return((uint8_t)((regf_shortcut_info1 & 0x00ff0000) >> 16));

        case 7:
            return((uint8_t)((regf_shortcut_info1 & 0xff000000) >> 24));

        default:
            return(0xfe);
    }
}


#if 0
//未使用
/**
 * ショートカット情報 中継機番号 書き込み
 * @param n     記憶位置０～７
 * @param num   中継機番号
 */
static void shortcut_info_number_write(uint8_t n, uint8_t num)
{
    switch(n)
    {
        case 0:
            regf_shortcut_info0 = ((regf_shortcut_info0 & 0xffffff00) | num);
            return;

        case 1:
            regf_shortcut_info0 = ((regf_shortcut_info0 & 0xffff00ff) | ((uint32_t)num << 8));
            return;

        case 2:
            regf_shortcut_info0 = ((regf_shortcut_info0 & 0xff00ffff) | ((uint32_t)num << 16));
            return;

        case 3:
            regf_shortcut_info0 = ((regf_shortcut_info0 & 0x00ffffff) | ((uint32_t)num << 24));
            return;

        case 4:
            regf_shortcut_info1 = ((regf_shortcut_info1 & 0xffffff00) | num);
            return;

        case 5:
            regf_shortcut_info1 = ((regf_shortcut_info1 & 0xffff00ff) | ((uint32_t)num << 8));
            return;

        case 6:
            regf_shortcut_info1 = ((regf_shortcut_info1 & 0xff00ffff) | ((uint32_t)num << 16));
            return;

        case 7:
            regf_shortcut_info1 = ((regf_shortcut_info1 & 0x00ffffff) | ((uint32_t)num << 24));
            return;

        default:
            break;
    }
}
#endif


/**
 * ショートカット情報 ＲＳＳＩ 読み込み
 * @param n     記憶位置０～７
 * @return      RSSI
 */
static uint8_t shortcut_info_rssi_read(uint8_t n)
{
    switch(n)
    {
        case 0:
            return((uint8_t)(regf_shortcut_info2 & 0x000000ff));

        case 1:
            return((uint8_t)((regf_shortcut_info2 & 0x0000ff00) >> 8));

        case 2:
            return((uint8_t)((regf_shortcut_info2 & 0x00ff0000) >> 16));

        case 3:
            return((uint8_t)((regf_shortcut_info2 & 0xff000000) >> 24));

        case 4:
            return((uint8_t)(regf_shortcut_info3 & 0x000000ff));

        case 5:
            return((uint8_t)((regf_shortcut_info3 & 0x0000ff00) >> 8));

        case 6:
            return((uint8_t)((regf_shortcut_info3 & 0x00ff0000) >> 16));

        case 7:
            return((uint8_t)((regf_shortcut_info3 & 0xff000000) >> 24));

        default:
            return(0);
    }
}

#if 0
//未使用
/**
 * ショートカット情報 ＲＳＳＩ 書き込み
 * @param n     記憶位置０～７
 * @param rssi  RSSI
 * @see regf_shortcut_info2
 * @see regf_shortcut_info3
 */
static void shortcut_info_rssi_write(uint8_t n, uint8_t rssi)
{
    switch(n)
    {
        case 0:
            regf_shortcut_info2 = ((regf_shortcut_info2 & 0xffffff00) | rssi);
            return;

        case 1:
            regf_shortcut_info2 = ((regf_shortcut_info2 & 0xffff00ff) | ((uint32_t)rssi << 8));
            return;

        case 2:
            regf_shortcut_info2 = ((regf_shortcut_info2 & 0xff00ffff) | ((uint32_t)rssi << 16));
            return;

        case 3:
            regf_shortcut_info2 = ((regf_shortcut_info2 & 0x00ffffff) | ((uint32_t)rssi << 24));
            return;

        case 4:
            regf_shortcut_info3 = ((regf_shortcut_info3 & 0xffffff00) | rssi);
            return;

        case 5:
            regf_shortcut_info3 = ((regf_shortcut_info3 & 0xffff00ff) | ((uint32_t)rssi << 8));
            return;

        case 6:
            regf_shortcut_info3 = ((regf_shortcut_info3 & 0xff00ffff) | ((uint32_t)rssi << 16));
            return;

        case 7:
            regf_shortcut_info3 = ((regf_shortcut_info3 & 0x00ffffff) | ((uint32_t)rssi << 24));
            return;

        default:
            break;
    }
}

#endif

/**
 * ショートカット情報 指定された中継器番号を探す
 * @param num   中継機番号
 * @return      true:見つかった false:見つからない
 */
static bool shortcut_info_search(uint8_t num)
{
    uint8_t i;


    for(i = 0; i < 8; i++)
    {
        // ＤＥＢＵＧ
        //if((shortcut_info_number_read(i) == num) && (shortcut_info_rssi_read(i) > 10)) return(true);
        //if((shortcut_info_number_read(i) == num) && (shortcut_info_rssi_read(i) > 40)) return(true);
        if((shortcut_info_number_read(i) == num) && (shortcut_info_rssi_read(i) > 100)) return(true);
    }

    return(false);
}


/**
 * ショートカット情報 次の中継機を指定する
 * @return
 */
uint8_t shortcut_info_redesignation(void)
{
    uint8_t i, n, k;


    if((my_rpt_number != r500.end_rpt) && (r500.para1.rpt_cnt > 1)) // 末端中継機ではない、中継あり
    {
        /*
        // ＤＥＢＵＧ
        if(r500.end_rpt == 0)
        {
            *(uint32_t *)&huge_buffer[0] = regf_shortcut_info0;
            *(uint32_t *)&huge_buffer[4] = regf_shortcut_info1;
            *(uint32_t *)&huge_buffer[8] = regf_shortcut_info2;
            *(uint32_t *)&huge_buffer[12] = regf_shortcut_info3;
            *(uint32_t *)&huge_buffer[16] = 0x55555555L;

        }
        */

        n = (rpt_cnt_search(r500.para1.rpt_cnt));               // 自局が見つかった中継テーブル番号を保存

        if(n < r500.para1.rpt_cnt)                              // 自機が見つかった
        {
            for(i = (uint8_t)(r500.para1.rpt_cnt - 1); i > n; i--)
            {
                k = rpt_sequence[i];

                if(shortcut_info_search(k) == true)
                {
                    // ＤＥＢＵＧ
                    //return(r500.next_rpt);
                    return(k);
                }
            }
        }
    }

    return(r500.next_rpt);
}



/**
 * RTR-500が受信したRSSIレベルから1-5段階を返す(親機でも使用する)
 * 使用機種：RTR-500GSM RTR-500 RTR-500W
 * @param OrgData   R500無線モジュールから出力されたレベル(48-145位)
 * @return  1-5段階のアプリに渡すレベル
 * @note    同様の動きをする関数がある ExLevelWL()
 * @see     ExLevelWL()
 */
uint8_t rtr500_RSSI_1to5(uint8_t OrgData)
{
	uint8_t cnt;
	const uint16_t Cmp_RSSI[5] = {120,100,80,60};

	for(cnt = 0 ; cnt < 4 ; cnt++){
		if(OrgData > Cmp_RSSI[cnt]){
			break;
		}
	}
	return ((uint8_t)(5 - cnt));
}
//********** end of rtr500_RSSI_1to5() **********









//########## 中継ルート作成用の処理 ##########



/// @brief  フルモニタリングの開始判定
/// @param[in] flg  0:モニタ時、1:吸い上げ時
/// @return true/false（実施する/実施しない）
/// フルモニタリングを実施するか判定する
bool RF_full_moni_start_check(uint8_t flg)
{
    bool bRet = false;

    if( flg == 0 ){          // モニタ時

    	// 前回のフルモニタリングから1時間経過？
        if( FULL_MONI_INTERVAL <= RfMonit_time ){      // 前回のフルモニタリングから1時間以上経過
// sakaguchi 2020.12.23 ↓
//            if( (moni_err[0] == 1 ) || (moni_err[1] == 1 )
//             || (moni_err[2] == 1 ) || (moni_err[3] == 1 ) ){           // 前回通信が失敗している
//                bRet = true;
//            }
            if( (moni_err[0] >= 1 ) || (moni_err[1] >= 1 )
             || (moni_err[2] >= 1 ) || (moni_err[3] >= 1 ) ){           // 前回通信が失敗している
                bRet = true;
            }
// sakaguchi 2020.12.23 ↑
        }

	}else if( flg == 1 ){   // 吸い上げ時

// sakaguchi 2020.12.23 ↓
//    	if( (download_err[0] == 1 ) || (download_err[1] == 1 )
//            || (download_err[2] == 1 ) || (download_err[3] == 1 ) ){   // 前回通信が失敗している
//            bRet = true;
//        }
        if( (download_err[0] >= 1 ) || (download_err[1] >= 1 )
         || (download_err[2] >= 1 ) || (download_err[3] >= 1 ) ){   // 前回通信が失敗している
            bRet = true;
        }
// sakaguchi 2020.12.23 ↑
	}
    return(bRet);

}


/// @brief  フルモニタリングの初期化処理
void RF_full_moni_init(void)
{
    uint8_t group_cnt;	//グループ個数ループするカウンタ

    memset(&S_tUnitRssiTbl, RSSI_VOID, sizeof(S_tUnitRssiTbl));
    RemoteUnitRssiTable_Clear(&S_tUnitRssiTbl);         // 子機電波強度テーブルのクリア

    //モニタリング用変数の初期化
	for( group_cnt=1; group_cnt<=GROUP_MAX; group_cnt++ ){
        WirelessGroup_Clear(&RF_Group[group_cnt-1]);	                    // 無線ルート情報の初期化	grp_noは1～指定になっている
		RF_Group[group_cnt-1].allRemoteUnitRssiTable = &S_tUnitRssiTbl;     // 電波強度テーブルの指定
        RF_get_regist_RegisterRemoteUnit(group_cnt);	                	// 登録ファイルからグループ番号を指定して無線ルート情報に子機を登録する
    }

    // 親番号リストの履歴を削除する
    RF_oya_log_clear();
}

/**
 * 無線ルートをを初期化してEWRSR無線通信して再設定する(全グループ)
 * @bug 未完成
 * @note  auto_thread, cmd_thread
 */
void RF_full_moni(void)
{
	uint8_t group_size,group_cnt;	// 登録グループ数,グループ個数ループするカウンタ

	RfMonit_time = 0;				//  無線フルモニタリング用タイマー
    //searchscan_time = 0;            // サーチスキャン用タイマー
    memset(&searchscan_time[0],0x00,sizeof(searchscan_time));   // サーチスキャン用タイマー // sakaguchi 2021.01.21
    RemoteUnitRssiTable_Clear(&S_tUnitRssiTbl);     // 子機電波強度テーブルのクリア

	// 前回の自律リアルスキャン取得データはここでクリアする
	rf_ram.auto_format1.data_byte[0] = 0;		    // FORMAT1 データバイト数クリア
	rf_ram.auto_format1.data_byte[1] = 0;		    // FORMAT1 データバイト数クリア
	rf_ram.auto_format2.data_byte[0] = 0;		    // FORMAT2 データバイト数クリア
	rf_ram.auto_format2.data_byte[1] = 0;		    // FORMAT2 データバイト数クリア
	rf_ram.auto_format3.data_byte[0] = 0;		    // FORMAT3 データバイト数クリア
	rf_ram.auto_format3.data_byte[1] = 0;		    // FORMAT3 データバイト数クリア
	rf_ram.auto_rpt_info.data_byte[0] = 0;		    // 中継機  データバイト数クリア
	rf_ram.auto_rpt_info.data_byte[1] = 0;		    // 中継機  データバイト数クリア

	group_size = get_regist_group_size();			// 登録ファイルからグループ取得する関数

//	if((root_registry.auto_route & 0x01) == 0){		// 自動ルート機能OFFの場合
//		return;
//	}

	for(group_cnt = 0 ; group_cnt < group_size ; group_cnt++){
		RF_buff.rf_req.current_group = (char)(group_cnt + 1);		// 2020/09/08 追加		saka
		RF_full_moni_group((uint8_t)(group_cnt+1));
//TODO        moni_err[group_cnt] = realscan_err_data_add();      // 受信すべきなのに受信しなかった場合に、無線通信エラー子機のダミーデータを追加
//TODO	    realscan_data_para(1);
	}

    if((VENDER_HIT != fact_config.Vender) && (Http_Use == HTTP_SEND)){ 
    //if( VENDER_HIT != fact_config.Vender){
        if((root_registry.auto_route & 0x01) == 1){     // 自動ルートONの場合 // 2023.07.27
            G_HttpFile[FILE_I].sndflg = SND_ON;             // [FILE]機器状態送信：ON
        }
    }
}


/**
 * 無線ルートをを初期化してEWRSR無線通信して再設定する(指定グループ)
 * @param grp_no   グループ番号(1～)
 * @bug 未完成
 */
void RF_full_moni_group(uint8_t grp_no)
{
	int i;

	union{
		uint8_t byte[2];
		uint16_t word;
	}b_w_chg;

	uint8_t max_unit_no;
	uint8_t rtn;
//	int8_t rssiLimit = 25;						// 無線通信可能とするRSSIレベル(後で変更できるようにFLASH保存する事検討)
    RouteArray_t route;                         // 中継ルート
    uint8_t retry_cnt = 0;			            // 再送カウンタ

	uint64_t GMT_cal;
	uint64_t LAST_cal;

    Printf("##########################[Group%d] RF_full_moni start\n", grp_no);
	
    RF_Group[grp_no-1].moni_time = RTC_GetGMTSec();         // モニタリングした時刻を保存

	WirelessGroup_Clear(&RF_Group[grp_no-1]);	// 無線ルート情報の初期化	grp_noは1～指定になっている

    RF_Group[grp_no-1].allRemoteUnitRssiTable = &S_tUnitRssiTbl;    // 電波強度テーブルの指定


	max_unit_no = RF_get_regist_RegisterRemoteUnit(grp_no);	// 登録ファイルから無線ルート情報に子機番号を追加する
    RF_Group[grp_no-1].max_unit_no = max_unit_no;           // 最大子機番号格納

    max_rpt_no = RF_get_rpt_registry_full(grp_no);				// 登録されている全中継機のリスト作成
    RF_Group[grp_no-1].max_rpt_no = max_rpt_no;             // 最大中継機番号格納


	memset(realscan_order,0xFF, sizeof(realscan_order));
	realscan_order[0] = 0;						// やるべきテーブルに親機番号を追加
	realscan_cnt = 0;

	while(realscan_order[realscan_cnt] != 0xFF){	// やるべきテーブルに残りがあるか

		rf_ram.auto_format1.data_byte[0] = 0;		// FORMAT1 データバイト数クリア
		rf_ram.auto_format1.data_byte[1] = 0;		// FORMAT1 データバイト数クリア

		RF_buff.rf_req.cmd1 = 0x30;
		RF_buff.rf_req.cmd2 = 0x00;
		RF_buff.rf_req.data_size = 2;
		RF_buff.rf_req.data_format = 1;			// 2020/09/08 saka
		complement_info[0] = max_unit_no;
		complement_info[1] = 1;				// (FORMAT1)
		complement_info[2] = max_rpt_no;


        memset( &route, 0x00, sizeof(RouteArray_t) );
        WirelessGroup_GetRepeaterRoute(&RF_Group[grp_no-1],(int)realscan_order[realscan_cnt], &route);
		group_registry.altogether = route.count;

        memset( rpt_sequence, 0x00, sizeof(rpt_sequence) );
		memcpy( rpt_sequence, route.value, route.count );
        rpt_sequence_cnt = route.count;                 // ルート段数

        RF_buff.rf_res.time = 0;
        timer.int125 = 0;										// コマンド開始から子機が応答したときまでの秒数をカウント開始
        DL_start_utc = RTC_GetGMTSec();							// コマンドを発行する直前のGMTを保存する

		rtn = RF_command_execute(RF_buff.rf_req.cmd1);	// リアルスキャン無線通信をする
		RF_buff.rf_res.status = rtn;

        // 時間計算をLASTを参照するように修正 2020.07.07
        GMT_cal = RTC_GetGMTSec();					// 無線通信終了後のGMT取得
		GMT_cal = (GMT_cal * 1000) + 500;			// 500msec足して誤差を四捨五入	2020/08/28 segi
        LAST_cal = timer.int125 * 125uL;
        GMT_cal = GMT_cal - LAST_cal;

        if((GMT_cal % 1000) < 500){
            GMT_cal = GMT_cal / 1000;
        }
        else{
            GMT_cal = GMT_cal / 1000;
            GMT_cal++;
        }
        RF_buff.rf_res.time = (uint32_t)GMT_cal;	// コマンドを発行する直前のGMTを保存する

    	if(RF_buff.rf_res.status == RFM_NORMAL_END){					// 正常終了したらデータをGSM渡し変数にコピー	RF_command_execute の応答が入っている

            if(r500.node == COMPRESS_ALGORITHM_NOTICE)	// 圧縮データを解凍(huge_buffer先頭は0xe0)
            {
                //i = r500.data_size - *(char *)&huge_buffer[2];
                i = r500.data_size - *(uint16_t *)&huge_buffer[2];          // sakaguchi 2021.02.03
                
                memcpy(work_buffer, huge_buffer, r500.data_size);
                r500.data_size = (uint16_t)LZSS_Decode((uint8_t *)work_buffer, (uint8_t *)huge_buffer);     // 2022.06.09
                
                if(i > 0)								// 非圧縮の中継機データ情報があるとき
                {
                    memcpy((char *)&huge_buffer[r500.data_size], (char *)&work_buffer[*(uint16_t *)&work_buffer[2]], (size_t)i);
                }
                
                r500.data_size = (uint16_t)(r500.data_size + i);
                property.acquire_number = r500.data_size;
                property.transferred_number = 0;
                
                r500.node = 0xff;						// 解凍が重複しないようにする
            }

    //            Printf("##########################[Group%d] RF_full_moni scan[%d] hit[%d]\n", grp_no, realscan_cnt, realscan_order[realscan_cnt]);

            b_w_chg.byte[0] = huge_buffer[0];							// 無線通信結果メモリを自律処理用変数にコピーする
            b_w_chg.byte[1] = huge_buffer[1];							// 無線通信結果メモリを自律処理用変数にコピーする
            //b_w_chg.word = (uint16_t)(b_w_chg.word + 2);
            b_w_chg.word = (uint16_t)(b_w_chg.word + rpt_sequence_cnt*2);   // 個別中継機データ分を加算
            memcpy(rf_ram.realscan.data_byte,&huge_buffer[2],b_w_chg.word);	// MAX=(30byte×128)+2 = 3842

//TODO            copy_realscan(1);

			set_flag(&realscan_buff.over_rpt[0],realscan_order[realscan_cnt]);	// 今回リアルスキャンした親(中継機)番号はやり終わったフラグ立てる

			RF_table_make(grp_no, realscan_order[realscan_cnt], FULL_MONI);						// 受信結果を自動ルートに反映する,1:自動ルートフルスキャン結果を、やる順番テーブルに追加する

            RF_WirelessGroup_RefreshParentTable(&RF_Group[grp_no-1], root_registry.rssi_limit1, root_registry.rssi_limit2);     // 電波強度を元に無線ルートを更新
            realscan_cnt++;
            retry_cnt = 0;                                                            // 再送カウンタ
//            tx_thread_sleep(10);   // 2020 06 04
        }else{
            retry_cnt++;                        // リトライカウンタ更新
            Printf("##########################[Group%d] RF_full_moni scan[%d] retry[%d] status[%x]\n", grp_no, realscan_cnt, retry_cnt,RF_buff.rf_res.status);
        }

        if(retry_cnt >= 3){                     // ３回失敗
            // 今回のルートをスキャンリストから削除
			realscan_cnt++;
            retry_cnt = 0;
        }
        tx_thread_sleep (70);

	}
	
    // 履歴の親番号リストを更新
	RF_oya_log_make(grp_no);

    Printf("##########################[Group%d] RF_full_moni end status[%x]\n", grp_no,RF_buff.rf_res.status );

}







/**
 * EWRSRコマンドなどにより取得した子機、中継機情報(rf_ram.auto_***に格納済みの物)
 * を総なめして自動ルート作成用変数に保存する(保存する関数を呼ぶ)
 * @param grp_no        グループ番号(1～)
 * @param parent_no     親番号（ルートの末端）
 * @param sw       スキャン方式(FULL_MONI/REGU_MONI)
 * @bug 未完成
 */
void RF_table_make(uint8_t grp_no, uint8_t parent_no, uint8_t sw)
{
	uint16_t dataByte;	// 配列の全データバイト数
	uint8_t unit_size;	// 上記から子機(又は中継機)台数を求めた結果
	uint8_t rx_size;	// 中継機データに含まれる子機の台数
	uint8_t cnt;
	uint8_t rx_cnt;
	uint8_t koki_no;	// (中継機が受信した)子機番号
	uint8_t koki_rssi;	// (中継機が受信した)子機電波強度
	uint8_t koki_batt;	    // 子機電池残量
	uint8_t repeater_no;	// 中継機番号
	uint8_t repeater_rssi;	// 中継機電波強度
	uint8_t repeater_batt;	// 中継機電池残量

	uint8_t realscan_cnt_tmp;
	uint8_t *in_poi;
	uint8_t *rep_poi;       // 中継機の先頭ポインタ（一時格納）
    int iRet;

    ScanRelayStatus_t   relayStatus[REPEATER_MAX];
    memset(relayStatus,0x00,sizeof(relayStatus));

//	if(((root_registry.auto_route & 0x01) == 0)||(RF_buff.rf_req.cmd1 != 0x30)){	// 自動ルート機能OFFの場合
//		return;
//	}

	realscan_cnt_tmp = realscan_cnt;							// 自動ルートフルスキャンのやる順番カウンタの現在値取得

	dataByte = *(uint16_t *)rf_ram.realscan.data_byte;			// 子機情報のデータバイト数
	unit_size = (uint8_t)(dataByte / 30);

	in_poi = (uint8_t *)&rf_ram.realscan.data;

    // 子機の電波強度を更新する前に、指定した親番号に対する全子機の電波強度の値を無線通信不可に更新する
    for( cnt=1; cnt<=REMOTE_UNIT_MAX; cnt++ ){
        WirelessGroup_UpdateRemoteUnitRssi(&RF_Group[grp_no-1], parent_no, cnt, RSSI_NONE);
    }
    // 中継機の電波強度を更新する前に、指定した親番号に対する中継機の電波強度の値を無線通信不可に更新する
    for( cnt=0; cnt<=REPEATER_MAX; cnt++ ){
        WirelessGroup_UpdateRepeaterRssi(&RF_Group[grp_no-1], parent_no, cnt, RSSI_NONE);
    }

    for(cnt = 0 ; cnt < unit_size ; cnt++){
		koki_no = *in_poi++;						// [0]子機番号
		koki_rssi = *in_poi++;						// [1]子機RSSI
		iRet = WirelessGroup_UpdateRemoteUnitRssi(&RF_Group[grp_no-1], parent_no, koki_no, koki_rssi);    // 子機の電波強度更新
    	if(0 == iRet){
//        	set_flag(realscan_buff.do_unit,koki_no);
        	//set_flag(realscan_buff.over_unit,koki_no);													// 2020.10.27 segi コメントアウト
//            Printf("########################## RF_table_make koki[%d],koki-rssi[%d]\n",koki_no, koki_rssi);
        }
        in_poi += 2;
        koki_batt = *in_poi;                                                        // 子機電池残量（0～15）
        RF_Group[grp_no-1].unit_battery[koki_no] = ExLevelBatt(koki_batt & 0x0F);   // 子機の電池残量更新

        in_poi += 26;
	}

	in_poi++;				// 子機情報の次には中継機情報のバイト数が入っている
	dataByte = *in_poi;		// 子機情報の次には中継機情報のバイト数が入っている[Hバイト取得]
	dataByte = (uint16_t)(dataByte * 256);		// 子機情報の次には中継機情報のバイト数が入っている[Hバイト256倍]
	in_poi--;				// 子機情報の次には中継機情報のバイト数が入っている[Lバイト加算]
	dataByte = (uint16_t)(dataByte + *in_poi);	// 子機情報の次には中継機情報のバイト数が入っている
	in_poi += 2;			// ポインタは中継機情報の先頭に移動
    unit_size = (uint8_t)(dataByte / 30);

    rep_poi = in_poi;       // 中継機情報の先頭ポインタを一時格納

	for(cnt = 0 ; cnt < unit_size ; cnt++){

        in_poi = rep_poi+30*cnt;                        // 中継機情報の先頭ポインタを台数分カウントアップ

		repeater_no = *in_poi++;						// [0]中継機番号
		repeater_rssi = *in_poi++;						// [1]中継機RSSI

        iRet = WirelessGroup_UpdateRepeaterRssi(&RF_Group[grp_no-1], parent_no, repeater_no, repeater_rssi);    //中継機の電波強度更新
//        Printf("########################## RF_table_make repeater[%d],repeater-rssi[%d]\n",repeater_no, repeater_rssi);

		if(sw == FULL_MONI){
			if(check_flag(&realscan_buff.do_rpt[0],repeater_no) != 0){			// 登録パラメータにある中継機番号である事
				if(check_flag(&realscan_buff.over_rpt[0],repeater_no) == 0){	// すでに無線通信済みの中継機番号ではない事
					realscan_cnt_tmp++;
					realscan_order[realscan_cnt_tmp] = repeater_no;				// やるべきテーブルに追加する。
//            		Printf("########################## RF_table_make realscan_order[%d]\n",realscan_order[realscan_cnt_tmp]);
				}
			}
		}else{
            if(0 == iRet){
                //set_flag(realscan_buff.over_rpt,repeater_no);													// 2020.10.27 segi コメントアウト
            }
		}

        in_poi += 2;
        repeater_batt = *in_poi;                                                        // 中継機電池残量（0:なし～5:FULL）
        RF_Group[grp_no-1].rpt_battery[repeater_no] = repeater_batt;                    // 中継機の電池残量更新
//        RF_Group[grp_no-1].rpt_battery[repeater_no] = BAT_level_0to5(repeater_batt);    // 中継機の電池残量更新

		in_poi++;					    // ポインタは子機台数の先頭に移動
		rx_size = *in_poi++;			// 受信した子機台数

        if(rx_size > 0){
		if(rx_size > 12)rx_size = 12;	// 変な子機台数でもリミッタ

		for(rx_cnt = 0 ; rx_cnt < rx_size ; rx_cnt++){
			koki_no = *in_poi++;		// (中継機が受信した)子機番号
			koki_rssi = *in_poi++;		// (中継機が受信した)子機電波強度

                iRet = WirelessGroup_UpdateRemoteUnitRssi(&RF_Group[grp_no-1], repeater_no, koki_no, koki_rssi);  // （中継機が受信した）子機の電波強度更新
                if(0 == iRet){
//                    set_flag(realscan_buff.do_unit,koki_no);
                    //set_flag(realscan_buff.over_unit,koki_no);                 // 2CH機の片方のCHで無線通信エラーが発生してもこの処理で無線通信成功となってしまうため削除 sakaguchi 2021.04.07
//                    Printf("########################## RF_table_make koki2[%d],koki-rssi[%d]\n",koki_no, koki_rssi);
                }
            }
        }

	}

    // 個別中継機データのRSSIで中継機の電波強度更新
    in_poi = rep_poi+30*unit_size;             // 中継機情報の先頭ポインタから台数分カウントアップし、個別中継機データに移動
    if(rpt_sequence_cnt>1){
        for(cnt=0; cnt<rpt_sequence_cnt; cnt++){
            relayStatus[cnt].repno = rpt_sequence[rpt_sequence_cnt-1-cnt];      // ルートの末端から格納
            relayStatus[cnt].rssi = *in_poi++;
            relayStatus[cnt].batteryLevel = *in_poi++;
        }

        for(cnt=0; cnt<rpt_sequence_cnt-1; cnt++){          // 親機のエリアは除外する(cnt-1)
            WirelessGroup_UpdateRepeaterRssi(&RF_Group[grp_no-1], relayStatus[cnt+1].repno, relayStatus[cnt].repno, relayStatus[cnt].rssi);
            repeater_no = relayStatus[cnt].repno;
            RF_Group[grp_no-1].rpt_battery[repeater_no] = relayStatus[cnt].batteryLevel;
        }
    }

}

/**
 * 親番号リストの履歴で最後に通信に成功した親機番号のみ更新する
 * @param grp_no   グループ番号(1～)
 */
void RF_oya_log_last(uint8_t grp_no)
{
    uint8_t ucRepNo;                 // 親機中継機番号
    uint8_t ucUnitNo;                // 子機番号

//    memcpy(&RF_OYALOG_Group[grp_no-1][5].repeaterParents, &RF_Group[grp_no-1].repeaterParents, sizeof(RepeaterParentArray_t));			            // 中継機0（今回）
//    memcpy(&RF_OYALOG_Group[grp_no-1][5].remoteUnitParents, &RF_Group[grp_no-1].remoteUnitParents, sizeof(RemoteUnitParentArray_t));                // 子機0（今回）

    for( ucRepNo=0; ucRepNo<=REPEATER_MAX; ucRepNo++ ){
        if( 0 <= RF_Group[grp_no-1].repeaterParents.value[ucRepNo]){
            RF_OYALOG_Group[grp_no-1][5].repeaterParents.value[ucRepNo] = RF_Group[grp_no-1].repeaterParents.value[ucRepNo];
        }
    }

    for( ucUnitNo=0; ucUnitNo<=REMOTE_UNIT_MAX; ucUnitNo++ ){
        if( 0 <= RF_Group[grp_no-1].remoteUnitParents.value[ucUnitNo]){
            RF_OYALOG_Group[grp_no-1][5].remoteUnitParents.value[ucUnitNo] = RF_Group[grp_no-1].remoteUnitParents.value[ucUnitNo];
        }
    }
}


/**
 * 自動作成した親番号リストを履歴保存変数にコピーする
 * @param grp_no   グループ番号(1～)
 * @bug 未完成
 */
void RF_oya_log_make(uint8_t grp_no)
{
    uint8_t ucRepNo;                 // 親機中継機番号
    uint8_t ucUnitNo;                // 子機番号
    uint8_t uclogNo;                 // 履歴番号

    // 中継機の親番号リストの履歴を更新
	memcpy(&RF_OYALOG_Group[grp_no-1][4].repeaterParents, &RF_OYALOG_Group[grp_no-1][3].repeaterParents, sizeof(RepeaterParentArray_t));			// 中継機3->4
	memcpy(&RF_OYALOG_Group[grp_no-1][3].repeaterParents, &RF_OYALOG_Group[grp_no-1][2].repeaterParents, sizeof(RepeaterParentArray_t));			// 中継機2->3
	memcpy(&RF_OYALOG_Group[grp_no-1][2].repeaterParents, &RF_OYALOG_Group[grp_no-1][1].repeaterParents, sizeof(RepeaterParentArray_t));			// 中継機1->2
	memcpy(&RF_OYALOG_Group[grp_no-1][1].repeaterParents, &RF_OYALOG_Group[grp_no-1][0].repeaterParents, sizeof(RepeaterParentArray_t));			// 中継機0->1
    memcpy(&RF_OYALOG_Group[grp_no-1][0].repeaterParents, &RF_Group[grp_no-1].repeaterParents, sizeof(RepeaterParentArray_t));			            // 中継機0（今回）

    // 子機の親番号リストの履歴を更新
	memcpy(&RF_OYALOG_Group[grp_no-1][4].remoteUnitParents, &RF_OYALOG_Group[grp_no-1][3].remoteUnitParents, sizeof(RemoteUnitParentArray_t));	    // 子機3->4
	memcpy(&RF_OYALOG_Group[grp_no-1][3].remoteUnitParents, &RF_OYALOG_Group[grp_no-1][2].remoteUnitParents, sizeof(RemoteUnitParentArray_t));	    // 子機2->3
	memcpy(&RF_OYALOG_Group[grp_no-1][2].remoteUnitParents, &RF_OYALOG_Group[grp_no-1][1].remoteUnitParents, sizeof(RemoteUnitParentArray_t));	    // 子機1->2
	memcpy(&RF_OYALOG_Group[grp_no-1][1].remoteUnitParents, &RF_OYALOG_Group[grp_no-1][0].remoteUnitParents, sizeof(RemoteUnitParentArray_t));	    // 子機0->1
    memcpy(&RF_OYALOG_Group[grp_no-1][0].remoteUnitParents, &RF_Group[grp_no-1].remoteUnitParents, sizeof(RemoteUnitParentArray_t));                // 子機0（今回）

    // 中継機の最後に通信できた親番号を更新
    for( ucRepNo=0; ucRepNo<=REPEATER_MAX; ucRepNo++ ){
        RF_OYALOG_Group[grp_no-1][5].repeaterParents.value[ucRepNo] = (-1);       // 一旦クリア
        for( uclogNo=0; uclogNo<=4; uclogNo++ ){
            if( 0 <= RF_OYALOG_Group[grp_no-1][uclogNo].repeaterParents.value[ucRepNo]){
                RF_OYALOG_Group[grp_no-1][5].repeaterParents.value[ucRepNo] = RF_OYALOG_Group[grp_no-1][uclogNo].repeaterParents.value[ucRepNo];
//                Printf("########################## RF_oya_log_make last[oya][%d] [RepNo][%d]\n",RF_OYALOG_Group[grp_no-1][5].repeaterParents.value[ucRepNo],ucRepNo);
                break;
            }
        }
    }

    // 子機の最後に通信できた親番号を更新
    for( ucUnitNo=0; ucUnitNo<=REMOTE_UNIT_MAX; ucUnitNo++ ){
        RF_OYALOG_Group[grp_no-1][5].remoteUnitParents.value[ucUnitNo] = (-1);    // 一旦クリア
        for( uclogNo=0; uclogNo<=4; uclogNo++ ){
            if( 0 <= RF_OYALOG_Group[grp_no-1][uclogNo].remoteUnitParents.value[ucUnitNo]){
                RF_OYALOG_Group[grp_no-1][5].remoteUnitParents.value[ucUnitNo] = RF_OYALOG_Group[grp_no-1][uclogNo].remoteUnitParents.value[ucUnitNo];
//                Printf("########################## RF_oya_log_make last[koki][%d] [ucUnitNo][%d]\n",RF_OYALOG_Group[grp_no-1][5].remoteUnitParents.value[ucUnitNo],ucUnitNo);
                break;
            }
        }
    }
}





/// @brief  親番号リストの履歴を削除する
void RF_oya_log_clear(void)
{
    uint8_t ucgrp;                   // グループ
    uint8_t uclogNo;                 // 履歴番号

    for( ucgrp=0; ucgrp<=3; ucgrp++ ){
        for( uclogNo=0; uclogNo<=5; uclogNo++ ){
            memset(&RF_OYALOG_Group[ucgrp][uclogNo].repeaterParents, -1, sizeof(RepeaterParentArray_t) );       // 中継機の親番号リスト
            memset(&RF_OYALOG_Group[ucgrp][uclogNo].remoteUnitParents, -1, sizeof(RemoteUnitParentArray_t) );   // 子機の親番号リスト
        }
    }
}


/// @brief  中継機の無線ルートを取得します。
/// @param [in] grp_no           無線グループ
/// @param [in] repeaterNum      中継機番号（親機を指定するときは0）
/// @param [out] route           無線ルート
/// @return 無線ルートの段数を返します。無線通信できない中継機のときは0を返します。
///         失敗すると負の値を返します。
int RF_GetRepeaterRoute(uint8_t grp_no, int repeaterNum, RouteArray_t *route)
{
    return(WirelessGroup_GetRepeaterRoute(&RF_Group[grp_no-1], repeaterNum, route));
}


/// @brief  子機の無線ルートを取得します。
/// @param [in] grp_no              無線グループ
/// @param [in] remoteUnitNum      中継機番号（親機を指定するときは0）
/// @param [out] route           無線ルート
/// @return 無線ルートの段数を返します。無線通信できない中継機のときは0を返します。
///         失敗すると負の値を返します。
int RF_GetRemoteRoute(uint8_t grp_no, int remoteUnitNum, RouteArray_t *route)
{
    return(WirelessGroup_GetRemoteRoute(&RF_Group[grp_no-1], remoteUnitNum, route));
}



/// @brief  レギュラーモニタリングの初期化処理
void RF_regu_moni_init(void){

    RemoteUnitRssiTable_Clear(&S_tUnitRssiTbl);     // 子機電波強度テーブルクリア
}

/// @brief  必須スキャンリストの作成（全グループ）
void RF_regu_Make_Required_ScanList(void){

    uint8_t grp_size,grp_no;

    grp_size = get_regist_group_size();		// 登録ファイルからグループ取得する関数

    for(grp_no = 1 ; grp_no <= grp_size ; grp_no++){
        Make_Required_ScanList(grp_no, &scan_route_list_all[grp_no-1][0]);   	// 必須スキャンリストの作成
    }
}


/// @brief  レギュラーモニタリングを実行する（全グループ）
/// @note   呼び出し元   cmd_thread  CFStartMonitor()
void RF_regu_moni(void){

	uint8_t group_size,group_cnt;	                // 登録グループ数,グループ個数ループするカウンタ

	// 前回の自律リアルスキャン取得データはここでクリアする
	rf_ram.auto_format1.data_byte[0] = 0;		    // FORMAT1 データバイト数クリア
	rf_ram.auto_format1.data_byte[1] = 0;		    // FORMAT1 データバイト数クリア
	rf_ram.auto_format2.data_byte[0] = 0;		    // FORMAT2 データバイト数クリア
	rf_ram.auto_format2.data_byte[1] = 0;		    // FORMAT2 データバイト数クリア
	rf_ram.auto_format3.data_byte[0] = 0;		    // FORMAT3 データバイト数クリア
	rf_ram.auto_format3.data_byte[1] = 0;		    // FORMAT3 データバイト数クリア
	rf_ram.auto_rpt_info.data_byte[0] = 0;		    // 中継機  データバイト数クリア
	rf_ram.auto_rpt_info.data_byte[1] = 0;		    // 中継機  データバイト数クリア

    group_size = get_regist_group_size();			// 登録ファイルからグループ取得する関数

	if(1 == (root_registry.auto_route & 0x01)){		// 自動ルート機能ONの場合

        RF_regu_Make_Required_ScanList();			        // 必須スキャンリストの作成
        RF_regu_moni_init();                                // レギュラーモニタリングの初期化処理
    }

    for(group_cnt = 0 ; group_cnt < group_size ; group_cnt++){

        moni_err[group_cnt] = 0;									// 無線通信エラー子機なければ0

        RF_buff.rf_req.current_group = (char)(group_cnt + 1);		// 通信対象グループ番号を０～指定

        // ##### 初回時のリアルスキャンフラグ設定 #####
        get_regist_scan_unit((uint8_t)(group_cnt+1), 0x03);		    // 警報監視 | モニタリング

        if(1 == (root_registry.auto_route & 0x01)){		            // 自動ルート機能ONの場合
            RF_regu_moni_group((uint8_t)(group_cnt+1), 1);          // レギュラーモニタリング（自動）
        }else{
            RF_regu_moni_group_manual((uint8_t)(group_cnt+1), 1);   // レギュラーモニタリング（手動）
        }

//TODO        moni_err[group_cnt] = realscan_err_data_add();		// 受信すべきなのに受信しなかった場合に、無線通信エラー子機のダミーデータを追加

//TODO        realscan_data_para(1);				// xxx25 2009.08.25追加 リアルスキャンデータに(自律用)子機設定フラグを追記する
    }

    if((VENDER_HIT != fact_config.Vender) && (Http_Use == HTTP_SEND)){ 
    //if(VENDER_HIT != fact_config.Vender){
		if((root_registry.auto_route & 0x01) == 1){     // 自動ルートONの場合 // 2023.07.27
            G_HttpFile[FILE_I].sndflg = SND_ON;             // [FILE]機器状態送信：ON
        }
    }
}

/// @brief  レギュラーモニタリングを実行する（手動）
/// @param[in] grp_no           無線グループ    未使用
/// @param[in] data_format      送信フォーマット    未使用
/// @attention  grp_no, data_format未使用
void RF_regu_moni_group_manual(uint8_t grp_no, uint8_t data_format){

    (void)(grp_no);
    (void)(data_format);

    int i;

	uint8_t rtn = 0;
	uint8_t retry_cnt;
	uint8_t MaxRealscan;
/*	union{
		uint8_t byte[2];
		uint16_t word;
	}b_w_chg;
*/
	uint16_t word_data;
	uint8_t max_unit_work;		// --------------------------------------- リアルスキャンするMAX子機を保存しておく	2014.10.14追加

    max_unit_work = group_registry.max_unit_no;		// --------------------------------------- リアルスキャンするMAX子機を保存しておく	2014.10.14追加

    retry_cnt = 2;		// リトライ回数
    do{
        MaxRealscan = (uint8_t)(group_registry.altogether + 1);	// 最悪時のリアルスキャン回数(中継機数+1(親機))
        while(MaxRealscan){
//TODO            rtn = auto_realscan_New(1);			// この中で無線通信を行っている取得データはＦＯＲＭＡＴ１
            if(rtn != AUTO_END){

                if(RF_buff.rf_res.status == RFM_NORMAL_END){					// 正常終了したらデータをGSM渡し変数にコピー	RF_command_execute の応答が入っている
                    if(r500.node == COMPRESS_ALGORITHM_NOTICE)	// 圧縮データを解凍(huge_buffer先頭は0xe0)
                    {
                        //i = r500.data_size - *(char *)&huge_buffer[2];
                        i = r500.data_size - *(uint16_t *)&huge_buffer[2];              // sakaguchi 2021.02.03
                        Printf("RF_regu_moni_group_manual  %04X(%d)  %02X %02X %02X %02X   \r\n",  r500.data_size, i, huge_buffer[0], huge_buffer[1],huge_buffer[2],huge_buffer[3]);

                        memcpy(work_buffer, huge_buffer, r500.data_size);
                        r500.data_size = (uint16_t)LZSS_Decode((uint8_t *)work_buffer, (uint8_t *)huge_buffer);     // 2022.06.09

                        if(i > 0)								// 非圧縮の中継機データ情報があるとき
                        {
                            memcpy((char *)&huge_buffer[r500.data_size], (char *)&work_buffer[*(uint16_t *)&work_buffer[2]], (size_t)i);
                        }

                        r500.data_size = (uint16_t)(r500.data_size + i);
                        property.acquire_number = r500.data_size;
                        property.transferred_number = 0;

                        r500.node = 0xff;						// 解凍が重複しないようにする
                    }
/*
                    b_w_chg.byte[0] = huge_buffer[0];							// 無線通信結果メモリを自律処理用変数にコピーする
                    b_w_chg.byte[1] = huge_buffer[1];							// 無線通信結果メモリを自律処理用変数にコピーする
                    b_w_chg.word = (uint16_t)(b_w_chg.word + 2);
*/
                    word_data = (uint16_t)(*(uint16_t *)&huge_buffer[0] + 2);
                    if(RF_buff.rf_req.cmd1 == 0x68){
//                        memcpy(rf_ram.realscan.data_byte,huge_buffer,b_w_chg.word);		// MAX=(30byte×128)+2 = 3842
                        memcpy(rf_ram.realscan.data_byte,huge_buffer, word_data);     // MAX=(30byte×128)+2 = 3842
                    }
                    else{
//                        memcpy(rf_ram.realscan.data_byte,&huge_buffer[2],b_w_chg.word);	// MAX=(30byte×128)+2 = 3842
                        memcpy(rf_ram.realscan.data_byte,&huge_buffer[2], word_data); // MAX=(30byte×128)+2 = 3842
                    }
//TODO                    copy_realscan(1);

                    group_registry.max_unit_no = max_unit_work;	// copy_realscan(1)でMAXを0にしてしまっていたmax_unit_work	2016.03.23修正
                }
            }
            else{
                break;
            }
            MaxRealscan--;
        }

//TODO        get_regist_scan_unit_retry();
        group_registry.max_unit_no = max_unit_work;
        tx_thread_sleep (70);
    }while(retry_cnt--);

}




/// @brief  レギュラーモニタリングを実行する
/// @param[in] grp_no           無線グループ
/// @param[in] data_format      送信フォーマット
/// @return 無し
/// @note   auto_thread
void RF_regu_moni_group(uint8_t grp_no, uint8_t data_format){

    uint8_t retry_cnt = 0;			    // 再送カウンタ

	RouteArray_t tmp_route;         	// 中継ルート
	uint8_t tmp_parent;					// 親番号（ルートの末端）
    char    status;                     // 処理結果

	uint8_t max_unit_no;

    RF_Group[grp_no-1].moni_time = RTC_GetGMTSec();        // モニタリングした時刻を保存

    //----------------------------------
    //---   １巡目の必須スキャン       ---
    //----------------------------------
	max_rpt_no = RF_get_rpt_registry_full(grp_no);				// 登録されている全中継機のリスト作成
    RF_Group[grp_no-1].max_rpt_no = max_rpt_no;             // 最大中継機番号格納

//	Make_Required_ScanList(grp_no, &scan_route_list[0]);   	// 必須スキャンリストの作成
    memcpy(&scan_route_list[0], &scan_route_list_all[grp_no-1][0], sizeof(scan_route_list));

    WirelessGroup_Clear(&RF_Group[grp_no-1]);       		// このグループの無線ルート情報を初期化
	
    max_unit_no = RF_get_regist_RegisterRemoteUnit(grp_no);    // 登録されている子機の電波強度テーブル関連付け
    RF_Group[grp_no-1].max_unit_no = max_unit_no;           // 最大子機番号格納


    retry_cnt = 0;
	Printf("##########################[Group%d] regu moni step1\n", grp_no);

    while(1){
        // 今回のスキャンルートを取得
        memcpy( &tmp_route, &scan_route_list[0], sizeof(RouteArray_t));

        // ルートが存在しない場合は処理終了
        if( 0 == tmp_route.count ){
            Printf("##########################[Group%d] regu moni step1 end\n", grp_no);
            break;
        }

        // 全ての子機がスキャン済みの場合は処理終了
        if( false == check_scan_notyet(grp_no, REGU_MONI, 1)){
            Printf("##########################[Group%d] regu moni step1 end2\n", grp_no);
            break;
        }

        tmp_parent = tmp_route.value[tmp_route.count-1];            // 親番号
        group_registry.altogether = tmp_route.count;                // ルート段数
        memset( rpt_sequence, 0x00, sizeof(rpt_sequence) );
        memcpy( rpt_sequence, tmp_route.value, tmp_route.count );   // ルート情報の格納
        rpt_sequence_cnt = tmp_route.count;                         // ルート段数

        // スキャン実行
        status = regu_moni_scan(data_format);

        if(status == RFM_NORMAL_END){           // 正常時

            // 無線通信の結果で電波強度を更新
            RF_table_make(grp_no, tmp_parent, REGU_MONI);

            // 無線ルート情報更新
            RF_WirelessGroup_RefreshParentTable(&RF_Group[grp_no-1], root_registry.rssi_limit1, root_registry.rssi_limit2);

            // 今回のルートをスキャンリストから削除
            scan_route_delete( &scan_route_list[0], tmp_route );
            retry_cnt = 0;

        }else{                                  // 異常時
            retry_cnt++;                        // リトライカウンタ更新
        }

        if(retry_cnt >= 3){                     // ３回失敗
            // 今回のルートをスキャンリストから削除
            scan_route_delete( &scan_route_list[0], tmp_route );
            retry_cnt = 0;
        }
        tx_thread_sleep (70);		// 2010.08.03 リトライ前のWaitをDelayに変更	xxx03
    }


    //----------------------------------
    //---   １巡目の実績スキャン       ---
    //----------------------------------
	Printf("##########################[Group%d] regu moni step1-2\n", grp_no);

	// 実績スキャンリストの作成
	Make_Performance_ScanList(grp_no, &scan_route_list[0]);

	// 既にスキャンしたルートは省く
    scan_route_adjust(&scan_route_list[0]);

    retry_cnt = 0;
	while(1){
		// 今回のスキャンルートを取得
		memcpy( &tmp_route, &scan_route_list[0], sizeof(RouteArray_t));

		// ルートが存在しない もしくは 全ルートスキャン済みの場合は処理終了
		if( 0 == tmp_route.count ){
            Printf("##########################[Group%d] regu moni step1-2 end\n", grp_no);
			break;
		}

        // 全ての子機がスキャン済みの場合は処理終了
		if( false == check_scan_notyet(grp_no, REGU_MONI, 1)){
            Printf("##########################[Group%d] regu moni step1-2 end2\n", grp_no);
			break;
		}

		tmp_parent = tmp_route.value[tmp_route.count-1];            // 親番号
        group_registry.altogether = tmp_route.count;                // ルート段数
        memset( rpt_sequence, 0x00, sizeof(rpt_sequence) );
		memcpy( rpt_sequence, tmp_route.value, tmp_route.count );   // ルート情報の格納
        rpt_sequence_cnt = tmp_route.count;                         // ルート段数

        // スキャン実行
		status = regu_moni_scan(data_format);

        if(status == RFM_NORMAL_END){           // 正常時

            // 無線通信の結果で電波強度を更新
            RF_table_make(grp_no, tmp_parent, REGU_MONI);

            // 無線ルート情報更新
            RF_WirelessGroup_RefreshParentTable(&RF_Group[grp_no-1], root_registry.rssi_limit1, root_registry.rssi_limit2);

            // 今回のルートをスキャンリストから削除
            scan_route_delete( &scan_route_list[0], tmp_route );
            retry_cnt = 0; 

        }else{                                  // 異常時
            retry_cnt++;                        // リトライカウンタ更新
        }

        if(retry_cnt >= 3){                     // ３回失敗
            // 今回のルートをスキャンリストから削除
            scan_route_delete( &scan_route_list[0], tmp_route );
            retry_cnt = 0;
        }
        tx_thread_sleep (70);		// 2010.08.03 リトライ前のWaitをDelayに変更	xxx03
    }


    //----------------------------------
    //---   ２巡目の実績スキャン       ---
    //----------------------------------
	// スキャンが完了していない子機がある？
	if( true == check_scan_notyet(grp_no, REGU_MONI, 1)){

		Printf("##########################[Group%d] regu moni step2\n", grp_no);

		// 実績スキャンリストの作成
		Make_Performance_ScanList(grp_no, &scan_route_list[0]);

        retry_cnt = 0;
		while(1){
			// 今回のスキャンルートを取得
			memcpy( &tmp_route, &scan_route_list[0], sizeof(RouteArray_t));

			// ルートが存在しない もしくは 全ルートスキャン済みの場合は処理終了
			if( 0 == tmp_route.count ){
        		Printf("##########################[Group%d] regu moni step2 end\n", grp_no);
				break;
			}

			// 全ての子機がスキャン済み
			if( false == check_scan_notyet(grp_no, REGU_MONI, 1)){
        		Printf("##########################[Group%d] regu moni step2 end2\n", grp_no);
				break;
			}

			tmp_parent = tmp_route.value[tmp_route.count-1];            // 親番号
            group_registry.altogether = tmp_route.count;                // ルート段数
            memset( rpt_sequence, 0x00, sizeof(rpt_sequence) );
			memcpy( rpt_sequence, tmp_route.value, tmp_route.count );   // ルート情報の格納
            rpt_sequence_cnt = tmp_route.count;                         // ルート段数

            // スキャン実行
			status = regu_moni_scan(data_format);

            if(status == RFM_NORMAL_END){            // 正常時

                // 無線通信の結果で電波強度を更新
                RF_table_make(grp_no, tmp_parent, REGU_MONI);

                // 無線ルート情報更新
                RF_WirelessGroup_RefreshParentTable(&RF_Group[grp_no-1], root_registry.rssi_limit1, root_registry.rssi_limit2);

                // 今回のルートをスキャンリストから削除
                scan_route_delete( &scan_route_list[0], tmp_route );
                retry_cnt = 0;

            }else{                                  // 異常時
                retry_cnt++;                        // リトライカウンタ更新
            }

            if(retry_cnt >= 3){                     // ３回失敗
                // 今回のルートをスキャンリストから削除
                scan_route_delete( &scan_route_list[0], tmp_route );
                retry_cnt = 0;
            }
            tx_thread_sleep (70);		// 2010.08.03 リトライ前のWaitをDelayに変更	xxx03
        }
	}


    //----------------------------------
    //---   ３巡目のサーチスキャン     ---
    //----------------------------------
	// スキャンが完了していない子機がある？
	if( true == check_scan_notyet(grp_no, REGU_MONI, 0)){
	
        // サーチスキャンの開始判定
        if( true == Search_Scan_Start_Check(grp_no) ){

    	    Printf("##########################[Group%d] regu moni step3\n", grp_no);

            // サーチスキャンリストの作成
            Make_Search_ScanList( grp_no, &scan_route_list[0] );

            retry_cnt = 0;
            while(1){
                // 今回のスキャンルートを取得
                memcpy( &tmp_route, &scan_route_list[0], sizeof(RouteArray_t));

                // ルートが存在しない もしくは 全ルートスキャン済みの場合は処理終了
                if( 0 == tmp_route.count ){
                    Printf("##########################[Group%d] regu moni step3 end3\n", grp_no);
                    break;
                }

                // 全ての子機がスキャン済み
                if( false == check_scan_notyet(grp_no, REGU_MONI, 0)){
                    Printf("##########################[Group%d] regu moni step3 end4\n", grp_no);
                    break;
                }

                tmp_parent = tmp_route.value[tmp_route.count-1];            // 親番号
                group_registry.altogether = tmp_route.count;                // ルート段数
                memset( rpt_sequence, 0x00, sizeof(rpt_sequence) );
                memcpy( rpt_sequence, tmp_route.value, tmp_route.count );   // ルート情報の格納
                rpt_sequence_cnt = tmp_route.count;                         // ルート段数

                // スキャン実行
                status = regu_moni_scan(data_format);

                if(status == RFM_NORMAL_END){                    // 成功時

                    // 無線通信の結果で電波強度を更新
                    RF_table_make(grp_no, tmp_parent, REGU_MONI);

                    // 無線ルート情報更新
                    RF_WirelessGroup_RefreshParentTable(&RF_Group[grp_no-1], root_registry.rssi_limit1, root_registry.rssi_limit2);

                    // 今回のルートをスキャンリストから削除
                    scan_route_delete( &scan_route_list[0], tmp_route );

                    // サーチスキャンリストの作成
                    Make_Search_ScanList( grp_no, &scan_route_list[0] );
                    retry_cnt = 0;

                }else{                                          // 異常時
                    retry_cnt++;                                // リトライカウンタ更新
                }

                if(retry_cnt >= 3){
                    // 今回のルートをスキャンリストから削除
                    scan_route_delete( &scan_route_list[0], tmp_route );
                    // サーチスキャンリストの作成
                    // Make_Search_ScanList( grp_no, &scan_route_list[0] );   // sakaguchi del 2020.11.26
                    retry_cnt = 0;                                         // 正常時はリトライカウンタをクリア
                }
                tx_thread_sleep (70);		// 2010.08.03 リトライ前のWaitをDelayに変更	xxx03

            }
        }else{
            Printf("##########################[Group%d] regu moni step3 end2\n", grp_no);
        }
    }else{
        Printf("##########################[Group%d] regu moni step3 end\n", grp_no);
    }

    // 履歴の親番号リストを更新
    RF_oya_log_make(grp_no);
    Printf("##########################[Group%d] regu moni end\n", grp_no);
}


/// @brief  必須スキャンのルートリスト作成
/// @param  grp_no           無線グループ
/// @param  scan_route      スキャンルート
/// @return 無し
/// 前回の無線ルート情報から必須親機中継機のスキャンリストを作成する
/// ある子機が通信できる親機が１台の場合、その親機中継機を必須とする
void Make_Required_ScanList(uint8_t grp_no, RouteArray_t *scan_route )
{

    uint8_t ucUnitNo;               // 子機番号
    uint8_t ucRepNo;                // 親機中継機番号
    uint8_t rssi;                   // 電波強度
    uint8_t lastRepNo;              // 最後に検索した親機中継機番号
    int repCnt = 0;                 // 親機中継機検索数
    RouteArray_t route;             // 中継ルート

    // スキャンリストの初期化
    memset(scan_route, 0x00, sizeof(RouteArray_t)*SCANLIST_MAX);
    memset(&route, 0x00, sizeof(RouteArray_t));

    // グループに登録されている子機すべてについて
	// その子機が通信できる親が1つだけがを無線ルート情報から調べる。
	// その子機の親が1つならば、スキャン用リストに追加していく。
    for( ucUnitNo = 1; ucUnitNo <= REMOTE_UNIT_MAX; ucUnitNo++ ){

//        // 通信すべき子機リストに存在していない
//        if( 0 == check_flag(&realscan_buff.do_unit[0], ucUnitNo) ){
//            continue;
//        }

        // 親番号リストの履歴から「最後に通信できた親機」が初期値-1の機器はスキャンしない
        if( 0 > RF_OYALOG_Group[grp_no-1][5].remoteUnitParents.value[ucUnitNo] ){
            continue;
        }

        // 子機の電波強度テーブルの値は無効？
        if( 0 != WirelessGroup_GetRemoteUnitRssi(&RF_Group[grp_no-1], 0, ucUnitNo, &rssi) ){
            continue;
        }

        // 子機が通信できる親は1台だけか？
        repCnt = 0;
        for( ucRepNo = 0; ucRepNo < REPEATER_MAX+1; ucRepNo++ ){

            WirelessGroup_GetRemoteUnitRssi(&RF_Group[grp_no-1], ucRepNo, ucUnitNo, &rssi);
//            if( rssi != RSSI_VOID ){
            if(( rssi != RSSI_VOID ) && ( rssi != RSSI_NONE )){
                repCnt++;
                lastRepNo = ucRepNo;        // 検索にヒットした親機中継機
                if(repCnt > 1)
                    break;
            }
        }

        // 親機は１台？
        if(repCnt == 1)
        {
            // この子機の中継ルートを取得する
//            WirelessGroup_GetRepeaterRoute( &RF_Group[grp_no-1], lastRepNo, &route );
            GetRoot_from_ResultTable(grp_no, 1, ucUnitNo, &route);
            if(route.count != 0){
            set_flag(&realscan_buff.do_rpt[0],(uint8_t)lastRepNo);    // スキャン対象の親機中継機をセット
                // スキャンリストに追加
                scan_route_add( scan_route, route );
            }
        }
    }
}


/// @brief  実績スキャンのルートリスト作成
/// @param  grp_no           無線グループ
/// @param  scan_route     スキャンルート
/// 前回の通信実績からスキャンリストを作成する
static void Make_Performance_ScanList(uint8_t grp_no, RouteArray_t *scan_route)
{

    uint8_t ucUnitNo;               // 子機番号
    RouteArray_t route;             // 中継ルート

    // スキャンリストの初期化
    memset(scan_route, 0x00, sizeof(RouteArray_t)*SCANLIST_MAX);
    memset(&route, 0x00, sizeof(RouteArray_t));

    // まだスキャンができていない子機を探して、ルートを作成する。
    for(ucUnitNo = 1; ucUnitNo <= REMOTE_UNIT_MAX; ucUnitNo++)
    {
        // 親番号リストの履歴から「最後に通信できた親機」が初期値-1の機器はスキャンしない
        if( 0 > RF_OYALOG_Group[grp_no-1][5].remoteUnitParents.value[ucUnitNo] ){
            continue;
        }

        // 今回のモニタリングでスキャン済みか？
        if( ( 0 != check_flag(&realscan_buff.do_unit[0], ucUnitNo) )        // 通信すべき子機リストに存在している
         && ( 0 != check_flag(&realscan_buff.over_unit[0], ucUnitNo) ) ){   // 通信できた子機リストに存在している
            continue;
        }

        // 指定した子機のルートを親番号リストから作成する
        GetRoot_from_ResultTable(grp_no, 1, ucUnitNo, &route);
            if(route.count != 0){
                // スキャンリスト追加
                scan_route_add( scan_route, route );
            }
        }
}


/// @brief  サーチスキャンのルートリスト作成
/// @param  grp_no             無線グループ
/// @param  scan_route         スキャンルート
/// @return 無し
/// スキャンが成功していない親機中継機があれば、スキャンリストを作成する
static void Make_Search_ScanList( uint8_t grp_no, RouteArray_t *scan_route ){

    uint8_t ucRepNo;                // 親機中継機番号
    RouteArray_t route;             // 中継ルート

    memset(&route, 0x00, sizeof(RouteArray_t));

    // スキャンリストの初期化
    memset(scan_route, 0x00, sizeof(RouteArray_t)*SCANLIST_MAX);

    // 親機がスキャンしていない
        if(check_flag(&realscan_buff.over_rpt[0],0) == 0){
            route.value[0] = 0;                                 // ルートに親機を登録
            route.count = 1;
            scan_route_add( scan_route, route );                // スキャンリストに追加
            return;                                             // 親機から行う
        }

    // 登録された中継機の中からまだスキャンしていない中継機を探す
    for( ucRepNo=1; ucRepNo<=REPEATER_MAX; ucRepNo++ ){

        // スキャンしていない中継機
        if(check_flag(&realscan_buff.do_rpt[0],ucRepNo) != 0){
            if(check_flag(&realscan_buff.over_rpt[0],ucRepNo) == 0){
                // この中継機の中継ルートを取得する
//                WirelessGroup_GetRepeaterRoute( &RF_Group[grp_no-1], ucRepNo, &route );
                GetRoot_from_ResultTable(grp_no, 0, ucRepNo, &route);
                if(route.count != 0){
                    // 自身もルートに追加する必要がある
                    route.value[route.count] = ucRepNo;
                    route.count++;

                    scan_route_add( scan_route, route );        // スキャンリストに追加
                }
            }
        }
    }
}

/// @brief  指定した子機/中継機番号のルートを親番号リストから作成して返す
/// @param[in]  grp_no   無線グループ番号
/// @param[in]  flg   中継機=0 子機=0以外
/// @param[in]  no    中継機番号または子機番号
/// @param      route     スキャンルート
//static void GetRoot_from_ResultTable(uint8_t grp_no, int flg, int no, RouteArray_t* route)
void GetRoot_from_ResultTable(uint8_t grp_no, int flg, int no, RouteArray_t* route)
{

	int8_t oyano;                  // 親番号
    int     i,j;                    // ループカウンタ
    int8_t tmpParent;               // 親番号（作業用）


    WirelessGroup_t *grp    = &RF_Group[grp_no-1];                     // 親番号リスト
    OYA_No_log_t *log_grp   = &RF_OYALOG_Group[grp_no-1][5];           // 親番号リストの履歴（最後に通信できた親機）

    // スキャンリストの初期化
    memset(route, 0x00, sizeof(RouteArray_t));

	if(flg == 0) {
		oyano = grp->repeaterParents.value[no];      // この中継機の親番号
	}
	else {
		oyano = grp->remoteUnitParents.value[no];    // この子機の親番号
	}

	// 親が負のときは、前回取得できていない。最後に通信できた親を入れる。
	if(oyano < 0) {

		if(flg == 0) {
			oyano = log_grp->repeaterParents.value[no];      // この中継機が最後に通信できた親
		}
		else {
			oyano = log_grp->remoteUnitParents.value[no];    // この子機が最後に通信できた親
		}
		if(oyano < 0) {
			// 実績が一切なし
			return;
		}
	}

	if(oyano == 0) {
		// 親は親機0。なのでこれで終了。
        route->value[0] = 0;                             // ルートに親機を登録
        route->count = 1;
	}
	else {

		// 中継機のルートの格納
        route->value[0] = (uint8_t)oyano;
        route->count = 1;

        for(i=0; i<=REPEATER_MAX; i++){

            tmpParent = grp->repeaterParents.value[oyano];
            if(tmpParent < 0) {
				oyano = log_grp->repeaterParents.value[oyano];   // この中継機が最後に通信できた親
                if(oyano < 0) {
                    // 実績が一切なし。親機0までルートがつながらない。
                    memset(route, 0x00, sizeof(RouteArray_t));     // ルートをクリア
                    return;
                }
            }
            else {
				oyano = tmpParent;
			}

        	if(oyano > 0) {
				// まだ中継する
                for(j=(route->count)-1; j>=0; j--){
                    route->value[j+1] = route->value[j];
                }
                route->value[0] = (uint8_t)oyano;
                route->count++;
            }
			else if(oyano == 0) {
				// 親は親機0
                for(j=(route->count)-1; j>=0; j--){
                    route->value[j+1] = route->value[j];
                }
               route->value[0] = (uint8_t)oyano;
                route->count++;
				return; // 正常終了
			}
        }

		// 親機0までルートがつながらなかった。
        memset(route, 0x00, sizeof(RouteArray_t));     // ルートをクリア
    }
}



/// @brief  サーチスキャンの開始判定
/// @param[in] grp_no           無線グループ
/// @return true/false（実施する/実施しない）
/// サーチスキャンを実施するか判定する
static bool Search_Scan_Start_Check(uint8_t grp_no)
{
    uint8_t ucUnitNo;                   // 子機番号
    bool    bRet = false;               // 戻り値
    uint32_t utime;                     // 実行時間

    for( ucUnitNo=1; ucUnitNo<=REMOTE_UNIT_MAX; ucUnitNo++ ){

        if( 0 == check_flag(&realscan_buff.do_unit[0], ucUnitNo) ){      // 通信すべき子機リストに存在していない
            continue;
        }

        // sakaguchi 2021.01.21 RTR-574,576の大きい子機番号はenableフラグがONされないためここで処理終了
        if(0 == RF_Group[grp_no-1].remoteUnitRssiTable[ucUnitNo]->enable){   // 子機登録なし
            continue;
        }

        if( 0 <= RF_Group[grp_no-1].remoteUnitParents.value[ucUnitNo] ){  // スキャンできている（親番号有り）？
            continue;
        }

        if( ( 0 > RF_OYALOG_Group[grp_no-1][0].remoteUnitParents.value[ucUnitNo] )
         && ( 0 > RF_OYALOG_Group[grp_no-1][1].remoteUnitParents.value[ucUnitNo] ) ){   // 直近２回失敗している

            utime = RTC_GetGMTSec();                    // UTC取得
            RF_buff.rf_res.time = utime;                // sakaguchi 2020.11.24
            if( 600 <= ( utime - searchscan_time[grp_no-1] ) ){   // 前回のサーチスキャンから10分以上経過 // sakaguchi 2021.01.21
                bRet = true;
                searchscan_time[grp_no-1] = utime;                // サーチスキャン開始時間更新 // sakaguchi 2021.01.21
                break;
            }
        }
    }
    return(bRet);

}

/// @brief  スキャンリストの調整
/// 既にスキャンした中継機のルートをスキャンリストから削除する
/// @param[inout] scan_route    スキャンリスト
/// @return 無し
static void scan_route_adjust(RouteArray_t *scan_route)
{
    int i;
    RouteArray_t tmp_route;
    uint8_t ucRepNo;

    for( i=0; i<=SCANLIST_MAX; i++ ){

        memcpy( &tmp_route, (scan_route+i), sizeof(RouteArray_t));
        if(1 <= tmp_route.count){
            ucRepNo = tmp_route.value[tmp_route.count-1];
            // スキャン済み
            if( (check_flag(&realscan_buff.do_rpt[0],ucRepNo) != 0)
             && (check_flag(&realscan_buff.over_rpt[0],ucRepNo) != 0) ){
                 scan_route_delete(scan_route, tmp_route);                 // ルートを削除
            }
        }
    }
}

/// @brief  スキャンリストの追加
/// 指定したルートをスキャンリストに追加し、ルートを短い順にソートする
/// @param[inout] scan_route    スキャンリスト
/// @param[in] add_route    追加したいスキャンルート
/// @return 無し
static void scan_route_add(RouteArray_t *scan_route, RouteArray_t add_route)
{
    int i,j;
    uint8_t sort_flg = 0;           // ソート実行フラグ
    RouteArray_t tmp_route;

    if( 0 != add_route.count ){

        for( i=0; i<=SCANLIST_MAX; i++ ){

            if( 0 != (scan_route+i)->count ){
                if( ( add_route.count == (scan_route+i)->count )
                 && ( 0 == memcmp((scan_route+i)->value, add_route.value, add_route.count)) ){
                    break;          // 同一ルートが既に登録済みのため処理終了
                }else{
                    continue;
                }

            }else{
                memcpy((scan_route+i), &add_route, sizeof(RouteArray_t));   // ルートを追加
                sort_flg = 1;
                break;
            }
        }

        if( 1 == sort_flg ){        // 短い順にソート実行
            for( i=0; i<=SCANLIST_MAX; i++ ){
                for( j=i+1; j<=SCANLIST_MAX; j++ ){
                    if( 0 == (scan_route+j)->count){
                        break;
                    }
                    if( (scan_route+i)->count > (scan_route+j)->count ){
                        memcpy( &tmp_route, (scan_route+i), sizeof(RouteArray_t));
                        memcpy( (scan_route+i), (scan_route+j), sizeof(RouteArray_t));
                        memcpy( (scan_route+j), &tmp_route, sizeof(RouteArray_t));
                    }
                }
            }
        }
    }
}


/// @brief  スキャンリストの削除
/// 指定したルートをスキャンリストから削除し、ルートを短い順にソートする
/// @param[inout] scan_route    スキャンリスト
/// @param[in] del_route    削除したいスキャンルート
/// @return 無し
static void scan_route_delete(RouteArray_t *scan_route, RouteArray_t del_route)
{
    int i;
    uint8_t sort_flg = 0;               // ソート実行フラグ

    if( 0 != del_route.count ){

        for( i=0; i<=SCANLIST_MAX; i++ ){
            if( 0 != (scan_route+i)->count ){
                if( ( del_route.count == (scan_route+i)->count )
                 && ( 0 == memcmp((scan_route+i)->value, del_route.value, del_route.count) ) ){
                    // 同一ルートが登録済みのため削除する
                    memset((scan_route+i), 0x00, sizeof(RouteArray_t));     // ルート削除
                    sort_flg = 1;
                    break;
                }
            }
        }

        if( 1 == sort_flg ){        // 段数=0があればそれ以降を前にコピー
            for( i=0; i<=SCANLIST_MAX-1; i++ ){
                if( (0 == (scan_route+i)->count) && (0 != (scan_route+i+1)->count) ){
                    memcpy( (scan_route+i), (scan_route+i+1), sizeof(RouteArray_t));        // 前にコピー
                    memset( (scan_route+i+1), 0x00, sizeof(RouteArray_t));                  // クリア
                }
            }
        }
    }
}



/// @brief  スキャンできていない子機があるか
/// @param[in] grp_no    無線グループ
/// @param[in] flg       モニタリング種別（REGU_MONI）
/// @param[in] check     0/1  1:「最後に通信できた親機」が無い場合はその子機は無視する
/// @return true/false  true=スキャンできていない子機ある
static bool check_scan_notyet(int grp_no, int flg, uint8_t check)
{
    uint8_t ucUnitNo;
    bool bRet = false;
    int8_t oyano;

	for(ucUnitNo=1; ucUnitNo<=REMOTE_UNIT_MAX; ucUnitNo++) {

        if( 0 != check_flag(&realscan_buff.do_unit[0], ucUnitNo) ){      // 通信すべき子機リストに存在している
            if( 0 == check_flag(&realscan_buff.over_unit[0], ucUnitNo) ){    // 通信できた子機リストに存在していない

                // 親番号リストの「最後に通信できた親機」が無効ならばその子機は無視
                if( (flg == REGU_MONI) && (check == 1) && (0 > RF_OYALOG_Group[grp_no-1][5].remoteUnitParents.value[ucUnitNo]) ){
                    continue;
                }else{
                    Printf("##########################[Group%d] check_scan_notyet exist[%d]\n", grp_no, ucUnitNo);
                    bRet = true;    // データ取得できてない子機有り
                    break;
                }

            }else{          // 通信できた子機リストに存在している

                // sakaguchi 2021.01.21 RTR-574,576の大きい子機番号はenableフラグがONされないためここで処理終了
                if(0 == RF_Group[grp_no-1].remoteUnitRssiTable[ucUnitNo]->enable){   // 子機登録なし
                    continue;
                }

                oyano = RF_Group[grp_no-1].remoteUnitParents.value[ucUnitNo];
                if( 0 > oyano ){
                    Printf("##########################[Group%d] check_scan_notyet exist2[%d]\n", grp_no, ucUnitNo);
                    bRet = true;    // データ取得できてない子機有り
                    break;
                }
            }
        }
	}
    if(bRet == false )  Printf("##########################[Group%d] check_scan_notyet non\n", grp_no);

	return(bRet);
}


/// @brief  指定した機器の電波強度をクリアする
/// @param[in] grp_no    無線グループ番号（全グループ指定：GROUP_ALL）
/// @param[in] flg       中継機=0 子機=0以外
/// @param[in] no        中継機番号または子機番号（全中継機：REPEATER_ALL/全子機：REMOTE_UNIT_ALL）
/// @return 実行結果 0:正常 0以外：異常
int RF_clear_rssi(int grp_no, int flg, uint8_t no)
{
    uint8_t i;
    uint8_t ucRepNo;                // 親機中継機番号
    uint8_t ucUnitNo;               // 子機番号
    int iRet = 0;                   // 戻り値
    uint8_t tmp_enable;


    // 全グループ指定
    if(GROUP_ALL == grp_no){
        for( i=1; i<=GROUP_MAX; i++ ){
            WirelessGroup_ClearRssiTable(&RF_Group[i-1]);      // グループの電波強度テーブルをクリア
        }
    }
    // グループ番号指定
    else{
        // 中継機
        if(0 == flg){
            // 中継機番号指定無し
            if(REPEATER_ALL == no){

                //中継機の電波強度テーブルをクリア
                RepeaterRssi_Clear(&RF_Group[grp_no-1].repeaterRssiTable);
                // 中継機番号指定無しの場合は、子機の電波強度もあわせてクリアする↓
                // 子機の電波強度テーブルをクリア
                for( ucUnitNo=1; ucUnitNo<=REMOTE_UNIT_MAX; ucUnitNo++ )
                {
                    RemoteUnitRssiArray_t *p = RF_Group[grp_no-1].remoteUnitRssiTable[ucUnitNo];
                    if(p)
                    {
                        tmp_enable = p->enable;
                        RemoteUnitRssiArray_Clear(p);
                        p->enable = tmp_enable;             // 元に戻す
                    }
                }
            }
            // 中継機番号指定有り
            else{

                // 中継機の電波強度更新（自身が子の場合）
                for( ucRepNo=0; ucRepNo<=REPEATER_MAX; ucRepNo++ ){
                    if(ucRepNo != no){                                  // 自身は除外
                        iRet = RepeaterRssi_Update(&RF_Group[grp_no-1].repeaterRssiTable, ucRepNo, no, RSSI_VOID, true);
                        if(iRet != 0)   break;                          // エラーの時点で終了
                    }
                }

                // 中継機の電波強度更新（自身が親の場合）
                for( ucRepNo=0; ucRepNo<=REPEATER_MAX; ucRepNo++ ){
                    if(ucRepNo != no){                                  // 自身は除外
                        iRet = RepeaterRssi_Update(&RF_Group[grp_no-1].repeaterRssiTable, no, ucRepNo, RSSI_VOID, true);
                        if(iRet != 0)   break;                          // エラーの時点で終了
                    }
                }

                // 子機の電波強度更新（自身が親の場合）
                for(ucUnitNo=1; ucUnitNo<=REMOTE_UNIT_MAX; ucUnitNo++) {
                    iRet = RemoteUnitRssi_Update(&RF_Group[grp_no-1], no, ucUnitNo, RSSI_VOID, true);
                    if((iRet != 0) && (iRet != (-3)))   break;         // エラーの時点で終了 クリア処理のため登録無し(-3)はエラーとしない
                    iRet = 0;                                          // 正常に戻す
                }
            }
        }
        // 子機
        else{
            // 子機番号指定無し
            if(REMOTE_UNIT_ALL == no){

                //子機の電波強度を初期化します。
                for( ucUnitNo=1; ucUnitNo<=REMOTE_UNIT_MAX; ucUnitNo++ )
                {
                    RemoteUnitRssiArray_t *p = RF_Group[grp_no-1].remoteUnitRssiTable[ucUnitNo];
                    if(p)
                    {
                        tmp_enable = p->enable;
                        RemoteUnitRssiArray_Clear(p);
                        p->enable = tmp_enable;             // 元に戻す
                    }
                }
            }
            else{

                // 子機の電波強度更新（自身が子の場合のみ）
                for( ucRepNo=0; ucRepNo<=REPEATER_MAX; ucRepNo++ ){
                    iRet = RemoteUnitRssi_Update(&RF_Group[grp_no-1], ucRepNo, no, RSSI_VOID, true);
                    if((iRet != 0) && (iRet != (-3)))   break;         // エラーの時点で終了 クリア処理のため登録無し(-3)はエラーとしない
                    iRet = 0;                                          // 正常に戻す
                }
            }
        }
    }

    return(iRet);
}


/// @brief  指定したグループの機器の親番号テーブルを取得する
/// @param  parent
/// @param[in] grp_no    無線グループ番号
/// @param[in] flg       中継機=0 子機=0以外
/// @param[in] log       最新=0 1回前=1 2回前=2 3回前=3 4回前=4 最後に通信できた親番号=5
/// @return 親番号テーブルサイズ
int RF_ParentTable_Get(char *parent, int grp_no, int flg, uint8_t log)
{
    int	size=0;                     // 戻り値：親番号テーブルサイズ
	uint8_t max_unit_no = 0;        // 最大子機番号
	uint8_t max_repeater_no = 0;    // 最大中継機番号

    if(GROUP_MAX < grp_no){
        return(size);
    }

    max_unit_no = RF_get_max_unitno(grp_no,&max_repeater_no);   // sakaguchi 2020.09.01 
    RF_Group[grp_no-1].max_unit_no = max_unit_no;               // 最大子機番号格納
    RF_Group[grp_no-1].max_rpt_no = max_repeater_no;            // 最大中継機番号格納

    // 中継機の親番号テーブル取得
    if(0 == flg){
        if(0 == log){
            memcpy(parent, &RF_Group[grp_no-1].repeaterParents.value[1], (size_t)max_repeater_no);                  // 最新の中継機親番号リスト取得
        }else{
            memcpy(parent, &RF_OYALOG_Group[grp_no-1][log].repeaterParents.value[1], (size_t)max_repeater_no);      // 履歴の中継機親番号リスト取得
        }
//        size = comma_separate_array_make(parent,iRepMax);
        size = max_repeater_no;

    }
    // 子機の親番号テーブル取得
    else{
        if(0 == log){
            memcpy(parent, &RF_Group[grp_no-1].remoteUnitParents.value[1], (size_t)max_unit_no);                // 最新の子機親番号リスト取得
        }else{
            memcpy(parent, &RF_OYALOG_Group[grp_no-1][log].remoteUnitParents.value[1], (size_t)max_unit_no);    // 履歴の子機親番号リスト取得
        }
//        size = comma_separate_array_make(parent,iUnitMax);
        size = max_unit_no;

    }

    return(size);

}


/// @brief  指定したグループの機器の電波強度テーブルを取得する
/// @param  parent
/// @param[in] grp_no    無線グループ番号
/// @param[in] flg       中継機=0 子機=0以外
/// @return 電波強度テーブルサイズ
int RF_RssiTable_Get(char *parent, int grp_no, int flg)
{
    int size=0;                 // 戻り値：電波強度テーブルサイズ
    int numL,numR;              // 中継機番号（ループカウンタ用）
    uint8_t rssi;               // 電波強度
    int icnt=0;
	uint8_t max_unit_no = 0;        // 最大子機番号
	uint8_t max_repeater_no = 0;    // 最大中継機番号

    if(GROUP_MAX < grp_no){
        return(size);
    }

    max_unit_no = RF_get_max_unitno(grp_no,&max_repeater_no);   // sakaguchi 2020.09.01 
    RF_Group[grp_no-1].max_unit_no = max_unit_no;               // 最大子機番号格納
    RF_Group[grp_no-1].max_rpt_no = max_repeater_no;            // 最大中継機番号格納

    // 中継機の電波強度テーブル取得
    if(0 == flg){

        // 中継機の電波強度テーブル取得
        for(numL=0; numL<=max_repeater_no; numL++){
            for(numR=0; numR<=max_repeater_no; numR++){
                if(0 == WirelessGroup_GetRepeaterRssi(&RF_Group[grp_no-1], numL, numR, &rssi)){
                    *(parent+icnt) = rssi;
                }else{
                    *(parent+icnt) = RSSI_VOID;
                }

                if(RSSI_VOID == *(parent+icnt)){
                    *(parent+icnt) = 255;                               // 電波強度不定は255に補正
				}
                if(RSSI_NONE == *(parent+icnt)){
                    *(parent+icnt) = 0;                                 // 無線通信不可は0に補正
                }
                icnt++;
            }
        }
    }
    // 子機の電波強度テーブル取得
    else{

        // 子機の電波強度テーブル取得
        for(numL=1; numL<=max_unit_no; numL++){
            for(numR=0; numR<=max_repeater_no; numR++){
                if(0 == WirelessGroup_GetRemoteUnitRssi(&RF_Group[grp_no-1], numR, numL, &rssi)){
                    *(parent+icnt) = rssi;
                }else{
                    *(parent+icnt) = RSSI_VOID;
                }

                if(RSSI_VOID == *(parent+icnt)){
					*(parent+icnt) = 255;                               // 電波強度不定は255に補正
				}
                if(RSSI_NONE == *(parent+icnt)){
                    *(parent+icnt) = 0;                                 // 無線通信不可は0に補正
                }
                icnt++;
            }
        }
    }

//    size = comma_separate_array_make(parent,icnt);
    size = icnt;

    return(size);
}


/// @brief  指定したグループの機器の電池残量テーブルを取得する
/// @param    parent
/// @param[in] grp_no    無線グループ番号
/// @param[in] flg       中継機=0 子機=0以外
/// @return 電池残量テーブルサイズ
int RF_BatteryTable_Get(char *parent, int grp_no, int flg)
{
    int size=0;                 // 戻り値：電池残量テーブルサイズ
    uint8_t max_unit_no = 0;        // 最大子機番号
    uint8_t max_repeater_no = 0;    // 最大中継機番号

    if(GROUP_MAX < grp_no){
        return(size);
    }

    max_unit_no = RF_get_max_unitno(grp_no,&max_repeater_no);   // sakaguchi 2020.09.01 
    RF_Group[grp_no-1].max_unit_no = max_unit_no;               // 最大子機番号格納
    RF_Group[grp_no-1].max_rpt_no = max_repeater_no;            // 最大中継機番号格納

    // 中継機の電池残量テーブル取得
    if(0 == flg){
        memcpy(parent, &RF_Group[grp_no-1].rpt_battery[1], (size_t)max_repeater_no);    // 中継機の電池残量取得
        size = max_repeater_no;
    }
    // 子機の電池残量テーブル取得
    else{
        memcpy(parent, &RF_Group[grp_no-1].unit_battery[1], (size_t)max_unit_no);  // 子機の電池残量取得
        size = max_unit_no;
    }

    return(size);

}






//------------------------------------------------------------------------------
//
//  RouteArray
//
//------------------------------------------------------------------------------

/// @brief  無線ルートを初期化します。
/// @param[inout] route   無線ルート
/// @return なし。
static void RouteArray_Clear(RouteArray_t *route)
//void RouteArray_Clear(RouteArray_t *route)
{
	    memset(route->value, 0, sizeof(route->value));
	    route->count = 0;
}



//------------------------------------------------------------------------------
//
//  WirelessGroup
//
//------------------------------------------------------------------------------

/// @brief  無線グループを初期化します。
/// @param[inout] grp   無線グループ
/// @return なし。
/// @note   allRemoteUnitRssiTable（static 変数）は初期化しません。
static void WirelessGroup_Clear(WirelessGroup_t *grp)
{
    RepeaterRssi_Clear(&grp->repeaterRssiTable);
    RepeaterParent_Clear(&grp->repeaterParents);
    RemoteUnitParent_Clear(&grp->remoteUnitParents);
    memset(&grp->remoteUnitRssiTable, 0, sizeof(grp->remoteUnitRssiTable));
    memset(&grp->unit_battery, 0, sizeof(grp->unit_battery));
    memset(&grp->rpt_battery, 0, sizeof(grp->rpt_battery));
}

/// @brief  無線グループの電波強度テーブルを初期化します。
/// @param[inout] grp   無線グループ
/// @return なし。
static void WirelessGroup_ClearRssiTable(WirelessGroup_t *grp)
{
    uint8_t tmp_enable;

    //中継機の電波強度テーブルを初期化します。
    RepeaterRssi_Clear(&grp->repeaterRssiTable);

    //子機の電波強度を初期化します。
    for(int i = 0; i <= REMOTE_UNIT_MAX; ++i) 
    {
        RemoteUnitRssiArray_t *p = grp->remoteUnitRssiTable[i];
        if(p)
        {
            tmp_enable = p->enable;
            RemoteUnitRssiArray_Clear(p);
            p->enable = tmp_enable;             // 元に戻す
        }
    }
}

/// @brief  中継機の電波強度を更新します。
/// @param[inout] grp           無線グループ
/// @param[in]    parentNum     親機番号
/// @param[in]    repeaterNum   中継機番号
/// @param[in]    rssi          電波強度
/// @return 成功すると0を返します。失敗すると0以外を返します。
static int WirelessGroup_UpdateRepeaterRssi(WirelessGroup_t *grp, int parentNum, int repeaterNum, uint8_t rssi)
{
    return (RepeaterRssi_Update(&grp->repeaterRssiTable, parentNum, repeaterNum, rssi, false));
}

/// @brief  子機の電波強度を更新します。
/// @param[inout] grp           無線グループ
/// @param[in]    parentNum     親機番号
/// @param[in]    remoteUnitNum 子機番号
/// @param[in]    rssi          電波強度
/// @return 成功すると0を返します。失敗すると0以外を返します。
static int WirelessGroup_UpdateRemoteUnitRssi(WirelessGroup_t *grp, int parentNum, int remoteUnitNum, uint8_t rssi)
{
    return (RemoteUnitRssi_Update(grp, parentNum, remoteUnitNum, rssi, false));
#if 0
    if(!((0 <= parentNum) && (parentNum <= REPEATER_MAX)))
    {
        return (-1);
    }

    if(!((1 <= remoteUnitNum) && (remoteUnitNum <= REMOTE_UNIT_MAX)))
    {
        return (-2);
    }

    RemoteUnitRssiArray_t *p = grp->remoteUnitRssiTable[remoteUnitNum];
    if(p == 0)
    {
        return (-3);
    }

    if(rssi > p->value[parentNum])
    {
        p->value[parentNum] = (uint8_t)rssi;
    }

    return (0);
#endif
}

/// @brief  電波強度を元に無線ルートを更新します。
/// @param[inout] grp           無線グループ
/// @param[in]    rssiLimit1    無線通信できると判断する電波強度のしきい値（境界を含む）
/// @param[in]    rssiLimit2    無線通信できると判断する電波強度のしきい値（境界を含む）
/// @return なし。
void RF_WirelessGroup_RefreshParentTable(WirelessGroup_t *grp, uint8_t rssiLimit1, uint8_t rssiLimit2)
{
// 登録情報に電波強度のしきい値が格納されていない場合はデフォルトで動作させる
    if(0 == rssiLimit1) rssiLimit1=80;
    if(0 == rssiLimit2) rssiLimit2=60;

    // △2020.06.06
    RepeaterParent_Clear(&grp->repeaterParents);
    RemoteUnitParent_Clear(&grp->remoteUnitParents);

    //中継機の親番号を更新します。
    RepeaterParent_Refresh(grp, rssiLimit1);

    //子機の親番号を更新します。
    RemoteUnitParent_Refresh(grp, rssiLimit1);

    // ▲2020.06.25
    //中継機の親番号を更新します。
    RepeaterParent_Refresh(grp, rssiLimit2);

    //子機の親番号を更新します。
    RemoteUnitParent_Refresh(grp, rssiLimit2);

    // △2020.06.05
    //中継機の親番号を更新します。
//    RepeaterParent_Refresh(grp, 1);
    RepeaterParent_Refresh(grp, 2);             // 閾値1は電波強度不可で使用

    //子機の親番号を更新します。
//    RemoteUnitParent_Refresh(grp, 1);
    RemoteUnitParent_Refresh(grp, 2);           // 閾値1は電波強度不可で使用

}


/// @brief  子機の電波強度を取得します。
/// @param[in] grp              無線グループ
/// @param[in] parentNum        親番号
/// @param[in] remoteUnitNum    子機番号
/// @param[out] rssi            電波強度
/// @return 成功すると0を返します。失敗すると0以外を返します。
static int WirelessGroup_GetRemoteUnitRssi(const WirelessGroup_t *grp, int parentNum, int remoteUnitNum, uint8_t *rssi)
{
//   if(!((0 <= parentNum) && (parentNum < sizeof(grp->remoteUnitRssiTable->value))))
    if(!((0 <= parentNum) && (parentNum < (REPEATER_MAX + 1))))
    {
        return (-1);
    }

//    if(!((1 <= remoteUnitNum) && (remoteUnitNum < (int)(sizeof(grp->remoteUnitRssiTable) / sizeof(RemoteUnitRssiArray_t)))))
    if(!((1 <= remoteUnitNum) && (remoteUnitNum < (REMOTE_UNIT_MAX + 1))))           // sakaguchi 2021.03.11
    {
        return (-2);
    }

    if(rssi == 0)
    {
        return (-3);
    }

    const RemoteUnitRssiArray_t *rssiArray = grp->remoteUnitRssiTable[remoteUnitNum];
    
    if(rssiArray == 0)
    {
        return (-4);
    }

    if(rssiArray->enable == 0)
    {
        return (-5);
    }

    *rssi = (uint8_t)rssiArray->value[parentNum];

    return (0);
}


/// @brief  中継機の電波強度を取得します。
/// @param[in] grp              無線グループ
/// @param[in] parentNum        親番号
/// @param[in] repeaterNum      中継機番号
/// @param[out] rssi            電波強度
/// @return 成功すると0を返します。失敗すると0以外を返します。
static int WirelessGroup_GetRepeaterRssi(const WirelessGroup_t *grp, int parentNum, int repeaterNum, uint8_t *rssi)
{
//    if(!((0 <= parentNum) && (parentNum < REPEATER_MAX)))
    if(!((0 <= parentNum) && (parentNum <= REPEATER_MAX)))              // sakaguchi 2020.10.07
    {
        return (-1);
    }

//    if(!((1 <= repeaterNum) && (repeaterNum <= REPEATER_MAX)))
    if(!((0 <= repeaterNum) && (repeaterNum <= REPEATER_MAX)))
    {
        return (-2);
    }

    if(rssi == 0)
    {
        return (-3);
    }

    *rssi = (uint8_t)RepeaterRssi_Get(&grp->repeaterRssiTable, parentNum, repeaterNum);

    return (0);
}


/// @brief  中継機の無線ルートを取得します。
/// @param[in] grp              無線グループ
/// @param[in] repeaterNum      中継機番号（親機を指定するときは0）
/// @param[out] route           無線ルート
/// @return 無線ルートの段数を返します。無線通信できない中継機のときは0を返します。
///         失敗すると負の値を返します。
static int WirelessGroup_GetRepeaterRoute(const WirelessGroup_t *grp, int repeaterNum, RouteArray_t *route)
{
    if(!((0 <= repeaterNum) && (repeaterNum <= REPEATER_MAX)))
    {
        return (-1);
    }

    return (RepeaterParent_GetRoute(&grp->repeaterParents, repeaterNum, route));
}



/// @brief  子機の無線ルートを取得します。
/// @param[in] grp             無線グループ
/// @param[in] remoteUnitNum   中継機番号（親機を指定するときは0）
/// @param[out] route          無線ルート
/// @return 無線ルートの段数を返します。無線通信できない中継機のときは0を返します。
///         失敗すると負の値を返します。
static int WirelessGroup_GetRemoteRoute(const WirelessGroup_t *grp, int remoteUnitNum, RouteArray_t *route)
{
    if(!((0 <= remoteUnitNum) && (remoteUnitNum <= REMOTE_UNIT_MAX)))
    {
        return (-1);
    }

    const int8_t parentNum = grp->remoteUnitParents.value[remoteUnitNum];
    if(parentNum >= 0)
    {
        return (RepeaterParent_GetRoute(&grp->repeaterParents, parentNum, route));
    }
    else
    {
        RouteArray_Clear(route);
        return (0);
    }
}

/**
    @brief  子機を登録します。
    @param[inout]   grp     無線グループ
    @param[in]  remoteUnitNum   子機番号
    @return 成功すると0を返します。失敗すると0以外を返します。
*/
static int WirelessGroup_RegisterRemoteUnit(WirelessGroup_t *grp, int remoteUnitNum)
{
    if(grp->allRemoteUnitRssiTable == 0){
        return (-1);
    }

    if(!((1 <= remoteUnitNum) && (remoteUnitNum <= REMOTE_UNIT_MAX))){
        return (-2);
    }

    if(grp->remoteUnitRssiTable[remoteUnitNum] != 0) {
        return (-3);
    }

    for(int i = 0; i < REMOTE_UNIT_MAX; ++i) {
        RemoteUnitRssiArray_t *item = &grp->allRemoteUnitRssiTable->units[i];
        if(item->enable) {
            continue;
        }

        RemoteUnitRssiArray_Clear(item);
        item->enable = 1;
        grp->remoteUnitRssiTable[remoteUnitNum] = item;
        break;
    }

    if(grp->remoteUnitRssiTable[remoteUnitNum] == 0) {
        return (-4);
    }

    return (0);
}

#if 0
//未使用
/**
    @brief  子機登録を解除します。
    @param[inout]   grp     無線グループ
    @param[in]  remoteUnitNum   子機番号
    @return 成功すると0を返します。失敗すると0以外を返します。
*/
static int WirelessGroup_UnregisterRemoteUnit(WirelessGroup_t *grp, int remoteUnitNum)
{
    if(grp->allRemoteUnitRssiTable == 0){
        return (-1);
    }

    if(!((1 <= remoteUnitNum) && (remoteUnitNum <= REMOTE_UNIT_MAX))){
        return (-2);
    }

    RemoteUnitRssiArray_t *p = grp->remoteUnitRssiTable[remoteUnitNum];
    if(p != 0) {
        p->enable = 0;
        p = 0;
    }

    return (0);
}


#endif

//------------------------------------------------------------------------------
//
//  RepeaterParent
//
//------------------------------------------------------------------------------


/// @brief  中継機の親番号テーブルを初期化します。
/// @param[inout]   tbl     中継機の親番号テーブル  
/// @return なし。
static void RepeaterParent_Clear(RepeaterParentArray_t *tbl)
{
    memset(tbl->value, -1, sizeof(RepeaterParentArray_t));
}

/**
    @brief  中継機の親番号テーブルを更新します。
    @param[inout]   grp     無線グループ
    @param[in]      rssiLimit    無線通信できると判断する電波強度のしきい値（境界を含む）
    @note   中継機の電波強度テーブルの情報を元に中継機の親番号テーブルを再計算します。
*/
static void RepeaterParent_Refresh(WirelessGroup_t *grp, uint8_t rssiLimit)
{
    //RepeaterParent_Clear(&grp->repeaterParents);

    //初回は親機と通信できる中継機を設定します。
    ParentArray_t parents;
    parents.count = 1;
    parents.value[0] = 0;

    //通信できる中継機が見つからなくなるまで繰り返します。
    while(parents.count > 0)
    {
        RepeaterParent_DoUpdate(grp, rssiLimit, &parents);
    }
}

/**
    @brief  中継機の親番号テーブルを更新します。
    @param[inout]   grp     無線グループ
    @param[in]      rssiLimit    無線通信できると判断する電波強度のしきい値（境界を含む）
    @param[inout]   parents     調べる親番号の配列
*/
//DUMMY TODO
void assert(int X) {} //TODO
static void RepeaterParent_DoUpdate(WirelessGroup_t *grp, uint8_t rssiLimit, ParentArray_t *parents)
{
    assert(parents->count < sizeof(parents->value));

    ParentArray_t items;
    memset(items.value, 0, sizeof(parents->value));
    items.count = 0;

    //親機が未設定の中継機を処理します。
    for(int repeaterNum = 1; repeaterNum <= REPEATER_MAX; ++repeaterNum)
    {
        assert(repeaterNum < (int)sizeof(grp->repeaterParents.value));
        int candidateNum = grp->repeaterParents.value[repeaterNum];
        if(candidateNum >= 0)
        {
            int find = 0;
            for (int i = 0; i < parents->count; ++i) {
                if (parents->value[i] == candidateNum) {
                    find = 1;
                    break;
                }
            }
            if (!find)
            {
                continue;
            }
        }
        else
        {
            int rssiMax = RSSI_VOID;

            //指定した親機と通信できるか調べます。
            for(int j = 0; j < parents->count; ++j)
            {
                assert(j < (int)sizeof(parents->value));
                const int8_t parentNum = (int8_t)parents->value[j];

                const uint8_t rssi = RepeaterRssi_Get(&grp->repeaterRssiTable, parentNum, repeaterNum);

                //指定した電波強度に達していない機器を除外します。
                //if(rssi < rssiLimit)
                if(rssi <= rssiLimit)           // 閾値も除外する   //sakaguchi 2021.10.05
                {
                    continue;
                }

                //電波強度が強い方を採用します。
                if(rssi < rssiMax)
                {
                    continue;
                }

                candidateNum = parentNum;
                rssiMax = rssi;
            }

            //通信できる親機が見つからなかった機器を除外します。
            if(candidateNum < 0)
            {
                continue;
            }
        }

        grp->repeaterParents.value[repeaterNum] = (int8_t)candidateNum;
        items.value[items.count++] = (uint8_t)repeaterNum;

#if 0
        assert(repeaterNum < (int)sizeof(grp->repeaterParents.value));
        if(grp->repeaterParents.value[repeaterNum] >= 0)
        {
            continue;
        }

        int candidateNum = -1;
        uint8_t rssiMax = RSSI_VOID;

        //指定した親機と通信できるか調べます。
        for(int j = 0; j < parents->count; ++j)
        {
            assert(j < (int)sizeof(parents->value));
            const int8_t parentNum = (int8_t)parents->value[j];

            const uint8_t rssi = RepeaterRssi_Get(&grp->repeaterRssiTable, parentNum, repeaterNum);

            //指定した電波強度に達していない機器を除外します。
            if(rssi < rssiLimit)
            {
                continue;
            }

            //電波強度が強い方を採用します。
            if(rssi < rssiMax)
            {
                continue;
            }

            candidateNum = parentNum;
            rssiMax = rssi;
        }

        //通信できる親機が見つからなかった機器を除外します。
        if(candidateNum < 0)
        {
            continue;
        }

        grp->repeaterParents.value[repeaterNum] = (int8_t)candidateNum;
        items.value[items.count++] = (uint8_t)repeaterNum;
#endif

    }

    memcpy(parents, &items, sizeof(ParentArray_t));
    assert(parents->count < sizeof(parents->value));
}

/// @brief  中継機の無線ルートを取得します。
/// @param[in]    tbl          中継機の親番号テーブル
/// @param[in]    repeaterNum  中継機番号（親機を指定するときは0）
/// @param[inout] route        中継機番号（親機を指定するときは0）
/// @return 無線ルートの段数を返します。無線通信できない中継機のときは0を返します。
///         失敗すると負の値を返します。
static int RepeaterParent_GetRoute(const RepeaterParentArray_t *tbl, int repeaterNum, RouteArray_t *route)
{
    if(!((0 <= repeaterNum) && (repeaterNum <= REPEATER_MAX)))
    {
        return (-1);
    }

    RouteArray_Clear(route);

    //先頭に無線ルートを調べる中継機番号を設定します。
    int index = 0;
    int num = repeaterNum;
    route->value[index++] = (uint8_t)num;

    //親機が見つかるまで繰り返します。
    while((num > 0) && (index < (int)sizeof(route->value)))
    {
        //指定した中継機の親番号を取得します。
        if(num >= (int)sizeof(tbl->value))
        {
            return (-2);
        }

        num = tbl->value[num];
        if(num >= 0)
        {
            route->value[index++] = (uint8_t)num;
        }
    }

    //親番号がないときは無線通信できない中継機です。
    if(num != 0)
    {
        return (0);
    }

    //順番を反転して親機からのルートにします。
    for(int i = 0, len = (index / 2); i < len; ++i)
    {
        const int j = index - 1 - i;
        const uint8_t tmp = route->value[i];
        route->value[i] = route->value[j];
        route->value[j] = tmp;
    }

    //無線ルートの段数を設定します。
    route->count = (uint8_t)index;

    return (route->count);
}





//------------------------------------------------------------------------------
//
//  RepeaterRSSI
//
//------------------------------------------------------------------------------


/// @brief  テーブルを初期化します。
/// @param[inout]   tbl     中継機の電波強度テーブル  
/// @return なし。
static void RepeaterRssi_Clear(RepeaterRssiTable_t *tbl)
{
    memset(tbl->value, RSSI_VOID, sizeof(RepeaterRssiTable_t));
}


/// @brief  中継機の電波強度にアクセスするインデックスを取得します。
/// @param[in]    numL  中継機番号（親機を指定するときは0）  
/// @param[in]    numR  中継機番号（親機を指定するときは0）  
/// @return 成功すると0以上の値を返します。失敗すると負の値を返します。
/// @note   numL, numRの順番を変えても結果は同じです。
static int RepeaterRssi_Index(int numL, int numR)
{
    if(numL > numR)
    {
        const int temp = numL;
        numL = numR;
        numR = temp;
    }

    if(numL < 0)
    {
        return (-1);
    }

    if(numL == numR)
    {
        return (-2);
    }

    if(numR > REPEATER_MAX)
    {
        return (-3);
    }

    return ((numL * REPEATER_MAX) + (numR - 1));
}


/// @brief  中継機の電波強度を取得します。
/// @param[in] tbl   中継機の電波強度テーブル
/// @param[in] numL  中継機番号（親機を指定するときは0）
/// @param[in] numR  中継機番号（親機を指定するときは0）
/// @return 電波強度を返します。失敗するとRSSI_VOIDを返します。
/// @note   numL, numRの順番を変えても結果は同じです。
static uint8_t RepeaterRssi_Get(const RepeaterRssiTable_t *tbl, int numL, int numR)
{
    const int index = RepeaterRssi_Index(numL, numR);
    return ( (index >= 0) ? (uint8_t)tbl->value[index] : RSSI_VOID );
}


/// @brief  中継機の電波強度を更新します。
/// @param[inout] tbl   中継機の電波強度テーブル
/// @param[in] numL  中継機番号（親機を指定するときは0）
/// @param[in] numR  中継機番号（親機を指定するときは0）
/// @param[in] rssi  電波強度
/// @param[in] force 電波強度の比較をしないで設定するか？
/// @return 成功すると0を返します。失敗すると0以外を返します。
/// @note   numL, numRの順番を変えても結果は同じです。
static int RepeaterRssi_Update(RepeaterRssiTable_t *tbl, int numL, int numR, uint8_t rssi, bool force)
{
    const int index = RepeaterRssi_Index(numL, numR);
    if(index < 0)
    {
        return (index);
    }

    if(force || (rssi > tbl->value[index]))
    {
        tbl->value[index] = (uint8_t)rssi;
    }

    return (0);

}



//------------------------------------------------------------------------------
//
//  RemoteUnit
//
//------------------------------------------------------------------------------


/// @brief  親機（中継機）－子機間の電波強度リストを初期化します。
/// @param[inout]   rssiArray   親機（中継機）－子機間の電波強度リスト  
/// @return なし。
static void RemoteUnitRssiArray_Clear(RemoteUnitRssiArray_t *rssiArray)
{
    memset(&rssiArray->value, RSSI_VOID, sizeof(rssiArray->value));
    rssiArray->enable = 0;
}


/// @brief  親機（中継機）－子機間の電波強度テーブルリストを初期化します。
/// @param[inout]   tbl   親機（中継機）－子機間の電波強度テーブル 
/// @return なし。
static void RemoteUnitRssiTable_Clear(RemoteUnitRssiTable_t *tbl)
{
    for(int i = 0; i < REMOTE_UNIT_MAX; ++i)
    {
        RemoteUnitRssiArray_Clear(&tbl->units[i]);
    }
}




/// @brief  子機の親番号リストを初期化します。
/// @param[inout]   parents   親番号リスト  
/// @return なし。
static void RemoteUnitParent_Clear(RemoteUnitParentArray_t *parents)
{
//    memset(&parents, -1, sizeof(parents));
    memset(parents, -1, sizeof(RemoteUnitParentArray_t));
}

/// @brief  子機の親番号リストを更新します。
/// @param[inout]   grp     無線グループ
/// @param[in]      rssiLimit    無線通信できると判断する電波強度のしきい値（境界を含む）
/// @return なし。
/// @note   中継機の親番号リストを利用するので、中継機の親番号リストを更新してから呼び出して下さい。
static void RemoteUnitParent_Refresh(WirelessGroup_t *grp, uint8_t rssiLimit)
{
    uint8_t rssiMax = 0;

    RouteArray_t tmpRoute;
    uint8_t tmpRssi = RSSI_VOID;

    for(int remoteUnitNum = 1; remoteUnitNum <= REMOTE_UNIT_MAX; ++remoteUnitNum)
    {
    	int routeCount = 255;
        RemoteUnitRssiArray_t *p = grp->remoteUnitRssiTable[remoteUnitNum];
        if(p == 0)
        {
            continue;
        }

        assert(p->enable);

		// ▲2020.06.05
        if(grp->remoteUnitParents.value[remoteUnitNum] >= 0)
        {
            continue;
        }

        //無線ルートなしの値で初期化します。
        grp->remoteUnitParents.value[remoteUnitNum] = -1;
        rssiMax = 0;

        for(int parentNum = 0; parentNum < (REPEATER_MAX + 1); ++parentNum)
        {
            //弱い電波強度の機器を除外します
            tmpRssi = p->value[parentNum];
            //if(tmpRssi < rssiLimit)
            if(tmpRssi <= rssiLimit)           // 閾値も除外する   //sakaguchi 2021.10.05
            {
                continue;
            }

            //無線ルートを取得します
            const int count = RepeaterParent_GetRoute(&grp->repeaterParents, parentNum, &tmpRoute);
            if(count <= 0)// ▲2020.06.05
            {
                continue;
            }

            //無線ルートの段数が多い機器を除外します
            if(count > routeCount)
            {
                continue;
            }

            //無線ルートの段数が同じときは子機の電波強度が強い方を選択します
            if((count == routeCount) && (tmpRssi < rssiMax))
            {
                continue;
            }

            routeCount = count;
            rssiMax = tmpRssi;
            grp->remoteUnitParents.value[remoteUnitNum] = (int8_t)parentNum;
        }
    }
}

/// @brief  子機の電波強度を更新します。
/// @param[inout] grp           無線グループ
/// @param[in]    parentNum     親機番号
/// @param[in]    remoteUnitNum 子機番号
/// @param[in]    rssi          電波強度
/// @param[in]    force 電波強度の比較をしないで設定するか？
/// @return 成功すると0を返します。失敗すると0以外を返します。
static int RemoteUnitRssi_Update(WirelessGroup_t *grp, int parentNum, int remoteUnitNum, uint8_t rssi, bool force)
{

    if(!((0 <= parentNum) && (parentNum <= REPEATER_MAX)))
    {
        return (-1);
    }

    if(!((1 <= remoteUnitNum) && (remoteUnitNum <= REMOTE_UNIT_MAX)))
    {
        return (-2);
    }

    RemoteUnitRssiArray_t *p = grp->remoteUnitRssiTable[remoteUnitNum];
    if(p == 0)
    {
        return (-3);
    }

    if(force || (rssi > p->value[parentNum]))
    {
        p->value[parentNum] = (uint8_t)rssi;
    }

    return (0);
}



//rf_thread_entry.cからRFunc.cに移動
/**
 * @brief   レギュラーモニタリングを行う。
 *
 * この関数を実行する前の手順
 * - rpt_sequence にルート情報を格納する
 * - group_registry.altogether にルート段数を格納する
 * @param data_format   通常は0x02を指定する。
 * @return
 */
static char regu_moni_scan(uint8_t data_format)
{

	int i;
	uint8_t scn_o_n = 1;	// 0:0x68  1:0x30
	uint8_t rtn;
	uint8_t max_unit_work;		// --------------------------------------- リアルスキャンするMAX子機を保存しておく	2014.10.14追加
/*	union{
		uint8_t byte[2];
		uint16_t word;
	}b_w_chg;
*/
	uint16_t word_data;
	uint64_t GMT_cal;
	uint64_t LAST_cal;


	max_unit_work = group_registry.max_unit_no;		// --------------------------------------- リアルスキャンするMAX子機を保存しておく	2014.10.14追加


	if((root_registry.auto_route & 0x01) == 0){		// 自動ルート機能OFFか
		scn_o_n = 0;
	}


	if(scn_o_n == 0){
		RF_buff.rf_req.cmd1 = 0x68;									// リアルスキャンコマンド(子機だけ)
	}else{
		RF_buff.rf_req.cmd1 = 0x30;									// リアルスキャンコマンド(子機と中継機)
		//complement_info[2] = group_registry.altogether;
		complement_info[2] = RF_Group[RF_buff.rf_req.current_group-1].max_rpt_no;   // 最大中継機番号       // sakaguchi 2021.10.05
	}

	RF_buff.rf_req.cmd2 = 0x00;

	RF_buff.rf_req.data_size = 2;									// 追加データは中継情報のみ

	//complement_info[0] = (char)(group_registry.max_unit_no + 1);	// 1個のグループに含まれる最大子機番号
	complement_info[0] = (char)(group_registry.max_unit_no);	    // 1個のグループに含まれる最大子機番号      // sakaguchi 2021.02.03
	complement_info[1] = data_format;								// データ番号を指定する(前回取得データを元に)

	RF_buff.rf_req.data_format = data_format;						// 2019.09.07追加 500WではR500_command_make()でやっていた

	End_Rpt_No = rpt_sequence[group_registry.altogether - 1];		// リアルスキャンの末端中継機Noを記憶

	RF_buff.rf_res.time = 0;
	timer.int125 = 0;										// コマンド開始から子機が応答したときまでの秒数をカウント開始
	DL_start_utc = RTC_GetGMTSec();							// コマンドを発行する直前のGMTを保存する

	Printf("auto_realscan_New 2 -->  RF_command_execute(%02X)\r\n", RF_buff.rf_req.cmd1);
	rtn = RF_command_execute(RF_buff.rf_req.cmd1);				// 無線通信をする。
	RF_buff.rf_res.status = rtn;

//	RF_buff.rf_res.time = RF_buff.rf_res.time + DL_start_utc;

	Printf("	DL_start_utc  %ld \r\n", DL_start_utc) ;
	Printf("	Attach.GMT    %ld \r\n", Download.Attach.GMT) ;

//	memcpy(&RF_buff.rf_res.time, &DL_start_utc,4);        // 子機とRF通信開始したGMTを保存
    // 時間計算をLASTを参照するように修正 2020.07.07
    GMT_cal = RTC_GetGMTSec();					// 無線通信終了後のGMT取得
	GMT_cal = (GMT_cal * 1000) + 500;			// 500msec足して誤差を四捨五入	2020/08/28 segi
    LAST_cal = timer.int125 * 125uL;
    GMT_cal = GMT_cal - LAST_cal;

    if((GMT_cal % 1000) < 500){
        GMT_cal = GMT_cal / 1000;
    }
    else{
        GMT_cal = GMT_cal / 1000;
        GMT_cal++;
    }
    RF_buff.rf_res.time = (uint32_t)GMT_cal;	// コマンドを発行する直前のGMTを保存する

	if(RF_buff.rf_res.status == RFM_NORMAL_END){					// 正常終了したらデータをGSM渡し変数にコピー	RF_command_execute の応答が入っている

		if(group_registry.altogether > 1){
			set_flag(&realscan_buff.over_rpt[0], rpt_sequence[group_registry.altogether - 1]);	// 中継機があれば末端中継機と通信済とする
		}
		else{
			set_flag(&realscan_buff.over_rpt[0],0);												// 中継機なければ親機と通信済とする
		}

//-------------
		if(r500.node == COMPRESS_ALGORITHM_NOTICE)	// 圧縮データを解凍(huge_buffer先頭は0xe0)
		{
			//i = r500.data_size - *(char *)&huge_buffer[2];
			i = r500.data_size - *(uint16_t *)&huge_buffer[2];          // sakaguchi 2021.02.03

			memcpy(work_buffer, huge_buffer, r500.data_size);
			r500.data_size = (uint16_t)LZSS_Decode((uint8_t *)work_buffer, (uint8_t *)huge_buffer);     // 2022.06.09

			if(i > 0)								// 非圧縮の中継機データ情報があるとき
			{
				memcpy((char *)&huge_buffer[r500.data_size], (char *)&work_buffer[*(uint16_t *)&work_buffer[2]], (size_t)i);
			}

			r500.data_size = (uint16_t)(r500.data_size + i);
			property.acquire_number = r500.data_size;
			property.transferred_number = 0;

			r500.node = 0xff;						// 解凍が重複しないようにする
		}
//--------------
/*
		b_w_chg.byte[0] = huge_buffer[0];							// 無線通信結果メモリを自律処理用変数にコピーする
		b_w_chg.byte[1] = huge_buffer[1];							// 無線通信結果メモリを自律処理用変数にコピーする
		b_w_chg.word = (uint16_t)(b_w_chg.word + 2);
*/
//		word_data = (uint16_t)(*(uint16_t *)&huge_buffer[0] + 2);
		word_data = (uint16_t)(*(uint16_t *)&huge_buffer[0] + rpt_sequence_cnt*2);  // 個別中継機データ分も加算

		if(RF_buff.rf_req.cmd1 == 0x68){
//			memcpy(rf_ram.realscan.data_byte,huge_buffer,b_w_chg.word);		// MAX=(30byte×128)+2 = 3842
            memcpy(rf_ram.realscan.data_byte,huge_buffer, word_data);     // MAX=(30byte×128)+2 = 3842
		}
		else{
//			memcpy(rf_ram.realscan.data_byte,&huge_buffer[2],b_w_chg.word);	// MAX=(30byte×128)+2 = 3842
			memcpy(rf_ram.realscan.data_byte,&huge_buffer[2], word_data); // MAX=(30byte×128)+2 = 3842
		}
//TODO		copy_realscan(1);

		group_registry.max_unit_no = max_unit_work;	// copy_realscan(1)でMAXを0にしてしまっていたmax_unit_work	2016.03.23修正
	}

	return(RF_buff.rf_res.status);

}

/**
 * 登録ファイルからグループ番号を指定して最大子機番号を出力する
 * @param [in]gnum      グループ番号
 * @param [out]rpt_no   最大中継機番号
 * @return          最大子機番号
 */
//uint8_t RF_get_max_unitno(int gnum)
uint8_t RF_get_max_unitno(int gnum, uint8_t *max_repeater_no)
{
    int         i;                                          // ループカウンタ
    uint16_t    adr,len;                                    // アドレス
    uint8_t     max_unit_no;                                // 最大子機番号
	uint8_t     read_rpt_no;                                // 中継機番号
    def_ru_registry read_ru_regist;                         // 子機情報             // sakaguchi 2020.09.01
    def_group_registry  read_group_registry;                // グループ情報         // sakaguchi 2020.09.01

// sakaguchi 2020.09.01 ↓
//    adr = get_regist_group_adr((uint8_t)gnum);                          // グループ番号が存在するかを調べる
    adr = RF_get_regist_group_adr((uint8_t)gnum, &read_group_registry);         // グループ番号が存在するかを調べる  // sakaguchi 2020.09.01

    adr = (uint16_t)(adr + read_group_registry.length);                      // グループヘッダ分アドレスＵＰ
    max_unit_no = 0;					                                // 登録されている最大子機番号把握する為、最初は0としておく

    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS)			// シリアルフラッシュ ミューテックス確保 スレッド確定
    {
        // 最大中継機番号の取得
        *max_repeater_no = 0;
        for(i = 0; i < read_group_registry.altogether; i++)					// 中継機情報分アドレスＵＰ
        {
            serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), 2, (char *)&len);                // 中継機情報サイズを読み込む
            serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr + 3), 1, (char *)&read_rpt_no);	// 最大中継機番号を把握
            if(*max_repeater_no < read_rpt_no){
                *max_repeater_no = read_rpt_no;                   // 最大中継機番号を更新
            }
            adr = (uint16_t)(adr + len);
        }

        for(i = 0; i < read_group_registry.remote_unit; i++)
        {
            serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), sizeof(read_ru_regist), (char *)&read_ru_regist);	// 子機情報構造体に読み込む

            if(max_unit_no < read_ru_regist.rtr501.number){
                max_unit_no = read_ru_regist.rtr501.number;	// 最大子機番号を把握する
            }

            if((read_ru_regist.rtr501.header == 0xfa)||(read_ru_regist.rtr501.header == 0xf9)){
                max_unit_no = max_unit_no + 1;		        // 574,576なら子機番号を2個使うのでインクリメント
            }

            adr = (uint16_t)(adr + read_ru_regist.rtr501.length);
        }
        tx_mutex_put(&mutex_sfmem);

    }
// sakaguchi 2020.09.01 ↑

    return(max_unit_no);            // 最大子機番号

}

// sakaguchi 2020.09.01 ↓
/**
 * 登録ファイルからグループ数を得る（ルート情報は更新無し）
 * @return      グループ番号
 * @note    ルート情報構造体にルート情報は格納しない
 */
uint8_t RF_get_regist_group_size(uint32_t *adr)
{
    def_root_registry read_root_registry;

    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS)          // シリアルフラッシュ ミューテックス確保 スレッド確定
    {
        serial_flash_multbyte_read(SFM_REGIST_START, sizeof(read_root_registry), (char *)&read_root_registry);  // ルート情報構造体に読み込む
        tx_mutex_put(&mutex_sfmem);
    }

    if(read_root_registry.length != 0x10){
        read_root_registry.group = 0;   // 情報サイズが違っていたら０
    }
    if(read_root_registry.header != 0xfb){
        read_root_registry.group = 0;   // 情報種類コードが違っていたら０
    }

    *adr = read_root_registry.length;
    return(read_root_registry.group);
}



/**
 * 登録ファイルからグループ番号指定してグループ情報アドレスを取得する（グループ情報更新無し）
 * @param gnum  グループ番号
 * @return      グループ情報の格納されているアドレス（SFM_REGIST_STARTからのオフセット）
 * @note        グループ情報構造体にグループ情報は格納しない
 */
uint16_t RF_get_regist_group_adr(uint8_t gnum, def_group_registry *read_group_registry)
{
    int i, j, max;
    uint32_t adr, len;

//    i = ~gnum;
//    max = get_regist_group_size();                                // 最大グループ番号（グループ数）
//    adr = root_registry.length;                                   // 最初のグループヘッダアドレス
    max = RF_get_regist_group_size(&adr);                       // 最大グループ番号（グループ数）       // sakaguchi 2020.09.01

    if(max == 0){
        return(0);
    }

    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS)              // シリアルフラッシュ ミューテックス確保 スレッド確定
    {
        for(i = 1; i <= max; i++)
        {
            serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), sizeof(def_group_registry), (char *)read_group_registry);  // グループ情報構造体に読み込む

            if(i == gnum){
                break;                                // 指定グループなら抜ける
            }

            adr += read_group_registry->length;                       // グループヘッダ分アドレスＵＰ

            for(j = 0; j < read_group_registry->altogether; j++)      // 中継機情報分アドレスＵＰ
            {
                serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), 2, (char *)&len);  // 中継機情報サイズを読み込む
                adr += (len & 0x0000ffff);
            }

            for(j = 0; j < read_group_registry->remote_unit; j++)     // 子機情報分アドレスＵＰ
            {
                serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), 2, (char *)&len);  // 子機情報サイズを読み込む
                adr += (len & 0x0000ffff);
            }
        }

        tx_mutex_put(&mutex_sfmem);
    }

    if(i != gnum){
        adr = 0;                                      // 見つからなかった場合
    }

    return((uint16_t)adr);
}
// sakaguchi 2020.09.01 ↑

// 2023.05.26 ↓
/**
 * 登録ファイルから無線通信IDを指定してグループ番号を取得する
 * @param id  無線通信ID
 * @return      グループ番号 0:該当なし、1～4
 * @note
 */
uint8_t RF_get_regist_group_no(char *id){

    uint8_t i, j, max;
    uint8_t grp_no = 0;                         // グループ番号
    uint32_t adr, len;
    def_group_registry  read_group_registry;    // グループ情報

    max = RF_get_regist_group_size(&adr);   // グループ数取得

    if(max == 0){           // グループ無し
        return(grp_no);
    }

    if(tx_mutex_get(&mutex_sfmem, WAIT_100MSEC) == TX_SUCCESS)              // シリアルフラッシュ ミューテックス確保 スレッド確定
    {
        for(i=1; i<=max; i++)
        {
            serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), sizeof(def_group_registry), &read_group_registry);  // グループ情報取得

            if(memcmp(id, read_group_registry.id, 8) == 0){
                grp_no = i;
                break;                                // 指定した無線通信IDと一致なら抜ける
            }

            adr += read_group_registry.length;                       // グループヘッダ分アドレスＵＰ

            for(j = 0; j < read_group_registry.altogether; j++)      // 中継機情報分アドレスＵＰ
            {
                serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), 2, (char *)&len);  // 中継機情報サイズを読み込む
                adr += (len & 0x0000ffff);
            }

            for(j = 0; j < read_group_registry.remote_unit; j++)     // 子機情報分アドレスＵＰ
            {
                serial_flash_multbyte_read((uint32_t)(SFM_REGIST_START + adr), 2, (char *)&len);  // 子機情報サイズを読み込む
                adr += (len & 0x0000ffff);
            }
        }

        tx_mutex_put(&mutex_sfmem);
    }

    return(grp_no);

}
// 2023.05.26 ↑
