#include "networking.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    struct authenticatedPlayer *player = find_player(ctx->bearer_token,&server_ctx->authenticatedPlayers);
    if (!player) {
        int idx = spawn_player(server_ctx->players);
        if (idx >= 0) {
            ctx->player_idx = idx;
            server_ctx->clients[idx] = *ctx;
            add_player(ctx->player_idx,ctx->bearer_token, &server_ctx->authenticatedPlayers);
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
    if (size != sizeof(struct InputBuffer)) {
        printf("Player buffer size mismatch in received webrtc.\n");
    }
    struct InputBuffer *buf = (struct InputBuffer*)message;
    if (ctx->player_idx >= 0 ) {
        input_buffer_player(server_ctx->inputBuffers, buf, ctx->player_idx);
    }
}
void sendPlayerData(const struct Player *players, int count, const ClientContext *ctx) {
    rtcSendMessage(ctx->dc_player, (char*)players, sizeof(struct Player) * count);
}
void sendNewProjectilesData(const struct ProjectilePool *projectilePool, const ClientContext *ctx) {
    rtcSendMessage(ctx->dc_projectiles, (char*)projectilePool->array, sizeof(struct Projectile) * projectilePool->length);
}
void sendExplodingProjectilesData(const struct intPool *exploding, const ClientContext *ctx) {
    rtcSendMessage(ctx->dc_explodingProjectiles, (char*)exploding->array, sizeof(short) * exploding->length);
}
static void on_ws_open(int ws, void *ptr) {
    ClientContext *ctx = (ClientContext *)ptr;
    printf("WebSocket open. Initializing PeerConnection.\n");
    const char *ice_servers[] = {
        "stun:stun.l.google.com:19302"
    };
    rtcConfiguration config = {
        .iceServers = ice_servers,
        .iceServersCount = 1,
        .bindAddress = "127.0.0.1",
        .disableAutoNegotiation = false
    };
    ctx->pc = rtcCreatePeerConnection(&config);
    rtcSetUserPointer(ctx->pc, ctx);

    rtcSetLocalDescriptionCallback(ctx->pc, on_local_description);
    rtcSetLocalCandidateCallback(ctx->pc, on_local_candidate);

    ctx->dc_player = rtcCreateDataChannel(ctx->pc, "player-data");
    ctx->dc_projectiles = rtcCreateDataChannel(ctx->pc, "projectiles");
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
        if (ctx->pc) rtcDeletePeerConnection(ctx->pc);
        free(ctx);
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


void stop_networking_server(int server) {
    rtcDeleteWebSocketServer(server);
}

void cleanup_networking() {
    rtcCleanup();
}
