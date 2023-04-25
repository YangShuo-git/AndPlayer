//
// Created by BaiYang on 2023-04-12.
//

#ifndef MUSICPLAYER_ANDPLAYSTATUS_H
#define MUSICPLAYER_ANDPLAYSTATUS_H


class AndPlayStatus {
public:
    bool exit;           // 退出
    bool play  = false;  // 播放
    bool pause = false;  // 暂停
    bool seek  = false;  // seek
    bool load = true;    // 加载

public:
    AndPlayStatus();
};


#endif //MUSICPLAYER_ANDPLAYSTATUS_H
