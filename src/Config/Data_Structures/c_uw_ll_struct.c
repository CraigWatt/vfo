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

#include "c_uw_ll_struct.h"


uw_node_t* uw_create_new_empty_node() {
  uw_node_t *result = malloc(sizeof(uw_node_t));
  result->unknown_word = "";
  result->next = NULL;
  return result;
}

uw_node_t* uw_create_new_node(char *unknown_word){
  uw_node_t *result = malloc(sizeof(uw_node_t));
  result->unknown_word = unknown_word;
  result->next = NULL;
  return result;
}

uw_node_t* uw_insert_at_head(uw_node_t **head, uw_node_t *node_to_insert) {
  node_to_insert->next = *head;
  *head = node_to_insert;
  return node_to_insert;
}

void uw_insert_after_node(uw_node_t *node_to_insert_after, uw_node_t* newnode) {
  newnode->next = node_to_insert_after->next;
  node_to_insert_after->next = newnode;
}

uw_node_t* uw_find_node(uw_node_t *head, char *unknown_word) {
  uw_node_t *tmp = head;
  while (tmp != NULL) {
    // this doesn't work for char's
    if(strcmp(tmp->unknown_word, unknown_word) == 0) return tmp;
    tmp = tmp->next;
  }
  return NULL;
}

bool uw_unknown_word_exits(uw_node_t *head, char *unknown_word) {
  uw_node_t *tmp = head;
  while (tmp != NULL) {
    if (strcmp(tmp->unknown_word, unknown_word) == 0) return true;
    tmp = tmp->next;
  }
  return false;
}

void uw_print_list(uw_node_t *head) {
  uw_node_t *temp = head;

  if(temp != NULL) {
    while (temp != NULL) {
      printf ("%s ()- \n", temp->unknown_word);
      temp = temp->next;
    }
    printf("\n");
  } else printf("list is empty\n");
}


int uw_get_count(uw_node_t *head) {
  uw_node_t *temporary = head;
  int counter = 0;
  if(temporary != NULL) {
    while (temporary != NULL) {
      counter++;
      temporary= temporary->next;
    }
  }
  return counter;
}

char* uw_get_a_unknown_word_from_count(uw_node_t *head, int count) {
  uw_node_t *temporary = head;
  int counter = 0;
  bool unknown_word_found = false;
  if(temporary != NULL) {
    while(temporary != NULL) {
      if(counter == count) {
        unknown_word_found = true;
        break;
      }
      counter++;
      temporary = temporary->next;
    }
  }
  if(unknown_word_found == true) {
    return temporary->unknown_word;
  }
  return 0;
}
