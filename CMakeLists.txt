cmake_minimum_required(VERSION 3.16)
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VITASDK})
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "VitaSDK toolchain")
endif()
project(CrashNitroKart3DNative C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Módulos REALES del futuro juego nativo. No emulador ni motor simulado.
add_library(cnk_native_core STATIC
    src/fufs.c
    src/cnk_pfnt.c
    src/cnk_lang.c
    src/cnk_pfnt_glyph_data.c
    src/cnk_model.c
    src/cnk_model_texl.c
    src/cnk_model_material.c
    src/cnk_source_kart_catalog.c
    src/cnk_scene.c
    src/cnk_nav.c
    src/cnk_route_direction.c
    src/cnk_minimap.c
    src/cnk_geometry.c
    src/cnk_collision.c
    src/cnk_collision_probe.c
    src/cnk_race.c
    src/cnk_race_progress.c
    src/cnk_cup.c
    src/cnk_story.c
    src/cnk_story_manifest.c
    src/cnk_story_checkpoint.c
    src/cnk_cup_manifest.c
    src/cnk_cup_checkpoint.c
    src/cnk_results_text.c
    src/cnk_records.c
    src/cnk_start_sequence.c
    src/cnk_launch.c
    src/cnk_drive.c
    src/cnk_boost.c
    src/cnk_boost_pads.c
    src/cnk_pad_layout.c
    src/cnk_ai_watchdog.c
    src/cnk_ai_tactics.c
    src/cnk_ai_traffic.c
    src/cnk_projectile_threat.c
    src/cnk_items.c
    src/cnk_item_visual.c
    src/cnk_item_visibility.c
    src/cnk_item_sprite_map.c
    src/cnk_item_layout.c
    src/cnk_kart_contact.c
    src/cnk_render.c
    src/cnk_object_render.c
    src/cnk_racer_visual.c
    src/cnk_racer_assets.c
    src/cnk_kart_skins.c
    src/cnk_driver_bindings.c
    src/cnk_dev_race_visuals.c
    src/cnk_model_lod.c
    src/cnk_camera.c
    src/cnk_frustum.c
    src/cnk_png.c
    src/cnk_texture_map.c
    src/cnk_texture_manifest.c
    src/cnk_session.c
    src/cnk_audio.c
    src/cnk_music.c
    src/cnk_music_cues.c
    src/cnk_game.c
    src/cnk_frontend.c
    src/cnk_frontend_actions.c
    src/cnk_app_runtime.c
    src/cnk_vita_diagnostics.c
    src/cnk_vita_telemetry.c
    src/cnk_vita_source_path.c
    src/cnk_sound_cues.c
    src/cnk_track_catalog.c
)
target_include_directories(cnk_native_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

# En PC solo se prueban algoritmos portátiles. Las pruebas no son el videojuego.
if(NOT DEFINED ENV{VITASDK})
  enable_testing()
  # Auditoría reproducible: 3DOB y materiales originales sin asignaciones inventadas.
  add_executable(cnk_exportar_catalogo tools/exportar_catalogo_pistas.c)
  target_link_libraries(cnk_exportar_catalogo PRIVATE cnk_native_core z m)
  add_executable(cnk_exportar_materiales_tomados_de_rom tools/exportar_materiales_tomados_de_rom.c)
  target_link_libraries(cnk_exportar_materiales_tomados_de_rom PRIVATE cnk_native_core z m)
  add_executable(cnk_exportar_materiales_galeria tools/exportar_materiales_galeria_original.c)
  target_compile_options(cnk_exportar_materiales_galeria PRIVATE -Wall -Wextra -Werror)
  target_link_libraries(cnk_exportar_materiales_galeria PRIVATE cnk_native_core z m)
  # Source PFNT glyph index and metrics audit. No fabricated font graphics.
  add_executable(cnk_exportar_catalogo_pfnt tools/exportar_catalogo_pfnt_original.c)
  target_link_libraries(cnk_exportar_catalogo_pfnt PRIVATE cnk_native_core z m)
  target_compile_options(cnk_exportar_catalogo_pfnt PRIVATE -Wall -Wextra -Werror)
  add_executable(cnk_test_pfnt tests/test_native_pfnt.c)
  target_link_libraries(cnk_test_pfnt PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_pfnt PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME pfnt COMMAND cnk_test_pfnt "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_exportar_atlas_pfnt_codificado tools/exportar_atlas_pfnt_codificado.c)
  target_link_libraries(cnk_exportar_atlas_pfnt_codificado PRIVATE cnk_native_core z m)
  target_compile_options(cnk_exportar_atlas_pfnt_codificado PRIVATE -Wall -Wextra -Werror)
  # Avance 85: validate exact encoded backing span + reproducible UI glyph layout.
  foreach(s IN ITEMS payload layout)
    add_executable(cnk_test_pfnt_${s} tests/test_native_pfnt_${s}.c)
    target_link_libraries(cnk_test_pfnt_${s} PRIVATE cnk_native_core z m)
    target_compile_options(cnk_test_pfnt_${s} PRIVATE -UNDEBUG -Wall -Wextra -Werror)
    add_test(NAME pfnt_${s} COMMAND cnk_test_pfnt_${s}
      "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  endforeach()
  # Advance 86: byte-exact glyph rectangle access, not invented raster decoding.
  add_executable(cnk_exportar_glifos_pfnt_codificados tools/exportar_glifos_pfnt_codificados.c)
  target_compile_options(cnk_exportar_glifos_pfnt_codificados PRIVATE -Wall -Wextra -Werror)
  target_link_libraries(cnk_exportar_glifos_pfnt_codificados PRIVATE cnk_native_core z m)
  add_executable(cnk_test_pfnt_glyph_data tests/test_native_pfnt_glyph_data.c)
  target_compile_options(cnk_test_pfnt_glyph_data PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  target_link_libraries(cnk_test_pfnt_glyph_data PRIVATE cnk_native_core z m)
  add_test(NAME pfnt_glyph_data COMMAND cnk_test_pfnt_glyph_data "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Original multilingual UI strings: 5 LANG streams, 229 source ordinals.
  add_executable(cnk_test_lang tests/test_native_lang.c)
  target_link_libraries(cnk_test_lang PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_lang PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME lang_original COMMAND cnk_test_lang "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_exportar_lang tools/exportar_textos_lang_originales.c)
  target_link_libraries(cnk_exportar_lang PRIVATE cnk_native_core z m)
  target_compile_options(cnk_exportar_lang PRIVATE -Wall -Wextra -Werror)
  # Avance 97: explicit developer Story objectives, only authentic scene IDs.
  # Story plot, chapters, track sequence and unlock rules are NOT recovered.
  add_executable(cnk_test_story tests/test_native_story.c)
  target_link_libraries(cnk_test_story PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_story PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME story_core COMMAND cnk_test_story "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_story_manifest tests/test_native_story_manifest.c)
  target_link_libraries(cnk_test_story_manifest PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_story_manifest PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME story_manifest COMMAND cnk_test_story_manifest
    "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs" "${CMAKE_CURRENT_BINARY_DIR}/cnk97_story.tsv")
  # Avance 98: source-bound, CRC validated developer Story save + safe resume.
  add_executable(cnk_test_story_checkpoint tests/test_native_story_checkpoint.c)
  target_link_libraries(cnk_test_story_checkpoint PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_story_checkpoint PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME story_checkpoint COMMAND cnk_test_story_checkpoint
    "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs" "${CMAKE_CURRENT_BINARY_DIR}/cnk98_story.dat")
  # Avance 93: optional multi-race standings, using validated source scene IDs.
  add_executable(cnk_test_cup tests/test_native_cup.c)
  target_link_libraries(cnk_test_cup PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_cup PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME cup_source_races COMMAND cnk_test_cup "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Advance 94: developer-authored Cup manifest, source ID and scoring validation;
  # no invented official Zeebo grouping, accessible only if explicitly configured.
  add_executable(cnk_test_cup_manifest tests/test_native_cup_manifest.c)
  target_link_libraries(cnk_test_cup_manifest PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_cup_manifest PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME cup_manifest COMMAND cnk_test_cup_manifest
    "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs" "${CMAKE_CURRENT_BINARY_DIR}/cnk94_test.cup")
  # Advance 95: durable optional Cup progress, never an invented original Cup.
  # Advance 96: completed-Cup archive and verified last-good fallback.
  add_executable(cnk_test_cup_archive tests/test_native_cup_archive.c)
  target_link_libraries(cnk_test_cup_archive PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_cup_archive PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME cup_final_archive COMMAND cnk_test_cup_archive
    "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs" "${CMAKE_CURRENT_BINARY_DIR}/cnk96_cup.dat")
  add_executable(cnk_test_cup_checkpoint tests/test_native_cup_checkpoint.c)
  target_link_libraries(cnk_test_cup_checkpoint PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_cup_checkpoint PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME cup_checkpoint COMMAND cnk_test_cup_checkpoint
    "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs" "${CMAKE_CURRENT_BINARY_DIR}/cnk95_cup.dat")
  # Advance 92: gated Kart Jacking pose swap, never a lap/checkpoint reward.
  add_executable(cnk_test_kart_jacking tests/test_native_kart_jacking.c)
  target_link_libraries(cnk_test_kart_jacking PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_kart_jacking PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME kart_jacking COMMAND cnk_test_kart_jacking "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Advance 92: source LANG confirms Aku-Aku speed boost and tackle.
  # Constants and animations remain explicitly provisional.
  add_executable(cnk_test_aku_aku tests/test_native_aku_aku.c)
  target_link_libraries(cnk_test_aku_aku PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_aku_aku PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME aku_aku COMMAND cnk_test_aku_aku "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Advance 91: original text describes Tropy Clock and Cursed Candle;
  # their provisional native effects are regression-tested against the ROM.
  add_executable(cnk_test_tropy_candle tests/test_native_tropy_candle.c)
  target_link_libraries(cnk_test_tropy_candle PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_tropy_candle PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME tropy_candle COMMAND cnk_test_tropy_candle "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Advance 90: source-LANG-backed Shield and ten-Wumpa mechanics,
  # with provisional numeric tuning and original ROM data validation.
  add_executable(cnk_test_original_item_effects tests/test_native_original_item_effects.c)
  target_link_libraries(cnk_test_original_item_effects PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_original_item_effects PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME original_item_effects COMMAND cnk_test_original_item_effects "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Authentic original source kart meshes, explicitly provisional racing
  # preview calibration and assignments; never enabled by default.
  add_executable(cnk_test_dev_race_visuals tests/test_native_dev_race_visuals.c)
  target_link_libraries(cnk_test_dev_race_visuals PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_dev_race_visuals PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME dev_race_visuals COMMAND cnk_test_dev_race_visuals
    "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Original ROM audio inventory only. Does not assign any historical cues.
  add_executable(cnk_extraer_wav_original tools/extraer_wav_original.c)
  target_compile_options(cnk_extraer_wav_original PRIVATE -Wall -Wextra -Werror)
  target_link_libraries(cnk_extraer_wav_original PRIVATE cnk_native_core z m)
  add_executable(cnk_test_wav_extraction tests/test_native_wav_extraction.c)
  target_link_libraries(cnk_test_wav_extraction PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_wav_extraction PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME wav_extraction COMMAND cnk_test_wav_extraction
    "$<TARGET_FILE:cnk_extraer_wav_original>" "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_exportar_catalogo_wav tools/exportar_catalogo_wav_originales.c)
  target_compile_options(cnk_exportar_catalogo_wav PRIVATE -Wall -Wextra -Werror)
  target_link_libraries(cnk_exportar_catalogo_wav PRIVATE cnk_native_core z m)
  add_executable(cnk_auditar_materiales tools/auditar_materiales_3dob.c)
  target_link_libraries(cnk_auditar_materiales PRIVATE cnk_native_core z m)
  foreach(t IN ITEMS models scenes navigation geometry collision segment_cast camera_occlusion world_collision kart_contacts camera_handedness race drive render objects png texture_map texture_consistency session audio music game spawn track_catalog frontend frontend_actions)
    add_executable(cnk_test_${t} tests/test_native_${t}.c)
    target_link_libraries(cnk_test_${t} PRIVATE cnk_native_core z m)
    # Regression tests use assert() for BOTH validation and setup; enabling
    # NDEBUG silently disables the tests and can cause invalid free/use.
    # Keep checks live even in optimized Release builds.
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(cnk_test_${t} PRIVATE -UNDEBUG)
    endif()
    add_test(NAME ${t} COMMAND cnk_test_${t} "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
    if(t STREQUAL "music")
      target_compile_definitions(cnk_test_music PRIVATE CNK_SOURCE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")
    endif()
  endforeach()
  add_executable(cnk_test_collision_probe tests/test_native_collision_probe.c)
  target_compile_options(cnk_test_collision_probe PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  target_link_libraries(cnk_test_collision_probe PRIVATE cnk_native_core z m)
  add_test(NAME collision_probe COMMAND cnk_test_collision_probe "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Full core smoke, all 12 original circuits with/without experimental
  # contacts, extended duration and in-race pause/resume. PC only.
  add_executable(cnk_test_extended_race_smoke tests/test_native_extended_race_smoke.c)
  target_compile_options(cnk_test_extended_race_smoke PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  target_link_libraries(cnk_test_extended_race_smoke PRIVATE cnk_native_core z m)
  add_test(NAME extended_race_smoke COMMAND cnk_test_extended_race_smoke "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_collision_matrix tests/test_native_collision_matrix.c)
  target_compile_options(cnk_test_collision_matrix PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  target_link_libraries(cnk_test_collision_matrix PRIVATE cnk_native_core z m)
  add_test(NAME collision_matrix COMMAND cnk_test_collision_matrix "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_collision_determinism tests/test_native_collision_determinism.c)
  target_compile_options(cnk_test_collision_determinism PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  target_link_libraries(cnk_test_collision_determinism PRIVATE cnk_native_core z m)
  add_test(NAME collision_determinism COMMAND cnk_test_collision_determinism "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_lap_settings_results tests/test_native_lap_settings_results.c)
  target_link_libraries(cnk_test_lap_settings_results PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_lap_settings_results PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME lap_settings_results COMMAND cnk_test_lap_settings_results "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_practice_records tests/test_native_practice_records.c)
  target_link_libraries(cnk_test_practice_records PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_practice_records PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME practice_records COMMAND cnk_test_practice_records "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_projectile_warning_event tests/test_native_projectile_warning_event.c)
  target_link_libraries(cnk_test_projectile_warning_event PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_projectile_warning_event PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME projectile_warning_event COMMAND cnk_test_projectile_warning_event "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_projectile_alert tests/test_native_projectile_alert.c)
  target_link_libraries(cnk_test_projectile_alert PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_projectile_alert PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME projectile_alert COMMAND cnk_test_projectile_alert "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_projectile_threat tests/test_native_projectile_threat.c)
  target_link_libraries(cnk_test_projectile_threat PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_projectile_threat PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME projectile_threat COMMAND cnk_test_projectile_threat)
  add_executable(cnk_test_ai_traffic tests/test_native_ai_traffic.c)
  target_link_libraries(cnk_test_ai_traffic PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_ai_traffic PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME ai_traffic COMMAND cnk_test_ai_traffic "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_ai_tactics tests/test_native_ai_tactics.c)
  target_link_libraries(cnk_test_ai_tactics PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_ai_tactics PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME ai_tactics COMMAND cnk_test_ai_tactics "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_sectors_ai_rescue tests/test_native_sectors_ai_rescue.c)
  target_link_libraries(cnk_test_sectors_ai_rescue PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_sectors_ai_rescue PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME sectors_ai_rescue COMMAND cnk_test_sectors_ai_rescue "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_race_progress tests/test_native_race_progress.c)
  target_link_libraries(cnk_test_race_progress PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_race_progress PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME race_progress COMMAND cnk_test_race_progress "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_route_direction tests/test_native_route_direction.c)
  target_link_libraries(cnk_test_route_direction PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_route_direction PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME route_direction COMMAND cnk_test_route_direction "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_wrong_way_runtime tests/test_native_wrong_way_runtime.c)
  target_link_libraries(cnk_test_wrong_way_runtime PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_wrong_way_runtime PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME wrong_way_runtime COMMAND cnk_test_wrong_way_runtime "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_audio_polyphony tests/test_native_audio_polyphony.c)
  target_link_libraries(cnk_test_audio_polyphony PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_audio_polyphony PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME audio_polyphony COMMAND cnk_test_audio_polyphony "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_item_visibility tests/test_native_item_visibility.c)
  target_link_libraries(cnk_test_item_visibility PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_item_visibility PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME item_visibility COMMAND cnk_test_item_visibility)
  add_executable(cnk_test_item_visual tests/test_native_item_visual.c)
  target_link_libraries(cnk_test_item_visual PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_item_visual PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME item_visual COMMAND cnk_test_item_visual "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_item_visual_gpu
    tests/test_native_item_visual_gpu.c tests/stub/vitaGL_stub.c
    src/cnk_vitagl_render.c src/cnk_vitagl_objects.c
    src/cnk_vitagl_game_frame.c src/cnk_vitagl_items.c src/cnk_vitagl_textures.c)
  target_include_directories(cnk_test_item_visual_gpu PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests/stub")
  target_compile_options(cnk_test_item_visual_gpu PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  target_link_libraries(cnk_test_item_visual_gpu PRIVATE cnk_native_core PNG::PNG z m)
  add_test(NAME item_visual_gpu COMMAND cnk_test_item_visual_gpu "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_weapons_runtime tests/test_native_weapons_runtime.c)
  target_link_libraries(cnk_test_weapons_runtime PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_weapons_runtime PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME weapons_runtime COMMAND cnk_test_weapons_runtime "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_weapons tests/test_native_weapons.c)
  target_link_libraries(cnk_test_weapons PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_weapons PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME weapons COMMAND cnk_test_weapons)
  add_executable(cnk_test_item_layout tests/test_native_item_layout.c)
  target_link_libraries(cnk_test_item_layout PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_item_layout PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME item_layout COMMAND cnk_test_item_layout "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Avance 68: pickup fairness by first swept contact, not racer index.
  # Avance 69: physical first-hit ordering for moving bombs and missiles.
  # Avance 70: stationary TNT/oil resolve first physical racer contact.
  add_executable(cnk_test_hazard_priority tests/test_native_hazard_priority.c)
  target_link_libraries(cnk_test_hazard_priority PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_hazard_priority PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME hazard_priority COMMAND cnk_test_hazard_priority)
  add_executable(cnk_test_projectile_priority tests/test_native_projectile_priority.c)
  target_link_libraries(cnk_test_projectile_priority PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_projectile_priority PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME projectile_priority COMMAND cnk_test_projectile_priority)
  # Avance 73: sub-frame source-gate timing and precise race finish.
  add_executable(cnk_test_finish_timing tests/test_native_finish_timing.c)
  target_link_libraries(cnk_test_finish_timing PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_finish_timing PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME finish_timing COMMAND cnk_test_finish_timing "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Avance 72: failed non-wall item steps cannot partially mutate race state.
  add_executable(cnk_test_item_transaction tests/test_native_item_transaction.c)
  target_link_libraries(cnk_test_item_transaction PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_item_transaction PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME item_transaction COMMAND cnk_test_item_transaction)
  # Avance 71: multiple pickups in one frame are processed by contact time.
  add_executable(cnk_test_pickup_order tests/test_native_pickup_order.c)
  target_link_libraries(cnk_test_pickup_order PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_pickup_order PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME pickup_order COMMAND cnk_test_pickup_order)
  add_executable(cnk_test_pickup_priority tests/test_native_pickup_priority.c)
  target_link_libraries(cnk_test_pickup_priority PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_pickup_priority PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME pickup_priority COMMAND cnk_test_pickup_priority)
  add_executable(cnk_test_items tests/test_native_items.c)
  target_link_libraries(cnk_test_items PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_items PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME items COMMAND cnk_test_items)
  add_executable(cnk_test_items_runtime tests/test_native_items_runtime.c)
  target_link_libraries(cnk_test_items_runtime PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_items_runtime PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME items_runtime COMMAND cnk_test_items_runtime "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Avance 59: reverse drive and opt-in source-wall projectile sweep.
  add_executable(cnk_test_reverse_projectile_walls tests/test_native_reverse_projectile_walls.c)
  target_link_libraries(cnk_test_reverse_projectile_walls PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_reverse_projectile_walls PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME reverse_projectile_walls COMMAND cnk_test_reverse_projectile_walls
    "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_boost tests/test_native_boost.c)
  target_link_libraries(cnk_test_boost PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_boost PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME boost COMMAND cnk_test_boost)
  add_executable(cnk_test_boost_runtime tests/test_native_boost_runtime.c)
  target_link_libraries(cnk_test_boost_runtime PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_boost_runtime PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME boost_runtime COMMAND cnk_test_boost_runtime "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_countdown_integration tests/test_native_countdown_integration.c)
  target_link_libraries(cnk_test_countdown_integration PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_countdown_integration PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME countdown_integration COMMAND cnk_test_countdown_integration "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_classification tests/test_native_classification.c)
  target_link_libraries(cnk_test_classification PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_classification PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME classification COMMAND cnk_test_classification "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_launch_boost tests/test_native_launch_boost.c)
  target_link_libraries(cnk_test_launch_boost PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_launch_boost PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME launch_boost COMMAND cnk_test_launch_boost "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Native timing stability against backwards clock readings and discontinuities.
  add_executable(cnk_test_runtime_timebase tests/test_native_runtime_timebase.c)
  target_compile_options(cnk_test_runtime_timebase PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  target_link_libraries(cnk_test_runtime_timebase PRIVATE cnk_native_core z m)
  add_test(NAME runtime_timebase COMMAND cnk_test_runtime_timebase "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Source-validated AI route length cache, no changes to DRV0 steering.
  add_executable(cnk_test_ai_route_cache tests/test_native_ai_route_cache.c)
  target_compile_options(cnk_test_ai_route_cache PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  target_link_libraries(cnk_test_ai_route_cache PRIVATE cnk_native_core z m)
  add_test(NAME ai_route_cache COMMAND cnk_test_ai_route_cache "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Power-loss/corruption recovery for development-only Vita record files.
  add_executable(cnk_test_record_recovery tests/test_native_record_recovery.c)
  target_compile_options(cnk_test_record_recovery PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  target_link_libraries(cnk_test_record_recovery PRIVATE cnk_native_core z m)
  add_test(NAME record_recovery COMMAND cnk_test_record_recovery "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_boost_pads tests/test_native_boost_pads.c)
  target_compile_options(cnk_test_boost_pads PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  target_link_libraries(cnk_test_boost_pads PRIVATE cnk_native_core z m)
  add_test(NAME boost_pads COMMAND cnk_test_boost_pads "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_pad_layout tests/test_native_pad_layout.c)
  target_compile_options(cnk_test_pad_layout PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  target_link_libraries(cnk_test_pad_layout PRIVATE cnk_native_core z m)
  add_test(NAME pad_layout COMMAND cnk_test_pad_layout "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_start_sequence tests/test_native_start_sequence.c)
  target_link_libraries(cnk_test_start_sequence PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_start_sequence PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME start_sequence COMMAND cnk_test_start_sequence)
  add_executable(cnk_test_music_cues tests/test_native_music_cues.c)
  target_link_libraries(cnk_test_music_cues PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_music_cues PRIVATE -UNDEBUG)
  target_compile_definitions(cnk_test_music_cues PRIVATE CNK_SOURCE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")
  add_test(NAME music_cues COMMAND cnk_test_music_cues "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Compiles the REAL production Vita GPU draw bridge against a host-only
  # OpenGL call recorder. Does not build an emulator or an installable VPK.
  add_executable(cnk_test_gpu_bridge
    tests/test_native_gpu_bridge.c tests/stub/vitaGL_stub.c
    src/cnk_vitagl_render.c src/cnk_vitagl_objects.c)
  target_include_directories(cnk_test_gpu_bridge PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests/stub")
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(cnk_test_gpu_bridge PRIVATE -UNDEBUG)
  endif()
  target_link_libraries(cnk_test_gpu_bridge PRIVATE cnk_native_core z m)
  add_test(NAME gpu_bridge COMMAND cnk_test_gpu_bridge "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Conservative scene frustum culling with exact original mesh bounds.
  add_executable(cnk_test_gpu_frustum
    tests/test_native_gpu_frustum.c tests/stub/vitaGL_stub.c
    src/cnk_vitagl_render.c)
  target_include_directories(cnk_test_gpu_frustum PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests/stub")
  target_compile_options(cnk_test_gpu_frustum PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  target_link_libraries(cnk_test_gpu_frustum PRIVATE cnk_native_core z m)
  add_test(NAME gpu_frustum COMMAND cnk_test_gpu_frustum "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Exercise the production texture uploader with libpng + host GPU recorder.
  # This DOES NOT infer original names or replace hardware validation.
  find_package(PNG REQUIRED)
  add_executable(cnk_test_gpu_textures
    tests/test_native_gpu_textures.c tests/stub/vitaGL_stub.c
    src/cnk_vitagl_textures.c
    src/cnk_vitagl_scene_assets.c)
  target_include_directories(cnk_test_gpu_textures PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests/stub")
  target_compile_options(cnk_test_gpu_textures PRIVATE -UNDEBUG)
  target_link_libraries(cnk_test_gpu_textures PRIVATE cnk_native_core PNG::PNG z m)
  add_test(NAME gpu_textures COMMAND cnk_test_gpu_textures "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Integration test of validated per-SCEN texture loading, GPU rollback
  # and the production draw bridge. Tests use deliberately synthetic
  # references; no fake mappings ship as historical evidence.
  add_executable(cnk_test_gpu_scene_assets
    tests/test_native_gpu_scene_assets.c tests/stub/vitaGL_stub.c
    src/cnk_vitagl_textures.c src/cnk_vitagl_scene_assets.c
    src/cnk_vitagl_render.c)
  target_include_directories(cnk_test_gpu_scene_assets PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests/stub")
  target_compile_options(cnk_test_gpu_scene_assets PRIVATE -UNDEBUG)
  target_link_libraries(cnk_test_gpu_scene_assets PRIVATE cnk_native_core PNG::PNG z m)
  add_test(NAME gpu_scene_assets COMMAND cnk_test_gpu_scene_assets "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Production race-session -> vitaGL integration: all 12 source race scenes,
  # live camera and HUD, no guessed kart assets or textures.
  add_executable(cnk_test_gpu_game_frame
    tests/test_native_gpu_game_frame.c tests/stub/vitaGL_stub.c
    src/cnk_vitagl_render.c src/cnk_vitagl_objects.c src/cnk_vitagl_game_frame.c src/cnk_vitagl_items.c)
  target_include_directories(cnk_test_gpu_game_frame PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests/stub")
  target_compile_options(cnk_test_gpu_game_frame PRIVATE -UNDEBUG)
  target_link_libraries(cnk_test_gpu_game_frame PRIVATE cnk_native_core z m)
  add_test(NAME gpu_game_frame COMMAND cnk_test_gpu_game_frame "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_racer_culling tests/test_native_racer_culling.c tests/stub/vitaGL_stub.c
    src/cnk_vitagl_render.c src/cnk_vitagl_objects.c src/cnk_vitagl_game_frame.c
    src/cnk_vitagl_items.c src/cnk_vitagl_textures.c)
  target_include_directories(cnk_test_racer_culling PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests/stub")
  target_compile_options(cnk_test_racer_culling PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  target_link_libraries(cnk_test_racer_culling PRIVATE cnk_native_core PNG::PNG z m)
  add_test(NAME racer_culling COMMAND cnk_test_racer_culling "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_gpu_racer_visuals tests/test_native_gpu_racer_visuals.c tests/stub/vitaGL_stub.c
    src/cnk_vitagl_render.c src/cnk_vitagl_objects.c src/cnk_vitagl_game_frame.c src/cnk_vitagl_items.c)
  target_include_directories(cnk_test_gpu_racer_visuals PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests/stub")
  target_compile_options(cnk_test_gpu_racer_visuals PRIVATE -UNDEBUG)
  target_link_libraries(cnk_test_gpu_racer_visuals PRIVATE cnk_native_core z m)
  add_test(NAME gpu_racer_visuals COMMAND cnk_test_gpu_racer_visuals "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Candidate source-original multi-resolution model groups. No character identity inferred.
  # Original TEXL names in all three 3DOB LOD variants, with strict bounds.
  add_executable(cnk_test_model_texl tests/test_native_model_texl.c)
  target_compile_options(cnk_test_model_texl PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  target_link_libraries(cnk_test_model_texl PRIVATE cnk_native_core z m)
  add_test(NAME model_texl COMMAND cnk_test_model_texl "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Independently reported Crash CHARACTER texture ID, checked against
  # the user's original archive and model source; no guessed kart mapping.
  add_executable(cnk_test_crash_texture_reference tests/test_native_crash_texture_reference.c)
  target_compile_options(cnk_test_crash_texture_reference PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  target_link_libraries(cnk_test_crash_texture_reference PRIVATE cnk_native_core z m)
  add_test(NAME crash_texture_reference COMMAND cnk_test_crash_texture_reference "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # First independently supported character texture bound all the way to
  # Vita GPU uploader and actual source 3DOB drawing commands. Host GL recorder.
  add_executable(cnk_test_crash_textured_render
    tests/test_native_crash_textured_render.c tests/stub/vitaGL_stub.c
    src/cnk_verified_crash.c src/cnk_object_skin.c src/cnk_vitagl_textures.c src/cnk_vitagl_objects.c)
  target_include_directories(cnk_test_crash_textured_render PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests/stub")
  target_compile_options(cnk_test_crash_textured_render PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  target_link_libraries(cnk_test_crash_textured_render PRIVATE cnk_native_core PNG::PNG z m)
  add_test(NAME crash_textured_render COMMAND cnk_test_crash_textured_render "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Character pairs sourced from original 3DOB material names and independently
  # published Zeebo character asset PNG IDs. Recorded GL, not Vita hardware.
  add_executable(cnk_test_verified_characters
    tests/test_native_verified_characters.c tests/stub/vitaGL_stub.c
    src/cnk_verified_characters.c src/cnk_object_skin.c
    src/cnk_vitagl_textures.c src/cnk_vitagl_objects.c)
  target_include_directories(cnk_test_verified_characters PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests/stub")
  target_compile_options(cnk_test_verified_characters PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  target_link_libraries(cnk_test_verified_characters PRIVATE cnk_native_core PNG::PNG z m)
  add_test(NAME verified_characters COMMAND cnk_test_verified_characters "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Original 3DOB/PNG gallery, isolated from any invented driver-kart pair.
  add_executable(cnk_test_character_gallery
    tests/test_native_character_gallery.c tests/stub/vitaGL_stub.c
    src/cnk_character_gallery.c src/cnk_verified_characters.c
    src/cnk_object_skin.c src/cnk_vitagl_textures.c src/cnk_vitagl_objects.c)
  target_include_directories(cnk_test_character_gallery PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests/stub")
  target_compile_options(cnk_test_character_gallery PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  target_link_libraries(cnk_test_character_gallery PRIVATE cnk_native_core PNG::PNG z m)
  add_test(NAME character_gallery COMMAND cnk_test_character_gallery "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Six original Zeebo vehicle families (18 FUFS/3DOB LOD meshes), untextured
  # until body PNG identities are independently proven. GL stub, not hardware.
  add_executable(cnk_test_source_kart_gallery
    tests/test_native_source_kart_gallery.c tests/stub/vitaGL_stub.c
    src/cnk_source_kart_gallery.c src/cnk_vitagl_objects.c)
  target_include_directories(cnk_test_source_kart_gallery PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests/stub")
  target_compile_options(cnk_test_source_kart_gallery PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  target_link_libraries(cnk_test_source_kart_gallery PRIVATE cnk_native_core PNG::PNG z m)
  add_test(NAME source_kart_gallery COMMAND cnk_test_source_kart_gallery "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Opt-in texture research for the six source-verified vehicle families.
  # Test creates a SYNTHETIC material/PNG association, never a default map.
  add_executable(cnk_test_gallery_skins
    tests/test_native_gallery_skins.c tests/stub/vitaGL_stub.c
    src/cnk_gallery_skins.c src/cnk_source_kart_gallery.c
    src/cnk_vitagl_textures.c src/cnk_vitagl_objects.c)
  target_include_directories(cnk_test_gallery_skins PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests/stub")
  target_compile_options(cnk_test_gallery_skins PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  target_link_libraries(cnk_test_gallery_skins PRIVATE cnk_native_core PNG::PNG z m)
  add_test(NAME gallery_skins COMMAND cnk_test_gallery_skins "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Render both a ROM kart source mesh and a verified textured ROM driver,
  # with independent source-checked sidecar calibration and GPU rollback.
  add_executable(cnk_test_driver_race_integration
    tests/test_native_driver_race_integration.c tests/stub/vitaGL_stub.c
    src/cnk_verified_characters.c src/cnk_object_skin.c
    src/cnk_vitagl_textures.c src/cnk_vitagl_objects.c
    src/cnk_vitagl_render.c src/cnk_vitagl_game_frame.c src/cnk_vitagl_items.c)
  target_include_directories(cnk_test_driver_race_integration PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests/stub")
  target_compile_options(cnk_test_driver_race_integration PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  target_link_libraries(cnk_test_driver_race_integration PRIVATE cnk_native_core PNG::PNG z m)
  add_test(NAME driver_race_integration COMMAND cnk_test_driver_race_integration "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_model_material tests/test_native_model_material.c)
  target_compile_options(cnk_test_model_material PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  target_link_libraries(cnk_test_model_material PRIVATE cnk_native_core z m)
  add_test(NAME model_material COMMAND cnk_test_model_material "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_model_lod tests/test_native_model_lod.c)
  target_compile_options(cnk_test_model_lod PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  target_link_libraries(cnk_test_model_lod PRIVATE cnk_native_core z m)
  add_test(NAME model_lod COMMAND cnk_test_model_lod "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Model-LOD candidates -> live source-race camera distances -> real Vita
  # renderer with host GL recorder; never assigns a guessed character.
  add_executable(cnk_test_gpu_lod_runtime
    tests/test_native_gpu_lod_runtime.c tests/stub/vitaGL_stub.c
    src/cnk_vitagl_render.c src/cnk_vitagl_objects.c src/cnk_vitagl_game_frame.c src/cnk_vitagl_items.c)
  target_include_directories(cnk_test_gpu_lod_runtime PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests/stub")
  target_compile_options(cnk_test_gpu_lod_runtime PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  target_link_libraries(cnk_test_gpu_lod_runtime PRIVATE cnk_native_core z m)
  add_test(NAME gpu_lod_runtime COMMAND cnk_test_gpu_lod_runtime "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_kart_skins
    tests/test_native_kart_skins.c tests/stub/vitaGL_stub.c
    src/cnk_vitagl_objects.c src/cnk_vitagl_textures.c)
  target_include_directories(cnk_test_kart_skins PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests/stub")
  target_compile_options(cnk_test_kart_skins PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  target_link_libraries(cnk_test_kart_skins PRIVATE cnk_native_core PNG::PNG z m)
  add_test(NAME kart_skins COMMAND cnk_test_kart_skins "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_racer_assets tests/test_native_racer_assets.c)
  target_compile_options(cnk_test_racer_assets PRIVATE -UNDEBUG)
  target_link_libraries(cnk_test_racer_assets PRIVATE cnk_native_core z m)
  add_test(NAME racer_assets COMMAND cnk_test_racer_assets "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Advance 100: small buffers and short heap strings with ASan coverage.
  add_executable(cnk_test_texture_memory_safety tests/test_native_texture_memory_safety.c)
  target_compile_options(cnk_test_texture_memory_safety PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  target_link_libraries(cnk_test_texture_memory_safety PRIVATE cnk_native_core z m)
  add_test(NAME texture_memory_safety COMMAND cnk_test_texture_memory_safety
    "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_texture_manifest tests/test_native_texture_manifest.c)
  target_compile_options(cnk_test_texture_manifest PRIVATE -UNDEBUG)
  target_link_libraries(cnk_test_texture_manifest PRIVATE cnk_native_core z m)
  add_test(NAME texture_manifest COMMAND cnk_test_texture_manifest "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_recovery_minimap tests/test_native_recovery_minimap.c)
  target_link_libraries(cnk_test_recovery_minimap PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_recovery_minimap PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME recovery_minimap COMMAND cnk_test_recovery_minimap "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  add_executable(cnk_test_app_runtime tests/test_native_app_runtime.c)
  target_link_libraries(cnk_test_app_runtime PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_app_runtime PRIVATE -UNDEBUG)
  add_test(NAME app_runtime COMMAND cnk_test_app_runtime "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Development source-route grid: preserves authenticated STRT for the
  # human kart, spaces six provisional rivals on each of 12 source routes.
  add_executable(cnk_test_source_grid tests/test_native_source_grid.c)
  target_link_libraries(cnk_test_source_grid PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_source_grid PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME source_grid COMMAND cnk_test_source_grid "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Slow but actual source-data round trips: 72 completed racer laps
  # across 12 SCEN tracks, with the human driven by test-only AI controls.
  add_executable(cnk_test_source_grid_endurance tests/test_native_source_grid_endurance.c)
  target_link_libraries(cnk_test_source_grid_endurance PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_source_grid_endurance PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME source_grid_endurance COMMAND cnk_test_source_grid_endurance "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Regression: finished rivals cannot become permanent finish-line walls.
  # Enable provisional contacts through 72 original-source scene race finishes.
  add_executable(cnk_test_contacts_finish_endurance tests/test_native_contacts_finish_endurance.c)
  target_link_libraries(cnk_test_contacts_finish_endurance PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_contacts_finish_endurance PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME contacts_finish_endurance COMMAND cnk_test_contacts_finish_endurance "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Slow but source-backed: all 12 source three-lap six-racer scenes.
  # The kart contact mode and rival driving remain development-only.
  add_executable(cnk_test_multilap_contacts_endurance tests/test_native_multilap_contacts_endurance.c)
  target_link_libraries(cnk_test_multilap_contacts_endurance PRIVATE cnk_native_core PNG::PNG z m)
  target_compile_options(cnk_test_multilap_contacts_endurance PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME multilap_contacts_endurance COMMAND cnk_test_multilap_contacts_endurance "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Source-grounded discrete frontend and checkpoint/lap/finish event stream:
  # enables future HUD/audio hookups without fabricating event-to-sample IDs.
  add_executable(cnk_test_app_events tests/test_native_app_events.c)
  target_compile_options(cnk_test_app_events PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  target_link_libraries(cnk_test_app_events PRIVATE cnk_native_core z m)
  add_test(NAME app_events COMMAND cnk_test_app_events "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Optional reviewed original WAV cue map. Synthetic cue associations are
  # made ONLY inside tests; no false soundtrack mapping is shipped.
  add_executable(cnk_test_sound_cues tests/test_native_sound_cues.c)
  target_compile_options(cnk_test_sound_cues PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  target_link_libraries(cnk_test_sound_cues PRIVATE cnk_native_core z m)
  add_test(NAME sound_cues COMMAND cnk_test_sound_cues "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Parse/compile-check the actual Vita application lifecycle against
  # narrow host API declarations. No Vita binary, emulator or VPK is made.
  # Real native entrypoint, not the earlier PNG-viewer main().
  # Syntax-only compile for host; no executable and no VPK here.
  add_library(cnk_vita_main_syntax OBJECT src/vita_main.c)
  target_include_directories(cnk_vita_main_syntax PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/vita_app_stub")
  target_compile_options(cnk_vita_main_syntax PRIVATE -Wall -Wextra -Werror)
  add_library(cnk_vita_app_syntax OBJECT src/cnk_vita_native_app.c)
  target_include_directories(cnk_vita_app_syntax PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/vita_app_stub"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/stub")
  target_compile_options(cnk_vita_app_syntax PRIVATE -Wall -Wextra -Werror)
  add_library(cnk_vita_audio_syntax OBJECT src/cnk_vita_audio.c src/cnk_vita_audio_worker.c)
  target_include_directories(cnk_vita_audio_syntax PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests/vita_app_stub")
  target_compile_options(cnk_vita_audio_syntax PRIVATE -Wall -Wextra -Werror)
  # Full application lifecycle exercised with fake external Vita APIs,
  # but TRUE native source geometry + frontend code. Nothing installable.
  add_executable(cnk_test_native_vita_loop
    tests/test_native_vita_loop.c tests/stub/vitaGL_stub.c tests/vita_app_stub/host_vita_audio.c
    src/cnk_vita_native_app.c src/cnk_character_gallery.c src/cnk_source_kart_gallery.c src/cnk_gallery_skins.c src/cnk_vita_controls.c
    src/cnk_vitagl_render.c src/cnk_vitagl_objects.c
    src/cnk_vitagl_game_frame.c src/cnk_vitagl_items.c src/cnk_vitagl_textures.c src/cnk_vitagl_scene_assets.c
    src/cnk_verified_characters.c src/cnk_object_skin.c
    src/cnk_vita_audio.c src/cnk_vita_audio_worker.c)
  target_include_directories(cnk_test_native_vita_loop PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/vita_app_stub"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/stub")
  target_compile_options(cnk_test_native_vita_loop PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  target_compile_definitions(cnk_test_native_vita_loop PRIVATE CNK_SOURCE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")
  target_link_libraries(cnk_test_native_vita_loop PRIVATE cnk_native_core PNG::PNG z m pthread)
  add_test(NAME native_vita_loop COMMAND cnk_test_native_vita_loop "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")
  # Vita input adapter compiled against a minimal host-only ctrl.h shim.
  # It checks production mapping without pretending to run on hardware.
  add_executable(cnk_test_vita_diagnostics tests/test_native_vita_diagnostics.c)
  target_link_libraries(cnk_test_vita_diagnostics PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_vita_diagnostics PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME vita_diagnostics COMMAND cnk_test_vita_diagnostics)

  add_executable(cnk_test_vita_source_path tests/test_native_vita_source_path.c)
  target_link_libraries(cnk_test_vita_source_path PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_vita_source_path PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME vita_source_path COMMAND cnk_test_vita_source_path "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs")

  add_executable(cnk_test_vita_telemetry tests/test_native_vita_telemetry.c)
  target_link_libraries(cnk_test_vita_telemetry PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_vita_telemetry PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME vita_telemetry COMMAND cnk_test_vita_telemetry)
  # AI rivals may be displaced by weapons/contacts beyond the local
  # route search window. Recover source-route guidance instead of aborting.
  add_executable(cnk_test_ai_reacquire tests/test_native_ai_reacquire.c)
  target_link_libraries(cnk_test_ai_reacquire PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_ai_reacquire PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME ai_reacquire COMMAND cnk_test_ai_reacquire)
  # No ROM required: guard against finite-but-overflowing scene extents.
  add_executable(cnk_test_collision_grid_extents tests/test_native_collision_grid_extents.c)
  target_link_libraries(cnk_test_collision_grid_extents PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_collision_grid_extents PRIVATE -UNDEBUG -Wall -Wextra -Werror)
  add_test(NAME collision_grid_extents COMMAND cnk_test_collision_grid_extents)

  add_executable(cnk_test_vita_controls
    tests/test_native_vita_controls.c src/cnk_vita_controls.c)
  target_include_directories(cnk_test_vita_controls PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests/stub")
  target_link_libraries(cnk_test_vita_controls PRIVATE cnk_native_core z m)
  target_compile_options(cnk_test_vita_controls PRIVATE -UNDEBUG)
  add_test(NAME vita_controls COMMAND cnk_test_vita_controls)
  # Inventory of names embedded in the original BREW executable.
  # Matching a filename does NOT imply its PNG VFS index is known.
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME source_zip_safety
      COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_source_zip_checker.py")
    add_test(NAME source_lang_catalog
      COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/verificar_catalogo_lang.py"
      "$<TARGET_FILE:cnk_exportar_lang>" "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs"
      "${CMAKE_CURRENT_SOURCE_DIR}/docs/catalogo_lang_originales.tsv")
    add_test(NAME source_pfnt_glyph_bytes
      COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/verificar_glifos_pfnt_codificados.py"
      "$<TARGET_FILE:cnk_exportar_glifos_pfnt_codificados>"
      "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs"
      "${CMAKE_CURRENT_SOURCE_DIR}/docs/catalogo_pfnt_glifos_codificados.tsv")
    add_test(NAME source_pfnt_bitmap_catalog
      COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/verificar_atlas_pfnt.py"
      "$<TARGET_FILE:cnk_exportar_atlas_pfnt_codificado>"
      "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs"
      "${CMAKE_CURRENT_SOURCE_DIR}/docs/catalogo_pfnt_payload_original.tsv")
    add_test(NAME source_pfnt_catalog
      COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/verificar_catalogo_pfnt.py"
      "$<TARGET_FILE:cnk_exportar_catalogo_pfnt>" "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs"
      "${CMAKE_CURRENT_SOURCE_DIR}/docs/catalogo_pfnt_originales.tsv"
      "${CMAKE_CURRENT_SOURCE_DIR}/docs/catalogo_pfnt_glifos_originales.tsv")
    add_test(NAME source_gallery_material_audit
      COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/verificar_materiales_galeria.py"
      "$<TARGET_FILE:cnk_exportar_materiales_galeria>"
      "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs"
      "${CMAKE_CURRENT_SOURCE_DIR}/docs/catalogo_modelos_materiales_nombres_originales.csv")
    add_test(NAME source_wav_catalog
      COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/verificar_catalogo_wav.py"
      "$<TARGET_FILE:cnk_exportar_catalogo_wav>" "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs"
      "${CMAKE_CURRENT_SOURCE_DIR}/docs/catalogo_wav_originales.tsv"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/manifest_recursos.csv")

    add_test(NAME published_model_pngs
      COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/verificar_recursos_modelos_publicados.py")
    add_test(NAME source_catalog
      COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/catalogar_nombres_originales.py" --check)
    add_test(NAME source_track_catalog
      COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/verificar_catalogo_pistas.py" "$<TARGET_FILE:cnk_exportar_catalogo>" "${CMAKE_CURRENT_SOURCE_DIR}/data.vfs" "${CMAKE_CURRENT_SOURCE_DIR}/docs/catalogo_pistas_fuente.csv")
  endif()
else()
  # GPU/controles NATIVOS de Vita, en un módulo opcional. Sin emulador.
  # No empaquetamos un VPK inacabado: esta librería se integra con la app final
  # cuando el loop de juego, recursos originales y estado de carrera existan.
  option(CNK_ENABLE_VITAGL_BACKEND "GPU y controles nativos Vita" OFF)
  option(CNK_ENABLE_VITA_ELF_LINK_CHECK "Enlazar ELF nativo de diagnóstico (sin SELF ni VPK)" OFF)
  if(CNK_ENABLE_VITA_ELF_LINK_CHECK)
    set(CNK_ENABLE_VITAGL_BACKEND ON)
  endif()
  if(CNK_ENABLE_VITAGL_BACKEND)
    add_library(cnk_vita_backend STATIC src/cnk_vitagl_render.c src/cnk_vita_controls.c src/cnk_vitagl_textures.c src/cnk_vita_audio.c src/cnk_vitagl_objects.c src/cnk_vitagl_scene_assets.c src/cnk_vitagl_game_frame.c src/cnk_vitagl_items.c src/cnk_object_skin.c src/cnk_verified_crash.c src/cnk_verified_characters.c src/cnk_character_gallery.c src/cnk_source_kart_gallery.c src/cnk_gallery_skins.c src/cnk_vita_native_app.c src/cnk_vita_audio_worker.c)
    target_include_directories(cnk_vita_backend PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(cnk_vita_backend PUBLIC cnk_native_core)
    if(CNK_ENABLE_VITA_ELF_LINK_CHECK)
      # This is an ELF link probe ONLY. No SELF and no installable VPK is built.
      # The real VitaSDK/vitaGL libraries must be installed by the developer.
      # Keep the list overridable for SDK/version-specific installations.
      set(CNK_VITA_LINK_LIBS
          "vitaGL;vitashark;SceShaccCgExt;stdc++;mathneon;png;z;m;taihen_stub;SceAppMgr_stub;SceCommonDialog_stub;SceKernelDmacMgr_stub;SceProcessmgr_stub;SceCtrl_stub;SceDisplay_stub;SceAudio_stub;SceGxm_stub;SceLibKernel_stub;SceKernelThreadMgr_stub;SceShaccCg_stub;SceSysmodule_stub;SceIofilemgr_stub"
          CACHE STRING "VitaSDK libraries required by ELF link check")
      add_executable(cnk_vita_elf_link_check src/vita_main.c)
      target_link_libraries(cnk_vita_elf_link_check PRIVATE
          cnk_vita_backend cnk_native_core ${CNK_VITA_LINK_LIBS})
      # Deliberately no vita_create_self and no vita_create_vpk calls.
    endif()
  endif()
  # NO declarar un VPK del juego antes de integrar motor, física, audio y escenas.
  # La biblioteca cnk_native_core se compila para el procesador PS Vita con VitaSDK.
  # No generar instalables incompletos. El viejo viewer de recursos queda
  # como codigo historico, fuera de todos los targets de VitaSDK.
  # Solo la aplicacion final, una vez verificada, podra crear el VPK.
endif()
