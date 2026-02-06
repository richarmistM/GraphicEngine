#include "linetool.h"
#include "drawengine.h"
#include "lineshape.h"

#include <QMouseEvent>

/**
 * @brief 构造函数
 * 初始化 currentLine 为空（表示当前没有在绘制任何线段）
 */
LineTool::LineTool()
    : currentLine(nullptr)
{
}


/**
 * @brief 鼠标左键按下：开始绘制新线段
 *
 * 创建一个新的 LineShape 对象
 * 将起点和终点都设为当前鼠标位置
 * 从 DrawEngine 读取当前的画笔样式（线宽/线型/线帽）
 * 并立即添加到 DrawEngine 的图元列表中
 */
void LineTool::onMousePress(QMouseEvent* e, DrawEngine* engine)
{
    // 只响应左键，且绘图引擎必须有效
    if (!engine || e->button() != Qt::LeftButton) return;

    // 创建一个新的 LineShape 对象
    currentLine = std::make_shared<LineShape>();
    currentLine->start = e->pos();
    currentLine->end = e->pos();                                        // 初始时起点和终点相同

    // 绑定当前画笔属性（从引擎读取）
    currentLine->penWidth = engine->getPenWidth();
    currentLine->lineStyle = engine->getLineStyle();
    currentLine->lineCap = engine->getLineCap();

    // 设置虚线偏移，偏移取决于起点坐标，使得不同位置的线拥有不同的 dash 节奏
    currentLine->dashOffset = (currentLine->start.x() + currentLine->start.y()) % 13;

    engine->addShape(currentLine);                                      // 将图元加入引擎，使其参与后续的重绘
}

/**
 * @brief 鼠标左键移动：实时更新线段终点
 *
 * 当鼠标按下拖动时，不断更新 currentLine->end
 * 并调用 DrawEngine 重绘当前线段，实现动态预览
 */
void LineTool::onMouseMove(QMouseEvent* e, DrawEngine* engine)
{
    if (!engine || !currentLine) return;

    if (!(e->buttons() & Qt::LeftButton)) return;                       // 仅响应左键

    // 更新终点
    currentLine->end = e->pos();

    // 立即重新绘制当前线段
    engine->redrawShape(currentLine);
}

/**
 * @brief 鼠标左键释放：结束绘制
 *
 * 表示用户已经确定了终点，清空 currentLine
 * 下次按下鼠标时将创建新的线段
 */
void LineTool::onMouseRelease(QMouseEvent* e, DrawEngine* engine)
{
    Q_UNUSED(e);
    Q_UNUSED(engine);

    if (e->button() != Qt::LeftButton) return;                          // 仅响应左键

    // 用户确认终点，结束绘制，此处仅释放 LineTool 的持有引用，DrawEngine 仍保存该图形用于重绘
    currentLine.reset();
}

