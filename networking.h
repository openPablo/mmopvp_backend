#ifndef NETWORKING_H
#define NETWORKING_H

#include <rtc/rtc.h>
#include "game.h"
#include "uthash.h"

typedef struct {
    struct Player *players;
    struct InputBuffer *inputBuffers;
    struct ClientContext *clients;
    struct authenticatedPlayer *authenticatedPlayers;
} ServerContext;

struct authenticatedPlayer {
    char bearer_token[31];
    int idx;
    UT_hash_handle hh;
};
typedef struct ClientContext {
    int ws;
    int pc;
    int dc_player;
    int dc_projectiles;
    char bearer_token[31];
    int player_idx;
} ClientContext;

int start_networking_server(int port, ServerContext *ctx);
void stop_networking_server(int server);
void cleanup_networking();
void sendPlayerData(const struct Player *players, int count, const ClientContext *ctx);
void sendProjectileData(const struct Projectile *projectiles, int count, const ClientContext *ctx);
#endif // NETWORKING_H
