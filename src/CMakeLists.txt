﻿# CMakeList.txt : CMake project for solver, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.31)
project("solver")

add_definitions(-std=c++2a)
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/../bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/../cmake")

# find_package(packagename COMPONENTS component names REQUIRED)
# eg boost components progoptions/iostreams, mysql++, zeromq, efvi, etc
find_package(Boost COMPONENTS program_options REQUIRED)

include_directories(
  "${PROJECT_SOURCE_DIR}"
  "/usr/local/include/"
  )

# message(stuff here) to print

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

# start solver
file(GLOB
  solver_srcs
  # path/to/*cpps
  solver.cpp
)

# add_executable(solver ${solver_srcs})
# target_compile_options(solver PRIVATE -g -static -O3 -Wall -Werror -march=native -finline-functions -flto)
# 	or -O2 above
# 	add flags with -D{flagname}
# target_link_libraries(
# 	target name
# 	var libaray names (eg ${MYSQLPP_LIBRARIES})
# 		and then other libs like
# 	pthread
# 	crypto, ssl, numa, rt, stdc++, z, lz4

# Add source to this project's executable.
#add_executable (solver "solver.cpp" "rps.h" "blotto.h")
add_executable(solver ${solver_srcs})

#target_compile_options(solver PRIVATE -g -static -O3 -Wall -Werror -march=native -finline-functions -flto -fsanitize=address)
target_compile_options(solver PRIVATE -g -static -O3 -Wall -Werror -Wshadow -Wextra -march=native -finline-functions -flto)

target_link_libraries(
  solver

  ${Boost_PROGRAM_OPTIONS_LIBRARY}
)

file(GLOB
  dudotest
  dudo_test.cpp
)

add_executable(dudotest ${dudotest})
target_compile_options(dudotest PRIVATE -g -static -O3 -Wall -Werror -Wshadow -Wextra -march=native -finline-functions -flto)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET solver dudotest PROPERTY CXX_STANDARD 20)
endif()

