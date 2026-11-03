cmake_minimum_required(VERSION 3.16)
project(xelang VERSION 1.7.0 LANGUAGES CXX)
find_package(Threads REQUIRED)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(XELANG_RUNTIME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src" CACHE PATH "Location of the XeLang C++ runtime header")
set(XELANG_STDLIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/std" CACHE PATH "Location of the Xe standard library source modules")
add_executable(xelang src/main.cpp src/lexer.cpp src/parser.cpp src/checker.cpp src/codegen.cpp)
set_target_properties(xelang PROPERTIES OUTPUT_NAME xe)
target_include_directories(xelang PRIVATE src)
target_compile_definitions(xelang PRIVATE XELANG_RUNTIME_DIR="${XELANG_RUNTIME_DIR}" XELANG_STDLIB_DIR="${XELANG_STDLIB_DIR}" XELANG_CXX_COMPILER="${CMAKE_CXX_COMPILER}")
target_link_libraries(xelang PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
if(MSVC)
  target_compile_options(xelang PRIVATE /W4)
else()
  target_compile_options(xelang PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/stage0_acceptance.cmake")
    add_test(
      NAME stage0_acceptance
      COMMAND ${CMAKE_COMMAND}
        -DXE=$<TARGET_FILE:xelang>
        -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/acceptance
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/stage0_acceptance.cmake)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/stage1_acceptance.cmake")
    add_test(
      NAME stage1_acceptance
      COMMAND ${CMAKE_COMMAND}
        -DXE=$<TARGET_FILE:xelang>
        -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/acceptance
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/stage1_acceptance.cmake)
  endif()
endif()
