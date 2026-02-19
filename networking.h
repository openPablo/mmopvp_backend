#ifndef NETWORKING_H
#define NETWORKING_H

#include <rtc/rtc.h>
#include "gameDataStructures.h"
#include "uthash.h"
#define PORT 8080

typedef struct {
    struct PlayerPool *playerPool;
    struct InputBuffer **inputBuffersMap;
    struct ClientContext *clients;
    struct authenticatedPlayer *authenticatedPlayersMap;
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
    int dc_explodingProjectiles;
    int dc_cones;
    int dc_circles;
    char bearer_token[31];
    uint16_t player_idx;
} ClientContext;

int start_networking_server(int port, ServerContext *ctx);
void stop_networking_server(int server);
void cleanup_networking();
void sendPlayerData(const struct PlayerPool *players, const ClientContext *ctx);
void sendNewProjectilesData(const struct ProjectilePool *projectilePool, const ClientContext *ctx);
void sendExplodingProjectilesData(const struct shortPool *exploding, const ClientContext *ctx);
void sendNewConesData(const struct AoEConePool *pool, const ClientContext *ctx);
void sendNewCirclesData(const struct AoECirclePool *pool, const ClientContext *ctx);
void update_player_clients(const ClientContext *clients, const struct PlayerPool *players, const struct shortPool *explodingProjectiles,struct SpellsContext *newSpells, unsigned char *gzipBuffer);
#endif // NETWORKING_H
