#pragma once

#include <atomic>
#include <windows.h>
#include <winhttp.h>
#include <string>

#define USR_LOAD_FROM_URL_LOADED 0x8AAA
#define USR_LOAD_FROM_URL_FINISHED 0x8AAB
#define LOAD_FROM_URL_CONTROL_ID 115

enum class EHttpLoadError
{
    eHLE_Success = 0,
    eHLE_UninitializedError = 1,
    eHLE_UrlCrackFailure = 2,
    eHLE_PathNotFound = 3,
    eHLE_UrlWithoutFileIdentifier = 4,
    eHLE_Terminated = 5,
    eHLE_InvalidHandle = 6,
    eHLE_InvalidUrl = 12005,
    eHLE_CannotConnect = 12029,
    eHLE_ConnectionError = 12030,
    eHLE_HeaderNotFound = 12150,
    eHLE_SecureFailure = 12175
};


std::wstring GetFileName( const wchar_t* url );
DWORD GetHttpFileSize( const wchar_t* canonicalUrl );
EHttpLoadError LoadFile( const wchar_t* webSite, 
                const wchar_t* path, 
                const wchar_t* fileToWrite, 
                std::atomic<bool>* isTerminate,
                HWND hWnd );
EHttpLoadError LoadFileFromUrl( const wchar_t* canonicalUrl, 
                        std::atomic<bool>* isTerminate,
                        HWND hWnd );
std::wstring GetErrorString( EHttpLoadError error );
EHttpLoadError GetLastHttpError( int err = -1 );
