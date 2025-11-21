#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QPixmap>
#include <QPainter>
#include <QPen>
#include <QtMath>
#include <QApplication>
#include <QKeyEvent>
#include <QTimer>
#include <climits>
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QPixmap pm(ui->frame->width(), ui->frame->height());
    pm.fill(bgColor);
    ui->frame->setPixmap(pm);

    connect(ui->frame, SIGNAL(Mouse_Pos()), this, SLOT(Mouse_Pressed()));
    connect(ui->frame, SIGNAL(sendMousePosition(QPoint&)), this, SLOT(showMousePosition(QPoint&)));

    cellSize = ui->spinGridSize->value();
    ui->lblHover->setText("Hover: X: 0, Y: 0");
    ui->lblInfo->setText("Click to add vertices. Close polygon to freeze 'original'.");
    updateOriginalUiState();

    qApp->installEventFilter(this);
    nudgeTimer = new QTimer(this);
    nudgeTimer->setSingleShot(false);
    nudgeTimer->setInterval(80);
    connect(nudgeTimer, &QTimer::timeout, this, &MainWindow::performNudge);
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event)
{
    Q_UNUSED(obj);
    if (event->type() == QEvent::KeyPress) {
        auto* e = static_cast<QKeyEvent*>(event);
        if (e->isAutoRepeat()) return true;
        const int k = e->key();
        if (k==Qt::Key_W || k==Qt::Key_A || k==Qt::Key_S || k==Qt::Key_D) {
            pressedKeys.insert(k);
            scheduleNudge();
            return true;
        }
    } else if (event->type() == QEvent::KeyRelease) {
        auto* e = static_cast<QKeyEvent*>(event);
        if (e->isAutoRepeat()) return true;
        const int k = e->key();
        if (k==Qt::Key_W || k==Qt::Key_A || k==Qt::Key_S || k==Qt::Key_D) {
            pressedKeys.remove(k);
            if (pressedKeys.isEmpty())
                nudgeTimer->stop();
            return true;
        }
    }
    return false;
}

void MainWindow::scheduleNudge()
{
    performNudge();
    if (!nudgeTimer->isActive())
        nudgeTimer->start();
}

void MainWindow::performNudge()
{
    const int dx = (pressedKeys.contains(Qt::Key_D) ? 1 : 0)
    - (pressedKeys.contains(Qt::Key_A) ? 1 : 0);
    const int dy = (pressedKeys.contains(Qt::Key_W) ? 1 : 0)
                   - (pressedKeys.contains(Qt::Key_S) ? 1 : 0);

    if (dx == 0 && dy == 0) {
        return;
    }
    translateByCells(dx, dy);
}

void MainWindow::showMousePosition(QPoint &pos)
{
    scrX = pos.x();
    scrY = pos.y();
    if (!gridDrawn || cellSize <= 0) {
        ui->lblHover->setText("Hover: X: –, Y: –");
        return;
    }
    QPoint g = screenPxToGridXY(scrX, scrY);
    ui->lblHover->setText(QString("Hover: X: %1, Y: %2").arg(g.x()).arg(g.y()));
}

void MainWindow::Mouse_Pressed()
{
    if (!gridDrawn) repaintFreshGrid();

    QPoint g = screenPxToGridXY(scrX, scrY);

    if (hasValid(lastPick)) prevPick = lastPick;
    lastPick = g;

    if (!polygonClosed) {
        polygon.push_back(g);
        ui->lblInfo->setText(QString("Vertices: %1 | Last pick: (%2,%3)")
                                 .arg(polygon.size()).arg(g.x()).arg(g.y()));
    } else {
        ui->lblInfo->setText(QString("Last pick: (%1,%2)").arg(g.x()).arg(g.y()));
    }

    fillPickedCellPx(scrX, scrY);
}

void MainWindow::on_btnDrawGrid_clicked()
{
    repaintFreshGrid();
    polygon.clear();
    originalPolygon.clear();
    hasOriginal   = false;
    polygonClosed = false;

    lastPick = prevPick = QPoint(INT_MIN, INT_MIN);
    hasDrawnLine = false;

    updateOriginalUiState();
    ui->lblInfo->setText("Grid ready. Click to add vertices.");
}

