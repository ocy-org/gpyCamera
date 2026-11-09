#include "groovecontour.h"

GrooveContour::GrooveContour()
{

}

struct GridCellInfo {
    int id;
    Point topLeft;
    Point topRight;
    Point bottomLeft;
    Point bottomRight;
    Scalar averageColor;
    double averageBrightness;
    bool isGray;
};

/**
 * @brief 改进的灰色检测函数
 */
bool GrooveContour::isGrayColor(const Scalar& color, int channelDiffThreshold, int maxBlueThreshold) {
    if (color[0] > maxBlueThreshold) {
        return false;
    }

    int diffBG = abs(color[0] - color[1]);
    int diffBR = abs(color[0] - color[2]);
    int diffGR = abs(color[1] - color[2]);

    return (diffBG <= channelDiffThreshold &&
            diffBR <= channelDiffThreshold &&
            diffGR <= channelDiffThreshold);
}

/**
 * @brief 判断区域是否为灰色区域
 */
bool GrooveContour::isGrayRegionImproved(const Mat& roi,
                         int maxBlueThreshold,
                         int channelDiffThreshold) {
    if (roi.empty()) return true;

    Scalar avgColor = mean(roi);
    return isGrayColor(avgColor, channelDiffThreshold, maxBlueThreshold);
}

/**
 * @brief 检测连续灰色区域并标记整行/整列
 */
void GrooveContour::markConsecutiveGrayRegions(vector<vector<bool>>& grayMatrix,
                               int gridRows, int gridCols,
                               int minConsecutive) {
    // 检查每一行是否有连续minConsecutive个灰色
    for (int row = 0; row < gridRows; ++row) {
        int consecutiveCount = 0;
        for (int col = 0; col < gridCols; ++col) {
            if (grayMatrix[row][col]) {
                consecutiveCount++;
            } else {
                consecutiveCount = 0;
            }

            if (consecutiveCount >= minConsecutive) {
                for (int c = 0; c < gridCols; ++c) {
                    grayMatrix[row][c] = true;
                }
                break;
            }
        }
    }

    // 检查第一列和最后一列是否有连续minConsecutive个灰色
    for (int col : {0, gridCols-1}) {
        int consecutiveCount = 0;
        for (int row = 0; row < gridRows; ++row) {
            if (grayMatrix[row][col]) {
                consecutiveCount++;
            } else {
                consecutiveCount = 0;
            }

            if (consecutiveCount >= minConsecutive) {
                for (int r = 0; r < gridRows; ++r) {
                    grayMatrix[r][col] = true;
                }
                break;
            }
        }
    }
}

/**
 * @brief 检测物体轮廓并返回矩形四个点
 * @param grayMatrix 灰色标记矩阵
 * @param gridRows 网格行数
 * @param gridCols 网格列数
 * @param rectTopLeftX 检测区域左上角X坐标
 * @param rectTopLeftY 检测区域左上角Y坐标
 * @param cellWidth 网格宽度
 * @param cellHeight 网格高度
 * @return 物体轮廓的四个点（左上、右上、右下、左下）
 */
vector<Point> GrooveContour::detectObjectContour(const vector<vector<bool>>& grayMatrix,
                                 int gridRows, int gridCols,
                                 int rectTopLeftX, int rectTopLeftY,
                                 float cellWidth, float cellHeight) {
    vector<Point> contourPoints;

    // 寻找物体的边界
    int minRow = gridRows, maxRow = 0;
    int minCol = gridCols, maxCol = 0;
    bool foundObject = false;

    // 找到非灰色区域的最小和最大行列索引
    for (int row = 0; row < gridRows; ++row) {
        for (int col = 0; col < gridCols; ++col) {
            if (!grayMatrix[row][col]) {
                foundObject = true;
                minRow = min(minRow, row);
                maxRow = max(maxRow, row);
                minCol = min(minCol, col);
                maxCol = max(maxCol, col);
            }
        }
    }

    if (!foundObject) {
        return contourPoints; // 返回空向量表示未找到物体
    }

    // 计算物体轮廓的四个角点
    Point topLeft, topRight, bottomRight, bottomLeft;

    // 左上角
    topLeft.x = rectTopLeftX + static_cast<int>(minCol * cellWidth);
    topLeft.y = rectTopLeftY + static_cast<int>(minRow * cellHeight);

    // 右上角
    topRight.x = rectTopLeftX + static_cast<int>((maxCol + 1) * cellWidth);
    topRight.y = rectTopLeftY + static_cast<int>(minRow * cellHeight);

    // 右下角
    bottomRight.x = rectTopLeftX + static_cast<int>((maxCol + 1) * cellWidth);
    bottomRight.y = rectTopLeftY + static_cast<int>((maxRow + 1) * cellHeight);

    // 左下角
    bottomLeft.x = rectTopLeftX + static_cast<int>(minCol * cellWidth);
    bottomLeft.y = rectTopLeftY + static_cast<int>((maxRow + 1) * cellHeight);

    // 添加到轮廓点向量
    contourPoints.push_back(topLeft);
    contourPoints.push_back(topRight);
    contourPoints.push_back(bottomRight);
    contourPoints.push_back(bottomLeft);

    return contourPoints;
}

