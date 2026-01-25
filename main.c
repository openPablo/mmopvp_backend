#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "networking.h"

#define TICK_RATE_MS 33
#define NS_PER_MS 1000000
#define NS_PER_SEC 1000000000

void game_loop() {
    struct timespec ts;
    uint64_t next_tick;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    next_tick = (uint64_t)ts.tv_sec * NS_PER_SEC + ts.tv_nsec;

    printf("Game loop started at %dms tickrate.\n", TICK_RATE_MS);

    while (1) {
        // update_game_world();

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
    int server = start_networking_server(8080);
    if (server < 0) {
        return EXIT_FAILURE;
    }

    printf("Signaling server listening on port 8080...\n");

    game_loop();

    stop_networking_server(server);
    cleanup_networking();
    return EXIT_SUCCESS;
}
