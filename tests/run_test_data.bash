# bin/bash

cxxtestgen --error-printer -o made-testes.cpp ./*.hpp
g++-12 made-testes.cpp -o tests -std=c++20
./tests


#-l:libbotan-3.a -I/usr/local/include/botan-3 -I./../include -I./../include/database -lsqlite3