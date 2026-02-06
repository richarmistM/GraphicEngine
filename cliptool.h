#ifndef CLIPTOOL_H
#define CLIPTOOL_H

#include "basetool.h"
#include <memory>
#include <vector>

class PolygonShape;
class DrawEngine;

class ClipTool : public BaseTool
{
public:
    ClipTool();
    ~ClipTool() override = default;

    void onMousePress(QMouseEvent* e, DrawEngine* engine) override;
    void onMouseMove(QMouseEvent* e, DrawEngine* engine) override;
    void onMouseRelease(QMouseEvent* e, DrawEngine* engine) override;

    QString toolName() const override { return "ClipTool"; }

private:
    QPoint startPt;
    QPoint curPt;
    bool isDrawing;
    std::shared_ptr<PolygonShape> previewRect; // preview rectangle added to engine once
};

#endif // CLIPTOOL_H

