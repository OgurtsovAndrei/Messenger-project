cmake_minimum_required(VERSION 3.24)
project(Messenger_project)

set(CMAKE_CXX_STANDARD 20)

find_package(Threads REQUIRED)

include_directories(/usr/local/include/botan-3)
include_directories(src/install-lib/botan-install)
include_directories(src/general)

add_executable(Messenger_project src/general/CryptoTest/Botan-Test-1.cpp)
add_executable(Net1 src/general/NetTest/NetTest-1.cpp src/server/database/SQL_DataBaseInterface.cpp src/server/database/Dialog.cpp src/server/database/Message.cpp src/server/database/Mock_DataBaseInterface.cpp src/server/database/User.cpp)
add_executable(Net2 src/general/NetTest/NetTest-2.cpp)
add_executable(DB-Test src/server/database/DB-Test.cpp)
#add_executable(NetG src/general/NetTest/NetGeneral.hpp)
#add_executable(Messenger_project src/general/NetTest/NetTest-2.cpp)
#add_executable(Messenger_project src/general/NetTest/NetTest-3.cpp)


target_link_libraries(Messenger_project libbotan-3)
#target_link_libraries(19_230228 PRIVATE Threads::Threads)