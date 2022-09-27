#include <windows.h>
#include <Windowsx.h>
#include <d2d1.h>
#include <iostream>
#include <string>

#include <list>
#include <memory>

using namespace std;

#pragma comment(lib, "d2d1")

//INCLUDE HEADER FILES
#include "basewin.h"
#include "resource.h"

//INCLUDE CUSTOM CPP FILES
#include "convexHullAlgo.cpp"

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

class DPIScale
{
    static float scaleX;
    static float scaleY;

public:
    static void Initialize(ID2D1Factory *pFactory)
    {
        FLOAT dpiX, dpiY;
        pFactory->GetDesktopDpi(&dpiX, &dpiY);
        scaleX = dpiX/96.0f;
        scaleY = dpiY/96.0f;
    }

    template <typename T>
    static float PixelsToDipsX(T x)
    {
        return static_cast<float>(x) / scaleX;
    }

    template <typename T>
    static float PixelsToDipsY(T y)
    {
        return static_cast<float>(y) / scaleY;
    }
};

float DPIScale::scaleX = 1.0f;
float DPIScale::scaleY = 1.0f;

struct MyEllipse
{
    D2D1_ELLIPSE    ellipse;
    D2D1_COLOR_F    color;

    void Draw(ID2D1RenderTarget *pRT, ID2D1SolidColorBrush *pBrush)
    {
        pBrush->SetColor(color);
        pRT->FillEllipse(ellipse, pBrush);
        pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
        pRT->DrawEllipse(ellipse, pBrush, 1.0f);
    }

    BOOL HitTest(float x, float y)
    {
        const float a = ellipse.radiusX;
        const float b = ellipse.radiusY;
        const float x1 = x - ellipse.point.x;
        const float y1 = y - ellipse.point.y;
        const float d = ((x1 * x1) / (a * a)) + ((y1 * y1) / (b * b));
        return d <= 1.0f;
    }
};

D2D1::ColorF::Enum colors[] = { D2D1::ColorF::Yellow, D2D1::ColorF::Salmon, D2D1::ColorF::LimeGreen };

//INHERIT FROM BaseWindow CLASS
class MainWindow : public BaseWindow<MainWindow>
{
    enum Mode
    {
        DrawMode,
        SelectMode,
        DragMode
    };

    enum Algorithm
    {
        MinkowskiDiff,
        MinkowskiSum,
        Quickhull,
        PointConvexHull,
        Gjk
    };

    HCURSOR                 hCursor;

    ID2D1Factory            *pFactory;
    ID2D1HwndRenderTarget   *pRenderTarget;
    ID2D1SolidColorBrush    *pBrush;
    D2D1_POINT_2F           ptMouse;
    D2D1_ELLIPSE            myPoint;

    Mode                    mode;
    Algorithm               algo; //used for determining which algorithm is being shown at the moment
    size_t                  nextColor;

    list<shared_ptr<MyEllipse>>             ellipses;
    list<shared_ptr<MyEllipse>>::iterator   selection;
    vector<D2D1_ELLIPSE*>                      myPoints;

     
    shared_ptr<MyEllipse> Selection() 
    { 
        if (selection == ellipses.end()) 
        { 
            return nullptr;
        }
        else
        {
            return (*selection);
        }
    }

    void    ClearSelection() { selection = ellipses.end(); }
    HRESULT InsertEllipse(float x, float y);

    BOOL    HitTest(float x, float y);
    void    SetMode(Mode m);
    void    MoveSelection(float x, float y);
    HRESULT CreateGraphicsResources();
    void    DiscardGraphicsResources();
    void    OnPaint();
    void    Resize();
    void    OnLButtonDown(int pixelX, int pixelY, DWORD flags);
    void    OnLButtonUp();
    void    OnMouseMove(int pixelX, int pixelY, DWORD flags);
    void    OnKeyDown(UINT vkey);


public:

