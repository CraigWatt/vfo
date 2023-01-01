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

#include "Data_Structures/a_aliases_struct.h"
#include "a_internal.h"
#include <stdio.h>
#include <stdlib.h>

/* Source to Aliases Work */

void a_source_to_aliases(aliases_t *aliases) {
  printf("ALIASes ALERT: initiating 'source_to_aliases'\n");
  //for every alias node in aliases
  if(aliases != NULL) {
    aliases_t *tmp = aliases;
    while(tmp != NULL) {
      //encode_source_to_alias();
      a_source_to_alias(tmp);
      tmp = tmp->next;
    }
  }
  printf("ALIASes ALERT: ALIASes completed successfully\n");
}
void a_source_to_alias(aliases_t *alias) {
  printf("ALIAS %s ALERT: initiating 'source_to_alias'\n", alias->name);
  /* pre-encode checks */
  a_pre_encode_checks(alias);

  /* find video folders not yet present in this alias/content */
  a_highlight_encode_candidates_to_user(alias);

  /*ask user if they wish to proceed with encoding */
  utils_wish_to_continue("'find an alias's candidates'", "'encode alias's candidates to alias'");

  /* encode */
  printf("ALIAS %s ALERT: initiating encoding.\n", alias->name);
  a_encode_source_to_an_alias(alias);
  /* post-encode checks */
  //might need to add to here
  printf("ALIAS %s ALERT: an alias completed successfully\n", alias->name);
}

void a_pre_encode_checks(aliases_t *alias) {
  printf("ALIAS %s ALERT: initiating 'pre-encode checks'\n", alias->name);
  //does alias/content folder contain valid custom folders
  utils_does_folder_contain_valid_custom_folders(alias->content, alias->cf_head);
  //is alias/content folder missing any custom folders
  utils_is_folder_missing_custom_folders(alias->content, alias->cf_head);
  /*in future I would like to be able to SAY what folders are missing */
  /* now we know all necessary folders contain necessary custom_folders */

  /*but let's check to see if each custom_folder is adhering to their custom_folder_type*/
  //are the alias/content folder custom_folders adhering to their types?
  //using mp4_original flag as alias also only uses mp4 format FOR NOW!
  utils_are_custom_folders_type_compliant(alias->content, "mp4_original", alias->cf_head);

  // maybe necessary but not sure?
  // are_custom_folders_files_alias_compliant();
  // perhaps some sort of check of the movie/tv files themselves to see if they are compliant with alias.

  //a_is_alias_out_of_sync_with_source(); - //simple meaning: if a movie/tv folder exists here that doesn't 
                                        //exist in source, tell the user.

  printf("ALIAS %s ALERT: 'pre-encode checks' completed successfully\n", alias->name);
}

void a_highlight_encode_candidates_to_user(aliases_t *alias) {
  if(strcmp(alias->source_content, "") != 0) {
    printf("ALIAS %s ALERT: found source/content\n", alias->name);
    a_highlight_encode_candidates_from_source_content(alias);
  }
}

