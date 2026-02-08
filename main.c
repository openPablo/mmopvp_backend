#include <stdio.h>
#include <time.h>
#include <stdatomic.h>

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

void update_players_states(struct PlayerPool *players, struct InputBuffer *buffers, struct SpellsContext *newSpells) {
    for (int i = 0; i < players->length; i++) {
        struct InputBuffer *buf;
        HASH_FIND_INT(buffers, &players->array[i].id, buf);
        if (buf) {
            compute_airmage_state(&players->array[i], buf, newSpells);
        }
    }
}

void update_player_clients(const ClientContext *clients, const struct PlayerPool *players, const struct intPool *explodingProjectiles,struct SpellsContext *newSpells) {
    for (int i = 0; i < MAX_PLAYERS ; i++) {
        if (clients[i].dc_player > 0) {
            sendPlayerData(players, &clients[i]);
            sendNewProjectilesData(&newSpells->projectiles, &clients[i]);
            sendNewConesData(&newSpells->cones, &clients[i]);
            sendNewCirclesData(&newSpells->circles, &clients[i]);
            sendExplodingProjectilesData(explodingProjectiles, &clients[i]);
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
void game_loop(struct PlayerPool *airmages, struct InputBuffer **buffers,const ClientContext *clients) {
    struct timespec ts;
    uint64_t next_tick;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    next_tick = (uint64_t)ts.tv_sec * NS_PER_SEC + ts.tv_nsec;
    printf("Game loop started at %dms tickrate.\n", TICK_RATE_MS);

    struct SpellsContext spells = {0};
    struct SpellsContext newSpells = {0};
    struct intPool explodingProjectiles= {0};
    while (1) {
        struct timespec start_ts, end_ts;
        clock_gettime(CLOCK_MONOTONIC, &start_ts);
        explodingProjectiles.length = 0;
        newSpells.projectiles.length = 0;
        newSpells.circles.length = 0;
        newSpells.cones.length = 0;

        update_players_states(airmages, *buffers, &newSpells);
        extendSpellPools(&newSpells,&spells);

        move_projectiles(&spells.projectiles, &explodingProjectiles);
        update_player_clients(clients, airmages, &explodingProjectiles, &newSpells);

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
    ClientContext clients[MAX_PLAYERS] = {0};
    struct InputBuffer *buffers = NULL;
    struct authenticatedPlayer *authenticatedPlayers = NULL;

    ServerContext server_ctx = {
        .playerPool = &airmages,
        .inputBuffers = &buffers,
        .clients = clients,
        .authenticatedPlayers = authenticatedPlayers
    };

    int server = start_networking_server(8080, &server_ctx);
    if (server < 0) {
        return EXIT_FAILURE;
    }

    printf("Signaling server listening on port %d...\n", PORT);
    game_loop(&airmages, &buffers, clients);

    stop_networking_server(server);
    cleanup_networking();
    return EXIT_SUCCESS;
}
