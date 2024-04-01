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

#ifndef C_SVC_STRUCT_H
#define C_SVC_STRUCT_H
/*Sole Var Content Struct*/

#include <stdlib.h>
#include <stdbool.h>

struct sole_var_content {
  char* original_location;
  char* source_location;
  bool keep_source;

  bool source_test;
  int source_test_trim_start;
  int source_test_trim_duration;

  bool is_original_location_valid;
  bool is_source_location_valid;
  bool is_keep_source_valid;

  bool is_source_test_valid;
  bool is_source_test_start_valid;
  bool is_source_test_duration_valid;

  bool is_entire_svc_valid;
};
typedef struct sole_var_content sole_var_content_t;

sole_var_content_t* svc_create_new_struct();

#endif // C_SVC_STRUCT_H
