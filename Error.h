/**
 * @file    Error.h
 *
 * @date 2019/02/01
 * @author  haya
 * @note	2020.Aug.07	0807ソース反映済み

 */

#ifndef ERROR_H_
#define ERROR_H_

#ifdef EDF
#undef EDF
#endif

#ifdef ERROR_C_
#define EDF
#else
#define EDF extern
#endif



//********************************************************
//  マクロ
//********************************************************

#define ERR( CAT, NAME )        ( (ERRCAT_BASE_ ## CAT) | (ERRCODE_ ## CAT ## _ ## NAME) )
//#define   SET_ERR( CAT, NAME )    ERRORCODE. ## CAT = ( (ERRCAT_BASE_ ## CAT) | (ERRCODE_ ## CAT ## _ ## NAME) )
#define SET_ERR( CAT, NAME )    ERRORCODE.ERR = ( (ERRCAT_BASE_ ## CAT) | (ERRCODE_ ## CAT ## _ ## NAME) )

// エラーコードのベースコードを定義
#define ERRCAT_BASE_ERR     0x00000000      ///< エラーコードのベースコード 全体
#define ERRCAT_BASE_SYS     0x00010000      ///< エラーコードのベースコード システム全般
#define ERRCAT_BASE_CMD     0x00020000      ///< エラーコードのベースコード コマンド
#define ERRCAT_BASE_GSM     0x00030000      ///< エラーコードのベースコード GSM
#define ERRCAT_BASE_RF      0x00040000      ///< エラーコードのベースコード RF
#define ERRCAT_BASE_IR      0x00050000      ///< エラーコードのベースコード IR
#define ERRCAT_BASE_GPS     0x00060000      ///< エラーコードのベースコード GPS
#define ERRCAT_BASE_LAN     0x00070000      ///< エラーコードのベースコード LAN
#define ERRCAT_BASE_NET     0x00090000      ///< エラーコードのベースコード NET
#define ERRCAT_BASE_UNKOWN  0x270F0000      ///< エラーコードのベースコード 未定義(9999-****)

//=======================================================
/// エラーオブジェクトの型定義
//=======================================================
typedef struct{
    ULONG   ERR;
    ULONG   ACT0;
} ERROR_OBJ;


//=======================================================
///エラーコード NET
//=======================================================
enum {
    ERRCODE_NET_NOERROR = 0,
};

//=======================================================
///エラーコード 全般
//=======================================================
enum {
    ERRCODE_ERR_NOERROR = 0,
};

//=======================================================
///エラーコード システム
//=======================================================
enum {
    ERRCODE_SYS_NOERROR = 0,

//----- CODE 1～10 ----------------------
    ERRCODE_SYS_SRAM,           ///< SRAM
    ERRCODE_SYS_EEPROM,         ///< EEPROM
    ERRCODE_SYS_LOWBATTERY,     ///< バッテリ電圧が低い
};

//=======================================================
///エラーコード コマンド
//=======================================================
enum {
    ERRCODE_CMD_NOERROR = 0,

//----- CODE 1～10 ----------------------
    ERRCODE_CMD_FORMAT,         ///< コマンドフォーマットエラー
    ERRCODE_CMD_CHECKSUM,       ///< チェックサムエラー
    ERRCODE_CMD_TIMEOUT,        ///< 一定時間内にコマンドが終了しなかった
    ERRCODE_CMD_RUNNING,        ///< コマンド実行中
    ERRCODE_CMD_PASS = 8,       ///< パスワード不一致

    ERRCODE_CMD_SW_RUN = 200,   ///< SW-RUN で実行できない


};

//=======================================================
///エラーコード  RF
//=======================================================
enum {
    ERRCODE_RF_NOERROR = 0,

//----- CODE 1～10 ----------------------
    ERRCODE_RF_BUSY,            ///< 無線通信中
    ERRCODE_RF_PROTECT,         ///< 子機記録開始プロテクト
    ERRCODE_RF_CANCEL,          ///< 無線通信途中キャンセルされた
    ERRCODE_RF_RPT_ERR,         ///< 中継機間通信エラー
    ERRCODE_RF_UNIT_ERR,        ///< 子機間通信エラー
    ERRCODE_RF_ToTERR,          ///< タイムアウトエラー
    ERRCODE_RF_RECTIME,         ///< 記録開始までの秒数が足りなかった場合

    ERRCODE_RF_PARAMISS,        ///< 引き数が不正な値だった
    ERRCODE_RF_NOTDATA,         ///< データが存在しない場合

// 2009/04/27 以降
    ERRCODE_RF_NOTFINISH,       ///< 吸い上げが終了していない(無線又はGSM通信でエラーになった子機がある)

//----- CODE 11～20 ----------------------

// 2009/05/21 以降
    ERRCODE_RF_MONI_ERR,        ///< 現在値送信でGSM通信エラーで通信できなかった。
    ERRCODE_RF_W_M_NOTFINISH,   ///< 吸い上げが終了していない&現在値送信でGSM通信エラーで通信できなかった。(上記2個の複合)

//----- CODE 21～30 ----------------------

    ERRCODE_RF_INUSE = 21,      ///< チャンネルが使用中で無線通信出来ない状態
    ERRCODE_RF_NOBAND,          ///< BAND指定されたがモジュールが未対応
    ERRCODE_RF_BREAK_END,       // 自律動作が途中で緊急停止した


//----- CODE 99 --------------------------

    ERRCODE_RF_OTHER_ERR = 99,  ///< その他のエラー
};


//=======================================================
///エラーコード   IR
//=======================================================
enum {
    ERRCODE_IR_NOERROR = 0,

//----- CODE 1～10 ----------------------
    ERRCODE_IR_NORES,           ///< 光通信の応答が無かった
    ERRCODE_IR_PARAMISS,        ///< 引き数が不正な値だった
    ERRCODE_IR_RESERR,          ///< 子機からの応答が不正だった
    ERRCODE_IR_REFUSE,          ///< 子機がコマンドを拒否した

//----- 内部エラー 90～ ----------------------
    ERRCODE_IR_NOTDATA = 90,    ///< 子機からの応答データが無い場合

};

//=======================================================
///エラーコード   LAN 7
//=======================================================
enum {
	ERRCODE_LAN_NOERROR = 0,


//----- CODE 1～10 ----------------------
	ERRCODE_LAN_BUSY = 1,			///< ネットワーク動作中
	ERRCODE_LAN_IP_ERROR,			///< IPアドレスが一致しない(ETLANのみ）
	ERRCODE_LAN_DHCP,				///< DHCP失敗
	ERRCODE_LAN_NONSP,				///< H/W未サポート
	ERRCODE_LAN_INIT,				///< ネットワーク初期化中
	ERRCODE_LAN_REBOOT,				///< 要再起動               // sakaguchi 2020.09.16


//----- CODE 40 ----------------------
	ERRCODE_LAN_OTHER = 40,		///< その他のエラー

//----- CODE 41～571 ----------------------
// iChip のエラーコードをそのまま返す

};



/// エラーオブジェクト
EDF ERROR_OBJ   ERRORCODE;



#endif /* ERROR_H_ */
