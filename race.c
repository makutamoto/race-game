#include<stdio.h>

#include "./cnsglib/include/cnsg.h"

#include "./include/common.h"
#include "./include/race.h"
#include "./include/menu.h"
#include "./include/record.h"

static Vector heroRecords, opponentRecords, opponent2pRecords;

static Image opponentImage;
static Image course;
static Image stageImage;

static Window race1pWindow, race2pWindow;
static View raceView, race1pView, race2pView;
static ControllerData *resetCamera;
static ControllerDataCross *move, *arrow;
static Scene raceScene;
static Camera camera2P;
static Node speedNode, lapNode, rankNode, finishNode, centerNode, timeNode, replayNode, wrongWayNode;
static Node time2pNode, speed2pNode, lap2pNode, rank2pNode, finish2pNode, replay2pNode;
static Node mapNode, map2pNode;
static Node lapJudgmentNodes[3];
static Node raceCarNode, heroDriftNode;
static Node opponentNode;
static Node heroMarkerNode, opponentMarkerNode;
static Node courseMapNode, courseDirtNode;
static Scene mapScene;

static float nextCameraPosition[3], opponentNextCameraPosition[3];
static float currentCameraPosition[3];
static float cameraAngle, cameraFov;
static int heroLapScore, opponentLapScore;
static int heroPreviousLap = -1, opponentPreviousLap = -1;
static int isRaceStarted, isFinished, isHeroFinished, isOpponentFinished, isReplaying;
static int isWrongWay, isWrongWay2p;
static float finishedTime;
static float currentTime;
static float heroAccel, opponentAccel;
static char rankList[][4] = { "1st", "2nd" };

static int is2p;

static void escapeEvent(void) {
	startMenu();
}

static void F1Event(void) {
  int i;
  for(i = 0;i < 3;i++) lapJudgmentNodes[i].isVisible = !lapJudgmentNodes[i].isVisible;
  courseDirtNode.isVisible = !courseDirtNode.isVisible;
}

static void replay(void) {
	isWrongWay = FALSE;
	if(is2p) {
		isWrongWay2p = FALSE;
	} else {
		opponentNode.isVisible = TRUE;
		opponentMarkerNode.isVisible = TRUE;
		resetIteration(&opponentRecords);
	}
	raceScene.clock = 0.0F;
}

static float controlCar(Node *node, float accel, float handle, float front[3]) {
	float tempVec3[3][3];
	float tempMat3[1][3][3];
	float tempMat4[1][4][4];
	float velocityLengthH;
	float angle;
	if(node->collisionFlags & DIRT_COLLISIONMASK) {
		node->collisionShape.dynamicFriction = 1.0F;
	} else {
		node->collisionShape.dynamicFriction = 0.1F;
	}
	convMat4toMat3(genRotationMat4(node->angle[0], node->angle[1], node->angle[2], tempMat4[0]), tempMat3[0]);
	initVec3(tempVec3[0], Y_MASK);
	mulMat3Vec3(tempMat3[0], tempVec3[0], tempVec3[1]);
	initVec3(tempVec3[0], Y_MASK);
	angle =	cosVec3(tempVec3[0], tempVec3[1]);
	if(angle < 0.75F) {
		accel = 0.0F;
		handle = 0.0F;
	}
	initVec3(tempVec3[0], Z_MASK);
	mulMat3Vec3(tempMat3[0], tempVec3[0], front);
	normalize3(extractComponents3(front, XZ_MASK, tempVec3[0]), tempVec3[1]);
	extractComponents3(node->velocity, XZ_MASK, tempVec3[0]);
	velocityLengthH = length3(tempVec3[0]);
	mulVec3ByScalar(tempVec3[1], sign(accel) * velocityLengthH * node->collisionShape.mass + accel * (sign(accel) > 0 ? 20000.0F : 5000.0F), tempVec3[0]);
	subVec3(tempVec3[0], mulVec3ByScalar(node->velocity, node->collisionShape.mass, tempVec3[2]), tempVec3[1]);
	applyImpulseForce(node, tempVec3[1], XZ_MASK, FALSE);
	if(velocityLengthH > 1.0F) node->torque[1] += handle * max(velocityLengthH * 500.0F, 50000.0F);
	return angle;
}

