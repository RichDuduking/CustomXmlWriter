cmake_minimum_required(VERSION 3.16)
project(CustomWriter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(custom_writer
  src/xml_writer.cpp
)
target_include_directories(custom_writer
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(MSVC)
  target_compile_options(custom_writer PRIVATE /W4 /permissive- /utf-8)
else()
  target_compile_options(custom_writer PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()

add_executable(xml_writer_tests
  tests/xml_writer_test.cpp
)
target_link_libraries(xml_writer_tests PRIVATE custom_writer)
add_test(NAME xml_writer_tests COMMAND xml_writer_tests)

add_executable(CustomWriter
  src/main.cpp
)
target_link_libraries(CustomWriter PRIVATE custom_writer)

add_test(NAME customwriter_cli
  COMMAND ${CMAKE_COMMAND}
    "-DWRITER_EXE=$<TARGET_FILE:CustomWriter>"
    "-DOUTPUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/cli-smoke.xml"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/cli_smoke.cmake"
)
