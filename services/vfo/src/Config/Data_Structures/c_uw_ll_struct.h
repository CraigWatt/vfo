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

#ifndef C_UNKNOWN_WORDS_LINKED_LIST_H
#define C_UNKNOWN_WORDS_LINKED_LIST_H

/* ----------custom_alias_linked_list header file---------- */
/*
 * The purpose of uw ll is to isolate each unknown word provided by the user for further process.
 * This has been made into a linked list to remain scalable, e.g. say user provides many alias name commands
 * we need to isolate the unknown words first, and then check to see if the match alias names defined in the 
 * conf file from ca ll.
 *
 * What functions use this?:
 * 1. This should only ever be used in Handover (which is essentially a sub area of Config)
 * It future it might be a good idea to condense Handover to Config and just ensure the processes
 * are crystal clear based on the ordering and naming of the Config functions
 * e.g. Config phase 1 gathers all valid/invalid conf file data
 * Phase 2/Handover uses 'handover flags' passed on to Config from InputHandler
 * to populate only the RELEVANT objects for future use.
 *it is up to the 'objects' to validate their own data.
 *Conf is only concerned if it CAN'T extract properly from conf file...
 */

 struct uw_node {
  char *unknown_word;
  struct uw_node *next;
};

typedef struct uw_node uw_node_t;

uw_node_t* uw_create_new_empty_node();
uw_node_t* uw_create_new_node(char *unknown_word);
void uw_print_list(uw_node_t *head);
uw_node_t* uw_insert_at_head(uw_node_t **head, uw_node_t *node_to_insert);
uw_node_t* uw_find_node(uw_node_t *head, char *unknown_word);
int uw_get_count(uw_node_t *head);
void uw_insert_after_node(uw_node_t *node_to_insert_after, uw_node_t* newnode);
bool uw_unknown_word_exits(uw_node_t *head, char *unknown_word);
char* uw_get_a_unknown_word_from_count(uw_node_t *head, int count);

 #endif // UNKNOWN_WORDS_LINKED_LIST_H
