# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Junior Deogracias

# The link to the core, shared by the application and its tests.
qt_add_library(embstudio_corelink STATIC
    src/framing.cpp src/framing.h
    src/coreclient.cpp src/coreclient.h
)
target_include_directories(embstudio_corelink PUBLIC src)
target_link_libraries(embstudio_corelink PUBLIC Qt6::Core)
target_compile_definitions(embstudio_corelink PUBLIC
    EMBSTUDIO_PYTHON="${Python3_EXECUTABLE}"
    EMBSTUDIO_CORE_SRC_DIR="${PROJECT_SOURCE_DIR}/core/src"
)

# The notices list shown in the About box, generated from third_party/components.json.
set(notices "${CMAKE_CURRENT_BINARY_DIR}/NOTICES.md")
add_custom_command(
    OUTPUT "${notices}"
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tools/notices.py"
            --components "${PROJECT_SOURCE_DIR}/third_party/components.json"
            --pyproject "${PROJECT_SOURCE_DIR}/core/pyproject.toml"
            --output "${notices}"
    DEPENDS "${PROJECT_SOURCE_DIR}/tools/notices.py"
            "${PROJECT_SOURCE_DIR}/third_party/components.json"
            "${PROJECT_SOURCE_DIR}/core/pyproject.toml"
    COMMENT "Generating the notices list"
    VERBATIM
)

qt_add_executable(embstudio WIN32 MACOSX_BUNDLE
    src/main.cpp
    src/mainwindow.cpp src/mainwindow.h
)
target_link_libraries(embstudio PRIVATE embstudio_corelink Qt6::Widgets)
target_compile_definitions(embstudio PRIVATE EMBSTUDIO_VERSION="${PROJECT_VERSION}")
set_source_files_properties("${notices}" PROPERTIES QT_RESOURCE_ALIAS NOTICES.md)
qt_add_resources(embstudio notices PREFIX "/" BASE "${CMAKE_CURRENT_BINARY_DIR}" FILES "${notices}")
set_target_properties(embstudio PROPERTIES
    OUTPUT_NAME EmbStudio
    MACOSX_BUNDLE_BUNDLE_NAME EmbStudio
    MACOSX_BUNDLE_GUI_IDENTIFIER org.emblink.embstudio
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
)

if(EMBSTUDIO_BUILD_TESTS)
    foreach(test tst_framing tst_coreclient)
        qt_add_executable(${test} tests/${test}.cpp)
        target_link_libraries(${test} PRIVATE embstudio_corelink Qt6::Test)
        add_test(NAME ${test} COMMAND ${test})
    endforeach()
    target_compile_definitions(tst_coreclient PRIVATE
        EMBSTUDIO_FAKE_CORE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fake_core.py")
endif()
