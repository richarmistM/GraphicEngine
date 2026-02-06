#ifndef LINESHAPE_H
#define LINESHAPE_H

#include "shape.h"

/**
 * @brief LineShape —— 直线图元类
 *
 * 使用 Bresenham 算法绘制直线，完全在像素级实现
 */
class LineShape : public Shape
{
public:
    QPoint start;                                               // 起点
    QPoint end;                                                 // 终点

    /**
     * @brief 使用 Bresenham 算法绘制直线
     * @param engine 绘图引擎，用于绘制像素
     */
    void draw(DrawEngine* engine) override;

    /**
     * @brief 判断某个点是否在直线段附近
     * @param pt 待检测点
     * @return true 表示该点在直线附近（容差2像素）
     */
    bool contains(const QPoint& pt) const override;


    QPointF centroid() const override;
};

#endif // LINESHAPE_H

