#pragma once

#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>
#include "TimerTickable.h"
#include "Logger.h"
#include "Consts.h"

enum TimerState {
    t_stop,
    t_run,
    t_reset,
};

class Timer {

public:
    Timer(bool tick, unsigned int intervallMS, TimerTickable *tT, int identifier);
    ~Timer();

    void start();
    void stop();
    void reset();

private:
    bool tick;
    unsigned int intervallMS;
    TimerState state;
    std::thread* timerThread;
    TimerTickable *tT;
    int identifier;
    std::mutex *stateMutex;
    std::timed_mutex mut;
    std::atomic_bool locked;

    void timerTask();
    void lock();
    void unlock();
    void sleepFor(const unsigned int ms);
    void wakeup();
};