#pragma once
#include <string>
#include <atomic>
#include <windows.h>
#include <CommCtrl.h>
#include <process.h>

#include "utils.h"
#include "base_proc_translator.h"
#include "http_file_loading.h"
#include "loger.h"

class FileDownloader : public BaseProcTranslator< FileDownloader >
{
public:
    FileDownloader();
    PCWSTR getClassName() const;
    void setUrl( std::wstring url );
    HANDLE getThreadHandle();
    HWND getProgressBar();
    std::atomic<bool>& getTerminator();
    std::wstring& getUrl();
    LRESULT handleMessage( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
protected:
    void log( EHttpLoadError returnCode );
private:
    std::wstring m_url;
    std::atomic<bool> m_isTerminate = false;
    DWORD m_loadedData = 0;
    DWORD m_fileSize = 0;
    HWND m_progressBar = NULL;
    HANDLE m_loadThread = NULL;
    bool isFailed = false;
};

