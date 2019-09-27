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

typedef struct {
	Image font0201;
	Image font0208;
	unsigned int offset0201;
	unsigned int offset0208;
	unsigned int height;
	unsigned int width[2];
} FontSJIS;

extern Image NO_IMAGE;

FontSJIS initFontSJIS(Image font0201, Image font0208, unsigned int width0201, unsigned int width0208, unsigned int height);

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

void fillTriangle(Vertex vertices[3], Image image, const float uv[3][2]);
void fillPolygons(Vector vertices, Vector indices, Image image, Vector uv, Vector uvIndices);

Image initImage(unsigned int width, unsigned int height, unsigned char color, unsigned char transparent);
void clearImage(Image image);
void cropImage(Image dest, Image src, unsigned int xth, unsigned int yth);
void pasteImage(Image dest, Image src, unsigned int x, unsigned int y);
BOOL drawCharSJIS(Image target, FontSJIS font, unsigned int x, unsigned int y, char *character);
void drawTextSJIS(Image target, FontSJIS font, unsigned int x, unsigned int y, char *text);
Image loadBitmap(char *fileName, unsigned char transparent);
Image genRect(unsigned int width, unsigned int height, unsigned char color);
Image genCircle(unsigned int rad, unsigned char color);
void freeImage(Image image);

#endif
