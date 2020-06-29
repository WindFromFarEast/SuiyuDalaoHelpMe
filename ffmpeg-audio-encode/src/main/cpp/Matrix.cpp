//
// Created by xiangweixin on 2020-06-28.
//

#include "Matrix.h"
#include <math.h>
#include <cstring>

#define normalize(x, y, z)                  \
{                                               \
        float norm = 1.0f / sqrt(x*x+y*y+z*z);  \
        x *= norm; y *= norm; z *= norm;        \
}

#define I(_i, _j) ((_j)+4*(_i))

void matrixSetIdentityM(float *m)
{
    memset((void*)m, 0, 16*sizeof(float));
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

void matrixSetRotateM(float *m, float a, float x, float y, float z)
{
    float s, c;

    memset((void*)m, 0, 15*sizeof(float));
    m[15] = 1.0f;

    a *= PI/180.0f;
    s = sin(a);
    c = cos(a);

    if (1.0f == x && 0.0f == y && 0.0f == z) {
        m[5] = c; m[10] = c;
        m[6] = s; m[9]  = -s;
        m[0] = 1;
    } else if (0.0f == x && 1.0f == y && 0.0f == z) {
        m[0] = c; m[10] = c;
        m[8] = s; m[2]  = -s;
        m[5] = 1;
    } else if (0.0f == x && 0.0f == y && 1.0f == z) {
        m[0] = c; m[5] = c;
        m[1] = s; m[4] = -s;
        m[10] = 1;
    } else {
        normalize(x, y, z);
        float nc = 1.0f - c;
        float xy = x * y;
        float yz = y * z;
        float zx = z * x;
        float xs = x * s;
        float ys = y * s;
        float zs = z * s;
        m[ 0] = x*x*nc +  c;
        m[ 4] =  xy*nc - zs;
        m[ 8] =  zx*nc + ys;
        m[ 1] =  xy*nc + zs;
        m[ 5] = y*y*nc +  c;
        m[ 9] =  yz*nc - xs;
        m[ 2] =  zx*nc - ys;
        m[ 6] =  yz*nc + xs;
        m[10] = z*z*nc +  c;
    }
}

void matrixMultiplyMM(float *m, float *lhs, float *rhs)
{
    float t[16];
    for (int i = 0; i < 4; i++) {
        const float rhs_i0 = rhs[I(i, 0)];
        float ri0 = lhs[ I(0,0) ] * rhs_i0;
        float ri1 = lhs[ I(0,1) ] * rhs_i0;
        float ri2 = lhs[ I(0,2) ] * rhs_i0;
        float ri3 = lhs[ I(0,3) ] * rhs_i0;
        for (int j = 1; j < 4; j++) {
            const float rhs_ij = rhs[ I(i,j) ];
            ri0 += lhs[ I(j,0) ] * rhs_ij;
            ri1 += lhs[ I(j,1) ] * rhs_ij;
            ri2 += lhs[ I(j,2) ] * rhs_ij;
            ri3 += lhs[ I(j,3) ] * rhs_ij;
        }
        t[ I(i,0) ] = ri0;
        t[ I(i,1) ] = ri1;
        t[ I(i,2) ] = ri2;
        t[ I(i,3) ] = ri3;
    }
    memcpy(m, t, sizeof(t));
}

void matrixMultiplyvec3(float *m, float *lhs, float *vec)
{
    float t[3];
    float vec0 = vec[0];
    float vec1 = vec[1];
    float vec2 = vec[2];
    for (int i = 0; i < 3; i++) {
        t[i] = lhs[ I(0,i) ] * vec0 + lhs[ I(1,i) ] * vec1 + lhs[ I(2,i) ] * vec2;
    }

    memcpy(m, t, sizeof(t));
}

void matrixScaleM(float *m, float x, float y, float z)
{
    for (int i = 0; i < 4; i++)
    {
        m[i] *= x; m[4+i] *=y; m[8+i] *= z;
    }
}

void matrixTranslateM(float *m, float x, float y, float z)
{
    for (int i = 0; i < 4; i++)
    {
        m[12+i] += m[i]*x + m[4+i]*y + m[8+i]*z;
    }
}

void getTranslateMatrix(float *m, float x, float y, float z) {
    matrixSetIdentityM(m);

    m[3] = x;
    m[7] = y;
    m[11] = z;
}

void matrixRotateM(float *m, float a, float x, float y, float z)
{
    float rot[16], res[16];
    matrixSetRotateM(rot, a, x, y, z);
    matrixMultiplyMM(res, m, rot);
    memcpy(m, res, 16*sizeof(float));
}

void matrixLookAtM(float *m,
                   float eyeX, float eyeY, float eyeZ,
                   float cenX, float cenY, float cenZ,
                   float  upX, float  upY, float  upZ)
{
    float fx = cenX - eyeX;
    float fy = cenY - eyeY;
    float fz = cenZ - eyeZ;
    normalize(fx, fy, fz);
    float sx = fy * upZ - fz * upY;
    float sy = fz * upX - fx * upZ;
    float sz = fx * upY - fy * upX;
    normalize(sx, sy, sz);
    float ux = sy * fz - sz * fy;
    float uy = sz * fx - sx * fz;
    float uz = sx * fy - sy * fx;

    m[ 0] = sx;
    m[ 1] = ux;
    m[ 2] = -fx;
    m[ 3] = 0.0f;
    m[ 4] = sy;
    m[ 5] = uy;
    m[ 6] = -fy;
    m[ 7] = 0.0f;
    m[ 8] = sz;
    m[ 9] = uz;
    m[10] = -fz;
    m[11] = 0.0f;
    m[12] = 0.0f;
    m[13] = 0.0f;
    m[14] = 0.0f;
    m[15] = 1.0f;
    matrixTranslateM(m, -eyeX, -eyeY, -eyeZ);
}

void matrixFrustumM(float *m, float left, float right, float bottom, float top, float near, float far)
{
    float r_width  = 1.0f / (right - left);
    float r_height = 1.0f / (top - bottom);
    float r_depth  = 1.0f / (near - far);
    float x = 2.0f * (near * r_width);
    float y = 2.0f * (near * r_height);
    float A = 2.0f * ((right+left) * r_width);
    float B = (top + bottom) * r_height;
    float C = (far + near) * r_depth;
    float D = 2.0f * (far * near * r_depth);

    memset((void*)m, 0, 16*sizeof(float));
    m[ 0] = x;
    m[ 5] = y;
    m[ 8] = A;
    m[ 9] = B;
    m[10] = C;
    m[14] = D;
    m[11] = -1.0f;
}

void perspectiveM(float *m, float yFovInDegrees, float aspect, float n, float f)
{
    float angleInRadians = yFovInDegrees * PI / 180.0f;
    float a = 1.0 / tanf(angleInRadians / 2.0f);

    m[0] = a / aspect;
    m[1] = 0;
    m[2] = 0;
    m[3] = 0;

    m[4] = 0;
    m[5] = a;
    m[6] = 0;
    m[7] = 0;

    m[8] = 0;
    m[9] = 0;
    m[10] = -((f+n)/(f-n));
    m[11] = -1.0f;

    m[12] = 0;
    m[13] = 0;
    m[14] = -((2.0*f*n)/(f-n));
    m[15] = 0;
}

void ortho(float *m, float left, float right, float bottom, float top, float nearPlane, float farPlane)
{
    // Bail out if the projection volume is zero-sized.
    if (left == right || bottom == top || nearPlane == farPlane)
        return;

    // Construct the projection.
    float width = right - left;
    float invheight = top - bottom;
    float clip = farPlane - nearPlane;
    matrixSetIdentityM(m);

    m[0] = 2.0f / width;
    m[1] = 0.0f;
    m[2] = 0.0f;
    m[3] = 0.0f;
    m[4] = 0.0f;
    m[5] = 2.0f / invheight;
    m[6] = 0.0f;
    m[7] = 0.0f;
    m[8] = 0.0f;
    m[9] = 0.0f;
    m[10] = -2.0f / clip;
    m[11] = 0.0f;
    m[12] = -(left + right) / width;
    m[13] = -(top + bottom) / invheight;
    m[14] = -(nearPlane + farPlane) / clip;
    m[15] = 1.0f;


//        m[0] = 2.0f / width;
//        m[1] = 0.0f;
//        m[2] = 0.0f;
//        m[3] = -(left + right) / width;
//        m[4] = 0.0f;
//        m[5] = 2.0f / invheight;
//        m[6] = 0.0f;
//        m[7] = -(top + bottom) / invheight;
//        m[8] = 0.0f;
//        m[9] = 0.0f;
//        m[10] = -2.0f / clip;
//        m[11] = -(nearPlane + farPlane) / clip;
//        m[12] = 0.0f;
//        m[13] = 0.0f;
//        m[14] = 0.0f;
//        m[15] = 1.0f;
}

bool matrixInverseM(const float m[16], float invOut[16])
{
    double inv[16], det;
    int i;

    inv[0] = m[5]  * m[10] * m[15] -
             m[5]  * m[11] * m[14] -
             m[9]  * m[6]  * m[15] +
             m[9]  * m[7]  * m[14] +
             m[13] * m[6]  * m[11] -
             m[13] * m[7]  * m[10];

    inv[4] = -m[4]  * m[10] * m[15] +
             m[4]  * m[11] * m[14] +
             m[8]  * m[6]  * m[15] -
             m[8]  * m[7]  * m[14] -
             m[12] * m[6]  * m[11] +
             m[12] * m[7]  * m[10];

    inv[8] = m[4]  * m[9] * m[15] -
             m[4]  * m[11] * m[13] -
             m[8]  * m[5] * m[15] +
             m[8]  * m[7] * m[13] +
             m[12] * m[5] * m[11] -
             m[12] * m[7] * m[9];

    inv[12] = -m[4]  * m[9] * m[14] +
              m[4]  * m[10] * m[13] +
              m[8]  * m[5] * m[14] -
              m[8]  * m[6] * m[13] -
              m[12] * m[5] * m[10] +
              m[12] * m[6] * m[9];

    inv[1] = -m[1]  * m[10] * m[15] +
             m[1]  * m[11] * m[14] +
             m[9]  * m[2] * m[15] -
             m[9]  * m[3] * m[14] -
             m[13] * m[2] * m[11] +
             m[13] * m[3] * m[10];

    inv[5] = m[0]  * m[10] * m[15] -
             m[0]  * m[11] * m[14] -
             m[8]  * m[2] * m[15] +
             m[8]  * m[3] * m[14] +
             m[12] * m[2] * m[11] -
             m[12] * m[3] * m[10];

    inv[9] = -m[0]  * m[9] * m[15] +
             m[0]  * m[11] * m[13] +
             m[8]  * m[1] * m[15] -
             m[8]  * m[3] * m[13] -
             m[12] * m[1] * m[11] +
             m[12] * m[3] * m[9];

    inv[13] = m[0]  * m[9] * m[14] -
              m[0]  * m[10] * m[13] -
              m[8]  * m[1] * m[14] +
              m[8]  * m[2] * m[13] +
              m[12] * m[1] * m[10] -
              m[12] * m[2] * m[9];

    inv[2] = m[1]  * m[6] * m[15] -
             m[1]  * m[7] * m[14] -
             m[5]  * m[2] * m[15] +
             m[5]  * m[3] * m[14] +
             m[13] * m[2] * m[7] -
             m[13] * m[3] * m[6];

    inv[6] = -m[0]  * m[6] * m[15] +
             m[0]  * m[7] * m[14] +
             m[4]  * m[2] * m[15] -
             m[4]  * m[3] * m[14] -
             m[12] * m[2] * m[7] +
             m[12] * m[3] * m[6];

    inv[10] = m[0]  * m[5] * m[15] -
              m[0]  * m[7] * m[13] -
              m[4]  * m[1] * m[15] +
              m[4]  * m[3] * m[13] +
              m[12] * m[1] * m[7] -
              m[12] * m[3] * m[5];

    inv[14] = -m[0]  * m[5] * m[14] +
              m[0]  * m[6] * m[13] +
              m[4]  * m[1] * m[14] -
              m[4]  * m[2] * m[13] -
              m[12] * m[1] * m[6] +
              m[12] * m[2] * m[5];

    inv[3] = -m[1] * m[6] * m[11] +
             m[1] * m[7] * m[10] +
             m[5] * m[2] * m[11] -
             m[5] * m[3] * m[10] -
             m[9] * m[2] * m[7] +
             m[9] * m[3] * m[6];

    inv[7] = m[0] * m[6] * m[11] -
             m[0] * m[7] * m[10] -
             m[4] * m[2] * m[11] +
             m[4] * m[3] * m[10] +
             m[8] * m[2] * m[7] -
             m[8] * m[3] * m[6];

    inv[11] = -m[0] * m[5] * m[11] +
              m[0] * m[7] * m[9] +
              m[4] * m[1] * m[11] -
              m[4] * m[3] * m[9] -
              m[8] * m[1] * m[7] +
              m[8] * m[3] * m[5];

    inv[15] = m[0] * m[5] * m[10] -
              m[0] * m[6] * m[9] -
              m[4] * m[1] * m[10] +
              m[4] * m[2] * m[9] +
              m[8] * m[1] * m[6] -
              m[8] * m[2] * m[5];

    det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

    if (det == 0)
        return false;

    det = 1.0 / det;

    for (i = 0; i < 16; i++)
        invOut[i] = inv[i] * det;

    return true;
}
