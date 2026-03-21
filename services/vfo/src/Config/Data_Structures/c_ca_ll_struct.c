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

#include "c_ca_ll_struct.h"

ca_node_t* ca_create_new_empty_node() {
  ca_node_t *result = malloc(sizeof(ca_node_t));
  result->alias_name = "";
  result->alias_location = "";
  result->alias_crit_codec = "";
  result->alias_crit_bits = "";
  result->alias_crit_color_space = "";
  result->alias_crit_min_width = "";
  result->alias_crit_min_height = "";
  result->alias_crit_max_width = "";
  result->alias_crit_max_height = "";
  // result->cs_head = cs_create_new_empty_node();
  result->cs_head = NULL;
  
  result->is_alias_name_valid = false;
  result->is_alias_location_valid = false;
  result->is_alias_crit_codec_valid = false;
  result->is_alias_crit_bits_valid = false;
  result->is_alias_crit_color_space_valid = false;
  result->is_alias_crit_min_width_valid = false;
  result->is_alias_crit_min_height_valid = false;
  result->is_alias_crit_max_width_valid = false;
  result->is_alias_crit_max_height_valid = false;

  result->is_alias_cs_head_valid = false;

  result->is_entire_ca_node_valid = false;

  result->next = NULL;
  return result;
} 

ca_node_t* ca_create_new_node(char *alias_name,
                              char *alias_location,
                              char *alias_crit_codec,
                              char *alias_crit_bits,
                              char *alias_crit_color_space,
                              char *alias_crit_min_width,
                              char *alias_crit_min_height,
                              char *alias_crit_max_width,
                              char *alias_crit_max_height,
                              cs_node_t *cs_head) {

  ca_node_t *result = malloc(sizeof(ca_node_t));
  result->alias_name = alias_name;
  result->alias_location = alias_location;
  result->alias_crit_codec = alias_crit_codec;
  result->alias_crit_bits = alias_crit_bits;
  result->alias_crit_color_space = alias_crit_color_space;
  result->alias_crit_min_width = alias_crit_min_width;
  result->alias_crit_min_height = alias_crit_min_height;
  result->alias_crit_max_width = alias_crit_max_width;
  result->alias_crit_max_height = alias_crit_max_height;
  result->cs_head = cs_head;

  result->is_alias_name_valid = false;
  result->is_alias_location_valid = false;
  result->is_alias_crit_codec_valid = false;
  result->is_alias_crit_bits_valid = false;
  result->is_alias_crit_color_space_valid = false;
  result->is_alias_crit_min_width_valid = false;
  result->is_alias_crit_min_height_valid = false;
  result->is_alias_crit_max_width_valid = false;
  result->is_alias_crit_max_height_valid = false;

  result->is_alias_cs_head_valid = false;

  result->is_entire_ca_node_valid = false;

  result->next = NULL;
  return result;
}

ca_node_t *ca_insert_at_head(ca_node_t **head, ca_node_t *node_to_insert) {
  node_to_insert->next = *head;
  *head = node_to_insert;
  return node_to_insert;
}

void ca_insert_after_node(ca_node_t *node_to_insert_after, ca_node_t* newnode) {
  newnode->next = node_to_insert_after->next;
  node_to_insert_after->next = newnode;
}

ca_node_t* ca_find_node(ca_node_t *head, char *alias_name) {
  // printf("parameter: %s\n", alias_name);
  ca_node_t *tmp = head;
  while (tmp != NULL) {
    // printf("tmp->alias_name_content: %s\n", tmp->alias_name_content);
    if(strcmp(tmp->alias_name, alias_name) == 0){
      // printf("return tmp occured\n");
      return tmp;
    }
    tmp = tmp->next;
  }
  return NULL;
}

void ca_print_list(ca_node_t *head) {
  ca_node_t *temporary = head;

  if(temporary != NULL) {
    while (temporary != NULL) {
      printf ("%s - ", temporary->alias_name);
      printf("\n");
      printf("%s, %s, %s, %s, %s, %s, %s, %s\n", temporary->alias_location,
                                                temporary->alias_crit_codec,
                                                temporary->alias_crit_bits,
                                                temporary->alias_crit_color_space,
                                                temporary->alias_crit_min_width,
                                                temporary->alias_crit_min_height,
                                                temporary->alias_crit_max_width,
                                                temporary->alias_crit_max_height);
      
      temporary = temporary->next;
    }
    printf("\n");
  } else printf("list is empty\n");
}

