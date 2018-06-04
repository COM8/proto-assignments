#pragma once

#include <iostream>
#include <mutex>
#include <string>

enum LogLevel
{
    l_none = 0,
    l_error = 1,
    l_warn = 2,
    l_info = 3,
    l_debug = 4
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
};