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

#include "c_cs_ll_struct.h"

cs_node_t* cs_create_new_empty_node() {
  cs_node_t *result = malloc(sizeof(cs_node_t));

  result->scenario_string = "";
  result->ffmpeg_command = "";

  result->incorrect_codec = false;
  result->incorrect_bit_rate = false;
  result->incorrect_color_space = false;
  result->no_valid_color_space_detected = false;
  result->res_too_low = false;
  result->res_too_high = false;

  result->codec_just_right = false;
  result->bit_rate_just_right = false;
  result->color_space_just_right = false;
  result->res_just_right = false;

  result->eq_higher_4k = false;
  result->below_4k = false;
  result->eq_higher_full_hd = false;
  result->below_full_hd = false;
  result->eq_higher_half_hd = false;
  result->below_half_hd = false;
  result->eq_higher_480p = false;
  result->below_480p = false;
  result->eq_higher_360p = false;
  result->below_360p = false;
  result->eq_higher_240p = false;
  result->below_240p = false;

  result->eq_less_width_res = false;
  result->eq_less_width_res_number = 0;
  result->eq_greater_width_res = false;
  result->eq_greater_width_res_number = 0;
  result->eq_less_height_res = false;
  result->eq_less_height_res_number = 0;
  result->eq_greater_height_res = false;
  result->eq_greater_height_res_number = 0;

  result->else_detected = false;

  result->next = NULL;
  return result;
}

cs_node_t* cs_create_new_unverified_node(char *scenario_string,
                                         char *ffmpeg_command) {
  cs_node_t *result = malloc(sizeof(cs_node_t));
  result->scenario_string = scenario_string;
  result->ffmpeg_command = ffmpeg_command;

  result->incorrect_codec = false;
  result->incorrect_bit_rate = false;
  result->incorrect_color_space = false;
  result->no_valid_color_space_detected = false;
  result->res_too_low = false;
  result->res_too_high = false;

  result->codec_just_right = false;
  result->bit_rate_just_right = false;
  result->color_space_just_right = false;
  result->res_just_right = false;

  result->eq_higher_4k = false;
  result->below_4k = false;
  result->eq_higher_full_hd = false;
  result->below_full_hd = false;
  result->eq_higher_half_hd = false;
  result->below_half_hd = false;
  result->eq_higher_480p = false;
  result->below_480p = false;
  result->eq_higher_360p = false;
  result->below_360p = false;
  result->eq_higher_240p = false;
  result->below_240p = false;

  result->eq_less_width_res = false;
  result->eq_less_width_res_number = 0;
  result->eq_greater_width_res = false;
  result->eq_greater_width_res_number = 0;
  result->eq_less_height_res = false;
  result->eq_less_height_res_number = 0;
  result->eq_greater_height_res = false;
  result->eq_greater_height_res_number = 0;

  result->else_detected = false;

  result->next = NULL;
  return result;
}

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
                              bool else_detected) {
  cs_node_t *result = malloc(sizeof(cs_node_t));

  result->scenario_string = scenario_string;
  result->ffmpeg_command = ffmpeg_command;

  result->incorrect_codec = incorrect_codec;
  result->incorrect_bit_rate = incorrect_bit_rate;
  result->incorrect_color_space = incorrect_color_space;
  result->no_valid_color_space_detected = no_valid_color_space_detected;
  result->res_too_low = res_too_low;
  result->res_too_high = res_too_high;

  result->codec_just_right = codec_just_right;
  result->bit_rate_just_right = bit_rate_just_right;
  result->color_space_just_right = color_space_just_right;
  result->res_just_right = res_just_right;

  result->eq_higher_4k = eq_higher_4k;
  result->below_4k = below_4k;
  result->eq_higher_full_hd = eq_higher_full_hd;
  result->below_full_hd = below_full_hd;
  result->eq_higher_half_hd = eq_higher_half_hd;
  result->below_half_hd = below_half_hd;
  result->eq_higher_480p = eq_higher_480p;
  result->below_480p = below_480p;
  result->eq_higher_360p = eq_higher_360p;
  result->below_360p = below_360p;
  result->eq_higher_240p = eq_higher_240p;
  result->below_240p = below_240p;

  result->eq_less_width_res = eq_less_width_res;
  result->eq_less_width_res_number = eq_less_width_res_number;
  result->eq_greater_width_res = eq_greater_width_res;
  result->eq_greater_width_res_number = eq_greater_width_res_number;
  result->eq_less_height_res = eq_less_height_res;
  result->eq_less_height_res_number = eq_less_height_res_number;
  result->eq_greater_height_res = eq_greater_height_res;
  result->eq_greater_height_res_number = eq_greater_height_res_number;

  result->else_detected = else_detected;

  result->next = NULL;
  return result;
}

