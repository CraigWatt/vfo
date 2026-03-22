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

#include "status_engine.h"

#include <stdlib.h>
#include <string.h>

static status_component_t* status_find_component(status_report_t *report, const char *name) {
  status_component_t *current = NULL;

  if(report == NULL || name == NULL)
    return NULL;

  current = report->head;
  while(current != NULL) {
    if(strcmp(current->name, name) == 0)
      return current;
    current = current->next;
  }

  return NULL;
}

static void status_print_json_escaped(FILE *stream, const char *value) {
  size_t i = 0;

  if(stream == NULL)
    return;

  if(value == NULL)
    return;

  for(i = 0; value[i] != '\0'; i++) {
    unsigned char character = (unsigned char)value[i];

    if(character == '\\') {
      fputs("\\\\", stream);
    } else if(character == '"') {
      fputs("\\\"", stream);
    } else if(character == '\n') {
      fputs("\\n", stream);
    } else if(character == '\r') {
      fputs("\\r", stream);
    } else if(character == '\t') {
      fputs("\\t", stream);
    } else if(character < 0x20) {
      fprintf(stream, "\\u%04x", character);
    } else {
      fputc(character, stream);
    }
  }
}

status_report_t* status_report_create(void) {
  status_report_t *report = malloc(sizeof(status_report_t));

  if(report == NULL)
    return NULL;

  report->head = NULL;
  report->count = 0;
  return report;
}

void status_report_free(status_report_t *report) {
  status_component_t *current = NULL;
  status_component_t *next = NULL;

  if(report == NULL)
    return;

  current = report->head;
  while(current != NULL) {
    next = current->next;
    free(current->name);
    free(current->detail);
    free(current);
    current = next;
  }

  free(report);
}

void status_report_update(status_report_t *report,
                          const char *name,
                          status_state_t state,
                          const char *detail) {
  status_component_t *component = NULL;
  status_component_t *tail = NULL;

  if(report == NULL || name == NULL)
    return;

  component = status_find_component(report, name);
  if(component == NULL) {
    component = malloc(sizeof(status_component_t));
    if(component == NULL)
      return;

    component->name = strdup(name);
    if(component->name == NULL) {
      free(component);
      return;
    }

    component->detail = strdup("");
    if(component->detail == NULL) {
      free(component->name);
      free(component);
      return;
    }

    component->state = STATUS_STATE_PENDING;
    component->next = NULL;

    if(report->head == NULL)
      report->head = component;
    else {
      tail = report->head;
      while(tail->next != NULL)
        tail = tail->next;
      tail->next = component;
    }

    report->count++;
  }

  component->state = state;

  free(component->detail);
  if(detail != NULL)
    component->detail = strdup(detail);
  else
    component->detail = strdup("");

  if(component->detail == NULL)
    component->detail = strdup("");
}

int status_report_count(const status_report_t *report) {
  if(report == NULL)
    return 0;
  return report->count;
}

bool status_report_has_state(const status_report_t *report, status_state_t state) {
  status_component_t *current = NULL;

  if(report == NULL)
    return false;

  current = report->head;
  while(current != NULL) {
    if(current->state == state)
      return true;
    current = current->next;
  }

  return false;
}

bool status_report_has_errors(const status_report_t *report) {
  return status_report_has_state(report, STATUS_STATE_ERROR);
}

bool status_report_is_healthy(const status_report_t *report) {
  return !status_report_has_errors(report);
}

const char* status_state_to_string(status_state_t state) {
  switch(state) {
    case STATUS_STATE_PENDING:
      return "pending";
    case STATUS_STATE_IN_PROGRESS:
      return "in_progress";
    case STATUS_STATE_COMPLETE:
      return "complete";
    case STATUS_STATE_ERROR:
      return "error";
    case STATUS_STATE_SKIPPED:
      return "skipped";
    default:
      return "unknown";
  }
}

void status_report_summary(const status_report_t *report,
                           int *pending,
                           int *in_progress,
                           int *complete,
                           int *error,
                           int *skipped) {
  status_component_t *current = NULL;

  if(pending != NULL)
    *pending = 0;
  if(in_progress != NULL)
    *in_progress = 0;
  if(complete != NULL)
    *complete = 0;
  if(error != NULL)
    *error = 0;
  if(skipped != NULL)
    *skipped = 0;

  if(report == NULL)
    return;

  current = report->head;
  while(current != NULL) {
    if(current->state == STATUS_STATE_PENDING && pending != NULL)
      (*pending)++;
    else if(current->state == STATUS_STATE_IN_PROGRESS && in_progress != NULL)
      (*in_progress)++;
    else if(current->state == STATUS_STATE_COMPLETE && complete != NULL)
      (*complete)++;
    else if(current->state == STATUS_STATE_ERROR && error != NULL)
      (*error)++;
    else if(current->state == STATUS_STATE_SKIPPED && skipped != NULL)
      (*skipped)++;

    current = current->next;
  }
}

void status_report_print_text(const status_report_t *report, FILE *stream) {
  status_component_t *current = NULL;
  int pending = 0;
  int in_progress = 0;
  int complete = 0;
  int error = 0;
  int skipped = 0;

  if(stream == NULL)
    return;

  if(report == NULL) {
    fprintf(stream, "STATUS REPORT: unavailable\n");
    return;
  }

  fprintf(stream, "STATUS REPORT\n");
  fprintf(stream, "component\tstate\tdetail\n");

  current = report->head;
  while(current != NULL) {
    fprintf(stream,
            "%s\t%s\t%s\n",
            current->name,
            status_state_to_string(current->state),
            current->detail == NULL ? "" : current->detail);
    current = current->next;
  }

  status_report_summary(report, &pending, &in_progress, &complete, &error, &skipped);
  fprintf(stream,
          "summary\tpending=%i in_progress=%i complete=%i error=%i skipped=%i\n",
          pending,
          in_progress,
          complete,
          error,
          skipped);
}

void status_report_print_json(const status_report_t *report, FILE *stream) {
  status_component_t *current = NULL;
  int pending = 0;
  int in_progress = 0;
  int complete = 0;
  int error = 0;
  int skipped = 0;
  bool healthy = false;
  bool first = true;

  if(stream == NULL)
    return;

  if(report == NULL) {
    fprintf(stream, "{\"schema_version\":1,\"healthy\":false,\"components\":[],\"summary\":{\"pending\":0,\"in_progress\":0,\"complete\":0,\"error\":0,\"skipped\":0}}\n");
    return;
  }

  status_report_summary(report, &pending, &in_progress, &complete, &error, &skipped);
  healthy = status_report_is_healthy(report);

  fprintf(stream, "{\"schema_version\":1,\"healthy\":%s,\"components\":[", healthy ? "true" : "false");
  current = report->head;
  while(current != NULL) {
    if(!first)
      fprintf(stream, ",");
    first = false;

    fprintf(stream, "{\"name\":\"");
    status_print_json_escaped(stream, current->name == NULL ? "" : current->name);
    fprintf(stream, "\",\"state\":\"");
    status_print_json_escaped(stream, status_state_to_string(current->state));
    fprintf(stream, "\",\"detail\":\"");
    status_print_json_escaped(stream, current->detail == NULL ? "" : current->detail);
    fprintf(stream, "\"}");

    current = current->next;
  }

  fprintf(stream,
          "],\"summary\":{\"pending\":%i,\"in_progress\":%i,\"complete\":%i,\"error\":%i,\"skipped\":%i}}\n",
          pending,
          in_progress,
          complete,
          error,
          skipped);
}
