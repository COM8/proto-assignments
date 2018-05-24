#pragma once

#include <iostream>
#include <mutex>
#include <string>

enum LogLevel
{
    none = 0,
    error = 1,
    warn = 2,
    info = 3,
    debug = 4
};

class Logger
{
  public:
    static LogLevel logLevel;

    static void debug(std::string msg);
    static void info(std::string msg);
    static void warn(std::string msg);
    static void error(std::string msg);
    static void none(std::string msg);

  private:
}