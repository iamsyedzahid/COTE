cmake_minimum_required(VERSION 3.16)
project(CloudTaskOrchestrator CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(UNIX)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
endif()

include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.0
    GIT_SHALLOW    TRUE
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

file(GLOB_RECURSE SOURCES src/*.cpp)

add_executable(orchestrator ${SOURCES})

target_include_directories(orchestrator PRIVATE src)

target_link_libraries(orchestrator PRIVATE raylib)

if(UNIX)
    target_link_libraries(orchestrator PRIVATE Threads::Threads)
endif()

if(WIN32)
    target_link_libraries(orchestrator PRIVATE winmm)
    set_target_properties(orchestrator PROPERTIES WIN32_EXECUTABLE FALSE)
endif()

if(MSVC)
    target_compile_options(orchestrator PRIVATE /W3)
else()
    target_compile_options(orchestrator PRIVATE -Wall -Wextra -O2)
endif()
