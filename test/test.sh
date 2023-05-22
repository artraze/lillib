#!/bin/bash

mkdir -p __builddir__

gcc -pthread -Wall -D TESTING -I ../ -I . ../*.c *.cc -lgtest -lstdc++ -lm -o __builddir__/test.elf || exit

__builddir__/test.elf
