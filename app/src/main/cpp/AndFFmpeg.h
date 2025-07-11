//
// Created by BaiYang on 2023-04-12.
//

#ifndef MUSICPLAYER_ANDFFMPEG_H
#define MUSICPLAYER_ANDFFMPEG_H

#include "AndAudio.h"
#include "AndVideo.h"
#include "AndCallJava.h"
#include "AndPlayStatus.h"


extern "C" {
#include "include/libavformat/avformat.h"
}

class AndFFmpeg {
public:
    pthread_t demuxThread;
    pthread_mutex_t seek_mutex;
    pthread_mutex_t init_mutex;
    bool exit = false;

    const char *url = nullptr;
    AVFormatContext *formatCtx = nullptr;

    AndAudio *andAudio = nullptr;
    AndVideo *andVideo = nullptr;
    AndCallJava *callJava = nullptr;
    AndPlayStatus *playStatus = nullptr;

    int duration = 0;

public:
    AndFFmpeg(AndPlayStatus *playStatus, AndCallJava *callJava, const char *url);

    int demuxFFmpegThread();

    // 解封装
    void prepared();

    // 打开解码器
    int openDecoder(AVCodecContext **avCodecContext, AVCodecParameters *codecpar);

    // 解码
    int start();

    void pause();

    void resume();

    void release();

    void seek(jint i);

    void setMute(jint i);

    void setSpeed(float speed);

    void setTone(float tone);
};


#endif //MUSICPLAYER_ANDFFMPEG_H
