#ifndef SHAPE_H
#define SHAPE_H

#include <QPoint>
#include <QColor>
#include <QTransform>

enum class LineStyle {Solid, Dash, Dot, DashDot};
enum class LineCap {Flat, Square, Round};

class DrawEngine;

/**
 * @brief Shape —— 图形抽象基类
 *
 * 所有具体图形（直线、矩形、圆、多边形等）都应继承自该类，
 * 并实现其纯虚函数：
 *  - draw()：定义如何在 DrawEngine 的像素画布上绘制自己；
 *  - contains()：定义如何判断某个点是否落在该图形内部，用于选中或编辑功能。
 */
class Shape
{
public:
    virtual ~Shape() = default;

    /**
     * @brief 在 DrawEngine 的画布上绘制该图形
     * @param engine 绘图引擎指针（提供 setPixel 接口）
     */
    virtual void draw(DrawEngine* engine) = 0;

    /**
     * @brief 判断点是否在该图形范围内（用于选择功能）
     * @param pt 要检测的点坐标
     * @return true 表示点在图形内
     */
    virtual bool contains(const QPoint& pt) const = 0;

    // 返回图形重心（浮点坐标，便于变换）
    virtual QPointF centroid() const { return QPointF(0.0, 0.0); }

    QColor color = Qt::black;                                       // 绘制颜色
    int penWidth = 1;                                               // 线宽
    LineStyle lineStyle = LineStyle::Solid;                         // 线型
    LineCap lineCap = LineCap::Round;                               // 线帽
    int dashOffset = 0;
};

#endif // SHAPE_H

