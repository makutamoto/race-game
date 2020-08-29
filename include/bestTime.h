#ifndef BESTTIME_H
#define BESTTIME_H

extern float bestTime;

void saveBestTime(const float *time, const char path[]);
void loadBestTime(float *time, const char path[]);

#endif