void a_highlight_encode_candidates_from_source_content(aliases_t *alias) {
  active_cf_node_t *active_cf = utils_generate_from_to_ll(alias->cf_head, alias->source_content, alias->content);
  int alias_encode_candidates_counter = 0;
  int already_present_in_alias_counter = 0;
  if(active_cf != NULL) {
    while(active_cf != NULL) {
      active_films_f_node_t *tmp_f = active_cf->active_films_f_head;
      if(tmp_f != NULL) {
        while(tmp_f != NULL) {
          //if to_folder does exist AND if from_folder does exist
          if(utils_does_folder_exist(tmp_f->from_films_f_folder) && utils_does_folder_exist(tmp_f->to_films_f_folder))
            already_present_in_alias_counter++;
          else if(utils_does_folder_exist(tmp_f->from_films_f_folder) && !(utils_does_folder_exist(tmp_f->to_films_f_folder)))
            alias_encode_candidates_counter++;
          tmp_f = tmp_f->next;
        }
      }
      active_tv_f_node_t *tmp_tv1 = active_cf->active_tv_f_head;
      if(tmp_tv1!= NULL) {
        while(tmp_tv1 != NULL) {
          active_tv_f_node_t *tmp_tv2 = tmp_tv1->active_tv_f_head;
          if(tmp_tv2 != NULL) {
            while(tmp_tv2 != NULL) {
              active_tv_f_node_t *tmp_tv3 = tmp_tv2->active_tv_f_head;
              if(tmp_tv3 != NULL) {
                while(tmp_tv3 != NULL) {
                  //if to_folder doesn't exist AND from_folder contains valid file
                  if(utils_does_folder_exist(tmp_tv3->from_tv_f_folder) && utils_does_folder_exist(tmp_tv3->to_tv_f_folder))
                    already_present_in_alias_counter++;
                  else if(utils_does_folder_exist(tmp_tv3->from_tv_f_folder) && !(utils_does_folder_exist(tmp_tv3->to_tv_f_folder)))
                    alias_encode_candidates_counter++;
                  tmp_tv3 = tmp_tv3->next;
                }
              }
              tmp_tv2 = tmp_tv2->next;
            }
          }
          tmp_tv1 = tmp_tv1->next;
        }
      }
      active_cf = active_cf->next;
    }
  }
  printf("ALIAS %s ALERT: %i source/content -> %s/content candidates found.\n",alias->name, alias_encode_candidates_counter, alias->name);
  printf("ALIAS %s ALERT: %i will be ignored as they appear to already exist in source/content.\n", alias->name, already_present_in_alias_counter);
  free(active_cf);
  active_cf = NULL;
}

void a_encode_source_to_an_alias(aliases_t *alias) {
  if(strcmp(alias->source_content, "") != 0)
    s_encode_from_source_to_alias(alias);
}

void s_encode_from_source_to_alias(aliases_t *alias) {
  active_cf_node_t *active_cf = utils_generate_from_to_ll(alias->cf_head, alias->source_content, alias->content);
  if(active_cf != NULL) {
    while(active_cf != NULL) {
      active_films_f_node_t *tmp_f = active_cf->active_films_f_head;
      if(tmp_f != NULL) {
        while(tmp_f != NULL) {
          //if to_folder doesn't exist, 
          if(!utils_does_folder_exist(tmp_f->to_films_f_folder)) {
            //create the folder
            utils_create_folder(tmp_f->to_films_f_folder);
            //encode the video file
            if(a_execute_ffmpeg_command(tmp_f->from_films_f_folder, tmp_f->to_films_f_folder, alias) == true) {
              //command executed successfully
              printf("ALIAS %s ALERT: ffmpeg command executed successfully.\n", alias->name);
            }
            else {
              //command unsuccessful - need to generate a error txt file in source->unable_to_process
              printf("ALIAS %s WARNING: ffmpeg command FAILED.  Printing error message to %s/unable_to_process\n", alias->name, alias->name);
              printf("DEV: attempting to create error txt file in unable to process.\n");
              utils_create_error_encoding_file(alias->unable_to_process, tmp_f->from_films_f_folder);
              //attempt to remove failed encode scrap.
              if(utils_is_folder_empty(tmp_f->to_films_f_folder) == false) {
                utils_danger_delete_contents_of_folder(tmp_f->to_films_f_folder);
                utils_delete_folder_if_it_is_empty(tmp_f->to_films_f_folder);
              } 
              else
                utils_delete_folder_if_it_is_empty(tmp_f->to_films_f_folder);
            }
          }
          tmp_f = tmp_f->next;
        }
      }
      active_tv_f_node_t *tmp_tv1 = active_cf->active_tv_f_head;
      if(tmp_tv1!= NULL) {
        while(tmp_tv1 != NULL) {
          active_tv_f_node_t *tmp_tv2 = tmp_tv1->active_tv_f_head;
          if(tmp_tv2 != NULL) {
            while(tmp_tv2 != NULL) {
              active_tv_f_node_t *tmp_tv3 = tmp_tv2->active_tv_f_head;
              if(tmp_tv3 != NULL) {
                while(tmp_tv3 != NULL) {
                  //if to_folder doesn't exist
                  if(!utils_does_folder_exist(tmp_tv3->to_tv_f_folder)) {
                    //create folder
                    if(!utils_does_folder_exist(tmp_tv1->to_tv_f_folder))
                      utils_create_folder(tmp_tv1->to_tv_f_folder);
                    if(!utils_does_folder_exist(tmp_tv2->to_tv_f_folder))
                      utils_create_folder(tmp_tv2->to_tv_f_folder);
                    utils_create_folder(tmp_tv3->to_tv_f_folder);
                    //encode the video file
                    if(a_execute_ffmpeg_command(tmp_tv3->from_tv_f_folder, tmp_tv3->to_tv_f_folder, alias) == true) {
                      //command executed successfully
                      printf("ALIAS %s ALERT: ffmpeg command executed successfully.\n", alias->name);
                    }
                    else {
                      //command unsuccessful - need to generate a error txt file in source->unable_to_process
                      printf("ALIAS %s WARNING: ffmpeg command FAILED.  Printing error message to %s/unable_to_process\n", alias->name, alias->name);
                      printf("DEV: attempting to create error txt file in unable to process.\n");
                      utils_create_error_encoding_file(alias->unable_to_process, tmp_tv3->from_tv_f_folder);
                      //attempt to remove failed encode scrap.
                      if(utils_is_folder_empty(tmp_tv3->to_tv_f_folder) == false) {
                        utils_danger_delete_contents_of_folder(tmp_tv3->to_tv_f_folder);
                        utils_delete_folder_if_it_is_empty(tmp_tv3->to_tv_f_folder);
                      } 
                      else
                        utils_delete_folder_if_it_is_empty(tmp_tv3->to_tv_f_folder);
                    }
                  }
                  tmp_tv3 = tmp_tv3->next;
                }
              }
              tmp_tv2 = tmp_tv2->next;
            }
          }
          tmp_tv1 = tmp_tv1->next;
        }
      }
      active_cf = active_cf->next;
    }
  }
  free(active_cf);
  active_cf = NULL;
}

