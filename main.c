#include <stdio.h>
#include <time.h>
#include <stdatomic.h>
#include <math.h>
#include <unistd.h>

#include "networking.h"
#include "gameDataStructures.h"
#include "gamelogic/character_state_machines.h"
#include "gamelogic/projectile.h"
#include "gamelogic/area_of_effect.h"
atomic_uint_fast16_t THREADSAFE_PLAYER_ID = 0;


int spawn_player(struct PlayerPool *players, Hero hero) {
    atomic_uint_fast16_t id = atomic_fetch_add(&THREADSAFE_PLAYER_ID, 1);
    atomic_uint_fast16_t index = atomic_fetch_add(&players->length, 1);
    if (index >= MAX_PLAYERS) {
        atomic_fetch_sub(&players->length, 1);
        return -1;
    }

    struct Player player;
    player.x = 500.0f;
    player.y = 100.0f;
    player.angle = 0.0f;
    player.id = id;
    player.health = 100;
    player.hero = hero;
    player.state = IDLE;
    player.animating_ms = 0;

    players->array[index] = player;

    return id;
}

void close_player(struct PlayerPool *players, struct InputBuffer **buffers, int id) {
    struct InputBuffer *s;
    HASH_FIND_INT(*buffers, &id, s);
    if (s) {
        HASH_DEL(*buffers, s);
        free(s);
    }
    for (int i = 0; i < players->length; i++) {
        if (players->array[i].id == id) {
            atomic_uint_fast16_t indx = atomic_fetch_sub(&players->length, 1);
            players->array[i] = players->array[indx - 1];
            printf("Player %d closed\n", id);
            break;
        }
    }
}
void save_inputbuffer(struct InputBuffer **buffers, struct InputBufferNetwork *net, int id) {
    struct InputBuffer *s;
    HASH_FIND_INT(*buffers, &id, s);

    if (s == NULL) {
        s = malloc(sizeof(struct InputBuffer));
        s->id = id;
        HASH_ADD_INT(*buffers, id, s);
    }
    s->dir_x = net->dir_x;
    s->dir_y = net->dir_y;
    s->angle = net->angle;
    s->x = net->x;
    s->y = net->y;
    s->castSpell = net->castSpell;

}

void update_players_states(struct PlayerPool *players, struct InputBuffer *buffersMap, struct SpellsContext *newSpells) {
    for (int i = 0; i < players->length; i++) {
        struct InputBuffer *buf;
        HASH_FIND_INT(buffersMap, &players->array[i].id, buf);
        if (buf) {
            compute_airmage_state(&players->array[i], buf, newSpells);
        }
    }
}
void extendSpellPools(struct SpellsContext *newSpells, struct SpellsContext *spells) {
    for (int i = 0 ; i < newSpells->projectiles.length; i++) {
        spells->projectiles.array[spells->projectiles.length] = newSpells->projectiles.array[i];
        spells->projectiles.length++;
    }
    for (int i = 0; i < newSpells->cones.length; i++) {
        spells->cones.array[spells->cones.length] = newSpells->cones.array[i];
        spells->cones.length++;
    }
    for (int i = 0; i < newSpells->circles.length; i++) {
        spells->circles.array[spells->circles.length] = newSpells->circles.array[i];
        spells->circles.length++;
    }
}

int hashPositionToGrid(const struct Player *player, int width, int height, int boxcount) {
    return floor(player->x/width) + floor(player->y/height) * boxcount;
}

void create_collision_grid(int *grid, struct Player *grid_data, int max_width_px, int max_height_px, int boxcount, const struct PlayerPool *players) {
    if (players->length == 0) return;
    memset(grid,0,sizeof(int) * (boxcount * boxcount + 1));

    int width = max_width_px / boxcount;
    int height = max_height_px / boxcount;

    for (int i = 0; i < players->length; i++) {
        grid[
            hashPositionToGrid(
                &players->array[i],
                width,
                height,
                boxcount)
            ] += 1;
    }
    int sum = 0;
    for (int i = 0; i < boxcount * boxcount +1; i++) {
        sum += grid[i];
        grid[i] = sum;
    }
    for (int i = 0; i < players->length; i++) {//
        int pointer = hashPositionToGrid(
                &players->array[i],
                width,
                height,
                boxcount)
            ;
        --grid[pointer];
        grid_data[grid[pointer]] = players->array[i];
    }
}
void limit_players_to_map(struct PlayerPool *players, int max_width, int max_height) {
    for (int i = 0; i < players->length; i++) {
        if (players->array[i].x > max_width-100.0f)
            players->array[i].x = max_width-100.0f;
        else if (players->array[i].x < 20.0f) {
            players->array[i].x = 20.0f;
        }
        if (players->array[i].y > max_height-50.0f)
            players->array[i].y = max_height-50.0f;
        else if (players->array[i].y < 20.0f) {
            players->array[i].y = 20.0f;
        }
    }
}
void check_projectile_player_collision(struct ProjectilePool projectiles, struct Player *grid_data) {

}

