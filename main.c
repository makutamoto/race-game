#include<stdio.h>
#include<math.h>
#include<stdint.h>
#include<windows.h>
#include<time.h>

#include "./include/matrix.h"
#include "./include/graphics.h"
#include "./include/colors.h"
#include "./include/scene.h"
#include "./include/sprite.h"
#include "./include/vector.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

#define NOF_MAX_EVENTS 10

HWND window;
HANDLE input;
HANDLE doubleScreens[2];
HANDLE visibleScreen, hiddenScreen;
char *buffer;
size_t bufferLength;
COORD bufferSizeCoord = { 125, 250 };
INPUT_RECORD inputRecords[NOF_MAX_EVENTS];
struct {
	float move[2];
	float direction[2];
	BOOL action;
} controller;

Image lifeBar;
Image hero;
Image heroBullet;
Image enemy1;

Sprite lifeBarSprite;
Scene scene;
Sprite heroSprite;
Sprite enemy1Sprite;

RECT storedSize;
void pushWindowSize() {
	WINDOWINFO info;
	info.cbSize = sizeof(WINDOWINFO);
	GetWindowInfo(window, &info);
	storedSize = info.rcWindow;
}
void popWindowSize() {
	MoveWindow(window, storedSize.left, storedSize.top, storedSize.right - storedSize.left, storedSize.bottom - storedSize.top, FALSE);
}

