#include <stdio.h>
#include <math.h>
#include <time.h>

#include "networking.h"
#include "game.h"

#define PORT 8080
const float SPEED = 15.0f;
const float PROJECTILESPEED = 20.0f;

int spawn_player(struct Player *players) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!(players[i].flags & ACTIVE)) {
            players[i].x = 500.0f;
            players[i].y = 100.0f;
            players[i].angle = 0.0f;
            players[i].flags |= ACTIVE;
            players[i].id = i;
            players[i].hero |= AIRMAGE;
            players[i].health = 100;
            players[i].animating_ms = 0;
            return i;
        }
    }
    return -1;
}

void shoot_projectile(int id, const struct InputBuffer *buffers, struct ProjectilePool *projectiles, struct ProjectilePool *newProjectiles) {
    struct Projectile projectile = {
        .x=buffers[id].x,
        .y=buffers[id].y,
        .dx=(float)cos(M_PI / 180 * buffers[id].angle),
        .dy=(float)sin(M_PI / 180 * buffers[id].angle),
        .id=(uint8_t)id,
        .castSpell=buffers[id].castSpell
    };
    projectiles->array[projectiles->length] = projectile;
    projectiles->length++;
    newProjectiles->array[newProjectiles->length] = projectile;
    newProjectiles->length++;
}
void close_player(struct Player *players, const int i) {
    printf("Player %d closed\n", i);
    players[i].flags = ~ACTIVE;
}
void input_buffer_player(struct InputBuffer *buffers, struct InputBuffer *buf, int idx) {
    buffers[idx] = *buf;
}
void buffer_status_to_players(struct Player *players, struct InputBuffer *buffers, struct ProjectilePool *projectiles, struct ProjectilePool *newProjectiles) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i].flags & ACTIVE) {
            players[i].x += buffers[i].dir_x * SPEED;
            players[i].y += buffers[i].dir_y * SPEED;
            players[i].angle = buffers[i].angle;
            buffers[i].dir_x  = 0.0f;
            buffers[i].dir_y  = 0.0f;
            if (buffers[i].castSpell > 0 && !(players[i].flags & IS_CASTING)) {
                players[i].flags |= buffers[i].castSpell; //todo: only allow one spell at a time to be
                players[i].animating_ms = HERO_DATA[players[i].hero].cast_time_ms;
                shoot_projectile(i,buffers,projectiles, newProjectiles); //todo: Check if spell casted needs to shoot a projectile or not
                buffers[i].castSpell = 0;
            }
            if (players[i].flags & IS_CASTING) {
                players[i].animating_ms -= TICK_RATE_MS;
                if (players[i].animating_ms <= 0) {
                    players[i].flags &= ~IS_CASTING;
                }
            }

        }
    }
}
void move_projectiles(struct ProjectilePool *projectiles) {
    for (int i = 0; i < projectiles->length; i++) {
        projectiles->array[i].x += projectiles->array[i].dx * PROJECTILESPEED;
        projectiles->array[i].y += projectiles->array[i].dy * PROJECTILESPEED;
    }
}
void update_player_clients(const struct Player *players,const struct ProjectilePool *newProjectiles, const ClientContext *clients) {
    struct Player active_players[MAX_PLAYERS];
    int count = 0;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i].flags & ACTIVE) {
            active_players[count] = players[i];
            count++;
        }
    }
    for (int i = 0; i < MAX_PLAYERS ; i++) {
        if (players[i].flags & ACTIVE) {
            sendPlayerData(active_players, count , &clients[i]);
            sendProjectileData(newProjectiles->array, newProjectiles->length, &clients[i]);
        }
    }
}
void game_loop(struct Player *players, struct InputBuffer *buffers, ClientContext *clients, struct ProjectilePool *projectiles) {
    struct timespec ts;
    uint64_t next_tick;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    next_tick = (uint64_t)ts.tv_sec * NS_PER_SEC + ts.tv_nsec;
    printf("Game loop started at %dms tickrate.\n", TICK_RATE_MS);
    struct ProjectilePool newProjectiles = {0};
    while (1) {
        struct timespec start_ts, end_ts;
        clock_gettime(CLOCK_MONOTONIC, &start_ts);
        newProjectiles.length = 0;

        buffer_status_to_players(players, buffers, projectiles, &newProjectiles);
        update_player_clients(players, &newProjectiles, clients);
        move_projectiles(projectiles);

        clock_gettime(CLOCK_MONOTONIC, &end_ts);
        uint64_t elapsed_ns = (end_ts.tv_sec - start_ts.tv_sec) * NS_PER_SEC + (end_ts.tv_nsec - start_ts.tv_nsec);
        printf("Loop processing time: %f ms\n", (double)elapsed_ns / NS_PER_MS);

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
    struct Player players[MAX_PLAYERS] = {0};
    struct InputBuffer buffers[MAX_PLAYERS] = {0};
    ClientContext clients[MAX_PLAYERS] = {0};
    struct authenticatedPlayer *authenticatedPlayers = NULL;
    struct ProjectilePool projectiles;

    ServerContext server_ctx = {
        .players = players,
        .inputBuffers = buffers,
        .clients = clients,
        .authenticatedPlayers = authenticatedPlayers
    };

    int server = start_networking_server(8080, &server_ctx);
    if (server < 0) {
        return EXIT_FAILURE;
    }

    printf("Signaling server listening on port %d...\n", PORT);
    game_loop(players, buffers, clients, &projectiles);

    stop_networking_server(server);
    cleanup_networking();
    return EXIT_SUCCESS;
}
