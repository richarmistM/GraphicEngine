#include "selecttool.h"
#include "drawengine.h"
#include "shape.h"
#include "lineshape.h"
#include "arcshape.h"
#include "polygonshape.h"
#include "beziershape.h"

#include <QMouseEvent>
#include <QPainter>
#include <algorithm>

static QPointF transform_point_about_ref(const QPointF &p,
                                         const QPointF &ref,
                                         double sx, double sy,
                                         double angleDeg,
                                         double tx, double ty)
{
    double x = p.x() - ref.x();
    double y = p.y() - ref.y();

    x *= sx;
    y *= sy;

    double a = angleDeg * M_PI / 180.0;
    double cosA = std::cos(a);
    double sinA = std::sin(a);
    double xr = x * cosA - y * sinA;
    double yr = x * sinA + y * cosA;

    double xf = xr + ref.x() + tx;
    double yf = yr + ref.y() + ty;

    return QPointF(xf, yf);
}

// 辅助：计算选中 shapes 的包围盒（整数）
static QRect computeSelectionBBox(const std::vector<std::shared_ptr<Shape>>& sel) {
    bool first = true;
    int xmin=0,xmax=0,ymin=0,ymax=0;
    for (auto &sp : sel) {
        // 处理常见 shape 型：Line/Polygon/Arc/Bezier
        if (auto line = std::dynamic_pointer_cast<LineShape>(sp)) {
            int lx0=line->start.x(), ly0=line->start.y();
            int lx1=line->end.x(),   ly1=line->end.y();
            if (first) { xmin = std::min(lx0,lx1); xmax = std::max(lx0,lx1); ymin = std::min(ly0,ly1); ymax = std::max(ly0,ly1); first=false;}
            else { xmin = std::min({xmin,lx0,lx1}); xmax = std::max({xmax,lx0,lx1}); ymin = std::min({ymin,ly0,ly1}); ymax = std::max({ymax,ly0,ly1}); }
        } else if (auto poly = std::dynamic_pointer_cast<PolygonShape>(sp)) {
            for (auto &p : poly->vertices) {
                if (first) { xmin = xmax = p.x(); ymin = ymax = p.y(); first=false; }
                else { xmin = std::min(xmin,p.x()); xmax = std::max(xmax,p.x()); ymin = std::min(ymin,p.y()); ymax = std::max(ymax,p.y()); }
            }
        } else if (auto arc = std::dynamic_pointer_cast<ArcShape>(sp)) {
            // 简单处理：用整个圆的 bbox（保守），因为 arc 可能不是完整圆
            int cx=arc->center.x(), cy=arc->center.y();
            int r=arc->radius;
            if (first) { xmin=cx-r; xmax=cx+r; ymin=cy-r; ymax=cy+r; first=false; }
            else { xmin = std::min(xmin,cx-r); xmax = std::max(xmax,cx+r); ymin = std::min(ymin,cy-r); ymax = std::max(ymax,cy+r); }
        } else if (auto bez = std::dynamic_pointer_cast<BezierShape>(sp)) {
            for (auto &p : bez->controlPoints) {
                if (first) { xmin = xmax = int(p.x()); ymin = ymax = int(p.y()); first=false; }
                else { xmin = std::min(xmin,int(std::floor(p.x()))); xmax = std::max(xmax,int(std::ceil(p.x()))); ymin = std::min(ymin,int(std::floor(p.y()))); ymax = std::max(ymax,int(std::ceil(p.y()))); }
            }
        }
    }
    if (first) return QRect();
    return QRect(xmin,ymin,xmax-xmin,ymax-ymin);
}

// 生成 handles（corner(0..3), edges mid(4..7), rotate handle(8)）
static std::vector<QPoint> makeHandlesFromBBox(const QRect& bbox, int rotateOffset = 28) {
    std::vector<QPoint> h;
    h.reserve(9);
    h.push_back(bbox.topLeft());
    h.push_back(bbox.topRight());
    h.push_back(bbox.bottomRight());
    h.push_back(bbox.bottomLeft());
    h.push_back(QPoint(bbox.center().x(), bbox.top()));
    h.push_back(QPoint(bbox.right(), bbox.center().y()));
    h.push_back(QPoint(bbox.center().x(), bbox.bottom()));
    h.push_back(QPoint(bbox.left(), bbox.center().y()));
    h.push_back(QPoint(bbox.center().x(), bbox.top() - rotateOffset));
    return h;
}

static int hitTestHandle(const std::vector<QPoint>& handles, const QPoint& pos, int hitPad, int rotateIndex = 8, int rotateRadius = 14) {
    if (rotateIndex >= 0 && rotateIndex < (int)handles.size())
    {
        int dx = handles[rotateIndex].x() - pos.x();
        int dy = handles[rotateIndex].y() - pos.y();
        if (dx*dx + dy*dy <= rotateRadius * rotateRadius) return rotateIndex;
    }

    for (int i = 0; i < (int)handles.size(); ++i) {
        if (i == rotateIndex) continue; // 已经检测过
        QRect r(handles[i].x() - hitPad, handles[i].y() - hitPad, hitPad*2, hitPad*2);
        if (r.contains(pos)) return i;
    }
    return -1;
}


SelectTool::SelectTool()
    : isDragging(false), useCustomRef(false),
    editingCtrlPoint(false), editingCtrlIndex(-1)
{
}

