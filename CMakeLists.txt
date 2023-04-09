cmake_minimum_required(VERSION 3.23)
project(Messenger_project)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
#[[set(CMAKE_AUTOUIC ON)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        welcWindow.cpp
        register.cpp

        welcWindow.h
        mainwindow.h
        register.h

        mainwindow.ui
        register.ui
        welcWindow.ui
        )

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(Messenger_project
            MANUAL_FINALIZATION
            ${PROJECT_SOURCES}
            )
    # Define target properties for Android with Qt 6 as:
    #    set_property(TARGET Ui_messendger APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
    #                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
    # For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(Messenger_project SHARED
                ${PROJECT_SOURCES}
                )
        # Define properties for Android with Qt 5 after find_package() calls as:
        #    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(Messenger_project
                ${PROJECT_SOURCES}
                )
    endif()
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)]]


include_directories(include)
include_directories(/usr/local/include/botan-3)
include_directories(src/install-lib/botan-install)
include_directories(src/general)

add_executable(Net1 src/general/NetTest/NetTest-1.cpp src/server/database/SQL_DataBaseInterface.cpp src/server/database/Dialog.cpp src/server/database/Message.cpp src/server/database/Mock_DataBaseInterface.cpp src/server/database/User.cpp)
add_executable(Net2 src/general/NetTest/NetTest-2.cpp)
add_executable(Net3 src/general/NetTest/NetTest-3.cpp)
#add_executable(NetG src/general/NetTest/NetGeneral.hpp)
#add_executable(Messenger_project src/general/NetTest/NetTest-2.cpp)
#add_executable(Messenger_project src/general/NetTest/NetTest-3.cpp)


target_link_libraries(Net1 libbotan-3.a sqlite3)
target_link_libraries(Net2 libbotan-3.a)
target_link_libraries(Net3 libbotan-3.a)
#target_link_libraries(19_230228 PRIVATE Threads::Threads)

#[[
target_link_libraries(Messenger_project PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(Messenger_project PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE TRUE
        WIN32_EXECUTABLE TRUE
        )

install(TARGETS Messenger_project
        BUNDLE DESTINATION .
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Messenger_project)
endif()
]]
