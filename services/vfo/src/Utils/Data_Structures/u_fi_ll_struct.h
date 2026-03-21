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

#ifndef U_FI_LL_STRUCT_H
#define U_FI_LL_STRUCT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* ----------folder_items_linked_list header file---------- */
/*
 * This fi ll was created because I started to hit the OS list of (openfiles)
 * due to the sheer number of opendir occurring in recursive functions
 * related to tv type custom_folders
 *
 *Please look closely at this particular function: tv_open_folder_for_move in original.c
 *
 * As this shows the alternative way to retain using recursion but to then activate the next functions
 * when the opendir's are closed.
 * we instead just reference the names within the linked list to continue scanning through each folder
 *
 * What functions use this?:
 * 1.
 * 2.
 * 3.
 * 4.
 */

struct fi_node {
  char *folder1;
  char *folder2;
  char *define_l1_f;
  char *define_l2_f;
  char *rule;
  char *layer;
  struct fi_node *next;
};
typedef struct fi_node fi_node_t;



fi_node_t* fi_create_new_node(char *folder1, char *folder2, char *define_l1_f, char *define_l2_f, char *rule, char *layer);
void fi_print_list(fi_node_t *head);
fi_node_t* fi_insert_at_head(fi_node_t **head, fi_node_t *node_to_insert);
fi_node_t* fi_find_node(fi_node_t *head, char *folder_name);
int fi_get_count(fi_node_t *head);
void fi_insert_after_node(fi_node_t *node_to_insert_after, fi_node_t* newnode);
bool fi_folder_name_exits(fi_node_t *head, char *folder_name);

#endif // UI_FI_LL_STRUCT_H
