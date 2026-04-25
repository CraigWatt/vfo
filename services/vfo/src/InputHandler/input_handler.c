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

#include "ih_internal.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define IH_LEGACY_CONFIG_DIR "/usr/local/bin/vfo_conf_folder"
#define IH_CONFIG_DIR_ENV "VFO_CONFIG_DIR"
#define IH_CONFIG_FILENAME "vfo_config.conf"
#define IH_INPUT_BUFFER_SIZE 4096
#define IH_PREVIEW_LIMIT 140
#define IH_VISUALIZE_SUBDIR "visualizations"
#define IH_VISUALIZE_STATUS_FILENAME "status.json"
#define IH_VISUALIZE_MERMAID_FILENAME "workflow.mmd"
#define IH_VISUALIZE_HTML_FILENAME "index.html"
#define IH_ASSUME_YES_ENV "VFO_ASSUME_YES"
#define IH_AUTO_INTERVAL_SECONDS_ENV "VFO_AUTO_INTERVAL_SECONDS"
#define IH_AUTO_MAX_ITERATIONS_ENV "VFO_AUTO_MAX_ITERATIONS"
#define IH_TOOLCHAIN_MODE_ENV "TOOLCHAIN_MODE"

typedef struct ih_stock_preset_option {
  const char *key;
  const char *display_name;
  const char *relative_path;
} ih_stock_preset_option_t;

typedef enum ih_toolchain_mode {
  IH_TOOLCHAIN_MODE_SYSTEM = 0,
  IH_TOOLCHAIN_MODE_AUTO,
  IH_TOOLCHAIN_MODE_MANAGED,
  IH_TOOLCHAIN_MODE_INVALID
} ih_toolchain_mode_t;

static const ih_stock_preset_option_t IH_STOCK_PRESET_OPTIONS[] = {
  {"balanced_open_audio", "Balanced Open Audio", "balanced_open_audio/vfo_config.preset.conf"},
  {"roku_family_all_sub_convert_audio_conform",
   "Roku Family All Subtitle Convert Audio Conform",
   "roku-family-all-sub-convert-audio-conform/vfo_config.preset.conf"},
  {"fire_tv_family_all_sub_convert_audio_conform",
   "Fire TV Family All Subtitle Convert Audio Conform",
   "fire-tv-family-all-sub-convert-audio-conform/vfo_config.preset.conf"},
  {"chromecast_google_tv_family_all_sub_convert_audio_conform",
   "Chromecast Google TV Family All Subtitle Convert Audio Conform",
   "chromecast-google-tv-family-all-sub-convert-audio-conform/vfo_config.preset.conf"},
  {"apple_tv_family_all_sub_convert_audio_conform",
   "Apple TV Family All Subtitle Convert Audio Conform",
   "apple-tv-family-all-sub-convert-audio-conform/vfo_config.preset.conf"},
  {"fire_tv_stick_4k_dv_all_sub_convert_audio_conform",
   "Fire TV Stick 4K DV All Subtitle Convert Audio Conform",
   "fire-tv-stick-4k-dv-all-sub-convert-audio-conform/vfo_config.preset.conf"},
  {"device_targets_open_audio", "Device Targets Open Audio", "device_targets_open_audio/vfo_config.preset.conf"},
  {"craigstreamy_hevc_all_sub_preserve",
   "Craigstreamy HEVC All Subtitle Preserve",
   "craigstreamy-hevc-all-sub-preserve/vfo_config.preset.conf"},
  {"craigstreamy_hevc_all_sub_audio_conform",
   "Craigstreamy HEVC All Subtitle Audio Conform",
   "craigstreamy-hevc-all-sub-audio-conform/vfo_config.preset.conf"},
  {"craigstreamy_hevc_smart_eng_sub_aggressive_vmaf",
   "Craigstreamy HEVC Smart English Subtitle Aggressive VMAF",
   "craigstreamy-hevc-smart-eng-sub-aggressive-vmaf/vfo_config.preset.conf"},
  {"craigstreamy_hevc_smart_eng_sub_audio_conform_aggressive_vmaf",
   "Craigstreamy HEVC Smart English Subtitle Audio Conform Aggressive VMAF",
   "craigstreamy-hevc-smart-eng-sub-audio-conform-aggressive-vmaf/vfo_config.preset.conf"},
  {"craigstreamy_hevc_smart_eng_sub_audio_conform",
   "Craigstreamy HEVC Smart English Subtitle Audio Conform",
   "craigstreamy-hevc-smart-eng-sub-audio-conform/vfo_config.preset.conf"},
  {"craigstreamy_hevc_smart_eng_sub_subtitle_convert_audio_conform",
   "Craigstreamy HEVC Smart English Subtitle Convert Audio Conform",
   "craigstreamy-hevc-smart-eng-sub-subtitle-convert-audio-conform/vfo_config.preset.conf"},
  {"craigstreamy_hevc_smart_eng_sub_subtitle_convert",
   "Craigstreamy HEVC Smart English Subtitle Convert",
   "craigstreamy-hevc-smart-eng-sub-subtitle-convert/vfo_config.preset.conf"},
  {"craigstreamy_hevc_selected_english_subtitle_preserve",
   "Craigstreamy HEVC Selected English Subtitle Preserve",
   "craigstreamy-hevc-selected-english-subtitle-preserve/vfo_config.preset.conf"}
};

#define IH_STOCK_PRESET_COUNT ((int)(sizeof(IH_STOCK_PRESET_OPTIONS) / sizeof(IH_STOCK_PRESET_OPTIONS[0])))

static int ih_cs_count(cs_node_t *cs_head);
static bool ih_execute_default_run(config_t *config);
static bool ih_stock_preset_token_matches_option(const ih_stock_preset_option_t *option, const char *token);
static int ih_resolve_stock_preset_index(const char *token);

static bool ih_stock_preset_token_matches_option(const ih_stock_preset_option_t *option, const char *token) {
  if(option == NULL || token == NULL)
    return false;

  if(strcasecmp(token, option->key) == 0)
    return true;

  if(strcmp(option->key, "craigstreamy_hevc_selected_english_subtitle_preserve") == 0
     && strcasecmp(token, "netflixy_main_subtitle_intent") == 0)
    return true;

  return false;
}

static int ih_resolve_stock_preset_index(const char *token) {
  if(token == NULL)
    return -1;

  for(int i = 0; i < IH_STOCK_PRESET_COUNT; i++) {
    if(ih_stock_preset_token_matches_option(&IH_STOCK_PRESET_OPTIONS[i], token))
      return i;
  }

  return -1;
}

static bool ih_suppress_stdout_begin(int *saved_stdout_fd) {
  int dev_null_fd = -1;

  if(saved_stdout_fd == NULL)
    return false;

  *saved_stdout_fd = dup(STDOUT_FILENO);
  if(*saved_stdout_fd < 0)
    return false;

  fflush(stdout);
  dev_null_fd = open("/dev/null", O_WRONLY);
  if(dev_null_fd < 0) {
    close(*saved_stdout_fd);
    *saved_stdout_fd = -1;
    return false;
  }

  if(dup2(dev_null_fd, STDOUT_FILENO) < 0) {
    close(dev_null_fd);
    close(*saved_stdout_fd);
    *saved_stdout_fd = -1;
    return false;
  }

  close(dev_null_fd);
  return true;
}

static void ih_suppress_stdout_end(int saved_stdout_fd) {
  if(saved_stdout_fd < 0)
    return;

  fflush(stdout);
  dup2(saved_stdout_fd, STDOUT_FILENO);
  close(saved_stdout_fd);
}

static void ih_copy_trimmed_string(const char *input, char *output, size_t output_size) {
  const char *start = input;
  const char *end = NULL;
  size_t length = 0;

  if(output == NULL || output_size == 0)
    return;

  output[0] = '\0';
  if(input == NULL)
    return;

  while(*start != '\0' && isspace((unsigned char)*start))
    start++;

  end = start + strlen(start);
  while(end > start && isspace((unsigned char)*(end - 1)))
    end--;

  length = (size_t)(end - start);
  if(length >= output_size)
    length = output_size - 1;

  if(length > 0)
    memcpy(output, start, length);
  output[length] = '\0';
}

static ih_toolchain_mode_t ih_resolve_toolchain_mode(char *mode_value_out, size_t mode_value_out_size) {
  const char *raw_mode = getenv(IH_TOOLCHAIN_MODE_ENV);
  char normalized_mode[64];

  if(mode_value_out != NULL && mode_value_out_size > 0)
    mode_value_out[0] = '\0';

  if(raw_mode == NULL) {
    if(mode_value_out != NULL && mode_value_out_size > 0)
      snprintf(mode_value_out, mode_value_out_size, "system");
    return IH_TOOLCHAIN_MODE_SYSTEM;
  }

  ih_copy_trimmed_string(raw_mode, normalized_mode, sizeof(normalized_mode));
  if(normalized_mode[0] == '\0') {
    if(mode_value_out != NULL && mode_value_out_size > 0)
      snprintf(mode_value_out, mode_value_out_size, "system");
    return IH_TOOLCHAIN_MODE_SYSTEM;
  }

  for(size_t i = 0; normalized_mode[i] != '\0'; i++)
    normalized_mode[i] = (char)tolower((unsigned char)normalized_mode[i]);

  if(mode_value_out != NULL && mode_value_out_size > 0)
    snprintf(mode_value_out, mode_value_out_size, "%s", normalized_mode);

  if(strcmp(normalized_mode, "system") == 0)
    return IH_TOOLCHAIN_MODE_SYSTEM;
  if(strcmp(normalized_mode, "auto") == 0)
    return IH_TOOLCHAIN_MODE_AUTO;
  if(strcmp(normalized_mode, "managed") == 0)
    return IH_TOOLCHAIN_MODE_MANAGED;

  return IH_TOOLCHAIN_MODE_INVALID;
}

static bool ih_validate_toolchain_mode(const char *context,
                                       ih_toolchain_mode_t *mode_out,
                                       char *mode_value_out,
                                       size_t mode_value_out_size,
                                       bool emit_logs) {
  ih_toolchain_mode_t mode = ih_resolve_toolchain_mode(mode_value_out, mode_value_out_size);
  const char *context_safe = context == NULL ? "TOOLCHAIN" : context;
  const char *mode_value = (mode_value_out == NULL || mode_value_out[0] == '\0') ? "unknown" : mode_value_out;

  if(mode_out != NULL)
    *mode_out = mode;

  if(mode == IH_TOOLCHAIN_MODE_INVALID) {
    if(emit_logs) {
      printf("%s ERROR: unsupported %s=%s\n", context_safe, IH_TOOLCHAIN_MODE_ENV, mode_value);
      printf("%s INFO: supported values are system|auto|managed\n", context_safe);
    }
    return false;
  }

  if(mode == IH_TOOLCHAIN_MODE_MANAGED) {
    if(emit_logs) {
      printf("%s ERROR: %s=managed is not available yet in vfo CLI runtime\n", context_safe, IH_TOOLCHAIN_MODE_ENV);
      printf("%s INFO: use %s=system or %s=auto for now\n", context_safe, IH_TOOLCHAIN_MODE_ENV, IH_TOOLCHAIN_MODE_ENV);
    }
    return false;
  }

  if(emit_logs) {
    if(mode == IH_TOOLCHAIN_MODE_AUTO)
      printf("%s INFO: %s=auto (managed lane not active yet; using host PATH commands)\n", context_safe, IH_TOOLCHAIN_MODE_ENV);
    else
      printf("%s INFO: %s=system (using host PATH commands)\n", context_safe, IH_TOOLCHAIN_MODE_ENV);
  }

  return true;
}

static bool ih_command_exists(const char *command_name) {
  char shell_command[256];
  snprintf(shell_command, sizeof(shell_command), "command -v %s >/dev/null 2>&1", command_name);
  return system(shell_command) == 0;
}

static bool ih_library_exists(const char *library_name) {
  char shell_command[512];

  if(library_name == NULL || library_name[0] == '\0')
    return false;

  snprintf(shell_command,
           sizeof(shell_command),
           "pkg-config --exists %s >/dev/null 2>&1 || pkgconf --exists %s >/dev/null 2>&1",
           library_name,
           library_name);
  return system(shell_command) == 0;
}

static const char* ih_command_install_hint(const char *command_name) {
  if(strcmp(command_name, "ffmpeg") == 0 || strcmp(command_name, "ffprobe") == 0)
    return "macOS install hint: brew install ffmpeg";

  if(strcmp(command_name, "mkvmerge") == 0)
    return "macOS install hint: brew install mkvtoolnix";

  if(strcmp(command_name, "dovi_tool") == 0)
    return "macOS install hint: brew install dovi_tool";

  return NULL;
}

static const char* ih_library_install_hint(const char *library_name) {
  if(strcmp(library_name, "libplacebo") == 0)
    return "macOS install hint: brew install libplacebo";

  return NULL;
}

static bool ih_check_command(const char *context, const char *command_name, bool required) {
  bool exists = ih_command_exists(command_name);
  const char *hint = ih_command_install_hint(command_name);

  if(exists) {
    printf("%s OK: command available: %s\n", context, command_name);
    return true;
  }

  if(required) {
    printf("%s ERROR: required command missing: %s\n", context, command_name);
    if(hint != NULL)
      printf("%s INFO: %s\n", context, hint);
    return false;
  }

  printf("%s WARN: optional command missing: %s\n", context, command_name);
  if(hint != NULL)
    printf("%s INFO: %s\n", context, hint);
  return true;
}

static bool ih_check_library(const char *context, const char *library_name, bool required) {
  bool exists = ih_library_exists(library_name);
  const char *hint = ih_library_install_hint(library_name);

  if(exists) {
    printf("%s OK: library available: %s\n", context, library_name);
    return true;
  }

  if(required) {
    printf("%s ERROR: required library missing: %s\n", context, library_name);
    if(hint != NULL)
      printf("%s INFO: %s\n", context, hint);
    return false;
  }

  printf("%s WARN: optional library missing: %s\n", context, library_name);
  if(hint != NULL)
    printf("%s INFO: %s\n", context, hint);
  return true;
}

static bool ih_ffmpeg_filter_available(const char *filter_name) {
  char shell_command[512];

  if(filter_name == NULL || filter_name[0] == '\0')
    return false;

  snprintf(shell_command,
           sizeof(shell_command),
           "ffmpeg -hide_banner -filters 2>/dev/null | grep -E -q '(^| )%s( |$)'",
           filter_name);
  return system(shell_command) == 0;
}

static void ih_append_csv_token(char *buffer, size_t buffer_size, const char *token) {
  size_t used = 0;

  if(buffer == NULL || token == NULL || token[0] == '\0' || buffer_size == 0)
    return;

  used = strlen(buffer);
  if(used >= buffer_size - 1)
    return;

  if(used > 0)
    snprintf(buffer + used, buffer_size - used, ",%s", token);
  else
    snprintf(buffer, buffer_size, "%s", token);
}

static status_state_t ih_resolve_base_tier_state(bool ffmpeg_available,
                                                 bool ffprobe_available,
                                                 bool mkvmerge_available,
                                                 char *detail,
                                                 size_t detail_size) {
  char missing[128];

  if(detail != NULL && detail_size > 0)
    detail[0] = '\0';
  missing[0] = '\0';

  if(!ffmpeg_available)
    ih_append_csv_token(missing, sizeof(missing), "ffmpeg");
  if(!ffprobe_available)
    ih_append_csv_token(missing, sizeof(missing), "ffprobe");
  if(!mkvmerge_available)
    ih_append_csv_token(missing, sizeof(missing), "mkvmerge");

  if(missing[0] == '\0') {
    if(detail != NULL && detail_size > 0)
      snprintf(detail, detail_size, "ffmpeg+ffprobe+mkvmerge ready");
    return STATUS_STATE_COMPLETE;
  }

  if(detail != NULL && detail_size > 0)
    snprintf(detail, detail_size, "missing required tools: %s", missing);
  return STATUS_STATE_ERROR;
}

static status_state_t ih_resolve_dv_tier_state(bool dovi_tool_available,
                                               char *detail,
                                               size_t detail_size) {
  if(dovi_tool_available) {
    if(detail != NULL && detail_size > 0)
      snprintf(detail, detail_size, "dovi_tool available");
    return STATUS_STATE_COMPLETE;
  }

  if(detail != NULL && detail_size > 0)
    snprintf(detail, detail_size, "dovi_tool missing (DV workflows unavailable)");
  return STATUS_STATE_SKIPPED;
}

static status_state_t ih_resolve_quality_tier_state(bool quality_enabled,
                                                    bool quality_include_vmaf,
                                                    bool ffmpeg_available,
                                                    bool libvmaf_available,
                                                    char *detail,
                                                    size_t detail_size) {
  if(!quality_enabled) {
    if(detail != NULL && detail_size > 0)
      snprintf(detail, detail_size, "QUALITY_CHECK_ENABLED=false");
    return STATUS_STATE_SKIPPED;
  }

  if(!ffmpeg_available) {
    if(detail != NULL && detail_size > 0)
      snprintf(detail, detail_size, "ffmpeg missing (quality scoring unavailable)");
    return STATUS_STATE_ERROR;
  }

  if(quality_include_vmaf && !libvmaf_available) {
    if(detail != NULL && detail_size > 0)
      snprintf(detail, detail_size, "QUALITY_CHECK_INCLUDE_VMAF=true but libvmaf unavailable");
    return STATUS_STATE_ERROR;
  }

  if(quality_include_vmaf) {
    if(detail != NULL && detail_size > 0)
      snprintf(detail, detail_size, "PSNR+SSIM+VMAF ready");
  } else {
    if(detail != NULL && detail_size > 0)
      snprintf(detail, detail_size, "PSNR+SSIM ready (VMAF optional disabled)");
  }
  return STATUS_STATE_COMPLETE;
}

static const char* ih_tier_label_from_state(status_state_t state) {
  switch(state) {
    case STATUS_STATE_COMPLETE:
      return "PASS";
    case STATUS_STATE_ERROR:
      return "FAIL";
    case STATUS_STATE_SKIPPED:
      return "WARN";
    case STATUS_STATE_PENDING:
    case STATUS_STATE_IN_PROGRESS:
    default:
      return "WARN";
  }
}

#ifdef TESTING
void ih_evaluate_tier_states_for_test(bool ffmpeg_available,
                                      bool ffprobe_available,
                                      bool mkvmerge_available,
                                      bool dovi_tool_available,
                                      bool quality_enabled,
                                      bool quality_include_vmaf,
                                      bool libvmaf_available,
                                      status_state_t *base_state_out,
                                      status_state_t *dv_state_out,
                                      status_state_t *quality_state_out) {
  if(base_state_out != NULL)
    *base_state_out = ih_resolve_base_tier_state(ffmpeg_available,
                                                 ffprobe_available,
                                                 mkvmerge_available,
                                                 NULL,
                                                 0);

  if(dv_state_out != NULL)
    *dv_state_out = ih_resolve_dv_tier_state(dovi_tool_available, NULL, 0);

  if(quality_state_out != NULL)
    *quality_state_out = ih_resolve_quality_tier_state(quality_enabled,
                                                       quality_include_vmaf,
                                                       ffmpeg_available,
                                                       libvmaf_available,
                                                       NULL,
                                                       0);
}

bool ih_resolve_stock_preset_for_test(const char *token,
                                      char *canonical_key_out,
                                      size_t canonical_key_out_size,
                                      char *relative_path_out,
                                      size_t relative_path_out_size) {
  int index = ih_resolve_stock_preset_index(token);

  if(index < 0)
    return false;

  if(canonical_key_out != NULL && canonical_key_out_size > 0)
    snprintf(canonical_key_out, canonical_key_out_size, "%s", IH_STOCK_PRESET_OPTIONS[index].key);

  if(relative_path_out != NULL && relative_path_out_size > 0)
    snprintf(relative_path_out, relative_path_out_size, "%s", IH_STOCK_PRESET_OPTIONS[index].relative_path);

  return true;
}

