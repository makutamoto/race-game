#ifndef LOGGER_H
#define LOGGER_H

int initLogger(void);
void closeLogger(void);
void logBase(const char *status, const char *message);
#define logInfo(message) (logBase("Info", message))
#define logError(message) (logBase("Error", message))
#define logDebug(message) (logBase("Debug", message))

#endif
