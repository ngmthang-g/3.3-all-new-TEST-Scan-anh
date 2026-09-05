cmake_minimum_required(VERSION 3.24)
project(ThanLongItemConsolidator VERSION 9.9 LANGUAGES CXX RC)

if(NOT WIN32)
    message(FATAL_ERROR "ThanLongItemConsolidator is Windows-only")
endif()
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "ThanLongItemConsolidator must be built x64")
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Only the public HTTPS Supabase Edge Function URL is compiled into the EXE.
# Never place service-role/secret credentials in this client build.
set(TL_LICENSE_API_URL "$ENV{TL_LICENSE_API_URL}" CACHE STRING "Public HTTPS Supabase license API URL")
if(TL_LICENSE_API_URL STREQUAL "")
    set(TL_LICENSE_API_URL "https://dklpxxxgapbnjixvbzai.supabase.co/functions/v1/license-api")
endif()
# Encode the public license endpoint before it reaches generated C++ source.
# The endpoint is not a credential; this only removes a convenient plaintext
# marker from static string scans of the client binary.
string(HEX "${TL_LICENSE_API_URL}" TL_LICENSE_API_URL_HEX)
string(LENGTH "${TL_LICENSE_API_URL_HEX}" TL_LICENSE_API_URL_HEX_LEN)
math(EXPR TL_LICENSE_API_URL_HEX_LAST "${TL_LICENSE_API_URL_HEX_LEN} - 2")
set(TL_LICENSE_API_XOR_KEY 93)
set(TL_LICENSE_API_URL_ENC "")
foreach(TL_URL_I RANGE 0 ${TL_LICENSE_API_URL_HEX_LAST} 2)
    string(SUBSTRING "${TL_LICENSE_API_URL_HEX}" ${TL_URL_I} 2 TL_URL_PAIR)
    math(EXPR TL_URL_BYTE "0x${TL_URL_PAIR} ^ ${TL_LICENSE_API_XOR_KEY}")
    string(APPEND TL_LICENSE_API_URL_ENC "${TL_URL_BYTE},")
endforeach()
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated")
configure_file(src/license_build_config.h.in "${CMAKE_BINARY_DIR}/generated/license_build_config.h" @ONLY)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(TL_IMAGE_SCAN_GEN_DIR "${CMAKE_BINARY_DIR}/image_scan_v4_generated")
file(MAKE_DIRECTORY "${TL_IMAGE_SCAN_GEN_DIR}")
execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/generate_image_scan_v4_sources.py"
            --source-root "${CMAKE_SOURCE_DIR}" --output-dir "${TL_IMAGE_SCAN_GEN_DIR}"
    RESULT_VARIABLE TL_IMAGE_SCAN_GEN_RESULT)
if(NOT TL_IMAGE_SCAN_GEN_RESULT EQUAL 0)
    message(FATAL_ERROR "IMAGE SCAN V4 source integration failed")
endif()
execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/apply_pre_close_x_patch.py"
            --source-root "${CMAKE_SOURCE_DIR}" --output-dir "${TL_IMAGE_SCAN_GEN_DIR}"
    RESULT_VARIABLE TL_PRE_CLOSE_X_PATCH_RESULT)
if(NOT TL_PRE_CLOSE_X_PATCH_RESULT EQUAL 0)
    message(FATAL_ERROR "PRE-CLOSE X one-shot integration failed")
endif()
foreach(TL_UI30_PATCH IN ITEMS
        apply_ui30_controller_base.py
        apply_ui30_controller_groups.py
        apply_ui30_controller_runtime.py
        apply_ui30_scanner.py)
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/${TL_UI30_PATCH}"
                --source-root "${CMAKE_SOURCE_DIR}" --output-dir "${TL_IMAGE_SCAN_GEN_DIR}"
        RESULT_VARIABLE TL_UI30_PATCH_RESULT)
    if(NOT TL_UI30_PATCH_RESULT EQUAL 0)
        message(FATAL_ERROR "UI30 / CON30 integration failed: ${TL_UI30_PATCH}")
    endif()
endforeach()
execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/apply_v99_automation_cp1.py"
            --source-root "${CMAKE_SOURCE_DIR}" --output-dir "${TL_IMAGE_SCAN_GEN_DIR}"
    RESULT_VARIABLE TL_AUTOMATION_CP1_PATCH_RESULT)
if(NOT TL_AUTOMATION_CP1_PATCH_RESULT EQUAL 0)
    message(FATAL_ERROR "V9.9 automation CP1 integration failed")
endif()
execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/apply_v99_automation_cp2.py"
            --source-root "${CMAKE_SOURCE_DIR}" --output-dir "${TL_IMAGE_SCAN_GEN_DIR}"
    RESULT_VARIABLE TL_AUTOMATION_CP2_PATCH_RESULT)
