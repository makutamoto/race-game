#ifndef SCENE_H
#define SCENE_H

#include "../include/sprite.h"
#include "../include/vector.h"

typedef struct {
  Vector children;
  char background;
} Scene;

Scene initScene();
void drawScene(Scene *scene);
void discardScene(Scene *scene);

#endif
