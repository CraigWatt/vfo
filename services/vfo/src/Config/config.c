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
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <strings.h>

static bool con_lenient_location_validation = false;

void con_set_lenient_location_validation(bool enabled) {
  con_lenient_location_validation = enabled;
}

static bool con_marker_at_line_start(char *string_conf, int marker_index) {
  int cursor = marker_index - 1;

  if(marker_index <= 0)
    return true;

  while(cursor >= 0) {
    if(string_conf[cursor] == '\n' || string_conf[cursor] == '\r')
      return true;
    if(isspace((unsigned char)string_conf[cursor]) == 0)
      return false;
    cursor--;
  }

  return true;
}

static char* con_fetch_optional_sole_var_content(char *string_conf, char *marker) {
  int marker_count = con_get_marker_count(string_conf, marker);
  if(marker_count == 0) {
    char *empty = malloc(1);
    empty[0] = '\0';
    return empty;
  }
  if(marker_count > 1) {
    printf("ERROR: There should only be 1 %s in config file.\n", marker);
    printf("vfo detected more than 1!\n");
    exit(EXIT_FAILURE);
  }
  return con_fetch_sole_var_content(string_conf, marker);
}

static char* con_trim_copy(const char *value) {
  size_t start = 0;
  size_t end = 0;
  char *trimmed = NULL;
  size_t out_length = 0;

  if(value == NULL) {
    trimmed = malloc(1);
    trimmed[0] = '\0';
    return trimmed;
  }

  while(value[start] != '\0' && isspace((unsigned char)value[start])) {
    start++;
  }

  end = strlen(value);
  while(end > start && isspace((unsigned char)value[end - 1])) {
    end--;
  }

  out_length = end - start;
  trimmed = malloc(out_length + 1);
  if(out_length > 0)
    memcpy(trimmed, value + start, out_length);
  trimmed[out_length] = '\0';
  return trimmed;
}

static int con_split_semicolon_list(char *value, char ***items_out) {
  int count = 0;
  char *copied = NULL;
  char *token = NULL;
  char *save_ptr = NULL;
  char **items = NULL;

  *items_out = NULL;
  if(value == NULL)
    return 0;

  copied = strdup(value);
  token = strtok_r(copied, ";", &save_ptr);
  while(token != NULL) {
    char *trimmed = con_trim_copy(token);
    if(trimmed[0] != '\0') {
      items = realloc(items, sizeof(char*) * (size_t)(count + 1));
      items[count] = trimmed;
      count++;
    } else {
      free(trimmed);
    }
    token = strtok_r(NULL, ";", &save_ptr);
  }

  free(copied);
  *items_out = items;
  return count;
}

static void con_validate_location_csv(char *csv, char *label) {
  char **locations = NULL;
  int location_count = con_split_semicolon_list(csv, &locations);

  if(location_count == 0) {
    printf("ERROR: %s must include at least one location when provided.\n", label);
    exit(EXIT_FAILURE);
  }

  for(int i = 0; i < location_count; i++) {
    if(utils_does_folder_exist(locations[i]) == false) {
      if(con_lenient_location_validation) {
        printf("WARN: %s contains a location that does not exist: %s (continuing in lenient validation mode)\n", label, locations[i]);
      } else {
        printf("ERROR: %s contains a location that does not exist: %s\n", label, locations[i]);
        exit(EXIT_FAILURE);
      }
    }
  }

  for(int i = 0; i < location_count; i++)
    free(locations[i]);
  free(locations);
}

static void con_validate_usage_cap_csv(char *csv, char *label, int expected_count) {
  char **caps = NULL;
  int cap_count = 0;

  if(utils_string_is_empty_or_spaces(csv))
    return;

  cap_count = con_split_semicolon_list(csv, &caps);
  if(cap_count != expected_count) {
    printf("ERROR: %s count (%i) must match location count (%i)\n", label, cap_count, expected_count);
    exit(EXIT_FAILURE);
  }

  for(int i = 0; i < cap_count; i++) {
    if(utils_string_only_contains_number_characters(caps[i]) == false) {
      printf("ERROR: %s contains a non-numeric cap value: %s\n", label, caps[i]);
      exit(EXIT_FAILURE);
    }

    int cap = utils_convert_string_to_integer(caps[i]);
    if(cap < 1 || cap > 100) {
      printf("ERROR: %s cap values must be between 1 and 100: %s\n", label, caps[i]);
      exit(EXIT_FAILURE);
    }
  }

  for(int i = 0; i < cap_count; i++)
    free(caps[i]);
  free(caps);
}

