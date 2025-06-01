//
// Created by BaiYang on 2023-04-12.
//

#include "AndAudio.h"

AndAudio::AndAudio(AndPlayStatus *playStatus, int sample_rate, AndCallJava *callJava) {
    this->sample_rate = sample_rate;
    // 采样率 声道数 采样位数
    this->outBuffer = (uint8_t *)av_malloc(sample_rate * 2 * 2);
    this->callJava = callJava;
    this->playStatus = playStatus;
    this->queue = new AndQueue(playStatus);

    this->sampleBuffer = static_cast<SAMPLETYPE *>(malloc(sample_rate * 2 * 2));
    this->soundTouch = new SoundTouch();
    this->soundTouch->setSampleRate(sample_rate);
    this->soundTouch->setChannels(2);

    this->soundTouch->setTempo(speed);  // 利用soundTouch的接口来设置变速
    this->soundTouch->setPitch(pitch);    // 利用soundTouch的接口来设置变调
}

// C的方法 传参 this就是这个data，把当前对象传进去，就可以调用该对象的方法了
// 用ffmpeg解码，并用opensl播放
static void* decodePlay(void *data) {
    AndAudio *andAudio = (AndAudio *) data;

    // 使用OpenSL ES处理解码后的音频数据
    andAudio->initOpenSLES();
    LOGD("exit audio decode thread.");
    pthread_exit(&andAudio->thread_play);
}

void AndAudio::play() {
    LOGD("create audio decode thread.");
    pthread_create(&thread_play, nullptr, decodePlay, this);
}

// 获取采样率
int AndAudio::getCurrentSampleRateForOpensles(int sample_rate) {
    int rate = 0;
    switch (sample_rate)
    {
        case 8000:
            rate = SL_SAMPLINGRATE_8;
            break;
        case 11025:
            rate = SL_SAMPLINGRATE_11_025;
            break;
        case 12000:
            rate = SL_SAMPLINGRATE_12;
            break;
        case 16000:
            rate = SL_SAMPLINGRATE_16;
            break;
        case 22050:
            rate = SL_SAMPLINGRATE_22_05;
            break;
        case 24000:
            rate = SL_SAMPLINGRATE_24;
            break;
        case 32000:
            rate = SL_SAMPLINGRATE_32;
            break;
        case 44100:
            rate = SL_SAMPLINGRATE_44_1;
            break;
        case 48000:
            rate = SL_SAMPLINGRATE_48;
            break;
        case 64000:
            rate = SL_SAMPLINGRATE_64;
            break;
        case 88200:
            rate = SL_SAMPLINGRATE_88_2;
            break;
        case 96000:
            rate = SL_SAMPLINGRATE_96;
            break;
        case 192000:
            rate = SL_SAMPLINGRATE_192;
            break;
        default:
            rate =  SL_SAMPLINGRATE_44_1;
    }
    return rate;
}

// 喇叭需要主动取出数据  喇叭主动调用该函数  （重要）
void pcmBufferCallBack(SLAndroidSimpleBufferQueueItf bf, void * handler)
{
    // AudioTrack 是被动 先解码，再送给AudioTrack
    // opengsl es 是主动 先触发opensl，再解码
    AndAudio *andAudio = (AndAudio *) handler;
    if (andAudio != nullptr) {
        // 喇叭配置 44100 2 2   数据量则有 44100*2*2 个字节  要1s播放完
//         int bufferSize = andAudio->resampleAudio();   // 未经soundTouch处理的pcm数据大小
        int bufferSize = andAudio->getSoundTouchData();  // 经过soundTouch处理的pcm数据大小
        if(bufferSize > 0)
        {
            // andAudio->clock 永远大于 pts
            // bufferSize * 单位采样点的时间
            andAudio->clock += bufferSize / ((double)(andAudio->sample_rate * 2 * 2));
            if(andAudio->clock - andAudio->last_time >= 0.1){
                andAudio->last_time = andAudio->clock;
                andAudio->callJava->onCallTimeInfo(CHILD_THREAD,andAudio->clock,andAudio->duration);
            }

            // 音频的数据送往喇叭
//             (*andAudio->pcmBufferQueue)->Enqueue(andAudio->pcmBufferQueue, andAudio->outBuffer, bufferSize);  // 未经soundTouch处理的pcm数据
             (*andAudio->pcmBufferQueue)->Enqueue(andAudio->pcmBufferQueue, (char *)andAudio->sampleBuffer, bufferSize*2*2);
        }
    }
}

