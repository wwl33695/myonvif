#include "rtsp_stream.h"
#include <unistd.h>

extern "C" {
#include <libavcodec/avcodec.h>
//#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
//#include <libavfilter/avfilter.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
//#include <libavutil/pixdesc.h>
}

RtspStream::RtspStream()
{
    m_ifmt_ctx = NULL;
    m_avctx = NULL;
    m_swsContext = NULL;
    m_video_index = -1;
    m_stop = false;

    av_register_all();
    avformat_network_init();
}

RtspStream::~RtspStream()
{
    if (m_avctx != NULL) {
        avcodec_free_context(&m_avctx);
        m_avctx = NULL;
    }

    if (m_ifmt_ctx != NULL) {
        avformat_close_input(&m_ifmt_ctx);
        m_ifmt_ctx = NULL;
    }
}

bool RtspStream::OpenUrl(const char* url)
{
    char    errbuf[64];
    int ret = avformat_open_input(&m_ifmt_ctx, url, 0, NULL);
    if (ret < 0) {
        printf("avformat_open_input '%s' (error '%s')\n", url, av_make_error_string(errbuf, sizeof(errbuf), ret));
        return false;
    }

    ret = avformat_find_stream_info(m_ifmt_ctx, NULL);
    if (ret < 0) {
        printf("avformat_find_stream_info (error '%s')\n", av_make_error_string(errbuf, sizeof(errbuf), ret));
        return false;
    }

    for (unsigned int i = 0; i < m_ifmt_ctx->nb_streams; i++) {
        av_dump_format(m_ifmt_ctx, i, url, 0);                                // dump information
        AVStream *st = m_ifmt_ctx->streams[i];
        switch(st->codecpar->codec_type) {
        case AVMEDIA_TYPE_AUDIO:
            break;
        case AVMEDIA_TYPE_VIDEO:
            m_video_index = i;
            m_avctx = GetCodec(st);
            break;
        default: break;
        }
    }
    if (m_avctx == NULL) {
        printf("No H.264 video stream in the input file\n");
        return false;
    }

    m_read = std::thread(ReadThread, this);
    m_decode = std::thread(DecodeThread, this);

    return true;
}

bool RtspStream::GetInfo(int *width, int *height)
{
    if (m_avctx == NULL) {
        return false;
    }

    *width = m_avctx->width;
    *height = m_avctx->height;
    return true;
}

bool RtspStream::GetRGBData(uint8_t *data, int width, int height)
{
    AVFrame *frame = NULL;
    while (frame == NULL) {
        std::lock_guard<std::mutex> lock(m_mtx);
        if (!m_frames.empty()) {
            frame = m_frames.front();
            m_frames.pop_front();
            break;
        }
    }

    if (frame == NULL) {
        return false;
    }

    if (frame->format != AV_PIX_FMT_BGR24 || frame->width != width || frame->height != height) {
        m_swsContext = sws_getCachedContext(m_swsContext,
                                            frame->width, frame->height, AV_PIX_FMT_YUV420P,
                                            width, height, AV_PIX_FMT_BGR24,
                                            0, NULL, NULL, NULL);
        uint8_t *pixels[4] = { data };
        int pitch[4] = { width * 3 };
        sws_scale(m_swsContext, (const uint8_t *const *)frame->data, frame->linesize,
                  0, frame->height, pixels, pitch);

        if (frame->pts == 93600) {
            FILE *file = fopen("1.rgb", "w+");
            fwrite(data, width * height * 3, 1, file);
            fclose(file);
        }

    } else {

    }

    av_frame_free(&frame);
    return true;
}

