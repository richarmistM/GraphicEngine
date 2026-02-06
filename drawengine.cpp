#include "drawengine.h"
#include "shape.h"

#include <stack>
#include <vector>


/**
 * @brief 各种线型（pattern）定义
 * 用整数数组定义“画/不画”的节奏：
 * 1 表示画，0 表示跳过
 */
static const int SOLID_PATTERN[]   = {1};                                       // 实线：始终画
static const int SOLID_LEN = 1;
static const int DASH_PATTERN[]    = {1,1,1,1,1,1,0,0,0,0};                     // 虚线：6画 + 4空
static const int DOT_PATTERN[]     = {1,0,0};                                   // 点线：1画 + 2空
static const int DASHDOT_PATTERN[] = {1,1,1,1,1,1,0,0,1,0,0};                   // 点划线：6画+2空+1画+2空

DrawEngine::DrawEngine(int width, int height, const QColor& bgColor)
    : penWidth(1),
    lineStyle(LineStyle::Solid),
    lineCap(LineCap::Round),
    canvas(width, height, QImage::Format_RGB32)
{
    canvas.fill(bgColor);                                                       // 初始化背景色
}

/**
 * @brief 清空画布（全部填充为背景色）
 * @param color
 */
void DrawEngine::clear(const QColor& color)
{
    canvas.fill(color);
}

void DrawEngine::clearAllShapes()
{
    // 释放所有 shared_ptr 管理的图元对象（vector::clear 会释放）
    shapes.clear();

    // 清空像素画布（恢复背景色）
    clear();
}

/**
 * @brief 低级像素绘制函数，仅在有效范围内设置一个像素颜色
 * @param x
 * @param y
 * @param color
 */
void DrawEngine::setPixel(int x, int y, const QColor& color)
{
    // 边界检查：防止访问越界
    if (x < 0 || y < 0 || x >= canvas.width() || y >= canvas.height())
        return;

    canvas.setPixelColor(x, y, color);                                          // 直接设置像素颜色（Qt 提供的低级接口）
}

/**
 * @brief 重绘指定图元（Shape）
 * - 调用图元的 draw() 函数
 * @param s
 */
void DrawEngine::redrawShape(std::shared_ptr<Shape> s)
{
    if (!s) return;
    //dashCounter = 0;
    s->draw(this);
}

/**
 * @brief 调整画布尺寸
 * - 一般在窗口 resize 时使用
 * @param w
 * @param h
 * @param bg
 */
void DrawEngine::resizeCanvas(int w, int h, const QColor& bg)
{
    QImage newCanvas(w, h, QImage::Format_RGB32);
    newCanvas.fill(bg);
    canvas = newCanvas;
}

/**
 * @brief 设置画笔宽度
 * @param value
 */
void DrawEngine::setPenWidth(int value)
{
    penWidth = std::max(1, value);                                              // 防止 width < 1
}

/**
 * @brief 获取画笔宽度
 * @return
 */
int DrawEngine::getPenWidth() const
{
    return penWidth;
}

/**
 * @brief 设置线型样式
 * @param text 对应 UI 下拉框文本
 */
void DrawEngine::setLineStyle(const QString &text)
{
    if (text == "Solid")
        lineStyle = LineStyle::Solid;
    else if (text == "Dash")
        lineStyle = LineStyle::Dash;
    else if (text == "Dot")
        lineStyle = LineStyle::Dot;
    else if (text == "DashDot")
        lineStyle = LineStyle::DashDot;
}

/**
 * @brief 获取线型样式
 * @return
 */
LineStyle DrawEngine::getLineStyle() const
{
    return lineStyle;
}

/**
 * @brief 设置线帽样式
 * - “Flat”   平头（直接截止）
 * - “Square” 方头（稍向外延伸）
 * - “Round”  圆头（以圆形收尾）
 * @param text 对应 UI 下拉框文本
 */
void DrawEngine::setLineCap(const QString &text)
{
    if (text == "Flat")
        lineCap = LineCap::Flat;
    else if (text == "Square")
        lineCap = LineCap::Square;
    else if (text == "Round")
        lineCap = LineCap::Round;
}

/**
 * @brief 获取线帽样式
 * @return
 */
LineCap DrawEngine::getLineCap() const
{
    return lineCap;
}

/**
 * @brief 返回当前画布 QImage，用于在 CanvasWidget 中显示
 * @return
 */
const QImage& DrawEngine::getCanvas() const
{
    return canvas;
}

/**
 * @brief 添加图元对象到 DrawEngine 的列表
 * - 统一管理所有 Shape 的绘制
 * @param s
 */
void DrawEngine::addShape(std::shared_ptr<Shape> s)
{
    if (s)
    {
        //dashCounter = 0;
        shapes.push_back(s);
    }
}

