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

#include "mc_internal.h"

#define MC_TITLE_MAX 512
#define MC_TAG_MAX 64

struct mc_probe_result {
  int width;
  int height;
  int bit_depth;
  char video_codec[64];
  char audio_codec[64];
  char color_transfer[64];
  char pix_fmt[64];
  bool has_dv;
  bool has_hdr10_plus;
};
typedef struct mc_probe_result mc_probe_result_t;

static bool mc_path_starts_with(const char *path, const char *prefix) {
  size_t prefix_len = 0;
  if(path == NULL || prefix == NULL)
    return false;
  prefix_len = strlen(prefix);
  if(prefix_len == 0)
    return false;
  if(strncmp(path, prefix, prefix_len) != 0)
    return false;
  return path[prefix_len] == '\0' || path[prefix_len] == '/';
}

static bool mc_is_hidden_name(const char *name) {
  return name != NULL && name[0] == '.';
}

static char* mc_strdup_safe(const char *value) {
  if(value == NULL) {
    char *empty = malloc(1);
    empty[0] = '\0';
    return empty;
  }
  return strdup(value);
}

static void mc_trim_in_place(char *value) {
  size_t start = 0;
  size_t end = 0;
  if(value == NULL || value[0] == '\0')
    return;
  while(value[start] != '\0' && isspace((unsigned char)value[start]))
    start++;
  end = strlen(value);
  while(end > start && isspace((unsigned char)value[end - 1]))
    end--;
  if(start > 0)
    memmove(value, value + start, end - start);
  value[end - start] = '\0';
}

static const char* mc_basename_const(const char *path) {
  const char *last = NULL;
  if(path == NULL)
    return "";
  last = strrchr(path, '/');
  if(last == NULL)
    return path;
  return last + 1;
}

static bool mc_parent_path(const char *path, char *out, size_t out_size) {
  const char *last = NULL;
  size_t length = 0;
  if(path == NULL || out == NULL || out_size == 0)
    return false;
  last = strrchr(path, '/');
  if(last == NULL)
    return false;
  length = (size_t)(last - path);
  if(length == 0)
    length = 1;
  if(length >= out_size)
    length = out_size - 1;
  memcpy(out, path, length);
  out[length] = '\0';
  return true;
}

static bool mc_is_video_file_name(const char *name) {
  const char *extension = strrchr(name, '.');
  char lower_ext[16];
  size_t i = 0;

  if(extension == NULL || extension[1] == '\0')
    return false;

  extension++;
  while(extension[i] != '\0' && i < sizeof(lower_ext) - 1) {
    lower_ext[i] = (char)tolower((unsigned char)extension[i]);
    i++;
  }
  lower_ext[i] = '\0';

  return strcmp(lower_ext, "mkv") == 0
      || strcmp(lower_ext, "mp4") == 0
      || strcmp(lower_ext, "m2ts") == 0
      || strcmp(lower_ext, "mov") == 0
      || strcmp(lower_ext, "m4v") == 0
      || strcmp(lower_ext, "mxf") == 0;
}

static void mc_append_path(char ***items, int *count, const char *path) {
  *items = realloc(*items, sizeof(char*) * (size_t)(*count + 1));
  (*items)[*count] = strdup(path);
  (*count)++;
}

static bool mc_entry_is_directory(const char *path, const struct dirent *entry) {
  struct stat stats;
  if(entry->d_type == DT_DIR)
    return true;
  if(entry->d_type != DT_UNKNOWN)
    return false;
  if(stat(path, &stats) != 0)
    return false;
  return S_ISDIR(stats.st_mode) != 0;
}

static bool mc_entry_is_regular_file(const char *path, const struct dirent *entry) {
  struct stat stats;
  if(entry->d_type == DT_REG)
    return true;
  if(entry->d_type != DT_UNKNOWN)
    return false;
  if(stat(path, &stats) != 0)
    return false;
  return S_ISREG(stats.st_mode) != 0;
}

