cmake_minimum_required(VERSION 3.14)
project(EduGrade LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8 /W3)
else()
    add_compile_options(-Wall -Wextra)
endif()

include_directories(${CMAKE_SOURCE_DIR}/lib)

add_executable(edugrade
    src/main.cpp
)

set_target_properties(edugrade PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

if(WIN32)
    target_link_libraries(edugrade ws2_32)
endif()
