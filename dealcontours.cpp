#include "dealcontours.h"

dealContours::dealContours()
{

}

// 计算两点之间的距离
double pointDistance(const Point& p1, const Point& p2) {
    return sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2));
}

// 合并距离过近的点
vector<Point> mergeClosePoints(const vector<Point>& points, double threshold = 10.0) {
    if (points.empty()) return {};

    vector<Point> result;
    vector<bool> merged(points.size(), false);

    for (size_t i = 0; i < points.size(); i++) {
        if (merged[i]) continue;

        // 查找与当前点距离小于阈值的所有点
        vector<int> closeIndices;
        for (size_t j = i; j < points.size(); j++) {
            if (!merged[j] && pointDistance(points[i], points[j]) < threshold) {
                closeIndices.push_back(j);
            }
        }

        // 计算这些点的平均值作为合并后的点
        if (closeIndices.size() > 1) {
            int sumX = 0, sumY = 0;
            for (int idx : closeIndices) {
                sumX += points[idx].x;
                sumY += points[idx].y;
                merged[idx] = true;
            }
            Point mergedPoint(sumX / closeIndices.size(), sumY / closeIndices.size());
            result.push_back(mergedPoint);
        } else {
            result.push_back(points[i]);
            merged[i] = true;
        }
    }

    return result;
}

// 按顺时针顺序排序点（基于质心）
vector<Point> sortPointsClockwise(const vector<Point>& points) {
    if (points.size() < 3) return points;

    // 计算质心
    Point center(0, 0);
    for (const auto& p : points) {
        center.x += p.x;
        center.y += p.y;
    }
    center.x /= points.size();
    center.y /= points.size();

    // 复制点并排序（顺时针）
    vector<Point> sorted = points;
    sort(sorted.begin(), sorted.end(), [center](const Point& a, const Point& b) {
        double angleA = atan2(a.y - center.y, a.x - center.x);
        double angleB = atan2(b.y - center.y, b.x - center.x);

        // 调整角度范围从0到2π
        if (angleA < 0) angleA += 2 * M_PI;
        if (angleB < 0) angleB += 2 * M_PI;

        return angleA < angleB;
    });

    return sorted;
}

// 使用鞋带公式计算多边形面积（数学方法）
double calculatePolygonArea(const vector<Point>& polygon) {
    if (polygon.size() < 3) return 0.0;

    double area = 0.0;
    int n = polygon.size();

    // 使用鞋带公式计算面积
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        area += (polygon[i].x * polygon[j].y) - (polygon[j].x * polygon[i].y);
    }

    return abs(area) / 2.0;
}

// 找到面积最大的多边形
int findLargestPolygon(const vector<vector<Point>>& polygons) {
    if (polygons.empty()) return -1;

    int maxIndex = 0;
    double maxArea = 0.0;

    for (size_t i = 0; i < polygons.size(); i++) {
        double area = calculatePolygonArea(polygons[i]);
        if (area > maxArea) {
            maxArea = area;
            maxIndex = i;
        }
    }

    return maxIndex;
}
//----------------------------------


// 验证边界矩形是否有效
bool dealContours::isValidBoundingRect(const cv::Rect& rect, const cv::Size& imageSize) {
    if (rect.x < 0 || rect.y < 0 ||
        rect.x + rect.width > imageSize.width ||
        rect.y + rect.height > imageSize.height) {
        cout << "Bounding rect out of image bounds" << endl;
        return false;
    }

    if (rect.width <= 0 || rect.height <= 0) {
        cout << "Bounding rect has invalid dimensions" << endl;
        return false;
    }

    return true;
}

// 检查顶点坐标是否异常
bool dealContours::hasInvalidPoints(const vector<Point>& points, const cv::Size& imageSize) {
    for (const auto& point : points) {
        if (point.x < 0 || point.x >= imageSize.width ||
            point.y < 0 || point.y >= imageSize.height) {
            cout << "Invalid point: " << point << endl;
            return true;
        }

        // 检查坐标是否异常大（如1953264993）
        if (point.x > imageSize.width * 10 || point.y > imageSize.height * 10) {
            cout << "Abnormally large point: " << point << endl;
            return true;
        }
    }
    return false;
}