HANDLE createScreen() {
	static CONSOLE_CURSOR_INFO info = { 1, FALSE };
	COORD actualBufferSize;
	CONSOLE_FONT_INFOEX font = { .cbSize = sizeof(CONSOLE_FONT_INFOEX) };
	HANDLE screen = CreateConsoleScreenBuffer(GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	pushWindowSize();
	SetConsoleActiveScreenBuffer(screen);
	SetConsoleCursorInfo(screen, &info);
	GetCurrentConsoleFontEx(screen, FALSE, &font);
	font.dwFontSize.X = 1;
	font.dwFontSize.Y = 1;
	SetCurrentConsoleFontEx(screen, FALSE, &font);
	actualBufferSize.X = 2 * bufferSizeCoord.X;
	actualBufferSize.Y = bufferSizeCoord.Y;
	SetConsoleScreenBufferSize(screen, actualBufferSize);
	popWindowSize();
	return screen;
}

void swapScreens() {
	static int visible = 1;
	hiddenScreen = doubleScreens[visible];
	visibleScreen = doubleScreens[(visible ^= 1)];
	SetConsoleActiveScreenBuffer(visibleScreen);
}

int enemy1Behaviour(Sprite *sprite) {
	float direction[2];
  // if(sprite->collisionTarget != NULL && !strcmp(sprite->collisionTarget->name, "bullet")) {
  //   removeByData(&scene.children, sprite->collisionTarget);
  //   sprite->collisionTarget = NULL;
  // }
  direction2(heroSprite.position, sprite->position, direction);
  mulVec2ByScalar(direction,  min(3.0, distance2(heroSprite.position, sprite->position) - 50.0), direction);
	addVec2(sprite->position, direction, sprite->position);
	return TRUE;
}

int bulletBehaviour(Sprite *sprite) {
	if(distance2(heroSprite.position, sprite->position) > 100) {
		removeByData(&scene.children, sprite);
		free(sprite);
		return FALSE;
	}
	sprite->position[0] += 1.0 * cos(sprite->angle - PI / 2.0);
	sprite->position[1] += 1.0 * sin(sprite->angle - PI / 2.0);
	return TRUE;
}

int heroBehaviour(Sprite *sprite) {
	float move[2];
	addVec2(sprite->position, mulVec2ByScalar(controller.move, 0.75, move), sprite->position);
	sprite->position[0] = max(min(sprite->position[0], bufferSizeCoord.X), 0.0);
	sprite->position[1] = max(min(sprite->position[1], bufferSizeCoord.Y / 2.0), 0.0);
	sprite->angle = angleVec2(controller.direction) + PI / 2.0;
	if(controller.action) {
		Sprite *bullet = malloc(sizeof(Sprite));
		*bullet = initSprite("heroBullet", heroBullet);
		bullet->angle = sprite->angle;
		bullet->position[0] = sprite->position[0];
		bullet->position[1] = sprite->position[1];
		bullet->behaviour = bulletBehaviour;
		push(&scene.children, bullet);
		controller.action = FALSE;
	}
	return TRUE;
}

void initialize() {
	window = GetConsoleWindow();
	input = GetStdHandle(STD_INPUT_HANDLE);
	doubleScreens[0] = createScreen();
	doubleScreens[1] = createScreen();
	swapScreens();
	bufferLength = bufferSizeCoord.X * bufferSizeCoord.Y;
	buffer = (char*)malloc(bufferLength);
	lifeBar = genRect(50, 10, RED);
	hero = loadBitmap("assets/hero.bmp", BLACK);
	heroBullet = loadBitmap("assets/heroBullet.bmp", WHITE);
	enemy1 = loadBitmap("assets/enemy1.bmp", BLACK);
	scene = initScene();
	scene.background = BLUE;
	lifeBarSprite = initSprite("heroSprite", lifeBar);
	lifeBarSprite.position[0] = 30.0;
	lifeBarSprite.position[1] = 10.0;
	lifeBarSprite.shadowScale = 0.5;
	lifeBarSprite.shadowOffset[0] = 1.0;
	lifeBarSprite.shadowOffset[1] = 1.0;
	heroSprite = initSprite("Hero", hero);
	heroSprite.shadowScale = 0.75;
	heroSprite.shadowOffset[1] = 10.0;
	heroSprite.behaviour = heroBehaviour;
	enemy1Sprite = initSprite("Enemy1", enemy1);
	enemy1Sprite.shadowScale = 0.75;
	enemy1Sprite.shadowOffset[1] = 10.0;
	enemy1Sprite.behaviour = enemy1Behaviour;
	push(&scene.children, &lifeBarSprite);
	push(&scene.children, &heroSprite);
	push(&scene.children, &enemy1Sprite);
}

BOOL pollEvents() {
	DWORD nofEvents;
	GetNumberOfConsoleInputEvents(input, &nofEvents);
	if(nofEvents == 0) return TRUE;
	ReadConsoleInput(input, inputRecords, NOF_MAX_EVENTS, &nofEvents);
	for(int i = 0;i < nofEvents;i += 1) {
		switch(inputRecords[i].EventType) {
			case KEY_EVENT:
			KEY_EVENT_RECORD *keyEvent = &inputRecords[i].Event.KeyEvent;
			if(keyEvent->bKeyDown) {
				switch(keyEvent->wVirtualKeyCode) {
					case 'Q': return FALSE;
					case 'W':
						controller.move[1] -= 1.0;
						break;
					case 'S':
						controller.move[1] = 1.0;
						break;
					case 'A':
						controller.move[0] = -1.0;
						break;
					case 'D':
						controller.move[0] = 1.0;
						break;
					case VK_UP:
						controller.direction[1] = -1.0;
						break;
					case VK_DOWN:
						controller.direction[1] = 1.0;
						break;
					case VK_LEFT:
						controller.direction[0] = -1.0;
						break;
					case VK_RIGHT:
						controller.direction[0] = 1.0;
						break;
					case VK_SPACE:
						controller.action = TRUE;
						break;
				}
			} else {
				switch(keyEvent->wVirtualKeyCode) {
					case 'W':
					case 'S':
						controller.move[1] = 0.0;
						break;
					case 'A':
					case 'D':
						controller.move[0] = 0.0;
						break;
					case VK_UP:
					case VK_DOWN:
						controller.direction[1] = 0.0;
						break;
					case VK_LEFT:
					case VK_RIGHT:
						controller.direction[0] = 0.0;
						break;
					case VK_SPACE:
						controller.action = FALSE;
						break;
				}
			}
			break;
			default: break;
		}
	}
	return TRUE;
}

void flush() {
	DWORD nofWritten;
	static COORD cursor = { 0, 0 };
	WORD *data = (WORD*)malloc(2 * bufferLength * sizeof(WORD));
	size_t index;
	for(index = 0;index < bufferLength;index++) {
		WORD attribute = BACKGROUND_BLUE - 1 + (((buffer[index] & 8) | ((buffer[index] & 1) << 2) | (buffer[index] & 2) | ((buffer[index] & 4) >> 2)) << 4);
		data[2 * index] = attribute;
		data[2 * index + 1] = attribute;
	}
	WriteConsoleOutputAttribute(hiddenScreen, data, bufferLength, cursor, &nofWritten);
	free(data);
	swapScreens();
}

void deinitialize() {
	freeImage(lifeBar);
	freeImage(hero);
	freeImage(heroBullet);
	discardScene(&scene);
	free(buffer);
}

int main() {
	initialize();
	while(TRUE) {
		if(!pollEvents()) break;
		memset(buffer, 0, bufferLength);
		clearTransformation();
		drawScene(&scene);
		flush();
	}
	deinitialize();
	return 0;
}
