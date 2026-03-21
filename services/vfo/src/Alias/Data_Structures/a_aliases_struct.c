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

#include "a_aliases_struct.h"
#include <stdlib.h>

aliases_t* alias_create_new_struct(config_t *config, ca_node_t *ca_node) {
  aliases_t *result = malloc(sizeof(aliases_t));
  //point to relevant param config variables

  //NEED TO VERIFY
  result->root = a_verify_alias_location(ca_node->alias_location);
  //NOW VERIFIED 

  //VERIFIED
  result->content = a_create_content(result->root);  
  //VERIFIED
  result->unable_to_process = a_create_unable_to_process(result->root);

  //ALREADY VERIFIED (by config)
  result->keep_source = config->svc->keep_source;

  //ALREADY VERIFIED (by config)
  result->original_root = config->svc->original_location;
  //VERIFIED
  result->original_mkv_original = a_get_mkv_original_if_it_exists(result->original_root);
  //VERIFIED
  result->original_mp4_original = a_get_mp4_original_if_it_exists(result->original_root);

  //ALREADY VERIFIED (by config)
  result->source_root = config->svc->source_location;
  //VERIFIED
  result->source_content = a_create_source_content(config->svc->source_location);

  //ALREADY VERIFIED (by config)
  result->cf_head = config->cf_head;

  //NEED TO VERIFY (only verified that it is not a duplicate by config)
  result->name = a_verify_alias_name(ca_node->alias_name);
  //NOW VERIFIED

  //NEED TO VERIFY 
  result->crit_codec = a_verify_alias_crit_codec(ca_node->alias_crit_codec, result->name);


  //NEED TO VERIFY 
  result->crit_bits = a_verify_alias_crit_bits(ca_node->alias_crit_bits, result->name);
  //VERIFIED

  //NEED TO VERIFY 
  result->crit_color_space = a_verify_alias_crit_color_space(ca_node->alias_crit_color_space, result->name);
  //VERIFIED

  //NEED TO VERIFY 
  result->crit_min_width = a_verify_alias_crit_min_width(ca_node->alias_crit_min_width, result->name);
  //VERIFIED

  //NEED TO VERIFY 
  result->crit_min_height = a_verify_alias_crit_min_height(ca_node->alias_crit_min_height, result->name);
  //VERIFIED

  //NEED TO VERIFY 
  result->crit_max_width = a_verify_alias_crit_max_width(ca_node->alias_crit_max_width, result->name);
  //VERIFIED

  //NEED TO VERIFY 
  result->crit_max_height = a_verify_alias_crit_max_height(ca_node->alias_crit_max_height, result->name);
  //VERIFIED

  //NEED TO VERIFY 
  result->cs_head = a_verifiy_alias_cs_head(ca_node->cs_head, result->name);
  //VERIFIED

  result->next = NULL;

  return result;
}

char* a_create_content(char *alias_root) {
  //create
  char *tmp_alias_content = malloc(BUFSIZ);
  strcpy(tmp_alias_content, alias_root);
  strcat(tmp_alias_content, "/content");
  //verify
  if(!utils_does_folder_exist(tmp_alias_content)) {
    printf("ALIAS WARNING: vfo could not find an essential /content folder in %s\n", alias_root);
    utils_ask_user_for_permission_to_create_a_folder(tmp_alias_content);
    utils_create_folder(tmp_alias_content);
    if(!utils_does_folder_exist(tmp_alias_content)) {
      printf("ALIAS MAJOR ERROR: vfo could not create %s & it does not exist ALIAS MAJOR ERROR\n", tmp_alias_content);
      exit(EXIT_FAILURE);
    }
  }
  return tmp_alias_content;
}

char* a_create_unable_to_process(char *alias_root) {
  //create
  char *tmp_alias_unable_to_process = malloc(BUFSIZ);
  strcpy(tmp_alias_unable_to_process, alias_root);
  strcat(tmp_alias_unable_to_process, "/unable_to_process");
  //verify
  if(!utils_does_folder_exist(tmp_alias_unable_to_process)) {
    printf("ALIAS WARNING: vfo could not find an essential /unable_to_process folder in %s\n", alias_root);
    utils_ask_user_for_permission_to_create_a_folder(tmp_alias_unable_to_process);
    utils_create_folder(tmp_alias_unable_to_process);
    if(!utils_does_folder_exist(tmp_alias_unable_to_process)) {
      printf("ALIAS MAJOR ERROR: vfo could not create %s & it does not exist ALIAS MAJOR ERROR\n", tmp_alias_unable_to_process);
      exit(EXIT_FAILURE);
    }
  }
  return tmp_alias_unable_to_process;
}

