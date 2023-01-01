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

#include "u_active_cf_ll_struct.h"
#include "u_active_tv_f_ll_struct.h"

active_cf_node_t* active_cf_create_new_node(char *from_cf_folder, char *to_cf_folder, char *type) {
  active_cf_node_t *result = malloc(sizeof(active_cf_node_t));
  result->from_cf_folder = from_cf_folder;
  result->to_cf_folder = to_cf_folder;
  result->type = type;
  result->active_films_f_head = NULL;
  result->active_tv_f_head = NULL;
  result->next = NULL;
  return result;
}

active_cf_node_t* active_cf_create_node(char *from_cf_folder, char *to_cf_folder, char *type, active_films_f_node_t *active_films_f_head, active_tv_f_node_t * active_tv_f_head) {
  active_cf_node_t * result = malloc(sizeof(active_cf_node_t));
  result->from_cf_folder = from_cf_folder;
  result->to_cf_folder = to_cf_folder;
  result->type = type;
  result->active_films_f_head = active_films_f_head;
  result->active_tv_f_head = active_tv_f_head;
  result->next = NULL;
  return result;

}

void active_cf_print_list(active_cf_node_t *head) {
  active_cf_node_t *temp = head;
  if(temp != NULL) {
    while(temp != NULL) {
      printf("a active_cf_node: %s (%s, %s)- \n", temp->from_cf_folder, temp->to_cf_folder, temp->type);
      if(temp->active_films_f_head != NULL) {
        printf("active_cf_node->active_films_f_head\n");
        active_films_f_print_list(temp->active_films_f_head);
      }
      else if(temp->active_tv_f_head != NULL) {
        printf("active_cf_node->active_tv_f_head\n");
        active_tv_f_print_list(temp->active_tv_f_head);
      } else {
        printf("active_cf_node->active_films_f_head & cf_node->active_films_f_head are EMPTY\n");
      }
      temp = temp->next;
    }
  } else printf("this active_cf_head - list is empty\n");
}

active_cf_node_t* active_cf_insert_at_head(active_cf_node_t **head, active_cf_node_t *node_to_insert) {
  node_to_insert->next = *head;
  *head = node_to_insert;
  return node_to_insert;
}

void active_cf_insert_after_node(active_cf_node_t *node_to_insert_after, active_cf_node_t *newnode) {
  newnode->next = node_to_insert_after->next;
  node_to_insert_after->next = newnode;
}

int active_cf_get_count(active_cf_node_t *head) {
  active_cf_node_t *temporary = head;
  int counter = 0;
  if(temporary != NULL) {
    while (temporary != NULL) {
      counter++;
      temporary= temporary->next;
    }
  }
  return counter;
}
