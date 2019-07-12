#include<Windows.h>

#include "./include/sprite.h"
#include "./include/graphics.h"
#include "./include/vector.h"

Sprite initSprite(const char *id, Image image) {
  Sprite sprite;
  memset(&sprite, 0, sizeof(sprite));
	memcpy_s(sprite.id, sizeof(sprite.id), id, min(sizeof(sprite.id), strlen(id)));
	sprite.scale[0] = 1.0;
	sprite.scale[1] = 1.0;
  sprite.scale[2] = 1.0;
	sprite.texture = image;
  sprite.children = initVector();
  return sprite;
}

void drawSprite(Sprite *sprite) {
  static unsigned long indices[6] = { 0, 1, 2, 1, 3, 2 };
  Vertex vertices[4] = {
    { { 0.0F, 0.0F, 0.0F, 1.0F }, 0, { 0.0F, 0.0F } },
    { { 0.0F, 0.0F, 0.0F, 1.0F }, 0, { 0.0F, 1.0F } },
    { { 0.0F, 0.0F, 0.0F, 1.0F }, 0, { 1.0F, 0.0F } },
    { { 0.0F, 0.0F, 0.0F, 1.0F }, 0, { 1.0F, 1.0F } },
  };
  Sprite *child;
  float halfWidth = sprite->texture.width / 2.0F;
  float halfHeight = sprite->texture.height / 2.0F;
  vertices[0].components[0] = -halfWidth;
  vertices[0].components[1] = -halfHeight;
  vertices[1].components[0] = -halfWidth;
  vertices[1].components[1] = halfHeight;
  vertices[2].components[0] = halfWidth;
  vertices[2].components[1] = -halfHeight;
  vertices[3].components[0] = halfWidth;
  vertices[3].components[1] = halfHeight;
  if(sprite->behaviour != NULL) {
    if(!sprite->behaviour(sprite)) return;
  }
	pushTransformation();
  translateTransformation(sprite->position[0], sprite->position[1], sprite->position[2]);
  rotateTransformation(sprite->angle[0], sprite->angle[1], sprite->angle[2]);
	// pushTransformation();
  // translateTransformation(sprite->shadowOffset[0], sprite->shadowOffset[1], 0.0);
	// scaleTransformation(sprite->shadowScale, sprite->shadowScale, 1.0);
	// if(sprite->shadowOffset[0] != 0.0F || sprite->shadowOffset[1] != 0.0F) fillBuffer(sprite->texture, TRUE);
	// popTransformation();
  resetIteration(&sprite->children);
  while((child = previousData(&sprite->children))) drawSprite(child);
  scaleTransformation(sprite->scale[0], sprite->scale[1], sprite->scale[2]);
	fillPolygons(vertices, indices, 6, sprite->texture);
	popTransformation();
}
