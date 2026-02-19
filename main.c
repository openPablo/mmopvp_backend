#include <stdio.h>
#include <time.h>
#include <stdatomic.h>
#include <math.h>

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

void update_player_clients(const ClientContext *clients, const struct PlayerPool *players, const struct shortPool *explodingProjectiles,struct SpellsContext *newSpells) {
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
// Create array of gridcells
// Create hash function to map x/y position to gridcell
//

int hashPositionToGrid(struct Player *player, int width, int height, int boxcount) {
    return floor(player->x/width) + floor(player->y/height) * boxcount;
}


void create_grid(int max_width_px, int max_height_px, int *grid, int boxcount, struct PlayerPool *players) {
    struct Player grid_data[players->length];
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
    for (int i = 0; i<= boxcount * boxcount +1; i++) {
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
        grid_data[grid[pointer]] = players->array[i];
        grid[pointer] -= 1;
    }
}
uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * NS_PER_SEC + ts.tv_nsec;
}

void sync_frame(uint64_t *next_tick) {
    *next_tick += (TICK_RATE_MS * NS_PER_MS);
    uint64_t now = get_time_ns();

    if (now < *next_tick) {
        struct timespec sleep_ts;
        sleep_ts.tv_sec = *next_tick - now / NS_PER_SEC;
        sleep_ts.tv_nsec = *next_tick - now % NS_PER_SEC;
        nanosleep(&sleep_ts, NULL);
    }
}

void game_loop(struct PlayerPool *airmages, struct InputBuffer **buffers, const ClientContext *clients) {
    uint64_t next_tick = get_time_ns();
    printf("Game loop started at %dms tickrate.\n", TICK_RATE_MS);

    struct SpellsContext spells = {0};
    struct SpellsContext newSpells = {0};
    struct shortPool explodingProjectiles = {0};
    int boxcount = 10;
    int grid[101] = {0}; //flattened 2d matrix, 10x10
    while (1) {
        create_grid(GAME_MAX_WIDTH,GAME_MAX_HEIGHT, grid, boxcount, airmages);
        explodingProjectiles.length = 0;
        newSpells.projectiles.length = 0;
        newSpells.circles.length = 0;
        newSpells.cones.length = 0;

        update_players_states(airmages, *buffers, &newSpells);
        extendSpellPools(&newSpells, &spells);

        move_projectiles(&spells.projectiles, &explodingProjectiles);
        timelapse_cones(&spells.cones);
        timelapse_circles(&spells.circles);
        update_player_clients(clients, airmages, &explodingProjectiles, &newSpells);

        sync_frame(&next_tick);
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
