#include <windows.h>
#include <Windowsx.h>
#include <d2d1.h>
#include <iostream>

#include <list>
#include <memory>
#include <vector> //for drawing convex hulls
using namespace std;

#pragma comment(lib, "d2d1")

//INCLUDE HEADER FILES
#include "basewin.h"
#include "resource.h"

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

    Mode                    mode;
    Algorithm               algo; //used for determining which algorithm is being shown at the moment
    size_t                  nextColor;

    list<shared_ptr<MyEllipse>>             ellipses;
    list<shared_ptr<MyEllipse>>::iterator   selection;
     
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

    return indexOfFarthestSoFar;
}

//ALL PAINTING OCCURS THROUGH THIS FUNCTION
void MainWindow::OnPaint()
{
    HRESULT hr = CreateGraphicsResources();
    if (SUCCEEDED(hr))
    {
        if (algo == PointConvexHull) {
            SetWindowTextW(m_hwnd, L"LMAOBOZOPPPOOPOO");

            //POINT FARTHEST FROM EDGE

            
            //implement pointfarthestfromedge in order to implement getConvexHull in order to draw convexhull to canvas
            //draw draggable circle
            //implement dragging and inside/outside detection


        }
        /*
        import AFG_ConvexHull from "../AFG_ConvexHull.js";
import AFG_Circle from "../AFG_Circle.js";
import {getRandomInt} from "../AFG_Math.js";
import {PointConvexHullIntersectionMouseMoveHandler} from "../AFG_MouseHandlers.js";

export default class PointConvexHullIntersectionDemo {
    constructor() {
    }

    resetDemo(sceneGraph,renderer) {
        this.sceneGraph = sceneGraph;
        this.renderer = renderer;
        this.renderer.renderStats = false;
        this.renderer.renderQuickHull = true;

        let mouseMoveHandler = new PointConvexHullIntersectionMouseMoveHandler();


        let margin = 20;

        var hull = new AFG_ConvexHull();

        hull.quickHull = true;

        // INITIALIZE ALL NEEDED SHAPES
        // var graph = new AFG_Graph();
        // window.afg.sceneGraph.addSceneObject(graph);

        for (var i = 0; i < 15; i++) {
            var circle = new AFG_Circle();
            circle.setMouseMoveHandler(mouseMoveHandler);
            var maxWidth = this.renderer.grid.windowWidth;
            var maxHeight = this.renderer.grid.windowHeight;
            circle.centerX = getRandomInt(0 + margin, maxWidth - margin);
            circle.centerY = getRandomInt(0 + margin, maxHeight - margin);
            circle.radius = 0;
            //window.afg.sceneGraph.addSceneObject(circle);
            hull.points.push(circle);
        }
        // PROVIDE EVENT HANDLING RESPONSES
        // THAT WILL EMPLOY INTERSECTION TESTS

        hull.setMouseMoveHandler(mouseMoveHandler);
        // PROVIDE TEXTUAL DESCRIPTIONS
        this.sceneGraph.addSceneObject(hull);


        var circle = new AFG_Circle();
        circle.setMouseMoveHandler(mouseMoveHandler);
        circle.centerX = 400;
        circle.centerY = 400;
        circle.radius = 10;
        circle.vectorProperty = null;
        this.sceneGraph.addSceneObject(circle);


    }

    generateStats(stats, eventHandler) {
        stats.numStats = 0;
    }

    
    getProperties(){
        let properties = new Array();
        properties['sceneGraph'] = this.sceneGraph;
        return properties;
    }
    -------------------------------------------------------------------------------------
    import AFG_Circle from "./AFG_Circle.js";
import AFG_Vector from "./AFG_Vector.js";
import AFG_QuickHull from "./AFG_QuickHull.js";
import { dotProductVectors } from "./AFG_Math.js";
import { AFG_ShapeType } from "./AFG_ShapeConstants.js";
import AFG_PhysicalProperties from "./AFG_PhysicalProperties.js";
import AFG_Shape from "./AFG_Shape.js";
import { setUpHandlers } from "./AFG_MouseHandlers.js";

export default class AFG_ConvexHull extends AFG_Shape {
    constructor() {
        super();
        setUpHandlers(this);
        this.shapeType = AFG_ShapeType.CONVEX_HULL;
        this.points = [];
        this.quickHull = false;
        this.strokeStyle = "white";

        this.physicalProperties = new AFG_PhysicalProperties();
    }

    //

    contains(properties, posX, posY) {
        let renderer = properties["renderer"];
        for (var i = 0; i < this.points.length; i++) {
            if (this.points[i].contains(properties, posX, posY))
                return false;
        }
        var hull = new AFG_QuickHull();
        hull.originalList = this.points;
        var outterHull = hull.getConvexHull(renderer.grid);

        var point = new AFG_Circle();
        point.centerX = renderer.grid.convertTransXToPixel(posX);
        point.centerY = renderer.grid.convertTransYToPixel(posY);

        for (var i = 1; i < outterHull.length; i++) {
            if (outterHull[i - 1] !== outterHull[i]) {
                if (!this.isPointInside(renderer.grid, outterHull[i - 1], outterHull[i], point)) {
                    //console.log("FALSE");
                    return false;
                }
            }
        }
        if (outterHull[outterHull.length - 1] !== outterHull[0]) {
            if (!this.isPointInside(renderer.grid, outterHull[outterHull.length - 1], outterHull[0], point)) {
                //console.log("FALSE");
                return false;
            }
        }
        //console.log("TRUE");
        return true;
    }

    // Tests if the point is on the right side of every side of the
    // convex hull

    isPointInside(grid, point1, point2, pointTested) {
        var lineVector = new AFG_Vector();
        lineVector.endPoints[0] = point1;
        lineVector.endPoints[1] = point2;
        var xComp = lineVector.getXComponent(grid);
        var yComp = lineVector.getYComponent(grid);
        lineVector.components.x = xComp;
        lineVector.components.y = yComp;

        var perpendicularVector = new AFG_Vector();
        perpendicularVector.components.x = lineVector.components.y;
        perpendicularVector.components.y = lineVector.components.x * -1;
        if (lineVector.components.y == 0)
            perpendicularVector.components.x = 0;

        if (lineVector.components.x == 0)
            perpendicularVector.components.y = 0;

        var vectorPointToPoint1 = new AFG_Vector();
        vectorPointToPoint1.components.x = pointTested.getDrawingX(grid.getGridXTranslation(), grid.centerWidth, grid.scalingFactor) -
            point1.getDrawingX(grid.getGridXTranslation(), grid.centerWidth, grid.scalingFactor);
        vectorPointToPoint1.components.y = (pointTested.getDrawingY(grid.getGridYTranslation(), grid.centerHeight, grid.scalingFactor) -
            point1.getDrawingY(grid.getGridYTranslation(), grid.centerHeight, grid.scalingFactor)) * -1;

        var normalizedVectorPointToPoint1 = vectorPointToPoint1.normalizeComponents();
        var normalizedPerpendicular = perpendicularVector.normalizeComponents();

        var dotProduct = dotProductVectors(normalizedPerpendicular, normalizedVectorPointToPoint1);

        if (dotProduct >= 0)
            return true;
        return false;
    }

}
-----------------------------------------------------------------------------------------------------------------------
drawQuickHull(points) {
        let processor = new AFG_QuickHull(points);
        let outLineList = processor.getConvexHull(this.grid);
        this.canvasContext.lineWidth = 2;
        for (let i = 1; i < outLineList.length; i++) {
            let pt1 = outLineList[i - 1];
            let pt2 = outLineList[i];
            this.drawLinePath(this.canvasContext, pt1.getDrawingX(this.grid.getGridXTranslation(), this.grid.centerWidth, this.grid.scalingFactor),
                pt1.getDrawingY(this.grid.getGridYTranslation(), this.grid.centerHeight, this.grid.scalingFactor),
                pt2.getDrawingX(this.grid.getGridXTranslation(), this.grid.centerWidth, this.grid.scalingFactor),
                pt2.getDrawingY(this.grid.getGridYTranslation(), this.grid.centerHeight, this.grid.scalingFactor));
        }
        this.drawLinePath(this.canvasContext, outLineList[0].getDrawingX(this.grid.getGridXTranslation(), this.grid.centerWidth, this.grid.scalingFactor),
            outLineList[0].getDrawingY(this.grid.getGridYTranslation(), this.grid.centerHeight, this.grid.scalingFactor),
            outLineList[outLineList.length - 1].getDrawingX(this.grid.getGridXTranslation(), this.grid.centerWidth, this.grid.scalingFactor),
            outLineList[outLineList.length - 1].getDrawingY(this.grid.getGridYTranslation(), this.grid.centerHeight, this.grid.scalingFactor));
    }
-------------------------------------------------------------------------------------------------------------------------
import AFG_Vector from "./AFG_Vector.js";
import { dotProductVectors, pointFarthestFromEdge } from "./AFG_Math.js";

export default class AFG_QuickHull {
    constructor(originalList) {
        this.originalList = originalList;
    }

    //Array.splice, Array.indexOf

    // @TODO PURGE INTERNAL POINTS EVERY TIME TO REDUCE COMPUTATION SPEEDS

    getConvexHull(grid) {
        var hull = [];
        var extremePoints = [];
        var leftMostPoint = this.originalList[0];
        var rightMostPoint = this.originalList[0];
        var topMostPoint = this.originalList[0];
        var bottomMostPoint = this.originalList[0];

        for (var i = 0; i < this.originalList.length; i++) {
            var pt = this.originalList[i];
            if (pt.centerX < leftMostPoint.centerX)
                leftMostPoint = pt;
            if (pt.centerX > rightMostPoint.centerX)
                rightMostPoint = pt;
            if (pt.centerY < topMostPoint.centerY)
                topMostPoint = pt;
            if (pt.centerY > bottomMostPoint.centerY)
                bottomMostPoint = pt;
        }

        hull.push(topMostPoint);
        hull.push(rightMostPoint);
        hull.push(bottomMostPoint);
        hull.push(leftMostPoint);

        extremePoints.push(topMostPoint);
        extremePoints.push(rightMostPoint);
        extremePoints.push(bottomMostPoint);
        extremePoints.push(leftMostPoint);

        //this.pointFarthestFromEdge(hull[0], hull[1], this.originalList);

        for (var i = 1; i < hull.length + 1; i++) {
            if (i != hull.length) {
                var farthestPt = pointFarthestFromEdge(grid, hull[i - 1], hull[i], this.originalList);
                if (farthestPt != undefined && hull.indexOf(farthestPt) == -1) {
                    hull.splice(i, 0, farthestPt);
                    if (i > 1)
                        i = i - 2;
                    else
                        i--;
                }
            } else {
                var farthestPt = pointFarthestFromEdge(grid, hull[hull.length - 1], hull[0], this.originalList);
                if (farthestPt != undefined && hull.indexOf(farthestPt) == -1) {
                    hull.splice(i, 0, farthestPt);
                    if (i > 1)
                        i = i - 2;
                    else
                        i--;
                }
            }
        }

        //console.log(hull);
        return hull;
    }

    purgeInternalPoints() {

    }

    isPointInside(grid, point1, point2, pointTested) {
        var lineVector = new AFG_Vector();
        lineVector.endPoints[0] = point1;
        lineVector.endPoints[1] = point2;
        var xComp = lineVector.getXComponent(grid);
        var yComp = lineVector.getYComponent(grid);
        lineVector.components.x = xComp;
        lineVector.components.y = yComp;

        var perpendicularVector = new AFG_Vector();
        perpendicularVector.components.x = lineVector.components.y;
        perpendicularVector.components.y = lineVector.components.x * -1;
        if (lineVector.components.y == 0)
            perpendicularVector.components.x = 0;

        if (lineVector.components.x == 0)
            perpendicularVector.components.y = 0;

        var vectorPointToPoint1 = new AFG_Vector();
        vectorPointToPoint1.components.x = pointTested.getDrawingX(grid.getGridXTranslation(), grid.centerWidth, grid.scalingFactor) -
            point1.getDrawingX(grid.getGridXTranslation(), grid.centerWidth, grid.scalingFactor);
        vectorPointToPoint1.components.y = (pointTested.getDrawingY(grid.getGridYTranslation(), grid.centerHeight, grid.scalingFactor) -
            point1.getDrawingY(grid.getGridYTranslation(), grid.centerHeight, grid.scalingFactor)) * -1;

        var normalizedVectorPointToPoint1 = vectorPointToPoint1.normalizeComponents();
        var normalizedPerpendicular = perpendicularVector.normalizeComponents();

        var dotProduct = dotProductVectors(normalizedPerpendicular, normalizedVectorPointToPoint1);

        if (dotProduct >= 0)
            return true;
        return false;
    }
}
------------------------------------------------------------------------------------------------------------------------------------------------
export function pointFarthestFromEdge(grid, point1, point2, pointsList) {
    var lineVector = new AFG_Vector();
    lineVector.endPoints[0] = point1;
    lineVector.endPoints[1] = point2;
    var xComp = lineVector.getXComponent(grid);
    var yComp = lineVector.getYComponent(grid);

    lineVector.components.x = xComp;
    lineVector.components.y = yComp;

    var perpendicularVector = new AFG_Vector();
    perpendicularVector.components.x = lineVector.components.y * -1;
    perpendicularVector.components.y = lineVector.components.x;

    var bestIndex = -1;

    // Note that this took 3 days to figure out, negative 0 is a thing

    if (lineVector.components.y == 0)
        perpendicularVector.components.x = 0;

    //

    var maxVal = -2;
    var rightMostValue = -2;


    for (var i = 0; i < pointsList.length; i++) {
        var vectorPointToPoint1 = new AFG_Vector();
        vectorPointToPoint1.components.x = pointsList[i].getDrawingX(grid.getGridXTranslation(), grid.centerWidth, grid.scalingFactor) -
            point1.getDrawingX(grid.getGridXTranslation(), grid.centerWidth, grid.scalingFactor);
        vectorPointToPoint1.components.y = (pointsList[i].getDrawingY(grid.getGridYTranslation(), grid.centerHeight, grid.scalingFactor) -
            point1.getDrawingY(grid.getGridYTranslation(), grid.centerHeight, grid.scalingFactor)) * -1;
        var normalizedVectorPointToPoint1 = vectorPointToPoint1.normalizeComponents();
        var normalizedPerpendicular = perpendicularVector.normalizeComponents();
        var normalizedLineVector = lineVector.normalizeComponents();
        var d = dotProductVectors(normalizedVectorPointToPoint1, normalizedPerpendicular);
        var r = dotProductVectors(normalizedVectorPointToPoint1, normalizedLineVector);
        if (d > maxVal || (d == maxVal && r > rightMostValue)) {
            bestIndex = i;
            maxVal = d;
            rightMostValue = r;
        }
    }

    return pointsList[bestIndex];
}

}*/

        PAINTSTRUCT ps;
        BeginPaint(m_hwnd, &ps);
     
        pRenderTarget->BeginDraw();

        pRenderTarget->Clear( D2D1::ColorF(D2D1::ColorF::SkyBlue) );

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
    }
    InvalidateRect(m_hwnd, NULL, FALSE);
}

void MainWindow::OnLButtonUp()
{
    if ((mode == DrawMode) && Selection())
    {
        ClearSelection();
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
    else if (mode == DragMode)
    {
        SetMode(SelectMode);
    }
    ReleaseCapture(); 
}


void MainWindow::OnMouseMove(int pixelX, int pixelY, DWORD flags)
{
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
}


void MainWindow::OnKeyDown(UINT vkey)
{
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
    }
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
    if ((mode == SelectMode) && Selection())
    {
        Selection()->ellipse.point.x += x;
        Selection()->ellipse.point.y += y;
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
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

    std::cout << "HI";

    if (!win.Create(L"Draw Circles", WS_OVERLAPPEDWINDOW)) //CALLS Create FUNCTION IN basewin.h AND CREATES THE MAIN WINDOW
    {
        return 0;
    }

    HACCEL hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCEL1));

    //SHOW THE MAIN WINDOW AFTER CREATION ABOVE
    ShowWindow(win.Window(), nCmdShow);

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
