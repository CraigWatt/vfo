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

#include "../quality_scoring.h"

#include <dirent.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../utils.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

typedef struct quality_profile_context {
  const quality_scoring_options_t *options;
  quality_scoring_report_t *report;
  quality_profile_result_t *profile_result;
  const char *profile_name;
  const char *reference_root;
  bool include_vmaf_effective;
  bool stop_for_limit;
} quality_profile_context_t;

static bool quality_is_media_extension(const char *file_name) {
  const char *dot = strrchr(file_name, '.');
  if(dot == NULL)
    return false;

  if(strcasecmp(dot, ".mp4") == 0
     || strcasecmp(dot, ".mkv") == 0
     || strcasecmp(dot, ".m2ts") == 0
     || strcasecmp(dot, ".mov") == 0
     || strcasecmp(dot, ".m4v") == 0
     || strcasecmp(dot, ".mxf") == 0
     || strcasecmp(dot, ".y4m") == 0)
    return true;

  return false;
}

static void quality_extract_stem(const char *file_name, char *stem_out, size_t stem_out_size) {
  const char *dot = strrchr(file_name, '.');
  size_t copy_length = 0;

  if(stem_out == NULL || stem_out_size == 0)
    return;

  if(file_name == NULL) {
    stem_out[0] = '\0';
    return;
  }

  if(dot == NULL)
    copy_length = strlen(file_name);
  else
    copy_length = (size_t)(dot - file_name);

  if(copy_length >= stem_out_size)
    copy_length = stem_out_size - 1;

  if(copy_length > 0)
    memcpy(stem_out, file_name, copy_length);
  stem_out[copy_length] = '\0';
}

static void quality_strip_profile_suffix(const char *stem,
                                         const char *profile_name,
                                         char *out,
                                         size_t out_size) {
  size_t stem_length = 0;
  size_t profile_length = 0;

  if(out == NULL || out_size == 0)
    return;

  if(stem == NULL) {
    out[0] = '\0';
    return;
  }

  snprintf(out, out_size, "%s", stem);
  if(profile_name == NULL)
    return;

  stem_length = strlen(out);
  profile_length = strlen(profile_name);

  if(stem_length > profile_length + 1
     && out[stem_length - profile_length - 1] == '_'
     && strcmp(out + stem_length - profile_length, profile_name) == 0) {
    out[stem_length - profile_length - 1] = '\0';
  }
}

static char* quality_format_string(const char *fmt, ...) {
  va_list args;
  va_list args_copy;
  int required = 0;
  char *result = NULL;

  va_start(args, fmt);
  va_copy(args_copy, args);
  required = vsnprintf(NULL, 0, fmt, args_copy);
  va_end(args_copy);

  if(required < 0) {
    va_end(args);
    return NULL;
  }

  result = malloc((size_t)required + 1);
  if(result == NULL) {
    va_end(args);
    return NULL;
  }

  vsnprintf(result, (size_t)required + 1, fmt, args);
  va_end(args);
  return result;
}

static char* quality_shell_quote(const char *value) {
  size_t length = 0;
  size_t i = 0;
  size_t out_length = 2;
  char *quoted = NULL;
  size_t out_index = 0;

  if(value == NULL)
    return strdup("''");

  length = strlen(value);
  for(i = 0; i < length; i++) {
    if(value[i] == '\'')
      out_length += 4;
    else
      out_length++;
  }

  quoted = malloc(out_length + 1);
  if(quoted == NULL)
    return NULL;

  quoted[out_index++] = '\'';
  for(i = 0; i < length; i++) {
    if(value[i] == '\'') {
      quoted[out_index++] = '\'';
      quoted[out_index++] = '\\';
      quoted[out_index++] = '\'';
      quoted[out_index++] = '\'';
    } else {
      quoted[out_index++] = value[i];
    }
  }
  quoted[out_index++] = '\'';
  quoted[out_index] = '\0';

  return quoted;
}

static char* quality_read_text_file(const char *path) {
  FILE *file = NULL;
  char *buffer = NULL;
  long length = 0;
  size_t read_bytes = 0;

  if(path == NULL)
    return NULL;

  file = fopen(path, "rb");
  if(file == NULL)
    return NULL;

  if(fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    return NULL;
  }

  length = ftell(file);
  if(length < 0) {
    fclose(file);
    return NULL;
  }

  rewind(file);
  buffer = malloc((size_t)length + 1);
  if(buffer == NULL) {
    fclose(file);
    return NULL;
  }

  read_bytes = fread(buffer, 1, (size_t)length, file);
  buffer[read_bytes] = '\0';
  fclose(file);
  return buffer;
}

