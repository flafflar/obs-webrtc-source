/*
OBS WebRTC Source
Copyright (C) 2024 Achilleas Michailidis <achmichail@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/
#include "websocket-server.h"

#include <pthread.h>
#include <libwebsockets.h>

#include <obs-module.h>
#include "plugin-support.h"

struct websocket_server {
    struct lws_context *context;
    pthread_t thread_id;
};

static int websocket_callback(
    struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len
) {
    UNUSED_PARAMETER(wsi);
    UNUSED_PARAMETER(user);
    UNUSED_PARAMETER(len);

    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:
            obs_log(LOG_INFO, "WebSocket connection established");
            break;

        case LWS_CALLBACK_RECEIVE:
            obs_log(LOG_INFO, "Received %d %s", len, (char *) in);
            lws_write(wsi, in, len, LWS_WRITE_TEXT);
            break;

        case LWS_CALLBACK_CLOSED:
            obs_log(LOG_INFO, "WebSocket connection closed");
    }

    return 0;
}

void* websocket_server_loop(void *arg) {
    struct websocket_server *server = arg;

     while (1) {
        lws_service(server->context, 50);
    }

    lws_context_destroy(server->context);
}

struct websocket_server* websocket_server_create(void) {
    struct websocket_server *server = malloc(sizeof(struct websocket_server));

    struct lws_protocols protocol = {
        .name = "echo",
        .callback = websocket_callback,
        .per_session_data_size = 0,
        .rx_buffer_size = 0,
    };

    struct lws_context_creation_info info = {
        .port = 3081,
        .protocols = &protocol,
        .uid = -1,
        .gid = -1,
        .options = 0,
    };

    server->context = lws_create_context(&info);

    if (server->context == NULL) {
        obs_log(LOG_ERROR, "lws_create_context failed");
        return NULL;
    }

    int result = pthread_create(
        &server->thread_id,
        NULL,
        websocket_server_loop,
        server
    );

    if (result != 0) {
        obs_log(LOG_ERROR, "Could not create thread for WebSocket server: %s",
            strerror(-result));
        return NULL;
    }

    return server;
}

void websocket_server_destroy(struct websocket_server **pserver) {
    struct websocket_server *server = *pserver;

    pthread_cancel(server->thread_id);

    lws_context_destroy(server->context);

    free(server);
}