// 改进的轮廓验证函数
bool dealContours::isValidContour(const vector<Point>& contour, const cv::Size& imageSize) {
    if (contour.size() < 3) {
        cout << "Contour has too few points: " << contour.size() << endl;
        return false;
    }

    // 检查轮廓点是否在合理范围内
    for (const auto& point : contour) {
        if (point.x < -100 || point.x > imageSize.width + 100 ||
            point.y < -100 || point.y > imageSize.height + 100) {
            cout << "Contour point out of bounds: " << point << endl;
            return false;
        }

        // 检查坐标是否为异常值
        if (abs(point.x) > 1000000 || abs(point.y) > 1000000) {
            cout << "Contour point has abnormal value: " << point << endl;
            return false;
        }
    }

    double area = cv::contourArea(contour);
    if (area < 100) { // 设置最小面积阈值
        cout << "Contour area too small: " << area << endl;
        return false;
    }

    return true;
}

// 获取指定网格区域的函数（使用从1开始的索引）
Rect dealContours::getGridCell(const Mat& image, int gridSize, int row, int col) {
    int rows = image.rows;
    int cols = image.cols;
    if(row<1)row=1;
    if(col<1)col=1;
    // 将1-based索引转换为0-based索引
    int rowIndex = row - 1;
    int colIndex = col - 1;

    // 确保转换后的索引在有效范围内
    rowIndex = max(0, min(rowIndex, gridSize - 1));
    colIndex = max(0, min(colIndex, gridSize - 1));

    // 计算网格步长
    int gridWidth = cols / gridSize;
    int gridHeight = rows / gridSize;

    // 计算网格位置和尺寸
    int x = colIndex * gridWidth;
    int y = rowIndex * gridHeight;
    int width = gridWidth;
    int height = gridHeight;

    // 处理边界情况，确保不超出图像范围
    if (colIndex == gridSize - 1) width = cols - x;
    if (rowIndex == gridSize - 1) height = rows - y;

    //cout << "Grid cell (row=" << row << ", col=" << col << "): "
    //     << "(" << x << "," << y << "),(" << x+width << "," << y+height << ")" << endl;

    return Rect(x, y, width, height);
}

// 绘制网格函数并返回四个边界区域
vector<Rect> dealContours::drawGrid(Mat& image, int gridSize = 36) {
    int rows = image.rows;
    int cols = image.cols;

    // 计算网格步长
    int gridWidth = cols / gridSize;
    int gridHeight = rows / gridSize;

    // 定义排除区域向量
    vector<cv::Rect> exclusionAreas;

    // 添加四个边缘区域
    exclusionAreas.push_back(cv::Rect(0, 0, cols, gridHeight)); // 第一行区域
    exclusionAreas.push_back(cv::Rect(0, (gridSize-1)*gridHeight, cols,
                                     rows - (gridSize-1)*gridHeight)); // 最后一行区域
    exclusionAreas.push_back(cv::Rect(0, 0, gridWidth, rows)); // 第一列区域
    exclusionAreas.push_back(cv::Rect((gridSize-1)*gridWidth, 0,
                                     cols - (gridSize-1)*gridWidth, rows)); // 最后一列区域

    return exclusionAreas;
}

// 改进的检查轮廓是否在排除区域内
bool dealContours::isContourInArea(const vector<Point>& contour,
                             const vector<Rect>& exclusionAreas,
                             double threshold = 0.75) {
    int pointsInArea = 0;
    int totalPoints = contour.size();

    // 打印所有排除区域
//    for (size_t i = 0; i < exclusionAreas.size(); i++) {
//        const auto& area = exclusionAreas[i];
//        std::cout << "Exclusion area " << i << ": (" << area.x << ", " << area.y
//                  << ", " << area.width << ", " << area.height << ")" << std::endl;
//    }

    // 手动检查每个点是否在排除区域内
    for (const auto& point : contour) {
        bool inArea = false;
        // 手动检查点是否在任何一个排除区域内
        for (size_t i = 0; i < exclusionAreas.size(); i++) {
            const auto& area = exclusionAreas[i];
            // 手动实现点是否在矩形内的检查
            if (point.x >= area.x && point.x < area.x + area.width &&
                point.y >= area.y && point.y < area.y + area.height) {
                pointsInArea++;
                inArea = true;
//                std::cout << "Point " << point << " is in exclusion area " << i
//                          << ": (" << area.x << ", " << area.y
//                          << ", " << area.width << ", " << area.height << ")" << std::endl;
                break;
            }
        }
//        if (!inArea) {
//            std::cout << "Point " << point << " is NOT in any exclusion area" << std::endl;
//        }
    }

    double ratio = (double)pointsInArea / totalPoints;
//    std::cout << "Points in exclusion areas: " << pointsInArea << "/" << totalPoints
//              << " = " << ratio << " (threshold: " << threshold << ")" << std::endl;

    // 如果超过阈值比例的点在排除区域内，则返回true
    return ratio > threshold;
}


