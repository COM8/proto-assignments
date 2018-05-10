#pragma once

#include <iostream>
#include <thread>
#include <unistd.h>
#include "TimerTickable.h"

enum TimerState {
    t_stop,
    t_run,
    t_reset,
};

class Timer {

public:
    unsigned int wakeupIntervallMS;

    Timer(bool tick, unsigned int intervallMS, TimerTickable *tT);
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

    void timerTask();

};