#include "beziertool.h"
#include "beziershape.h"
#include "drawengine.h"
#include <QMouseEvent>
#include <QPainter>
#include <cmath>

BezierTool::BezierTool()
    : current(nullptr), isDrawing(false), draggingPoint(false), draggingIndex(-1)
{
}

void BezierTool::onMousePress(QMouseEvent* e, DrawEngine* engine)
{
    if (!engine) return;
    if (e->button() == Qt::LeftButton)
    {
        QPoint p = e->pos();

        // 若当前处于绘制中：添加控制点（或开始第一个）
        if (!isDrawing)
        {
            current = std::make_shared<BezierShape>();
            current->controlPoints.clear();
            current->controlPoints.push_back(QPointF(p));
            current->color = Qt::black;
            current->penWidth = engine->getPenWidth();
            current->lineStyle = engine->getLineStyle();
            current->lineCap = engine->getLineCap();
            current->dashOffset = (p.x() + p.y()) % 13;

            engine->addShape(current);
            isDrawing = true;
        }
        else
        {
            // 继续添加固定控制点
            current->controlPoints.push_back(QPointF(p));
        }

        engine->redrawShape(current);
    }
    else if (e->button() == Qt::RightButton)
    {
        // 右键：结束绘制（如果在绘制）
        if (isDrawing && current)
        {
            if (current->controlPoints.size() < 2)
            {
                // 顶点不足 -> 取消
                engine->removeShape(current);
            } else {
                // keep the curve as-is, completed
            }
            current.reset();
            isDrawing = false;
        }
        // 右键也可用于取消当前编辑拖拽
        draggingPoint = false;
        draggingIndex = -1;
    }
}

void BezierTool::onMouseMove(QMouseEvent* e, DrawEngine* engine)
{
    if (!engine) return;

    QPoint p = e->pos();

    // 如果处在绘制模式并有至少一个点：更新最后一个点为当前鼠标位置（橡皮筋）
    if (isDrawing && current && !current->controlPoints.empty())
    {
        // 临时更新最后一个点（如果刚添加则更新；如果用户想要点击精确点，按住不动再右键结束）
        current->controlPoints.back() = QPointF(p);
        engine->redrawShape(current);
        return;
    }

    // 编辑模式：如果正在拖拽某控制点，移动它
    if (draggingPoint && draggingIndex >= 0 && current)
    {
        current->controlPoints[draggingIndex] = QPointF(p);
        engine->redrawShape(current);
    }
}

void BezierTool::onMouseRelease(QMouseEvent* e, DrawEngine* engine)
{
    if (!engine) return;

    if (e->button() == Qt::LeftButton)
    {
        QPoint p = e->pos();

        // 如果处于非绘制状态，判断是否要进入控制点拖拽：
        // 查找最近的 control point（阈值 6）
        if (!isDrawing)
        {
            // 找在已有 shapes 中是否有 BezierShape 且点击靠近其控制点，则开启拖拽该曲线与该点
            auto &shapes = engine->getShapes();
            const int TH = 6;
            for (auto it = shapes.rbegin(); it != shapes.rend(); ++it)
            {
                auto bz = std::dynamic_pointer_cast<BezierShape>(*it);
                if (!bz) continue;
                for (int i = 0; i < (int)bz->controlPoints.size(); ++i)
                {
                    double dx = bz->controlPoints[i].x() - p.x();
                    double dy = bz->controlPoints[i].y() - p.y();
                    if (dx*dx + dy*dy <= TH*TH)
                    {
                        current = bz;
                        draggingPoint = true;
                        draggingIndex = i;
                        return;
                    }
                }
            }
        } else {
            // 若绘制中，MouseRelease 不一定做事
        }
    } else if (e->button() == Qt::RightButton)
    {
        // 释放右键结束拖拽
        draggingPoint = false;
        draggingIndex = -1;
    }
}

void BezierTool::drawOverlay(QPainter* painter, QWidget* widget)
{
    Q_UNUSED(widget);

    // 绘制当前/最近编辑曲线的控制点（如果有）
    std::shared_ptr<BezierShape> show = current;
    if (!show) return;

    painter->setPen(Qt::blue);
    painter->setBrush(Qt::white);
    for (size_t i = 0; i < show->controlPoints.size(); ++i)
    {
        QPointF p = show->controlPoints[i];
        painter->drawEllipse(p, 4, 4);
    }

    // 绘制控制多边形线（用 Qt 绘制只是 overlay，像素输出仍由 engine 管理）
    painter->setPen(QPen(Qt::gray, 1, Qt::DashLine));
    for (size_t i = 1; i < show->controlPoints.size(); ++i)
    {
        painter->drawLine(show->controlPoints[i-1], show->controlPoints[i]);
    }
}

