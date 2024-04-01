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

#include "s_internal.h"

void s_original_to_source(source_t *source) {
  printf("SOURCE ALERT: initiating 'source'\n");
  /* pre-encode checks */
  s_pre_encode_checks(source);
  
  /* find video folders not yet present in source/content */
  s_highlight_encode_candidates_to_user(source);

  /* ask user if they wish to proceed with encoding */
  utils_wish_to_continue("'find source candidates'", "'encode source candidates to source'");

  /* encode */
  printf("SOURCE ALERT: initiating encoding.\n");
  s_encode_original_to_source(source);

  /* post-encode checks */
  //might need to add to here
  printf("SOURCE ALERT: 'source' completed successfully\n");
}

void s_wipe_source(source_t *source) {
  printf("SOURCE ALERT: initiating 'wipe source'\n");
  /* pre-wipe checks */
  utils_wish_to_continue("'wipe source'", "'wipe source'");
  /* wipe source */
  s_danger_wipe_all_in_source_content(source);
  /* post-wipe checks */
  printf("SOURCE ALERT: 'wipe source' completed successfully\n");
}

void s_pre_encode_checks(source_t *source) {
  printf("SOURCE ALERT: initiating 'pre-encode checks'\n");
  //does source/content folder contain valid custom folders
  utils_does_folder_contain_valid_custom_folders(source->content, source->cf_head);
  //is source/content folder missing any custom folders
  utils_is_folder_missing_custom_folders(source->content, source->cf_head);
  /*in future I would like to be able to SAY what folders are missing*/
  /*now we know all necessary folders contain necessary custom_folders*/

  /*but let's check to see if each custom_folder is adhering to their custom_folder_type*/
  //are the source/content folder custom_folders adhering to their types?
  //(using mp4_original flag as source also only uses mp4 format.)
  utils_are_custom_folders_type_compliant(source->content, "mp4_original", source->cf_head);
    
  // maybe necessary but not sure?
  // are_custom_folders_files_source_compliant();
  // perhaps some sort of check of the movie/tv files themselves to see if they are compliant with source.

  //s_is_source_out_of_sync_with_original(source); - //simple meaning: if a movie/tv folder exists here that doesn't 
                                        //exist in original, tell the user.
  printf("SOURCE ALERT: 'pre-encode checks' completed successfully\n");
}

void s_danger_wipe_all_in_source_content(source_t *source) {
  active_cf_node_t *active_cf = utils_generate_from_to_ll(source->cf_head, source->content, source->content);
  if(active_cf != NULL) {
    while(active_cf != NULL) {
      active_films_f_node_t *tmp_f = active_cf->active_films_f_head;
      if(tmp_f != NULL) {
        while(tmp_f != NULL) {
          //if from_folder doesn't exist, 
          if(utils_does_folder_exist(tmp_f->from_films_f_folder)) {
            //delete contents of folder
            utils_danger_delete_contents_of_folder(tmp_f->from_films_f_folder);
            //delete folder
            utils_delete_folder_if_it_is_empty(tmp_f->from_films_f_folder);
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
                  //if from_folder does exist
                  if(utils_does_folder_exist(tmp_tv3->from_tv_f_folder)) {
                    //delete contents of folder
                    utils_danger_delete_contents_of_folder(tmp_tv3->from_tv_f_folder);
                    //delete folder
                    utils_delete_folder_if_it_is_empty(tmp_tv3->from_tv_f_folder);
                  }
                  tmp_tv3 = tmp_tv3->next;
                }
              }
              if(utils_does_folder_exist(tmp_tv2->from_tv_f_folder))
                utils_delete_folder_if_it_is_empty(tmp_tv2->from_tv_f_folder);
              tmp_tv2 = tmp_tv2->next;
            }
          }
          if(utils_does_folder_exist(tmp_tv1->from_tv_f_folder))
            utils_delete_folder_if_it_is_empty(tmp_tv1->from_tv_f_folder);
          tmp_tv1 = tmp_tv1->next;
        }
      }
      active_cf = active_cf->next;
    }
  }
  free(active_cf);
  active_cf = NULL;
}

