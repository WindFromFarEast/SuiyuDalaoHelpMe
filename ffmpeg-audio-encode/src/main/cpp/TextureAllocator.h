//
// Created by xiangweixin on 2020/11/18.
//

#ifndef FFMPEG4ANDROID_MASTER_TEXTUREALLOCATOR_H
#define FFMPEG4ANDROID_MASTER_TEXTUREALLOCATOR_H

#include "logger.h"
#include "GLData.h"

class TextureAllocator {

public:
    static GLuint allocateTexture(int width, int height, unsigned char *data = nullptr);

};


#endif //FFMPEG4ANDROID_MASTER_TEXTUREALLOCATOR_H
