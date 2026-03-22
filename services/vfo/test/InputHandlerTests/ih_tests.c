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

#include "ih_tests.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test_status_report_update_and_summary(void **state) {
  status_report_t *report = NULL;
  int pending = 0;
  int in_progress = 0;
  int complete = 0;
  int error = 0;
  int skipped = 0;
  (void)state;

  report = status_report_create();
  assert_non_null(report);

  status_report_update(report, "engine.snapshot", STATUS_STATE_IN_PROGRESS, "collecting");
  status_report_update(report, "dependency.ffmpeg", STATUS_STATE_COMPLETE, "available");
  status_report_update(report, "stage.source", STATUS_STATE_SKIPPED, "KEEP_SOURCE=false");

  assert_int_equal(status_report_count(report), 3);
  assert_true(status_report_has_state(report, STATUS_STATE_IN_PROGRESS));
  assert_true(status_report_has_state(report, STATUS_STATE_COMPLETE));
  assert_true(status_report_has_state(report, STATUS_STATE_SKIPPED));
  assert_false(status_report_has_errors(report));

  status_report_summary(report, &pending, &in_progress, &complete, &error, &skipped);
  assert_int_equal(pending, 0);
  assert_int_equal(in_progress, 1);
  assert_int_equal(complete, 1);
  assert_int_equal(error, 0);
  assert_int_equal(skipped, 1);

  status_report_free(report);
}

void test_status_report_update_overwrites_existing_component(void **state) {
  status_report_t *report = NULL;
  (void)state;

  report = status_report_create();
  assert_non_null(report);

  status_report_update(report, "dependency.ffprobe", STATUS_STATE_ERROR, "missing");
  status_report_update(report, "dependency.ffprobe", STATUS_STATE_COMPLETE, "available");

  assert_int_equal(status_report_count(report), 1);
  assert_true(status_report_has_state(report, STATUS_STATE_COMPLETE));
  assert_false(status_report_has_errors(report));

  status_report_free(report);
}

void test_status_report_print_json_contract(void **state) {
  status_report_t *report = NULL;
  FILE *stream = NULL;
  char output[4096];
  size_t read_bytes = 0;
  (void)state;

  report = status_report_create();
  assert_non_null(report);

  status_report_update(report, "engine.snapshot", STATUS_STATE_COMPLETE, "ok");
  status_report_update(report, "dependency.ffmpeg", STATUS_STATE_COMPLETE, "available");
  status_report_update(report, "stage.source", STATUS_STATE_SKIPPED, "KEEP_SOURCE=false");

  stream = tmpfile();
  assert_non_null(stream);

  status_report_print_json(report, stream);
  rewind(stream);
  read_bytes = fread(output, 1, sizeof(output) - 1, stream);
  output[read_bytes] = '\0';

  assert_non_null(strstr(output, "\"schema_version\":1"));
  assert_non_null(strstr(output, "\"healthy\":true"));
  assert_non_null(strstr(output, "\"components\":["));
  assert_non_null(strstr(output, "\"summary\":{"));

  fclose(stream);
  status_report_free(report);
}

void test_ih_arguments_parser_detects_status_commands(void **state) {
  arguments_t *arguments = NULL;
  char *argv[] = {"vfo", "status", "status-json", "status_json", "doctor"};
  int argc = 5;
  (void)state;

  arguments = arguments_create_new_struct();
  assert_non_null(arguments);

  ih_arguments_parser(argc, argv, arguments);
  assert_true(arguments->status_detected);
  assert_true(arguments->status_json_detected);
  assert_true(arguments->doctor_detected);

  free(arguments);
}

void test_ih_arguments_parser_detects_mezzanine_clean_commands(void **state) {
  arguments_t *arguments = NULL;
  char *argv[] = {"vfo", "mezzanine-clean", "mezzanine_clean"};
  int argc = 3;
  (void)state;

  arguments = arguments_create_new_struct();
  assert_non_null(arguments);

  ih_arguments_parser(argc, argv, arguments);
  assert_true(arguments->mezzanine_clean_detected);

  free(arguments);
}

void test_quality_reference_mode_parser_accepts_valid_values(void **state) {
  bool valid = false;
  (void)state;

  assert_int_equal(quality_reference_mode_from_string("auto", &valid), QUALITY_REFERENCE_AUTO);
  assert_true(valid);

  assert_int_equal(quality_reference_mode_from_string("source", &valid), QUALITY_REFERENCE_SOURCE);
  assert_true(valid);

  assert_int_equal(quality_reference_mode_from_string("mezzanine", &valid), QUALITY_REFERENCE_MEZZANINE);
  assert_true(valid);
}

void test_quality_reference_mode_parser_rejects_invalid_values(void **state) {
  bool valid = true;
  (void)state;

  assert_int_equal(quality_reference_mode_from_string("invalid_mode", &valid), QUALITY_REFERENCE_AUTO);
  assert_false(valid);
}