void MainWindow::on_spinGridSize_valueChanged(int value)
{
    if (value < 5) return;
    cellSize = value;
    refreshScene();
}

void MainWindow::on_btnClear_clicked()
{
    polygon.clear();
    originalPolygon.clear();
    hasOriginal   = false;
    polygonClosed = false;

    lastPick = prevPick = QPoint(INT_MIN, INT_MIN);
    hasDrawnLine = false;

    refreshScene();
    updateOriginalUiState();
    ui->lblInfo->setText("Cleared. Click to add vertices.");
}

void MainWindow::repaintFreshGrid()
{
    QPixmap pm(ui->frame->width(), ui->frame->height());
    pm.fill(bgColor);

    gridCols = qMax(1, pm.width()  / cellSize);
    gridRows = qMax(1, pm.height() / cellSize);
    centerX  = gridCols / 2;
    centerY  = gridRows / 2;

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, false);

    QPen gridPen(gridColor);
    gridPen.setWidth(1);
    p.setPen(gridPen);
    for (int c = 0; c <= gridCols; ++c)
        p.drawLine(c * cellSize, 0, c * cellSize, gridRows * cellSize);
    for (int r = 0; r <= gridRows; ++r)
        p.drawLine(0, r * cellSize, gridCols * cellSize, r * cellSize);

    drawAxes(p);

    p.end();
    ui->frame->setPixmap(pm);
    gridDrawn = true;
}

void MainWindow::drawAxes(QPainter& p)
{
    p.setPen(Qt::NoPen);
    p.setBrush(axisColor);
    for (int r = 0; r < gridRows; ++r)
        p.fillRect(QRect(centerX * cellSize, r * cellSize, cellSize, cellSize), axisColor);
    for (int c = 0; c < gridCols; ++c)
        p.fillRect(QRect(c * cellSize, centerY * cellSize, cellSize, cellSize), axisColor);
}

void MainWindow::refreshScene()
{
    repaintFreshGrid();

    QPixmap pm = ui->frame->pixmap();
    if (pm.isNull()) return;

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, false);

    drawHelperLine(p);
    drawPolygon(p);

    p.end();
    ui->frame->setPixmap(pm);
}

void MainWindow::fillPickedCellPx(int px, int py)
{
    QPixmap pm = ui->frame->pixmap();
    if (pm.isNull() || cellSize <= 0) return;
    QPainter painter(&pm);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(Qt::NoPen);

    const int cx = (px / cellSize) * cellSize;
    const int cy = (py / cellSize) * cellSize;
    painter.fillRect(QRect(cx, cy, cellSize, cellSize), pickFill);
    painter.end();
    ui->frame->setPixmap(pm);
}

QPoint MainWindow::screenPxToGridXY(int px, int py) const
{
    const int gx = px / cellSize;
    const int gy = py / cellSize;
    const int X  = gx - centerX;
    const int Y  = -(gy - centerY);
    return QPoint(X, Y);
}

QPoint MainWindow::gridXYToCellTL(int X, int Y) const
{
    const int gx = X + centerX;
    const int gy = centerY - Y;
    return QPoint(gx * cellSize, gy * cellSize);
}

inline void MainWindow::fillCellGrid(int gx, int gy, QPainter& p)
{
    if (gx >= 0 && gx < gridCols && gy >= 0 && gy < gridRows)
        p.fillRect(QRect(gx * cellSize, gy * cellSize, cellSize, cellSize), p.brush());
}

void MainWindow::rasterBresenham(int X1, int Y1, int X2, int Y2, QVector<QPoint>& out)
{
    out.clear();
    int gx1 = X1 + centerX;
    int gy1 = centerY - Y1;
    int gx2 = X2 + centerX;
    int gy2 = centerY - Y2;

    int dx = std::abs(gx2 - gx1);
    int dy = std::abs(gy2 - gy1);
    int sx = (gx1 < gx2) ? 1 : -1;
    int sy = (gy1 < gy2) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        out.push_back(QPoint(gx1, gy1));
        if (gx1 == gx2 && gy1 == gy2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; gx1 += sx; }
        if (e2 <  dx) { err += dx; gy1 += sy; }
    }
}

