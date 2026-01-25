#include "networking.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    rtcCreateDataChannel(ctx->pc, "game-data");
}

static void on_ws_closed(int ws, void *ptr) {
    ClientContext *ctx = (ClientContext *)ptr;
    printf("Client disconnected.\n");
    if (ctx) {
        if (ctx->pc) rtcDeletePeerConnection(ctx->pc);
        free(ctx);
    }
}

static void on_ws_client(int server, int ws, void *ptr) {
    printf("New client connection request...\n");

    ClientContext *ctx = calloc(1, sizeof(ClientContext));
    ctx->ws = ws;

    rtcSetUserPointer(ws, ctx);
    rtcSetOpenCallback(ws, on_ws_open);
    rtcSetMessageCallback(ws, on_ws_message);
    rtcSetClosedCallback(ws, on_ws_closed);
}

int start_networking_server(int port) {
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