char* a_get_mkv_original_if_it_exists(char *original_root) {
  //create
  char *tmp_alias_mkv_original = malloc(BUFSIZ);
  strcpy(tmp_alias_mkv_original, original_root);
  strcpy(tmp_alias_mkv_original, "/mkv_original");
  //verify
  if(utils_does_folder_exist(tmp_alias_mkv_original)) {
    printf("ALIAS ALERT: original's /mkv_original detected. if KEEP_SOURCE=FALSE, encoding CAN occur from mkv_original to this alias.\n");
    return tmp_alias_mkv_original;
  }
  else
    printf("ALIAS ALERT: original's /mkv_original does not exist, if KEEP_SOURCE=FALSE, no encoding will occur from mkv_original to this alias.\n");
  return "";
}

char* a_get_mp4_original_if_it_exists(char *original_root) {
  //create
  char *tmp_alias_mp4_original = malloc(BUFSIZ);
  strcpy(tmp_alias_mp4_original, original_root);
  strcpy(tmp_alias_mp4_original, "/mp4_original");
  //verify
  if(utils_does_folder_exist(tmp_alias_mp4_original)) {
    printf("ALIAS ALERT: original's /mp4_original detected.if KEEP_SOURCE=FALSE, encoding CAN occur from mp4_original to this alias.\n");
    return tmp_alias_mp4_original;
  } else
    printf("ALIAS ALERT: original's /mp4_original does not exist, if KEEP_SOURCE=FALSE, no encoding will occur from mp4_original to this alias.\n");
  return "";
}

char* a_create_source_content(char *source_root) {
  //create
  char *tmp_alias_source_content = malloc(BUFSIZ);
  strcpy(tmp_alias_source_content, source_root);
  strcat(tmp_alias_source_content, "/content");
  //verify
  if(!utils_does_folder_exist(tmp_alias_source_content)) {
    printf("ALIAS WARNING: vfo could not find an essential SOURCE /content folder in %s\n", source_root);
    utils_ask_user_for_permission_to_create_a_folder(tmp_alias_source_content);
    utils_create_folder(tmp_alias_source_content);
    if(!utils_does_folder_exist(tmp_alias_source_content)) {
      printf("ALIAS MAJOR ERROR: vfo could not create SOURCE %s & it does not exist ALIAS MAJOR ERROR\n", tmp_alias_source_content);
      exit(EXIT_FAILURE);
    }
  }
  return tmp_alias_source_content;
}

char* a_verify_alias_location(char *alias_location) {
  //verify
  printf("THIS RUNS\n");
  if(utils_string_is_empty_or_spaces(alias_location))
    printf("ALIAS ERROR: an alias location specified in config file appears to be empty\n");
  if(!utils_does_folder_exist(alias_location)) {
    printf("ALIAS ERROR: vfo could not find an alias location specified in your config file: %s\n", alias_location);
    exit(EXIT_FAILURE);
  }
  return alias_location;
}

char* a_verify_alias_name(char *alias_name) {
  //verify
  if(utils_string_is_empty_or_spaces(alias_name))
    printf("ALIAS WARNING: an alias name specified in config file appears to be empty\n");
  return alias_name;
}

char* a_verify_alias_crit_codec(char *alias_crit_codec, char *alias_name) {
  //verify
  if(utils_string_is_empty_or_spaces(alias_crit_codec)) {
    printf("ALIAS %s ERROR: alias criteria codec appears to be empty %s\n", alias_name, alias_crit_codec);
    exit(EXIT_FAILURE);
  }
  return alias_crit_codec;
}