// 解码一帧音频，并进行重采样
// 主要是获取解码后的pcm数据(outBuffer)及其实际大小(bufferSize)  因为opensl需要这两个
int AndAudio::resampleAudio(void **pcmBuf) {
    while (playStatus != nullptr && !playStatus->isExited) {
        avPacket = av_packet_alloc();
        if (queue->getAvpacket(avPacket) != 0) {
            // 释放3步曲
            av_packet_free(&avPacket);  // 释放AVPacket里的容器 data
            av_free(avPacket);          // 释放对象avPacket
            avPacket = nullptr;
            continue;
        }

        ret = avcodec_send_packet(codecCtx, avPacket);
        if(ret != 0)
        {
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = nullptr;
            continue;
        }
        avFrame = av_frame_alloc();
        ret = avcodec_receive_frame(codecCtx, avFrame);
        if(ret == 0) {
            // 转换器上下文
            SwrContext *swr_ctx = nullptr;
            // 设值输入参数 和 输出参数   （通道布局、采样格式、采样率）
            swr_ctx = swr_alloc_set_opts(NULL,
                    AV_CH_LAYOUT_STEREO,AV_SAMPLE_FMT_S16,avFrame->sample_rate,
                    avFrame->channel_layout,(AVSampleFormat) avFrame->format,avFrame->sample_rate,
                    NULL, NULL);
            if(!swr_ctx || swr_init(swr_ctx) <0)
            {
                av_packet_free(&avPacket);
                av_free(avPacket);
                avPacket = nullptr;
                av_frame_free(&avFrame);
                av_free(avFrame);
                avFrame = nullptr;
                swr_free(&swr_ctx);
                continue;
            }

            // 开始转换 swr_convert  将一帧pcm转入outBuffer中
            nb = swr_convert(swr_ctx,
                    &outBuffer,avFrame->nb_samples,
                    (const uint8_t **) avFrame->data,avFrame->nb_samples);
            int out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
            // 获取转换后的一帧pcm的真实大小 （AudioTrack不需要这个操作，但是opensl需要）
            data_size = nb * out_channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
            *pcmBuf = outBuffer;  // 将数据存放到pcmBuf中 用于变速处理

            // 当前解码时间不等于当前播放时间   因为还要拿去播放，这个过程要耗时间
            // time_base表示一个刻度的时间，pts表示有多少个刻度  （需要在解封装的时候获取）
            now_time= avFrame->pts * av_q2d(time_base);
            // now_time= avFrame->pts * (time_base.num / (double)  time_base.den);  // 与上行等价
            if(now_time < clock)
            {
                now_time = clock;
            }
            clock = now_time;

            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = nullptr;
            av_frame_free(&avFrame);
            av_free(avFrame);
            avFrame = nullptr;
            swr_free(&swr_ctx);
            break;
        } else{
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = nullptr;

            av_frame_free(&avFrame);
            av_free(avFrame);
            avFrame = nullptr;
            continue;
        }
    }
    return data_size;
}