bool a_execute_ffmpeg_command(char *from, char *to, aliases_t *alias) {
  int ffmpeg_error_number = system(a_generate_ffmpeg_command(from, to, alias));
  if(ffmpeg_error_number == 0)
    return true;
  return false;
}

char* a_generate_ffmpeg_command(char *from, char *to, aliases_t *alias) {
  /* begin constructing ffmpeg command */
  char *vfo_input = utils_combine_strings("\"", utils_fetch_single_file(from));
  vfo_input = utils_combine_strings(vfo_input, "\"");
  char *vfo_output = utils_combine_strings("\"", a_generate_alias_file_name(from, to, alias->name));
  vfo_output = utils_combine_strings(vfo_output, "\"");
  
  /* we need to determine what ffmpeg we want to run from alias scenarios */

  /* to do this we need to probe the vfo_input , 
  assess the json, match with scenario, delete json, 
  execute command */

  //probe vfo_input
  if(a_execute_ffprobe(vfo_input, alias) == true) {
    printf("DEV: ffprobe command executed successfully\n");
  }
  else {
    //command unsuccessful - need to generate a error txt file in source->unable_to_process
    printf("DEV: ffprobe command FAILED.  Printing error message to %s/unable_to_process\n", alias->name);
    printf("DEV: This still needs to be implemented.\n");
    exit(EXIT_FAILURE);
  }
  //assess json
  //match with scenario
  //delete json
  char *relevant_ffmpeg_command = a_match_json_file_with_scenario(alias);
  char *input_revised_ffmpeg_command = utils_replace_characters(relevant_ffmpeg_command, "$vfo_input", vfo_input);
  char *input_and_output_revised_ffmpeg_command = utils_replace_characters(input_revised_ffmpeg_command, "$vfo_output", vfo_output);
  //execute relevant command
  return input_and_output_revised_ffmpeg_command;
}

