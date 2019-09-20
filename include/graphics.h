#ifndef GRAPHICS_H
#define GRAPHICS_H

#include<Windows.h>

#include "./vector.h"

typedef struct _Vertex {
	float components[4];
	unsigned char color;
} Vertex;

typedef struct {
	unsigned int width;
	unsigned int height;
	unsigned char transparent;
	unsigned char *data;
} Image;

void initGraphics(unsigned int width, unsigned int height);
void deinitGraphics(void);
void clearBuffer(unsigned char color);
void clearZBuffer(void);
void flushBuffer(HANDLE screen);

Vertex initVertex(float x, float y, float z, unsigned char color);

void pushTransformation(void);
void popTransformation(void);
void clearTransformation(void);
void setCameraMat4(float mat[4][4]);
void clearCameraMat4(void);
void translateTransformation(float dx, float dy, float dz);
void scaleTransformation(float sx, float sy, float sz);
void rotateTransformation(float rx, float ry, float rz);

void clearAABB(void);
float (*getAABB(float out[3][2]))[2];

void fillTriangle(Vertex vertices[3], Image *image, float *uv[3]);
void fillPolygons(Vector vertices, Vector indices, Image *image, Vector uv, Vector uvIndices);

Image loadBitmap(char *fileName, unsigned char transparent);
Image genRect(unsigned int width, unsigned int height, unsigned char color);
Image genCircle(unsigned int rad, unsigned char color);
void freeImage(Image image);

#endif
