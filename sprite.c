#include<windows.h>

#include "./include/sprite.h"
#include "./include/graphics.h"

void initSprite(Sprite *sprite, const char *id, Image *image) {
	memcpy_s(sprite->id, sizeof(sprite->id), id, min( sizeof(sprite->id), strlen(id)));
	sprite->velocity[0] = 0.0;
	sprite->velocity[1] = 0.0;
	sprite->angVelocity = 0.0;
	sprite->position[0] = 0.0;
	sprite->position[1] = 0.0;
	sprite->angle = 0.0;
	sprite->scale[0] = 1.0;
	sprite->scale[1] = 1.0;
	sprite->shadowScale = 0.0;
	sprite->shadowOffset[0] = 0.0;
	sprite->shadowOffset[1] = 0.0;
	sprite->image = image;
	sprite->parent = NULL;
	int i;
	for(i = 0;i < sizeof(sprite->children) / sizeof(Sprite*);i++) sprite->children[i] = NULL;
}

int addChild(Sprite *parent, Sprite *child) {
	int i;
	for(i = 0;i < sizeof(parent->children) / sizeof(Sprite*);i++) {
		if(parent->children[i] == NULL) {
			parent->children[i] = child;
			child->parent = parent;
			return TRUE;
		}
	}
	return FALSE;
}

void drawSprite(Sprite sprite) {
	pushTransformation();
	scaleTransformation(sprite.scale[0], sprite.scale[1]);
	rotateTransformation(sprite.angle);
	pushTransformation();
	scaleTransformation(sprite.shadowScale, sprite.shadowScale);
	translateTransformation(sprite.position[0] + sprite.shadowOffset[0], sprite.position[1] + sprite.shadowOffset[1]);
	if(sprite.shadowOffset[0] != 0.0 || sprite.shadowOffset[1] != 0.0) fillBuffer(*sprite.image, TRUE);
	popTransformation();
	translateTransformation(sprite.position[0], sprite.position[1]);
	int i;
	for(i = 0;i < sizeof(sprite.children) / sizeof(Sprite*);i++) {
		if(sprite.children[i] != NULL) drawSprite(*sprite.children[i]);
	}
	fillBuffer(*sprite.image, FALSE);
	popTransformation();
}
