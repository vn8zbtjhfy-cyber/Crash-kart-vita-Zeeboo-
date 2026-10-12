cmake_minimum_required(VERSION 3.18)
set(RUST_VITA_TARGET "armv7-sony-vita-newlibeabihf")
if(NOT DEFINED ENV{VITASDK})
  message(FATAL_ERROR "VITASDK required")
endif()
set(VITASDK "$ENV{VITASDK}")
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "${VITASDK}/share/vita.toolchain.cmake" CACHE PATH "" FORCE)
endif()
project(CrashNitroKartVita LANGUAGES C ASM)
include("${VITASDK}/share/vita.cmake" REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(VITA_APP_NAME "Crash Nitro Kart 3D")
set(VITA_TITLEID "CNKVITA01")
set(VITA_VERSION "01.00")
set(ZEEBX_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/zeebx-emu")
set(CARGO_TARGET_DIR "${CMAKE_CURRENT_BINARY_DIR}/cargo-target")
set(RUST_BUILD_CORE "${CARGO_TARGET_DIR}/${RUST_VITA_TARGET}/release/libzeebx_vita.a")
set(RUST_CORE "${CARGO_TARGET_DIR}/${RUST_VITA_TARGET}/release/libzeebx.a")
set(RUST_MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/rust/zeebx_vita/Cargo.toml")
if(NOT EXISTS "${ZEEBX_SOURCE_DIR}/Cargo.toml")
 message(FATAL_ERROR "Zeebx source missing; run scripts/build_vpk.sh")
endif()
find_program(CARGO_EXECUTABLE cargo REQUIRED)
add_custom_command(OUTPUT "${RUST_CORE}"
  COMMAND "${CMAKE_COMMAND}" -E env
    "VITASDK=${VITASDK}" "CARGO_TARGET_DIR=${CARGO_TARGET_DIR}"
    "PATH=${VITASDK}/bin:$ENV{PATH}"
    "${CARGO_EXECUTABLE}" +nightly build -Z build-std=std,panic_abort
      --manifest-path "${RUST_MANIFEST}" --target "${RUST_VITA_TARGET}" --release
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${RUST_BUILD_CORE}" "${RUST_CORE}"
  DEPENDS "${RUST_MANIFEST}" "${CMAKE_CURRENT_SOURCE_DIR}/rust/zeebx_vita/src/lib.rs"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/rust/zeebx_vita"
  COMMENT "Compile Zeebx ARM interpreter as staticlib" VERBATIM)
add_custom_target(zeebx_rust_build DEPENDS "${RUST_CORE}")
add_library(zeebx_rust STATIC IMPORTED GLOBAL)
set_target_properties(zeebx_rust PROPERTIES IMPORTED_LOCATION "${RUST_CORE}")
add_dependencies(zeebx_rust zeebx_rust_build)
set(GENERATED "${CMAKE_CURRENT_BINARY_DIR}/generated")
execute_process(
 COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/make_blob_asm.py"
  --root "${CMAKE_CURRENT_SOURCE_DIR}/assets" --out "${GENERATED}"
 RESULT_VARIABLE BLOB_RESULT)
if(NOT BLOB_RESULT EQUAL 0)
 message(FATAL_ERROR "Missing or mismatched embedded ROM assets")
endif()
add_executable(${PROJECT_NAME}
 src/main.c src/vita_input.c src/vita_video.c
 "${GENERATED}/embedded.c" "${GENERATED}/embedded.S")
target_include_directories(${PROJECT_NAME} PRIVATE src "${GENERATED}")
target_compile_options(${PROJECT_NAME} PRIVATE -O2 -Wall -Wextra)
target_link_libraries(${PROJECT_NAME} PRIVATE
 zeebx_rust vitaGL
 SceCtrl_stub SceGxm_stub SceDisplay_stub SceSysmodule_stub
 SceKernelDmacMgr_stub SceAppMgr_stub SceAudio_stub
 SceIofilemgr_stub SceProcessmgr_stub pthread m z stdc++)
option(NITRO_EXPERIMENTAL_VPK "Package unverified game VPK" ON)
if(NITRO_EXPERIMENTAL_VPK)
 vita_create_self(eboot.bin ${PROJECT_NAME} UNSAFE)
 vita_create_vpk(Crash_Nitro_Kart_3D.vpk ${VITA_TITLEID} eboot.bin
 VERSION ${VITA_VERSION} NAME ${VITA_APP_NAME})
endif()
