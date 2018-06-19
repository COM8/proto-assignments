#pragma once

#include <iostream>
#include <mutex>
#include <list>
#include <unistd.h>
#include <time.h>
#include "Filesystem.h"
#include "net/Client2.h"
#include "net/Server2.h"
#include "WorkingSet.h"
#include "AbstractClient.h"

enum FileClient2State
{
    disconnected = 0,
    sendClientHello = 1,
    clientAuth = 2,
    connected = 3,
    ping = 4,
    reqestedFileStatus = 5,
    sendingFS = 6,
    awaitingAck = 7,
    resending = 8
};

class FileClient2 : public AbstractClient
{
  public:
    FileClient2(std::string serverAddress, unsigned short serverPort, std::string username, std::string clientPassword, FilesystemClient *fS);
    ~FileClient2();

    void connect();
    void disconnect();
    void onTimerTick(int identifier);
    FileClient2State getState();
    void printToDo();

  private:
    FileClient2State state;
    std::mutex *stateMutex;
    FilesystemClient *fS;
    unsigned short serverPort;
    unsigned short listenPort;
    unsigned short uploadPort;
    std::string serverAddress;
    std::thread helloThread;
    std::thread workingSetThread;
    bool shouldHelloRun;
    bool gettingWorkingSet;
    std::string clientPassword;
    std::string username;
    net::Client2 *client;
    net::Client2 *uploadClient;
    net::Server2 *server;
    bool reconnect;
    unsigned int msgTimeoutCount;
    WorkingSet *curWorkingSet;
    bool transferFinished;
    bool joinedWorkingSetThread;
    time_t lastGetWorkingSet;

    void setState(FileClient2State state);
    void startSendingFS();
    unsigned int getRandomClientId();
    void startHelloThread();
    void stopHelloThread();
    void startGetWorkingSet();
    void joinGetWorkingSet();
    void getWorkingSet();
    void helloTask(unsigned short listenPort, bool reconnecting, net::Client2 *client);
    void onMessageReceived(net::ReadMessage *msg);
    void sendNextFilePart();
    bool sendFilePartMessage(std::string fid, std::shared_ptr<File> f, unsigned int nextPartNr, net::Client2 *client);
    void onServerHelloMessage(net::ReadMessage *msg);
    void onFileStatusMessage(net::ReadMessage *msg);
    void onAckMessage(net::ReadMessage *msg);
    void onPingMessage(net::ReadMessage *msg);
    void onAuthResultMessage(net::ReadMessage *msg);
};