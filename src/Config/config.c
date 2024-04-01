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

#include "Data_Structures/c_cs_ll_struct.h"
#include "c_internal.h"
#include <errno.h>

config_t* con_init(const char *config_dir, char **revised_argv, int revised_argc) {
  // config struct container
  config_t *config = config_create_new_struct(); 
  /*----Populate and validate config struct -------------------*/
  /*-----------------------------------------------------------*/

  //get conf file & full conf string
  char *full_conf_path = NULL;
  char *conf_string = NULL;
  const char *config_filename = "vfo_config.conf";
  full_conf_path = con_find_config_file(config_dir, config_filename, full_conf_path);
  conf_string = con_extract_to_string(full_conf_path);
  
  //svc - extract conf_string to svc variables
  //validate svc data - all must be valid
  con_extract_to_sole_vars(conf_string, config->svm, config->svc);

  //custom folders ll - extract conf_string to cf ll
  //validate cf ll data - all must be valid
  char *cf_marker = "CUSTOM_FOLDER=";
  char *pre_approved_vfo_cf_types[]= {"films", "tv"};
  int cf_size = 2;

  config->cf_head = con_extract_to_cf_ll(conf_string, cf_marker, pre_approved_vfo_cf_types, cf_size, config->cf_head);

  //not all must be valid
  //custom alias ll //custom scenarios ll (inside alias nodes) -- extract conf_string to ca ll and within, cs ll
  char *ca_marker = "ALIAS=";
  config->ca_head = con_extract_to_ca_ll(conf_string, ca_marker, config->ca_head);

  //discard now irrelevant memory.
  free(full_conf_path);
  full_conf_path = NULL;
  free(conf_string);
  conf_string = NULL;

  /* assess uw words.  this is being done here BECAUSE ca nodes have to 
  be populated first before determining if a word*/
  bool activate_uw_work = false;
  char *pre_alias_approved_words[] = {"vfo", "original", "source", "revert", "wipe", "all_aliases", "do_it_all"};
  int pre_array_length = (sizeof pre_alias_approved_words / sizeof(char*));
  for(int i = 0; i < revised_argc; i++) {
    bool pre_approved_word_found = false;
    for(int j = 0; j < pre_array_length; j++) {
      if (strcmp(revised_argv[i], pre_alias_approved_words[j]) == 0) {
        // word matches pre approved word, all good
        pre_approved_word_found = true;
      } 
    }
    if(pre_approved_word_found == false) {
      // mark word as 'currently_unknown'
      uw_node_t *uw_tmp = NULL;
      uw_tmp = uw_create_new_node(revised_argv[i]);
      uw_insert_at_head(&config->uw_head, uw_tmp);
      activate_uw_work = true;
    }
  }

  if(activate_uw_work == true) {

    bool unknown_word_error_occurred = false;
    //foreach currently_unknown word, check to see if it matches any of ca_head
    for(int i = 0; i < uw_get_count(config->uw_head); i++) {
      //does particular unknown word exist in ca?
      char *tmp_u_word = uw_get_a_unknown_word_from_count(config->uw_head, i);
      if(ca_alias_name_exits(config->ca_head, tmp_u_word) == true) {
        // unknown_words contains a valid [alias word]
        printf("%s is a valid alias\n", tmp_u_word);
      } else {
        //ERROR, vfo has scanned conf, and we still cannot detect what this word means
        //Please make sure either the word is not a typo
        //and make sure the word matches an Alias defined in conf...
        printf("%s is NOT a valid alias\n", tmp_u_word);
        unknown_word_error_occurred = true;
      } 
    }
    if(unknown_word_error_occurred == true) {
      printf("ERROR - detected one or more unknown words.\n");
      exit(EXIT_FAILURE);
    }
  }

  //config.uw_head ll now contains only valid aliases specified by the user! 

  /*----Populate and validate config struct -------------------*/
  return config;
  /*----DONE---------------------------------------------------*/
}
ca_node_t* con_extract_to_ca_ll(char *conf_string, char *ca_marker, ca_node_t *ca_head) {
  int ca_marker_count = 0;
  ca_marker_count = con_get_marker_count(conf_string, ca_marker);
  ca_node_t *ca_tmp;
  for (int i = 1; i <= ca_marker_count; i++) {
    // may need to change the name of this fetch as i think it can be used here for alias capture as well
    char *tmpcontent = con_fetch_custom_folder_var_content(conf_string, ca_marker, i);
    // now that we have the alias name, we can define the additional necessary alias related markers:
    char *alias_uppercase = utils_uppercase_string(tmpcontent); 

    //append to start of criteria markers alias...
    char *tmp_alias_location_marker = utils_combine_strings(alias_uppercase, "_LOCATION=");
    char *tmp_crit_cod_name_marker = utils_combine_strings(alias_uppercase, "_CRITERIA_CODEC_NAME=");
    char *tmp_crit_bits_marker = utils_combine_strings(alias_uppercase, "_CRITERIA_BITS=");
    char *tmp_crit_col_space_marker = utils_combine_strings(alias_uppercase, "_CRITERIA_COLOR_SPACE=");
    char *tmp_min_width_marker = utils_combine_strings(alias_uppercase, "_CRITERIA_RESOLUTION_MIN_WIDTH=");
    char *tmp_min_height_marker = utils_combine_strings(alias_uppercase, "_CRITERIA_RESOLUTION_MIN_HEIGHT=");
    char *tmp_max_width_marker = utils_combine_strings(alias_uppercase, "_CRITERIA_RESOLUTION_MAX_WIDTH=");
    char *tmp_max_height_marker = utils_combine_strings(alias_uppercase, "_CRITERIA_RESOLUTION_MAX_HEIGHT=");

    //now fetch the content of these markers
    char *tmp_alias_location_content = con_fetch_sole_var_content(conf_string, tmp_alias_location_marker);
    char *tmp_crit_cod_name_content = con_fetch_sole_var_content(conf_string, tmp_crit_cod_name_marker);
    char *tmp_crit_bits_content = con_fetch_sole_var_content(conf_string, tmp_crit_bits_marker);
    char *tmp_crit_col_space_content = con_fetch_sole_var_content(conf_string, tmp_crit_col_space_marker);
    char *tmp_min_width_content = con_fetch_sole_var_content(conf_string, tmp_min_width_marker);
    char *tmp_min_height_content = con_fetch_sole_var_content(conf_string, tmp_min_height_marker);
    char *tmp_max_width_content = con_fetch_sole_var_content(conf_string, tmp_max_width_marker);
    char *tmp_max_height_content = con_fetch_sole_var_content(conf_string, tmp_max_height_marker);

    printf("tmp_alias_location_content: %s\n", tmp_alias_location_content);
    printf("tmp_crit_cod_name_content: %s\n", tmp_crit_cod_name_content);
    printf("tmp_crit_bits_content: %s\n", tmp_crit_bits_content);
    printf("tmp_crit_col_space_content: %s\n", tmp_crit_col_space_content);
    printf("tmp_min_width_content: %s\n", tmp_min_width_content);
    printf("tmp_min_height_content: %s\n", tmp_min_height_content);
    printf("tmp_max_width_content: %s\n", tmp_max_width_content);
    printf("tmp_max_height_content: %s\n", tmp_max_height_content);

    //asociate a link from linked list node to another linked list which is intended on storing ffmpeg & scenario configurations for a particular node alias
    char *tmp_alias_scenario_marker = utils_combine_strings(alias_uppercase, "_SCENARIO=");
    char *tmp_alias_ffmpeg_command_marker = utils_combine_strings(alias_uppercase, "_FFMPEG_COMMAND=");
    // char *tmp_alias_default_ffmpeg_command_marker = utils_combine_strings(alias_uppercase, "_DEFAULT_FFMPEG_COMMAND=");

    //now count the occurrences of these markers
    int cs_scenario_marker_count = 0;
    int cs_ffmpeg_command_marker_count = 0;
    // int cs_ffmpeg_default_command_marker_count = 0;

    cs_scenario_marker_count = con_get_atleast_one_marker_count(conf_string, tmp_alias_scenario_marker);
    cs_ffmpeg_command_marker_count = con_get_atleast_one_marker_count(conf_string, tmp_alias_ffmpeg_command_marker);
    // cs_ffmpeg_default_command_marker_count = con_get_marker_count(conf_string, tmp_alias_default_ffmpeg_command_marker);

    if(cs_scenario_marker_count != cs_ffmpeg_command_marker_count) {
      printf("ERROR, number of *alias*_SCENARIO's does not match number of *alias*_FFMPEG_COMMAND's\n");
      exit(EXIT_FAILURE);
    }

    cs_node_t *cs_head = NULL;
    for (int j = 1; j <= cs_scenario_marker_count; j++) {
      char *tmpcs_alias_scenario_content = con_fetch_custom_folder_var_content(conf_string, tmp_alias_scenario_marker, j);
      char *tmpcs_alias_ffmpeg_command_content = con_fetch_custom_folder_var_content(conf_string, tmp_alias_ffmpeg_command_marker, j);
      cs_node_t *cs_tmp;
      cs_tmp = cs_create_new_unverified_node(tmpcs_alias_scenario_content, tmpcs_alias_ffmpeg_command_content);
      cs_insert_at_head(&cs_head, cs_tmp);
    }

    //add all of the above to ca_node
    ca_tmp = ca_create_new_node(tmpcontent,
                              tmp_alias_location_content, 
                              tmp_crit_cod_name_content, 
                              tmp_crit_bits_content, 
                              tmp_crit_col_space_content,
                              tmp_min_width_content,
                              tmp_min_height_content,
                              tmp_max_width_content,
                              tmp_max_height_content,
                              cs_head);
    ca_insert_at_head(&ca_head, ca_tmp);
  }
  /*don't allow any alias name duplicates!*/
  ca_validate_alias_name_duplicates(ca_head);
  return ca_head;
}

