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
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct webrtc_connection;

typedef void (*webrtc_video_callback_t)(uint8_t *buffer, size_t len, void *data);

struct webrtc_connection_config {
    uint16_t port;
    webrtc_video_callback_t video_callback;
    void *video_callback_data;
};

struct webrtc_connection* webrtc_connection_create(
    struct webrtc_connection_config *config
);

void webrtc_connection_delete(struct webrtc_connection **);

#ifdef __cplusplus
}
#endif