#include "Logger.h"

using namespace std;

LogLevel Logger::logLevel = LogLevel::l_debug;

void Logger::debug(string msg)
{
    if ((int)logLevel >= (int)l_debug)
    {
        cout << "[DEBUG]: " << msg << endl;
    }
}

void Logger::info(string msg)
{
    if ((int)logLevel >= (int)l_info)
    {
        cout << "[INFO]: " << msg << endl;
    }
}

void Logger::warn(string msg)
{
    if ((int)logLevel >= (int)l_warn)
    {
        cout << "[WARN]: " << msg << endl;
    }
}

void Logger::error(string msg)
{
    if ((int)logLevel >= (int)l_error)
    {
        cerr << "[ERROR]: " << msg << endl;
    }
}