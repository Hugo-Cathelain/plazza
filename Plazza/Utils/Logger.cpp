///////////////////////////////////////////////////////////////////////////////
// Dependencies
///////////////////////////////////////////////////////////////////////////////
#include "Utils/Logger.hpp"
#include <iomanip>
#include <sstream>

///////////////////////////////////////////////////////////////////////////////
// Namespace Plazza
///////////////////////////////////////////////////////////////////////////////
namespace Plazza
{

///////////////////////////////////////////////////////////////////////////////
std::string Logger::m_logFilePath = "";

///////////////////////////////////////////////////////////////////////////////
bool Logger::m_consoleOutput = true;

///////////////////////////////////////////////////////////////////////////////
Mutex Logger::m_logMutex;

///////////////////////////////////////////////////////////////////////////////
void Logger::SetLogFile(const std::string& filePath)
{
    std::lock_guard<std::mutex> lock(m_logMutex);
    m_logFilePath = filePath;
}

///////////////////////////////////////////////////////////////////////////////
void Logger::SetConsoleOutput(bool enable)
{
    std::lock_guard<std::mutex> lock(m_logMutex);
    m_consoleOutput = enable;
}

///////////////////////////////////////////////////////////////////////////////
void Logger::Log(
    Level level,
    const std::string& sender,
    const std::string& message
)
{
    std::string logMessage = "[" + GetTimestamp() + "] "
                           + "[" + LogLevelToString(level) + "] "
                           + "[" + sender + "] "
                           + message;
    WriteLog(logMessage);
}

///////////////////////////////////////////////////////////////////////////////
void Logger::Info(const std::string& sender, const std::string& message)
{
    Log(Level::INFO, sender, message);
}

///////////////////////////////////////////////////////////////////////////////
void Logger::Warning(const std::string& sender, const std::string& message)
{
    Log(Level::WARNING, sender, message);
}

///////////////////////////////////////////////////////////////////////////////
void Logger::Error(const std::string& sender, const std::string& message)
{
    Log(Level::ERROR, sender, message);
}

///////////////////////////////////////////////////////////////////////////////
void Logger::Debug(const std::string& sender, const std::string& message)
{
    Log(Level::DEBUG, sender, message);
}

///////////////////////////////////////////////////////////////////////////////
void Logger::WriteLog(const std::string& logMessage)
{
    std::lock_guard<std::mutex> lock(m_logMutex);

    if (m_consoleOutput)
    {
        std::cout << logMessage << std::endl;
    }

    if (!m_logFilePath.empty())
    {
        std::ofstream logFile(m_logFilePath, std::ios::app);
        if (logFile.is_open())
        {
            logFile << logMessage << std::endl;
            logFile.close();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
std::string Logger::GetTimestamp(void)
{
    auto now = std::time(nullptr);
    auto localTime = *std::localtime(&now);

    std::stringstream ss;
    ss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return (ss.str());
}

///////////////////////////////////////////////////////////////////////////////
std::string Logger::LogLevelToString(Level level)
{
    switch (level)
    {
        case Level::INFO:    return ("INFO");
        case Level::WARNING: return ("WARN");
        case Level::ERROR:   return ("ERROR");
        case Level::DEBUG:   return ("DEBUG");
        default:             return ("UNKNOWN");
    }
}

} // !namespace Plazza
