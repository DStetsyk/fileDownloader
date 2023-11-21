#pragma once
#include <windows.h>
#include <type_traits>

template< class DERIVED_CLASS >
class BaseProcTranslator
{
public:
    HWND getHwnd() const
    {
        return m_hWnd;
    }

    BOOL create( PCWSTR windowName, 
                    DWORD windowStyle, 
                    DWORD windowExStyle, 
                    int x, 
                    int y, 
                    int width, 
                    int height, 
                    HWND parentHwnd, 
                    HMENU hMenu )
    {
        WNDCLASSEX wc = {};
        wc.cbSize = sizeof( WNDCLASSEX );
        wc.lpszClassName = getClassName();
        wc.hInstance = GetModuleHandle( NULL );
        wc.lpfnWndProc = windowProc;

        RegisterClassEx(&wc);

        m_hWnd = CreateWindowEx( windowExStyle,
                                    getClassName(),
                                    windowName,
                                    windowStyle,
                                    x,
                                    y,
                                    width,
                                    height,
                                    parentHwnd,
                                    hMenu,
                                    GetModuleHandle( NULL ),
                                    this );

        ShowWindow( m_hWnd, SW_SHOW );

        return (m_hWnd) ? TRUE : FALSE;
    }
    static LRESULT CALLBACK windowProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
    {
        BaseProcTranslator* pThis;
        if( msg == WM_NCCREATE )
        {
            pThis = ( BaseProcTranslator* ) ( ( CREATESTRUCT* )lParam )->lpCreateParams;
            SetWindowLongPtr( hWnd, GWLP_USERDATA, ( LONG_PTR ) pThis );

            pThis->m_hWnd = hWnd;
        }
        else
        {
            pThis = ( BaseProcTranslator* ) GetWindowLongPtr( hWnd, GWLP_USERDATA );
        }

        if( pThis )
        {
            return pThis->handleMessage( hWnd, msg, wParam, lParam );
        }
        else
        {
            return DefWindowProc( hWnd, msg, wParam, lParam );
        }
    }
protected:
    HWND m_hWnd = NULL;

    virtual PCWSTR getClassName() const = 0;
    virtual LRESULT handleMessage( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ) = 0;
};

