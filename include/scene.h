#ifndef SCENE_H
#define SCENE_H

#include "../include/sprite.h"
#include "../include/vector.h"

typedef struct {
  float position[3];
  float target[3];
  float worldUp[3];
  float fov;
  float nearLimit;
  float farLimit;
  float aspect;
} Camera;

typedef struct {
  Vector objects;
  Vector interfaces;
  unsigned char background;
  Camera camera;
} Scene;

Scene initScene(void);
void drawScene(Scene *scene);
void discardScene(Scene *scene);

#endif
