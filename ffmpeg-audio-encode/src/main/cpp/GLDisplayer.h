//
// Created by xiangweixin on 2020-06-28.
//

#ifndef FFMPEG4ANDROID_MASTER_GLDISPLAYER_H
#define FFMPEG4ANDROID_MASTER_GLDISPLAYER_H


class GLDisplayer {
public:
    void setSurfaceSize(int width, int height);
    int init();
    void draw();
};


#endif //FFMPEG4ANDROID_MASTER_GLDISPLAYER_H
