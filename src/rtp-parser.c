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
#include "rtp-parser.h"

#include <stdlib.h>
#include <obs.h>
#include "plugin-support.h"

static inline uint16_t get_16bit_number(uint8_t *data) {
    return (data[0] << 8) | data[1];
}

static inline uint32_t get_32bit_number(uint8_t *data) {
    return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
}

void rtp_packet_free(struct rtp_packet *packet) {
    if (packet)
    free(packet->csrc);
    free(packet);
}

void rtp_packet_debug_print(struct rtp_packet *packet) {
    obs_log(LOG_INFO, "RTP Packet %llx", packet);
    obs_log(LOG_INFO, "\tVersion %d", packet->version);
    obs_log(LOG_INFO, "\tPayload type %d", packet->payload_type);
    obs_log(LOG_INFO, "\tMarker: %s", packet->marker ? "True" : "False");
    obs_log(LOG_INFO, "\tSSRC: %08x", packet->ssrc);
    for (int i = 0; i < packet->csrc_count; i++) {
        obs_log(LOG_INFO, "\tCSRC: %08x", packet->csrc[i]);
    }
    obs_log(LOG_INFO, "\tPayload size: %d", packet->payload_size);
}

struct rtp_packet* rtp_packet_parse(uint8_t *data, size_t len) {
    struct rtp_packet *packet = NULL;
    uint8_t *current = data;

    if (len < 12) {
        // An RTP header must be at least 12 bytes
        return NULL;
    }

    packet = calloc(sizeof(struct rtp_packet), 1);

    packet->version = data[0] >> 6;
    packet->csrc_count = data[0] & 0xf;

    int padding = (data[0] >> 5) & 1;
    int extension = (data[0] >> 4) & 1;

    packet->payload_type = data[1] & 0x7f;
    packet->marker = data[1] >> 7;

    packet->sequence_number = ((uint16_t) data[2] << 8) | data[3];

    packet->timestamp = get_32bit_number(&data[4]);
    packet->ssrc = get_32bit_number(&data[8]);

    current = data + 12;

    if (packet->csrc_count > 0) {
        // Check that the header is long enough to contain the CSRCs
        if (len < 12 + (size_t) packet->csrc_count * 4) {
            goto error;
        }

        packet->csrc = malloc(packet->csrc_count * sizeof(uint32_t));
        for (int i = 0; i < packet->csrc_count; i++) {
            packet->csrc[i] = get_32bit_number(current);
            current += 4;
        }
    }

    if (extension) {
        // Check that the header can contain an extension header header
        if (len < 12 + (size_t) packet->csrc_count*4 + 4) {
            goto error;
        }

        uint16_t ext_header_len = get_16bit_number(current + 2);
        current += ext_header_len;
    }

    packet->payload = current;
    packet->payload_size = len - (current - data);

    if (padding) {
        uint8_t padding_size = data[len];

        if (padding_size > packet->payload_size) {
            goto error;
        }

        packet->payload_size -= padding_size;
    }

    return packet;

error:
    if (packet) {
        if (packet->csrc) {
            free(packet->csrc);
        }
        free(packet);
    }
    return NULL;
}