const char* ih_library_install_hint_for_test(const char *library_name) {
  return ih_library_install_hint(library_name);
}
#endif

static bool ih_doctor_check_command(const char *command_name, bool required) {
  return ih_check_command("DOCTOR", command_name, required);
}

static bool ih_doctor_check_library(const char *library_name, bool required) {
  return ih_check_library("DOCTOR", library_name, required);
}

static bool ih_doctor_check_path(const char *label, const char *path, bool required) {
  bool exists = utils_does_folder_exist((char *)path);
  if(exists) {
    printf("DOCTOR OK: %s exists: %s\n", label, path);
    return true;
  }
  if(required) {
    printf("DOCTOR ERROR: %s missing: %s\n", label, path);
    return false;
  }
  printf("DOCTOR WARN: %s missing: %s\n", label, path);
  return true;
}

static bool ih_doctor_check_location_list(const char *label, char *locations_csv) {
  int location_count = 0;
  char **locations = NULL;
  bool healthy = true;

  if(utils_string_is_empty_or_spaces(locations_csv))
    return false;

  locations = utils_split_semicolon_list(locations_csv, &location_count);
  if(location_count == 0) {
    printf("DOCTOR ERROR: %s has no valid locations configured\n", label);
    utils_free_string_array(locations, location_count);
    return false;
  }
  for(int i = 0; i < location_count; i++) {
    char list_label[256];
    snprintf(list_label, sizeof(list_label), "%s[%i]", label, i + 1);
    healthy = ih_doctor_check_path(list_label, locations[i], true) && healthy;
  }
  utils_free_string_array(locations, location_count);
  return healthy;
}

static bool ih_run_doctor(config_t *config, const char *config_dir) {
  bool healthy = true;
  ih_toolchain_mode_t toolchain_mode = IH_TOOLCHAIN_MODE_SYSTEM;
  char toolchain_mode_value[64];
  bool ffmpeg_available = false;
  bool ffprobe_available = false;
  bool mkvmerge_available = false;
  bool dovi_tool_available = false;
  bool libvmaf_available = false;
  status_state_t tier_base_state = STATUS_STATE_PENDING;
  status_state_t tier_dv_state = STATUS_STATE_PENDING;
  status_state_t tier_quality_state = STATUS_STATE_PENDING;
  char tier_base_detail[256];
  char tier_dv_detail[256];
  char tier_quality_detail[256];

  printf("DOCTOR ALERT: running environment and configuration checks\n");
  if(ih_validate_toolchain_mode("DOCTOR",
                                &toolchain_mode,
                                toolchain_mode_value,
                                sizeof(toolchain_mode_value),
                                true) == false) {
    printf("DOCTOR ALERT: checks completed with errors\n");
    return false;
  }
  healthy = ih_doctor_check_command("ffmpeg", true) && healthy;
  healthy = ih_doctor_check_command("ffprobe", true) && healthy;
  healthy = ih_doctor_check_command("mkvmerge", true) && healthy;
  healthy = ih_doctor_check_command("dovi_tool", false) && healthy;
  healthy = ih_doctor_check_library("libplacebo", false) && healthy;

  healthy = ih_doctor_check_path("config directory", config_dir, true) && healthy;
  healthy = ih_doctor_check_location_list("MEZZANINE_LOCATIONS", config->svc->original_locations) && healthy;
  healthy = ih_doctor_check_location_list("SOURCE_LOCATIONS", config->svc->source_locations) && healthy;

  if(!(utils_string_is_empty_or_spaces(config->svc->source_as_location)))
    healthy = ih_doctor_check_path("SOURCE_AS_LOCATION", config->svc->source_as_location, false) && healthy;

  if(ca_get_count(config->ca_head) > 0)
    printf("DOCTOR OK: detected %i profile(s) in config\n", ca_get_count(config->ca_head));
  else {
    printf("DOCTOR ERROR: no profiles detected in config\n");
    healthy = false;
  }

  printf("DOCTOR INFO: KEEP_SOURCE=%s\n", config->svc->keep_source ? "true" : "false");
  printf("DOCTOR INFO: MEZZANINE_CLEAN_ENABLED=%s\n", config->svc->mezzanine_clean_enabled ? "true" : "false");
  printf("DOCTOR INFO: MEZZANINE_CLEAN_APPLY_CHANGES=%s\n", config->svc->mezzanine_clean_apply_changes ? "true" : "false");
  printf("DOCTOR INFO: MEZZANINE_CLEAN_APPEND_MEDIA_TAGS=%s\n", config->svc->mezzanine_clean_append_media_tags ? "true" : "false");
  printf("DOCTOR INFO: MEZZANINE_CLEAN_STRICT_QUALITY_GATE=%s\n", config->svc->mezzanine_clean_strict_quality_gate ? "true" : "false");
  printf("DOCTOR INFO: QUALITY_CHECK_ENABLED=%s\n", config->svc->quality_check_enabled ? "true" : "false");
  printf("DOCTOR INFO: QUALITY_CHECK_INCLUDE_VMAF=%s\n", config->svc->quality_check_include_vmaf ? "true" : "false");
  printf("DOCTOR INFO: QUALITY_CHECK_STRICT_GATE=%s\n", config->svc->quality_check_strict_gate ? "true" : "false");
  printf("DOCTOR INFO: QUALITY_CHECK_REFERENCE_LAYER=%s\n", config->svc->quality_check_reference_layer);
  printf("DOCTOR INFO: QUALITY_CHECK_MIN_PSNR=%.3f\n", config->svc->quality_check_min_psnr);
  printf("DOCTOR INFO: QUALITY_CHECK_MIN_SSIM=%.6f\n", config->svc->quality_check_min_ssim);
  printf("DOCTOR INFO: QUALITY_CHECK_MIN_VMAF=%.3f\n", config->svc->quality_check_min_vmaf);
  printf("DOCTOR INFO: QUALITY_CHECK_MAX_FILES_PER_PROFILE=%i\n", config->svc->quality_check_max_files_per_profile);

  ffmpeg_available = ih_command_exists("ffmpeg");
  ffprobe_available = ih_command_exists("ffprobe");
  mkvmerge_available = ih_command_exists("mkvmerge");
  dovi_tool_available = ih_command_exists("dovi_tool");
  libvmaf_available = ih_ffmpeg_filter_available("libvmaf");

  tier_base_state = ih_resolve_base_tier_state(ffmpeg_available,
                                               ffprobe_available,
                                               mkvmerge_available,
                                               tier_base_detail,
                                               sizeof(tier_base_detail));
  tier_dv_state = ih_resolve_dv_tier_state(dovi_tool_available,
                                           tier_dv_detail,
                                           sizeof(tier_dv_detail));
  tier_quality_state = ih_resolve_quality_tier_state(config->svc->quality_check_enabled,
                                                     config->svc->quality_check_include_vmaf,
                                                     ffmpeg_available,
                                                     libvmaf_available,
                                                     tier_quality_detail,
                                                     sizeof(tier_quality_detail));

  printf("DOCTOR TIER SUMMARY:\n");
  printf("DOCTOR TIER: base => %s (%s)\n",
         ih_tier_label_from_state(tier_base_state),
         tier_base_detail);
  printf("DOCTOR TIER: dv => %s (%s)\n",
         ih_tier_label_from_state(tier_dv_state),
         tier_dv_detail);
  printf("DOCTOR TIER: quality => %s (%s)\n",
         ih_tier_label_from_state(tier_quality_state),
         tier_quality_detail);

  if(tier_base_state == STATUS_STATE_ERROR
     || tier_dv_state == STATUS_STATE_ERROR
     || tier_quality_state == STATUS_STATE_ERROR)
    healthy = false;

  ca_node_t *alias_profile = config->ca_head;
  while(alias_profile != NULL) {
    char label[256];
    snprintf(label, sizeof(label), "PROFILE_%s_LOCATIONS", alias_profile->alias_name);
    healthy = ih_doctor_check_location_list(label, alias_profile->alias_locations) && healthy;
    alias_profile = alias_profile->next;
  }

  if(healthy) {
    printf("DOCTOR ALERT: checks completed successfully\n");
    return true;
  }
  printf("DOCTOR ALERT: checks completed with errors\n");
  return false;
}

static void ih_status_update_with_log(status_report_t *report,
                                      const char *component,
                                      status_state_t state,
                                      const char *detail) {
  status_report_update(report, component, state, detail);
  printf("STATUS %s %s: %s\n",
         status_state_to_string(state),
         component,
         detail == NULL ? "" : detail);
}

static bool ih_status_check_command(status_report_t *report, const char *command_name, bool required) {
  char component[256];
  char detail[512];
  bool exists = ih_command_exists(command_name);
  const char *hint = ih_command_install_hint(command_name);

  snprintf(component, sizeof(component), "dependency.%s", command_name);

  if(exists) {
    snprintf(detail, sizeof(detail), "%s available", command_name);
    status_report_update(report, component, STATUS_STATE_COMPLETE, detail);
    return true;
  }

  if(required) {
    if(hint != NULL)
      snprintf(detail, sizeof(detail), "%s missing (%s)", command_name, hint);
    else
      snprintf(detail, sizeof(detail), "%s missing", command_name);
    status_report_update(report, component, STATUS_STATE_ERROR, detail);
    return false;
  }

  if(hint != NULL)
    snprintf(detail, sizeof(detail), "%s missing (%s)", command_name, hint);
  else
    snprintf(detail, sizeof(detail), "%s missing", command_name);
  status_report_update(report, component, STATUS_STATE_SKIPPED, detail);
  return true;
}

static void ih_status_mark_dependency_blocked(status_report_t *report,
                                              const char *command_name,
                                              bool required,
                                              const char *reason) {
  char component[256];
  char detail[512];

  snprintf(component, sizeof(component), "dependency.%s", command_name);
  snprintf(detail,
           sizeof(detail),
           "%s check blocked: %s",
           command_name,
           reason == NULL ? "toolchain mode unavailable" : reason);
  status_report_update(report,
                       component,
                       required ? STATUS_STATE_ERROR : STATUS_STATE_SKIPPED,
                       detail);
}

static bool ih_status_check_location_list(status_report_t *report,
                                          const char *component_prefix,
                                          char *locations_csv,
                                          bool required) {
  int location_count = 0;
  char **locations = NULL;
  bool healthy = true;
  char missing_detail[256];

  if(utils_string_is_empty_or_spaces(locations_csv)) {
    snprintf(missing_detail, sizeof(missing_detail), "no configured locations for %s", component_prefix);
    status_report_update(report,
                         component_prefix,
                         required ? STATUS_STATE_ERROR : STATUS_STATE_SKIPPED,
                         missing_detail);
    return !required;
  }

  locations = utils_split_semicolon_list(locations_csv, &location_count);
  if(location_count == 0) {
    snprintf(missing_detail, sizeof(missing_detail), "no valid locations in list for %s", component_prefix);
    status_report_update(report,
                         component_prefix,
                         required ? STATUS_STATE_ERROR : STATUS_STATE_SKIPPED,
                         missing_detail);
    utils_free_string_array(locations, location_count);
    return !required;
  }

  for(int i = 0; i < location_count; i++) {
    char component[320];
    char detail[512];
    bool exists = utils_does_folder_exist(locations[i]);

    snprintf(component, sizeof(component), "%s[%i]", component_prefix, i + 1);

    if(exists) {
      snprintf(detail, sizeof(detail), "%s exists", locations[i]);
      status_report_update(report, component, STATUS_STATE_COMPLETE, detail);
    } else {
      snprintf(detail, sizeof(detail), "%s missing", locations[i]);
      status_report_update(report, component, required ? STATUS_STATE_ERROR : STATUS_STATE_SKIPPED, detail);
      if(required)
        healthy = false;
    }
  }

  utils_free_string_array(locations, location_count);
  return healthy;
}

static bool ih_status_check_profiles(status_report_t *report, ca_node_t *profiles_head) {
  int profile_count = ca_get_count(profiles_head);
  ca_node_t *profile = profiles_head;
  bool healthy = true;
  char detail[512];

  if(profile_count == 0) {
    status_report_update(report, "profiles.detected", STATUS_STATE_ERROR, "no profiles configured");
    return false;
  }

  snprintf(detail, sizeof(detail), "%i profile(s) configured", profile_count);
  status_report_update(report, "profiles.detected", STATUS_STATE_COMPLETE, detail);

  while(profile != NULL) {
    char location_component[320];
    char scenario_component[320];
    int scenario_count = ih_cs_count(profile->cs_head);

    snprintf(location_component, sizeof(location_component), "profile.%s.locations", profile->alias_name);
    if(ih_status_check_location_list(report, location_component, profile->alias_locations, true) == false)
      healthy = false;

    snprintf(scenario_component, sizeof(scenario_component), "profile.%s.scenarios", profile->alias_name);
    if(scenario_count > 0) {
      snprintf(detail, sizeof(detail), "%i scenario(s) configured", scenario_count);
      status_report_update(report, scenario_component, STATUS_STATE_COMPLETE, detail);
    } else {
      status_report_update(report, scenario_component, STATUS_STATE_ERROR, "no scenarios configured");
      healthy = false;
    }

    profile = profile->next;
  }

  return healthy;
}

static status_state_t ih_status_state_for_component(const status_report_t *report,
                                                    const char *component_name,
                                                    bool *found_out) {
  status_component_t *cursor = NULL;

  if(found_out != NULL)
    *found_out = false;

  if(report == NULL || component_name == NULL)
    return STATUS_STATE_PENDING;

  cursor = report->head;
  while(cursor != NULL) {
    if(cursor->name != NULL && strcmp(cursor->name, component_name) == 0) {
      if(found_out != NULL)
        *found_out = true;
      return cursor->state;
    }
    cursor = cursor->next;
  }

  return STATUS_STATE_PENDING;
}

static const char* ih_status_css_class_for_state(status_state_t state) {
  switch(state) {
    case STATUS_STATE_IN_PROGRESS:
      return "in_progress";
    case STATUS_STATE_COMPLETE:
      return "complete";
    case STATUS_STATE_ERROR:
      return "error";
    case STATUS_STATE_SKIPPED:
      return "skipped";
    case STATUS_STATE_PENDING:
    default:
      return "pending";
  }
}

static bool ih_ensure_directory_recursive(const char *path) {
  char buffer[IH_INPUT_BUFFER_SIZE];
  size_t length = 0;
  char *cursor = NULL;

  if(path == NULL || path[0] == '\0')
    return false;

  snprintf(buffer, sizeof(buffer), "%s", path);
  length = strlen(buffer);
  if(length == 0)
    return false;

  if(length > 1 && buffer[length - 1] == '/')
    buffer[length - 1] = '\0';

  for(cursor = buffer + 1; *cursor != '\0'; cursor++) {
    if(*cursor == '/') {
      *cursor = '\0';
      if(mkdir(buffer, 0755) != 0 && errno != EEXIST)
        return false;
      *cursor = '/';
    }
  }

  if(mkdir(buffer, 0755) != 0 && errno != EEXIST)
    return false;

  return true;
}

static void ih_build_visualize_run_id(char *output, size_t output_size) {
  time_t now = time(NULL);
  struct tm local_time;

  if(output == NULL || output_size == 0)
    return;

  if(localtime_r(&now, &local_time) == NULL) {
    snprintf(output, output_size, "run-%ld", (long)now);
    return;
  }

  if(strftime(output, output_size, "%Y%m%d-%H%M%S", &local_time) == 0)
    snprintf(output, output_size, "run-%ld", (long)now);
}

static void ih_write_html_escaped(FILE *stream, const char *value) {
  size_t i = 0;

  if(stream == NULL)
    return;

  if(value == NULL)
    value = "";

  for(i = 0; value[i] != '\0'; i++) {
    switch(value[i]) {
      case '&':
        fputs("&amp;", stream);
        break;
      case '<':
        fputs("&lt;", stream);
        break;
      case '>':
        fputs("&gt;", stream);
        break;
      case '"':
        fputs("&quot;", stream);
        break;
      case '\'':
        fputs("&#39;", stream);
        break;
      default:
        fputc(value[i], stream);
        break;
    }
  }
}

static bool ih_write_visualize_mermaid(FILE *stream, const status_report_t *report) {
  status_state_t snapshot_state = ih_status_state_for_component(report, "engine.snapshot", NULL);
  status_state_t mezzanine_clean_state = ih_status_state_for_component(report, "stage.mezzanine_clean", NULL);
  status_state_t mezzanine_state = ih_status_state_for_component(report, "stage.mezzanine", NULL);
  status_state_t source_state = ih_status_state_for_component(report, "stage.source", NULL);
  status_state_t profiles_state = ih_status_state_for_component(report, "stage.profiles", NULL);
  status_state_t quality_state = ih_status_state_for_component(report, "stage.quality", NULL);
  status_state_t execute_state = ih_status_state_for_component(report, "stage.execute", NULL);

  if(stream == NULL)
    return false;

  fprintf(stream, "flowchart LR\n");
  fprintf(stream, "  A[\"Snapshot\"] --> B[\"Mezzanine Clean\"] --> C[\"Mezzanine\"] --> D[\"Source\"] --> E[\"Profiles\"] --> F[\"Quality\"] --> G[\"Execute\"]\n");
  fprintf(stream, "  classDef pending fill:#fff4db,stroke:#d3a80f,color:#5a4700;\n");
  fprintf(stream, "  classDef in_progress fill:#d9ecff,stroke:#2082d8,color:#083a66;\n");
  fprintf(stream, "  classDef complete fill:#dbf6e3,stroke:#2f9b5f,color:#0f4f2d;\n");
  fprintf(stream, "  classDef error fill:#ffdede,stroke:#cf2f2f,color:#6e1111;\n");
  fprintf(stream, "  classDef skipped fill:#ececec,stroke:#8a8a8a,color:#4a4a4a;\n");
  fprintf(stream, "  class A %s;\n", ih_status_css_class_for_state(snapshot_state));
  fprintf(stream, "  class B %s;\n", ih_status_css_class_for_state(mezzanine_clean_state));
  fprintf(stream, "  class C %s;\n", ih_status_css_class_for_state(mezzanine_state));
  fprintf(stream, "  class D %s;\n", ih_status_css_class_for_state(source_state));
  fprintf(stream, "  class E %s;\n", ih_status_css_class_for_state(profiles_state));
  fprintf(stream, "  class F %s;\n", ih_status_css_class_for_state(quality_state));
  fprintf(stream, "  class G %s;\n", ih_status_css_class_for_state(execute_state));

  return true;
}