    //MainWindow CONSTRUCTOR WITH INTIIALIZER LIST
    MainWindow() : pFactory(NULL), pRenderTarget(NULL), pBrush(NULL), 
        ptMouse(D2D1::Point2F()), nextColor(0), selection(ellipses.end())
    {
    }

    //ButtonWindow CONSTRUCTOR WITH INITIALIZER LIST
    /*ButtonWindow() : pFactory(NULL), pRenderTarget(NULL), pBrush(NULL),
        ptMouse(D2D1::Point2F()), nextColor(0), selection(ellipses.end())
    {
    }*/

    PCWSTR  ClassName() const { return L"Circle Window Class"; }
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

//RESOLUTION OPERATOR USED HERE TO DEFINE FUNCTION OUTSIDE OF CLASS
HRESULT MainWindow::CreateGraphicsResources()
{
    HRESULT hr = S_OK;
    if (pRenderTarget == NULL)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        hr = pFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(m_hwnd, size),
            &pRenderTarget);

        if (SUCCEEDED(hr))
        {
            const D2D1_COLOR_F color = D2D1::ColorF(1.0f, 1.0f, 0);
            hr = pRenderTarget->CreateSolidColorBrush(color, &pBrush);
        }
    }
    return hr;
}

void MainWindow::DiscardGraphicsResources()
{
    SafeRelease(&pRenderTarget);
    SafeRelease(&pBrush);
}

int dot_product(int vector_a[], int vector_b[]) {
    int product = 0;
    for (int i = 0; i < 3; i++)
        product = product + vector_a[i] * vector_b[i];
    return product;
}


int PointFarthestFromEdge(int point_a[], int point_b[], int** points, int numPoints) { //might need to convert points[][] to pointer since we don't know the size

    int vector_edge[2] = { point_b[0] - point_a[0], point_b[1] - point_a[1] };
    int vector_perpendicularEdge[2] = { -vector_edge[1], vector_edge[0]};

    int indexOfFarthestSoFar = -1;
    float maxVal = -FLT_MAX, rightMostVal = -FLT_MAX;

    for (int i = 0; i < numPoints; i++) {
        int point[2] = { **(points + i), *(*points + 1) }; //access row and column
        int d = dot_product(point, vector_perpendicularEdge);
        int r = dot_product(point, vector_edge);
        if (d > maxVal || (d == maxVal && r > rightMostVal)) {
            indexOfFarthestSoFar = i;
            maxVal = (float)d;
            rightMostVal = (float)r;
        }
    }

    return **(points + indexOfFarthestSoFar);
}

