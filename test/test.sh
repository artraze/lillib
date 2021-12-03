#!/bin/bash

mkdir -p __builddir__

gcc -Wall -D TESTING -I ../ -I . ../*.c *.cc -pthread -lstdc++ -lgtest -o __builddir__/test.elf || exit

__builddir__/test.elf
