//
// Created by xiangweixin on 2020-06-30.
//

#include "GLUtils.h"

void writeTextureToFile(GLuint texture, int width, int height, const char *file) {
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    if (fbo <= 0) {
        LOGE("writeTextureToFile failed.");
        return;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    GLenum gLenum = glGetError();
    if (gLenum != GL_NO_ERROR) {
        LOGE("glFramebufferTexture2D failed.. error: %d", gLenum);
        glDeleteFramebuffers(1, &fbo);
        return;
    }
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOGE("frame buffer status is wrong. status: %d", status);
        glDeleteFramebuffers(1, &fbo);
        return;
    }
    glBindTexture(GL_TEXTURE_2D, texture);

    uint8_t *pixels = new uint8_t[width * height * 4];
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    FILE *fp = fopen(file, "wb+");
    if (fp == nullptr) {
        LOGE("Cannot open file %s", file);
        glDeleteFramebuffers(1, &fbo);
        delete[] pixels;
        return;
    }
    fwrite(pixels, width * height * 4, 1, fp);
    fclose(fp);

    delete[] pixels;

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
