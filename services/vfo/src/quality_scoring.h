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

#ifndef QUALITY_SCORING_H
#define QUALITY_SCORING_H

#include <stdbool.h>

#include "Config/Data_Structures/c_ca_ll_struct.h"

typedef enum quality_reference_mode {
  QUALITY_REFERENCE_AUTO = 0,
  QUALITY_REFERENCE_SOURCE,
  QUALITY_REFERENCE_MEZZANINE
} quality_reference_mode_t;

struct quality_profile_result {
  char *profile_name;
  int files_considered;
  int files_scored;
  int files_failed_thresholds;
  int missing_reference;
  int metric_errors;
  int files_skipped_limit;

  double psnr_sum;
  double ssim_sum;
  double vmaf_sum;
  int vmaf_scored_files;

  double avg_psnr;
  double avg_ssim;
  double avg_vmaf;

  struct quality_profile_result *next;
};
typedef struct quality_profile_result quality_profile_result_t;

struct quality_scoring_options {
  bool enabled;
  bool include_vmaf;
  bool strict_gate;
  quality_reference_mode_t reference_mode;
  double min_psnr;
  double min_ssim;
  double min_vmaf;
  int max_files_per_profile;
};
typedef struct quality_scoring_options quality_scoring_options_t;

struct quality_scoring_report {
  int profiles_scored;
  int files_considered;
  int files_scored;
  int files_failed_thresholds;
  int missing_reference;
  int metric_errors;
  int warnings;
  int errors;
  int files_skipped_limit;
  int vmaf_scored_files;

  double psnr_sum;
  double ssim_sum;
  double vmaf_sum;

  double avg_psnr;
  double avg_ssim;
  double avg_vmaf;

  bool include_vmaf_effective;
  quality_profile_result_t *profiles;
};
typedef struct quality_scoring_report quality_scoring_report_t;

void quality_scoring_options_init(quality_scoring_options_t *options);
void quality_scoring_report_init(quality_scoring_report_t *report);
void quality_scoring_report_free(quality_scoring_report_t *report);
void quality_scoring_finalize_report(quality_scoring_report_t *report);

quality_reference_mode_t quality_reference_mode_from_string(const char *value, bool *valid_out);
const char* quality_reference_mode_to_string(quality_reference_mode_t mode);

bool quality_scoring_is_libvmaf_available(void);

bool quality_scoring_run(ca_node_t *profiles_head,
                         const char *source_root,
                         const char *mezzanine_root,
                         bool keep_source,
                         const quality_scoring_options_t *options,
                         quality_scoring_report_t *report);

#endif // QUALITY_SCORING_H
