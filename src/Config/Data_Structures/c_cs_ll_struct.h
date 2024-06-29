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
