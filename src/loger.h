#pragma once
#include <string>
#include <mutex>
#include <fstream>
#include <locale>


class Loger
{
public:

    static Loger& getInstance();
    void setLogFile( std::string logFileName );
    
    template< class T, class... Arguments >
    void log( const T& firstArg, const Arguments&... args )
    {
        if( !m_logFile.is_open() )
        {
            return;
        }
        std::lock_guard< std::recursive_mutex > lock( m_writeMutex );
        m_logFile << firstArg;
        m_logFile.flush();
        log( args... );
    }
    
    template<class T>
    void log( const T& arg )
    {
        if( !m_logFile.is_open() )
        {
            return;
        }
        std::lock_guard< std::recursive_mutex > lock( m_writeMutex );
        m_logFile << arg;
        m_logFile.flush();
    }
    
    Loger( const Loger& obj) = delete;
    Loger& operator=( const Loger& obj ) = delete;
    Loger( const Loger&& obj ) = delete;

    ~Loger();
private:

    Loger();

    std::recursive_mutex m_writeMutex;
    std::wofstream m_logFile;
};
