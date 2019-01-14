cmake_minimum_required(VERSION 3.0)

add_library(net STATIC)
target_sources(net
    PRIVATE
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
