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
void translateTransformation(float dx, float dy);
void scaleTransformation(float sx, float sy);
void rotateTransformation(float rotation);

void fillBuffer(Image image, int shadow);

BOOL loadBitmap(char *fileName, Image *image, char transparent);
int drawRect(unsigned int width, unsigned int height, char color, Image *image);
int drawCircle(unsigned int radius, char color, Image *image);
void discardImage(Image image);

#endif