// 采集 selectedShapes 的快照
void SelectTool::snapshotSelectedShapes(DrawEngine* engine) {
    originals.clear();
    for (auto &sp : selectedShapes) {
        ShapeBackup bk;
        if (auto line = std::dynamic_pointer_cast<LineShape>(sp)) {
            bk.line_start = line->start; bk.line_end = line->end;
        } else if (auto poly = std::dynamic_pointer_cast<PolygonShape>(sp)) {
            bk.poly_vertices = poly->vertices;
        } else if (auto arc = std::dynamic_pointer_cast<ArcShape>(sp)) {
            bk.arc_center = arc->center; bk.arc_radius = arc->radius;
            bk.arc_startAngle = arc->startAngle; bk.arc_endAngle = arc->endAngle;
        } else if (auto bezier = std::dynamic_pointer_cast<BezierShape>(sp)) {
            bk.bezier_ctrls = bezier->controlPoints;
        }
        originals[sp] = std::move(bk);
    }
}

// 如果需要取消，恢复快照
void SelectTool::restoreFromSnapshot(DrawEngine* engine) {
    for (auto &entry : originals) {
        auto sp = entry.first;
        const ShapeBackup &bk = entry.second;
        if (auto line = std::dynamic_pointer_cast<LineShape>(sp)) {
            line->start = bk.line_start; line->end = bk.line_end;
            engine->redrawShape(line);
        } else if (auto poly = std::dynamic_pointer_cast<PolygonShape>(sp)) {
            poly->vertices = bk.poly_vertices;
            engine->redrawShape(poly);
        } else if (auto arc = std::dynamic_pointer_cast<ArcShape>(sp)) {
            arc->center = bk.arc_center; arc->radius = bk.arc_radius;
            arc->startAngle = bk.arc_startAngle; arc->endAngle = bk.arc_endAngle;
            engine->redrawShape(arc);
        } else if (auto bezier = std::dynamic_pointer_cast<BezierShape>(sp)) {
            bezier->controlPoints = bk.bezier_ctrls;
            engine->redrawShape(bezier);
        }
    }
    originals.clear();
}


