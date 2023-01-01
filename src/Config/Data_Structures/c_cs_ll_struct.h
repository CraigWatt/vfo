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

#ifndef C_CUSTOM_SCENARIOS_LINKED_LIST_H
#define C_CUSTOM_SCENARIOS_LINKED_LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* ----------custom_scenarios_linked_list header file---------- */
/*
 * The purpose of cs ll is to store each 'alias ffmpeg command' and it's
 * 'logical rules' gathered from .conf file
 * 
 * this data still needs to be used by the program, these functions are still to be built
 *
 * What functions use this?:
 * 1.
 * 2.
 * 3.
 * 4.
 */

struct cs_node {
  char *scenario_string;
  char *ffmpeg_command;
  
  bool incorrect_codec;
  bool incorrect_bit_rate;
  bool incorrect_color_space;
  bool no_valid_color_space_detected;
  bool res_too_low;
  bool res_too_high;

  bool codec_just_right;
  bool bit_rate_just_right;
  bool color_space_just_right;
  bool res_just_right;
  
  bool eq_higher_4k;
  bool below_4k;
  bool eq_higher_full_hd;
  bool below_full_hd;
  bool eq_higher_half_hd;
  bool below_half_hd;
  bool eq_higher_480p;
  bool below_480p;
  bool eq_higher_360p;
  bool below_360p;
  bool eq_higher_240p;
  bool below_240p;
  
  bool eq_less_width_res;
  int eq_less_width_res_number;
  bool eq_greater_width_res;
  int eq_greater_width_res_number;
  bool eq_less_height_res;
  int eq_less_height_res_number;
  bool eq_greater_height_res;
  int eq_greater_height_res_number;

  bool else_detected;

  struct cs_node* next;
};

typedef struct cs_node cs_node_t;

cs_node_t* cs_create_new_empty_node();
cs_node_t* cs_create_new_unverified_node(char *scenario_string,
                                         char *ffmpeg_command);
cs_node_t* cs_create_new_node(char *scenario_string,
                              char *ffmpeg_command,
                              bool incorrect_codec,
                              bool incorrect_bit_rate,
                              bool incorrect_color_space,
                              bool no_valid_color_space_detected,
                              bool res_too_low,
                              bool res_too_high,
                              bool codec_just_right,
                              bool bit_rate_just_right,
                              bool color_space_just_right,
                              bool res_just_right,
                              bool eq_higher_4k,
                              bool below_4k,
                              bool eq_higher_full_hd,
                              bool below_full_hd,
                              bool eq_higher_half_hd,
                              bool below_half_hd,
                              bool eq_higher_480p,
                              bool below_480p,
                              bool eq_higher_360p,
                              bool below_360p,
                              bool eq_higher_240p,
                              bool below_240p,
                              bool eq_less_width_res,
                              int eq_less_width_res_number,
                              bool eq_greater_width_res,
                              int eq_greater_width_res_number,
                              bool eq_less_height_res,
                              int eq_less_height_res_number,
                              bool eq_greater_height_res,
                              int eq_greater_height_res_number,
                              bool else_detected);
void cs_print_list(cs_node_t *head);
cs_node_t* cs_insert_at_head(cs_node_t **head, cs_node_t *node_to_insert);
cs_node_t* cs_find_node(cs_node_t *head, char *string_content);
void cs_insert_after_node(cs_node_t *node_to_insert_after, cs_node_t* newnode);

#endif // C_CUSTOM_SCENARIOS_LINKED_LIST_H
