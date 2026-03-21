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

#ifndef C_CA_LL_STRUCT_H
#define C_CA_LL_STRUCT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "c_cs_ll_struct.h"


/*-----Custom Alias Linked List Struct-----*/
/*
 * The purpose of ca ll is to gather the alias name and it's targets from the conf file
 * What functions use this?:
 * 1.
 * 2.
 * 3.
 * 4.
 */

struct ca_node {
  char *alias_name;
  char *alias_location;
  char *alias_crit_codec;
  char *alias_crit_bits;
  char *alias_crit_color_space;
  char *alias_crit_min_width;
  char *alias_crit_min_height;
  char *alias_crit_max_width;
  char *alias_crit_max_height;

  bool is_alias_name_valid;
  bool is_alias_location_valid;
  bool is_alias_crit_codec_valid;
  bool is_alias_crit_bits_valid;
  bool is_alias_crit_color_space_valid;
  bool is_alias_crit_min_width_valid;
  bool is_alias_crit_min_height_valid;
  bool is_alias_crit_max_width_valid;
  bool is_alias_crit_max_height_valid;

  //ll attached to this ll node
  struct cs_node *cs_head;

  bool is_alias_cs_head_valid;

  bool is_entire_ca_node_valid;


  struct ca_node *next;
};

typedef struct ca_node ca_node_t;

ca_node_t* ca_create_new_empty_node();
ca_node_t *ca_create_new_node(char *alias_name,
                              char *alias_location,
                              char *alias_crit_codec,
                              char *alias_crit_bits,
                              char *alias_crit_color_space,
                              char *alias_crit_min_width,
                              char *alias_crit_min_height,
                              char *alias_crit_max_width,
                              char *alias_crit_max_height,
                              cs_node_t *cs_head);
void ca_print_list(ca_node_t *head);
ca_node_t* ca_insert_at_head(ca_node_t **head, ca_node_t *node_to_insert);
ca_node_t* ca_find_node(ca_node_t *head, char *alias_name_content);
void ca_insert_after_node(ca_node_t *node_to_insert_after, ca_node_t* newnode);
bool ca_alias_name_exits(ca_node_t *head, char *alias_name);
int ca_get_count(ca_node_t *head);
char* ca_get_a_alias_name_from_count(ca_node_t *head, int count);
char* ca_get_a_alias_location_content_from_count(ca_node_t *head, int count);
char* ca_get_a_alias_crit_codec_from_count(ca_node_t *head, int count);
char* ca_get_a_alias_crit_bits_from_count(ca_node_t *head, int count);
char* ca_get_a_alias_crit_color_space_from_count(ca_node_t *head, int count);
char* ca_get_a_alias_crit_min_width_from_count(ca_node_t *head, int count);
char* ca_get_a_alias_crit_min_height_from_count(ca_node_t *head, int count);
char* ca_get_a_alias_crit_max_width_from_count(ca_node_t *head, int count);
char* ca_get_a_alias_crit_max_height_from_count(ca_node_t *head, int count);
cs_node_t* ca_get_a_alias_cs_head_from_count(ca_node_t *head, int count);

ca_node_t* ca_get_a_node_from_alias_name(ca_node_t *head, char *alias_name);
ca_node_t* ca_get_a_node_from_count(ca_node_t *head, int count);

void ca_validate_alias_name_duplicates(ca_node_t *head);

#endif // CUSTOM_ALIAS_LINKED_LIST_H