static bool quality_run_command_capture(const char *command, char **output_out, int *exit_code_out) {
  char tmp_template[] = "/tmp/vfo_quality_XXXXXX";
  int fd = -1;
  char *tmp_path_quoted = NULL;
  char *command_with_redirect = NULL;
  int system_result = 0;
  int exit_code = 1;
  char *output = NULL;

  if(output_out != NULL)
    *output_out = NULL;
  if(exit_code_out != NULL)
    *exit_code_out = 1;

  if(command == NULL)
    return false;

  fd = mkstemp(tmp_template);
  if(fd == -1)
    return false;
  close(fd);

  tmp_path_quoted = quality_shell_quote(tmp_template);
  command_with_redirect = quality_format_string("%s > %s 2>&1", command, tmp_path_quoted);
  if(tmp_path_quoted == NULL || command_with_redirect == NULL) {
    remove(tmp_template);
    free(tmp_path_quoted);
    free(command_with_redirect);
    return false;
  }
  system_result = system(command_with_redirect);

  if(system_result != -1 && WIFEXITED(system_result))
    exit_code = WEXITSTATUS(system_result);
  else
    exit_code = 1;

  output = quality_read_text_file(tmp_template);
  remove(tmp_template);

  free(tmp_path_quoted);
  free(command_with_redirect);

  if(output_out != NULL)
    *output_out = output;
  else
    free(output);

  if(exit_code_out != NULL)
    *exit_code_out = exit_code;

  return exit_code == 0;
}

static const char* quality_find_last_substring(const char *haystack, const char *needle) {
  const char *cursor = NULL;
  const char *last_match = NULL;
  size_t needle_length = 0;

  if(haystack == NULL || needle == NULL || needle[0] == '\0')
    return NULL;

  needle_length = strlen(needle);
  cursor = haystack;
  while((cursor = strstr(cursor, needle)) != NULL) {
    last_match = cursor;
    cursor += needle_length;
  }
  return last_match;
}

static bool quality_parse_last_double_after_marker(const char *output,
                                                   const char *marker,
                                                   double *value_out) {
  const char *match = NULL;
  char *end_ptr = NULL;
  double parsed = 0.0;

  if(output == NULL || marker == NULL || value_out == NULL)
    return false;

  match = quality_find_last_substring(output, marker);
  if(match == NULL)
    return false;

  match += strlen(marker);
  while(*match == ' ' || *match == '\t')
    match++;

  parsed = strtod(match, &end_ptr);
  if(end_ptr == match)
    return false;

  *value_out = parsed;
  return true;
}

static bool quality_compute_metric_command(const char *command, const char *marker, double *score_out) {
  char *output = NULL;
  int exit_code = 1;
  bool command_ok = quality_run_command_capture(command, &output, &exit_code);
  bool parsed = false;

  if(command_ok == false) {
    free(output);
    return false;
  }

  parsed = quality_parse_last_double_after_marker(output, marker, score_out);
  free(output);
  return parsed;
}

static bool quality_compute_psnr(const char *distorted_file, const char *reference_file, double *psnr_out) {
  char *distorted_q = quality_shell_quote(distorted_file);
  char *reference_q = quality_shell_quote(reference_file);
  char *command = quality_format_string(
    "ffmpeg -hide_banner -nostdin -i %s -i %s -lavfi \"[1:v][0:v]scale2ref=flags=bicubic[ref_s][dist_s];[dist_s][ref_s]psnr\" -f null -",
    distorted_q,
    reference_q);
  bool ok = false;

  if(distorted_q != NULL && reference_q != NULL && command != NULL)
    ok = quality_compute_metric_command(command, "average:", psnr_out);

  free(distorted_q);
  free(reference_q);
  free(command);
  return ok;
}

