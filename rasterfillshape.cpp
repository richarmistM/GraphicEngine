#include "rasterfillshape.h"
#include "drawengine.h"

void RasterFillShape::draw(DrawEngine* engine)
{
    if (!engine) return;
    // 逐像素写回画布，直接用 engine->setPixel（填充不受线型/线帽影响）
    for (const QPoint &p : pixels)
    {
        engine->setPixel(p.x(), p.y(), color);
    }
}