char* a_match_json_file_with_scenario(aliases_t *alias) {
  char *tmp_json_file = utils_combine_to_full_path(alias->root, "tmp_json_file.json");
  if(!utils_does_file_exist(tmp_json_file)) {
    printf("DEV MAJOR ERROR: cannot find json file that vfo just created\n");
    exit(EXIT_FAILURE);
  }
  char *tmp_json = a_extract_json_to_string(tmp_json_file);
  remove(tmp_json_file);

  //populate placeholder json variables for ease of comparison
  char *json_codec_name = "";
  // char *json_codec_long_name = "";
  // char *json_profile = "";
  int json_width = 0;
  int json_height = 0;
  int json_bits_per_raw_sample = 0;
  // int json_level = 0;
  // char *json_color_range = "";
  char *json_color_space = "";
  // char *json_color_transfer = "";
  // char *json_color_primaries = "";

  json_codec_name = a_fetch_json_string(tmp_json, "\"codec_name\": ");
  // json_codec_long_name = a_fetch_json_string(tmp_json, "\"codec_long_name\": ");
  // json_profile = a_fetch_json_string(tmp_json, "\"profile\": ");
  // json_color_range = a_fetch_json_string(tmp_json, "\"color_range\": ");
  json_color_space = a_fetch_json_string(tmp_json, "\"color_space\": ");
  // json_color_transfer = a_fetch_json_string(tmp_json, "\"color_transfer\": ");
  // json_color_primaries = a_fetch_json_string(tmp_json, "\"color_primaries\": ");

  json_width = a_fetch_json_number(tmp_json, "\"width\": ");
  json_height = a_fetch_json_number(tmp_json, "\"height\": ");
  json_bits_per_raw_sample = a_fetch_json_number(tmp_json, "\"bits_per_raw_sample\": ");
  // json_level = a_fetch_json_number(tmp_json, "\"level\": ");

  printf("DEV: vfo found json_codec_name to be: %s\n", json_codec_name);
  printf("DEV: vfo found json_color_space to be: %s\n", json_color_space);
  printf("DEV: vfo found json_width to be: %i\n", json_width);
  printf("DEV: vfo found json_height to be: %i\n", json_height);

  /* we now know exactly what the file candidate consists of */
  /* so now, let's use logic to cycle through all possible scenarios */

  /* so prepare to cycle all scenarios for a match...*/
  cs_node_t *tmp = alias->cs_head;
  char *command_for_run = NULL;
  if(tmp != NULL) {
    while(tmp != NULL) {
      printf("currently checking this scenario: %s\n", tmp->scenario_string);
      printf("currently checking this ffmpeg command: %s\n", tmp->ffmpeg_command);

      bool may_command_be_run = false;
      // is CODEC_JUST_RIGHT specified in Scenario?
      if(tmp->codec_just_right) {
        printf("checking candidate codec matches criteria codec\n");
        printf("json_codec_name: %s\n", json_codec_name);
        printf("alias->crit_codec: %s\n", alias->crit_codec);
        //does the candidate file match alias criteria codec?
        if(strcmp(json_codec_name, alias->crit_codec) == 0) {
          may_command_be_run = true;
        } 
        else {
          tmp = tmp->next;
          continue;
        }
      }
      // is BIT_RATE_JUST RIGHT specified in Scenario?
      if(tmp->bit_rate_just_right) {
        //does the candidate file mach alias critera bit rate?
        printf("DEV: Not yet sure how to establish candidate file bit rate.\n");
        printf("DEV: Please remove 'BIT_RATE_JUST_RIGHT for now.\n");
        exit(EXIT_FAILURE);
      }
      // is COLOR_SPACE_JUST_RIGHT specified in Scenario?
      if(tmp->color_space_just_right) {
        printf("checking candidate color space matches criteria color space\n");
        printf("json_color_space: %s\n", json_color_space);
        printf("alias->crit_color_space: %s\n", alias->crit_color_space);
        //does the candidate file match alias criteria color space?
        //DEV: perhaps this should be 'json_color_primaries' instead?  unsure
        if(strcmp(json_color_space,alias->crit_color_space) == 0) {
          may_command_be_run = true;
        }
        else {
          tmp = tmp->next;
          continue;
        }
      }

      //is RES_JUST_RIGHT specified in Scenario?
      if(tmp->res_just_right) {
        printf("checking candidate resolution falls within criteria max and min\n");
        printf("json_width: %i\n", json_width);
        printf("alias->crit_min_width: %i\n", alias->crit_min_width);
        printf("alias->crit_max_width: %i\n", alias->crit_max_width);
        printf("json_height: %i\n", json_height);
        printf("alias->crit_min_height: %i\n", alias->crit_min_height);
        printf("alias->crit_max_height: %i\n", alias->crit_max_height);
        //does the candidate file match alias criteria max height & max width?
        if(json_width <= alias->crit_max_width && json_height <= alias->crit_max_height && json_width >= alias->crit_min_width && json_height >= alias->crit_min_height) {
          may_command_be_run = true;
        }
        else {
          tmp = tmp->next;
          continue;
        }
      }

      // is INCORRECT_CODEC specified in Scenario?
      if(tmp->incorrect_codec) {
        //does the candidate file NOT MATCH alias crit codec?
        if(strcmp(json_codec_name, alias->crit_codec) != 0) {
          may_command_be_run = true;
        }
        else {
          tmp = tmp->next;
          continue;
        }
      }

      // is INCORRECT_BIT_RATE specified in Scenario?
      if(tmp->incorrect_bit_rate) {
        //does the candidate file NOT MATCH alias crit bit rate?
        if(json_bits_per_raw_sample != alias->crit_bits) {
          may_command_be_run = true;
        } else {
          tmp = tmp->next;
          continue;
        }
      }

      //is INCORRECT_COLOR_SPACE specified in Scenario?
      if(tmp->incorrect_color_space) {
        //does the candidate file NOT MATCH alias crit color space?
        if(strcmp(json_color_space, alias->crit_color_space) != 0) {
          may_command_be_run = true;
        }
        else {
          tmp = tmp->next;
          continue;
        }
      }

      //is NO_VALID_COLOR_SPACE_DETECTED specified in Scenario?
      if(tmp->no_valid_color_space_detected) {
        //does the candidate file CONTAIN NO VALID INFORMATION about color space?
        if(strcmp(json_color_space, "rgb") == 0 ||
           strcmp(json_color_space, "bt709") == 0 ||
           strcmp(json_color_space, "fcc") == 0 ||
           strcmp(json_color_space, "bt470bg") == 0 ||
           strcmp(json_color_space, "smpte170m") == 0 ||
           strcmp(json_color_space, "smpte240m") == 0 ||
           strcmp(json_color_space, "ycocg") == 0 ||
           strcmp(json_color_space, "bt2020nc") == 0 ||
           strcmp(json_color_space, "bt2020_ncl") == 0 ||
           strcmp(json_color_space, "bt2020c") == 0 ||
           strcmp(json_color_space, "bt2020_cl") == 0 ||
           strcmp(json_color_space, "smpte2085") == 0 ||
           strcmp(json_color_space, "chroma-derived-nc") == 0 ||
           strcmp(json_color_space, "chroma-derived-c") == 0 ||
           strcmp(json_color_space, "ictcp") == 0) {
            tmp = tmp->next;
            continue;
        }
        else {
          may_command_be_run = true;  
        }
      }

      //is RES_TOO_LOW specified in Scenario?
      if(tmp->res_too_low) {
        //is the candidate files width or height lower than alias crit min width or height?
        if(json_width < alias->crit_min_width || json_height < alias->crit_min_height) {
          may_command_be_run = true;
        }
        else {
          tmp = tmp->next;
          continue;
        }
      }

      //is RES_TOO_HIGH specified in Scenario?
      if(tmp->res_too_high) {
        //is the candidate files width or height higher than alias crit max width or height?
        if(json_width > alias->crit_max_width || json_height > alias->crit_max_height) {
          may_command_be_run = true;
        }
        else {
          tmp = tmp->next;
          continue;
        }
      }

      //is BELOW_4K specified in Scenario?
      if(tmp->below_4k) {
        // is the candidate width below 3860 and height below 2160?
        if(json_width < 3860 && json_height < 2160) {
          may_command_be_run = true;
        }
        else {
          tmp = tmp->next;
          continue;
        }
      }

      //is 4K_OR_HIGHER specified in Scenario?
      if(tmp->eq_higher_4k) {
        //is the candidate width equal or above 3860 and height equal or above 2160?
        if(json_width >= 3860 && json_height >= 2160) {
          may_command_be_run = true;
        }
        else {
          tmp = tmp->next;
          continue;
        }
      }

      //is BELOW_FULL_HD specified in Scenario?
      if(tmp->below_full_hd) {
        //is the candidate width below 1920 and height below 1080?
        if(json_width < 1920 && json_height < 1080) {
          may_command_be_run = true;
        }
        else {
          tmp = tmp->next;
          continue;
        }
      }

      //is FULL_HD_OR_HIGHER specified in Scenario?
      if(tmp->eq_higher_full_hd) {
        //is the candidate width equal or above 1920 and height equal or above 1080?
        if(json_width >= 1920 && json_height >= 1080) {
          may_command_be_run = true;
        } else tmp = tmp->next;
      }

      //is BELOW_HALF_HD specified in Scenario?
      if(tmp->below_half_hd) {
        // is the candidate width below 1280 and height below 720?
        if(json_width < 1280 && json_height < 720) {
          may_command_be_run = true;
        }
        else {
          tmp = tmp->next;
          continue;
        }
      }

      //is HALF_HD_OR_HIGHER specified in Scenario?
      if(tmp->eq_higher_half_hd) {
        //is the candidate width equal or above 1280 and height equal or above 720?
        if(json_width >= 1280 && json_height >= 720) {
          may_command_be_run = true;
        }
        else {
          tmp = tmp->next;
          continue;
        }
      }

      //is BELOW_480p specified in Scenario?
      if(tmp->below_480p) {
        //is the candidate width below 858 and height below 480?
        if(json_width < 858 && json_height < 480) {
          may_command_be_run = true;
        }
        else {
          tmp = tmp->next;
          continue;
        }
      }

      //is 480p_OR_HIGHER specified in Scenario?
      if(tmp->eq_higher_480p) {
        //is the candidate width equal or above 858 and height equal or above 480?
        if(json_width >= 858 && json_height >= 480) {
          may_command_be_run = true;
        }
        else {
          tmp = tmp->next;
          continue;
        }
      }

      //is BELOW_360p specified in Scenario?
      if(tmp->below_360p) {
        // is the candidate width below 480 and height below 360?
        if(json_width < 480 && json_height < 360) {
          may_command_be_run = true;
        }
        else {
          tmp = tmp->next;
          continue;
        }
      }

      //is 360p_OR_HIGHER specified in Scenario?
      if(tmp->eq_higher_360p) {
        //is the candidate width equal or higher than 480 and height equal or higher than 360?
        if(json_width >= 480 && json_height >= 360) {
          may_command_be_run = true;
        }
        else {
          tmp = tmp->next;
          continue;
        }
      }

      //is BELOW_240p specified in Scenario?
      if(tmp->below_240p) {
        //is the candidate width below 352 and height below 240?
        if(json_width < 352 && json_height < 240) {
          may_command_be_run = true;
        }
        else {
          tmp = tmp->next;
          continue;
        }
      }

      //is 240p_OR_HIGHER specified in Scenario?
      if(tmp->eq_higher_240p) {
        //is the candidate width equal or above 352 and height equal or above 240?
        if(json_width >= 352 && json_height >= 240) {
          may_command_be_run = true;
        }
        else {
          tmp = tmp->next;
          continue;
        }
      }

      //is WIDTH_RES<= specified in Scenario?
      if(tmp->eq_less_width_res) {
        //is the candidate width equal or less than specified number?
        if(json_width <= tmp->eq_less_width_res_number) {
          may_command_be_run = true;
        }
        else {
          tmp = tmp->next;
          continue;
        }
      }

      //is WIDTH_RES>= specified in Scenario?
      if(tmp->eq_greater_width_res) {
        //is the candidate width equal or greater than specified number?
        if(json_width >= tmp->eq_greater_width_res_number) {
          may_command_be_run = true;
        }
        else {
          tmp = tmp->next;
          continue;
        }
      }

      //is HEIGHT_RES<= specified in Scenario?
      if(tmp->eq_less_height_res) {
        //is the candidate height equal or less than specified number?
        if(json_height <= tmp->eq_less_height_res_number) {
          may_command_be_run = true;
        }
        else {
          tmp = tmp->next;
          continue;
        }
      }

      //is HEIGHT_RES>= specified in Scenario?
      if(tmp->eq_greater_height_res) {
        //is the candidate height equal or greater than specified number?
        if(json_height >= tmp->eq_greater_height_res_number) {
          may_command_be_run = true;
        }
        else {
          tmp = tmp->next;
          continue;
        }
      }

      //is ELSE specified in Scenario?
      if(tmp->else_detected) {
        //run no matter what
        may_command_be_run = true;
      }

      if(may_command_be_run == true) {
        command_for_run = tmp->ffmpeg_command;
        printf("SCENARIO MATCH FOUND: %s\n", tmp->scenario_string);
        printf("VFO will attempt tp execute this command: %s\n", tmp->ffmpeg_command);
        sleep(3);
        break;
      }
      printf("CANDIDATE FILE DID NOT MATCH SCENARIO\n");
      tmp = tmp->next;
    }
  }
  return command_for_run;
}

