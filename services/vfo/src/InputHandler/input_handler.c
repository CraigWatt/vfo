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

#define IH_LEGACY_CONFIG_DIR "/usr/local/bin/vfo_conf_folder"
#define IH_CONFIG_DIR_ENV "VFO_CONFIG_DIR"
#define IH_CONFIG_FILENAME "vfo_config.conf"
#define IH_INPUT_BUFFER_SIZE 4096
#define IH_PREVIEW_LIMIT 140

typedef struct ih_stock_preset_option {
  const char *key;
  const char *display_name;
  const char *relative_path;
} ih_stock_preset_option_t;

static const ih_stock_preset_option_t IH_STOCK_PRESET_OPTIONS[] = {
  {"balanced_open_audio", "Balanced Open Audio", "balanced_open_audio/vfo_config.preset.conf"},
  {"device_targets_open_audio", "Device Targets Open Audio", "device_targets_open_audio/vfo_config.preset.conf"},
  {"netflixy_main_subtitle_intent", "Netflixy Main Subtitle Intent", "netflixy_main_subtitle_intent/vfo_config.preset.conf"}
};

#define IH_STOCK_PRESET_COUNT ((int)(sizeof(IH_STOCK_PRESET_OPTIONS) / sizeof(IH_STOCK_PRESET_OPTIONS[0])))

static int ih_cs_count(cs_node_t *cs_head);

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

