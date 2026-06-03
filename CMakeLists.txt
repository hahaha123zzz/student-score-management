cmake_minimum_required(VERSION 3.10)
project(EduGrade LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8 /W3)
else()
    add_compile_options(-Wall -Wextra)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

set(SOURCES
    src/main.cpp
    src/utils.cpp
    src/storage.cpp
    src/models.cpp
    src/handlers.cpp
    src/stats.cpp
    src/sort.cpp
    src/server.cpp
)

add_executable(edugrade ${SOURCES})

set_target_properties(edugrade PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

if(WIN32)
    target_link_libraries(edugrade ws2_32)
endif()