// 从 shapes 向量中移除第一个与 s 相等的 shared_ptr。
// 返回 true 表示成功移除；false 表示未找到。
bool DrawEngine::removeShape(const std::shared_ptr<Shape>& s)
{
    if (!s) return false;

    auto it = std::find_if(shapes.begin(), shapes.end(),
                           [&s](const std::shared_ptr<Shape>& elem) {
                               return elem == s; // 比较 shared_ptr 管理的相同对象
                           });
    if (it != shapes.end())
    {
        shapes.erase(it);
        return true;
    }
    return false;
}

/**
 * @brief 返回所有图元对象（只读）
 * @return
 */
const std::vector<std::shared_ptr<Shape>>& DrawEngine::getShapes() const
{
    return shapes;
}

/**
 * @brief 模拟笔宽的“加粗像素绘制”
 * 方法：在当前点周围按圆形范围扩散绘制
 * @param x
 * @param y
 * @param color
 * @param width
 */
void DrawEngine::drawThickPixel(int x, int y, const QColor &color, int width)
{
    int r = std::max(1, width / 2);                                             // 半径 = 线宽的一半
    for (int dy = -r; dy <= r; ++dy)
    {
        for (int dx = -r; dx <= r; ++dx)
        {
            // 若在圆内（使用 r^2 检测）
            if (dx * dx + dy * dy <= r * r)
            {
                setPixel(x + dx, y + dy, color);
            }
        }
    }
}

/**
 * @brief 判断第 step 步是否应该绘制（虚线节奏控制）
 * @param step 当前步数
 * @param style 当前线型
 * @param width 当前线宽
 * @param offset 线型偏移（动画或同步偏移）
 * @return
 */
bool DrawEngine::shouldDrawAtStep(int step, LineStyle style, int width, int offset) const
{
    const int* pat = SOLID_PATTERN;
    int plen = SOLID_LEN;

    // 根据 lineStyle 选择对应 pattern 数组
    switch(style)
    {
        case LineStyle::Solid:   pat = SOLID_PATTERN;   plen = SOLID_LEN; break;
        case LineStyle::Dash:    pat = DASH_PATTERN;    plen = 10;        break;
        case LineStyle::Dot:     pat = DOT_PATTERN;     plen = 3;         break;
        case LineStyle::DashDot: pat = DASHDOT_PATTERN; plen = 11;        break;
    }

    int idx = (step / width + offset) % plen;                                   // 取当前节奏位置
    return (pat[idx] != 0);                                                     // 返回 pattern[index] 是否为 1
}

/**
 * @brief 在绘图算法的“第 step 步”调用，根据线型判断是否应画，并按线宽绘制
 * @param x
 * @param y
 * @param color
 * @param step 当前步数
 * @param style 当前线型
 * @param width 当前线宽
 * @param offset 线型偏移（动画或同步偏移）
 */
void DrawEngine::drawStyledPixelAtStep(int x, int y, const QColor& color,
                                       int step, LineStyle style, int width, int offset)
{
    // 若该步在 pattern 上对应“画”的区段，则绘制加粗像素
    if (shouldDrawAtStep(step, style, width, offset))
        drawThickPixel(x, y, color, width);
}

/**
 * @brief DrawEngine::floodFillAddShape
 *
 * 实现要点（扫描线 flood-fill，非递归）：
 * 1. 读取画布上种子点的颜色 targetColor（使用 canvas.pixelColor）
 * 2. 如果 targetColor == fillColor，直接返回 nullptr（不需要填充）
 * 3. 使用 visited 数组记录已处理像素（大小 width*height）
 * 4. 用 stack 保存需要扩展的像素种子（以 x,y 表示）
 * 5. 每次弹出一个种子 (x,y)，往左/右扫描得到 [xl..xr] 区间，标记并把这些像素加入结果像素列表
 * 6. 对区间上下两行，检测未处理且颜色 == targetColor 的子区间，压入 stack 的种子（其起点）
 * 7. 最终把收集到的像素数组放入 RasterFillShape 并返回（调用者会 addShape）
 */
