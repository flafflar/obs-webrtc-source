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
 */
struct http_server* http_server_create(void);

/**
 * Stops and destroys the HTTP server.
 */
void http_server_destroy(struct http_server **server_ptr);