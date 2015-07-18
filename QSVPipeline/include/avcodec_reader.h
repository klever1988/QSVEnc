﻿//  -----------------------------------------------------------------------------------------
//    QSVEnc by rigaya
//  -----------------------------------------------------------------------------------------
//   ソースコードについて
//   ・無保証です。
//   ・本ソースコードを使用したことによるいかなる損害・トラブルについてrigayaは責任を負いません。
//   以上に了解して頂ける場合、本ソースコードの使用、複製、改変、再頒布を行って頂いて構いません。
//  ---------------------------------------------------------------------------------------
#ifndef _AVCODEC_READER_H_
#define _AVCODEC_READER_H_

#include "sample_utils.h"

#if ENABLE_AVCODEC_QSV_READER
#include "avcodec_qsv.h"

using std::vector;

static const mfxU32 AVCODEC_READER_INPUT_BUF_SIZE = 16 * 1024 * 1024;

enum {
    AVQSV_AUDIO_NONE         = 0x00,
    AVQSV_AUDIO_MUX          = 0x01,
    AVQSV_AUDIO_COPY_TO_FILE = 0x02,
};

enum : mfxU32 {
    AVQSV_PTS_SOMETIMES_INVALID = 0x01, //時折、無効なptsやdtsを得る
    AVQSV_PTS_HALF_INVALID      = 0x02, //PAFFなため、半分のフレームのptsやdtsが無効
    AVQSV_PTS_ALL_INVALID       = 0x04, //すべてのフレームのptsやdtsが無効
    AVQSV_PTS_NONKEY_INVALID    = 0x08, //キーフレーム以外のフレームのptsやdtsが無効
};

//フレームの位置情報と長さを格納する
typedef struct FramePos {
    int64_t pts;  //pts
    int64_t dts;  //dts
    int duration; //該当フレームの表示時間
    int flags;    //flags
} FramePos;

//動画フレームのデータ
typedef struct VideoFrameData {
    FramePos *frame;      //各フレームの情報への配列 (デコードが開始されるフレームから取得する)
    int fixed_num;        //frame配列でフレーム順序が確定したフレーム数
    int num;              //frame配列で現在格納されているデータ数
    int capacity;         //frame配列を確保した数
    int64_t duration;     //合計の動画の長さ
    CRITICAL_SECTION cs;  //frame配列アクセスへの排他制御用
    bool cs_initialized;  //csが初期化されているかどうか
} VideoFrameData;

typedef struct AVDemuxFormat {
    AVFormatContext          *pFormatCtx;            //動画ファイルのformatContext
    int                       nAnalyzeSec;           //動画ファイルを先頭から分析する時間
    bool                      bIsPipe;               //入力がパイプ
    uint32_t                  nPreReadBufferIdx;     //先読みバッファの読み込み履歴
} AVDemuxFormat;

typedef struct AVDemuxVideo {
    AVCodecContext           *pCodecCtx;             //動画のcodecContext
    int                       nIndex;                //動画のストリームID
    int64_t                   nStreamFirstPts;       //動画ファイルの最初のpts
    mfxU32                    nStreamPtsInvalid;     //動画ファイルのptsが無効 (H.264/ES, 等)
    bool                      bGotFirstKeyframe;     //動画の最初のキーフレームを取得済み
    VideoFrameData            frameData;             //動画フレームのptsのリスト
    AVBitStreamFilterContext *pH264Bsfc;             //必要なら使用するbitstreamfilter
    mfxU8                    *pExtradata;            //動画のヘッダ情報
    int                       nExtradataSize;        //動画のヘッダサイズ
    AVPacket                  packet[2];             //取得した動画ストリームの1フレーム分のデータ
    AVRational                nAvgFramerate;         //動画のフレームレート
    bool                      bUseHEVCmp42AnnexB;    //HEVCのmp4->AnnexB変換

    mfxU32                    nSampleLoadCount;      //sampleをLoadNextFrameでロードした数
    mfxU32                    nSampleGetCount;       //sampleをGetNextBitstreamで取得した数
} AVDemuxVideo;

typedef struct AVDemuxAudio {
    int                       nIndex;                 //音声のストリームID (libavのストリームID)
    int                       nTrackId;               //音声のトラックID (QSVEncC独自, 1,2,3,...)
    AVCodecContext           *pCodecCtx;              //音声のcodecContext
    int                       nLastVidIndex;          //音声の直前の相当する動画の位置
    mfxI64                    nExtractErrExcess;      //音声抽出のあまり (音声が多くなっていれば正、足りなくなっていれば負)
    AVPacket                  pktSample;              //サンプル用の音声データ
    int                       nDelayOfAudio;          //音声側の遅延 (pkt_timebase基準)
} AVDemuxAudio;

typedef struct AVDemuxer {
    AVDemuxFormat        format;
    AVDemuxVideo         video;
    vector<AVDemuxAudio> audio;
} AVDemuxer;

