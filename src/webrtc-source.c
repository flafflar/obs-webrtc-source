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
#include "h264-decoder.h"

struct webrtc_source {
    obs_source_t *source;
    struct webrtc_connection *webrtc_conn;
    struct h264_decoder *decoder;
};

void webrtc_video_callback(uint8_t *buffer, size_t len, void *data) {
    struct webrtc_source *src = data;

    struct rtp_packet *packet = rtp_packet_parse(buffer, len);

    rtp_process_h264_packet(src->decoder, packet);

    AVFrame *f = h264_decoder_get_frame(src->decoder);
    if (!f) return;

    uint32_t pixels[400 * 300];

    struct obs_source_frame frame;
    obs_source_frame_init(&frame, VIDEO_FORMAT_I420, f->width, f->height);

    video_format_get_parameters_for_format(
        VIDEO_CS_DEFAULT,
        VIDEO_RANGE_DEFAULT,
        VIDEO_FORMAT_I420,
        frame.color_matrix,
        frame.color_range_min,
        frame.color_range_max
    );

    for (int line = 0; line < f->height; line++) {
        memcpy(
		frame.data[0] + line*f->linesize[0],
		f->data[0] + line*f->linesize[0],
		f->linesize[0]
	);
    }

    for (int line = 0; line < f->height / 2; line++) {
        memcpy(
                frame.data[1] + line*f->linesize[1],
                f->data[1] + line*f->linesize[1],
                f->linesize[1]
	);
        memcpy(
                frame.data[2] + line*f->linesize[2],
                f->data[2] + line*f->linesize[2],
                f->linesize[2]
	);
    }

    for (int y = 0; y < 300; y++) {
        for (int x = 0; x < 400; x++) {
            pixels[y + x * 300] = 0xff00ff00;
        }
    }

    obs_source_output_video(src->source, &frame);
    rtp_packet_free(packet);
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

    src->decoder = h264_decoder_create();

    return src;
}

void webrtc_source_destroy(void *data) {
    struct webrtc_source *src = data;

    webrtc_connection_delete(&src->webrtc_conn);

    h264_decoder_destroy(&src->decoder);

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