static void mc_collect_video_files_recursive(const char *directory, char ***files, int *count) {
  DIR *dir = opendir(directory);
  struct dirent *entry = NULL;

  if(dir == NULL)
    return;

  while((entry = readdir(dir)) != NULL) {
    char *entry_path = NULL;

    if(utils_directory_is_current_or_parent(entry->d_name) || mc_is_hidden_name(entry->d_name))
      continue;

    entry_path = utils_combine_to_full_path(directory, entry->d_name);
    if(mc_entry_is_directory(entry_path, entry))
      mc_collect_video_files_recursive(entry_path, files, count);
    else if(mc_entry_is_regular_file(entry_path, entry) && mc_is_video_file_name(entry->d_name))
      mc_append_path(files, count, entry_path);

    free(entry_path);
  }

  closedir(dir);
}

static void mc_free_paths(char **files, int count) {
  for(int i = 0; i < count; i++)
    free(files[i]);
  free(files);
}

static char* mc_shell_quote(const char *value) {
  size_t length = strlen(value);
  char *quoted = malloc((length * 5) + 3);
  size_t write_index = 0;

  quoted[write_index++] = '\'';
  for(size_t i = 0; i < length; i++) {
    if(value[i] == '\'') {
      memcpy(quoted + write_index, "'\"'\"'", 6);
      write_index += 6;
    } else {
      quoted[write_index++] = value[i];
    }
  }
  quoted[write_index++] = '\'';
  quoted[write_index] = '\0';
  return quoted;
}

static bool mc_contains_case_insensitive(const char *haystack, const char *needle) {
  char *haystack_copy = NULL;
  char *needle_copy = NULL;
  bool found = false;

  if(haystack == NULL || needle == NULL)
    return false;

  haystack_copy = mc_strdup_safe(haystack);
  needle_copy = mc_strdup_safe(needle);
  for(size_t i = 0; haystack_copy[i] != '\0'; i++)
    haystack_copy[i] = (char)tolower((unsigned char)haystack_copy[i]);
  for(size_t i = 0; needle_copy[i] != '\0'; i++)
    needle_copy[i] = (char)tolower((unsigned char)needle_copy[i]);

  found = strstr(haystack_copy, needle_copy) != NULL;
  free(haystack_copy);
  free(needle_copy);
  return found;
}

static bool mc_probe_video_stream(const char *file_path, mc_probe_result_t *probe) {
  char command[4096];
  char line[1024];
  char *quoted = mc_shell_quote(file_path);
  FILE *stream = NULL;

  snprintf(command,
           sizeof(command),
           "ffprobe -v error -select_streams v:0 "
           "-show_entries stream=width,height,codec_name,bits_per_raw_sample,color_transfer,pix_fmt "
           "-show_entries stream_side_data=side_data_type "
           "-of default=noprint_wrappers=1 %s 2>/dev/null",
           quoted);
  free(quoted);

  stream = popen(command, "r");
  if(stream == NULL)
    return false;

  while(fgets(line, sizeof(line), stream) != NULL) {
    mc_trim_in_place(line);
    if(strncmp(line, "width=", 6) == 0)
      probe->width = atoi(line + 6);
    else if(strncmp(line, "height=", 7) == 0)
      probe->height = atoi(line + 7);
    else if(strncmp(line, "codec_name=", 11) == 0)
      snprintf(probe->video_codec, sizeof(probe->video_codec), "%s", line + 11);
    else if(strncmp(line, "bits_per_raw_sample=", 20) == 0)
      probe->bit_depth = atoi(line + 20);
    else if(strncmp(line, "color_transfer=", 15) == 0)
      snprintf(probe->color_transfer, sizeof(probe->color_transfer), "%s", line + 15);
    else if(strncmp(line, "pix_fmt=", 8) == 0)
      snprintf(probe->pix_fmt, sizeof(probe->pix_fmt), "%s", line + 8);
    else if(strncmp(line, "side_data_type=", 15) == 0) {
      if(mc_contains_case_insensitive(line, "dovi"))
        probe->has_dv = true;
      if(mc_contains_case_insensitive(line, "2094") || mc_contains_case_insensitive(line, "hdr10+"))
        probe->has_hdr10_plus = true;
    }
  }

  pclose(stream);

  if(probe->bit_depth == 0) {
    if(mc_contains_case_insensitive(probe->pix_fmt, "12"))
      probe->bit_depth = 12;
    else if(mc_contains_case_insensitive(probe->pix_fmt, "10") || mc_contains_case_insensitive(probe->pix_fmt, "p010"))
      probe->bit_depth = 10;
    else if(probe->pix_fmt[0] != '\0')
      probe->bit_depth = 8;
  }

  return probe->video_codec[0] != '\0';
}