/*
 * This function takes in param conf_string, cf_marker, cf_type & cf_head
 * The purpose of this function is to extract CUSTOM_FOLDER= variable content
 * from conf file, and store them in a ll called cf_head.
 * cf_head should be null
 * cf_type is an array of strings.  this is used to verify type matches pre-defined type
 * returns: populated cf_head 
 */
cf_node_t* con_extract_to_cf_ll(char *conf_string, char *cf_marker, char **cf_type, int cf_size, cf_node_t *cf_head) {

  //used to count the occurrence of cf_marker in conf_string
  int cf_marker_count = 0;
  //atleast one cf_marker should be present in conf string
  cf_marker_count = con_get_atleast_one_marker_count(conf_string, cf_marker);

  //for every variable marker found
  cf_head = NULL;
  bool is_every_cf_node_entirely_valid = true;
  for (int i = 0; i < cf_marker_count; i++) {
    cf_node_t *tmp = cf_create_new_empty_node();
    //fetch variable content - this is never stored
    bool tmpcontent_is_valid = false;
    char *tmpcontent = con_fetch_custom_folder_var_content(conf_string, cf_marker, i+1);
    //detect a comma, keep only string before comma
    tmp->folder_name = con_split_before_comma(tmpcontent);
    //detect a comma, keep only string after comma
    tmp->folder_type = con_split_after_comma(tmpcontent);
    /*HOW TO VERIFY ABOVE NOTES*/
    /*
      tmpcontent should:
      1. contain a single comma ,
      2. should not be empty or just spaces
      firstsplit should:
      1. contain NO commas ,
      2. should not be empty or just spaces
      secondsplit should:
      1. contain NO commas,
      2. should not be empty or just spaces
      3. should match a pre defined vfo cf type
    */

    //verify tmpcontent
    if(!utils_string_is_empty_or_spaces(tmpcontent))
      if(utils_string_contains_single_occurrence_of_char(tmpcontent, ','))
        tmpcontent_is_valid = true;

    //verify firstsplit
    if(!utils_string_is_empty_or_spaces(tmp->folder_name))
      if(!utils_string_contains_char(tmp->folder_name, ','))
        tmp->is_folder_name_valid = true;

    //verify secondsplit
    if(!utils_string_is_empty_or_spaces(tmp->folder_type))
      if(!utils_string_contains_char(tmp->folder_type, ','))
        if(utils_string_array_contains_single_occurrence_of_string(cf_type, cf_size, tmp->folder_type))
          tmp->is_folder_type_valid = true;

    //tell the user if something isn't valid
    if(tmpcontent_is_valid == false)
      printf("WARNING: the content of %s variable in config file might now be correct.  It should be something like this, %s\"'folder name','folder type'\"\n", cf_marker, cf_marker);
      
    if(tmp->is_folder_name_valid == false)
      printf("ERROR: the folder name %s found in the %s %s variable in config file is invalid.\n", tmp->folder_name, utils_fmt_ordinal(i), cf_marker);
      
    if(tmp->is_folder_type_valid == false)
      printf("ERROR: the folder type %s found in the %s %s variable in config file is invalid.\n", tmp->folder_type, utils_fmt_ordinal(i), cf_marker);

    //is cf_node entirely valid?
    if(tmp->is_folder_name_valid == true && tmp->is_folder_type_valid == true)
      tmp->is_entire_custom_folder_node_valid = true;
    else
     is_every_cf_node_entirely_valid = false;

    cf_insert_at_head(&cf_head, tmp);
  }

  //is cf_head entirely valid?
  if(is_every_cf_node_entirely_valid == false) {
    printf("vfo cannot continue unless the errors mentioned above are addressed\n");
    printf("MAJOR ERROR DETAILS: The program failed because atleast one of the detected CUSTOM FOLDER variables detected in config file was found to be invalid by vfo\n");
    exit(EXIT_FAILURE);
  }

  //return the cf_head ll
  return cf_head;
}

