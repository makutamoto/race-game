#include<stdio.h>
#include<math.h>
#include<stdint.h>
#include<Windows.h>
#include<time.h>

#include "./include/matrix.h"
#include "./include/graphics.h"
#include "./include/colors.h"
#include "./include/scene.h"
#include "./include/node.h"
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

static Node lifeBarNode;
static Scene scene;
static Node heroNode;
static Node stageNode;

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

static int bulletBehaviour(Node *node) {
	if(node->collisionTargets.length > 0 || distance2(heroNode.position, node->position) > 500) {
		removeByData(&scene.objects, node);
		free(node);
		return FALSE;
	}
	return TRUE;
}

static void shootBullet(const char *name, Shape shape, const float position[3], float angle, unsigned int collisionMask) {
	Node *bullet = malloc(sizeof(Node));
	*bullet = initNode(name, NULL);
	bullet->shape = shape;
	bullet->velocity[0] = 2.0F * cosf(angle - PI / 2.0F);
	bullet->velocity[2] = -2.0F * sinf(angle - PI / 2.0F);
	bullet->angle[1] = angle;
	bullet->position[0] = position[0];
	bullet->position[1] = position[1];
	bullet->position[2] = position[2];
	bullet->collisionMaskActive = collisionMask;
	bullet->behaviour = bulletBehaviour;
	push(&scene.objects, bullet);
}

static int enemy1Behaviour(Node *node) {
	// float direction[2];
	// if(node->collisionTarget != NULL && !strcmp(node->collisionTarget->name, "bullet")) {
		//   removeByData(&scene.objects, node->collisionTarget);
		//   node->collisionTarget = NULL;
		// }
		// direction2(heroNode.position, node->position, direction);
		// mulVec2ByScalar(direction,  min(1.0F, distance2(heroNode.position, node->position) - 50.0F), direction);
		// addVec2(node->position, direction, node->position);
	node->position[2] -= 0.1F;
	return TRUE;
}

static int enemy1BehaviourInterval(Node *node) {
	shootBullet("enemyBullet", enemyBulletShape, node->position, PI / 2.0F, ENEMY_BULLET_COLLISIONMASK);
	shootBullet("enemyBullet", enemyBulletShape, node->position, PI, ENEMY_BULLET_COLLISIONMASK);
	shootBullet("enemyBullet", enemyBulletShape, node->position, PI / 2.0F + PI, ENEMY_BULLET_COLLISIONMASK);
	return TRUE;
}

static Node* spawnEnemy1(float x, float y, float z) {
	Node *enemy = malloc(sizeof(Node));
	Node *bar = malloc(sizeof(Node));
	*enemy = initNode("Enemy1", &enemy1);
	*bar = initNode("EnemyLifeBar", NULL);
	enemy->shape = enemy1Shape;
	enemy->position[0] = x;
	enemy->position[1] = y;
	enemy->position[2] = z;
	enemy->scale[0] = 32.0F;
	enemy->scale[1] = 32.0F;
	enemy->scale[2] = 32.0F;
	enemy->collisionMaskPassive = HERO_BULLET_COLLISIONMASK;
	enemy->behaviour = enemy1Behaviour;
	addIntervalEvent(enemy, 1000, enemy1BehaviourInterval);
	bar->shape = enemyLifeShape;
	bar->position[1] = 16.0F;
	bar->isInterface = TRUE;
	push(&enemy->children, bar);
	push(&scene.objects, enemy);
	return enemy;
}

static int heroBehaviour(Node *node) {
	float move[2];
	addVec2(node->position, mulVec2ByScalar(controller.move, 1.0F, move), node->position);
	node->position[0] = max(min(node->position[0], HALF_FIELD_SIZE), -HALF_FIELD_SIZE);
	node->position[1] = max(min(node->position[1], HALF_FIELD_SIZE), -HALF_FIELD_SIZE);
	node->angle[1] = angleVec2(controller.direction) + PI / 2.0F;
	if(controller.action) {
		shootBullet("heroBullet", heroBulletShape, node->position, node->angle[1], HERO_BULLET_COLLISIONMASK);
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
	// lifeBar = genRect(5, 1, RED);
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
	lifeBarNode = initNode("lifeBarNode", NULL);
	stageNode = initNode("stage", &stage);
	initShapeFromObj(&stageNode.shape, "./assets/test.obj");
	stageNode.position[2] = 10.0F;
	lifeBarNode.shape = initShapePlaneInv(1.0F, 0.1F, RED);
	lifeBarNode.position[0] = -1.00F;
	lifeBarNode.position[1] = -1.00F;
	lifeBarNode.position[2] = 0.01F;
	heroNode = initNode("Hero", &hero);
	initShapeFromObj(&heroNode.shape, "./assets/hero.obj");
	heroNode.scale[0] = 32.0F;
	heroNode.scale[1] = 32.0F;
	heroNode.scale[2] = 32.0F;
	heroNode.collisionMaskPassive = ENEMY_BULLET_COLLISIONMASK;
	heroNode.behaviour = heroBehaviour;
	push(&scene.interfaces, &lifeBarNode);
	push(&scene.objects, &heroNode);
	push(&scene.objects, &stageNode);
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
	discardShape(heroNode.shape);
	discardShape(stageNode.shape);
	discardShape(enemy1Shape);
	discardShape(enemyLifeShape);
	discardShape(heroBulletShape);
	discardShape(enemyBulletShape);
	discardNode(lifeBarNode);
	discardNode(heroNode);
	discardNode(stageNode);
	freeImage(lifeBar);
	freeImage(hero);
	freeImage(heroBullet);
	discardScene(&scene);
	deinitGraphics();
}

int main(void) {
	initialize();


	// spawnEnemy1(0.0F, 0.0F, 500.0F);


	while(TRUE) {
		if(!pollEvents()) break;
		scene.camera.target[0] = heroNode.position[0];
		scene.camera.target[1] = heroNode.position[1];
		drawScene(&scene, screen);
	}
	deinitialize();
	return 0;
}
