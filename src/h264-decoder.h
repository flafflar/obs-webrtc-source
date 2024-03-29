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
#pragma once

#include <stddef.h>
#include <stdint.h>

#include <libavcodec/avcodec.h>

#include "rtp-parser.h"

struct h264_decoder;

struct h264_decoder* h264_decoder_create();

void h264_decoder_destroy(struct h264_decoder **decoder);

void rtp_process_h264_packet(
    struct h264_decoder *decoder,
    struct rtp_packet *packet
);

AVFrame* h264_decoder_get_frame(struct h264_decoder *decoder);
