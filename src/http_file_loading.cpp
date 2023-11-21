#include "http_file_loading.h"

std::wstring GetFileName( const wchar_t* url )
{
    URL_COMPONENTS crackedUrl;
    ZeroMemory( &crackedUrl, sizeof( URL_COMPONENTS ) );
    crackedUrl.dwStructSize = sizeof(URL_COMPONENTS);

    crackedUrl.dwSchemeLength = ( DWORD )-1;
    crackedUrl.dwHostNameLength = ( DWORD )-1;
    crackedUrl.dwUrlPathLength = ( DWORD )-1;
    crackedUrl.dwExtraInfoLength = ( DWORD )-1;

    if ( !WinHttpCrackUrl( url, ( DWORD )wcslen( url ), 0, &crackedUrl ) )
    {
        return std::wstring();
    }
    else
    {
        wchar_t* path = new wchar_t[crackedUrl.dwUrlPathLength + 1];
        memcpy(path, crackedUrl.lpszUrlPath, crackedUrl.dwUrlPathLength * sizeof(wchar_t));
        path[crackedUrl.dwUrlPathLength] = '\0';
        bool ext = false;

        int strCounter = 0;
        for (int i = crackedUrl.dwUrlPathLength - 1; i >= 0; --i)
        {
            if (crackedUrl.lpszUrlPath[i] == '/' || crackedUrl.lpszUrlPath[i] == '\0')
            {
                break;
            }
            else if( crackedUrl.lpszUrlPath[i] == '.' && i > 0 )
            {
                ext = true;
                ++strCounter;
            }
            else
            {
                ++strCounter;
            }
        }

        if( strCounter == 0 || !ext )
        {
            return std::wstring();
        }


        wchar_t* fileName = new wchar_t[( ( size_t )crackedUrl.dwUrlPathLength ) + 1];
        memcpy( fileName,
            &( crackedUrl.lpszUrlPath[crackedUrl.dwUrlPathLength - strCounter] ),
            strCounter * sizeof( wchar_t ) );
        fileName[strCounter] = '\0';
        return std::wstring( fileName );
    }
}