static bool quality_compute_ssim(const char *distorted_file, const char *reference_file, double *ssim_out) {
  char *distorted_q = quality_shell_quote(distorted_file);
  char *reference_q = quality_shell_quote(reference_file);
  char *command = quality_format_string(
    "ffmpeg -hide_banner -nostdin -i %s -i %s -lavfi \"[1:v][0:v]scale2ref=flags=bicubic[ref_s][dist_s];[dist_s][ref_s]ssim\" -f null -",
    distorted_q,
    reference_q);
  bool ok = false;

  if(distorted_q != NULL && reference_q != NULL && command != NULL)
    ok = quality_compute_metric_command(command, "All:", ssim_out);

  free(distorted_q);
  free(reference_q);
  free(command);
  return ok;
}

static bool quality_compute_vmaf(const char *distorted_file, const char *reference_file, double *vmaf_out) {
  char *distorted_q = quality_shell_quote(distorted_file);
  char *reference_q = quality_shell_quote(reference_file);
  char *command = quality_format_string(
    "ffmpeg -hide_banner -nostdin -i %s -i %s -lavfi \"[1:v][0:v]scale2ref=flags=bicubic[ref_s][dist_s];[dist_s][ref_s]libvmaf=n_threads=0\" -f null -",
    distorted_q,
    reference_q);
  bool ok = false;

  if(distorted_q != NULL && reference_q != NULL && command != NULL)
    ok = quality_compute_metric_command(command, "VMAF score:", vmaf_out);

  free(distorted_q);
  free(reference_q);
  free(command);
  return ok;
}

static bool quality_file_is_regular(const char *path) {
  struct stat stat_info;

  if(path == NULL)
    return false;

  if(stat(path, &stat_info) != 0)
    return false;

  return S_ISREG(stat_info.st_mode) != 0;
}

static bool quality_find_reference_file(const char *reference_root,
                                        const char *relative_dir,
                                        const char *reference_stem,
                                        char *reference_path_out,
                                        size_t reference_path_out_size,
                                        bool *ambiguous_out) {
  char *reference_dir = NULL;
  DIR *directory = NULL;
  struct dirent *entry = NULL;
  bool found = false;
  int match_count = 0;

  if(ambiguous_out != NULL)
    *ambiguous_out = false;

  if(reference_root == NULL || reference_stem == NULL || reference_path_out == NULL || reference_path_out_size == 0)
    return false;

  if(relative_dir != NULL && relative_dir[0] != '\0')
    reference_dir = utils_combine_to_full_path(reference_root, relative_dir);
  else
    reference_dir = strdup(reference_root);

  if(reference_dir == NULL)
    return false;

  directory = opendir(reference_dir);
  if(directory == NULL) {
    free(reference_dir);
    return false;
  }

  while((entry = readdir(directory)) != NULL) {
    char candidate_stem[PATH_MAX];
    char *candidate_path = NULL;
    bool candidate_regular = false;

    if(utils_directory_is_current_or_parent(entry->d_name))
      continue;
    if(utils_file_is_macos_hidden_files(entry->d_name))
      continue;
    if(quality_is_media_extension(entry->d_name) == false)
      continue;

    candidate_path = utils_combine_to_full_path(reference_dir, entry->d_name);
    candidate_regular = quality_file_is_regular(candidate_path);
    if(candidate_regular == false) {
      free(candidate_path);
      continue;
    }

    quality_extract_stem(entry->d_name, candidate_stem, sizeof(candidate_stem));
    if(strcmp(candidate_stem, reference_stem) == 0) {
      match_count++;
      if(found == false) {
        snprintf(reference_path_out, reference_path_out_size, "%s", candidate_path);
        found = true;
      }
    }

    free(candidate_path);
  }

  closedir(directory);
  free(reference_dir);

  if(ambiguous_out != NULL && match_count > 1)
    *ambiguous_out = true;

  return found;
}

static char* quality_join_relative_path(const char *relative_dir, const char *child_name) {
  if(relative_dir == NULL || relative_dir[0] == '\0')
    return strdup(child_name);
  return quality_format_string("%s/%s", relative_dir, child_name);
}