char* con_split_before_comma(char *string) {
  int s_length = strlen(string);
  char *rs = calloc(s_length, sizeof(char));
  int commaFound = 0;

  for(int i = 0; i <=s_length; i++) {
    if(string[i] == ',') {
      commaFound = i;
    } 
  }
  if(commaFound == 0) {
    printf("FAIL, comma not found in variable!\n");
    exit(EXIT_FAILURE);
  }
  for (int j = 0; j < commaFound; j++) {
    rs[j] = string[j];
  }
  
  rs[commaFound+1] = '\0';

  return rs;
}

char* con_split_after_comma(char *string) {
  int s_length = strlen(string);
  char *rs = calloc(s_length, sizeof(char));
  int commaFound = 0;
  for(int i = 0; i <=s_length; i++) {
    if(string[i] == ',') {
      commaFound = i + 1;
    } 
  }
  if(commaFound == 0) {
    printf("FAIL, comma not found in variable!\n");
    exit(EXIT_FAILURE);
  }
  for (int j = 0; j < commaFound; j++) {
    rs[j] = string[j + commaFound];
  }
  return rs;
}

char* con_fetch_custom_folder_var_content(char *string_conf, char *marker, int iteration) {
  int slen = strlen(string_conf);
  int wlen = strlen(marker);
  int end = slen - wlen + 1;

  int tmpCounter = 1;

  char *buffer = malloc(BUFSIZ);

  for (int i = 0; i < end; i++) {
    bool content_found = true;
    for (int j = 0; j < wlen; j++) {
      if (marker[j] != string_conf[i + j]) {
        content_found = false;
        break;
      }
    }
    // when cursor has found and is at start of variable...
    if (content_found && tmpCounter == iteration) {
      //move cursor to end of found word...
      //This should be just after the first double quote character
      int just_after_first_quotes = i + wlen + 1;
      int just_before_last_quotes;
      //Where is the next double quote character?
      for (int k = just_after_first_quotes; k <= slen ; k++) {
        bool second_quotes_found = true;
        for (int l = k; l <= slen ; l++) {
          if(string_conf[l] != '"') {
            second_quotes_found = false;
            break;
          }
          if (second_quotes_found) {
            just_before_last_quotes = l;
            int tmp = 0;
            for (int m = just_after_first_quotes; m <= just_before_last_quotes; m++) {
              if (m == just_before_last_quotes) {
                buffer[tmp] = '\0';
                break;
              }
              buffer[tmp] = string_conf[m];
              tmp++;
            }
            break;
          }      
        }
        if (second_quotes_found == true) {
          break;
        }
      }
      break;
    } else if (content_found && tmpCounter < iteration) {
      tmpCounter++;
    }
  }
  return buffer;
}

