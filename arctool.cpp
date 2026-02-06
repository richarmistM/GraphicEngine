#include "arctool.h"
#include "drawengine.h"
#include <QMouseEvent>
#include <cmath>

/**
 * @brief 构造函数：初始化绘制状态
 */
ArcTool::ArcTool() : isDrawing(false)
{
}

/**
 * @brief 鼠标按下事件：确定圆心并创建圆弧对象
 *
 * 逻辑说明：
 * 1. 检查参数有效性；
 * 2. 左键按下后，创建一个新的 ArcShape；
 * 3. 将鼠标当前位置作为圆心；
 * 4. 初始化半径和角度；
 * 5. 将圆弧加入 DrawEngine 的图元列表；
 * 6. 标记进入绘制状态。
 */
void ArcTool::onMousePress(QMouseEvent* e, DrawEngine* engine)
{
    // 只响应左键，且绘图引擎必须有效
    if (!engine || e->button() != Qt::LeftButton) return;

    // 创建一个新的圆弧对象（智能指针交给引擎托管）
    currentArc = std::make_shared<ArcShape>();
    currentArc->center = e->pos();                                          // 圆心为鼠标点击点
    currentArc->radius = 0;
    currentArc->startAngle = 0;
    currentArc->endAngle = 0;
    currentArc->color = Qt::black;
    isDrawing = true;                                                       // 标记进入绘制状态

    // 绑定当前画笔属性（从引擎读取）
    currentArc->penWidth = engine->getPenWidth();
    currentArc->lineStyle = engine->getLineStyle();
    currentArc->lineCap = engine->getLineCap();

    engine->addShape(currentArc);                                           // 将图元加入引擎，使其参与后续的重绘
}

/**
 * @brief 鼠标移动事件：动态更新圆弧半径与角度
 *
 * 鼠标当前位置与圆心的距离决定圆弧半径
 * 鼠标位置相对圆心的方向（atan2）决定终止角度
 * 以圆心右侧方向为 0°，顺时针为正方向
 * 每次更新后，通知 DrawEngine 重绘该图元
 */
void ArcTool::onMouseMove(QMouseEvent* e, DrawEngine* engine)
{
    if (!isDrawing || !engine || !(e->buttons() & Qt::LeftButton))
        return;

    // 计算当前鼠标点与圆心的偏移
    QPoint current = e->pos();
    int dx = current.x() - currentArc->center.x();
    int dy = current.y() - currentArc->center.y();

    // 半径为欧几里得距离
    currentArc->radius = std::sqrt(dx * dx + dy * dy);

    /**
     * 角度计算说明：
     * atan2() 返回值范围 [-π, π]
     * Qt 坐标系 y 轴向下，因此 y 取负号以保持数学上“上方为正”
     * 乘以 -180/M_PI 是因为希望 0° 在圆心右侧
     * 顺时针增加角度（与 Qt 图像坐标方向一致）
     */
    currentArc->startAngle = 0;
    currentArc->endAngle = atan2(-dy, dx) * -180.0 / M_PI;
    if (currentArc->endAngle < 0) currentArc->endAngle += 360;

    // 通知引擎仅重绘当前圆弧（避免全清屏）
    engine->redrawShape(currentArc);
}

/**
 * @brief 鼠标释放事件：结束绘制
 *
 * 当用户松开左键后：
 * 标记绘制状态为 false
 * 清空当前圆弧指针
 * 圆弧对象本身仍保存在 DrawEngine 的图元容器中
 */
void ArcTool::onMouseRelease(QMouseEvent* e, DrawEngine* engine)
{
    if (!isDrawing || e->button() != Qt::LeftButton)
        return;

    isDrawing = false;                                                      // 结束绘制
    currentArc.reset();                                                     // 释放指针（DrawEngine 仍保留拷贝）
}

