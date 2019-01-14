cmake_minimum_required(VERSION 3.0)

find_package(Threads REQUIRED)

add_library(hash STATIC)
target_sources(hash
    PRIVATE
        lib/hash-library/md5.cpp
        lib/hash-library/crc32.cpp
)

add_subdirectory(net)
add_subdirectory(sec)
add_subdirectory(test)

add_library(${PROJECT_NAME}lib STATIC)

target_sources(${PROJECT_NAME}lib
    PRIVATE
        AbstractClient.cpp
        ClientsToDo.cpp
        ClientToDo.cpp
        diffieTest.cpp
        FileClient2.cpp
        FileServer2.cpp
        FileServerClient.cpp
        FileServerUser.cpp
        Filesystem.cpp
        Logger.cpp
        Timer.cpp
        UserStore.cpp
)

target_link_libraries(${PROJECT_NAME}lib
    PRIVATE
        net
        sec
        test
        hash
        stdc++fs
        Threads::Threads
)
