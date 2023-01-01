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

#ifndef TESTING
int main (int argc, char **argv) {  
  /*immediately revise argc & argv if duplicate words are found in command ---------------*/
  int revised_argc= 0;
  char **revised_argv = utils_remove_duplicates_in_array_of_strings(argv, argc, &revised_argc);
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
  const char *config_dir = "/usr/local/bin/vfo_conf_folder";
  //extract config file data and unknown words from user input, to config struct
  config_t *config = con_init(config_dir, revised_argv, revised_argc);
  //does argv contain an unknown word << this is actually validated by the con_init, because if an arg doesn't match config pre defined words AND doesn't define an alias mentioned in the config file, we have found an unknown word and the program will stop

  /*now that we have config, let's now CONTINUE TO HANDLE errors that we can at this stage*/
  //error check 'source' AND config.keep_source = false
  if(ih->arguments->source_detected == true && config->svc->keep_source == false) {
    printf("ERROR: You cannot execute source command because config variable KEEP_SOURCE= is set to false\n");
    exit(EXIT_FAILURE);
  }
  //can i validate any more errors at this point?

  //can I now execute things>>

  printf("ih->arguments->original_detected: %i\n", ih->arguments->original_detected);
  printf("ih->arguments->revert_detected: %i\n", ih->arguments->revert_detected);
  printf("ih->arguments->source_detected: %i\n", ih->arguments->source_detected);
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
