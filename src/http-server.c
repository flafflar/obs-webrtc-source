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

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "mongoose.h"

#include <obs-module.h>
#include "plugin-support.h"

struct http_server {
    struct mg_mgr mgr;
    pthread_t listen_thread;
    char *ws_port;
};

static void http_server_handler(
    struct mg_connection *conn,
    int ev,
    void *data
) {
    struct http_server *server = (struct http_server *) conn->mgr->userdata;

    if (ev == MG_EV_ACCEPT) {
        struct mg_tls_opts opts = {
            .cert = mg_file_read(&mg_fs_posix, obs_module_file("cert.pem")),
            .key = mg_file_read(&mg_fs_posix, obs_module_file("key.pem")),
        };
        mg_tls_init(conn, &opts);

    } else if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *http_msg = (struct mg_http_message *) data;
        struct mg_http_serve_opts opts = {};

        if (mg_http_match_uri(http_msg, "/ws-port")) {
            mg_http_reply(
                conn,
                200,
                "Content-Type: text/plain\r\n",
                server->ws_port
            );
        } else {
            mg_http_serve_file(
                conn,
                http_msg,
                obs_module_file("client/index.html"),
                &opts
            );
        }
    }
}

const char http_header[] = "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n";

void* http_server_loop(void *arg) {
    struct http_server *server = arg;

    while (1) {
        mg_mgr_poll(&server->mgr, 1000);
    }

    return NULL;
}

struct http_server* http_server_create(int port) {
    struct http_server *server = malloc(sizeof(struct http_server));

    mg_mgr_init(&server->mgr);
    server->mgr.userdata = server;

    char addr[32];
    snprintf(addr, 32, "https://0.0.0.0:%d", port);
    mg_http_listen(&server->mgr, addr, http_server_handler, NULL);

    server->ws_port = "-1";

    int result = pthread_create(
        &server->listen_thread,
        NULL,
        http_server_loop,
        server
    );

    if (result != 0) {
        obs_log(LOG_ERROR, "pthread_create failed: %s", strerror(result));
        return NULL;
    }

    return server;
}

void http_server_destroy(struct http_server **server_ptr) {
    struct http_server *server = *server_ptr;

    // Stop the listen thread
    pthread_cancel(server->listen_thread);

    free(server);

    *server_ptr = NULL;
}

void http_server_set_ws_port(struct http_server *server, int port) {
    server->ws_port = malloc(6 * sizeof(char));
    snprintf(server->ws_port, 6, "%d", port);
}