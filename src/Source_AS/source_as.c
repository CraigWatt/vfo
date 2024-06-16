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

 #include "sas_internal.h"

// void as_pre_encode_checks(audio_strat_t *audio_strat);
// void as_highlight_encode_candidates_to_user(audio_strat_t *audio_strat);
// void as_encode_from_source_to_source(audio_strat_t *audio_strat);
// char* as_generate_ffmpeg_command(char *source_from, char *source_to);
// char* as_generate_audio_strat_file_name(char *source_from, char *source_to);
// bool as_execute_ffmpeg_command(char *source_from, char *source_to);

void as_highlight_encode_candidates_to_user(audio_strat_t *audio_strat) {
  if(strcmp(audio_strat->source_content, "") != 0) {
    printf("AUDIO_STRAT ALERT: found source content\n");
    as_highlight_encode_candidates_from_source_content(audio_strat);
  }
}

void as_highlight_encode_candidates_from_source_content(audio_strat_t *audio_strat) {
active_cf_node_t *active_cf = utils_generate_from_to_ll(audio_strat->cf_head, audio_strat->original_mkv_original, source->content);
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





