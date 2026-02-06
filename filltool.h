#ifndef FILLTOOL_H
#define FILLTOOL_H

#include "basetool.h"
#include <QColor>

class FillTool : public BaseTool
{
public:
    FillTool();

    void onMousePress(QMouseEvent* e, DrawEngine* engine) override;
    void onMouseMove(QMouseEvent* e, DrawEngine* engine) override {}
    void onMouseRelease(QMouseEvent* e, DrawEngine* engine) override {}

    void setFillColor(const QColor &c) { fillColor = c; }
    QColor getFillColor() const { return fillColor; }

private:
    QColor fillColor;
};

#endif // FILLTOOL_H

