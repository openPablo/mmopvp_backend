#ifndef PVPMMO_BACKEND_GAME_H
#define PVPMMO_BACKEND_GAME_H
#include <stdatomic.h>
#include <stdint.h>

#include "uthash.h"


#define GAME_MAX_WIDTH 1920
#define GAME_MAX_HEIGHT 1080

#define MAX_PLAYERS 1000
#define SPEED 30.0f
#define SPELLS_MAX 4000 //amount of same sort of spells at the same time
#define PROJECTILES_MAX 4000 //Assuming max 4k projectiles will be in flight and 1k will explode at the same game tick
#define PROJECTILE_SPEED 40.0f

#define TICK_RATE_MS 16
#define NS_PER_MS 1000000
#define NS_PER_SEC 1000000000

typedef enum {
    IDLE,
    WALKING,
    DEAD,
    CASTING_1,
    CASTING_2,
    CASTING_3,
    CASTING_ULTI,
    INVULNERABLE,
    STUNNED
} PlayerState;
typedef enum {
    AIRMAGE,
    PRIEST,
    ROGUE
} Hero;

struct HeroStats {
    int cast_time_ms;
    int base_health;
};

#pragma pack(push, 1)
struct Player {
    float x;
    float y;
    float angle;
    int animating_ms;
    PlayerState state;
    uint16_t id;
    uint16_t hero;
    uint16_t health;
};
#pragma pack(pop)
struct PlayerPool {
    struct Player *array;
    atomic_uint_fast16_t length;
};
#pragma pack(push, 1)
struct InputBufferNetwork {
    float dir_x;
    float dir_y;
    float angle;
    int16_t x;
    int16_t y;
    uint8_t castSpell;
};
#pragma

struct InputBuffer {
    float dir_x;
    float dir_y;
    float angle;
    int id;
    int16_t x;
    int16_t y;
    uint8_t castSpell;
    UT_hash_handle hh;
};


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
struct shortPool{
    uint16_t *array;
    short length;
};
#pragma pack(push, 1)
struct AoECone {
    float x;
    float y;
    float angle;
    int time_ms;
    uint16_t length;
    uint16_t id;
    uint8_t castSpell;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct AoECircle {
    float x;
    float y;
    int time_ms;
    uint16_t radius;
    uint16_t id;
    uint8_t castSpell;
};
#pragma pack(pop)

struct AoECirclePool{
    struct AoECircle array[SPELLS_MAX];
    short length;
};
struct AoEConePool{
    struct AoECone array[SPELLS_MAX];
    short length;
};

typedef struct {
    void *array;
    ssize_t size;
    short length;
} Pool;

struct SpellsContext {
    struct ProjectilePool projectiles;
    struct AoEConePool cones;
    struct AoECirclePool circles;;
};
int spawn_player(struct PlayerPool *players, Hero hero);
void close_player(struct PlayerPool *players, struct InputBuffer **buffers, int id);
void save_inputbuffer(struct InputBuffer **buffers, struct InputBufferNetwork *net, int id);
#endif