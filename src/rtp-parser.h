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

struct rtp_packet {
    uint8_t version;
    uint8_t payload_type;
    uint16_t sequence_number;
    uint8_t marker;
    uint32_t timestamp;
    uint32_t ssrc;
    uint32_t *csrc;
    uint8_t csrc_count;

    uint8_t *payload;
    size_t payload_size;
};

void rtp_packet_free(struct rtp_packet *packet);

void rtp_packet_debug_print(struct rtp_packet *packet);

struct rtp_packet* rtp_packet_parse(uint8_t *data, size_t len);