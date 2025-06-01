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

    pthread_mutex_init(&seek_mutex, nullptr);
    pthread_mutex_init(&init_mutex, nullptr);
}

void *demuxFFmpeg(void *handler)
{
    AndFFmpeg *andFFmpeg = (AndFFmpeg *) handler;
    andFFmpeg->demuxFFmpegThread();
    LOGD("exit demux thread.");
    pthread_exit(&andFFmpeg->demuxThread);
}

void AndFFmpeg::prepared() {
    LOGD("create demux thread.");
    pthread_create(&demuxThread, nullptr, demuxFFmpeg, this);
}

int AndFFmpeg::openDecoder (AVCodecContext **codecCtx, AVCodecParameters *codecpar) {
    /* ************************ 打开解码器 ************************ */
    *codecCtx = avcodec_alloc_context3(nullptr);
    if(!*codecCtx){
        LOGE("Couldn't alloc new decoderCtx");
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }
    if (avcodec_parameters_to_context(*codecCtx, codecpar) < 0) {
        LOGE("Couldn't fill decoderCtx");
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }
    AVCodec *decoder = avcodec_find_decoder(codecpar->codec_id);
    if(!decoder){
        LOGE("Couldn't find decoder");
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }
    if (avcodec_open2(*codecCtx, decoder, nullptr) < 0) {
        LOGE("Couldn't open codec.\n");
        exit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }
    /* ************************ 打开解码器结束 ************************ */
    return 0;
}

int AndFFmpeg::demuxFFmpegThread() {
    pthread_mutex_lock(&init_mutex);

    avformat_network_init();

    /* ************************ 解封装 ************************ */
    // 1.打开文件，将码流信息填充进AVFormatContext
    formatCtx = avformat_alloc_context();
    if (avformat_open_input(&formatCtx, url, nullptr, nullptr) != 0) {
        LOGE("Couldn't open input stream %s.\n", url);
        return -1;
    }
    // 2.获取码流的完整信息，并填充进AVFormatContext
    if (avformat_find_stream_info(formatCtx, nullptr) < 0) {
        LOGE("Couldn't find stream information\n");
        return -1;
    }

    // 3.遍历所有流，并从AVFormatContext中拿到流的详细信息:解码器参数、时间基等
    for (int i = 0; i < formatCtx->nb_streams; ++i) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (andAudio == nullptr) {
                andAudio = new AndAudio(playStatus, formatCtx->streams[i]->codecpar->sample_rate, callJava);
                andAudio->streamIndex = i;
                andAudio->codecpar = formatCtx->streams[i]->codecpar;
                andAudio->time_base = formatCtx->streams[i]->time_base;
                andAudio->duration = formatCtx->duration / AV_TIME_BASE;
                duration = andAudio->duration;
                LOGD("andAudio->time_base  den: %d, duration: %d\n", andAudio->time_base.den,
                     duration);
            }
        } else if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (andVideo == nullptr) {
                andVideo = new AndVideo(playStatus, callJava);
                andVideo->streamIndex = i;
                andVideo->codecpar = formatCtx->streams[i]->codecpar;
                andVideo->time_base = formatCtx->streams[i]->time_base;

                int num = formatCtx->streams[i]->avg_frame_rate.num;
                int den = formatCtx->streams[i]->avg_frame_rate.den;
                if (num != 0 && den != 0) {
                    int fps = num / den;  // 计算视频的帧率
                    andVideo->defaultDelayTime = 1.0 / fps;  // 音视频同步时，视频的延迟时间
                }
                andVideo->delayTime = andVideo->defaultDelayTime;
                LOGD("andVideo->time_base  den: %d, defaultDelayTime: %f\n",
                     andVideo->time_base.den, andVideo->defaultDelayTime);
            }
        }
    }
    if (andAudio->streamIndex == -1) {
        LOGE("Couldn't find a vAudio stream.\n");
        return -1;
    }
    /* ************************** 解封装结束 ************************** */

    if (andAudio != nullptr) {
        openDecoder(&andAudio->codecCtx, andAudio->codecpar);
        LOGD("success to open audio decoder.\n");
    }
    if (andVideo != nullptr) {
        openDecoder(&andVideo->codecCtx, andVideo->codecpar);
        LOGD("success to open video decoder.\n");
    }
    pthread_mutex_unlock(&init_mutex);

    // 回调java层函数，可以将一些状态回调到java层; 这里是子线程，所以传递参数 CHILD_THREAD
    callJava->onCallPrepared(CHILD_THREAD);

    return 0;
}

