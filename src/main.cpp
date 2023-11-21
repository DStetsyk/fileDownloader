#include <atomic>
#include <windows.h>
#include <CommCtrl.h>
#include <process.h>
#include <string>
#include <list>
#include <vector>

#include "http_file_loading.h"
#include "base_proc_translator.h"
#include "file_downloader.h"
#include "utils.h"
#include "loger.h"

const wchar_t* WINDOW_CLASS_NAME = L"DownloaderClass";

unsigned int startDownload( void* data );

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{

    InitCommonControls();
    Loger& loger = Loger::getInstance();
    loger.setLogFile( "log.txt" );

    std::list< std::wstring > cmdParams = ParseCommandLine( pCmdLine );
    std::vector< HANDLE > threads;
    
    for( std::wstring link : cmdParams )
    {
        std::wstring* url = new std::wstring( link );
        HANDLE thread = ( HANDLE ) _beginthreadex( NULL,
                                                    0,
                                                    ( _beginthreadex_proc_type ) startDownload,
                                                    url,
                                                    NULL,
                                                    NULL );
        if( !thread )
        {
            MessageBox( NULL, L"Cannot create thread", L"ERROR", MB_OK | MB_ICONERROR );
        }
        else
        {
            threads.push_back( thread );
        }
    }

    WaitForMultipleObjects( threads.size(), threads.data(), TRUE, INFINITE );

    for( HANDLE thread : threads )
    {
        CloseHandle( thread );
    }
    return 0;
}

unsigned int startDownload( void* data )
{
    std::wstring* strData = ( std::wstring* ) data;
    FileDownloader downloader;
    downloader.setUrl( *strData );
    delete strData;
    bool res = downloader.create( L"Download",
                                    WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX,
                                    NULL,
                                    CW_USEDEFAULT,
                                    CW_USEDEFAULT,
                                    500,
                                    170,
                                    NULL,
                                    NULL );

    if( !res )
    {
        MessageBox( NULL, L"Cannot create thread", L"ERROR", MB_OK | MB_ICONERROR );
        return 0;
    }

    MSG message;
    while( GetMessage( &message, NULL, 0, 0 ) )
    {
        TranslateMessage( &message );
        DispatchMessage( &message );
    }
    
    return 0;
}

