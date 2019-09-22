#ifndef NODE_H
#define NODE_H

#include<time.h>

#include "./graphics.h"
#include "./vector.h"

typedef struct {
	Vector indices;
	Vector vertices;
	Vector uv;
	Vector uvIndices;
} Shape;

typedef struct _Node {
	char id[10];
	float velocity[3];
	float angVelocity;
	float position[3];
	float angle[3];
	float scale[3];
	float aabb[3][2];
	Image texture;
	Shape shape;
	unsigned int collisionFlags;
	unsigned int collisionMaskActive;
	unsigned int collisionMaskPassive;
	Vector collisionTargets;
	Vector intervalEvents;
	struct _Node *parent;
	Vector children;
	int (*behaviour)(struct _Node*);
	BOOL isInterface;
	void *data;
}	Node;

typedef struct {
	clock_t begin;
	unsigned int interval;
	void (*callback)(Node*);
} IntervalEventNode;

Node initNode(const char *id, Image image);
Node initNodeUI(const char *id, Image image, unsigned char color);
void discardNode(Node node);

void drawNode(Node *node);
int testCollision(Node a, Node b);
void addIntervalEventNode(Node *node, unsigned int milliseconds, void (*callback)(Node*));

Shape initShapePlane(float width, float height, unsigned char color);
Shape initShapePlaneInv(float width, float height, unsigned char color);
Shape initShapeBox(float width, float height, float depth, unsigned char color);
int initShapeFromObj(Shape *shape, char *filename);
void discardShape(Shape shape);

#endif
