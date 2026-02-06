#ifndef ARCTOOL_H
#define ARCTOOL_H

#include "basetool.h"
#include "arcshape.h"

/**
 * @brief 鼠标绘制圆弧工具类（ArcTool）
 *
 * 继承自 BaseTool
 * 主要负责响应鼠标事件，动态生成 ArcShape 对象：
 * 按下鼠标左键（onMousePress） → 确定圆心，创建 ArcShape
 * 拖动鼠标左键（onMouseMove） → 根据当前位置计算半径与终止角度
 * 释放鼠标左键（onMouseRelease） → 绘制结束，提交图元
 *
 * 本类不直接绘图，圆弧的具体绘制算法由 ArcShape 内实现
 */
class ArcTool : public BaseTool
{
public:
    ArcTool();

    QString toolName() const override { return "ArcTool"; }

    void onMousePress(QMouseEvent* e, DrawEngine* engine) override;
    void onMouseMove(QMouseEvent* e, DrawEngine* engine) override;
    void onMouseRelease(QMouseEvent* e, DrawEngine* engine) override;

private:
    std::shared_ptr<ArcShape> currentArc;                                   // 当前正在绘制的圆弧（共享指针，交由 DrawEngine 管理）
    bool isDrawing;                                                         // 是否处于绘制状态（按下左键后为 true）
};

#endif // ARCTOOL_H

