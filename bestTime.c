#include<stdio.h>

#include "./cnsglib/include/cnsg.h"

#include "./include/bestTime.h"

void saveBestTime(const float *time, const char path[]) {
	FILE *file;
	if(fopen_s(&file, path, "wb")) {
		fprintf(stderr, "saveBestTime: Failed to open a file: %s\n", path);
	} else {
		fwrite(time, sizeof(float), 1, file);
		fclose(file);
	}
}

void loadBestTime(float *time, const char path[]) {
	FILE *file;
	if(fopen_s(&file, path, "rb")) {
		fprintf(stderr, "loadBestTime: Failed to open a file: %s\n", path);
		*time = 599.999F;
	} else {
		fread_s(time, sizeof(float), sizeof(float), 1, file);
		fclose(file);
	}
}
