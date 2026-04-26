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

#include "c_tests.h"

/**
 * test if @param config_dir is NULL
 * DEV: need to improve this
 */
void test_con_init_config_dir_null_ptr(void **state) {
  const char *config_dir = "";
  int revised_argv_length = 1;
  char **revised_argv = malloc(revised_argv_length * sizeof(char*));
  int revised_argc = 1;
  // config_t *test_config = con_init(config_dir, revised_argv, revised_argc);

  //need to improve this

  assert_int_equal(revised_argc, 1);
}

/**
 * test if @param config_dir is empty str
 * DEV: need to improve this
 */
void test_con_init_config_dir_empty_str(void **state) {
  const char *config_dir = "";
  int revised_argv_length = 1;
  char **revised_argv = malloc(revised_argv_length * sizeof(char*));
  int revised_argc = 1;
  // config_t *test_config = con_init(config_dir, revised_argv, revised_argc);

  //need to improve this
  
  assert_int_equal(revised_argc, 1);
}

void test_con_word_count_ignores_partial_marker_matches(void **state) {
  char conf[] =
    "QUALITY_CHECK_MAX_FILES_PER_PROFILE=\"0\"\n"
    "PROFILE=\"netflixy\"\n";
  (void)state;

  assert_int_equal(con_word_count(conf, "PROFILE="), 1);
  assert_int_equal(con_word_count(conf, "QUALITY_CHECK_MAX_FILES_PER_PROFILE="), 1);
}

void test_con_fetch_profile_marker_ignores_quality_suffix_marker(void **state) {
  char conf[] =
    "QUALITY_CHECK_MAX_FILES_PER_PROFILE=\"0\"\n"
    "PROFILE=\"netflixy_preserve_audio_main_subtitle_intent_1080p\"\n";
  char *value = NULL;
  (void)state;

  value = con_fetch_custom_folder_var_content(conf, "PROFILE=", 1);
  assert_non_null(value);
  assert_string_equal(value, "netflixy_preserve_audio_main_subtitle_intent_1080p");
  free(value);
}

void test_con_extract_custom_folder_accepts_mixed_type(void **state) {
  char conf[] =
    "CUSTOM_FOLDER=\"Movies_and_TV_Shows,mixed\"\n";
  char *allowed_types[] = {"films", "tv", "mixed"};
  cf_node_t *head = NULL;
  (void)state;

  head = con_extract_to_cf_ll(conf, "CUSTOM_FOLDER=", allowed_types, 3, head);
  assert_non_null(head);
  assert_int_equal(cf_get_count(head), 1);
  assert_string_equal(cf_get_type_from_folder_name(head, "Movies_and_TV_Shows"), "mixed");
}
