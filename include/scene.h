#ifndef SCENE_H
#define SCENE_H

#include<Windows.h>
#include<time.h>

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
  clock_t previousClock;
  Vector intervalEvents;
} Scene;

typedef struct {
	clock_t begin;
	unsigned int interval;
	void (*callback)(Scene*);
} IntervalEventScene;

Camera initCamera(float x, float y, float z, float aspect);

Scene initScene(void);
void addIntervalEventScene(Scene *scene, unsigned int milliseconds, void (*callback)(Scene*));
void resetSceneClock(Scene *scene);
void drawScene(Scene *scene);
void discardScene(Scene *scene);

#endif
