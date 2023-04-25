//
// Created by BaiYang on 2023-04-12.
//

#ifndef MUSICPLAYER_ANDAUDIO_H
#define MUSICPLAYER_ANDAUDIO_H

#include "AndQueue.h"
#include "AndCallJava.h"
#include "SoundTouch.h"

using namespace soundtouch;

extern "C"
{
#include <libswresample/swresample.h>
#include <libavcodec/codec_par.h>
#include <libavcodec/avcodec.h>

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
};


class AndAudio {
public:
    int ret = 0;
    AVCodecContext *codecCtx = NULL;
    AVCodecParameters *codecpar = NULL;
    AVCodec * avCodec = NULL;

    pthread_t thread_play;

    // 引擎对象
    SLObjectItf engineObject = NULL;
    // 引擎接口
    SLEngineItf engineEngine = NULL;
    // 混音器对象
    SLObjectItf outputMixObject = NULL;
    // 混音器接口
    SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;
    // 播放器对象
    SLObjectItf pcmPlayerObject = NULL;
    // 播放器操作接口
    SLPlayItf pcmPlayerPlay = NULL;
    // 静音接口
    SLMuteSoloItf  pcmMutePlay = NULL;
    SLVolumeItf pcmVolumePlay = NULL;
    // 缓冲器队列接口
    SLAndroidSimpleBufferQueueItf pcmBufferQueue = NULL;

    AndQueue *queue = NULL;
    AndCallJava *callJava = NULL;
    AndPlayStatus *playStatus = NULL;

    AVPacket *avPacket = NULL;
    AVFrame *avFrame = NULL;

    // 音频流索引
    int streamIndex = -1;
    // 采样频率
    int sample_rate = 0;
    // 输出音频缓冲区
    uint8_t *outBuffer = NULL;
    // 输出音频数据大小
    int data_size = 0;

    int duration = 0; // 总时长
    // 时间单位 总时间/帧数   单位时间 * 时间戳= pts  * 总时间/帧数
    AVRational time_base; // 时间基
    double now_time;  // 当前Frme的时间（解码时间）
    double clock;     // 当前播放的时间 （准确时间）

    double last_time; // 上一次调用时间

    bool isMuted = false;
    int mute = 2;  // 立体声
    SLmillibel volLevel = 0; // 音量值
    float speed = 1.0f; // 变速
    float pitch = 1.0f; // 变调

    SoundTouch *soundTouch = NULL;
    // 新的缓冲区  喂给soundTouch
    SAMPLETYPE *sampleBuffer = NULL;

    uint8_t *out_buffer = NULL;
    // 波是否处理结束
    bool finished = true;

    // 新波的实际个数
    int nb = 0;
    int num = 0;  // 整理之后的波形大小

public:
    AndAudio(AndPlayStatus *playStatus, int sample_rate, AndCallJava *callJava);

    int getCurrentSampleRateForOpensles(int sample_rate);

    int resampleAudio(void **pcmBuf);  // 解码 并重采样

    int getSoundTouchData();

    void initOpenSLES();

    void play();

    void pause();

    void resume();

    void setVolume(int percent);

    SLmillibel getVolume();

    void setMute(int mute);

    void setSpeed(float speed);

    void setTone(float tone);

    void release();
};


#endif //MUSICPLAYER_ANDAUDIO_H
