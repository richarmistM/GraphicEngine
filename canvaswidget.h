#ifndef CANVASWIDGET_H
#define CANVASWIDGET_H

#include <QWidget>
#include <QImage>
#include <QTimer>

class BaseTool;
class DrawEngine;

/**
 * @brief CanvasWidget 显示画布和处理鼠标事件的 QWidget
 * @param engine 绘图引擎对象指针（负责像素操作）
 * @param parent 父控件
 *
 * 说明：
 * CanvasWidget 是主绘图区的 QWidget，主要负责：
 * 响应鼠标输入事件，并把事件交给当前工具（BaseTool）
 * 定时刷新画面（使用 QTimer 每16ms 调用一次）
 * 负责把 DrawEngine 生成的 QImage 绘制到屏幕上
 */
class CanvasWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CanvasWidget(DrawEngine* engine, QWidget* parent = nullptr);
    ~CanvasWidget();

    void setTool(BaseTool* tool);                                               // 设置当前鼠标工具

protected:
    void paintEvent(QPaintEvent* e) override;                                   // 绘制事件
    void resizeEvent(QResizeEvent* e) override;                                 // 窗口缩放事件

    // 鼠标事件转发给当前工具
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

private slots:
    void onFrame();                                                             // 每帧刷新画布

private:
    DrawEngine* drawEngine;                                                     // 指向绘制引擎
    BaseTool* currentTool;                                                      // 当前鼠标工具
    QTimer* frameTimer;                                                         // 定时刷新画布
};

#endif // CANVASWIDGET_H
