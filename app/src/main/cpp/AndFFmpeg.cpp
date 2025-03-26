//
// Created by BaiYang on 2023-04-12.
//

//#include <pthread.h>
#include "AndFFmpeg.h"
#include "AndroidLog.h"

AndFFmpeg::AndFFmpeg(AndPlayStatus *playStatus, AndCallJava *callJava, const char *url) {
    this->playStatus = playStatus;
    this->callJava = callJava;
    this->url = url;

    pthread_mutex_init(&seek_mutex, NULL);
    pthread_mutex_init(&init_mutex, NULL);
}

void *demuxFFmpeg(void *handler)
{
    AndFFmpeg *andFFmpeg = (AndFFmpeg *) handler;
    andFFmpeg->demuxFFmpegThead();
    pthread_exit(&andFFmpeg->demuxThead);
}

void AndFFmpeg::prepared() {
    pthread_create(&demuxThead, NULL, demuxFFmpeg, this);
}

// 打开解码器
int AndFFmpeg::openDecoder (AVCodecContext **codecCtx, AVCodecParameters *codecpar) {
    /* ************************ 打开解码器4步曲 ************************ */
    *codecCtx = avcodec_alloc_context3(NULL);
    if(!*codecCtx){
        if(LOG_DEBUG){
            LOGE("Couldn't alloc new decoderCtx");
        }
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }
    if (avcodec_parameters_to_context(*codecCtx, codecpar) < 0) {
        if(LOG_DEBUG){
            LOGE("Couldn't fill decoderCtx");
        }
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }
    AVCodec *decoder = avcodec_find_decoder(codecpar->codec_id);
    if(!decoder){
        if(LOG_DEBUG){
            LOGE("Couldn't find decoder");
        }
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }
    if (avcodec_open2(*codecCtx , decoder, NULL) < 0) {
        LOGE("Couldn't open vAudio codec.\n");
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }
    /* ************************ 打开解码器结束 ************************ */
    return 0;
}

// 解封装
int AndFFmpeg::demuxFFmpegThead() {
    pthread_mutex_lock(&init_mutex);
    // 初始化网络库
    avformat_network_init();

    /* ************************ 解封装3步曲 ************************ */
    // 1.分配解码器上下文
    formatCtx = avformat_alloc_context();
    // 2.打开文件并解析
    if(avformat_open_input(&formatCtx, url, NULL, NULL) != 0){
        LOGE("Couldn't open input stream %s.\n", url);
        return -1;
    }
    // 3.查找流的上下文信息，并填充Stream的MetaData信息
    if (avformat_find_stream_info(formatCtx, NULL) < 0) {
        LOGE("Couldn't find stream information\n");
        return -1;
    }

    // 遍历获取流索引、解码器参数
    for (int i = 0; i < formatCtx->nb_streams; ++i) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (andAudio == NULL) {
                andAudio = new AndAudio(playStatus, formatCtx->streams[i]->codecpar->sample_rate, callJava);
                andAudio->streamIndex = i;
                andAudio->codecpar = formatCtx->streams[i]->codecpar;

                andAudio->time_base = formatCtx->streams[i]->time_base;
                andAudio->duration = formatCtx->duration / AV_TIME_BASE;
                duration = andAudio->duration;
                LOGD("andAudio->time_base  den: %d\n", andAudio->time_base.den);
                LOGD("Found audio stream!\n");
            }
        } else if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (andVideo == NULL) {
                andVideo = new AndVideo(playStatus, callJava);
                andVideo->streamIndex = i;
                andVideo->codecpar = formatCtx->streams[i]->codecpar;

                andVideo->time_base = formatCtx->streams[i]->time_base;
                LOGD("andVideo->time_base  den: %d\n", andVideo->time_base.den);
                int num = formatCtx->streams[i]->avg_frame_rate.num;
                int den = formatCtx->streams[i]->avg_frame_rate.den;
                if (num != 0 && den != 0)
                {
                    int fps = num / den;  // 计算视频的帧率
                    andVideo->defaultDelayTime = 1.0 / fps;  // 音视频同步时，视频的延迟时间
                }
                andVideo->delayTime = andVideo->defaultDelayTime;
                LOGD("Found video stream! %f\n", andVideo->defaultDelayTime);
            }
        }
    }
    if (andAudio->streamIndex == -1) {
        LOGE("Couldn't find a vAudio stream.\n");
        return -1;
    }
    /* ************************** 解封装结束 ************************** */

    if (andAudio != NULL) {
        openDecoder(&andAudio->codecCtx, andAudio->codecpar);
        LOGD("成功打开音频解码器.\n");
    }
    if(andVideo != NULL){
        openDecoder(&andVideo->codecCtx, andVideo->codecpar);
        LOGD("成功打开视频解码器.\n");
    }
    pthread_mutex_unlock(&init_mutex);

    // 回调java层函数，可以将一些状态回调到java层  使用子线程
    // 调用java层的onCallPrepared()
    callJava->onCallPrepared(CHILD_THREAD);

    return 0;
}

