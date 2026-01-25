#ifndef NETWORKING_H
#define NETWORKING_H

#include <rtc/rtc.h>
#include "game.h"

typedef struct {
    struct Player *players;
    struct ClientContext *clients;
} ServerContext;

typedef struct ClientContext {
    int ws;
    int pc;
    int dc;
    int player_idx;
} ClientContext;

int start_networking_server(int port, ServerContext *ctx);
void stop_networking_server(int server);
void cleanup_networking();

#endif // NETWORKING_H
