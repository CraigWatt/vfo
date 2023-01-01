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

#ifndef A_ALIASES_STRUCT_H
#define A_ALIASES_STRUCT_H
/*Alias Struct*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../config.h"
#include "../../utils.h"

/* What exactly does alias need to point to in config to do it's work?*/

struct alias {
  char *root;
  char *content;
  char *unable_to_process;
  bool keep_source;

  /*only required if keep_source = false*/
  char *original_root;
  char *original_mkv_original;
  char *original_mp4_original;
  /*------------------------------------*/

  char *source_root;
  char *source_content;

  struct cf_node *cf_head;

  char *name;
  char *crit_codec;
  int crit_bits;
  char *crit_color_space;
  int crit_min_width;
  int crit_min_height;
  int crit_max_width;
  int crit_max_height;

  struct cs_node *cs_head;

  /*REMEMBER, I must validate all of the above!*/
  struct alias *next;
};
typedef struct alias aliases_t;

aliases_t* alias_create_new_struct(config_t *config, ca_node_t *ca_node);
char* a_create_content(char *alias_root);
char* a_create_unable_to_process(char *alias_root);
char* a_get_mkv_original_if_it_exists(char *original_root);
char* a_get_mp4_original_if_it_exists(char *original_root);
char* a_create_source_content(char *source_root);

char* a_verify_alias_location(char *alias_location);
char* a_verify_alias_name(char *alias_name);
char* a_verify_alias_crit_codec(char *alias_crit_codec, char *alias_name);
int a_verify_alias_crit_bits(char *alias_crit_bits, char *alias_name);
char* a_verify_alias_crit_color_space(char *alias_crit_color_space, char *alias_name);
int a_verify_alias_crit_min_width(char *alias_crit_min_width, char *alias_name);
int a_verify_alias_crit_min_height(char *alias_crit_min_height, char *alias_name);
int a_verify_alias_crit_max_width(char *alias_crit_max_width, char *alias_name);
int a_verify_alias_crit_max_height(char *alias_crit_max_height, char *alias_name);
cs_node_t* a_verifiy_alias_cs_head(cs_node_t *cs_head, char *alias_name);

int a_fetch_specific_res_from_scenario_string(char *scenario_string, char *res_marker);

aliases_t *alias_insert_at_head(aliases_t **head, aliases_t *node_to_insert);
void alias_insert_after_node(aliases_t *node_to_insert_after, aliases_t* newnode);
#endif // A_ALIASES_STRUCT_H
