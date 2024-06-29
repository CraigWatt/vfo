#include "sas_struct.h"
#include <stdlib.h>
#include <string.h>

source_as_t* sas_create_new_struct(config_t *config) {
  source_as_t *result = malloc(sizeof(source_as_t));
  //point to relevant param config variables

  //ALREADY VERIFIED (by config)
  result->root = config->svc->source_as_location;
  //VERIFIED
  result->content = sas_create_content(result->root);
  //VERIFIED
  result->unable_to_process = sas_create_unable_to_process(result->root);

  //VERIFIED
  result->source = config->svc->source_location;

  result->cf_head = config->cf_head;
  return result;
}

char* sas_create_content(char *source_as_root) {
  //create
  char *tmp_source_content = malloc(BUFSIZ);
  strcpy(tmp_source_content, source_as_root);
  strcat(tmp_source_content, "/content");
  //verify
  if(!utils_does_folder_exist(tmp_source_content)) {
    printf("AUDIO_STRAT WARNING: vfo could not find an essential /content folder in %s\n",audio_strat_root);
    utils_ask_user_for_permission_to_create_a_folder(tmp_source_content);
    utils_create_folder(tmp_source_content);
    if(!utils_does_folder_exist(char *location))
  }
}

char* sas_create_unable_to_process(char *source_as_root) {
  //create
  char *tmp_audio_strat_unable_to_process = malloc(BUFSIZ);
  strcpy(tmp_audio_strat_unable_to_process, audio_strat_root);
  strcat(tmp_audio_strat_unable_to_process, "/unable_to_process");
  //verify
  if(!utils_does_folder_exist(tmp_audio_strat_unable_to_process)) {
    printf("AUDIO_STRAT WARNING: vfo could not find an essential /unable_to_process folder in %s\n", audio_strat_root);
    utils_ask_user_for_permission_to_create_a_folder(tmp_audio_strat_unable_to_process);
    utils_create_folder(tmp_audio_strat_unable_to_process);
    if(!utils_does_folder_exist(tmp_audio_strat_unable_to_process)) {
      printf("AUDIO_STRAT MAJOR ERROR: vfo could not create %s & it does not exist AUDIO_STRAT MAJOR ERROR\n", tmp_audio_strat_unable_to_process);
      exit(EXIT_FAILURE);
    }
  }
  return tmp_audio_strat_unable_to_process;
}