static void mc_probe_audio_stream(const char *file_path, mc_probe_result_t *probe) {
  char command[4096];
  char line[256];
  char *quoted = mc_shell_quote(file_path);
  FILE *stream = NULL;

  snprintf(command,
           sizeof(command),
           "ffprobe -v error -select_streams a:0 -show_entries stream=codec_name "
           "-of default=nokey=1:noprint_wrappers=1 %s 2>/dev/null",
           quoted);
  free(quoted);

  stream = popen(command, "r");
  if(stream == NULL) {
    snprintf(probe->audio_codec, sizeof(probe->audio_codec), "none");
    return;
  }

  if(fgets(line, sizeof(line), stream) != NULL) {
    mc_trim_in_place(line);
    snprintf(probe->audio_codec, sizeof(probe->audio_codec), "%s", line);
  } else {
    snprintf(probe->audio_codec, sizeof(probe->audio_codec), "none");
  }

  pclose(stream);
}

static void mc_upper_token(char *value, size_t value_size, const char *source) {
  size_t write_index = 0;
  if(source == NULL || source[0] == '\0') {
    snprintf(value, value_size, "UNKNOWN");
    return;
  }

  while(source[write_index] != '\0' && write_index < value_size - 1) {
    char current = source[write_index];
    if(current == '-')
      current = '_';
    value[write_index] = (char)toupper((unsigned char)current);
    write_index++;
  }
  value[write_index] = '\0';
}

static void mc_resolution_tag(const mc_probe_result_t *probe, char *output, size_t output_size) {
  if(probe->width >= 3800 || probe->height >= 2100)
    snprintf(output, output_size, "UHD");
  else if(probe->width >= 1900 || probe->height >= 1000)
    snprintf(output, output_size, "FHD");
  else if(probe->width >= 1200 || probe->height >= 700)
    snprintf(output, output_size, "HD");
  else
    snprintf(output, output_size, "SD");
}

static void mc_dynamic_range_tag(const mc_probe_result_t *probe, char *output, size_t output_size) {
  if(probe->has_dv)
    snprintf(output, output_size, "DV");
  else if(probe->has_hdr10_plus)
    snprintf(output, output_size, "HDR10PLUS");
  else if(mc_contains_case_insensitive(probe->color_transfer, "smpte2084"))
    snprintf(output, output_size, "HDR10");
  else if(mc_contains_case_insensitive(probe->color_transfer, "arib-std-b67"))
    snprintf(output, output_size, "HLG");
  else
    snprintf(output, output_size, "SDR");
}

static bool mc_is_mezzanine_video_codec(const char *codec) {
  if(codec == NULL || codec[0] == '\0')
    return false;
  return mc_contains_case_insensitive(codec, "hevc")
      || mc_contains_case_insensitive(codec, "h265")
      || mc_contains_case_insensitive(codec, "av1")
      || mc_contains_case_insensitive(codec, "prores")
      || mc_contains_case_insensitive(codec, "dnx");
}

static bool mc_is_lossy_audio_codec(const char *codec) {
  if(codec == NULL || codec[0] == '\0')
    return false;
  return mc_contains_case_insensitive(codec, "aac")
      || mc_contains_case_insensitive(codec, "mp3")
      || mc_contains_case_insensitive(codec, "ac3")
      || mc_contains_case_insensitive(codec, "eac3")
      || mc_contains_case_insensitive(codec, "opus");
}

static void mc_report_warning(mezzanine_clean_report_t *report, const char *file_path, const char *message) {
  report->warnings++;
  printf("MEZZANINE CLEAN WARN: %s (%s)\n", message, file_path);
}

