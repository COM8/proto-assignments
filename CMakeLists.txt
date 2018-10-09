cmake_minimum_required(VERSION 3.0)
project(csync VERSION 0.0.1)


find_package(Threads REQUIRED)
target_link_libraries(Threads::Threads)
link_libraries(stdc++fs)
link_libraries(pthread)

include_directories(src/sec/ src/net src/lib/hash-library/)
add_subdirectory(src)

link_libraries(csynclib)

add_executable(csync src/CSync.cpp)
