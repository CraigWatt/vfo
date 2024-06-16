#include "as_audio_strat_struct.h"
#include <string.h>

audio_strat_t* audio_strat_create_new_struct(config_t *config) {
  audio_strat_t *result = malloc(sizeof(audio_strat_t));
  //point to relevant param config variables

  //ALREADY VERIFIED (by config)
  result->root = config->svc->source_location;
  //VERIFIED
  result->content = as_create_content(result->root);
  //VERIFIED
  result->unable_to_process = as_create_unable_to_process(result->root);

  //VERIFIED
  result->source = as_get_source_root(config->svc->source_location);

  result->cf_head = config->cf_head;
  return result;
}

char* as_create_content(char *audio_strat_root) {
  //create
  char *tmp_source_content = malloc(BUFSIZ);
  strcpy(tmp_source_content, audio_strat_root);
  strcat(tmp_source_content, "/content");
  //verify
  if(!utils_does_folder_exist(tmp_source_content)) {
    printf("AUDIO_STRAT WARNING: vfo could not find an essential /content folder in %s\n",audio_strat_root);
    utils_ask_user_for_permission_to_create_a_folder(tmp_source_content);
    utils_create_folder(tmp_source_content);
    if(!utils_does_folder_exist(char *location))
  }
}

char* as_create_unable_to_process(char *audio_strat_root) {

}


