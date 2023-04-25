//
// Created by BaiYang on 2023-04-12.
//

#ifndef MUSICPLAYER_ANDQUEUE_H
#define MUSICPLAYER_ANDQUEUE_H

#include "queue"
#include "pthread.h"
#include "AndroidLog.h"
#include "AndPlayStatus.h"

extern "C"
{
#include "libavcodec/avcodec.h"
};


class AndQueue {
public:
    std::queue<AVPacket *> queuePacket;
    pthread_mutex_t mutexPacket;
    pthread_cond_t condPacket;
    AndPlayStatus *playstatus = NULL;

public:
    AndQueue(AndPlayStatus *playstatus);
    ~AndQueue();

    int putAvpacket(AVPacket *packet);

    int getAvpacket(AVPacket *packet);

    int getQueueSize();

    void clearAvpacket();

    void lock();

    void unlock();
};


#endif //MUSICPLAYER_ANDQUEUE_H
