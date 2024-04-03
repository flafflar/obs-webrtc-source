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

#include <obs-module.h>
#include "plugin-support.h"

#define READ_BUFFER_SIZE 4096

struct http_server {
    int socket_fd;
    pthread_t listen_thread;
    char *html;
    char *ws_port;
};

char* http_server_read_html() {
    char *path = obs_module_file("client/index.html");

    FILE *file = fopen(path, "r");

    char *data = bmalloc(READ_BUFFER_SIZE * sizeof(char));
    size_t allocated = READ_BUFFER_SIZE;
    int len = 0;
    int chars_read;
    do {
        chars_read = fread(
            data + allocated - READ_BUFFER_SIZE,
            sizeof(char),
            READ_BUFFER_SIZE,
            file
        );

        data = brealloc(data, allocated + READ_BUFFER_SIZE);
        allocated += READ_BUFFER_SIZE;

        len += chars_read;
    } while (chars_read > 0);

    char *str = bstrdup_n(data, len);
    return str;
}

static void get_path(char *header, int len, char *path) {
    int pathstart = -1;
    int pathlen = 0;
    for (int i = 0; i < len; i++) {
        if (header[i] == ' ') {
            if (pathstart == -1) {
                pathstart = i + 1;
                continue;
            } else {
                pathlen = pathstart - i + 1;
                break;
            }
        }

        if (pathstart != -1) {
            path[i - pathstart] = header[i];
        }
    }

    path[pathlen] = '\0';
}

const char http_header[] = "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n";

void* http_server_loop(void *arg) {
    struct http_server *server = arg;

    while (1) {
        struct sockaddr_in client_address;
        socklen_t client_address_len = sizeof(client_address);

        int client_fd = accept(
            server->socket_fd,
            (struct sockaddr *) &client_address,
            &client_address_len
        );

        if (client_fd < 0) {
            obs_log(LOG_ERROR, "accept failed: %s", strerror(errno));
            continue;
        }

        char header[1024];
        int bytes_recvd = recv(client_fd, header, 1024, 0);
        if (bytes_recvd < 0) {
            obs_log(LOG_ERROR, "recv failed: %s", strerror(errno));
        }

        char path[1024];
        get_path(header, bytes_recvd, path);

        send(client_fd, http_header, (sizeof(http_header) - 1)/sizeof(char), 0);
        if (strcmp(path, "/ws-port") == 0) {
            send(client_fd, server->ws_port, strlen(server->ws_port)*sizeof(char), 0);
        } else {
            send(client_fd, server->html, strlen(server->html)*sizeof(char), 0);
        }

        shutdown(client_fd, SHUT_RDWR);
    }

    return NULL;
}

struct http_server* http_server_create(int port) {
    struct http_server *server = malloc(sizeof(struct http_server));

    server->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->socket_fd < 0) {
        obs_log(LOG_ERROR, "socket failed: %s", strerror(errno));
        return NULL;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    setsockopt(server->socket_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    setsockopt(server->socket_fd, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int));

    if (bind(server->socket_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        obs_log(LOG_ERROR, "bind failed: %s", strerror(errno));
        return NULL;
    }

    if (listen(server->socket_fd, 2) < 0) {
        obs_log(LOG_ERROR, "listen failed: %s", strerror(errno));
        return NULL;
    }

    server->html = http_server_read_html();

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

    bfree(server->html);
    free(server);

    *server_ptr = NULL;
}

void http_server_set_ws_port(struct http_server *server, int port) {
    server->ws_port = malloc(6 * sizeof(char));
    snprintf(server->ws_port, 6, "%d", port);
}