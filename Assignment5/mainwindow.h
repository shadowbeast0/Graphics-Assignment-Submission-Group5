#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QPoint>
#include <QColor>
#include <QTransform>
#include <QSet>
#include <QTimer>
#include <climits>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void Mouse_Pressed();
    void showMousePosition(QPoint &pos);

    void on_btnDrawGrid_clicked();
    void on_spinGridSize_valueChanged(int value);
    void on_btnClear_clicked();

    void on_btnClosePolygon_clicked();

    void on_btnRevertOriginal_clicked();

    void on_btnTranslate_clicked();
    void on_btnRotate_clicked();
    void on_btnRotateAboutPoint_clicked();
    void on_btnScale_clicked();
    void on_btnShear_clicked();
    void on_btnReflectX_clicked();
    void on_btnReflectY_clicked();
    void on_btnDrawLine_clicked();
    void on_btnReflectDrawnLine_clicked();

    void nudgeUp();
    void nudgeLeft();
    void nudgeDown();
    void nudgeRight();

private:
    Ui::MainWindow *ui = nullptr;

    int  cellSize  = 20;
    int  gridCols  = 0, gridRows = 0;
    int  centerX   = 0, centerY = 0;
    bool gridDrawn = false;

    QColor bgColor    = QColor(11,21,64);
    QColor gridColor  = QColor(240,240,240);
    QColor axisColor  = QColor(255,255,255);
    QColor polyColor  = QColor(255,208,96);
    QColor origColor  = QColor(140,255,200);
    QColor pickFill   = QColor(255,105,180);
    QColor lineColor  = QColor(80,220,255);

    int    scrX = 0, scrY = 0;
    QPoint lastPick = QPoint(INT_MIN, INT_MIN);
    QPoint prevPick = QPoint(INT_MIN, INT_MIN);

    QVector<QPoint> polygon;
    QVector<QPoint> originalPolygon;
    bool hasOriginal   = false;
    bool polygonClosed = false;

    bool   hasDrawnLine = false;
    QPoint lineP1, lineP2;

    void repaintFreshGrid();
    void drawAxes(class QPainter& p);
    void refreshScene();
    void fillPickedCellPx(int px, int py);

    QPoint screenPxToGridXY(int px, int py) const;
    QPoint gridXYToCellTL(int X, int Y) const;

    void drawPolygon(class QPainter& p);
    void drawPoly(const QVector<QPoint>& poly, class QPainter& p, const QColor& color);
    void drawHelperLine(class QPainter& p);
    void rasterBresenham(int X1, int Y1, int X2, int Y2, QVector<QPoint>& outCells);
    inline void fillCellGrid(int gx, int gy, class QPainter& p);

    void applyTransform(const QTransform& T);

    QTransform reflectAboutTwoPointLine(const QPointF& p1, const QPointF& p2);


    bool hasValid(const QPoint& p) const { return p.x()!=INT_MIN && p.y()!=INT_MIN; }
    QPointF firstVertexPivot() const;
    void updateOriginalUiState();

    QPointF polygonCentroid(const QVector<QPoint>& poly) const;
    QPointF currentPivot() const;


    QPointF reflectPointAboutLine(const QPointF& v,
                                  const QPointF& a,
                                  const QPointF& b) const;
    bool reflectSpecialIfSlopePM1(const QPoint& a, const QPoint& b,
                                  QVector<QPoint>& out) const;

    double effectiveScale(double s) const;

    void translateByCells(int dx, int dy);

    QSet<int> pressedKeys;
    QTimer*   nudgeTimer = nullptr;
    void scheduleNudge();
    void performNudge();
};

#endif
