#ifndef POLYGONTOOL_H
#define POLYGONTOOL_H

#include "basetool.h"
#include <memory>
#include <vector>

class PolygonShape;
class DrawEngine;

/**
 * @brief PolygonTool
 * 鼠标左键：依次添加顶点
 * 鼠标右键或双击：结束并创建 PolygonShape（默认不填充）
 * Shift + 右键（或其他快捷键）可切换填充标志（可按需扩展）
 */
class PolygonTool : public BaseTool
{
public:
    PolygonTool();
    ~PolygonTool() override = default;

    void onMousePress(QMouseEvent* e, DrawEngine* engine) override;
    void onMouseMove(QMouseEvent* e, DrawEngine* engine) override;
    void onMouseRelease(QMouseEvent* e, DrawEngine* engine) override;

    QString toolName() const override { return "PolygonTool"; }

    void setFillOnComplete(bool v) { fillOnComplete = v; }
    void setFillColor(const QColor &c) { fillColor = c; }

    bool fillOnComplete = false;
    QColor fillColor = Qt::yellow;

private:
    std::vector<QPoint> tempVertices;            // 当前临时顶点（用户输入中）
    std::shared_ptr<PolygonShape> previewShape;  // 用于实时预览（可为 nullptr）
    bool isDrawing;
};

#endif // POLYGONTOOL_H

