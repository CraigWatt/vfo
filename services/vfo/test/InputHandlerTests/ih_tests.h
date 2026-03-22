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

#ifndef IH_TESTS_H
#define IH_TESTS_H

#include "../t_internal.h"

#include "../../src/InputHandler/Data_Structures/ih_arguments_struct.h"
#include "../../src/quality_scoring.h"
#include "../../src/Status/status_engine.h"

void ih_resolve_config_dir_for_test(char *output, size_t output_size, bool wizard_mode);

void test_status_report_update_and_summary(void **state);
void test_status_report_update_overwrites_existing_component(void **state);
void test_status_report_print_json_contract(void **state);
void test_ih_arguments_parser_detects_status_commands(void **state);
void test_ih_arguments_parser_detects_mezzanine_clean_commands(void **state);
void test_quality_reference_mode_parser_accepts_valid_values(void **state);
void test_quality_reference_mode_parser_rejects_invalid_values(void **state);
void test_ih_resolve_config_dir_prefers_env_override(void **state);
void test_ih_resolve_config_dir_uses_user_dir_for_wizard(void **state);

#endif // IH_TESTS_H