DWORD GetHttpFileSize( const wchar_t* canonicalUrl )
{
    HINTERNET hSession = NULL;
    HINTERNET hConnection = NULL;
    HINTERNET hRequest = NULL;
    BOOL bResult = FALSE;

    URL_COMPONENTS crackedUrl;
    ZeroMemory( &crackedUrl, sizeof( URL_COMPONENTS ) );
    crackedUrl.dwStructSize = sizeof(URL_COMPONENTS);

    crackedUrl.dwSchemeLength = ( DWORD )-1;
    crackedUrl.dwHostNameLength = ( DWORD )-1;
    crackedUrl.dwUrlPathLength = ( DWORD )-1;
    crackedUrl.dwExtraInfoLength = ( DWORD )-1;

    if ( !WinHttpCrackUrl( canonicalUrl, ( DWORD )wcslen( canonicalUrl ), 0, &crackedUrl ) )
    {
        return -1;
    }
    wchar_t* hostName = new wchar_t[crackedUrl.dwHostNameLength + 1];
    wchar_t* path = new wchar_t[crackedUrl.dwUrlPathLength + 1];
    memcpy( hostName, crackedUrl.lpszHostName, crackedUrl.dwHostNameLength * sizeof( wchar_t ) );
    memcpy( path, crackedUrl.lpszUrlPath, crackedUrl.dwUrlPathLength * sizeof( wchar_t ) );
    hostName[crackedUrl.dwHostNameLength] = '\0';
    path[crackedUrl.dwUrlPathLength] = '\0';

    hSession = WinHttpOpen( L"MoziIIa", 
                                WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, 
                                WINHTTP_NO_PROXY_NAME, 
                                WINHTTP_NO_PROXY_BYPASS, 
                                0 );
    if( !hSession )
    { 
        return -1;
    }

    hConnection = WinHttpConnect( hSession, hostName, INTERNET_DEFAULT_HTTPS_PORT, 0 );
    if( !hConnection )
    {
        WinHttpCloseHandle( hSession );
        return -1;
    }

    const wchar_t* types[2] = { L"*/*", NULL };

    hRequest = WinHttpOpenRequest( hConnection, 
                                    L"GET", 
                                    path,
                                    NULL, 
                                    WINHTTP_NO_REFERER, 
                                    types,
                                    WINHTTP_FLAG_SECURE );

    if( !hRequest )
    {
        WinHttpCloseHandle( hSession );
        WinHttpCloseHandle( hConnection );
        return -1;
    }

    bResult = WinHttpSendRequest( hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0 );

    if( !bResult )
    {
        WinHttpCloseHandle( hSession );
        WinHttpCloseHandle( hConnection );
        WinHttpCloseHandle( hRequest );
        return -1;
    }

    bResult = WinHttpReceiveResponse( hRequest, NULL );

    if( !bResult )
    {
        WinHttpCloseHandle( hSession );
        WinHttpCloseHandle( hConnection );
        WinHttpCloseHandle( hRequest );
        return -1;
    }

    wchar_t* size = new wchar_t[128];
    DWORD sizeLength = 128 * sizeof( wchar_t );
    if( !WinHttpQueryHeaders( hRequest, 
                            WINHTTP_QUERY_CONTENT_LENGTH, 
                            WINHTTP_HEADER_NAME_BY_INDEX, 
                            size, 
                            &sizeLength, 
                            WINHTTP_NO_HEADER_INDEX ) )
    {
        return -1;
    }

    size[sizeLength + 1] = '\0';
    LONG dwTotalSize = 0;
    for( int i = 0; i < sizeLength / sizeof( wchar_t ); ++i )
    {
        dwTotalSize *= 10;
        dwTotalSize += size[i] - '0';
    }
    WinHttpCloseHandle( hSession );
    WinHttpCloseHandle( hConnection );
    WinHttpCloseHandle( hRequest );
    delete[] hostName;
    delete[] path;
    delete[] size;
    return dwTotalSize;
}

