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
#include <limits.h>
#include <strings.h>
#include <sys/stat.h>

#define IH_DEFAULT_CONFIG_DIR "/usr/local/bin/vfo_conf_folder"
#define IH_CONFIG_FILENAME "vfo_config.conf"
#define IH_INPUT_BUFFER_SIZE 4096
#define IH_PREVIEW_LIMIT 140

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

static bool ih_run_doctor(config_t *config, const char *config_dir) {
  bool healthy = true;

  printf("DOCTOR ALERT: running environment and configuration checks\n");
  healthy = ih_doctor_check_command("ffmpeg", true) && healthy;
  healthy = ih_doctor_check_command("ffprobe", true) && healthy;
  healthy = ih_doctor_check_command("mkvmerge", true) && healthy;
  healthy = ih_doctor_check_command("dovi_tool", false) && healthy;

  healthy = ih_doctor_check_path("config directory", config_dir, true) && healthy;
  healthy = ih_doctor_check_path("ORIGINAL_LOCATION", config->svc->original_location, true) && healthy;
  healthy = ih_doctor_check_path("SOURCE_LOCATION", config->svc->source_location, true) && healthy;

  if(!(utils_string_is_empty_or_spaces(config->svc->source_as_location)))
    healthy = ih_doctor_check_path("SOURCE_AS_LOCATION", config->svc->source_as_location, false) && healthy;

  if(ca_get_count(config->ca_head) > 0)
    printf("DOCTOR OK: detected %i profile(s) in config\n", ca_get_count(config->ca_head));
  else {
    printf("DOCTOR ERROR: no profiles (aliases) detected in config\n");
    healthy = false;
  }

  printf("DOCTOR INFO: KEEP_SOURCE=%s\n", config->svc->keep_source ? "true" : "false");

  if(healthy) {
    printf("DOCTOR ALERT: checks completed successfully\n");
    return true;
  }
  printf("DOCTOR ALERT: checks completed with errors\n");
  return false;
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

static void ih_execute_default_run(config_t *config) {
  printf("RUN ALERT: initiating default pipeline\n");

  original_t *original = original_create_new_struct(config);
  o_original(original);
  free(original);
  original = NULL;

  if(config->svc->keep_source == true) {
    source_t *source = source_create_new_struct(config);
    s_original_to_source(source);
    free(source);
    source = NULL;
  }

  ih_execute_all_profiles(config);
  printf("RUN ALERT: default pipeline completed successfully\n");
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
  if(utils_does_folder_exist((char *)config_dir))
    return true;

  if(mkdir(config_dir, 0755) == 0)
    return true;

  if(errno == EEXIST)
    return true;

  return false;
}

static int ih_run_wizard(const char *config_dir) {
  char original_location[IH_INPUT_BUFFER_SIZE];
  char source_location[IH_INPUT_BUFFER_SIZE];
  char source_test_trim_start[IH_INPUT_BUFFER_SIZE];
  char source_test_trim_duration[IH_INPUT_BUFFER_SIZE];
  char custom_folder_name[IH_INPUT_BUFFER_SIZE];
  char custom_folder_type[IH_INPUT_BUFFER_SIZE];
  char alias_name[IH_INPUT_BUFFER_SIZE];
  char alias_upper[IH_INPUT_BUFFER_SIZE];
  char alias_location[IH_INPUT_BUFFER_SIZE];
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
  char *config_path = NULL;
  FILE *config_file = NULL;
  bool keep_source = true;
  bool source_test_active = false;

  printf("WIZARD ALERT: guided setup for %s\n", IH_CONFIG_FILENAME);
  printf("WIZARD INFO: press Enter to accept defaults\n");

  if(!ih_prompt_line("Original location", "/Volumes/Media/original", original_location, sizeof(original_location), true))
    return EXIT_FAILURE;

  if(!ih_prompt_line("Source location", "/Volumes/Media/source", source_location, sizeof(source_location), true))
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

  if(!ih_prompt_line("First library folder name", "Movies", custom_folder_name, sizeof(custom_folder_name), true))
    return EXIT_FAILURE;

  while(true) {
    if(!ih_prompt_line("First library folder type (films|tv)", "films", custom_folder_type, sizeof(custom_folder_type), true))
      return EXIT_FAILURE;
    if(strcasecmp(custom_folder_type, "films") == 0 || strcasecmp(custom_folder_type, "tv") == 0)
      break;
    printf("WIZARD INFO: folder type must be 'films' or 'tv'\n");
  }

  if(!ih_prompt_line("Profile name", "netflixy_open_audio_1080p", alias_name, sizeof(alias_name), true))
    return EXIT_FAILURE;
  ih_sanitize_alias_name(alias_name, sizeof(alias_name));
  printf("WIZARD INFO: profile key set to '%s'\n", alias_name);

  snprintf(default_alias_location, sizeof(default_alias_location), "%s/%s", source_location, alias_name);
  if(!ih_prompt_line("Profile output location", default_alias_location, alias_location, sizeof(alias_location), true))
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

  ih_copy_string(alias_upper, sizeof(alias_upper), alias_name);
  ih_uppercase_in_place(alias_upper);

  fprintf(config_file, "/* vfo config generated by `vfo wizard` */\n\n");
  fprintf(config_file, "ORIGINAL_LOCATION=\"%s\"\n", original_location);
  fprintf(config_file, "SOURCE_LOCATION=\"%s\"\n", source_location);
  fprintf(config_file, "KEEP_SOURCE=\"%s\"\n\n", keep_source ? "true" : "false");
  fprintf(config_file, "SOURCE_TEST_ACTIVE=\"%s\"\n", source_test_active ? "true" : "false");
  fprintf(config_file, "SOURCE_TEST_TRIM_START=\"%s\"\n", source_test_trim_start);
  fprintf(config_file, "SOURCE_TEST_TRIM_DURATION=\"%s\"\n\n", source_test_trim_duration);
  fprintf(config_file, "CUSTOM_FOLDER=\"%s,%s\"\n\n", custom_folder_name, custom_folder_type);
  fprintf(config_file, "ALIAS=\"%s\"\n", alias_name);
  fprintf(config_file, "%s_LOCATION=\"%s\"\n", alias_upper, alias_location);
  fprintf(config_file, "%s_CRITERIA_CODEC_NAME=\"%s\"\n", alias_upper, alias_crit_codec);
  fprintf(config_file, "%s_CRITERIA_BITS=\"%s\"\n", alias_upper, alias_crit_bits);
  fprintf(config_file, "%s_CRITERIA_COLOR_SPACE=\"%s\"\n", alias_upper, alias_crit_color_space);
  fprintf(config_file, "%s_CRITERIA_RESOLUTION_MIN_WIDTH=\"%s\"\n", alias_upper, alias_crit_min_width);
  fprintf(config_file, "%s_CRITERIA_RESOLUTION_MIN_HEIGHT=\"%s\"\n", alias_upper, alias_crit_min_height);
  fprintf(config_file, "%s_CRITERIA_RESOLUTION_MAX_WIDTH=\"%s\"\n", alias_upper, alias_crit_max_width);
  fprintf(config_file, "%s_CRITERIA_RESOLUTION_MAX_HEIGHT=\"%s\"\n", alias_upper, alias_crit_max_height);
  fprintf(config_file, "%s_SCENARIO=\"%s\"\n", alias_upper, alias_scenario);
  fprintf(config_file, "%s_FFMPEG_COMMAND=\"%s\"\n", alias_upper, alias_ffmpeg_command);

  fclose(config_file);

  printf("WIZARD ALERT: config written to %s\n", config_path);
  printf("WIZARD NEXT: run `vfo show`, then `vfo doctor`, then `vfo run`\n");

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
  printf("ORIGINAL_LOCATION=%s\n", config->svc->original_location);
  printf("SOURCE_LOCATION=%s\n", config->svc->source_location);
  printf("KEEP_SOURCE=%s\n", config->svc->keep_source ? "true" : "false");
  printf("SOURCE_TEST_ACTIVE=%s\n", config->svc->source_test ? "true" : "false");
  if(config->svc->source_test) {
    printf("SOURCE_TEST_TRIM_START=%s\n", config->svc->source_test_trim_start);
    printf("SOURCE_TEST_TRIM_DURATION=%s\n", config->svc->source_test_trim_duration);
  }

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
  const char *config_dir = IH_DEFAULT_CONFIG_DIR;
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
    printf("ERROR: You cannot execute revert command without the original command\n");
    exit(EXIT_FAILURE);
  }

  if(ih->arguments->wizard_detected == true) {
    return ih_run_wizard(config_dir);
  }
  // if(ih->arguments->do_it_all_detected && any other arguments present) {
  //   printf("Error, do_it_all cannot be executed along side your other commands\n");
  //   printf("do_it_all cannot work in conjuction with other vfo commands.\n");
  //   exit(EXIT_FAILURE);
  // }
  // if(ih->arguments->all_aliases_detected && any other alias arguments detected) {
  //   printf("Error, all_aliases cannot be executed along side your other commands\n");
  //   printf("all_aliases cannot work in conjuction with other alias related vfo commands.\n");
  //   exit(EXIT_FAILURE);
  // }

  /* initiate config data extraction and validation */
  //extract config file data and unknown words from user input, to config struct
  config_t *config = con_init(config_dir, revised_argv, revised_argc);
  //does argv contain an unknown word << this is actually validated by the con_init, because if an arg doesn't match config pre defined words AND doesn't define an alias mentioned in the config file, we have found an unknown word and the program will stop

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

  if(ih->arguments->run_detected == true) {
    ih_execute_default_run(config);
    free(config);
    config = NULL;
    return EXIT_SUCCESS;
  }
  //can i validate any more errors at this point?

  //can I now execute things>>

  printf("ih->arguments->original_detected: %i\n", ih->arguments->original_detected);
  printf("ih->arguments->revert_detected: %i\n", ih->arguments->revert_detected);
  printf("ih->arguments->source_detected: %i\n", ih->arguments->source_detected);
  printf("ih->arguments->run_detected: %i\n", ih->arguments->run_detected);
  printf("ih->arguments->doctor_detected: %i\n", ih->arguments->doctor_detected);
  printf("ih->arguments->wizard_detected: %i\n", ih->arguments->wizard_detected);
  printf("ih->arguments->show_detected: %i\n", ih->arguments->show_detected);
  printf("ih->arguments->wipe_detected: %i\n", ih->arguments->wipe_detected);
  printf("ih->arguments->alias_queue_detected: %i\n", ih->arguments->alias_queue_detected);
  printf("ih->arguments->all_aliases_detected: %i\n", ih->arguments->all_aliases_detected);
  printf("ih->arguments->do_it_all_detected: %i\n", ih->arguments->do_it_all_detected);

  //does argv contain the words 'original' AND 'revert'
  if(ih->arguments->original_detected == true && ih->arguments->revert_detected == true) {
    printf("DEV: ih logic check = 1\n");
    //create a original object and point to relevant data in config
    original_t *original = original_create_new_struct(config);
    //execute revert_original_to_start();
    o_revert_to_start(original);
    free(original);
    original = NULL;
  }
  //does argv contain the word 'original' WITHOUT 'revert'
  else if(ih->arguments->original_detected == true && ih->arguments->revert_detected == false){
    printf("DEV: ih logic check = 2\n");
    //create a original object and point to relevant data in config
    original_t *original = original_create_new_struct(config);
    //execute execute_original();
    o_original(original);
    free(original);
    original = NULL;
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
  //does argv contain the word 'all_aliases' AND 'wipe'
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
  //does argv contain 1 or many alias words AND word 'wipe'
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
    //create a source object and point it to relevant data in config
    source_t *source = source_create_new_struct(config);
    //execute encode_original_to_source_();
    s_original_to_source(source);
    free(source);
    source = NULL;
  }
  //does argv contain the word 'all_aliases' WITHOUT 'wipe' AND 'config.keep_source_bool == FALSE 
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
  //does argv contain the word 'all_aliases' WITHOUT 'wipe' AND 'config.keep_source_bool == TRUE 
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
    //create a original object and point it to relevant data in config
    original_t *original = original_create_new_struct(config);
    //execute execute_original();
    o_original(original);
    free(original);
    original = NULL;
    //create a source object and point it to relevant data in config
    source_t *source = source_create_new_struct(config);
    //execute encode_original_to_source_();
    s_original_to_source(source);
    free(source);
    source = NULL;

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
    //create a original object and point it to relevant data in config
    original_t *original = original_create_new_struct(config);
    //execute execute_original();
    o_original(original);
    free(original);
    original = NULL;
   
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
  //does argv contain 1 or many alias words WITHOUT word 'wipe' AND 'config.keep_source_bool == TRUE
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
  //does argv contain 1 or many alias words WITHOUT word 'wipe' AND 'config.keep_source_bool == FALSE
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