void MainWindow::drawPoly(const QVector<QPoint>& poly, QPainter& p, const QColor& color)
{
    if (poly.isEmpty()) return;
    p.setPen(Qt::NoPen);
    p.setBrush(color);

    for (const QPoint& v : poly) {
        const int gx = v.x() + centerX;
        const int gy = centerY - v.y();
        fillCellGrid(gx, gy, p);
    }

    QVector<QPoint> cells;
    for (int i = 0; i + 1 < poly.size(); ++i) {
        rasterBresenham(poly[i].x(), poly[i].y(),
                        poly[i+1].x(), poly[i+1].y(), cells);
        for (const QPoint& c : cells)
            fillCellGrid(c.x(), c.y(), p);
    }
}

void MainWindow::drawHelperLine(QPainter& p)
{
    if (!hasDrawnLine) return;
    p.setPen(Qt::NoPen);
    p.setBrush(lineColor);

    QVector<QPoint> cells;
    rasterBresenham(lineP1.x(), lineP1.y(), lineP2.x(), lineP2.y(), cells);
    for (const QPoint& c : cells)
        fillCellGrid(c.x(), c.y(), p);
}

void MainWindow::drawPolygon(QPainter& p)
{
    if (hasOriginal)
        drawPoly(originalPolygon, p, origColor);
    drawPoly(polygon, p, polyColor);
}

void MainWindow::on_btnClosePolygon_clicked()
{
    if (polygon.size() >= 3 && polygon.front() != polygon.back())
        polygon.push_back(polygon.front());

    if (!hasOriginal && polygon.size() >= 4) {
        originalPolygon = polygon;
        hasOriginal     = true;
        polygonClosed   = true;
        ui->lblInfo->setText("Original frozen. You can transform the working polygon.");
        updateOriginalUiState();
    } else {
        polygonClosed = true;
    }
    refreshScene();
}

void MainWindow::on_btnRevertOriginal_clicked()
{
    if (!hasOriginal) return;
    polygon       = originalPolygon;
    polygonClosed = true;

    hasDrawnLine = false;

    refreshScene();
    ui->lblInfo->setText("Reverted to original polygon.");
}

void MainWindow::updateOriginalUiState()
{
    ui->btnRevertOriginal->setEnabled(hasOriginal);
}

QPointF MainWindow::firstVertexPivot() const
{
    if (!originalPolygon.isEmpty())
        return QPointF(originalPolygon.front());
    if (!polygon.isEmpty())
        return QPointF(polygon.front());
    return QPointF(0,0);
}

QPointF MainWindow::polygonCentroid(const QVector<QPoint>& poly) const
{
    const int m = poly.size();
    if (m == 0) return QPointF(0,0);

    int n = m;
    if (n > 1 && poly.front() == poly.back()) --n;

    double A = 0.0, Cx = 0.0, Cy = 0.0;
    for (int i = 0; i < n; ++i) {
        const QPoint& p0 = poly[i];
        const QPoint& p1 = poly[(i + 1) % n];
        const double cross = double(p0.x()) * p1.y() - double(p1.x()) * p0.y();
        A  += cross;
        Cx += (p0.x() + p1.x()) * cross;
        Cy += (p0.y() + p1.y()) * cross;
    }

    if (qAbs(A) > 1e-9) {
        A *= 0.5;
        Cx /= (6.0 * A);
        Cy /= (6.0 * A);
        return QPointF(Cx, Cy);
    }

    int minx = poly[0].x(), maxx = poly[0].x();
    int miny = poly[0].y(), maxy = poly[0].y();
    for (int i = 1; i < n; ++i) {
        minx = qMin(minx, poly[i].x());
        maxx = qMax(maxx, poly[i].x());
        miny = qMin(miny, poly[i].y());
        maxy = qMax(maxy, poly[i].y());
    }
    return QPointF(0.5 * (minx + maxx), 0.5 * (miny + maxy));
}

