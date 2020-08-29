#include<stdio.h>

#include "./cnsglib/include/cnsg.h"

#include "./include/record.h"

void addCarRecord(Vector *records, float time, float position[3], float angle[3], float velocity[3], float angVelocity[3], float angMomentum[3]) {
	CarRecord record;
	record.time = time;
	memcpy_s(record.position, SIZE_VEC3, position, SIZE_VEC3);
	memcpy_s(record.angle, SIZE_VEC3, angle, SIZE_VEC3);
	memcpy_s(record.velocity, SIZE_VEC3, velocity, SIZE_VEC3);
	memcpy_s(record.angVelocity, SIZE_VEC3, angVelocity, SIZE_VEC3);
	memcpy_s(record.angMomentum, SIZE_VEC3, angMomentum, SIZE_VEC3);
	pushAlloc(records, sizeof(CarRecord), &record);
}

void recordCar(Vector *records, Node *node, float time) {
	if(records->length < 36000) addCarRecord(records, time, node->position, node->angle, node->velocity, node->angVelocity, node->angMomentum);
}

int playCar(Vector *records, Node *node, float time) {
	CarRecord *record;
	record = nextData(records);
	if(record == NULL) return -1;
	if(record->time > time) {
		previousData(records);
	} else {
		memcpy_s(node->position, SIZE_VEC3, record->position, SIZE_VEC3);
		memcpy_s(node->angle, SIZE_VEC3, record->angle, SIZE_VEC3);
		memcpy_s(node->velocity, SIZE_VEC3, record->velocity, SIZE_VEC3);
		memcpy_s(node->angVelocity, SIZE_VEC3, record->angVelocity, SIZE_VEC3);
		memcpy_s(node->angMomentum, SIZE_VEC3, record->angMomentum, SIZE_VEC3);
		while(record->time > time) record = nextData(records);
	}
	return 0;
}

void saveRecord(Vector *records, const char path[]) {
	FILE *file;
	CarRecord *record;
	if(fopen_s(&file, path, "wb")) {
		fprintf(stderr, "saveRecord: Failed to open a file: %s\n", path);
	} else {
		iterf(records, &record) {
			CarRecordForSave data;
			data.time = record->time;
			memcpy_s(data.position, SIZE_VEC3, record->position, SIZE_VEC3);
			memcpy_s(data.angle, SIZE_VEC3, record->angle, SIZE_VEC3);
			memcpy_s(data.velocity, SIZE_VEC3, record->velocity, SIZE_VEC3);
			memcpy_s(data.angVelocity, SIZE_VEC3, record->angVelocity, SIZE_VEC3);
			memcpy_s(data.angMomentum, SIZE_VEC3, record->angMomentum, SIZE_VEC3);
			fwrite(&data, sizeof(CarRecordForSave), 1, file);
		}
		fclose(file);
	}
}

void loadRecord(Vector *records, const char path[]) {
	FILE *file;
	*records = initVector();
	if(fopen_s(&file, path, "rb")) {
		fprintf(stderr, "loadRecord: Failed to open a file: %s\n", path);
	} else {
		CarRecordForSave data;
		size_t count;
		for(count = fread_s(&data, sizeof(CarRecordForSave), sizeof(CarRecordForSave), 1, file);count == 1;count = fread_s(&data, sizeof(CarRecordForSave), sizeof(CarRecordForSave), 1, file)) {
			addCarRecord(records, data.time, data.position, data.angle, data.velocity, data.angVelocity, data.angMomentum);
		}
		fclose(file);
	}
}
