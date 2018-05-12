#pragma once

class TimerTickable
{
  public:
    virtual void onTimerTick(int identifier) {}
};