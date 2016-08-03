cmake_minimum_required(VERSION 3.5)
project(Assembler)

set(SOURCE_FILES main.c assembler.c assembler.h tokenize.c tokenize.h binary_code.c binary_code.h common.h)
add_executable(dasm ${SOURCE_FILES})