int a_verify_alias_crit_bits(char *alias_crit_bits, char *alias_name) {
  //verify
  if(utils_string_is_empty_or_spaces(alias_crit_bits)) {
    printf("ALIAS %s ERROR: alias criteria bits appears to be empty %s\n", alias_name, alias_crit_bits);
    exit(EXIT_FAILURE);
  }

  printf("ALIAS %s DEV: reading alias_crit_bits as %s\n", alias_name, alias_crit_bits);
  //convert char to int
  int tmp_int = utils_convert_string_to_integer(alias_crit_bits);
  //verify int
  if(tmp_int != 8 && tmp_int != 10) {
    printf("ALIAS %s ERROR: vfo is reading %s criteria bits from config file NOT equal to 8 or 10. config file variable: %s int conversion: %i\n", alias_name, alias_name, alias_crit_bits, tmp_int);
    exit(EXIT_FAILURE);
  }
  printf("ALIAS %s ALERT: reading criteria bits as: %i\n", alias_name, tmp_int);
  return tmp_int;
}

char* a_verify_alias_crit_color_space(char *alias_crit_color_space, char *alias_name) {
  //verify
  if(utils_string_is_empty_or_spaces(alias_crit_color_space)) {
    printf("ALIAS %s ERROR: alias criteria color space appears to be empty %s\n", alias_name, alias_crit_color_space);
    exit(EXIT_FAILURE);
  }

  return alias_crit_color_space;
}

int a_verify_alias_crit_min_width(char *alias_crit_min_width, char *alias_name) {
  //verify
  if(utils_string_is_empty_or_spaces(alias_crit_min_width)) {
    printf("ALIAS %s ERROR: alias criteria min width appears to be empty %s\n", alias_name, alias_crit_min_width);
    exit(EXIT_FAILURE);
  }

  printf("ALIAS %s DEV: reading alias_crit_min_width as %s\n", alias_name, alias_crit_min_width);
  //convert char to int
  int tmp_int = utils_convert_string_to_integer(alias_crit_min_width);
  //verify int
  if(tmp_int < 0) {
    printf("ALIAS %s ERROR: vfo is reading %s criteria min width from config file as less than 0. config file variable: %s int conversion: %i\n", alias_name, alias_name, alias_crit_min_width, tmp_int);
    exit(EXIT_FAILURE);
  }
  if(tmp_int > 12288)
    printf("ALIAS %s WARNING: vfo is reading %s criteria min width from config file as greater than 12288 (greater than 12k).  config file variable: %s int conversion %i.  You must be an absolute mad lad.\n", alias_name, alias_name, alias_crit_min_width, tmp_int);
  printf("ALIAS %s ALERT: reading criteria min width as: %i\n", alias_name, tmp_int);
  return tmp_int;
}

int a_verify_alias_crit_min_height(char *alias_crit_min_height, char *alias_name) {
  //verify
  if(utils_string_is_empty_or_spaces(alias_crit_min_height)) {
    printf("ALIAS %s ERROR: alias criteria min height appears to be empty %s\n", alias_name, alias_crit_min_height);
    exit(EXIT_FAILURE);
  }
  //convert char to int
  int tmp_int = utils_convert_string_to_integer(alias_crit_min_height);
  //verify int
  if(tmp_int < 0) {
    printf("ALIAS %s ERROR: vfo is reading %s criteria min height from config file as less than 0. config file variable: %s int conversion: %i\n", alias_name, alias_name, alias_crit_min_height, tmp_int);
    exit(EXIT_FAILURE);
  }
  if(tmp_int > 6480)
    printf("ALIAS %s WARNING: vfo is reading %s criteria min height from config file as greater than 6480 (greater than 12k).  config file variable: %s int conversion %i.  You must be an absolute mad lad.\n", alias_name, alias_name, alias_crit_min_height, tmp_int);
  printf("ALIAS %s ALERT: reading criteria min height as: %i\n", alias_name, tmp_int);
  return tmp_int;
}

