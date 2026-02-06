#include "polygonshape.h"
#include "drawengine.h"

#include <algorithm>
#include <limits>

/**
 * @brief 构造函数（方便外部直接传点）
 */
PolygonShape::PolygonShape(const std::vector<QPoint> &pts)
    : vertices(pts)
{
}

/**
 * @brief contains
 * 简单的点在多边形内判定（射线法）
 * 注意：处理共线/顶点情况比较粗糙，但对交互选择足够用。
 */
bool PolygonShape::contains(const QPoint& pt) const
{
    bool inside = false;
    int n = (int)vertices.size();
    if (n < 3) return false;

    for (int i = 0, j = n - 1; i < n; j = i++)
    {
        const QPoint &pi = vertices[i];
        const QPoint &pj = vertices[j];
        bool intersect = ((pi.y() > pt.y()) != (pj.y() > pt.y())) &&
                         (pt.x() < (pj.x() - pi.x()) * (pt.y() - pi.y()) / double(pj.y() - pi.y() + 0.0) + pi.x());
        if (intersect) inside = !inside;
    }
    return inside;
}

/**
 * @brief draw - 多边形绘制入口
 * 设计：若 filled==true，先执行扫描线填充；随后用现有 LineShape 的绘制机制描边（保持样式）。
 *
 * 注意：
 *  - 填充使用 DrawEngine::setPixel 逐像素填充（填充不受线型/线帽控制）
 *  - 描边通过调用 DrawEngine 的线段绘制（例如你当前 LineShape 所使用的 drawLine 方法）
 */
void PolygonShape::draw(DrawEngine* engine)
{
    if (!engine) return;

    int n = (int)vertices.size();
    if (n == 0) return;

    // 1) 若填充：扫描线填充（整数扫描线）
    if (filled && n >= 3)
    {
        // 找到 minY, maxY
        int minY = std::numeric_limits<int>::max();
        int maxY = std::numeric_limits<int>::min();
        for (const auto& p : vertices) {
            minY = std::min(minY, p.y());
            maxY = std::max(maxY, p.y());
        }

        struct Edge {
            int ymax;
            double x;
            double invSlope;
        };

        int height = maxY - minY + 1;
        std::vector<std::vector<Edge>> buckets(std::max(1, height));

        for (int i = 0; i < n; ++i)
        {
            const QPoint &p1 = vertices[i];
            const QPoint &p2 = vertices[(i + 1) % n];

            if (p1.y() == p2.y()) continue;

            int ymin = std::min(p1.y(), p2.y());
            int ymax = std::max(p1.y(), p2.y());
            double x_at_ymin = (p1.y() < p2.y()) ? p1.x() : p2.x();
            double dx = double(p2.x() - p1.x());
            double dy = double(p2.y() - p1.y());
            double invslope = dx / dy;

            int bucketIndex = ymin - minY;
            if (bucketIndex >= 0 && bucketIndex < (int)buckets.size())
            {
                buckets[bucketIndex].push_back({ymax, x_at_ymin, invslope});
            }
        }

        // AET
        std::vector<Edge> AET;
        AET.reserve(16);

        for (int scanY = minY; scanY <= maxY; ++scanY)
        {
            int bucketIndex = scanY - minY;
            if (bucketIndex >= 0 && bucketIndex < (int)buckets.size())
            {
                for (const Edge &e : buckets[bucketIndex])
                    AET.push_back(e);
            }

            AET.erase(std::remove_if(AET.begin(), AET.end(),
                                     [scanY](const Edge &e){ return e.ymax <= scanY; }),
                      AET.end());

            if (AET.empty()) {
                for (auto &e : AET) e.x += e.invSlope;
                continue;
            }

            std::sort(AET.begin(), AET.end(), [](const Edge &a, const Edge &b){
                return a.x < b.x;
            });

            for (size_t i = 0; i + 1 < AET.size(); i += 2)
            {
                int xStart = int(std::ceil(AET[i].x));
                int xEnd   = int(std::floor(AET[i+1].x));
                if (xStart > xEnd) continue;

                for (int x = xStart; x <= xEnd; ++x)
                    engine->setPixel(x, scanY, fillColor);
            }

            for (auto &e : AET) e.x += e.invSlope;
        }
    }

    // 2) 描边：把每条边交给 DrawEngine 或 LineShape 的 draw 实现
    //    这里直接用简单 Bresenham 从 vertex i 到 i+1，并用图元的样式（lineStyle / penWidth / color）
    for (int i = 0; i < n; ++i)
    {
        QPoint p1 = vertices[i];
        QPoint p2 = vertices[(i + 1) % n];

        int x0 = p1.x(), y0 = p1.y();
        int x1 = p2.x(), y1 = p2.y();
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
}




// 多边形重心（基于顶点多边形重心公式；若退化则取平均）
QPointF PolygonShape::centroid() const
{
    int n = vertices.size();
    if (n == 0) return QPointF(0,0);
    // 使用多边形重心公式
    double A = 0.0;
    double cx = 0.0, cy = 0.0;
    for (int i = 0, j = n - 1; i < n; j = i++)
    {
        double xi = vertices[i].x();
        double yi = vertices[i].y();
        double xj = vertices[j].x();
        double yj = vertices[j].y();
        double cross = xj * yi - xi * yj;
        A += cross;
        cx += (xj + xi) * cross;
        cy += (yj + yi) * cross;
    }
    A *= 0.5;
    if (fabs(A) < 1e-6) {
        // 退化：返回顶点平均
        double sx = 0, sy = 0;
        for (const auto &p : vertices) { sx += p.x(); sy += p.y(); }
        return QPointF(sx / n, sy / n);
    }
    cx /= (6.0 * A);
    cy /= (6.0 * A);
    return QPointF(cx, cy);
}

