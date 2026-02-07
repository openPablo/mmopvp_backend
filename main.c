#include <stdio.h>
#include <math.h>
#include <time.h>

#include "networking.h"
#include "gameData.h"

int spawn_player(struct Player *players) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!(players[i].flags & ACTIVE)) {
            players[i].x = 500.0f;
            players[i].y = 100.0f;
            players[i].angle = 0.0f;
            players[i].id = i;
            players[i].hero |= AIRMAGE;
            players[i].flags |= ACTIVE;
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
void game_loop(struct Player *players, struct InputBuffer *buffers,const ClientContext *clients) {
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

        buffer_status_to_players(players, buffers, &projectiles, &newProjectiles);
        move_projectiles(&projectiles, &explodingProjectiles);
        update_player_clients(clients, players, &newProjectiles, &explodingProjectiles);

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
    struct Player players[MAX_PLAYERS] = {0};
    struct InputBuffer buffers[MAX_PLAYERS] = {0};
    ClientContext clients[MAX_PLAYERS] = {0};
    struct authenticatedPlayer *authenticatedPlayers = NULL;

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
    game_loop(players, buffers, clients);

    stop_networking_server(server);
    cleanup_networking();
    return EXIT_SUCCESS;
}