void dealContours::getallContours(const Mat& inputImage, vector<vector<Point>>& contours) {
    cv::Mat gray, blurred, edges;
    cv::cvtColor(inputImage, gray, cv::COLOR_BGR2GRAY);
    GaussianBlur(gray, blurred, Size(9, 9), 3.0);
    // 3. Canny边缘检测
    double cannyThreshold1 = 50;
    double cannyThreshold2 = 150;
    cv::Canny(blurred, edges, cannyThreshold1, cannyThreshold2);
    // 增强形态学操作，更好地连接边缘
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7, 7));
    cv::morphologyEx(edges, edges, cv::MORPH_CLOSE, kernel);
    cv::dilate(edges, edges, kernel); // 添加膨胀操作
    // 查找轮廓
    cv::findContours(edges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    //cout << "find " << contours.size() << " Contours" << endl;
}

/**
 * 判断一个点是否在图像边界附近
 * @param pt        待检测的点
 * @param width     图像宽度（img.cols）
 * @param height    图像高度（img.rows）
 * @param tolerance 容差（默认15像素）
 * @return          true 表示在边界上或附近
 */
bool dealContours::isPointOnBorder(const vector<Point>& points, int width, int height, int tolerance) {
    bool onBorder;
    for (size_t i = 0; i < points.size(); ++i) {
            cv::Point pt = points[i];
             onBorder = (pt.x <= tolerance) ||
                    (pt.y <= tolerance) ||
                    (pt.x >= width - 1 - tolerance) ||
                    (pt.y >= height - 1 - tolerance);
    }
    return onBorder;
}

//获取最小外接矩形
vector<Point> dealContours::makerectContours(const vector<vector<Point>>& contours, Mat& inputImage) {
    if (contours.empty()) {
        return vector<Point>();
    }

    // 存储所有处理后的多边形
    vector<vector<Point>> processedPolygons;
    for (size_t i = 0; i < contours.size(); i++) {
        double area = contourArea(contours[i]);
        //int point=contours[i].size();
        //if (point > 100) {
        if (area > 500) {
            // 使用多边形近似
            vector<Point> approx;
            double epsilon = 0.02 * arcLength(contours[i], true);
            approxPolyDP(contours[i], approx, epsilon, true);
            if(approx.size()<4)continue;
            // 合并距离过近的点
            vector<Point> mergedPoints = mergeClosePoints(approx, 15.0); // 15像素阈值
            // 确保多边形至少有三个点
            if (mergedPoints.size() >= 3) {
                // 按顺时针顺序排序点
                mergedPoints = sortPointsClockwise(mergedPoints);
                processedPolygons.push_back(mergedPoints);
            }
        }
    }

    // 1. 找到面积最大的轮廓
    auto maxContour = *max_element(contours.begin(), contours.end(),
        [](const vector<Point>& a, const vector<Point>& b) {
            return cv::contourArea(a) < cv::contourArea(b);
        });

    // 2. 验证轮廓点的有效性
    if (!isValidContour(maxContour, inputImage.size())) {
        cout << "Invalid contour detected, skipping..." << endl;
        return vector<Point>();
    }

    // 3. 获取最小外接矩形
    cv::RotatedRect rotatedRect = cv::minAreaRect(maxContour);
    cv::Point2f vertices[4];
    rotatedRect.points(vertices);

    // 5. 转换为整数点坐标
    vector<Point> rectPoints;
    for (int i = 0; i < 4; i++) {
        int x = static_cast<int>(std::round(vertices[i].x));
        int y = static_cast<int>(std::round(vertices[i].y));

        // 确保坐标在合理范围内
        x = std::max(0, std::min(x, inputImage.cols - 1));
        y = std::max(0, std::min(y, inputImage.rows - 1));

        rectPoints.push_back(cv::Point(x, y));
    }

    if (rectPoints.empty()) {
        return vector<Point>();
    }
    // 计算并显示面积
    double area = cv::contourArea(rectPoints);
    //cout << "isPointOnBorder: " << inputImage.cols << inputImage.rows << endl;
    //bool bret=isPointOnBorder(rectPoints,3264,2448);
    bool bret=isPointOnBorder(rectPoints,inputImage.cols,inputImage.rows);
    if(area<GROOVE_MIN_SIZE || bret){//面积太小的时候，检测中间凹槽是不是有卡片
        //cout << "=======GrooveContour=======" << endl;
        GrooveContour checkContour;
        rectPoints=checkContour.maindetect(inputImage);
        if(rectPoints.size()!=4) return vector<Point>();
    }

    // 7. 检查顶点坐标是否异常
    if (hasInvalidPoints(rectPoints, inputImage.size())) {
        cout << "Rectangle has invalid points, using convex hull instead..." << endl;
        // 使用凸包作为备选方案
        vector<Point> hull;
        cv::convexHull(maxContour, hull);
        return hull;
    }

    return rectPoints;
}

