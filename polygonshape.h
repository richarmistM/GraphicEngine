#ifndef POLYGONSHAPE_H
#define POLYGONSHAPE_H

#include "shape.h"
#include <vector>

/**
 * @brief PolygonShape
 * 支持：仅轮廓绘制 或 扫描线填充（填充色独立于边颜色）
 *
 * 使用说明：
 *  - vertices 存储顶点（按用户输入顺序）
 *  - filled 控制是否执行扫描线填充
 *  - fillColor 为填充颜色（默认为 color 的淡色，工具可设置）
 */
class PolygonShape : public Shape
{
public:
    PolygonShape() = default;
    explicit PolygonShape(const std::vector<QPoint> &pts);

    // 绘制：若 filled 则先填充再描边（以保证边可见）
    void draw(DrawEngine* engine) override;

    // 简单点内判定（用于选择）：使用射线法（非严格处理边界情况）
    bool contains(const QPoint& pt) const override;

    QPointF centroid() const override;


    // 顶点数据
    std::vector<QPoint> vertices;

    // 填充相关
    bool filled = false;
    QColor fillColor = Qt::white; // 默认填充色（可由工具或 UI 设置）



};

#endif // POLYGONSHAPE_H

