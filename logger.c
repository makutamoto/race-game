#include<stdio.h>
#include<time.h>

FILE *logFile = NULL;

char* getCurrentTime(char *result, size_t n) {
	time_t raw = time(NULL);
	struct tm info;
	if(localtime_s(&info, &raw)) return NULL;
	strftime(result, n, "%a %m %d %H:%M:%S %Y", &info);
	return result;
}

int initLogger(void) {
	char date[64];
	if(fopen_s(&logFile, "./log.txt", "w") != 0) return -1;
	fprintf(logFile, "This session started at %s.\n", getCurrentTime(date, 64));
	return 0;
}

void closeLogger(void) {
	if(logFile != NULL) fclose(logFile);
}

void logBase(const char *status, const char *message) {
	if(logFile == NULL) {
		printf("Logger must be initialized before logging.");
		return;
	}
	char date[64];
	fprintf(logFile, "%s: [%s] %s\n", getCurrentTime(date, 64), status, message);
}
