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
add_executable(Net1 netTest-1.cpp SQL_DataBaseInterface.cpp dialog.cpp message.cpp Mock_DataBaseInterface.cpp bdUser.cpp)
add_executable(Net2 netTest-2.cpp)
add_executable(ui main.cpp mainwindow.cpp welcWindow.cpp register.cpp user.cpp)

target_link_libraries(Net2 botan-3)
target_link_libraries(Net1 botan-3)
target_link_libraries(Net1 sqlite3)
target_link_libraries(ui botan-3)
target_link_libraries(ui Qt${QT_VERSION_MAJOR}::Widgets)
