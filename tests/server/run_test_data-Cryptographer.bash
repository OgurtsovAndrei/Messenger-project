# bin/bash

cxxtestgen --error-printer -o made-testes.cpp ./*.hpp
g++-12 made-testes.cpp -o tests -std=c++20 -I/usr/local/include/botan-3 -I./../../include -l:libbotan-3.a
./tests


#   -I./../include/database -lsqlite3