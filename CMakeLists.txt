cmake_minimum_required(VERSION 3.23)

project(Messenger-project VERSION 0.1 LANGUAGES CXX)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -v ")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


#for server compiling
find_package(Threads REQUIRED)

find_package(Boost 1.78 REQUIRED system)
include_directories(/usr/local/include) #boost
link_directories(/usr/local/lib)

include_directories(/usr/local/include/botan-3) #botan

include_directories(/opt/homebrew/opt/sqlite/include)  # sqlite3 -I
link_directories(/opt/homebrew/opt/sqlite/lib)  # -L

add_executable(Net1 netTest-1.cpp SQL_DataBaseInterface.cpp dialog.cpp message.cpp Mock_DataBaseInterface.cpp user.cpp)

target_link_libraries(Net1 botan-3 sqlite3)


#for qt compiling

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_PREFIX_PATH /Users/arina/Apps/Qt/6.4.3/macos)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        welcWindow.cpp
        register.cpp
        userUI.cpp

        welcWindow.h
        mainwindow.h
        register.h
        userUI.h

        mainwindow.ui
        register.ui
        welcWindow.ui
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_link_libraries(${PROJECT_NAME} Qt${QT_VERSION_MAJOR}::Widgets botan-3)
