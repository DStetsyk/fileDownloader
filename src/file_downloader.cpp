#include "file_downloader.h"

#define PROGRESS_BAR_WIDTH 450
#define PROGRESS_BAR_HEIGHT 20
#define PROGRESS_BAR_TIMER_ID 1

FileDownloader::FileDownloader()
    : BaseProcTranslator()
{}

PCWSTR FileDownloader::getClassName() const 
{
    return L"FileDownloaderClassName";
}

void FileDownloader::setUrl( std::wstring url )
{
    m_url = url;
    m_fileSize = GetHttpFileSize( url.c_str() );
}

HANDLE FileDownloader::getThreadHandle()
{
    return m_loadThread;
}

HWND FileDownloader::getProgressBar()
{
    return m_progressBar;
}

std::atomic<bool>& FileDownloader::getTerminator()
{
    return m_isTerminate;
}

std::wstring& FileDownloader::getUrl()
{
    return m_url;
}

void FileDownloader::log( EHttpLoadError returnCode )
{
    std::wstring outString;

    std::wstring fileName = GetFileName( m_url.c_str() );

    if( returnCode == EHttpLoadError::eHLE_Success )
    {
        outString += L"DOWNLOADED: " + fileName + L'\n';
    }
    else
    {
        outString += L"FAILED\n";
    }
    outString += GetErrorString( returnCode ) + L'\n';
    outString += L"URL: " + m_url + L'\n';
    outString += L"SIZE: " + WriteSize( m_loadedData ) + L"\n\n";

    Loger::getInstance().log( outString );
}

LRESULT FileDownloader::handleMessage( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    static HBRUSH backgroundBrush = CreateSolidBrush( RGB( 170, 170, 170 ) );

    switch( msg )
    {
       case WM_CREATE:
       {
           RECT rt;
           GetClientRect( hWnd, &rt );
           m_progressBar = CreateWindow( PROGRESS_CLASS,
                            L"",
                            WS_CHILD | WS_VISIBLE | PBS_MARQUEE,
                            rt.right / 2 - PROGRESS_BAR_WIDTH / 2,
                            rt.bottom / 2 - PROGRESS_BAR_HEIGHT / 2,
                            PROGRESS_BAR_WIDTH,
                            PROGRESS_BAR_HEIGHT,
                            hWnd,
                            NULL,
                            ( HINSTANCE ) GetWindowLongPtr( hWnd, GWLP_HINSTANCE ),
                            NULL );


           LONG fileSize = GetHttpFileSize( m_url.c_str() );

           SendMessage( m_progressBar, PBM_SETBARCOLOR, 0, ( LPARAM ) RGB( 0, 50, 200 ) );
           SendMessage( m_progressBar, PBM_SETPOS, ( WPARAM ) 0, 0 );


           m_loadThread = ( HANDLE ) _beginthreadex( NULL,
                                        0, 
                                        ( _beginthreadex_proc_type ) [] ( void* data )
                                        {
                                            FileDownloader* downloader = ( FileDownloader* ) data;
                                            EHttpLoadError retValue = LoadFileFromUrl( ( downloader->getUrl() ).c_str(), 
                                                                                &( downloader->getTerminator() ), 
                                                                                ( HWND ) downloader-> getHwnd() );
                                            SendMessage( downloader->getHwnd(),
                                                            USR_LOAD_FROM_URL_FINISHED, 
                                                            ( WPARAM ) retValue, 
                                                            0 );
                                            downloader->getTerminator() = true;
                                            return ( unsigned int ) 0;
                                        }, 
                                        this,
                                        0,
                                        NULL );

           if( !m_loadThread )
           {
               MessageBox( hWnd, L"Can't create thread!", L"ERROR", MB_OK );
           }
           break;
       }

       case USR_LOAD_FROM_URL_FINISHED:
       {
           int returnCode = ( int ) wParam;
           if( returnCode )
           {
               isFailed = true;
               SendMessage( m_progressBar, PBM_SETBKCOLOR, 0, ( LPARAM ) RGB( 255, 0, 0 ) );
           }
           log( ( EHttpLoadError ) wParam );
           InvalidateRect( hWnd, NULL, FALSE );
           break;
       }

       case WM_PAINT:
       {
           PAINTSTRUCT ps;
           RECT rt;

           HDC hdc = BeginPaint( hWnd, &ps );
           GetClientRect( hWnd, &rt );
           HDC memDC = CreateCompatibleDC( hdc );
           HBITMAP memBMP = CreateCompatibleBitmap( hdc, rt.right, rt.bottom );
           HGDIOBJ oldBMP = SelectObject( memDC, memBMP );

           FillRect( memDC, &rt, backgroundBrush );
           double viewData = m_loadedData;

           std::wstring outString;

           std::wstring fileName = GetFileName( m_url.c_str() );
           outString += fileName.substr( 0, 20 );
           if( fileName.length() > 20 )
           {
               outString += L"... ";
           }
           else
           {
               outString += L' ';
           }

           SetBkMode( memDC, TRANSPARENT );

           if( !m_isTerminate.load() )
           {
               outString += WriteSize( m_loadedData );
               outString += L' ' + std::to_wstring( m_loadedData / ( m_fileSize / 100 ) ) + L'%';
               outString += L" downloaded.";
           }
           else if( !isFailed )
           {
               outString += L"Done.";
           }
           else
           {
               outString += L"Failed.";
           }

           TextOut( memDC, 5, 5, outString.c_str(), outString.length() );

           BitBlt( hdc, 0, 0, rt.right, rt.bottom, memDC, 0, 0, SRCCOPY );

           SelectObject( memDC, oldBMP );
           DeleteDC( memDC );
           DeleteObject( memBMP );
           EndPaint( hWnd, &ps );
           break;
       }

       case USR_LOAD_FROM_URL_LOADED:
       {
           m_loadedData += wParam;
           SendMessage( m_progressBar, PBM_SETPOS, ( WPARAM ) ( m_loadedData / ( m_fileSize / 100 ) ), 0 );
           InvalidateRect( hWnd, NULL, FALSE );
           break;
       }

       case WM_CLOSE:
       {
           if( !m_isTerminate )
           {
               int choice = MessageBox( hWnd, 
                                        L"Downloading is not finished. \nDo you want to interrupt downloading?", 
                                        L"WARNING", 
                                        MB_YESNO | MB_ICONWARNING );
               if( choice == IDYES )
               {
                   m_isTerminate.store( true );
                   WaitForSingleObject( m_loadThread, INFINITE );
               }
               else
               {
                   break;
               }
           }
       }

       case WM_DESTROY:
       {
           WaitForSingleObject( m_loadThread, INFINITE );
           PostQuitMessage( 0 );
           break;
       }

       default:
       {
           return DefWindowProc( hWnd, msg, wParam, lParam );
       }
    }

    return TRUE;
}