void AndAudio::initOpenSLES() {
    // 1.创建引擎对象 engineObject
    SLresult result;
    result= slCreateEngine(&engineObject, 0, 0,
                           0, 0, 0);
    if(result != SL_RESULT_SUCCESS) {
        return;
    }
    // 初始化引擎  false 同步  true 异步
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    // 获取引擎接口 engineEngine，之后通过接口来操作引擎
    (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    if(result!=SL_RESULT_SUCCESS) {
        return;
    }
    LOGD("-------->initOpenSLES  engineObject");

    // 2.创建混音器 outputMixObject
    const SLInterfaceID mids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean mreq[1] = {SL_BOOLEAN_FALSE};
    result =(*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, mids, mreq);
    if(result!=SL_RESULT_SUCCESS) {
        return;
    }
    // 初始化混音器
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if(result!=SL_RESULT_SUCCESS) {
        return;
    }
    // 获取混音器接口 outputMixEnvironmentalReverb
    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB, &outputMixEnvironmentalReverb);
    if(result!=SL_RESULT_SUCCESS) {
        return;
    }
    LOGD("-------->initOpenSLES  outputMixObject");


    // 传递的是流数据 2代表一个声音两个字节
    SLDataLocator_AndroidSimpleBufferQueue android_queue={SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,2};
    // pcm数据
    SLDataFormat_PCM pcm={
            SL_DATAFORMAT_PCM,   // 播放数据的格式是pcm格式
            2,       // 2个声道（立体声）
            static_cast<SLuint32>(getCurrentSampleRateForOpensles(sample_rate)), // 默认是44100hz的频率
            SL_PCMSAMPLEFORMAT_FIXED_16,                    // 位数 16位
            SL_PCMSAMPLEFORMAT_FIXED_16,                    // 和位数一致就行
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT, // 立体声（前左前右）
            SL_BYTEORDER_LITTLEENDIAN                       // 结束标志
    };
    SLDataSource slDataSource = {&android_queue, &pcm};

    // 配置混音器 audioSnk  播放器跟混音器建立联系
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk ={&outputMix, 0};

    // SLDataSource *pAudioSrc,  音频源  音频配置
    // 各种配置与对应的开关
    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE,SL_IID_VOLUME,SL_IID_MUTESOLO};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE,SL_BOOLEAN_TRUE,SL_BOOLEAN_TRUE};

    // 3.创建播放器对象 pcmPlayerObject
    (*engineEngine)->CreateAudioPlayer(engineEngine, &pcmPlayerObject, &slDataSource, &audioSnk, 2, ids, req);
    // 初始化播放器
    (*pcmPlayerObject)->Realize(pcmPlayerObject, SL_BOOLEAN_FALSE);
    // 获取播放器接口 pcmPlayerPlay
    (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_PLAY, &pcmPlayerPlay);
    // 获取静音接口 pcmMutePlay
    (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_MUTESOLO, &pcmMutePlay);
    // 获取控制播放暂停恢复、音量的接口
    (*pcmPlayerObject)->GetInterface(pcmPlayerObject,SL_IID_VOLUME,&pcmVolumePlay);
    // 注册回调缓冲区 获取缓冲队列接口
    (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_BUFFERQUEUE, &pcmBufferQueue);
    // 通知喇叭怎么拉数据
    (*pcmBufferQueue)->RegisterCallback(pcmBufferQueue, pcmBufferCallBack, this);
    // 获取播放状态接口
    (*pcmPlayerPlay)->SetPlayState(pcmPlayerPlay, SL_PLAYSTATE_PLAYING);

    // 播放接口
    // 声道接口
    // 暂停恢复 接口
    // 注册 缓冲接口
    // 注册回调缓冲接口  激活
    pcmBufferCallBack(pcmBufferQueue, this);
    LOGD("-------->initOpenSLES pcmPlayerObject");
}


// 有关音频播放的相关操作，都在OpenSL ES的操作接口中
void AndAudio::pause() {
    queue->lock();
    if (pcmPlayerPlay != nullptr)
    {
        (*pcmPlayerPlay)->SetPlayState(pcmPlayerPlay, SL_PLAYSTATE_PAUSED);
    }
}

void AndAudio::resume() {
    queue->unlock();
    if (pcmPlayerPlay != nullptr)
    {
        (*pcmPlayerPlay)->SetPlayState(pcmPlayerPlay, SL_PLAYSTATE_PLAYING);
    }
}

void AndAudio::setVolume(int percent) {
    if (pcmVolumePlay != nullptr) {
        if (percent > 30) {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -20);
        } else if (percent > 25) {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -22);
        } else if (percent > 20) {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -25);
        } else if (percent > 15) {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -28);
        } else if (percent > 10) {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -30);
        } else if (percent > 5) {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -34);
        } else if (percent > 3) {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -37);
        } else if (percent > 0) {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -40);
        } else {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, (100 - percent) * -100);
        }
    }
}

SLmillibel AndAudio::getVolume() {
    SLmillibel level;
    (*pcmVolumePlay)->GetVolumeLevel(pcmVolumePlay, &level);
    return level;
}

