#ifndef NETWORKING_H
#define NETWORKING_H

#include <rtc/rtc.h>

typedef struct {
    int ws;
    int pc;
} ClientContext;

int start_networking_server(int port);
void stop_networking_server(int server);
void cleanup_networking();

#endif // NETWORKING_H
