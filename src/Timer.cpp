#include "Timer.h"

using namespace std;

Timer::Timer(bool tick, unsigned int intervallMS, TimerTickable *tT, int identifier)
{
    this->tick = tick;
    this->intervallMS = intervallMS;
    this->tT = tT;
    this->identifier = identifier;
    this->wakeupIntervallMS = 250;
    this->timerThread = NULL;
    this->state = t_stoped;
    this->elapsedMS = 0;
    this->stateMutex = new mutex();
}

Timer::~Timer()
{
    stop();
}

void Timer::start()
{
    cout << "Started Timer: " << identifier << endl;
    std::unique_lock<std::mutex> mlock(*stateMutex);
    state = t_run;
    mlock.unlock();
    elapsedMS = 0;

    if (!timerThread)
    {
        timerThread = new thread(&Timer::timerTask, this);
    }
}

void Timer::stop()
{
    std::unique_lock<std::mutex> mlock(*stateMutex);
    if (state == t_stop || state == t_stoped)
    {
        mlock.unlock();
        return;
    }
    state = t_stop;
    if (timerThread && timerThread->joinable() && timerThread->get_id() != this_thread::get_id())
    {
        timerThread->join();
    }
    state = t_stoped;
    timerThread = NULL;
    mlock.unlock();
    cout << "Stoped Timer: " << identifier << endl;
}

void Timer::reset()
{
    std::unique_lock<std::mutex> mlock(*stateMutex);
    if (state == t_run)
    {
        state = t_reset;
    }
    mlock.unlock();
}

void Timer::timerTask()
{
    while (state != t_stop && state != t_stoped)
    {
        std::unique_lock<std::mutex> mlock(*stateMutex);
        switch (state)
        {
        case t_run:
            mlock.unlock();
            if (elapsedMS >= intervallMS)
            {
                if (tT)
                {
                    tT->onTimerTick(identifier);
                }
                if (!tick)
                {
                    return;
                }
                elapsedMS = 0;
            }
            break;

        case t_reset:
            mlock.unlock();
            elapsedMS = 0;
            break;

        case t_stoped:
        case t_stop:
            mlock.unlock();
            return;

        default:
            mlock.unlock();
            break;
        }

        usleep(intervallMS * 1000);
        elapsedMS += intervallMS;
    }
}
