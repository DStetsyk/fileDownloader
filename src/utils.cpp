#include "utils.h"

enum class ESizeModifiers
{
    eSM_KiloByte = 1024,
    eSM_MegaByte = 1048576,
    eSM_GigaByte = 1073741824
};

std::list<std::wstring> ParseCommandLine( PWSTR pCmdLine )
{
    std::list< std::wstring > resultList;
    std::wstring cmdString( pCmdLine );
    std::wstring bufferString;

    for( wchar_t ch : cmdString )
    {
        if( ch == ' ' )
        {
            if( !bufferString.empty() )
            {
                resultList.push_back( bufferString );
                bufferString = L"";
            }
        }
        else
        {
            bufferString += ch;
        }
    }

    if( !bufferString.empty() )
    {
        resultList.push_back( bufferString );
    }

    return resultList;
}

std::wstring WriteSize( int sizeInBytes )
{
    std::wstring outString;
    std::wstring dataUnit;
    if ( sizeInBytes > ( int ) ESizeModifiers::eSM_GigaByte )
    {
        sizeInBytes /= ( int ) ESizeModifiers::eSM_GigaByte;
        dataUnit = L"Gb";
    }
    else if ( sizeInBytes > ( int ) ESizeModifiers::eSM_MegaByte )
    {
        sizeInBytes /= ( int ) ESizeModifiers::eSM_MegaByte;
        dataUnit = L"Mb";
    }
    else if ( sizeInBytes > ( int ) ESizeModifiers::eSM_KiloByte )
    {
        sizeInBytes /= ( int ) ESizeModifiers::eSM_KiloByte;
        dataUnit = L"Kb";
    }
    outString = std::to_wstring( sizeInBytes );
    outString += dataUnit;

    return outString;
}