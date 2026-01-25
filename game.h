#ifndef PVPMMO_BACKEND_GAME_H
#define PVPMMO_BACKEND_GAME_H
#include <stdint.h>
typedef enum {
    PLAYER_ACTIVE   = 1 << 0,
    PLAYER_DEAD     = 1 << 1,
} PlayerFlags;
struct Player {
    float x;
    float y;
    float angle;
    uint16_t flags;
};
struct PlayerInput {
    float dir_x;
    float dir_y;
    float angle;
};
#endif