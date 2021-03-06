cmake_minimum_required(VERSION 3.10)

# Project name
project(tikpp CXX)
set(MAIN_PROJECT_NAME ${PROJECT_NAME})

# Project options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic")
endif()

set(LIBRARY_OUTPUT_PATH lib)

# Thiry-party libraries
add_subdirectory(lib/fmt)
add_subdirectory(lib/googletest)
add_subdirectory(lib/openssl-cmake)

# Project files
set(SOURCE_FILES src/error.cpp
                 src/crypto.cpp
                 src/request.cpp
                 src/response.cpp)
include_directories(include/)

# Targets
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} fmt::fmt ssl crypto)
target_include_directories(${PROJECT_NAME} PUBLIC include)

# Testing
enable_testing()
add_subdirectory(tests)