QPointF MainWindow::currentPivot() const
{
    if (!polygon.isEmpty())         return polygonCentroid(polygon);
    if (!originalPolygon.isEmpty()) return polygonCentroid(originalPolygon);
    return QPointF(0,0);
}

double MainWindow::effectiveScale(double s) const
{
    if (s < 0.0) return 1.0 / (1.0 + (-s));
    return s;
}

void MainWindow::applyTransform(const QTransform& T)
{
    if (polygon.isEmpty()) return;

    QVector<QPoint> out;
    out.reserve(polygon.size());

    for (const QPoint& v : polygon) {
        QPointF r = T.map(QPointF(v.x(), v.y()));
        out.push_back(QPoint(qRound(r.x()), qRound(r.y())));
    }
    polygon = out;
    refreshScene();
}

void MainWindow::on_btnTranslate_clicked()
{
    QTransform T;
    T.translate(ui->spinTx->value(), ui->spinTy->value());
    applyTransform(T);
}

void MainWindow::on_btnRotate_clicked()
{
    QTransform T;
    T.rotate(-ui->spinAngle->value());
    applyTransform(T);
}

void MainWindow::on_btnRotateAboutPoint_clicked()
{
    if (!hasValid(lastPick)) {
        ui->lblInfo->setText("Pick a pivot on the grid (click) and try again.");
        return;
    }
    const double deg = -ui->spinAngle->value();

    QTransform T;
    T.translate(lastPick.x(), lastPick.y());
    T.rotate(deg);
    T.translate(-lastPick.x(), -lastPick.y());
    applyTransform(T);

    ui->lblInfo->setText(QString("Rotated about (%1,%2)").arg(lastPick.x()).arg(lastPick.y()));
}

void MainWindow::on_btnScale_clicked()
{
    const QPointF c  = currentPivot();
    const double sxI = ui->spinSx->value();
    const double syI = ui->spinSy->value();
    const double sx  = effectiveScale(sxI);
    const double sy  = effectiveScale(syI);

    QTransform T;
    T.translate(c.x(), c.y());
    T.scale(sx, sy);
    T.translate(-c.x(), -c.y());
    applyTransform(T);

    ui->lblInfo->setText(
        QString("Scaled about centroid (%1,%2) | input(Sx,Sy)=(%3,%4) → effective=(%5,%6)")
            .arg(c.x()).arg(c.y())
            .arg(sxI,0,'g',4).arg(syI,0,'g',4)
            .arg(sx,0,'g',4).arg(sy,0,'g',4));
}

void MainWindow::on_btnShear_clicked()
{
    const QPointF c  = currentPivot();
    const double shx = ui->spinShx->value();
    const double shy = ui->spinShy->value();

    QTransform T;
    T.translate(c.x(), c.y());
    T.shear(shx, shy);
    T.translate(-c.x(), -c.y());
    applyTransform(T);

    ui->lblInfo->setText(QString("Sheared about centroid (%1,%2)").arg(c.x()).arg(c.y()));
}

void MainWindow::on_btnReflectX_clicked()
{
    QTransform T(1, 0, 0, -1, 0, 0);
    applyTransform(T);
}

void MainWindow::on_btnReflectY_clicked()
{
    QTransform T(-1, 0, 0, 1, 0, 0);
    applyTransform(T);
}

QPointF MainWindow::reflectPointAboutLine(const QPointF& v,
                                          const QPointF& a,
                                          const QPointF& b) const
{
    double ux = b.x() - a.x();
    double uy = b.y() - a.y();
    const double len2 = ux*ux + uy*uy;
    if (len2 < 1e-12) return v;

    const double invLen = 1.0 / std::sqrt(len2);
    ux *= invLen;  uy *= invLen;

    const double r11 = 2.0*ux*ux - 1.0;
    const double r12 = 2.0*ux*uy;
    const double r21 = r12;
    const double r22 = 2.0*uy*uy - 1.0;

    const double wx = v.x() - a.x();
    const double wy = v.y() - a.y();

    const double rx = r11*wx + r12*wy;
    const double ry = r21*wx + r22*wy;

    return QPointF(a.x() + rx, a.y() + ry);
}

