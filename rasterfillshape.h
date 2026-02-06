#ifndef RASTERFILLSHAPE_H
#define RASTERFILLSHAPE_H

#include "shape.h"
#include <vector>
#include <QPoint>

/**
 * @brief RasterFillShape
 * 将一次像素级填充的结果（若干像素点集合）作为一个 Shape 持久化保存。
 * draw() 会把这些像素写回 DrawEngine 的画布。
 *
 * 优点：填充效果会随着 engine 的重绘持久化显示，不会在下一次 paintEvent 中丢失。
 */
class RasterFillShape : public Shape
{
public:
    RasterFillShape() = default;
    explicit RasterFillShape(const std::vector<QPoint>& pts, const QColor &c = Qt::black)
        : pixels(pts) { color = c; }

    void draw(DrawEngine* engine) override;
    bool contains(const QPoint& pt) const override { return false; }

    std::vector<QPoint> pixels;
};

#endif // RASTERFILLSHAPE_H
