#ifndef PVPMMO_BACKEND_GAME_H
#define PVPMMO_BACKEND_GAME_H
#include <stdint.h>

#define MAX_PLAYERS 1000
#define SPEED 15.0f;


#define PROJECTILES_MAX 5000 //Assuming max 10k projectiles will be in flight and 1k will explode at the same game tick
#define PROJECTILE_SPEED 20.0f;
#define PROJECTILE_DISTANCE 400.0f

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
struct Player { //ToDO: convert x/y to sint16
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
struct PlayerPool {
    struct Player array[MAX_PLAYERS];
    short length;
};
#pragma pack(push, 1)
struct InputBuffer {
    float dir_x;
    float dir_y;
    float angle;
    int16_t x;
    int16_t y;
    uint8_t castSpell;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct Projectile {
    float dx;
    float dy;
    float x;
    float y;
    int travelled;
    uint16_t id;
    uint8_t castSpell;
};
#pragma pack(pop)

struct ProjectilePool{
    struct Projectile array[PROJECTILES_MAX];
    short length;
};
struct intPool{
    short array[PROJECTILES_MAX/10];
    short length;
};
int spawn_player(struct Player *players);
void close_player(struct Player *players, int i);
void input_buffer_player(struct InputBuffer *buffers, struct InputBuffer *buf, int idx);

#endif