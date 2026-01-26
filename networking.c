#include "networking.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"

static ServerContext *server_ctx = NULL;

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

    if (strstr(message, "v=0") != NULL) {
        rtcSetRemoteDescription(ctx->pc, message, NULL);
    } else {
        rtcAddRemoteCandidate(ctx->pc, message, NULL);
    }
}

static void on_dc_open(int dc, void *ptr) {
    ClientContext *ctx = (ClientContext *)ptr;
    printf("DataChannel open.\n");
    if (!server_ctx) return;

    int idx = spawn_player(server_ctx->players);
    if (idx >= 0) {
        ctx->player_idx = idx;
        server_ctx->clients[idx] = *ctx;
        printf("Player spawned at index %d\n", idx);
    } else {
        printf("Failed to spawn player: server full\n");
    }
}
static void on_dc_message(int ws, const char *message, int size, void *ptr) {
    ClientContext *ctx = (ClientContext *)ptr;
    if (!ctx) return;
    if (size != sizeof(struct inputBuffer)) {
        printf("Player buffer size mismatch in received webrtc.\n");
    }
    struct inputBuffer *buf = (struct inputBuffer*)message;
    if (ctx->player_idx >= 0 ) {
        input_buffer_player(server_ctx->inputBuffers, buf, ctx->player_idx);
    }
}
static void on_ws_open(int ws, void *ptr) {
    ClientContext *ctx = (ClientContext *)ptr;
    printf("WebSocket open. Initializing PeerConnection.\n");

    rtcConfiguration config = {
        .iceServers = NULL,
        .iceServersCount = 0,
        .disableAutoNegotiation = false
    };

    ctx->pc = rtcCreatePeerConnection(&config);
    rtcSetUserPointer(ctx->pc, ctx);

    rtcSetLocalDescriptionCallback(ctx->pc, on_local_description);
    rtcSetLocalCandidateCallback(ctx->pc, on_local_candidate);

    ctx->dc = rtcCreateDataChannel(ctx->pc, "game-data");
    rtcSetUserPointer(ctx->dc, ctx);
    rtcSetOpenCallback(ctx->dc, on_dc_open);
    rtcSetMessageCallback(ctx->dc, on_dc_message);
}

static void on_ws_closed(int ws, void *ptr) {
    ClientContext *ctx = (ClientContext *)ptr;
    printf("Client disconnected.\n");
    if (ctx) {
        if (ctx->player_idx >= 0 && server_ctx) {
            close_player(server_ctx->players, ctx->player_idx);
            memset(&server_ctx->clients[ctx->player_idx], 0, sizeof(ClientContext));
        }
        if (ctx->dc) rtcDeletePeerConnection(ctx->dc);
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
