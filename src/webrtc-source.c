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

#include <obs-module.h>
#include <util/platform.h>
#include "plugin-support.h"

#include "webrtc.h"
#include "rtp-parser.h"

struct webrtc_source {
    obs_source_t *source;
    struct webrtc_connection *webrtc_conn;
};

void webrtc_video_callback(uint8_t *buffer, size_t len, void *data) {
    obs_log(LOG_INFO, "Received video callback");
    struct webrtc_source *src = data;
    UNUSED_PARAMETER(buffer);
    UNUSED_PARAMETER(len);

    struct rtp_packet *packet = rtp_packet_parse(buffer, len);
    rtp_packet_debug_print(packet);
    rtp_packet_free(packet);

    uint32_t pixels[400 * 300];

    struct obs_source_frame frame = {
        .data = {[0] = (uint8_t *) pixels },
        .linesize = {[0] = 400 * 4},
        .width = 400,
        .height = 300,
        .format = VIDEO_FORMAT_BGRX,
        .timestamp = os_gettime_ns(),
    };

    for (int y = 0; y < 300; y++) {
        for (int x = 0; x < 400; x++) {
            pixels[y + x * 300] = 0xff00ff00;
        }
    }

    obs_source_output_video(src->source, &frame);
}

void* webrtc_source_create(obs_data_t *settings, obs_source_t *source) {
    UNUSED_PARAMETER(settings);

    struct webrtc_source *src = bzalloc(sizeof(struct webrtc_source));
    src->source = source;

    struct webrtc_connection_config webrtc_conf = {
        .port = 3081,
        .video_callback = webrtc_video_callback,
        .video_callback_data = src,
    };

    src->webrtc_conn = webrtc_connection_create(&webrtc_conf);

    return src;
}

void webrtc_source_destroy(void *data) {
    struct webrtc_source *src = data;

    webrtc_connection_delete(&src->webrtc_conn);

    bfree(src);
}

const char* webrtc_source_name(void *data) {
    UNUSED_PARAMETER(data);
    return "WebRTC Source";
}

uint32_t webrtc_source_width(void *data) {
    UNUSED_PARAMETER(data);
    return 400;
}

uint32_t webrtc_source_height(void *data) {
    UNUSED_PARAMETER(data);
    return 300;
}

void webrtc_source_render(void *data, gs_effect_t *effect) {
    UNUSED_PARAMETER(data);
    UNUSED_PARAMETER(effect);
}

struct obs_source_info webrtc_source = {
    .id = "webrtc_source",
    .type = OBS_SOURCE_TYPE_INPUT,
    .output_flags = OBS_SOURCE_ASYNC_VIDEO,
    .get_name = webrtc_source_name,
    .create = webrtc_source_create,
    .destroy = webrtc_source_destroy,
};