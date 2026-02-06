#ifndef DRAWENGINE_H
#define DRAWENGINE_H

#include <QImage>
#include <QColor>
#include <vector>
#include <memory>
#include "shape.h"
#include "rasterfillshape.h"

class Shape;

/**
 * @brief DrawEngine —— 绘图引擎核心类
 * 负责所有“像素级”的绘图操作
 * 所有算法（直线、圆、多边形等）都会调用这里的 setPixel 来操作画布
 * 画布本质上是一张 QImage，直接往它写像素
 */
class DrawEngine
{
public:
    // 构造函数：创建一个指定宽高的画布，并填充背景色
    DrawEngine(int width = 800, int height = 600, const QColor& bgColor = Qt::white);

    // 清空画布（全部填充为背景色）
    void clear(const QColor& color = Qt::white);

    // 清空画布像素并移除所有 Shape 对象
    void clearAllShapes();

    // 低级像素绘制函数，仅在有效范围内设置一个像素颜色
    void setPixel(int x, int y, const QColor& color = Qt::black);

    // 重绘指定图元（Shape）
    void redrawShape(std::shared_ptr<Shape> s);

    // 调整画布尺寸
    void resizeCanvas(int w, int h, const QColor& bg = Qt::white);

    // 设置画笔宽度
    void setPenWidth(int value);

    // 获取画笔宽度
    int getPenWidth() const;

    // 设置线型样式
    void setLineStyle(const QString &text);

    // 获取线型样式
    LineStyle getLineStyle() const;

    // 设置线帽样式
    void setLineCap(const QString &text);

    // 获取线帽样式
    LineCap getLineCap() const;

    // 返回当前画布 QImage，用于在 CanvasWidget 中显示
    const QImage& getCanvas() const;

    // 添加图元对象到 DrawEngine 的列表
    void addShape(std::shared_ptr<Shape> s);

    //
    // 从 shapes 列表中移除某个图元（按 shared_ptr 匹配）
    // 如果找到并移除返回 true，否则返回 false
    bool removeShape(const std::shared_ptr<Shape>& s);

    // 返回所有图元对象（只读）
    const std::vector<std::shared_ptr<Shape>>& getShapes() const;

    // 模拟笔宽的“加粗像素绘制”
    void drawThickPixel(int x, int y, const QColor &color, int width);

    bool shouldDrawAtStep(int step, LineStyle style, int width, int offset) const;

    // 在绘图算法的“第 step 步”调用，根据线型判断是否应画，并按线宽绘制
    void drawStyledPixelAtStep(int x, int y, const QColor& color,
                               int step, LineStyle style, int width, int offset);

    // 扫描线（非递归）连通区域填充：
    // 从种子点 (sx,sy) 开始，填充与该点颜色相同的连通区域，填充色为 fillColor。
    // 返回被创建并填充像素的 RasterFillShape（若返回 nullptr，表示不需要添加/填充失败）
    std::shared_ptr<RasterFillShape> floodFillAddShape(int sx, int sy, const QColor& fillColor);

    // 截断/裁剪相关函数
    // 对直线段使用 Cohen-Sutherland 裁剪到矩形 [xmin, ymin] - [xmax, ymax]
    // 返回 true 表示线段在裁剪窗口内（并且 x0,y0,x1,y1 被更新为裁剪后的端点）
    // 返回 false 表示线段完全被裁掉
    bool cohenSutherlandClip(int &x0, int &y0, int &x1, int &y1, int xmin, int ymin, int xmax, int ymax) const;

    // 对任意多边形使用 Sutherland-Hodgman 对矩形做裁剪
    // 返回裁剪后的顶点列表（可能为空）
    std::vector<QPoint> clipPolygonWithRect(const std::vector<QPoint>& poly, int xmin, int ymin, int xmax, int ymax) const;


private:
    int penWidth;                                                       // 线宽
    LineStyle lineStyle;                                                // 线型
    LineCap lineCap;                                                    // 线帽
    QImage canvas;                                                      // 内存画布（像素矩阵）
    std::vector<std::shared_ptr<Shape>> shapes;                         // 当前所有图形对象

    //int dashCounter = 0;
};

#endif // DRAWENGINE_H