EHttpLoadError LoadFile( const wchar_t* webSite, 
                const wchar_t* path, 
                const wchar_t* fileToWrite, 
                std::atomic<bool>* isTerminate, 
                HWND hWnd )
{
    HINTERNET hSession = NULL;
    HINTERNET hConnection = NULL;
    HINTERNET hRequest = NULL;
    BOOL bResult = TRUE;
    int err = 0;
    LPSTR outBuffer;
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;

    hSession = WinHttpOpen( L"MoziIIa", 
                                WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, 
                                WINHTTP_NO_PROXY_NAME, 
                                WINHTTP_NO_PROXY_BYPASS, 
                                0 );
    err = GetLastError();
    if( !hSession && err )
    { 
        return GetLastHttpError( err );
    }

    hConnection = WinHttpConnect( hSession, webSite, INTERNET_DEFAULT_HTTPS_PORT, 0 );
    err = GetLastError();
    if( !hConnection && err )
    {
        WinHttpCloseHandle( hSession );
        return GetLastHttpError( err );
    }

    const wchar_t* types[2] = { L"*/*", NULL };

    hRequest = WinHttpOpenRequest( hConnection, 
                                    L"GET", 
                                    path,
                                    NULL, 
                                    WINHTTP_NO_REFERER, 
                                    types,
                                    WINHTTP_FLAG_SECURE );
    err = GetLastError();

    if( !hRequest && err )
    {
        WinHttpCloseHandle( hSession );
        WinHttpCloseHandle( hConnection );
        return GetLastHttpError( err );
    }

    bResult = WinHttpSendRequest( hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0 );
    err = GetLastError();

    if( !bResult && err )
    {
        WinHttpCloseHandle( hSession );
        WinHttpCloseHandle( hConnection );
        WinHttpCloseHandle( hRequest );
        return GetLastHttpError( err );
    }

    bResult = WinHttpReceiveResponse( hRequest, NULL );
    err = GetLastError();

    if( !bResult && err )
    {
        WinHttpCloseHandle( hSession );
        WinHttpCloseHandle( hConnection );
        WinHttpCloseHandle( hRequest );
        return GetLastHttpError( err );
    }

    HANDLE file = CreateFile( fileToWrite, 
                                GENERIC_WRITE, 
                                0, 
                                NULL, 
                                CREATE_ALWAYS, 
                                FILE_ATTRIBUTE_NORMAL, 
                                NULL );
    err = GetLastError();

    if( file == INVALID_HANDLE_VALUE && err )
    {
        WinHttpCloseHandle( hSession );
        WinHttpCloseHandle( hConnection );
        WinHttpCloseHandle( hRequest );
        return GetLastHttpError( err );
    }

    do
    {
        dwSize = 0;

        bResult = WinHttpQueryDataAvailable( hRequest, &dwSize );
        err = GetLastError();
        if( !bResult && err )
        {
            WinHttpCloseHandle( hSession );
            WinHttpCloseHandle( hConnection );
            WinHttpCloseHandle( hRequest );
            CloseHandle( file );
            return GetLastHttpError( err );
        }

        outBuffer = new char[ dwSize + 1 ];
        if( !outBuffer )
        {
            return GetLastHttpError();
        }

        ZeroMemory( outBuffer, dwSize + 1 );

        bResult = WinHttpReadData( hRequest, ( LPVOID ) outBuffer, dwSize, &dwDownloaded );
        err = GetLastError();
        if( !bResult && err )
        {
            WinHttpCloseHandle( hSession );
            WinHttpCloseHandle( hConnection );
            WinHttpCloseHandle( hRequest );
            CloseHandle( file );
            return GetLastHttpError( err );
        }
        SendMessage( hWnd, USR_LOAD_FROM_URL_LOADED, ( WPARAM ) dwDownloaded, 0 );

        WriteFile( file, outBuffer, dwDownloaded, NULL, NULL );
        delete[] outBuffer;

    } while( dwSize > 0 && !isTerminate->load() );

    if( dwSize > 0 && isTerminate->load() )
    {
        CloseHandle(file);
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnection);
        WinHttpCloseHandle(hSession);

        return EHttpLoadError::eHLE_Terminated;
    }
    CloseHandle( file );
    WinHttpCloseHandle( hRequest );
    WinHttpCloseHandle( hConnection );
    WinHttpCloseHandle( hSession );

    return EHttpLoadError::eHLE_Success;
}

EHttpLoadError LoadFileFromUrl( const wchar_t* canonicalUrl, std::atomic<bool>* isTerminate, HWND hWnd )
{
    URL_COMPONENTS crackedUrl;
    ZeroMemory( &crackedUrl, sizeof( URL_COMPONENTS ) );
    crackedUrl.dwStructSize = sizeof( URL_COMPONENTS );

    crackedUrl.dwSchemeLength = ( DWORD )-1;
    crackedUrl.dwHostNameLength = ( DWORD )-1;
    crackedUrl.dwUrlPathLength = ( DWORD )-1;
    crackedUrl.dwExtraInfoLength = ( DWORD )-1;

    if( !WinHttpCrackUrl( canonicalUrl, ( DWORD ) wcslen( canonicalUrl ), 0, &crackedUrl ) )
    {
        return EHttpLoadError::eHLE_UrlCrackFailure;
    }
    PWSTR hostName = new wchar_t[( ( size_t ) crackedUrl.dwHostNameLength ) + 1];
    PWSTR path = new wchar_t[( ( size_t ) crackedUrl.dwUrlPathLength ) + 1];

    std::wstring strFileName = ( PWSTR ) GetFileName( canonicalUrl ).c_str();
    
    memcpy( hostName, crackedUrl.lpszHostName, crackedUrl.dwHostNameLength * sizeof( wchar_t ) );
    memcpy( path, crackedUrl.lpszUrlPath, crackedUrl.dwUrlPathLength * sizeof( wchar_t ) );
    if( strFileName.length() == 0 )
    {
        return EHttpLoadError::eHLE_UrlWithoutFileIdentifier;
    }
    hostName[crackedUrl.dwHostNameLength] = '\0';
    path[crackedUrl.dwUrlPathLength] = '\0';

    EHttpLoadError retValue = LoadFile( hostName, path, strFileName.c_str(), isTerminate, hWnd );
    if( retValue != EHttpLoadError::eHLE_Success )
    {
        delete[] hostName;
        delete[] path;
        return retValue;
    }
    else
    {
        delete[] hostName;
        delete[] path;
        return EHttpLoadError::eHLE_Success;
    }
}