void s_encode_original_to_source(source_t *source) {
  if(strcmp(source->original_mkv_original, "") != 0)
    s_encode_from_mkv_original_to_source(source);
  if(strcmp(source->original_mp4_original, "") != 0)
    s_encode_from_mp4_original_to_source(source);
  if(strcmp(source->original_m2ts_original, "") != 0)
    s_encode_from_m2ts_original_to_source(source);
}

void s_encode_from_mkv_original_to_source(source_t *source) {
  active_cf_node_t *active_cf = utils_generate_from_to_ll(source->cf_head, source->original_mkv_original, source->content);
  s_encode_shared_code(active_cf, source);
}

void s_encode_from_mp4_original_to_source(source_t *source) {
  active_cf_node_t *active_cf = utils_generate_from_to_ll(source->cf_head, source->original_mp4_original, source->content);
  s_encode_shared_code(active_cf, source);
}

void s_encode_from_m2ts_original_to_source(source_t *source) {
  active_cf_node_t *active_cf = utils_generate_from_to_ll(source->cf_head, source->original_m2ts_original, source->content);
  s_encode_shared_code(active_cf, source);
}

void s_encode_shared_code(active_cf_node_t *active_cf, source_t *source) {
  if(active_cf != NULL) {
    while(active_cf != NULL) {
      active_films_f_node_t *tmp_f = active_cf->active_films_f_head;
      if(tmp_f != NULL) {
        while(tmp_f != NULL) {
          //if to_folder doesn't exist, 
          if(!utils_does_folder_exist(tmp_f->to_films_f_folder)) {
            //create the folder
            utils_create_folder(tmp_f->to_films_f_folder);

            //check if source_test is active
            if(source->source_test){
              //encode the video file with source_test times
              
              //ELSE IF encode the video file
            }  else if(s_execute_ffmpeg_command(tmp_f->from_films_f_folder, tmp_f->to_films_f_folder) == true) {
              //command executed successfully
              printf("SOURCE ALERT: ffmpeg command executed successfully.\n");
            }
            else {
              //command unsuccessful - need to generate a error txt file in source->unable_to_process
              printf("SOURCE WARNING: ffmpeg command FAILED.  Printing error message to source/unable_to_process\n");
              printf("DEV: attempting to create error txt file in unable to process.\n");
              utils_create_error_encoding_file(source->unable_to_process, tmp_f->from_films_f_folder);
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
                    if(s_execute_ffmpeg_command(tmp_tv3->from_tv_f_folder, tmp_tv3->to_tv_f_folder) == true) {
                      //command executed successfully
                      printf("SOURCE ALERT: ffmpeg command executed successfully.\n");
                    }
                    else {
                      //command unsuccessful - need to generate a error txt file in source->unable_to_process
                      printf("SOURCE WARNING: ffmpeg command FAILED.  Printing error message to source/unable_to_process\n");
                      printf("DEV: attempting to create error txt file in unable to process.\n");
                      utils_create_error_encoding_file(source->unable_to_process, tmp_tv3->from_tv_f_folder);
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

bool s_execute_ffmpeg_command(char *original_from, char *source_to) {
  int ffmpeg_error_number = system(s_generate_ffmpeg_command(original_from, source_to));
  if(ffmpeg_error_number == 0)
    return true;
  return false;
}

char* s_generate_ffmpeg_command(char *original_from, char *source_to) {
  /* construct ffmpeg command */
  char *vfo_input = utils_fetch_single_file(original_from);
  char *vfo_output = s_generate_source_file_name(original_from, source_to);
  char *tmp_ffmpeg = "ffmpeg -nostdin -i \"";
  tmp_ffmpeg = utils_combine_strings(tmp_ffmpeg, vfo_input);

  // --- is source test active
  if(source->source_test)


  // ---


  tmp_ffmpeg = utils_combine_strings(tmp_ffmpeg, "\" -c copy -sn -movflags faststart -strict -2 \"");
  tmp_ffmpeg = utils_combine_strings(tmp_ffmpeg, vfo_output);
  tmp_ffmpeg = utils_combine_strings(tmp_ffmpeg, "\"");
  return tmp_ffmpeg;
}

char* s_generate_source_file_name(char *original_from, char *source_to) {
  char *tmp_name = utils_fetch_single_file(original_from);
  tmp_name = basename(tmp_name);
  /* temporarily remove file extension from filename */
  if(utils_is_file_extension_mkv(tmp_name))
    tmp_name = utils_replace_characters(tmp_name, ".mkv", "");
  if(utils_is_file_extension_mp4(tmp_name))
    tmp_name = utils_replace_characters(tmp_name, ".mp4", "");
  /* append _source to end of filename */
  tmp_name = utils_combine_strings(tmp_name, "_source");
  /* add back .mp4 extension */
  tmp_name = utils_combine_strings(tmp_name, ".mp4");
  /* add name to destination */
  tmp_name = utils_combine_to_full_path(source_to, tmp_name);
  return tmp_name;
}

void s_highlight_encode_candidates_to_user(source_t *source) {  
  if(strcmp(source->original_mkv_original, "") != 0) {
    printf("SOURCE ALERT: found mkv_original\n");
    s_highlight_encode_candidates_from_mkv_original(source);
  }
  if(strcmp(source->original_mp4_original, "") != 0) {
    printf("SOURCE ALERT: found mp4_original\n");
    s_highlight_encode_candidates_from_mp4_original(source);
  }
}

void s_highlight_encode_candidates_from_mkv_original(source_t *source) {
  active_cf_node_t *active_cf = utils_generate_from_to_ll(source->cf_head, source->original_mkv_original, source->content);
  int mkv_encode_candidates_counter = 0;
  int already_present_in_source_counter = 0;
  if(active_cf != NULL) {
    while(active_cf != NULL) {
      active_films_f_node_t *tmp_f = active_cf->active_films_f_head;
      if(tmp_f != NULL) {
        while(tmp_f != NULL) {
          //if to_folder does exist AND if from_folder does exist
          if(utils_does_folder_exist(tmp_f->from_films_f_folder) && utils_does_folder_exist(tmp_f->to_films_f_folder))
            already_present_in_source_counter++;
          else if(utils_does_folder_exist(tmp_f->from_films_f_folder) && !(utils_does_folder_exist(tmp_f->to_films_f_folder)))
            mkv_encode_candidates_counter++;
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
                    already_present_in_source_counter++;
                  else if(utils_does_folder_exist(tmp_tv3->from_tv_f_folder) && !(utils_does_folder_exist(tmp_tv3->to_tv_f_folder)))
                    mkv_encode_candidates_counter++;
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
  printf("SOURCE ALERT: %i mkv_original -> source/content candidates found.\n", mkv_encode_candidates_counter);
  printf("SOURCE ALERT: %i will be ignored as they appear to already exist in source/content.\n", already_present_in_source_counter);
  free(active_cf);
  active_cf = NULL;
}

void s_highlight_encode_candidates_from_mp4_original(source_t *source) {
  active_cf_node_t *active_cf = utils_generate_from_to_ll(source->cf_head, source->original_mp4_original, source->content);
  int mp4_encode_candidates_counter = 0;
  int already_present_in_source_counter = 0;
  if(active_cf != NULL) {
    while(active_cf != NULL) {
      active_films_f_node_t *tmp_f = active_cf->active_films_f_head;
      if(tmp_f != NULL) {
        while(tmp_f != NULL) {
          //if to_folder does exist AND if from_folder does exist
          if(utils_does_folder_exist(tmp_f->from_films_f_folder) && utils_does_folder_exist(tmp_f->to_films_f_folder))
            already_present_in_source_counter++;
          else if(utils_does_folder_exist(tmp_f->from_films_f_folder) && !(utils_does_folder_exist(tmp_f->to_films_f_folder)))
            mp4_encode_candidates_counter++;
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
                    already_present_in_source_counter++;
                  else if(utils_does_folder_exist(tmp_tv3->from_tv_f_folder) && !(utils_does_folder_exist(tmp_tv3->to_tv_f_folder)))
                    mp4_encode_candidates_counter++;
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
  printf("SOURCE ALERT: %i mp4_original -> source/content candidates found.\n", mp4_encode_candidates_counter);
  printf("SOURCE ALERT: %i will be ignored as they appear to already exist in source/content.\n", already_present_in_source_counter);
  free(active_cf);
  active_cf = NULL;
}
