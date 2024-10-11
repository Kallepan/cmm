#!/bin/bash


cmake -S . -B _build -DCMAKE_BUILD_TYPE=Debug
cmake --build _build

./_build/cmm input.cm

cd _test && nasm -felf64 test.asm && x86_64-linux-gnu-ld -m elf_x86_64 test.o -o test && ./test
echo $?