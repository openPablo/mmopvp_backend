#ifndef PVPMMO_BACKEND_GAME_H
#define PVPMMO_BACKEND_GAME_H
#include <stdint.h>

#define MAX_PLAYERS 1000
#define TICK_RATE_MS 66
#define NS_PER_MS 1000000
#define NS_PER_SEC 1000000000

typedef enum {
    PLAYER_ACTIVE   = 1 << 0,
    PLAYER_DEAD     = 1 << 1,
} PlayerFlags;
struct Player {
    float x;
    float y;
    float angle;
    uint16_t flags;
    uint16_t id;
};
struct PlayerInput {
    float dir_x;
    float dir_y;
    float angle;
};

int spawn_player(struct Player *players);
void close_player(struct Player *players, int i);

#endif