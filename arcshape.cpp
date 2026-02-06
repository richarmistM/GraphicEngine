#include "arcshape.h"
#include "drawengine.h"
#include <cmath>
#include <QtMath>

/**
 * @brief 默认构造函数：初始化为无效圆弧
 */
ArcShape::ArcShape()
    : center(0, 0), radius(0), startAngle(0), endAngle(0), color(Qt::black)
{
}

/**
 * @brief 构造函数
 * @param c 圆心
 * @param r 半径
 * @param startAngleDeg 起始角度（单位：°）
 * @param endAngleDeg 终止角度（单位：°）
 * @param color 颜色
 */
ArcShape::ArcShape(const QPoint& c, int r, double startAngleDeg, double endAngleDeg, const QColor& color)
    : center(c), radius(r), startAngle(startAngleDeg), endAngle(endAngleDeg), color(color)
{
}

/**
 * @brief 使用中点圆弧算法绘制圆弧
 *
 * 该算法基于中点判别函数 f(x, y) = x² + y² - r²：
 * 初始点为 (0, r)
 * 每次沿圆的八分之一弧递增 x
 * 通过判断中点是否在圆内决定是否减少 y
 *
 * 每个计算得到的点 (x, y) 可以利用八对称性生成 8 个圆上像素点。
 * 在此基础上，额外判断每个点是否处于指定角度范围内。
 */
void ArcShape::draw(DrawEngine* engine)
{
    if (!engine || radius <= 0) return;

    // 1. 初始化中点圆参数
    int x = 0;
    int y = radius;
    int d = 1 - radius;                                                 // 初始判别值 d0 = 1 - r

    bool rotateFullCircle = false;                                      // 旋转结束后圆弧是否跨越 0°
    if (startAngle > endAngle) rotateFullCircle = true;

    // 2. 角度范围准备，将角度转换为弧度
    double startRad = qDegreesToRadians(startAngle);
    double endRad = qDegreesToRadians(endAngle);

    int cx = center.x();
    int cy = center.y();

    // 辅助函数：判断点(px, py)是否落在圆弧角度范围内
    auto inArcRange = [&](int px, int py) {
        // Qt 坐标系中：y 轴向下，因此使用 atan2(py - cy, px - cx)
        double angle = atan2(py - cy, px - cx);
        if (angle < 0) angle += 2 * M_PI;                               // 保证角度为 [0, 2π)


        double start = fmod(startRad, 2 * M_PI);
        double end = fmod(endRad, 2 * M_PI);
        if (start < 0) start += 2 * M_PI;                               // 跨越 0° 时修正范围
        if (end < 0) end += 2* M_PI;

        return rotateFullCircle ? (angle >= start || angle <= end) : (angle >= start && angle <= end);
    };

    // 3. 绘制八对称点
    int step = 0;
    auto drawSymmetricPoints = [&](int x, int y) {
        int cx = center.x();
        int cy = center.y();

        // 八对称点（圆的 8 个象限）
        int points[8][2] = {
            {cx + x, cy + y}, {cx - x, cy + y},
            {cx + x, cy - y}, {cx - x, cy - y},
            {cx + y, cy + x}, {cx - y, cy + x},
            {cx + y, cy - x}, {cx - y, cy - x}
        };

        // 仅绘制处于圆弧角度范围内的点
        for (auto& p : points)
            if (inArcRange(p[0], p[1]))
            {
                engine->drawStyledPixelAtStep(p[0], p[1], color, step, lineStyle, penWidth, dashOffset);
                ++step;
            }
    };

    // 4. 主循环：中点判别递推
    while (x <= y)
    {
        // 绘制当前八分点
        drawSymmetricPoints(x, y);

        // 更新判别式并递推下一个点
        if (d < 0)
            d += 2 * x + 3;                                             // 下一个点在圆内：只增加 x
        else
        {
            // 下一个点在圆外：增加 x，减少 y
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
}

// 圆的重心就是圆心
QPointF ArcShape::centroid() const
{
    return QPointF(center);
}


