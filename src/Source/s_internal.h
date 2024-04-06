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

#ifndef S_INTERNAL_H
#define S_INTERNAL_H

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include <ftw.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <libgen.h>

#include "../source.h"
#include "../utils.h"

void s_pre_encode_checks(source_t *source);

void s_highlight_encode_candidates_to_user(source_t *source);
void s_highlight_encode_candidates_from_mkv_original(source_t *source);
void s_highlight_encode_candidates_from_mp4_original(source_t *source);
void s_highlight_encode_candidates_from_m2ts_original(source_t *source);

void s_encode_original_to_source(source_t *source);
void s_encode_from_mkv_original_to_source(source_t *source);
void s_encode_from_mp4_original_to_source(source_t *source);
void s_encode_from_m2ts_original_to_source(source_t *source);
void s_encode_shared_code(active_cf_node_t *active_cf, source_t *source);

char* s_generate_ffmpeg_command(char *original_from, char *source_to);
char* s_generate_ffmpeg_command_source_test_active(char *original_from, char *source_to, char *source_test_start, char *source_test_duration);
char* s_generate_source_file_name(char *original_from, char *source_to);
bool s_execute_ffmpeg_command(char *original_from, char *source_to, bool source_test, char *source_test_start, char *source_test_duration);

void s_danger_wipe_all_in_source_content(source_t *source);

/* s_messages */
void s_mes_init_source();

#endif // S_INTERNAL_H
