#include<Windows.h>

#include "./include/controller.h"
#include "./include/vector.h"

#define NOF_MAX_EVENTS 10

static HANDLE input;
static INPUT_RECORD inputRecords[NOF_MAX_EVENTS];

void initInput(void) {
  input = GetStdHandle(STD_INPUT_HANDLE);
}

Controller initController(void) {
  Controller controller = { 0 };
  return controller;
}

ControllerEvent initControllerEvent(WORD key, float down, float up, float *dest) {
  ControllerEvent event;
  event.key = key;
  event.valueDown = down;
  event.valueUp = up;
  event.dest = dest;
  return event;
}

void initControllerEventCross(ControllerEvent events[4], WORD up, WORD left, WORD down, WORD right, float dest[2]) {
  events[0] = initControllerEvent(up, 1.0F, 0.0F, &dest[1]);
  events[1] = initControllerEvent(left, -1.0F, 0.0F, &dest[0]);
  events[2] = initControllerEvent(down, -1.0F, 0.0F, &dest[1]);
  events[3] = initControllerEvent(right, 1.0F, 0.0F, &dest[0]);
}

void updateController(Controller controller) {
  int i;
	DWORD nofEvents;
	KEY_EVENT_RECORD *keyEvent;
  ControllerEvent *controllerEvent;
	GetNumberOfConsoleInputEvents(input, &nofEvents);
	if(nofEvents == 0) return;
	ReadConsoleInput(input, inputRecords, NOF_MAX_EVENTS, &nofEvents);
	for(i = 0;i < (int)nofEvents;i += 1) {
		switch(inputRecords[i].EventType) {
			case KEY_EVENT:
			  keyEvent = &inputRecords[i].Event.KeyEvent;
        resetIteration(&controller.events);
        controllerEvent = nextData(&controller.events);
        while(controllerEvent) {
          if(keyEvent->wVirtualKeyCode == controllerEvent->key) {
            if(keyEvent->bKeyDown) {
              *controllerEvent->dest = controllerEvent->valueDown;
            } else {
              *controllerEvent->dest = controllerEvent->valueUp;
            }
          }
          controllerEvent = nextData(&controller.events);
        }
			  break;
			default: break;
		}
	}
}