int a_verify_alias_crit_max_width(char *alias_crit_max_width, char *alias_name) {
  //verify
  if(utils_string_is_empty_or_spaces(alias_crit_max_width)) {
    printf("ALIAS %s ERROR: alias criteria max width appears to be empty %s\n", alias_name, alias_crit_max_width);
    exit(EXIT_FAILURE);
  }
  //convert char to int
  int tmp_int = utils_convert_string_to_integer(alias_crit_max_width);
  //verify int
  if(tmp_int < 0) {
    printf("ALIAS %s ERROR: vfo is reading %s criteria max width from config file as less than 0.  config file variable: %s int conversion %i\n", alias_name, alias_name, alias_crit_max_width, tmp_int);
    exit(EXIT_FAILURE);
  }
  if(tmp_int > 12288)
    printf("ALIAS %s WARNING: vfo is reading %s criteria max width from config file as greater than 12288 (greater than 12k).  config file variable %s int conversion %i.  You must be an absolute mad lad.\n", alias_name, alias_name, alias_crit_max_width, tmp_int);
  printf("ALIAS %s ALERT: reading criteria max width as %i\n", alias_name, tmp_int);
  return tmp_int;
}

int a_verify_alias_crit_max_height(char *alias_crit_max_height, char *alias_name) {
  //verify
  if(utils_string_is_empty_or_spaces(alias_crit_max_height)) {
    printf("ALIAS %s ERROR: alias criteria max height appears to be empty %s\n", alias_name, alias_crit_max_height);
    exit(EXIT_FAILURE);
  }
  //convert char to int
  int tmp_int = utils_convert_string_to_integer(alias_crit_max_height);
  //verify int
  if(tmp_int < 0) {
    printf("ALIAS %s ERROR: vfo is reading %s criteria max height from config file as less than 0.  config file variable: %s int conversion %i\n", alias_name, alias_name, alias_crit_max_height, tmp_int);
    exit(EXIT_FAILURE);
  }
  if(tmp_int > 6480)
    printf("ALIAS %s WARNING: vfo is reading %s criteria max height from config file as greater than 6480 (greater than 12k).  config file variable %s int conversion %i.  You must be an absolute mad lad.\n", alias_name, alias_name, alias_crit_max_height, tmp_int);
  printf("ALIAS %s ALERT: reading criteria max height as %i\n", alias_name, tmp_int);
  return tmp_int;
}

