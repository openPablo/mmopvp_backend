#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "networking.h"
#include "game.h"

#define PORT 8080
const float SPEED = 20.0f;

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
void close_player(struct Player *players, const int i) {
    printf("Player %d closed\n", i);
    players[i].flags = ~ACTIVE;
}
void input_buffer_player(struct inputBuffer *buffers, struct inputBuffer *buf, int idx) {
    buffers[idx] = *buf;
}
void sync_buffer_to_player(struct Player *players, struct inputBuffer *buffers) {
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
void update_player_clients(struct Player *players, ClientContext *clients) {
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
            if (clients[i].dc) {
                rtcSendMessage(clients[i].dc, (char*)active_players, sizeof(struct Player) * count);
            }
        }
    }
}
void game_loop(struct Player *players, struct inputBuffer *buffers, ClientContext *clients) {
    struct timespec ts;
    uint64_t next_tick;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    next_tick = (uint64_t)ts.tv_sec * NS_PER_SEC + ts.tv_nsec;
    printf("Game loop started at %dms tickrate.\n", TICK_RATE_MS);

    while (1) {
        sync_buffer_to_player(players, buffers);
        update_player_clients(players, clients);




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
    struct inputBuffer buffers[MAX_PLAYERS] = {0};
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