static void mc_copy_without_extension(const char *name, char *output, size_t output_size) {
  const char *dot = strrchr(name, '.');
  size_t length = 0;

  if(dot == NULL)
    dot = name + strlen(name);

  length = (size_t)(dot - name);
  if(length >= output_size)
    length = output_size - 1;
  memcpy(output, name, length);
  output[length] = '\0';
}

static void mc_strip_suffix_tags(char *value) {
  size_t length = strlen(value);
  while(length > 0 && isspace((unsigned char)value[length - 1])) {
    value[length - 1] = '\0';
    length--;
  }
  if(length > 0 && value[length - 1] == ']') {
    char *open = strrchr(value, '[');
    if(open != NULL)
      *open = '\0';
  }
  mc_trim_in_place(value);
}

static void mc_sanitize_title(const char *input, char *output, size_t output_size) {
  size_t write_index = 0;
  bool previous_space = false;

  if(input == NULL) {
    snprintf(output, output_size, "Unknown");
    return;
  }

  for(size_t i = 0; input[i] != '\0' && write_index < output_size - 1; i++) {
    unsigned char current = (unsigned char)input[i];

    if(isalnum(current)) {
      output[write_index++] = (char)current;
      previous_space = false;
      continue;
    }

    if(current == '\'' || current == '-' || current == '&') {
      output[write_index++] = (char)current;
      previous_space = false;
      continue;
    }

    if(previous_space == false && write_index > 0) {
      output[write_index++] = ' ';
      previous_space = true;
    }
  }

  output[write_index] = '\0';
  mc_trim_in_place(output);
  if(output[0] == '\0')
    snprintf(output, output_size, "Unknown");
}

static bool mc_find_sxxeyy(const char *value, int *season, int *episode, int *match_index) {
  size_t length = strlen(value);
  for(size_t i = 0; i + 5 < length; i++) {
    if(toupper((unsigned char)value[i]) == 'S'
       && isdigit((unsigned char)value[i + 1]) != 0
       && isdigit((unsigned char)value[i + 2]) != 0
       && toupper((unsigned char)value[i + 3]) == 'E'
       && isdigit((unsigned char)value[i + 4]) != 0
       && isdigit((unsigned char)value[i + 5]) != 0) {
      *season = (value[i + 1] - '0') * 10 + (value[i + 2] - '0');
      *episode = (value[i + 4] - '0') * 10 + (value[i + 5] - '0');
      *match_index = (int)i;
      return true;
    }
  }
  return false;
}

static int mc_parse_season_folder_number(const char *folder_name) {
  int season = 0;
  if(folder_name == NULL)
    return 0;
  if(sscanf(folder_name, "Season %d", &season) == 1 && season > 0)
    return season;
  if(sscanf(folder_name, "season %d", &season) == 1 && season > 0)
    return season;
  return 0;
}

static bool mc_mkdir_p(const char *path) {
  char tmp[PATH_MAX];
  size_t length = 0;

  if(path == NULL || path[0] == '\0')
    return false;

  snprintf(tmp, sizeof(tmp), "%s", path);
  length = strlen(tmp);
  if(length == 0)
    return false;
  if(tmp[length - 1] == '/')
    tmp[length - 1] = '\0';

  for(char *cursor = tmp + 1; *cursor != '\0'; cursor++) {
    if(*cursor == '/') {
      *cursor = '\0';
      if(mkdir(tmp, 0755) != 0 && errno != EEXIST)
        return false;
      *cursor = '/';
    }
  }
  if(mkdir(tmp, 0755) != 0 && errno != EEXIST)
    return false;

  return true;
}

static void mc_cleanup_empty_parent_dirs(const char *from_file_path, const char *root) {
  char current[PATH_MAX];
  if(mc_parent_path(from_file_path, current, sizeof(current)) == false)
    return;

  while(mc_path_starts_with(current, root) && strcmp(current, root) != 0) {
    char *slash = NULL;
    if(rmdir(current) != 0)
      break;
    slash = strrchr(current, '/');
    if(slash == NULL)
      break;
    if(slash == current)
      current[1] = '\0';
    else
      *slash = '\0';
  }
}

