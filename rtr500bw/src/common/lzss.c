/**
 ----------------------------------------------------------------------------
    @brief          LZSS圧縮、解凍処理
    @file           lzss.c
    @author         Takashi.S
    @date           Apr.5 2017
    @note           STM32L475/476用 Rev.0 を移植

 ----------------------------------------------------------------------------
*/
#define _LZSS_C_

#include "lzss.h"
#include <string.h>

uint16_t NumCounter[256];                                       // １バイトで表現できる数値は０～２５５の２５６種類


// 2022.06.09 関数修正（圧縮情報の開始を示す数値（EncodeInfo.EncodeCode）が暗黙の型変換で整数拡張され正しく解凍できない問題が発生）
/**
 * ＬＺＳＳ圧縮
 * @param Src       ソースアドレス
 * @param SrcSize   ソースバイト数
 * @param Dest      圧縮データ格納アドレス
 * @return          圧縮データサイズ
 */
uint32_t LZSS_Encode(uint8_t *SrcPoint, uint32_t SrcSize, uint8_t *DestPoint)
{
	int32_t Index, EqualNum, MaxEqualNum, MaxIndex;
	
    uint32_t MinNum, i, t, PressSizeCounter, SrcSizeCounter;

    uint8_t *PressData, EncodeCode;

    struct LZSS_ENCODE_INFO EncodeInfo;

    // 圧縮元データの中で一番出現頻度が低い数値を算出する
    {
        // カウンターを初期化
        for(i = 0; i < 256; i++) NumCounter[i] = 0;

        // 出現回数をカウント
        for(i = 0; i < SrcSize; i++) NumCounter[SrcPoint[i]]++;

        // 最初は仮に０番を開始を示す数値としておく
        EncodeCode = 0;
        MinNum = NumCounter[0];

        for(i = 1; i < 256; i++)
        {
            t = NumCounter[i];

            // より出現数が低い数値が見つかったら更新
            if(MinNum > t)
            {
                MinNum = t;
                EncodeCode = (uint8_t)i;    
            }
        }
    }

    // 圧縮データを格納するアドレスをセット(圧縮データ本体は元のサイズ、圧縮後のサイズ、圧縮情報の開始を示す数値を格納するデータ領域の後になる)
    PressData = DestPoint + sizeof(EncodeInfo);

    // 圧縮するデータの参照アドレスを初期化
    SrcSizeCounter = 0;

    // 圧縮したデータの格納アドレスを初期化
     PressSizeCounter = 0;

    // 全てのデータを圧縮するまで繰り返す
    while(SrcSizeCounter < SrcSize)
    {
        // 今までに同じ数値の羅列がなかったか調べる
        MaxEqualNum = MaxIndex = -1;

        // 最高で２５４バイト前まで調べる
        //for(Index = 1; Index <= 254; Index++)

        // ３２バイト前まで調べる
        for(Index = 4; Index <= 32; Index++)
        {
            // データの先頭より前を調べようとしていたら抜ける
            if(SrcSizeCounter < (uint32_t)Index) break;

            // 同じ数値が何回続いているか調べる
            for(EqualNum = 0; EqualNum < Index; EqualNum++)
            {
                t = SrcSizeCounter + (uint32_t)EqualNum;

                // 数値が違ったらループを抜ける
                if(SrcPoint[t] != SrcPoint[t - (uint32_t)Index]) break;

                // 圧縮元データの最後に到達したらループを抜ける
                if(t >= SrcSize) break;
            }

            // 同じだった回数が４以上(４未満だと逆にサイズが増える)で、且、今まで見つけた回数よりも多い場合に参照アドレスを更新する
            if((EqualNum >= 4) && (MaxEqualNum < EqualNum))
            {
                MaxEqualNum = EqualNum;
                MaxIndex = Index;
            }
        }

        if(MaxIndex == -1)
        {
            t = SrcPoint[SrcSizeCounter];

            // 同じ数値の羅列が見つからなかったら普通に出力
            PressData[PressSizeCounter++] = (uint8_t)t;

            // 圧縮情報の開始を示す数値と同じだった場合は２回連続で出力する
            if((uint8_t)t == EncodeCode) PressData[PressSizeCounter++] = (uint8_t)t;

            // 圧縮元データの参照アドレスを一つ進める
            SrcSizeCounter++;
        }
        else
        {
            // 見つかった場合は見つけた位置と長さを出力する

            // 最初に圧縮情報の開始を示す数値を出力する
            PressData[PressSizeCounter++] = EncodeCode;

            // 次に『何バイト前からが同じか？』の数値を出力する
            PressData[PressSizeCounter] = (uint8_t)MaxIndex;

            // もし圧縮情報の開始を示す数値と同じ場合、展開時に『圧縮情報の開始を示す数値そのもの』と判断されてしまうため、圧縮情報の開始を示す数値と同じかそれ以上の場合は数値を +1 するというルールを使う。(展開時は逆に -1 にする)
            if(PressData[PressSizeCounter] >= EncodeCode) PressData[PressSizeCounter]++;

            PressSizeCounter++;

            // 次に『何バイト同じか？』の数値を出力する
            PressData[PressSizeCounter++] = (uint8_t)MaxEqualNum;

            // 同じだったバイト数分だけ圧縮元データの参照アドレスを進める
            SrcSizeCounter += (uint8_t)MaxEqualNum;
        }
    }

    // 圧縮情報の開始を知らせる数値をセット
    EncodeInfo.EncodeType = 0xe0;
    EncodeInfo.EncodeCode = EncodeCode;
    EncodeInfo.PressSize = (uint16_t)(PressSizeCounter + sizeof(EncodeInfo));

    // 圧縮データの情報を圧縮データにコピーする
    memcpy(DestPoint, &EncodeInfo, sizeof(EncodeInfo));

    // 圧縮後のデータのサイズを返す
    return (EncodeInfo.PressSize);
}


