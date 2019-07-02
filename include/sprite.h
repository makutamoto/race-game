#ifndef SPRITE_H
#define SPRITE_H

#include "./graphics.h"

typedef struct _Sprite {
	char id[10];
	float velocity[3];
	float angVelocity;
	float position[3];
	float angle[3];
	float scale[3];
	float shadowScale;
	float shadowOffset[2];
	Image image;
	struct _Sprite *parent;
	struct _Sprite *children[10];
	int (*behaviour)(struct _Sprite*);
}	Sprite;

Sprite initSprite(const char *id, Image image);
int addChild(Sprite *parent, Sprite *child);
void drawSprite(Sprite *sprite);

#endif
