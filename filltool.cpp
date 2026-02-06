#include "filltool.h"
#include "drawengine.h"
#include <QMouseEvent>
#include <QDebug>

FillTool::FillTool()
    : fillColor(Qt::yellow) // 默认填充色
{
}

void FillTool::onMousePress(QMouseEvent* e, DrawEngine* engine)
{
    if (!engine) return;
    if (e->button() != Qt::LeftButton) return;

    int x = e->pos().x();
    int y = e->pos().y();

    // 调用 DrawEngine 的填充函数，获得一个 RasterFillShape（若成功）
    auto rs = engine->floodFillAddShape(x, y, fillColor);
    if (rs)
    {
        engine->addShape(rs);    // 把填充结果作为永久图元加入 engine
        engine->redrawShape(rs); // 可选：立即把它绘到当前 canvas（下一帧也会重绘）
    }
    else
    {
        // fill 返回 nullptr：要么种子点颜色==fillColor，要么超出范围/无效
        qDebug() << "FillTool: no fill performed at" << x << y;
    }
}
