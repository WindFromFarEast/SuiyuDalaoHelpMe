//
// Created by xiangweixin on 2020/11/18.
//

#ifndef FFMPEG4ANDROID_MASTER_RECORDERMANAGER_H
#define FFMPEG4ANDROID_MASTER_RECORDERMANAGER_H

#include "Encoder.h"
#include "logger.h"

class RecorderManager {

public:
    RecorderManager();
    ~RecorderManager();

    int init();
    int startRecord();

    void pushTexture(GLFrame *frame);

private:

};


#endif //FFMPEG4ANDROID_MASTER_RECORDERMANAGER_H