bool con_detect_a_word(char *full_string, char *word) {
  int slen = strlen(full_string);
  int wlen = strlen(word);
  int end = slen - wlen + 1;

  for (int i = 0; i < end; i++) {
    bool word_found = true;
    for (int j = 0; j < wlen; j++) {
      if(word[j] != full_string [i+j]) {
        word_found = false;
        break;
      }
    }
    if(word_found) {
      return true;
    }
  }
  return false;
}

char* con_fetch_sole_var_content(char *string_conf, char *marker) {
  int slen = strlen(string_conf);
  int wlen = strlen(marker);
  int end = slen - wlen + 1;

  char *buffer = malloc(BUFSIZ);

  for (int i = 0; i < end; i++) {
    bool content_found = true;
    for(int j = 0; j < wlen; j++) {
      if(marker[j] != string_conf[i + j]) {
        content_found = false;
        break;
      }
    }
    // when cursor has found and is at start of variable...
    if(content_found) {
      //move cursor to end of found word...
      //This should be just after the first double quote character
      int just_after_first_quotes = i + wlen + 1;
      int just_before_last_quotes;
      //Where is the next double quote character?
      for(int k = just_after_first_quotes; k <= slen; k++) {
        bool second_quotes_found = true;
        for(int l = k; l <= slen; l++) {
          if(string_conf[l] != '"') {
            second_quotes_found = false;
            break;
          }
          if(second_quotes_found) {
            just_before_last_quotes = l;
            int tmp = 0;
            for(int m = just_after_first_quotes; m <= just_before_last_quotes; m++) {
              if(m == just_before_last_quotes) {
                buffer[tmp] = '\0';
                break;
              }
              buffer[tmp] = string_conf[m];
              tmp++;
            }
            break;
          }      
        }
        if(second_quotes_found == true) break;
      }
    }
  }
  return buffer;
}

