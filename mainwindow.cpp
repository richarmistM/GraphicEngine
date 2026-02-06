#include "mainwindow.h"
#include "drawengine.h"
#include "canvaswidget.h"
#include "linetool.h"
#include "arctool.h"
#include "polygontool.h"
#include "beziertool.h"
#include "cliptool.h"
#include "selecttool.h"
#include "filltool.h"

#include <QToolBar>
#include <QAction>
#include <QDockWidget>
#include <QLineEdit>
#include <QFormLayout>
#include <QCheckBox>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    canvas(nullptr),
    drawEngine(nullptr),
    currentTool(nullptr),
    lineTool(nullptr),
    arcTool(nullptr),
    polygonTool(nullptr),
    bezierTool(nullptr),
    clipTool(nullptr),
    fillTool(nullptr),
    selectTool(nullptr)
{
    initTools();                                                                // 初始化工具实例
    initUI();                                                                   // 初始化 UI 界面
}

MainWindow::~MainWindow()
{
    // canvas 会在 Qt 的父子机制中自动释放
    delete lineTool;
    delete arcTool;
    delete polygonTool;
    delete bezierTool;
    delete clipTool;
    delete fillTool;
    delete selectTool;
    delete drawEngine;                                                          // DrawEngine 由 MainWindow 统一管理
}


/**
 * @brief 初始化绘图工具
 *
 * 每种工具继承自 BaseTool
 */
void MainWindow::initTools()
{
    // 创建工具实例
    lineTool = new LineTool();
    arcTool = new ArcTool();
    polygonTool = new PolygonTool();
    bezierTool = new BezierTool();
    clipTool = new ClipTool();
    selectTool = new SelectTool();
    fillTool = new FillTool();


    currentTool = lineTool;                                                     // 默认工具
}

/**
 * @brief 初始化主界面布局
 * 包括：
 * 创建 DrawEngine 实例
 * 创建 CanvasWidget 并绑定 DrawEngine
 * 构建工具栏并连接按钮事件
 */