int dealContours::filterContours(const vector<vector<Point>>& contours,
                   vector<vector<Point>>& filteredContours,
                   const vector<Rect>& exclusionAreas) {
    int excludedCount = 0;
    for (const auto& contour : contours) {
        if (isContourInArea(contour, exclusionAreas, 0.75)) {
            excludedCount++;
            continue; // 跳过这个轮廓
        }
        filteredContours.push_back(contour);
    }

    //cout << "filter " << excludedCount << " edge Contours" << endl;
    return excludedCount;
}

vector<Point> dealContours::detectContour(cv::Mat& inputImage) {
    Mat getImagecontours = inputImage.clone();
    //1.在原始图像上绘制gridsizexgridsize网格并获取排除区域
    int gridsize = 20;
    vector<Rect> exclusionAreas;
    //drawGrid的行和列的索引
    {//四条边界
        exclusionAreas=drawGrid(inputImage, gridsize);
    }
    //第2行和第3行的第2列网格需要排除
    if(0)
    {
        Rect pos;
        pos=getGridCell(inputImage,gridsize,2,2);
        exclusionAreas.push_back(pos);
        pos=getGridCell(inputImage,gridsize,3,2);
        exclusionAreas.push_back(pos);

        pos=getGridCell(inputImage,gridsize,2,19);
        exclusionAreas.push_back(pos);
    }
    //第5行的第4和第5列网格需要排除
    {
        Rect pos;
        pos=getGridCell(inputImage,gridsize,5,4);
        exclusionAreas.push_back(pos);
        pos=getGridCell(inputImage,gridsize,5,5);
        exclusionAreas.push_back(pos);
    }
    //第5行的第16和第17列网格需要排除
    {
        Rect pos;
        pos=getGridCell(inputImage,gridsize,5,16);
        exclusionAreas.push_back(pos);
        pos=getGridCell(inputImage,gridsize,5,17);
        exclusionAreas.push_back(pos);
    }

    //2. 查找所有轮廓并用绿色绘制
    vector<vector<Point>> contours;
    getallContours(getImagecontours, contours);
    if (contours.empty()) {
        return vector<Point>();
    }

    //过滤轮廓，排除位于边缘区域的轮廓
    vector<vector<Point>> filteredContours;
    filterContours(contours, filteredContours, exclusionAreas);

    vector<Point> rectPoints=makerectContours(contours, inputImage);
//    if (rectPoints.size() == 4) {
//        cout << "points:" << endl;
//        for (int i = 0; i < 4; i++) {
//            cout << "point" << i + 1 << ": (" << rectPoints[i].x << ", " << rectPoints[i].y << ")" << endl;
//        }
//    }
    //return rectPoints;
    //判断是否是在固定的中心指定区域---有凹痕的底板会干扰，这里是检测银行卡被干扰的情况
    if(isInsideFixed(inputImage,rectPoints)){
        //std::cout << "isInsideFixed" << std::endl;
        //如果已经是卡片尺寸，就不处理了
        if(!isCard(rectPoints)){
            std::cout << "new card" << std::endl;
             vector<Point> rect;
             //rect=extractObjectByColorClustering(getImagecontours);
             rect=getCardContoursByHSV_Otsu(getImagecontours);
             return rect;
        }
    }

    return rectPoints;
}


