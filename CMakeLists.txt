cmake_minimum_required(VERSION 2.8)

project(v)

enable_testing()

set(CMAKE_MACOSX_RPATH 1)

if (MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif ()

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
	set (CMAKE_CXX_FLAGS "-Wconversion -Wno-char-subscripts -Wno-shorten-64-to-32 -Wno-sign-conversion -msse4.2 -mavx -std=c++1z")
endif ()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")

add_library(common
	compile.cpp
	emit.cpp
	optimize.cpp
	parse.cpp
	stringtable.cpp
	test.cpp
	vm.cpp
)

add_executable(v main.cpp)
target_link_libraries(v common)

add_executable(vtest test.cpp)
target_link_libraries(vtest common)
