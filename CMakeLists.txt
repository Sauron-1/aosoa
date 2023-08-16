cmake_minimum_required(VERSION 3.11)
project(aosoa)

set(CMAKE_CXX_STANDARD 20) 
set(CMAKE_CXX_STANDARD_REQUIRED ON) 

add_library(aosoa INTERFACE)
target_include_directories(
    aosoa INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/aosoa>
)

# require tuple_arithmetic library. Download from github if not found.
find_package(tuple_arithmetic QUIET)
if(NOT tuple_arithmetic_FOUND)
    message(STATUS "tuple_arithmetic not found. Downloading from github...")
    include(FetchContent)
    FetchContent_Declare(
        tuple_arithmetic
        GIT_REPOSITORY https://github.com/Sauron-1/tuple_arithmetic.git
        GIT_TAG        master
    )
    FetchContent_MakeAvailable(tuple_arithmetic)
endif()
# link tuple_arithmetic to aosoa
target_link_libraries(aosoa INTERFACE tuple_arithmetic)

target_compile_features(aosoa INTERFACE cxx_std_20)

install(TARGETS aosoa EXPORT aosoaConfig)
install(EXPORT aosoaConfig DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/aosoa)
install(DIRECTORY aosoa DESTINATION include)

export(TARGETS aosoa FILE aosoaConfig.cmake)
