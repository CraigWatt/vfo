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

#ifndef IH_INTERNAL_H
#define IH_INTERNAL_H

/* ---------- This is an Internal Header File ----------
These Functions and variables should only ever be used internally
*/

/* preprocessor defines general info */
#define __PROGRAM_NAME__ "vfo"
#define __PROGRAM_VERSION__ "0.1.0"
#define __PROGRAM_AUTHOR__ "Craig Watt (craig@webrefine.co.uk)"

/* ANSI color definitions for pretty-print*/
#define NO_COLOR    "\x1b[0m"
#define RED         "\x1b[31m"
#define GREEN       "\x1b[32m"
#define BROWN       "\x1b[33m"
#define BLUE        "\x1b[34m"
#define MAGENTA     "\x1b[35m"
#define CYAN        "\x1b[36m"
#define GRAY        "\x1b[37m"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <string.h>

//include necessary public facing headers
#include "../input_handler.h"

//include relevant structs
#include "Data_Structures/ih_options_struct.h"
#include "Data_Structures/ih_arguments_struct.h"

//include relevant container structs
#include "Data_Structures/ih_input_handler_struct.h"

//ih_messages
void ih_mes_help();
void ih_mes_usage();
void ih_mes_description();
void ih_mes_options();
void ih_mes_author();
void ih_mes_version();
void ih_mes_user_provided_no_args_and_no_options();
void ih_mes_user_provided_help_option();
void ih_mes_user_provided_version_option();


#endif // IH_INTERNAL_H
