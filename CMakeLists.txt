
cmake_minimum_required(VERSION 3.10)

project(zmtx-zmrx VERSION 0.1)

add_executable(zmtx.ttp zmtx.c zmdm.c crctab.c tosterm.c tosfile.c getopt.c)
add_executable(zmrx.ttp zmrx.c zmdm.c crctab.c tosterm.c tosfile.c getopt.c)

file(WRITE libcmini/Make.config.local "CFLAGS += -DSTDIO_MAP_NEWLINE\nONLY_68K=Y\n")

add_custom_target(
    libcmini
    COMMAND make
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/libcmini
)

target_compile_options(zmrx.ttp PUBLIC -Wall -Werror -O2 -std=c99)
target_include_directories(zmrx.ttp SYSTEM PUBLIC ${CMAKE_SOURCE_DIR}/libcmini/include)
target_link_directories(zmrx.ttp PUBLIC ${CMAKE_SOURCE_DIR}/libcmini/build)
target_link_libraries(zmrx.ttp PUBLIC cmini gcc)
target_link_options(zmrx.ttp PUBLIC -s -nostdlib ${CMAKE_SOURCE_DIR}/libcmini/build/crt0.o)
add_dependencies(zmrx.ttp libcmini)

target_compile_options(zmtx.ttp PUBLIC -Wall -Werror -O2 -std=c99)
target_include_directories(zmtx.ttp SYSTEM PUBLIC ${CMAKE_SOURCE_DIR}/libcmini/include)
target_link_directories(zmtx.ttp PUBLIC ${CMAKE_SOURCE_DIR}/libcmini/build)
target_link_libraries(zmtx.ttp PUBLIC cmini gcc)
target_link_options(zmtx.ttp PUBLIC -s -nostdlib ${CMAKE_SOURCE_DIR}/libcmini/build/crt0.o)
add_dependencies(zmtx.ttp libcmini)

