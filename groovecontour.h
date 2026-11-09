#ifndef GROOVECONTOUR_H
#define GROOVECONTOUR_H
#include <opencv2/opencv.hpp>
using namespace cv;
using namespace std;
#define GROOVE_MIN_SIZE 40000  //凹槽面积
class GrooveContour
{
public:
    GrooveContour();
    bool isGrayColor(const Scalar& color, int channelDiffThreshold = 15, int maxBlueThreshold = 160);
    bool isGrayRegionImproved(const Mat& roi,int maxBlueThreshold = 160,int channelDiffThreshold = 10);

    void markConsecutiveGrayRegions(vector<vector<bool>>& grayMatrix,int gridRows, int gridCols,
                                    int minConsecutive = 10) ;
    vector<Point> detectObjectContour(const vector<vector<bool>>& grayMatrix,
                                     int gridRows, int gridCols,
                                     int rectTopLeftX, int rectTopLeftY,
                                     float cellWidth, float cellHeight);
    void drawObjectContour(Mat& image, const vector<Point>& contourPoints,
                          const Scalar& color, int thickness);
    vector<Point> maindetect(Mat& image,int rectWidth = 900,int rectHeight = 800,int gridRows = 64,int gridCols = 64);
};

#endif // GROOVECONTOUR_H
