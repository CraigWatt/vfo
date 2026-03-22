#include "c_svm_struct.h"

sole_var_markers_t* svm_create_new_struct() {
  sole_var_markers_t *result = malloc(sizeof(sole_var_markers_t));
  result->original_location = "ORIGINAL_LOCATION=";
  result->original_locations = "ORIGINAL_LOCATIONS=";
  result->original_location_max_usage_pct = "ORIGINAL_LOCATION_MAX_USAGE_PCT=";
  result->source_location = "SOURCE_LOCATION=";
  result->source_locations = "SOURCE_LOCATIONS=";
  result->source_location_max_usage_pct = "SOURCE_LOCATION_MAX_USAGE_PCT=";
  result->keep_source = "KEEP_SOURCE=";

  result->source_test_active_marker = "SOURCE_TEST_ACTIVE=";
  result->source_test_trim_start_marker = "SOURCE_TEST_TRIM_START=";
  result->source_test_trim_duration_marker = "SOURCE_TEST_TRIM_DURATION=";
  result->mezzanine_clean_enabled_marker = "MEZZANINE_CLEAN_ENABLED=";
  result->mezzanine_clean_apply_changes_marker = "MEZZANINE_CLEAN_APPLY_CHANGES=";
  result->mezzanine_clean_append_media_tags_marker = "MEZZANINE_CLEAN_APPEND_MEDIA_TAGS=";
  result->mezzanine_clean_strict_quality_gate_marker = "MEZZANINE_CLEAN_STRICT_QUALITY_GATE=";

  result->quality_check_enabled_marker = "QUALITY_CHECK_ENABLED=";
  result->quality_check_include_vmaf_marker = "QUALITY_CHECK_INCLUDE_VMAF=";
  result->quality_check_strict_gate_marker = "QUALITY_CHECK_STRICT_GATE=";
  result->quality_check_reference_layer_marker = "QUALITY_CHECK_REFERENCE_LAYER=";
  result->quality_check_min_psnr_marker = "QUALITY_CHECK_MIN_PSNR=";
  result->quality_check_min_ssim_marker = "QUALITY_CHECK_MIN_SSIM=";
  result->quality_check_min_vmaf_marker = "QUALITY_CHECK_MIN_VMAF=";
  result->quality_check_max_files_per_profile_marker = "QUALITY_CHECK_MAX_FILES_PER_PROFILE=";

  result->source_as_activate = "SOURCE_AS_ACTIVATE=";
  result->source_as_location = "SOURCE_AS_LOCATION=";
  result->keep_source_as = "KEEP_SOURCE_AS=";
  result->source_as_valid_codecs_list = "SOURCE_AS_VALID_CODECS_LIST=";
  result->source_as_carry_over_audio_codecs_not_specified_in_valid_codecs_list = "SOURCE_AS_CARRY_OVER_AUDIO_CODECS_NOT_SPECIFIED_IN_VALID_CODECS_LIST=";
  result->source_as_preferred_language_metadata = "SOURCE_AS_PREFERRED_LANGUAGE_METADATA=";

  return result;
}
