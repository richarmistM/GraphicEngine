#include "canvaswidget.h"
#include "drawengine.h"
#include "basetool.h"
#include "shape.h"

#include <QPainter>
#include <QMouseEvent>
#include <QDebug>

CanvasWidget::CanvasWidget(DrawEngine* engine, QWidget* parent)
    : QWidget(parent),
    frameTimer(new QTimer(this)),
    drawEngine(engine),
    currentTool(nullptr)
{
    // 设置布局策略，允许随窗口大小变化而拉伸
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // 初始大小与画布一致（最小尺寸）
    setMinimumSize(drawEngine->getCanvas().width(), drawEngine->getCanvas().height());

    // 连接定时器信号槽：每隔 16ms 调用一次 onFrame（约 60 FPS）
    connect(frameTimer, &QTimer::timeout, this, &CanvasWidget::onFrame);
    frameTimer->start(16);

    qDebug() << "CanvasWidget 初始化完成";
}

/**
 * @brief 析构函数
 * 负责停止定时器
 */
CanvasWidget::~CanvasWidget()
{
    frameTimer->stop();
}

/**
 * @brief 设置当前使用的鼠标工具
 * @param tool BaseTool* 类型工具（如 LineTool、CircleTool 等）
 *
 * 说明：
 * 该函数通常由 MainWindow 调用
 * 用于切换当前绘制模式，例如：setTool(new LineTool())
 */
void CanvasWidget::setTool(BaseTool* tool)
{
    currentTool = tool;
}

/**
 * @brief 绘制事件（Qt 自动调用）
 *
 * 调用时机：
 * - 窗口第一次显示
 * - 调用 update() 时
 * - 窗口被遮挡/最小化后重新显示
 *
 * 逻辑：
 * 让 DrawEngine 清空画布
 * 让每个 Shape 自行在画布上绘制像素
 * 使用 QPainter 将内存画布（QImage）绘制到屏幕
 */
void CanvasWidget::paintEvent(QPaintEvent*)
{
    if (!drawEngine) return;

    drawEngine->clear();                                                // 每次先清空画布（重置为背景色）

    // 让所有图形重新绘制自己
    for (auto& s : drawEngine->getShapes())
        s->draw(drawEngine);

    // 将 QImage 绘制到窗口中，使用 rect() 确保图像会按窗口大小自动缩放
    QPainter painter(this);
    painter.drawImage(rect(), drawEngine->getCanvas());

    // 调用当前工具绘制 overlay（选中框、控制点等）
    if (currentTool) {
        currentTool->drawOverlay(&painter, this);
    }

}

/**
 * @brief 窗口大小变化事件
 * @param e Qt 自动传入的事件对象
 *
 * 当用户拖动窗口边界导致大小变化时，
 * 同步调整 DrawEngine 的画布尺寸，
 * 确保像素绘制区域与窗口显示区域一致。
 */
void CanvasWidget::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);

    if (drawEngine)
        drawEngine->resizeCanvas(width(), height());
}

/**
 * @brief 鼠标按下事件
 * @param e 鼠标事件对象（包含位置、按键类型等）
 *
 * 若当前有激活的工具（currentTool），则将事件交由该工具处理
 */
void CanvasWidget::mousePressEvent(QMouseEvent* e)
{
    if (currentTool)
        currentTool->onMousePress(e, drawEngine);
}

/**
 * @brief 鼠标移动事件
 * @param e 鼠标事件对象
 *
 * 当鼠标移动时，若有激活工具，则传递给工具以便更新临时绘制
 */
void CanvasWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (currentTool)
        currentTool->onMouseMove(e, drawEngine);
}

/**
 * @brief 鼠标释放事件
 * @param e 鼠标事件对象
 *
 * 工具完成一次操作的结束点
 */
void CanvasWidget::mouseReleaseEvent(QMouseEvent* e)
{
    if (currentTool)
        currentTool->onMouseRelease(e, drawEngine);
}

/**
 * @brief 每帧刷新逻辑（由 QTimer 调用）
 *
 * 该函数每 16ms 被调用一次
 * 用于更新动画逻辑或重绘画布
 * 当前版本仅调用 update()，让 Qt 触发 paintEvent()
 */
void CanvasWidget::onFrame()
{
    // 后面可以在这里更新动画、物体移动等逻辑
    update();                                                           // 通知 Qt 重新绘制（显示最新的像素缓冲）
}























