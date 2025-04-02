//
// Created by BaiYang on 2023-04-12.
//

#ifndef MUSICPLAYER_ANDPLAYSTATUS_H
#define MUSICPLAYER_ANDPLAYSTATUS_H


class AndPlayStatus {
public:
    bool isExited;           // 退出
    bool isPlaying;  // 播放
    bool isPaused;  // 暂停
    bool isSeek;  // isSeek
    bool isLoad;    // 加载

public:
    AndPlayStatus();
};


#endif //MUSICPLAYER_ANDPLAYSTATUS_H
