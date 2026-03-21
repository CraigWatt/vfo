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

#ifndef C_CONFIG_STRUCT_H
#define C_CONFIG_STRUCT_H
/*Config Struct*/

#include <stdlib.h>

#include "c_svm_struct.h"
#include "c_svc_struct.h"
#include "c_cf_ll_struct.h"
#include "c_ca_ll_struct.h"
#include "c_uw_ll_struct.h"

struct config {
  struct sole_var_markers *svm;
  struct sole_var_content *svc;  
  struct uw_node *uw_head;
  struct cf_node *cf_head;
  struct ca_node *ca_head;
};
typedef struct config config_t;

config_t* config_create_new_struct();

#endif // C_CONFIG_STRUCT_H
