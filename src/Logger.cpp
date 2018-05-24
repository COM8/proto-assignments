#include "Logger.h"

using namespace std;

LogLevel Logger::logLevel = LogLevel::info;

void Logger::debug(string msg)
{
    if ((int)logLevel >= (int)debug)
    {
        cout << "[DEBUG]: " << msg << endl;
    }
}

void Logger::info(string msg)
{
    if ((int)logLevel >= (int)debug)
    {
        cout << "[INFO]: " << msg << endl;
    }
}

void Logger::warn(string msg)
{
    if ((int)logLevel >= (int)debug)
    {
        cout << "[WARN]: " << msg << endl;
    }
}

void Logger::error(string msg)
{
    if ((int)logLevel >= (int)debug)
    {
        cerr << "[ERROR]: " << msg << endl;
    }
}