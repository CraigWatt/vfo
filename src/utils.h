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

#ifndef UTILS_H
#define UTILS_H

#include "Utils/Data_Structures/u_fi_ll_struct.h"

#include "Utils/Data_Structures/u_active_cf_ll_struct.h"

#include "Utils/Data_Structures/u_active_films_f_ll_struct.h"

#include "Utils/Data_Structures/u_active_tv_f_ll_struct.h"

#include "Config/Data_Structures/c_cf_ll_struct.h"


extern bool utils_does_folder_exist(char *location);
extern void utils_create_folder(char *location);
extern char* utils_combine_to_full_path(const char *directory, const char *filename);
extern bool utils_does_file_exist(char *location);
extern void extract_file_to_string();
extern char* utils_uppercase_string(char *s);
extern char* utils_lowercase_string(char *s);
extern char *utils_combine_strings(char *start_string, char*end_string);
extern bool utils_is_folder_empty(char *folder);
extern void utils_replace_spaces(char *file_name, char *parent_directory);
extern bool utils_is_file_extension_valid(char *file_name, char* rf_rules);
extern bool utils_file_is_macos_hidden_files(char *file_name);
extern bool utils_directory_is_current_or_parent(char *folder_name);
extern void utils_found_a_rogue_file(char *file_name, char *folder);
extern void utils_wish_to_continue(char *previous_process, char *next_action);
extern bool utils_is_file_extension_mkv(char *file_name);
extern bool utils_is_file_extension_mp4(char *file_name);
extern bool utils_does_folder_contain_file_with_extension(char *folder, char *extension);
extern char* utils_fetch_single_file(char *folder);
extern char* utils_fetch_single_file_name(char *folder);
extern bool utils_string_is_empty_or_spaces(char *string);
extern void utils_ask_user_for_permission_to_create_a_folder(char *folder);
extern bool utils_is_string_a_reserved_word(char *string);
extern void utils_does_folder_contain_valid_custom_folders(char *folder, cf_node_t *cf_ll_head);
extern void utils_is_folder_missing_custom_folders(char *folder, cf_node_t *cf_ll_head);
extern void utils_are_custom_folders_type_compliant(char *root_folder, char* rf_rules, cf_node_t *cf_ll_head);
// extern void utils_wipe_start_empty_tv_folders(char *folder);
extern void utils_is_custom_folder_films_type_compliant(char *custom_folder, char *rf_rules, char *layer);
extern void utils_is_custom_folder_tv_type_compliant(char *custom_folder, char *rf_rules, char *layer);
extern void utils_wipe_here(char *folder);
extern bool utils_string_contains_char(char *string, char c);
extern bool utils_string_contains_single_occurrence_of_char(char *string, char c);
extern bool utils_string_contains_multiple_occurrences_of_char(char *string, char c);
extern bool utils_string_array_contains_string(char **array_of_strings, int array_size, char *string);
extern bool utils_string_array_contains_single_occurrence_of_string(char **array_of_strings, int array_size, char *string);
extern bool utils_string_array_contains_multiple_occurrences_of_string(char **array_of_strings, int array_size, char *string);
extern char* utils_fmt_ordinal(int number);
extern void utils_remove_duplicates_in_string(char *string);
extern char** utils_remove_duplicates_in_array_of_strings(char *array[], int length, int *new_length);

extern active_cf_node_t* utils_generate_from_to_ll(cf_node_t *cf_head, char *from_cf_parent_folder, char *to_cf_parent_folder);

extern void utils_delete_folder_if_it_is_empty(char *folder);

char* utils_convert_integer_to_char(int N);

extern char* utils_replace_characters(char *before, char *oldsub, char *newsub);

extern void utils_danger_delete_contents_of_folder(char *folder);

extern int utils_convert_string_to_integer(char *S);

extern bool utils_is_substring(char *check, char *string);

extern void utils_create_error_encoding_file(char *location, char *candidate_name);

extern bool utils_string_only_contains_number_characters(char *string);

extern bool utils_string_is_ffmpeg_timecode_compliant(char *string);

#endif // UTILS_H
