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
}

Timer::~Timer()
{
    stop();
}

void Timer::start()
{
    if (state != t_stoped)
    {
        return;
    }

    state = t_run;
    elapsedMS = 0;

    if (!timerThread)
    {
        timerThread = new thread(&Timer::timerTask, this);
    }
}

void Timer::stop()
{
    if (state != t_stop)
    {
        return;
    }

    state = t_stop;
    if (timerThread && timerThread->joinable())
    {
        timerThread->join();
    }
    state = t_stoped;
}

void Timer::reset()
{
    if (state != t_run)
    {
        return;
    }
    state = t_reset;
}

void Timer::timerTask()
{
    while (1)
    {
        switch (state)
        {
        case t_run:
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
            elapsedMS = 0;
            break;

        case t_stop:
            return;
        }

        usleep(intervallMS * 1000);
        elapsedMS += intervallMS;
    }
}