static bool ih_command_exists(const char *command_name) {
  char shell_command[256];
  snprintf(shell_command, sizeof(shell_command), "command -v %s >/dev/null 2>&1", command_name);
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

static bool ih_doctor_check_command(const char *command_name, bool required) {
  return ih_check_command("DOCTOR", command_name, required);
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

  printf("DOCTOR ALERT: running environment and configuration checks\n");
  healthy = ih_doctor_check_command("ffmpeg", true) && healthy;
  healthy = ih_doctor_check_command("ffprobe", true) && healthy;
  healthy = ih_doctor_check_command("mkvmerge", true) && healthy;
  healthy = ih_doctor_check_command("dovi_tool", false) && healthy;

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

  if(config->svc->quality_check_enabled && config->svc->quality_check_include_vmaf) {
    if(ih_ffmpeg_filter_available("libvmaf"))
      printf("DOCTOR OK: ffmpeg libvmaf filter is available\n");
    else {
      printf("DOCTOR WARN: ffmpeg libvmaf filter is unavailable; VMAF scoring will be skipped\n");
      healthy = false;
    }
  }

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

static bool ih_run_status_snapshot(config_t *config, const char *config_dir, bool json_output) {
  status_report_t *report = status_report_create();
  bool healthy = true;
  bool originals_healthy = true;
  bool source_healthy = true;
  bool profiles_healthy = true;

  if(report == NULL) {
    printf("STATUS ERROR: could not allocate status report\n");
    return false;
  }

  status_report_update(report, "engine.snapshot", STATUS_STATE_IN_PROGRESS, "collecting component status");

  if(utils_does_folder_exist((char *)config_dir))
    status_report_update(report, "config.directory", STATUS_STATE_COMPLETE, config_dir);
  else {
    status_report_update(report, "config.directory", STATUS_STATE_ERROR, "config directory missing");
    healthy = false;
  }

  healthy = ih_status_check_command(report, "ffmpeg", true) && healthy;
  healthy = ih_status_check_command(report, "ffprobe", true) && healthy;
  healthy = ih_status_check_command(report, "mkvmerge", true) && healthy;
  healthy = ih_status_check_command(report, "dovi_tool", false) && healthy;
  if(config->svc->quality_check_enabled && config->svc->quality_check_include_vmaf) {
    if(ih_ffmpeg_filter_available("libvmaf")) {
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

  status_report_update(report, "stage.execute", STATUS_STATE_PENDING, "run `vfo run` to execute the pipeline");

  status_report_update(report,
                       "engine.snapshot",
                       healthy ? STATUS_STATE_COMPLETE : STATUS_STATE_ERROR,
                       healthy ? "snapshot completed successfully" : "snapshot completed with errors");

  if(json_output)
    status_report_print_json(report, stdout);
  else
    status_report_print_text(report, stdout);

  healthy = status_report_is_healthy(report);
  status_report_free(report);
  return healthy;
}

static bool ih_run_runtime_dependency_precheck() {
  bool healthy = true;

  printf("PRECHECK ALERT: validating required commands before execution\n");
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

  printf("PRECHECK ALERT: validating mezzanine-clean dependencies\n");
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

  if(arguments->run_detected)
    return true;

  if(arguments->original_detected && arguments->revert_detected == false)
    return true;

  if(arguments->source_detected && arguments->wipe_detected == false)
    return true;

  if(arguments->all_aliases_detected && arguments->wipe_detected == false)
    return true;

  if(arguments->do_it_all_detected && arguments->wipe_detected == false)
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
      || arguments->status_json_detected;
}

static void ih_execute_all_profiles(config_t *config) {
  aliases_t *aliases = NULL;
  for(int i = 0; i < ca_get_count(config->ca_head); i++) {
    aliases_t *alias = alias_create_new_struct(config, ca_get_a_node_from_count(config->ca_head, i));
    alias_insert_at_head(&aliases, alias);
  }

  if(config->svc->keep_source == true)
    a_source_to_aliases(aliases);
  else
    a_original_to_aliases(aliases);

  free(aliases);
  aliases = NULL;
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
    o_original(original);
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
    o_revert_to_start(original);
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
    s_original_to_source(source);
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
      for(int i = 0; i < IH_STOCK_PRESET_COUNT; i++) {
        if(strcasecmp(token, IH_STOCK_PRESET_OPTIONS[i].key) == 0) {
          selected_index = i;
          break;
        }
      }
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
  char original_location[IH_INPUT_BUFFER_SIZE];
  char original_locations[IH_INPUT_BUFFER_SIZE];
  char original_location_max_usage_pct[IH_INPUT_BUFFER_SIZE];
  char source_location[IH_INPUT_BUFFER_SIZE];
  char source_locations[IH_INPUT_BUFFER_SIZE];
  char source_location_max_usage_pct[IH_INPUT_BUFFER_SIZE];
  char source_test_trim_start[IH_INPUT_BUFFER_SIZE];
  char source_test_trim_duration[IH_INPUT_BUFFER_SIZE];
  char custom_folder_name[IH_INPUT_BUFFER_SIZE];
  char custom_folder_type[IH_INPUT_BUFFER_SIZE];
  char alias_name[IH_INPUT_BUFFER_SIZE];
  char alias_upper[IH_INPUT_BUFFER_SIZE];
  char alias_location[IH_INPUT_BUFFER_SIZE];
  char alias_locations[IH_INPUT_BUFFER_SIZE];
  char alias_location_max_usage_pct[IH_INPUT_BUFFER_SIZE];
  char alias_crit_codec[IH_INPUT_BUFFER_SIZE];
  char alias_crit_bits[IH_INPUT_BUFFER_SIZE];
  char alias_crit_color_space[IH_INPUT_BUFFER_SIZE];
  char alias_crit_min_width[IH_INPUT_BUFFER_SIZE];
  char alias_crit_min_height[IH_INPUT_BUFFER_SIZE];
  char alias_crit_max_width[IH_INPUT_BUFFER_SIZE];
  char alias_crit_max_height[IH_INPUT_BUFFER_SIZE];
  char alias_scenario[IH_INPUT_BUFFER_SIZE];
  char alias_ffmpeg_command[IH_INPUT_BUFFER_SIZE];
  char default_alias_location[IH_INPUT_BUFFER_SIZE];
  char default_original_caps[IH_INPUT_BUFFER_SIZE];
  char default_source_caps[IH_INPUT_BUFFER_SIZE];
  char default_alias_caps[IH_INPUT_BUFFER_SIZE];
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

  printf("WIZARD ALERT: guided setup for %s\n", IH_CONFIG_FILENAME);
  printf("WIZARD INFO: press Enter to accept defaults\n");
  printf("WIZARD INFO: target config directory: %s\n", config_dir);

  if(!ih_prompt_line("Mezzanine location", "/Volumes/Media/mezzanine", original_location, sizeof(original_location), true))
    return EXIT_FAILURE;
  if(!ih_prompt_line("Mezzanine locations (semicolon list)", original_location, original_locations, sizeof(original_locations), true))
    return EXIT_FAILURE;
  ih_build_default_cap_list(original_locations, original_location_max_usage_pct, sizeof(original_location_max_usage_pct));
  ih_copy_string(default_original_caps, sizeof(default_original_caps), original_location_max_usage_pct);
  if(!ih_prompt_line("Mezzanine max usage pct list (semicolon, 1-100)", default_original_caps, original_location_max_usage_pct, sizeof(original_location_max_usage_pct), true))
    return EXIT_FAILURE;
  if(!ih_first_location_from_list(original_locations, original_location, sizeof(original_location)))
    return EXIT_FAILURE;

  if(!ih_prompt_line("Source location", "/Volumes/Media/source", source_location, sizeof(source_location), true))
    return EXIT_FAILURE;
  if(!ih_prompt_line("Source locations (semicolon list)", source_location, source_locations, sizeof(source_locations), true))
    return EXIT_FAILURE;
  ih_build_default_cap_list(source_locations, source_location_max_usage_pct, sizeof(source_location_max_usage_pct));
  ih_copy_string(default_source_caps, sizeof(default_source_caps), source_location_max_usage_pct);
  if(!ih_prompt_line("Source max usage pct list (semicolon, 1-100)", default_source_caps, source_location_max_usage_pct, sizeof(source_location_max_usage_pct), true))
    return EXIT_FAILURE;
  if(!ih_first_location_from_list(source_locations, source_location, sizeof(source_location)))
    return EXIT_FAILURE;

  if(!ih_prompt_bool("Keep source outputs?", true, &keep_source))
    return EXIT_FAILURE;

  if(!ih_prompt_bool("Enable source test clipping?", false, &source_test_active))
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

  if(!ih_prompt_bool("Enable mezzanine-clean hygiene checks?", false, &mezzanine_clean_enabled))
    return EXIT_FAILURE;
  if(!ih_prompt_bool("Apply mezzanine-clean renames/moves automatically?", false, &mezzanine_clean_apply_changes))
    return EXIT_FAILURE;
  if(!ih_prompt_bool("Append probe-derived media tags to mezzanine filenames?", true, &mezzanine_clean_append_media_tags))
    return EXIT_FAILURE;
  if(!ih_prompt_bool("Fail mezzanine-clean when warnings are found (strict gate)?", false, &mezzanine_clean_strict_quality_gate))
    return EXIT_FAILURE;

  if(!ih_prompt_bool("Enable post-profile quality scoring (PSNR/SSIM)?", false, &quality_check_enabled))
    return EXIT_FAILURE;
  if(!ih_prompt_bool("Include VMAF in quality scoring (requires ffmpeg libvmaf)?", false, &quality_check_include_vmaf))
    return EXIT_FAILURE;
  if(!ih_prompt_bool("Fail run when quality checks fail (strict gate)?", false, &quality_check_strict_gate))
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

  if(!ih_prompt_line("First library folder name", "Movies", custom_folder_name, sizeof(custom_folder_name), true))
    return EXIT_FAILURE;

  while(true) {
    if(!ih_prompt_line("First library folder type (films|tv)", "films", custom_folder_type, sizeof(custom_folder_type), true))
      return EXIT_FAILURE;
    if(strcasecmp(custom_folder_type, "films") == 0 || strcasecmp(custom_folder_type, "tv") == 0)
      break;
    printf("WIZARD INFO: folder type must be 'films' or 'tv'\n");
  }

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

  if(use_stock_presets) {
    ih_print_stock_preset_menu();
    while(true) {
      if(!ih_prompt_line("Select stock preset packs (comma list, names, or all)", "all", stock_preset_selection, sizeof(stock_preset_selection), true))
        return EXIT_FAILURE;
      if(ih_parse_stock_preset_selection(stock_preset_selection, selected_stock_presets, &selected_stock_preset_count))
        break;
      printf("WIZARD INFO: choose one or more presets, e.g. 1,3 or all\n");
    }

    if(!ih_prompt_line("Profiles output base location", "/Volumes/Media/profiles", stock_profile_location, sizeof(stock_profile_location), true))
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
    if(!ih_prompt_line("Profile name", "netflixy_preserve_audio_main_subtitle_intent_1080p", alias_name, sizeof(alias_name), true))
      return EXIT_FAILURE;
    ih_sanitize_alias_name(alias_name, sizeof(alias_name));
    printf("WIZARD INFO: profile key set to '%s'\n", alias_name);

    snprintf(default_alias_location, sizeof(default_alias_location), "%s/%s", source_location, alias_name);
    if(!ih_prompt_line("Profile output location", default_alias_location, alias_location, sizeof(alias_location), true))
      return EXIT_FAILURE;
    if(!ih_prompt_line("Profile output locations (semicolon list)", alias_location, alias_locations, sizeof(alias_locations), true))
      return EXIT_FAILURE;
    ih_build_default_cap_list(alias_locations, alias_location_max_usage_pct, sizeof(alias_location_max_usage_pct));
    ih_copy_string(default_alias_caps, sizeof(default_alias_caps), alias_location_max_usage_pct);
    if(!ih_prompt_line("Profile max usage pct list (semicolon, 1-100)", default_alias_caps, alias_location_max_usage_pct, sizeof(alias_location_max_usage_pct), true))
      return EXIT_FAILURE;
    if(!ih_first_location_from_list(alias_locations, alias_location, sizeof(alias_location)))
      return EXIT_FAILURE;

    if(!ih_prompt_line("Profile criteria codec", "h264", alias_crit_codec, sizeof(alias_crit_codec), true))
      return EXIT_FAILURE;
    if(!ih_prompt_line("Profile criteria bits", "8", alias_crit_bits, sizeof(alias_crit_bits), true))
      return EXIT_FAILURE;
    if(!ih_prompt_line("Profile criteria color space", "bt709", alias_crit_color_space, sizeof(alias_crit_color_space), true))
      return EXIT_FAILURE;
    if(!ih_prompt_line("Profile minimum width", "352", alias_crit_min_width, sizeof(alias_crit_min_width), true))
      return EXIT_FAILURE;
    if(!ih_prompt_line("Profile minimum height", "240", alias_crit_min_height, sizeof(alias_crit_min_height), true))
      return EXIT_FAILURE;
    if(!ih_prompt_line("Profile maximum width", "1920", alias_crit_max_width, sizeof(alias_crit_max_width), true))
      return EXIT_FAILURE;
    if(!ih_prompt_line("Profile maximum height", "1080", alias_crit_max_height, sizeof(alias_crit_max_height), true))
      return EXIT_FAILURE;

    if(!ih_prompt_line("Profile scenario", "ELSE", alias_scenario, sizeof(alias_scenario), true))
      return EXIT_FAILURE;

    if(!ih_prompt_line("Profile ffmpeg command", "ffmpeg -nostdin -y -i $vfo_input -map 0:v:0 -map 0:a? -map 0:s? -c:v libx264 -preset medium -crf 19 -pix_fmt yuv420p -c:a copy -c:s copy -movflags +faststart $vfo_output", alias_ffmpeg_command, sizeof(alias_ffmpeg_command), true))
      return EXIT_FAILURE;

    if(strstr(alias_ffmpeg_command, "$vfo_input") == NULL || strstr(alias_ffmpeg_command, "$vfo_output") == NULL) {
      printf("WIZARD ERROR: ffmpeg command must include both $vfo_input and $vfo_output\n");
      return EXIT_FAILURE;
    }
  }

  if(!ih_create_config_dir_if_needed(config_dir)) {
    printf("WIZARD ERROR: could not create config directory: %s\n", config_dir);
    printf("WIZARD INFO: if this path requires elevated permissions, run wizard with sudo\n");
    return EXIT_FAILURE;
  }

  config_path = utils_combine_to_full_path(config_dir, IH_CONFIG_FILENAME);
  config_file = fopen(config_path, "w");
  if(config_file == NULL) {
    printf("WIZARD ERROR: could not write config file: %s\n", config_path);
    printf("WIZARD INFO: if this path requires elevated permissions, run wizard with sudo\n");
    free(config_path);
    return EXIT_FAILURE;
  }

  fprintf(config_file, "/* vfo config generated by `vfo wizard` */\n\n");
  fprintf(config_file, "MEZZANINE_LOCATION=\"%s\"\n", original_location);
  fprintf(config_file, "MEZZANINE_LOCATIONS=\"%s\"\n", original_locations);
  fprintf(config_file, "MEZZANINE_LOCATION_MAX_USAGE_PCT=\"%s\"\n", original_location_max_usage_pct);
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
      remove(config_path);
      printf("WIZARD ERROR: failed to append selected stock presets\n");
      free(config_path);
      return EXIT_FAILURE;
    }
  } else {
    ih_copy_string(alias_upper, sizeof(alias_upper), alias_name);
    ih_uppercase_in_place(alias_upper);

    fprintf(config_file, "PROFILE=\"%s\"\n", alias_name);
    fprintf(config_file, "%s_LOCATION=\"%s\"\n", alias_upper, alias_location);
    fprintf(config_file, "%s_LOCATIONS=\"%s\"\n", alias_upper, alias_locations);
    fprintf(config_file, "%s_LOCATION_MAX_USAGE_PCT=\"%s\"\n", alias_upper, alias_location_max_usage_pct);
    fprintf(config_file, "%s_CRITERIA_CODEC_NAME=\"%s\"\n", alias_upper, alias_crit_codec);
    fprintf(config_file, "%s_CRITERIA_BITS=\"%s\"\n", alias_upper, alias_crit_bits);
    fprintf(config_file, "%s_CRITERIA_COLOR_SPACE=\"%s\"\n", alias_upper, alias_crit_color_space);
    fprintf(config_file, "%s_CRITERIA_RESOLUTION_MIN_WIDTH=\"%s\"\n", alias_upper, alias_crit_min_width);
    fprintf(config_file, "%s_CRITERIA_RESOLUTION_MIN_HEIGHT=\"%s\"\n", alias_upper, alias_crit_min_height);
    fprintf(config_file, "%s_CRITERIA_RESOLUTION_MAX_WIDTH=\"%s\"\n", alias_upper, alias_crit_max_width);
    fprintf(config_file, "%s_CRITERIA_RESOLUTION_MAX_HEIGHT=\"%s\"\n", alias_upper, alias_crit_max_height);
    fprintf(config_file, "%s_SCENARIO=\"%s\"\n", alias_upper, alias_scenario);
    fprintf(config_file, "%s_FFMPEG_COMMAND=\"%s\"\n", alias_upper, alias_ffmpeg_command);
  }

  fclose(config_file);

  printf("WIZARD ALERT: config written to %s\n", config_path);
  if(use_stock_presets) {
    printf("WIZARD INFO: selected stock preset packs: %i\n", selected_stock_preset_count);
  }
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

  /*Let's handle what argument errors WE CAN HANDLE before running con_init*/
  //vfo revert [no_original]
  if(ih->arguments->revert_detected && !(ih->arguments->original_detected)) {
    printf("ERROR: You cannot execute revert command without the mezzanine command\n");
    exit(EXIT_FAILURE);
  }

  ih_resolve_config_dir(config_dir, sizeof(config_dir), ih->arguments->wizard_detected == true);

  if(ih->arguments->wizard_detected == true) {
    return ih_run_wizard(config_dir);
  }
  // if(ih->arguments->do_it_all_detected && any other arguments present) {
  //   printf("Error, do_it_all cannot be executed along side your other commands\n");
  //   printf("do_it_all cannot work in conjuction with other vfo commands.\n");
  //   exit(EXIT_FAILURE);
  // }
  // if(ih->arguments->all_aliases_detected && any other profile arguments detected) {
  //   printf("Error, profiles cannot be executed along side your other commands\n");
  //   printf("profiles cannot work in conjuction with other profile related vfo commands.\n");
  //   exit(EXIT_FAILURE);
  // }

  /* initiate config data extraction and validation */
  //extract config file data and unknown words from user input, to config struct
  int suppressed_stdout_fd = -1;

  if(ih->arguments->status_detected == true || ih->arguments->status_json_detected == true)
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

  if(ih->arguments->run_detected == true) {
    bool run_success = ih_execute_default_run(config);
    free(config);
    config = NULL;
    return run_success ? EXIT_SUCCESS : EXIT_FAILURE;
  }
  //can i validate any more errors at this point?

  //can I now execute things>>

  printf("ih->arguments->mezzanine_detected: %i\n", ih->arguments->original_detected);
  printf("ih->arguments->revert_detected: %i\n", ih->arguments->revert_detected);
  printf("ih->arguments->source_detected: %i\n", ih->arguments->source_detected);
  printf("ih->arguments->run_detected: %i\n", ih->arguments->run_detected);
  printf("ih->arguments->doctor_detected: %i\n", ih->arguments->doctor_detected);
  printf("ih->arguments->status_detected: %i\n", ih->arguments->status_detected);
  printf("ih->arguments->status_json_detected: %i\n", ih->arguments->status_json_detected);
  printf("ih->arguments->wizard_detected: %i\n", ih->arguments->wizard_detected);
  printf("ih->arguments->show_detected: %i\n", ih->arguments->show_detected);
  printf("ih->arguments->mezzanine_clean_detected: %i\n", ih->arguments->mezzanine_clean_detected);
  printf("ih->arguments->wipe_detected: %i\n", ih->arguments->wipe_detected);
  printf("ih->arguments->profile_queue_detected: %i\n", ih->arguments->alias_queue_detected);
  printf("ih->arguments->profiles_detected: %i\n", ih->arguments->all_aliases_detected);
  printf("ih->arguments->do_it_all_detected: %i\n", ih->arguments->do_it_all_detected);

  //does argv contain the words 'mezzanine' AND 'revert'
  if(ih->arguments->original_detected == true && ih->arguments->revert_detected == true) {
    printf("DEV: ih logic check = 1\n");
    ih_execute_revert_stage_for_all_roots(config);
  }
  //does argv contain the word 'mezzanine' WITHOUT 'revert'
  else if(ih->arguments->original_detected == true && ih->arguments->revert_detected == false){
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
  if(ih->arguments->all_aliases_detected == true && ih->arguments->wipe_detected == true) {
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
  if(ih->arguments->all_aliases_detected == true && ih->arguments->wipe_detected == false && config->svc->keep_source == false) {
    printf("DEV: ih logic check = 8\n");
    aliases_t *aliases = NULL;
    for(int i = 0; i < ca_get_count(config->ca_head); i++) {
        aliases_t *alias = alias_create_new_struct(config, ca_get_a_node_from_count(config->ca_head, i));
        alias_insert_at_head(&aliases, alias);        
    }
    //now that every alias is validated, execute relevant execution for each alias
    a_original_to_aliases(aliases);
    free(aliases);
    aliases = NULL;
  }
  //does argv contain the word 'profiles' WITHOUT 'wipe' AND 'config.keep_source_bool == TRUE
  else if(ih->arguments->all_aliases_detected == true && ih->arguments->wipe_detected == false && config->svc->keep_source == true) {
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
    a_source_to_aliases(aliases);
    free(aliases);
    aliases = NULL;
  }

  //does argv contain the word 'do_it_all' WITHOUT 'wipe' AND 'config.keep_source_bool == TRUE
  if(ih->arguments->do_it_all_detected == true && ih->arguments->wipe_detected == false && config->svc->keep_source == true) {
    printf("DEV: ih logic check = 10\n");
    ih_execute_original_stage_for_all_roots(config);
    ih_execute_source_stage_for_all_original_roots(config);

    aliases_t *aliases = NULL;
    for(int i = 0; i < uw_get_count(config->uw_head); i++) {
      aliases_t *alias = alias_create_new_struct(config, ca_get_a_node_from_alias_name(config->ca_head, uw_get_a_unknown_word_from_count(config->uw_head, i)));
      alias_insert_at_head(&aliases, alias);
    }
    //now that specific aliases are validated, execute relevant execution for each alias
    a_source_to_aliases(aliases);
    free(aliases);
    aliases = NULL;
  }
  //does argv contain the word 'do_it_all' WITHOUT 'wipe' AND 'config.keep_source_bool == FALSE 
  else if(ih->arguments->do_it_all_detected == true && ih->arguments->wipe_detected == false && config->svc->keep_source == false) {
    printf("DEV: ih logic check = 11\n");
    ih_execute_original_stage_for_all_roots(config);
   
    aliases_t *aliases = NULL;
    for(int i = 0; i < uw_get_count(config->uw_head); i++) {
      aliases_t *alias = alias_create_new_struct(config, ca_get_a_node_from_alias_name(config->ca_head, uw_get_a_unknown_word_from_count(config->uw_head, i)));
      alias_insert_at_head(&aliases, alias);
    }
    //now that specific aliases are validated, execute relevant execution for each alias
    a_original_to_aliases(aliases);
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
    a_source_to_aliases(aliases);
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
    a_original_to_aliases(aliases);
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
// #ifdef DEBUG
  //     /*Options*/
  //     fprintf(stdout, BLUE "Command line options:\n" NO_COLOR);
  //     fprintf(stdout, BROWN "help: %d\n" NO_COLOR, options.help);
  //     fprintf(stdout, BROWN "version: %d\n" NO_COLOR, options.version);
  //     fprintf(stdout, BROWN "use colors: %d\n" NO_COLOR, options.use_colors);
  //     /*Arguments*/
  //     fprintf(stdout, BLUE "Command line arguments:\n" NO_COLOR);
  //     fprintf(stdout, BROWN "original: %d\n" NO_COLOR, arguments.original);
  //     fprintf(stdout, BROWN "source: %d\n" NO_COLOR, arguments.source);
  //     fprintf(stdout, BROWN "joker: %d\n" NO_COLOR, arguments.joker);
  //     fprintf(stdout, BROWN "custom_alias: %d\n" NO_COLOR, arguments.custom_alias);
  //     fprintf(stdout, BROWN "do_it_all: %d\n" NO_COLOR, arguments.do_it_all);
  // #endif
