cmake_minimum_required(VERSION 3.16)
project(test_duplicate_files LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Test)
qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(main)

target_sources(main PRIVATE
    main.cpp
)

qt_add_qml_module(main
    URI MyAppUri
)

target_link_libraries(main
    PRIVATE
    Qt::Quick
    Qt::Test
)

qt_target_qml_sources(main
    QML_FILES
        Main.qml
    RESOURCES
        file.txt
)

if(run STREQUAL "case_qml_files")
    qt_target_qml_sources(main
        QML_FILES
            Main.qml
    )
elseif(run STREQUAL "case_resources")
    qt_target_qml_sources(main
        RESOURCES
            file.txt
    )
else()
    message(FATAL_ERROR "Unrecognized run=${run}")
endif()

## TODO: Add additional test at configure time for de-duplication
#qt_query_qml_module(main
#    QML_FILES qml_files
#    RESOURCES resources
#)