// 解码
int AndFFmpeg::start() {
    if(andAudio == NULL) {
        if(LOG_DEBUG) {
            LOGE("andAudio is null");
            return -1;
        }
    }
    andAudio->play();
    andVideo->play();
    andVideo->vAudio = andAudio;  // 用于音视频同步

    while(playStatus != NULL && !playStatus->exit)
    {
        if(playStatus->seek){
            continue;
        }
        if(playStatus->pause){
            av_usleep(500*1000); // 休眠500ms
            continue;
        }
        // 放入队列  40这个值可以设大一点
        if(andAudio->queue->getQueueSize() > 40 || andVideo->queue->getQueueSize() > 40){
            continue;
        }

        AVPacket *avPacket = av_packet_alloc();
        // 用户停止 或者最后一帧 文件末尾  要有清空队列的操作
        if(av_read_frame(formatCtx, avPacket) == 0)
        {
            if(avPacket->stream_index == andAudio->streamIndex)
            {
                andAudio->queue->putAvpacket(avPacket);
            } else if (avPacket->stream_index == andVideo->streamIndex)
            {
                andVideo->queue->putAvpacket(avPacket);
            } else
            {
                av_packet_free(&avPacket);
                av_free(avPacket);
            }
        } else
        {
            av_packet_free(&avPacket);
            av_free(avPacket);

            //特殊情况
            while(playStatus != NULL && !playStatus->exit)
            {
                if(andVideo->queue->getQueueSize() > 0)
                {
                    continue;
                }
                if(andVideo->queue->getQueueSize() > 0)
                {
                    continue;
                }

                playStatus->exit = true;
                break;
            }
        }

        if(playStatus != NULL && playStatus->exit)
        {
            andAudio->queue->clearAvpacket();
            playStatus->exit = true;
        }
    }
    return 0;
}

void AndFFmpeg::pause() {
    playStatus->pause = true;
    playStatus->seek = false;
    playStatus->play = false;
    if(andAudio != NULL)
    {
        andAudio->pause();
    }
    if(andVideo != NULL)
    {
        andVideo->pause();
    }
}

void AndFFmpeg::release() {
    if(LOG_DEBUG)
    {
        LOGD("Begin to release AndFFmpeg.");
    }
    playStatus->exit = true;
    //  队列  stop    exit
    int sleepCount = 0;
    pthread_mutex_lock(&init_mutex);
    while (!exit)
    {
        if(sleepCount > 100)
        {
            exit = true;
        }
        if(LOG_DEBUG)
        {
            LOGD("Wait AndFFmpeg  exit %d", sleepCount);
        }
        sleepCount++;
        av_usleep(1000 * 10); //暂停10毫秒
    }

    if(andAudio != NULL)
    {
        andAudio->release();
        delete(andAudio);
        andAudio = NULL;
    }

    if(andVideo != NULL)
    {
        andVideo->release();
        delete(andVideo);
        andVideo = NULL;
    }

    if(LOG_DEBUG)
    {
        LOGD("AndFFmpeg: release formatCtx.");
    }
    if(formatCtx != NULL)
    {
        avformat_close_input(&formatCtx);
        avformat_free_context(formatCtx);
        formatCtx = NULL;
    }
    if(LOG_DEBUG)
    {
        LOGD("AndFFmpeg: release callJava.");
    }
    if(callJava != NULL)
    {
        callJava = NULL;
    }
    if(LOG_DEBUG)
    {
        LOGD("AndFFmpeg: release playstatus.");
    }
    if(playStatus != NULL)
    {
        playStatus = NULL;
    }
    pthread_mutex_unlock(&init_mutex);
}

void AndFFmpeg::resume() {
    playStatus->pause = false;
    playStatus->seek = false;
    playStatus->play = true;

    if(andAudio != NULL)
    {
        andAudio->resume();
    }
    if(andVideo != NULL)
    {
        andVideo->resume();
    }
}

void AndFFmpeg::seek(jint secds) {
    if (duration <= 0)
    {
        return;
    }

    if (secds >= 0 && secds <= duration)
    {
        pthread_mutex_lock(&seek_mutex);
        playStatus->seek = true;
        int64_t rel = secds * AV_TIME_BASE;  // s    *  us
        // seek 是调用ffmpeg的avformat_seek_file
        avformat_seek_file(formatCtx, -1, INT64_MIN, rel, INT64_MAX, 0);
        if (andAudio != NULL) {
            andAudio->queue->clearAvpacket();
            andAudio->clock = 0;
            andAudio->last_time = 0;
        }
        if (andVideo != NULL) {
            andVideo->queue->clearAvpacket();
        }
        playStatus->seek = false;
        pthread_mutex_unlock(&seek_mutex);
    }
}

void AndFFmpeg::setMute(jint mute) {
    if(andAudio != NULL)
    {
        andAudio->setMute(mute);
    }
}

void AndFFmpeg::setSpeed(float speed) {
    if(andAudio != NULL)
    {
        andAudio->setSpeed(speed);
    }
}

void AndFFmpeg::setTone(float tone) {
    if(andAudio != NULL)
    {
        andAudio->setTone(tone);
    }
}

