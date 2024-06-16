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

#ifndef AS_AUDIO_STRAT_STRUCT_H
#define AS_AUDIO_STRAT_STRUCT_H
/*Source Struct*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../config.h"
#include "../../utils.h"

struct source_as {
  char *root;
  char *content;
  char *unable_to_process;

  char *source_content;
  char *source_as_content;

  char *current_codec_name;
  char *current_codec_long_name;

  struct cf_node *cf_head;
};
typedef struct source_as source_as_t;

source_as* sas_create_new_struct(config_t *config);
char* sas_create_content(char *source_as_root);
char* sas_create_unable_to_process(char *source_as_root);


#endif // AS_AUDIO_STRAT_STRUCT_H
