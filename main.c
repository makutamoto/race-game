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

#define HERO_BULLET_COLLISIONMASK 0x01
#define ENEMY_BULLET_COLLISIONMASK 0x02

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

static Shape enemy1Shape;
static Shape enemyLifeShape;
static Shape heroBulletShape;
static Shape enemyBulletShape;

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

static int bulletBehaviour(Sprite *sprite) {
	if(sprite->collisionTargets.length > 0 || distance2(heroSprite.position, sprite->position) > 300) {
		removeByData(&scene.objects, sprite);
		free(sprite);
		return FALSE;
	}
	return TRUE;
}

static void shootBullet(const char *name, Shape shape, const float position[3], float angle, unsigned int collisionMask) {
	Sprite *bullet = malloc(sizeof(Sprite));
	*bullet = initSprite(name, NULL);
	bullet->shape = shape;
	bullet->velocity[0] = 2.0F * cosf(angle - PI / 2.0F);
	bullet->velocity[2] = -2.0F * sinf(angle - PI / 2.0F);
	bullet->angle[1] = angle;
	bullet->position[0] = position[0];
	bullet->position[1] = position[1];
	bullet->position[2] = position[2];
	bullet->collisionMask = collisionMask;
	bullet->behaviour = bulletBehaviour;
	push(&scene.objects, bullet);
}

static int enemy1Behaviour(Sprite *sprite) {
	// float direction[2];
	// if(sprite->collisionTarget != NULL && !strcmp(sprite->collisionTarget->name, "bullet")) {
		//   removeByData(&scene.objects, sprite->collisionTarget);
		//   sprite->collisionTarget = NULL;
		// }
		// direction2(heroSprite.position, sprite->position, direction);
		// mulVec2ByScalar(direction,  min(1.0F, distance2(heroSprite.position, sprite->position) - 50.0F), direction);
		// addVec2(sprite->position, direction, sprite->position);
		shootBullet("enemyBullet", enemyBulletShape, sprite->position, PI / 2.0F, ENEMY_BULLET_COLLISIONMASK);
		shootBullet("enemyBullet", enemyBulletShape, sprite->position, PI, ENEMY_BULLET_COLLISIONMASK);
		shootBullet("enemyBullet", enemyBulletShape, sprite->position, -PI, ENEMY_BULLET_COLLISIONMASK);
		sprite->position[2] -= 0.1F;
		return TRUE;
}

static int enemy1BehaviourInterval(Sprite *sprite) {
		shootBullet("enemyBullet", enemyBulletShape, sprite->position, PI / 2.0F, ENEMY_BULLET_COLLISIONMASK);
		shootBullet("enemyBullet", enemyBulletShape, sprite->position, PI, ENEMY_BULLET_COLLISIONMASK);
		shootBullet("enemyBullet", enemyBulletShape, sprite->position, -PI, ENEMY_BULLET_COLLISIONMASK);
		return TRUE;
}

static Sprite* spawnEnemy1(float x, float y, float z) {
	Sprite *enemy = malloc(sizeof(Sprite));
	Sprite *bar = malloc(sizeof(Sprite));
	*enemy = initSprite("Enemy1", &enemy1);
	*bar = initSprite("EnemyLifeBar", NULL);
	enemy->shape = enemy1Shape;
	enemy->position[0] = x;
	enemy->position[1] = y;
	enemy->position[2] = z;
	enemy->scale[0] = 32.0F;
	enemy->scale[1] = 32.0F;
	enemy->scale[2] = 32.0F;
	enemy->collisionMask = HERO_BULLET_COLLISIONMASK;
	enemy->behaviour = enemy1Behaviour;
	addIntervalEvent(enemy, 1000, enemy1BehaviourInterval);
	bar->shape = enemyLifeShape;
	bar->position[1] = 16.0F;
	bar->isInterface = TRUE;
	push(&enemy->children, bar);
	push(&scene.objects, enemy);
	return enemy;
}

static int heroBehaviour(Sprite *sprite) {
	float move[2];
	addVec2(sprite->position, mulVec2ByScalar(controller.move, 1.0F, move), sprite->position);
	sprite->position[0] = max(min(sprite->position[0], HALF_FIELD_SIZE), -HALF_FIELD_SIZE);
	sprite->position[1] = max(min(sprite->position[1], HALF_FIELD_SIZE), -HALF_FIELD_SIZE);
	sprite->angle[1] = angleVec2(controller.direction) + PI / 2.0F;
	if(controller.action) {
		shootBullet("heroBullet", heroBulletShape, sprite->position, sprite->angle[1], HERO_BULLET_COLLISIONMASK);
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
	enemy1 = loadBitmap("assets/enemy1.bmp", NULL_COLOR);
	stage = loadBitmap("assets/stage.bmp", NULL_COLOR);
	scene = initScene();
	scene.background = BLUE;
	initShapeFromObj(&enemy1Shape, "./assets/enemy1.obj");
	enemyLifeShape = initShapePlane(20, 5, RED);
	heroBulletShape = initShapeBox(5, 5, 30, YELLOW);
	enemyBulletShape = initShapeBox(5, 5, 30, MAGENTA);
	// lifeBarSprite = initSprite("heroSprite", lifeBar);
	stageSprite = initSprite("stage", &stage);
	stageSprite.shape = initShapeBox(100, 100, 100, YELLOW);
	// initShapeFromObj(&stageSprite.shape, "./assets/test.obj");
	stageSprite.position[2] = 10.0;
	// lifeBarSprite.position[0] = 0.01;
	// lifeBarSprite.position[1] = 0.01;
	heroSprite = initSprite("Hero", &hero);
	initShapeFromObj(&heroSprite.shape, "./assets/hero.obj");
	heroSprite.scale[0] = 32.0F;
	heroSprite.scale[1] = 32.0F;
	heroSprite.scale[2] = 32.0F;
	heroSprite.collisionMask = ENEMY_BULLET_COLLISIONMASK;
	heroSprite.behaviour = heroBehaviour;
	// push(&scene.interfaces, &lifeBarSprite);
	push(&scene.objects, &heroSprite);
	// push(&scene.objects, &stageSprite);
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
						controller.move[0] = 1.0F;
						break;
					case 'D':
						controller.move[0] = -1.0F;
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
	discardShape(heroSprite.shape);
	discardShape(stageSprite.shape);
	discardShape(enemy1Shape);
	discardShape(enemyLifeShape);
	discardShape(heroBulletShape);
	discardShape(enemyBulletShape);
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


	spawnEnemy1(0.0F, 0.0F, 100.0F);


	while(TRUE) {
		if(!pollEvents()) break;
		scene.camera.target[0] = heroSprite.position[0];
		scene.camera.target[1] = heroSprite.position[1];
		drawScene(&scene, screen);
	}
	deinitialize();
	return 0;
}
