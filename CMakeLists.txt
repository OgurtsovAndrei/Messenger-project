cmake_minimum_required(VERSION 3.23)

project(Messenger-project VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(Boost REQUIRED system)
find_package(SQLite3 REQUIRED)
add_definitions(-DCMAKE_EXPECTED_SQLite3_VERSION="${SQLite3_VERSION}")

include_directories(${Boost_INCLUDE_DIRS}) #boost
link_directories(${Boost_LIBRARY_DIRS})

include_directories(${SQLite3_INCLUDE_DIRS}) #sqlite

include_directories(/usr/local/include/botan-3) #botan
link_directories(/usr/local/lib)


include_directories(include)
include_directories(/usr/local/include/botan-3)
include_directories(src/install-lib/botan-install)
include_directories(/usr/local/include/nlohmann)
include_directories(nlohmann/include)

set(SERVER_SOURCES
        src/RunServer.cpp
        src/database/SQL_DataBaseInterface.cpp
        src/database/Dialog.cpp
        src/database/Message.cpp
        src/database/Mock_DataBaseInterface.cpp
        src/database/User.cpp)



set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_QT_SOURCES
        src/interface/main.cpp
        src/interface/mainwindow.cpp
        src/interface/welcWindow.cpp
        src/interface/register.cpp
        src/interface/bubble.cpp
        src/interface/chatInfo.cpp
        src/interface/mesSetting.cpp
        src/interface/popUp.cpp
        src/interface/sureDo.cpp
        src/interface/chatSetting.cpp

        include/interface/welcWindow.h
        include/interface/mainwindow.h
        include/interface/register.h
        include/interface/bubble.h
        include/interface/chatInfo.h
        include/interface/clientinfo.h
        include/interface/mesSetting.h
        include/interface/popUp.h
        include/interface/sureDo.h
        include/interface/chatSetting.h

        src/interface/mainwindow.ui
        src/interface/register.ui
        src/interface/welcWindow.ui
        src/interface/chatInfo.ui
        src/interface/mesSetting.ui
        src/interface/popUp.ui
        src/interface/sureDo.ui
        src/interface/chatSetting.ui
        )

add_executable(server ${SERVER_SOURCES})
target_link_libraries(server ${SQLite3_LIBRARIES} botan-3)

add_executable(${PROJECT_NAME} ${PROJECT_QT_SOURCES})
target_link_libraries(${PROJECT_NAME} Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(${PROJECT_NAME} botan-3)