void SelectTool::onMousePress(QMouseEvent* e, DrawEngine* engine)
{
    if (!engine) return;
    // 如果处于 pickRefMode，则优先处理参考点拾取（single click）

    if (pickRefMode && e->button() == Qt::LeftButton)
    {
        QPoint clicked = e->pos();
        hasPickedRef = false;
        pickedRefVertexIndex = -1;
        pickedRefShape.reset();

        // 1) 在当前选中图元中查找最近顶点（按像素距离阈值）
        const int TH = pickedRefSearchRadius;
        double bestDist2 = (TH+1)*(TH+1);
        std::shared_ptr<Shape> bestShape = nullptr;
        int bestIndex = -1;

        for (auto &sp : selectedShapes)
        {
            // 只对具有顶点的 shape 做查找：LineShape (2 pts), PolygonShape (vertices), BezierShape (controlPoints)
            // 需要 dynamic cast 检查具体类型
            if (auto line = std::dynamic_pointer_cast<LineShape>(sp)) {
                QPoint vs[2] = { line->start, line->end };
                for (int i=0;i<2;++i) {
                    double dx = vs[i].x() - clicked.x();
                    double dy = vs[i].y() - clicked.y();
                    double d2 = dx*dx + dy*dy;
                    if (d2 <= bestDist2) { bestDist2 = d2; bestShape = sp; bestIndex = i; }
                }
            } else if (auto poly = std::dynamic_pointer_cast<PolygonShape>(sp)) {
                for (int i=0;i<(int)poly->vertices.size();++i) {
                    QPoint v = poly->vertices[i];
                    double dx = v.x() - clicked.x(), dy = v.y() - clicked.y();
                    double d2 = dx*dx + dy*dy;
                    if (d2 <= bestDist2) { bestDist2 = d2; bestShape = sp; bestIndex = i; }
                }
            } else if (auto bez = std::dynamic_pointer_cast<BezierShape>(sp)) {
                for (int i=0;i<(int)bez->controlPoints.size();++i) {
                    QPointF v = bez->controlPoints[i];
                    double dx = v.x() - clicked.x(), dy = v.y() - clicked.y();
                    double d2 = dx*dx + dy*dy;
                    if (d2 <= bestDist2) { bestDist2 = d2; bestShape = sp; bestIndex = i; }
                }
            }
            // ArcShape: 可以检测圆心是否接近
            else if (auto arc = std::dynamic_pointer_cast<ArcShape>(sp)) {
                double dx = arc->center.x() - clicked.x(), dy = arc->center.y() - clicked.y();
                double d2 = dx*dx + dy*dy;
                if (d2 <= bestDist2) { bestDist2 = d2; bestShape = sp; bestIndex = 0; } // vertexIndex 0 表示圆心
            }
        }

        if (bestShape) {
            // pick 成功，记录为顶点引用
            pickedRefShape = bestShape;
            pickedRefVertexIndex = bestIndex;

            if (auto line = std::dynamic_pointer_cast<LineShape>(bestShape)) {
                pickedRefPoint = (bestIndex==0) ? QPointF(line->start) : QPointF(line->end);
            } else if (auto poly = std::dynamic_pointer_cast<PolygonShape>(bestShape)) {
                pickedRefPoint = QPointF(poly->vertices[bestIndex]);
            } else if (auto bez = std::dynamic_pointer_cast<BezierShape>(bestShape)) {
                pickedRefPoint = bez->controlPoints[bestIndex];
            } else if (auto arc = std::dynamic_pointer_cast<ArcShape>(bestShape)) {
                pickedRefPoint = QPointF(arc->center);
            } else {
                pickedRefPoint = QPoint(clicked);
            }
        } else {
            // 没找到顶点，直接把点击点当作 canvas point
            pickedRefShape.reset();
            pickedRefVertexIndex = -1;
            pickedRefPoint = QPointF(clicked);
        }

        hasPickedRef = true;
        pickRefMode = false; // 退出 pick 模式（一次性选择）

        return;
    }




    // 若点击靠近已选中的 BezierShape 的控制点，则进入“控制点拖拽”模式 ---
    if (e->button() == Qt::LeftButton)
    {
        QPoint clicked = e->pos();
        const int TH = handleHitPad;
        // 遍历已选图元（从上到下），优先选择顶层的 Bezier 控制点
        for (auto it = selectedShapes.rbegin(); it != selectedShapes.rend(); ++it)
        {
            auto bz = std::dynamic_pointer_cast<BezierShape>(*it);
            if (!bz) continue;
            for (int i = 0; i < (int)bz->controlPoints.size(); ++i)
            {
                double dx = bz->controlPoints[i].x() - clicked.x();
                double dy = bz->controlPoints[i].y() - clicked.y();
                if (dx*dx + dy*dy <= TH*TH)
                {
                    // 开始拖拽该控制点
                    editingBezier = bz;
                    editingCtrlIndex = i;
                    editingCtrlPoint = true;
                    return; // 不进入框选逻辑
                }
            }
        }
    }




    // 检测是否点中 handles 或 bbox，若是进入 transform 模式 ---
    if (e->button() == Qt::LeftButton && !selectedShapes.empty())
    {
        QRect bbox = computeSelectionBBox(selectedShapes);
        if (!bbox.isNull())
        {
            auto handles = makeHandlesFromBBox(bbox, rotateOffset);
            int hit = hitTestHandle(handles, e->pos(), handleHitPad, 8, rotateHitRadius);
            if (hit >= 0) {
                // 点击到某个 handle -> 开始变换（scale or rotate）
                startMousePos = e->pos();
                activeHandleIndex = hit;
                if (hit <= 7) transformMode = TransformMode::Scaling;
                else transformMode = TransformMode::Rotating;

                // 参考点：若用户已 pick custom ref 则用 pickedRefPoint，否则取选中图元重心平均
                if (useCustomRef && hasPickedRef) refPointDuringTransform = pickedRefPoint;
                else {
                    QPointF sum(0,0); int cnt=0;
                    for (auto &s : selectedShapes) { QPointF c = s->centroid(); sum += c; ++cnt; }
                    if (cnt == 0) refPointDuringTransform = bbox.center();
                    else refPointDuringTransform = QPointF(sum.x()/cnt, sum.y()/cnt);
                }

                snapshotSelectedShapes(engine); // 保存原始几何
                initialAngleRad = std::atan2(double(startMousePos.y()) - refPointDuringTransform.y(),
                                             double(startMousePos.x()) - refPointDuringTransform.x());
                return; // 已处理，直接返回（不要进入下面的框选拖拽逻辑）
            }

            // 若未点中 handle 但点在 bbox 内 -> 平移
            if (bbox.contains(e->pos())) {
                startMousePos = e->pos();
                transformMode = TransformMode::Translating;
                // 参考点同上
                if (useCustomRef && hasPickedRef) refPointDuringTransform = pickedRefPoint;
                else {
                    QPointF sum(0,0); int cnt=0;
                    for (auto &s : selectedShapes) { QPointF c = s->centroid(); sum += c; ++cnt; }
                    refPointDuringTransform = (cnt>0) ? QPointF(sum.x()/cnt, sum.y()/cnt) : bbox.center();
                }
                snapshotSelectedShapes(engine);
                return;
            }
        }
    }



    // 若没有进入 ctrl-point 拖拽，则保持原有行为：开始拖拽/框选
    if (e->button() == Qt::LeftButton)
    {
        isDragging = true;
        dragStart = e->pos();
        dragEnd = dragStart;
    }
}

