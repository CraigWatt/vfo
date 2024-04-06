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

#include "u_internal.h"
#include <stdlib.h>

/**
 * Checks to see if param is a real directory on current machine
 * 
 * @param directory (full path string (char*))
 * @return bool
 */
bool utils_does_folder_exist(char *location) {
  if(location == NULL) 
    return false;

  DIR* dir = opendir(location);
  if (dir) {
    /* Folder exists. */
    if (closedir(dir) == -1) {
      printf("Error closing directory.\n");
      exit(EXIT_FAILURE);
    }
    return true;
  } else if (ENOENT == errno) {
    /* Folder does not exist. */
    return false;
  } else if (EACCES == errno) {
    printf("folder exists BUT vfo does not have permission to access it\n");
    printf("please grant access to %s\n", location);
    exit(EXIT_FAILURE);
  } else {
    /* failed for some other reason. */
    printf("DEV: check utils.c, serious error has occurred, please contact vfo developer\n");
    printf("ENOENT: %i\n", ENOENT);
    printf("errno: %i\n",errno);
    exit(EXIT_FAILURE);
  }
}

/*
 * This function returns true if:
 * 1. The string contains null ""
 * 2. The string contains only empty spaces "   "
 * returns: bool
 */
bool utils_string_is_empty_or_spaces(char *string) {
  bool only_spaces_found = false;
  if(string[0] == '\0') {
    return true;
  } else {
    int count = 0;
    do {
      if(string[count] == '\0')
        only_spaces_found = true;
      count++;
    }while(isspace(string[count] == true));
  }
  if(only_spaces_found == true)
    return true;
  return false;
}


/*
 * This function creates a folder using the param location
 */
void utils_create_folder(char *location) {
  mkdir(location, BUFSIZ);
}

void utils_create_error_encoding_file(char *location, char *candidate_name) {
  printf("DEV: UTILS_CREATE_ERROR_ENCODING_FILE RUNS!\n");
  char *bn = basename(candidate_name);
  char *txt_file = utils_combine_to_full_path(location, bn);
  FILE *fp;
  fp = fopen(txt_file, "w");
  fprintf(fp, "There was an error encoding this candidate file\n");
  fclose(fp);
}


/*
 * This function combines param directory and '/' and param filename
 * returns: combined string
 */
char *utils_combine_to_full_path(const char *directory, const char *filename) {
  int dir_length = strlen(directory);
  int file_length = strlen(filename);
  int size = dir_length + file_length + 2;
  char *s = calloc(size, sizeof(char));
  strcat(s, directory);
  strcat(s, "/");
  strcat(s, filename);
  s[size -1] = '\0';
  return s;
}

/*
 * This function combines param start_string and param end_string
 * returns: combined string
 */
char *utils_combine_strings(char *start_string, char *end_string) {
  int first_length = strlen(start_string);
  int last_length = strlen(end_string);
  int size = first_length + last_length + 1;
  char *s = calloc(size, sizeof(char));
  strcat(s, start_string);
  strcat(s, end_string);
  s[size - 1] = '\0';
  return s;
}

/*
 * This function returns true if:
 * 1. param location is a file\
 * returns: bool
 */
bool utils_does_file_exist(char *location) {
  if (access(location, F_OK) == 0) {
      // file exists
      return true;
  } else {
      // file doesn't exist
      return false;
  }
}

/*
 * This function UPPERCASE's param s
 * returns: UPPERCASE'd string
 */
char* utils_uppercase_string(char *s) {
  int length = strlen(s);
  char *tmp = calloc(length, sizeof(char));
  for(int i = 0; i < length; i++) {
    tmp[i] = toupper(s[i]);
  }
  return tmp;
}

/*
 * This function lowercase's param s
 * returns: lowercase'd string
 */
char* utils_lowercase_string(char *s) {
  int length = strlen(s);
  char *tmp = calloc(length, sizeof(char));
  for(int i = 0; i < length; i++) {
    tmp[i] = tolower(s[i]);
  }
  return tmp;
}

/*
 * This function returns true if:
 * 1. param folder exists AND param folder contains no files/directories
 * returns bool
 */
bool utils_is_folder_empty(char *folder) {
  utils_does_folder_exist(folder);
  int n = 0;
  struct dirent *entry;
  DIR *directory = opendir(folder);
  if (directory == NULL) {
    printf("ERROR utils_is_folder_empty, %s does not exist\n", folder);
    exit(EXIT_FAILURE);
  }
  while ((entry = readdir(directory)) != NULL) {
    if(++n > 2)
      break;
  }
  if (closedir(directory) == -1) {
    printf("Error closing directory.\n");
    exit(EXIT_FAILURE);
  }
  if(n <= 2) {
    return true;
  } 
  else {
    return false;
  }
}

/*
 * This function replaces ' ' characters in param file_name with '_'
 */
