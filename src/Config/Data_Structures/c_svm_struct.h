#ifndef C_SVM_STRUCT_H
#define C_SVM_STRUCT_H
/*Sole Var Marker Struct*/

#include <stdlib.h>

struct sole_var_markers {
  char* original_location;
  char* source_location;
  char* keep_source;

  char* source_test_active_marker;
  char* source_test_trim_start_marker;
  char* source_test_trim_duration_marker;

  char* source_as_activate;
  char* source_as_location;
  char* keep_source_as;
  char* source_as_valid_codecs_list;
  char* source_as_carry_over_audio_codecs_not_specified_in_valid_codecs_list;
  char* source_as_preferred_language_metadata;
};
/* Exports variable_markers as a global type */
typedef struct sole_var_markers sole_var_markers_t;

sole_var_markers_t* svm_create_new_struct();

#endif // C_SVM_STRUCT_H
