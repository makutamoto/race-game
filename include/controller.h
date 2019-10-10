#ifndef CONTROLLER_H
#define CONTROLLER_H

#include<Windows.h>

#include "vector.h"

typedef struct {
  Vector events;
} Controller;

typedef struct {
  WORD key;
  float valueDown;
  float valueUp;
  float *dest;
} ControllerEvent;

void initInput(void);
Controller initController(void);
ControllerEvent initControllerEvent(WORD key, float down, float up, float *dest);
void initControllerEventCross(ControllerEvent events[4], WORD up, WORD left, WORD down, WORD right, float dest[2]);
void updateController(Controller controller);

#endif