struct PlayerPool createPlayerPool (short capacity){
    struct PlayerPool tmp;
    tmp.length = 0;
    tmp.array = malloc(sizeof(struct Player) * capacity);
    return tmp;
}
void log_performance(struct timespec *start_ts, struct timespec *end_ts) {
    clock_gettime(CLOCK_MONOTONIC, end_ts);
    uint64_t elapsed_ns = (end_ts->tv_sec - start_ts->tv_sec) * NS_PER_SEC + (end_ts->tv_nsec - start_ts->tv_nsec);
    printf("Loop processing time: %f ms\n", (double)elapsed_ns / NS_PER_MS);
}
void sleep_until_next_tick(struct timespec *ts, uint64_t *next_tick) {
    *next_tick += (TICK_RATE_MS * NS_PER_MS);
    clock_gettime(CLOCK_MONOTONIC, ts);
    uint64_t now = (uint64_t)ts->tv_sec * NS_PER_SEC + ts->tv_nsec;

    if (now < *next_tick) {
        struct timespec sleep_ts;
        uint64_t diff = *next_tick - now;
        sleep_ts.tv_sec = diff / NS_PER_SEC;
        sleep_ts.tv_nsec = diff % NS_PER_SEC;
        nanosleep(&sleep_ts, NULL);
    }
}
void game_loop(struct PlayerPool *airmages, struct InputBuffer **buffersMap, const ClientContext *clients) {
    printf("Game loop started at %dms tickrate.\n", TICK_RATE_MS);

    struct SpellsContext spells = {0};
    struct SpellsContext newSpells = {0};
    struct shortPool explodingProjectiles;
    explodingProjectiles.length = 0;
    explodingProjectiles.array = malloc(sizeof(short) * PROJECTILES_MAX);
    int *grid = malloc(sizeof(int) * 101);
    struct Player *grid_data = malloc(sizeof(struct Player) * MAX_PLAYERS);
    struct timespec ts, start_ts, end_ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t next_tick = (uint64_t)ts.tv_sec * NS_PER_SEC + ts.tv_nsec;
    unsigned char *gzipBuffer = malloc(sizeof(struct Player) * MAX_PLAYERS);

    while (1) {
        clock_gettime(CLOCK_MONOTONIC, &start_ts);
        explodingProjectiles.length = 0;
        newSpells.projectiles.length = 0;
        newSpells.circles.length = 0;
        newSpells.cones.length = 0;

        update_players_states(airmages, *buffersMap, &newSpells);
        extendSpellPools(&newSpells, &spells);
        move_projectiles(&spells.projectiles, &explodingProjectiles);
        timelapse_cones(&spells.cones);
        timelapse_circles(&spells.circles);


        limit_players_to_map(airmages,GAME_MAX_WIDTH,GAME_MAX_HEIGHT);
        create_collision_grid(grid, grid_data, GAME_MAX_WIDTH,GAME_MAX_HEIGHT, 10, airmages);
        check_projectile_player_collision(spells.projectiles, grid_data);
        update_player_clients(clients, airmages, &explodingProjectiles, &newSpells, gzipBuffer);

        log_performance(&start_ts, &end_ts);
        sleep_until_next_tick(&ts, &next_tick);
    }
}

int main() {
    struct PlayerPool airmages = createPlayerPool(MAX_PLAYERS);

    ClientContext *clients = malloc(sizeof(ClientContext) * MAX_PLAYERS);
    //init hashmaps to NULL
    struct InputBuffer *buffersMap = NULL;
    struct authenticatedPlayer *authenticatedPlayersMap = NULL;

    ServerContext server_ctx = {
        .playerPool = &airmages,
        .inputBuffersMap = &buffersMap,
        .clients = clients,
        .authenticatedPlayersMap = authenticatedPlayersMap
    };

    int server = start_networking_server(8080, &server_ctx);
    if (server < 0) {
        return EXIT_FAILURE;
    }

    printf("Signaling server listening on port %d...\n", PORT);
    game_loop(&airmages, &buffersMap, clients);

    stop_networking_server(server);
    cleanup_networking();
    return EXIT_SUCCESS;
}
