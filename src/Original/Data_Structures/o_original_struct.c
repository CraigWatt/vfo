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

#include "o_original_struct.h"

original_t* original_create_new_struct(config_t *config) {
  original_t *result = malloc(sizeof(original_t));
  //point to relevant param config variables
  result->root = config->svc->original_location;
  //needs to be verified
  result->start = o_create_start(result->root);
  //needs to be verified
  result->unable_to_process = o_create_unable_to_process(result->root);

  result->mkv_extension = ".mkv";
  result->mp4_extension = ".mp4";

  //if a /mkv_original folder already exists in root, get it!
  result->mkv_original = o_get_mkv_original_if_it_exists(result->root);
  //if a /mp4_original folder already exists in root, get it!
  result->mp4_original = o_get_mp4_original_if_it_exists(result->root);

  //this simply points to cf_head generated by config.  this means i don't have to
  //pass config struct to relevant original functions, i can just now pass this original
  //object
  //basically a cleaner way to access and share memory locations when applicable!
  result->cf_head = config->cf_head;
  return result;
}

char* o_create_start(char *original_root) {
  //create
  char *tmp_original_start = malloc(BUFSIZ);
  strcpy(tmp_original_start, original_root);
  strcat(tmp_original_start, "/start");
  //verify
  if(!utils_does_folder_exist(tmp_original_start)) {
    printf("ORIGINAL WARNING: vfo could not find an essential /start folder in %s\n",original_root);
    utils_ask_user_for_permission_to_create_a_folder(tmp_original_start);
    utils_create_folder(tmp_original_start);
    if(!utils_does_folder_exist(tmp_original_start)) {
      printf("ORIGINAL MAJOR ERROR: vfo could not create %s & it does not exist ORIGINAL MAJOR ERROR\n", tmp_original_start);
      exit(EXIT_FAILURE);
    }
  }
  return tmp_original_start;
}

char* o_create_unable_to_process(char *original_root) {
  //create
  char *tmp_original_unable_to_process = malloc(BUFSIZ);
  strcpy(tmp_original_unable_to_process, original_root);
  strcat(tmp_original_unable_to_process, "/unable_to_process");
  //verify
  if(!utils_does_folder_exist(tmp_original_unable_to_process)) {
    printf("ORIGINAL WARNING: vfo could not find an essential /unable_to_process folder in %s\n",original_root);
    utils_ask_user_for_permission_to_create_a_folder(tmp_original_unable_to_process);
    utils_create_folder(tmp_original_unable_to_process);
    if(!utils_does_folder_exist(tmp_original_unable_to_process)) {
      printf("ORIGINAL MAJOR ERROR: vfo could not create %s & it does not exist ORIGINAL MAJOR ERROR\n", tmp_original_unable_to_process);
      exit(EXIT_FAILURE);
    }
  }
  return tmp_original_unable_to_process;
}

char* o_get_mkv_original_if_it_exists(char *original_root) {
  //create
  char *tmp_original_mkv_original = malloc(BUFSIZ);
  strcpy(tmp_original_mkv_original, original_root);
  strcat(tmp_original_mkv_original, "/mkv_original");
  //verify
  if(utils_does_folder_exist(tmp_original_mkv_original)) {
    printf("ORIGINAL ALERT: /mkv_original detected.\n");
    return tmp_original_mkv_original;
  }
  else
    printf("ORIGINAL ALERT: if /mkv_original is required, vfo will create it later.\n");
  return "";
}

char* o_get_mp4_original_if_it_exists(char *original_root) {
  //create
  char *tmp_original_mp4_original = malloc(BUFSIZ);
  strcpy(tmp_original_mp4_original, original_root);
  strcat(tmp_original_mp4_original, "/mp4_original");
  //verify
  if(utils_does_folder_exist(tmp_original_mp4_original)) {
    printf("ORIGINAL ALERT: /mp4_original detected.\n");
    return tmp_original_mp4_original;
  }
  else
    printf("ORIGINAL ALERT: if /mp4_original is required, vfo will create it later.\n");
  return "";
}