static bool ih_write_visualize_html(FILE *stream,
                                    const status_report_t *report,
                                    const char *generated_at,
                                    const char *status_json_path,
                                    const char *mermaid_path,
                                    bool healthy) {
  int pending = 0;
  int in_progress = 0;
  int complete = 0;
  int error = 0;
  int skipped = 0;
  status_component_t *cursor = NULL;
  int i = 0;
  struct {
    const char *label;
    const char *component;
  } stage_views[] = {
    {"Snapshot", "engine.snapshot"},
    {"Mezzanine Clean", "stage.mezzanine_clean"},
    {"Mezzanine", "stage.mezzanine"},
    {"Source", "stage.source"},
    {"Profiles", "stage.profiles"},
    {"Quality", "stage.quality"},
    {"Execute", "stage.execute"}
  };
  int stage_view_count = (int)(sizeof(stage_views) / sizeof(stage_views[0]));

  if(stream == NULL || report == NULL)
    return false;

  status_report_summary(report, &pending, &in_progress, &complete, &error, &skipped);

  fprintf(stream, "<!doctype html>\n");
  fprintf(stream, "<html lang=\"en\">\n");
  fprintf(stream, "<head>\n");
  fprintf(stream, "  <meta charset=\"utf-8\" />\n");
  fprintf(stream, "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />\n");
  fprintf(stream, "  <title>vfo visualize report</title>\n");
  fprintf(stream, "  <style>\n");
  fprintf(stream, "    body { font-family: -apple-system, BlinkMacSystemFont, \"Segoe UI\", sans-serif; margin: 24px; color: #102018; background: #f7faf7; }\n");
  fprintf(stream, "    h1, h2 { margin: 0 0 12px 0; }\n");
  fprintf(stream, "    .meta { color: #284034; margin-bottom: 18px; }\n");
  fprintf(stream, "    .summary { display: flex; gap: 10px; flex-wrap: wrap; margin: 14px 0 24px 0; }\n");
  fprintf(stream, "    .pill { border-radius: 999px; padding: 6px 10px; font-size: 13px; border: 1px solid transparent; }\n");
  fprintf(stream, "    .pending { background: #fff4db; border-color: #d3a80f; color: #5a4700; }\n");
  fprintf(stream, "    .in_progress { background: #d9ecff; border-color: #2082d8; color: #083a66; }\n");
  fprintf(stream, "    .complete { background: #dbf6e3; border-color: #2f9b5f; color: #0f4f2d; }\n");
  fprintf(stream, "    .error { background: #ffdede; border-color: #cf2f2f; color: #6e1111; }\n");
  fprintf(stream, "    .skipped { background: #ececec; border-color: #8a8a8a; color: #4a4a4a; }\n");
  fprintf(stream, "    .flow { display: flex; align-items: stretch; gap: 8px; overflow-x: auto; padding-bottom: 8px; margin-bottom: 24px; }\n");
  fprintf(stream, "    .node { min-width: 150px; border: 1px solid #cfd8cf; border-radius: 10px; padding: 10px; background: #fff; }\n");
  fprintf(stream, "    .node .label { font-weight: 600; margin-bottom: 6px; }\n");
  fprintf(stream, "    .node .component { font-family: ui-monospace, SFMono-Regular, Menlo, monospace; font-size: 12px; opacity: 0.85; }\n");
  fprintf(stream, "    .arrow { align-self: center; color: #4f6357; font-size: 22px; }\n");
  fprintf(stream, "    table { width: 100%%; border-collapse: collapse; background: #fff; }\n");
  fprintf(stream, "    th, td { border: 1px solid #d5ddd5; padding: 8px; text-align: left; vertical-align: top; font-size: 13px; }\n");
  fprintf(stream, "    th { background: #edf3ed; }\n");
  fprintf(stream, "    code { background: #edf2ed; border-radius: 6px; padding: 1px 6px; }\n");
  fprintf(stream, "    .refs { margin-top: 16px; color: #2b4033; }\n");
  fprintf(stream, "  </style>\n");
  fprintf(stream, "</head>\n");
  fprintf(stream, "<body>\n");
  fprintf(stream, "  <h1>vfo visualize report</h1>\n");
  fprintf(stream, "  <div class=\"meta\">Generated: ");
  ih_write_html_escaped(stream, generated_at);
  fprintf(stream, "</div>\n");

  fprintf(stream, "  <div class=\"summary\">\n");
  fprintf(stream, "    <span class=\"pill %s\">healthy=%s</span>\n",
          healthy ? "complete" : "error",
          healthy ? "true" : "false");
  fprintf(stream, "    <span class=\"pill pending\">pending=%d</span>\n", pending);
  fprintf(stream, "    <span class=\"pill in_progress\">in_progress=%d</span>\n", in_progress);
  fprintf(stream, "    <span class=\"pill complete\">complete=%d</span>\n", complete);
  fprintf(stream, "    <span class=\"pill error\">error=%d</span>\n", error);
  fprintf(stream, "    <span class=\"pill skipped\">skipped=%d</span>\n", skipped);
  fprintf(stream, "  </div>\n");

  fprintf(stream, "  <h2>Pipeline View</h2>\n");
  fprintf(stream, "  <div class=\"flow\">\n");
  for(i = 0; i < stage_view_count; i++) {
    bool found = false;
    status_state_t stage_state = ih_status_state_for_component(report, stage_views[i].component, &found);
    const char *state_label = status_state_to_string(stage_state);
    const char *css_class = ih_status_css_class_for_state(stage_state);
    fprintf(stream, "    <div class=\"node %s\">\n", css_class);
    fprintf(stream, "      <div class=\"label\">");
    ih_write_html_escaped(stream, stage_views[i].label);
    fprintf(stream, "</div>\n");
    fprintf(stream, "      <div class=\"component\">");
    ih_write_html_escaped(stream, stage_views[i].component);
    fprintf(stream, "</div>\n");
    fprintf(stream, "      <div><span class=\"pill %s\">", css_class);
    ih_write_html_escaped(stream, state_label);
    fprintf(stream, "</span></div>\n");
    fprintf(stream, "    </div>\n");
    if(i < stage_view_count - 1)
      fprintf(stream, "    <div class=\"arrow\">&#8594;</div>\n");
  }
  fprintf(stream, "  </div>\n");

  fprintf(stream, "  <h2>Components</h2>\n");
  fprintf(stream, "  <table>\n");
  fprintf(stream, "    <thead><tr><th>component</th><th>state</th><th>detail</th></tr></thead>\n");
  fprintf(stream, "    <tbody>\n");
  cursor = report->head;
  while(cursor != NULL) {
    const char *css_class = ih_status_css_class_for_state(cursor->state);
    fprintf(stream, "      <tr>\n");
    fprintf(stream, "        <td><code>");
    ih_write_html_escaped(stream, cursor->name == NULL ? "" : cursor->name);
    fprintf(stream, "</code></td>\n");
    fprintf(stream, "        <td><span class=\"pill %s\">", css_class);
    ih_write_html_escaped(stream, status_state_to_string(cursor->state));
    fprintf(stream, "</span></td>\n");
    fprintf(stream, "        <td>");
    ih_write_html_escaped(stream, cursor->detail == NULL ? "" : cursor->detail);
    fprintf(stream, "</td>\n");
    fprintf(stream, "      </tr>\n");
    cursor = cursor->next;
  }
  fprintf(stream, "    </tbody>\n");
  fprintf(stream, "  </table>\n");

  fprintf(stream, "  <div class=\"refs\">\n");
  fprintf(stream, "    <p>Artifacts: <code>");
  ih_write_html_escaped(stream, status_json_path);
  fprintf(stream, "</code>, <code>");
  ih_write_html_escaped(stream, mermaid_path);
  fprintf(stream, "</code></p>\n");
  fprintf(stream, "    <p>Formal models: <code>services/vfo/docs/workflow-engine.bpmn</code> and <code>services/vfo/docs/workflow-decisions.dmn</code>.</p>\n");
  fprintf(stream, "  </div>\n");
  fprintf(stream, "</body>\n");
  fprintf(stream, "</html>\n");

  return true;
}

static void ih_try_open_visualization(const char *html_path) {
  char command[IH_INPUT_BUFFER_SIZE * 2];

  if(html_path == NULL || html_path[0] == '\0')
    return;

#if defined(__APPLE__)
  snprintf(command, sizeof(command), "open \"%s\" >/dev/null 2>&1", html_path);
  if(system(command) == 0)
    printf("VISUALIZE ALERT: opened report in browser\n");
  else
    printf("VISUALIZE WARN: could not open report automatically; open this path manually: %s\n", html_path);
#elif defined(__linux__)
  snprintf(command, sizeof(command), "xdg-open \"%s\" >/dev/null 2>&1", html_path);
  if(system(command) == 0)
    printf("VISUALIZE ALERT: opened report in browser\n");
  else
    printf("VISUALIZE WARN: could not open report automatically; open this path manually: %s\n", html_path);
#else
  printf("VISUALIZE WARN: browser auto-open is not supported on this platform. Open this path manually: %s\n", html_path);
#endif
}

static bool ih_collect_status_snapshot(config_t *config, const char *config_dir, status_report_t *report) {
  bool healthy = true;
  bool originals_healthy = true;
  bool source_healthy = true;
  bool profiles_healthy = true;
  ih_toolchain_mode_t toolchain_mode = IH_TOOLCHAIN_MODE_SYSTEM;
  char toolchain_mode_value[64];
  char toolchain_detail[512];
  bool toolchain_mode_supported = false;
  bool ffmpeg_available = false;
  bool ffprobe_available = false;
  bool mkvmerge_available = false;
  bool dovi_tool_available = false;
  bool libplacebo_available = false;
  bool libvmaf_available = false;
  status_state_t tier_base_state = STATUS_STATE_PENDING;
  status_state_t tier_dv_state = STATUS_STATE_PENDING;
  status_state_t tier_quality_state = STATUS_STATE_PENDING;
  char tier_base_detail[256];
  char tier_dv_detail[256];
  char tier_quality_detail[256];

  status_report_update(report, "engine.snapshot", STATUS_STATE_IN_PROGRESS, "collecting component status");

  if(utils_does_folder_exist((char *)config_dir))
    status_report_update(report, "config.directory", STATUS_STATE_COMPLETE, config_dir);
  else {
    status_report_update(report, "config.directory", STATUS_STATE_ERROR, "config directory missing");
    healthy = false;
  }

  toolchain_mode_supported = ih_validate_toolchain_mode("STATUS",
                                                        &toolchain_mode,
                                                        toolchain_mode_value,
                                                        sizeof(toolchain_mode_value),
                                                        false);
  if(toolchain_mode_supported) {
    if(toolchain_mode == IH_TOOLCHAIN_MODE_AUTO)
      snprintf(toolchain_detail,
               sizeof(toolchain_detail),
               "%s=auto (managed lane not active yet; using host PATH commands)",
               IH_TOOLCHAIN_MODE_ENV);
    else
      snprintf(toolchain_detail,
               sizeof(toolchain_detail),
               "%s=system (using host PATH commands)",
               IH_TOOLCHAIN_MODE_ENV);
    status_report_update(report, "config.toolchain_mode", STATUS_STATE_COMPLETE, toolchain_detail);
  } else {
    healthy = false;
    if(toolchain_mode == IH_TOOLCHAIN_MODE_MANAGED)
      snprintf(toolchain_detail,
               sizeof(toolchain_detail),
               "%s=managed requested but managed CLI toolchain is not available yet",
               IH_TOOLCHAIN_MODE_ENV);
    else
      snprintf(toolchain_detail,
               sizeof(toolchain_detail),
               "unsupported %s=%s (supported: system|auto|managed)",
               IH_TOOLCHAIN_MODE_ENV,
               toolchain_mode_value[0] == '\0' ? "unknown" : toolchain_mode_value);
    status_report_update(report, "config.toolchain_mode", STATUS_STATE_ERROR, toolchain_detail);
  }

  if(toolchain_mode_supported) {
    ffmpeg_available = ih_command_exists("ffmpeg");
    ffprobe_available = ih_command_exists("ffprobe");
    mkvmerge_available = ih_command_exists("mkvmerge");
    dovi_tool_available = ih_command_exists("dovi_tool");
    libplacebo_available = ih_library_exists("libplacebo");

    healthy = ih_status_check_command(report, "ffmpeg", true) && healthy;
    healthy = ih_status_check_command(report, "ffprobe", true) && healthy;
    healthy = ih_status_check_command(report, "mkvmerge", true) && healthy;
    healthy = ih_status_check_command(report, "dovi_tool", false) && healthy;
    if(libplacebo_available) {
      status_report_update(report,
                           "dependency.libplacebo",
                           STATUS_STATE_COMPLETE,
                           "libplacebo library available");
    } else {
      status_report_update(report,
                           "dependency.libplacebo",
                           STATUS_STATE_SKIPPED,
                           "libplacebo library missing (optional, future profiles only)");
    }
    if(config->svc->quality_check_enabled && config->svc->quality_check_include_vmaf) {
      libvmaf_available = ih_ffmpeg_filter_available("libvmaf");
      if(libvmaf_available) {
        status_report_update(report,
                             "dependency.libvmaf",
                             STATUS_STATE_COMPLETE,
                             "ffmpeg libvmaf filter available");
      } else {
        status_report_update(report,
                             "dependency.libvmaf",
                             STATUS_STATE_ERROR,
                             "ffmpeg libvmaf filter missing (QUALITY_CHECK_INCLUDE_VMAF=true)");
        healthy = false;
      }
    } else {
      status_report_update(report,
                           "dependency.libvmaf",
                           STATUS_STATE_SKIPPED,
                           "VMAF scoring disabled");
    }
  } else {
    ih_status_mark_dependency_blocked(report,
                                      "ffmpeg",
                                      true,
                                      "toolchain mode is not usable");
    ih_status_mark_dependency_blocked(report,
                                      "ffprobe",
                                      true,
                                      "toolchain mode is not usable");
    ih_status_mark_dependency_blocked(report,
                                      "mkvmerge",
                                      true,
                                      "toolchain mode is not usable");
    ih_status_mark_dependency_blocked(report,
                                      "dovi_tool",
                                      false,
                                      "toolchain mode is not usable");
    status_report_update(report,
                         "dependency.libplacebo",
                         STATUS_STATE_SKIPPED,
                         "libplacebo check blocked because toolchain mode is not usable");
    status_report_update(report,
                         "dependency.libvmaf",
                         STATUS_STATE_SKIPPED,
                         "libvmaf check blocked because toolchain mode is not usable");
  }

  tier_base_state = ih_resolve_base_tier_state(ffmpeg_available,
                                               ffprobe_available,
                                               mkvmerge_available,
                                               tier_base_detail,
                                               sizeof(tier_base_detail));
  tier_dv_state = ih_resolve_dv_tier_state(dovi_tool_available,
                                           tier_dv_detail,
                                           sizeof(tier_dv_detail));
  tier_quality_state = ih_resolve_quality_tier_state(config->svc->quality_check_enabled,
                                                     config->svc->quality_check_include_vmaf,
                                                     ffmpeg_available,
                                                     libvmaf_available,
                                                     tier_quality_detail,
                                                     sizeof(tier_quality_detail));

  status_report_update(report, "engine.tier.base", tier_base_state, tier_base_detail);
  status_report_update(report, "engine.tier.dv", tier_dv_state, tier_dv_detail);
  status_report_update(report, "engine.tier.quality", tier_quality_state, tier_quality_detail);

  if(tier_base_state == STATUS_STATE_ERROR || tier_quality_state == STATUS_STATE_ERROR)
    healthy = false;

  originals_healthy = ih_status_check_location_list(report,
                                                    "storage.mezzanine",
                                                    config->svc->original_locations,
                                                    true);
  source_healthy = ih_status_check_location_list(report,
                                                 "storage.source",
                                                 config->svc->source_locations,
                                                 true);
  profiles_healthy = ih_status_check_profiles(report, config->ca_head);

  healthy = originals_healthy && source_healthy && profiles_healthy && healthy;

  status_report_update(report,
                       "stage.mezzanine",
                       originals_healthy ? STATUS_STATE_COMPLETE : STATUS_STATE_ERROR,
                       originals_healthy ? "ready to run mezzanine stage" : "mezzanine stage blocked by missing mezzanine locations");

  if(config->svc->mezzanine_clean_enabled) {
    status_report_update(report,
                         "stage.mezzanine_clean",
                         originals_healthy ? STATUS_STATE_PENDING : STATUS_STATE_ERROR,
                         originals_healthy
                           ? (config->svc->mezzanine_clean_apply_changes
                               ? "enabled (apply mode)"
                               : "enabled (audit mode)")
                           : "mezzanine-clean blocked by missing mezzanine locations");
  } else {
    status_report_update(report, "stage.mezzanine_clean", STATUS_STATE_SKIPPED, "MEZZANINE_CLEAN_ENABLED=false");
  }

  if(config->svc->keep_source) {
    status_report_update(report,
                         "stage.source",
                         source_healthy ? STATUS_STATE_COMPLETE : STATUS_STATE_ERROR,
                         source_healthy ? "ready to run source stage" : "source stage blocked by missing source locations");
  } else {
    status_report_update(report, "stage.source", STATUS_STATE_SKIPPED, "KEEP_SOURCE=false");
  }

  status_report_update(report,
                       "stage.profiles",
                       profiles_healthy ? STATUS_STATE_COMPLETE : STATUS_STATE_ERROR,
                       profiles_healthy ? "ready to run profile stage" : "profile stage blocked by configuration errors");

  if(config->svc->quality_check_enabled) {
    status_report_update(report,
                         "stage.quality",
                         profiles_healthy ? STATUS_STATE_PENDING : STATUS_STATE_ERROR,
                         profiles_healthy
                           ? "quality scoring enabled (runs after profile stage)"
                           : "quality scoring blocked by profile configuration errors");
  } else {
    status_report_update(report, "stage.quality", STATUS_STATE_SKIPPED, "QUALITY_CHECK_ENABLED=false");
  }

  if(toolchain_mode_supported)
    status_report_update(report, "stage.execute", STATUS_STATE_PENDING, "run `vfo run` to execute the pipeline");
  else
    status_report_update(report,
                         "stage.execute",
                         STATUS_STATE_ERROR,
                         "run blocked by TOOLCHAIN_MODE configuration");

  status_report_update(report,
                       "engine.snapshot",
                       healthy ? STATUS_STATE_COMPLETE : STATUS_STATE_ERROR,
                       healthy ? "snapshot completed successfully" : "snapshot completed with errors");

  return status_report_is_healthy(report) && healthy;
}

static bool ih_run_status_snapshot(config_t *config, const char *config_dir, bool json_output) {
  status_report_t *report = status_report_create();
  bool healthy = true;

  if(report == NULL) {
    printf("STATUS ERROR: could not allocate status report\n");
    return false;
  }

  healthy = ih_collect_status_snapshot(config, config_dir, report);

  if(json_output)
    status_report_print_json(report, stdout);
  else
    status_report_print_text(report, stdout);

  healthy = status_report_is_healthy(report) && healthy;
  status_report_free(report);
  return healthy;
}

static bool ih_run_visualize(config_t *config, const char *config_dir, bool open_report) {
  status_report_t *report = status_report_create();
  bool healthy = true;
  time_t now = time(NULL);
  struct tm local_time;
  char generated_at[128];
  char run_id[64];
  char visualize_root[IH_INPUT_BUFFER_SIZE];
  char run_dir[IH_INPUT_BUFFER_SIZE];
  char status_json_path[IH_INPUT_BUFFER_SIZE];
  char mermaid_path[IH_INPUT_BUFFER_SIZE];
  char html_path[IH_INPUT_BUFFER_SIZE];
  FILE *status_file = NULL;
  FILE *mermaid_file = NULL;
  FILE *html_file = NULL;

  if(report == NULL) {
    printf("VISUALIZE ERROR: could not allocate status report\n");
    return false;
  }

  if(localtime_r(&now, &local_time) != NULL)
    strftime(generated_at, sizeof(generated_at), "%Y-%m-%d %H:%M:%S %Z", &local_time);
  else
    snprintf(generated_at, sizeof(generated_at), "%ld", (long)now);

  ih_build_visualize_run_id(run_id, sizeof(run_id));

  if(config_dir != NULL && config_dir[0] != '\0')
    snprintf(visualize_root, sizeof(visualize_root), "%s/%s", config_dir, IH_VISUALIZE_SUBDIR);
  else
    snprintf(visualize_root, sizeof(visualize_root), "/tmp/vfo-visualizations");

  if(ih_ensure_directory_recursive(visualize_root) == false) {
    snprintf(visualize_root, sizeof(visualize_root), "/tmp/vfo-visualizations");
    if(ih_ensure_directory_recursive(visualize_root) == false) {
      printf("VISUALIZE ERROR: could not create output directory: %s\n", visualize_root);
      status_report_free(report);
      return false;
    }
    printf("VISUALIZE WARN: using fallback output directory: %s\n", visualize_root);
  }

  snprintf(run_dir, sizeof(run_dir), "%s/%s", visualize_root, run_id);
  if(ih_ensure_directory_recursive(run_dir) == false) {
    printf("VISUALIZE ERROR: could not create run directory: %s\n", run_dir);
    status_report_free(report);
    return false;
  }

  snprintf(status_json_path, sizeof(status_json_path), "%s/%s", run_dir, IH_VISUALIZE_STATUS_FILENAME);
  snprintf(mermaid_path, sizeof(mermaid_path), "%s/%s", run_dir, IH_VISUALIZE_MERMAID_FILENAME);
  snprintf(html_path, sizeof(html_path), "%s/%s", run_dir, IH_VISUALIZE_HTML_FILENAME);

  healthy = ih_collect_status_snapshot(config, config_dir, report);

  status_file = fopen(status_json_path, "w");
  if(status_file == NULL) {
    printf("VISUALIZE ERROR: could not write %s\n", status_json_path);
    status_report_free(report);
    return false;
  }
  status_report_print_json(report, status_file);
  fclose(status_file);
  status_file = NULL;

  mermaid_file = fopen(mermaid_path, "w");
  if(mermaid_file == NULL) {
    printf("VISUALIZE ERROR: could not write %s\n", mermaid_path);
    status_report_free(report);
    return false;
  }
  ih_write_visualize_mermaid(mermaid_file, report);
  fclose(mermaid_file);
  mermaid_file = NULL;

  html_file = fopen(html_path, "w");
  if(html_file == NULL) {
    printf("VISUALIZE ERROR: could not write %s\n", html_path);
    status_report_free(report);
    return false;
  }
  ih_write_visualize_html(html_file, report, generated_at, status_json_path, mermaid_path, healthy);
  fclose(html_file);
  html_file = NULL;

  printf("VISUALIZE ALERT: generated workflow visualization artifacts\n");
  printf("VISUALIZE INFO: status-json => %s\n", status_json_path);
  printf("VISUALIZE INFO: mermaid map => %s\n", mermaid_path);
  printf("VISUALIZE INFO: html report => %s\n", html_path);
  printf("VISUALIZE INFO: formal BPMN => services/vfo/docs/workflow-engine.bpmn\n");
  printf("VISUALIZE INFO: formal DMN => services/vfo/docs/workflow-decisions.dmn\n");

  if(open_report)
    ih_try_open_visualization(html_path);
  else
    printf("VISUALIZE NEXT: pass --open with `vfo visualize` to open the report in a browser\n");

  status_report_free(report);
  return healthy;
}

