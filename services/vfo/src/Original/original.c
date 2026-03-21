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

#include "Data_Structures/o_original_struct.h"
#include "o_internal.h"

void o_original(original_t *original) {
  printf("ORIGINAL ALERT: initiating 'original'\n");
  /* pre-move checks */
  o_pre_move_checks(original);
  /*if a duplicate is found, vfo will stop and tell the user*/
  /*ask user if they wish to continue on to the 'move' process of o_original*/
  utils_wish_to_continue("'pre-move-checks'", "'original'");

  /* move actions */
  // move all mkv content from start to mkv_original  
  o_move_from_start_to_mkv_original(original);
  // move all mp4 content from start to mp4_original  
  o_move_from_start_to_mp4_original(original);
  // move all m2ts content from start to m2ts_original
  o_move_from_start_to_m2ts_original(original);
  
  /* post move checks */
  //might need to add to here in future
  printf("ORIGINAL ALERT: 'original' completed successfully.\n");
}

void o_revert_to_start(original_t *original) {
  printf("ORIGINAL ALERT: initiating 'revert-to-start'\n");
  /* pre move checks */
  o_pre_move_checks(original);
  utils_wish_to_continue("'pre-move checks'", "'revert-to-start'");
  // move all mkv content from mkv_original to start
  o_move_from_mkv_original_to_start(original);
  // move all mp4 content from mp4_original to start
  o_move_from_mp4_original_to_start(original);
  // move all m2ts content from m2ts_original to start
  o_move_from_m2ts_original_to_start(original);

  /* post move checks */
  //might need to add to here in future
  printf("ORIGINAL ALERT: 'revert-to-start' completed successfully.\n");
}

void o_pre_move_checks(original_t *original) {
  printf("ORIGINAL ALERT: initiating 'pre-move checks'\n");
  //does start folder contain valid custom folders
  printf("ORIGINAL ALERT: 'pre-move check': scanning original/start to see if it contains custom folders.\n");
  utils_does_folder_contain_valid_custom_folders(original->start, original->cf_head);
  //does mkv_original folder contain valid custom folders
  printf("ORIGINAL ALERT: 'pre-move check': scanning original/mkv_original to see if it contains custom folders.\n");
  utils_does_folder_contain_valid_custom_folders(original->mkv_original, original->cf_head);
  //does mp4_original folder contain valid custom folders
  printf("ORIGINAL ALERT: 'pre-move check': scanning original/mp4_original to see if it contains custom folders.\n");
  utils_does_folder_contain_valid_custom_folders(original->mp4_original, original->cf_head);
  
  //is start folder missing any custom folders
  printf("ORIGINAL ALERT: 'pre-move check': scanning original/start to see if it is missing any custom folders.\n");
  utils_is_folder_missing_custom_folders(original->start, original->cf_head);
  //is mkv_original missing any custom folders
  printf("ORIGINAL ALERT: 'pre-move check': scanning original/mkv_original to see if it is missing any custom folders.\n");
  utils_is_folder_missing_custom_folders(original->mkv_original, original->cf_head);
  //is mp4_original missing any custom folders
  printf("ORIGINAL ALERT: 'pre-move check': scanning original/mp4_original to see if it is missing any custom folders.\n");
  utils_is_folder_missing_custom_folders(original->mp4_original, original->cf_head);
  //is m2ts_original missing any custom folders
  printf("ORIGINAL ALERT: 'pre-move check': scanning original/m2ts_original to see if it is missing any custom folders.\n");
  utils_is_folder_missing_custom_folders(original->m2ts_original, original->cf_head);
  /*in future I would like to be able to SAY what folders are missing*/
  /*now we know all necessary folders contain necessary custom_folders*/

  /*but let's check to see if each custom_folder is adhering to their custom_folder_type*/
  //are the start folder custom_folders adhering to their types?
  printf("ORIGINAL ALERT: 'pre-move check': scanning original/start to see if it is compliant with custom folder rules.\n");
  utils_are_custom_folders_type_compliant(original->start, "start", original->cf_head);
  //are the mkv_original folder custom_folders adhering to their types?
  printf("ORIGINAL ALERT: 'pre-move check': scanning original/mkv_original to see if it is compliant with custom folder rules.\n");
  utils_are_custom_folders_type_compliant(original->mkv_original, "mkv_original", original->cf_head);
  //are the mp4_original folder custom_folders adhering to their types?
  printf("ORIGINAL ALERT: 'pre-move check': scanning original/mp4_original to see if it is compliant with custom folder rules.\n");
  utils_are_custom_folders_type_compliant(original->mp4_original, "mp4_original", original->cf_head);
  //are the m2ts_original folder folder custom_folders adhering to their types?
  printf("ORIGINAL ALERT: 'pre-move check': scanning original/m2ts_original to see if it is compliant with custom folder rules.\n");
  utils_are_custom_folders_type_compliant(original->m2ts_original, "m2ts_original", original->cf_head);
  /*now we know every file & folder location within all relevant original folders are adhering to their custom_folder type rules*/

  /*let's now check for duplicates*/
  //does start contain video folders that match the folders in mkv_original?
  printf("ORIGINAL ALERT: 'pre-move check': scanning original/start VERSUS original/mkv_original in an attempt to detect duplicates\n");
  o_detect_duplicates_start_versus_mkv_original(original);
  //does start contain video folders that match the folders in mp4_original?
  printf("ORIGINAL ALERT: 'pre-move check': scanning original/start VERSUS original/mp4_original in an attempt to detect duplicates\n");
  o_detect_duplicates_start_versus_mp4_original(original);
  //does mkv_original contain video folders that match the folders in mp4_original?
  printf("ORIGINAL ALERT: 'pre-move check': scanning original/mkv_original VERSUS original/mp4_original in an attempt to detect duplicates\n");
  o_detect_duplicates_mkv_original_versus_mp4_original(original);
  //does mp4_original contain video folders that match the folders in mkv_original?
  printf("ORIGINAL ALERT: 'pre-move check': scanning original/mp4_original VERSUS original/mkv_original in an attempt to detect duplicates\n");
  o_detect_duplicates_mp4_original_versus_mkv_original(original);
  printf("ORIGINAL ALERT: 'pre-move checks' completed successfully.\n");
}

