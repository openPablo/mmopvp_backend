#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "networking.h"
#include "gameDataStructures.h"

static ServerContext *server_ctx = NULL;

void add_player(int id, char token[30], struct authenticatedPlayer **authenticatedPlayers) {
    struct authenticatedPlayer *s;
    s = malloc(sizeof *s);
    s->idx = id;
    strcpy(s->bearer_token, token);
    HASH_ADD_STR(*authenticatedPlayers, bearer_token, s);
}
struct authenticatedPlayer *find_player(char bearer_token[30], struct authenticatedPlayer **authenticatedPlayers) {
    struct authenticatedPlayer *s;
    HASH_FIND_STR(*authenticatedPlayers, bearer_token, s);
    return s;
}
void delete_player(struct authenticatedPlayer *authenticatedPlayer, struct authenticatedPlayer **authenticatedPlayers) {
    HASH_DEL(*authenticatedPlayers, authenticatedPlayer);
    free(authenticatedPlayer);
}
static void on_local_description(int pc, const char *sdp, const char *type, void *ptr) {
    ClientContext *ctx = (ClientContext *)ptr;
    if (rtcIsOpen(ctx->ws)) {
        printf("Sending SDP %s via WS\n", type);
        rtcSendMessage(ctx->ws, sdp, -1);
    }
}

static void on_local_candidate(int pc, const char *cand, const char *mid, void *ptr) {
    ClientContext *ctx = (ClientContext *)ptr;
    if (rtcIsOpen(ctx->ws)) {
        rtcSendMessage(ctx->ws, cand, -1);
    }
}

static void on_ws_message(int ws, const char *message, int size, void *ptr) {
    ClientContext *ctx = (ClientContext *)ptr;
    if (!ctx || ctx->pc == 0) return;
    char *pos;
    if ((pos = strstr(message, "bearer_token="))) {
        strncpy(ctx->bearer_token, pos + 13, 30);
        printf("bearer_token=%s\n",ctx->bearer_token);
        return;
    }
    if (strstr(message, "v=0") != NULL) {
        rtcSetRemoteDescription(ctx->pc, message, NULL);
    } else {
        rtcAddRemoteCandidate(ctx->pc, message, NULL);
    }
}
static void sendIdConfirmationMessage(ClientContext *ctx) {
    char confirmIdMessage[10];
    snprintf(confirmIdMessage,10, "id=%d",ctx->player_idx);
    rtcSendMessage(ctx->ws, confirmIdMessage, 10);
}
static void on_dc_open(int dc, void *ptr) {
    ClientContext *ctx = (ClientContext *)ptr;
    printf("DataChannel open.\n");
    if (!server_ctx) return;
    struct authenticatedPlayer *player = find_player(ctx->bearer_token,&server_ctx->authenticatedPlayersMap);
    if (!player) {
        int idx = spawn_player(server_ctx->playerPool, AIRMAGE);
        if (idx >= 0) {
            ctx->player_idx = idx;
            server_ctx->clients[idx] = *ctx;
            add_player(ctx->player_idx,ctx->bearer_token, &server_ctx->authenticatedPlayersMap);
            printf("Player spawned at index %d\n", idx);
            sendIdConfirmationMessage(ctx);
        } else {
            printf("Failed to spawn player: server full\n");
        }
    } else {
        ctx->player_idx = player->idx;
        server_ctx->clients[player->idx] = *ctx;
        sendIdConfirmationMessage(ctx);
        printf("Player found at index %d\n", player->idx);
    }
}