static void con_parse_optional_bool_marker(char *raw_value,
                                           bool default_value,
                                           bool *target_value,
                                           bool *target_valid,
                                           const char *label) {
  if(utils_string_is_empty_or_spaces(raw_value)) {
    *target_value = default_value;
    *target_valid = true;
    return;
  }

  if(strcasecmp(raw_value, "true") == 0) {
    *target_value = true;
    *target_valid = true;
    return;
  }
  if(strcasecmp(raw_value, "false") == 0) {
    *target_value = false;
    *target_valid = true;
    return;
  }

  *target_valid = false;
  printf("ERROR: the content of %s variable in config file is invalid (expected true/false)\n", label);
}

static bool con_parse_non_negative_double(const char *value, double *parsed_out) {
  char *end_ptr = NULL;
  double parsed = 0.0;

  if(value == NULL)
    return false;

  errno = 0;
  parsed = strtod(value, &end_ptr);
  if(errno != 0 || end_ptr == value || *end_ptr != '\0')
    return false;
  if(!isfinite(parsed) || parsed < 0.0)
    return false;

  *parsed_out = parsed;
  return true;
}

static void con_parse_optional_non_negative_double_marker(char *raw_value,
                                                          double default_value,
                                                          double *target_value,
                                                          bool *target_valid,
                                                          const char *label) {
  double parsed = 0.0;

  if(utils_string_is_empty_or_spaces(raw_value)) {
    *target_value = default_value;
    *target_valid = true;
    return;
  }

  if(con_parse_non_negative_double(raw_value, &parsed)) {
    *target_value = parsed;
    *target_valid = true;
    return;
  }

  *target_valid = false;
  printf("ERROR: the content of %s variable in config file is invalid (expected non-negative number)\n", label);
}

static void con_parse_optional_non_negative_int_marker(char *raw_value,
                                                       int default_value,
                                                       int *target_value,
                                                       bool *target_valid,
                                                       const char *label) {
  char *end_ptr = NULL;
  long parsed = 0;

  if(utils_string_is_empty_or_spaces(raw_value)) {
    *target_value = default_value;
    *target_valid = true;
    return;
  }

  errno = 0;
  parsed = strtol(raw_value, &end_ptr, 10);
  if(errno != 0 || end_ptr == raw_value || *end_ptr != '\0' || parsed < 0 || parsed > INT_MAX) {
    *target_valid = false;
    printf("ERROR: the content of %s variable in config file is invalid (expected non-negative integer)\n", label);
    return;
  }

  *target_value = (int)parsed;
  *target_valid = true;
}

