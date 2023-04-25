//
// Created by BaiYang on 2023-04-20.
//

#include "AndVideo.h"

AndVideo::AndVideo(AndPlayStatus *playStatus, AndCallJava *callJava)
{
    this->playStatus = playStatus;
    this->callJava = callJava;
    this->queue = new AndQueue(playStatus);
    pthread_mutex_init(&codecMutex, NULL);
}

AndVideo::~AndVideo()
{

}

static void * decodePlay(void *handler)
{
    AndVideo *andVideo = static_cast<AndVideo *>(handler);

    //  死循环轮训
    while(andVideo->playStatus != NULL && !andVideo->playStatus->exit)
    {
        // 解码 seek puase  队列没有数据
        if(andVideo->playStatus->seek)
        {
            av_usleep(1000 * 100);
            continue;
        }
        if(andVideo->playStatus->pause)
        {
            av_usleep(1000 * 100);
            continue;
        }
        if (andVideo->queue->getQueueSize() == 0) {
            // 队列中无数据，需要休眠  请慢慢等待  回调应用层
            if(!andVideo->playStatus->load)
            {
                andVideo->playStatus->load = true;
                andVideo->callJava->onCallLoad(CHILD_THREAD, true);
                av_usleep(1000 * 100);
                continue;
            }
        }

        // 视频解码 比较耗时  多线程 需要加锁
        pthread_mutex_lock(&andVideo->codecMutex);
        AVPacket *avPacket = av_packet_alloc();
        AVFrame *avFrame = av_frame_alloc();

        if(andVideo->queue->getAvpacket(avPacket) != 0)
        {
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            continue;
        }
        if(avcodec_send_packet(andVideo->codecCtx, avPacket) != 0)
        {
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            pthread_mutex_unlock(&andVideo->codecMutex);
            continue;
        }
        if(avcodec_receive_frame(andVideo->codecCtx, avFrame) != 0)
        {
            av_frame_free(&avFrame);
            av_free(avFrame);
            avFrame = NULL;
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            pthread_mutex_unlock(&andVideo->codecMutex);
            continue;
        }
        pthread_mutex_unlock(&andVideo->codecMutex);

        // 解码成功 回调送去渲染
        if(avFrame->format == AV_PIX_FMT_YUV420P)  // 当前视频是YUV420P格式
        {
            // avFrame->data[0]代表y，avFrame->data[1]代表u， avFrame->data[2]代表v
            // av_usleep(33 * 1000);  // 不考虑同步的情况下  视频延迟33ms

            double diff = andVideo->getFrameDiffTime(avFrame);
            double delay = andVideo->getDelayTime(diff) * 1000000;
            av_usleep(delay);
            andVideo->callJava->onCallRenderYUV(
                    andVideo->codecCtx->width,
                    andVideo->codecCtx->height,
                    avFrame->data[0],
                    avFrame->data[1],
                    avFrame->data[2]);
        } else
        {
             // 当前视频不是YUV420P格式，就要使用转换器将视频格式转为YUV420P
            AVFrame *pFrameYUV420P = av_frame_alloc();
            int num = av_image_get_buffer_size(
                    AV_PIX_FMT_YUV420P,
                    andVideo->codecCtx->width,
                    andVideo->codecCtx->height,
                    1);
            uint8_t *buffer = static_cast<uint8_t *>(av_malloc(num * sizeof(uint8_t)));
            av_image_fill_arrays(
                    pFrameYUV420P->data,
                    pFrameYUV420P->linesize,
                    buffer,
                    AV_PIX_FMT_YUV420P,
                    andVideo->codecCtx->width,
                    andVideo->codecCtx->height,
                    1);
            SwsContext *sws_ctx = sws_getContext(
                    andVideo->codecCtx->width,andVideo->codecCtx->height,andVideo->codecCtx->pix_fmt,
                    andVideo->codecCtx->width,andVideo->codecCtx->height,AV_PIX_FMT_YUV420P,
                    SWS_BICUBIC, NULL, NULL, NULL);

            if(!sws_ctx)
            {
                av_frame_free(&pFrameYUV420P);
                av_free(pFrameYUV420P);
                av_free(buffer);
                pthread_mutex_unlock(&andVideo->codecMutex);
                continue;
            }
            sws_scale(
                    sws_ctx,
                    reinterpret_cast<const uint8_t *const *>(avFrame->data),
                    avFrame->linesize,
                    0,
                    avFrame->height,
                    pFrameYUV420P->data,
                    pFrameYUV420P->linesize);
            // 回调渲染
            andVideo->callJava->onCallRenderYUV(
                    andVideo->codecCtx->width,
                    andVideo->codecCtx->height,
                    pFrameYUV420P->data[0],
                    pFrameYUV420P->data[1],
                    pFrameYUV420P->data[2]);

            av_frame_free(&pFrameYUV420P);
            av_free(pFrameYUV420P);
            av_free(buffer);
            sws_freeContext(sws_ctx);
        }
        av_frame_free(&avFrame);
        av_free(avFrame);
        avFrame = NULL;
        av_packet_free(&avPacket);
        av_free(avPacket);
        avPacket = NULL;
        pthread_mutex_unlock(&andVideo->codecMutex);
    }
    pthread_exit(&andVideo->thread_play);
}

