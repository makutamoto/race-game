#ifdef __BORLANDC__
#ifndef BORLAND_H
#define BORLAND_H

#include<stdio.h>

typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;

#define atanf(x) (float)atan(x)
#define ceilf(x) (float)ceil(x)
#define floorf(x) (float)floor(x)
#define cosf(x) (float)cos(x)
#define sinf(x) (float)sin(x)
#define sqrtf(x) (float)sqrt(x)
#define tanf(x) (float)tan(x)
float roundf(float x);

int fopen_s(FILE** pFile, const char *filename, const char *mode);
#define fread_s(buffer, bufferSize, elementSize, count, stream) fread(buffer, elementSize, count, stream)

#define memcpy_s(dest, destSize, src, count) memcpy(dest, src, count)

#endif
#endif
