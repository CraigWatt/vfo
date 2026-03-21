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

#include "c_internal.h"

void c_mes_init_find_config_file() {
  fprintf(stdout, "find_config_file initiating...\n");
}

void c_mes_find_config_file_success() {
  fprintf(stdout, "mes_find_config_file_success FOUND\n");
}

void c_mes_find_config_file_fail() {
  fprintf(stdout, "mes_find_config_file_fail NOT FOUND\n"
          "vfo_config.conf file is essential for vfo to run\n"
          "vfo_config.conf must be located in /usr/local/bin directory\n");
}

void c_mes_init_access_config_variable() {
  fprintf(stdout, "mes_init_access_config_variable initiating...\n");
}

void c_mes_extract_to_string() {
  fprintf(stdout, "mes_extract_to_string initiating...\n");
}

void c_mes_init_extract_to_variables() {
  fprintf(stdout, "mes_init_extract_to_variables initiating...\n");
}

void c_mes_init_generate_default_config_file() {
    fprintf(stdout, "mes_init_generate_default_config_file initiating...\n");
    fprintf(stdout, "UNFORTUNATELY, THIS FEATURE IS STILL IN DEVELOPMENT.  PLEASE REINSTALL vfo...\n"
            "PLEASE CHECK TO SEE IF /usr/local/bin/vfo_config.conf is present\n"
            "PLEASE CHECK TO SEE IF vfo executable is installed in /usr/local/bin as vfo\n");
}


void c_mes_particular_variable_has_problems(char* var_name_string, const char* conf_file_name, int count) {
  fprintf(stdout, "mes_particular_variable_has_problems ERROR\n"
          "It looks like this essential variable: %s \n"
          "in: %s \n"
          "had a count of: %i \n", var_name_string, conf_file_name, count);
  if (count >= 2) {
    fprintf(stdout, "Help message: Duplicate variables could be present in config file\n");
  } else if (count == 0) {
    fprintf(stdout, "Help message: Essential Variable could be missing from config file\n");
  } else {
    fprintf(stdout, "Help message: Very strange error, please inform lead dev\n"); 
  }          
}

void c_mes_essential_variable_found(char* var_name_string) {
   fprintf(stdout, "mes_essential_variable_found SUCCESS - found %s in configFile\n", var_name_string);
}
