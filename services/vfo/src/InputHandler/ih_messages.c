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

/*
 * Input Handler Help message
 */
void ih_mes_help () {
  fprintf(stdout, BLUE __PROGRAM_NAME__ "\n\n" NO_COLOR);
  ih_mes_usage();
  ih_mes_description();
  ih_mes_options();
  ih_mes_arguments();
  ih_mes_author();
  ih_mes_version();
}

/*
 * Input Handler Usage message
 */
void ih_mes_usage () {
  fprintf(stdout, BROWN "Usage: " NO_COLOR);
  fprintf(stdout, "%s [argument] || [options]\n\n", __PROGRAM_NAME__);
  fprintf(stdout, BROWN "Usage examples: " NO_COLOR);
  fprintf(stdout, "%s doctor      # check environment and config readiness\n\n", __PROGRAM_NAME__);
  fprintf(stdout, "%s wizard      # guided config setup/update\n\n", __PROGRAM_NAME__);
  fprintf(stdout, "%s show        # print active config summary\n\n", __PROGRAM_NAME__);
  fprintf(stdout, "%s status      # high-level engine observability snapshot\n\n", __PROGRAM_NAME__);
  fprintf(stdout, "%s status-json # machine-readable observability snapshot\n\n", __PROGRAM_NAME__);
  fprintf(stdout, "%s status_json # legacy alias for status-json\n\n", __PROGRAM_NAME__);
  fprintf(stdout, "%s run         # run default pipeline end-to-end\n\n", __PROGRAM_NAME__);
  fprintf(stdout, "%s original    # mezzanine preparation stage\n\n", __PROGRAM_NAME__);
  fprintf(stdout, "%s source      # normalized source stage\n\n", __PROGRAM_NAME__);
  fprintf(stdout, "%s all_aliases # run all delivery profiles (legacy term: aliases)\n\n", __PROGRAM_NAME__);
  fprintf(stdout, "%s <profile_name> # run one delivery profile by name (legacy runtime term: alias)\n\n", __PROGRAM_NAME__);
  fprintf(stdout, "%s --version\n\n", __PROGRAM_NAME__);
  fprintf(stdout, "%s --help\n\n", __PROGRAM_NAME__);
}

/*
 * Input Handler Description message
 */
void ih_mes_description () {
    fprintf(stdout, BROWN "Description: " NO_COLOR);
    fprintf(stdout, "vfo is a CLU that sits on top of FFmpeg.  The program\n"
            "gives you the power of automation when it comes to encoding\n"
            "your video files in bulk in preparation for video streaming.\n"
            "Use `status`/`status-json` for component-level visibility.\n"
            "Pipeline terminology: mezzanine -> source -> profile\n"
            "(legacy runtime/config term: alias).\n");
}

/*
 * Input Handler Options message
 */
void ih_mes_options () {
    fprintf(stdout, BROWN "Options:\n\n" NO_COLOR);
    fprintf(stdout, GRAY "\t-v|--version\n" NO_COLOR
                    "\t\tPrints %s version\n\n", __PROGRAM_NAME__);
    fprintf(stdout, GRAY "\t-h|--help\n" NO_COLOR
                    "\t\tPrints this help message\n\n");
    fprintf(stdout, GRAY "\t--no-color\n" NO_COLOR
                    "\t\tDoes not use colors for printing\n\n");
}

/*
 * Input Handler Arguments message
 */
void ih_mes_arguments() {
      fprintf(stdout, BROWN "Arguments:\n\n" NO_COLOR);
    fprintf(stdout, GRAY "\tdoctor\n" NO_COLOR
                    "\t\trun preflight checks (commands, paths, config)\n\n");
    fprintf(stdout, GRAY "\twizard\n" NO_COLOR
                    "\t\tinteractive setup/update for vfo_config.conf\n\n");
    fprintf(stdout, GRAY "\tshow\n" NO_COLOR
                    "\t\tprint current vfo configuration summary\n\n");
    fprintf(stdout, GRAY "\tstatus\n" NO_COLOR
                    "\t\tprint high-level component readiness and stage status\n\n");
    fprintf(stdout, GRAY "\tstatus-json\n" NO_COLOR
                    "\t\tprint machine-readable status report for automation/tests\n\n");
    fprintf(stdout, GRAY "\tstatus_json (legacy)\n" NO_COLOR
                    "\t\tcompatibility alias for status-json\n\n");
    fprintf(stdout, GRAY "\trun\n" NO_COLOR
                    "\t\texecutes default pipeline: mezzanine -> source (if enabled) -> profiles\n\n");
    fprintf(stdout, GRAY "\toriginal\n" NO_COLOR
                    "\t\texecutes mezzanine preparation stage only\n\n");
    fprintf(stdout, GRAY "\tsource\n" NO_COLOR
                    "\t\texecutes normalized source stage only\n\n");
    fprintf(stdout, GRAY "\t<profile_name>\n" NO_COLOR
                    "\t\texecutes a specific delivery profile by name (legacy: alias)\n\n");
    fprintf(stdout, GRAY "\tdo_it_all\n" NO_COLOR
                    "\t\texecutes full default pipeline: mezzanine -> source -> profiles (legacy command)\n\n");
}

/*
 * Input Handler Author message
 */
void ih_mes_author () {
    fprintf(stdout, BROWN "Written by: " GRAY "%s\n\n" NO_COLOR,
            __PROGRAM_AUTHOR__);
}

/*
 * Input Handler Version message
 */
void ih_mes_version () {
    fprintf(stdout, __PROGRAM_NAME__ " version: " GRAY "%s\n" NO_COLOR,
            __PROGRAM_VERSION__);
    printf("Real Version: %s\n", VERSION);
}

/*
 * Input Handler User provided no arguments and no options
 */
void ih_mes_user_provided_no_args_and_no_options() {
  fprintf(stdout, __PROGRAM_NAME__ " found no arguments and no options in your command.\n");
  fprintf(stdout, "please type 'vfo --help' if you are unsure how to interact with vfo.\n");
}

void ih_mes_user_provided_help_option() {
  ih_mes_help();
}

void ih_mes_user_provided_version_option() {
  ih_mes_version();
}
