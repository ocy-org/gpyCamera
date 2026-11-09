#ifndef DEALCONTOURS_H
#define DEALCONTOURS_H
#include <opencv2/opencv.hpp>
#include <vector>
#include "groovecontour.h"
using namespace cv;
using namespace std;
class dealContours
{
public:
    dealContours();
    vector<Point> detectContour(Mat& inputImage);
    bool isPointOnBorder(const vector<Point>& points, int width, int height, int tolerance = 10);
    //新增，处理灰白底版
    bool isInsideFixed(const cv::Mat& image, const std::vector<cv::Point>& corners,
                                     int rectWidth = 1000,
                                     int rectHeight = 800);
    bool isCard(const std::vector<cv::Point>& corners);
    double angleBetweenVectors(cv::Point2f v1, cv::Point2f v2);
    bool isApproxRectangle(const std::vector<cv::Point>& pts, double tolerance_deg = 5.0);

private:
    bool isValidContour(const vector<Point>& contour, const cv::Size& imageSize);
    bool isValidBoundingRect(const cv::Rect& rect, const cv::Size& imageSize) ;

    bool hasInvalidPoints(const vector<Point>& points, const cv::Size& imageSize);
    Rect getGridCell(const Mat& image, int gridSize, int row, int col);
    vector<Rect> drawGrid(Mat& image, int gridSize);
    bool isContourInArea(const vector<Point>& contour,const vector<Rect>& exclusionAreas,double threshold);
    void getallContours(const Mat& inputImage, vector<vector<Point>>& contours);
    vector<Point> makerectContours(const vector<vector<Point>>& contours, Mat& inputImage);
    int filterContours(const vector<vector<Point>>& contours,vector<vector<Point>>& filteredContours,const vector<Rect>& exclusionAreas);
    //新增，处理灰白底版
    double Rectarea(const std::vector<cv::Point>& corners);//获取四边形的面积
    vector<Point> getBlackRectangleCorners(const Mat& mask);
    vector<Point> makerectContoursByColor(const Mat& inputImage);//备用的方法，通过颜色获取物体轮廓点
    vector<Point> getCardContoursByHSV_Otsu(const Mat& inputImage);//色域二级化方式提取坐标--适配国光的灰色底板提取银行卡
    vector<Point> extractObjectByColorClustering(const Mat& inputImage) ;//kmeans二级分类方式提取坐标--适合提取银行卡，但算法耗时
};

#endif // DEALCONTOURS_H
