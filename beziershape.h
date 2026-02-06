#ifndef BEZIERSHAPE_H
#define BEZIERSHAPE_H

#include "shape.h"
#include <vector>
#include <QPointF>

/**
 * @brief BezierShape
 * 表示任意阶 Bezier 曲线（通过若干控制点定义）
 *
 * 控件：
 * - controlPoints: 控制点（QPointF）——允许非整数位置，绘制时会四舍五入到像素
 * - color / penWidth / lineStyle / lineCap / dashOffset 从 Shape 继承
 *
 * 绘制：使用 de Casteljau 算法按均匀 t 采样生成一系列点，
 *       然后用 Bresenham 在相邻采样点间画像素段（以保留虚线和线宽等特性）。
 */
class BezierShape : public Shape
{
public:
    BezierShape() = default;
    explicit BezierShape(const std::vector<QPointF>& pts) : controlPoints(pts) {}

    void draw(DrawEngine* engine) override;
    bool contains(const QPoint& pt) const override;

    std::vector<QPointF> controlPoints;   // 控制点（可为 QPointF）
    QColor fillColor;
};

#endif // BEZIERSHAPE_H

