#include<stdio.h>
#include<math.h>
#include<stdint.h>
#include<Windows.h>
#include<time.h>

#include "./include/matrix.h"
#include "./include/graphics.h"
#include "./include/colors.h"
#include "./include/scene.h"
#include "./include/sprite.h"
#include "./include/vector.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "Winmm.lib")

#define NOF_MAX_EVENTS 10
#define HALF_FIELD_SIZE 100.0F

static HWND window;
static HANDLE input;
static HANDLE screen;
static INPUT_RECORD inputRecords[NOF_MAX_EVENTS];
static struct {
	float move[2];
	float direction[2];
	BOOL action;
} controller;

static Image lifeBar;
static Image hero;
static Image heroBullet;
static Image enemy1;
static Image stage;

static Sprite lifeBarSprite;
static Scene scene;
static Sprite heroSprite;
static Sprite stageSprite;

static void initScreen(short width, short height) {
	CONSOLE_CURSOR_INFO info = { 1, FALSE };
	COORD bufferSize;
	CONSOLE_FONT_INFOEX font = { .cbSize = sizeof(CONSOLE_FONT_INFOEX) };
	screen = CreateConsoleScreenBuffer(GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(screen);
	GetCurrentConsoleFontEx(screen, FALSE, &font);
	font.dwFontSize.X = 3;
	font.dwFontSize.Y = 5;
	SetCurrentConsoleFontEx(screen, FALSE, &font);
	bufferSize.X = 2 * width;
	bufferSize.Y = height;
	SetConsoleScreenBufferSize(screen, bufferSize);
	SetConsoleCursorInfo(screen, &info);
}

static int enemy1Behaviour(Sprite *sprite) {
	float direction[2];
	// if(sprite->collisionTarget != NULL && !strcmp(sprite->collisionTarget->name, "bullet")) {
		//   removeByData(&scene.objects, sprite->collisionTarget);
		//   sprite->collisionTarget = NULL;
		// }
		direction2(heroSprite.position, sprite->position, direction);
		mulVec2ByScalar(direction,  min(1.0F, distance2(heroSprite.position, sprite->position) - 50.0F), direction);
		addVec2(sprite->position, direction, sprite->position);
		return TRUE;
}

static Sprite* spawnEnemy1(float x, float y) {
	Sprite *enemy = malloc(sizeof(Sprite));
	Sprite *bar = malloc(sizeof(Sprite));
	// *enemy = initSprite("Enemy1", enemy1);
	// *bar = initSprite("EnemyLifeBar", genRect(20, 5, RED));
	enemy->position[0] = x;
	enemy->position[1] = y;
	enemy->shadowScale = 0.75;
	enemy->shadowOffset[1] = 10.0;
	enemy->behaviour = enemy1Behaviour;
	bar->position[1] = enemy->texture->height / 2.0F + 5.0F;
	bar->isInterface = TRUE;
	push(&enemy->children, bar);
	push(&scene.objects, enemy);
	return enemy;
}

static int bulletBehaviour(Sprite *sprite) {
	if(distance2(heroSprite.position, sprite->position) > 300) {
		removeByData(&scene.objects, sprite);
		free(sprite);
		return FALSE;
	}
	sprite->position[0] += -2.0F * cosf(sprite->angle[1] - PI / 2.0F);
	sprite->position[2] += 2.0F * sinf(sprite->angle[1] - PI / 2.0F);
	return TRUE;
}

static int heroBehaviour(Sprite *sprite) {
	float move[2];
	addVec2(sprite->position, mulVec2ByScalar(controller.move, 1.0F, move), sprite->position);
	sprite->position[0] = max(min(sprite->position[0], HALF_FIELD_SIZE), -HALF_FIELD_SIZE);
	sprite->position[1] = max(min(sprite->position[1], HALF_FIELD_SIZE), -HALF_FIELD_SIZE);
	sprite->angle[1] = angleVec2(controller.direction) + PI / 2.0F;
	if(controller.action) {
		Sprite *bullet = malloc(sizeof(Sprite));
		*bullet = initSprite("heroBullet", NULL);
		genPolygonsBox(5, 5, 30, bullet, YELLOW);
		bullet->angle[1] = sprite->angle[1];
		bullet->position[0] = sprite->position[0];
		bullet->position[1] = sprite->position[1];
		bullet->behaviour = bulletBehaviour;
		push(&scene.objects, bullet);
		controller.action = FALSE;
		PlaySound(TEXT("assets/laser.wav"), NULL, SND_ASYNC | SND_FILENAME);
	}
	return TRUE;
}

static void initialize(void) {
	window = GetConsoleWindow();
	input = GetStdHandle(STD_INPUT_HANDLE);
	initScreen(200, 200);
	initGraphics(200, 200);
	lifeBar = genRect(5, 1, RED);
	hero = loadBitmap("assets/hero3d.bmp", NULL_COLOR);
	heroBullet = loadBitmap("assets/heroBullet.bmp", WHITE);
	enemy1 = loadBitmap("assets/enemy1.bmp", BLACK);
	stage = loadBitmap("assets/texture.bmp", NULL_COLOR);
	scene = initScene();
	scene.background = BLUE;
	// lifeBarSprite = initSprite("heroSprite", lifeBar);
	stageSprite = initSprite("stage", &stage);
	// genPolygonsBox(100, 100, 100, &stageSprite.indices, &stageSprite.vertices, &stageSprite.uv, &stageSprite.uvIndices);
	readObj("./assets/test.obj", &stageSprite);
	stageSprite.position[2] = 10.0;
	// lifeBarSprite.position[0] = 0.01;
	// lifeBarSprite.position[1] = 0.01;
	heroSprite = initSprite("Hero", &hero);
	readObj("./assets/hero.obj", &heroSprite);
	heroSprite.scale[0] = 32.0F;
	heroSprite.scale[1] = 32.0F;
	heroSprite.scale[2] = 32.0F;
	heroSprite.shadowScale = 0.75;
	heroSprite.shadowOffset[1] = 10.0;
	heroSprite.behaviour = heroBehaviour;
	// push(&scene.interfaces, &lifeBarSprite);
	push(&scene.objects, &heroSprite);
	push(&scene.objects, &stageSprite);
	// spawnEnemy1(0.0F, -2000.0F);
}

static BOOL pollEvents(void) {
	DWORD nofEvents;
	GetNumberOfConsoleInputEvents(input, &nofEvents);
	if(nofEvents == 0) return TRUE;
	ReadConsoleInput(input, inputRecords, NOF_MAX_EVENTS, &nofEvents);
	for(int i = 0;i < (int)nofEvents;i += 1) {
		switch(inputRecords[i].EventType) {
			case KEY_EVENT:
			KEY_EVENT_RECORD *keyEvent = &inputRecords[i].Event.KeyEvent;
			if(keyEvent->bKeyDown) {
				switch(keyEvent->wVirtualKeyCode) {
					case 'Q': return FALSE;
					case 'W':
						controller.move[1] = -1.0F;
						break;
					case 'S':
						controller.move[1] = 1.0F;
						break;
					case 'A':
						controller.move[0] = -1.0F;
						break;
					case 'D':
						controller.move[0] = 1.0F;
						break;
					case VK_UP:
						controller.direction[1] = -1.0F;
						break;
					case VK_DOWN:
						controller.direction[1] = 1.0F;
						break;
					case VK_LEFT:
						controller.direction[0] = 1.0F;
						break;
					case VK_RIGHT:
						controller.direction[0] = -1.0F;
						break;
					case VK_SPACE:
						controller.action = TRUE;
						break;
				}
			} else {
				switch(keyEvent->wVirtualKeyCode) {
					case 'W':
					case 'S':
						controller.move[1] = 0.0F;
						break;
					case 'A':
					case 'D':
						controller.move[0] = 0.0F;
						break;
					case VK_UP:
					case VK_DOWN:
						controller.direction[1] = 0.0F;
						break;
					case VK_LEFT:
					case VK_RIGHT:
						controller.direction[0] = 0.0F;
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

static void deinitialize(void) {
	discardSprite(lifeBarSprite);
	discardSprite(heroSprite);
	discardSprite(stageSprite);
	freeImage(lifeBar);
	freeImage(hero);
	freeImage(heroBullet);
	discardScene(&scene);
	deinitGraphics();
}

int main(void) {
	initialize();
	while(TRUE) {
		if(!pollEvents()) break;
		scene.camera.target[0] = heroSprite.position[0];
		scene.camera.target[1] = heroSprite.position[1];
		drawScene(&scene, screen);
	}
	deinitialize();
	return 0;
}
