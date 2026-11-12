# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Junior Deogracias
#
# This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
# Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.
#
# Build of the desktop: the corelink, workbench and frame libraries, the EmbLinkStudio
# application, its notices resource, its translations, and the Qt tests.

# The link to the core, shared by the application and its tests.
qt_add_library(emblink_studio_corelink STATIC
    src/framing.cpp src/framing.h
    src/coreclient.cpp src/coreclient.h
)
target_include_directories(emblink_studio_corelink PUBLIC src)
target_link_libraries(emblink_studio_corelink PUBLIC Qt6::Core)
target_compile_definitions(emblink_studio_corelink PUBLIC
    EMBLINK_STUDIO_PYTHON="${Python3_EXECUTABLE}"
    EMBLINK_STUDIO_CORE_SRC_DIR="${PROJECT_SOURCE_DIR}/core/src"
)

# The dockable layout: every view a dock, layouts saved and restored (spec section 7).
qt_add_library(emblink_studio_workbench STATIC
    src/workbench.cpp src/workbench.h
)
target_include_directories(emblink_studio_workbench PUBLIC src)
target_link_libraries(emblink_studio_workbench PUBLIC Qt6::Widgets ads::qtadvanceddocking-qt6)

# The frame (design brief section 4f): the theme read from the design tokens, the segmented
# control, the interface language and Settings. The tokens file in docs/design is the only
# source of the look; it is built in as :/design/tokens.json.
qt_add_library(emblink_studio_ui STATIC
    src/theme.cpp src/theme.h
    src/segmentedcontrol.cpp src/segmentedcontrol.h
    src/language.cpp src/language.h
    src/settingsdialog.cpp src/settingsdialog.h
)
target_include_directories(emblink_studio_ui PUBLIC src)
target_link_libraries(emblink_studio_ui PUBLIC Qt6::Widgets)
qt_add_resources(emblink_studio_ui design PREFIX "/design" BASE "${PROJECT_SOURCE_DIR}/docs/design"
    FILES "${PROJECT_SOURCE_DIR}/docs/design/tokens.json")

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

qt_add_executable(emblinkstudio WIN32 MACOSX_BUNDLE
    src/main.cpp
    src/mainwindow.cpp src/mainwindow.h
)
target_link_libraries(emblinkstudio PRIVATE emblink_studio_corelink emblink_studio_workbench
    emblink_studio_ui Qt6::Widgets)
target_compile_definitions(emblinkstudio PRIVATE EMBLINK_STUDIO_VERSION="${PROJECT_VERSION}")
set_source_files_properties("${notices}" PROPERTIES QT_RESOURCE_ALIAS NOTICES.md)
qt_add_resources(emblinkstudio notices PREFIX "/" BASE "${CMAKE_CURRENT_BINARY_DIR}" FILES "${notices}")
set_target_properties(emblinkstudio PROPERTIES
    OUTPUT_NAME EmbLinkStudio
    MACOSX_BUNDLE_BUNDLE_NAME "EmbLink Studio"
    MACOSX_BUNDLE_GUI_IDENTIFIER org.emblink.studio
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
)

# The interface in French and Simplified Chinese (design brief section 4f). English is the
# source. `cmake --build --preset dev --target update_translations` refreshes the .ts files
# from the sources; translate them in Qt Linguist. They are compiled into the frame library.
qt_add_translations(TARGETS emblink_studio_ui
    SOURCE_TARGETS emblinkstudio emblink_studio_ui
    TS_FILES i18n/emblinkstudio_fr.ts i18n/emblinkstudio_zh_CN.ts
    RESOURCE_PREFIX /i18n
    LUPDATE_OPTIONS -no-obsolete -locations none
)

if(EMBLINK_STUDIO_BUILD_TESTS)
    foreach(test tst_framing tst_coreclient)
        qt_add_executable(${test} tests/${test}.cpp)
        target_link_libraries(${test} PRIVATE emblink_studio_corelink Qt6::Test)
        add_test(NAME ${test} COMMAND ${test})
    endforeach()
    target_compile_definitions(tst_coreclient PRIVATE
        EMBLINK_STUDIO_FAKE_CORE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fake_core.py")

    qt_add_executable(tst_workbench tests/tst_workbench.cpp)
    target_link_libraries(tst_workbench PRIVATE emblink_studio_workbench Qt6::Test)
    add_test(NAME tst_workbench COMMAND tst_workbench)

    foreach(test tst_theme tst_segmentedcontrol tst_language)
        qt_add_executable(${test} tests/${test}.cpp)
        target_link_libraries(${test} PRIVATE emblink_studio_ui Qt6::Test)
        add_test(NAME ${test} COMMAND ${test})
    endforeach()
    # qt_standard_project_setup() makes executables GUI programs on Windows and bundles on
    # macOS; a test is a console program, so its output reaches CTest on every system.
    set_target_properties(tst_framing tst_coreclient tst_workbench tst_theme tst_segmentedcontrol
        tst_language PROPERTIES WIN32_EXECUTABLE OFF MACOSX_BUNDLE OFF)
    target_compile_definitions(tst_theme PRIVATE
        EMBLINK_STUDIO_TOKENS="${PROJECT_SOURCE_DIR}/docs/design/tokens.json")
endif()