if(NOT TL_AUTOMATION_CP2_PATCH_RESULT EQUAL 0)
    message(FATAL_ERROR "V9.9 automation CP2 PRECHECK integration failed")
endif()

add_library(ThanLongCleanRouteBridge SHARED "${TL_IMAGE_SCAN_GEN_DIR}/bridge.cpp")
target_include_directories(ThanLongCleanRouteBridge PRIVATE "${TL_IMAGE_SCAN_GEN_DIR}" src)
target_compile_definitions(ThanLongCleanRouteBridge PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(ThanLongCleanRouteBridge PRIVATE user32 kernel32)

# Rename the controller entry symbol at compile time so the license launcher
# owns the real wWinMain and opens the core only after validation succeeds.
set_source_files_properties("${TL_IMAGE_SCAN_GEN_DIR}/controller.cpp" PROPERTIES COMPILE_DEFINITIONS "wWinMain=ThanLongCoreMain")
add_executable(ThanLongItemConsolidator WIN32
    src/license_launcher.cpp
    "${TL_IMAGE_SCAN_GEN_DIR}/controller.cpp"
    "${TL_IMAGE_SCAN_GEN_DIR}/image_scan_test.cpp"
    src/telegram_notifier.cpp
    src/telegram_account_filter.cpp
    resources/app.rc)
target_include_directories(ThanLongItemConsolidator PRIVATE "${TL_IMAGE_SCAN_GEN_DIR}" src "${CMAKE_BINARY_DIR}/generated")
target_compile_definitions(ThanLongItemConsolidator PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0A00)
target_link_libraries(ThanLongItemConsolidator PRIVATE comctl32 comdlg32 user32 kernel32 gdi32 windowscodecs ole32 winhttp crypt32 advapi32 bcrypt)
target_link_options(ThanLongItemConsolidator PRIVATE /MANIFEST:NO)

add_executable(route_logic_tests src/route_logic_test.cpp)
target_include_directories(route_logic_tests PRIVATE src)
add_executable(trade_coordinator_logic_tests src/trade_coordinator_logic_test.cpp)
target_include_directories(trade_coordinator_logic_tests PRIVATE src)
add_executable(background_ui_logic_tests src/background_ui_logic_test.cpp)
target_include_directories(background_ui_logic_tests PRIVATE src)
add_executable(fixed_slot_sell_logic_tests src/fixed_slot_sell_logic_test.cpp)
target_include_directories(fixed_slot_sell_logic_tests PRIVATE src)
add_executable(unity_geometry_logic_tests src/unity_geometry_logic_test.cpp)
target_include_directories(unity_geometry_logic_tests PRIVATE src)
add_executable(internal_ui_click_logic_tests src/internal_ui_click_logic_test.cpp)
target_include_directories(internal_ui_click_logic_tests PRIVATE src)
add_executable(travel_fight_guard_logic_tests src/travel_fight_guard_logic_test.cpp)
target_include_directories(travel_fight_guard_logic_tests PRIVATE src)
add_executable(auto_fight_retry_logic_tests src/auto_fight_retry_logic_test.cpp)
target_include_directories(auto_fight_retry_logic_tests PRIVATE src)
add_executable(telegram_logic_tests src/telegram_logic_test.cpp)
target_include_directories(telegram_logic_tests PRIVATE src)
if(MSVC)
    # Native release hardening. This raises the cost of reverse engineering and
    # exploitation, but no client-side binary can be made impossible to inspect.
    set(TL_HARDEN_COMPILE_OPTIONS /O2 /Ob3 /Oi /GL /GS /sdl /guard:cf /Gy /Gw /GF /GR- /Zc:inline /Brepro)
    set(TL_HARDEN_LINK_OPTIONS /LTCG /OPT:REF /OPT:ICF /DYNAMICBASE /HIGHENTROPYVA /NXCOMPAT /guard:cf /CETCOMPAT /INCREMENTAL:NO /RELEASE /Brepro)

    target_compile_options(ThanLongCleanRouteBridge PRIVATE /utf-8 /W4 /permissive- /FIstdlib.h ${TL_HARDEN_COMPILE_OPTIONS})
    target_link_options(ThanLongCleanRouteBridge PRIVATE ${TL_HARDEN_LINK_OPTIONS})
    target_compile_options(ThanLongItemConsolidator PRIVATE ${TL_HARDEN_COMPILE_OPTIONS})
    target_link_options(ThanLongItemConsolidator PRIVATE ${TL_HARDEN_LINK_OPTIONS})
    foreach(t ThanLongItemConsolidator route_logic_tests trade_coordinator_logic_tests background_ui_logic_tests fixed_slot_sell_logic_tests unity_geometry_logic_tests internal_ui_click_logic_tests travel_fight_guard_logic_tests auto_fight_retry_logic_tests telegram_logic_tests)
        target_compile_options(${t} PRIVATE /utf-8 /W4 /permissive-)
    endforeach()
endif()