static bool ih_run_runtime_dependency_precheck() {
  bool healthy = true;
  ih_toolchain_mode_t toolchain_mode = IH_TOOLCHAIN_MODE_SYSTEM;
  char toolchain_mode_value[64];

  printf("PRECHECK ALERT: validating required commands before execution\n");
  if(ih_validate_toolchain_mode("PRECHECK",
                                &toolchain_mode,
                                toolchain_mode_value,
                                sizeof(toolchain_mode_value),
                                true) == false) {
    printf("PRECHECK ALERT: dependency checks failed\n");
    printf("PRECHECK INFO: run with %s=system or %s=auto until managed toolchain support lands\n",
           IH_TOOLCHAIN_MODE_ENV,
           IH_TOOLCHAIN_MODE_ENV);
    return false;
  }
  healthy = ih_check_command("PRECHECK", "ffmpeg", true) && healthy;
  healthy = ih_check_command("PRECHECK", "ffprobe", true) && healthy;
  healthy = ih_check_command("PRECHECK", "mkvmerge", true) && healthy;
  healthy = ih_check_command("PRECHECK", "dovi_tool", false) && healthy;

  if(healthy) {
    printf("PRECHECK ALERT: dependency checks passed\n");
    return true;
  }

  printf("PRECHECK ALERT: dependency checks failed\n");
  printf("PRECHECK INFO: run `vfo doctor` for full diagnostics after installing missing commands\n");
  return false;
}

static bool ih_run_mezzanine_clean_dependency_precheck() {
  bool healthy = true;
  ih_toolchain_mode_t toolchain_mode = IH_TOOLCHAIN_MODE_SYSTEM;
  char toolchain_mode_value[64];

  printf("PRECHECK ALERT: validating mezzanine-clean dependencies\n");
  if(ih_validate_toolchain_mode("PRECHECK",
                                &toolchain_mode,
                                toolchain_mode_value,
                                sizeof(toolchain_mode_value),
                                true) == false) {
    printf("PRECHECK ALERT: mezzanine-clean dependency checks failed\n");
    return false;
  }
  healthy = ih_check_command("PRECHECK", "ffprobe", true) && healthy;

  if(healthy) {
    printf("PRECHECK ALERT: mezzanine-clean dependency checks passed\n");
    return true;
  }

  printf("PRECHECK ALERT: mezzanine-clean dependency checks failed\n");
  return false;
}

static bool ih_pipeline_work_requested(arguments_t *arguments, config_t *config) {
  bool alias_targets_requested = uw_get_count(config->uw_head) > 0;

  if(arguments->auto_detected)
    return true;

  if(arguments->run_detected)
    return true;

  if(arguments->mezzanine_detected && arguments->revert_detected == false)
    return true;

  if(arguments->source_detected && arguments->wipe_detected == false)
    return true;

  if(arguments->profiles_detected && arguments->wipe_detected == false)
    return true;

  if(alias_targets_requested && arguments->wipe_detected == false)
    return true;

  return false;
}

static bool ih_should_use_lenient_config_validation(arguments_t *arguments) {
  if(arguments == NULL)
    return false;

  return arguments->doctor_detected
      || arguments->show_detected
      || arguments->status_detected
      || arguments->status_json_detected
      || arguments->visualize_detected;
}

static void ih_execute_all_profiles(config_t *config) {
  aliases_t *profiles = NULL;
  for(int i = 0; i < ca_get_count(config->ca_head); i++) {
    aliases_t *profile = alias_create_new_struct(config, ca_get_a_node_from_count(config->ca_head, i));
    alias_insert_at_head(&profiles, profile);
  }

  if(config->svc->keep_source == true)
    p_source_to_profiles(profiles);
  else
    p_mezzanine_to_profiles(profiles);

  free(profiles);
  profiles = NULL;
}

static char** ih_resolve_locations(char *locations_csv, char *fallback, int *count) {
  char **locations = NULL;

  *count = 0;
  if(utils_string_is_empty_or_spaces(locations_csv) == false)
    locations = utils_split_semicolon_list(locations_csv, count);

  if(*count == 0 && utils_string_is_empty_or_spaces(fallback) == false) {
    locations = malloc(sizeof(char*));
    locations[0] = strdup(fallback);
    *count = 1;
  }

  return locations;
}

static const char* ih_get_custom_folder_name_for_type(cf_node_t *cf_head,
                                                       const char *folder_type,
                                                       const char *fallback) {
  cf_node_t *current = cf_head;
  while(current != NULL) {
    if(current->folder_type != NULL
       && current->folder_name != NULL
       && strcasecmp(current->folder_type, folder_type) == 0
       && utils_string_is_empty_or_spaces(current->folder_name) == false) {
      return current->folder_name;
    }
    current = current->next;
  }
  return fallback;
}

static bool ih_execute_mezzanine_clean_for_all_roots(config_t *config,
                                                     bool force_enabled,
                                                     bool force_apply_changes) {
  mezzanine_clean_options_t options;
  mezzanine_clean_report_t aggregate_report;
  int mezzanine_root_count = 0;
  char **mezzanine_roots = NULL;
  bool healthy = true;
  bool should_run = force_enabled || config->svc->mezzanine_clean_enabled;

  if(should_run == false) {
    printf("MEZZANINE CLEAN ALERT: disabled (MEZZANINE_CLEAN_ENABLED=false)\n");
    return true;
  }

  mezzanine_roots = ih_resolve_locations(config->svc->original_locations,
                                         config->svc->original_location,
                                         &mezzanine_root_count);
  if(mezzanine_root_count == 0) {
    printf("MEZZANINE CLEAN ERROR: no mezzanine roots configured\n");
    return false;
  }

  mc_options_init(&options);
  mc_report_init(&aggregate_report);
  options.apply_changes = force_apply_changes ? true : config->svc->mezzanine_clean_apply_changes;
  options.append_media_tags = config->svc->mezzanine_clean_append_media_tags;
  options.strict_quality_gate = config->svc->mezzanine_clean_strict_quality_gate;
  options.movies_folder_name = ih_get_custom_folder_name_for_type(config->cf_head, "films", "Movies");
  options.tv_folder_name = ih_get_custom_folder_name_for_type(config->cf_head, "tv", "TV Shows");

  for(int i = 0; i < mezzanine_root_count; i++) {
    mezzanine_clean_report_t root_report;
    bool root_ok = false;

    mc_report_init(&root_report);
    printf("MEZZANINE CLEAN ALERT: processing root %i/%i => %s\n",
           i + 1,
           mezzanine_root_count,
           mezzanine_roots[i]);

    root_ok = mc_run_for_root(mezzanine_roots[i], &options, &root_report);
    mc_report_add(&aggregate_report, &root_report);
    if(root_ok == false)
      healthy = false;
  }

  utils_free_string_array(mezzanine_roots, mezzanine_root_count);

  printf("MEZZANINE CLEAN OVERALL: roots=%i scanned=%i planned_moves=%i applied_moves=%i warnings=%i errors=%i skipped=%i\n",
         mezzanine_root_count,
         aggregate_report.scanned_files,
         aggregate_report.planned_moves,
         aggregate_report.applied_moves,
         aggregate_report.warnings,
         aggregate_report.errors,
         aggregate_report.skipped);

  return healthy && aggregate_report.errors == 0;
}

static void ih_execute_original_stage_for_all_roots(config_t *config) {
  char *original_location_before = config->svc->original_location;
  int original_root_count = 0;
  char **original_roots = ih_resolve_locations(config->svc->original_locations,
                                               config->svc->original_location,
                                               &original_root_count);

  for(int i = 0; i < original_root_count; i++) {
    config->svc->original_location = original_roots[i];
    printf("RUN ALERT: mezzanine stage root %i/%i => %s\n", i + 1, original_root_count, original_roots[i]);
    original_t *original = original_create_new_struct(config);
    m_mezzanine(original);
    free(original);
    original = NULL;
  }

  config->svc->original_location = original_location_before;
  utils_free_string_array(original_roots, original_root_count);
}

static void ih_execute_revert_stage_for_all_roots(config_t *config) {
  char *original_location_before = config->svc->original_location;
  int original_root_count = 0;
  char **original_roots = ih_resolve_locations(config->svc->original_locations,
                                               config->svc->original_location,
                                               &original_root_count);

  for(int i = 0; i < original_root_count; i++) {
    config->svc->original_location = original_roots[i];
    printf("RUN ALERT: revert stage root %i/%i => %s\n", i + 1, original_root_count, original_roots[i]);
    original_t *original = original_create_new_struct(config);
    m_revert_to_mezzanine(original);
    free(original);
    original = NULL;
  }

  config->svc->original_location = original_location_before;
  utils_free_string_array(original_roots, original_root_count);
}

static void ih_execute_source_stage_for_all_original_roots(config_t *config) {
  char *original_location_before = config->svc->original_location;
  int original_root_count = 0;
  char **original_roots = ih_resolve_locations(config->svc->original_locations,
                                               config->svc->original_location,
                                               &original_root_count);

  for(int i = 0; i < original_root_count; i++) {
    config->svc->original_location = original_roots[i];
    printf("RUN ALERT: source stage root %i/%i => %s\n", i + 1, original_root_count, original_roots[i]);
    source_t *source = source_create_new_struct(config);
    s_mezzanine_to_source(source);
    free(source);
    source = NULL;
  }

  config->svc->original_location = original_location_before;
  utils_free_string_array(original_roots, original_root_count);
}

static bool ih_execute_quality_scoring_stage(config_t *config, status_report_t *run_report) {
  quality_scoring_options_t options;
  quality_scoring_report_t report;
  quality_profile_result_t *profile_cursor = NULL;
  bool valid_reference_mode = false;
  bool stage_ok = true;

  if(config->svc->quality_check_enabled == false) {
    if(run_report != NULL)
      ih_status_update_with_log(run_report, "stage.quality", STATUS_STATE_SKIPPED, "QUALITY_CHECK_ENABLED=false");
    return true;
  }

  quality_scoring_options_init(&options);
  quality_scoring_report_init(&report);

  options.enabled = true;
  options.include_vmaf = config->svc->quality_check_include_vmaf;
  options.strict_gate = config->svc->quality_check_strict_gate;
  options.reference_mode = quality_reference_mode_from_string(config->svc->quality_check_reference_layer,
                                                              &valid_reference_mode);
  if(valid_reference_mode == false) {
    printf("QUALITY ERROR: invalid QUALITY_CHECK_REFERENCE_LAYER value: %s\n",
           config->svc->quality_check_reference_layer);
    quality_scoring_report_free(&report);
    if(run_report != NULL)
      ih_status_update_with_log(run_report, "stage.quality", STATUS_STATE_ERROR, "invalid QUALITY_CHECK_REFERENCE_LAYER");
    return false;
  }

  options.min_psnr = config->svc->quality_check_min_psnr;
  options.min_ssim = config->svc->quality_check_min_ssim;
  options.min_vmaf = config->svc->quality_check_min_vmaf;
  options.max_files_per_profile = config->svc->quality_check_max_files_per_profile;

  if(run_report != NULL)
    ih_status_update_with_log(run_report, "stage.quality", STATUS_STATE_IN_PROGRESS, "running quality scoring stage");

  stage_ok = quality_scoring_run(config->ca_head,
                                 config->svc->source_location,
                                 config->svc->original_location,
                                 config->svc->keep_source,
                                 &options,
                                 &report);

  profile_cursor = report.profiles;
  while(profile_cursor != NULL) {
    char component[320];
    char detail[1024];
    status_state_t state = STATUS_STATE_COMPLETE;

    if(profile_cursor->metric_errors > 0 || profile_cursor->missing_reference > 0)
      state = STATUS_STATE_ERROR;
    else if(profile_cursor->files_failed_thresholds > 0)
      state = config->svc->quality_check_strict_gate ? STATUS_STATE_ERROR : STATUS_STATE_SKIPPED;

    snprintf(component, sizeof(component), "profile.%s.quality", profile_cursor->profile_name);
    snprintf(detail,
             sizeof(detail),
             "scored=%i failed=%i missing_ref=%i metric_errors=%i avg_psnr=%.3f avg_ssim=%.6f avg_vmaf=%.3f",
             profile_cursor->files_scored,
             profile_cursor->files_failed_thresholds,
             profile_cursor->missing_reference,
             profile_cursor->metric_errors,
             profile_cursor->avg_psnr,
             profile_cursor->avg_ssim,
             profile_cursor->avg_vmaf);

    if(run_report != NULL)
      ih_status_update_with_log(run_report, component, state, detail);

    profile_cursor = profile_cursor->next;
  }

  if(run_report != NULL) {
    char stage_detail[1024];
    snprintf(stage_detail,
             sizeof(stage_detail),
             "profiles=%i scored=%i failed=%i missing_ref=%i metric_errors=%i avg_psnr=%.3f avg_ssim=%.6f avg_vmaf=%.3f",
             report.profiles_scored,
             report.files_scored,
             report.files_failed_thresholds,
             report.missing_reference,
             report.metric_errors,
             report.avg_psnr,
             report.avg_ssim,
             report.avg_vmaf);
    ih_status_update_with_log(run_report,
                              "stage.quality",
                              stage_ok ? STATUS_STATE_COMPLETE : STATUS_STATE_ERROR,
                              stage_detail);
  }

  quality_scoring_report_free(&report);
  return stage_ok;
}

static int ih_read_env_int(const char *name,
                           int default_value,
                           int minimum_value,
                           bool allow_zero) {
  const char *raw = getenv(name);
  char *end_ptr = NULL;
  long parsed = 0;

  if(raw == NULL || raw[0] == '\0')
    return default_value;

  errno = 0;
  parsed = strtol(raw, &end_ptr, 10);
  if(errno != 0 || end_ptr == raw || *end_ptr != '\0' || parsed > INT_MAX) {
    printf("AUTO WARN: invalid %s=%s, using default %i\n", name, raw, default_value);
    return default_value;
  }

  if(allow_zero) {
    if(parsed < 0) {
      printf("AUTO WARN: invalid %s=%s, using default %i\n", name, raw, default_value);
      return default_value;
    }
  } else if(parsed < minimum_value) {
    printf("AUTO WARN: invalid %s=%s, using default %i\n", name, raw, default_value);
    return default_value;
  }

  return (int)parsed;
}

static bool ih_execute_auto(config_t *config) {
  int interval_seconds = ih_read_env_int(IH_AUTO_INTERVAL_SECONDS_ENV, 60, 1, false);
  int max_iterations = ih_read_env_int(IH_AUTO_MAX_ITERATIONS_ENV, 0, 1, true);
  int completed_runs = 0;
  int failed_runs = 0;

  if(setenv(IH_ASSUME_YES_ENV, "1", 1) != 0)
    printf("AUTO WARN: could not set %s=1, some prompts may still appear\n", IH_ASSUME_YES_ENV);

  printf("AUTO ALERT: initiating auto mode\n");
  printf("AUTO INFO: %s=%i\n", IH_AUTO_INTERVAL_SECONDS_ENV, interval_seconds);
  if(max_iterations == 0)
    printf("AUTO INFO: %s=0 (infinite loop; stop with Ctrl+C)\n", IH_AUTO_MAX_ITERATIONS_ENV);
  else
    printf("AUTO INFO: %s=%i\n", IH_AUTO_MAX_ITERATIONS_ENV, max_iterations);

  while(max_iterations == 0 || completed_runs < max_iterations) {
    bool run_success = false;

    completed_runs++;
    printf("AUTO ALERT: cycle %i started\n", completed_runs);
    run_success = ih_execute_default_run(config);
    if(run_success) {
      printf("AUTO ALERT: cycle %i completed successfully\n", completed_runs);
    } else {
      failed_runs++;
      printf("AUTO WARN: cycle %i completed with failures; auto mode will continue\n", completed_runs);
    }

    if(max_iterations > 0 && completed_runs >= max_iterations)
      break;

    printf("AUTO ALERT: sleeping %i second(s) before next cycle\n", interval_seconds);
    sleep((unsigned int)interval_seconds);
  }

  printf("AUTO ALERT: auto mode stopped after %i cycle(s), failures=%i\n", completed_runs, failed_runs);
  return failed_runs == 0;
}