typedef struct AvcodecReaderPrm {
    mfxU8          memType;                 //使用するメモリの種類
    mfxU32         nReadAudio;              //音声の読み込みを行うかどうか (AVQSV_AUDIO_xxx)
    mfxU16         nAnalyzeSec;             //入力ファイルを分析する秒数
    mfxU16         nTrimCount;              //Trimする動画フレームの領域の数
    sTrim         *pTrimList;               //Trimする動画フレームの領域のリスト
    mfxU8          nAudioSelectCount;       //muxする音声のトラック数
    sAudioSelect **ppAudioSelect;           //muxする音声のトラック番号のリスト 1,2,...(1から連番で指定)
} AvcodecReaderPrm;


class CAvcodecReader : public CSmplYUVReader
{
public:
    CAvcodecReader();
    virtual ~CAvcodecReader();

    virtual mfxStatus Init(const TCHAR *strFileName, mfxU32 ColorFormat, const void *option, CEncodingThread *pEncThread, CEncodeStatusInfo *pEncSatusInfo, sInputCrop *pInputCrop) override;

    virtual void Close();

    //動画ストリームの1フレーム分のデータをm_sPacketに格納する
    //m_sPacketからの取得はGetNextBitstreamで行う
    virtual mfxStatus LoadNextFrame(mfxFrameSurface1* pSurface) override;

    //動画ストリームの1フレーム分のデータをbitstreamに追加する
    virtual mfxStatus GetNextBitstream(mfxBitstream *bitstream) override;

    //ストリームのヘッダ部分を取得する
    virtual mfxStatus GetHeader(mfxBitstream *bitstream) override;
    
    //音声パケットの配列を取得する
    vector<AVPacket> GetAudioDataPackets();

    //音声のコーデックコンテキストを取得する
    vector<AVDemuxAudio> GetInputAudioInfo();

    //デコードするストリームの情報を取得する
    void GetDecParam(mfxVideoParam *decParam) {
        memcpy(decParam, &m_sDecParam, sizeof(m_sDecParam));
    }
private:
    //avcodecのコーデックIDからIntel Media SDKのコーデックのFourccを取得
    mfxU32 getQSVFourcc(mfxU32 id);

    //avcodecのストリームIDを取得 (typeはAVMEDIA_TYPE_xxxxx)
    vector<int> getStreamIndex(AVMediaType type);

    //VC-1のスタートコードの確認
    bool vc1StartCodeExists(mfxU8 *ptr);

    //動画のptsをソートする
    void sortVideoPtsList();

    //動画のptsをリストに加える
    void addVideoPtsToList(FramePos pos);

    //対象ストリームのパケットを取得
    int getSample(AVPacket *pkt);

    //対象の音声パケットを追加するかどうか
    bool checkAudioPacketToAdd(const AVPacket *pkt, AVDemuxAudio *pAudio);

    //対象のパケットの必要な対象のストリーム情報へのポインタ
    AVDemuxAudio *getAudioPacketStreamData(const AVPacket *pkt);

    //bitstreamにpktの内容を追加する
    mfxStatus setToMfxBitstream(mfxBitstream *bitstream, AVPacket *pkt);

    //音声パケットリストを開放
    void clearAudioPacketList(std::vector<AVPacket>& pktList);

    //QSVでデコードした際の最初のフレームのptsを取得する
    //さらに、平均フレームレートを推定する
    //fpsDecoderはdecoderの推定したfps
    mfxStatus getFirstFramePosAndFrameRate(AVRational fpsDecoder, mfxSession session, mfxBitstream *bitstream, const sTrim *pTrimList, int nTrimCount);

    //指定したptsとtimebaseから、該当する動画フレームを取得する
    int getVideoFrameIdx(mfxI64 pts, AVRational timebase, const FramePos *framePos, int framePosCount, int iStart);

    //ptsを動画のtimebaseから音声のtimebaseに変換する
    mfxI64 convertTimebaseVidToAud(mfxI64 pts, const AVDemuxAudio *pAudio);

    //HEVCのmp4->AnnexB簡易変換
    void hevcMp42Annexb(AVPacket *pkt);

    //VC-1のヘッダの修正を行う
    void vc1FixHeader();

    //VC-1のフレームヘッダを追加
    void vc1AddFrameHeader(AVPacket *pkt);

    void CloseAudio(AVDemuxAudio *pAudio);
    void CloseVideo(AVDemuxVideo *pVideo);
    void CloseFormat(AVDemuxFormat *pFormat);

    AVDemuxer        m_Demux;                      //デコード用情報
    vector<mfxU8>    m_hevcMp42AnnexbBuffer;       //HEVCのmp4->AnnexB簡易変換用バッファ
    vector<AVPacket> m_PreReadBuffer;              //解析用に先行取得した映像パケット
    vector<AVPacket> m_AudioPacketsBufferL1[2];    //音声のAVPacketのバッファ (マルチスレッドで追加されてくることに注意する)
    vector<AVPacket> m_AudioPacketsBufferL2;       //音声のAVPacketのバッファ
    mfxU32           m_AudioPacketsBufferL2Used;   //m_AudioPacketsBufferL2のパケットのうち、すでに使用したもの
};

#endif //ENABLE_AVCODEC_QSV_READER

#endif //_AVCODEC_READER_H_