void utils_replace_spaces(char *file_name, char *parent_directory) {
  int s_length = strlen(file_name);
  for(int i = 0; i <= s_length; i++) {
    if (isspace(file_name[i])) {
      char *tmp_filename = calloc(s_length, sizeof(char));
      for(int j = 0; j <= s_length; j++) {
        if(isspace(file_name[j])) {
          tmp_filename[j] = '_';
        } 
        else {
          tmp_filename[j] = file_name[j];
        }
      }
      char answer;
      printf("change %s to %s ?\n", file_name, tmp_filename);
      printf("y/n ?\n");
      scanf(" %c", &answer);
      printf("\n user input: %c\n", answer);
      
      if(answer == 'n' || answer == 'N') {
        printf("exiting vfo...\n");
        exit(EXIT_FAILURE);
      } else if (answer == 'y' || answer == 'Y') {
        char *old_file_full_path = utils_combine_to_full_path(parent_directory, file_name);
        char *new_file_full_path = utils_combine_to_full_path(parent_directory, tmp_filename);
        if(rename(old_file_full_path, new_file_full_path) != 0) {
          fprintf(stderr, "Errno: %i\n", errno);
          perror("Error msg");
        } else printf("%s -> %s\n", file_name, tmp_filename);
        i--;
        break;
      }
      else {
        printf("unknown response, exiting vfo...\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

/*
 * This function pauses to ask the user if they wish to continue with the next process, and tells the user what process was just completed.
 */
void utils_wish_to_continue(char *previous_process, char *next_action) {
   char answer;
    printf("USER PROMPT:\n");
    printf("Everything appears to be in order with %s, do you wish to continue with the next action:%s?\n", previous_process, next_action);
    printf("y/n ?\n");
    scanf(" %c", &answer);
    printf("\n user input: %c\n", answer);
    
    if(answer == 'n' || answer == 'N') {
      printf("exiting vfo...\n");
      exit(EXIT_SUCCESS);
    } else if (answer == 'y' || answer == 'Y') {
      
    }
    else {
      printf("unknown response, exiting vfo...\n");
      exit(EXIT_FAILURE);
    }
}

/*
 * This functions pauses to ask for permission to create a specific folder
 */
void utils_ask_user_for_permission_to_create_a_folder(char *folder) {
  char answer;
  printf("USER PROMPT:\n");
  printf("vfo requires %s to exist.\n", folder);
  printf("Would you like vfo to create this folder for you?\n");
  printf("y/n ?\n");
  scanf(" %c", &answer);
  printf("\n user input: %c\n", answer);

    if(answer == 'n' || answer == 'N') {
      printf("vfo cannot execute your command without this folder: %s\n", folder);
      printf("vfo will now exit...\n");
      exit(EXIT_SUCCESS);
    } else if (answer == 'y' || answer == 'Y') {
      
    }
    else {
      printf("unknown response, exiting vfo.\n");
      exit(EXIT_SUCCESS);
    }
}

/*
 * This function checks to see if a particular param folder contains a file that matches the param extension provided.
 */
bool utils_does_folder_contain_file_with_extension(char *folder, char *extension) {
  bool extension_found = false;
  //open folder
  DIR *directory;
  struct dirent *entry;
  directory = opendir(folder);
  if(directory == NULL) {
    printf("this Error opening %s directory\n", folder);
    exit(EXIT_FAILURE);
  }
  while((entry = readdir(directory)) != NULL) {
    if(entry->d_type == DT_REG && !(utils_file_is_macos_hidden_files(entry->d_name))) {
      
      if(strcmp(extension, ".mkv") == 0) {
        extension_found = utils_is_file_extension_mkv(entry->d_name);
      }
      else if(strcmp(extension, ".mp4") == 0) {
        extension_found = utils_is_file_extension_mp4(entry->d_name);     
      }
      if(extension_found == true) break;
    } 
  }
  if (closedir(directory) == -1) {
    printf("Error closing directory.\n");
    exit(EXIT_FAILURE);
  }
  if(extension_found == true) {
    return true;
  }
  return false;
}

bool utils_does_folder_contain_file_with_valid_video_extension(char *folder) {
  bool valid_extension_detected = false;
  DIR *directory;
  struct dirent *entry;
  directory = opendir(folder);
  if(directory == NULL) {
    printf("MAJOR ERROR: Could not open %s to check if it contains a file with a valid video file extension\n", folder);
    exit(EXIT_FAILURE);
  }
  while((entry = readdir(directory)) != NULL) {
    if(entry->d_type == DT_REG && !(utils_file_is_macos_hidden_files(entry->d_name))) {
      bool is_file_mkv = utils_is_file_extension_mkv(entry->d_name);
      bool is_file_mp4 = utils_is_file_extension_mp4(entry->d_name);     
      if(is_file_mkv || is_file_mp4) {
        valid_extension_detected = true;
        break;
      }
    }
  }
  if(closedir(directory) == -1) {
    printf("MAJOR ERROR: Could not close %s after checking for a file within that contains a valid video file extension\n", folder);
    exit(EXIT_FAILURE);
  }
  return valid_extension_detected;
}

bool utils_does_folder_contain_file_with_mkv_extension(char *folder) {
  bool valid_extension_detected = false;
  DIR *directory;
  struct dirent *entry;
  directory = opendir(folder);
  if(directory == NULL) {
    printf("MAJOR ERROR: Could not open %s to check if it contains a file with a .mkv extension\n", folder);
    exit(EXIT_FAILURE);
  }
  while((entry = readdir(directory)) != NULL) {
    if(entry->d_type == DT_REG && !(utils_file_is_macos_hidden_files(entry->d_name))) {
      bool is_file_mkv = utils_is_file_extension_mkv(entry->d_name);     
      if(is_file_mkv) {
        valid_extension_detected = true;
        break;
      }
    }
  }
  if(closedir(directory) == -1) {
    printf("MAJOR ERROR: Could not close %s after checking for a file within that contains .mkv extension\n", folder);
    exit(EXIT_FAILURE);
  }
  return valid_extension_detected;
}

bool utils_does_folder_contain_file_with_mp4_extension(char *folder) {
  bool valid_extension_detected = false;
  DIR *directory;
  struct dirent *entry;
  directory = opendir(folder);
  if(directory == NULL) {
    printf("MAJOR ERROR: Could not open %s to check if it contains a file with .mp4 extension\n", folder);
    exit(EXIT_FAILURE);
  }
  while((entry = readdir(directory)) != NULL) {
    if(entry->d_type == DT_REG && !(utils_file_is_macos_hidden_files(entry->d_name))) {
      bool is_file_mp4 = utils_is_file_extension_mp4(entry->d_name);     
      if(is_file_mp4) {
        valid_extension_detected = true;
        break;
      }
    }
  }
  if(closedir(directory) == -1) {
    printf("MAJOR ERROR: Could not close %s after checking for a file within that contains .mp4 file extension\n", folder);
    exit(EXIT_FAILURE);
  }
  return valid_extension_detected;
}

/*
 * This function fetches the filename of a single file found within a folder
 * The function will return the first file it finds in the folder.
 * The function will not fail if a folder contains more than a single file.
 */
char *utils_fetch_single_file_name(char *folder) {
  DIR *directory;
  struct dirent *entry;
  directory = opendir(folder);
  if(directory == NULL) {
    printf("Could not open directory to fetch single file name in: %s\n", folder);
    exit(EXIT_FAILURE);
  }
  char *tmp_file_name;
  char *for_return;
  bool file_found = false;
  while((entry=readdir(directory)) != NULL) {  
    if(entry->d_type == DT_REG && !(utils_file_is_macos_hidden_files(entry->d_name))) {
      file_found = true;
      tmp_file_name = entry->d_name;
      break;
    }
  }
  if(closedir(directory) == -1) {
    printf("Error closing directory: %s\n", folder);
  }
  if(file_found == true) {
    for_return = tmp_file_name;
    return for_return;
  } else if(file_found == false) {
    printf("bool false, could not fetch a file in %s to get get file name:\n", folder);
    exit(EXIT_FAILURE);
  }
  return 0;
}

/*
 * This function fetches a single valid movie file from a folder
 * In it's current form, the function only fetches files that are mkv or mp4
 * The function will fail if a matching file is not present in the param folder
 */
char* utils_fetch_single_file(char *folder) {
  DIR *directory;
  struct dirent *entry;
  directory = opendir(folder);
  if(directory == NULL) {
    printf("Could not open directory to find any file in: %s\n", folder);
    exit(EXIT_FAILURE);
  }
  char *tmp_file_name;
  char *for_return;
  bool file_found = false;
  while((entry=readdir(directory)) != NULL) {  
    if(entry->d_type == DT_REG && !(utils_file_is_macos_hidden_files(entry->d_name))) {
      if(utils_is_file_extension_mkv(entry->d_name) || utils_is_file_extension_mp4(entry->d_name)) {
        file_found = true;
        tmp_file_name = entry->d_name;
        break;
      }
    }
  }
  if(closedir(directory) == -1) {
    printf("Error closing directory: %s\n", folder);
  }
  if(file_found == true) {
    for_return = utils_combine_to_full_path(folder, tmp_file_name);
    return for_return;
  } else if(file_found == false) {
    printf("bool false, could not find a file in: %s\n", folder);
    exit(EXIT_FAILURE);
  }
  return 0;
}

/*
 * This function takes param string file_name
 * returns true if end of file_name string is ".mkv"
 */
bool utils_is_file_extension_mkv(char *file_name) {
  char *point = file_name + strlen(file_name);
  if((point = strrchr(file_name,'.')) != NULL) {
    if (strcmp(point, ".mkv") == 0) {
      return true;
    }
  }
  return false;
}

/*
 * This function takes param string file_name
 * returns true if end of file_name string is ".mp4"
 */
bool utils_is_file_extension_mp4(char *file_name) {
  char *point = file_name + strlen(file_name);
  if((point = strrchr(file_name,'.')) != NULL) {
    if (strcmp(point, ".mp4") == 0) {
      return true;
    }
  }
  return false;
}

/*
 * This function returns true if:
 * 1. param file_name ends in : .srt, .sup, .sub, .ssa, .smi, .vtt, .txt, .rtf
 * 2. depending on param rf_rules, will return true if end of file matches rf_rules logic
 * rf_rules "start" returns true if file_name ends in .mkv OR .mp4
 * rf_rules "mkv_original" returns true if file_name ends in .mkv
 * rf_rules "mp4_original" returns true if file_name ends in .mp4
 *The function will fail if it is impossible to determine any sort of file extension from file_name param
 */
bool utils_is_file_extension_valid(char *file_name, char* rf_rules) {
  char *point = file_name + strlen(file_name);
  if((point = strrchr(file_name,'.')) != NULL) {
    //alternative valid extensions
    if (strcmp(point,".srt") == 0 || strcmp(point,".sup") == 0 || strcmp(point,".sub") == 0 || strcmp(point,".ssa") == 0 || 
        strcmp(point,".smi") == 0 || strcmp(point,".vtt") == 0 || strcmp(point,".txt") == 0 || strcmp(point,".rtf") == 0) {
      //ends with srt || ends with sup || ends with sub || ends with ssa || ends with smi || ends with vtt || ends with txt || ends with rtf
      return false;
    }
    else if(strcmp(rf_rules, "start") == 0) {
      if(strcmp(point,".mkv") == 0 || strcmp(point, ".mp4") == 0) {
        return true;
      }
    }
    else if (strcmp(rf_rules, "mkv_original") == 0) {
      if(strcmp(point,".mkv") == 0) {
        return true;
      }
    }
    else if (strcmp(rf_rules, "mp4_original") == 0) {
      if(strcmp(point,".mp4") == 0) {
        return true;
      }
    }
    else {
      return false;
    }
  } else {
    printf("Could not detect a file extension for this file: %s\n", file_name);
    exit(EXIT_FAILURE);
  }
  return false;
}

/*
 * This function returns true if generic macos_hidden files are detected
 * This function will return true if:
 * 1. file_name starts with "._"
 * 2. file_name equals ".DS_Store"
 */
bool utils_file_is_macos_hidden_files(char *file_name) {
  int file_length = strlen(file_name);
  if (file_name[0] == '.' && file_name[1] == '_' && file_length >= 2) {
    return true;
  }
  if(strcmp(file_name, ".DS_Store") == 0) {
    return true;
  }
  return false;
}


/*
 * This function returns true if:
 * 1. param folder_name equals "."
 * 2. param folder_name equals ".."
 */
bool utils_directory_is_current_or_parent(char *folder_name) {
  if(strcmp(folder_name, ".") == 0 || strcmp(folder_name, "..") == 0) {
    return true;
  }
  return false;
}

/*
 * This function is really just a glorified error message
 */
void utils_found_a_rogue_file(char *file_name, char *folder) {
  printf("vfo found a rogue file in %s\n", folder);
  printf("file: %s\n", file_name);
  printf("please ensure you are storing your files correctly as per the CUSTOM_FOLDER rules\n");
}

bool utils_is_string_a_reserved_word(char *string) {
  char *reserved_words[] = {"original", "source", "revert", "wipe", "all_aliases", "do_it_all", "alias"};
  int reserved_words_array_length = (sizeof reserved_words / sizeof(char*));
  
  bool match_found = false;
  for(int i = 0; i <= reserved_words_array_length; i++) {
    if (strcmp(string, reserved_words[i]) == 0) {
      match_found = true;
    } 
  }
  return match_found;
}

/*
 * This function is perfectly fine because there is no recursion breaking 'os open file limit'
 */
void utils_does_folder_contain_valid_custom_folders(char *folder, cf_node_t *cf_head) {
  DIR *directory;
  struct dirent *entry;
  directory = opendir(folder);
  if (directory == NULL) {
    printf("14 Error opening directory.\n");
    exit(EXIT_FAILURE);
  }
  while ((entry = readdir(directory)) != NULL) {
    if(entry->d_type == DT_REG && !(utils_file_is_macos_hidden_files(entry->d_name))) {
      printf("THIS RUNS\n");
      utils_found_a_rogue_file(entry->d_name, folder);
      exit(EXIT_FAILURE);
    } 
    else if (entry->d_type == DT_DIR && !(utils_directory_is_current_or_parent(entry->d_name)) && !(cf_folder_name_exits(cf_head, entry->d_name))) {
      printf("woah lad what's this\n");
      printf("%s is not a custom folder in %s\n", entry->d_name, folder);
      exit(EXIT_FAILURE);
    }
  }
  if (closedir(directory) == -1) {
    printf("Error closing directory.\n");
    exit(EXIT_FAILURE);
  }
}


/*
 * This function could be improved,
 * but it atleast isn't breaking 'os open file limit'
 */
void utils_is_folder_missing_custom_folders(char *folder, cf_node_t *cf_ll_head) {
  DIR *directory;
  struct dirent *entry;
  directory = opendir(folder);
  if (directory == NULL) {
    printf("15 Error opening directory.\n");
    exit(EXIT_FAILURE);
  }

  int totalCount = cf_get_count(cf_ll_head);
  int counter = 0;

  while ((entry = readdir(directory)) != NULL) {
    if(entry->d_type == DT_REG && !(utils_file_is_macos_hidden_files(entry->d_name))) {
      utils_found_a_rogue_file(entry->d_name, folder);
      exit(EXIT_FAILURE);
    } 
    else if (entry->d_type == DT_DIR && !(utils_directory_is_current_or_parent(entry->d_name)) && cf_folder_name_exits(cf_ll_head, entry->d_name)) {
      counter++;
    }     
  }
  if (closedir(directory) == -1) {
    printf("Error closing directory.\n");
    exit(EXIT_FAILURE);
  }

  if(counter != totalCount) {
    printf("MISSING custom_folders in %s\n", folder);
    exit(EXIT_FAILURE);
  } 
}


/*
 * This function's sibling functions COULD BREAK 'os open file limit'
 */
void utils_are_custom_folders_type_compliant(char *root_folder, char* rf_rules, cf_node_t *cf_ll_head) {
    DIR *directory;
  struct dirent *entry;
  directory = opendir(root_folder);
  if(directory == NULL) {
    printf("11 Error opening %s directory\n", root_folder);
    exit(EXIT_FAILURE);
  }
  while ((entry = readdir(directory)) != NULL) {
    if(entry->d_type == DT_DIR && !(utils_directory_is_current_or_parent(entry->d_name))) {
      //for each custom_folder dir
      //get their type
      char *ftype = cf_get_type_from_folder_name(cf_ll_head, entry->d_name);
      //if type is films
      if (strcmp(ftype, "films") == 0) {
        char *tmp_cf_loc = utils_combine_to_full_path(root_folder, entry->d_name);
        char *tmp_layer = "layer1";
        utils_is_custom_folder_films_type_compliant(tmp_cf_loc, rf_rules, tmp_layer);
      }
      //if type is tv
      if (strcmp(ftype, "tv") == 0) {
        char *tmp_cf_loc = utils_combine_to_full_path(root_folder, entry->d_name);
        char *tmp_layer = "layer1";
        utils_is_custom_folder_tv_type_compliant(tmp_cf_loc, rf_rules, tmp_layer);
      }
      printf("VALID: %s in %s\n", entry->d_name, root_folder);
    }
  }
  if (closedir(directory) == -1) {
    printf("Error closing directory.\n");
    exit(EXIT_FAILURE);
  }
}

void utils_is_custom_folder_tv_type_compliant(char *custom_folder, char *rf_rules, char *layer) {
  DIR *directory;
  struct dirent *entry;
  directory = opendir(custom_folder);
  if(directory == NULL) {
    printf("13 Error opening %s directory\n", custom_folder);
    exit(EXIT_FAILURE);
  }
  int movie_file_count = 0;
  while ((entry = readdir(directory)) != NULL) {
    if(strcmp(layer, "layer1") == 0 || strcmp(layer, "layer2") == 0 ||  strcmp(layer, "layer3") == 0) {
      if(entry->d_type == DT_REG && !(utils_file_is_macos_hidden_files(entry->d_name))) {
        utils_found_a_rogue_file(entry->d_name, custom_folder);
        exit(EXIT_FAILURE);
      }
      if(entry->d_type == DT_DIR && !(utils_directory_is_current_or_parent(entry->d_name))) {
        char *tmp_cf_loc = utils_combine_to_full_path(custom_folder, entry->d_name);
        if (utils_is_folder_empty(tmp_cf_loc)) {
          printf("ERROR, %s is empty.  Please delete this folder\n", tmp_cf_loc);
          exit(EXIT_FAILURE);
        }
        char *tmp_layer;
        if(strcmp(layer, "layer1") == 0) {
          tmp_layer = "layer2";
        } else if(strcmp(layer, "layer2") == 0) {
          tmp_layer = "layer3";
        } else if(strcmp(layer, "layer3") == 0) {
          tmp_layer = "layer4";
        }
        else {
          printf("MAJOR Layer error occurred\n");
          exit(EXIT_FAILURE);
        }
        utils_is_custom_folder_tv_type_compliant(tmp_cf_loc, rf_rules, tmp_layer);
      }
    }
    else if(strcmp(layer, "layer4") == 0) {
      if(entry->d_type == DT_REG && !(utils_file_is_macos_hidden_files(entry->d_name))) {
        utils_replace_spaces(entry->d_name , custom_folder);
        if (utils_is_file_extension_valid(entry->d_name, rf_rules)) {
          movie_file_count++;
        }
      }
      if(movie_file_count >= 2) {
        printf("looks like %s contains more than a single movie file!\n", custom_folder);
        exit(EXIT_FAILURE);
      }
      if(entry->d_type == DT_DIR && !(utils_directory_is_current_or_parent(entry->d_name))) {
        char *tmp_cf_loc = utils_combine_to_full_path(custom_folder, entry->d_name);
        printf("woah lad, no folders should be present here in %s\n", tmp_cf_loc);
        exit(EXIT_FAILURE);
      }
    }
  }
  if (closedir(directory) == -1) {
    printf("Error closing directory.\n");
    exit(EXIT_FAILURE);
  }
}

void utils_delete_folder_if_it_is_empty(char *folder) {
  DIR *directory;
  struct dirent *entry;
  directory = opendir(folder);
  if(directory == NULL) {
    printf("MAJOR ERROR: Could not open this folder %s to check if it was empty so it could be deleted.\n", folder);
    exit(EXIT_FAILURE);
  }
  bool found_item_in_folder = false;
  while((entry=readdir(directory))!= NULL) {
    if(entry->d_type == DT_DIR && utils_directory_is_current_or_parent(entry->d_name)) {
      //found folders that can be discounted from empty check
      
    } else if (entry->d_type == DT_REG && utils_file_is_macos_hidden_files(entry->d_name)) {
      //found files that can be discounted from empty check
    } else {
      //found something - folder is NOT empty
      found_item_in_folder = true;
    }
  }
  if(closedir(directory) == -1) {
    printf("MAJOR ERROR: Could not close this folder %s after checking if it was empty to so it could be deleted.\n", folder);
  }
  if(found_item_in_folder == false) {
    //we can now delete the folder because it 100% is empty and that is what we want to do.
    printf("remove empty folder runs\n");
    rmdir(folder);
    if(utils_does_folder_exist(folder))
      printf("MAJOR ERROR: vfo attempted to deleted an empty folder %s, but it could not be deleted\n", folder);
  }
}

void utils_is_custom_folder_films_type_compliant(char *custom_folder, char *rf_rules, char *layer) {
  DIR *directory;
  struct dirent *entry;
  directory = opendir(custom_folder);
  if(directory == NULL) {
    printf("12 Error opening %s directory\n", custom_folder);
    exit(EXIT_FAILURE);
  }
  int movie_file_count = 0;
  while ((entry = readdir(directory)) != NULL) {
    if(strcmp(layer, "layer1") == 0) {
      if(entry->d_type == DT_REG && !(utils_file_is_macos_hidden_files(entry->d_name))) {
          utils_found_a_rogue_file(entry->d_name, custom_folder);
          exit(EXIT_FAILURE);
      }
      if(entry->d_type == DT_DIR && !(utils_directory_is_current_or_parent(entry->d_name))) {
        char *tmp_cf_loc = utils_combine_to_full_path(custom_folder, entry->d_name);
        if (utils_is_folder_empty(tmp_cf_loc)) {
          printf("ERROR, %s is empty.  Please delete this folder\n", tmp_cf_loc);
          exit(EXIT_FAILURE);
        }
        char *tmp_layer = "layer2";
        utils_is_custom_folder_films_type_compliant(tmp_cf_loc, rf_rules, tmp_layer);
      }
    }
    else if (strcmp(layer, "layer2") == 0) { 
      if(entry->d_type == DT_REG && !(utils_file_is_macos_hidden_files(entry->d_name))) {
        utils_replace_spaces(entry->d_name , custom_folder);
        if (utils_is_file_extension_valid(entry->d_name, rf_rules)) {
            movie_file_count++;
        }
      }
      if(entry->d_type == DT_DIR && !(utils_directory_is_current_or_parent(entry->d_name))) {
        char *tmp_cf_loc = utils_combine_to_full_path(custom_folder, entry->d_name);
        if (utils_is_folder_empty(tmp_cf_loc)) {
          printf("ERROR, %s is empty.  Please delete this folder\n", tmp_cf_loc);
          exit(EXIT_FAILURE);
        } else {
          printf("FOLDER %s should not be present here.  please remove this folder and it's contents\n", tmp_cf_loc);
          exit(EXIT_FAILURE);
        }
      }
      if(movie_file_count >= 2) {
        printf("looks like %s contains more than a single movie file!\n", custom_folder);
        exit(EXIT_FAILURE);
      }
      if(entry->d_type == DT_DIR && !(utils_directory_is_current_or_parent(entry->d_name))) {
        char *tmp_cf_loc = utils_combine_to_full_path(custom_folder, entry->d_name);
        printf("woah lad, no folders should be present here in %s\n", tmp_cf_loc);
        exit(EXIT_FAILURE);
      }
    }
  }
  if (closedir(directory) == -1) {
    printf("Error closing directory.\n");
    exit(EXIT_FAILURE);
  }
}

/*
 * This function returns true if:
 * 1. the param string contains a character of param c
 * returns: bool
 */
bool utils_string_contains_char(char *string, char c) {
  bool char_found = false;
  int string_length = strlen(string);
  for(int i = 0; i < string_length; i++) {
    if(string[i] == c) {
      char_found = true;
      break;
    }
  }
  return char_found;
}

/*
 * This function returns true if:
 * 1. the param string contains a single occurrence of param c
 * returns: bool  
 */
bool utils_string_contains_single_occurrence_of_char(char *string, char c) {
  bool single_char_found = false;
  int string_length = strlen(string);
  for(int i = 0; i < string_length; i++) {
    if(string[i] == c) {
      single_char_found = true;
    } 
    else if(string[i] == c && single_char_found == true) {
      //found a second occurrence of c
      return false;
    }
  }
  return single_char_found;
}

/*
 * This functions returns true if:
 * 1. the param string contains multiple_occurrences of param c
 * returns: bool
 *
 */
bool utils_string_contains_multiple_occurrences_of_char(char *string, char c) {
  bool single_char_found = false;
  bool multiple_char_found = false;
  int string_length = strlen(string);
  for(int i = 0; i < string_length; i++) {
    if(string[i] == c) {
      single_char_found = true;
    } 
    else if(string[i] == c && single_char_found == true) {
      multiple_char_found = true;
      break;
    }
  }
  return multiple_char_found;
}

/*
 * This function returns true if:
 * 1. the param string_array contains an array element that matches param string
 * returns: bool
 */
bool utils_string_array_contains_string(char *array_of_strings[], int array_size, char *string) {
  int aos_size = array_size;
  bool string_found_in_array = false;
  for(int i = 0; i < aos_size; i++) {
    if (strcmp(string, array_of_strings[i]) == 0) {
      string_found_in_array = true;
      break;
    } 
  }
  return string_found_in_array;
}

/*
 * is check found within string
 */
bool utils_is_substring(char *check, char *string) {
  int slen = strlen(string);
  int clen = strlen(check);

  int end = slen - clen + 1;

  for(int i = 0; i < end; i++) {
    bool check_found = true;
    for (int j = 0; j < clen; j++) {
      if(check[j] != string[i+ j]) {
        check_found = false;
        break;
      }
    }
    if(check_found) return true;
  }
  return false;
}

/*
 * This function returns true if:
 * 1. when of all param string_array elements, only 1 element matches param string
 * returns bool
 */
bool utils_string_array_contains_single_occurrence_of_string(char **array_of_strings, int array_size, char *string) {

  int aos_size = array_size;

  bool string_found_in_array_once = false;
  bool is_single_occurrence = false;
  for(int i = 0; i < aos_size; i++) {
    //string found once
    if (strcmp(string, array_of_strings[i]) == 0 && string_found_in_array_once == false) {
      string_found_in_array_once = true;
      is_single_occurrence = true;
    } 
    //second occurrence of string found
    else if (strcmp(string, array_of_strings[i]) == 0 && string_found_in_array_once == true) {
      is_single_occurrence = false;
      break;
    } 
  }
  return is_single_occurrence;
}

/*
 * This function returns true if:
 * 1. when of all param string_array elements, 2 or more matches param string
 */
bool utils_string_array_contains_multiple_occurrences_of_string(char **array_of_strings, int array_size, char *string) {
  int aos_size = array_size;
  bool string_found_in_array_once = false;
  bool string_found_in_array_multiple = false;
  bool is_multiple_occurrence = false;
  for(int i = 0; i < aos_size; i++) {
    //string found once
    if (strcmp(string, array_of_strings[i]) == 0 && string_found_in_array_multiple == false) {
      string_found_in_array_once = true;
    } 
    //second occurrence of string found
    else if (strcmp(string, array_of_strings[i]) == 0 && string_found_in_array_once == true) {
      is_multiple_occurrence = true;
      break;
    } 
  }
  return is_multiple_occurrence;
}

/*
 * This function converts number to ordinal numerals
 * credit to: https://stackoverflow.com/questions/46139112/printing-out-ordinals
 */
char* utils_fmt_ordinal(int number) {
  if(number < 0)
    return "vfo INTERNAL ERROR: negative_number_ordinal_error";
  char *suffix = "th";
  if(1<= number%10 && number%10 <= 3) {
    suffix =  (number%10 == 1) ?  "st"
            : (number%10 == 2) ?  "nd"
            :                     "rd"              
            ;
  }
  char *char_number = utils_convert_integer_to_char(number);
  char *final = strcat(char_number, suffix);
  return final;
}

char* utils_convert_integer_to_char(int N) {
    // Count digits in number N
    int m = N;
    int digit = 0;
    while (m) {
 
        // Increment number of digits
        digit++;
 
        // Truncate the last
        // digit from the number
        m /= 10;
    }
 
    // Declare char array for result
    char* arr;
 
    // Declare duplicate char array
    char arr1[digit];
 
    // Memory allocation of array
    arr = (char*)malloc(digit);
 
    // Separating integer into digits and
    // accommodate it to character array
    int index = 0;
    while (N) {
 
        // Separate last digit from
        // the number and add ASCII
        // value of character '0' is 48
        arr1[++index] = N % 10 + '0';
 
        // Truncate the last
        // digit from the number
        N /= 10;
    }
 
    // Reverse the array for result
    int i;
    for (i = 0; i < index; i++) {
        arr[i] = arr1[index - i];
    }
 
    // Char array truncate by null
    arr[i] = '\0';
 
    // Return char array
    return (char*)arr;
}

int utils_convert_string_to_integer(char *S) {
  char *endptr;
  return strtoimax(S, &endptr, 10);
}

void utils_remove_duplicates_in_string(char *string) {
  int length = strlen(string);
  //out most loop examines the entire string
  for (int i = 0; i < length; i++) {
    // one to the right of the character we are currently examining
    for (int j = i + 1; j < length;) {
      if (string[i] == string[j]) {
        for (int k = j; k < length; k++) {
          string[k] = string[k + 1];
          length--;
        }
      }
      else j++;
    }
  }
}

char** utils_remove_duplicates_in_array_of_strings(char *array[], int length, int *new_length) {
  char **new_array = malloc(length * sizeof(char*));
  int unique_count = 0;
  for (int i = 0; i < length; i++) {
    bool is_unique = true;
    for (int j = 0; j < unique_count; j++) {
      if(strcmp(new_array[j],array[i]) == 0) {
        is_unique = false;
        break;
      }
    }
    if(is_unique) {
      //not sure this will work, might need to strcpy...
      new_array[unique_count] = array[i];
      unique_count++;
    }
  }
  if(unique_count != length)
    new_array = realloc(new_array, unique_count * sizeof(char*));

  *new_length = unique_count;
  
  return new_array;
}

/*
 * NEED TO EXPLAIN THIS CLEARLY!
 */
active_cf_node_t* utils_generate_from_to_ll(cf_node_t *cf_head, char *from_cf_parent_folder, char *to_cf_parent_folder) {
  DIR *directory;
  struct dirent *entry;
  directory = opendir(from_cf_parent_folder);
  if(directory == NULL)
    printf("MAJOR ERROR: vfo could not open %s\n", from_cf_parent_folder);
  active_cf_node_t *active_cf_head = NULL;
  while((entry = readdir(directory)) != NULL) {
    if(entry->d_type == DT_DIR && !(utils_directory_is_current_or_parent(entry->d_name))) {
      char *tmp_current_custom_folder_type = cf_get_type_from_folder_name(cf_head, entry->d_name);
      char *from_cf_loc = utils_combine_to_full_path(from_cf_parent_folder, entry->d_name);
      char *to_cf_loc = utils_combine_to_full_path(to_cf_parent_folder, entry->d_name);
      
      active_cf_node_t *active_cf_node = active_cf_create_new_node(from_cf_loc, to_cf_loc, tmp_current_custom_folder_type);
      active_cf_insert_at_head(&active_cf_head, active_cf_node);
    }
  }
  if(closedir(directory) == -1)
    printf("MAJOR ERROR: vfo could not close this directory: %s\n", from_cf_parent_folder);  
  //for EVERY node in active_cf_head
  active_cf_node_t *tmp_active_cf = active_cf_head;
  active_cf_node_t *final_active_cf_head = NULL;
  if(tmp_active_cf != NULL) {
    while (tmp_active_cf != NULL) {
      //if the active_cf_node is of film type
      if(strcmp(tmp_active_cf->type, "films") == 0) {
        //create active_films_f_ll
        DIR *directory2;
        struct dirent *entry2;
        directory2 = opendir(tmp_active_cf->from_cf_folder);
        if(directory2 == NULL)
          printf("MAJOR ERROR: vfo could not open %s\n", tmp_active_cf->from_cf_folder);
        while((entry2 = readdir(directory2)) != NULL) {
          if(entry2->d_type == DT_DIR && !(utils_directory_is_current_or_parent(entry2->d_name))) {
            char *from_films_f_folder = utils_combine_to_full_path(tmp_active_cf->from_cf_folder, entry2->d_name); 
            char *to_films_f_folder = utils_combine_to_full_path(tmp_active_cf->to_cf_folder, entry2->d_name);
            active_films_f_node_t *active_films_f_node = active_films_f_create_node(from_films_f_folder, to_films_f_folder);
            active_films_f_insert_at_head(&tmp_active_cf->active_films_f_head, active_films_f_node);
          }
        }
        if(closedir(directory2) == -1)
          printf("MAJOR ERROR: vfo could not close this directory: %s\n", tmp_active_cf->from_cf_folder);
      }  
      //if the active_cf_node_t is of tv type
      else if (strcmp(tmp_active_cf->type, "tv") == 0) {
        //create active_tv_f_ll
        DIR *directory3;
        struct dirent *entry3;
        directory3 = opendir(tmp_active_cf->from_cf_folder);
        if(directory3 == NULL)
          printf("MAJOR ERROR: vfo could not open %s\n", tmp_active_cf->from_cf_folder);
        while((entry3 = readdir(directory3)) != NULL) {
          if(entry3->d_type == DT_DIR && !(utils_directory_is_current_or_parent(entry3->d_name))) {
            char *from_tv_f_folder = utils_combine_to_full_path(tmp_active_cf->from_cf_folder, entry3->d_name); 
            char *to_tv_f_folder = utils_combine_to_full_path(tmp_active_cf->to_cf_folder, entry3->d_name);
            int this_layer_is = 1;
            active_tv_f_node_t *active_tv_f_node = active_tv_f_create_new_node(from_tv_f_folder, to_tv_f_folder, this_layer_is);
            active_tv_f_insert_at_head(&tmp_active_cf->active_tv_f_head, active_tv_f_node);
          }
        }
        if(closedir(directory3) == -1)
          printf("MAJOR ERROR: vfo could not close this directory: %s\n", tmp_active_cf->from_cf_folder);

        active_tv_f_node_t *tmp_active_tv_head = tmp_active_cf->active_tv_f_head;
        if(tmp_active_tv_head !=NULL) {
          while(tmp_active_tv_head != NULL) {
              DIR *directory4;
              struct dirent *entry4;
              directory4 = opendir(tmp_active_tv_head->from_tv_f_folder);
              if(directory4 == NULL)
                printf("MAJOR ERROR: vfo could not open %s\n", tmp_active_tv_head->from_tv_f_folder);
              while((entry4 = readdir(directory4)) != NULL) {
                if(entry4->d_type == DT_DIR && !(utils_directory_is_current_or_parent(entry4->d_name))) {
                  char *from_tv_f_folder2 = utils_combine_to_full_path(tmp_active_tv_head->from_tv_f_folder, entry4->d_name);
                  char *to_tv_f_folder2 = utils_combine_to_full_path(tmp_active_tv_head->to_tv_f_folder, entry4->d_name);
                  int this_layer_is2 = 2;
                  active_tv_f_node_t *active_tv_f_node2 = active_tv_f_create_new_node(from_tv_f_folder2, to_tv_f_folder2, this_layer_is2);
                  active_tv_f_insert_at_head(&tmp_active_tv_head->active_tv_f_head, active_tv_f_node2);
                }
              }
              if(closedir(directory4) == -1)
                printf("MAJOR ERROR: vfo could not close this directory: %s\n", tmp_active_tv_head->from_tv_f_folder);
              
              active_tv_f_node_t *tmp_SECOND_LEVEL_active_tv_head = tmp_active_tv_head->active_tv_f_head;
              if(tmp_SECOND_LEVEL_active_tv_head != NULL) {
                while(tmp_SECOND_LEVEL_active_tv_head != NULL) {
                  DIR *directory5;
                  struct dirent *entry5;
                  directory5 = opendir(tmp_SECOND_LEVEL_active_tv_head->from_tv_f_folder);
                  if(directory5 == NULL)
                    printf("MAJOR ERROR: vfo could not open %s\n", tmp_SECOND_LEVEL_active_tv_head->from_tv_f_folder);
                  while((entry5 = readdir(directory5)) != NULL) {
                    if(entry5->d_type == DT_DIR && !(utils_directory_is_current_or_parent(entry5->d_name))) {
                      char *from_tv_f_folder3 = utils_combine_to_full_path(tmp_SECOND_LEVEL_active_tv_head->from_tv_f_folder, entry5->d_name);
                      char *to_tv_f_folder3 = utils_combine_to_full_path(tmp_SECOND_LEVEL_active_tv_head->to_tv_f_folder, entry5->d_name);
                      int this_layer_is3 = 3;
                      active_tv_f_node_t *active_tv_f_node3 = active_tv_f_create_new_node(from_tv_f_folder3, to_tv_f_folder3, this_layer_is3);
                      active_tv_f_insert_at_head(&tmp_SECOND_LEVEL_active_tv_head->active_tv_f_head, active_tv_f_node3);
                    }
                  }
                  if(closedir(directory5) == -1)
                    printf("MAJOR ERROR: vfo could not close this directory: %s\n", tmp_SECOND_LEVEL_active_tv_head->from_tv_f_folder);

                  tmp_SECOND_LEVEL_active_tv_head = tmp_SECOND_LEVEL_active_tv_head->next;
                }
              }
            tmp_active_tv_head = tmp_active_tv_head->next;
          }
        }
      }
      active_cf_node_t *finale_active_cf_node = active_cf_create_node(tmp_active_cf->from_cf_folder, tmp_active_cf->to_cf_folder, tmp_active_cf->type, tmp_active_cf->active_films_f_head, tmp_active_cf->active_tv_f_head);
      active_cf_insert_at_head(&final_active_cf_head, finale_active_cf_node);
      tmp_active_cf = tmp_active_cf->next;
    }
  }
  return final_active_cf_head;
}

char* utils_replace_characters(char *before, char *oldsub, char *newsub) {
  int old_length = strlen(oldsub);
  int new_length = strlen(newsub);
  int before_length = strlen(before);

  char *after;

  if (old_length == new_length) {
    after = malloc((before_length + 1) * sizeof(char));
  }
  else {
    int occurrences = 0;
    int i = 0;
    while(i < before_length) {
      if(strstr(&before[i], oldsub) == &before[i]) {
        occurrences++;
        i += old_length;
      }
      else i++;
    }

    int sub_diff = new_length - old_length;
    int after_length = before_length;
    after_length += occurrences * sub_diff;

    after = malloc((after_length + 1) * sizeof(char));
  }

  int i = 0;
  int j = 0;

  while (i < before_length) {
    if (strstr(&before[i], oldsub) == &before[i]) {
      strcpy(&after[j], newsub);
      i += old_length;
      j += new_length;

    }
    else {
      after[j] = before[i];
      i++;
      j++;
    }
  }

  after[j] = '\0';

  return after;
}


void utils_danger_delete_contents_of_folder(char *folder) {
  DIR *directory;
  struct dirent *entry;
  directory = opendir(folder);
  if(directory == NULL) {
    printf("MAJOR ERROR: Could not open this folder %s delete it's contents.\n", folder);
    exit(EXIT_FAILURE);
  }

  while((entry=readdir(directory))!= NULL) {
    if(entry->d_type == DT_DIR && !(utils_directory_is_current_or_parent(entry->d_name))) {
      //found folders inside folder, this should NEVER happen
      printf("MAJOR ERROR: found a folder within a folder of which we never expected a folder\n");
      exit(EXIT_FAILURE);
    } else if (entry->d_type == DT_REG && !(utils_file_is_macos_hidden_files(entry->d_name))) {
      //found files that we are now allowed to delete.
      printf("remove runs\n");
      printf("attempting to remove %s\n", entry->d_name);
      char *for_removal = utils_combine_to_full_path(folder, entry->d_name);
      if(remove(for_removal) == 0) {
        printf("WIPED FILE: %s\n", entry->d_name);
      } else {
        printf("FAILED TO WIPE FILE: %s\n", entry->d_name);
      }
    }
  }
  if(closedir(directory) == -1) {
    printf("MAJOR ERROR: Could not close this folder %s after deleting it's contents.\n", folder);
  }
}

bool utils_string_only_contains_number_characters(char *string) {
  while (*string) {
        if (!(isdigit(*string++) == 0))
          return false;
    }
  return true;
}

bool utils_string_is_ffmpeg_timecode_compliant(char *string) {
  //string must only be 8 characters long
  int stringLength = 8;

  if(strlen(string) != stringLength)
    return false;

  for (int i = 0; i <= stringLength && string[i] != '\0'; i++) {
    //first 2 characters must be digits
    if(i == 0 || i == 1 && isdigit(string[i]) != 0)
      return false;
    //third character must be :
    if(i == 2 && string[i] != ':')
      return false;
    //fourth and fifth character must be digits
    if(i == 3 || i == 4 && isdigit(string[i]) != 0)
      return false;
    //sixth character must be :
    if(i == 5 && string[i] != ':')
      return false;
    //seventh and eights character must be digits
    if(i == 6 || i == 7 && isdigit(string[i]) != 0)
      return false;
  }
  return true;
}