void SelectTool::onMouseMove(QMouseEvent* e, DrawEngine* engine)
{
    if (!engine) return;

    // 如果正在拖拽 Bezier 的控制点（编辑模式）
    if (editingCtrlPoint)
    {
        auto bz = editingBezier.lock();
        if (!bz) {
            // 所指对象已被删除或释放
            editingCtrlPoint = false;
            editingCtrlIndex = -1;
            return;
        }
        // 更新控制点坐标
        if (editingCtrlIndex >= 0 && editingCtrlIndex < (int)bz->controlPoints.size())
        {
            bz->controlPoints[editingCtrlIndex] = QPointF(e->pos());
            engine->redrawShape(bz);
        }
        return;
    }


    if (transformMode != TransformMode::None)
    {
        // 当前鼠标位置
        QPoint cur = e->pos();

        // 参考点（变换中心）
        QPointF ref = refPointDuringTransform;

        // 计算平移量（用于 Translating）
        double dx = double(cur.x() - startMousePos.x());
        double dy = double(cur.y() - startMousePos.y());

        // 计算缩放因子与旋转角度（基于起始鼠标点 startMousePos 与当前点 cur 相对于 ref 的比值/角度差）
        // 注意：使用 startMousePos 与 cur 对 ref 做比值，防止累积误差（因为我们从 originals 应用变换）
        double denomX = double(startMousePos.x()) - ref.x();
        double denomY = double(startMousePos.y()) - ref.y();

        double sX = 1.0, sY = 1.0;
        if (std::abs(denomX) > 1e-6) sX = (double(cur.x()) - ref.x()) / denomX;
        if (std::abs(denomY) > 1e-6) sY = (double(cur.y()) - ref.y()) / denomY;

        // 如果起始鼠标恰好在参考点（分母接近 0），退回到基于距离的缩放（更稳健）
        if (std::abs(denomX) <= 1e-6 || std::abs(denomY) <= 1e-6)
        {
            double startDist = std::hypot(double(startMousePos.x()) - ref.x(), double(startMousePos.y()) - ref.y());
            double curDist   = std::hypot(double(cur.x()) - ref.x(), double(cur.y()) - ref.y());
            if (startDist > 1e-6) {
                double s = curDist / startDist;
                sX = sY = s;
            } else {
                sX = sY = 1.0;
            }
        }

        // 旋转角度（度）
        //double angle0 = std::atan2(double(startMousePos.y()) - ref.y(), double(startMousePos.x()) - ref.x());
        double angle1 = std::atan2(double(cur.y()) - ref.y(), double(cur.x()) - ref.x());
        double deltaAngleDeg = (angle1 - initialAngleRad) * 180.0 / M_PI;

        bool applyScaleX = true, applyScaleY = true;
        if (transformMode == TransformMode::Scaling)
        {
            if (activeHandleIndex >= 0 && activeHandleIndex <= 7)
            {
                if (activeHandleIndex >= 4 && activeHandleIndex <= 7)
                {
                    if (activeHandleIndex == 4 || activeHandleIndex == 6) { applyScaleX = false; applyScaleY = true; }
                    else { applyScaleX = true; applyScaleY = false; }
                } else {
                    applyScaleX = applyScaleY = true;
                }
            } else {
                applyScaleX = applyScaleY = true;
            }
        }

        // ----------------------------
        // 2) 遍历 selectedShapes，基于 originals 快照计算新的几何并写回（**非累积**）
        // ----------------------------
        for (auto &sp : selectedShapes)
        {
            // 找到原始快照
            auto it = originals.find(sp);
            if (it == originals.end()) continue; // 若没有快照则跳过（通常不会发生）

            const ShapeBackup &bk = it->second;

            // 2.1 LineShape：对线段的两个端点分别做仿射变换
            if (auto line = std::dynamic_pointer_cast<LineShape>(sp))
            {
                // 从快照读取原始点
                QPointF p0(bk.line_start), p1(bk.line_end);

                // 根据 transformMode 分别处理
                if (transformMode == TransformMode::Translating)
                {
                    QPointF np0 = QPointF(p0.x() + dx, p0.y() + dy);
                    QPointF np1 = QPointF(p1.x() + dx, p1.y() + dy);
                    line->start = QPoint(int(std::round(np0.x())), int(std::round(np0.y())));
                    line->end   = QPoint(int(std::round(np1.x())), int(std::round(np1.y())));
                }
                else if (transformMode == TransformMode::Scaling)
                {
                    // 以 ref 为中心，先相对坐标缩放，再根据 active axis 判断
                    QPointF rp = QPointF(ref);
                    double sx = applyScaleX ? sX : 1.0;
                    double sy = applyScaleY ? sY : 1.0;

                    QPointF np0 = transform_point_about_ref(p0, rp, sx, sy, 0.0, 0.0, 0.0);
                    QPointF np1 = transform_point_about_ref(p1, rp, sx, sy, 0.0, 0.0, 0.0);

                    line->start = QPoint(int(std::round(np0.x())), int(std::round(np0.y())));
                    line->end   = QPoint(int(std::round(np1.x())), int(std::round(np1.y())));
                }
                else if (transformMode == TransformMode::Rotating)
                {
                    QPointF rp = QPointF(ref);
                    // 保持缩放为 1, 只旋转再平移（旋转角度 = deltaAngleDeg）
                    QPointF np0 = transform_point_about_ref(p0, rp, 1.0, 1.0, deltaAngleDeg, 0.0, 0.0);
                    QPointF np1 = transform_point_about_ref(p1, rp, 1.0, 1.0, deltaAngleDeg, 0.0, 0.0);

                    line->start = QPoint(int(std::round(np0.x())), int(std::round(np0.y())));
                    line->end   = QPoint(int(std::round(np1.x())), int(std::round(np1.y())));
                    qDebug() << "Rotating, deltaAngleDeg=" << deltaAngleDeg;

                }

                // 将变换后的图元写回画布（单个重绘）
                engine->redrawShape(line);
                continue;
            }

            // 2.2 PolygonShape：对每个顶点做变换
            if (auto poly = std::dynamic_pointer_cast<PolygonShape>(sp))
            {
                // 遍历快照顶点并生成新顶点
                std::vector<QPoint> newVerts;
                newVerts.reserve(bk.poly_vertices.size());
                for (const QPoint &origPt : bk.poly_vertices)
                {
                    QPointF p(origPt);
                    QPointF np;
                    if (transformMode == TransformMode::Translating)
                        np = QPointF(p.x() + dx, p.y() + dy);
                    else if (transformMode == TransformMode::Scaling)
                    {
                        double sx = applyScaleX ? sX : 1.0;
                        double sy = applyScaleY ? sY : 1.0;
                        np = transform_point_about_ref(p, ref, sx, sy, 0.0, 0.0, 0.0);
                    }
                    else // Rotating
                        np = transform_point_about_ref(p, ref, 1.0, 1.0, deltaAngleDeg, 0.0, 0.0);

                    newVerts.push_back(QPoint(int(std::round(np.x())), int(std::round(np.y()))));
                }

                poly->vertices = std::move(newVerts);
                engine->redrawShape(poly);
                continue;
            }

            // 2.3 ArcShape：对圆心做仿射，半径按平均缩放，角度按旋转叠加
            if (auto arc = std::dynamic_pointer_cast<ArcShape>(sp))
            {
                // 原始中心和参数来自快照
                QPointF origCenter(bk.arc_center);
                int origR = bk.arc_radius;
                double origStart = bk.arc_startAngle;
                double origEnd   = bk.arc_endAngle;

                if (transformMode == TransformMode::Translating)
                {
                    QPointF nc(origCenter.x() + dx, origCenter.y() + dy);
                    arc->center = QPoint(int(std::round(nc.x())), int(std::round(nc.y())));
                }
                else if (transformMode == TransformMode::Scaling)
                {
                    double sx = applyScaleX ? sX : 1.0;
                    double sy = applyScaleY ? sY : 1.0;
                    QPointF nc = transform_point_about_ref(origCenter, ref, sx, sy, 0.0, 0.0, 0.0);
                    arc->center = QPoint(int(std::round(nc.x())), int(std::round(nc.y())));
                    double scaleApprox = (std::abs(sx) + std::abs(sy)) * 0.5;
                    arc->radius = std::max(0, int(std::round(origR * scaleApprox)));
                }
                else if (transformMode == TransformMode::Rotating)
                {
                    QPointF nc = transform_point_about_ref(origCenter, ref, 1.0, 1.0, deltaAngleDeg, 0.0, 0.0);
                    arc->center = QPoint(int(std::round(nc.x())), int(std::round(nc.y())));
                    arc->startAngle = origStart + deltaAngleDeg;
                    arc->endAngle   = origEnd   + deltaAngleDeg;
                    arc->startAngle = fmod(arc->startAngle, 360.0);
                    if (arc->startAngle < 0) arc->startAngle += 360.0;
                    arc->endAngle   = fmod(arc->endAngle, 360.0);
                    if (arc->endAngle < 0) arc->endAngle += 360.0;
                }

                engine->redrawShape(arc);
                continue;
            }

            // 2.4 其它图元（Bezier 等）可以按需类似扩展：基于原始控制点逐点变换
            // if (auto bez = std::dynamic_pointer_cast<BezierShape>(sp)) { ... }
            // ------------- BezierShape 处理 -------------
            // 处理 n 阶 Bezier：对每个 control point 基于快照做非累积变换
            if (auto bez = std::dynamic_pointer_cast<BezierShape>(sp))
            {
                // 找快照
                auto itb = originals.find(sp);
                // 如果没有快照，就跳过（正常情况不该发生）
                if (itb == originals.end()) {
                    // 可能是逻辑错误，但为稳健性保留当前 controlPoints 不变
                    engine->redrawShape(bez);
                    continue;
                }
                const ShapeBackup &bk = itb->second;

                // 以快照为源（若快照中没有数据，则退回到现有 controlPoints）
                const std::vector<QPointF> &srcCtrls = (bk.bezier_ctrls.empty() ? bez->controlPoints : bk.bezier_ctrls);
                std::vector<QPointF> newCtrls;
                newCtrls.reserve(srcCtrls.size());

                for (size_t i = 0; i < srcCtrls.size(); ++i)
                {
                    QPointF orig = srcCtrls[i];
                    QPointF transformed;

                    if (transformMode == TransformMode::Translating)
                    {
                        // 平移：直接加偏移
                        transformed = QPointF(orig.x() + dx, orig.y() + dy);
                    }
                    else if (transformMode == TransformMode::Scaling)
                    {
                        // 缩放：以 ref 为中心，分别应用 sX/sY（考虑只对某轴缩放的情况）
                        double sx = applyScaleX ? sX : 1.0;
                        double sy = applyScaleY ? sY : 1.0;
                        transformed = transform_point_about_ref(orig, ref, sx, sy, 0.0, 0.0, 0.0);
                    }
                    else if (transformMode == TransformMode::Rotating)
                    {
                        // 旋转：以 ref 为中心，按 deltaAngleDeg 旋转（不改变尺度）
                        transformed = transform_point_about_ref(orig, ref, 1.0, 1.0, deltaAngleDeg, 0.0, 0.0);
                    }
                    else
                    {
                        transformed = orig; // 不变
                    }

                    newCtrls.push_back(transformed);
                }

                // 写回 control points 并重绘
                bez->controlPoints = std::move(newCtrls);
                engine->redrawShape(bez);
                continue;
            }


        }
        return;
    }



    if (!isDragging) return;

    dragEnd = e->pos();
    // overlay 会在 CanvasWidget::paintEvent 调用 drawOverlay，显示拖拽矩形
    engine->redrawShape(nullptr);
}