int a_fetch_json_number(char *json_string, char *marker) {
  int slen = strlen(json_string);
  int wlen = strlen(marker);
  int end = slen - wlen + 1;

  char *buffer = malloc(BUFSIZ);

  for(int i = 0; i < end; i++) {
    bool content_found = true;
    for(int j = 0; j < wlen; j++) {
      if(marker[j] != json_string[i + j]) {
        content_found = false;
        break;
      }
    }
    //when cursor has found and is at start of variable...
    if(content_found) {
      //move cursor to end of found word...
      //This should be just after the 'space' character
      int just_after_first_space = i + wlen;
      int just_before_last_comma;
      //Where is the ending comma?
      for(int k = just_after_first_space; k <= slen; k++) {
        bool comma_found = true;
        for(int l = k; l <= slen; l++) {
          if(json_string[l] != ',') {
            comma_found = false;
            break;
          }
          if(comma_found) {
            just_before_last_comma = l;
            int tmp = 0;
            for(int m = just_after_first_space; m <= just_before_last_comma; m++) {
              if(m == just_before_last_comma) {
                buffer[tmp] = '\0';
                break;
              }
              buffer[tmp] = json_string[m];
              tmp++;
            }
            break;
          }
        }
        if(comma_found == true) break;
      }
    }
  }
  //got char buffer, now convert to int
  printf("fetch json number buffer string is: %s\n", buffer);
  int temp_int = utils_convert_string_to_integer(buffer);
  return temp_int;
}