static float convVelocityToSpeed(float velocity[3]) {
	return length3(velocity) * 3600 / 10000;
}

static int updateLapScore(unsigned int flags, int *previousLap, int *lapScore, int wrongWay) {
	if(flags & LAPA_COLLISIONMASK) {
		if(*previousLap == 2) {
			*lapScore += 1;
			wrongWay = FALSE;
		} else if(*previousLap == 1) {
			*lapScore -= 1;
			wrongWay = TRUE;
		}
		*previousLap = 0;
	}
	if(flags & LAPB_COLLISIONMASK) {
		if(*previousLap == 0) {
			*lapScore += 1;
			wrongWay = FALSE;
		} else if(*previousLap == 2) {
			*lapScore -= 1;
			wrongWay = TRUE;
		}
		*previousLap = 1;
	}
	if(flags & LAPC_COLLISIONMASK) {
		if(*previousLap == 1) {
			*lapScore += 1;
			wrongWay = FALSE;
		} else if(*previousLap == 0) {
			*lapScore -= 1;
			wrongWay = TRUE;
		}
		*previousLap = 2;
	}
	return wrongWay;
}

static void spawnItem(Scene *scene, float x, float y, float z) {
	Node item;
	item = initNode("item", NO_IMAGE);
	item.shape = initShapeBox(10.0F, 10.0F, 10.0F, BLACK);
	item.collisionShape = item.shape;
	item.position[0] = x;
	item.position[1] = y;
	item.position[2] = z;
	pushAlloc(&scene->nodes, sizeof(Node), &item);
}

static int heroBehaviour(Node *node, float elapsed) {
	float front[3];
	if(isRaceStarted) {
		if(isReplaying) {
			if(playCar(&heroRecords, node, currentTime)) replay();
		} else {
			recordCar(&heroRecords, node, currentTime);
			if(node->collisionFlags & COURSE_COLLISIONMASK) {
				float upper[3] = { 0.0F, 25.0F, 0.0F };
				if(heroLapScore >= 3 * 45) {
					isHeroFinished = TRUE;
					finishNode.isVisible = TRUE;
					controlCar(node, 0, 0, front);
				} else {
					int isDrift;
					if(controlCar(node, heroAccel, move->states[0], front) < 0.0F) {
						node->angle[0] = 0.0F;
						node->angle[2] = 0.0F;
						node->angVelocity[0] = 0.0F;
						node->angVelocity[2] = 0.0F;
						node->angMomentum[0] = 0.0F;
						node->angMomentum[2] = 0.0F;
					}
					isDrift = cosVec3(front, node->velocity) < 0.95F;
					if(heroDriftNode.isVisible && !isDrift) {
						// float acceleration[3];
						// applyForce(&node, );
					}
					// heroDriftNode.isVisible = isDrift;
				}
				mulVec3ByScalar(front, -75.0F, nextCameraPosition);
				addVec3(nextCameraPosition, upper, nextCameraPosition);
			}
		}
	}
	isWrongWay = updateLapScore(node->collisionFlags, &heroPreviousLap, &heroLapScore, isWrongWay);
	memcpy_s(heroMarkerNode.position, SIZE_VEC3, node->position, SIZE_VEC3);
	memcpy_s(heroMarkerNode.angle, SIZE_VEC3, node->angle, SIZE_VEC3);
	return TRUE;
}