static quality_profile_result_t* quality_profile_result_create(const char *profile_name) {
  quality_profile_result_t *result = malloc(sizeof(quality_profile_result_t));
  if(result == NULL)
    return NULL;

  result->profile_name = strdup(profile_name == NULL ? "unknown_profile" : profile_name);
  result->files_considered = 0;
  result->files_scored = 0;
  result->files_failed_thresholds = 0;
  result->missing_reference = 0;
  result->metric_errors = 0;
  result->files_skipped_limit = 0;
  result->psnr_sum = 0.0;
  result->ssim_sum = 0.0;
  result->vmaf_sum = 0.0;
  result->vmaf_scored_files = 0;
  result->avg_psnr = 0.0;
  result->avg_ssim = 0.0;
  result->avg_vmaf = 0.0;
  result->next = NULL;

  return result;
}

static void quality_profile_result_append(quality_scoring_report_t *report, quality_profile_result_t *result) {
  quality_profile_result_t *tail = NULL;

  if(report == NULL || result == NULL)
    return;

  if(report->profiles == NULL) {
    report->profiles = result;
    return;
  }

  tail = report->profiles;
  while(tail->next != NULL)
    tail = tail->next;
  tail->next = result;
}

static void quality_profile_result_finalize(quality_profile_result_t *result) {
  if(result == NULL)
    return;

  if(result->files_scored > 0) {
    result->avg_psnr = result->psnr_sum / (double)result->files_scored;
    result->avg_ssim = result->ssim_sum / (double)result->files_scored;
  } else {
    result->avg_psnr = 0.0;
    result->avg_ssim = 0.0;
  }

  if(result->vmaf_scored_files > 0)
    result->avg_vmaf = result->vmaf_sum / (double)result->vmaf_scored_files;
  else
    result->avg_vmaf = 0.0;
}

static bool quality_scores_fail_thresholds(const quality_scoring_options_t *options,
                                           double psnr,
                                           double ssim,
                                           bool has_vmaf,
                                           double vmaf) {
  if(options == NULL)
    return false;

  if(options->min_psnr > 0.0 && psnr < options->min_psnr)
    return true;
  if(options->min_ssim > 0.0 && ssim < options->min_ssim)
    return true;
  if(options->min_vmaf > 0.0) {
    if(has_vmaf == false)
      return true;
    if(vmaf < options->min_vmaf)
      return true;
  }

  return false;
}

static void quality_process_profile_file(const char *file_path,
                                         const char *relative_dir,
                                         const char *file_name,
                                         quality_profile_context_t *context) {
  char output_stem[PATH_MAX];
  char reference_stem[PATH_MAX];
  char reference_path[PATH_MAX];
  bool ambiguous_reference = false;
  double psnr = 0.0;
  double ssim = 0.0;
  double vmaf = 0.0;
  bool has_vmaf = false;
  bool below_thresholds = false;
  bool psnr_ok = false;
  bool ssim_ok = false;

  if(context == NULL || file_path == NULL || file_name == NULL)
    return;

  if(context->options->max_files_per_profile > 0
     && context->profile_result->files_considered >= context->options->max_files_per_profile) {
    context->stop_for_limit = true;
    context->profile_result->files_skipped_limit++;
    context->report->files_skipped_limit++;
    return;
  }

  context->profile_result->files_considered++;
  context->report->files_considered++;

  quality_extract_stem(file_name, output_stem, sizeof(output_stem));
  quality_strip_profile_suffix(output_stem, context->profile_name, reference_stem, sizeof(reference_stem));

  if(quality_find_reference_file(context->reference_root,
                                 relative_dir,
                                 reference_stem,
                                 reference_path,
                                 sizeof(reference_path),
                                 &ambiguous_reference) == false) {
    printf("QUALITY WARNING: profile=%s missing reference for %s (expected stem=%s)\n",
           context->profile_name,
           file_path,
           reference_stem);
    context->profile_result->missing_reference++;
    context->report->missing_reference++;
    context->report->warnings++;
    return;
  }

  if(ambiguous_reference) {
    printf("QUALITY WARNING: profile=%s multiple reference candidates for stem=%s (using first match)\n",
           context->profile_name,
           reference_stem);
    context->report->warnings++;
  }

  psnr_ok = quality_compute_psnr(file_path, reference_path, &psnr);
  ssim_ok = quality_compute_ssim(file_path, reference_path, &ssim);
  if(psnr_ok == false || ssim_ok == false) {
    printf("QUALITY ERROR: profile=%s metric execution failed for %s\n",
           context->profile_name,
           file_path);
    context->profile_result->metric_errors++;
    context->report->metric_errors++;
    context->report->errors++;
    return;
  }

  if(context->include_vmaf_effective) {
    has_vmaf = quality_compute_vmaf(file_path, reference_path, &vmaf);
    if(has_vmaf == false) {
      printf("QUALITY WARNING: profile=%s VMAF scoring failed for %s\n",
             context->profile_name,
             file_path);
      context->profile_result->metric_errors++;
      context->report->metric_errors++;
      context->report->warnings++;
    }
  }

  below_thresholds = quality_scores_fail_thresholds(context->options, psnr, ssim, has_vmaf, vmaf);
  if(below_thresholds) {
    if(has_vmaf) {
      printf("QUALITY WARNING: profile=%s below thresholds for %s (PSNR=%.3f SSIM=%.6f VMAF=%.3f)\n",
             context->profile_name,
             file_path,
             psnr,
             ssim,
             vmaf);
    } else {
      printf("QUALITY WARNING: profile=%s below thresholds for %s (PSNR=%.3f SSIM=%.6f)\n",
             context->profile_name,
             file_path,
             psnr,
             ssim);
    }
    context->profile_result->files_failed_thresholds++;
    context->report->files_failed_thresholds++;
    context->report->warnings++;
  }

  context->profile_result->files_scored++;
  context->profile_result->psnr_sum += psnr;
  context->profile_result->ssim_sum += ssim;
  context->report->files_scored++;
  context->report->psnr_sum += psnr;
  context->report->ssim_sum += ssim;

  if(has_vmaf) {
    context->profile_result->vmaf_scored_files++;
    context->profile_result->vmaf_sum += vmaf;
    context->report->vmaf_scored_files++;
    context->report->vmaf_sum += vmaf;
  }

  if(has_vmaf) {
    printf("QUALITY INFO: profile=%s scored file=%s ref=%s PSNR=%.3f SSIM=%.6f VMAF=%.3f\n",
           context->profile_name,
           file_path,
           reference_path,
           psnr,
           ssim,
           vmaf);
  } else {
    printf("QUALITY INFO: profile=%s scored file=%s ref=%s PSNR=%.3f SSIM=%.6f\n",
           context->profile_name,
           file_path,
           reference_path,
           psnr,
           ssim);
  }
}