//ALL PAINTING OCCURS THROUGH THIS FUNCTION
void MainWindow::OnPaint()
{
    HRESULT hr = CreateGraphicsResources();
    if (SUCCEEDED(hr))
    {
        if (algo == PointConvexHull) {
            HDC hdc;
            SetWindowTextW(m_hwnd, L"PointConvexHull => ONPAINT");

            PAINTSTRUCT ps;
            hdc = BeginPaint(m_hwnd, &ps);
            TextOut(hdc, 500, 200, L"Hello, Windows!", 15);

            pRenderTarget->BeginDraw();

            pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));

            pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Red));
            
            vector<Point> points;

            points.push_back({ 0,3 });
            points.push_back({ 2,2 });
            points.push_back({ 1,1 });
            points.push_back({ 2,1 });
            points.push_back({ 3,0 });
            points.push_back({ 0,0 });
            points.push_back({ 3,3 });
            points.push_back({ 4,4 });
            points.push_back({ -2,-8 });
            points.push_back({ 10, 2 });
            points.push_back({ 7, 10 });

            vector<Point> CH = getConvexHullPoints(points); //RANDOMLY GENERATE POINTS (HARDCODED FOR NOW)

            //D2D1_SIZE_F size = pRenderTarget->GetSize();

            //DRAW THE CONVEX HULL LINES
            D2D1_POINT_2F* p1 = new D2D1_POINT_2F();
            p1->x = 600 + (CH.at(0).x * 25);
            p1->y = 300 + (CH.at(0).y * 25);

            D2D1_POINT_2F* p2 = new D2D1_POINT_2F();
            p2->x = 600 + (CH.at(1).x * 25);
            p2->y = 300 + (CH.at(1).y * 25);
            
            pRenderTarget->DrawLine(*p1, *p2, pBrush, 1);

            D2D1_POINT_2F* p3 = new D2D1_POINT_2F();
            p3->x = 600 + (CH.at(2).x * 25);
            p3->y = 300 + (CH.at(2).y * 25);

            pRenderTarget->DrawLine(*p2, *p3, pBrush, 1);

            D2D1_POINT_2F* p4 = new D2D1_POINT_2F();
            p4->x = 600 + (CH.at(3).x * 25);
            p4->y = 300 + (CH.at(3).y * 25);

            pRenderTarget->DrawLine(*p3, *p4, pBrush, 1);
            pRenderTarget->DrawLine(*p4, *p1, pBrush, 1);

            //ADDS ALL POINTS INTO myPoints VECTOR
            for (int point = 0; point < points.size(); point++) {
                myPoint.point.x = (float)(600 + (points.at(point).x * 25));
                myPoint.point.y = (float)(300 + (points.at(point).y * 25));
                myPoint = D2D1::Ellipse(D2D1::Point2F(myPoint.point.x, myPoint.point.y), 5, 5);
                myPoints.push_back(new D2D1_ELLIPSE(myPoint));
            }

            //SHOULD PAINT ALL CONVEXHULL POINTS BLUE INSTEAD OF RED (ONLY WORKS FOR SOME)
            bool isCHPt = false;
            for (auto& point : myPoints) // access by reference to avoid copying
            {
                for (auto& convexHullPoint : CH) // access by reference to avoid copying
                {
                    if ((float)(600 + (convexHullPoint.x * 25)) == point->point.x && (float)(300 + (convexHullPoint.y * 25)) == point->point.y)
                    {
                        pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Blue));
                        pRenderTarget->FillEllipse(*point, pBrush);
                        pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Red));
                        isCHPt = true;
                        break;
                    }
                }
                if (!isCHPt) {
                    pRenderTarget->FillEllipse(*point, pBrush);
                }
                isCHPt = false;
            }
            

            /*
            bool isCHPt = false;
            for (int i = 0; i < myPoints.size(); i++) // access by reference to avoid copying
            {
                for (int j = 0; j < CH.size(); j++) // access by reference to avoid copying
                {
                    if (600 + (CH.at(j).x * 25) == (myPoints.at(i))->point.x && 300 + (CH.at(j).y * 25) == (myPoints.at(i))->point.y)
                    {
                        pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Blue));
                        pRenderTarget->FillEllipse(myPoints.at(i), pBrush);
                        pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Red));
                        isCHPt = true;
                        break;
                    }
                }
                if (!isCHPt) {
                    pRenderTarget->FillEllipse(myPoints.at(i), pBrush);
                }
                isCHPt = false;
            }*/

            //InvalidateRect(m_hwnd, NULL, FALSE); //THIS WAS CAUSING PROBLEMS WITH RENDERING CH POINTS

            hr = pRenderTarget->EndDraw();
            if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
            {
                DiscardGraphicsResources();
            }
            EndPaint(m_hwnd, &ps);
        }
        else {
            //INITIAL PAINT CODE
            PAINTSTRUCT ps;
            BeginPaint(m_hwnd, &ps);

            pRenderTarget->BeginDraw();

            pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Blue));

            for (auto i = ellipses.begin(); i != ellipses.end(); ++i)
            {
                (*i)->Draw(pRenderTarget, pBrush);
            }

            if (Selection())
            {
                pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Red));
                pRenderTarget->DrawEllipse(Selection()->ellipse, pBrush, 2.0f);
            }

            hr = pRenderTarget->EndDraw();
            if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
            {
                DiscardGraphicsResources();
            }
            EndPaint(m_hwnd, &ps);
        }
    }
}

