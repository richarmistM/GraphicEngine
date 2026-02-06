#include "lineshape.h"
#include "drawengine.h"

/**
 * @brief 使用 Bresenham 算法绘制一条直线
 *
 * 整个算法完全基于整数计算
 * 只使用加减法和比较操作
 * 避免浮点数计算，提高性能
 *
 * 每次迭代计算下一个像素点，并通过 DrawEngine::drawStyledPixelAtStep 写入画布
 */
void LineShape::draw(DrawEngine* engine)
{
    if (!engine) return;

    int x0 = start.x();
    int y0 = start.y();
    int x1 = end.x();
    int y1 = end.y();

    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    int step = 0;

    while (true)
    {
        engine->drawStyledPixelAtStep(x0, y0, color, step, lineStyle, penWidth, dashOffset);
        ++step;
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}


/**
 * @brief 判断点是否在直线段上（用于选中判断）
 *
 * 算法：
 * 计算点到直线段的最短距离
 * 若距离 < 容差（例如2像素），则认为“在直线上”
 * 用投影+叉积方法实现，避免除0错误
 */
bool LineShape::contains(const QPoint& pt) const
{
    double x0 = start.x(), y0 = start.y();
    double x1 = end.x(),   y1 = end.y();
    double x  = pt.x(),    y  = pt.y();

    double dx = x1 - x0;
    double dy = y1 - y0;

    double len2 = dx*dx + dy*dy;
    if (len2 < 1e-6) return false;                                  // 起点终点重合

    // 点到直线距离（叉积法）
    double dist = std::abs((x - x0)*dy - (y - y0)*dx) / std::sqrt(len2);

    // 点的投影在线段范围内
    double t = ((x - x0)*dx + (y - y0)*dy) / len2;
    if (t < 0.0 || t > 1.0) return false;

    return dist <= 2.0;                                             // 容差 2 像素
}


// centroid：线段重心（中点）
QPointF LineShape::centroid() const
{
    return QPointF( (start.x() + end.x()) * 0.5, (start.y() + end.y()) * 0.5 );
}


