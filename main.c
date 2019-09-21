#include<stdio.h>
#include<math.h>
#include<Windows.h>
#include<time.h>

#include "./include/borland.h"
#include "./include/matrix.h"
#include "./include/graphics.h"
#include "./include/colors.h"
#include "./include/scene.h"
#include "./include/node.h"
#include "./include/vector.h"

#ifndef __BORLANDC__
#pragma comment(lib, "Winmm.lib")
#endif

#define NOF_MAX_EVENTS 10

#define HERO_BULLET_COLLISIONMASK 0x01
#define ENEMY_BULLET_COLLISIONMASK 0x02
#define OBSTACLE_COLLISIONMASK 0x04

static HANDLE input;
static HANDLE screen;
static INPUT_RECORD inputRecords[NOF_MAX_EVENTS];
static struct {
	float move[2];
	float direction[2];
	BOOL action;
	BOOL retry;
} controller;

static Image lifeBarBunch;
static Image hero;
static Image heroBullet;
static Image enemy1;
static Image stage;
static Image stoneImage;
static Image gameoverImage;

static Shape enemy1Shape;
static Shape enemyLifeShape;
static Shape heroBulletShape;
static Shape enemyBulletShape;
static Shape stoneShape;

static Node lifeBarNode;
static Scene scene;
static Scene gameoverScene;
static Node heroNode;
static Node stageNode;
static Node gameoverNode;

static unsigned int heroHP;

typedef struct {
	unsigned int hp;
	Node *bar;
} Enemy1;