void AndAudio::setMute(int mute) {
    LOGD(" 声道  接口%p", pcmMutePlay);
    LOGD(" 声道  接口%d", mute);
    if (pcmMutePlay == nullptr)
    {
        LOGE("pcmMutePlay is null");
        if ( mute == 2 )
        {
            this->volLevel = getVolume();
            setVolume(0);
            isMuted = true;
        } else {
            (*pcmVolumePlay)->SetVolumeLevel(pcmVolumePlay, this->volLevel);
        }
        return;
    }
    this->mute = mute;
    if(mute == 0) // right 0   左通道播放
    {
        (*pcmMutePlay)->SetChannelMute(pcmMutePlay, 1, false);
        (*pcmMutePlay)->SetChannelMute(pcmMutePlay, 0, true);

    } else if(mute == 1) // left
    {
        (*pcmMutePlay)->SetChannelMute(pcmMutePlay, 1, true);
        (*pcmMutePlay)->SetChannelMute(pcmMutePlay, 0, false);
    } else if(mute == 2) // right、left都静音
    {
        (*pcmMutePlay)->SetChannelMute(pcmMutePlay, 1, false);
        (*pcmMutePlay)->SetChannelMute(pcmMutePlay, 0, false);
    }else if(mute == 3) // 取消静音
    {
        (*pcmMutePlay)->SetChannelMute(pcmMutePlay, 1, true);
        (*pcmMutePlay)->SetChannelMute(pcmMutePlay, 0, true);
    }
}


// 获取整理之后的波形  倍速 波形变小；慢放 波形变大
int AndAudio::getSoundTouchData() {
    //我们先取数据 pcm就在out_buffer
    while (playStatus != nullptr && !playStatus->isExited) {
//        LOGE("------------------循环获取音频数据---------------------------finished %d",finished)
        out_buffer = nullptr;
        if (finished) {
            // 开始整理波形，没有完成
            finished = false;
            // 从网络流 文件 读取数据 out_buffer  字节数量  out_buffer   是一个旧波
            data_size = this->resampleAudio(reinterpret_cast<void **>(&out_buffer));
            if (data_size > 0)  // 表示有波形
            {
                for(int i = 0; i < data_size / 2 + 1; i++){
                    // short  2个字节  pcm数据 因为是2声道，所以两个8位合成一个数据
                    sampleBuffer[i] = (out_buffer[i * 2] | ((out_buffer[i * 2 + 1]) << 8));
                }
                // for循环后，把原始波形数据直接丢给sountouch来整理波形
                soundTouch->putSamples(sampleBuffer, nb);
                // 接受一个新波 sampleBuffer  返回值0表示继续整理波形  非0值表示新波形的大小
                num = soundTouch->receiveSamples(sampleBuffer, data_size / 4);
//                LOGE("------------第一个num %d ", num);
            }else
            {
                soundTouch->flush();
            }
        }

        if (num == 0) {
            finished = true;
            continue;
        } else
        {
            if (out_buffer == nullptr)
            {
                num=soundTouch->receiveSamples(sampleBuffer, data_size / 4);
//                LOGE("------------第二个num %d ",num);
                if(num == 0)
                {
                    finished = true;
                    continue;
                }
            }
            return num;
        }

    }
    return 0;
}

void AndAudio::setSpeed(float speed) {
    this->speed = speed;
    if (soundTouch != nullptr) {
        soundTouch->setTempo(speed);
    }
}

void AndAudio::setTone(float tone) {
    this->speed = speed;
    if (soundTouch != nullptr) {
        soundTouch->setPitch(tone);
    }
}

void AndAudio::release() {
    if (queue != nullptr)
    {
        delete(queue);
        queue = nullptr;
    }
    if (pcmPlayerObject != nullptr)
    {
        (*pcmPlayerObject)->Destroy(pcmPlayerObject);
        pcmPlayerObject = nullptr;
        pcmPlayerPlay = nullptr;
        pcmBufferQueue = nullptr;
    }
    if (outputMixObject != nullptr)
    {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = nullptr;
        outputMixEnvironmentalReverb = nullptr;
    }
    if (engineObject != nullptr)
    {
        (*engineObject)->Destroy(engineObject);
        engineObject = nullptr;
        engineEngine = nullptr;
    }
    if (outBuffer != nullptr)
    {
        free(outBuffer);
        outBuffer = nullptr;
    }
    if (sampleBuffer != nullptr)
    {
        free(sampleBuffer);
        sampleBuffer = nullptr;
    }
    if (soundTouch != nullptr)
    {
        delete soundTouch;
        soundTouch = nullptr;
    }
    if (codecCtx != nullptr)
    {
        avcodec_close(codecCtx);
        avcodec_free_context(&codecCtx);
        codecCtx = nullptr;
    }
    if (playStatus != nullptr)
    {
        playStatus = nullptr;
    }
    if (callJava != nullptr)
    {
        callJava = nullptr;
    }
}
