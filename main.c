#include<stdio.h>
#include<math.h>
#include<time.h>
#include<Windows.h>

#include "./cnsglib/include/cnsg.h"

#define SCREEN_SIZE 128
#define FRAME_PER_SECOND 60

#define HERO_BULLET_COLLISIONMASK 0x01
#define ENEMY_BULLET_COLLISIONMASK 0x02
#define OBSTACLE_COLLISIONMASK 0x04
#define STAGE_COLLISIONMASK 0x08

static struct {
	float move[2];
	float direction[2];
	float action;
	float retry;
	float quit;
} controller;

static FontSJIS shnm12;

static Controller keyboard;
static ControllerEvent wasd[4], action, restart, quit;
static float autoMove[2];
static BOOL autoAction;

static Image lifeBarBunch;
static Image lifebarImage;
static Image scoreImage;
static Image hero;
static Image heroBullet;
static Image enemy1;
static Image stage;
static Image stoneImage;
static Image gameoverImage;
static Image startImage;
static Image explosionImage;

static Shape enemy1Shape;
static Shape enemyLifeShape;
static Shape heroBulletShape;
static Shape enemyBulletShape;
static Shape stoneShape;
static Shape explosionShape;

static Node lifeBarNode;
static Node scoreNode;
static Scene scene;
static Node heroNode;
static Node rayNode;
static Node stageNode;
static Node gameoverNode;
static Node startNode;

BOOL start = FALSE;
static unsigned int heroHP;
static unsigned int score;

typedef struct {
	int i;
	float dx;
} Explosion;

typedef struct {
	unsigned int hp;
	Node *bar;
} Enemy1;

static void explosionInterval(Node *node) {
	Explosion *explosion = node->data;
	if(explosion->i < 5) {
		node->scale[0] += explosion->dx;
		node->scale[1] += explosion->dx;
		node->scale[2] += explosion->dx;
		explosion->i += 1;
	} else {
		removeByData(&scene.nodes, node);
		free(node);
	}
}

static void causeExplosion(float position[3], float radius) {
	Node *explosionNode = malloc(sizeof(Node));
	Explosion *explosion = calloc(sizeof(Explosion), 1);
	*explosionNode = initNode("explosion", explosionImage);
	explosionNode->shape = explosionShape;
	explosionNode->velocity[2] = -100.0F;
	explosionNode->position[0] = position[0];
	explosionNode->position[1] = position[1];
	explosionNode->position[2] = position[2];
	explosionNode->scale[0] = 0.0F;
	explosionNode->scale[1] = 0.0F;
	explosionNode->scale[2] = 0.0F;
	explosionNode->data = explosion;
	explosion->dx = radius / 5.0F;
	addIntervalEventNode(explosionNode, 100, explosionInterval);
	push(&scene.nodes, explosionNode);
	PlaySound(TEXT("./assets/se_maoudamashii_retro12.wav"), NULL, SND_ASYNC | SND_FILENAME);
}

