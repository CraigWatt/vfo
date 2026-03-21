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

#include "ih_arguments_struct.h"


arguments_t* arguments_create_new_struct() {
  arguments_t *result = malloc(sizeof(arguments_t));
  result->original_detected = false;
  result->revert_detected = false;
  result->source_detected = false;
  result->all_aliases_detected = false;
  result->do_it_all_detected = false;
  result->wipe_detected = false;

  result->alias_queue_detected = false;

  return result;
}

void ih_arguments_parser(int argc, char **argv, arguments_t* arguments) {  
  if(utils_string_array_contains_string(argv, argc, "original"))
    arguments->original_detected = true;
  if(utils_string_array_contains_string(argv, argc, "revert"))
    arguments->revert_detected = true;
  if(utils_string_array_contains_string(argv, argc, "source"))
    arguments->source_detected = true;
  if(utils_string_array_contains_string(argv, argc, "all_aliases"))
    arguments->all_aliases_detected = true;
  if(utils_string_array_contains_string(argv, argc, "do_it_all"))
    arguments->do_it_all_detected = true;
  if(utils_string_array_contains_string(argv, argc, "wipe"))
    arguments->wipe_detected = true;
}
