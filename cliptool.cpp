#include "cliptool.h"
#include "drawengine.h"
#include "polygonshape.h"
#include "lineshape.h"
#include "shape.h"

#include <QMouseEvent>
#include <memory>

ClipTool::ClipTool()
    : isDrawing(false)
{}

void ClipTool::onMousePress(QMouseEvent* e, DrawEngine* engine)
{
    if (!engine) return;
    if (e->button() != Qt::LeftButton) return;

    startPt = e->pos();
    curPt = startPt;
    isDrawing = true;

    previewRect = std::make_shared<PolygonShape>();
    previewRect->filled = false;
    previewRect->color = Qt::blue;
    previewRect->penWidth = std::max(1, engine->getPenWidth());
    previewRect->lineStyle = LineStyle::Dash;
    previewRect->lineCap = LineCap::Square;
    previewRect->dashOffset = 0;
    previewRect->vertices = { startPt, startPt, startPt, startPt };
    engine->addShape(previewRect);
    engine->redrawShape(previewRect);
}

void ClipTool::onMouseMove(QMouseEvent* e, DrawEngine* engine)
{
    if (!isDrawing || !engine) return;

    curPt = e->pos();

    if (previewRect)
    {
        int xmin = std::min(startPt.x(), curPt.x());
        int xmax = std::max(startPt.x(), curPt.x());
        int ymin = std::min(startPt.y(), curPt.y());
        int ymax = std::max(startPt.y(), curPt.y());

        std::vector<QPoint> verts = {
            QPoint(xmin, ymin),
            QPoint(xmax, ymin),
            QPoint(xmax, ymax),
            QPoint(xmin, ymax)
        };
        previewRect->vertices = verts;
        engine->redrawShape(previewRect);
    }
}

void ClipTool::onMouseRelease(QMouseEvent* e, DrawEngine* engine)
{
    if (!isDrawing || !engine) return;
    if (e->button() != Qt::LeftButton) return;

    curPt = e->pos();
    isDrawing = false;

    int xmin = std::min(startPt.x(), curPt.x());
    int xmax = std::max(startPt.x(), curPt.x());
    int ymin = std::min(startPt.y(), curPt.y());
    int ymax = std::max(startPt.y(), curPt.y());

    auto shapes = engine->getShapes();

    for (auto &sptr : shapes)
    {
        if (sptr == previewRect) continue;

        auto line = std::dynamic_pointer_cast<LineShape>(sptr);
        if (line)
        {
            int x0 = line->start.x(), y0 = line->start.y();
            int x1 = line->end.x(),   y1 = line->end.y();

            if (engine->cohenSutherlandClip(x0, y0, x1, y1, xmin, ymin, xmax, ymax))
            {
                line->start = QPoint(x0, y0);
                line->end   = QPoint(x1, y1);
            }
            else
            {
                engine->removeShape(line);
            }
            continue;
        }

        auto poly = std::dynamic_pointer_cast<PolygonShape>(sptr);
        if (poly)
        {
            std::vector<QPoint> clipped = engine->clipPolygonWithRect(poly->vertices, xmin, ymin, xmax, ymax);
            if (clipped.size() >= 3)
            {
                poly->vertices = clipped;
            }
            else
            {
                engine->removeShape(poly);
            }
            continue;
        }
    }

    if (previewRect)
    {
        engine->removeShape(previewRect);
        previewRect.reset();
    }

    engine->clear();
}