static void quality_walk_profile_directory(const char *current_dir,
                                           const char *relative_dir,
                                           quality_profile_context_t *context) {
  DIR *directory = NULL;
  struct dirent *entry = NULL;

  if(current_dir == NULL || context == NULL)
    return;
  if(context->stop_for_limit)
    return;

  directory = opendir(current_dir);
  if(directory == NULL)
    return;

  while((entry = readdir(directory)) != NULL) {
    char *child_path = NULL;
    char *next_relative = NULL;
    struct stat stat_info;

    if(context->stop_for_limit)
      break;

    if(utils_directory_is_current_or_parent(entry->d_name))
      continue;
    if(utils_file_is_macos_hidden_files(entry->d_name))
      continue;

    child_path = utils_combine_to_full_path(current_dir, entry->d_name);
    if(stat(child_path, &stat_info) != 0) {
      free(child_path);
      continue;
    }

    if(S_ISDIR(stat_info.st_mode)) {
      next_relative = quality_join_relative_path(relative_dir, entry->d_name);
      quality_walk_profile_directory(child_path, next_relative, context);
      free(next_relative);
    } else if(S_ISREG(stat_info.st_mode) && quality_is_media_extension(entry->d_name)) {
      quality_process_profile_file(child_path, relative_dir, entry->d_name, context);
    }

    free(child_path);
  }

  closedir(directory);
}

void quality_scoring_options_init(quality_scoring_options_t *options) {
  if(options == NULL)
    return;

  options->enabled = false;
  options->include_vmaf = false;
  options->strict_gate = false;
  options->reference_mode = QUALITY_REFERENCE_AUTO;
  options->min_psnr = 0.0;
  options->min_ssim = 0.0;
  options->min_vmaf = 0.0;
  options->max_files_per_profile = 0;
}

