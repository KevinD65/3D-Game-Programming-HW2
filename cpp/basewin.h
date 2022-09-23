#ifndef _BASEWIN_H
#define _BASEWIN_H

#define MINKOW_DIFF 0
#define MINKOW_SUM 1
#define QUICK 2
#define POINT_CONVEX 3
#define GJK 4

template <class DERIVED_TYPE> 
class BaseWindow
{
public:

    //DEFINES THE BEHAVIOR, APPEARANCE, ETC OF THE WINDOW (INVOKED THROUGH MESSAGE LOOP; ONCE PER MESSAGE)
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        DERIVED_TYPE *pThis = NULL;

        if (uMsg == WM_NCCREATE)
        {
            CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
            pThis = (DERIVED_TYPE*)pCreate->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);

            pThis->m_hwnd = hwnd;
        }
        else
        {
            pThis = (DERIVED_TYPE*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        }
        if (pThis)
        {
            return pThis->HandleMessage(uMsg, wParam, lParam);
        }
        else
        {
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
    }

    BaseWindow() : m_hwnd(NULL) { }


    //CREATES THE MAIN WINDOW AND ASSIGNS THE HANDLE TO A POINTER (RETURNS TRUE IF CREATED AND ASSIGNED)
    BOOL Create(
        PCWSTR lpWindowName,
        DWORD dwStyle,
        DWORD dwExStyle = 0,
        int x = CW_USEDEFAULT,
        int y = CW_USEDEFAULT,
        int nWidth = CW_USEDEFAULT,
        int nHeight = CW_USEDEFAULT,
        HWND hWndParent = 0,
        HMENU hMenu = 0
        )
    {
        //NEW WINDOW CLASS DATA (WNDCLASS STRUCTURE) FOR MAIN WINDOW
        WNDCLASS wc = {};

        wc.lpfnWndProc   = DERIVED_TYPE::WindowProc;
        wc.hInstance     = GetModuleHandle(NULL);
        wc.lpszClassName = ClassName();

        RegisterClass(&wc);

        m_hwnd = CreateWindowEx(
            dwExStyle, ClassName(), lpWindowName, dwStyle, x, y,
            nWidth, nHeight, hWndParent, hMenu, GetModuleHandle(NULL), this
            );

        //NEW WINDOWS FOR BUTTONS
        HWND MDhwndButton = CreateWindow(
            L"BUTTON",  // Predefined class; Unicode assumed 
            L"Minkowski Difference Demo",      // Button text 
            WS_TABSTOP | WS_VISIBLE | WS_CHILD,  // Styles 
            10,         // x position 
            10,         // y position 
            200,        // Button width
            50,        // Button height
            m_hwnd,     // Parent window
            (HMENU) MINKOW_DIFF,       // HMENU with MINKOW_DIFF
            /*(HINSTANCE)GetWindowLongPtr(m_hwnd, GWLP_HINSTANCE)*/NULL,
            NULL);      // Pointer not needed.

        HWND MShwndButton = CreateWindow(
            L"BUTTON",  // Predefined class; Unicode assumed 
            L"Minkowski Sum Demo",      // Button text 
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
            10,         // x position 
            60,         // y position 
            200,        // Button width
            50,        // Button height
            m_hwnd,     // Parent window
            (HMENU) MINKOW_SUM,       // HMENU with MINKOW_SUM
            (HINSTANCE)GetWindowLongPtr(m_hwnd, GWLP_HINSTANCE),
            NULL);      // Pointer not needed.

        HWND QHhwndButton = CreateWindow(
            L"BUTTON",  // Predefined class; Unicode assumed 
            L"Quickhull Demo",      // Button text 
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
            10,         // x position 
            110,         // y position 
            200,        // Button width
            50,        // Button height
            m_hwnd,     // Parent window
            (HMENU) QUICK,       // HMENU with QUICK
            (HINSTANCE)GetWindowLongPtr(m_hwnd, GWLP_HINSTANCE),
            NULL);      // Pointer not needed.

        HWND PQHhwndButton = CreateWindow(
            L"BUTTON",  // Predefined class; Unicode assumed 
            L"Point Convex Hull Demo",      // Button text 
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
            10,         // x position 
            160,         // y position 
            200,        // Button width
            50,        // Button height
            m_hwnd,     // Parent window
            (HMENU) POINT_CONVEX,       // HMENU with POINT_CONVEX
            (HINSTANCE)GetWindowLongPtr(m_hwnd, GWLP_HINSTANCE),
            NULL);      // Pointer not needed.

        HWND GJKhwndButton = CreateWindow(
            L"BUTTON",  // Predefined class; Unicode assumed 
            L"GJK",      // Button text 
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
            10,         // x position 
            210,         // y position 
            200,        // Button width
            50,        // Button height
            m_hwnd,     // Parent window
            (HMENU) GJK,       // HMENU with GJK
            (HINSTANCE)GetWindowLongPtr(m_hwnd, GWLP_HINSTANCE),
            NULL);      // Pointer not needed.

        return (m_hwnd ? TRUE : FALSE);
    }

    HWND Window() const { return m_hwnd; }

protected:

    virtual PCWSTR  ClassName() const = 0;
    virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0; //DECLARED OUTSIDE OF CLASS SO WHEN USED IN main.cpp, NEEDS RESOLUTION OPERATOR IN DEFINITION

    HWND m_hwnd;
};

#endif