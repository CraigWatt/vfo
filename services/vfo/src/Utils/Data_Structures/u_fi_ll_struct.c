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

#include "u_fi_ll_struct.h"

fi_node_t *fi_create_new_node(char *folder1, char *folder2, char *define_l1_f, char *define_l2_f, char *rule, char *layer){
  fi_node_t *result = malloc(sizeof(fi_node_t));
  result->folder1 = folder1;
  result->folder2 = folder2;
  result->define_l1_f = define_l1_f;
  result->define_l2_f = define_l2_f;
  result->rule = rule;
  result->layer = layer;
  result->next = NULL;
  return result;
}

fi_node_t *fi_insert_at_head(fi_node_t **head, fi_node_t *node_to_insert) {
  node_to_insert->next = *head;
  *head = node_to_insert;
  return node_to_insert;
}

void fi_insert_after_node(fi_node_t *node_to_insert_after, fi_node_t *newnode) {
  newnode->next = node_to_insert_after->next;
  node_to_insert_after->next = newnode;
}

fi_node_t *fi_find_node(fi_node_t *head, char *folder1) {
  fi_node_t *tmp = head;
  while (tmp != NULL) {
    // this doesn't work for char's
    if(strcmp(tmp->folder1, folder1) == 0) return tmp;
    tmp = tmp->next;
  }
  return NULL;
}

bool fi_folder_name_exits(fi_node_t *head, char *folder1) {
  fi_node_t *tmp = head;
  while (tmp != NULL) {
    if (strcmp(tmp->folder1, folder1) == 0) return true;
    tmp = tmp->next;
  }
  return false;
}

void fi_print_list(fi_node_t *head) {
  fi_node_t *temp = head;

  if(temp != NULL) {
    while (temp != NULL) {
      printf ("%s (%s, %s, %s, %s, %s)- \n", temp->folder1, temp->folder2, temp->define_l1_f, temp->define_l2_f, temp->rule, temp->layer);
      temp = temp->next;
    }
    printf("\n");
  } else printf("list is empty\n");
}

int fi_get_count(fi_node_t *head) {
  fi_node_t *temporary = head;
  int counter = 0;
  if(temporary != NULL) {
    while (temporary != NULL) {
      counter++;
      temporary= temporary->next;
    }
  }
  return counter;
}