static int opponentBehaviour(Node *node, float elapsed) {
	if(isRaceStarted) {
		if(is2p) {
			if(isReplaying) {
				playCar(&opponent2pRecords, node, currentTime);
			} else {
				recordCar(&opponent2pRecords, node, currentTime);
				if(node->collisionFlags & COURSE_COLLISIONMASK) {
					float upper[3] = { 0.0F, 25.0F, 0.0F };
					float front[3];
					if(opponentLapScore >= 3 * 45) {
						isOpponentFinished = TRUE;
						finish2pNode.isVisible = TRUE;
						controlCar(node, 0.0F, 0.0F, front);
					} else {
						if(controlCar(node, opponentAccel, arrow->states[0], front) < 0.0F) {
							node->angle[0] = 0.0F;
							node->angle[2] = 0.0F;
							node->angVelocity[0] = 0.0F;
							node->angVelocity[2] = 0.0F;
							node->angMomentum[0] = 0.0F;
							node->angMomentum[2] = 0.0F;
						}
					}
					mulVec3ByScalar(front, -75.0F, opponentNextCameraPosition);
					addVec3(opponentNextCameraPosition, upper, opponentNextCameraPosition);
				}
			}
		} else {
			if(playCar(&opponentRecords, node, currentTime)) {
				node->isVisible = FALSE;
				opponentMarkerNode.isVisible = FALSE;
			}
		}
	}
	isWrongWay2p = updateLapScore(node->collisionFlags, &opponentPreviousLap, &opponentLapScore, isWrongWay2p);
	memcpy_s(opponentMarkerNode.position, SIZE_VEC3, node->position, SIZE_VEC3);
	memcpy_s(opponentMarkerNode.angle, SIZE_VEC3, node->angle, SIZE_VEC3);
	return TRUE;
}

static int timeBehaviour(Node *node, float elapsed) {
	if((!is2p && isHeroFinished) || (isHeroFinished && isOpponentFinished)) {
		currentTime = raceScene.clock;
	} else {
		if(isRaceStarted) {
			char buffer[14];
			int minutes;
			float seconds;
			currentTime = raceScene.clock - 3.5F;
			minutes = currentTime / 60;
			seconds = currentTime - 60.0F * minutes;
			if(minutes > 9) {
				minutes = 9;
				seconds = 59.999F;
			}
			sprintf(buffer, "TIME %d`%06.3f", minutes, seconds);
			drawTextSJIS(&node->texture, &shnm12, 0, 0, buffer);
		} else {
			drawTextSJIS(&node->texture, &shnm12, 0, 0, "TIME 0`00.000");
		}
	}
	return TRUE;
}

static void speedBehaviourCommon(Node *node, Node *car) {
	char buffer[11];
	float speed = convVelocityToSpeed(car->velocity);
	if(speed >= 1000.0F) {
		sprintf(buffer, "???.? km/h");
	} else {
		sprintf(buffer, "%5.1f km/h", speed);
	}
	drawTextSJIS(&node->texture, &shnm12, 0, 0, buffer);
}

static int speedBehaviour(Node *node, float elapsed) {
	speedBehaviourCommon(node, &raceCarNode);
	return TRUE;
}

static int speed2pBehaviour(Node *node, float elapsed) {
	speedBehaviourCommon(node, &opponentNode);
	return TRUE;
}

static void lapBehaviourCommon(Node *node, int score) {
	char buffer[8];
	int lap = min(max(score / 45 + 1, 1), 3);
	sprintf(buffer, "LAP %d/3", lap);
	drawTextSJIS(&node->texture, &shnm12, 0, 0, buffer);
}

static int lapBehaviour(Node *node, float elapsed) {
	if(!isHeroFinished) lapBehaviourCommon(node, heroLapScore);
	return TRUE;
}

static int lap2pBehaviour(Node *node, float elapsed) {
	if(!isOpponentFinished) lapBehaviourCommon(node, opponentLapScore);
	return TRUE;
}

static void rankBehaviourCommon(Node *node, int *rank, int scoreA, int scoreB) {
	if(!(isHeroFinished || isOpponentFinished)) {
		if(scoreA != scoreB) {
			*rank = scoreA > scoreB ? 0 : 1;
		}
		drawTextSJIS(&node->texture, &shnm12, 0, 0, rankList[*rank]);
	}
}

static int rankBehaviour(Node *node, float elapsed) {
	static int rank = 0;
	rankBehaviourCommon(node, &rank, heroLapScore, opponentLapScore);
	return TRUE;
}

static int rank2pBehaviour(Node *node, float elapsed) {
	static int rank = 1;
	rankBehaviourCommon(node, &rank, opponentLapScore, heroLapScore);
	return TRUE;
}

static void raceStarted(void) {
	PlaySoundNeo("./assets/bgm.wav", TRUE);
}

