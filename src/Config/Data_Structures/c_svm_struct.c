#include "c_svm_struct.h"

sole_var_markers_t* svm_create_new_struct() {
  sole_var_markers_t *result = malloc(sizeof(sole_var_markers_t));
  result->original_location = "ORIGINAL_LOCATION=";
  result->source_location = "SOURCE_LOCATION=";
  result->keep_source = "KEEP_SOURCE=";

  result->source_test_active_marker = "SOURCE_TEST_ACTIVE=";
  result->source_test_trim_start_marker = "SOURCE_TEST_TRIM_START=";
  result->source_test_trim_duration_marker = "SOURCE_TEST_TRIM_DURATION=";

  result->source_as_activate = "SOURCE_AS_ACTIVATE=";
  result->source_as_location = "SOURCE_AS_LOCATION=";
  result->keep_source_as = "KEEP_SOURCE_AS=";
  result->source_as_valid_codecs_list = "SOURCE_AS_VALID_CODECS_LIST=";
  result->source_as_carry_over_audio_codecs_not_specified_in_valid_codecs_list = "SOURCE_AS_CARRY_OVER_AUDIO_CODECS_NOT_SPECIFIED_IN_VALID_CODECS_LIST=";
  result->source_as_preferred_language_metadata = "SOURCE_AS_PREFERRED_LANGUAGE_METADATA=";

  return result;
}
