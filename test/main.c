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

/* main.h handles all includes relevant to tests */
#include "main.h"

/* A test case that does nothing and succeeds. */
static void null_test_success(void **state) {
    /**
     * If you want to know how to use cmocka, please refer to:
     * https://api.cmocka.org/group__cmocka__asserts.html
     */
    (void) state; /* unused */
}

/**
 * Test runner function
 */
#ifdef TESTING
int main(void) {
    /**
     * Insert here your test functions
     */
    const struct CMUnitTest tests[] = {
        /* so, we want to test all functions related to Input Handler */

        // to be added

        /* Utils */
        cmocka_unit_test(test_utils_does_folder_exist_null_ptr),
        cmocka_unit_test(test_utils_does_folder_exist_empty_str),
        /* Config */
        cmocka_unit_test(test_con_init_config_dir_null_ptr),
        cmocka_unit_test(test_con_init_config_dir_empty_str),
        /* Original */
        cmocka_unit_test(test_o_original_null_ptr),
        /* Source */
        cmocka_unit_test(test_s_original_to_source_null_ptr),
        /* Alias */
        cmocka_unit_test(test_a_source_to_aliases),

        /* Other (see above)*/
        cmocka_unit_test(null_test_success),
        /* Done */
    };

    /* Run the tests */
    return cmocka_run_group_tests(tests, NULL, NULL);
}
#endif