void SelectTool::onMouseRelease(QMouseEvent* e, DrawEngine* engine)
{
    if (!engine) return;

    // 如果刚在编辑 Bezier 控制点，左键释放结束编辑
    if (editingCtrlPoint && e->button() == Qt::LeftButton)
    {
        editingCtrlPoint = false;
        editingCtrlIndex = -1;
        editingBezier.reset();
        return;
    }

    if (transformMode != TransformMode::None)
    {
        // 右键：撤销此次变换，恢复快照的原始几何
        if (e->button() == Qt::RightButton)
        {
            restoreFromSnapshot(engine);   // 恢复快照并重绘所有项
        }
        else if (e->button() == Qt::LeftButton)
        {
            // 左键：确认变换 —— 对某些图元做收尾修正（例如角度归一化等）
            for (auto &sp : selectedShapes)
            {
                if (auto arc = std::dynamic_pointer_cast<ArcShape>(sp))
                {
                    // 归一化角度到 [0,360)
                    arc->startAngle = fmod(arc->startAngle, 360.0);
                    if (arc->startAngle < 0) arc->startAngle += 360.0;
                    arc->endAngle = fmod(arc->endAngle, 360.0);
                    if (arc->endAngle < 0) arc->endAngle += 360.0;

                    // 防止半径负值或小于 0 的异常
                    if (arc->radius < 0) arc->radius = 0;

                    engine->redrawShape(arc);
                    continue;
                }

                // 其它类型无需额外处理（已在 onMouseMove 中写回）
                engine->redrawShape(sp);
            }
        }

        // 清理变换状态与快照
        transformMode = TransformMode::None;
        activeHandleIndex = -1;
        originals.clear();
        // 结束后不进入下面的选择逻辑
        return;
    }

    if (!isDragging) return;
    isDragging = false;
    dragEnd = e->pos();

    // 如果移动距离很小，视为单击
    int dx = std::abs(dragEnd.x() - dragStart.x());
    int dy = std::abs(dragEnd.y() - dragStart.y());
    const int CLICK_THRESH = 4;

    selectedShapes.clear();

    auto shapes = engine->getShapes();
    if (dx <= CLICK_THRESH && dy <= CLICK_THRESH)
    {
        // 单击：从后向前选（顶层优先）
        for (auto it = shapes.rbegin(); it != shapes.rend(); ++it)
        {
            if ((*it)->contains(dragEnd))
            {
                selectedShapes.push_back(*it);
                break;
            }
        }
    }
    else
    {
        // 框选：选择所有和矩形有交集或中心在矩形内的 shape
        int xmin = std::min(dragStart.x(), dragEnd.x());
        int xmax = std::max(dragStart.x(), dragEnd.x());
        int ymin = std::min(dragStart.y(), dragEnd.y());
        int ymax = std::max(dragStart.y(), dragEnd.y());

        for (auto &sp : shapes)
        {
            QPointF c = sp->centroid();
            if (c.x() >= xmin && c.x() <= xmax && c.y() >= ymin && c.y() <= ymax)
                selectedShapes.push_back(sp);
        }
    }

    // 如果用户没有设置自定义参考点，默认重心为变换中心
    useCustomRef = false;
}