static int centerBehaviour(Node *node, float _elapsed) {
	float elapsed = 4.5F - raceScene.clock;
	if(isRaceStarted) {
		if(elapsed > 0.0F) {
			drawTextSJIS(&node->texture, &shnm16b, 0, 0, "START! ");
		} else {
			node->isVisible = FALSE;
		}
	} else {
		char buffer[8];
		if(elapsed > 1.0F) {
			sprintf(buffer, "%4d   ", min((int)elapsed, 3));
			drawTextSJIS(&node->texture, &shnm16b, 0, 0, buffer);
			node->isVisible = TRUE;
		} else {
			raceStarted();
			isRaceStarted = TRUE;
		}
	}
	return TRUE;
}

static int mapBehaviour(Node *node, float elapsed) {
	drawScene(&mapScene, &node->texture);
	return TRUE;
}

static void raceSceneBehaviour(Scene *scene, float elapsed) {
	float tempVec3[2][3];
	float tempMat4[1][4][4];
	float tempMat3[1][3][3];
	if(isFinished) {
		if(!isReplaying) {
			if(raceScene.clock - finishedTime > 3.0F) {
				replay();
				finishNode.isVisible = FALSE;
				finish2pNode.isVisible = FALSE;
				if(is2p) replay2pNode.isVisible = TRUE;
				else replayNode.isVisible = TRUE;
				isReplaying = TRUE;
			}
		}
	} else {
		if((!is2p && isHeroFinished) || (isHeroFinished && isOpponentFinished)) {
			isFinished = TRUE;
			finishedTime = raceScene.clock;
		}
	}
	if(!isReplaying) {
		heroAccel += 0.3F * (move->states[1] - heroAccel);
		opponentAccel += 0.3F * (arrow->states[1] - opponentAccel);
	}
	if(is2p) {
		cameraFov = PI / 3.0F * 1.5F;
		cameraAngle = 0.0F;
	} else {
		cameraFov = min(PI / 3.0F * 1.5F, max(PI / 3.0F, cameraFov - 0.5F * arrow->states[1] * elapsed));
		cameraAngle -= arrow->states[0] * elapsed;
	}
	raceScene.camera.fov = cameraFov + heroAccel / 5.0F;
	camera2P.fov = cameraFov + opponentAccel / 5.0F;
	addVec3(currentCameraPosition, mulVec3ByScalar(subVec3(nextCameraPosition, currentCameraPosition, tempVec3[0]), 2.0F * elapsed, tempVec3[1]), currentCameraPosition);
	addVec3(camera2P.position, mulVec3ByScalar(subVec3(opponentNextCameraPosition, camera2P.position, tempVec3[0]), 2.0F * elapsed, tempVec3[1]), camera2P.position);
	if(resetCamera->state) cameraAngle -= 10.0F * cameraAngle * elapsed;
	genRotationMat4(0.0F, cameraAngle, 0.0F, tempMat4[0]);
	mulMat3Vec3(convMat4toMat3(tempMat4[0], tempMat3[0]), currentCameraPosition, raceScene.camera.position);
}

