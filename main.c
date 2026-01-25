#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "networking.h"
#include "game.h"

#define TICK_RATE_MS 66
#define NS_PER_MS 1000000
#define NS_PER_SEC 1000000000
#define MAX_PLAYERS 1000
const float SPEED = 0.001f;

void move_players(struct Player *players, struct PlayerInput *player_inputs) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i].flags & PLAYER_ACTIVE) {
            players[i].x += player_inputs[i].dir_x * SPEED;
            players[i].y += player_inputs[i].dir_y * SPEED;

            player_inputs[i].dir_x  = 0.0f;
            player_inputs[i].dir_y  = 0.0f;
        }
    }
}
void update_player_clients(struct Player *players, ClientContext *clients) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i].flags & PLAYER_ACTIVE) {
            //rtcSendMessage(--------------, (char*)&players[i], sizeof(struct Player));
        }
    }
}
void game_loop(struct Player *players, struct PlayerInput *player_inputs, ClientContext *clients) {
    struct timespec ts;
    uint64_t next_tick;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    next_tick = (uint64_t)ts.tv_sec * NS_PER_SEC + ts.tv_nsec;
    printf("Game loop started at %dms tickrate.\n", TICK_RATE_MS);

    while (1) {
        move_players(players, player_inputs);
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
    struct PlayerInput player_inputs[MAX_PLAYERS] = {0};
    ClientContext clients[MAX_PLAYERS] = {0};

    int server = start_networking_server(8080);
    if (server < 0) {
        return EXIT_FAILURE;
    }

    printf("Signaling server listening on port 8080...\n");

    game_loop(players, player_inputs, clients);

    stop_networking_server(server);
    cleanup_networking();
    return EXIT_SUCCESS;
}