void SelectTool::applyTransformToSelection_params(double tx, double ty,
                                                  double sx, double sy,
                                                  double angleDeg,
                                                  const QPointF &ref,
                                                  DrawEngine* engine)
{
    if (!engine) return;

    // 遍历所有被选图形并对其几何顶点做逐点变换
    for (auto &sp : selectedShapes)
    {
        // LineShape
        if (auto line = std::dynamic_pointer_cast<LineShape>(sp))
        {
            QPointF s = transform_point_about_ref(QPointF(line->start), ref, sx, sy, angleDeg, tx, ty);
            QPointF e = transform_point_about_ref(QPointF(line->end),   ref, sx, sy, angleDeg, tx, ty);

            line->start = QPoint(int(std::round(s.x())), int(std::round(s.y())));
            line->end   = QPoint(int(std::round(e.x())), int(std::round(e.y())));

            engine->redrawShape(line);
            continue;
        }

        // PolygonShape
        if (auto poly = std::dynamic_pointer_cast<PolygonShape>(sp))
        {
            for (auto &pt : poly->vertices)
            {
                QPointF qf = transform_point_about_ref(QPointF(pt), ref, sx, sy, angleDeg, tx, ty);
                pt = QPoint(int(std::round(qf.x())), int(std::round(qf.y())));
            }
            engine->redrawShape(poly);
            continue;
        }

        // ArcShape
        if (auto arc = std::dynamic_pointer_cast<ArcShape>(sp))
        {
            QPointF newCenter = transform_point_about_ref(QPointF(arc->center), ref, sx, sy, angleDeg, tx, ty);

            double scaleApprox = (std::abs(sx) + std::abs(sy)) * 0.5;
            double newR = arc->radius * scaleApprox;

            arc->center = QPoint(int(std::round(newCenter.x())), int(std::round(newCenter.y())));
            arc->radius = std::max(0, int(std::round(newR)));

            arc->startAngle += angleDeg;
            arc->endAngle   += angleDeg;

            if (arc->startAngle >= 360.0) arc->startAngle = fmod(arc->startAngle, 360.0);
            if (arc->endAngle >= 360.0)   arc->endAngle   = fmod(arc->endAngle,   360.0);

            engine->redrawShape(arc);
            continue;
        }

        // BezierShape
        if (auto bez = std::dynamic_pointer_cast<BezierShape>(sp))
        {
            for (auto &cp : bez->controlPoints)
            {
                QPointF qf = transform_point_about_ref(cp, ref, sx, sy, angleDeg, tx, ty);
                cp = QPointF(qf.x(), qf.y());
            }
            engine->redrawShape(bez);
            continue;
        }
    }
}


