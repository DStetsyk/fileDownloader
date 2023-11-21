#include <windows.h>
#include <list>
#include <string>

std::list<std::wstring> ParseCommandLine( PWSTR pCmdLine );
std::wstring WriteSize( int sizeInBytes );