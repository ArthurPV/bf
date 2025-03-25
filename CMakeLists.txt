cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_compile_options(-g)

add_executable(bf
	${CMAKE_SOURCE_DIR}/src/interpreter.c
	${CMAKE_SOURCE_DIR}/src/main.c)
