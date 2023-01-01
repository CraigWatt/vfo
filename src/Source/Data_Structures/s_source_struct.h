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

#ifndef S_SOURCE_STRUCT_H
#define S_SOURCE_STRUCT_H
/*Source Struct*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../config.h"
#include "../../utils.h"

/*What exactly does source need to point to in config to do it's work?*/

struct source {
  char *root;
  char *content;
  char *unable_to_process;

  char *mp4_extension;

  char *original_mkv_original;
  char *original_mp4_original;

  struct cf_node *cf_head;
};
typedef struct source source_t;

source_t* source_create_new_struct(config_t *config);
char* s_create_content(char *source_root);
char* s_create_unable_to_process(char *source_root);
char* s_get_original_root();
char* s_get_mkv_original_if_it_exists();
char* s_get_mp4_original_if_it_exists();

#endif // S_SOURCE_STRUCT_H
