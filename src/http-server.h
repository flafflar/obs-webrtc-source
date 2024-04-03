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

struct http_server;

/**
 * Creates and starts an HTTP server.
 *
 * @param port The port to listen on.
 */
struct http_server* http_server_create(int port);

/**
 * Stops and destroys the HTTP server.
 */
void http_server_destroy(struct http_server **server_ptr);

/**
 * Set the port that the WebSocket server listens on, so it gets sent to the
 * clients.
 */
void http_server_set_ws_port(struct http_server *server, int port);