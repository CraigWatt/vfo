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

#ifndef M_TESTS_H
#define M_TESTS_H

#include "../t_internal.h"

#include "../../src/mezzanine.h"
#include "../../src/mezzanine_clean.h"
#include "../../src/Mezzanine/m_internal.h"

void test_o_original_null_ptr(void **state);
void test_mc_sanitize_title_uses_underscores(void **state);
void test_mc_prepare_filename_with_tags_uses_underscores(void **state);
void test_mc_prepare_movie_folder_name_appends_tags(void **state);

#endif // M_TESTS_H