static void on_dc_message(int ws, const char *message, int size, void *ptr) {
    ClientContext *ctx = (ClientContext *)ptr;
    if (!ctx) return;
    if (size != sizeof(struct InputBufferNetwork)) {
        printf("Player buffer size mismatch in received webrtc.\n");
    }
    struct InputBufferNetwork *buf = (struct InputBufferNetwork*)message;
    if (ctx->player_idx >= 0 ) {
        save_inputbuffer(server_ctx->inputBuffersMap, buf, ctx->player_idx);
    }
}
void sendPlayerData(const char *gzipped_players, int size, const ClientContext *ctx) {
    if (size > 0) {
        rtcSendMessage(ctx->dc_player, gzipped_players, size);
    }
}
void sendNewProjectilesData(const struct ProjectilePool *pool, const ClientContext *ctx) {
    if (pool && pool->length > 0) {
        rtcSendMessage(ctx->dc_projectiles, (char*)pool->array, sizeof(struct Projectile) * pool->length);
    }
}
void sendNewConesData(const struct AoEConePool *pool, const ClientContext *ctx) {
    if (pool && pool->length > 0) {
        rtcSendMessage(ctx->dc_cones, (char*)pool->array, sizeof(struct AoECone) * pool->length);
    }
}
void sendNewCirclesData(const struct AoECirclePool *pool, const ClientContext *ctx) {
    if (pool && pool->length > 0) {
        rtcSendMessage(ctx->dc_circles, (char*)pool->array, sizeof(struct AoECircle) * pool->length);
    }
}
void sendExplodingProjectilesData(const struct shortPool *exploding, const ClientContext *ctx) {
    if (exploding && exploding->length > 0) {
        printf("explode a projectile\n");
        rtcSendMessage(ctx->dc_explodingProjectiles, (char*)exploding->array, sizeof(short) * exploding->length);
    }
}
static void on_ws_open(int ws, void *ptr) {
    ClientContext *ctx = (ClientContext *)ptr;
    printf("WebSocket open. Initializing PeerConnection.\n");
    const char *ice_servers[] = {
        "stun:stun.l.google.com:19302"
    };
    rtcConfiguration config = {
        .iceServers = ice_servers,
        .iceServersCount = 0,
        .disableAutoNegotiation = false
    };
    ctx->pc = rtcCreatePeerConnection(&config);
    rtcSetUserPointer(ctx->pc, ctx);

    rtcSetLocalDescriptionCallback(ctx->pc, on_local_description);
    rtcSetLocalCandidateCallback(ctx->pc, on_local_candidate);

    ctx->dc_player = rtcCreateDataChannel(ctx->pc, "player-data");
    ctx->dc_projectiles = rtcCreateDataChannel(ctx->pc, "projectiles");
    ctx->dc_cones = rtcCreateDataChannel(ctx->pc, "cones");
    ctx->dc_circles = rtcCreateDataChannel(ctx->pc, "circles");
    ctx->dc_explodingProjectiles = rtcCreateDataChannel(ctx->pc, "exploding-projectiles");
    rtcSetUserPointer(ctx->dc_player, ctx);
    rtcSetOpenCallback(ctx->dc_player, on_dc_open);
    rtcSetMessageCallback(ctx->dc_player, on_dc_message);
}

static void on_ws_closed(int ws, void *ptr) {
    ClientContext *ctx = (ClientContext *)ptr;
    printf("Client disconnected.\n");
    if (ctx) {
        if (ctx->player_idx >= 0 && server_ctx) { //ToDo: implement timer to close player if he doesnt reconnect in 30s
            //close_player(server_ctx->players, ctx->player_idx);
            memset(&server_ctx->clients[ctx->player_idx], 0, sizeof(ClientContext));
        }
        if (ctx->dc_player) rtcDeletePeerConnection(ctx->dc_player);
        if (ctx->dc_projectiles) rtcDeletePeerConnection(ctx->dc_projectiles);
        if (ctx->dc_explodingProjectiles) rtcDeletePeerConnection(ctx->dc_explodingProjectiles);
        if (ctx->dc_cones) rtcDeletePeerConnection(ctx->dc_cones);
        if (ctx->dc_circles) rtcDeletePeerConnection(ctx->dc_circles);


        if (ctx->pc) rtcDeletePeerConnection(ctx->pc);
        free(ctx);
    }
}

//Not sure if gzip compr will be worth it
void update_player_clients(const ClientContext *clients, const struct PlayerPool *players, const struct shortPool *explodingProjectiles,struct SpellsContext *newSpells, unsigned char *gzipBuffer) {
    uLong sourceLen = sizeof(struct Player) * players->length;
    uLong destLen = compressBound(sourceLen);
    compress(gzipBuffer, &destLen, (const unsigned char*)players->array, sourceLen);
    for (int i = 0; i < MAX_PLAYERS ; i++) {
        if (clients[i].dc_player > 0) {
            sendPlayerData((char *)gzipBuffer, (int)destLen, &clients[i]);
            sendNewProjectilesData(&newSpells->projectiles, &clients[i]);
            sendNewConesData(&newSpells->cones, &clients[i]);
            sendNewCirclesData(&newSpells->circles, &clients[i]);
            sendExplodingProjectilesData(explodingProjectiles, &clients[i]);
        }
    }
}

static void on_ws_client(int server, int ws, void *ptr) {
    printf("New client connection request...\n");

    ClientContext *ctx = calloc(1, sizeof(ClientContext));
    ctx->ws = ws;
    ctx->player_idx = -1;

    rtcSetUserPointer(ws, ctx);
    rtcSetOpenCallback(ws, on_ws_open);
    rtcSetMessageCallback(ws, on_ws_message);
    rtcSetClosedCallback(ws, on_ws_closed);
}

int start_networking_server(int port, ServerContext *ctx) {
    server_ctx = ctx;
    rtcPreload();
    rtcInitLogger(RTC_LOG_INFO, NULL);

    rtcWsServerConfiguration ws_config = {
        .port = (uint16_t)port,
        .enableTls = false
    };

    int server = rtcCreateWebSocketServer(&ws_config, on_ws_client);
    if (server < 0) {
        fprintf(stderr, "Failed to start server on port %d\n", port);
    }
    return server;
}


void stop_networking_server(const int server) {
    rtcDeleteWebSocketServer(server);
}

void cleanup_networking() {
    rtcCleanup();
}