static bool ih_execute_default_run(config_t *config) {
  status_report_t *run_report = status_report_create();
  bool mezzanine_clean_ok = true;
  bool quality_stage_ok = true;

  printf("RUN ALERT: initiating default pipeline\n");
  if(run_report != NULL)
    ih_status_update_with_log(run_report, "engine.run", STATUS_STATE_IN_PROGRESS, "default pipeline started");

  if(config->svc->mezzanine_clean_enabled) {
    if(run_report != NULL)
      ih_status_update_with_log(run_report, "stage.mezzanine_clean", STATUS_STATE_IN_PROGRESS, "running mezzanine-clean hygiene stage");
    mezzanine_clean_ok = ih_execute_mezzanine_clean_for_all_roots(config, false, false);
    if(run_report != NULL) {
      ih_status_update_with_log(run_report,
                                "stage.mezzanine_clean",
                                mezzanine_clean_ok ? STATUS_STATE_COMPLETE : STATUS_STATE_ERROR,
                                mezzanine_clean_ok ? "mezzanine-clean stage completed" : "mezzanine-clean stage failed");
    }
    if(mezzanine_clean_ok == false) {
      if(run_report != NULL) {
        ih_status_update_with_log(run_report, "engine.run", STATUS_STATE_ERROR, "aborting run after mezzanine-clean failure");
        status_report_print_text(run_report, stdout);
        status_report_free(run_report);
      }
      printf("RUN ERROR: mezzanine-clean failed. Disable MEZZANINE_CLEAN_ENABLED or fix reported issues.\n");
      return false;
    }
  } else if(run_report != NULL) {
    ih_status_update_with_log(run_report, "stage.mezzanine_clean", STATUS_STATE_SKIPPED, "MEZZANINE_CLEAN_ENABLED=false");
  }

  if(run_report != NULL)
    ih_status_update_with_log(run_report, "stage.mezzanine", STATUS_STATE_IN_PROGRESS, "processing mezzanine stage");
  ih_execute_original_stage_for_all_roots(config);
  if(run_report != NULL)
    ih_status_update_with_log(run_report, "stage.mezzanine", STATUS_STATE_COMPLETE, "mezzanine stage completed");

  if(config->svc->keep_source == true) {
    if(run_report != NULL)
      ih_status_update_with_log(run_report, "stage.source", STATUS_STATE_IN_PROGRESS, "processing source stage");
    ih_execute_source_stage_for_all_original_roots(config);
    if(run_report != NULL)
      ih_status_update_with_log(run_report, "stage.source", STATUS_STATE_COMPLETE, "source stage completed");
  } else if(run_report != NULL) {
    ih_status_update_with_log(run_report, "stage.source", STATUS_STATE_SKIPPED, "KEEP_SOURCE=false");
  }

  if(run_report != NULL)
    ih_status_update_with_log(run_report, "stage.profiles", STATUS_STATE_IN_PROGRESS, "processing profile stage");
  ih_execute_all_profiles(config);
  if(run_report != NULL)
    ih_status_update_with_log(run_report, "stage.profiles", STATUS_STATE_COMPLETE, "profile stage completed");

  quality_stage_ok = ih_execute_quality_scoring_stage(config, run_report);
  if(quality_stage_ok == false) {
    if(run_report != NULL) {
      ih_status_update_with_log(run_report, "engine.run", STATUS_STATE_ERROR, "aborting run after quality stage failure");
      status_report_print_text(run_report, stdout);
      status_report_free(run_report);
    }
    printf("RUN ERROR: quality stage failed. Review QUALITY_CHECK_* settings and reported issues.\n");
    return false;
  }

  if(run_report != NULL) {
    ih_status_update_with_log(run_report, "engine.run", STATUS_STATE_COMPLETE, "default pipeline completed");
    status_report_print_text(run_report, stdout);
    status_report_free(run_report);
  }
  printf("RUN ALERT: default pipeline completed successfully\n");
  return true;
}

static void ih_copy_string(char *destination, size_t destination_size, const char *source) {
  if(destination == NULL || destination_size == 0)
    return;

  if(source == NULL) {
    destination[0] = '\0';
    return;
  }

  snprintf(destination, destination_size, "%s", source);
}

static void ih_strip_newline(char *value) {
  size_t length = 0;
  if(value == NULL)
    return;

  length = strlen(value);
  while(length > 0 && (value[length - 1] == '\n' || value[length - 1] == '\r')) {
    value[length - 1] = '\0';
    length--;
  }
}

static bool ih_is_blank(const char *value) {
  size_t i = 0;
  if(value == NULL)
    return true;

  if(value[0] == '\0')
    return true;

  for(i = 0; value[i] != '\0'; i++) {
    if(!isspace((unsigned char)value[i]))
      return false;
  }

  return true;
}

static void ih_trim_spaces_in_place(char *value) {
  size_t start = 0;
  size_t end = 0;
  size_t i = 0;

  if(value == NULL)
    return;

  if(value[0] == '\0')
    return;

  while(value[start] != '\0' && isspace((unsigned char)value[start])) {
    start++;
  }

  end = strlen(value);
  while(end > start && isspace((unsigned char)value[end - 1])) {
    end--;
  }

  if(start > 0) {
    for(i = start; i < end; i++) {
      value[i - start] = value[i];
    }
    value[end - start] = '\0';
  } else {
    value[end] = '\0';
  }
}

static bool ih_path_has_config_file(const char *config_dir) {
  char *config_path = NULL;
  bool exists = false;

  if(config_dir == NULL || config_dir[0] == '\0')
    return false;

  config_path = utils_combine_to_full_path(config_dir, IH_CONFIG_FILENAME);
  exists = utils_does_file_exist(config_path);
  free(config_path);
  return exists;
}

static bool ih_read_env_config_dir(char *output, size_t output_size) {
  const char *env_value = getenv(IH_CONFIG_DIR_ENV);

  if(output == NULL || output_size == 0)
    return false;

  output[0] = '\0';
  if(env_value == NULL)
    return false;

  ih_copy_string(output, output_size, env_value);
  ih_trim_spaces_in_place(output);
  return output[0] != '\0';
}

static bool ih_build_user_config_dir(char *output, size_t output_size) {
  const char *xdg_config_home = getenv("XDG_CONFIG_HOME");
  const char *home = getenv("HOME");
  int written = 0;

  if(output == NULL || output_size == 0)
    return false;

  output[0] = '\0';
  if(xdg_config_home != NULL && xdg_config_home[0] != '\0')
    written = snprintf(output, output_size, "%s/vfo", xdg_config_home);
  else if(home != NULL && home[0] != '\0')
    written = snprintf(output, output_size, "%s/.config/vfo", home);
  else
    return false;

  return written >= 0 && (size_t)written < output_size;
}

static void ih_resolve_config_dir(char *output, size_t output_size, bool wizard_mode) {
  char env_config_dir[IH_INPUT_BUFFER_SIZE];
  char user_config_dir[IH_INPUT_BUFFER_SIZE];
  bool has_env = false;
  bool has_user_dir = false;

  if(output == NULL || output_size == 0)
    return;

  output[0] = '\0';
  has_env = ih_read_env_config_dir(env_config_dir, sizeof(env_config_dir));
  has_user_dir = ih_build_user_config_dir(user_config_dir, sizeof(user_config_dir));

  if(has_env) {
    ih_copy_string(output, output_size, env_config_dir);
    return;
  }

  if(wizard_mode) {
    if(has_user_dir) {
      ih_copy_string(output, output_size, user_config_dir);
      return;
    }

    ih_copy_string(output, output_size, IH_LEGACY_CONFIG_DIR);
    return;
  }

  if(has_user_dir && ih_path_has_config_file(user_config_dir)) {
    ih_copy_string(output, output_size, user_config_dir);
    return;
  }

  if(ih_path_has_config_file(IH_LEGACY_CONFIG_DIR)) {
    ih_copy_string(output, output_size, IH_LEGACY_CONFIG_DIR);
    return;
  }

  if(has_user_dir) {
    ih_copy_string(output, output_size, user_config_dir);
    return;
  }

  ih_copy_string(output, output_size, IH_LEGACY_CONFIG_DIR);
}

#ifdef TESTING
void ih_resolve_config_dir_for_test(char *output, size_t output_size, bool wizard_mode) {
  ih_resolve_config_dir(output, output_size, wizard_mode);
}
#endif

static bool ih_prompt_line(const char *prompt,
                           const char *default_value,
                           char *buffer,
                           size_t buffer_size,
                           bool required) {
  while(true) {
    if(default_value != NULL && default_value[0] != '\0')
      printf("%s [%s]: ", prompt, default_value);
    else
      printf("%s: ", prompt);

    if(fgets(buffer, (int)buffer_size, stdin) == NULL) {
      printf("\nWIZARD ERROR: input stream ended unexpectedly\n");
      return false;
    }

    ih_strip_newline(buffer);
    ih_trim_spaces_in_place(buffer);

    if(ih_is_blank(buffer) && default_value != NULL && default_value[0] != '\0')
      ih_copy_string(buffer, buffer_size, default_value);

    if(required && ih_is_blank(buffer)) {
      printf("WIZARD INFO: value required\n");
      continue;
    }

    return true;
  }
}

static bool ih_prompt_bool(const char *prompt, bool default_value, bool *result) {
  char input[IH_INPUT_BUFFER_SIZE];
  const char *default_word = default_value ? "true" : "false";

  while(true) {
    if(!ih_prompt_line(prompt, default_word, input, sizeof(input), true))
      return false;

    if(strcasecmp(input, "true") == 0 || strcasecmp(input, "t") == 0 ||
       strcasecmp(input, "yes") == 0 || strcasecmp(input, "y") == 0 ||
       strcmp(input, "1") == 0) {
      *result = true;
      return true;
    }

    if(strcasecmp(input, "false") == 0 || strcasecmp(input, "f") == 0 ||
       strcasecmp(input, "no") == 0 || strcasecmp(input, "n") == 0 ||
       strcmp(input, "0") == 0) {
      *result = false;
      return true;
    }

    printf("WIZARD INFO: please answer true/false, yes/no, y/n, or 1/0\n");
  }
}

static bool ih_prompt_timecode(const char *prompt, const char *default_value, char *output, size_t output_size) {
  while(true) {
    if(!ih_prompt_line(prompt, default_value, output, output_size, true))
      return false;

    if(utils_string_is_ffmpeg_timecode_compliant(output))
      return true;

    printf("WIZARD INFO: invalid timecode. Example: 00:00:20\n");
  }
}

static bool ih_stdout_is_tty(void) {
  return isatty(STDOUT_FILENO) == 1;
}

static void ih_wizard_print_banner(void) {
  if(ih_stdout_is_tty()) {
    printf(CYAN "==============================================================\n" NO_COLOR);
    printf(CYAN "                    vfo onboarding wizard                    \n" NO_COLOR);
    printf(CYAN "==============================================================\n" NO_COLOR);
    return;
  }

  printf("==============================================================\n");
  printf("vfo onboarding wizard\n");
  printf("==============================================================\n");
}

static void ih_wizard_print_step(int current, int total, const char *title, const char *detail) {
  if(ih_stdout_is_tty())
    printf(BLUE "\n[step %i/%i] %s" NO_COLOR "\n", current, total, title);
  else
    printf("\n[step %i/%i] %s\n", current, total, title);

  if(detail != NULL && detail[0] != '\0')
    printf("WIZARD INFO: %s\n", detail);
}

static bool ih_wizard_parse_mode(const char *input, bool *advanced_mode) {
  char normalized[IH_INPUT_BUFFER_SIZE];

  if(input == NULL || advanced_mode == NULL)
    return false;

  ih_copy_string(normalized, sizeof(normalized), input);
  ih_trim_spaces_in_place(normalized);
  utils_lowercase_string(normalized);

  if(strcmp(normalized, "quickstart") == 0 || strcmp(normalized, "quick") == 0 || strcmp(normalized, "q") == 0) {
    *advanced_mode = false;
    return true;
  }

  if(strcmp(normalized, "advanced") == 0 || strcmp(normalized, "adv") == 0 || strcmp(normalized, "a") == 0) {
    *advanced_mode = true;
    return true;
  }

  return false;
}

static bool ih_wizard_prompt_mode(bool *advanced_mode) {
  char mode_input[IH_INPUT_BUFFER_SIZE];

  while(true) {
    if(!ih_prompt_line("Onboarding mode (quickstart|advanced)", "quickstart", mode_input, sizeof(mode_input), true))
      return false;
    if(ih_wizard_parse_mode(mode_input, advanced_mode))
      return true;
    printf("WIZARD INFO: choose quickstart or advanced\n");
  }
}

static bool ih_extract_parent_dir(const char *path, char *parent, size_t parent_size) {
  char copy[IH_INPUT_BUFFER_SIZE];
  char *slash = NULL;

  if(path == NULL || parent == NULL || parent_size == 0)
    return false;

  ih_copy_string(copy, sizeof(copy), path);
  ih_trim_spaces_in_place(copy);
  if(copy[0] == '\0')
    return false;

  slash = strrchr(copy, '/');
  if(slash == NULL) {
    ih_copy_string(parent, parent_size, ".");
    return true;
  }

  if(slash == copy) {
    ih_copy_string(parent, parent_size, "/");
    return true;
  }

  *slash = '\0';
  ih_copy_string(parent, parent_size, copy);
  return true;
}

static bool ih_wizard_preflight(const char *config_dir) {
  bool healthy = true;
  char parent_dir[IH_INPUT_BUFFER_SIZE];
  ih_toolchain_mode_t toolchain_mode = IH_TOOLCHAIN_MODE_SYSTEM;
  char toolchain_mode_value[64];

  printf("WIZARD PREFLIGHT: validating command dependencies\n");
  if(ih_validate_toolchain_mode("WIZARD PREFLIGHT",
                                &toolchain_mode,
                                toolchain_mode_value,
                                sizeof(toolchain_mode_value),
                                true) == false)
    return false;
  healthy = ih_check_command("WIZARD PREFLIGHT", "ffmpeg", true) && healthy;
  healthy = ih_check_command("WIZARD PREFLIGHT", "ffprobe", true) && healthy;
  healthy = ih_check_command("WIZARD PREFLIGHT", "mkvmerge", true) && healthy;
  healthy = ih_check_command("WIZARD PREFLIGHT", "dovi_tool", false) && healthy;

  if(ih_ffmpeg_filter_available("libvmaf"))
    printf("WIZARD PREFLIGHT INFO: ffmpeg libvmaf filter is available\n");
  else
    printf("WIZARD PREFLIGHT INFO: ffmpeg libvmaf filter not detected (optional)\n");

  if(config_dir == NULL || config_dir[0] == '\0') {
    printf("WIZARD PREFLIGHT ERROR: config directory is empty\n");
    return false;
  }

  if(utils_does_folder_exist((char *)config_dir)) {
    if(access(config_dir, W_OK) != 0) {
      printf("WIZARD PREFLIGHT ERROR: config directory is not writable: %s\n", config_dir);
      printf("WIZARD PREFLIGHT INFO: run wizard with sudo or choose a writable VFO_CONFIG_DIR\n");
      healthy = false;
    } else {
      printf("WIZARD PREFLIGHT OK: config directory writable: %s\n", config_dir);
    }
  } else {
    if(!ih_extract_parent_dir(config_dir, parent_dir, sizeof(parent_dir))) {
      printf("WIZARD PREFLIGHT ERROR: could not determine parent directory for %s\n", config_dir);
      healthy = false;
    } else if(access(parent_dir, W_OK) != 0) {
      printf("WIZARD PREFLIGHT ERROR: parent directory is not writable: %s\n", parent_dir);
      printf("WIZARD PREFLIGHT INFO: run wizard with sudo or choose a writable VFO_CONFIG_DIR\n");
      healthy = false;
    } else {
      printf("WIZARD PREFLIGHT OK: config directory will be created: %s\n", config_dir);
    }
  }

  return healthy;
}

static bool ih_copy_file(const char *source_path, const char *dest_path) {
  FILE *source = NULL;
  FILE *dest = NULL;
  char buffer[8192];
  size_t bytes_read = 0;

  if(source_path == NULL || dest_path == NULL)
    return false;

  source = fopen(source_path, "rb");
  if(source == NULL)
    return false;

  dest = fopen(dest_path, "wb");
  if(dest == NULL) {
    fclose(source);
    return false;
  }

  while((bytes_read = fread(buffer, 1, sizeof(buffer), source)) > 0) {
    if(fwrite(buffer, 1, bytes_read, dest) != bytes_read) {
      fclose(dest);
      fclose(source);
      return false;
    }
  }

  fclose(dest);
  fclose(source);
  return true;
}

static bool ih_create_config_backup_if_present(const char *config_path,
                                               char *backup_path,
                                               size_t backup_path_size) {
  time_t now = 0;
  struct tm time_info;
  int written = 0;

  if(backup_path != NULL && backup_path_size > 0)
    backup_path[0] = '\0';

  if(config_path == NULL || !utils_does_file_exist((char *)config_path))
    return true;

  now = time(NULL);
  if(localtime_r(&now, &time_info) == NULL)
    return false;

  written = snprintf(backup_path,
                     backup_path_size,
                     "%s.bak.%04i%02i%02i_%02i%02i%02i",
                     config_path,
                     time_info.tm_year + 1900,
                     time_info.tm_mon + 1,
                     time_info.tm_mday,
                     time_info.tm_hour,
                     time_info.tm_min,
                     time_info.tm_sec);
  if(written < 0 || (size_t)written >= backup_path_size)
    return false;

  return ih_copy_file(config_path, backup_path);
}

static int ih_suggest_max_usage_pct_for_location(const char *location) {
  unsigned long long total_bytes = 0ULL;
  unsigned long long free_bytes = 0ULL;
  (void)free_bytes;

  if(location == NULL)
    return 95;
  if(utils_get_path_space_bytes((char *)location, &total_bytes, &free_bytes) == false)
    return 95;

  if(total_bytes < (unsigned long long)(2ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL))
    return 85;
  if(total_bytes < (unsigned long long)(8ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL))
    return 90;
  return 95;
}

static bool ih_first_location_from_list(const char *locations_csv, char *output, size_t output_size) {
  int count = 0;
  char **locations = NULL;

  if(locations_csv == NULL)
    return false;

  locations = utils_split_semicolon_list((char *)locations_csv, &count);
  if(count == 0) {
    utils_free_string_array(locations, count);
    return false;
  }

  ih_copy_string(output, output_size, locations[0]);
  utils_free_string_array(locations, count);
  return true;
}

static void ih_build_default_cap_list(const char *locations_csv, char *output, size_t output_size) {
  int count = 0;
  char **locations = NULL;
  output[0] = '\0';

  if(locations_csv == NULL)
    return;

  locations = utils_split_semicolon_list((char *)locations_csv, &count);
  for(int i = 0; i < count; i++) {
    char cap_value[16];
    int suggested = ih_suggest_max_usage_pct_for_location(locations[i]);
    snprintf(cap_value, sizeof(cap_value), "%i", suggested);
    if(i > 0)
      strncat(output, ";", output_size - strlen(output) - 1);
    strncat(output, cap_value, output_size - strlen(output) - 1);
  }
  utils_free_string_array(locations, count);
}

static void ih_sanitize_alias_name(char *alias_name, size_t alias_name_size) {
  size_t i = 0;
  size_t write_index = 0;
  char sanitized[IH_INPUT_BUFFER_SIZE];

  if(alias_name == NULL || alias_name_size == 0)
    return;

  for(i = 0; alias_name[i] != '\0' && write_index < sizeof(sanitized) - 1; i++) {
    unsigned char character = (unsigned char)alias_name[i];
    if(isalnum(character)) {
      sanitized[write_index++] = (char)tolower(character);
      continue;
    }

    if(character == '_' || character == '-' || isspace(character) || character == '.') {
      if(write_index == 0 || sanitized[write_index - 1] == '_')
        continue;
      sanitized[write_index++] = '_';
      continue;
    }
  }

  while(write_index > 0 && sanitized[write_index - 1] == '_') {
    write_index--;
  }

  sanitized[write_index] = '\0';

  if(write_index == 0)
    ih_copy_string(sanitized, sizeof(sanitized), "default_profile");

  ih_copy_string(alias_name, alias_name_size, sanitized);
}

static void ih_uppercase_in_place(char *value) {
  size_t i = 0;

  if(value == NULL)
    return;

  for(i = 0; value[i] != '\0'; i++) {
    value[i] = (char)toupper((unsigned char)value[i]);
  }
}

static int ih_cs_count(cs_node_t *cs_head) {
  int count = 0;
  cs_node_t *temporary = cs_head;
  while(temporary != NULL) {
    count++;
    temporary = temporary->next;
  }
  return count;
}

static void ih_print_preview(const char *label, const char *value) {
  char preview[IH_PREVIEW_LIMIT + 1];
  size_t length = 0;

  if(value == NULL || value[0] == '\0') {
    printf("%s<empty>\n", label);
    return;
  }

  length = strlen(value);
  if(length <= IH_PREVIEW_LIMIT) {
    printf("%s%s\n", label, value);
    return;
  }

  snprintf(preview, sizeof(preview), "%.*s", IH_PREVIEW_LIMIT, value);
  printf("%s%s...\n", label, preview);
}

