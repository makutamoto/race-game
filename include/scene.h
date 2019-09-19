#ifndef SCENE_H
#define SCENE_H

#include<Windows.h>

#include "../include/node.h"
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
  Vector nodes;
  unsigned char background;
  Camera camera;
} Scene;

Scene initScene(void);
void drawScene(Scene *scene, HANDLE screen);
void discardScene(Scene *scene);

#endif