void MainWindow::Resize()
{
    if (pRenderTarget != NULL)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        pRenderTarget->Resize(size);

        InvalidateRect(m_hwnd, NULL, FALSE);
    }
}

void MainWindow::OnLButtonDown(int pixelX, int pixelY, DWORD flags)
{
    
    const float dipX = DPIScale::PixelsToDipsX(pixelX);
    const float dipY = DPIScale::PixelsToDipsY(pixelY);

    /*
    if (mode == DrawMode)
    {
        POINT pt = { pixelX, pixelY };

        if (DragDetect(m_hwnd, pt))
        {
            SetCapture(m_hwnd);
        
            // Start a new ellipse.
            InsertEllipse(dipX, dipY);
        }
    }
    else
    {
        ClearSelection();

        if (HitTest(dipX, dipY))
        {
            SetCapture(m_hwnd);

            ptMouse = Selection()->ellipse.point;
            ptMouse.x -= dipX;
            ptMouse.y -= dipY;

            SetMode(DragMode);
        }
    }*/

    if (algo == PointConvexHull) {
        if (HitTest(dipX, dipY))
        {
            SetCapture(m_hwnd);

            ptMouse = Selection()->ellipse.point;
            ptMouse.x -= dipX;
            ptMouse.y -= dipY;

            SetMode(DragMode);
        }
    }

    //InvalidateRect(m_hwnd, NULL, FALSE);
    
}

void MainWindow::OnLButtonUp()
{
    /*
    if ((mode == DrawMode) && Selection())
    {
        ClearSelection();
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
    else if (mode == DragMode)
    {
        SetMode(SelectMode);
    }
    ReleaseCapture(); */
}


void MainWindow::OnMouseMove(int pixelX, int pixelY, DWORD flags)
{
    /*
    const float dipX = DPIScale::PixelsToDipsX(pixelX);
    const float dipY = DPIScale::PixelsToDipsY(pixelY);

    if ((flags & MK_LBUTTON) && Selection())
    { 
        if (mode == DrawMode)
        {
            // Resize the ellipse.
            const float width = (dipX - ptMouse.x) / 2;
            const float height = (dipY - ptMouse.y) / 2;
            const float x1 = ptMouse.x + width;
            const float y1 = ptMouse.y + height;

            Selection()->ellipse = D2D1::Ellipse(D2D1::Point2F(x1, y1), width, height);
        }
        else if (mode == DragMode)
        {
            // Move the ellipse.
            Selection()->ellipse.point.x = dipX + ptMouse.x;
            Selection()->ellipse.point.y = dipY + ptMouse.y;
        }
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
    */
}


void MainWindow::OnKeyDown(UINT vkey)
{
    /*
    switch (vkey)
    {
    case VK_BACK:
    case VK_DELETE:
        if ((mode == SelectMode) && Selection())
        {
            ellipses.erase(selection);
            ClearSelection();
            SetMode(SelectMode);
            InvalidateRect(m_hwnd, NULL, FALSE);
        };
        break;

    case VK_LEFT:
        MoveSelection(-1, 0);
        break;

    case VK_RIGHT:
        MoveSelection(1, 0);
        break;

    case VK_UP:
        MoveSelection(0, -1);
        break;

    case VK_DOWN:
        MoveSelection(0, 1);
        break;
    }*/
}

HRESULT MainWindow::InsertEllipse(float x, float y)
{
    try
    {
        selection = ellipses.insert(
            ellipses.end(), 
            shared_ptr<MyEllipse>(new MyEllipse()));

        Selection()->ellipse.point = ptMouse = D2D1::Point2F(x, y);
        Selection()->ellipse.radiusX = Selection()->ellipse.radiusY = 2.0f; 
        Selection()->color = D2D1::ColorF( colors[nextColor] );

        nextColor = (nextColor + 1) % ARRAYSIZE(colors);
    }
    catch (std::bad_alloc)
    {
        return E_OUTOFMEMORY;
    }
    return S_OK;
}


