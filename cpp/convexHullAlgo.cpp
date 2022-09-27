// Online C++ compiler to run C++ program online
#include <iostream>
#include<stdio.h>
#include <vector>
#include <algorithm>

using namespace std;

bool upper = true;

struct Point {
    int x, y;

    //USED FOR VALUE COMPARISON WHEN SORTING POINTS FROM LEFT TO RIGHT
    bool operator<(Point point)
    {
        if (!upper) { //FOR FORMING LOWER HALF OF THE HULL
            if (x > point.x) {
                return true;
            }
            else if (x == point.x) {
                if (y < point.y) {
                    return true;
                }
                else {
                    return false;
                }
            }
        }
        else { //FOR FORMING UPPER HALF OF THE HULL
            if (x < point.x) {
                return true;
            }
            else if (x == point.x) {
                if (y > point.y) {
                    return true;
                }
                else {
                    return false;
                }
            }
        }
        return false;
    }

    //USED FOR CHECKING IF TWO POINT VALUES ARE EQUAL
    bool operator==(Point point) {
        if (x == point.x && y == point.y) {
            return true;
        }
        return false;
    }
};

//DETERMINES THE POSITION OF A POINT RELATIVE TO AN EDGE (-1: LEFT, 0: COLLINEAR, 1: RIGHT)
int relativeToLine(Point linePoint1, Point linePoint2, Point checkAgainst) {
    if (((linePoint2.x - linePoint1.x) * (checkAgainst.y - linePoint1.y)) - ((linePoint2.y - linePoint1.y) * (checkAgainst.x - linePoint1.x)) > 0) {
        return -1; //LEFT OF EDGE
    }
    else if (((linePoint2.x - linePoint1.x) * (checkAgainst.y - linePoint1.y)) - ((linePoint2.y - linePoint1.y) * (checkAgainst.x - linePoint1.x)) < 0) {
        return 1; //RIGHT OF EDGE
    }
    else {
        return 0; //COLLINEAR
    }
}

std::vector<Point> getConvexHullPoints(std::vector<Point> points) {
    std::vector<Point> convexHull; //CREATE VECTOR FOR HOLDING POINTS OF FINAL CONVEXHULL
    std::vector<Point> upperHull; //VECTOR WHICH HOLDS THE POINTS OF THE UPPERHULL
    std::vector<Point> lowerHull; //VECTOR WHICH HOLDS THE POINTS OF THE LOWERHULL

    bool hullInError = false;

    //cout << "HI";

    //SORT POINTS FROM LEFT TO RIGHT
    std::sort(points.begin(), points.end());


    for (int i = 0; i < points.size(); i++)
        cout << "(" << points[i].x << ", "
        << points[i].y << ")" << endl;
    cout << "-----------------------------------------------------------\n";

    //AT LEAST 3 POINTS TO FORM CONVEX HULL
    if (points.size() > 3) {

        //PUSH THE FIRST TWO LEFT MOST POINTS TO BEGIN
        upperHull.push_back(points.at(0));
        upperHull.push_back(points.at(1));
        //cout << "Size: " <<upperHull.size();

        for (int i = 0; i < upperHull.size(); i++)
            cout << "(" << upperHull[i].x << ", "
            << upperHull[i].y << ")" << endl;
        cout << "-----------------------------------------------------------\n";

        //FORM THE UPPER PORTION OF THE HULL
        for (size_t p = 2; p < points.size(); p++) {
            if (relativeToLine(upperHull[upperHull.size() - 2], upperHull[upperHull.size() - 1], points.at(p)) == -1) { //IF POINT IS TO THE LEFT OF THE EDGE
                cout << "ERROR IN HULL\n";
                hullInError = true;
                int pointBeingChecked = upperHull.size() - 1; //INITIALIZED TO THE LAST INDEX IN convexHull
                while (hullInError && upperHull.size() > 1) { //CONTINUE REMOVING POINTS FROM convexHull UNTIL THE HULL IS NO LONGER IN ERROR
                    if (relativeToLine(upperHull[upperHull.size() - 2], upperHull[upperHull.size() - 1], points.at(p)) == -1) {
                        upperHull.pop_back();
                        cout << "Size: " << upperHull.size();
                        cout << "\n";
                        pointBeingChecked--;
                    }
                    else {
                        hullInError = false;
                    }
                }
                cout << "done";
            }
            //THE POINT IS TO THE RIGHT OF THE EDGE OR COLLINEAR
            upperHull.push_back(points.at(p));
            //cout << "Sorted Point: " << p;
            for (int i = 0; i < upperHull.size(); i++)
                cout << "(" << upperHull[i].x << ", "
                << upperHull[i].y << ")" << endl;
            cout << "-----------------------------------------------------------\n";
        }
        upper = false;
        cout << "------------------------MOVING ONTO LOWER HULL---------------------------------------\n";

        std::sort(points.begin(), points.end());
        for (int i = 0; i < points.size(); i++)
            cout << "(" << points[i].x << ", "
            << points[i].y << ")" << endl;
        cout << "-----------------------------------------------------------\n";

        lowerHull.push_back(points.at(0));
        lowerHull.push_back(points.at(1));
        //FORM THE LOWER PORTION OF THE HULL (-1 IN FOR LOOP SINCE WE DON'T NEED TO CONSIDER THE FARTHEST LEFT POINT (ALREADY TAKEN CARE OF BY UPPER HULL))
        for (size_t p = 2; p < points.size(); p++) {
            if (relativeToLine(lowerHull[lowerHull.size() - 2], lowerHull[lowerHull.size() - 1], points.at(p)) == -1) { //IF POINT IS TO THE LEFT OF THE EDGE
                hullInError = true;
                cout << "ERROR IN HULL\n";
                int pointBeingChecked = lowerHull.size() - 1; //INITIALIZED TO THE LAST INDEX IN convexHull

                while (hullInError && lowerHull.size() > 1) { //CONTINUE REMOVING POINTS FROM convexHull UNTIL THE HULL IS NO LONGER IN ERROR
                    if (relativeToLine(lowerHull[lowerHull.size() - 2], lowerHull[lowerHull.size() - 1], points.at(p)) == -1) {
                        lowerHull.pop_back();
                        cout << "Size: " << upperHull.size();
                        cout << "\n";
                        pointBeingChecked--;
                    }
                    else {
                        hullInError = false;
                    }
                }

            }
            cout << "done";
            lowerHull.push_back(points.at(p));
            for (int i = 0; i < lowerHull.size(); i++)
                cout << "(" << lowerHull[i].x << ", "
                << lowerHull[i].y << ")" << endl;
            cout << "-----------------------------------------------------------\n";
        }

        cout << "-------------------------PRINTING----------------------------------\n";

        for (int i = 0; i < upperHull.size(); i++) {
            convexHull.push_back(upperHull.at(i));
        }
        for (int i = 0; i < lowerHull.size(); i++) {
            convexHull.push_back(lowerHull.at(i));
        }

        for (int c = 0; c < convexHull.size(); c++) {
            cout << "(" << convexHull[c].x << ", "
                << convexHull[c].y << ")" << endl;
        }
    }

    return convexHull;
}


int main() {
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

    vector<Point> ans = getConvexHullPoints(points);

    return 0;
}