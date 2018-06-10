#include "Timer.h"

using namespace std;

Timer::Timer(bool tick, unsigned int intervallMS, TimerTickable *tT, int identifier)
{
    this->tick = tick;
    this->intervallMS = intervallMS;
    this->tT = tT;
    this->identifier = identifier;
    this->timerThread = NULL;
    this->state = t_stop;
    this->stateMutex = new mutex();
    lock();
}

Timer::~Timer()
{
    if (locked)
    {
        unlock();
    }
    stop();
}

void Timer::sleepFor(const unsigned int ms)
{
    if (!locked)
    {
        lock();
    }

    if (mut.try_lock_for(chrono::milliseconds(intervallMS)))
    {
        mut.unlock();
    }
}

void Timer::wakeup()
{
    if (locked)
    {
        unlock();
    }
}

void Timer::lock()
{
    mut.lock();
    locked = true;
}

void Timer::unlock()
{
    locked = false;
    mut.unlock();
}

void Timer::start()
{
    std::unique_lock<std::mutex> mlock(*stateMutex);
    switch (state)
    {
    case t_stop:
        state = t_run;
        break;

    case t_run:
        state = t_reset;
        break;

    default:
        break;
    }
    mlock.unlock();
    wakeup();

    if (!timerThread)
    {
        timerThread = new thread(&Timer::timerTask, this);
    }
}

void Timer::stop()
{
    std::unique_lock<std::mutex> mlock(*stateMutex);
    if (state == t_stop)
    {
        mlock.unlock();
        return;
    }

    state = t_stop;
    mlock.unlock();

    wakeup();

    if (timerThread && timerThread->joinable() && timerThread->get_id() != this_thread::get_id())
    {
        timerThread->join();
        delete timerThread;
        timerThread = NULL;
    }
    Logger::debug("Stoped timer: " + to_string(identifier));
}

void Timer::reset()
{
    std::unique_lock<std::mutex> mlock(*stateMutex);
    if (state == t_run)
    {
        state = t_reset;
    }
    mlock.unlock();

    wakeup();
}

void Timer::timerTask()
{
    Logger::debug("Started timer: " + to_string(identifier));
    while (state != t_stop)
    {
        sleepFor(intervallMS);

        std::unique_lock<std::mutex> mlock(*stateMutex);
        switch (state)
        {
        case t_run:
            mlock.unlock();
            if (tT)
            {
                if (ENABLE_TIMER_TICKED_DEBUG_OUTPUT)
                {
                    Logger::debug("Timer ticked: " + to_string(identifier));
                }
                tT->onTimerTick(identifier);
            }
            if (!tick)
            {
                return;
            }
            break;

        case t_reset:
            state = t_run;
            mlock.unlock();
            break;

        case t_stop:
            mlock.unlock();
            return;

        default:
            mlock.unlock();
            break;
        }
    }
}
