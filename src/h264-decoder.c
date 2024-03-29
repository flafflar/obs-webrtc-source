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
#include "h264-decoder.h"

#include <obs.h>
#include "plugin-support.h"

struct h264_decoder {
    const AVCodec *codec;
    AVCodecParserContext *parser;
    AVCodecContext *ctx;

    AVPacket *pkt;

    uint8_t *buffer;
    size_t buffer_size;
};

struct h264_decoder* h264_decoder_create() {
    struct h264_decoder decoder = {};

    decoder.codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!decoder.codec) {
        obs_log(LOG_ERROR, "libavcodec: Could not find H.264 codec");
        return NULL;
    }

    decoder.parser = av_parser_init(decoder.codec->id);
    if (!decoder.parser) {
        obs_log(LOG_ERROR, "libavcodec: Could not create H.264 parser");
        return NULL;
    }

    decoder.ctx = avcodec_alloc_context3(decoder.codec);
    if (!decoder.ctx) {
        obs_log(LOG_ERROR, "libavcodec: Could not allocate video context");
        return NULL;
    }

    if (avcodec_open2(decoder.ctx, decoder.codec, NULL) < 0) {
        obs_log(LOG_ERROR, "libavcodec: Could not open H.264 codec");
        return NULL;
    }

    // Clone the struct to the heap before returning
    return bmemdup(&decoder, sizeof(decoder));
}

void h264_decoder_destroy(struct h264_decoder **decoder) {
    av_parser_close((*decoder)->parser);

    avcodec_free_context(&(*decoder)->ctx);

    bfree(*decoder);
    *decoder = NULL;
}

void rtp_process_h264_packet(
    struct h264_decoder *decoder,
    struct rtp_packet *packet
) {
    int ret;
    uint8_t *buffer = malloc(0);
    size_t bufsize = 0;

    const uint8_t nal_prefix[3] = {0, 0, 1};

    uint8_t fragment_type = packet->payload[0] & 0b11111;
    switch (fragment_type) {
        case 1: { // Single NAL
            buffer = malloc(3 + packet->payload_size);
            bufsize = 3 + packet->payload_size;

            memcpy(buffer, nal_prefix, 3);
            memcpy(buffer + 3, packet->payload, packet->payload_size);
        } break;

        case 24: { // STAP-A
            uint8_t *nalu = packet->payload + 1;
            while (nalu < packet->payload + packet->payload_size) {
                uint16_t nalu_size = (nalu[0] << 8) | nalu[1];
                nalu += 2;

                buffer = realloc(buffer, bufsize + 3 + nalu_size);
                memcpy(buffer + bufsize, nal_prefix, 3);
                memcpy(buffer + bufsize + 3, nalu, nalu_size);
                bufsize += 3 + nalu_size;

                nalu += nalu_size;
            }
        } break;

        case 28: { // FU-A
            uint8_t start_bit = packet->payload[1] >> 7;
            uint8_t nal_unit = (packet->payload[0] & 0b11100000) | (packet->payload[1] & 0b11111);

            if (start_bit) {
                buffer = malloc(4);
                bufsize = 4;
                memcpy(buffer, nal_prefix, 3);
                buffer[3] = nal_unit;
            }

            buffer = realloc(buffer, bufsize + packet->payload_size - 2);
            memcpy(buffer + bufsize, packet->payload + 2, packet->payload_size - 2);
            bufsize += packet->payload_size - 2;
        } break;
    }

    AVPacket *pkt = av_packet_alloc();

    size_t parsed = 0;
    while (parsed < bufsize) {
        ret = av_parser_parse2(
            decoder->parser,
            decoder->ctx,
            &pkt->data,
            &pkt->size,
            buffer + parsed,
            bufsize - parsed,
            AV_NOPTS_VALUE,
            AV_NOPTS_VALUE,
            0
        );

        if (ret < 0) {
            obs_log(LOG_ERROR, "Parsing error");
            exit(1);
        }

        parsed += ret;

        if (pkt->size > 0) {
            ret = avcodec_send_packet(decoder->ctx, pkt);
            if (ret < 0) {
                obs_log(LOG_ERROR, "Sending packet error");
                exit(1);
            }
        }
    }

    free(buffer);
}

AVFrame* h264_decoder_get_frame(struct h264_decoder *decoder) {
    AVFrame *frame = av_frame_alloc();

    int ret = avcodec_receive_frame(decoder->ctx, frame);
    if (ret == AVERROR(EAGAIN)) {
        return NULL;
    } else if (ret < 0) {
        obs_log(LOG_ERROR, "Error receiving frame");
        exit(1);
    }

    return frame;
}