cs_node_t* a_verifiy_alias_cs_head(cs_node_t *cs_head, char *alias_name) {
  //verify every node in cs_head
  if(cs_head == NULL) {
    printf("ALIAS %s MAJOR ERROR: vfo could not find any SCENARIO configuration relevant to %s.  Please check you config file.\n", alias_name, alias_name);
    exit(EXIT_FAILURE);
  }
  bool cs_head_error_detected = false;
  cs_node_t* cs_tmp = cs_head;
  cs_node_t* verified_cs_head;
  if(cs_tmp != NULL){
    while(cs_tmp != NULL) {
      //verify scenario_string
      cs_node_t* cs_verified_node = cs_create_new_empty_node();
      cs_verified_node->scenario_string = cs_tmp->scenario_string;
      cs_verified_node->ffmpeg_command = cs_tmp->ffmpeg_command;

      if(utils_string_is_empty_or_spaces(cs_tmp->scenario_string)) {
        printf("ALIAS %s MAJOR ERROR: we found a %s_SCENARIO to be empty: %s\n", alias_name, alias_name, cs_tmp->scenario_string);
        cs_head_error_detected = true;
      }
      //set relevant bools & ints from scenario_string
      if(utils_is_substring("INCORRECT_CODEC", cs_tmp->scenario_string)) {
        cs_verified_node->incorrect_codec = true;
      }
      if(utils_is_substring("INCORRECT_BIT_RATE", cs_tmp->scenario_string)) {
        cs_verified_node->incorrect_bit_rate = true;
      }
      if(utils_is_substring("INCORRECT_COLOR_SPACE", cs_tmp->scenario_string)) {
        cs_verified_node->incorrect_color_space = true;
      }
      if(utils_is_substring("NO_VALID_COLOR_SPACE_DETECTED", cs_tmp->scenario_string)) {
        cs_verified_node->no_valid_color_space_detected = true;
      }
      if(utils_is_substring("RES_TOO_LOW", cs_tmp->scenario_string)) {
        cs_verified_node->res_too_low = true;
      }
      if(utils_is_substring("RES_TOO_HIGH", cs_tmp->scenario_string)) {
        cs_verified_node->res_too_high = true;
      }
      if(utils_is_substring("CODEC_JUST_RIGHT", cs_tmp->scenario_string)) {
        cs_verified_node->codec_just_right = true;
      }
      if(utils_is_substring("BIT_RATE_JUST_RIGHT", cs_tmp->scenario_string)) {
        cs_verified_node->bit_rate_just_right = true;
      }
      if(utils_is_substring("COLOR_SPACE_JUST_RIGHT", cs_tmp->scenario_string)) {
        cs_verified_node->color_space_just_right = true;
      }
      if(utils_is_substring("RES_JUST_RIGHT", cs_tmp->scenario_string)) {
        cs_verified_node->res_just_right = true;
      }
      if(utils_is_substring("ALL_RIGHT", cs_tmp->scenario_string)) {
        cs_verified_node->codec_just_right = true;
        cs_verified_node->bit_rate_just_right = true;
        cs_verified_node->color_space_just_right = true;
        cs_verified_node->res_just_right = true;
      }
      if(utils_is_substring("BELOW_4K", cs_tmp->scenario_string)) {
        cs_verified_node->below_4k = true;
      }
      if(utils_is_substring("4K_OR_HIGHER", cs_tmp->scenario_string)) {
        cs_verified_node->eq_higher_4k = true;
      }
      if(utils_is_substring("BELOW_FULL_HD", cs_tmp->scenario_string)) {
        cs_verified_node->below_full_hd = true;
      }
      if(utils_is_substring("FULL_HD_OR_HIGHER", cs_tmp->scenario_string)) {
        cs_verified_node->eq_higher_full_hd = true;
      }
      if(utils_is_substring("BELOW_HALF_HD", cs_tmp->scenario_string)) {
        cs_verified_node->below_half_hd = true;
      }
      if(utils_is_substring("HALF_HD_OR_HIGHER", cs_tmp->scenario_string)) {
        cs_verified_node->eq_higher_half_hd = true;
      }
      if(utils_is_substring("BELOW_480p", cs_tmp->scenario_string)) {
        cs_verified_node->below_480p = true;
      }
      if(utils_is_substring("480p_OR_HIGHER", cs_tmp->scenario_string)) {
        cs_verified_node->eq_higher_480p = true;
      }
      if(utils_is_substring("BELOW_360p", cs_tmp->scenario_string)) {
        cs_verified_node->below_360p = true;
      }
      if(utils_is_substring("360p_OR_HIGHER", cs_tmp->scenario_string)) {
        cs_verified_node->eq_higher_360p = true;
      }
      if(utils_is_substring("BELOW_240p", cs_tmp->scenario_string)) {
        cs_verified_node->below_240p = true;
      }
      if(utils_is_substring("240p_OR_HIGHER", cs_tmp->scenario_string)) {
        cs_verified_node->eq_higher_240p = true;
      }
      if(utils_is_substring("WIDTH_RES<=", cs_tmp->scenario_string)) {
        cs_verified_node->eq_less_width_res = true;
        cs_verified_node->eq_less_width_res_number = a_fetch_specific_res_from_scenario_string(cs_tmp->scenario_string, "WIDTH_RES<=");
        printf("DEV detected WIDTH_RES<=*number* to be: %i\n", cs_verified_node->eq_less_width_res_number);
      }
      if(utils_is_substring("WIDTH_RES>=", cs_tmp->scenario_string)) {
        cs_verified_node->eq_greater_width_res = true;
        cs_verified_node->eq_greater_width_res_number = a_fetch_specific_res_from_scenario_string(cs_tmp->scenario_string, "WIDTH_RES>=");
        printf("DEV detected WIDTH_RES>=*number* to be: %i\n", cs_verified_node->eq_greater_width_res_number);
      }
      if(utils_is_substring("HEIGHT_RES<=", cs_tmp->scenario_string)) {
        cs_verified_node->eq_less_height_res = true;
        cs_verified_node->eq_less_height_res_number = a_fetch_specific_res_from_scenario_string(cs_tmp->scenario_string, "HEIGHT_RES<=");
        printf("DEV detected HEIGHT_RES<=*number* to be: %i\n", cs_verified_node->eq_less_height_res_number);
      }
      if(utils_is_substring("HEIGHT_RES>=", cs_tmp->scenario_string)) {
        cs_verified_node->eq_greater_height_res = true;
        cs_verified_node->eq_greater_height_res_number = a_fetch_specific_res_from_scenario_string(cs_verified_node->scenario_string, "HEIGHT_RES>=");
        printf("DEV detected HEIGHT_RES>=*number* to be: %i\n", cs_verified_node->eq_greater_height_res_number);
      }
      if(utils_is_substring("ELSE", cs_tmp->scenario_string)) {
        cs_verified_node->else_detected = true;
      }

      //verify ffmpeg command
      if(utils_string_is_empty_or_spaces(cs_tmp->ffmpeg_command)) {
        printf("ALIAS %s MAJOR ERROR: we found a %s_FFMPEG_COMMAND to be empty: %s\n", alias_name, alias_name, cs_tmp->ffmpeg_command);
        cs_head_error_detected = true;
      }
      if(utils_is_substring("$vfo_input", cs_tmp->ffmpeg_command) == false || utils_is_substring("$vfo_output", cs_tmp->ffmpeg_command) == false) {
        if(utils_is_substring("$vfo_input", cs_tmp->ffmpeg_command) == false) {
          printf("ALIAS %s MAJOR ERROR: we found a %s_FFMPEG_COMMAND does not contain $vfo_input\n", alias_name, alias_name);
          printf("DEV: relevant command: %s\n", cs_tmp->ffmpeg_command);
        }
        if(utils_is_substring("$vfo_output", cs_tmp->ffmpeg_command) == false) {
          printf("ALIAS %s MAJOR ERROR: we found a %s_FFMPEG_COMMAND does not contain $vfo_output\n", alias_name, alias_name);
          printf("DEV: relevant command: %s\n", cs_tmp->ffmpeg_command);
        }
        cs_head_error_detected = true;
      }
      cs_insert_at_head(&verified_cs_head, cs_verified_node);
      cs_tmp = cs_tmp->next;
    }
  }

  if(cs_head_error_detected == true) {
    printf("ALIASES MAJOR ERROR: vfo has stopped because we detected an issue with config file alias/scenario variables\n");
    exit(EXIT_FAILURE);
  }
  return verified_cs_head;
}

