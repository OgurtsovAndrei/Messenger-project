cmake_minimum_required(VERSION 3.23)

project(Ui_messendger VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_PREFIX_PATH /Users/arina/Apps/Qt/6.4.3/macos)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

find_package(Threads REQUIRED)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -v ")



find_package(Boost 1.78 REQUIRED system)

include_directories(/usr/local/include/botan-3)
include_directories(/usr/local/include)

link_directories(/usr/local/lib)

include_directories(/opt/homebrew/opt/sqlite/include)  # -I
link_directories(/opt/homebrew/opt/sqlite/lib)  # -L

include_directories(include)
add_executable(Net1 netTest-1.cpp SQL_DataBaseInterface.cpp dialog.cpp message.cpp Mock_DataBaseInterface.cpp user.cpp)
add_executable(Net2 netTest-2.cpp)
add_executable(ui main.cpp mainwindow.cpp welcWindow.cpp register.cpp userUI.cpp)

target_link_libraries(Net2 botan-3)
target_link_libraries(Net1 botan-3)
target_link_libraries(Net1 sqlite3)
target_link_libraries(ui botan-3)
target_link_libraries(ui Qt${QT_VERSION_MAJOR}::Widgets)
#
#set(PROJECT_SOURCES
#        main.cpp
#        mainwindow.cpp
#        welcWindow.cpp
#        register.cpp
#        user.cpp
#
#        welcWindow.h
#        mainwindow.h
#        register.h
#        user.h
#
#        mainwindow.ui
#        register.ui
#        welcWindow.ui
#)
#
#if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
#    qt_add_executable(Ui_messendger
#        MANUAL_FINALIZATION
#        ${PROJECT_SOURCES}
#    )
## Define target properties for Android with Qt 6 as:
##    set_property(TARGET Ui_messendger APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
##                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
## For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
#else()
#    if(ANDROID)
#        add_library(Ui_messendger SHARED
#            ${PROJECT_SOURCES}
#        )
## Define properties for Android with Qt 5 after find_package() calls as:
##    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
#    else()
#        add_executable(Ui_messendger
#            ${PROJECT_SOURCES}
#        )
#    endif()
#endif()
#
#target_link_libraries(Ui_messendger PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
#
#set_target_properties(Ui_messendger PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
#    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
#    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
#    MACOSX_BUNDLE TRUE
#    WIN32_EXECUTABLE TRUE
#)
#
#install(TARGETS Ui_messendger
#    BUNDLE DESTINATION .
#    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
#
#if(QT_VERSION_MAJOR EQUAL 6)
#    qt_finalize_executable(Ui_messendger)
#endif()