void initRace(void) {
  int i;
  char lapNames[3][50] = { "./assets/courseMk2CollisionA.obj", "./assets/courseMk2CollisionB.obj", "./assets/courseMk2CollisionC.obj" };

  opponentImage = loadBitmap("assets/subaru2p.bmp", NULL_COLOR);
	course = loadBitmap("assets/courseMk2.bmp", NULL_COLOR);
  carImage = loadBitmap("assets/subaru.bmp", NULL_COLOR);

	carShape = loadObj("./assets/subaru.obj");
	carCollisionShape = loadObj("./assets/subaruCollision.obj");

	race1pWindow = initWindow();
	raceView = initView(&raceScene);
	push(&race1pWindow.views, &raceView);

	race2pWindow = initWindow();
	race1pView = initView(&raceScene);
	race1pView.width = 0.5F;
	race2pView = initView(NULL);
	setViewSceneEx(&race2pView, &raceScene, &camera2P, NULL, 0.0F);
	race2pView.isSceneUpdateDisabled = TRUE;
	race2pView.x = 0.5F;
	race2pView.width = 0.5F;
	pushUntilNull(&race2pWindow.views, &race1pView, &race2pView, NULL);

	raceScene = initScene();
	raceScene.camera.parent = &raceCarNode;
	raceScene.camera.positionMask[1] = TRUE;
	raceScene.camera.farLimit = 3000.0F;
	raceScene.background = BLUE;
	raceScene.camera.isRotationDisabled = TRUE;
	raceScene.behaviour = raceSceneBehaviour;

  move = initControllerDataCross(&raceScene.camera.controllerList, 'W', 'A', 'S', 'D');
	arrow = initControllerDataCross(&raceScene.camera.controllerList, VK_UP, VK_LEFT, VK_DOWN, VK_RIGHT);
	resetCamera = initControllerData(&raceScene.camera.controllerList, VK_SPACE);
	initControllerEvent(&raceScene.camera.controllerList, VK_ESCAPE, NULL, escapeEvent);
  initControllerEvent(&raceScene.camera.controllerList, VK_F1, NULL, F1Event);

	camera2P = raceScene.camera;
	camera2P.parent = &opponentNode;
	pushUntilNull(&raceScene.camera.nodes, &speedNode, &lapNode, &rankNode, &wrongWayNode, &timeNode, &finishNode, &mapNode, &replayNode, NULL);
	pushUntilNull(&camera2P.nodes, &speed2pNode, &lap2pNode, &rank2pNode, &time2pNode, &map2pNode, &finish2pNode, &replay2pNode, NULL);

  courseNode = initNode("course", course);
	courseNode.shape = loadObj("./assets/courseMk2.obj");
	courseNode.collisionShape = loadObj("./assets/courseMk2Collision.obj");
	setVec3(courseNode.scale, 4.0F, XYZ_MASK);
	courseNode.collisionShape.dynamicFriction = 0.8F;
	courseNode.collisionShape.rollingFriction = 0.8F;
	courseNode.collisionMaskActive = COURSE_COLLISIONMASK;

	courseDirtNode = initNode("courseDirt", NO_IMAGE);
	courseDirtNode.shape = loadObj("./assets/courseMk2DirtCollision.obj");
  courseDirtNode.collisionShape = courseDirtNode.shape;
  setNodeMass(&courseDirtNode, 100.0F);
	setVec3(courseDirtNode.scale, 4.0F, XYZ_MASK);
	courseDirtNode.collisionMaskActive = DIRT_COLLISIONMASK;
	courseDirtNode.isVisible = FALSE;
	courseDirtNode.isThrough = TRUE;

	stageImage = loadBitmap("assets/stage.bmp", NULL_COLOR);
	stageNode = initNode("stage", stageImage);
	stageNode.shape = loadObj("./assets/stage.obj");
	stageNode.collisionShape = stageNode.shape;
	setVec3(stageNode.scale, 4.0F, XYZ_MASK);

  raceCarNode = initNode("hero", carImage);
	raceCarNode.shape = carShape;
	raceCarNode.collisionShape = carCollisionShape;
	setNodeMass(&raceCarNode, 2000.0F);
	raceCarNode.physicsMode = PHYSICS_3D;
  raceCarNode.isGravityEnabled = TRUE;
	setVec3(raceCarNode.scale, 4.0F, XYZ_MASK);
	raceCarNode.collisionMaskActive = CAR_COLLISIONMASK;
	raceCarNode.collisionMaskPassive = CAR_COLLISIONMASK | COURSE_COLLISIONMASK | LAP_COLLISIONMASK | DIRT_COLLISIONMASK;
	raceCarNode.behaviour = heroBehaviour;
	heroDriftNode = initNode("heroDrift", NO_IMAGE);
	heroDriftNode.shape = initShapeBox(50.0F, 50.0F, 50.0F, YELLOW);
	heroDriftNode.isVisible = FALSE;
	push(&raceCarNode.children, &heroDriftNode);

	opponentNode = initNode("opponent", opponentImage);
	opponentNode.shape = carShape;
	opponentNode.collisionShape = carCollisionShape;
	opponentNode.isGravityEnabled = TRUE;
	opponentNode.physicsMode = PHYSICS_3D;
	setNodeMass(&opponentNode, 2000.0F);
	setVec3(opponentNode.scale, 4.0F, XYZ_MASK);
	opponentNode.behaviour = opponentBehaviour;

	for(i = 0;i < 3;i++) {
		lapJudgmentNodes[i] = initNode("lapNode", NO_IMAGE);
		lapJudgmentNodes[i].shape = loadObj(lapNames[i]);
		lapJudgmentNodes[i].collisionShape = lapJudgmentNodes[i].shape;
		setNodeMass(&lapJudgmentNodes[i], 100.0F);
		setVec3(lapJudgmentNodes[i].scale, 4.0F, XYZ_MASK);
		lapJudgmentNodes[i].collisionMaskActive = LAPA_COLLISIONMASK << i;
		lapJudgmentNodes[i].isVisible = FALSE;
		lapJudgmentNodes[i].isThrough = TRUE;
	}

	timeNode = initNodeText("time", 0.0F, 0.0F, RIGHT, TOP, 78, 12, timeBehaviour);
	time2pNode = initNodeText("time2p", -39.0F, 0.0F, LEFT, TOP, 78, 12, NULL);
	freeImage(&time2pNode.texture);
	time2pNode.texture = timeNode.texture;
	speedNode = initNodeText("speed", 0.0F, 12.0F, RIGHT, TOP, 60, 12, speedBehaviour);
	speed2pNode = initNodeText("speed2p", 0.0F, 12.0F, RIGHT, TOP, 60, 12, speed2pBehaviour);
	lapNode = initNodeText("lap", 0.0F, 0.0F, LEFT, TOP, 42, 12, lapBehaviour);
	lap2pNode = initNodeText("lap2p", 0.0F, 0.0F, LEFT, TOP, 42, 12, lap2pBehaviour);
	lap2pNode.interfaceAlign[0] = RIGHT;
	rankNode = initNodeText("rank", 0.0F, 12.0F, LEFT, TOP, 36, 12, rankBehaviour);
	rank2pNode = initNodeText("rank2p", 0.0F, 12.0F, LEFT, TOP, 36, 12, rank2pBehaviour);
	finishNode = initNodeText("finished", 0.0F, 0.0F, CENTER, CENTER, 56, 16, NULL);
	finishNode.isVisible = FALSE;
	drawTextSJIS(&finishNode.texture, &shnm16b, 0, 0, "FINISH!");
	finish2pNode = finishNode;
	centerNode = initNodeText("center", 0.0F, 0.0F, CENTER, CENTER, 56, 16, centerBehaviour);
	centerNode.isVisible = FALSE;
	wrongWayNode = initNodeText("wrongWay", 0.0F, 0.0F, CENTER, CENTER, 40, 16, NULL);
	wrongWayNode.isVisible = FALSE;
	drawTextSJIS(&wrongWayNode.texture, &shnm16b, 0, 0, "‹t‘–!");
	replayNode = initNodeText("replay", 0.0F, 0.0F, RIGHT, BOTTOM, 48, 16, NULL);
	drawTextSJIS(&replayNode.texture, &shnm16b, 0, 0, "REPLAY");
	replay2pNode = replayNode;

	mapNode = initNodeText("map", 0.0F, 24.0F, RIGHT, TOP, 32, 32, mapBehaviour);
	mapNode.texture.transparent = BLACK;
	map2pNode = initNodeText("map2p", -16.0F, 24.0F, LEFT, TOP, 32, 32, NULL);
	freeImage(&map2pNode.texture);
	map2pNode.texture = mapNode.texture;

	mapScene = initScene();
	mapScene.camera = initCamera(0.0F, 1000.0F, 0.0F);
	clearVec3(mapScene.camera.worldUp);
	mapScene.camera.worldUp[0] = -1.0F;
	mapScene.camera.farLimit = 1000.0F;

	courseMapNode = courseNode;
	courseMapNode.texture = initImage(1, 1, WHITE, NULL_COLOR);
	push(&mapScene.nodes, &courseMapNode);

	heroMarkerNode = initNode("heroMarker", NO_IMAGE);
	heroMarkerNode.shape = initShapeBox(250.0F, 250.0F, 250.0F, DARK_BLUE);
	push(&mapScene.nodes, &heroMarkerNode);

	opponentMarkerNode = initNode("opponentMarker", NO_IMAGE);
	opponentMarkerNode.shape = initShapeBox(250.0F, 250.0F, 250.0F, DARK_RED);
	push(&mapScene.nodes, &opponentMarkerNode);

	loadRecord(&opponentRecords, "./carRecords/record.crd");
}

