# CMakeList.txt : CMake project for solver, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)
project ("solver")

add_definitions(-std=c++2a)
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/../bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/../cmake")

# find_package(packagename COMPONENTS component names REQUIRED)
# eg boost components progoptions/iostreams, mysql++, zeromq, efvi, etc
find_package(Boost COMPONENTS program_options REQUIRED)

include_directories(
  "${PROJECT_SOURCE_DIR}"
  "${Boost_INCLUDE_DIR}"
  "/usr/local/include/"
  # other path/to/include
  )

# message(stuff here) to print

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

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
add_executable (solver ${solver_srcs})

#target_compile_options(solver PRIVATE -g -static -O3 -Wall -Werror -march=native -finline-functions -flto -fsanitize=address)
target_compile_options(solver PRIVATE -g -static -O3 -Wall -Werror -march=native -finline-functions -flto )
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET solver PROPERTY CXX_STANDARD 20)
endif()

target_link_libraries(
  solver

  ${Boost_PROGRAM_OPTIONS_LIBRARY}
)


# TODO: Add tests and install targets if needed.

# example
# 
