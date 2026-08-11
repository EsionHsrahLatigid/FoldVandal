cmake_minimum_required(VERSION 3.31)

if(APPLE)
    project(FoldVandal VERSION 0.1.0 LANGUAGES C CXX OBJC OBJCXX)
else()
    project(FoldVandal VERSION 0.1.0 LANGUAGES C CXX)
endif()

option(FOLDVANDAL_BUILD_PLUGIN "Build the YUP Standalone/VST3/AU targets" ON)
option(FOLDVANDAL_BUILD_TESTS "Build deterministic DSP regression tests" ON)
option(FOLDVANDAL_ADHOC_SIGN "Ad-hoc sign local macOS plugin and app bundles after building" ON)
set(FOLDVANDAL_CREATE_AU OFF)
if(APPLE)
    set(FOLDVANDAL_CREATE_AU ON)
endif()
set(FOLDVANDAL_YUP_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../yup" CACHE PATH
    "Path to a local YUP checkout; fetched from upstream when unavailable")

add_library(foldvandal_engine STATIC
    source/FoldVandalEngine.cpp)
target_include_directories(foldvandal_engine PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/include")
target_compile_features(foldvandal_engine PUBLIC cxx_std_20)
set_target_properties(foldvandal_engine PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(foldvandal_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(FOLDVANDAL_BUILD_TESTS)
    include(CTest)

    function(enable_test_assertions target_name)
        if(MSVC)
            target_compile_options(${target_name} PRIVATE /UNDEBUG)
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(${target_name} PRIVATE -UNDEBUG)
        endif()
    endfunction()

    add_executable(foldvandal_engine_tests
        tests/FoldVandalEngineTests.cpp)
    target_link_libraries(foldvandal_engine_tests PRIVATE foldvandal_engine)
    target_compile_features(foldvandal_engine_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(foldvandal_engine_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    enable_test_assertions(foldvandal_engine_tests)
    add_test(NAME foldvandal_engine_tests COMMAND foldvandal_engine_tests)
endif()

if(FOLDVANDAL_BUILD_PLUGIN)
    set(YUP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(YUP_BUILD_PLUGINS OFF CACHE BOOL "" FORCE)
    set(YUP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(YUP_ENABLE_PLUGINVAL OFF CACHE BOOL "" FORCE)
    set(YUP_ENABLE_VST3_VALIDATOR OFF CACHE BOOL "" FORCE)
    set(YUP_ENABLE_CLAP_VALIDATOR OFF CACHE BOOL "" FORCE)
    set(YUP_ENABLE_AUVAL_VALIDATOR OFF CACHE BOOL "" FORCE)

    if(EXISTS "${FOLDVANDAL_YUP_SOURCE_DIR}/CMakeLists.txt")
        add_subdirectory("${FOLDVANDAL_YUP_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/yup")
    else()
        include(FetchContent)
        FetchContent_Declare(yup
            GIT_REPOSITORY https://github.com/kunitoki/yup.git
            GIT_TAG 9a1c9bc699b6a714f6f52486462d98a140c8bf95)
        FetchContent_MakeAvailable(yup)
    endif()

    # YUP's iconutil-based helper rejects its generated iconset on current
    # macOS. Keep the compatibility fix project-local until upstream changes.
    if(APPLE)
        include(cmake/YupMacOSIconWorkaround.cmake)
    endif()

    yup_audio_plugin(
        TARGET_NAME foldvandal
        TARGET_VERSION ${PROJECT_VERSION}
        TARGET_IDE_GROUP "FoldVandal"
        TARGET_APP_ID "jp.ehl.foldvandal"
        TARGET_APP_NAMESPACE "jp.ehl"
        TARGET_CXX_STANDARD 20
        PLUGIN_ID "jp.ehl.foldvandal"
        PLUGIN_NAME "FoldVandal"
        PLUGIN_VENDOR "ehl_"
        PLUGIN_EMAIL ""
        PLUGIN_VERSION "${PROJECT_VERSION}"
        PLUGIN_DESCRIPTION "Asymmetric stereo multi-fold wavefolder"
        PLUGIN_URL "https://github.com/EsionHsrahLatigid/FoldVandal"
        PLUGIN_IS_SYNTH OFF
        PLUGIN_IS_MONO OFF
        PLUGIN_CREATE_CLAP OFF
        PLUGIN_CREATE_VST3 ON
        PLUGIN_CREATE_AU ${FOLDVANDAL_CREATE_AU}
        PLUGIN_AU_SUBTYPE "FdVn"
        PLUGIN_AU_MANUFACTURER "EHL1"
        PLUGIN_AU_SANDBOX_SAFE ON
        PLUGIN_CREATE_AUv3 OFF
        PLUGIN_CREATE_LV2 OFF
        PLUGIN_CREATE_AAX OFF
        PLUGIN_CREATE_STANDALONE ON
        PLUGIN_COPY_AFTER_BUILD OFF
        MODULES
            yup::yup_gui
            yup::yup_audio_processors)

    target_sources(foldvandal_shared INTERFACE
        source/ParameterGridEditor.cpp
        source/ParameterGridEditor.h
        source/ProductState.h
        source/FoldVandalPlugin.cpp
        source/FoldVandalPlugin.h)
    target_link_libraries(foldvandal_shared INTERFACE foldvandal_engine)
    target_include_directories(foldvandal_shared INTERFACE
        "${CMAKE_CURRENT_LIST_DIR}/include"
        "${CMAKE_CURRENT_LIST_DIR}/source")

    if(FOLDVANDAL_BUILD_TESTS)
        add_executable(foldvandal_plugin_bridge_tests
            tests/FoldVandalPluginBridgeTests.cpp
            source/FoldVandalPlugin.cpp)
        target_link_libraries(foldvandal_plugin_bridge_tests PRIVATE foldvandal_engine yup::yup_audio_processors)
        target_include_directories(foldvandal_plugin_bridge_tests PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/include"
            "${CMAKE_CURRENT_LIST_DIR}/source")
        target_compile_definitions(foldvandal_plugin_bridge_tests PRIVATE FOLDVANDAL_HEADLESS_TEST=1)
        target_compile_features(foldvandal_plugin_bridge_tests PRIVATE cxx_std_20)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(foldvandal_plugin_bridge_tests PRIVATE -Wall -Wextra -Wpedantic)
        endif()
        enable_test_assertions(foldvandal_plugin_bridge_tests)
        add_test(NAME foldvandal_plugin_bridge_tests COMMAND foldvandal_plugin_bridge_tests)

        add_executable(foldvandal_standalone_bridge_tests
            tests/FoldVandalStandaloneBridgeTests.cpp
            source/FoldVandalPlugin.cpp)
        target_link_libraries(foldvandal_standalone_bridge_tests PRIVATE foldvandal_engine yup::yup_audio_processors)
        target_include_directories(foldvandal_standalone_bridge_tests PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/include"
            "${CMAKE_CURRENT_LIST_DIR}/source")
        target_compile_definitions(foldvandal_standalone_bridge_tests PRIVATE
            FOLDVANDAL_HEADLESS_TEST=1
            YUP_AUDIO_PLUGIN_ENABLE_STANDALONE=1)
        target_compile_features(foldvandal_standalone_bridge_tests PRIVATE cxx_std_20)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(foldvandal_standalone_bridge_tests PRIVATE -Wall -Wextra -Wpedantic)
        endif()
        enable_test_assertions(foldvandal_standalone_bridge_tests)
        add_test(NAME foldvandal_standalone_bridge_tests COMMAND foldvandal_standalone_bridge_tests)
    endif()

    set(foldvandal_release_bundle_targets
        foldvandal_standalone_plugin
        foldvandal_vst3_plugin)
    if(APPLE)
        list(APPEND foldvandal_release_bundle_targets foldvandal_au_plugin)
    endif()

    add_custom_target(foldvandal_release_bundles
        DEPENDS ${foldvandal_release_bundle_targets})

    include(cmake/EhlYupArtifactLayout.cmake)
    ehl_yup_add_artifact_target(
        TARGET ehl_stage_products
        PRODUCT FoldVandal
        SLUG foldvandal
        STANDALONE_TARGET foldvandal_standalone_plugin
        VST3_TARGET foldvandal_vst3_plugin
        AU_TARGET foldvandal_au_plugin)

    if(APPLE)
        set_target_properties(foldvandal_standalone_plugin PROPERTIES
            MACOSX_BUNDLE_GUI_IDENTIFIER "jp.ehl.foldvandal"
            MACOSX_BUNDLE_BUNDLE_NAME "FoldVandal"
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "jp.ehl.foldvandal")
    endif()

    if(APPLE AND FOLDVANDAL_ADHOC_SIGN)
        find_program(FOLDVANDAL_CODESIGN_EXECUTABLE codesign REQUIRED)
        add_custom_command(TARGET foldvandal_vst3_plugin POST_BUILD
            COMMAND "${FOLDVANDAL_CODESIGN_EXECUTABLE}" --force --deep --sign -
                    "$<TARGET_BUNDLE_DIR:foldvandal_vst3_plugin>"
            COMMENT "Ad-hoc signing FoldVandal VST3 for local loading"
            VERBATIM)
        add_custom_command(TARGET foldvandal_standalone_plugin POST_BUILD
            COMMAND "${FOLDVANDAL_CODESIGN_EXECUTABLE}" --force --deep --sign -
                    "$<TARGET_BUNDLE_DIR:foldvandal_standalone_plugin>"
            COMMENT "Ad-hoc signing FoldVandal standalone app for local launch"
            VERBATIM)
    endif()
endif()
