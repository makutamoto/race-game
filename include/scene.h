#ifndef SCENE_H
#define SCENE_H

#include "../include/sprite.h"
#include "../include/vector.h"

typedef struct {
  Vector children;
  unsigned char background;
} Scene;

Scene initScene(void);
void drawScene(Scene *scene);
void discardScene(Scene *scene);

#endif