static void mc_build_tags(const mc_probe_result_t *probe,
                          char *resolution_tag,
                          size_t resolution_tag_size,
                          char *dynamic_tag,
                          size_t dynamic_tag_size,
                          char *video_tag,
                          size_t video_tag_size,
                          char *audio_tag,
                          size_t audio_tag_size) {
  mc_resolution_tag(probe, resolution_tag, resolution_tag_size);
  mc_dynamic_range_tag(probe, dynamic_tag, dynamic_tag_size);
  mc_upper_token(video_tag, video_tag_size, probe->video_codec);
  mc_upper_token(audio_tag, audio_tag_size, probe->audio_codec);
}

static void mc_prepare_filename_with_tags(char *output,
                                          size_t output_size,
                                          const char *title,
                                          const char *extension,
                                          const char *resolution_tag,
                                          const char *dynamic_tag,
                                          const char *video_tag,
                                          const char *audio_tag,
                                          bool append_tags) {
  if(append_tags) {
    snprintf(output,
             output_size,
             "%s [%s %s %s %s].%s",
             title,
             resolution_tag,
             dynamic_tag,
             video_tag,
             audio_tag,
             extension);
  } else {
    snprintf(output, output_size, "%s.%s", title, extension);
  }
}

static bool mc_process_single_file(const char *file_path,
                                   const char *root,
                                   const mezzanine_clean_options_t *options,
                                   mezzanine_clean_report_t *report) {
  mc_probe_result_t probe;
  char file_name[PATH_MAX];
  char base_name[MC_TITLE_MAX];
  char extension[32];
  char resolution_tag[MC_TAG_MAX];
  char dynamic_tag[MC_TAG_MAX];
  char video_tag[MC_TAG_MAX];
  char audio_tag[MC_TAG_MAX];
  char parent_path[PATH_MAX];
  char parent_name[MC_TITLE_MAX];
  char grandparent_path[PATH_MAX];
  char grandparent_name[MC_TITLE_MAX];
  char canonical_title[MC_TITLE_MAX];
  char show_name[MC_TITLE_MAX];
  char season_folder[32];
  char canonical_filename[MC_TITLE_MAX];
  char target_dir[PATH_MAX];
  char target_path[PATH_MAX];
  int season = 0;
  int episode = 0;
  int marker_index = 0;
  bool is_tv = false;

  memset(&probe, 0, sizeof(probe));
  memset(file_name, 0, sizeof(file_name));
  memset(base_name, 0, sizeof(base_name));
  memset(extension, 0, sizeof(extension));
  memset(parent_path, 0, sizeof(parent_path));
  memset(parent_name, 0, sizeof(parent_name));
  memset(grandparent_path, 0, sizeof(grandparent_path));
  memset(grandparent_name, 0, sizeof(grandparent_name));
  memset(canonical_title, 0, sizeof(canonical_title));
  memset(show_name, 0, sizeof(show_name));
  memset(season_folder, 0, sizeof(season_folder));
  memset(canonical_filename, 0, sizeof(canonical_filename));
  memset(target_dir, 0, sizeof(target_dir));
  memset(target_path, 0, sizeof(target_path));

  report->scanned_files++;

  snprintf(file_name, sizeof(file_name), "%s", mc_basename_const(file_path));
  mc_copy_without_extension(file_name, base_name, sizeof(base_name));
  mc_strip_suffix_tags(base_name);

  {
    const char *dot = strrchr(file_name, '.');
    if(dot == NULL) {
      report->errors++;
      printf("MEZZANINE CLEAN ERROR: no extension detected (%s)\n", file_path);
      return false;
    }
    snprintf(extension, sizeof(extension), "%s", dot + 1);
    for(size_t i = 0; extension[i] != '\0'; i++)
      extension[i] = (char)tolower((unsigned char)extension[i]);
  }

  if(mc_probe_video_stream(file_path, &probe) == false) {
    report->errors++;
    printf("MEZZANINE CLEAN ERROR: could not probe video stream (%s)\n", file_path);
    return false;
  }
  mc_probe_audio_stream(file_path, &probe);

  if(mc_is_mezzanine_video_codec(probe.video_codec) == false)
    mc_report_warning(report, file_path, "video codec may be weak for mezzanine; recommended: HEVC/AV1/ProRes/DNx");
  if(probe.bit_depth > 0 && probe.bit_depth < 10)
    mc_report_warning(report, file_path, "bit depth below 10-bit; recommended mezzanine depth is 10-bit or higher");
  if(probe.color_transfer[0] == '\0')
    mc_report_warning(report, file_path, "missing color transfer metadata; consider remuxing with complete HDR/SDR metadata");
  if(probe.width < 1920 || probe.height < 1080)
    mc_report_warning(report, file_path, "resolution below 1080p; verify if this should remain in mezzanine tier");
  if(mc_is_lossy_audio_codec(probe.audio_codec))
    mc_report_warning(report, file_path, "first audio stream appears lossy; mezzanine usually keeps a lossless/transparent source");

  mc_build_tags(&probe,
                resolution_tag,
                sizeof(resolution_tag),
                dynamic_tag,
                sizeof(dynamic_tag),
                video_tag,
                sizeof(video_tag),
                audio_tag,
                sizeof(audio_tag));

  if(mc_parent_path(file_path, parent_path, sizeof(parent_path))) {
    snprintf(parent_name, sizeof(parent_name), "%s", mc_basename_const(parent_path));
    if(mc_parent_path(parent_path, grandparent_path, sizeof(grandparent_path)))
      snprintf(grandparent_name, sizeof(grandparent_name), "%s", mc_basename_const(grandparent_path));
  }

  is_tv = mc_find_sxxeyy(base_name, &season, &episode, &marker_index);
  if(is_tv) {
    char show_candidate[MC_TITLE_MAX];
    size_t copy_length = (size_t)marker_index;
    if(copy_length >= sizeof(show_candidate))
      copy_length = sizeof(show_candidate) - 1;
    memcpy(show_candidate, base_name, copy_length);
    show_candidate[copy_length] = '\0';
    mc_sanitize_title(show_candidate, show_name, sizeof(show_name));

    if(show_name[0] == '\0' || strcmp(show_name, "Unknown") == 0) {
      if(grandparent_name[0] != '\0')
        mc_sanitize_title(grandparent_name, show_name, sizeof(show_name));
      else
        mc_sanitize_title(parent_name, show_name, sizeof(show_name));
    }

    if(season <= 0)
      season = mc_parse_season_folder_number(parent_name);
    if(season <= 0)
      season = 1;

    snprintf(season_folder, sizeof(season_folder), "Season %02d", season);
    snprintf(canonical_title, sizeof(canonical_title), "%s - S%02dE%02d", show_name, season, episode);

    mc_prepare_filename_with_tags(canonical_filename,
                                  sizeof(canonical_filename),
                                  canonical_title,
                                  extension,
                                  resolution_tag,
                                  dynamic_tag,
                                  video_tag,
                                  audio_tag,
                                  options->append_media_tags);

    snprintf(target_dir, sizeof(target_dir), "%s/%s/%s/%s", root, options->tv_folder_name, show_name, season_folder);
  } else {
    mc_sanitize_title(base_name, canonical_title, sizeof(canonical_title));
    mc_prepare_filename_with_tags(canonical_filename,
                                  sizeof(canonical_filename),
                                  canonical_title,
                                  extension,
                                  resolution_tag,
                                  dynamic_tag,
                                  video_tag,
                                  audio_tag,
                                  options->append_media_tags);
    snprintf(target_dir, sizeof(target_dir), "%s/%s/%s", root, options->movies_folder_name, canonical_title);
  }

  snprintf(target_path, sizeof(target_path), "%s/%s", target_dir, canonical_filename);
  if(strcmp(target_path, file_path) == 0) {
    report->skipped++;
    return true;
  }

  report->planned_moves++;
  if(options->apply_changes == false) {
    printf("MEZZANINE CLEAN PLAN: %s -> %s\n", file_path, target_path);
    return true;
  }

  if(utils_does_file_exist(target_path)) {
    report->errors++;
    printf("MEZZANINE CLEAN ERROR: target already exists, skipping move: %s\n", target_path);
    return false;
  }

  if(mc_mkdir_p(target_dir) == false) {
    report->errors++;
    printf("MEZZANINE CLEAN ERROR: could not create destination directories: %s\n", target_dir);
    return false;
  }

  if(rename(file_path, target_path) != 0) {
    report->errors++;
    printf("MEZZANINE CLEAN ERROR: failed to move file (%s -> %s): errno=%i\n", file_path, target_path, errno);
    return false;
  }

  report->applied_moves++;
  printf("MEZZANINE CLEAN APPLY: %s -> %s\n", file_path, target_path);
  mc_cleanup_empty_parent_dirs(file_path, root);
  return true;
}

