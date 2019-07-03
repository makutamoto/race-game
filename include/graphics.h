#ifndef GRAPHICS_H
#define GRAPHICS_H

#include<Windows.h>

typedef struct {
	unsigned int width;
	unsigned int height;
	unsigned char transparent;
	unsigned char *data;
} Image;

extern unsigned char *buffer;
extern size_t bufferLength;
extern COORD bufferSizeCoord;

void pushTransformation(void);
void popTransformation(void);
void clearTransformation(void);
void mulTransformationL(float a[4][4]);
void translateTransformation(float dx, float dy, float dz);
void scaleTransformation(float sx, float sy, float sz);
void rotateTransformation(float rx, float ry, float rz);

void setBuffer(unsigned char color);
void fillBuffer(Image image, int shadow);

Image loadBitmap(char *fileName, unsigned char transparent);
Image genRect(unsigned int width, unsigned int height, unsigned char color);
Image genCircle(unsigned int rad, unsigned char color);
void freeImage(Image image);

#endif