bool dealContours::isInsideFixed(const cv::Mat& image, const std::vector<cv::Point>& corners,
                                 int rectWidth,
                                 int rectHeight) {
    // 检查输入合法性
    if (corners.size() != 4) {
        return false;
    }

    int imgWidth = image.cols;
    int imgHeight = image.rows;

    // 计算图像中心
    double centerX = imgWidth / 2.0;
    double centerY = imgHeight / 2.0;

    // 定义固定区域边界（以中心为基准，宽1000，高800）
    double left   = centerX - rectWidth / 2;
    double right  = centerX + rectWidth / 2;
    double top    = centerY - rectHeight / 2;
    double bottom = centerY + rectHeight / 2;

    // 检查每个角点是否在区域内
    for (const auto& pt : corners) {
        if (pt.x < left   || pt.x > right ||
            pt.y < top    || pt.y > bottom) {
            return false; // 有一个点在外面就返回 false
        }
    }

    return true; // 所有点都在区域内
}

//判断是不是银行卡、身份证等标准存储的卡片
bool dealContours::isCard(const std::vector<cv::Point>& corners){
    bool ret=false;
    ret=isApproxRectangle(corners);//看看是不是矩形
    double areas=Rectarea(corners);
    //尺寸大约：741*463
    if(areas>680000)
        ret=false;
    //cout << __FUNCTION__ << " areas:"  << areas << " ret:" << ret << endl;
    return ret;
}

// 计算两个向量之间的夹角（弧度）
double dealContours::angleBetweenVectors(cv::Point2f v1, cv::Point2f v2) {
    double dot = v1.x * v2.x + v1.y * v2.y;
    double norm1 = std::sqrt(v1.x * v1.x + v1.y * v1.y);
    double norm2 = std::sqrt(v2.x * v2.x + v2.y * v2.y);

    if (norm1 == 0 || norm2 == 0) return 0; // 避免除零

    double cosTheta = dot / (norm1 * norm2);
    // 数值误差可能导致 cosTheta 超出 [-1, 1]
    cosTheta = std::max(-1.0, std::min(1.0, cosTheta));

    return std::acos(cosTheta); // 返回弧度
}

// 判断四点是否构成近似矩形（角度误差 <= tolerance_deg）
bool dealContours::isApproxRectangle(const std::vector<cv::Point>& pts, double tolerance_deg) {
    if (pts.size() != 4) return false;

    // 按顺序：p0, p1, p2, p3 （必须是连续的四边形顶点，顺时针或逆时针）
    std::vector<cv::Point2f> corners(pts.begin(), pts.end());

    // 计算每个角的夹角
    for (int i = 0; i < 4; ++i) {
        cv::Point2f prev = corners[(i - 1 + 4) % 4];
        cv::Point2f curr = corners[i];
        cv::Point2f next = corners[(i + 1) % 4];

        // 从 curr 出发的两条边向量
        cv::Point2f v1 = prev - curr;   // 指向前一个点
        cv::Point2f v2 = next - curr;   // 指向后一个点

        double angleRad = angleBetweenVectors(v1, v2);
        double angleDeg = angleRad * 180.0 / CV_PI;

        // 内角应为 ～90°，但由于向量方向，可能得到的是外角（270°）
        // 所以取 min(angle, 360 - angle)
        if (angleDeg > 180) angleDeg = 360 - angleDeg;

        // 检查是否在 [90 - tol, 90 + tol] 范围内
        if (std::abs(angleDeg - 90.0) > tolerance_deg) {
            std::cout << "角 " << i << " 不符合: " << angleDeg << "°\n";
            return false;
        }
    }

    return true;
}

//返回4个点(矩形)的面积
double dealContours::Rectarea(const std::vector<cv::Point>& corners){
    if(corners.size() != 4) {
        // 如果点的数量不是4，则无法形成四边形
        return -1.0;
    }

    // 使用向量叉乘计算两个三角形的面积之和

    // 计算第一个三角形的面积 p1, p2, p3
    double area1 = abs((corners[0].x * (corners[1].y - corners[2].y) +
                        corners[1].x * (corners[2].y - corners[0].y) +
                        corners[2].x * (corners[0].y - corners[1].y)) / 2.0);

    // 计算第二个三角形的面积 p1, p3, p4
    double area2 = abs((corners[0].x * (corners[2].y - corners[3].y) +
                        corners[2].x * (corners[3].y - corners[0].y) +
                        corners[3].x * (corners[0].y - corners[2].y)) / 2.0);

    // 返回总面积
    return area1 + area2;
}