void mc_options_init(mezzanine_clean_options_t *options) {
  if(options == NULL)
    return;
  options->apply_changes = false;
  options->append_media_tags = true;
  options->strict_quality_gate = false;
  options->movies_folder_name = "Movies";
  options->tv_folder_name = "TV Shows";
}

void mc_report_init(mezzanine_clean_report_t *report) {
  if(report == NULL)
    return;
  report->scanned_files = 0;
  report->planned_moves = 0;
  report->applied_moves = 0;
  report->warnings = 0;
  report->errors = 0;
  report->skipped = 0;
}

void mc_report_add(mezzanine_clean_report_t *target, const mezzanine_clean_report_t *source) {
  if(target == NULL || source == NULL)
    return;
  target->scanned_files += source->scanned_files;
  target->planned_moves += source->planned_moves;
  target->applied_moves += source->applied_moves;
  target->warnings += source->warnings;
  target->errors += source->errors;
  target->skipped += source->skipped;
}

bool mc_run_for_root(const char *mezzanine_root,
                     const mezzanine_clean_options_t *options,
                     mezzanine_clean_report_t *report) {
  mezzanine_clean_options_t defaults;
  const mezzanine_clean_options_t *effective = options;
  char *start_workspace = NULL;
  const char *workspace_root = mezzanine_root;
  char **files = NULL;
  int file_count = 0;
  bool success = true;

  if(mezzanine_root == NULL || mezzanine_root[0] == '\0') {
    printf("MEZZANINE CLEAN ERROR: root is empty\n");
    return false;
  }

  if(report == NULL)
    return false;

  mc_report_init(report);

  if(utils_does_folder_exist((char *)mezzanine_root) == false) {
    printf("MEZZANINE CLEAN ERROR: mezzanine root does not exist: %s\n", mezzanine_root);
    report->errors++;
    return false;
  }

  start_workspace = utils_combine_to_full_path(mezzanine_root, "start");
  if(utils_does_folder_exist(start_workspace))
    workspace_root = start_workspace;
  else {
    free(start_workspace);
    start_workspace = NULL;
  }

  if(effective == NULL) {
    mc_options_init(&defaults);
    effective = &defaults;
  }

  printf("MEZZANINE CLEAN ALERT: root=%s workspace=%s mode=%s append_media_tags=%s strict_quality_gate=%s\n",
         mezzanine_root,
         workspace_root,
         effective->apply_changes ? "apply" : "audit",
         effective->append_media_tags ? "true" : "false",
         effective->strict_quality_gate ? "true" : "false");

  mc_collect_video_files_recursive(workspace_root, &files, &file_count);
  if(file_count == 0) {
    printf("MEZZANINE CLEAN ALERT: no video files found under %s\n", workspace_root);
    free(start_workspace);
    return true;
  }

  for(int i = 0; i < file_count; i++) {
    bool file_ok = mc_process_single_file(files[i], workspace_root, effective, report);
    if(file_ok == false)
      success = false;
  }
  mc_free_paths(files, file_count);
  free(start_workspace);

  if(effective->strict_quality_gate && report->warnings > 0) {
    printf("MEZZANINE CLEAN ERROR: strict quality gate failed (%i warning(s) detected)\n", report->warnings);
    success = false;
  }

  printf("MEZZANINE CLEAN SUMMARY: scanned=%i planned_moves=%i applied_moves=%i warnings=%i errors=%i skipped=%i\n",
         report->scanned_files,
         report->planned_moves,
         report->applied_moves,
         report->warnings,
         report->errors,
         report->skipped);

  return success && report->errors == 0;
}