char* a_fetch_json_string(char *json_string, char *marker) {
  int slen = strlen(json_string);
  int wlen = strlen(marker);
  int end = slen - wlen + 1;
  char *buffer = malloc(BUFSIZ);

  for(int i = 0; i < end; i++) {
    bool content_found = true;
    for(int j = 0; j < wlen; j++) {
      if(marker[j] != json_string[i + j]) {
        content_found = false;
        break;
      }
    }
    // when cursor has has found and is at start of variable...
    if(content_found) {
      //move cursor to end of found word...
      //This should be just after the first double quote character
      int just_after_first_quotes = i + wlen + 1;
      int just_before_last_quotes;
      //Where is the next double quote character?
      for(int k = just_after_first_quotes; k <= slen; k++) {
        bool second_quotes_found = true;
        for(int l = k; l <= slen; l++) {
          if(json_string[l] != '"') {
            second_quotes_found = false;
            break;
          }
          if(second_quotes_found) {
            just_before_last_quotes = l;
            int tmp = 0;
            for(int m = just_after_first_quotes; m <= just_before_last_quotes; m++) {
              if (m == just_before_last_quotes) {
                buffer[tmp] = '\0';
                break;
              }
              buffer[tmp] = json_string[m];
              tmp++;
            }
            break;
          }
        }
        if (second_quotes_found == true) break;
      }
    }
  }
  return buffer;  
}