static int bulletBehaviour(Node *node) {
	if(node->collisionFlags || distance2(heroNode.position, node->position) > 500 || node->position[2] < -100.0F) {
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
	bullet->velocity[0] = 400.0F * cosf(angle - PI / 2.0F);
	bullet->velocity[2] = -400.0F * sinf(angle - PI / 2.0F);
	bullet->angle[1] = angle;
	bullet->position[0] = position[0];
	bullet->position[1] = position[1];
	bullet->position[2] = position[2];
	bullet->collisionMaskActive = collisionMask;
	bullet->behaviour = bulletBehaviour;
	push(&scene.nodes, bullet);
}

static int enemy1Behaviour(Node *node) {
	if(node->position[2] < -100.0F) {
		removeByData(&scene.nodes, node);
		free(node);
		return FALSE;
	}
	if(node->collisionFlags & HERO_BULLET_COLLISIONMASK) {
		Enemy1 *enemy = node->data;
		enemy->hp -= 1;
		cropImage(enemy->bar->texture, lifeBarBunch, 0, 10 * enemy->hp / 1);
		if(enemy->hp <= 0) {
			causeExplosion(node->position, 50.0F);
			removeByData(&scene.nodes, node);
			free(node);
			score += 10;
			return FALSE;
		}
	}
	return TRUE;
}

static void enemy1BehaviourInterval(Node *node) {
	shootBullet("enemyBullet", enemyBulletShape, node->position, PI, ENEMY_BULLET_COLLISIONMASK);
}

static void spawnEnemy1(float x, float y, float z) {
	Node *enemy = malloc(sizeof(Node));
	Node *bar = malloc(sizeof(Node));
	Enemy1 *data = malloc(sizeof(Enemy1));
	Image image = initImage(192, 32, BLACK, NULL_COLOR);
	*enemy = initNode("enemy1", enemy1);
	cropImage(image, lifeBarBunch, 0, 10);
	*bar = initNode("EnemyLifeBar", image);
	data->hp = 1;
	data->bar = bar;
	enemy->shape = enemy1Shape;
	enemy->position[0] = x;
	enemy->position[1] = y;
	enemy->position[2] = z;
	enemy->velocity[2] = -100.0F;
	enemy->scale[0] = 50.0F;
	enemy->scale[1] = 50.0F;
	enemy->scale[2] = 50.0F;
	enemy->collisionMaskActive = OBSTACLE_COLLISIONMASK;
	enemy->collisionMaskPassive = HERO_BULLET_COLLISIONMASK;
	enemy->behaviour = enemy1Behaviour;
	enemy->data = data;
	addIntervalEventNode(enemy, 3000, enemy1BehaviourInterval);
	bar->shape = enemyLifeShape;
	bar->position[1] = -16.0F;
	push(&enemy->children, bar);
	push(&scene.nodes, enemy);
}

static int stoneBehaviour(Node *node) {
	if(node->position[2] < -100.0F) {
		removeByData(&scene.nodes, node);
		free(node);
		return FALSE;
	}
	return TRUE;
}

static void spawnStone(float x, float y, float z) {
	Node *stone = malloc(sizeof(Node));
	*stone = initNode("stone", stoneImage);
	stone->shape = stoneShape;
	stone->position[0] = x;
	stone->position[1] = y;
	stone->position[2] = z;
	stone->velocity[2] = -100.0F;
	stone->scale[0] = 32.0F;
	stone->scale[1] = 32.0F;
	stone->scale[2] = 32.0F;
	stone->behaviour = stoneBehaviour;
	stone->collisionMaskActive = OBSTACLE_COLLISIONMASK;
	stone->collisionMaskPassive = HERO_BULLET_COLLISIONMASK | ENEMY_BULLET_COLLISIONMASK;
	push(&scene.nodes, stone);
}

static void gameover(void) {
	if(!start) {
		push(&scene.nodes, &gameoverNode);
	}
}

static void autoControl(void) {
	Node *node;
	autoMove[0] = 0.0F;
	autoMove[1] = 0.0F;
	resetIteration(&scene.nodes);
	node = nextData(&scene.nodes);
	while(node) {
		float temp[2];
		if(strcmp("enemy1", node->id) == 0) {
			subVec2(node->position, heroNode.position, temp);
			if(node->position[2] > 200.0F) {
				addVec2(autoMove, temp, autoMove);
				autoAction = TRUE;
				break;
			} else {
				subVec2(autoMove, node->position, autoMove);
			}
		} else if(strcmp("stone", node->id) == 0 || strcmp("enemyBullet", node->id) == 0) {
			if(node->position[2] < 200.0F) {
				subVec2(autoMove, node->position, autoMove);
			}
		}
		node = nextData(&scene.nodes);
	}
	normalize2(autoMove, autoMove);
}

static int heroBehaviour(Node *node) {
	float move[2];
	if(node->collisionFlags) {
		if(node->collisionFlags & ENEMY_BULLET_COLLISIONMASK) heroHP -= 1;
		if(node->collisionFlags & OBSTACLE_COLLISIONMASK) heroHP = 0;
		cropImage(lifebarImage, lifeBarBunch, 0, heroHP);
		if(heroHP <= 0) {
			causeExplosion(node->position, 50.0F);
			removeByData(&scene.nodes, node);
			gameover();
		}
	}
	if(start) {
		mulVec2ByScalar(autoMove, 200.0F, move);
	} else {
		mulVec2ByScalar(controller.move, 200.0F, move);
	}
	node->force[0] += move[0];
	node->force[2] += move[1];
	// node->position[0] = max(min(node->position[0], 100.0F), -100.0F);
	// node->position[1] = max(min(node->position[1], 100.0F), -100.0F);
	if(controller.action || (start && autoAction)) {
		static clock_t previousClock;
		if((clock() - previousClock) * 1000 / CLOCKS_PER_SEC > 500) {
			shootBullet("heroBullet", heroBulletShape, node->position, node->angle[1], HERO_BULLET_COLLISIONMASK);
			controller.action = FALSE;
			PlaySound(TEXT("assets/laser.wav"), NULL, SND_ASYNC | SND_FILENAME);
			previousClock = clock();
		}
	}
	return TRUE;
}

static int scoreBehaviour(Node *node) {
	char buffer[11];
	sprintf(buffer, "SCORE %04u", score);
	drawTextSJIS(scoreImage, shnm12, 0, 0, buffer);
	return TRUE;
}

static void sceneInterval() {
	// if(heroHP > 0) {
	// 	int x, y;
	// 	for(x = 0;x < 3;x++) {
	// 		for(y = 0;y < 3;y++) {
	// 			float cx = 200.0F / 3.0F * x - 100.0F;
	// 			float cy = 200.0F / 3.0F * y - 60.0F;
	// 			switch(rand() % 4) {
	// 				case 0:
	// 					spawnStone(cx, cy, 500.0F);
	// 					break;
	// 				case 1:
	// 					spawnEnemy1(cx, cy, 500.0F);
	// 					break;
	// 				case 2:
	// 				case 3:
	// 					break;
	// 			}
	// 		}
	// 	}
	// 	score += 5;
	// }
}

static void initialize(void) {
	initCNSG(SCREEN_SIZE, SCREEN_SIZE);
	shnm12 = initFontSJIS(loadBitmap("assets/shnm6x12r.bmp", NULL_COLOR), loadBitmap("assets/shnmk12.bmp", NULL_COLOR), 6, 12, 12);

	keyboard = initController();
	initControllerEventCross(wasd, 'W', 'A', 'S', 'D', controller.move);
	action = initControllerEvent(VK_SPACE, 1.0F, 0.0F, &controller.action);
	restart = initControllerEvent('R', 1.0F, 0.0F, &controller.retry);
	quit = initControllerEvent(VK_ESCAPE, 1.0F, 0.0F, &controller.quit);
	push(&keyboard.events, &wasd[0]);
	push(&keyboard.events, &wasd[1]);
	push(&keyboard.events, &wasd[2]);
	push(&keyboard.events, &wasd[3]);
	push(&keyboard.events, &action);
	push(&keyboard.events, &restart);
	push(&keyboard.events, &quit);

	lifeBarBunch = loadBitmap("assets/lifebar.bmp", NULL_COLOR);
	lifebarImage = initImage(192, 32, BLACK, NULL_COLOR);
	cropImage(lifebarImage, lifeBarBunch, 0, 10);
	hero = loadBitmap("assets/hero3d.bmp", NULL_COLOR);
	heroBullet = loadBitmap("assets/heroBullet.bmp", WHITE);
	enemy1 = loadBitmap("assets/enemy1.bmp", NULL_COLOR);
	stoneImage = loadBitmap("assets/stone.bmp", NULL_COLOR);
	stage = loadBitmap("assets/stage.bmp", NULL_COLOR);
	explosionImage = loadBitmap("./assets/explosion.bmp", NULL_COLOR);
	scene = initScene();
	scene.camera = initCamera(0.0F, 50.0F, -100.0F, 1.0F);
	// scene.camera.parent = &heroNode;
	scene.background = DARK_BLUE;
	addIntervalEventScene(&scene, 5000, sceneInterval);
	initShapeFromObj(&enemy1Shape, "./assets/enemy1.obj");
	initShapeFromObj(&stoneShape, "./assets/stone.obj");
	initShapeFromObj(&explosionShape, "./assets/explosion.obj");
	enemyLifeShape = initShapePlane(20, 5, RED);
	heroBulletShape = initShapeBox(5, 5, 30, YELLOW);
	enemyBulletShape = initShapeBox(5, 5, 30, MAGENTA);
	lifeBarNode = initNodeUI("lifeBarNode", lifebarImage, BLACK);
	stageNode = initNode("stage", stage);
	initShapeFromObj(&stageNode.shape, "./assets/test.obj");
	stageNode.position[1] = -100.0F;
	stageNode.position[2] = 0.0F;
	stageNode.scale[0] = 3.0F;
	stageNode.scale[2] = 3.0F;
	stageNode.angle[2] = -0.5F;
	stageNode.collisionMaskActive = STAGE_COLLISIONMASK;
	// stageNode.shape.mass = 100.0F;
	lifeBarNode.position[0] = 2.5F;
	lifeBarNode.position[1] = 2.5F;
	lifeBarNode.scale[0] = 30.0F;
	lifeBarNode.scale[1] = 5.0F;
	heroNode = initNode("Hero", hero);
	// heroNode.shape = initShapeBox(1.0F, 1.0F, 1.0F, RED);
	initShapeFromObj(&heroNode.shape, "./assets/hero.obj");
	// heroNode.shape.mass = 100.0F;
	heroNode.isPhysicsEnabled = TRUE;
	heroNode.scale[0] = 32.0F;
	heroNode.scale[1] = 32.0F;
	heroNode.scale[2] = 32.0F;
	// heroNode.angle[2] = -1.0F;
	heroNode.collisionMaskPassive = ENEMY_BULLET_COLLISIONMASK | OBSTACLE_COLLISIONMASK | STAGE_COLLISIONMASK;
	heroNode.behaviour = heroBehaviour;
	rayNode = initNode("ray", NO_IMAGE);
	rayNode.shape = initShapeBox(3, 3, 512, RED);
	rayNode.position[2] = 256.0F;
	push(&heroNode.children, &rayNode);

	startImage = initImage(96, 48, BLACK, BLACK);
	startNode = initNodeUI("gameover", startImage, BLACK);
	startNode.scale[0] = 9600 / SCREEN_SIZE;
	startNode.scale[1] = 4800 / SCREEN_SIZE;
	startNode.position[0] = 50 - startNode.scale[0] / 2;
	startNode.position[1] = 50 - startNode.scale[1] / 2;
	drawTextSJIS(startImage, shnm12, 0, 0, "SPACE SHOOTER\n\n\"SPACE\"でプレイ\n\"ESC\"で終了");

	scoreImage = initImage(60, 12, BLACK, BLACK);
	scoreNode = initNodeUI("score", scoreImage, NULL_COLOR);
	scoreNode.position[0] = 100 - 6000 / 128;
	scoreNode.scale[0] = 6000 / 128;
	scoreNode.scale[1] = 1200 / 128;
	scoreNode.behaviour = scoreBehaviour;

	gameoverImage = initImage(84, 48, BLACK, BLACK);
	gameoverNode = initNodeUI("gameover", gameoverImage, BLACK);
	gameoverNode.scale[0] = 8400 / SCREEN_SIZE;
	gameoverNode.scale[1] = 4800 / SCREEN_SIZE;
	gameoverNode.position[0] = 50 - gameoverNode.scale[0] / 2;
	gameoverNode.position[1] = 50 - gameoverNode.scale[1] / 2;
	drawTextSJIS(gameoverImage, shnm12, 0, 0, "ゲームオーバー\n\n\"R\"でリプレイ\n\"ESC\"で終了");
}

static void startGame(void) {
	heroHP = 10;
	cropImage(lifebarImage, lifeBarBunch, 0, 10);
	heroNode.position[0] = 0.0F;
	heroNode.position[1] = 0.0F;
	heroNode.position[2] = 0.0F;
	clearVec3(heroNode.velocity);

	srand(0);

	clearVector(&scene.nodes);
	if(start) push(&scene.nodes, &startNode);
	push(&scene.nodes, &lifeBarNode);
	push(&scene.nodes, &scoreNode);
	push(&scene.nodes, &heroNode);
	push(&scene.nodes, &stageNode);
	sceneInterval();
	score = 0;
}

static void deinitialize(void) {
	discardShape(heroNode.shape);
	discardShape(rayNode.shape);
	discardShape(stageNode.shape);
	discardShape(enemy1Shape);
	discardShape(enemyLifeShape);
	discardShape(heroBulletShape);
	discardShape(enemyBulletShape);
	discardShape(stoneShape);
	discardShape(explosionShape);
	discardNode(lifeBarNode);
	discardNode(scoreNode);
	discardNode(heroNode);
	discardNode(stageNode);
	discardNode(startNode);
	freeImage(shnm12.font0201);
	freeImage(shnm12.font0208);
	freeImage(lifeBarBunch);
	freeImage(lifebarImage);
	freeImage(scoreImage);
	freeImage(hero);
	freeImage(heroBullet);
	freeImage(stoneImage);
	freeImage(startImage);
	freeImage(gameoverImage);
	freeImage(explosionImage);
	discardScene(&scene);
	deinitGraphics();
}

float elapsedTime(LARGE_INTEGER start, LARGE_INTEGER frequency) {
	LARGE_INTEGER current;
	LARGE_INTEGER elapsed;
	QueryPerformanceCounter(&current);
	elapsed.QuadPart = current.QuadPart - start.QuadPart;
	return (float)elapsed.QuadPart / frequency.QuadPart;
}

int main(void) {
	LARGE_INTEGER previousClock;
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	initialize();
	startGame();
	QueryPerformanceCounter(&previousClock);
	while(TRUE) {
		LARGE_INTEGER previousSceneClock;
		updateController(keyboard);
		drawScene(&scene);
		updateScene(&scene, 1.0F / FRAME_PER_SECOND);
		QueryPerformanceCounter(&previousSceneClock);
		while(elapsedTime(previousClock, frequency) < 1.0F / FRAME_PER_SECOND) {
			if(elapsedTime(previousSceneClock, frequency) < 0.001F) continue;
			// updateScene(&scene, elapsedTime(previousSceneClock, frequency));
			QueryPerformanceCounter(&previousSceneClock);
		};
		QueryPerformanceCounter(&previousClock);
		if(controller.quit) break;
		if(start) {
			if(controller.action) {
				start = FALSE;
				startGame();
			} else {
				autoControl();
			}
		}
		if(controller.retry) startGame();
	}
	deinitialize();
	return 0;
}
