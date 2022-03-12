
cmake_minimum_required(VERSION 3.10)

project(zmtx-zmrx VERSION 0.1)

add_executable(zmtx.ttp zmtx.c zmdm.c crctab.c tosterm.c tosfile.c)
add_executable(zmrx.ttp zmrx.c zmdm.c crctab.c tosterm.c tosfile.c)


target_compile_options(zmrx.ttp PUBLIC -Wall -Werror -O2)
target_include_directories(zmrx.ttp SYSTEM PUBLIC ${CMAKE_SOURCE_DIR}/libcmini/include)
target_link_directories(zmrx.ttp PUBLIC ${CMAKE_SOURCE_DIR}/libcmini/build)
target_link_libraries(zmrx.ttp PUBLIC cmini gcc)
target_link_options(zmrx.ttp PUBLIC -s -nostdlib ${CMAKE_SOURCE_DIR}/libcmini/build/crt0.o)

target_compile_options(zmtx.ttp PUBLIC -Wall -Werror -O2)
target_include_directories(zmtx.ttp SYSTEM PUBLIC ${CMAKE_SOURCE_DIR}/libcmini/include)
target_link_directories(zmtx.ttp PUBLIC ${CMAKE_SOURCE_DIR}/libcmini/build)
target_link_libraries(zmtx.ttp PUBLIC cmini gcc)
target_link_options(zmtx.ttp PUBLIC -s -nostdlib ${CMAKE_SOURCE_DIR}/libcmini/build/crt0.o)
