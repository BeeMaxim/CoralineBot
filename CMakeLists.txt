cmake_minimum_required(VERSION 3.0.0)
project(CoralineBot VERSION 0.1.0 LANGUAGES C CXX)

#include(CTest)
#enable_testing()

add_compile_options(-fno-exceptions)

# set(CMAKE_EXE_LINKER_FLAGS "-static")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os")
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
set(BUILD_SHARED_LIBS OFF) 
include(CPack)

add_executable(CoralineBotExec BitBoard.cpp ZobristHash.cpp Position.cpp main.cpp)