static void initScreen(short width, short height) {
	CONSOLE_CURSOR_INFO info = { 1, FALSE };
	COORD bufferSize;
	#ifndef __BORLANDC__
	CONSOLE_FONT_INFOEX font = { sizeof(CONSOLE_FONT_INFOEX) };
	#endif
	screen = CreateConsoleScreenBuffer(GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(screen);
	#ifndef __BORLANDC__
	GetCurrentConsoleFontEx(screen, FALSE, &font);
	font.dwFontSize.X = 1;
	font.dwFontSize.Y = 2;
	SetCurrentConsoleFontEx(screen, FALSE, &font);
	// Specify font family.
	#endif
	bufferSize.X = 2 * width;
	bufferSize.Y = height;
	SetConsoleScreenBufferSize(screen, bufferSize);
	SetConsoleCursorInfo(screen, &info);
}

static int bulletBehaviour(Node *node) {
	if(node->collisionFlags || distance2(heroNode.position, node->position) > 500) {
		removeByData(&scene.nodes, node);
		free(node);
		return FALSE;
	}
	return TRUE;
}

static void shootBullet(const char *name, Shape shape, const float position[3], float angle, unsigned int collisionMask) {
	Node *bullet = malloc(sizeof(Node));
	*bullet = initNode(name, NO_IMAGE);
	bullet->shape = shape;
	bullet->velocity[0] = 4.0F * cosf(angle - PI / 2.0F);
	bullet->velocity[2] = -4.0F * sinf(angle - PI / 2.0F);
	bullet->angle[1] = angle;
	bullet->position[0] = position[0];
	bullet->position[1] = position[1];
	bullet->position[2] = position[2];
	bullet->collisionMaskActive = collisionMask;
	bullet->behaviour = bulletBehaviour;
	push(&scene.nodes, bullet);
}

static int enemy1Behaviour(Node *node) {
	if(node->collisionFlags & HERO_BULLET_COLLISIONMASK) {
		Enemy1 *enemy = node->data;
		enemy->hp -= 1;
		enemy->bar->texture = cropImage(lifeBarBunch, 192, 32, 0, 10 * enemy->hp / 3);
		if(enemy->hp <= 0) {
			removeByData(&scene.nodes, node);
			free(node);
			return FALSE;
		}
	}
	return TRUE;
}

static void enemy1BehaviourInterval(Node *node) {
	shootBullet("enemyBullet", enemyBulletShape, node->position, PI / 4.0F * 3.0F, ENEMY_BULLET_COLLISIONMASK);
	shootBullet("enemyBullet", enemyBulletShape, node->position, PI, ENEMY_BULLET_COLLISIONMASK);
	shootBullet("enemyBullet", enemyBulletShape, node->position, PI / 4.0F * 5.0F, ENEMY_BULLET_COLLISIONMASK);
}

static void spawnEnemy1(float x, float y, float z) {
	Node *enemy = malloc(sizeof(Node));
	Node *bar = malloc(sizeof(Node));
	Enemy1 *data = malloc(sizeof(Enemy1));
	*enemy = initNode("Enemy1", enemy1);
	*bar = initNode("EnemyLifeBar", cropImage(lifeBarBunch, 192, 32, 0, 10));
	data->hp = 3;
	data->bar = bar;
	enemy->shape = enemy1Shape;
	enemy->position[0] = x;
	enemy->position[1] = y;
	enemy->position[2] = z;
	enemy->velocity[2] = -0.5F;
	enemy->scale[0] = 32.0F;
	enemy->scale[1] = 32.0F;
	enemy->scale[2] = 32.0F;
	enemy->collisionMaskActive = OBSTACLE_COLLISIONMASK;
	enemy->collisionMaskPassive = HERO_BULLET_COLLISIONMASK;
	enemy->behaviour = enemy1Behaviour;
	enemy->data = data;
	addIntervalEvent(enemy, 1000, enemy1BehaviourInterval);
	bar->shape = enemyLifeShape;
	bar->position[1] = -16.0F;
	push(&enemy->children, bar);
	push(&scene.nodes, enemy);
}

static void spawnStone(float x, float y, float z) {
	Node *stone = malloc(sizeof(Node));
	*stone = initNode("stone", stoneImage);
	stone->shape = stoneShape;
	stone->position[0] = x;
	stone->position[1] = y;
	stone->position[2] = z;
	stone->velocity[2] = -1.0F;
	stone->scale[0] = 50.0F;
	stone->scale[1] = 50.0F;
	stone->scale[2] = 10.0F;
	stone->collisionMaskActive = OBSTACLE_COLLISIONMASK;
	stone->collisionMaskPassive = HERO_BULLET_COLLISIONMASK | ENEMY_BULLET_COLLISIONMASK;
	push(&scene.nodes, stone);
}

static int heroBehaviour(Node *node) {
	float move[2];
	if(node->collisionFlags) {
		if(node->collisionFlags & ENEMY_BULLET_COLLISIONMASK) heroHP -= 1;
		if(node->collisionFlags & OBSTACLE_COLLISIONMASK) heroHP = 0;
		lifeBarNode.texture = cropImage(lifeBarBunch, 192, 32, 0, heroHP);
	}
	mulVec2ByScalar(controller.move, 2.0F, move);
	addVec2(node->position, move, node->position);
	node->position[0] = max(min(node->position[0], 100.0F), -100.0F);
	node->position[1] = max(min(node->position[1], 100.0F), -100.0F);
	if(controller.action) {
		shootBullet("heroBullet", heroBulletShape, node->position, node->angle[1], HERO_BULLET_COLLISIONMASK);
		controller.action = FALSE;
		PlaySound(TEXT("assets/laser.wav"), NULL, SND_ASYNC | SND_FILENAME);
	}
	return TRUE;
}

static void initialize(void) {
	input = GetStdHandle(STD_INPUT_HANDLE);
	initScreen(128, 128);
	initGraphics(128, 128);
	lifeBarBunch = loadBitmap("assets/lifebar.bmp", NULL_COLOR);
	hero = loadBitmap("assets/hero3d.bmp", NULL_COLOR);
	heroBullet = loadBitmap("assets/heroBullet.bmp", WHITE);
	enemy1 = loadBitmap("assets/enemy1.bmp", NULL_COLOR);
	stoneImage = loadBitmap("assets/stone.bmp", NULL_COLOR);
	stage = loadBitmap("assets/stage.bmp", NULL_COLOR);
	scene = initScene();
	scene.camera = initCamera(0.0F, 0.0F, -100.0F, 1.0F);
	scene.background = BLUE;
	initShapeFromObj(&enemy1Shape, "./assets/enemy1.obj");
	initShapeFromObj(&stoneShape, "./assets/stone.obj");
	enemyLifeShape = initShapePlane(20, 5, RED);
	heroBulletShape = initShapeBox(5, 5, 30, YELLOW);
	enemyBulletShape = initShapeBox(5, 5, 30, MAGENTA);
	lifeBarNode = initNodeUI("lifeBarNode", cropImage(lifeBarBunch, 192, 32, 0, 10), BLACK);
	stageNode = initNode("stage", stage);
	initShapeFromObj(&stageNode.shape, "./assets/test.obj");
	stageNode.position[2] = 10.0F;
	lifeBarNode.position[0] = 2.5F;
	lifeBarNode.position[1] = 2.5F;
	lifeBarNode.scale[0] = 30.0F;
	lifeBarNode.scale[1] = 5.0F;
	heroNode = initNode("Hero", hero);
	initShapeFromObj(&heroNode.shape, "./assets/hero.obj");
	heroNode.scale[0] = 32.0F;
	heroNode.scale[1] = 32.0F;
	heroNode.scale[2] = 32.0F;
	heroNode.collisionMaskPassive = ENEMY_BULLET_COLLISIONMASK | OBSTACLE_COLLISIONMASK;
	heroNode.behaviour = heroBehaviour;

	gameoverScene = initScene();
	gameoverImage = loadBitmap("assets/gameover.bmp", NULL_COLOR);
	gameoverNode = initNodeUI("gameover", gameoverImage, BLACK);
	gameoverNode.scale[0] = 100.0F;
	gameoverNode.scale[1] = 100.0F;
	push(&gameoverScene.nodes, &gameoverNode);
}

static void startGame(void) {
	heroHP = 10;
	lifeBarNode.texture = cropImage(lifeBarBunch, 192, 32, 0, 10);
	heroNode.position[0] = 0.0F;
	heroNode.position[1] = 0.0F;
	heroNode.position[2] = 0.0F;

	clearVector(&scene.nodes);
	push(&scene.nodes, &lifeBarNode);
	push(&scene.nodes, &heroNode);
	push(&scene.nodes, &stageNode);
	spawnEnemy1(0.0F, 100.0F, 500.0F);
	spawnEnemy1(0.0F, -100.0F, 500.0F);
	spawnStone(0.0F, 0.0F, 500.0F);
	// add bricks.
}

static BOOL pollEvents(void) {
	int i;
	DWORD nofEvents;
	KEY_EVENT_RECORD *keyEvent;
	GetNumberOfConsoleInputEvents(input, &nofEvents);
	if(nofEvents == 0) return TRUE;
	ReadConsoleInput(input, inputRecords, NOF_MAX_EVENTS, &nofEvents);
	for(i = 0;i < (int)nofEvents;i += 1) {
		switch(inputRecords[i].EventType) {
			case KEY_EVENT:
			keyEvent = &inputRecords[i].Event.KeyEvent;
			if(keyEvent->bKeyDown) {
				switch(keyEvent->wVirtualKeyCode) {
					case 'Q': return FALSE;
					case 'W':
						controller.move[1] = 1.0F;
						break;
					case 'S':
						controller.move[1] = -1.0F;
						break;
					case 'A':
						controller.move[0] = -1.0F;
						break;
					case 'D':
						controller.move[0] = 1.0F;
						break;
					case 'R':
						controller.retry = TRUE;
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
					case 'R':
						controller.retry = FALSE;
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
	discardShape(heroNode.shape);
	discardShape(stageNode.shape);
	discardShape(enemy1Shape);
	discardShape(enemyLifeShape);
	discardShape(heroBulletShape);
	discardShape(enemyBulletShape);
	discardShape(stoneShape);
	discardNode(lifeBarNode);
	discardNode(heroNode);
	discardNode(stageNode);
	freeImage(lifeBarBunch);
	freeImage(hero);
	freeImage(heroBullet);
	freeImage(stoneImage);
	freeImage(gameoverImage);
	discardScene(&scene);
	discardScene(&gameoverScene);
	deinitGraphics();
}

int main(void) {
	initialize();
	startGame();
	while(TRUE) {
		if(!pollEvents()) break;
		if(heroHP > 0) {
			scene.camera.target[0] = heroNode.position[0] / 2.0F;
			drawScene(&scene, screen);
		} else {
			drawScene(&gameoverScene, screen);
			if(controller.retry) startGame();
		}
	}
	deinitialize();
	return 0;
}
