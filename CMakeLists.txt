cmake_minimum_required(VERSION 3.10)
project(stardust)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)

# Specify the directories
include_directories(inc)

# Find the Flex package
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

# Generate the lexer source file
BISON_TARGET(MyParser src/lex/syntax.y ${CMAKE_CURRENT_BINARY_DIR}/syntax.cpp)
FLEX_TARGET(MyScanner src/lex/lex.l ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.cpp)

ADD_FLEX_BISON_DEPENDENCY(MyScanner MyParser)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# List all other source files
aux_source_directory(./src DIR_SRCS)

# Create the executable
add_executable(stardust ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.cpp ${CMAKE_CURRENT_BINARY_DIR}/syntax.cpp ${DIR_SRCS})