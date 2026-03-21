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

#ifndef C_INTERNAL_H
#define C_INTERNAL_H
/* ----------Config---------- */
/*
 * The main role of Config is to extract all valid & invalid data from conf file
 */

#define LOWER 1024
#define UPPER 65536

#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

//include necessary public facing headers
#include "../config.h"
#include "../utils.h"

//include relevant structs
#include "Data_Structures/c_svm_struct.h"
#include "Data_Structures/c_svc_struct.h"

//include relevant ll structs
#include "Data_Structures/c_cf_ll_struct.h"
#include "Data_Structures/c_ca_ll_struct.h" //<ca itself included cs  ll as this is always located in a ca node (ll within an ll)
#include "Data_Structures/c_uw_ll_struct.h"

//include relevant struct container (like an instance of an object)
#include "Data_Structures/c_config_struct.h"


char* con_find_config_file(const char *config_dir, const char* config_filename, char *full_conf_path);
void con_does_config_file_exist(char *location);
char* con_extract_to_string(char *full_conf_path);
void con_access_config_variable();
void con_extract_to_sole_vars(char* string_conf, sole_var_markers_t *sole_var_markers, sole_var_content_t* sole_var_content);
void con_extract_to_linked_lists();
void con_set_default_sole_var_markers(sole_var_markers_t *sole_var_markers);
int con_word_count(char* string, char* word);
void con_set_default_sole_var_content(sole_var_content_t *sole_var_content);
void con_get_sole_marker_count(char *string_conf, char *word);
int con_get_atleast_one_marker_count(char *string_conf, char *word);
int con_get_marker_count(char *string_conf, char *word);
void con_get_sole_var_content(char *string_conf, char *marker, char *content);
char* con_fetch_sole_var_content(char *string_conf, char *content);
char* con_fetch_custom_folder_var_content(char *string_conf, char *marker, int iteration);
cf_node_t* con_extract_to_cf_ll(char *conf_string, char *cf_marker, char **cf_type, int cf_size, cf_node_t *cf_head);
ca_node_t* con_extract_to_ca_ll(char *conf_string, char *ca_marker, ca_node_t *ca_head);
char* con_split_before_comma(char *string);
char* con_split_after_comma(char *string);
bool con_detect_a_word(char *full_string, char *word);
void con_set_essentials(sole_var_content_t sole_var_content);
extern sole_var_content_t con_get_essentials();
void con_set_custom_folders_ll(cf_node_t *cf_head);
extern cf_node_t* con_get_custom_folders_ll();
void con_set_custom_alias_ll(ca_node_t *ca_head);
extern ca_node_t* con_get_custom_alias_ll();

// void set_var_cont(variable_content_t);
extern sole_var_content_t get_config_essential_variable_content_struct_byval();
// extern variable_content_t* get_var_cont_ptr();

//c_messages
void c_mes_init_find_config_file();
void c_mes_find_config_file_success();
void c_mes_find_config_file_fail();
void c_mes_init_access_config_variable();
void c_mes_extract_to_string();
void c_mes_init_extract_to_variables();
void c_mes_init_generate_default_config_file();
void c_mes_particular_variable_has_problems(char* var_name_string, const char* conf_file_name, int count);
void c_mes_essential_variable_found(char* var_name_string);

#endif // C_INTERNAL_H