char* a_extract_json_to_string(char *tmp_json_file) {
  FILE *file;
  file = fopen(tmp_json_file, "r");
  if(file == NULL) {
    printf("DEV ERROR: could not open temporary json file after internal ffprobe\n");
    printf("errno: %i\n", errno);
    if(errno == 13) {
      printf("DEV: there may be an issue with the permissions setting of JSON file.\n");
    }
    exit(EXIT_FAILURE);
  }
    size_t increase = LOWER;
  size_t allocated = increase;
  char *string = malloc(allocated);
  size_t total = 0;
  while (!feof(file) && !ferror(file)) {
    string[total] = fgetc(file);
    total++;
    if (total >= allocated) {
      if(increase >= UPPER) increase = UPPER;
      allocated += increase;
      string = realloc(string, allocated);
      increase *= 2;
    }
  }
  if (ferror(file)) {
    printf("DEV: Error reading from json file\n");
    exit(EXIT_FAILURE);
  }
  string = realloc(string, total);
  string[total - 1] = '\0';
  fclose(file);
  return(string);
}



bool a_execute_ffprobe(char *from_file, aliases_t *alias) {
  int ffprobe_error_number = system(a_generate_ffprobe_command(from_file, alias));
  if(ffprobe_error_number == 0)
    return true;

  printf("DEV: ffprobe error number: %i\n", ffprobe_error_number);
  return false;
}

