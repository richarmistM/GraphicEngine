#include "beziershape.h"
#include "drawengine.h"
#include <cmath>
#include <algorithm>

/**
 * @brief de Casteljau 运算：用 controlPoints 计算 t 处点
 * @param pts 控制点（QPointF）
 * @param t 取值范围 [0,1]
 * @return 曲线上对应的 QPointF
 */
static QPointF evalBezier(const std::vector<QPointF>& pts, double t)
{
    int n = (int)pts.size();
    if (n == 0) return QPointF(0,0);
    if (n == 1) return pts[0];

    // 使用一个临时数组（拷贝），原地递归降阶
    std::vector<QPointF> tmp = pts;
    for (int r = 1; r < n; ++r) {
        for (int i = 0; i < n - r; ++i) {
            tmp[i].setX( (1.0 - t) * tmp[i].x() + t * tmp[i+1].x() );
            tmp[i].setY( (1.0 - t) * tmp[i].y() + t * tmp[i+1].y() );
        }
    }
    return tmp[0];
}

/**
 * @brief 在整数像素坐标上画一条 Bresenham 线段
 * 使用 engine->drawStyledPixelAtStep 来绘制每个像素（保留线型/线宽）
 *
 * 说明：
 * - 这个实现与 LineShape::draw 的逻辑保持一致（逐像素递增 step）
 * - x0,y0,x1,y1 是整数像素（四舍五入）
 */
static void drawSegmentStyled(DrawEngine* engine, int x0, int y0, int x1, int y1,
                              const QColor &color, int &step,
                              LineStyle style, int width, int dashOffset)
{
    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    while (true)
    {
        engine->drawStyledPixelAtStep(x0, y0, color, step, style, width, dashOffset);
        ++step;
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void BezierShape::draw(DrawEngine* engine)
{
    if (!engine) return;
    int npts = (int)controlPoints.size();
    if (npts < 2) return; // 至少需要两点才有曲线（退化为点/线）

    // 采样点数：依据控制点个数决定（保证精细度）
    // 经验值：samples = max(50, 50 * npts)
    int samples = std::max(50, 50 * npts);

    // 生成第一点
    QPointF prev = evalBezier(controlPoints, 0.0);
    int step = 0; // 用于虚线判定（曲线上连续像素的“步数”）
    for (int i = 1; i <= samples; ++i)
    {
        double t = double(i) / samples;
        QPointF pf = evalBezier(controlPoints, t);

        // 把浮点坐标四舍五入为像素坐标进行 Bresenham 段绘制
        int x0 = int(std::round(prev.x()));
        int y0 = int(std::round(prev.y()));
        int x1 = int(std::round(pf.x()));
        int y1 = int(std::round(pf.y()));

        drawSegmentStyled(engine, x0, y0, x1, y1, color, step, lineStyle, penWidth, dashOffset);

        prev = pf;
    }
}

/**
 * @brief contains: 判断点是否靠近控制点或靠近曲线（用于选中）
 * - 优先判断控制点（半径 6 像素）
 * - 否则对曲线进行采样判断，若任意采样点距离 <= 4 则认为命中
 */
bool BezierShape::contains(const QPoint& pt) const
{
    // 容差与阈值：基于线宽自动放大，保证粗线更容易被选中
    const int CTRL_RADIUS_BASE = 6;                         // 控制点拾取基础半径
    const int CURVE_TOL_BASE = 4;                           // 曲线命中基础容差
    int tol = std::max(CURVE_TOL_BASE, penWidth + 1);       // 点到曲线距离阈值（像素）
    int ctrlRad = std::max(CTRL_RADIUS_BASE, penWidth + 2); // 控制点检测半径

    // 1) 若控制点靠近，优先命中（便于拖拽编辑控制点）
    for (size_t i = 0; i < controlPoints.size(); ++i)
    {
        double dx = controlPoints[i].x() - pt.x();
        double dy = controlPoints[i].y() - pt.y();
        if (dx*dx + dy*dy <= double(ctrlRad)*double(ctrlRad)) return true;
    }

    // 2) 包围盒快速拒绝：计算 control polygon 的 bbox 并扩展 tol
    if (controlPoints.empty()) return false;
    double minx = controlPoints[0].x(), maxx = controlPoints[0].x();
    double miny = controlPoints[0].y(), maxy = controlPoints[0].y();
    for (const auto &p : controlPoints)
    {
        if (p.x() < minx) minx = p.x();
        if (p.x() > maxx) maxx = p.x();
        if (p.y() < miny) miny = p.y();
        if (p.y() > maxy) maxy = p.y();
    }
    minx -= tol; maxx += tol; miny -= tol; maxy += tol;
    if (pt.x() < minx || pt.x() > maxx || pt.y() < miny || pt.y() > maxy)
        return false; // 明显在外面，快速拒绝

    // 3) 自适应采样数量估计：
    //    用控制多边形近似周长作为曲线长度近似
    double approxLen = 0.0;
    for (size_t i = 1; i < controlPoints.size(); ++i) {
        double dx = controlPoints[i].x() - controlPoints[i-1].x();
        double dy = controlPoints[i].y() - controlPoints[i-1].y();
        approxLen += std::hypot(dx, dy);
    }
    // baseline samples，随控制点数和近似长度增长
    int npts = (int)controlPoints.size();
    int samples = std::max(60, int(std::round(approxLen * 0.5)) ); // 经验系数
    samples = std::min(samples, 2000); // 限制上界，防止过多开销
    // 最低采样量与控制点数相关，避免 npts 很大时样本过小
    samples = std::max(samples, 50 * std::max(1, npts));

    // 4) 对曲线按 samples 采样，检查每一小段与 pt 的最短距离
    QPointF prev = evalBezier(controlPoints, 0.0);
    for (int i = 1; i <= samples; ++i)
    {
        double t = double(i) / samples;
        QPointF cur = evalBezier(controlPoints, t);

        // 计算点 pt 到线段 prev-cur 的最短距离（投影法）
        double x0 = prev.x(), y0 = prev.y();
        double x1 = cur.x(),  y1 = cur.y();
        double x  = pt.x(),   y  = pt.y();

        double dx = x1 - x0, dy = y1 - y0;
        double len2 = dx*dx + dy*dy;
        if (len2 < 1e-9) {
            // 段退化为点
            double ddx = x - x0, ddy = y - y0;
            if (ddx*ddx + ddy*ddy <= double(tol)*double(tol)) return true;
        } else {
            double proj = ((x - x0) * dx + (y - y0) * dy) / len2;
            if (proj < 0.0) {
                double ddx = x - x0, ddy = y - y0;
                if (ddx*ddx + ddy*ddy <= double(tol)*double(tol)) return true;
            } else if (proj > 1.0) {
                double ddx = x - x1, ddy = y - y1;
                if (ddx*ddx + ddy*ddy <= double(tol)*double(tol)) return true;
            } else {
                double px = x0 + proj * dx;
                double py = y0 + proj * dy;
                double ddx = x - px, ddy = y - py;
                if (ddx*ddx + ddy*ddy <= double(tol)*double(tol)) return true;
            }
        }

        prev = cur;
    }

    // 5) 没命中
    return false;
}