std::shared_ptr<RasterFillShape> DrawEngine::floodFillAddShape(int sx, int sy, const QColor& fillColor)
{
    int w = canvas.width();
    int h = canvas.height();
    if (sx < 0 || sy < 0 || sx >= w || sy >= h) return nullptr;

    QColor target = canvas.pixelColor(sx, sy);
    if (target == fillColor) return nullptr; // 无需填充

    std::vector<unsigned char> visited(w * h, 0);

    auto index = [&](int x, int y) { return y * w + x; };

    std::vector<QPoint> filledPixels;
    filledPixels.reserve(1024); // 预留一点空间，避免频繁 realloc

    std::stack<std::pair<int,int>> st;
    st.push({sx, sy});

    while (!st.empty())
    {
        auto pr = st.top(); st.pop();
        int x = pr.first;
        int y = pr.second;

        // 如果已访问或颜色不同则跳过
        if (x < 0 || x >= w || y < 0 || y >= h) continue;
        if (visited[index(x,y)]) continue;
        if (canvas.pixelColor(x,y) != target) continue;

        // 扩展到当前扫描线的最左和最右
        int xl = x;
        while (xl - 1 >= 0 && !visited[index(xl-1,y)] && canvas.pixelColor(xl-1,y) == target) xl--;
        int xr = x;
        while (xr + 1 < w && !visited[index(xr+1,y)] && canvas.pixelColor(xr+1,y) == target) xr++;

        // 将 [xl..xr] 标记为已访问并加入结果
        for (int xi = xl; xi <= xr; ++xi)
        {
            visited[index(xi,y)] = 1;
            filledPixels.emplace_back(xi, y);
        }

        // 检查上一行（y-1）在 [xl..xr] 的子区间
        if (y - 1 >= 0)
        {
            int xi = xl;
            while (xi <= xr)
            {
                // 跳过非目标颜色或已访问
                while (xi <= xr && (visited[index(xi,y-1)] || canvas.pixelColor(xi,y-1) != target)) xi++;
                if (xi > xr) break;
                int segStart = xi;
                while (xi <= xr && !visited[index(xi,y-1)] && canvas.pixelColor(xi,y-1) == target) xi++;
                st.push({segStart, y-1});
            }
        }

        // 检查下一行（y+1）在 [xl..xr] 的子区间
        if (y + 1 < h)
        {
            int xi = xl;
            while (xi <= xr)
            {
                while (xi <= xr && (visited[index(xi,y+1)] || canvas.pixelColor(xi,y+1) != target)) xi++;
                if (xi > xr) break;
                int segStart = xi;
                while (xi <= xr && !visited[index(xi,y+1)] && canvas.pixelColor(xi,y+1) == target) xi++;
                st.push({segStart, y+1});
            }
        }
    }

    if (filledPixels.empty()) return nullptr;

    // 创建 RasterFillShape 并返回（调用者负责 addShape）
    std::shared_ptr<RasterFillShape> rs = std::make_shared<RasterFillShape>(filledPixels, fillColor);
    // 把 shape 的绘制属性设置合理值（若需要）
    rs->penWidth = 1;
    rs->lineStyle = LineStyle::Solid;
    rs->lineCap = LineCap::Flat;
    return rs;
}



// --------------------------- Cohen-Sutherland 定义 ---------------------------
// Outcode 位掩码
static const int CS_INSIDE = 0; // 0000
static const int CS_LEFT   = 1; // 0001
static const int CS_RIGHT  = 2; // 0010
static const int CS_BOTTOM = 4; // 0100
static const int CS_TOP    = 8; // 1000

// 计算 outcode
static int computeOutCode(int x, int y, int xmin, int ymin, int xmax, int ymax)
{
    int code = CS_INSIDE;
    if (x < xmin) code |= CS_LEFT;
    else if (x > xmax) code |= CS_RIGHT;
    if (y < ymin) code |= CS_TOP;    // 注意：Qt y 向下，约定这里 top 为 y < ymin
    else if (y > ymax) code |= CS_BOTTOM;
    return code;
}

/**
 * Cohen-Sutherland 裁剪
 * 输入/输出：x0,y0,x1,y1 (像素整数)
 * 窗口：xmin,ymin,xmax,ymax
 * 返回：true => 裁剪后线段仍存在（端点通过引用返回）
 *       false => 线段完全在窗口外被丢弃
 */
