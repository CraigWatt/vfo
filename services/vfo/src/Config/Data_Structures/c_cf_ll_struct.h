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

#ifndef C_CUSTOM_FOLDERS_LINKED_LIST_H
#define C_CUSTOM_FOLDERS_LINKED_LIST_H

#include <stdbool.h>


/* ----------custom_folders_linked_list header file---------- */
/*
 * The purpose of cf ll is to store each 'custom folder name'
 * and each 'custom folder type' from the conf file.
 * 
 * This gives the end user scalable customization of his/her custom folders.
 *
 * Right now, across the program, this generated ll is used extensively for many functions...
 * This is because vfo needs to check to see if in each essential folder such as
 * original (start, mkv_original, mp4_original etc),
 * source.content, 
 * any alias folder.content.
 * vfo needs to determine whether custom folders located in these essential folders
 *  1. actually exist
 *  2. are containing files and folders IN A WAY that always conform to their specified type tules
 *
 * What functions use this?:
 * 1.
 * 2.
 * 3.
 * 4.
 */

struct cf_node {
  char *folder_name;
  char *folder_type;
  bool is_folder_name_valid;
  bool is_folder_type_valid;
  bool is_entire_custom_folder_node_valid;
  struct cf_node* next;
};

typedef struct cf_node cf_node_t;

cf_node_t* cf_create_new_empty_node();
cf_node_t* cf_create_new_node(char *folder_name, char *folder_type);
void cf_print_list(cf_node_t *head);
cf_node_t *cf_insert_at_head(cf_node_t **head, cf_node_t *node_to_insert);
cf_node_t *cf_find_node(cf_node_t *head, char *folder_name);
int cf_get_count(cf_node_t *head);
void cf_insert_after_node(cf_node_t *node_to_insert_after, cf_node_t* newnode);
bool cf_folder_name_exits(cf_node_t *head, char *folder_name);
char* cf_get_type_from_folder_name(cf_node_t *head, char *folder_name);

#endif // CUSTOM_FOLDERS_LINKED_LIST_H
