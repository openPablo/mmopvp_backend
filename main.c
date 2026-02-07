#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdatomic.h>

#include "networking.h"
#include "gameDataStructures.h"
atomic_uint_fast16_t THREADSAFE_PLAYER_ID = 1;


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

void shoot_projectile(int id, const struct InputBuffer *buffers, struct ProjectilePool *projectiles, struct ProjectilePool *newProjectiles) {
    struct Projectile projectile = {
        .x=buffers[id].x,
        .y=buffers[id].y,
        .dx=(float)cos(M_PI / 180 * buffers[id].angle),
        .dy=(float)sin(M_PI / 180 * buffers[id].angle),
        .id=(uint16_t)id,
        .travelled = PROJECTILE_DISTANCE,
        .castSpell=buffers[id].castSpell
    };
    projectiles->array[projectiles->length] = projectile;
    projectiles->length++;
    newProjectiles->array[newProjectiles->length] = projectile;
    newProjectiles->length++;
}
void explode_projectile(int i, struct ProjectilePool *projectiles, struct intPool *explodingProjectiles) {
    explodingProjectiles->array[explodingProjectiles->length] = projectiles->array[i].id;
    explodingProjectiles->length++;
    projectiles->array[i] = projectiles->array[projectiles->length - 1];
    projectiles->length--;
}
void detect_collission_projectiles(struct ProjectilePool *projectiles, struct Player players, struct intPool *explodingProjectiles) {
    for (int i = 0; i < projectiles->length; i++) {
        explode_projectile(i, projectiles, explodingProjectiles);
        i--;
    }
}
void close_player(struct PlayerPool *players,int id) { //threadsafe
    for (int i = 0; i < players->length; i++) {
        if (players->array[i].id == id) {
            atomic_uint_fast16_t indx  = atomic_fetch_sub(&players->length, 1);
            players->array[i] = players->array[indx+1];
            printf("Player %d closed\n", id);
        }
    }
}
void input_buffer_player(struct InputBuffer *buffers, struct InputBuffer *buf, int idx) {
    buffers[idx] = *buf;// todo
}
void sync_inputbuffer_to_players(struct PlayerPool *players, struct InputBuffer *buffers) {
    for (int i = 0; i < players->length; i++) {
        players->array[i].x += buffers[i].dir_x * SPEED;
        players->array[i].y += buffers[i].dir_y * SPEED;
        players->array[i].angle = buffers[i].angle;
        players->array[i].state |= buffers[i].castSpell;
        buffers[i].castSpell = 0;
        buffers[i].dir_x  = 0.0f;
        buffers[i].dir_y  = 0.0f;
    }
}

void move_projectiles(struct ProjectilePool *projectiles, struct intPool *explodingProjectiles) {
    for (int i = 0; i < projectiles->length; i++) {
        if (projectiles->array[i].travelled <= 0) {
            explode_projectile(i,projectiles, explodingProjectiles);
            i--;
        }else {
            projectiles->array[i].x += projectiles->array[i].dx * PROJECTILE_SPEED;
            projectiles->array[i].y += projectiles->array[i].dy * PROJECTILE_SPEED;
            projectiles->array[i].travelled -= TICK_RATE_MS;
        }
    }
}
void update_player_clients(const ClientContext *clients, const struct Player *players,const struct ProjectilePool *newProjectiles, const struct intPool *explodingProjectiles ) {
    struct Player active_players[MAX_PLAYERS];
    int count = 0;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i].flags & ACTIVE) {
            active_players[count] = players[i];
            count++;
        }
    }
    for (int i = 0; i < MAX_PLAYERS ; i++) {
        if (clients[i].dc_player > 0) {
            sendPlayerData(active_players, count , &clients[i]);

            if (newProjectiles && newProjectiles->length > 0) {
                sendNewProjectilesData(newProjectiles, &clients[i]);
            }
            if (explodingProjectiles && explodingProjectiles->length > 0) {
                sendExplodingProjectilesData(explodingProjectiles, &clients[i]);
            }
        }
    }
}
void game_loop(struct PlayerPool *airmages, struct InputBuffer *buffers,const ClientContext *clients) {
    struct timespec ts;
    uint64_t next_tick;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    next_tick = (uint64_t)ts.tv_sec * NS_PER_SEC + ts.tv_nsec;
    printf("Game loop started at %dms tickrate.\n", TICK_RATE_MS);

    struct ProjectilePool projectiles = {0};
    struct ProjectilePool newProjectiles = {0};
    struct intPool explodingProjectiles= {0};
    while (1) {
        struct timespec start_ts, end_ts;
        clock_gettime(CLOCK_MONOTONIC, &start_ts);
        newProjectiles.length = 0;
        explodingProjectiles.length = 0;

        sync_inputbuffer_to_players(airmages->array, buffers);
        move_projectiles(&projectiles, &explodingProjectiles);
        update_player_clients(clients, airmages->array, &newProjectiles, &explodingProjectiles);

        clock_gettime(CLOCK_MONOTONIC, &end_ts);
        uint64_t elapsed_ns = (end_ts.tv_sec - start_ts.tv_sec) * NS_PER_SEC + (end_ts.tv_nsec - start_ts.tv_nsec);
        //printf("Loop processing time: %f ms\n", (double)elapsed_ns / NS_PER_MS);

        next_tick += (TICK_RATE_MS * NS_PER_MS);
        clock_gettime(CLOCK_MONOTONIC, &ts);
        uint64_t now = (uint64_t)ts.tv_sec * NS_PER_SEC + ts.tv_nsec;

        if (now < next_tick) {
            struct timespec sleep_ts;
            uint64_t diff = next_tick - now;
            sleep_ts.tv_sec = diff / NS_PER_SEC;
            sleep_ts.tv_nsec = diff % NS_PER_SEC;
            nanosleep(&sleep_ts, NULL);
        }
    }
}

int main() {
    struct PlayerPool airmages = {0};
    struct InputBuffer buffers[MAX_PLAYERS] = {0};
    ClientContext clients[MAX_PLAYERS] = {0};
    struct authenticatedPlayer *authenticatedPlayers = NULL;

    ServerContext server_ctx = {
        .players = airmages.array,
        .inputBuffers = buffers,
        .clients = clients,
        .authenticatedPlayers = authenticatedPlayers
    };

    int server = start_networking_server(8080, &server_ctx);
    if (server < 0) {
        return EXIT_FAILURE;
    }

    printf("Signaling server listening on port %d...\n", PORT);
    game_loop(&airmages, buffers, clients);

    stop_networking_server(server);
    cleanup_networking();
    return EXIT_SUCCESS;
}
