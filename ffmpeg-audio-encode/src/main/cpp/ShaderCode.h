//
// Created by xiangweixin on 2020-06-28.
//

#ifndef FFMPEG4ANDROID_MASTER_SHADERCODE_H
#define FFMPEG4ANDROID_MASTER_SHADERCODE_H

#define SHADER_STRING(...) #__VA_ARGS__

const float VERTEX_POS_ALL[] = {
        -1.f, -1.f,
        1.f, -1.f,
        -1.f, 1.f,
        1.f, 1.f,
};
const float TEXTURE_POS_ALL[] = {
        0.f, 1.f,
        1.f, 1.f,
        0.f, 0.f,
        1.f, 0.f,
};

#endif //FFMPEG4ANDROID_MASTER_SHADERCODE_H
