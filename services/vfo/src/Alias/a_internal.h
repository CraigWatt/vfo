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

#ifndef A_INTERNAL_H
#define A_INTERNAL_H

#define LOWER 1024
#define UPPER 65536

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
#include "../alias.h"
#include "../utils.h"

void a_source_to_alias(aliases_t *alias);

void a_pre_encode_checks(aliases_t *alias);
void a_highlight_encode_candidates_to_user(aliases_t *alias);
void a_highlight_encode_candidates_from_source_content(aliases_t *alias);
void a_encode_source_to_an_alias(aliases_t *alias);
void s_encode_from_source_to_alias(aliases_t *alias);
bool a_execute_ffmpeg_command(char *from, char *to, aliases_t *alias);
char* a_generate_ffmpeg_command(char *from, char *to, aliases_t *alias);
char* a_generate_alias_file_name(char *from, char *to, char *alias_name);
bool a_execute_ffprobe(char *from_file, aliases_t *alias);
char* a_generate_ffprobe_command(char *from_file, aliases_t *alias);
char* a_match_json_file_with_scenario(aliases_t *alias);
char* a_extract_json_to_string(char *tmp_json_file);
char* a_fetch_json_string(char *json_string, char *marker);
int a_fetch_json_number(char *json_string, char *marker);

#endif // A_INTERNAL_H
