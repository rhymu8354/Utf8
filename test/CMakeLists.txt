# CMakeLists.txt for Utf8Tests
#
# © 2018 by Richard Walters

cmake_minimum_required(VERSION 3.8)
set(This Utf8Tests)

set(Sources
    src/Utf8Tests.cpp
)

add_executable(${This} ${Sources})
set_target_properties(${This} PROPERTIES
    FOLDER Tests
)

target_include_directories(${This} PRIVATE ..)

target_link_libraries(${This} PUBLIC
    gtest_main
    Utf8
)

add_test(
    NAME ${This}
    COMMAND ${This}
)