// 2022.06.09 関数修正（圧縮情報の開始を示す数値（EncodeInfo.EncodeCode）が暗黙の型変換で整数拡張され正しく解凍できない問題が発生）
/**
 * ＬＺＳＳ解凍
 * @param Press 圧縮データ格納アドレス
 * @param Dest  解凍データ格納アドレス
 * @return      解凍データサイズ
 */
uint32_t LZSS_Decode(uint8_t *PressPoint, uint8_t *DestPoint)
{
	uint32_t PressSize, PressSizeCounter, DestSizeCounter, Index, EqualNum;

    uint8_t *PressData;

    struct LZSS_ENCODE_INFO EncodeInfo;


    // 圧縮データの情報を取得する
    memcpy(&EncodeInfo, PressPoint, sizeof(EncodeInfo));

    // 圧縮データ本体のサイズを取得する
    PressSize = (EncodeInfo.PressSize - sizeof(EncodeInfo));

    // 圧縮データ本体の先頭アドレスをセット(圧縮データ本体は元のサイズ、圧縮後のサイズ、圧縮情報の開始を示す数値を格納するデータ領域の後にある)
    PressData = PressPoint + sizeof(EncodeInfo);

    // 解凍したデータを格納するアドレスの初期化
    DestSizeCounter = 0;

    // 解凍する圧縮データの参照アドレスの初期化
    PressSizeCounter = 0;

    // 全ての圧縮データを解凍するまでループ
    while(PressSizeCounter < PressSize)
    {
        // 圧縮情報の開始を示す数値かどうかで処理を分岐
        if(PressData[PressSizeCounter] == EncodeInfo.EncodeCode)
        {
            PressSizeCounter++;

            // ２バイト連続で圧縮情報の開始を示す数値だった場合、開始を示す数値そのものを示しているのでそのまま出力する
            if(PressData[PressSizeCounter] == EncodeInfo.EncodeCode)
            {
                DestPoint[DestSizeCounter] = (uint8_t)EncodeInfo.EncodeCode;
                DestSizeCounter++;
                PressSizeCounter++;
            }
            else
            {
                // 普通に圧縮情報を示す数値だった場合『何バイト前から？』の数値を得る
                {
                    Index = PressData[PressSizeCounter];
                    PressSizeCounter++;

                    // 『何バイト前から？』の数値が圧縮情報を示す数値より大きい値の場合は－１する
                    if(Index > EncodeInfo.EncodeCode) Index--;	// EncodeInfo.EncodeCodeは最小0だからIndexはマイナスにはならない
                }

                // 『何バイト同じか？』の数値を得る
                EqualNum = PressData[PressSizeCounter];
                PressSizeCounter++;

                // 指定のバイト数分だけ前のアドレスから、指定のバイト数分だけコピー
                memcpy(&DestPoint[DestSizeCounter], &DestPoint[DestSizeCounter - Index], (uint32_t)EqualNum);
                DestSizeCounter += EqualNum;
            }
        }
        else
        {
            // 普通に数値を出力
            DestPoint[DestSizeCounter] = PressData[PressSizeCounter];

            DestSizeCounter++;
            PressSizeCounter++;
        }
    }

    // 解凍後のサイズを返す
    return (DestSizeCounter);
}

/* end of file */
