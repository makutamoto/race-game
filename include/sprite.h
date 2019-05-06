#ifndef SPRITE_H
#define SPRITE_H

#include "./graphics.h"

typedef struct _Sprite {
	char id[10];
	float velocity[2];
	float angVelocity;
	float position[2];
	float angle;
	float scale[2];
	float shadowScale;
	float shadowOffset[2];
	Image *image;
	struct _Sprite *parent;
	struct _Sprite *children[10];
}	Sprite;

void initSprite(Sprite *sprite, const char *id, Image *image);
int addChild(Sprite *parent, Sprite *child);
void drawSprite(Sprite sprite);

#endif
