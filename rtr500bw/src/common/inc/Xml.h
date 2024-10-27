/**
 * Xml.h
 *
 *  Created on: 2019/09/03
 *      Author: tooru.hayashi
 */

#ifndef _XML_H_
#define _XML_H_





#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
#include <stdarg.h>
#include "Version.h"
#include "flag_def.h"
#include "MyDefine.h"
#include "Globals.h"
#include "General.h"
#include "Unicode.h"

#undef EDF

#ifndef _XML_C_
    #define EDF extern
#else
    #define EDF
#endif

//********************************************************
//	マクロ定義
//********************************************************

#define	XML_HEADER	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"

#define	XML_FORM_CURRENT        "<file format=\"current_readings\" version=\""VERSION_XML "\" name=\""
#define	XML_FORM_RECORD         "<file format=\"recorded_data\" version=\"" VERSION_XML "\" name=\""
#define	XML_FORM_PUSH			"<file format=\"push_601\" version=\"" VERSION_PUSH "\" name=\""

#define	XML_FORM_CURRENT_TEST	"<file format=\"current_readings\" version=\"" VERSION_XML "\" sample=\"sample\" name=\""
#define	XML_FORM_RECORD_TEST	"<file format=\"recorded_data\" version=\"" VERSION_XML "\" sample=\"sample\" name=\""
#define	XML_FORM_PUSH_TEST		"<file format=\"push_601\" version=\"" VERSION_PUSH "\" sample=\"sample\" name=\""

#define	XML_FORM_WARN			"<file format=\"warning\" version=\""VERSION_XML_W "\" name=\""
#define	XML_FORM_WARN_TEST		"<file format=\"warning\" version=\""VERSION_XML "\" sample=\"sample\" name=\""
#define	XML_FORM_WARN_DATASRV	"<file format=\"warning\" version=\""VERSION_XML_WDS "\" name=\""		// sakaguchi 2021.03.01

#define	XML_OUT_EMAIL	1
#define	XML_OUT_FTP		2
#define	XML_OUT_NONE	3		// 内部処理


//========================================================
//  XMLオブジェクト
//========================================================

typedef struct {

	void	( *const Create )( int );			// 初期化（コンストラクタ）
	void	( *const OpenTag )( char * );
	void	( *const CloseTag )(void);
	void 	( *const OpenTagAttrib)(char *, char *);
	void	( *const TagOnce )( char *, const char *fmt, ... );
	void	( *const TagOnce2 )( char *, const char *fmt, ... );
	void	( *const TagOnceAttrib )( char *, char *, const char *fmt, ... );
	void	( *const TagOnceEmpty )( bool, char *, const char *fmt, ... );
	void	( *const Entity )( const char *fmt, ... );
	void	( *const Plain  )( const char *fmt, ... );
	void	( *const Base64 )( bool, char *src, int size );
	void	( *const CloseAll )(void);
    int		( *Encode )( char *dst, char *src );
	int		( *Encode2 )( char *dst, char *src );


    void	( *Output )(void);		// バッファ書き出し用コールバック関数
	int     OutFlag;			// Output()でFALSEかどうか

	char	*TagPos;			// 現在オープンされているタグの格納アドレス
	char    *BuffPos;			// 現在書き込まれているバッファの先頭
	char	*FileName;			// XMLファイル名

    uint32_t Size;              // バッファにある有効データ数

} OBJ_XML;
 
#ifndef	_XML_C_
	extern OBJ_XML	XML;
	extern const char Encode[];
#else
	const char Encode[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
#endif


#endif /* _XML_H_ */
