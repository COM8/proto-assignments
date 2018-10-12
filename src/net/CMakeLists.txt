cmake_minimum_required(VERSION 3.0)

include_directories(../ ./ ../sec/ ../lib/hash-library/)

set(netfiles
    AbstractMessage.cpp
    AckMessage.cpp
    AuthRequestMessage.cpp
    AuthResultMessage.cpp
    Client2.cpp
    ClientHelloMessage.cpp
    FileCreationMessage.cpp
    FileStatusMessage.cpp
    FileTransferMessage.cpp
    PingMessage.cpp
    Server2.cpp
    ServerHelloMessage.cpp
    TransferEndedMessage.cpp

    )

add_library(net ${netfiles})