// 获取黑色区域矩形四角坐标的函数
vector<Point> dealContours::getBlackRectangleCorners(const Mat& mask) {
    vector<Point> corners;

    // Step 1: 反转Mask，使黑色区域变为白色（便于轮廓检测）
    Mat invertedMask;
    bitwise_not(mask, invertedMask);

    // Step 2: 对反转后的Mask进行形态学操作，确保区域连接
    //Mat morphKernel = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));
    //morphologyEx(invertedMask, invertedMask, MORPH_CLOSE, morphKernel);
    //morphologyEx(invertedMask, invertedMask, MORPH_OPEN, morphKernel);

    // Step 2: 查找轮廓
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;
    findContours(invertedMask, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    if (contours.empty()) {
        cerr << "Error: No contours found in the mask image." << endl;
        return corners;
    }

    // Step 3: 找到最大的轮廓（假设黑色区域是最大的轮廓）
    int maxContourIdx = 0;
    double maxArea = 0;
    for (size_t i = 0; i < contours.size(); i++) {
        double area = contourArea(contours[i]);
        if (area > maxArea) {
            maxArea = area;
            maxContourIdx = i;
        }
    }

    // Step 4: 获取最小外接矩形
    RotatedRect rotatedRect = minAreaRect(contours[maxContourIdx]);

    // Step 5: 获取矩形的四个角点
    Point2f rectPoints[4];
    rotatedRect.points(rectPoints);

    // 将Point2f转换为Point，并存储到vector中
    for (int i = 0; i < 4; i++) {
        corners.push_back(Point(rectPoints[i].x, rectPoints[i].y));
    }

    // 可选：按左上、右上、右下、左下的顺序排序
    // 但根据您的需求，返回原始四个点即可

    return corners;
}


vector<Point> dealContours::makerectContoursByColor(const Mat& inputImage){
    // Step 1: 将图像从 BGR 转换为 Lab 颜色空间
    // Lab 空间优势：L（亮度）与 a/b（色度）解耦，对光照变化鲁棒
    Mat lab;
    cvtColor(inputImage, lab, COLOR_BGR2Lab);
    // Step 2: 分离 Lab 三个通道
    vector<Mat> channels;
    split(lab, channels); // channels[0]=L, [1]=a, [2]=b
    // Step 3: 准备 KMeans 聚类所需的数据矩阵: N x 2 matrix (a, b)
    int rows = inputImage.rows;
    int cols = inputImage.cols;
    int N = rows * cols;
    // 创建 N×2 的浮点型矩阵，每行代表一个像素的 (a, b) 色度特征
    // 注意：KMeans 要求输入为 CV_32F（float）
    Mat samples(N, 2, CV_32F);
    //填充每个像素的 a 和 b 值
    for (int i = 0; i < N; ++i) {
        // channels[1].data[i] 是第 i 个像素的 a 值（uchar 类型，范围 0~255）
        // 在 OpenCV 的 Lab 实现中：
        //   - a = 128 表示无红/绿偏（中性）
        //   - a < 128 → 偏绿；a > 128 → 偏红
        //   - b = 128 表示无蓝/黄偏（中性）
        //   - b < 128 → 偏蓝；b > 128 → 偏黄
        // 蓝灰色通常：a ≈ 128（中性），b < 128（略偏蓝）
        samples.at<float>(i, 0) = static_cast<float>(channels[1].data[i]); // a
        samples.at<float>(i, 1) = static_cast<float>(channels[2].data[i]); // b
    }

    // Step 4: 执行 KMeans 聚类（K=2：假设图像只有两类主要颜色：背景 + 前景）
    const int K = 2;
    Mat labels;    // 输出：每个像素所属的簇标签（0 或 1）
    Mat centers;   // 输出：每个簇的中心点（2 维，即 [a_center, b_center]）
    kmeans(
        samples,                     // 输入数据（N×2）
        K,                           // 聚类数量
        labels,                      // 输出标签
        TermCriteria(                // 迭代终止条件：最多 10 次或精度 1.0
            TermCriteria::EPS + TermCriteria::MAX_ITER, 10, 1.0),
        3,                           // 尝试 3 次不同初始中心，取最优
        KMEANS_RANDOM_CENTERS,       // 随机初始化聚类中心
        centers                      // 输出聚类中心
    );

    // Step 5: 判断哪个聚类是“蓝灰色背景”
    // 典型蓝灰色在 OpenCV Lab 中表现为：
    //   a ≈ 128（无红绿偏），b ≈ 110~125（略偏蓝）
    // 我们设定一个参考点 (a=128, b=120)，找离它最近的聚类中心
    int backgroundCluster = 0;
    float minDist = FLT_MAX;
    int lab_a=128;
    int lab_b=120;
    for (int k = 0; k < K; ++k) {
        float a_center = centers.at<float>(k, 0);
        float b_center = centers.at<float>(k, 1);
        // 计算欧氏距离：sqrt((a-lab_a)^2 + (b-lab_b)^2)
        float dist = sqrt(pow(a_center - lab_a, 2) + pow(b_center - lab_b, 2));
        if (dist < minDist) {
            minDist = dist;
            backgroundCluster = k;
        }
    }

    // Step 6: 构建背景掩码（白色 = 背景，黑色 = 前景）
    Mat mask(rows, cols, CV_8UC1, Scalar(0));
    for (int i = 0; i < N; ++i) {
        int y = i / cols;
        int x = i % cols;
        // 如果该像素属于背景簇，则在掩码中标记为白色（255）
        if (labels.at<int>(i) == backgroundCluster) {
            mask.at<uchar>(y, x) = 255;
        }
    }

    // Step 7: 形态学后处理（去除小噪声、填补小孔）
    Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(3, 3));
    morphologyEx(mask, mask, MORPH_CLOSE, kernel);
    morphologyEx(mask, mask, MORPH_OPEN, kernel);

    vector<Point> corners = getBlackRectangleCorners(mask);// 获取黑色区域矩形四角坐标的函数
    return corners;
}


