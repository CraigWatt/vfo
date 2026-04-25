/*
 * Copyright (c) 2022 Craig Watt
 *
 * Contact: craig@webrefine.co.uk
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef M_INTERNAL_H
#define M_INTERNAL_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <dirent.h>
#include <ftw.h>
#include <unistd.h>

#include "../mezzanine.h"
#include "../utils.h"
#include "Data_Structures/m_mezzanine_struct.h"

void o_pre_move_checks(original_t *original);

void o_move_from_start_to_mkv_original(original_t *original);
void o_move_from_start_to_mkv_original_webm(original_t *original);
void o_move_from_start_to_mp4_original(original_t *original);
void o_move_from_start_to_mp4_original_mov(original_t *original);
void o_move_from_start_to_mp4_original_avi(original_t *original);
void o_move_from_start_to_mp4_original_mxf(original_t *original);
void o_move_from_start_to_m2ts_original(original_t *original);
void o_move_from_start_to_m2ts_original_mts(original_t *original);
void o_move_from_start_to_ts_original(original_t *original);
void o_move_from_start_to_ts_original_mpg(original_t *original);
void o_move_from_start_to_ts_original_mpeg(original_t *original);
void o_move_from_start_to_ts_original_vob(original_t *original);
void o_move_from_mkv_original_to_start(original_t *original);
void o_move_from_mkv_original_webm_to_start(original_t *original);
void o_move_from_mp4_original_to_start(original_t *original);
void o_move_from_mp4_original_mov_to_start(original_t *original);
void o_move_from_mp4_original_avi_to_start(original_t *original);
void o_move_from_mp4_original_mxf_to_start(original_t *original);
void o_move_from_m2ts_original_to_start(original_t *original);
void o_move_from_m2ts_original_mts_to_start(original_t *original);
void o_move_from_ts_original_to_start(original_t *original);
void o_move_from_ts_original_mpg_to_start(original_t *original);
void o_move_from_ts_original_mpeg_to_start(original_t *original);
void o_move_from_ts_original_vob_to_start(original_t *original);
void o_move(char *from_cf_parent_folder, char *to_cf_parent_folder, cf_node_t *cf_head, char *from_valid_extension_string);

void o_detect_duplicates_start_versus_mkv_original(original_t *original);
void o_detect_duplicates_start_versus_mp4_original(original_t *original);
void o_detect_duplicates_start_versus_m2ts_original(original_t *original);
void o_detect_duplicates_start_versus_ts_original(original_t *original);
void o_detect_duplicates_mkv_original_versus_mp4_original(original_t *original);
void o_detect_duplicates_mp4_original_versus_mkv_original(original_t *original);
void o_detect_duplicates_mkv_original_versus_ts_original(original_t *original);
void o_detect_duplicates_mp4_original_versus_ts_original(original_t *original);
void o_detect_duplicates_m2ts_original_versus_ts_original(original_t *original);
void o_detect_duplicates_ts_original_versus_mkv_original(original_t *original);
void o_detect_duplicates_ts_original_versus_mp4_original(original_t *original);
void o_detect_duplicates_ts_original_versus_m2ts_original(original_t *original);
void o_detect_duplicates(char *from_cf_parent_folder, char *to_cf_parent_folder, cf_node_t *cf_head);

/* o_messages */
void o_mes_init_original();

#endif // M_INTERNAL_H