bool ca_alias_name_exits(ca_node_t *head, char *alias_name) {
  ca_node_t *tmp = head;
  while (tmp != NULL) {
    if (strcmp(tmp->alias_name, alias_name) == 0) return true;
    tmp = tmp->next;
  }
  return false;
}

int ca_get_count(ca_node_t *head) {
  ca_node_t *temporary = head;
  int counter = 0;
  if(temporary != NULL) {
    while (temporary != NULL) {
      counter++;
      temporary = temporary->next;
    }
  }
  return counter;
}

char* ca_get_a_alias_name_from_count(ca_node_t *head, int count) {
  ca_node_t *temporary = head;
  int counter = 0;
  bool alias_word_found = false;
  if(temporary != NULL) {
    while(temporary != NULL) {
      if(counter == count) {
        alias_word_found = true;
        break;
      }
      temporary = temporary->next;
    }
  }
  if(alias_word_found == true)
    return temporary->alias_name;
  return 0;
}

// char *ca_get_a_alias_name_from_alias_name(ca_node_t *head, char *alias_name) {

// }

char* ca_get_a_alias_location_from_count(ca_node_t *head, int count) {
  ca_node_t *temporary = head;
  int counter = 0;
  bool alias_location_content_found = false;
  if(temporary != NULL) {
    while(temporary != NULL) {
      if(counter == count) {
        alias_location_content_found = true;
        break;
      }
      temporary = temporary->next;
    }
  }
  if(alias_location_content_found == true)
    return temporary->alias_location;
  return 0;
}

char* ca_get_a_alias_crit_codec_from_count(ca_node_t *head, int count) {
  ca_node_t *temporary = head;
  int counter = 0;
  bool alias_crit_codec_found = false;
  if(temporary != NULL) {
    while(temporary != NULL) {
      if(counter == count) {
        alias_crit_codec_found = true;
        break;
      }
      temporary = temporary->next;
    }
  }
  if(alias_crit_codec_found == true)
    return temporary->alias_crit_codec;
  return 0;
}

char* ca_get_a_alias_crit_bits_from_count(ca_node_t *head, int count) {
  ca_node_t *temporary = head;
  int counter = 0;
  bool alias_crit_bits_found = false;
  if(temporary != NULL) {
    while(temporary != NULL) {
      if(counter == count) {
        alias_crit_bits_found = true;
        break;
      }
      temporary = temporary->next;
    }
  }
  if(alias_crit_bits_found == true)
    return temporary->alias_crit_bits;
  return 0;
}

char* ca_get_a_alias_crit_color_space_from_count(ca_node_t *head, int count) {
  ca_node_t *temporary = head;
  int counter = 0;
  bool alias_crit_color_space_found = false;
  if(temporary != NULL) {
    while(temporary != NULL) {
      if(counter == count) {
        alias_crit_color_space_found = true;
        break;
      }
      temporary = temporary->next;
    }
  }
  if(alias_crit_color_space_found == true)
    return temporary->alias_crit_color_space;
  return 0;
}

char* ca_get_a_alias_crit_min_width_from_count(ca_node_t *head, int count) {
  ca_node_t *temporary = head;
  int counter = 0;
  bool alias_crit_min_width_found = false;
  if(temporary != NULL) {
    while(temporary != NULL) {
      if(counter == count) {
        alias_crit_min_width_found = true;
        break;
      }
      temporary = temporary->next;
    }
  }
  if(alias_crit_min_width_found == true)
    return temporary->alias_crit_min_width;
  return 0;
}

