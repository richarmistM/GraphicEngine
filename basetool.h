#ifndef BASETOOL_H
#define BASETOOL_H

#include <QMouseEvent>
#include <QImage>

class DrawEngine;

/**
 * @brief BaseTool —— 鼠标工具基类（抽象接口）
 *
 * 所有的绘图工具（例如直线、矩形、圆、多边形等）都应该继承自该类。
 * 负责响应鼠标事件，并通过 DrawEngine 在画布上绘制。
 *
 * 注意：
 * - BaseTool 自身不做具体绘图，只定义通用接口。
 * - 由子类根据鼠标事件执行对应的绘图逻辑。
 */
class BaseTool
{
public:
    virtual ~BaseTool() = default;

    /**
     * @brief 鼠标左键（按下/移动/释放）事件
     * @param e 鼠标事件（包含坐标、按键信息）
     * @param engine 绘图引擎，用于像素级绘制
     */
    virtual void onMousePress(QMouseEvent* e, DrawEngine* engine) {}
    virtual void onMouseMove(QMouseEvent* e, DrawEngine* engine) {}
    virtual void onMouseRelease(QMouseEvent* e, DrawEngine* engine) {}

    // 工具在 CanvasWidget 上的 overlay 绘制（选中框、控制点、参考点）
    virtual void drawOverlay(QPainter* painter, QWidget* widget) {}

    /**
     * @brief 工具名称
     * @return QString 工具名字符串，用于 UI 显示或调试
     */
    virtual QString toolName() const { return "BaseTool"; }
};

#endif // BASETOOL_H
