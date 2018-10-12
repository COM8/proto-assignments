cmake_minimum_required(VERSION 3.0)

include_directories(./ net/ sec/ lib/hash-library/)

set(hashfiles
    lib/hash-library/md5.cpp
    lib/hash-library/crc32.cpp
    )

add_library(hash ${hashfiles})

link_libraries(hash)

add_subdirectory(net)
add_subdirectory(sec)
add_subdirectory(test)

link_libraries(net)
link_libraries(sec)
link_libraries(test)


set(csycfiles
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

add_library(csynclib ${csycfiles})
