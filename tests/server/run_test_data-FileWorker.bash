# bin/bash

mkdir "./test-FileWorker-directory"
cxxtestgen --error-printer -o made-testes.cpp ./test-FileWorker.hpp
g++-12 made-testes.cpp -o tests -std=c++20 -I/usr/local/include/botan-3 -I./../../include -l:libbotan-3.a
./tests
exit_code=$?
rm made-testes.cpp tests
rm -r "./test-FileWorker-directory"
exit $exit_code

#   -I./../include/database -lsqlite3