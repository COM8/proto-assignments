cmake_minimum_required(VERSION 3.0)
project(csync VERSION 0.0.1)

add_subdirectory(src)

add_executable(${PROJECT_NAME} src/CSync.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}static)
target_include_directories(${PROJECT_NAME} PRIVATE src)
