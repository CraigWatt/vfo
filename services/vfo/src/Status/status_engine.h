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

#ifndef STATUS_ENGINE_H
#define STATUS_ENGINE_H

#include <stdbool.h>
#include <stdio.h>

/* Status lifecycle used by vfo observability snapshots. */
typedef enum status_state {
  STATUS_STATE_PENDING = 0,
  STATUS_STATE_IN_PROGRESS,
  STATUS_STATE_COMPLETE,
  STATUS_STATE_ERROR,
  STATUS_STATE_SKIPPED
} status_state_t;

struct status_component {
  char *name;
  status_state_t state;
  char *detail;
  struct status_component *next;
};
typedef struct status_component status_component_t;

struct status_report {
  status_component_t *head;
  int count;
};
typedef struct status_report status_report_t;

status_report_t* status_report_create(void);
void status_report_free(status_report_t *report);
void status_report_update(status_report_t *report,
                          const char *name,
                          status_state_t state,
                          const char *detail);

int status_report_count(const status_report_t *report);
bool status_report_has_state(const status_report_t *report, status_state_t state);
bool status_report_has_errors(const status_report_t *report);
bool status_report_is_healthy(const status_report_t *report);

const char* status_state_to_string(status_state_t state);
void status_report_summary(const status_report_t *report,
                           int *pending,
                           int *in_progress,
                           int *complete,
                           int *error,
                           int *skipped);

void status_report_print_text(const status_report_t *report, FILE *stream);
void status_report_print_json(const status_report_t *report, FILE *stream);

#endif // STATUS_ENGINE_H