static bool ih_create_config_dir_if_needed(const char *config_dir) {
  char tmp[IH_INPUT_BUFFER_SIZE];
  size_t length = 0;

  if(utils_does_folder_exist((char *)config_dir))
    return true;

  if(config_dir == NULL || config_dir[0] == '\0')
    return false;

  ih_copy_string(tmp, sizeof(tmp), config_dir);
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

static bool ih_string_ends_with(const char *value, const char *suffix) {
  size_t value_length = 0;
  size_t suffix_length = 0;

  if(value == NULL || suffix == NULL)
    return false;

  value_length = strlen(value);
  suffix_length = strlen(suffix);
  if(suffix_length > value_length)
    return false;

  return strcmp(value + value_length - suffix_length, suffix) == 0;
}

static bool ih_join_root_and_leaf(const char *root, const char *leaf, char *output, size_t output_size) {
  char root_copy[IH_INPUT_BUFFER_SIZE];
  size_t root_length = 0;
  int written = 0;

  if(root == NULL || output == NULL || output_size == 0)
    return false;

  ih_copy_string(root_copy, sizeof(root_copy), root);
  ih_trim_spaces_in_place(root_copy);
  if(root_copy[0] == '\0')
    return false;

  root_length = strlen(root_copy);
  while(root_length > 0 && root_copy[root_length - 1] == '/') {
    root_copy[root_length - 1] = '\0';
    root_length--;
  }

  if(leaf == NULL || leaf[0] == '\0')
    written = snprintf(output, output_size, "%s", root_copy);
  else
    written = snprintf(output, output_size, "%s/%s", root_copy, leaf);

  if(written < 0 || (size_t)written >= output_size)
    return false;

  return true;
}

static void ih_extract_last_path_component(const char *path, char *output, size_t output_size) {
  const char *last_slash = NULL;

  if(output == NULL || output_size == 0)
    return;

  output[0] = '\0';
  if(path == NULL)
    return;

  last_slash = strrchr(path, '/');
  if(last_slash != NULL && last_slash[1] != '\0') {
    ih_copy_string(output, output_size, last_slash + 1);
    return;
  }

  ih_copy_string(output, output_size, path);
}

static bool ih_extract_assignment_key(const char *line, char *key, size_t key_size) {
  const char *equals = NULL;
  size_t copy_length = 0;

  if(line == NULL || key == NULL || key_size == 0)
    return false;

  equals = strchr(line, '=');
  if(equals == NULL || equals == line)
    return false;

  copy_length = (size_t)(equals - line);
  if(copy_length >= key_size)
    copy_length = key_size - 1;

  memcpy(key, line, copy_length);
  key[copy_length] = '\0';
  ih_trim_spaces_in_place(key);

  return key[0] != '\0';
}

static bool ih_extract_assignment_quoted_value(const char *line, char *value, size_t value_size) {
  const char *first_quote = NULL;
  const char *second_quote = NULL;
  size_t copy_length = 0;

  if(line == NULL || value == NULL || value_size == 0)
    return false;

  first_quote = strchr(line, '"');
  if(first_quote == NULL)
    return false;
  first_quote++;

  second_quote = strchr(first_quote, '"');
  if(second_quote == NULL)
    return false;

  copy_length = (size_t)(second_quote - first_quote);
  if(copy_length >= value_size)
    copy_length = value_size - 1;

  memcpy(value, first_quote, copy_length);
  value[copy_length] = '\0';
  return true;
}

static bool ih_build_profile_locations_for_leaf(const char *roots_csv,
                                                const char *leaf,
                                                char *output,
                                                size_t output_size) {
  int root_count = 0;
  char **roots = NULL;
  size_t used = 0;

  if(roots_csv == NULL || output == NULL || output_size == 0)
    return false;

  output[0] = '\0';
  roots = utils_split_semicolon_list((char *)roots_csv, &root_count);
  if(root_count == 0) {
    utils_free_string_array(roots, root_count);
    return false;
  }

  for(int i = 0; i < root_count; i++) {
    char mapped[IH_INPUT_BUFFER_SIZE];
    int written = 0;

    if(!ih_join_root_and_leaf(roots[i], leaf, mapped, sizeof(mapped))) {
      utils_free_string_array(roots, root_count);
      return false;
    }

    written = snprintf(output + used,
                       output_size - used,
                       "%s%s",
                       i == 0 ? "" : ";",
                       mapped);
    if(written < 0 || (size_t)written >= (output_size - used)) {
      utils_free_string_array(roots, root_count);
      return false;
    }
    used += (size_t)written;
  }

  utils_free_string_array(roots, root_count);
  return true;
}

static bool ih_rewrite_stock_preset_line(const char *line,
                                         const char *profile_location,
                                         const char *profile_locations,
                                         const char *profile_location_caps,
                                         char *output,
                                         size_t output_size) {
  char key[IH_INPUT_BUFFER_SIZE];
  char value[IH_INPUT_BUFFER_SIZE];
  char profile_leaf[IH_INPUT_BUFFER_SIZE];
  char mapped_value[IH_INPUT_BUFFER_SIZE];
  int written = 0;

  if(line == NULL || output == NULL || output_size == 0)
    return false;

  if(!ih_extract_assignment_key(line, key, sizeof(key))) {
    ih_copy_string(output, output_size, line);
    return true;
  }

  if(ih_string_ends_with(key, "_LOCATION_MAX_USAGE_PCT")) {
    written = snprintf(output, output_size, "%s=\"%s\"", key, profile_location_caps);
    return written >= 0 && (size_t)written < output_size;
  }

  if(ih_string_ends_with(key, "_LOCATIONS")) {
    if(!ih_extract_assignment_quoted_value(line, value, sizeof(value))) {
      ih_copy_string(output, output_size, line);
      return true;
    }
    ih_extract_last_path_component(value, profile_leaf, sizeof(profile_leaf));
    if(!ih_build_profile_locations_for_leaf(profile_locations, profile_leaf, mapped_value, sizeof(mapped_value)))
      return false;
    written = snprintf(output, output_size, "%s=\"%s\"", key, mapped_value);
    return written >= 0 && (size_t)written < output_size;
  }

  if(ih_string_ends_with(key, "_LOCATION")) {
    if(!ih_extract_assignment_quoted_value(line, value, sizeof(value))) {
      ih_copy_string(output, output_size, line);
      return true;
    }
    ih_extract_last_path_component(value, profile_leaf, sizeof(profile_leaf));
    if(!ih_join_root_and_leaf(profile_location, profile_leaf, mapped_value, sizeof(mapped_value)))
      return false;
    written = snprintf(output, output_size, "%s=\"%s\"", key, mapped_value);
    return written >= 0 && (size_t)written < output_size;
  }

  ih_copy_string(output, output_size, line);
  return true;
}

static bool ih_resolve_stock_preset_path(const char *config_dir,
                                         const char *relative_path,
                                         char *resolved_path,
                                         size_t resolved_path_size) {
  char candidate[IH_INPUT_BUFFER_SIZE];

  if(relative_path == NULL || resolved_path == NULL || resolved_path_size == 0)
    return false;

  if(config_dir != NULL && config_dir[0] != '\0') {
    snprintf(candidate, sizeof(candidate), "%s/presets/%s", config_dir, relative_path);
    if(utils_does_file_exist(candidate)) {
      ih_copy_string(resolved_path, resolved_path_size, candidate);
      return true;
    }
  }

  snprintf(candidate, sizeof(candidate), "%s/presets/%s", IH_LEGACY_CONFIG_DIR, relative_path);
  if(utils_does_file_exist(candidate)) {
    ih_copy_string(resolved_path, resolved_path_size, candidate);
    return true;
  }

  snprintf(candidate, sizeof(candidate), "services/vfo/presets/%s", relative_path);
  if(utils_does_file_exist(candidate)) {
    ih_copy_string(resolved_path, resolved_path_size, candidate);
    return true;
  }

  snprintf(candidate, sizeof(candidate), "presets/%s", relative_path);
  if(utils_does_file_exist(candidate)) {
    ih_copy_string(resolved_path, resolved_path_size, candidate);
    return true;
  }

  return false;
}

static void ih_print_stock_preset_menu(void) {
  printf("WIZARD INFO: available stock preset packs:\n");
  for(int i = 0; i < IH_STOCK_PRESET_COUNT; i++) {
    printf("  %i) %s (%s)\n",
           i + 1,
           IH_STOCK_PRESET_OPTIONS[i].key,
           IH_STOCK_PRESET_OPTIONS[i].display_name);
  }
  printf("  all) all preset packs\n");
}

static bool ih_parse_stock_preset_selection(const char *input, bool *selected, int *selected_count) {
  char working[IH_INPUT_BUFFER_SIZE];
  char *token = NULL;
  char *save_ptr = NULL;

  if(input == NULL || selected == NULL || selected_count == NULL)
    return false;

  for(int i = 0; i < IH_STOCK_PRESET_COUNT; i++) {
    selected[i] = false;
  }
  *selected_count = 0;

  ih_copy_string(working, sizeof(working), input);
  ih_trim_spaces_in_place(working);
  if(ih_is_blank(working))
    return false;

  for(size_t i = 0; working[i] != '\0'; i++) {
    if(working[i] == ';')
      working[i] = ',';
  }

  if(strcasecmp(working, "all") == 0) {
    for(int i = 0; i < IH_STOCK_PRESET_COUNT; i++) {
      selected[i] = true;
    }
    *selected_count = IH_STOCK_PRESET_COUNT;
    return true;
  }

  token = strtok_r(working, ",", &save_ptr);
  while(token != NULL) {
    int selected_index = -1;

    ih_trim_spaces_in_place(token);
    if(token[0] == '\0') {
      token = strtok_r(NULL, ",", &save_ptr);
      continue;
    }

    if(utils_string_only_contains_number_characters(token)) {
      int number = utils_convert_string_to_integer(token);
      if(number >= 1 && number <= IH_STOCK_PRESET_COUNT)
        selected_index = number - 1;
      else
        return false;
    } else {
      selected_index = ih_resolve_stock_preset_index(token);
      if(selected_index < 0)
        return false;
    }

    if(!selected[selected_index]) {
      selected[selected_index] = true;
      (*selected_count)++;
    }

    token = strtok_r(NULL, ",", &save_ptr);
  }

  return *selected_count > 0;
}

static bool ih_append_selected_stock_presets(FILE *config_file,
                                             const char *config_dir,
                                             const bool *selected,
                                             const char *profile_location,
                                             const char *profile_locations,
                                             const char *profile_location_caps) {
  char line_buffer[IH_INPUT_BUFFER_SIZE];
  char output_line[IH_INPUT_BUFFER_SIZE];
  char preset_path[IH_INPUT_BUFFER_SIZE];

  if(config_file == NULL || selected == NULL)
    return false;

  for(int i = 0; i < IH_STOCK_PRESET_COUNT; i++) {
    FILE *preset_file = NULL;
    bool in_block_comment = false;

    if(!selected[i])
      continue;

      if(!ih_resolve_stock_preset_path(config_dir,
                                       IH_STOCK_PRESET_OPTIONS[i].relative_path,
                                       preset_path,
                                       sizeof(preset_path))) {
        printf("WIZARD ERROR: could not resolve preset file for %s\n", IH_STOCK_PRESET_OPTIONS[i].key);
        return false;
      }

    preset_file = fopen(preset_path, "r");
    if(preset_file == NULL) {
      printf("WIZARD ERROR: could not open preset file: %s\n", preset_path);
      return false;
    }

    fprintf(config_file, "\n/* stock preset: %s */\n", IH_STOCK_PRESET_OPTIONS[i].key);
    while(fgets(line_buffer, sizeof(line_buffer), preset_file) != NULL) {
      ih_strip_newline(line_buffer);
      ih_trim_spaces_in_place(line_buffer);

      if(in_block_comment) {
        if(strstr(line_buffer, "*/") != NULL)
          in_block_comment = false;
        continue;
      }

      if(line_buffer[0] == '\0')
        continue;

      if(strncmp(line_buffer, "/*", 2) == 0) {
        if(strstr(line_buffer, "*/") == NULL)
          in_block_comment = true;
        continue;
      }
      if(strncmp(line_buffer, "//", 2) == 0 || line_buffer[0] == '*')
        continue;

      if(!ih_rewrite_stock_preset_line(line_buffer,
                                       profile_location,
                                       profile_locations,
                                       profile_location_caps,
                                       output_line,
                                       sizeof(output_line))) {
        fclose(preset_file);
        printf("WIZARD ERROR: could not rewrite preset line for %s\n", IH_STOCK_PRESET_OPTIONS[i].key);
        return false;
      }

      fprintf(config_file, "%s\n", output_line);
    }

    fclose(preset_file);
  }

  return true;
}

static int ih_run_wizard(const char *config_dir) {
  char mezzanine_location[IH_INPUT_BUFFER_SIZE];
  char mezzanine_locations[IH_INPUT_BUFFER_SIZE];
  char mezzanine_location_max_usage_pct[IH_INPUT_BUFFER_SIZE];
  char source_location[IH_INPUT_BUFFER_SIZE];
  char source_locations[IH_INPUT_BUFFER_SIZE];
  char source_location_max_usage_pct[IH_INPUT_BUFFER_SIZE];
  char source_test_trim_start[IH_INPUT_BUFFER_SIZE];
  char source_test_trim_duration[IH_INPUT_BUFFER_SIZE];
  char custom_folder_name[IH_INPUT_BUFFER_SIZE];
  char custom_folder_type[IH_INPUT_BUFFER_SIZE];
  char profile_name[IH_INPUT_BUFFER_SIZE];
  char profile_upper[IH_INPUT_BUFFER_SIZE];
  char profile_location[IH_INPUT_BUFFER_SIZE];
  char profile_locations[IH_INPUT_BUFFER_SIZE];
  char profile_location_max_usage_pct[IH_INPUT_BUFFER_SIZE];
  char profile_crit_codec[IH_INPUT_BUFFER_SIZE];
  char profile_crit_bits[IH_INPUT_BUFFER_SIZE];
  char profile_crit_color_space[IH_INPUT_BUFFER_SIZE];
  char profile_crit_min_width[IH_INPUT_BUFFER_SIZE];
  char profile_crit_min_height[IH_INPUT_BUFFER_SIZE];
  char profile_crit_max_width[IH_INPUT_BUFFER_SIZE];
  char profile_crit_max_height[IH_INPUT_BUFFER_SIZE];
  char profile_scenario[IH_INPUT_BUFFER_SIZE];
  char profile_ffmpeg_command[IH_INPUT_BUFFER_SIZE];
  char default_profile_location[IH_INPUT_BUFFER_SIZE];
  char default_mezzanine_caps[IH_INPUT_BUFFER_SIZE];
  char default_source_caps[IH_INPUT_BUFFER_SIZE];
  char default_profile_caps[IH_INPUT_BUFFER_SIZE];
  char config_temp_path[IH_INPUT_BUFFER_SIZE];
  char backup_config_path[IH_INPUT_BUFFER_SIZE];
  char *config_path = NULL;
  FILE *config_file = NULL;
  bool keep_source = true;
  bool source_test_active = false;
  bool mezzanine_clean_enabled = false;
  bool mezzanine_clean_apply_changes = false;
  bool mezzanine_clean_append_media_tags = true;
  bool mezzanine_clean_strict_quality_gate = false;
  bool quality_check_enabled = false;
  bool quality_check_include_vmaf = false;
  bool quality_check_strict_gate = false;
  char quality_check_reference_layer[IH_INPUT_BUFFER_SIZE];
  char quality_check_min_psnr[IH_INPUT_BUFFER_SIZE];
  char quality_check_min_ssim[IH_INPUT_BUFFER_SIZE];
  char quality_check_min_vmaf[IH_INPUT_BUFFER_SIZE];
  char quality_check_max_files_per_profile[IH_INPUT_BUFFER_SIZE];
  char profile_setup_mode[IH_INPUT_BUFFER_SIZE];
  char stock_preset_selection[IH_INPUT_BUFFER_SIZE];
  char stock_profile_location[IH_INPUT_BUFFER_SIZE];
  char stock_profile_locations[IH_INPUT_BUFFER_SIZE];
  char stock_profile_location_max_usage_pct[IH_INPUT_BUFFER_SIZE];
  char default_stock_profile_caps[IH_INPUT_BUFFER_SIZE];
  bool selected_stock_presets[IH_STOCK_PRESET_COUNT];
  int selected_stock_preset_count = 0;
  bool use_stock_presets = true;
  bool advanced_mode = false;
  bool write_config = true;
  int wizard_step = 0;
  int wizard_total_steps = 0;

  ih_wizard_print_banner();
  printf("WIZARD ALERT: guided setup for %s\n", IH_CONFIG_FILENAME);
  printf("WIZARD INFO: press Enter to accept defaults\n");
  printf("WIZARD INFO: target config directory: %s\n", config_dir);

  if(!ih_wizard_prompt_mode(&advanced_mode))
    return EXIT_FAILURE;

  wizard_total_steps = advanced_mode ? 7 : 6;
  printf("WIZARD INFO: onboarding mode selected: %s\n", advanced_mode ? "advanced" : "quickstart");

  ih_wizard_print_step(++wizard_step,
                       wizard_total_steps,
                       "Preflight",
                       "Checking command availability and config path permissions");
  if(!ih_wizard_preflight(config_dir))
    return EXIT_FAILURE;

  ih_wizard_print_step(++wizard_step,
                       wizard_total_steps,
                       "Storage roots",
                       "Configure mezzanine and source locations, including per-root capacity caps");
  if(!ih_prompt_line("Mezzanine location", "/Users/craigwatt/Downloads/PRACTICE101/mezzanine", mezzanine_location, sizeof(mezzanine_location), true))
    return EXIT_FAILURE;
  if(!ih_prompt_line("Mezzanine locations (semicolon list)", mezzanine_location, mezzanine_locations, sizeof(mezzanine_locations), true))
    return EXIT_FAILURE;
  ih_build_default_cap_list(mezzanine_locations, mezzanine_location_max_usage_pct, sizeof(mezzanine_location_max_usage_pct));
  ih_copy_string(default_mezzanine_caps, sizeof(default_mezzanine_caps), mezzanine_location_max_usage_pct);
  if(!ih_prompt_line("Mezzanine max usage pct list (semicolon, 1-100)", default_mezzanine_caps, mezzanine_location_max_usage_pct, sizeof(mezzanine_location_max_usage_pct), true))
    return EXIT_FAILURE;
  if(!ih_first_location_from_list(mezzanine_locations, mezzanine_location, sizeof(mezzanine_location)))
    return EXIT_FAILURE;

  if(!ih_prompt_line("Source location", "/Users/craigwatt/Downloads/PRACTICE101/source", source_location, sizeof(source_location), true))
    return EXIT_FAILURE;
  if(!ih_prompt_line("Source locations (semicolon list)", source_location, source_locations, sizeof(source_locations), true))
    return EXIT_FAILURE;
  ih_build_default_cap_list(source_locations, source_location_max_usage_pct, sizeof(source_location_max_usage_pct));
  ih_copy_string(default_source_caps, sizeof(default_source_caps), source_location_max_usage_pct);
  if(!ih_prompt_line("Source max usage pct list (semicolon, 1-100)", default_source_caps, source_location_max_usage_pct, sizeof(source_location_max_usage_pct), true))
    return EXIT_FAILURE;
  if(!ih_first_location_from_list(source_locations, source_location, sizeof(source_location)))
    return EXIT_FAILURE;

  ih_wizard_print_step(++wizard_step,
                       wizard_total_steps,
                       "Pipeline defaults",
                       advanced_mode
                         ? "Choose detailed runtime behavior, hygiene policy, and quality scoring thresholds"
                         : "Applying recommended quickstart defaults for runtime toggles");
  if(advanced_mode) {
    if(!ih_prompt_bool("Keep source outputs?", true, &keep_source))
      return EXIT_FAILURE;

    if(!ih_prompt_bool("Enable source test clipping?", true, &source_test_active))
      return EXIT_FAILURE;

    if(source_test_active) {
      if(!ih_prompt_timecode("Source test trim start", "00:00:20", source_test_trim_start, sizeof(source_test_trim_start)))
        return EXIT_FAILURE;
      if(!ih_prompt_timecode("Source test trim duration", "00:01:00", source_test_trim_duration, sizeof(source_test_trim_duration)))
        return EXIT_FAILURE;
    } else {
      ih_copy_string(source_test_trim_start, sizeof(source_test_trim_start), "00:00:20");
      ih_copy_string(source_test_trim_duration, sizeof(source_test_trim_duration), "00:01:00");
    }

    if(!ih_prompt_bool("Enable mezzanine-clean hygiene checks?", true, &mezzanine_clean_enabled))
      return EXIT_FAILURE;
    if(!ih_prompt_bool("Apply mezzanine-clean renames/moves automatically?", true, &mezzanine_clean_apply_changes))
      return EXIT_FAILURE;
    if(!ih_prompt_bool("Append probe-derived media tags to mezzanine filenames?", true, &mezzanine_clean_append_media_tags))
      return EXIT_FAILURE;
    if(!ih_prompt_bool("Fail mezzanine-clean when warnings are found (strict gate)?", true, &mezzanine_clean_strict_quality_gate))
      return EXIT_FAILURE;

    if(!ih_prompt_bool("Enable post-profile quality scoring (PSNR/SSIM)?", true, &quality_check_enabled))
      return EXIT_FAILURE;
    if(!ih_prompt_bool("Include VMAF in quality scoring (requires ffmpeg libvmaf)?", true, &quality_check_include_vmaf))
      return EXIT_FAILURE;
    if(!ih_prompt_bool("Fail run when quality checks fail (strict gate)?", true, &quality_check_strict_gate))
      return EXIT_FAILURE;
    if(!ih_prompt_line("Quality reference layer (auto|source|mezzanine)", "auto", quality_check_reference_layer, sizeof(quality_check_reference_layer), true))
      return EXIT_FAILURE;
    if(!ih_prompt_line("Quality minimum PSNR (0 disables threshold)", "0", quality_check_min_psnr, sizeof(quality_check_min_psnr), true))
      return EXIT_FAILURE;
    if(!ih_prompt_line("Quality minimum SSIM (0 disables threshold)", "0", quality_check_min_ssim, sizeof(quality_check_min_ssim), true))
      return EXIT_FAILURE;
    if(!ih_prompt_line("Quality minimum VMAF (0 disables threshold)", "0", quality_check_min_vmaf, sizeof(quality_check_min_vmaf), true))
      return EXIT_FAILURE;
    if(!ih_prompt_line("Quality max files per profile to score (0 means all)", "0", quality_check_max_files_per_profile, sizeof(quality_check_max_files_per_profile), true))
      return EXIT_FAILURE;

    if(strcasecmp(quality_check_reference_layer, "auto") != 0
       && strcasecmp(quality_check_reference_layer, "source") != 0
       && strcasecmp(quality_check_reference_layer, "mezzanine") != 0) {
      printf("WIZARD ERROR: quality reference layer must be auto, source, or mezzanine\n");
      return EXIT_FAILURE;
    }
    utils_lowercase_string(quality_check_reference_layer);

    {
      char *end_ptr = NULL;
      double parsed_double = 0.0;
      long parsed_int = 0;

      parsed_double = strtod(quality_check_min_psnr, &end_ptr);
      if(end_ptr == quality_check_min_psnr || *end_ptr != '\0' || parsed_double < 0.0) {
        printf("WIZARD ERROR: Quality minimum PSNR must be a non-negative number\n");
        return EXIT_FAILURE;
      }

      end_ptr = NULL;
      parsed_double = strtod(quality_check_min_ssim, &end_ptr);
      if(end_ptr == quality_check_min_ssim || *end_ptr != '\0' || parsed_double < 0.0) {
        printf("WIZARD ERROR: Quality minimum SSIM must be a non-negative number\n");
        return EXIT_FAILURE;
      }

      end_ptr = NULL;
      parsed_double = strtod(quality_check_min_vmaf, &end_ptr);
      if(end_ptr == quality_check_min_vmaf || *end_ptr != '\0' || parsed_double < 0.0) {
        printf("WIZARD ERROR: Quality minimum VMAF must be a non-negative number\n");
        return EXIT_FAILURE;
      }

      end_ptr = NULL;
      parsed_int = strtol(quality_check_max_files_per_profile, &end_ptr, 10);
      if(end_ptr == quality_check_max_files_per_profile || *end_ptr != '\0' || parsed_int < 0) {
        printf("WIZARD ERROR: Quality max files per profile must be a non-negative integer\n");
        return EXIT_FAILURE;
      }
    }
  } else {
    keep_source = true;
    source_test_active = false;
    ih_copy_string(source_test_trim_start, sizeof(source_test_trim_start), "00:00:20");
    ih_copy_string(source_test_trim_duration, sizeof(source_test_trim_duration), "00:01:00");

    mezzanine_clean_enabled = true;
    mezzanine_clean_apply_changes = false;
    mezzanine_clean_append_media_tags = true;
    mezzanine_clean_strict_quality_gate = false;

    quality_check_enabled = false;
    quality_check_include_vmaf = false;
    quality_check_strict_gate = false;
    ih_copy_string(quality_check_reference_layer, sizeof(quality_check_reference_layer), "auto");
    ih_copy_string(quality_check_min_psnr, sizeof(quality_check_min_psnr), "0");
    ih_copy_string(quality_check_min_ssim, sizeof(quality_check_min_ssim), "0");
    ih_copy_string(quality_check_min_vmaf, sizeof(quality_check_min_vmaf), "0");
    ih_copy_string(quality_check_max_files_per_profile, sizeof(quality_check_max_files_per_profile), "0");

    printf("WIZARD INFO: quickstart defaults applied:\n");
    printf("  KEEP_SOURCE=true\n");
    printf("  SOURCE_TEST_ACTIVE=false\n");
    printf("  MEZZANINE_CLEAN_ENABLED=true (apply=false)\n");
    printf("  QUALITY_CHECK_ENABLED=false\n");
  }

  ih_wizard_print_step(++wizard_step,
                       wizard_total_steps,
                       "Library and profile mode",
                       "Choose library folder metadata and whether to use stock presets or custom profile authoring");
  if(!ih_prompt_line("First library folder name", "Movies", custom_folder_name, sizeof(custom_folder_name), true))
    return EXIT_FAILURE;

  while(true) {
    if(!ih_prompt_line("First library folder type (films|tv)", "films", custom_folder_type, sizeof(custom_folder_type), true))
      return EXIT_FAILURE;
    if(strcasecmp(custom_folder_type, "films") == 0 || strcasecmp(custom_folder_type, "tv") == 0)
      break;
    printf("WIZARD INFO: folder type must be 'films' or 'tv'\n");
  }

  if(advanced_mode) {
    if(!ih_prompt_line("Profile setup mode (stock|custom)", "stock", profile_setup_mode, sizeof(profile_setup_mode), true))
      return EXIT_FAILURE;
    ih_trim_spaces_in_place(profile_setup_mode);
    utils_lowercase_string(profile_setup_mode);
    if(strcmp(profile_setup_mode, "stock") == 0
       || strcmp(profile_setup_mode, "presets") == 0
       || strcmp(profile_setup_mode, "preset") == 0) {
      use_stock_presets = true;
    } else if(strcmp(profile_setup_mode, "custom") == 0) {
      use_stock_presets = false;
    } else {
      printf("WIZARD ERROR: profile setup mode must be stock or custom\n");
      return EXIT_FAILURE;
    }
  } else {
    use_stock_presets = true;
    ih_copy_string(profile_setup_mode, sizeof(profile_setup_mode), "stock");
    printf("WIZARD INFO: quickstart mode uses stock presets\n");
  }

  ih_wizard_print_step(++wizard_step,
                       wizard_total_steps,
                       "Profile details",
                       use_stock_presets
                         ? "Choose stock preset packs and configure profile output roots"
                         : "Configure one custom profile with criteria and execution command");
  if(use_stock_presets) {
    ih_print_stock_preset_menu();
    while(true) {
      if(!ih_prompt_line("Select stock preset packs (comma list, names, or all)", "craigstreamy_hevc_selected_english_subtitle_preserve", stock_preset_selection, sizeof(stock_preset_selection), true))
        return EXIT_FAILURE;
      if(ih_parse_stock_preset_selection(stock_preset_selection, selected_stock_presets, &selected_stock_preset_count))
        break;
      printf("WIZARD INFO: choose one or more presets, e.g. 1,3 or all\n");
    }

    if(!ih_prompt_line("Profiles output base location", "/Users/craigwatt/Downloads/PRACTICE101/profiles", stock_profile_location, sizeof(stock_profile_location), true))
      return EXIT_FAILURE;
    if(!ih_prompt_line("Profiles output base locations (semicolon list)", stock_profile_location, stock_profile_locations, sizeof(stock_profile_locations), true))
      return EXIT_FAILURE;
    ih_build_default_cap_list(stock_profile_locations, stock_profile_location_max_usage_pct, sizeof(stock_profile_location_max_usage_pct));
    ih_copy_string(default_stock_profile_caps, sizeof(default_stock_profile_caps), stock_profile_location_max_usage_pct);
    if(!ih_prompt_line("Profiles max usage pct list (semicolon, 1-100)", default_stock_profile_caps, stock_profile_location_max_usage_pct, sizeof(stock_profile_location_max_usage_pct), true))
      return EXIT_FAILURE;
    if(!ih_first_location_from_list(stock_profile_locations, stock_profile_location, sizeof(stock_profile_location)))
      return EXIT_FAILURE;
  } else {
    if(!ih_prompt_line("Profile name", "craigstreamy_hevc_selected_english_subtitle_preserve_1080p", profile_name, sizeof(profile_name), true))
      return EXIT_FAILURE;
    ih_sanitize_alias_name(profile_name, sizeof(profile_name));
    printf("WIZARD INFO: profile key set to '%s'\n", profile_name);

    snprintf(default_profile_location, sizeof(default_profile_location), "%s/%s", source_location, profile_name);
    if(!ih_prompt_line("Profile output location", default_profile_location, profile_location, sizeof(profile_location), true))
      return EXIT_FAILURE;
    if(!ih_prompt_line("Profile output locations (semicolon list)", profile_location, profile_locations, sizeof(profile_locations), true))
      return EXIT_FAILURE;
    ih_build_default_cap_list(profile_locations, profile_location_max_usage_pct, sizeof(profile_location_max_usage_pct));
    ih_copy_string(default_profile_caps, sizeof(default_profile_caps), profile_location_max_usage_pct);
    if(!ih_prompt_line("Profile max usage pct list (semicolon, 1-100)", default_profile_caps, profile_location_max_usage_pct, sizeof(profile_location_max_usage_pct), true))
      return EXIT_FAILURE;
    if(!ih_first_location_from_list(profile_locations, profile_location, sizeof(profile_location)))
      return EXIT_FAILURE;

    if(!ih_prompt_line("Profile criteria codec", "h264", profile_crit_codec, sizeof(profile_crit_codec), true))
      return EXIT_FAILURE;
    if(!ih_prompt_line("Profile criteria bits", "8", profile_crit_bits, sizeof(profile_crit_bits), true))
      return EXIT_FAILURE;
    if(!ih_prompt_line("Profile criteria color space", "bt709", profile_crit_color_space, sizeof(profile_crit_color_space), true))
      return EXIT_FAILURE;
    if(!ih_prompt_line("Profile minimum width", "352", profile_crit_min_width, sizeof(profile_crit_min_width), true))
      return EXIT_FAILURE;
    if(!ih_prompt_line("Profile minimum height", "240", profile_crit_min_height, sizeof(profile_crit_min_height), true))
      return EXIT_FAILURE;
    if(!ih_prompt_line("Profile maximum width", "1920", profile_crit_max_width, sizeof(profile_crit_max_width), true))
      return EXIT_FAILURE;
    if(!ih_prompt_line("Profile maximum height", "1080", profile_crit_max_height, sizeof(profile_crit_max_height), true))
      return EXIT_FAILURE;

    if(!ih_prompt_line("Profile scenario", "ELSE", profile_scenario, sizeof(profile_scenario), true))
      return EXIT_FAILURE;

    if(!ih_prompt_line("Profile ffmpeg command", "ffmpeg -nostdin -y -i $vfo_input -map 0:v:0 -map 0:a? -map 0:s? -c:v libx264 -preset medium -crf 19 -pix_fmt yuv420p -c:a copy -c:s copy -movflags +faststart $vfo_output", profile_ffmpeg_command, sizeof(profile_ffmpeg_command), true))
      return EXIT_FAILURE;

    if(strstr(profile_ffmpeg_command, "$vfo_input") == NULL || strstr(profile_ffmpeg_command, "$vfo_output") == NULL) {
      printf("WIZARD ERROR: ffmpeg command must include both $vfo_input and $vfo_output\n");
      return EXIT_FAILURE;
    }
  }

  ih_wizard_print_step(++wizard_step,
                       wizard_total_steps,
                       "Review and write",
                       "Confirm configuration preview before writing");
  printf("WIZARD REVIEW: mode=%s\n", advanced_mode ? "advanced" : "quickstart");
  ih_print_preview("WIZARD REVIEW: MEZZANINE_LOCATIONS=", mezzanine_locations);
  ih_print_preview("WIZARD REVIEW: SOURCE_LOCATIONS=", source_locations);
  ih_print_preview("WIZARD REVIEW: PROFILE_ROOT_LOCATIONS=",
                   use_stock_presets ? stock_profile_locations : profile_locations);
  printf("WIZARD REVIEW: profile_setup_mode=%s\n", use_stock_presets ? "stock" : "custom");
  if(use_stock_presets)
    printf("WIZARD REVIEW: stock_preset_count=%i\n", selected_stock_preset_count);
  else
    printf("WIZARD REVIEW: custom_profile=%s\n", profile_name);

  if(!ih_prompt_bool("Write config file now?", true, &write_config))
    return EXIT_FAILURE;
  if(!write_config) {
    printf("WIZARD ALERT: onboarding canceled before writing changes\n");
    return EXIT_SUCCESS;
  }

  if(!ih_create_config_dir_if_needed(config_dir)) {
    printf("WIZARD ERROR: could not create config directory: %s\n", config_dir);
    printf("WIZARD INFO: if this path requires elevated permissions, run wizard with sudo\n");
    return EXIT_FAILURE;
  }

  config_path = utils_combine_to_full_path(config_dir, IH_CONFIG_FILENAME);
  if(config_path == NULL) {
    printf("WIZARD ERROR: could not resolve config path for directory: %s\n", config_dir);
    return EXIT_FAILURE;
  }

  if(snprintf(config_temp_path, sizeof(config_temp_path), "%s.tmp", config_path) < 0
     || strlen(config_temp_path) >= sizeof(config_temp_path) - 1) {
    printf("WIZARD ERROR: temporary config path is too long\n");
    free(config_path);
    return EXIT_FAILURE;
  }

  config_file = fopen(config_temp_path, "w");
  if(config_file == NULL) {
    printf("WIZARD ERROR: could not write config file: %s\n", config_temp_path);
    printf("WIZARD INFO: if this path requires elevated permissions, run wizard with sudo\n");
    free(config_path);
    return EXIT_FAILURE;
  }

  fprintf(config_file, "/* vfo config generated by `vfo wizard` */\n\n");
  fprintf(config_file, "MEZZANINE_LOCATION=\"%s\"\n", mezzanine_location);
  fprintf(config_file, "MEZZANINE_LOCATIONS=\"%s\"\n", mezzanine_locations);
  fprintf(config_file, "MEZZANINE_LOCATION_MAX_USAGE_PCT=\"%s\"\n", mezzanine_location_max_usage_pct);
  fprintf(config_file, "SOURCE_LOCATION=\"%s\"\n", source_location);
  fprintf(config_file, "SOURCE_LOCATIONS=\"%s\"\n", source_locations);
  fprintf(config_file, "SOURCE_LOCATION_MAX_USAGE_PCT=\"%s\"\n", source_location_max_usage_pct);
  fprintf(config_file, "KEEP_SOURCE=\"%s\"\n\n", keep_source ? "true" : "false");
  fprintf(config_file, "SOURCE_TEST_ACTIVE=\"%s\"\n", source_test_active ? "true" : "false");
  fprintf(config_file, "SOURCE_TEST_TRIM_START=\"%s\"\n", source_test_trim_start);
  fprintf(config_file, "SOURCE_TEST_TRIM_DURATION=\"%s\"\n", source_test_trim_duration);
  fprintf(config_file, "MEZZANINE_CLEAN_ENABLED=\"%s\"\n", mezzanine_clean_enabled ? "true" : "false");
  fprintf(config_file, "MEZZANINE_CLEAN_APPLY_CHANGES=\"%s\"\n", mezzanine_clean_apply_changes ? "true" : "false");
  fprintf(config_file, "MEZZANINE_CLEAN_APPEND_MEDIA_TAGS=\"%s\"\n", mezzanine_clean_append_media_tags ? "true" : "false");
  fprintf(config_file, "MEZZANINE_CLEAN_STRICT_QUALITY_GATE=\"%s\"\n\n", mezzanine_clean_strict_quality_gate ? "true" : "false");
  fprintf(config_file, "QUALITY_CHECK_ENABLED=\"%s\"\n", quality_check_enabled ? "true" : "false");
  fprintf(config_file, "QUALITY_CHECK_INCLUDE_VMAF=\"%s\"\n", quality_check_include_vmaf ? "true" : "false");
  fprintf(config_file, "QUALITY_CHECK_STRICT_GATE=\"%s\"\n", quality_check_strict_gate ? "true" : "false");
  fprintf(config_file, "QUALITY_CHECK_REFERENCE_LAYER=\"%s\"\n", quality_check_reference_layer);
  fprintf(config_file, "QUALITY_CHECK_MIN_PSNR=\"%s\"\n", quality_check_min_psnr);
  fprintf(config_file, "QUALITY_CHECK_MIN_SSIM=\"%s\"\n", quality_check_min_ssim);
  fprintf(config_file, "QUALITY_CHECK_MIN_VMAF=\"%s\"\n", quality_check_min_vmaf);
  fprintf(config_file, "QUALITY_CHECK_MAX_FILES_PER_PROFILE=\"%s\"\n\n", quality_check_max_files_per_profile);
  fprintf(config_file, "CUSTOM_FOLDER=\"%s,%s\"\n\n", custom_folder_name, custom_folder_type);

  if(use_stock_presets) {
    if(!ih_append_selected_stock_presets(config_file,
                                         config_dir,
                                         selected_stock_presets,
                                         stock_profile_location,
                                         stock_profile_locations,
                                         stock_profile_location_max_usage_pct)) {
      fclose(config_file);
      remove(config_temp_path);
      printf("WIZARD ERROR: failed to append selected stock presets\n");
      free(config_path);
      return EXIT_FAILURE;
    }
  } else {
    ih_copy_string(profile_upper, sizeof(profile_upper), profile_name);
    ih_uppercase_in_place(profile_upper);

    fprintf(config_file, "PROFILE=\"%s\"\n", profile_name);
    fprintf(config_file, "%s_LOCATION=\"%s\"\n", profile_upper, profile_location);
    fprintf(config_file, "%s_LOCATIONS=\"%s\"\n", profile_upper, profile_locations);
    fprintf(config_file, "%s_LOCATION_MAX_USAGE_PCT=\"%s\"\n", profile_upper, profile_location_max_usage_pct);
    fprintf(config_file, "%s_CRITERIA_CODEC_NAME=\"%s\"\n", profile_upper, profile_crit_codec);
    fprintf(config_file, "%s_CRITERIA_BITS=\"%s\"\n", profile_upper, profile_crit_bits);
    fprintf(config_file, "%s_CRITERIA_COLOR_SPACE=\"%s\"\n", profile_upper, profile_crit_color_space);
    fprintf(config_file, "%s_CRITERIA_RESOLUTION_MIN_WIDTH=\"%s\"\n", profile_upper, profile_crit_min_width);
    fprintf(config_file, "%s_CRITERIA_RESOLUTION_MIN_HEIGHT=\"%s\"\n", profile_upper, profile_crit_min_height);
    fprintf(config_file, "%s_CRITERIA_RESOLUTION_MAX_WIDTH=\"%s\"\n", profile_upper, profile_crit_max_width);
    fprintf(config_file, "%s_CRITERIA_RESOLUTION_MAX_HEIGHT=\"%s\"\n", profile_upper, profile_crit_max_height);
    fprintf(config_file, "%s_SCENARIO=\"%s\"\n", profile_upper, profile_scenario);
    fprintf(config_file, "%s_FFMPEG_COMMAND=\"%s\"\n", profile_upper, profile_ffmpeg_command);
  }

  fclose(config_file);
  backup_config_path[0] = '\0';
  if(!ih_create_config_backup_if_present(config_path, backup_config_path, sizeof(backup_config_path))) {
    printf("WIZARD ERROR: could not create backup for existing config: %s\n", config_path);
    remove(config_temp_path);
    free(config_path);
    return EXIT_FAILURE;
  }

  if(rename(config_temp_path, config_path) != 0) {
    printf("WIZARD ERROR: could not replace config file: %s\n", config_path);
    printf("WIZARD INFO: temporary config retained at %s\n", config_temp_path);
    free(config_path);
    return EXIT_FAILURE;
  }

  printf("WIZARD ALERT: config written to %s\n", config_path);
  if(backup_config_path[0] != '\0')
    printf("WIZARD INFO: backup saved to %s\n", backup_config_path);
  if(use_stock_presets)
    printf("WIZARD INFO: selected stock preset packs: %i\n", selected_stock_preset_count);
  printf("WIZARD NEXT: run `vfo show`, then `vfo doctor`, then `vfo mezzanine-clean`, then `vfo run`\n");

  free(config_path);
  return EXIT_SUCCESS;
}

static void ih_show_config(config_t *config, const char *config_dir) {
  int profile_count = 0;
  char *config_path = utils_combine_to_full_path(config_dir, IH_CONFIG_FILENAME);
  cf_node_t *custom_folder = NULL;
  ca_node_t *profile = NULL;

  printf("SHOW ALERT: active config file: %s\n", config_path);
  free(config_path);

  printf("\n[global]\n");
  printf("MEZZANINE_LOCATION=%s\n", config->svc->original_location);
  printf("MEZZANINE_LOCATIONS=%s\n", config->svc->original_locations);
  if(utils_string_is_empty_or_spaces(config->svc->original_location_max_usage_pct) == false)
    printf("MEZZANINE_LOCATION_MAX_USAGE_PCT=%s\n", config->svc->original_location_max_usage_pct);
  printf("SOURCE_LOCATION=%s\n", config->svc->source_location);
  printf("SOURCE_LOCATIONS=%s\n", config->svc->source_locations);
  if(utils_string_is_empty_or_spaces(config->svc->source_location_max_usage_pct) == false)
    printf("SOURCE_LOCATION_MAX_USAGE_PCT=%s\n", config->svc->source_location_max_usage_pct);
  printf("KEEP_SOURCE=%s\n", config->svc->keep_source ? "true" : "false");
  printf("SOURCE_TEST_ACTIVE=%s\n", config->svc->source_test ? "true" : "false");
  if(config->svc->source_test) {
    printf("SOURCE_TEST_TRIM_START=%s\n", config->svc->source_test_trim_start);
    printf("SOURCE_TEST_TRIM_DURATION=%s\n", config->svc->source_test_trim_duration);
  }
  printf("MEZZANINE_CLEAN_ENABLED=%s\n", config->svc->mezzanine_clean_enabled ? "true" : "false");
  printf("MEZZANINE_CLEAN_APPLY_CHANGES=%s\n", config->svc->mezzanine_clean_apply_changes ? "true" : "false");
  printf("MEZZANINE_CLEAN_APPEND_MEDIA_TAGS=%s\n", config->svc->mezzanine_clean_append_media_tags ? "true" : "false");
  printf("MEZZANINE_CLEAN_STRICT_QUALITY_GATE=%s\n", config->svc->mezzanine_clean_strict_quality_gate ? "true" : "false");
  printf("QUALITY_CHECK_ENABLED=%s\n", config->svc->quality_check_enabled ? "true" : "false");
  printf("QUALITY_CHECK_INCLUDE_VMAF=%s\n", config->svc->quality_check_include_vmaf ? "true" : "false");
  printf("QUALITY_CHECK_STRICT_GATE=%s\n", config->svc->quality_check_strict_gate ? "true" : "false");
  printf("QUALITY_CHECK_REFERENCE_LAYER=%s\n", config->svc->quality_check_reference_layer);
  printf("QUALITY_CHECK_MIN_PSNR=%.3f\n", config->svc->quality_check_min_psnr);
  printf("QUALITY_CHECK_MIN_SSIM=%.6f\n", config->svc->quality_check_min_ssim);
  printf("QUALITY_CHECK_MIN_VMAF=%.3f\n", config->svc->quality_check_min_vmaf);
  printf("QUALITY_CHECK_MAX_FILES_PER_PROFILE=%i\n", config->svc->quality_check_max_files_per_profile);

  printf("\n[custom_folders]\n");
  custom_folder = config->cf_head;
  if(custom_folder == NULL) {
    printf("none\n");
  } else {
    while(custom_folder != NULL) {
      printf("- %s (%s)\n", custom_folder->folder_name, custom_folder->folder_type);
      custom_folder = custom_folder->next;
    }
  }

  profile_count = ca_get_count(config->ca_head);
  printf("\n[profiles] count=%i\n", profile_count);
  if(profile_count == 0) {
    printf("none\n");
    return;
  }

  profile = config->ca_head;
  while(profile != NULL) {
    int scenario_count = ih_cs_count(profile->cs_head);
    printf("\nprofile=%s\n", profile->alias_name);
    printf("  location=%s\n", profile->alias_location);
    printf("  locations=%s\n", profile->alias_locations);
    if(utils_string_is_empty_or_spaces(profile->alias_location_max_usage_pct) == false)
      printf("  location_max_usage_pct=%s\n", profile->alias_location_max_usage_pct);
    printf("  criteria codec=%s bits=%s color_space=%s min=%sx%s max=%sx%s\n",
           profile->alias_crit_codec,
           profile->alias_crit_bits,
           profile->alias_crit_color_space,
           profile->alias_crit_min_width,
           profile->alias_crit_min_height,
           profile->alias_crit_max_width,
           profile->alias_crit_max_height);
    printf("  scenario_count=%i\n", scenario_count);
    if(profile->cs_head != NULL) {
      ih_print_preview("  first_scenario=", profile->cs_head->scenario_string);
      ih_print_preview("  first_command=", profile->cs_head->ffmpeg_command);
    }
    profile = profile->next;
  }
}

#ifndef TESTING
int main (int argc, char **argv) {  
  /*immediately revise argc & argv if duplicate words are found in command ---------------*/
  int revised_argc= 0;
  char **revised_argv = utils_remove_duplicates_in_array_of_strings(argv, argc, &revised_argc);
  char config_dir[IH_INPUT_BUFFER_SIZE];
  /*DONE----------------------------------------------------------------------------------*/
  input_handler_t *ih = input_handler_create_new_struct();
  /* add any captured options to options struct*/
  ih_options_parser(revised_argc, revised_argv, ih->options);
  /* add any captured arguments to arguments struct*/
  ih_arguments_parser(revised_argc, revised_argv, ih->arguments);
  //ih is now populated

  /* no options and no arg error handle */
  // vfo [nothing]
  if(revised_argc == 1) {
    ih_mes_user_provided_no_args_and_no_options();
    exit(EXIT_SUCCESS);
  }
  
  /* if an option is detected at all, just respond with the option message*/
  // vfo -h || -help || --h || --help
  if(ih->options->help_detected == true) {
    ih_mes_user_provided_help_option();
    exit(EXIT_SUCCESS);
  }
  // vfo -v || -version || --v || --version
  if(ih->options->version_detected == true) {
    ih_mes_user_provided_version_option();
    exit(EXIT_SUCCESS);
  }

  if(ih->options->open_detected == true && ih->arguments->visualize_detected == false) {
    printf("ERROR: --open is only supported with `vfo visualize`\n");
    exit(EXIT_FAILURE);
  }

  /*Let's handle what argument errors WE CAN HANDLE before running con_init*/
  //vfo revert [no_original]
  if(ih->arguments->revert_detected && !(ih->arguments->mezzanine_detected)) {
    printf("ERROR: You cannot execute revert command without the mezzanine command\n");
    exit(EXIT_FAILURE);
  }

  ih_resolve_config_dir(config_dir, sizeof(config_dir), ih->arguments->wizard_detected == true);

  if(ih->arguments->wizard_detected == true) {
    return ih_run_wizard(config_dir);
  }

  /* initiate config data extraction and validation */
  //extract config file data and unknown words from user input, to config struct
  int suppressed_stdout_fd = -1;

  if(ih->arguments->status_detected == true
     || ih->arguments->status_json_detected == true
     || ih->arguments->visualize_detected == true)
    ih_suppress_stdout_begin(&suppressed_stdout_fd);

  con_set_lenient_location_validation(ih_should_use_lenient_config_validation(ih->arguments));
  config_t *config = con_init(config_dir, revised_argv, revised_argc);
  con_set_lenient_location_validation(false);
  ih_suppress_stdout_end(suppressed_stdout_fd);
  //does argv contain an unknown word << this is actually validated by the con_init, because if an arg doesn't match config pre defined words AND doesn't define an alias mentioned in the config file, we have found an unknown word and the program will stop

  if(ih->arguments->status_detected == true || ih->arguments->status_json_detected == true) {
    bool json_output = ih->arguments->status_json_detected == true;
    bool status_success = ih_run_status_snapshot(config, config_dir, json_output);
    free(config);
    config = NULL;
    return status_success ? EXIT_SUCCESS : EXIT_FAILURE;
  }

  if(ih->arguments->visualize_detected == true) {
    bool visualize_success = ih_run_visualize(config, config_dir, ih->options->open_detected == true);
    free(config);
    config = NULL;
    return visualize_success ? EXIT_SUCCESS : EXIT_FAILURE;
  }

  if(ih->arguments->show_detected == true) {
    ih_show_config(config, config_dir);
    free(config);
    config = NULL;
    return EXIT_SUCCESS;
  }

  if(ih_pipeline_work_requested(ih->arguments, config) == true) {
    if(ih_run_runtime_dependency_precheck() == false) {
      free(config);
      config = NULL;
      return EXIT_FAILURE;
    }
  }

  /*now that we have config, let's now CONTINUE TO HANDLE errors that we can at this stage*/
  //error check 'source' AND config.keep_source = false
  if(ih->arguments->source_detected == true && config->svc->keep_source == false) {
    printf("ERROR: You cannot execute source command because config variable KEEP_SOURCE= is set to false\n");
    exit(EXIT_FAILURE);
  }

  if(ih->arguments->doctor_detected == true) {
    bool doctor_success = ih_run_doctor(config, config_dir);
    free(config);
    config = NULL;
    return doctor_success ? EXIT_SUCCESS : EXIT_FAILURE;
  }

  if(ih->arguments->mezzanine_clean_detected == true) {
    if(ih_run_mezzanine_clean_dependency_precheck() == false) {
      free(config);
      config = NULL;
      return EXIT_FAILURE;
    }
    bool mezzanine_clean_success = ih_execute_mezzanine_clean_for_all_roots(config, true, false);
    free(config);
    config = NULL;
    return mezzanine_clean_success ? EXIT_SUCCESS : EXIT_FAILURE;
  }

  if(ih->arguments->auto_detected == true) {
    bool auto_success = ih_execute_auto(config);
    free(config);
    config = NULL;
    return auto_success ? EXIT_SUCCESS : EXIT_FAILURE;
  }

  if(ih->arguments->run_detected == true) {
    bool run_success = ih_execute_default_run(config);
    free(config);
    config = NULL;
    return run_success ? EXIT_SUCCESS : EXIT_FAILURE;
  }
  //can i validate any more errors at this point?

  //can I now execute things>>

  printf("ih->arguments->mezzanine_detected: %i\n", ih->arguments->mezzanine_detected);
  printf("ih->arguments->revert_detected: %i\n", ih->arguments->revert_detected);
  printf("ih->arguments->source_detected: %i\n", ih->arguments->source_detected);
  printf("ih->arguments->run_detected: %i\n", ih->arguments->run_detected);
  printf("ih->arguments->auto_detected: %i\n", ih->arguments->auto_detected);
  printf("ih->arguments->doctor_detected: %i\n", ih->arguments->doctor_detected);
  printf("ih->arguments->status_detected: %i\n", ih->arguments->status_detected);
  printf("ih->arguments->status_json_detected: %i\n", ih->arguments->status_json_detected);
  printf("ih->arguments->visualize_detected: %i\n", ih->arguments->visualize_detected);
  printf("ih->arguments->wizard_detected: %i\n", ih->arguments->wizard_detected);
  printf("ih->arguments->show_detected: %i\n", ih->arguments->show_detected);
  printf("ih->arguments->mezzanine_clean_detected: %i\n", ih->arguments->mezzanine_clean_detected);
  printf("ih->arguments->wipe_detected: %i\n", ih->arguments->wipe_detected);
  printf("ih->arguments->profiles_detected: %i\n", ih->arguments->profiles_detected);

  //does argv contain the words 'mezzanine' AND 'revert'
  if(ih->arguments->mezzanine_detected == true && ih->arguments->revert_detected == true) {
    printf("DEV: ih logic check = 1\n");
    ih_execute_revert_stage_for_all_roots(config);
  }
  //does argv contain the word 'mezzanine' WITHOUT 'revert'
  else if(ih->arguments->mezzanine_detected == true && ih->arguments->revert_detected == false){
    printf("DEV: ih logic check = 2\n");
    ih_execute_original_stage_for_all_roots(config);
  }
  //does argv contain the word 'source' AND 'wipe'
  if(ih->arguments->source_detected == true && ih->arguments->wipe_detected == true) {
    printf("DEV: ih logic check = 3\n");
    //create a source object and extract from config
    source_t *source = source_create_new_struct(config);
    //execute wipe_source();
    s_wipe_source(source);
    free(source);
    source = NULL;
  }
  //does argv contain the word 'profiles' AND 'wipe'
  if(ih->arguments->profiles_detected == true && ih->arguments->wipe_detected == true) {
    printf("DEV: ih logic check = 4\n");
    //extract relevant data from config>>
    //for every ca_node in config>>
    aliases_t *aliases = NULL;
    for(int i = 0; i < ca_get_count(config->ca_head); i++) {
        //(create and validate an alias struct)
        aliases_t *alias = alias_create_new_struct(config, ca_get_a_node_from_count(config->ca_head, i));
        alias_insert_at_head(&aliases, alias);        
    }
    //now that every alias is validated, execute relevant execution for each alias
    a_wipe_aliases(aliases);
    free(aliases);
    aliases = NULL;
  }
  //does argv contain 1 or many profile words AND word 'wipe'
  if(ih->arguments->wipe_detected == true && uw_get_count(config->uw_head) > 0 && config->svc->keep_source == false) {
    printf("DEV: ih logic check = 5\n");
    //THIS ALSO SHOULD NOT RUN IN CASES AND I NEED TO CHECK THIS CAREFULLY ABOUT UW_COUNT
    aliases_t *aliases = NULL;
    for(int i = 0; i < uw_get_count(config->uw_head); i++) {
      aliases_t *alias = alias_create_new_struct(config, ca_get_a_node_from_alias_name(config->ca_head, uw_get_a_unknown_word_from_count(config->uw_head, i)));
      alias_insert_at_head(&aliases, alias);
    }
    //now that specific aliases are validated, execute relevant execution for each alias
    a_wipe_aliases(aliases);
    free(aliases);
    aliases = NULL;
  }

  //all following commands need to know what the keep_source value is.
  
  //does argv contain the word 'source' WITHOUT 'wipe'
  if(ih->arguments->source_detected == true && ih->arguments->wipe_detected == false && config->svc->keep_source == true) {
    printf("DEV: ih logic check = 7\n");
    ih_execute_source_stage_for_all_original_roots(config);
  }
  //does argv contain the word 'profiles' WITHOUT 'wipe' AND 'config.keep_source_bool == FALSE
  if(ih->arguments->profiles_detected == true && ih->arguments->wipe_detected == false && config->svc->keep_source == false) {
    printf("DEV: ih logic check = 8\n");
    aliases_t *aliases = NULL;
    for(int i = 0; i < ca_get_count(config->ca_head); i++) {
        aliases_t *alias = alias_create_new_struct(config, ca_get_a_node_from_count(config->ca_head, i));
        alias_insert_at_head(&aliases, alias);        
    }
    //now that every alias is validated, execute relevant execution for each alias
    p_mezzanine_to_profiles(aliases);
    free(aliases);
    aliases = NULL;
  }
  //does argv contain the word 'profiles' WITHOUT 'wipe' AND 'config.keep_source_bool == TRUE
  else if(ih->arguments->profiles_detected == true && ih->arguments->wipe_detected == false && config->svc->keep_source == true) {
    printf("DEV: ih logic check = 9\n");
    aliases_t *aliases = NULL;
    // ca_print_list(config->ca_head);
    // printf("ca_get_count: %i\n", ca_get_count(config->ca_head));
    // ca_print_list(ca_get_a_node_from_count(config->ca_head, 0));
    // ca_print_list(ca_get_a_node_from_count(config->ca_head, 1));
    // exit(EXIT_FAILURE);
    for(int i = 0; i < ca_get_count(config->ca_head); i++) {
        aliases_t *alias = alias_create_new_struct(config, ca_get_a_node_from_count(config->ca_head, i));
        alias_insert_at_head(&aliases, alias);        
    }
    //now that every alias is validated, execute relevant execution for each alias
    p_source_to_profiles(aliases);
    free(aliases);
    aliases = NULL;
  }

  //does argv contain 1 or many profile words WITHOUT word 'wipe' AND 'config.keep_source_bool == TRUE
  if (ih->arguments->wipe_detected == false && uw_get_count(config->uw_head) > 0 && config->svc->keep_source == true && config->svc->keep_source == false) {
    //I REALLY NEED TO FIX UW_GET_COUNT logic here, as this runs when proper, so can't rely on count of uw as it is implemented!
    printf("DEV: ih logic check = 12\n");
    aliases_t *aliases = NULL;
    for(int i = 0; i < uw_get_count(config->uw_head); i++) {
      aliases_t *alias = alias_create_new_struct(config, ca_get_a_node_from_alias_name(config->ca_head, uw_get_a_unknown_word_from_count(config->uw_head, i)));
      alias_insert_at_head(&aliases, alias);
    }
    //now that specific aliases are validated, execute relevant execution for each alias
    p_source_to_profiles(aliases);
    free(aliases);
  }
  //does argv contain 1 or many profile words WITHOUT word 'wipe' AND 'config.keep_source_bool == FALSE
  else if(ih->arguments->wipe_detected == false && uw_get_count(config->uw_head) > 0 && config->svc->keep_source == false) {
    printf("DEV: ih logic check = 13\n");
    aliases_t *aliases = NULL;
    for(int i = 0; i < uw_get_count(config->uw_head); i++) {
      aliases_t *alias = alias_create_new_struct(config, ca_get_a_node_from_alias_name(config->ca_head, uw_get_a_unknown_word_from_count(config->uw_head, i)));
      alias_insert_at_head(&aliases, alias);
    }
    //now that specific aliases are validated, execute relevant execution for each alias
    p_mezzanine_to_profiles(aliases);
    free(aliases);
    aliases = NULL;
  }
  
  //free all used memory
  free(config); //<<could probably free things more efficiently based on ordering above also for other objects :)
  config = NULL;
  //successfully exit program
  return EXIT_SUCCESS;
}
#endif

/*when required, place this at the bottom of main function just before EXIT_SUCCESS*/
