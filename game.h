#ifndef PVPMMO_BACKEND_GAME_H
#define PVPMMO_BACKEND_GAME_H
#include <stdint.h>

#define MAX_PLAYERS 1000
#define TICK_RATE_MS 66
#define NS_PER_MS 1000000
#define NS_PER_SEC 1000000000

typedef enum {
    ACTIVE   = 1 << 0,
    DEAD     = 1 << 1,
    INVULNERABLE  = 1 << 2,
    CASTING_1    = 1 << 3,
    CASTING_2    = 1 << 5,
    CASTING_3    = 1 << 6,
    CASTING_ULTI    = 1 << 7
} PlayerFlag;
#define IS_CASTING (CASTING_1 | CASTING_2 | CASTING_3 | CASTING_ULTI)
typedef enum {
    AIRMAGE = 1 << 0,
    PRIEST  = 1 << 1,
    ROGUE   = 1 <<2
} PlayerHero;

struct HeroStats {
    int cast_time_ms;
    int base_health;
};

static const struct HeroStats HERO_DATA[] = {
    [AIRMAGE]   = { .cast_time_ms = 400, .base_health = 100 },
    [PRIEST] = { .cast_time_ms = 400, .base_health = 100 },
    [ROGUE] = { .cast_time_ms = 400, .base_health = 100 }
};
#pragma pack(push, 1)
struct Player {
    float x;
    float y;
    float angle;
    int animating_ms;
    uint16_t id;
    uint16_t flags;
    uint16_t hero;
    uint16_t health;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct InputBuffer {
    float dir_x;
    float dir_y;
    float angle;
    uint8_t castSpell;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct Projectile {
    float x;
    float y;
    float dx;
    float dy;
    uint16_t id;
};
#pragma pack(pop)

struct ProjectilePool{
    struct Projectile array[MAX_PLAYERS * 20];
    int length;
} ;
int spawn_player(struct Player *players);
void close_player(struct Player *players, int i);
void input_buffer_player(struct InputBuffer *buffers, struct InputBuffer *buf, int idx);

#endif