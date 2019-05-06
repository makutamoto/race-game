#ifndef GRAPHICS_H
#define GRAPHICS_H

typedef struct {
	unsigned int width;
	unsigned int height;
	char transparent;
	char *data;
} Image;

void pushTransformation();
void popTransformation();
void clearTransformation();
void scaleTransformation(float sx, float sy);
void rotateTransformation(float rotation);

void fillBuffer(int ox, int oy, unsigned int width, unsigned int height, char *data, char transparent, int shadow);

BOOL loadBitmap(char *fileName, Image *image, char transparent);
void discardImage(Image image);
void drawImage(int ox, int oy, Image image, char transparent);

void drawRect(int ox, int oy, unsigned int width, unsigned int height, char color);
void drawCircle(int ox, int oy, unsigned int radius, char color);

#endif