//通过HSV颜色空间中的Otsu's方法进行二值化，并找到卡片轮廓
vector<Point> dealContours::getCardContoursByHSV_Otsu(const Mat& inputImage) {
    // 转换为HSV颜色空间
    Mat hsvImg;
    cvtColor(inputImage, hsvImg, COLOR_BGR2HSV);

    // 分离H、S、V通道
    vector<Mat> hsvChannels;
    split(hsvImg, hsvChannels);
    Mat hue = hsvChannels[0], sat = hsvChannels[1], val = hsvChannels[2];

    // 对S通道应用Otsu's二值化，因为S通道通常能较好地区分颜色
    Mat binary;
    double otsuThreshValue = threshold(sat, binary, 0, 255, THRESH_BINARY + THRESH_OTSU);

    // 应用形态学操作以去除噪声并闭合孔洞
    Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));
    morphologyEx(binary, binary, MORPH_OPEN, kernel); // 先腐蚀后膨胀去除小噪点
    morphologyEx(binary, binary, MORPH_CLOSE, kernel); // 先膨胀后腐蚀填补空洞

    // 查找轮廓
    vector<vector<Point>> contours;
    findContours(binary, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    // 1. 找到面积最大的轮廓
    auto maxContour = *max_element(contours.begin(), contours.end(),
        [](const vector<Point>& a, const vector<Point>& b) {
            return cv::contourArea(a) < cv::contourArea(b);
        });

    // 3. 获取最小外接矩形
    cv::RotatedRect rotatedRect = cv::minAreaRect(maxContour);
    cv::Point2f vertices[4];
    rotatedRect.points(vertices);

    // 5. 转换为整数点坐标
    vector<Point> rectPoints;
    for (int i = 0; i < 4; i++) {
        int x = static_cast<int>(std::round(vertices[i].x));
        int y = static_cast<int>(std::round(vertices[i].y));

        // 确保坐标在合理范围内
        x = std::max(0, std::min(x, inputImage.cols - 1));
        y = std::max(0, std::min(y, inputImage.rows - 1));

        rectPoints.push_back(cv::Point(x, y));
    }

    if (rectPoints.empty()) {
        return vector<Point>();
    }

    return rectPoints;
}