void con_extract_to_sole_vars(char* conf_string, sole_var_markers_t *svm, sole_var_content_t *svc) {
  //each should only be present once in vfo_config.conf
  con_get_sole_marker_count(conf_string, svm->original_location);
  con_get_sole_marker_count(conf_string, svm->source_location);
  con_get_sole_marker_count(conf_string, svm->keep_source);
  con_get_sole_marker_count(conf_string, svm->source_test_active_marker);
  con_get_sole_marker_count(conf_string, svm->source_test_trim_start_marker);
  con_get_sole_marker_count(conf_string, svm->source_test_trim_duration_marker);

  /*fetch each sole_var_content from each sole_var_marker*/
  //fetch svc->original_location
  svc->original_location = con_fetch_sole_var_content(conf_string, svm->original_location);
  //fetch svc_source_location
  svc->source_location = con_fetch_sole_var_content(conf_string, svm->source_location);
  //fetch char *tmp_bool_word
  char *tmp_bool_word = con_fetch_sole_var_content(conf_string, svm->keep_source);

  char *tmp_bool_word_source_test = con_fetch_sole_var_content(conf_string, svm->source_test_active_marker);

  char *tmp_source_test_trim_start = con_fetch_sole_var_content(conf_string, svm->source_test_trim_start_marker);

  char *tmp_source_test_trim_duration = con_fetch_sole_var_content(conf_string, svm->source_test_trim_duration_marker);
  /* HOW TO VERIFY ABOVE NOTES:*/
  /*  svc->original_location AND 
      svc->source_location
  should:
    1. never be null
    2. never just be empty spaces
    3. be a real location on current machine */

  /* tmp_bool_word 
  should:
    1. never be null
    2. never just be empty spaces
    3. has to be either TRUE or FALSE
    we can make use of utils_uppercase to help the user avoid issues here */
  //verify svc->original_location
  if(!(utils_string_is_empty_or_spaces(svc->original_location))) 
    if(utils_does_folder_exist(svc->original_location))
      svc->is_original_location_valid = true;
  //verify svc->source_location
  if(!(utils_string_is_empty_or_spaces(svc->source_location)))
    if(utils_does_folder_exist(svc->source_location))
      svc->is_source_location_valid = true;
  //verify tmp_bool_word / svc->keep_source
  tmp_bool_word = utils_lowercase_string(tmp_bool_word);
  if(strcmp(tmp_bool_word,"true") == 0 || strcmp(tmp_bool_word,"false") == 0) {
    if(strcmp(tmp_bool_word,"true") == 0)
      svc->keep_source = true;
    if(strcmp(tmp_bool_word,"false") == 0)
      svc->keep_source = false;
    svc->is_keep_source_valid = true;
  }

  //verify tmp_bool_word_source_test / svc->source_test
  tmp_bool_word_source_test = utils_lowercase_string(tmp_bool_word_source_test);
  if(strcmp(tmp_bool_word_source_test,"true") == 0 || strcmp(tmp_bool_word_source_test,"false") == 0) {
    if(strcmp(tmp_bool_word_source_test,"true") == 0)
      svc->source_test = true;
    if(strcmp(tmp_bool_word_source_test,"false") == 0)
      svc->source_test = false;
    svc->is_source_test_valid = true;
  }

  //only check trim start and duration if source_test is true
  if(svc->source_test == true) {
    //verify source_test_trim_start / svc->
    if(!(utils_string_is_empty_or_spaces(tmp_source_test_trim_start)) == 0)
      if(utils_string_only_contains_number_characters(tmp_source_test_trim_start) == true) {
        svc->source_test_trim_start = utils_convert_string_to_integer(tmp_source_test_trim_start);
         svc->is_source_test_start_valid = true;
      }
    if(!(utils_string_is_empty_or_spaces(tmp_source_test_trim_duration)) == 0)
      if(utils_string_only_contains_number_characters(tmp_source_test_trim_duration) == true) {
        svc->source_test_trim_duration = utils_convert_string_to_integer(tmp_source_test_trim_duration);
         svc->is_source_test_duration_valid = true;
      }
  }

  //tell the user if something isn't valid
  if(svc->is_original_location_valid == false)
    printf("ERROR: the content of %s variable in config file is invalid\n", svm->original_location);
  if(svc->is_source_location_valid == false)
    printf("ERROR: the content of %s variable in config file is invalid\n", svm->source_location);
  if(svc->is_keep_source_valid == false) 
    printf("ERROR: the content of %s variable in config file is invalid\n", svm->keep_source);
  //is svc entirely valid?
  if(svc->is_original_location_valid && svc->is_source_location_valid && svc->is_keep_source_valid && svc->is_source_test_valid)
    if(svc->source_test == false)
      svc->is_entire_svc_valid = true;
    else {
     if(svc->is_source_test_start_valid && svc->is_source_test_duration_valid)
      svc->is_entire_svc_valid = true;
    }
  else {
    printf("vfo can not continue unless the errors mentioned above are addressed.\n");
    printf("MAJOR ERROR DETAILS: The program failed because an essential variable required from config file was found to be invalid by vfo.\n");
    exit(EXIT_FAILURE);
  }
}

