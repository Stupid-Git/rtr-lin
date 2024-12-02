/*
 * Lang.h
 *
 *  Created on: 2019/09/09
 *      Author: tooru.hayashi
 */

#ifndef _LANG_H_
#define _LANG_H_



#ifdef	EDF
#undef	EDF
#endif

#ifdef	_LANG_C_
#define	EDF							// ソース中のみ extern を削除
#else
#define	EDF	extern					// 外部変数
#endif

//********************************************************

#define		LANG_TABLE_US		0			// テーブル番号
#define		LANG_TABLE_JP		1			// 0:US/EU, 1:JP

///言語テーブル取得マクロ（配列）
#define		LANG(KEY)	LangTable[ LANG_ ## KEY ][LangFlag]
///言語テーブル取得マクロ（ポインタ）
#define     pLANG(KEY)   (char *)&LangTable[ LANG_ ## KEY ][LangFlag]

EDF	int		LangFlag;

//********************************************************

/**
 * 言語テーブル（English/日本語）
 * @attention   LangTableの順番にあわせること
 */
enum
{
	LANG_BASE_UNIT		= 0,		// 0
	LANG_REMOTE_UNIT,				// 1

	LANG_COMERR,					// 2
	LANG_SNSERR,					// 3
	LANG_NODATA,					// 4
	LANG_UNDERERR,					// 5
	LANG_OVERERR,					// 6

	LANG_CURRENT_OK,				// 7
	LANG_RECORD_OK,					// 8
	LANG_END_EMAIL,					// 9

//----- 10

	LANG_W_LIMIT,					// 10
	LANG_W_RECOVER,					// 11
	LANG_W_BACK,					// 12
	LANG_W_OTHER,					// 13

	LANG_W_EDGE,					// 14
	LANG_W_FALL,					// 15
	LANG_W_RISE,					// 16

	LANG_W_SENSOR,					// 17
	LANG_W_OVER,					// 18
	LANG_W_UNDER,					// 19

//----- 20

	LANG_W_ERR_SENSOR,				// 20
	LANG_W_ERR_RF,					// 21
	LANG_W_ERR_BATT,				// 22
	LANG_W_OK_SENSOR,				// 23
	LANG_W_OK_RF,					// 24
	LANG_W_OK_BATT,					// 25

	LANG_INPUT_ON,					// 26
	LANG_INPUT_OFF,					// 27

	LANG_W_TEST,					// 28

	LANG_WARN_UNKNOWN,				// 29

};

#ifdef	_LANG_C_

/**
 * 言語テーブル本体（English/日本語）
 */
const char *const LangTable[][2] =
{
	{ "Base Unit",		"親機" },
	{ "Remote Unit",	"子機" },

	{ "ComErr ",	"通信エラー " },
	{ "SnsErr ",	"センサエラー " },
	{ "NoData ",	"データ無し " },
	{ "Under ",		"下限 " },
	{ "Over ",		"上限 " },

	{ "Current Readings Test OK",	"現在値テスト ＯＫ" },
	{ "Recorded Data Test OK",		"記録データテスト ＯＫ" },

	{ "End of mail",		"End of mail"		},

//----- 10

	{ "Warning:Up/Lo Limit","上下限値警報"		},
	{ "Recovery",			"復帰"				},
	{ "Back to Norm",		"正常復帰"			},
	{ "Other",				"その他"			},


	{ " Edge",				""					},
	{ "Falling",			"立ち下がり"		},
	{ "Rising",				"立ち上がり"		},

	{ "N/A ",				"センサエラー "		},
	{ "Over ",				"範囲外(Over) "		},
	{ "Under ",				"範囲外(Under) "	},

//----- 20

	{ "Sensor Fail",		"センサNG"			},
	{ "RF Com Err",			"無線通信エラー"	},
	{ "Batt LO",			"電池残量少"		},
	{ "Sensor OK",			"センサOK"			},
	{ "RF Com OK",			"無線通信OK"		},
	{ "Batt OK",			"電池残量OK"		},

	{ "Input ON",			"接点 ON"			},
	{ "Input OFF",			"接点 OFF"			},

	{ "Warning Report Test","警報テスト"		},
	{ "More than 7 days ago","7日以上前"		},

};

#else

extern const char *const LangTable[][2];

#endif





#endif /* _LANG_H_ */