BOOL MainWindow::HitTest(float x, float y)
{
    for (auto i = ellipses.rbegin(); i != ellipses.rend(); ++i)
    {
        if ((*i)->HitTest(x, y))
        {
            selection = (++i).base();
            return TRUE;
        }
    }
    return FALSE;
}

void MainWindow::MoveSelection(float x, float y)
{
    /*
    if ((mode == SelectMode) && Selection())
    {
        Selection()->ellipse.point.x += x;
        Selection()->ellipse.point.y += y;
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
    */
}

void MainWindow::SetMode(Mode m)
{
    mode = m;

    LPWSTR cursor;
    switch (mode)
    {
    case DrawMode:
        cursor = IDC_CROSS;
        break;

    case SelectMode:
        cursor = IDC_HAND;
        break;

    case DragMode:
        cursor = IDC_SIZEALL;
        break;
    }

    hCursor = LoadCursor(NULL, cursor);
    SetCursor(hCursor);
}

//ENTRY POINT OF THE APPLICATION
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    MainWindow win;

    if (!win.Create(L"Draw Circles", WS_OVERLAPPEDWINDOW)) //CALLS Create FUNCTION IN basewin.h AND CREATES THE MAIN WINDOW
    {
        return 0;
    }

    HACCEL hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCEL1));

    //SHOW THE MAIN WINDOW AFTER CREATION ABOVE
    ShowWindow(win.Window(), nCmdShow);
    //UpdateWindow(win.Window());

    //RUN THE MAIN MESSAGE LOOP (REMAINS IN HERE UNTIL APPLICATION IS CLOSED)
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(win.Window(), hAccel, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return 0;
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        if (FAILED(D2D1CreateFactory(
                D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory)))
        {
            return -1;  // Fail CreateWindowEx.
        }
        DPIScale::Initialize(pFactory);
        SetMode(DrawMode);
        return 0;

    case WM_DESTROY:
        DiscardGraphicsResources();
        SafeRelease(&pFactory);
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
        OnPaint();
        return 0;

    case WM_SIZE:
        Resize();
        return 0;

    case WM_LBUTTONDOWN: 
        OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (DWORD)wParam);
        return 0;

    case WM_LBUTTONUP: 
        OnLButtonUp();
        return 0;

    case WM_MOUSEMOVE: 
        OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (DWORD)wParam);
        return 0;

    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT)
        {
            SetCursor(hCursor);
            return TRUE;
        }
        break;

    case WM_KEYDOWN:
        OnKeyDown((UINT)wParam);
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case MINKOW_DIFF:
            algo = MinkowskiDiff;
            SetWindowTextW(m_hwnd, L"MD");
            break;
        case MINKOW_SUM:
            algo = MinkowskiSum;
            SetWindowTextW(m_hwnd, L"MS");
            break;
        case QUICK:
            algo = Quickhull;
            SetWindowTextW(m_hwnd, L"Q");
            break;
        case POINT_CONVEX:
            algo = PointConvexHull;
            SetWindowTextW(m_hwnd, L"PC");
            OnPaint();
            break;
        case GJK:
            algo = Gjk;
            SetWindowTextW(m_hwnd, L"GJK");
            break;

        case ID_DRAW_MODE:
            SetMode(DrawMode);
            break;

        case ID_SELECT_MODE:
            SetMode(SelectMode);
            break;

        case ID_TOGGLE_MODE:
            if (mode == DrawMode)
            {
                SetMode(SelectMode);
            }
            else
            {
                SetMode(DrawMode);
            }
            break;
        }
        return 0;
    }

    return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}
