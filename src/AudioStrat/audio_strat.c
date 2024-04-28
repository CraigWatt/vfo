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

 #include "as_internal.h"

// void as_pre_encode_checks(audio_strat_t *audio_strat);
// void as_highlight_encode_candidates_to_user(audio_strat_t *audio_strat);
// void as_encode_from_source_to_source(audio_strat_t *audio_strat);
// char* as_generate_ffmpeg_command(char *source_from, char *source_to);
// char* as_generate_audio_strat_file_name(char *source_from, char *source_to);
// bool as_execute_ffmpeg_command(char *source_from, char *source_to);

void as_highlight_encode_candidates_to_user(audio_strat_t *audio_strat) {
  if(strcmp(audio_strat->source_content, "") != 0) {
    printf("AUDIO STRAT ALERT: found source content\n");
    as_highlight_encode_candidates_from_source_content(audio_strat);
  }
}





