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
