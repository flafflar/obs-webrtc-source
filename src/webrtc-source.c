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

struct webrtc_source {
    obs_source_t *source;
};


void* webrtc_source_create(obs_data_t *settings, obs_source_t *source) {
    UNUSED_PARAMETER(settings);

    struct webrtc_source *src = bzalloc(sizeof(struct webrtc_source));
    src->source = source;

    return src;
}

void webrtc_source_destroy(void *data) {
    struct webrtc_source *src = data;

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
    .output_flags = OBS_SOURCE_VIDEO,
    .get_name = webrtc_source_name,
    .create = webrtc_source_create,
    .destroy = webrtc_source_destroy,
    .video_render = webrtc_source_render,
    .get_width = webrtc_source_width,
    .get_height = webrtc_source_height,
};