void o_move_from_start_to_mkv_original(original_t *original) {
  o_move(original->start, original->mkv_original, original->cf_head, original->mkv_extension);
}

void o_move_from_start_to_mp4_original(original_t *original) {
  o_move(original->start, original->mp4_original, original->cf_head, original->mp4_extension);
}

void o_move_from_start_to_m2ts_original(original_t *original) {
  o_move(original->start, original->m2ts_original, original->cf_head, original->m2ts_extension);
}

void o_move_from_mkv_original_to_start(original_t *original) {
  o_move(original->mkv_original, original->start, original->cf_head, original->mkv_extension);
}

void o_move_from_mp4_original_to_start(original_t *original) {
  o_move(original->mp4_original, original->start, original->cf_head, original->mp4_extension);
}

void o_move_from_m2ts_original_to_start(original_t *original) {
  o_move(original->m2ts_original, original->start, original->cf_head, original->m2ts_extension);
}
/*
 * This function is responsible for moving anything within the original folder workspace
 * This function accepts param cf_head to determine what the custom folders are and what their types are
 * This function accepts from_cf_parent_folder - this is a folder that holds custom folders
 * This function accepts param to_cf_parent_folder - this is a folder that holds custom folders
 * This function accepts param FuncPtr-Allowed-Ext - this function is used as a criteria check for both films and tv cf types
 */
