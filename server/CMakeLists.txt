cmake_minimum_required(VERSION 3.21)
project(ImpostorServer LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
file(GLOB_RECURSE SOURCES src/*.cpp src/*.h)
add_executable(ImpostorServer ${SOURCES})
set_target_properties(ImpostorServer PROPERTIES
    WIN32_EXECUTABLE $<CONFIG:Release>
    OUTPUT_NAME LittleImpostorGame)
target_compile_definitions(ImpostorServer PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(ImpostorServer PRIVATE Qt6::Widgets ws2_32 iphlpapi)

add_custom_target(CopyQuestions
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/questions.txt
            ${CMAKE_CURRENT_BINARY_DIR}/questions.txt)
add_dependencies(ImpostorServer CopyQuestions)
