#ifndef LINETOOL_H
#define LINETOOL_H

#include "basetool.h"
#include <memory>

class DrawEngine;
class LineShape;

/**
 * @brief LineTool —— 直线绘制工具
 *
 * 继承自 BaseTool，实现鼠标拖拽绘制直线的逻辑：
 * 按下鼠标左键（onMousePress） → 确定起点
 * 拖动鼠标左键（onMouseMove） → 实时更新终点（动态预览）
 * 释放鼠标左键（onMouseRelease） → 确认终点，完成绘制
 *
 * 本类不直接绘图，直线的具体绘制算法由 LineShape 内实现
 */
class LineTool : public BaseTool
{
public:
    LineTool();
    ~LineTool() override = default;

    QString toolName() const override { return "LineTool"; }

    void onMousePress(QMouseEvent* e, DrawEngine* engine) override;
    void onMouseMove(QMouseEvent* e, DrawEngine* engine) override;
    void onMouseRelease(QMouseEvent* e, DrawEngine* engine) override;

private:
    std::shared_ptr<LineShape> currentLine;                             // 当前正在绘制的线段对象（共享指针，交由 DrawEngine 管理）
};

#endif // LINETOOL_H

