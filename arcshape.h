#ifndef ARCSHAPE_H
#define ARCSHAPE_H

#include "shape.h"
#include <QPoint>
#include <QColor>

/**
 * @brief 圆弧图元类（基于中点圆弧算法）
 *
 * 使用整数运算的中点圆弧算法绘制圆弧像素点，可控制起止角度
 * 与中点画圆算法原理相同
 * 在每个八分圆对称区中仅绘制处于角度范围内的像素
 */
class ArcShape : public Shape
{
public:
    ArcShape();
    ArcShape(const QPoint& c, int r, double startAngleDeg, double endAngleDeg, const QColor& color = Qt::black);

    /**
     * @brief 绘制圆弧（调用中点圆弧算法）
     */
    void draw(DrawEngine* engine) override;

    /**
     * @brief 判断某点是否在图形中（本实验未使用）
     */
    bool contains(const QPoint& pt) const override { return false; }


    QPointF centroid() const override;

    QPoint center;                                              // 圆心坐标
    int radius;                                                 // 半径
    double startAngle;                                          // 起始角度（单位：°， 0°在右侧，顺时针方向增加）
    double endAngle;                                            // 终止角度（单位：°）
    QColor color;                                               // 绘制颜色
};

#endif // ARCSHAPE_H