std::wstring GetErrorString( EHttpLoadError error )
{
    switch( error )
    {
       case EHttpLoadError::eHLE_Success:
       {
           return std::wstring( L"Success" );
       }

       case EHttpLoadError::eHLE_UrlCrackFailure:
       {
           return std::wstring( L"Cannot parse link." );
       }

       case EHttpLoadError::eHLE_UrlWithoutFileIdentifier:
       {
           return std::wstring( L"Cannot extract file name from link." );
       }

       case EHttpLoadError::eHLE_Terminated:
       {
           return std::wstring( L"Load terminated" );
       }

       case EHttpLoadError::eHLE_InvalidHandle:
       {
           return std::wstring( L"Invalid handle" );
       }

       case EHttpLoadError::eHLE_CannotConnect:
       {
           return std::wstring( L"Cannot connecto to the resource." );
       }

       case EHttpLoadError::eHLE_ConnectionError:
       {
           return std::wstring( L"Connection error. Connection has been terminated" );
       }

       case EHttpLoadError::eHLE_HeaderNotFound:
       {
           return std::wstring( L"Cannot request data from header." );
       }

       case EHttpLoadError::eHLE_InvalidUrl:
       {
           return std::wstring( L"Invalid URL." );
       }

       case EHttpLoadError::eHLE_PathNotFound:
       {
           return std::wstring( L"Cannot create file." );
       }

       case EHttpLoadError::eHLE_SecureFailure:
       {
           return std::wstring( L"Secure failure." );
       }

       default:
       {
           return std::wstring( L"Uninitialized error." );
       }

    }
}

EHttpLoadError GetLastHttpError( int err )
{
    if( err == -1 )
    {
        int err = GetLastError();
    }
    switch( err )
    {
       case 0:
       {
           return EHttpLoadError::eHLE_Success;
       }

       case ERROR_INVALID_HANDLE:
       {
           return EHttpLoadError::eHLE_InvalidHandle;
       }

       case ERROR_WINHTTP_CANNOT_CONNECT:
       {
           return EHttpLoadError::eHLE_CannotConnect;
       }

       case ERROR_WINHTTP_CONNECTION_ERROR:
       {
           return EHttpLoadError::eHLE_ConnectionError;
       }

       case ERROR_WINHTTP_HEADER_NOT_FOUND:
       {
           return EHttpLoadError::eHLE_HeaderNotFound;
       }

       case ERROR_WINHTTP_INVALID_URL:
       {
           return EHttpLoadError::eHLE_InvalidUrl;
       }

       case ERROR_PATH_NOT_FOUND:
       {
           return EHttpLoadError::eHLE_PathNotFound;
       }

       case ERROR_WINHTTP_SECURE_FAILURE:
       {
           return EHttpLoadError::eHLE_SecureFailure;
       }

       default:
       {
           return EHttpLoadError::eHLE_UninitializedError;
       }

    }
}