char* a_generate_ffprobe_command(char *from_file, aliases_t *alias) {
  /* construct ffprobe command & tmp json file */
  char *tmp_ffprobe = "ffprobe -v quiet -print_format json -select_streams v:0 -show_entries stream=width,height,codec_name,bits_per_raw_sample,profile,level,color_space ";
  tmp_ffprobe = utils_combine_strings(tmp_ffprobe, from_file);
  tmp_ffprobe = utils_combine_strings(tmp_ffprobe, " > \"");
  char *tmp_json_file = utils_combine_to_full_path(alias->root, "tmp_json_file.json");
  tmp_ffprobe = utils_combine_strings(tmp_ffprobe, tmp_json_file);
  tmp_ffprobe = utils_combine_strings(tmp_ffprobe, "\"");
  printf("DEV: this is final tmp_ffprobe: %s\n", tmp_ffprobe);
  return tmp_ffprobe;
}

char* a_generate_alias_file_name(char *from, char *to, char *alias_name) {
  char *tmp_name = utils_fetch_single_file(from);
  tmp_name = basename(tmp_name);
  /* temporarily remove file extension from filename */
  if(utils_is_file_extension_mkv(tmp_name))
    tmp_name = utils_replace_characters(tmp_name, ".mkv", "");
  if(utils_is_file_extension_mp4(tmp_name))
    tmp_name = utils_replace_characters(tmp_name, ".mp4", "");
  /* append _alias_name to end of filename */
  tmp_name = utils_combine_strings(tmp_name, "_");
  tmp_name = utils_combine_strings(tmp_name, alias_name);
  /* add back .mp4 extension */
  tmp_name = utils_combine_strings(tmp_name, ".mp4");
  /* add name to destination */
  tmp_name = utils_combine_to_full_path(to, tmp_name);
  return tmp_name;
}



/* Original to Aliases Work */

void a_original_to_aliases(aliases_t *aliases) {
  printf("ALIASes ALERT: initiating 'original_to_aliases'\n");
  //create a ?? object and point it to relevant data in config
    //for every file in original
      //original_file_to_source();
      //for every alias node
        //encode_source_to_alias();
      //wipe_source();
}
void a_original_to_alias() {
  printf("ALIAS ALERT: initiating 'original_to_alias'\n");
}

/* Wipe Aliases Work */
void a_wipe_aliases(aliases_t *aliases) {
  printf("ALIASes ALERT: initiating 'wipe aliases'\n");
}
void a_wipe_an_alias() {
  printf("ALIAS ALERT: initiating 'wipe alias'\n");
}