void restartRace(void) {
	startRace(is2p);
}

void startRace(int multi) {
	StopAllSound();
  is2p = multi;

	cameraAngle = 0.0F;
	cameraFov = PI / 3.0F * 1.5F;
	isFinished = FALSE;
	isHeroFinished = FALSE;
	isOpponentFinished = FALSE;
	isRaceStarted = FALSE;
	isReplaying = FALSE;
	isWrongWay = FALSE;
	isWrongWay2p = FALSE;
	heroLapScore = 0;
	heroPreviousLap = -1;
	opponentLapScore = 0;
	opponentPreviousLap = -1;
	replayNode.isVisible = FALSE;
	replay2pNode.isVisible = FALSE;
	finishNode.isVisible = FALSE;
	finish2pNode.isVisible = FALSE;

	freeVector(&heroRecords);

	currentCameraPosition[0] = 0.0F;
	currentCameraPosition[1] = 25.0F;
	currentCameraPosition[2] = -75.0F;
	memcpy_s(nextCameraPosition, SIZE_VEC3, currentCameraPosition, SIZE_VEC3);
	if(is2p) {
		freeVector(&opponent2pRecords);
		timeNode.position[0] = -timeNode.scale[0] / 2.0F;
		mapNode.position[0] = -mapNode.scale[0] / 2.0F;
		mapNode.interfaceAlign[0] = RIGHT;
		opponentNode.collisionMaskActive = CAR_COLLISIONMASK;
		opponentNode.collisionMaskPassive = DIRT_COLLISIONMASK | CAR_COLLISIONMASK | COURSE_COLLISIONMASK | LAP_COLLISIONMASK;
		memcpy_s(camera2P.position, SIZE_VEC3, currentCameraPosition, SIZE_VEC3);
		memcpy_s(opponentNextCameraPosition, SIZE_VEC3, camera2P.position, SIZE_VEC3);
	} else {
		resetIteration(&opponentRecords);
		timeNode.position[0] = 0.0F;
		timeNode.interfaceAlign[0] = RIGHT;
		mapNode.position[0] = 0.0F;
		mapNode.interfaceAlign[0] = LEFT;
		opponentNode.collisionMaskActive = 0;
		opponentNode.collisionMaskPassive = DIRT_COLLISIONMASK | COURSE_COLLISIONMASK | LAP_COLLISIONMASK;
	}

	raceCarNode.position[0] = 650.0F;
	raceCarNode.position[1] = 60.0F;
	raceCarNode.position[2] = 0.0F;
	clearVec3(raceCarNode.velocity);
	clearVec3(raceCarNode.angle);
	clearVec3(raceCarNode.angVelocity);
	clearVec3(raceCarNode.angMomentum);

	opponentNode.position[0] = 600.0F;
	opponentNode.position[1] = 60.0F;
	opponentNode.position[2] = 0.0F;
	opponentNode.isVisible = TRUE;
	opponentMarkerNode.isVisible = TRUE;
	clearVec3(opponentNode.velocity);
	clearVec3(opponentNode.angle);
	clearVec3(opponentNode.angVelocity);
	clearVec3(opponentNode.angMomentum);

	spawnItem(&raceScene, 650.0F, 75.0F, 0.0F);

	clearVector(&raceScene.nodes);
	pushUntilNull(&raceScene.nodes, &centerNode, NULL);
	pushUntilNull(&raceScene.nodes, &lapJudgmentNodes[0], &lapJudgmentNodes[1], &lapJudgmentNodes[2], NULL);
	pushUntilNull(&raceScene.nodes, &raceCarNode, &opponentNode, NULL);
	pushUntilNull(&raceScene.nodes, &courseNode, &courseDirtNode, NULL);
	push(&raceScene.nodes, &stageNode);

	raceScene.clock = 0.0F;
	setWindow(rootManager, multi ? &race2pWindow : &race1pWindow, revoluteTransition, 1.0F);
}