bool DrawEngine::cohenSutherlandClip(int &x0, int &y0, int &x1, int &y1, int xmin, int ymin, int xmax, int ymax) const
{
    // 复制为 double 用于计算交点（保持精度）
    double x0d = x0, y0d = y0, x1d = x1, y1d = y1;

    int out0 = computeOutCode((int)std::round(x0d), (int)std::round(y0d), xmin, ymin, xmax, ymax);
    int out1 = computeOutCode((int)std::round(x1d), (int)std::round(y1d), xmin, ymin, xmax, ymax);

    bool accept = false;

    while (true)
    {
        if ((out0 | out1) == 0) {
            // 两端点都在窗口内：完全接受
            accept = true;
            break;
        } else if (out0 & out1) {
            // 两端共享一个“外部位”，线段在同一侧，完全丢弃
            break;
        } else {
            // 部分相交：选择一个在外的端点来裁剪
            int outcodeOut = out0 ? out0 : out1;
            double x = 0.0, y = 0.0;

            // 计算与边的交点（处理水平/垂直）
            if (outcodeOut & CS_TOP) {
                // 与 ymin (top) 相交 (注意坐标系)
                x = x0d + (x1d - x0d) * ( (double)ymin - y0d ) / (y1d - y0d);
                y = ymin;
            } else if (outcodeOut & CS_BOTTOM) {
                x = x0d + (x1d - x0d) * ( (double)ymax - y0d ) / (y1d - y0d);
                y = ymax;
            } else if (outcodeOut & CS_RIGHT) {
                y = y0d + (y1d - y0d) * ( (double)xmax - x0d ) / (x1d - x0d);
                x = xmax;
            } else if (outcodeOut & CS_LEFT) {
                y = y0d + (y1d - y0d) * ( (double)xmin - x0d ) / (x1d - x0d);
                x = xmin;
            }

            // 把交点替换掉外部端点
            if (outcodeOut == out0) {
                x0d = x; y0d = y;
                out0 = computeOutCode((int)std::round(x0d), (int)std::round(y0d), xmin, ymin, xmax, ymax);
            } else {
                x1d = x; y1d = y;
                out1 = computeOutCode((int)std::round(x1d), (int)std::round(y1d), xmin, ymin, xmax, ymax);
            }
        }
    }

    if (accept) {
        x0 = (int)std::round(x0d);
        y0 = (int)std::round(y0d);
        x1 = (int)std::round(x1d);
        y1 = (int)std::round(y1d);
        return true;
    } else {
        return false;
    }
}

/* ---------------- Sutherland-Hodgman 多边形与矩形裁剪 ----------------
 * 对输入多边形 poly（顶点按顺序）按矩形四条边顺序裁剪（左、右、top,bottom）
 * 返回裁剪后的多边形顶点（可能为空）
 */
static bool insideRect(const QPoint &p, int edge, int xmin, int ymin, int xmax, int ymax)
{
    // edge: 0=left,1=right,2=top,3=bottom
    switch(edge) {
    case 0: return p.x() >= xmin;
    case 1: return p.x() <= xmax;
    case 2: return p.y() >= ymin;
    case 3: return p.y() <= ymax;
    }
    return true;
}

// 交点计算：line AB 与 指定矩形边 edge 的交点
static QPoint intersectEdge(const QPoint &A, const QPoint &B, int edge, int xmin, int ymin, int xmax, int ymax)
{
    double x1 = A.x(), y1 = A.y(), x2 = B.x(), y2 = B.y();
    double x=0, y=0;
    double dx = x2 - x1, dy = y2 - y1;

    switch(edge) {
    case 0:
        if (fabs(dx) < 1e-9) { x = xmin; y = y1; }
        else {
            double t = (xmin - x1) / dx;
            x = xmin; y = y1 + t * dy;
        }
        break;
    case 1:
        if (fabs(dx) < 1e-9) { x = xmax; y = y1; }
        else {
            double t = (xmax - x1) / dx;
            x = xmax; y = y1 + t * dy;
        }
        break;
    case 2:
        if (fabs(dy) < 1e-9) { x = x1; y = ymin; }
        else {
            double t = (ymin - y1) / dy;
            y = ymin; x = x1 + t * dx;
        }
        break;
    case 3:
        if (fabs(dy) < 1e-9) { x = x1; y = ymax; }
        else {
            double t = (ymax - y1) / dy;
            y = ymax; x = x1 + t * dx;
        }
        break;
    }
    return QPoint((int)std::round(x), (int)std::round(y));
}

std::vector<QPoint> DrawEngine::clipPolygonWithRect(const std::vector<QPoint>& poly, int xmin, int ymin, int xmax, int ymax) const
{
    std::vector<QPoint> output = poly;
    if (output.empty()) return {};

    // 顺序裁剪四条边：left(0), right(1), top(2), bottom(3)
    for (int edge = 0; edge < 4; ++edge)
    {
        std::vector<QPoint> input = std::move(output);
        output.clear();
        if (input.empty()) break;

        QPoint S = input.back();
        for (const QPoint &E : input)
        {
            bool Ein = insideRect(E, edge, xmin, ymin, xmax, ymax);
            bool Sin = insideRect(S, edge, xmin, ymin, xmax, ymax);

            if (Sin && Ein) {
                output.push_back(E);
            } else if (Sin && !Ein) {
                QPoint ip = intersectEdge(S, E, edge, xmin, ymin, xmax, ymax);
                output.push_back(ip);
            } else if (!Sin && Ein) {
                QPoint ip = intersectEdge(S, E, edge, xmin, ymin, xmax, ymax);
                output.push_back(ip);
                output.push_back(E);
            }
            S = E;
        }
    }
    return output;
}

