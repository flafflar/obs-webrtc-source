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

#include "http-server.h"
#include "webrtc.h"
#include "rtp-parser.h"
#include "h264-decoder.h"

struct webrtc_source {
    obs_source_t *source;
    obs_data_t *settings;
    struct http_server *http_server;
    struct webrtc_connection *webrtc_conn;
    struct h264_decoder *decoder;
};

void webrtc_video_callback(uint8_t *buffer, size_t len, void *data) {
    struct webrtc_source *src = data;

    struct rtp_packet *packet = rtp_packet_parse(buffer, len);

    rtp_process_h264_packet(src->decoder, packet);

    AVFrame *f = h264_decoder_get_frame(src->decoder);
    if (!f) return;

    struct obs_source_frame frame = {
        .data = {
            [0] = f->data[0],
            [1] = f->data[1],
            [2] = f->data[2],
        },
        .linesize = {
            [0] = f->linesize[0],
            [1] = f->linesize[1],
            [2] = f->linesize[2],
        },
        .width = f->width,
        .height = f->height,
        .format = VIDEO_FORMAT_I420,
    };

    video_format_get_parameters_for_format(
        VIDEO_CS_DEFAULT,
        VIDEO_RANGE_DEFAULT,
        VIDEO_FORMAT_I420,
        frame.color_matrix,
        frame.color_range_min,
        frame.color_range_max
    );

    obs_source_output_video(src->source, &frame);

    av_frame_free(&f);
    rtp_packet_free(packet);
}

void* webrtc_source_create(obs_data_t *settings, obs_source_t *source) {
    obs_data_set_default_int(settings, "http_server_port", 3080);
    obs_data_set_default_int(settings, "websocket_server_port", 3081);

    struct webrtc_source *src = bzalloc(sizeof(struct webrtc_source));
    src->source = source;
    src->settings = settings;

    src->decoder = h264_decoder_create();

    return src;
}

static void webrtc_source_stop_http_server(struct webrtc_source *src) {
    if (src->http_server) {
        obs_log(LOG_INFO, "Stopping HTTP server");
        http_server_destroy(&src->http_server);
    }
}

static void webrtc_source_stop_ws_server(struct webrtc_source *src) {
    if (src->webrtc_conn) {
        obs_log(LOG_INFO, "Stopping WebSocket server");
        webrtc_connection_delete(&src->webrtc_conn);
    }
}

static bool webrtc_source_start_http_server(
    struct webrtc_source *src,
    int port
) {
    if (src->http_server) {
        webrtc_source_stop_http_server(src);
    }

    obs_log(LOG_INFO, "Starting HTTP server");
    src->http_server = http_server_create(port);

    if (!src->http_server) {
        obs_log(LOG_ERROR, "HTTP server could not be created");
        return false;
    }

    return true;
}

static bool webrtc_source_start_ws_server(struct webrtc_source *src, int port) {
    if (src->webrtc_conn) {
        webrtc_connection_delete(&src->webrtc_conn);
    }

    obs_log(LOG_INFO, "Starting WebSocket server");
    struct webrtc_connection_config webrtc_conf = {
        .port = port,
        .video_callback = webrtc_video_callback,
        .video_callback_data = src,
    };
    src->webrtc_conn = webrtc_connection_create(&webrtc_conf);

    if (!src->webrtc_conn) {
        obs_log(LOG_ERROR, "WebSocket server could not be started");
        return false;
    }

    return true;
}

bool webrtc_source_start_servers(
    obs_properties_t *props,
    obs_property_t *property,
    void *data
) {
    struct webrtc_source *src = data;
    char error_desc[1024];

    obs_property_t *error_text = obs_properties_get(props, "error_text");
    obs_property_set_visible(error_text, false);

    int http_port = obs_data_get_int(src->settings, "http_server_port");
    if (!webrtc_source_start_http_server(src, http_port)) {
        snprintf(
            error_desc, 1024,
            "Error while starting HTTP server: %s", strerror(errno)
        );

        obs_property_set_description(error_text, error_desc);
        obs_property_text_set_info_type(error_text, OBS_TEXT_INFO_ERROR);
        obs_property_set_visible(error_text, true);
        return true;
    }

    int ws_port = obs_data_get_int(src->settings, "websocket_server_port");
    if (!webrtc_source_start_ws_server(src, ws_port)) {
        webrtc_source_stop_http_server(src);

        snprintf(
            error_desc, 1024,
            "Error while starting WebSocket server: %s", strerror(errno)
        );

        obs_property_set_description(error_text, error_desc);
        obs_property_text_set_info_type(error_text, OBS_TEXT_INFO_ERROR);
        obs_property_set_visible(error_text, true);
        return true;
    }

    // Hide the "Start Servers" button
    obs_property_set_visible(property, false);
    // Show the "Stop Servers" button
    obs_property_set_visible(
        obs_properties_get(props, "stop_servers_button"),
        true
    );

    return true;
}

bool webrtc_source_stop_servers(
    obs_properties_t *props,
    obs_property_t *property,
    void *data
) {
    struct webrtc_source *src = data;

    webrtc_source_stop_http_server(src);

    webrtc_source_stop_ws_server(src);

    // Hide the "Stop Servers" button
    obs_property_set_visible(property, false);
    // Show the "Start Servers" button
    obs_property_set_visible(
        obs_properties_get(props, "start_servers_button"),
        true
    );

    return true;
}

obs_properties_t* webrtc_source_get_properties(void *data) {
    struct webrtc_source *src = data;
    UNUSED_PARAMETER(src);

    obs_properties_t *props = obs_properties_create();

    obs_properties_add_int(props,
        "http_server_port",
        "HTTP server port",
        1024, 65535, 1
    );

    obs_properties_add_int(props,
        "websocket_server_port",
        "WebSocket server port",
        1024, 65535, 1
    );

    obs_property_t *start_servers_button = obs_properties_add_button2(props,
        "start_servers_button",
        "Start servers",
        webrtc_source_start_servers,
        src
    );

    obs_property_t *stop_servers_button = obs_properties_add_button2(props,
        "stop_servers_button",
        "Stop servers",
        webrtc_source_stop_servers,
        src
    );

    if (src->http_server) {
        obs_property_set_visible(start_servers_button, false);
        obs_property_set_visible(stop_servers_button, true);
    } else {
        obs_property_set_visible(start_servers_button, true);
        obs_property_set_visible(stop_servers_button, false);
    }

    obs_property_t *error_text = obs_properties_add_text(props,
        "error_text",
        "",
        OBS_TEXT_INFO
    );
    obs_property_set_visible(error_text, false);

    return props;
}

void webrtc_source_destroy(void *data) {
    struct webrtc_source *src = data;

    webrtc_source_stop_http_server(src);

    webrtc_source_stop_ws_server(src);

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
    .get_properties = webrtc_source_get_properties,
    .create = webrtc_source_create,
    .destroy = webrtc_source_destroy,
};
