#pragma once

#include <iostream>
#include <thread>
#include <unistd.h>
#include <mutex>
#include "TimerTickable.h"

enum TimerState {
    t_stoped,
    t_stop,
    t_run,
    t_reset,
};

class Timer {

public:
    unsigned int wakeupIntervallMS;

    Timer(bool tick, unsigned int intervallMS, TimerTickable *tT, int identifier);
    ~Timer();

    void start();
    void stop();
    void reset();

private:
    bool tick;
    unsigned int intervallMS;
    unsigned int elapsedMS;
    TimerState state;
    std::thread* timerThread;
    TimerTickable *tT;
    int identifier;
    std::mutex *stateMutex;

    void timerTask();

};