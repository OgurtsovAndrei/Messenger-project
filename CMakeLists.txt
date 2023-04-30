cmake_minimum_required(VERSION 3.23)
project(Messenger_project)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

include_directories(include)
include_directories(/usr/local/include/botan-3)
include_directories(src/install-lib/botan-install)
include_directories(src/general)
include_directories(include/database)
include_directories(src/server/database)
include_directories(bd)
find_package(SQLite3 REQUIRED)

add_definitions(-DCMAKE_EXPECTED_SQLite3_VERSION="${SQLite3_VERSION}")

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

target_link_libraries(main SQLite::SQLite3)
target_include_directories(main PRIVATE ${SQLite3_INCLUDE_DIRS})











#ARI
#cmake_minimum_required(VERSION 3.23)
#
#project(Messenger-project VERSION 0.1 LANGUAGES CXX)
#
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -v ")
#
#set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#
#
##for server compiling
#find_package(Threads REQUIRED)
#
#find_package(Boost 1.78 REQUIRED system)
#include_directories(/usr/local/include) #boost
#link_directories(/usr/local/lib)
#
#include_directories(/usr/local/include/botan-3) #botan
#
#include_directories(/opt/homebrew/opt/sqlite/include)  # sqlite3 -I
#link_directories(/opt/homebrew/opt/sqlite/lib)  # -L
#
#add_executable(Net1 src/general/NetTest/NetTest-1.cpp src/server/database/SQL_DataBaseInterface.cpp src/server/database/Dialog.cpp src/server/database/Message.cpp src/server/database/Mock_DataBaseInterface.cpp src/server/database/User.cpp)
#
#
#target_link_libraries(Net1 botan-3 sqlite3)
#
#
##for qt compiling
#
#set(CMAKE_AUTOUIC ON)
#set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTORCC ON)
#
#set(CMAKE_PREFIX_PATH /Users/arina/Apps/Qt/6.4.3/macos)
#
#find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
#find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
#
#set(PROJECT_SOURCES
#        src/interface/main.cpp
#        src/interface/mainwindow.cpp
#        src/interface/welcWindow.cpp
#        src/interface/register.cpp
#
#        include/interface/welcWindow.h
#        include/interface/mainwindow.h
#        include/interface/register.h
#
#        src/interface/mainwindow.ui
#        src/interface/register.ui
#        src/interface/welcWindow.ui
#        )
#
#add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
#
#target_link_libraries(${PROJECT_NAME} Qt${QT_VERSION_MAJOR}::Widgets botan-3)