void quality_scoring_report_init(quality_scoring_report_t *report) {
  if(report == NULL)
    return;

  report->profiles_scored = 0;
  report->files_considered = 0;
  report->files_scored = 0;
  report->files_failed_thresholds = 0;
  report->missing_reference = 0;
  report->metric_errors = 0;
  report->warnings = 0;
  report->errors = 0;
  report->files_skipped_limit = 0;
  report->vmaf_scored_files = 0;
  report->psnr_sum = 0.0;
  report->ssim_sum = 0.0;
  report->vmaf_sum = 0.0;
  report->avg_psnr = 0.0;
  report->avg_ssim = 0.0;
  report->avg_vmaf = 0.0;
  report->include_vmaf_effective = false;
  report->profiles = NULL;
}

void quality_scoring_report_free(quality_scoring_report_t *report) {
  quality_profile_result_t *current = NULL;

  if(report == NULL)
    return;

  current = report->profiles;
  while(current != NULL) {
    quality_profile_result_t *next = current->next;
    free(current->profile_name);
    free(current);
    current = next;
  }
  report->profiles = NULL;
}

void quality_scoring_finalize_report(quality_scoring_report_t *report) {
  quality_profile_result_t *profile = NULL;

  if(report == NULL)
    return;

  if(report->files_scored > 0) {
    report->avg_psnr = report->psnr_sum / (double)report->files_scored;
    report->avg_ssim = report->ssim_sum / (double)report->files_scored;
  } else {
    report->avg_psnr = 0.0;
    report->avg_ssim = 0.0;
  }

  if(report->vmaf_scored_files > 0)
    report->avg_vmaf = report->vmaf_sum / (double)report->vmaf_scored_files;
  else
    report->avg_vmaf = 0.0;

  profile = report->profiles;
  while(profile != NULL) {
    quality_profile_result_finalize(profile);
    profile = profile->next;
  }
}

quality_reference_mode_t quality_reference_mode_from_string(const char *value, bool *valid_out) {
  if(valid_out != NULL)
    *valid_out = true;

  if(value == NULL || value[0] == '\0')
    return QUALITY_REFERENCE_AUTO;

  if(strcasecmp(value, "auto") == 0)
    return QUALITY_REFERENCE_AUTO;
  if(strcasecmp(value, "source") == 0)
    return QUALITY_REFERENCE_SOURCE;
  if(strcasecmp(value, "mezzanine") == 0)
    return QUALITY_REFERENCE_MEZZANINE;

  if(valid_out != NULL)
    *valid_out = false;
  return QUALITY_REFERENCE_AUTO;
}

const char* quality_reference_mode_to_string(quality_reference_mode_t mode) {
  if(mode == QUALITY_REFERENCE_SOURCE)
    return "source";
  if(mode == QUALITY_REFERENCE_MEZZANINE)
    return "mezzanine";
  return "auto";
}

bool quality_scoring_is_libvmaf_available(void) {
  char *output = NULL;
  bool available = false;

  if(quality_run_command_capture("ffmpeg -hide_banner -filters", &output, NULL) == false) {
    free(output);
    return false;
  }

  if(output != NULL && strstr(output, "libvmaf") != NULL)
    available = true;

  free(output);
  return available;
}

static const char* quality_select_reference_root(const char *source_root,
                                                 const char *mezzanine_root,
                                                 bool keep_source,
                                                 quality_reference_mode_t reference_mode,
                                                 char *buffer_out,
                                                 size_t buffer_out_size) {
  if(buffer_out == NULL || buffer_out_size == 0)
    return NULL;

  buffer_out[0] = '\0';

  if(reference_mode == QUALITY_REFERENCE_SOURCE || (reference_mode == QUALITY_REFERENCE_AUTO && keep_source)) {
    if(source_root == NULL || source_root[0] == '\0')
      return NULL;
    snprintf(buffer_out, buffer_out_size, "%s/content", source_root);
    return buffer_out;
  }

  if(mezzanine_root == NULL || mezzanine_root[0] == '\0')
    return NULL;
  snprintf(buffer_out, buffer_out_size, "%s/start", mezzanine_root);
  return buffer_out;
}

