#ifndef _BASEWIN_H
#define _BASEWIN_H

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

        //NEW WINDOW CLASS DATA (WNDCLASS STRUCTURE) FOR BUTTONS
        WNDCLASS wcb = {};

        wcb.lpfnWndProc = DERIVED_TYPE::WindowProc;
        wcb.hInstance = GetModuleHandle(NULL);
        wcb.lpszClassName = ClassName();

        RegisterClass(&wcb);

        return (m_hwnd ? TRUE : FALSE);
    }

    HWND Window() const { return m_hwnd; }

protected:

    virtual PCWSTR  ClassName() const = 0;
    virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0; //DECLARED OUTSIDE OF CLASS SO WHEN USED IN main.cpp, NEEDS RESOLUTION OPERATOR IN DEFINITION

    HWND m_hwnd;
};

#endif