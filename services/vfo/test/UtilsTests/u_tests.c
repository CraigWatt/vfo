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

#include "u_tests.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

void test_utils_does_folder_exist_null_ptr(void **state) {
  (void)state;
  bool result = utils_does_folder_exist(NULL);
  assert_false(result);
}

void test_utils_does_folder_exist_empty_str(void **state) {
  (void)state;
  char *empty_string = "";
  bool result = utils_does_folder_exist(empty_string);
  assert_string_equal(empty_string, "");
  assert_false(result);
}

void test_utils_split_semicolon_list_trims_and_counts(void **state) {
  (void)state;
  int count = 0;
  char **items = utils_split_semicolon_list(" /tmp ; /var/tmp ;/opt ", &count);
  assert_int_equal(count, 3);
  assert_string_equal(items[0], "/tmp");
  assert_string_equal(items[1], "/var/tmp");
  assert_string_equal(items[2], "/opt");
  utils_free_string_array(items, count);
}

void test_utils_location_pool_create_and_map_path(void **state) {
  (void)state;
  utils_location_pool_t *pool = utils_location_pool_create("/tmp", "/tmp", "90", 95, 0ULL);
  char *mapped = NULL;

  assert_non_null(pool);
  assert_int_equal(pool->count, 1);
  assert_int_equal(pool->max_usage_pct[0], 90);
  mapped = utils_location_pool_map_path(pool,
                                        0,
                                        "/Volumes/A/source/content/Movies/Test",
                                        "/Volumes/A/source/content");
  assert_string_equal(mapped, "/tmp/Movies/Test");

  free(mapped);
  utils_location_pool_free(pool);
}

void test_utils_create_folder_creates_accessible_directory(void **state) {
  (void)state;
  char parent_template[] = "/tmp/vfo-utils-create-folder-XXXXXX";
  char *parent = mkdtemp(parent_template);
  char *child = NULL;
  struct stat st;

  assert_non_null(parent);

  child = utils_combine_to_full_path(parent, "child");
  assert_non_null(child);

  utils_create_folder(child);

  assert_int_equal(stat(child, &st), 0);
  assert_true(S_ISDIR(st.st_mode));
  assert_true((st.st_mode & S_IRUSR) != 0);
  assert_true((st.st_mode & S_IWUSR) != 0);
  assert_true((st.st_mode & S_IXUSR) != 0);
  assert_int_equal(access(child, R_OK | W_OK | X_OK), 0);

  assert_int_equal(rmdir(child), 0);
  assert_int_equal(rmdir(parent), 0);
  free(child);
}