AVCodecContext *RtspStream::GetCodec(AVStream *st)
{
    int ret = 0;
    AVCodec *codec = NULL;
    const char *forced_codec_name = NULL;

    AVCodecContext *avctx = avcodec_alloc_context3(NULL);
    if (!avctx) {
        printf("avcodec_alloc_context3 error\n");
        return NULL;
    }

    ret = avcodec_parameters_to_context(avctx, st->codecpar);
    if (ret < 0) {
        printf("avcodec_parameters_to_context error\n");
        goto fail;
    }

    avctx->pkt_timebase = st->time_base;

    if (avctx->codec_id == AV_CODEC_ID_MJPEG) {
        forced_codec_name = "mjpeg_cuvid";
    } else if (avctx->codec_id == AV_CODEC_ID_H264) {
        forced_codec_name = "h264_cuvid";
    } else if (avctx->codec_id == AV_CODEC_ID_HEVC) {
        forced_codec_name = "hevc_cuvid";
    } else if (avctx->codec_id == AV_CODEC_ID_MPEG4) {
        forced_codec_name = "mpeg4_cuvid";
    }

    if (forced_codec_name) {
        codec = avcodec_find_decoder_by_name(forced_codec_name);
        if (codec) {
            ret = avcodec_open2(avctx, codec, NULL);
            if (ret < 0) {
                char errbuf[64];
                printf("avcodec_open2 %s error '%s'\n", forced_codec_name, av_make_error_string(errbuf, sizeof(errbuf), ret));
                codec = NULL;
            }
        }
    }

    if (!codec) {
        codec = avcodec_find_decoder(avctx->codec_id);
    }
    if (!codec) {
        printf("avcodec_find_decoder error\n");
        goto fail;
    }

    avctx->codec_id = codec->id;

    ret = avcodec_open2(avctx, codec, NULL);
    if (ret < 0) {
        char errbuf[64];
        printf("avcodec_open2 error '%s'\n", av_make_error_string(errbuf, sizeof(errbuf), ret));
        goto fail;
    }

    goto out;

fail:
    avcodec_free_context(&avctx);
    avctx = NULL;

out:
    return avctx;
}

void RtspStream::ReadThread(RtspStream *that)
{
    AVPacket *pkt = NULL;
    while (!that->m_stop) {
        if (pkt == NULL) {
            pkt = new AVPacket;
        }

        if (pkt == NULL) {
            printf("new AVPacket error, exit thread!!!");
            break;
        }

        int ret = av_read_frame(that->m_ifmt_ctx, pkt);
        if (ret < 0) {
            char errbuf[64];
            printf("av_read_frame error '%s', exit thread!!!\n", av_make_error_string(errbuf, sizeof(errbuf), ret));
            break;
        } else if (ret == AVERROR(EAGAIN)) {
            usleep(10 * 1000);
            continue;
        } else if (pkt->stream_index != that->m_video_index) {
            av_packet_unref(pkt);
        } else {
            std::lock_guard<std::mutex> lock(that->m_mtx);
            that->m_pkts.push_back(pkt);
            pkt = NULL;
        }
    }

    if (pkt) {
        delete pkt;
        pkt = NULL;
    }
}

void RtspStream::DecodeThread(RtspStream *that)
{
    AVFrame *frame = av_frame_alloc();
    while (!that->m_stop) {
        if (frame == NULL) {
            frame = av_frame_alloc();
        }

        if (frame == NULL) {
            printf("av_frame_alloc error, exit thread!!!");
            break;
        }

        while (avcodec_receive_frame(that->m_avctx, frame) == 0) {
            printf("frame pts:%ld format:%d\n", frame->pts, frame->format);

            {
                std::lock_guard<std::mutex> lock(that->m_mtx);
                that->m_frames.push_back(frame);
            }
            frame = av_frame_alloc();
            if (frame == NULL) {
                printf("av_frame_alloc error!!!");
                break;
            }
        }

        AVPacket *pkt = NULL;
        {
            std::lock_guard<std::mutex> lock(that->m_mtx);
            if (!that->m_pkts.empty()) {
                pkt = that->m_pkts.front();
                that->m_pkts.pop_front();
            }
        }

        if (pkt) {
            avcodec_send_packet(that->m_avctx, pkt);
            av_packet_unref(pkt);
            delete pkt;
            pkt = NULL;
        }
    }
}
