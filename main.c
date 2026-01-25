#include <rtc/rtc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define TICK_RATE_MS 33
#define NS_PER_MS 1000000
#define NS_PER_SEC 1000000000

typedef struct {
    int ws;
    int pc;
} ClientContext;

// --- WebRTC Callbacks ---

void on_local_description(int pc, const char *sdp, const char *type, void *ptr) {
    ClientContext *ctx = (ClientContext *)ptr;
    if (rtcIsOpen(ctx->ws)) {
        printf("Sending SDP %s via WS\n", type);
        rtcSendMessage(ctx->ws, sdp, -1);
    }
}

void on_local_candidate(int pc, const char *cand, const char *mid, void *ptr) {
    ClientContext *ctx = (ClientContext *)ptr;
    if (rtcIsOpen(ctx->ws)) {
        rtcSendMessage(ctx->ws, cand, -1);
    }
}

void on_ws_message(int ws, const char *message, int size, void *ptr) {
    ClientContext *ctx = (ClientContext *)ptr;
    if (!ctx || ctx->pc == 0) return;

    if (strstr(message, "v=0") != NULL) {
        rtcSetRemoteDescription(ctx->pc, message, NULL);
    } else {
        rtcAddRemoteCandidate(ctx->pc, message, NULL);
    }
}

// --- WebSocket Callbacks ---

void on_ws_open(int ws, void *ptr) {
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

    // Creating DataChannel triggers the initial Offer
    rtcCreateDataChannel(ctx->pc, "game-data");
}

void on_ws_closed(int ws, void *ptr) {
    ClientContext *ctx = (ClientContext *)ptr;
    printf("Client disconnected.\n");
    if (ctx) {
        if (ctx->pc) rtcDeletePeerConnection(ctx->pc);
        free(ctx);
    }
}

void on_ws_client(int server, int ws, void *ptr) {
    printf("New client connection request...\n");

    ClientContext *ctx = calloc(1, sizeof(ClientContext));
    ctx->ws = ws;

    rtcSetUserPointer(ws, ctx);
    rtcSetOpenCallback(ws, on_ws_open);
    rtcSetMessageCallback(ws, on_ws_message);
    rtcSetClosedCallback(ws, on_ws_closed);
}

// --- Game Loop ---

void game_loop() {
    struct timespec ts;
    uint64_t next_tick;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    next_tick = (uint64_t)ts.tv_sec * NS_PER_SEC + ts.tv_nsec;

    printf("Game loop started at 33ms tickrate.\n");

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
    rtcInitLogger(RTC_LOG_INFO, NULL);

    rtcWsServerConfiguration ws_config = {
        .port = 8080,
        .enableTls = false
    };

    int server = rtcCreateWebSocketServer(&ws_config, on_ws_client);
    if (server < 0) {
        fprintf(stderr, "Failed to start server on port 8080\n");
        return EXIT_FAILURE;
    }

    printf("Signaling server listening on port 8080...\n");

    game_loop();

    rtcDeleteWebSocketServer(server);
    rtcCleanup();
    return EXIT_SUCCESS;
}