int a_fetch_specific_res_from_scenario_string(char *scenario_string, char *res_marker) {
  int slen = strlen(scenario_string);
  int wlen = strlen(res_marker);
  int end = slen - wlen + 1;

  char *buffer = malloc(BUFSIZ);

  for (int i = 0; i < end; i++) {
    bool res_marker_found = true;
    for (int j = 0; j < wlen; j++) {
      if(res_marker[j] != scenario_string[i + j]) {
        res_marker_found = true;
        break;
      }
    }
    // when cursor has found res_marker and is at start of variable
    if(res_marker_found) {
      //move cursor to end of found word...
      //This should be just after the end of >= or <= of res_marker
      int just_after_res_marker = i + wlen + 1;
      int just_last_char_of_number;
      //Where is the comma
      for (int k = just_after_res_marker; k <= slen; k++) {
        bool end_comma_found = true;
        for(int l = k; l <= slen; l++) {
          if(scenario_string[l] != ',') {
            end_comma_found = false;
            break;
          }
          if(end_comma_found) {
            just_last_char_of_number = l;
            int tmp = 0;
            for (int m = just_after_res_marker; m <= just_last_char_of_number; m++) {
              if(m == just_last_char_of_number) {
                buffer[tmp] = '\0';
                break;
              }
              buffer[tmp] = scenario_string[m];
              tmp++;
            }
            break;
          }
        }
        if(end_comma_found == true) break;
      }
      break;
    }
  }
  //convert buffer char* to number;
  return utils_convert_string_to_integer(buffer);
}

aliases_t* alias_insert_at_head(aliases_t **head, aliases_t *node_to_insert) {
  node_to_insert->next = *head;
  *head = node_to_insert;
  return node_to_insert;
}

void alias_insert_after_node(aliases_t *node_to_insert_after, aliases_t* newnode) {
  newnode->next = node_to_insert_after->next;
  node_to_insert_after->next = newnode;
}
