#ifndef RECORD_H
#define RECORD_H

#include "../cnsglib/include/cnsg.h"

typedef struct {
	float time;
	float position[3];
	float angle[3];
	float velocity[3];
	float angVelocity[3];
	float angMomentum[3];
} CarRecord;

#pragma pack(1)
typedef struct {
	float time;
	float position[3];
	float angle[3];
	float velocity[3];
	float angVelocity[3];
	float angMomentum[3];
	char reserved[36];
} CarRecordForSave;
#pragma pack()

void addCarRecord(Vector *records, float time, float position[3], float angle[3], float velocity[3], float angVelocity[3], float angMomentum[3]);
void recordCar(Vector *records, Node *node, float time);
int playCar(Vector *records, Node *node, float time);
void saveRecord(Vector *records, const char path[]);
void loadRecord(Vector *records, const char path[]);

#endif
