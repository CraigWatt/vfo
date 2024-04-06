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

#include "s_source_struct.h"

source_t* source_create_new_struct(config_t *config) {
  source_t *result = malloc(sizeof(source_t));
  //point to relevant param config variables

  //ALREADY VERIFIED (by config)
  result->root = config->svc->source_location;
  //VERIFIED
  result->content = s_create_content(result->root);
  //VERIFIED
  result->unable_to_process = s_create_unable_to_process(result->root);

  //VERIFIED
  result->original_mkv_original = s_get_mkv_original_if_it_exists(config->svc->original_location);
  //VERIFIED
  result->original_mp4_original = s_get_mp4_original_if_it_exists(config->svc->original_location);
  //VERIFIED
  result->original_m2ts_original = s_get_m2ts_original_if_it_exists(config->svc->original_location);

  //VERIFIED
  result->source_test = config->svc->source_test;
  //VERIFIED
  result->source_test_start = config->svc->source_test_trim_start;
  //VERIFIED
  result->source_test_duration = config->svc->source_test_trim_duration;
  //ALREADY VERIFIED (by config)
  result->cf_head = config->cf_head;
  return result;
}

char* s_create_content(char *source_root) {
  //create
  char *tmp_source_content = malloc(BUFSIZ);
  strcpy(tmp_source_content, source_root);
  strcat(tmp_source_content, "/content");
  //verify
  if(!utils_does_folder_exist(tmp_source_content)) {
    printf("SOURCE WARNING: vfo could not find an essential /content folder in %s\n",source_root);
    utils_ask_user_for_permission_to_create_a_folder(tmp_source_content);
    utils_create_folder(tmp_source_content);
    if(!utils_does_folder_exist(tmp_source_content)) {
      printf("SOURCE MAJOR ERROR: vfo could not create %s & it does not exist SOURCE MAJOR ERROR\n", tmp_source_content);
      exit(EXIT_FAILURE);
    }
  }
  return tmp_source_content;
}
char* s_create_unable_to_process(char *source_root) {
  //create
  char *tmp_source_unable_to_process = malloc(BUFSIZ);
  strcpy(tmp_source_unable_to_process, source_root);
  strcat(tmp_source_unable_to_process, "/unable_to_process");
  //verify
  if(!utils_does_folder_exist(tmp_source_unable_to_process)) {
    printf("SOURCE WARNING: vfo could not find an essential /unable_to_process folder in %s\n",source_root);
    utils_ask_user_for_permission_to_create_a_folder(tmp_source_unable_to_process);
    utils_create_folder(tmp_source_unable_to_process);
    if(!utils_does_folder_exist(tmp_source_unable_to_process)) {
      printf("SOURCE MAJOR ERROR: vfo could not create %s & it does not exist SOURCE MAJOR ERROR\n", tmp_source_unable_to_process);
      exit(EXIT_FAILURE);
    }
  }
  return tmp_source_unable_to_process;
}

char* s_get_mkv_original_if_it_exists(char *original_root) {
  //create
  char *tmp_source_mkv_original = malloc(BUFSIZ);
  strcpy(tmp_source_mkv_original, original_root);
  strcat(tmp_source_mkv_original, "/mkv_original");
  //verify
  if(utils_does_folder_exist(tmp_source_mkv_original)) {
    printf("SOURCE ALERT: original's mkv_original detected.\n");
    return tmp_source_mkv_original;
  }
  else {
    printf("DEV: tmp_source_mkv_original: %s\n", tmp_source_mkv_original);
    printf("SOURCE ALERT: original's /mkv_original does not exist, mkv to source encoding will not occur.\n");
  }
    
  return "";
}
char* s_get_mp4_original_if_it_exists(char *original_root) {
  //create
  char *tmp_source_mp4_original = malloc(BUFSIZ);
  strcpy(tmp_source_mp4_original, original_root);
  strcat(tmp_source_mp4_original, "/mp4_original");
  //verify
  if(utils_does_folder_exist(tmp_source_mp4_original)) {
    printf("SOURCE ALERT: original's mp4_original detected.\n");
    return tmp_source_mp4_original;
  } 
  else {
    printf("DEV: tmp_source_mp4_original: %s\n", tmp_source_mp4_original);
    printf("SOURCE ALERT: original's /mp4_original does not exist, mp4 to source encoding will not occur.\n");
  }
  return "";
}
char* s_get_m2ts_original_if_it_exists(char *original_root) {
  //create
  char *tmp_source_m2ts_original = malloc(BUFSIZ);
  strcpy(tmp_source_m2ts_original, original_root);
  strcat(tmp_source_m2ts_original, "/m2ts_original");
  //verify
  if(utils_does_folder_exist(tmp_source_m2ts_original)) {
    printf("SOURCE ALERT: original's m2ts_original detected.\n");
    return tmp_source_m2ts_original;
  } 
  else {
    printf("DEV: tmp_source_m2ts_original: %s\n", tmp_source_m2ts_original);
    printf("SOURCE ALERT: original's /m2ts_original does not exist, m2ts to source encoding will not occur.\n");
  }
  return "";
}