void SelectTool::drawOverlay(QPainter* painter, QWidget* widget)
{
    Q_UNUSED(widget);

    // 基本样式：用于 overlay 的笔/刷
    QPen dashedPen(Qt::blue);
    dashedPen.setStyle(Qt::DashLine);
    dashedPen.setWidth(1);

    QPen solidThin(Qt::red);
    solidThin.setWidth(1);

    QBrush noBrush(Qt::NoBrush);

    // 1) 拖拽矩形（如果在拖拽中）
    if (isDragging)
    {
        QRect dragRect(dragStart, dragEnd);
        painter->setPen(dashedPen);
        painter->setBrush(noBrush);
        painter->drawRect(dragRect);
    }

    // 2) 为每个选中图形绘制 bbox + handles + 控制点（如果有）
    //    同时绘制图元重心（红色小方块）
    painter->setPen(solidThin);

    auto computeBBox = [this](const std::shared_ptr<Shape>& sp) -> QRectF {
        QRectF bbox;
        if (!sp) return bbox;

        if (auto line = std::dynamic_pointer_cast<LineShape>(sp))
        {
            int x0 = line->start.x(), y0 = line->start.y();
            int x1 = line->end.x(),   y1 = line->end.y();
            int xmin = std::min(x0, x1), xmax = std::max(x0, x1);
            int ymin = std::min(y0, y1), ymax = std::max(y0, y1);
            bbox = QRectF(QPointF(xmin, ymin), QPointF(xmax, ymax));
        }
        else if (auto poly = std::dynamic_pointer_cast<PolygonShape>(sp))
        {
            if (poly->vertices.empty()) return QRectF();
            int xmin = poly->vertices[0].x(), xmax = xmin;
            int ymin = poly->vertices[0].y(), ymax = ymin;
            for (const QPoint &p : poly->vertices) {
                xmin = std::min(xmin, p.x()); xmax = std::max(xmax, p.x());
                ymin = std::min(ymin, p.y()); ymax = std::max(ymax, p.y());
            }
            bbox = QRectF(QPointF(xmin, ymin), QPointF(xmax, ymax));
        }
        else if (auto arc = std::dynamic_pointer_cast<ArcShape>(sp))
        {
            // 为 overlay 简化处理：用完整圆的 bbox（比严格的 arc bbox 更简单可靠）
            int cx = arc->center.x(), cy = arc->center.y();
            int r = std::max(0, arc->radius);
            bbox = QRectF(QPointF(cx - r, cy - r), QPointF(cx + r, cy + r));
        }
        else if (auto bez = std::dynamic_pointer_cast<BezierShape>(sp))
        {
            if (bez->controlPoints.empty()) return QRectF();
            double xmin = bez->controlPoints[0].x(), xmax = xmin;
            double ymin = bez->controlPoints[0].y(), ymax = ymin;
            for (const QPointF &p : bez->controlPoints) {
                xmin = std::min(xmin, p.x()); xmax = std::max(xmax, p.x());
                ymin = std::min(ymin, p.y()); ymax = std::max(ymax, p.y());
            }
            bbox = QRectF(QPointF(xmin, ymin), QPointF(xmax, ymax));
        }
        else {
            QPointF c = sp->centroid();
            bbox = QRectF(c.x()- handleDrawSize / 2, c.y()-handleDrawSize/2.0, handleDrawSize, handleDrawSize);
        }

        int pad = std::max(2, sp->penWidth / 2 + 2);
        bbox.adjust(-pad, -pad, pad, pad);
        return bbox;
    };

    auto drawHandle = [&](const QPointF &pos, int idx) {
        const int H = handleDrawSize;
        QRectF r(pos.x() - H/2.0, pos.y() - H/2.0, H, H);
        if (idx == activeHandleIndex && activeHandleIndex >= 0) {
            painter->setBrush(Qt::green);
            painter->setPen(QPen(Qt::green));
        } else {
            painter->setBrush(Qt::white);
            painter->setPen(QPen(Qt::black));
        }
        // 旋转把手（index==8）：画圆形而不是方块
        if (idx == 8) {
            painter->setBrush(activeHandleIndex==8 ? Qt::green : Qt::white);
            painter->setPen(QPen(Qt::black));
            painter->drawEllipse(QPointF(pos), handleDrawSize/2.0, handleDrawSize/2.0);
        } else {
            painter->drawRect(r);
        }
        painter->setBrush(noBrush);
    };

    // 对每个选中 shape 绘制
    for (auto &sp : selectedShapes)
    {
        if (!sp) continue;

        QRectF bbox = computeBBox(sp);
        if (bbox.isNull()) continue;

        painter->setPen(dashedPen);
        painter->setBrush(noBrush);
        painter->drawRect(bbox);

        painter->setPen(QPen(Qt::red));
        QPointF c = sp->centroid();
        QRectF centRect(c.x()-6, c.y()-6, 12, 12);
        painter->drawRect(centRect);

        QPointF tl = bbox.topLeft();
        QPointF tr = bbox.topRight();
        QPointF br = bbox.bottomRight();
        QPointF bl = bbox.bottomLeft();
        QPointF midTop((tl.x()+tr.x())/2.0, (tl.y()+tr.y())/2.0);
        QPointF midRight((tr.x()+br.x())/2.0, (tr.y()+br.y())/2.0);
        QPointF midBottom((bl.x()+br.x())/2.0, (bl.y()+br.y())/2.0);
        QPointF midLeft((tl.x()+bl.x())/2.0, (tl.y()+bl.y())/2.0);

        drawHandle(tl, 0);
        drawHandle(tr, 1);
        drawHandle(br, 2);
        drawHandle(bl, 3);
        drawHandle(midTop, 4);
        drawHandle(midRight,5);
        drawHandle(midBottom,6);
        drawHandle(midLeft,7);

        // 画每类图元的控制点/可编辑点（使用不同颜色）
        painter->setPen(QPen(Qt::blue));
        painter->setBrush(Qt::white);

        if (auto poly = std::dynamic_pointer_cast<PolygonShape>(sp))
        {
            // 顶点
            for (const QPoint &v : poly->vertices)
            {
                QRectF r(v.x()-handleDrawSize/2.0, v.y()-handleDrawSize/2.0, handleDrawSize, handleDrawSize);
                painter->drawRect(r);
            }
        }
        else if (auto bez = std::dynamic_pointer_cast<BezierShape>(sp))
        {
            painter->setPen(QPen(Qt::darkCyan, 1, Qt::DashLine));
            // 如果 bezier 存在控制柄关系（例如按顺序），连线相邻 control points 显示“框架”
            for (size_t i = 0; i + 1 < bez->controlPoints.size(); ++i)
            {
                QPointF a = bez->controlPoints[i], b = bez->controlPoints[i+1];
                painter->drawLine(QPointF(a), QPointF(b));
            }
            painter->setPen(QPen(Qt::blue));
            for (const QPointF &cp : bez->controlPoints)
            {
                QRectF r(cp.x()-handleDrawSize/2.0, cp.y()-handleDrawSize/2.0, handleDrawSize, handleDrawSize);
                painter->drawRect(r);
            }
        }
        else if (auto line = std::dynamic_pointer_cast<LineShape>(sp))
        {
            // 端点
            QPoint p0 = line->start, p1 = line->end;
            painter->setPen(QPen(Qt::blue));
            painter->drawRect(QRectF(p0.x()-handleDrawSize/2.0, p0.y()-handleDrawSize/2.0, handleDrawSize, handleDrawSize));
            painter->drawRect(QRectF(p1.x()-handleDrawSize/2.0, p1.y()-handleDrawSize/2.0, handleDrawSize, handleDrawSize));
        }
        else if (auto arc = std::dynamic_pointer_cast<ArcShape>(sp))
        {
            // 圆心与近似的起终点（通过角度计算）
            painter->setPen(QPen(Qt::magenta));
            QPointF center = arc->center;
            painter->drawEllipse(center, handleDrawSize/2.0, handleDrawSize/2.0);

            // 起终点（按角度计算）
            double srad = qDegreesToRadians(arc->startAngle);
            double erad = qDegreesToRadians(arc->endAngle);
            QPointF spnt(center.x() + arc->radius * std::cos(srad), center.y() + arc->radius * std::sin(srad));
            QPointF epnt(center.x() + arc->radius * std::cos(erad), center.y() + arc->radius * std::sin(erad));
            painter->drawRect(QRectF(spnt.x()-handleDrawSize/2.0, spnt.y()-handleDrawSize/2.0, handleDrawSize, handleDrawSize));
            painter->drawRect(QRectF(epnt.x()-handleDrawSize/2.0, epnt.y()-handleDrawSize/2.0, handleDrawSize, handleDrawSize));
        }

        painter->setPen(solidThin);
    }

    // 3) 如果用户设置了自定义参考点（useCustomRef），绘制该点（绿色）
    if (useCustomRef)
    {
        painter->setPen(QPen(Qt::green, 1));
        painter->setBrush(Qt::green);
        painter->drawEllipse(QPointF(referencePoint), 4, 4);
        painter->setBrush(noBrush);
        painter->setPen(Qt::black);
        painter->drawText(referencePoint + QPointF(6, -6), QString("Custom Ref"));
    }

    // 4) 如果 hasPickedRef（上次 pickRef 操作），显示 pickedRefPoint 与标签
    if (hasPickedRef)
    {
        painter->setPen(QPen(Qt::darkGreen, 2));
        painter->setBrush(Qt::darkGreen);
        painter->drawEllipse(pickedRefPoint, 4, 4);
        painter->setBrush(noBrush);

        QString label;
        if (!pickedRefShape.expired() && pickedRefVertexIndex >= 0) {
            label = QString("Ref: vertex %1").arg(pickedRefVertexIndex);
        } else {
            label = QString("Ref: (%1,%2)").arg(int(pickedRefPoint.x())).arg(int(pickedRefPoint.y()));
        }
        painter->setPen(Qt::black);
        painter->drawText(pickedRefPoint + QPointF(6, -6), label);
    }
}