static void con_parse_optional_reference_layer_marker(char *raw_value,
                                                      char **target_value,
                                                      bool *target_valid,
                                                      const char *label) {
  if(utils_string_is_empty_or_spaces(raw_value)) {
    *target_value = "auto";
    *target_valid = true;
    return;
  }

  raw_value = utils_lowercase_string(raw_value);
  if(strcmp(raw_value, "auto") == 0
     || strcmp(raw_value, "source") == 0
     || strcmp(raw_value, "mezzanine") == 0) {
    *target_value = raw_value;
    *target_valid = true;
    return;
  }

  *target_valid = false;
  printf("ERROR: the content of %s variable in config file is invalid (expected auto|source|mezzanine)\n", label);
}

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
  //custom profile ll //custom scenarios ll (inside profile nodes) -- extract conf_string to ca ll and within, cs ll
  char *ca_marker = "ALIAS=";
  int alias_marker_count = con_get_marker_count(conf_string, "ALIAS=");
  int profile_marker_count = con_get_marker_count(conf_string, "PROFILE=");
  if(alias_marker_count > 0 && profile_marker_count > 0) {
    printf("ERROR: config cannot mix canonical and deprecated profile markers. Please use PROFILE= only.\n");
    exit(EXIT_FAILURE);
  }
  if(profile_marker_count > 0)
    ca_marker = "PROFILE=";
  config->ca_head = con_extract_to_ca_ll(conf_string, ca_marker, config->ca_head);

  //discard now irrelevant memory.
  free(full_conf_path);
  full_conf_path = NULL;
  free(conf_string);
  conf_string = NULL;

  /* assess uw words.  this is being done here BECAUSE ca nodes have to 
  be populated first before determining if a word*/
  bool activate_uw_work = false;
  char *pre_alias_approved_words[] = {"vfo", "mezzanine", "original", "source", "revert", "wipe", "profiles", "all_aliases", "do_it_all", "run", "doctor", "wizard", "show", "status", "status-json", "status_json", "visualize", "mezzanine-clean", "mezzanine_clean", "-o", "--open"};
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
        // unknown_words contains a valid [profile word]
        printf("%s is a valid profile\n", tmp_u_word);
      } else {
        //ERROR, vfo has scanned conf, and we still cannot detect what this word means
        //Please make sure either the word is not a typo
        //and make sure the word matches a PROFILE defined in conf...
        printf("%s is NOT a valid profile\n", tmp_u_word);
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
    char *tmp_alias_locations_marker = utils_combine_strings(alias_uppercase, "_LOCATIONS=");
    char *tmp_alias_location_max_usage_pct_marker = utils_combine_strings(alias_uppercase, "_LOCATION_MAX_USAGE_PCT=");
    char *tmp_crit_cod_name_marker = utils_combine_strings(alias_uppercase, "_CRITERIA_CODEC_NAME=");
    char *tmp_crit_bits_marker = utils_combine_strings(alias_uppercase, "_CRITERIA_BITS=");
    char *tmp_crit_col_space_marker = utils_combine_strings(alias_uppercase, "_CRITERIA_COLOR_SPACE=");
    char *tmp_min_width_marker = utils_combine_strings(alias_uppercase, "_CRITERIA_RESOLUTION_MIN_WIDTH=");
    char *tmp_min_height_marker = utils_combine_strings(alias_uppercase, "_CRITERIA_RESOLUTION_MIN_HEIGHT=");
    char *tmp_max_width_marker = utils_combine_strings(alias_uppercase, "_CRITERIA_RESOLUTION_MAX_WIDTH=");
    char *tmp_max_height_marker = utils_combine_strings(alias_uppercase, "_CRITERIA_RESOLUTION_MAX_HEIGHT=");

    //now fetch the content of these markers
    char *tmp_alias_location_content = con_fetch_sole_var_content(conf_string, tmp_alias_location_marker);
    char *tmp_alias_locations_content = con_fetch_optional_sole_var_content(conf_string, tmp_alias_locations_marker);
    char *tmp_alias_location_max_usage_pct_content = con_fetch_optional_sole_var_content(conf_string, tmp_alias_location_max_usage_pct_marker);
    char *tmp_crit_cod_name_content = con_fetch_sole_var_content(conf_string, tmp_crit_cod_name_marker);
    char *tmp_crit_bits_content = con_fetch_sole_var_content(conf_string, tmp_crit_bits_marker);
    char *tmp_crit_col_space_content = con_fetch_sole_var_content(conf_string, tmp_crit_col_space_marker);
    char *tmp_min_width_content = con_fetch_sole_var_content(conf_string, tmp_min_width_marker);
    char *tmp_min_height_content = con_fetch_sole_var_content(conf_string, tmp_min_height_marker);
    char *tmp_max_width_content = con_fetch_sole_var_content(conf_string, tmp_max_width_marker);
    char *tmp_max_height_content = con_fetch_sole_var_content(conf_string, tmp_max_height_marker);

    if(utils_string_is_empty_or_spaces(tmp_alias_locations_content) == false) {
      con_validate_location_csv(tmp_alias_locations_content, tmp_alias_locations_marker);
      char **alias_locations = NULL;
      int alias_location_count = con_split_semicolon_list(tmp_alias_locations_content, &alias_locations);
      if(alias_location_count > 0)
        tmp_alias_location_content = alias_locations[0];
      con_validate_usage_cap_csv(tmp_alias_location_max_usage_pct_content,
                                 tmp_alias_location_max_usage_pct_marker,
                                 alias_location_count);
      for(int x = 1; x < alias_location_count; x++)
        free(alias_locations[x]);
      free(alias_locations);
    } else {
      if(utils_does_folder_exist(tmp_alias_location_content) == false) {
        if(con_lenient_location_validation) {
          printf("WARN: profile location does not exist: %s (continuing in lenient validation mode)\n", tmp_alias_location_content);
        } else {
          printf("ERROR: profile location does not exist: %s\n", tmp_alias_location_content);
          exit(EXIT_FAILURE);
        }
      }
      tmp_alias_locations_content = tmp_alias_location_content;
      con_validate_usage_cap_csv(tmp_alias_location_max_usage_pct_content,
                                 tmp_alias_location_max_usage_pct_marker,
                                 1);
    }

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
      printf("ERROR: number of *PROFILE*_SCENARIO markers must match *PROFILE*_FFMPEG_COMMAND markers\n");
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
                              tmp_alias_locations_content,
                              tmp_alias_location_max_usage_pct_content,
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
  buffer[0] = '\0';

  for (int i = 0; i < end; i++) {
    bool content_found = true;
    for (int j = 0; j < wlen; j++) {
      if (marker[j] != string_conf[i + j]) {
        content_found = false;
        break;
      }
    }
    if(content_found && con_marker_at_line_start(string_conf, i) == false)
      content_found = false;

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
    if(word_found && con_marker_at_line_start(full_string, i) == false)
      word_found = false;

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
  buffer[0] = '\0';

  for (int i = 0; i < end; i++) {
    bool content_found = true;
    for(int j = 0; j < wlen; j++) {
      if(marker[j] != string_conf[i + j]) {
        content_found = false;
        break;
      }
    }
    if(content_found && con_marker_at_line_start(string_conf, i) == false)
      content_found = false;

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
      break;
    }
  }
  return buffer;
}