void AndVideo::play() {
    // 子线程 解码 播放
    pthread_create(&thread_play, NULL, decodePlay, this);
}

void AndVideo::pause() {
    queue->lock();
}

void AndVideo::resume() {
    queue->unlock();
}

void AndVideo::release() {
    if(queue != NULL)
    {
        delete(queue);
        queue = NULL;
    }
    if(codecCtx != NULL)
    {
        avcodec_close(codecCtx);
        avcodec_free_context(&codecCtx);
        codecCtx = NULL;
    }
    if(playStatus != NULL)
    {
        playStatus = NULL;
    }
    if(callJava != NULL)
    {
        callJava = NULL;
    }
}

// 计算音频与视频的pts差  根据pts差再去做调整（丢帧还是延迟播放）
double AndVideo::getFrameDiffTime(AVFrame *avFrame) {
    // 获取视频的pts
    double pts = av_frame_get_best_effort_timestamp(avFrame);
    if(pts == AV_NOPTS_VALUE)
    {
        pts = 0;
    }
    // time_base表示1个刻度有多大， pts表示有多少个刻度
    // pts = pts * time_base.num / time_base.den; // 与下行等价
    pts *= av_q2d(time_base);  // 单位是s
    if(pts > 0)
    {
        clock = pts;
    }

    double diff = vAudio->clock - clock;
    return diff;
}

// 以30ms为基准 进行动态计算
// 通过diff 计算视频延迟时间
double AndVideo::getDelayTime(double diff) {
    // diff>0: 音频快，则视频休眠时间要减小
    // diff<0: 视频快，则视频休眠时间要增大

    //以(-0.003, 0.003)为门限阈值  在该区间内，则无需同步处理
    if(diff > SyncThreshold) {
        // 音频快，则减小视频延迟时间 减小为原来的2/3
        delayTime = delayTime * 2 / 3;

        // 减小后的值太大或者太小，也不合适，给出边界值  若defaultDelayTime=40，则(20, 80)
        if (delayTime < defaultDelayTime / 2)
        {
            delayTime = defaultDelayTime / 2;
        }else if(delayTime > defaultDelayTime * 2)
        {
            delayTime = defaultDelayTime * 2;
        }
    } else if(diff < -SyncThreshold)
    {
        // 视频快 则增大视频延迟时间 增大为原来的3/2
        delayTime = delayTime * 3 / 2;

        // 增大后的值太大或者太小，也不合适，给出边界值  若defaultDelayTime=40，则(20, 80)
        if(delayTime < defaultDelayTime / 2)
        {
            delayTime = defaultDelayTime / 2;
        }
        else if(delayTime > defaultDelayTime * 2)
        {
            delayTime = defaultDelayTime * 2;
        }
    }

    // 极端情况 音频太快了 视频怎么赶也赶不上  需要将视频队列全部清空，直接解析最新的
    // 极端情况 视频太快了 音频赶不上 需要将音频队列全部清空，直接解析最新的
    if (diff >= 5 || diff<= -5) {
        queue->clearAvpacket();
        vAudio->queue->clearAvpacket();
        delayTime = defaultDelayTime;
    }
    return delayTime;
}
