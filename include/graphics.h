#ifndef GRAPHICS_H
#define GRAPHICS_H

typedef struct {
	unsigned int width;
	unsigned int height;
	char *data;
} Image;

void pushTransformation();
void popTransformation();
void clearTransformation();
void scaleTransformation(float sx, float sy);
void rotateTransformation(float rotation);

void fillBuffer(int x, int y, unsigned int width, unsigned int height, char *data);
void setPixelColor(int x, int y, char color);

BOOL loadImage(char *fileName, Image *image);
void discardImage(Image image);
void drawImage(int ox, int oy, Image image);
void drawImageTransformed(int ox, int oy, Image image);

void drawRect(int ox, int oy, unsigned int width, unsigned int height, char color);
void drawCircle(int ox, int oy, unsigned int radius, char color);

#endif
