#include "loger.h"

Loger::Loger() {}

Loger& Loger::getInstance()
{
    static Loger m_instance;
    return m_instance;
}

void Loger::setLogFile( std::string logFileName )
{
    std::lock_guard< std::recursive_mutex > lock( m_writeMutex );
    if( m_logFile.is_open() )
    {
        m_logFile.close();
    }
    m_logFile.open( logFileName,  std::ios::out  );
}

Loger::~Loger()
{
    std::lock_guard< std::recursive_mutex > lock( m_writeMutex );
    if( m_logFile.is_open() )
    {
        m_logFile.close();
    }
}

