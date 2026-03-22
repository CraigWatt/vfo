/*
 * Copyright (c) 2026 Craig Watt
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

#ifndef MEZZANINE_CLEAN_H
#define MEZZANINE_CLEAN_H

#include <stdbool.h>

struct mezzanine_clean_options {
  bool apply_changes;
  bool append_media_tags;
  bool strict_quality_gate;
  const char *movies_folder_name;
  const char *tv_folder_name;
};
typedef struct mezzanine_clean_options mezzanine_clean_options_t;

struct mezzanine_clean_report {
  int scanned_files;
  int planned_moves;
  int applied_moves;
  int warnings;
  int errors;
  int skipped;
};
typedef struct mezzanine_clean_report mezzanine_clean_report_t;

void mc_options_init(mezzanine_clean_options_t *options);
void mc_report_init(mezzanine_clean_report_t *report);
void mc_report_add(mezzanine_clean_report_t *target, const mezzanine_clean_report_t *source);
bool mc_run_for_root(const char *mezzanine_root,
                     const mezzanine_clean_options_t *options,
                     mezzanine_clean_report_t *report);

#endif // MEZZANINE_CLEAN_H
