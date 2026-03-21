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

#include "ih_options_struct.h"


options_t* options_create_new_struct() {
  options_t *result = malloc(sizeof(options_t));
  result->help_detected = false;
  result->version_detected = false;
  return result;
}

// /*
//  * Sets the default options
//  */
// void ih_set_default_options(options_t* options) {
//     options->help = false;
//     options->version = false;
// }


void ih_options_parser(int argc, char* argv[], options_t* options) {
  int arg; /* Current option */
  /* getopt allowed options */
  static struct option long_options[] = {
      /*General*/
      {"help", no_argument, 0, 'h'},
      {"version", no_argument, 0, 'v'},
  };
  while (true) {
    int option_index = 0;
    arg = getopt_long(argc, argv, "hvt:", long_options, &option_index);
    /* End of the options? */
    if (arg == -1) break;
    /* Find the matching case of the argument */
    ih_options_switch(arg, options);
  }
}

/*
 * Finds the matching case of the current command line option
 */
void ih_options_switch(int arg, options_t* options) {
  switch (arg)
  {
      case 'h':
        options->help_detected = true;
        break;
        // ih_mes_help();
        // exit(EXIT_SUCCESS);

      case 'v':
        options->version_detected = true;
        break;
        // ih_mes_version();
        // exit(EXIT_SUCCESS);
      // case '?':
      //   ih_mes_usage();
      //   exit(EXIT_FAILURE);

      // default:
      //   ih_mes_usage();
      //   abort();
  }
}
