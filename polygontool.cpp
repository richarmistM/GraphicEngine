#include "polygontool.h"
#include "polygonshape.h"
#include "drawengine.h"

#include <QMouseEvent>

/**
 * PolygonTool 稳健实现说明：
 * - 左键：按次添加固定顶点（push 到 tempVertices）。
 *         第一次按下会创建 previewShape 并 engine->addShape(previewShape)（只 add 一次）。
 * - 鼠标移动：若正在绘制，将预览图形的最后一点临时设置为鼠标位置并 engine->redrawShape(previewShape)。
 * - 右键：结束绘制。
 *         如果顶点 >= 3：把 previewShape 保留作为最终图形（已在 engine->shapes 中），
 *                        同时清空临时数据（previewShape 不 reset 以保留在 engine 中）。
 *         如果顶点 < 3：调用 engine->removeShape(previewShape)，释放预览对象并清理状态。
 */

PolygonTool::PolygonTool()
    : isDrawing(false)
{
}

void PolygonTool::onMousePress(QMouseEvent* e, DrawEngine* engine)
{
    if (!engine) return;

    // --------- 左键：添加固定顶点或开始新的多边形 ----------
    if (e->button() == Qt::LeftButton)
    {
        // 将新的固定顶点存入 tempVertices
        tempVertices.push_back(e->pos());

        // 若是刚开始绘制：创建 previewShape 并只 add 一次到引擎
        if (!isDrawing)
        {
            isDrawing = true;

            previewShape = std::make_shared<PolygonShape>(tempVertices);

            // 把当前画笔样式 snapshot 到 preview（保持创建时样式）
            previewShape->color = engine->getCanvas().isNull() ? Qt::black : previewShape->color; // 保证有值（可省）
            previewShape->penWidth = engine->getPenWidth();
            previewShape->lineStyle = engine->getLineStyle();
            previewShape->lineCap = engine->getLineCap();
            previewShape->dashOffset = (tempVertices.front().x() + tempVertices.front().y()) % 13;

            previewShape->filled = fillOnComplete;
            previewShape->fillColor = fillColor;
            // 默认不填充，UI 可在完成后修改这个字段
            // previewShape->filled = false;
            // previewShape->fillColor = Qt::yellow;

            // 把 preview 加入引擎（只 add 一次）
            engine->addShape(previewShape);

            // 请求引擎绘制（首次显示）
            engine->redrawShape(previewShape);
        }
        else
        {
            // 正在绘制，只更新 previewShape 的顶点（固定部分）
            previewShape->vertices = tempVertices;
            engine->redrawShape(previewShape);
        }
    }

    // --------- 右键：结束绘制（提交或取消） ----------
    else if (e->button() == Qt::RightButton && isDrawing)
    {
        if (tempVertices.size() >= 3)
        {
            // 完成：previewShape 已在 engine->shapes 中，保持即可。
            // 可在此处对 previewShape 做最终设置（颜色、filled 等），目前保持默认。
            // 清理临时状态（注意不要 remove previewShape）
            tempVertices.clear();
            previewShape.reset();
            isDrawing = false;
        }
        else
        {
            // 顶点不足 -> 取消：从 engine 中移除预览并释放
            if (previewShape)
            {
                engine->removeShape(previewShape);
                previewShape.reset();
            }
            tempVertices.clear();
            isDrawing = false;
        }
    }
}

void PolygonTool::onMouseMove(QMouseEvent* e, DrawEngine* engine)
{
    if (!engine) return;

    if (!isDrawing) return;

    // 动态预览：在 previewShape 的顶点末尾临时加上当前鼠标位置作为“橡皮筋”线段
    if (previewShape)
    {
        tempVertices.pop_back();
        tempVertices.push_back(e->pos());

        previewShape->vertices = tempVertices;
        engine->redrawShape(previewShape);
    }
}

void PolygonTool::onMouseRelease(QMouseEvent* e, DrawEngine* engine)
{
    Q_UNUSED(e);
    Q_UNUSED(engine);
    // 不需要在释放时做额外处理（按下/移动/右键完成流程即可）
}


