cmake_minimum_required(VERSION 3.28)

project(timetracker)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SQLite3 REQUIRED)
if(NOT SQLite3_FOUND)
  message(FATAL_ERROR "SQLite3 not found. Please install libsqlite3-dev.")
endif()

add_executable(timetracker db.cpp main.cpp)

target_include_directories(timetracker PUBLIC ${SQLite3_INCLUDE_DIRS})
target_link_libraries(timetracker ${SQLite3_LIBRARIES})