void con_get_sole_marker_count(char *string_conf, char *word) {
  int essential_marker_count = con_word_count(string_conf, word);
  if(essential_marker_count != 1) {
    printf("ERROR: There should only be 1 %s in config file.\n", word);
    printf("vfo detected more than 1!\n");
    exit(EXIT_FAILURE);
  }
}

int con_get_atleast_one_marker_count(char *string_conf, char *word) {
  int marker_count = con_word_count(string_conf, word);
  // printf("counted %i of %s \n", marker_count, word);
  if(!(marker_count >= 1)) {
    printf("ERROR: could not find atleast one count of %s \n", word);
    printf("MAJOR ERROR DETAILS: The program failed because vfo requires at least one of these variables in config file: %s \n", word);
    exit(EXIT_FAILURE);
  }
  return marker_count;
}

int con_get_marker_count(char *string_conf, char *word) {
  int marker_count = con_word_count(string_conf, word);
  return marker_count;
}

/*
 * Counts number of times a word is found in a string
 * returns count
 */
int con_word_count(char *string_conf, char *word) {
  int slen = strlen(string_conf);
  int wlen = strlen(word);
  int end = slen - wlen + 1;
  int count = 0;
  for (int i = 0; i < end; i++) {
    bool word_found = true;
    for (int j = 0; j< wlen; j++) {
      if (word[j] != string_conf [i + j]) {
        word_found = false;
        break;
      }
    }
    if (word_found) count++;
  }
  return count;
}

char* con_find_config_file(const char *config_dir, const char *config_filename, char *full_conf_path) {
  full_conf_path = utils_combine_to_full_path(config_dir, config_filename);
  con_does_config_file_exist(full_conf_path);  
  return full_conf_path;
}

void con_does_config_file_exist(char *location) {
  if(!utils_does_file_exist(location)) {  
    c_mes_find_config_file_fail();
    exit(EXIT_FAILURE);
  }
}

char* con_extract_to_string(char *full_conf_path) {
  FILE *file;
  file = fopen(full_conf_path, "r");
  printf("CONFIG FILE ALERT: full config path is: %s\n", full_conf_path);
  if(file == NULL) {
    printf("Error opening config file!\n");
    printf("errno: %i\n", errno);
    if(errno == 13) {
      printf("there may be an issue with the permissions setting of config file.  try running 'sudo vfo [your command]\n");
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
    printf("Error reading from file!\n");
    exit(EXIT_FAILURE);
  }
  string = realloc(string, total);
  string[total - 1] = '\0';
  fclose(file);
  return(string);
}