void MainWindow::initUI()
{
    setWindowTitle("2D绘图引擎");
    resize(800, 600);

    drawEngine = new DrawEngine(800, 600);                                      // 初始化绘图引擎
    // 创建画布并绑定当前工具
    canvas = new CanvasWidget(drawEngine, this);
    canvas->setTool(currentTool);
    setCentralWidget(canvas);

    // ------------------- 顶部工具栏 -------------------
    // 创建工具栏
    QToolBar* toolbar = addToolBar("Tools");

    // 添加工具栏按钮
    QAction* lineAction = toolbar->addAction("Line");
    QAction* arcAction = toolbar->addAction("Arc");

    // 信号槽连接：点击按钮 → 切换工具
    connect(lineAction, &QAction::triggered, this, &MainWindow::selectLineTool);
    connect(arcAction, &QAction::triggered, this, &MainWindow::selectArcTool);

    QAction* polyAction = toolbar->addAction("Polygon");
    connect(polyAction, &QAction::triggered, this, [=](){
        currentTool = polygonTool;
        canvas->setTool(currentTool);
    });

    QAction* bezierAction = toolbar->addAction("Bezier");
    connect(bezierAction, &QAction::triggered, this, [=](){
        currentTool = bezierTool;
        canvas->setTool(currentTool);
    });

    QAction* selectAction = toolbar->addAction("Select");
    connect(selectAction, &QAction::triggered, this, &MainWindow::selectSelectTool);

    QAction* clipAction = toolbar->addAction("Clip");
    connect(clipAction, &QAction::triggered, this, [=](){
        currentTool = clipTool;
        canvas->setTool(currentTool);
    });


    // ---------- 填充工具按钮 ----------
    QAction* fillAction = toolbar->addAction("Fill");
    connect(fillAction, &QAction::triggered, this, [=](){
        currentTool = fillTool;
        canvas->setTool(currentTool);
    });

    // ---------- 多边形创建时是否填充（Checkbox） ----------
    fillPolygonsCheckbox = new QCheckBox("Fill polygons", this);
    fillPolygonsCheckbox->setChecked(false);
    toolbar->addWidget(fillPolygonsCheckbox);

    // 连接 Checkbox 到 polygonTool
    connect(fillPolygonsCheckbox, &QCheckBox::toggled, this, [=](bool checked){
        if (polygonTool) polygonTool->setFillOnComplete(checked);
    });


    QAction* clearAction = toolbar->addAction("clear");
    connect(clearAction, &QAction::triggered, this, [=](){
        if (!drawEngine) return;
        drawEngine->clearAllShapes();
        if (canvas) canvas->update();
    });

    // ------------------- 线型 ComboBox -------------------
    QComboBox* lineTypeBox = new QComboBox(this);
    lineTypeBox->addItem("Solid");
    lineTypeBox->addItem("Dash");
    //lineTypeBox->addItem("Dot");
    //lineTypeBox->addItem("DashDot");

    toolbar->addWidget(lineTypeBox);

    connect(lineTypeBox, &QComboBox::currentTextChanged, this, [=](const QString &text){
        if(drawEngine) drawEngine->setLineStyle(text);
    });

    // ------------------- 线帽 ComboBox -------------------
    // QComboBox* lineCapBox = new QComboBox(this);
    // lineCapBox->addItem("Flat");
    // lineCapBox->addItem("Square");
    // lineCapBox->addItem("Round");

    // toolbar->addWidget(lineCapBox);

    // connect(lineCapBox, &QComboBox::currentTextChanged, this, [=](const QString &text){
    //     if(drawEngine) drawEngine->setLineCap(text);
    // });

    // ------------------- 左侧竖直滑动条：线宽 -------------------
    penWidthSlider = new QSlider(Qt::Vertical, this);
    penWidthSlider->setRange(1,20);
    penWidthSlider->setValue(drawEngine->getPenWidth());
    penWidthSlider->setTickPosition(QSlider::TicksRight);
    penWidthSlider->setTickInterval(1);

    QDockWidget* dock = new QDockWidget("Line Width", this);
    dock->setWidget(penWidthSlider);
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    addDockWidget(Qt::LeftDockWidgetArea, dock);

    connect(penWidthSlider, &QSlider::valueChanged, this, [=](int value){
        if(drawEngine)
            drawEngine->setPenWidth(value);                                     // 滑动条改变线宽
    });



    // 变换面板
    transformDock = new QDockWidget("Transform", this);
    QWidget* dockW = new QWidget(transformDock);
    QFormLayout* fl = new QFormLayout(dockW);

    txEdit = new QLineEdit("0"); tyEdit = new QLineEdit("0");
    sxEdit = new QLineEdit("1"); syEdit = new QLineEdit("1");
    angleEdit = new QLineEdit("0");

    refCentroidBtn = new QRadioButton("Centroid"); refCustomBtn = new QRadioButton("Custom point");
    refCentroidBtn->setChecked(true);

    applyTransformBtn = new QPushButton("Apply");

    fl->addRow("Translate X:", txEdit);
    fl->addRow("Translate Y:", tyEdit);
    fl->addRow("Scale X:", sxEdit);
    fl->addRow("Scale Y:", syEdit);
    fl->addRow("Rotate (deg):", angleEdit);
    fl->addRow(refCentroidBtn);
    fl->addRow(refCustomBtn);
    fl->addRow(applyTransformBtn);

    dockW->setLayout(fl);
    transformDock->setWidget(dockW);
    addDockWidget(Qt::RightDockWidgetArea, transformDock);

    // 连接 apply 按钮
    connect(applyTransformBtn, &QPushButton::clicked, this, [=](){
        if (!selectTool || !drawEngine) return;

        double tx = txEdit->text().toDouble();
        double ty = tyEdit->text().toDouble();
        double sx = sxEdit->text().toDouble();
        double sy = syEdit->text().toDouble();
        double angle = angleEdit->text().toDouble();

        // 参考点
        QPointF ref(0,0);
        bool useCustom = refCustomBtn->isChecked();
        selectTool->pickRefMode = useCustom;
        if (useCustom && selectTool && selectTool->isRefPicked()) {
            ref = selectTool->getPickedRefPoint();
        } else {
            const auto &ss = selectTool->getSelection();
            if (ss.empty()) return;
            QPointF sum(0,0);
            for (auto &s : ss) sum += s->centroid();
            ref = QPointF(sum.x() / ss.size(), sum.y() / ss.size());
        }

        selectTool->applyTransformToSelection_params(tx, ty, sx, sy, angle, ref, drawEngine);
    });

}


/**
 * @brief 切换为画线工具
 */
void MainWindow::selectLineTool()
{
    currentTool = lineTool;
    canvas->setTool(currentTool);
}

/**
 * @brief 切换为圆弧工具
 */
void MainWindow::selectArcTool()
{
    currentTool = arcTool;
    canvas->setTool(currentTool);
}

/**
 * @brief 切换为选择工具
 */
void MainWindow::selectSelectTool()
{
    currentTool = selectTool;
    canvas->setTool(currentTool);
}