cs_node_t *cs_insert_at_head(cs_node_t **head, cs_node_t *node_to_insert) {
  node_to_insert->next = *head;
  *head = node_to_insert;
  return node_to_insert;
}

void cs_insert_after_node(cs_node_t *node_to_insert_after, cs_node_t* newnode) {
  newnode->next = node_to_insert_after->next;
  node_to_insert_after->next = newnode;
}

cs_node_t *cs_find_node(cs_node_t *head, char *scenario_string) {
  cs_node_t *tmp = head;
  while (tmp != NULL) {
    //this doesn't work for char's
    if(strcmp(tmp->scenario_string, scenario_string) == 0) return tmp;
    tmp = tmp->next;
  }
  return NULL;
}

void cs_print_list(cs_node_t *head) {
  cs_node_t *temporary = head;

  if(temporary != NULL) {
    while (temporary != NULL) {
      printf("a cs node:\n");
      printf("  scenario string: %s\n" 
            "   ffmpeg_command: %s\n" 
            "   incorrect_codec: %i\n" 
            "   incorrect_bit_rate: %i\n"
            "   incorrect_color_space: %i\n" 
            "   no_valid_color_space_detected: %i\n" 
            "   res_too_low: %i\n"
            "   res_too_high: %i\n"
            "   codec_just_right: %i\n"
            "   bit_rate_just_right: %i\n"
            "   color_space_just_right: %i\n"
            "   res_just_right: %i\n"
            "   eq_higher_4k: %i\n"
            "   below_4k: %i\n"
            "   eq_higher_full_hd: %i\n"
            "   below_full_hd: %i\n"
            "   eq_higher_half_hd: %i\n"
            "   below_half_hd: %i\n"
            "   eq_higher_480p: %i\n"
            "   below_480p: %i\n" 
            "   eq_higher_360p: %i\n"
            "   below_360p: %i\n"
            "   eq_higher_240p: %i\n"
            "   below_240p: %i\n"
            "   eq_less_width_res: %i\n"
            "   eq_less_width_res_number: %i\n"
            "   eq_greater_width_res: %i\n"
            "   eq_greater_width_res_number: %i\n"
            "   eq_less_height_res: %i\n"
            "   eq_less_height_res_number: %i\n"
            "   eq_greater_height_res: %i\n"
            "   eq_greater_height_res_number: %i\n"
            "   else_detected: %i\n",
            temporary->scenario_string,
            temporary->ffmpeg_command,
            temporary->incorrect_codec,
            temporary->incorrect_bit_rate,
            temporary->incorrect_color_space,
            temporary->no_valid_color_space_detected,
            temporary->res_too_low,
            temporary->res_too_high,
            temporary->codec_just_right,
            temporary->bit_rate_just_right,
            temporary->color_space_just_right,
            temporary->res_just_right,
            temporary->eq_higher_4k,
            temporary->below_4k,
            temporary->eq_higher_full_hd,
            temporary->below_full_hd,
            temporary->eq_higher_half_hd,
            temporary->below_half_hd,
            temporary->eq_higher_480p,
            temporary->below_480p,
            temporary->eq_higher_360p,
            temporary->below_360p,
            temporary->eq_higher_240p,
            temporary->below_240p,
            temporary->eq_less_width_res,
            temporary->eq_less_width_res_number,
            temporary->eq_greater_width_res,
            temporary->eq_greater_width_res_number,
            temporary->eq_less_height_res,
            temporary->eq_less_height_res_number,
            temporary->eq_greater_height_res,
            temporary->eq_greater_height_res_number,
            temporary->else_detected);
      printf("\n");
      temporary = temporary->next;
    }
  } else printf("list is empty\n");
}
