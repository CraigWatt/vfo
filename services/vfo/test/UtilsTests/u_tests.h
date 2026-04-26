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

#ifndef U_TESTS_H
#define U_TESTS_H

#include "../t_internal.h"

#include "../../src/utils.h"
#include "../../src/Utils/u_internal.h"

void test_utils_does_folder_exist_null_ptr(void **state);
void test_utils_does_folder_exist_empty_str(void **state);
void test_utils_split_semicolon_list_trims_and_counts(void **state);
void test_utils_location_pool_create_and_map_path(void **state);
void test_utils_create_folder_creates_accessible_directory(void **state);
void test_utils_is_file_extension_ts(void **state);
void test_utils_is_file_extension_valid_supports_ts_original(void **state);
void test_utils_is_file_extension_valid_supports_extended_mezzanine_inputs(void **state);
void test_utils_are_custom_folders_type_compliant_accepts_mixed_library(void **state);
void test_utils_are_custom_folders_type_compliant_ignores_hidden_transient_dirs(void **state);
void test_utils_prepared_dv_p81_variant_is_preferred_single_movie_file(void **state);

#endif // U_TESTS_H