void con_extract_to_sole_vars(char* conf_string, sole_var_markers_t *svm, sole_var_content_t *svc) {
  const char *mezzanine_location_marker = "MEZZANINE_LOCATION=";
  const char *mezzanine_locations_marker = "MEZZANINE_LOCATIONS=";
  const char *mezzanine_location_max_usage_pct_marker = "MEZZANINE_LOCATION_MAX_USAGE_PCT=";

  int original_location_count = con_get_marker_count(conf_string, svm->original_location);
  int original_locations_count = con_get_marker_count(conf_string, svm->original_locations);
  int mezzanine_location_count = con_get_marker_count(conf_string, (char *)mezzanine_location_marker);
  int mezzanine_locations_count = con_get_marker_count(conf_string, (char *)mezzanine_locations_marker);
  int original_location_max_usage_pct_count = con_get_marker_count(conf_string, svm->original_location_max_usage_pct);
  int mezzanine_location_max_usage_pct_count = con_get_marker_count(conf_string, (char *)mezzanine_location_max_usage_pct_marker);

  int source_location_count = con_get_marker_count(conf_string, svm->source_location);
  int source_locations_count = con_get_marker_count(conf_string, svm->source_locations);

  const char *primary_mezzanine_location_marker = mezzanine_location_count == 1
                                                    ? mezzanine_location_marker
                                                    : svm->original_location;
  const char *primary_mezzanine_locations_marker = mezzanine_locations_count == 1
                                                    ? mezzanine_locations_marker
                                                    : svm->original_locations;
  const char *primary_mezzanine_location_max_usage_pct_marker = mezzanine_location_max_usage_pct_count == 1
                                                                  ? mezzanine_location_max_usage_pct_marker
                                                                  : svm->original_location_max_usage_pct;
  int primary_mezzanine_location_count = mezzanine_location_count == 1
                                          ? mezzanine_location_count
                                          : original_location_count;
  int primary_mezzanine_locations_count = mezzanine_locations_count == 1
                                           ? mezzanine_locations_count
                                           : original_locations_count;

  if(original_location_count > 1 || original_locations_count > 1
      || mezzanine_location_count > 1 || mezzanine_locations_count > 1) {
    printf("ERROR: MEZZANINE_LOCATION/MEZZANINE_LOCATIONS may only be defined once each.\n");
    exit(EXIT_FAILURE);
  }
  if((mezzanine_location_count > 0 && original_location_count > 0)
      || (mezzanine_locations_count > 0 && original_locations_count > 0)) {
    printf("ERROR: config cannot mix canonical and deprecated mezzanine keys. Please use MEZZANINE_*.\n");
    exit(EXIT_FAILURE);
  }
  if(source_location_count > 1 || source_locations_count > 1) {
    printf("ERROR: SOURCE_LOCATION/SOURCE_LOCATIONS may only be defined once each.\n");
    exit(EXIT_FAILURE);
  }
  if(primary_mezzanine_location_count == 0 && primary_mezzanine_locations_count == 0) {
    printf("ERROR: config must include MEZZANINE_LOCATION or MEZZANINE_LOCATIONS.\n");
    exit(EXIT_FAILURE);
  }
  if(source_location_count == 0 && source_locations_count == 0) {
    printf("ERROR: config must include SOURCE_LOCATION or SOURCE_LOCATIONS.\n");
    exit(EXIT_FAILURE);
  }

  //each should only be present once in vfo_config.conf when provided
  con_get_sole_marker_count(conf_string, svm->keep_source);
  con_get_sole_marker_count(conf_string, svm->source_test_active_marker);
  con_get_sole_marker_count(conf_string, svm->source_test_trim_start_marker);
  con_get_sole_marker_count(conf_string, svm->source_test_trim_duration_marker);
  if(original_location_max_usage_pct_count > 1 || mezzanine_location_max_usage_pct_count > 1) {
    printf("ERROR: There should only be 1 %s in config file.\n", primary_mezzanine_location_max_usage_pct_marker);
    exit(EXIT_FAILURE);
  }
  if(original_location_max_usage_pct_count > 0 && mezzanine_location_max_usage_pct_count > 0) {
    printf("ERROR: config cannot mix %s and %s.\n",
           svm->original_location_max_usage_pct,
           mezzanine_location_max_usage_pct_marker);
    exit(EXIT_FAILURE);
  }
  if(con_get_marker_count(conf_string, svm->source_location_max_usage_pct) > 1) {
    printf("ERROR: There should only be 1 %s in config file.\n", svm->source_location_max_usage_pct);
    exit(EXIT_FAILURE);
  }

  /*fetch each sole_var_content from each sole_var_marker*/
  if(primary_mezzanine_location_count == 1)
    svc->original_location = con_fetch_sole_var_content(conf_string, (char *)primary_mezzanine_location_marker);
  else {
    svc->original_location = malloc(1);
    svc->original_location[0] = '\0';
  }

  if(source_location_count == 1)
    svc->source_location = con_fetch_sole_var_content(conf_string, svm->source_location);
  else {
    svc->source_location = malloc(1);
    svc->source_location[0] = '\0';
  }

  svc->original_locations = con_fetch_optional_sole_var_content(conf_string, (char *)primary_mezzanine_locations_marker);
  svc->source_locations = con_fetch_optional_sole_var_content(conf_string, svm->source_locations);
  svc->original_location_max_usage_pct = con_fetch_optional_sole_var_content(conf_string, (char *)primary_mezzanine_location_max_usage_pct_marker);
  svc->source_location_max_usage_pct = con_fetch_optional_sole_var_content(conf_string, svm->source_location_max_usage_pct);

  if(utils_string_is_empty_or_spaces(svc->original_locations))
    svc->original_locations = svc->original_location;
  if(utils_string_is_empty_or_spaces(svc->source_locations))
    svc->source_locations = svc->source_location;

  if(utils_string_is_empty_or_spaces(svc->original_locations) == false) {
    char **locations = NULL;
    int location_count = con_split_semicolon_list(svc->original_locations, &locations);
    con_validate_location_csv(svc->original_locations, (char *)primary_mezzanine_locations_marker);
    con_validate_usage_cap_csv(svc->original_location_max_usage_pct, (char *)primary_mezzanine_location_max_usage_pct_marker, location_count);
    if(location_count > 0)
      svc->original_location = locations[0];
    for(int i = 1; i < location_count; i++)
      free(locations[i]);
    free(locations);
  }

  if(utils_string_is_empty_or_spaces(svc->source_locations) == false) {
    char **locations = NULL;
    int location_count = con_split_semicolon_list(svc->source_locations, &locations);
    con_validate_location_csv(svc->source_locations, svm->source_locations);
    con_validate_usage_cap_csv(svc->source_location_max_usage_pct, svm->source_location_max_usage_pct, location_count);
    if(location_count > 0)
      svc->source_location = locations[0];
    for(int i = 1; i < location_count; i++)
      free(locations[i]);
    free(locations);
  }

  //fetch char *tmp_bool_word
  char *tmp_bool_word = con_fetch_sole_var_content(conf_string, svm->keep_source);

  char *tmp_bool_word_source_test = con_fetch_sole_var_content(conf_string, svm->source_test_active_marker);

  char *tmp_source_test_trim_start = con_fetch_sole_var_content(conf_string, svm->source_test_trim_start_marker);
  char *tmp_source_test_trim_duration = con_fetch_sole_var_content(conf_string, svm->source_test_trim_duration_marker);
  char *tmp_mezzanine_clean_enabled = con_fetch_optional_sole_var_content(conf_string, svm->mezzanine_clean_enabled_marker);
  char *tmp_mezzanine_clean_apply_changes = con_fetch_optional_sole_var_content(conf_string, svm->mezzanine_clean_apply_changes_marker);
  char *tmp_mezzanine_clean_append_media_tags = con_fetch_optional_sole_var_content(conf_string, svm->mezzanine_clean_append_media_tags_marker);
  char *tmp_mezzanine_clean_strict_quality_gate = con_fetch_optional_sole_var_content(conf_string, svm->mezzanine_clean_strict_quality_gate_marker);
  char *tmp_quality_check_enabled = con_fetch_optional_sole_var_content(conf_string, svm->quality_check_enabled_marker);
  char *tmp_quality_check_include_vmaf = con_fetch_optional_sole_var_content(conf_string, svm->quality_check_include_vmaf_marker);
  char *tmp_quality_check_strict_gate = con_fetch_optional_sole_var_content(conf_string, svm->quality_check_strict_gate_marker);
  char *tmp_quality_check_reference_layer = con_fetch_optional_sole_var_content(conf_string, svm->quality_check_reference_layer_marker);
  char *tmp_quality_check_min_psnr = con_fetch_optional_sole_var_content(conf_string, svm->quality_check_min_psnr_marker);
  char *tmp_quality_check_min_ssim = con_fetch_optional_sole_var_content(conf_string, svm->quality_check_min_ssim_marker);
  char *tmp_quality_check_min_vmaf = con_fetch_optional_sole_var_content(conf_string, svm->quality_check_min_vmaf_marker);
  char *tmp_quality_check_max_files_per_profile = con_fetch_optional_sole_var_content(conf_string, svm->quality_check_max_files_per_profile_marker);
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
  if(!(utils_string_is_empty_or_spaces(svc->original_location)))
    if(utils_does_folder_exist(svc->original_location) || con_lenient_location_validation)
      svc->is_original_location_valid = true;
  if(!(utils_string_is_empty_or_spaces(svc->source_location)))
    if(utils_does_folder_exist(svc->source_location) || con_lenient_location_validation)
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
    if(utils_string_is_empty_or_spaces(tmp_source_test_trim_start) == false) {
      if(utils_string_is_ffmpeg_timecode_compliant(tmp_source_test_trim_start) == true) {
        svc->source_test_trim_start = tmp_source_test_trim_start;
         svc->is_source_test_start_valid = true;
      }
    }
    if(utils_string_is_empty_or_spaces(tmp_source_test_trim_duration) == false) {
      if(utils_string_is_ffmpeg_timecode_compliant(tmp_source_test_trim_duration) == true) {
        svc->source_test_trim_duration = tmp_source_test_trim_duration;
         svc->is_source_test_duration_valid = true;
      }
    }
  }

  con_parse_optional_bool_marker(tmp_mezzanine_clean_enabled,
                                 false,
                                 &svc->mezzanine_clean_enabled,
                                 &svc->is_mezzanine_clean_enabled_valid,
                                 svm->mezzanine_clean_enabled_marker);
  con_parse_optional_bool_marker(tmp_mezzanine_clean_apply_changes,
                                 false,
                                 &svc->mezzanine_clean_apply_changes,
                                 &svc->is_mezzanine_clean_apply_changes_valid,
                                 svm->mezzanine_clean_apply_changes_marker);
  con_parse_optional_bool_marker(tmp_mezzanine_clean_append_media_tags,
                                 true,
                                 &svc->mezzanine_clean_append_media_tags,
                                 &svc->is_mezzanine_clean_append_media_tags_valid,
                                 svm->mezzanine_clean_append_media_tags_marker);
  con_parse_optional_bool_marker(tmp_mezzanine_clean_strict_quality_gate,
                                 false,
                                 &svc->mezzanine_clean_strict_quality_gate,
                                 &svc->is_mezzanine_clean_strict_quality_gate_valid,
                                 svm->mezzanine_clean_strict_quality_gate_marker);
  con_parse_optional_bool_marker(tmp_quality_check_enabled,
                                 false,
                                 &svc->quality_check_enabled,
                                 &svc->is_quality_check_enabled_valid,
                                 svm->quality_check_enabled_marker);
  con_parse_optional_bool_marker(tmp_quality_check_include_vmaf,
                                 false,
                                 &svc->quality_check_include_vmaf,
                                 &svc->is_quality_check_include_vmaf_valid,
                                 svm->quality_check_include_vmaf_marker);
  con_parse_optional_bool_marker(tmp_quality_check_strict_gate,
                                 false,
                                 &svc->quality_check_strict_gate,
                                 &svc->is_quality_check_strict_gate_valid,
                                 svm->quality_check_strict_gate_marker);
  con_parse_optional_reference_layer_marker(tmp_quality_check_reference_layer,
                                            &svc->quality_check_reference_layer,
                                            &svc->is_quality_check_reference_layer_valid,
                                            svm->quality_check_reference_layer_marker);
  con_parse_optional_non_negative_double_marker(tmp_quality_check_min_psnr,
                                                0.0,
                                                &svc->quality_check_min_psnr,
                                                &svc->is_quality_check_min_psnr_valid,
                                                svm->quality_check_min_psnr_marker);
  con_parse_optional_non_negative_double_marker(tmp_quality_check_min_ssim,
                                                0.0,
                                                &svc->quality_check_min_ssim,
                                                &svc->is_quality_check_min_ssim_valid,
                                                svm->quality_check_min_ssim_marker);
  con_parse_optional_non_negative_double_marker(tmp_quality_check_min_vmaf,
                                                0.0,
                                                &svc->quality_check_min_vmaf,
                                                &svc->is_quality_check_min_vmaf_valid,
                                                svm->quality_check_min_vmaf_marker);
  con_parse_optional_non_negative_int_marker(tmp_quality_check_max_files_per_profile,
                                             0,
                                             &svc->quality_check_max_files_per_profile,
                                             &svc->is_quality_check_max_files_per_profile_valid,
                                             svm->quality_check_max_files_per_profile_marker);

  //tell the user if something isn't valid
  if(svc->is_original_location_valid == false) {
    if(con_lenient_location_validation)
      printf("WARN: the content of MEZZANINE_LOCATION= variable in config file points to a missing location (lenient validation mode)\n");
    else
      printf("ERROR: the content of MEZZANINE_LOCATION= variable in config file is invalid\n");
  }
  if(svc->is_source_location_valid == false) {
    if(con_lenient_location_validation)
      printf("WARN: the content of %s variable in config file points to a missing location (lenient validation mode)\n", svm->source_location);
    else
      printf("ERROR: the content of %s variable in config file is invalid\n", svm->source_location);
  }
  if(svc->is_keep_source_valid == false) 
    printf("ERROR: the content of %s variable in config file is invalid\n", svm->keep_source);
  if(svc->is_mezzanine_clean_enabled_valid == false)
    printf("ERROR: the content of %s variable in config file is invalid\n", svm->mezzanine_clean_enabled_marker);
  if(svc->is_mezzanine_clean_apply_changes_valid == false)
    printf("ERROR: the content of %s variable in config file is invalid\n", svm->mezzanine_clean_apply_changes_marker);
  if(svc->is_mezzanine_clean_append_media_tags_valid == false)
    printf("ERROR: the content of %s variable in config file is invalid\n", svm->mezzanine_clean_append_media_tags_marker);
  if(svc->is_mezzanine_clean_strict_quality_gate_valid == false)
    printf("ERROR: the content of %s variable in config file is invalid\n", svm->mezzanine_clean_strict_quality_gate_marker);
  if(svc->is_quality_check_enabled_valid == false)
    printf("ERROR: the content of %s variable in config file is invalid\n", svm->quality_check_enabled_marker);
  if(svc->is_quality_check_include_vmaf_valid == false)
    printf("ERROR: the content of %s variable in config file is invalid\n", svm->quality_check_include_vmaf_marker);
  if(svc->is_quality_check_strict_gate_valid == false)
    printf("ERROR: the content of %s variable in config file is invalid\n", svm->quality_check_strict_gate_marker);
  if(svc->is_quality_check_reference_layer_valid == false)
    printf("ERROR: the content of %s variable in config file is invalid\n", svm->quality_check_reference_layer_marker);
  if(svc->is_quality_check_min_psnr_valid == false)
    printf("ERROR: the content of %s variable in config file is invalid\n", svm->quality_check_min_psnr_marker);
  if(svc->is_quality_check_min_ssim_valid == false)
    printf("ERROR: the content of %s variable in config file is invalid\n", svm->quality_check_min_ssim_marker);
  if(svc->is_quality_check_min_vmaf_valid == false)
    printf("ERROR: the content of %s variable in config file is invalid\n", svm->quality_check_min_vmaf_marker);
  if(svc->is_quality_check_max_files_per_profile_valid == false)
    printf("ERROR: the content of %s variable in config file is invalid\n", svm->quality_check_max_files_per_profile_marker);

  bool is_location_set_valid = svc->is_original_location_valid && svc->is_source_location_valid;
  bool is_non_location_valid = svc->is_keep_source_valid
                            && svc->is_source_test_valid
                            && svc->is_mezzanine_clean_enabled_valid
                            && svc->is_mezzanine_clean_apply_changes_valid
                            && svc->is_mezzanine_clean_append_media_tags_valid
                            && svc->is_mezzanine_clean_strict_quality_gate_valid
                            && svc->is_quality_check_enabled_valid
                            && svc->is_quality_check_include_vmaf_valid
                            && svc->is_quality_check_strict_gate_valid
                            && svc->is_quality_check_reference_layer_valid
                            && svc->is_quality_check_min_psnr_valid
                            && svc->is_quality_check_min_ssim_valid
                            && svc->is_quality_check_min_vmaf_valid
                            && svc->is_quality_check_max_files_per_profile_valid;
  bool is_source_test_window_valid = (svc->source_test == false)
                                      || (svc->is_source_test_start_valid && svc->is_source_test_duration_valid);

  if(is_non_location_valid && is_source_test_window_valid) {
    if(is_location_set_valid || con_lenient_location_validation)
      svc->is_entire_svc_valid = true;
  }

  if(svc->is_entire_svc_valid == false) {
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
    if(word_found && con_marker_at_line_start(string_conf, i) == false)
      word_found = false;

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
    c_mes_find_config_file_fail(location);
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
