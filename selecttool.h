#ifndef SELECTTOOL_H
#define SELECTTOOL_H

#include "basetool.h"
#include <vector>
#include <memory>
#include <unordered_map>

class Shape;
class DrawEngine;
class BezierShape;

class SelectTool : public BaseTool
{
public:
    SelectTool();
    ~SelectTool() override = default;

    void onMousePress(QMouseEvent* e, DrawEngine* engine) override;
    void onMouseMove(QMouseEvent* e, DrawEngine* engine) override;
    void onMouseRelease(QMouseEvent* e, DrawEngine* engine) override;

    void applyTransformToSelection(const QTransform& t, DrawEngine* engine);
    void applyTransformToSelection_params(double tx, double ty,
                                                      double sx, double sy,
                                                      double angleDeg,
                                                      const QPointF &ref,
                                          DrawEngine* engine);

    void setReferencePoint(const QPointF& p) { referencePoint = p; useCustomRef = true; }

    void clearSelection() { selectedShapes.clear(); }

    void drawOverlay(QPainter* painter, QWidget* widget) override;

    const std::vector<std::shared_ptr<Shape>>& getSelection() const { return selectedShapes; }


    void startPickRefMode() { pickRefMode = true; hasPickedRef = false; pickedRefVertexIndex = -1; pickedRefShape.reset(); }
    void cancelPickRefMode() { pickRefMode = false; hasPickedRef = false; pickedRefVertexIndex = -1; pickedRefShape.reset(); }
    QPointF getPickedRefPoint() const { return pickedRefPoint; }
    bool isRefPicked() const { return hasPickedRef; }


    bool pickRefMode = false;                // 是否处于“Pick Ref”模式
    QPointF pickedRefPoint;                  // 当前自定义参考点坐标（如果有）
    bool hasPickedRef = false;               // 标志：是否已经 pick
    // 记录 pickedRef 是否对应某个 shape 的顶点
    std::weak_ptr<Shape> pickedRefShape;
    int pickedRefVertexIndex = -1;           // 若不是顶点则为 -1
    int pickedRefSearchRadius = 8;           // 查找最近顶点阈值（像素）


    void snapshotSelectedShapes(DrawEngine* engine);
    void restoreFromSnapshot(DrawEngine* engine);


private:
    bool isDragging;
    QPoint dragStart;
    QPoint dragEnd;

    std::vector<std::shared_ptr<Shape>> selectedShapes;
    QPointF referencePoint;
    bool useCustomRef = false;

    bool editingCtrlPoint = false;                  // 正在拖拽 Bezier 控制点
    int editingCtrlIndex = -1;                      // 被拖拽控制点的索引
    std::weak_ptr<BezierShape> editingBezier;      // 指向正在编辑的 BezierShape（weak_ptr 避免循环引用）


    enum class TransformMode { None, Translating, Scaling, Rotating };

    TransformMode transformMode = TransformMode::None;
    QPoint startMousePos;              // 鼠标按下的位置（屏幕坐标）
    QPointF refPointDuringTransform;   // 变换参考点（屏幕坐标）
    int activeHandleIndex = -1;        // -1 表示非 handle 拖动；0..n 表示哪一个 handle

    // 保存变换开始时的快照，用于实时预览与取消
    struct ShapeBackup {
        // 标记 shape 类型可选
        QPoint line_start, line_end;
        std::vector<QPoint> poly_vertices;
        QPoint arc_center; int arc_radius; double arc_startAngle, arc_endAngle;
        std::vector<QPointF> bezier_ctrls;
    };
    std::unordered_map<std::shared_ptr<Shape>, ShapeBackup> originals;

    // handle 布局参数（像素大小）
    int handleSize = 6;



    // 统一 handle 大小与旋转把手参数，方便调节与一致性
    int handleDrawSize = 10;         // overlay 中绘制 handle 的正方形边长（像素）
    int handleHitPad   = 5;         // hit-test 时的像素半径（用于更容易点击）
    int rotateOffset   = 40;         // 旋转把手相对于 bbox top 的向上偏移（像素）
    int rotateHitRadius = 16;        // 旋转把手的圆形命中半径（像素）

    // 旋转用基准角（按下时记录）
    double initialAngleRad = 0.0;


};

#endif // SELECTTOOL_H
