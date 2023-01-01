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

#ifndef U_ACTIVE_CF_LL_STRUCT_H
#define U_ACTIVE_CF_LL_STRUCT_H

#include "u_active_films_f_ll_struct.h"
#include "u_active_tv_f_ll_struct.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* utils active custom folder linked list */
/*
 * This ll was created as a precaution to avoid hitting OS open files limit
 * This ll is specifically used when opening custom folders (first layer)
 * of any folder that is supposed to be storing custom folders,

 * this ll should exclusively be used by utils_custom_folder_type_function_pointer()
 * function
 */

 struct active_cf_node {
  char *from_cf_folder;
  char *to_cf_folder;
  char *type;

  //if type = films, this will be used
  struct active_films_f_node *active_films_f_head;
  //if type = tv, this will be used
  struct active_tv_f_node *active_tv_f_head;

  struct active_cf_node *next;
};
typedef struct active_cf_node active_cf_node_t;

active_cf_node_t* active_cf_create_new_node(char *from_cf_folder, char *to_cf_folder, char *type);
active_cf_node_t* active_cf_create_node(char *from_cf_folder, char *to_cf_folder, char *type, active_films_f_node_t *active_films_f_head, active_tv_f_node_t * active_tv_f_head);
void active_cf_print_list(active_cf_node_t *head);
active_cf_node_t* active_cf_insert_at_head(active_cf_node_t **head, active_cf_node_t *node_to_insert);
void active_cf_insert_after_node(active_cf_node_t *node_to_insert_after, active_cf_node_t *newnode);
int active_cf_get_count(active_cf_node_t *head);

#endif // U_ACTIVE_CF_LL_STRUCT_H
