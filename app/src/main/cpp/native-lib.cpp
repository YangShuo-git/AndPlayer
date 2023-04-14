#include <jni.h>
#include <string>

extern "C" {
#include "include/libavcodec/avcodec.h"
#include "include/libavformat/avformat.h"
#include "include/libswscale/swscale.h"
#include "include/libavutil/imgutils.h"
#include "libavutil/time.h"
#include "libswresample/swresample.h"
}

#include <android/log.h>
#include <android/native_window_jni.h>

#define LOGD(...) __android_log_print(ANDROID_LOG_INFO,"baiyang",__VA_ARGS__)

static AVFormatContext *formatCtx;
static AVCodecContext *codecCtx;
AVCodec *vCodec;

static AVPacket *avPacket;
static AVFrame *avFrame;
static AVFrame *rgbFrame;
struct SwsContext *swsCtx;
uint8_t *outbuffer;

ANativeWindow* nativeWindow;
ANativeWindow_Buffer windowBuffer;

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_andplayer_MainActivity_play(JNIEnv *env, jobject thiz, jstring url_,
                                             jobject surface) {
    // 获取文件的地址
    const char *url = env->GetStringUTFChars(url_, 0);

    /* ************************ 解封装3步曲 ************************ */
    // 1.分配解码器上下文
    formatCtx = avformat_alloc_context();
    // 2.打开文件并解析
    if(avformat_open_input(&formatCtx, url, NULL, NULL) != 0){
        LOGD("Couldn't open input stream.\n");
        return -1;
    }
    LOGD("打开视频成功.\n");

    // 3.查找流的上下文信息，并填充Stream的MetaData信息
    if (avformat_find_stream_info(formatCtx, NULL) < 0) {
        LOGD("Couldn't find stream information\n");
        return -1;
    }

    // 遍历获取流索引
    int videoIndex = -1;
    for (int i = 0; i < formatCtx->nb_streams; ++i) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoIndex = i;
            break;
        }
    }

    if (videoIndex == -1) {
        LOGD("Couldn't find a video stream.\n");
        return -1;
    }
    LOGD("成功找到视频流.\n");
    /* ************************** 解封装结束 ************************** */

    /* ************************ 打开解码器4步曲 ************************ */
    codecCtx = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(codecCtx, formatCtx->streams[videoIndex]->codecpar);
    vCodec = avcodec_find_decoder(formatCtx->streams[videoIndex]->codecpar->codec_id);
    if (avcodec_open2(codecCtx, vCodec, NULL) < 0) {
        LOGD("Couldn't open vedio codec.\n");
        return -1;
    }
    LOGD("成功打开解码器.\n");
    /* ************************ 打开解码器结束 ************************ */

    /* ************************** 渲染准备 ************************** */
    // 获取界面传下来的surface
    nativeWindow = ANativeWindow_fromSurface(env, surface);
    if (nativeWindow == 0) {
        LOGD("Couldn't get native window from surface.\n");
        return -1;
    }
    // 三个容器
    avPacket = av_packet_alloc();
    avFrame  = av_frame_alloc();
    rgbFrame = av_frame_alloc(); // 存放转换后的数据

    int width = codecCtx->width;
    int height = codecCtx->height;

    // 创建输入缓冲区 (要喂给surface去渲染）
    int numBytes =  av_image_get_buffer_size(AV_PIX_FMT_RGBA, width, height, 1);
    outbuffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
    // 不具备内存申请的功能，类似于将 av_malloc申请的内存 进行格式化
    av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, outbuffer, AV_PIX_FMT_RGBA, width, height,1);
    LOGD("width： %d\theight: %d\n", width, height);

    // 获取转换器上下文  宽、高、采样格式
    swsCtx = sws_getContext(width,height,codecCtx->pix_fmt,
                   width,height,AV_PIX_FMT_RGBA,SWS_BICUBIC,NULL,NULL,NULL);

    if (ANativeWindow_setBuffersGeometry(nativeWindow, width, height,WINDOW_FORMAT_RGBA_8888) < 0) {
        LOGD("Couldn't set buffers geometry.\n");
        ANativeWindow_release(nativeWindow);
        return -1;
    }
    LOGD("成功调用ANativeWindow_setBuffersGeometry.\n");
    /* ************************ 渲染准备结束 ************************ */

    /* ************************** 解码渲染 ************************** */
    while (av_read_frame(formatCtx,avPacket) >= 0) {
        if (avPacket->stream_index == videoIndex) {
            int ret = avcodec_send_packet(codecCtx,avPacket);
            if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF){
                LOGD("解码出错");
                return -1;
            }

            ret = avcodec_receive_frame(codecCtx,avFrame);
            if (ret == AVERROR(EAGAIN)) { // 还要送数据过来，才能解码
                continue;
            } else if (ret < 0) {
                break;
            }

            // 处理解码后的frame
            sws_scale(swsCtx,avFrame->data,avFrame->linesize,0,codecCtx->height,
                      rgbFrame->data,rgbFrame->linesize);
            if (ANativeWindow_lock(nativeWindow, &windowBuffer, NULL) < 0) {
                LOGD("Couldn't lock window");
            } else {
                //将图像绘制到界面上，注意这里pFrameRGBA一行的像素和windowBuffer一行的像素长度可能不一致
                //需要进行转换，否则可能花屏
                uint8_t *dst = (uint8_t *) windowBuffer.bits;
                for (int h = 0; h < height; h++)
                {
                    memcpy(dst + h * windowBuffer.stride * 4,
                           outbuffer + h * rgbFrame->linesize[0],
                           rgbFrame->linesize[0]);
                }
            }
            av_usleep(33*1000);  // 否则视频播放会很快
            ANativeWindow_unlockAndPost(nativeWindow);
        }
    }
    /* ************************** 解码渲染结束 ************************** */

    av_frame_free(&avFrame);
    av_free(avPacket);
    sws_freeContext(swsCtx);
    avcodec_close(codecCtx);
    avformat_close_input(&formatCtx);

    env->ReleaseStringUTFChars(url_, url);
    return -1;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_andplayer_MainActivity_playSound(JNIEnv *env, jobject thiz, jstring url_) {
    const char *url = env->GetStringUTFChars(url_, 0);

    /* ************************ 解封装3步曲 ************************ */
    AVFormatContext *formatCtx = avformat_alloc_context();
    if(avformat_open_input(&formatCtx, url, NULL, NULL) != 0){
        LOGD("Couldn't open input stream.\n");
        return -1;
    }
    LOGD("成功打开音频.\n");

    if (avformat_find_stream_info(formatCtx, NULL) < 0) {
        LOGD("Couldn't find stream information\n");
        return -1;
    }

    int audioIndex = -1;
    for (int i = 0; i < formatCtx->nb_streams; ++i) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioIndex = i;
            break;
        }
    }

    if (audioIndex == -1) {
        LOGD("Couldn't find a audio stream.\n");
        return -1;
    }
    LOGD("成功找到音频流： %d.\n", audioIndex);
    /* ************************** 解封装结束 ************************** */

    /* ************************ 打开解码器4步曲 ************************ */
    AVCodecContext *codecCtx = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(codecCtx, formatCtx->streams[audioIndex]->codecpar);
    AVCodec *vCodec = avcodec_find_decoder(formatCtx->streams[audioIndex]->codecpar->codec_id);
    if (avcodec_open2(codecCtx, vCodec, NULL) < 0) {
        LOGD("Couldn't open audio codec.\n");
        return -1;
    }
    LOGD("成功打开解码器.\n");
    /* ************************ 打开解码器结束 ************************ */

    /* *************************** 播放准备 *************************** */
    AVPacket *packet = av_packet_alloc();
    AVFrame  *frame = av_frame_alloc();
    SwrContext *swrCtx = swr_alloc();  // 转换器上下文

    uint64_t out_channels_layout = AV_CH_LAYOUT_STEREO;   // 通道布局
    enum AVSampleFormat out_formart = AV_SAMPLE_FMT_S16;  // 采样格式
    int out_sample_rate = codecCtx->sample_rate;          // 采样率
    int out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);  // 声道数
    LOGD("out_sample_rate： %d\tout_channels: %d\n", out_sample_rate, out_channels);

    // 设置转换器上下文参数并对其初始化   (否则不能播放)
    swr_alloc_set_opts(swrCtx,
                       out_channels_layout,out_formart,out_sample_rate,
                       codecCtx->channel_layout, codecCtx->sample_fmt, codecCtx->sample_rate,
                       0,NULL);
    swr_init(swrCtx);

    // 缓冲区 是要喂给AudioTrack播放  采样率*声道数，就是每秒的采样点数
    uint8_t *out_buffer = (uint8_t *) av_malloc(out_sample_rate * out_channels);

    // C++调用java的方法  反射
    jclass mainActivityClass = env->GetObjectClass(thiz);
    jmethodID creatTrackID = env->GetMethodID(mainActivityClass, "createTrack", "(II)V");
    env->CallVoidMethod(thiz, creatTrackID, out_sample_rate, out_channels);

    jmethodID playTrackID = env->GetMethodID(mainActivityClass, "playTrack", "([BI)V");
    /* ************************* 播放准备结束 ************************** */

    /* *********************** 解码、重采样、播放 ********************** */
    while (av_read_frame(formatCtx, packet) >= 0) {
        if (packet->stream_index == audioIndex) {
            int ret = avcodec_send_packet(codecCtx,packet);
            if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF){
                LOGD("解码出错\n");
                return -1;
            }
            ret = avcodec_receive_frame(codecCtx,frame);
            if (ret == AVERROR(EAGAIN)) { // 还要送数据过来，才能解码
                continue;
            } else if (ret < 0) {
                break;
            }

            // 处理解码后的frame，输出到out_buffer
            swr_convert(swrCtx, &out_buffer, out_sample_rate * out_channels,
                        (const uint8_t **)(frame->data), frame->nb_samples);
            // 获取一个音频帧的字节大小  (out_channels*nb_samples*byte_per_sample)
            int size = av_samples_get_buffer_size(NULL, out_channels, frame->nb_samples,AV_SAMPLE_FMT_S16, 1);

            // 回调到java层送给AudioTrack播放   uint8_t * ---> jbyteArray
            jbyteArray audio_sample_array = env->NewByteArray(size);
            env->SetByteArrayRegion(audio_sample_array, 0, size,reinterpret_cast<const jbyte *>(out_buffer));
            env->CallVoidMethod(thiz, playTrackID, audio_sample_array, size);
            env->DeleteLocalRef(audio_sample_array);
        }
    }
    /* *********************** 解码、重采样、播放结束 ********************** */

    av_frame_free(&frame);
    av_free(packet);
    swr_free(&swrCtx);
    avcodec_close(codecCtx);
    avformat_close_input(&formatCtx);
    env->ReleaseStringUTFChars(url_, url);
}