char* ca_get_a_alias_crit_min_height_from_count(ca_node_t *head, int count) {
  ca_node_t *temporary = head;
  int counter = 0;
  bool alias_crit_min_height_found = false;
  if(temporary != NULL) {
    while(temporary != NULL) {
      if(counter == count) {
        alias_crit_min_height_found = true;
        break;
      }
      temporary = temporary->next;
    }
  }
  if(alias_crit_min_height_found == true)
    return temporary->alias_crit_min_height;
  return 0;
}

char* ca_get_a_alias_crit_max_width_from_count(ca_node_t *head, int count) {
  ca_node_t *temporary = head;
  int counter = 0;
  bool alias_crit_max_width_found = false;
  if(temporary != NULL) {
    while(temporary != NULL) {
      if(counter == count) {
        alias_crit_max_width_found = true;
        break;
      }
      temporary = temporary->next;
    }
  }
  if(alias_crit_max_width_found == true)
    return temporary->alias_crit_max_width;
  return 0;
}

char* ca_get_a_alias_crit_max_height_from_count(ca_node_t *head, int count) {
  ca_node_t *temporary = head;
  int counter = 0;
  bool alias_crit_max_height_found = false;
  if(temporary != NULL) {
    while(temporary != NULL) {
      if(counter == count) {
        alias_crit_max_height_found = true;
        break;
      }
      temporary = temporary->next;
    }
  }
  if(alias_crit_max_height_found == true)
    return temporary->alias_crit_max_height;
  return 0;
}

cs_node_t* ca_get_a_alias_cs_head_from_count(ca_node_t *head, int count) {
  ca_node_t *temporary = head;
  int counter = 0;
  bool alias_cs_head_found = false;
  if(temporary != NULL) {
    while(temporary != NULL) {
      if(counter == count) {
        alias_cs_head_found = true;
        break;
      }
      temporary = temporary->next;
    }
  }
  if(alias_cs_head_found == true)
    return temporary->cs_head;
  return 0;
}

/*
 * This function stops vfo if a duplicate alias_name is detected in a ca_head ll 
 */
void ca_validate_alias_name_duplicates(ca_node_t *head) {
  ca_node_t *temporary = head;
  ca_node_t *temporary_2 = head;
  bool duplicate_found = false;
  int dupe_count = 0;
  if(temporary != NULL) {
    while (temporary != NULL) {
      if(temporary_2 != NULL) {
        while (temporary_2 !=NULL) {
          if(strcmp(temporary->alias_name, temporary_2->alias_name) == 0) {
            dupe_count++;
            if(dupe_count == 2) {
              duplicate_found = true;
              break;
            }
          }
          temporary_2 = temporary_2->next;
        }
      }
      dupe_count = 0;
      temporary = temporary->next;
    }
  }

  if(duplicate_found == true) {
    printf("CONFIG FILE MAJOR ERROR: vfo found duplicate alias names in the config file CONFIG FILE MAJOR ERROR\n");
    printf("Please make sure all of your ALIAS='names' have unique names!\n");
    exit(EXIT_FAILURE);
  } 
  else {
    printf("CONFIG FILE ALERT: All Alias Names are unique\n");
  }
}


/*
 * This function returns a single ca_node if it's alias_name matches param
 */
ca_node_t* ca_get_a_node_from_alias_name(ca_node_t *head, char *alias_name) {
  ca_node_t *temporary = head;
  if(temporary != NULL) {
    while (temporary != NULL) {
      if(strcmp(temporary->alias_name, alias_name) == 0) {
        return temporary;
      }
      temporary = temporary->next;
    }
  }
  printf("DEV NOTE: MAJOR ERROR, ca_get_a_node_from_alias_name has failed\n");
  exit(EXIT_FAILURE);
  return temporary;
}

/*
 * This function returns a single ca_node if it's count matches param
 */
ca_node_t* ca_get_a_node_from_count(ca_node_t *head, int count) {
  ca_node_t *temporary = head;
  int counter = 0;
  if(temporary != NULL) {
    while (temporary != NULL) {
      if(count == counter) {
        return temporary;
      }
      counter++;
      temporary = temporary->next;
    }
  }
  printf("DEV NOTE: MAJOR ERROR, ca_get_a_node_from_count has failed\n");
  exit(EXIT_FAILURE);
  return temporary;
}
