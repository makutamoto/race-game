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

Image loadBitmap(char *fileName, char transparent);
Image genRect(unsigned int width, unsigned int height, char color);
Image genCircle(unsigned int radius, char color);
void freeImage(Image image);

#endif