/**
 * 判断一个点是否在图像边界附近
 * @param pt        待检测的点
 * @param width     图像宽度（img.cols）
 * @param height    图像高度（img.rows）
 * @param tolerance 容差（默认15像素）
 * @return          true 表示在边界上或附近
 */
bool isPointOnBorder(const cv::Point& pt, int width, int height, int tolerance) {
    return (pt.x <= tolerance) ||
           (pt.y <= tolerance) ||
           (pt.x >= width - 1 - tolerance) ||
           (pt.y >= height - 1 - tolerance);
}

/**
 * @brief 主检测函数
 */
vector<Point> GrooveContour::maindetect(Mat& image,int rectWidth,int rectHeight,int gridRows,int gridCols) {
    Mat resultImage = image.clone();

    int imgWidth = image.cols;
    int imgHeight = image.rows;

    // 计算中心矩形
    int centerX = imgWidth / 2;
    int centerY = imgHeight / 2;

    rectWidth = min(rectWidth, imgWidth);
    rectHeight = min(rectHeight, imgHeight);

    int rectTopLeftX = max(0, centerX - rectWidth / 2);
    int rectTopLeftY = max(0, centerY - rectHeight / 2);
    int rectBottomRightX = min(imgWidth, centerX + rectWidth / 2);
    int rectBottomRightY = min(imgHeight, centerY + rectHeight / 2);

    // 计算网格尺寸
    int rectWidthPixels = rectBottomRightX - rectTopLeftX;
    int rectHeightPixels = rectBottomRightY - rectTopLeftY;
    float cellWidth = static_cast<float>(rectWidthPixels) / gridCols;
    float cellHeight = static_cast<float>(rectHeightPixels) / gridRows;

    // 创建灰色标记矩阵
    vector<vector<bool>> grayMatrix(gridRows, vector<bool>(gridCols, false));

    // 第一遍：处理每个网格，记录灰色状态
    for (int row = 0; row < gridRows; ++row) {
        for (int col = 0; col < gridCols; ++col) {
            int x1 = rectTopLeftX + static_cast<int>(col * cellWidth);
            int y1 = rectTopLeftY + static_cast<int>(row * cellHeight);
            int x2 = rectTopLeftX + static_cast<int>((col + 1) * cellWidth);
            int y2 = rectTopLeftY + static_cast<int>((row + 1) * cellHeight);

            x2 = min(x2, rectBottomRightX);
            y2 = min(y2, rectBottomRightY);

            Rect cellRect(x1, y1, x2 - x1, y2 - y1);
            Mat cellRegion = image(cellRect);

            // 使用改进的灰色检测
            bool isGray = isGrayRegionImproved(cellRegion);

            // 对第一列进行特殊处理
            if (col == 0 && !isGray) {
                isGray = isGrayRegionImproved(cellRegion, 200);
            }
            grayMatrix[row][col] = isGray;
        }
    }

    // 第二遍：检测连续灰色区域并标记整行/整列
    markConsecutiveGrayRegions(grayMatrix, gridRows, gridCols, 10);
    // 检测物体轮廓
    vector<Point> objectContour = detectObjectContour(grayMatrix, gridRows, gridCols,
                                                     rectTopLeftX, rectTopLeftY,
                                                     cellWidth, cellHeight);
    return objectContour;
}
