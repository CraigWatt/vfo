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
#include <stdio.h>

#include "../../src/Config/c_internal.h"

static void u_write_file(const char *path) {
  FILE *fp = fopen(path, "w");
  assert_non_null(fp);
  fputs("test", fp);
  fclose(fp);
}

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

void test_utils_is_file_extension_ts(void **state) {
  (void)state;
  assert_true(utils_is_file_extension_ts("example.ts"));
  assert_false(utils_is_file_extension_ts("example.m2ts"));
  assert_false(utils_is_file_extension_ts("example.mkv"));
}

void test_utils_is_file_extension_valid_supports_ts_original(void **state) {
  (void)state;
  assert_true(utils_is_file_extension_valid("example.ts", "ts_original"));
  assert_true(utils_is_file_extension_valid("example.m2ts", "m2ts_original"));
  assert_false(utils_is_file_extension_valid("example.mp4", "ts_original"));
}

void test_utils_is_file_extension_valid_supports_extended_mezzanine_inputs(void **state) {
  (void)state;
  assert_true(utils_is_file_extension_valid("example.webm", "mkv_original"));
  assert_true(utils_is_file_extension_valid("example.mov", "mp4_original"));
  assert_true(utils_is_file_extension_valid("example.avi", "mp4_original"));
  assert_true(utils_is_file_extension_valid("example.mxf", "mp4_original"));
  assert_true(utils_is_file_extension_valid("example.mts", "m2ts_original"));
  assert_true(utils_is_file_extension_valid("example.mpg", "ts_original"));
  assert_true(utils_is_file_extension_valid("example.mpeg", "ts_original"));
  assert_true(utils_is_file_extension_valid("example.vob", "ts_original"));
  assert_true(utils_is_file_extension_valid("example.webm", "start"));
  assert_true(utils_is_file_extension_valid("example.mov", "mp4_mezzanine"));
  assert_false(utils_is_file_extension_valid("example.txt", "start"));
}

void test_utils_are_custom_folders_type_compliant_accepts_mixed_library(void **state) {
  char root_template[] = "/tmp/vfo-mixed-library-XXXXXX";
  char *root = mkdtemp(root_template);
  char *mixed_root = NULL;
  char *movie_folder = NULL;
  char *movie_file = NULL;
  char *tv_show_folder = NULL;
  char *tv_season_folder = NULL;
  char *tv_episode_folder = NULL;
  char *tv_episode_leaf_folder = NULL;
  char *tv_episode_file = NULL;
  char *allowed_types[] = {"films", "tv", "mixed"};
  char conf[] = "CUSTOM_FOLDER=\"Movies_and_TV_Shows,mixed\"\n";
  cf_node_t *cf_head = NULL;

  (void)state;
  assert_non_null(root);

  mixed_root = utils_combine_to_full_path(root, "Movies_and_TV_Shows");
  movie_folder = utils_combine_to_full_path(mixed_root, "MovieOne");
  movie_file = utils_combine_to_full_path(movie_folder, "MovieOne.mkv");
  tv_show_folder = utils_combine_to_full_path(mixed_root, "ShowOne");
  tv_season_folder = utils_combine_to_full_path(tv_show_folder, "Season_01");
  tv_episode_folder = utils_combine_to_full_path(tv_season_folder, "Episode_01");
  tv_episode_leaf_folder = utils_combine_to_full_path(tv_episode_folder, "Part_01");
  tv_episode_file = utils_combine_to_full_path(tv_episode_leaf_folder, "ShowOne_S01E01.mkv");

  utils_create_folder(mixed_root);
  utils_create_folder(movie_folder);
  utils_create_folder(tv_show_folder);
  utils_create_folder(tv_season_folder);
  utils_create_folder(tv_episode_folder);
  utils_create_folder(tv_episode_leaf_folder);
  u_write_file(movie_file);
  u_write_file(tv_episode_file);

  cf_head = con_extract_to_cf_ll(conf, "CUSTOM_FOLDER=", allowed_types, 3, cf_head);
  utils_are_custom_folders_type_compliant(root, "mkv_original", cf_head);

  unlink(movie_file);
  unlink(tv_episode_file);
  rmdir(tv_episode_leaf_folder);
  rmdir(tv_episode_folder);
  rmdir(tv_season_folder);
  rmdir(tv_show_folder);
  rmdir(movie_folder);
  rmdir(mixed_root);
  rmdir(root);

  free(movie_file);
  free(tv_episode_file);
  free(tv_episode_leaf_folder);
  free(tv_episode_folder);
  free(tv_season_folder);
  free(tv_show_folder);
  free(movie_folder);
  free(mixed_root);
}
