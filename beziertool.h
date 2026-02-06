#ifndef BEZIERTOOL_H
#define BEZIERTOOL_H

#include "basetool.h"
#include <memory>
#include <vector>
#include <QPointF>

class BezierShape;
class DrawEngine;

/**
 * @brief BezierTool
 * - 左键：依次添加控制点；鼠标移动时更新最后一个点以预览
 * - 右键：完成当前曲线（若控制点 < 2，则取消）
 * - 完成后：点击靠近控制点开始拖拽以调整控制点（编辑模式）
 */
class BezierTool : public BaseTool
{
public:
    BezierTool();

    void onMousePress(QMouseEvent* e, DrawEngine* engine) override;
    void onMouseMove(QMouseEvent* e, DrawEngine* engine) override;
    void onMouseRelease(QMouseEvent* e, DrawEngine* engine) override;

    void drawOverlay(QPainter* painter, QWidget* widget) override; // optional overlay (control points)

private:
    std::shared_ptr<BezierShape> current;   // 当前正在绘制或编辑的曲线
    bool isDrawing;                         // 正在添加控制点
    bool draggingPoint;                     // 编辑时是否拖拽控制点
    int draggingIndex;                      // 被拖拽的控制点索引
};

#endif // BEZIERTOOL_H

