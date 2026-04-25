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

#ifndef IH_TESTS_H
#define IH_TESTS_H

#include "../t_internal.h"

#include "../../src/InputHandler/Data_Structures/ih_arguments_struct.h"
#include "../../src/quality_scoring.h"
#include "../../src/Status/status_engine.h"

void ih_resolve_config_dir_for_test(char *output, size_t output_size, bool wizard_mode);
void ih_evaluate_tier_states_for_test(bool ffmpeg_available,
                                      bool ffprobe_available,
                                      bool mkvmerge_available,
                                      bool dovi_tool_available,
                                      bool quality_enabled,
                                      bool quality_include_vmaf,
                                      bool libvmaf_available,
                                      status_state_t *base_state_out,
                                      status_state_t *dv_state_out,
                                      status_state_t *quality_state_out);
const char* ih_library_install_hint_for_test(const char *library_name);
bool ih_resolve_stock_preset_for_test(const char *token,
                                      char *canonical_key_out,
                                      size_t canonical_key_out_size,
                                      char *relative_path_out,
                                      size_t relative_path_out_size);

void test_status_report_update_and_summary(void **state);
void test_status_report_update_overwrites_existing_component(void **state);
void test_status_report_print_json_contract(void **state);
void test_ih_arguments_parser_detects_status_commands(void **state);
void test_ih_arguments_parser_detects_mezzanine_and_profiles_commands(void **state);
void test_ih_arguments_parser_detects_visualize_command(void **state);
void test_ih_arguments_parser_detects_auto_command(void **state);
void test_quality_reference_mode_parser_accepts_valid_values(void **state);
void test_quality_reference_mode_parser_rejects_invalid_values(void **state);
void test_ih_resolve_config_dir_prefers_env_override(void **state);
void test_ih_resolve_config_dir_uses_user_dir_for_wizard(void **state);
void test_ih_tier_state_evaluation(void **state);
void test_ih_library_install_hint_supports_libplacebo(void **state);
void test_ih_stock_preset_alias_resolution_supports_craigstreamy_and_legacy_name(void **state);
void test_ih_stock_preset_resolution_supports_aggressive_vmaf_pack(void **state);
void test_ih_stock_preset_resolution_supports_video_only_aggressive_vmaf_pack(void **state);
void test_ih_stock_preset_resolution_supports_all_sub_preserve_pack(void **state);
void test_ih_stock_preset_resolution_supports_all_sub_audio_conform_pack(void **state);
void test_ih_stock_preset_resolution_supports_audio_conform_pack(void **state);
void test_ih_stock_preset_resolution_supports_roku_family_pack(void **state);
void test_ih_stock_preset_resolution_supports_fire_tv_family_pack(void **state);
void test_ih_stock_preset_resolution_supports_chromecast_family_pack(void **state);
void test_ih_stock_preset_resolution_supports_apple_tv_family_pack(void **state);
void test_ih_stock_preset_resolution_supports_fire_tv_dv_pack(void **state);
void test_ih_stock_preset_resolution_supports_subtitle_convert_pack(void **state);
void test_ih_stock_preset_resolution_supports_subtitle_convert_audio_conform_pack(void **state);

#endif // IH_TESTS_H