bool MainWindow::reflectSpecialIfSlopePM1(const QPoint& a, const QPoint& b,
                                          QVector<QPoint>& out) const
{
    const int dx = b.x() - a.x();
    const int dy = b.y() - a.y();
    if (dx == 0 && dy == 0) return false;

    if (dy == dx) {
        const int k = a.y() - a.x();
        out.clear(); out.reserve(polygon.size());
        for (const QPoint& v : polygon) {
            const int xp = v.y() - k;
            const int yp = v.x() + k;
            out.push_back(QPoint(xp, yp));
        }
        return true;
    }

    if (dy == -dx) {
        const int k = a.y() + a.x();
        out.clear(); out.reserve(polygon.size());
        for (const QPoint& v : polygon) {
            const int xp = -v.y() + k;
            const int yp = -v.x() + k;
            out.push_back(QPoint(xp, yp));
        }
        return true;
    }

    return false;
}

void MainWindow::on_btnDrawLine_clicked()
{
    if (!hasValid(prevPick) || !hasValid(lastPick)) {
        ui->lblInfo->setText("Select two points (click twice) to define the line.");
        return;
    }
    lineP1 = prevPick;
    lineP2 = lastPick;
    hasDrawnLine = true;
    refreshScene();
    ui->lblInfo->setText(QString("Helper line set: (%1,%2) → (%3,%4)")
                             .arg(lineP1.x()).arg(lineP1.y())
                             .arg(lineP2.x()).arg(lineP2.y()));
}

void MainWindow::on_btnReflectDrawnLine_clicked()
{
    if (!hasDrawnLine) {
        ui->lblInfo->setText("No helper line. Click two points and press 'Draw Line' first.");
        return;
    }

    const QPoint a = lineP1;
    const QPoint b = lineP2;

    QVector<QPoint> out;

    if (reflectSpecialIfSlopePM1(a, b, out)) {
        polygon = out;
        refreshScene();
        ui->lblInfo->setText("Reflected about line with slope ±1 (exact swap).");
        return;
    }

    out.clear(); out.reserve(polygon.size());
    for (const QPoint& v : polygon) {
        const QPointF r = reflectPointAboutLine(QPointF(v), QPointF(a), QPointF(b));
        out.push_back(QPoint(qRound(r.x()), qRound(r.y())));
    }
    polygon = out;
    refreshScene();
}

QTransform MainWindow::reflectAboutTwoPointLine(const QPointF& p1, const QPointF& p2)
{
    const double dx = p2.x() - p1.x();
    const double dy = p2.y() - p1.y();
    const double len2 = dx*dx + dy*dy;
    if (len2 < 1e-9) return QTransform();

    const double thetaRad = std::atan2(dy, dx);

    QTransform A; A.translate(-p1.x(), -p1.y());
    QTransform B; B.rotateRadians(-thetaRad);
    QTransform C(1, 0, 0, -1, 0, 0);
    QTransform D; D.rotateRadians(thetaRad);
    QTransform E; E.translate(p1.x(), p1.y());
    return E * D * C * B * A;
}

void MainWindow::translateByCells(int dx, int dy)
{
    if (polygon.isEmpty()) return;
    QTransform T;
    T.translate(dx, dy);
    applyTransform(T);
    ui->lblInfo->setText(QString("Nudged by (%1, %2)").arg(dx).arg(dy));
}

void MainWindow::nudgeUp()    { translateByCells(0, +1); }
void MainWindow::nudgeLeft()  { translateByCells(-1, 0); }
void MainWindow::nudgeDown()  { translateByCells(0, -1); }
void MainWindow::nudgeRight() { translateByCells(+1, 0); }
