#include "Timer.h"

using namespace std;

Timer::Timer(bool tick, unsigned int intervallMS, TimerTickable *tT)
{
    this->tick = tick;
    this->intervallMS = intervallMS;
    this->tT = tT;
    this->wakeupIntervallMS = 250;
    this->timerThread = NULL;
    this->state = t_stop;
    this->elapsedMS = 0;
}

Timer::~Timer(){
    stop();
}

void Timer::start()
{
    state = t_run;

    if (!timerThread)
    {
        timerThread = new thread(&Timer::timerTask, this);
    }
}

void Timer::stop()
{
    state = t_stop;
    if (timerThread && timerThread->joinable())
	{
		timerThread->join();
	}
}

void Timer::reset()
{
    state = t_reset;    
}

void Timer::timerTask()
{
    while (1)
    {
        switch (state)
        {
        case t_run:
            if(elapsedMS >= intervallMS) {
                if(tT) {
                    tT->onTimerTick();
                }
                if(!tick) {
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

        usleep(intervallMS*1000);
        elapsedMS += intervallMS;
    }
}
