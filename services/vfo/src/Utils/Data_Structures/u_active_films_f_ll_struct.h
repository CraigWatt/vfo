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

#ifndef U_ACTIVE_FILMS_F_LL_STRUCT_H
#define U_ACTIVE_FILMS_F_LL_STRUCT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* utils active films folder linked list struct */
/*
 *
 */

 struct active_films_f_node {
  char *from_films_f_folder;
  char *to_films_f_folder;

  //it is unnecessary for layer information for films because in all cases 
  //we have a single film folder and within that a single film file + subtitles etc
  //no need to know more...

  struct active_films_f_node *next;
};
typedef struct active_films_f_node active_films_f_node_t;

active_films_f_node_t* active_films_f_create_node(char *from_films_f_folder, char *to_films_f_folder);
void active_films_f_print_list(active_films_f_node_t *head);
active_films_f_node_t* active_films_f_insert_at_head(active_films_f_node_t **head, active_films_f_node_t *node_to_insert);
void active_films_f_insert_after_node(active_films_f_node_t *node_to_insert_after, active_films_f_node_t *newnode);
int active_films_f_get_count(active_films_f_node_t *head);

#endif // U_ACTIVE_FILMS_F_LL_STRUCT_H
