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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "c_cf_ll_struct.h"

cf_node_t* cf_create_new_empty_node() {
  cf_node_t *result = malloc(sizeof(cf_node_t));
  result->folder_name = "";
  result->folder_type = "";
  result->is_folder_name_valid = false;
  result->is_folder_type_valid = false;
  result->is_entire_custom_folder_node_valid = false;
  return result;
}

cf_node_t *cf_create_new_node(char *folder_name, char *folder_type) {
  cf_node_t *result = malloc(sizeof(cf_node_t));
  result->folder_name = folder_name;
  result->folder_type = folder_type;
  result->is_folder_name_valid = false;
  result->is_folder_type_valid = false;
  result->is_entire_custom_folder_node_valid = false;
  result->next = NULL;
  return result;
}

cf_node_t *cf_insert_at_head(cf_node_t **head, cf_node_t *node_to_insert) {
  node_to_insert->next = *head;
  *head = node_to_insert;
  return node_to_insert;
}

void cf_insert_after_node(cf_node_t *node_to_insert_after, cf_node_t* newnode) {
  newnode->next = node_to_insert_after->next;
  node_to_insert_after->next = newnode;
}

cf_node_t *cf_find_node(cf_node_t *head, char *folder_name) {
  cf_node_t *tmp = head;
  while (tmp != NULL) {
    // this doesn't work for char's
    if(strcmp(tmp->folder_name, folder_name) == 0) return tmp;
    tmp = tmp->next;
  }
  return NULL;
}

bool cf_folder_name_exits(cf_node_t *head, char *folder_name) {
  cf_node_t *tmp = head;
  while (tmp != NULL) {
    if (strcmp(tmp->folder_name, folder_name) == 0) return true;
    tmp = tmp->next;
  }
  return false;
}

char* cf_get_type_from_folder_name(cf_node_t *head, char *folder_name) {
  cf_node_t *tmp = head;
  while (tmp != NULL) {
    if (strcmp(tmp->folder_name, folder_name) == 0) return tmp->folder_type;
    tmp = tmp->next;
  }
  return "";
}



void cf_print_list(cf_node_t *head) {
  cf_node_t *temporary = head;

  if(temporary != NULL) {
    while (temporary != NULL) {
      printf ("%s (%s) - ", temporary->folder_name, temporary->folder_type);
      temporary = temporary->next;
    }
    printf("\n");
  } else printf("list is empty\n");
}


int cf_get_count(cf_node_t *head) {
  cf_node_t *temporary = head;
  int counter = 0;
  if(temporary != NULL) {
    while (temporary != NULL) {
      counter++;
      temporary= temporary->next;
    }
  }
  return counter;
}
