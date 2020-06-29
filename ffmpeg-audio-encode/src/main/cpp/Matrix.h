//
// Created by xiangweixin on 2020-06-28.
//

#ifndef FFMPEG4ANDROID_MASTER_MATRIX_H
#define FFMPEG4ANDROID_MASTER_MATRIX_H

#include <math.h>
#include <stdlib.h>
#include <string>

#define PI 3.1415926f
#define MATRIX_LENGTH 16

void matrixSetIdentityM(float *m);
void matrixSetRotateM(float *m, float a, float x, float y, float z);
void matrixMultiplyMM(float *m, float *lhs, float *rhs);
void matrixScaleM(float *m, float x, float y, float z);
void matrixTranslateM(float *m, float x, float y, float z);
void matrixRotateM(float *m, float a, float x, float y, float z);
void matrixLookAtM(float *m, float eyeX, float eyeY, float eyeZ, float cenX,
                   float cenY, float cenZ, float upX, float upY, float upZ);
void matrixFrustumM(float *m, float left, float right, float bottom, float top, float near, float far);

void getTranslateMatrix(float *m, float x, float y, float z);

void perspectiveM(float *m, float yFovInDegrees, float aspect, float n, float f);

void ortho(float *m, float left, float right, float bottom, float top, float nearPlane, float farPlane);

void matrixMultiplyvec3(float *m, float *lhs, float *vec);

bool matrixInverseM(const float m[16], float invOut[16]);


#endif //FFMPEG4ANDROID_MASTER_MATRIX_H