//kmeans二级分类方式提取坐标
vector<Point> dealContours::extractObjectByColorClustering(const Mat& inputImage) {
    // Step 1: 将图像从 BGR 转换为 Lab 颜色空间
    // Lab 空间优势：L（亮度）与 a/b（色度）解耦，对光照变化鲁棒
    Mat lab;
    cvtColor(inputImage, lab, COLOR_BGR2Lab);

    // Step 2: 分离 Lab 三个通道
    vector<Mat> channels;
    split(lab, channels); // channels[0]=L, [1]=a, [2]=b

    // Step 3: 准备 KMeans 聚类所需的数据矩阵: N x 2 matrix (a, b)
    int rows = inputImage.rows;
    int cols = inputImage.cols;
    int N = rows * cols;

    // 创建 N×2 的浮点型矩阵，每行代表一个像素的 (a, b) 色度特征
    // 注意：KMeans 要求输入为 CV_32F（float）
    Mat samples(N, 2, CV_32F);

    //填充每个像素的 a 和 b 值
    for (int i = 0; i < N; ++i) {
        // channels[1].data[i] 是第 i 个像素的 a 值（uchar 类型，范围 0~255）
        // 在 OpenCV 的 Lab 实现中：
        //   - a = 128 表示无红/绿偏（中性）
        //   - a < 128 → 偏绿；a > 128 → 偏红
        //   - b = 128 表示无蓝/黄偏（中性）
        //   - b < 128 → 偏蓝；b > 128 → 偏黄
        // 蓝灰色通常：a ≈ 128（中性），b < 128（略偏蓝）
        samples.at<float>(i, 0) = static_cast<float>(channels[1].data[i]); // a
        samples.at<float>(i, 1) = static_cast<float>(channels[2].data[i]); // b
    }

    // Step 4: 执行 KMeans 聚类（K=2：假设图像只有两类主要颜色：背景 + 前景）
    const int K = 2;
    Mat labels;    // 输出：每个像素所属的簇标签（0 或 1）
    Mat centers;   // 输出：每个簇的中心点（2 维，即 [a_center, b_center]）
    kmeans(
        samples,                     // 输入数据（N×2）
        K,                           // 聚类数量
        labels,                      // 输出标签
        TermCriteria(                // 迭代终止条件：最多 10 次或精度 1.0
            TermCriteria::EPS + TermCriteria::MAX_ITER, 10, 1.0),
        3,                           // 尝试 3 次不同初始中心，取最优
        KMEANS_RANDOM_CENTERS,       // 随机初始化聚类中心
        centers                      // 输出聚类中心
    );

    // Step 5: 判断哪个聚类是“蓝灰色背景”
    // 典型蓝灰色在 OpenCV Lab 中表现为：
    //   a ≈ 128（无红绿偏），b ≈ 110~125（略偏蓝）
    // 我们设定一个参考点 (a=128, b=120)，找离它最近的聚类中心
    int backgroundCluster = 0;
    float minDist = FLT_MAX;
    int lab_a=128;
    int lab_b=120;
    for (int k = 0; k < K; ++k) {
        float a_center = centers.at<float>(k, 0);
        float b_center = centers.at<float>(k, 1);
        // 计算欧氏距离：sqrt((a-lab_a)^2 + (b-lab_b)^2)
        float dist = sqrt(pow(a_center - lab_a, 2) + pow(b_center - lab_b, 2));
        if (dist < minDist) {
            minDist = dist;
            backgroundCluster = k;
        }
    }

    // Step 6: 构建背景掩码（白色 = 背景，黑色 = 前景）
    Mat mask(rows, cols, CV_8UC1, Scalar(0));
    for (int i = 0; i < N; ++i) {
        int y = i / cols;
        int x = i % cols;
        // 如果该像素属于背景簇，则在掩码中标记为白色（255）
        if (labels.at<int>(i) == backgroundCluster) {
            mask.at<uchar>(y, x) = 255;
        }
    }

    // Step 7: 形态学后处理（去除小噪声、填补小孔）
    Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(3, 3));
    morphologyEx(mask, mask, MORPH_CLOSE, kernel);
    morphologyEx(mask, mask, MORPH_OPEN, kernel);

    vector<Point> corners = getBlackRectangleCorners(mask);
    if (corners.size() != 4) {
        return vector<Point>();
    }
    return corners;
}