int AndFFmpeg::start() {
    if (andAudio == nullptr) {
        LOGE("andAudio is null");
        return -1;
    }
    andAudio->play();
    andVideo->play();
    andVideo->vAudio = andAudio;  // 用于音视频同步

    while (playStatus != nullptr && !playStatus->isExited) {
        if (playStatus->isSeek) {
            continue;
        }
        if (playStatus->isPaused) {
            av_usleep(500 * 1000); // 休眠500ms
            continue;
        }
        // 放入队列  40这个值可以设大一点
        if (andAudio->queue->getQueueSize() > 80 || andVideo->queue->getQueueSize() > 80) {
            continue;
        }

        AVPacket *avPacket = av_packet_alloc();
        // 用户停止 或者最后一帧 文件末尾  要有清空队列的操作
        if (av_read_frame(formatCtx, avPacket) == 0) {
            if (avPacket->stream_index == andAudio->streamIndex) {
                andAudio->queue->putAvpacket(avPacket);
            } else if (avPacket->stream_index == andVideo->streamIndex) {
                andVideo->queue->putAvpacket(avPacket);
            } else {
                av_packet_free(&avPacket);
                av_free(avPacket);
            }
        } else {
            av_packet_free(&avPacket);
            av_free(avPacket);

            //特殊情况
            while (playStatus != nullptr && !playStatus->isExited) {
                if (andVideo->queue->getQueueSize() > 0) {
                    continue;
                }
                if (andVideo->queue->getQueueSize() > 0) {
                    continue;
                }

                playStatus->isExited = true;
                break;
            }
        }

        if (playStatus != nullptr && playStatus->isExited) {
            andAudio->queue->clearAvpacket();
            playStatus->isExited = true;
        }
    }
    return 0;
}

void AndFFmpeg::pause() {
    playStatus->isPaused = true;
    playStatus->isSeek = false;
    playStatus->isPlaying = false;
    if (andAudio != nullptr) {
        andAudio->pause();
    }
    if (andVideo != nullptr) {
        andVideo->pause();
    }
}

void AndFFmpeg::release() {
    playStatus->isExited = true;
    //  队列  stop
    // 通过忙等待（busy-wait）的方式确保资源释放的线程安全性
    int sleepCount = 0;
    pthread_mutex_lock(&init_mutex);
    while (!exit) {
        if (sleepCount > 110) {
            exit = true;
        }
        sleepCount++;
        av_usleep(1000 * 10); //暂停10毫秒
    }

    if (formatCtx != nullptr) {
        avformat_close_input(&formatCtx);
        avformat_free_context(formatCtx);
        formatCtx = nullptr;
    }
    if (LOG_DEBUG) {
        LOGD("AndFFmpeg: release formatCtx.");
    }

    if (andAudio != nullptr) {
        andAudio->release();
        delete(andAudio);
        andAudio = nullptr;
    }
    if (andVideo != nullptr) {
        andVideo->release();
        delete(andVideo);
        andVideo = nullptr;
    }
    if (LOG_DEBUG) {
        LOGD("AndFFmpeg: release andAudio and andVideo.");
    }

    if (callJava != nullptr)
        callJava = nullptr;
    if (LOG_DEBUG) {
        LOGD("AndFFmpeg: release callJava.");
    }

    if (playStatus != nullptr)
        playStatus = nullptr;
    if (LOG_DEBUG) {
        LOGD("AndFFmpeg: release playstatus.");
    }

    if (LOG_DEBUG) {
        LOGD("Release AndFFmpeg.");
    }
    pthread_mutex_unlock(&init_mutex);
}

void AndFFmpeg::resume() {
    playStatus->isPaused = false;
    playStatus->isSeek = false;
    playStatus->isPlaying = true;

    if (andAudio != nullptr) {
        andAudio->resume();
    }
    if (andVideo != nullptr) {
        andVideo->resume();
    }
}

void AndFFmpeg::seek(jint secds) {
    if (duration <= 0)
        return;

    if (secds >= 0 && secds <= duration)
    {
        pthread_mutex_lock(&seek_mutex);
        playStatus->isSeek = true;
        int64_t rel = secds * AV_TIME_BASE;  // s    *  us
        // isSeek 是调用ffmpeg的avformat_seek_file
        avformat_seek_file(formatCtx, -1, INT64_MIN, rel, INT64_MAX, 0);
        if (andAudio != nullptr) {
            andAudio->queue->clearAvpacket();
            andAudio->clock = 0;
            andAudio->last_time = 0;
        }
        if (andVideo != nullptr) {
            andVideo->queue->clearAvpacket();
        }
        playStatus->isSeek = false;
        pthread_mutex_unlock(&seek_mutex);
    }
}

void AndFFmpeg::setMute(jint mute) {
    if (andAudio != nullptr)
    {
        andAudio->setMute(mute);
    }
}

void AndFFmpeg::setSpeed(float speed) {
    if (andAudio != nullptr)
    {
        andAudio->setSpeed(speed);
    }
}

void AndFFmpeg::setTone(float tone) {
    if (andAudio != nullptr)
    {
        andAudio->setTone(tone);
    }
}