void o_move(char *from_cf_parent_folder, char *to_cf_parent_folder, cf_node_t *cf_head, char *from_valid_extension_string) {
  active_cf_node_t *active_cf = utils_generate_from_to_ll(cf_head, from_cf_parent_folder, to_cf_parent_folder);
  //for every active_cf_node in active_cf_head
  if(active_cf != NULL) {
    while(active_cf != NULL) {
      active_films_f_node_t *tmp_f = active_cf->active_films_f_head;
      //for every active_films_f_node node in active_cf_node->active_films_f_head
      if(tmp_f != NULL) {
        while(tmp_f != NULL) {
          //if to_folder doesn't exist AND if from folder contains valid file
          if(!utils_does_folder_exist(tmp_f->to_films_f_folder) && utils_does_folder_contain_file_with_extension(tmp_f->from_films_f_folder, from_valid_extension_string)) {
            //rename from_folder to to_folder
            if(rename(tmp_f->from_films_f_folder, tmp_f->to_films_f_folder) == 0) {
              printf("ORIGINAL ALERT: move folder successful %s -> %s\n", tmp_f->from_films_f_folder, tmp_f->to_films_f_folder);
              if(utils_does_folder_exist(tmp_f->from_films_f_folder)) {
                printf("ORIGINAL ALERT: Attempting to delete EMPTY film folder.\n");
                utils_delete_folder_if_it_is_empty(tmp_f->from_films_f_folder);
              }
            }
            else {
              printf("ORIGINAL MAJOR ERROR: could not rename %s to %s\n", tmp_f->from_films_f_folder, tmp_f->to_films_f_folder);
              exit(EXIT_FAILURE);
            }
          }
          tmp_f = tmp_f->next; 
        }
      }
      active_tv_f_node_t *tmp_tv1 = active_cf->active_tv_f_head;
      //for every active_tv_f_node in active_cf_node->active_tv_f_head
      if(tmp_tv1!= NULL) {
        while(tmp_tv1 != NULL) {
          active_tv_f_node_t *tmp_tv2 = tmp_tv1->active_tv_f_head;
          //for every active_tv_f_node in active_cf_node->active_tv_f_node->active_tv_f_head
          if(tmp_tv2 != NULL) {
            while(tmp_tv2 != NULL) {
              active_tv_f_node_t *tmp_tv3 = tmp_tv2->active_tv_f_head;
              //for every active_tv_f_node in active_cf_Node->active_tv_f_node->active_tv_f_node->active_tv_f_head
              if(tmp_tv3 != NULL) {
                while(tmp_tv3 != NULL) {
                  //if to_folder doesn't exist AND from_folder contains valid file
                  if(!utils_does_folder_exist(tmp_tv3->to_tv_f_folder) && utils_does_folder_contain_file_with_extension(tmp_tv3->from_tv_f_folder, from_valid_extension_string)) {
                    //if to_folder doesn't exist
                    if(!utils_does_folder_exist(tmp_tv1->to_tv_f_folder)) {
                      //create it
                      utils_create_folder(tmp_tv1->to_tv_f_folder);
                    }
                    //if to_folder doesn't exist
                    if(!utils_does_folder_exist(tmp_tv2->to_tv_f_folder)) {
                      //create it
                      utils_create_folder(tmp_tv2->to_tv_f_folder);
                    }
                    //rename from_folder to to_folder
                    if(rename(tmp_tv3->from_tv_f_folder, tmp_tv3->to_tv_f_folder) == 0) {
                      printf("ORIGINAL ALERT: move folder successful %s -> %s\n", tmp_tv3->from_tv_f_folder, tmp_tv3->to_tv_f_folder);
                      if(utils_does_folder_exist(tmp_tv3->from_tv_f_folder)) {
                        printf("ORIGINAL ALERT: Attempting to delete EMPTY tv episode folder.\n");
                        utils_delete_folder_if_it_is_empty(tmp_tv3->from_tv_f_folder);
                      }
                    }
                    else {
                      printf("ORIGINAL MAJOR ERROR: could not rename %s to %s\n", tmp_tv3->from_tv_f_folder, tmp_tv3->to_tv_f_folder);
                      exit(EXIT_FAILURE);
                    }
                  }
                  tmp_tv3 = tmp_tv3->next;
                }
              }
              if(utils_does_folder_exist(tmp_tv2->from_tv_f_folder)) {
                printf("ORIGINAL ALERT: Attempting to delete EMPTY tv season folder.\n");
                utils_delete_folder_if_it_is_empty(tmp_tv2->from_tv_f_folder);
              }
              tmp_tv2 = tmp_tv2->next;
            }
          }
          if(utils_does_folder_exist(tmp_tv1->from_tv_f_folder)){
            printf("ORIGINAL ALERT: Attempting to delete EMPTY tv show folder.\n");
            utils_delete_folder_if_it_is_empty(tmp_tv1->from_tv_f_folder);
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

void o_detect_duplicates_start_versus_mkv_original(original_t *original) {
  o_detect_duplicates(original->start, original->mkv_original, original->cf_head);
}

void o_detect_duplicates_start_versus_mp4_original(original_t *original) {
  o_detect_duplicates(original->start, original->mp4_original, original->cf_head);
}

void o_detect_duplicates_start_versus_m2ts_original(original_t *original) {
  o_detect_duplicates(original->start, original->m2ts_original, original->cf_head);
}

void o_detect_duplicates_mkv_original_versus_mp4_original(original_t *original) {
  o_detect_duplicates(original->mkv_original, original->mp4_original, original->cf_head);
}

void o_detect_duplicates_mp4_original_versus_mkv_original(original_t *original) {
  o_detect_duplicates(original->mp4_original, original->mkv_original, original->cf_head);
}

void o_detect_duplicates(char *from_cf_parent_folder, char *to_cf_parent_folder, cf_node_t *cf_head) {
  bool duplicate_detected = false;
  active_cf_node_t *active_cf = utils_generate_from_to_ll(cf_head, from_cf_parent_folder, to_cf_parent_folder);
  //for every active_cf_node in active_cf_head
  if(active_cf != NULL) {
    while(active_cf != NULL) {
      active_films_f_node_t *tmp_f = active_cf->active_films_f_head;
      //for every active_films_f_node node in active_cf_node->active_films_f_head
      if(tmp_f != NULL) {
        while(tmp_f != NULL) {
          //if to_folder does exist AND if from_folder does exist
          if(utils_does_folder_exist(tmp_f->from_films_f_folder) && utils_does_folder_exist(tmp_f->to_films_f_folder)) {
            //duplicate detected
            duplicate_detected = true;
            printf("ORIGINAL ERROR: Duplicate detected. %s exists in %s AND %s exists in %s\n", tmp_f->from_films_f_folder, from_cf_parent_folder, tmp_f->to_films_f_folder, to_cf_parent_folder); 
          }
          tmp_f = tmp_f->next; 
        }
      }
      active_tv_f_node_t *tmp_tv1 = active_cf->active_tv_f_head;
      //for every active_tv_f_node in active_cf_node->active_tv_f_head
      if(tmp_tv1!= NULL) {
        while(tmp_tv1 != NULL) {
          //if to_folder does exist AND if from folder does exist - we need to investigate further
          active_tv_f_node_t *tmp_tv2 = tmp_tv1->active_tv_f_head;
          //for every active_tv_f_node in active_cf_node->active_tv_f_node->active_tv_f_head
          if(tmp_tv2 != NULL) {
            while(tmp_tv2 != NULL) {
              //if to_folder does exist AND if from folder does exist - we need to investigate further
              active_tv_f_node_t *tmp_tv3 = tmp_tv2->active_tv_f_head;
              //for every active_tv_f_node in active_cf_Node->active_tv_f_node->active_tv_f_node->active_tv_f_head
              if(tmp_tv3 != NULL) {
                while(tmp_tv3 != NULL) {
                  //if to_folder doesn't exist AND from_folder contains valid file
                  if(utils_does_folder_exist(tmp_tv3->from_tv_f_folder) && utils_does_folder_exist(tmp_tv3->to_tv_f_folder)) {
                    //duplicate detected
                    duplicate_detected = true;
                    printf("ORIGINAL ERROR: Duplicate detected. %s exists in %s AND %s exists in %s\n", tmp_tv3->from_tv_f_folder, from_cf_parent_folder, tmp_tv3->to_tv_f_folder, to_cf_parent_folder); 
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

  if(duplicate_detected == true) {
    printf("ORIGINAL MAJOR ERROR: Unfortunately, we have picked up on duplicate error/s when comparing %s with %s\n", from_cf_parent_folder, to_cf_parent_folder);
    printf("vfo recommends you resolve these conflicts manually.\n");
    exit(EXIT_FAILURE);
  }
}



