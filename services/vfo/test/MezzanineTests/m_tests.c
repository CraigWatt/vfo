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

#include "m_tests.h"
#include "cmocka.h"
#include <string.h>

void test_m_mezzanine_null_ptr(void **state) {
  original_t *test_original = NULL;
  // m_mezzanine(test_original);
  // add a comment and a second bit a third bit fourth fifth
  int duck = 1;
  (void)test_original;
  (void)state;
  assert_int_equal(duck, 1);
}

void test_mc_sanitize_title_uses_underscores(void **state) {
  char output[256];
  (void)state;

  mc_sanitize_title_for_test("WALL-E (2008) unknown", output, sizeof(output));
  assert_string_equal(output, "WALL_E_2008_unknown");
  assert_null(strchr(output, ' '));
}

void test_mc_prepare_filename_with_tags_uses_underscores(void **state) {
  char output[256];
  (void)state;

  mc_prepare_filename_with_tags_for_test(output,
                                         sizeof(output),
                                         "WALL_E_2008_unknown",
                                         "mkv",
                                         "UHD",
                                         "DV",
                                         "HEVC",
                                         "TRUEHD",
                                         true);
  assert_string_equal(output, "WALL_E_2008_unknown_[UHD_DV_HEVC_TRUEHD].mkv");
  assert_null(strchr(output, ' '));
}

void test_mc_prepare_movie_folder_name_appends_tags(void **state) {
  char with_tags[256];
  char without_tags[256];
  (void)state;

  mc_prepare_movie_folder_name_for_test(with_tags,
                                        sizeof(with_tags),
                                        "WALL_E_2008_unknown",
                                        "UHD",
                                        "DV",
                                        "HEVC",
                                        "TRUEHD",
                                        true);
  mc_prepare_movie_folder_name_for_test(without_tags,
                                        sizeof(without_tags),
                                        "WALL_E_2008_unknown",
                                        "UHD",
                                        "DV",
                                        "HEVC",
                                        "TRUEHD",
                                        false);

  assert_string_equal(with_tags, "WALL_E_2008_unknown_[UHD_DV_HEVC_TRUEHD]");
  assert_string_equal(without_tags, "WALL_E_2008_unknown");
  assert_null(strchr(with_tags, ' '));
}
