#ifndef __RTSP_STREAM_H__
#define __RTSP_STREAM_H__

#include <list>
#include <mutex>
#include <thread>

struct AVFormatContext;
struct AVCodecContext;
struct SwsContext;
struct AVStream;
struct AVPacket;
struct AVFrame;

class RtspStream {
public:
    RtspStream();
    ~RtspStream();

    bool OpenUrl(const char *url);
    bool GetInfo(int *width, int *height);
    bool GetRGBData(uint8_t *data, int width, int height);

private:
    AVCodecContext *GetCodec(AVStream *st);
    static void ReadThread(RtspStream *that);
    static void DecodeThread(RtspStream *that);

private:
    AVFormatContext *m_ifmt_ctx;
    AVCodecContext *m_avctx;
    SwsContext *m_swsContext;
    int m_video_index;

    std::mutex m_mtx;
    std::list<AVPacket*> m_pkts;
    std::list<AVFrame*> m_frames;

    bool m_stop;
    std::thread m_read;
    std::thread m_decode;
};

#endif
