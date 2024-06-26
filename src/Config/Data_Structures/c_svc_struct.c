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

#include "c_svc_struct.h"

sole_var_content_t* svc_create_new_struct() {
  sole_var_content_t *result = malloc(sizeof(sole_var_content_t));
  result->original_location = "";
  result->source_location = "";
  result->keep_source = false;
  result->source_test = false;
  result->source_test_trim_start = "";
  result->source_test_trim_duration = "";



  //validator bool's
  result->is_original_location_valid = false;
  result->is_source_location_valid = false;
  result->is_keep_source_valid = false;

  result->is_source_test_valid = false;
  result->is_source_test_start_valid = false;
  result->is_source_test_duration_valid = false;
  result->is_entire_svc_valid = false;
  return result;
}
