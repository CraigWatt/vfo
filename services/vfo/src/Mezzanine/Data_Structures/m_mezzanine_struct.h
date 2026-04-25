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

#ifndef M_MEZZANINE_STRUCT_H
#define M_MEZZANINE_STRUCT_H
/* Mezzanine struct (legacy internal type names retained for compatibility). */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../config.h"
#include "../../utils.h"

/* What does mezzanine need to point to in config to do its work? */

struct original {
  char *root;
  char *start;
  char *unable_to_process;

  char *mkv_extension;
  char *mp4_extension;
  char *m2ts_extension;
  char *ts_extension;

  char *mkv_original;
  char *mp4_original;
  char *m2ts_original;
  char *ts_original;

  struct cf_node *cf_head;
};
typedef struct original original_t;

original_t* original_create_new_struct(config_t *config);
char* o_create_start(char *original_root);
char* o_create_unable_to_process(char *original_root);
char* o_get_mkv_original_if_it_exists(char *original_root, cf_node_t *cf_head);
char* o_get_mp4_original_if_it_exists(char *original_root, cf_node_t *cf_head);
char* o_get_m2ts_original_if_it_exists(char *original_root, cf_node_t *cf_head);
char* o_get_ts_original_if_it_exists(char *original_root, cf_node_t *cf_head);

#endif // M_MEZZANINE_STRUCT_H