static void quality_score_profile_locations(ca_node_t *profile,
                                            quality_profile_context_t *context) {
  char **locations = NULL;
  int location_count = 0;

  if(profile == NULL || context == NULL)
    return;

  if(utils_string_is_empty_or_spaces(profile->alias_locations) == false)
    locations = utils_split_semicolon_list(profile->alias_locations, &location_count);

  if(location_count == 0 && utils_string_is_empty_or_spaces(profile->alias_location) == false) {
    locations = malloc(sizeof(char*));
    locations[0] = strdup(profile->alias_location);
    location_count = 1;
  }

  if(location_count == 0) {
    printf("QUALITY WARNING: profile=%s has no configured locations to score\n", context->profile_name);
    context->report->warnings++;
    return;
  }

  for(int i = 0; i < location_count; i++) {
    char *content_root = utils_combine_to_full_path(locations[i], "content");
    if(utils_does_folder_exist(content_root) == false) {
      printf("QUALITY WARNING: profile=%s content path missing: %s\n", context->profile_name, content_root);
      context->report->warnings++;
      free(content_root);
      continue;
    }

    printf("QUALITY ALERT: profile=%s scoring location %i/%i => %s\n",
           context->profile_name,
           i + 1,
           location_count,
           content_root);

    quality_walk_profile_directory(content_root, "", context);
    free(content_root);

    if(context->stop_for_limit)
      break;
  }

  utils_free_string_array(locations, location_count);
}

bool quality_scoring_run(ca_node_t *profiles_head,
                         const char *source_root,
                         const char *mezzanine_root,
                         bool keep_source,
                         const quality_scoring_options_t *options,
                         quality_scoring_report_t *report) {
  ca_node_t *profile_cursor = profiles_head;
  char reference_root[PATH_MAX];
  const char *reference_root_selected = NULL;

  if(options == NULL || report == NULL)
    return false;

  if(options->enabled == false)
    return true;

  reference_root_selected = quality_select_reference_root(source_root,
                                                          mezzanine_root,
                                                          keep_source,
                                                          options->reference_mode,
                                                          reference_root,
                                                          sizeof(reference_root));
  if(reference_root_selected == NULL || utils_does_folder_exist((char *)reference_root_selected) == false) {
    printf("QUALITY ERROR: reference layer path is missing (%s)\n",
           reference_root_selected == NULL ? "unset" : reference_root_selected);
    report->errors++;
    return false;
  }

  report->include_vmaf_effective = options->include_vmaf;
  if(options->include_vmaf && quality_scoring_is_libvmaf_available() == false) {
    printf("QUALITY WARNING: QUALITY_CHECK_INCLUDE_VMAF=true but ffmpeg libvmaf filter is unavailable. Continuing with PSNR+SSIM only.\n");
    report->include_vmaf_effective = false;
    report->warnings++;
  }

  printf("QUALITY ALERT: reference_layer=%s root=%s include_vmaf=%s strict_gate=%s max_files_per_profile=%i\n",
         quality_reference_mode_to_string(options->reference_mode),
         reference_root_selected,
         report->include_vmaf_effective ? "true" : "false",
         options->strict_gate ? "true" : "false",
         options->max_files_per_profile);

  while(profile_cursor != NULL) {
    quality_profile_result_t *profile_result = quality_profile_result_create(profile_cursor->alias_name);
    quality_profile_context_t context;

    if(profile_result == NULL) {
      report->errors++;
      break;
    }

    context.options = options;
    context.report = report;
    context.profile_result = profile_result;
    context.profile_name = profile_cursor->alias_name;
    context.reference_root = reference_root_selected;
    context.include_vmaf_effective = report->include_vmaf_effective;
    context.stop_for_limit = false;

    quality_score_profile_locations(profile_cursor, &context);
    quality_profile_result_finalize(profile_result);

    printf("QUALITY SUMMARY: profile=%s considered=%i scored=%i failed=%i missing_ref=%i metric_errors=%i avg_psnr=%.3f avg_ssim=%.6f avg_vmaf=%.3f\n",
           profile_result->profile_name,
           profile_result->files_considered,
           profile_result->files_scored,
           profile_result->files_failed_thresholds,
           profile_result->missing_reference,
           profile_result->metric_errors,
           profile_result->avg_psnr,
           profile_result->avg_ssim,
           profile_result->avg_vmaf);

    quality_profile_result_append(report, profile_result);
    report->profiles_scored++;
    profile_cursor = profile_cursor->next;
  }

  quality_scoring_finalize_report(report);

  if(options->strict_gate) {
    if(report->files_failed_thresholds > 0
       || report->missing_reference > 0
       || report->metric_errors > 0
       || report->errors > 0)
      return false;
  }

  if(report->errors > 0)
    return false;

  return true;
}
